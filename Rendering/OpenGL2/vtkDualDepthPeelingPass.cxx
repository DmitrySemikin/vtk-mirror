/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDualDepthPeelingPass.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkDualDepthPeelingPass.h"

#include "vtkFrameBufferObject2.h"
#include "vtkInformation.h"
#include "vtkInformationKey.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLActor.h"
#include "vtkOpenGLBufferObject.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLFenceSync.h"
#include "vtkOpenGLOcclusionQueryQueue.h"
#include "vtkOpenGLRenderUtilities.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLShaderCache.h"
#include "vtkOpenGLVertexArrayObject.h"
#include "vtkPixelBufferObject.h"
#include "vtkRenderer.h"
#include "vtkRenderState.h"
#include "vtkShaderProgram.h"
#include "vtkTextureObject.h"
#include "vtkTimerLog.h"
#include "vtkTypeTraits.h"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <string>

// Define to print debug statements to the OpenGL CS stream (useful for e.g.
// apitrace debugging):
//#define ANNOTATE_STREAM

// Define to add start/end events to the vtkTimerLog.
//#define TIMER_LOG

// Define to output details about each peel:
//#define DEBUG_PEEL

// Define to output details about each frame:
//#define DEBUG_FRAME

// Define to debug fragment counting:
//#define DEBUG_FRAGMENTCOUNT

vtkStandardNewMacro(vtkDualDepthPeelingPass)

namespace {

#ifdef ANNOTATE_STREAM
// Write an entry to the OpenGL debug stream. This is handy for generating
// apitrace logs to make it easier to identify what stage the rendering is in.
void annotate(const std::string &str)
{
  vtkOpenGLStaticCheckErrorMacro("Error before glDebug.")
  glDebugMessageInsert(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_OTHER,
                       GL_DEBUG_SEVERITY_NOTIFICATION,
                       0, str.size(), str.c_str());
  vtkOpenGLClearErrorMacro();
}
#endif // ANNOTATE_STREAM

// Called at the start of an 'event'. If TIMER_LOG is defined, a timer event
// is started. If ANNOTATE_STREAM is defined, a message is written to the
// OpenGL debug log.
void startEvent(const std::string &str)
{
  (void)str; // Prevent unused variable warnings
#ifdef ANNOTATE_STREAM
  annotate(std::string("Start event: ") + str);
#endif // ANNOTATE_STREAM
#ifdef TIMER_LOG
  vtkTimerLog::MarkStartEvent(str.c_str());
#endif // TIMER_LOG
}

// Called at the end of an 'event'. If TIMER_LOG is defined, the previously
// start timer event is ended. If ANNOTATE_STREAM is defined, a message is
// written to the OpenGL debug log.
void endEvent(const std::string &str)
{
  (void)str; // Prevent unused variable warnings
#ifdef TIMER_LOG
  vtkTimerLog::MarkEndEvent(str.c_str());
#endif // TIMER_LOG
#ifdef ANNOTATE_STREAM
  annotate(std::string("End event: ") + str);
#endif // ANNOTATE_STREAM
}

// RAII-ish object for ensuring that events are closed from functions that may
// have multiple return points. Calls startEvent when constructed, and endEvent
// when destroyed.
struct EventMarker
{
  EventMarker(const std::string &str) : Event(str) { startEvent(this->Event); }
  ~EventMarker() { endEvent(this->Event); }
  std::string Event;
};

} // end anon namespace

//------------------------------------------------------------------------------
void vtkDualDepthPeelingPass::PrintSelf(std::ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void vtkDualDepthPeelingPass::Render(const vtkRenderState *s)
{
  EventMarker marker("vtkDDP::Render");

  // Setup vtkOpenGLRenderPass
  this->PreRender(s);

  this->Initialize(s);
  this->Prepare();

  while (!this->PeelingDone())
    {
    this->Peel();
    }

  this->Finalize();

  this->PostRender(s);
}

//------------------------------------------------------------------------------
void vtkDualDepthPeelingPass::ReleaseGraphicsResources(vtkWindow *)
{
  this->FreeGLObjects();
}

//------------------------------------------------------------------------------
bool vtkDualDepthPeelingPass::ReplaceShaderValues(std::string &,
                                                  std::string &,
                                                  std::string &fragmentShader,
                                                  vtkAbstractMapper *,
                                                  vtkProp *)
{
  switch (this->CurrentStage)
    {
    case vtkDualDepthPeelingPass::InitializingDepth:
      vtkShaderProgram::Substitute(
            fragmentShader, "//VTK::DepthPeeling::Dec",
            "uniform sampler2D opaqueDepth;\n");
      vtkShaderProgram::Substitute(
            fragmentShader, "//VTK::DepthPeeling::PreColor",
            "ivec2 pixel = ivec2(gl_FragCoord.xy);\n"
            "  float oDepth = texelFetch(opaqueDepth, pixel, 0).y;\n"
            "  if (oDepth != -1. && gl_FragCoord.z > oDepth)\n"
            "    { // Discard fragments that are occluded by opaque geometry:\n"
            "    discard;\n"
            "    }\n"
            "  else\n"
            "    {\n"
            "    gl_FragData[1].xy = vec2(-gl_FragCoord.z, gl_FragCoord.z);\n"
            "    return;\n"
            "    }\n"
            );
      break;

    case vtkDualDepthPeelingPass::Peeling:
      vtkShaderProgram::Substitute(
            fragmentShader, "//VTK::DepthPeeling::Dec",
            "uniform sampler2D lastFrontPeel;\n"
            "uniform sampler2D lastDepthPeel;\n");
      vtkShaderProgram::Substitute(
            fragmentShader, "//VTK::DepthPeeling::PreColor",
            "float depth = gl_FragCoord.z;\n"
            "  ivec2 pixelCoord = ivec2(gl_FragCoord.xy);\n"
            "  vec4 front = texelFetch(lastFrontPeel, pixelCoord, 0);\n"
            "  vec2 minMaxDepth = texelFetch(lastDepthPeel, pixelCoord, 0).xy;\n"
            "  float minDepth = -minMaxDepth.x;\n"
            "  float maxDepth = minMaxDepth.y;\n"
            "\n"
            "  // Default outputs (no data/change):\n"
            "  gl_FragData[0] = vec4(0.);\n"
            "  gl_FragData[1] = front;\n"
            "  gl_FragData[2].xy = vec2(-1.);\n"
            "\n"
            "  // Is this fragment outside the current peels?\n"
            "  if (depth < minDepth || depth > maxDepth)\n"
            "    {\n"
            "    return;\n"
            "    }\n"
            "\n"
            "  // Is this fragment inside the current peels?\n"
            "  if (depth > minDepth && depth < maxDepth)\n"
            "    {\n"
            "    // Write out depth so this frag will be peeled later:\n"
            "    gl_FragData[2].xy = vec2(-depth, depth);\n"
            "    return;\n"
            "    }\n"
            "\n"
            "  // Continue processing for fragments on the current peel:\n"
            );
      vtkShaderProgram::Substitute(
            fragmentShader, "//VTK::DepthPeeling::Impl",
            "vec4 frag = gl_FragData[0];\n"
            "  // Default outputs (no data/change):\n"
            "\n"
            "  // This fragment is on a current peel:\n"
            "  // Write to the back buffer if min=max (e.g. only a single\n"
            "  // fragment to peel). This ensures that occlusion queries\n"
            "  // are accurate.\n"
            "  if (depth == maxDepth)\n"
            "    { // Back peel:\n"
            "    // Dump premultiplied fragment, it will be blended later:\n"
            "    frag.rgb *= frag.a;\n"
            "    gl_FragData[0] = frag;\n"
            "    return;\n"
            "    }\n"
            "  else\n"
            "    { // Front peel:\n"
            "    // Clear the back color:\n"
            "    gl_FragData[0] = vec4(0.);\n"
            "\n"
            "    // We store the front alpha value as (1-alpha) to allow MAX\n"
            "    // blending. This also means it is really initialized to 1,\n"
            "    // as it should be for under-blending.\n"
            "    front.a = 1. - front.a;\n"
            "\n"
            "    // Use under-blending to combine fragment with front color:\n"
            "    gl_FragData[1].rgb = front.a * frag.a * frag.rgb + front.rgb;\n"
            "    // Write out (1-alpha):\n"
            "    gl_FragData[1].a = 1. - (front.a * (1. - frag.a));\n"
            "    return;\n"
            "    }\n"
            );
      break;

    case vtkDualDepthPeelingPass::AlphaBlending:
      vtkShaderProgram::Substitute(
            fragmentShader, "//VTK::DepthPeeling::Dec",
            "uniform sampler2D lastDepthPeel;\n");
      vtkShaderProgram::Substitute(
            fragmentShader, "//VTK::DepthPeeling::PreColor",
            "float depth = gl_FragCoord.z;\n"
            "  ivec2 pixelCoord = ivec2(gl_FragCoord.xy);\n"
            "  vec2 minMaxDepth = texelFetch(lastDepthPeel, pixelCoord, 0).xy;\n"
            "  float minDepth = -minMaxDepth.x;\n"
            "  float maxDepth = minMaxDepth.y;\n"
            "\n"
            "  // Discard all fragments outside of the last set of peels:\n"
            "  if (depth < minDepth || depth > maxDepth)\n"
            "    {\n"
            "    discard;\n"
            "    }\n"
            );
      vtkShaderProgram::Substitute(
            fragmentShader, "//VTK::DepthPeeling::Impl",
            "\n"
            "  // Pre-multiply alpha for depth peeling:\n"
            "  gl_FragData[0].rgb *= gl_FragData[0].a;\n"
            );
      break;

    default:
      break;
    }

  return true;
}

//------------------------------------------------------------------------------
bool vtkDualDepthPeelingPass::SetShaderParameters(vtkShaderProgram *program,
                                                  vtkAbstractMapper *,
                                                  vtkProp *)
{
  switch (this->CurrentStage)
    {
    case vtkDualDepthPeelingPass::InitializingDepth:
      program->SetUniformi(
            "opaqueDepth",
            this->Textures[this->DepthDestination]->GetTextureUnit());
      break;

    case vtkDualDepthPeelingPass::Peeling:
      program->SetUniformi(
            "lastDepthPeel",
            this->Textures[this->DepthSource]->GetTextureUnit());
      program->SetUniformi(
            "frontDepthPeel",
            this->Textures[this->FrontSource]->GetTextureUnit());
      break;

    case vtkDualDepthPeelingPass::AlphaBlending:
      program->SetUniformi(
            "lastDepthPeel",
            this->Textures[this->DepthSource]->GetTextureUnit());
      break;

    default:
      break;
    }

  return true;
}

//------------------------------------------------------------------------------
unsigned long vtkDualDepthPeelingPass::GetShaderStageMTime()
{
  return this->CurrentStageTimeStamp.GetMTime();
}

//------------------------------------------------------------------------------
vtkDualDepthPeelingPass::vtkDualDepthPeelingPass()
  : RenderState(NULL),
    CopyDepthProgram(NULL),
    CopyDepthVAO(NULL),
    BackBlendProgram(NULL),
    BackBlendVAO(NULL),
    BlendProgram(NULL),
    BlendVAO(NULL),
    FragmentCountFB(NULL),
    FragmentCountTransfer(NULL),
    FragmentCountFence(NULL),
    Framebuffer(NULL),
    FrontSource(FrontA),
    FrontDestination(FrontB),
    DepthSource(DepthA),
    DepthDestination(DepthB),
    CurrentStage(Inactive),
    LastFramePassCount(5), // Will change for subsequent rendering...
    DepthComplexity(-1),
    DepthComplexityPasses(-1),
    CurrentPeel(0),
    WrittenPixels(0),
    OcclusionThreshold(0),
    RenderCount(0)
{
  std::fill(this->Textures, this->Textures + static_cast<int>(NumberOfTextures),
            static_cast<vtkTextureObject*>(NULL));
}

//------------------------------------------------------------------------------
vtkDualDepthPeelingPass::~vtkDualDepthPeelingPass()
{
  this->FreeGLObjects();
}

//------------------------------------------------------------------------------
void vtkDualDepthPeelingPass::SetCurrentStage(ShaderStage stage)
{
  if (stage != this->CurrentStage)
    {
    this->CurrentStage = stage;
    this->CurrentStageTimeStamp.Modified();
    }
}

//------------------------------------------------------------------------------
// Delete the vtkObject subclass pointed at by ptr if it is set.
namespace {
template <typename T> void DeleteHelper(T *& ptr)
{
  if (ptr)
    {
    ptr->Delete();
    ptr = NULL;
    }
}
} // end anon namespace

//------------------------------------------------------------------------------
void vtkDualDepthPeelingPass::FreeGLObjects()
{
  // don't delete the shader programs -- let the cache clean them up.

  if (this->Framebuffer)
    {
    this->Framebuffer->Delete();
    this->Framebuffer = NULL;

    for (int i = 0; i < static_cast<int>(NumberOfTextures); ++i)
      {
      this->Textures[i]->Delete();
      this->Textures[i] = NULL;
      }
    }

  DeleteHelper(this->CopyDepthVAO);
  DeleteHelper(this->BackBlendVAO);
  DeleteHelper(this->BlendVAO);
  DeleteHelper(this->FragmentCountFB);
  DeleteHelper(this->FragmentCountTransfer);
  DeleteHelper(this->FragmentCountFence);

  this->QueryQueue->Reset();
}

//------------------------------------------------------------------------------
void vtkDualDepthPeelingPass::RenderTranslucentPass()
{
  this->TranslucentPass->Render(this->RenderState);
  ++this->RenderCount;
}

//------------------------------------------------------------------------------
void vtkDualDepthPeelingPass::Initialize(const vtkRenderState *s)
{
  EventMarker marker("vtkDDP::Initialize");
  this->RenderState = s;

  // Get current viewport size:
  vtkRenderer *r=s->GetRenderer();
  if(s->GetFrameBuffer()==0)
    {
    // get the viewport dimensions
    r->GetTiledSizeAndOrigin(&this->ViewportWidth, &this->ViewportHeight,
                             &this->ViewportX, &this->ViewportY);
    }
  else
    {
    int size[2];
    s->GetWindowSize(size);
    this->ViewportWidth = size[0];
    this->ViewportHeight = size[1];
    this->ViewportX =0 ;
    this->ViewportY = 0;
    }

  // See if we can reuse existing textures:
  if (this->Textures[Back] &&
      (static_cast<int>(this->Textures[Back]->GetHeight()) !=
       this->ViewportHeight ||
       static_cast<int>(this->Textures[Back]->GetWidth()) !=
       this->ViewportWidth))
    {
    this->FreeGLObjects();
    }

  // Allocate new textures if needed:
  if (!this->Framebuffer)
    {
    this->Framebuffer = vtkFrameBufferObject2::New();

    std::generate(this->Textures,
                  this->Textures + static_cast<int>(NumberOfTextures),
                  &vtkTextureObject::New);

    this->InitColorTexture(this->Textures[BackTemp], s);
    this->InitColorTexture(this->Textures[Back], s);
    this->InitColorTexture(this->Textures[FrontA], s);
    this->InitColorTexture(this->Textures[FrontB], s);
    this->InitDepthTexture(this->Textures[DepthA], s);
    this->InitDepthTexture(this->Textures[DepthB], s);
    this->InitOpaqueDepthTexture(this->Textures[OpaqueDepth], s);
    this->InitFragmentCountTexture(this->Textures[FragmentCount], s);

    this->InitFramebuffer(s);
    this->InitFragmentCountPBO(s);
    }
}

//------------------------------------------------------------------------------
void vtkDualDepthPeelingPass::InitColorTexture(vtkTextureObject *tex,
                                               const vtkRenderState *s)
{
  tex->SetContext(static_cast<vtkOpenGLRenderWindow*>(
                    s->GetRenderer()->GetRenderWindow()));
  tex->SetFormat(GL_RGBA);
  tex->SetInternalFormat(GL_RGBA8);
  tex->Allocate2D(this->ViewportWidth, this->ViewportHeight, 4,
                  vtkTypeTraits<vtkTypeUInt8>::VTK_TYPE_ID);
}

//------------------------------------------------------------------------------
void vtkDualDepthPeelingPass::InitDepthTexture(vtkTextureObject *tex,
                                               const vtkRenderState *s)
{
  tex->SetContext(static_cast<vtkOpenGLRenderWindow*>(
                    s->GetRenderer()->GetRenderWindow()));
  tex->SetFormat(GL_RG);
  tex->SetInternalFormat(GL_RG32F);
  tex->Allocate2D(this->ViewportWidth, this->ViewportHeight, 2,
                  vtkTypeTraits<vtkTypeFloat32>::VTK_TYPE_ID);
}

//------------------------------------------------------------------------------
void vtkDualDepthPeelingPass::InitOpaqueDepthTexture(vtkTextureObject *tex,
                                                     const vtkRenderState *s)
{
  tex->SetContext(static_cast<vtkOpenGLRenderWindow*>(
                    s->GetRenderer()->GetRenderWindow()));
  tex->AllocateDepth(this->ViewportWidth, this->ViewportHeight,
                     vtkTextureObject::Float32);
}

//------------------------------------------------------------------------------
void vtkDualDepthPeelingPass::InitFragmentCountTexture(vtkTextureObject *tex,
                                                       const vtkRenderState *s)
{
  tex->SetContext(static_cast<vtkOpenGLRenderWindow*>(
                    s->GetRenderer()->GetRenderWindow()));
  tex->AllocateDepthStencil(this->ViewportWidth, this->ViewportHeight,
                            vtkTextureObject::Depth24Stencil8);
}

//------------------------------------------------------------------------------
void vtkDualDepthPeelingPass::InitFramebuffer(const vtkRenderState *s)
{
  this->Framebuffer->SetContext(static_cast<vtkOpenGLRenderWindow*>(
                                  s->GetRenderer()->GetRenderWindow()));

  // Save the current FBO bindings to restore them later.
  this->Framebuffer->SaveCurrentBindings();
  this->Framebuffer->Bind(GL_DRAW_FRAMEBUFFER);

  this->Framebuffer->AddColorAttachment(GL_DRAW_FRAMEBUFFER, BackTemp,
                                        this->Textures[BackTemp]);
  this->Framebuffer->AddColorAttachment(GL_DRAW_FRAMEBUFFER, Back,
                                        this->Textures[Back]);

  this->Framebuffer->AddColorAttachment(GL_DRAW_FRAMEBUFFER, FrontA,
                                        this->Textures[FrontA]);
  this->Framebuffer->AddColorAttachment(GL_DRAW_FRAMEBUFFER, FrontB,
                                        this->Textures[FrontB]);

  // The depth has to be treated like a color attachment, since it's a 2
  // component min-max texture.
  this->Framebuffer->AddColorAttachment(GL_DRAW_FRAMEBUFFER, DepthA,
                                        this->Textures[DepthA]);
  this->Framebuffer->AddColorAttachment(GL_DRAW_FRAMEBUFFER, DepthB,
                                        this->Textures[DepthB]);

  const char *desc = NULL;
  if (!this->Framebuffer->GetFrameBufferStatus(GL_DRAW_FRAMEBUFFER, desc))
    {
    vtkErrorMacro("Depth peeling error detected: Draw framebuffer incomplete: "
                  << desc);
    }

  this->Framebuffer->UnBind(GL_DRAW_FRAMEBUFFER);
}

//------------------------------------------------------------------------------
void vtkDualDepthPeelingPass::InitFragmentCountPBO(const vtkRenderState *s)
{
  vtkOpenGLRenderWindow *context =
      static_cast<vtkOpenGLRenderWindow*>(
        s->GetRenderer()->GetRenderWindow());
  size_t numPixels = this->ViewportWidth * this->ViewportHeight;

  this->FragmentCountFB = vtkFrameBufferObject2::New();
  this->FragmentCountFB->SetContext(context);

  // Allocate 32 bits per pixel for the depth/stencil data. We only need the
  // stencil info, but async readback via PBO requires component size, type,
  // and ordering to be the same in both GPU memory and the PBO. See
  // http://stackoverflow.com/questions/11409693
  this->FragmentCountTransfer = vtkPixelBufferObject::New();
  this->FragmentCountTransfer->SetContext(context);
  this->FragmentCountTransfer->Allocate(numPixels * 4 /* 32 bits */,
                                        vtkPixelBufferObject::PACKED_BUFFER);

  this->FragmentCountFence = vtkOpenGLFenceSync::New();
}

//------------------------------------------------------------------------------
void vtkDualDepthPeelingPass::Prepare()
{
  EventMarker marker("vtkDDP::Prepare");
  // Prevent vtkOpenGLActor from messing with the depth mask:
  size_t numProps = this->RenderState->GetPropArrayCount();
  for (size_t i = 0; i < numProps; ++i)
    {
    vtkProp *prop = this->RenderState->GetPropArray()[i];
    vtkInformation *info = prop->GetPropertyKeys();
    if (!info)
      {
      info = vtkInformation::New();
      prop->SetPropertyKeys(info);
      info->FastDelete();
      }
    info->Set(vtkOpenGLActor::GLDepthMaskOverride(), -1);
    }

  // Setup GL state:
  glDisable(GL_DEPTH_TEST);
  this->InitializeOcclusionQuery();
  this->CurrentPeel = 0;
  this->RenderCount = 0;
  this->DepthComplexity = -1;
  this->DepthComplexityPasses = -1;

  // Save the current FBO bindings to restore them later.
  this->Framebuffer->SaveCurrentBindings();
  this->Framebuffer->Bind(GL_DRAW_FRAMEBUFFER);

  // Attach the fragment count buffer for initialization:
  this->Framebuffer->AddDepthStencilAttachment(GL_DRAW_FRAMEBUFFER,
                                               this->Textures[FragmentCount]);

  // The source front buffer must be initialized, since it simply uses additive
  // blending.
  // The back-blending may discard fragments, so the back peel accumulator needs
  // initialization as well.
  unsigned int targets[2] = { Back, this->FrontSource };
  this->Framebuffer->ActivateDrawBuffers(targets, 2);
  glClearColor(0.f, 0.f, 0.f, 0.f);
  glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

  this->Framebuffer->RemoveTexDepthStencilAttachment(GL_DRAW_FRAMEBUFFER);

  // Fill both depth buffers with -1, -1. This lets us discard fragments in
  // CopyOpaqueDepthBuffers, which gives a moderate performance boost.
  targets[0] = this->DepthSource;
  targets[1] = this->DepthDestination;
  this->Framebuffer->ActivateDrawBuffers(targets, 2);
  glClearColor(-1, -1, 0, 0);
  glClear(GL_COLOR_BUFFER_BIT);

  // Pre-fill the depth buffer with opaque pass data:
  this->CopyOpaqueDepthBuffer();

  // Initialize the transparent depths for the peeling algorithm:
  this->InitializeDepth();
}

//------------------------------------------------------------------------------
void vtkDualDepthPeelingPass::InitializeOcclusionQuery()
{
  this->QueryQueue->Reset();

  int numPixels = this->ViewportHeight * this->ViewportWidth;
  this->OcclusionThreshold = numPixels * this->OcclusionRatio;
  this->WrittenPixels = this->OcclusionThreshold + 1;

  this->QueryQueue->SetPixelThreshold(this->OcclusionThreshold);
}

//------------------------------------------------------------------------------
void vtkDualDepthPeelingPass::CopyOpaqueDepthBuffer()
{
  EventMarker marker("vtkDDP::CopyOpaqueDepthBuffer");

  // Initialize the peeling depth buffer using the existing opaque depth buffer.
  // Note that the min component is stored as -depth, allowing
  // glBlendEquation = GL_MAX to be used during peeling.

  // Copy from the current (default) framebuffer's depth buffer into a texture:
  this->Framebuffer->UnBind(GL_DRAW_FRAMEBUFFER);
  this->Textures[OpaqueDepth]->CopyFromFrameBuffer(
        this->ViewportX, this->ViewportY, 0, 0,
        this->ViewportWidth, this->ViewportHeight);
  this->Framebuffer->Bind(GL_DRAW_FRAMEBUFFER);

  // Fill both depth buffers with the opaque fragment depths. InitializeDepth
  // will compare translucent fragment depths with values in DepthDestination
  // and write to DepthSource using MAX blending, so we need both to have opaque
  // fragments (src/dst seem reversed because they're named for their usage in
  // PeelRender).
  this->Framebuffer->ActivateDrawBuffer(this->DepthDestination);
  this->Textures[OpaqueDepth]->Activate();

  glDisable(GL_BLEND);

  typedef vtkOpenGLRenderUtilities GLUtil;

  vtkOpenGLRenderWindow *renWin = static_cast<vtkOpenGLRenderWindow*>(
        this->RenderState->GetRenderer()->GetRenderWindow());
  if (!this->CopyDepthProgram)
    {
    std::string fragShader = GLUtil::GetFullScreenQuadFragmentShaderTemplate();
    vtkShaderProgram::Substitute(
          fragShader, "//VTK::FSQ::Decl",
          "uniform float clearValue;\n"
          "uniform sampler2D oDepth;\n");
    vtkShaderProgram::Substitute(
          fragShader, "//VTK::FSQ::Impl",
          "  float d = texture2D(oDepth, texCoord).x;\n"
          "  if (d == clearValue)\n"
          "    { // If no depth value has been written, discard the frag:\n"
          "    discard;\n"
          "    }\n"
          "  gl_FragData[0] = vec4(-1, d, 0., 0.);\n"
          );
    this->CopyDepthProgram = renWin->GetShaderCache()->ReadyShaderProgram(
          GLUtil::GetFullScreenQuadVertexShader().c_str(),
          fragShader.c_str(),
          GLUtil::GetFullScreenQuadGeometryShader().c_str());
    }
  else
    {
    renWin->GetShaderCache()->ReadyShaderProgram(this->CopyDepthProgram);
    }

  if (!this->CopyDepthVAO)
    {
    this->CopyDepthVAO = vtkOpenGLVertexArrayObject::New();
    GLUtil::PrepFullScreenVAO(this->CopyDepthVAO, this->CopyDepthProgram);
    }

  // Get the clear value. We don't set this, so it should still be what the
  // opaque pass uses:
  GLfloat clearValue = 1.f;
  glGetFloatv(GL_DEPTH_CLEAR_VALUE, &clearValue);
  this->CopyDepthProgram->SetUniformf("clearValue", clearValue);
  this->CopyDepthProgram->SetUniformi(
        "oDepth", this->Textures[OpaqueDepth]->GetTextureUnit());

  this->CopyDepthVAO->Bind();

  GLUtil::DrawFullScreenQuad();

  this->CopyDepthVAO->Release();

  this->Textures[OpaqueDepth]->Deactivate();
}

//------------------------------------------------------------------------------
void vtkDualDepthPeelingPass::InitializeDepth()
{
  EventMarker marker("vtkDDP::InitializeDepth");

  // Pre-peeling initialization. We render the translucent geometry and
  // determine the first set of inner and outer peels. We also count the number
  // of fragments using a stencil buffer, which allows us to determine how
  // many passes will be needed and minimize the number of pixels processed
  // during blending passes.

  // Attach the depth-stencil buffer for counting fragments:
  this->Framebuffer->AddDepthStencilAttachment(GL_DRAW_FRAMEBUFFER,
                                               this->Textures[FragmentCount]);

  const char *desc = NULL;
  if (!this->Framebuffer->GetFrameBufferStatus(GL_DRAW_FRAMEBUFFER, desc))
    {
    vtkErrorMacro("Depth peeling error detected: Draw framebuffer incomplete: "
                  << desc);
    }

  glDisable(GL_DEPTH_TEST);
  // Setup stencil testing to count fragments:
  glEnable(GL_STENCIL_TEST);
  glStencilFunc(GL_ALWAYS, 0, 0);
  glStencilOp(GL_KEEP, GL_INCR, GL_INCR);

  // We bind the front destination buffer as render target 0 -- the data we
  // write to it isn't used, but this makes it easier to work with the existing
  // polydata shaders as they expect gl_FragData[0] to be RGBA. The front
  // destination buffer is cleared prior to peeling, so it's just a dummy
  // buffer at this point.
  unsigned int targets[2] = { this->FrontDestination, this->DepthSource };
  this->Framebuffer->ActivateDrawBuffers(targets, 2);

  this->SetCurrentStage(InitializingDepth);
  this->Textures[this->DepthDestination]->Activate();

  glEnable(GL_BLEND);
  glBlendEquation(GL_MAX);
  this->RenderTranslucentPass();

  this->Textures[this->DepthDestination]->Deactivate();

  glDisable(GL_STENCIL_TEST);

  // Start the stencil buffer transfer:
  this->BeginFragmentCountTransfer();
}

//------------------------------------------------------------------------------
void vtkDualDepthPeelingPass::EnableStencilForCurrentPass()
{
  // Only process fragments for pixels that the complexity analysis indicates
  // need processing for this peel.
  GLuint pass = this->CurrentPeel + 1; // 0-index to 1-index
  glEnable(GL_STENCIL_TEST);
  // pass 1 handles stencil values >= 1, pass 2 handles >= 3, pass 3 >= 5, etc
  glStencilFunc(GL_GEQUAL, pass * 2 - 1, 0);
  glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
}

//------------------------------------------------------------------------------
void vtkDualDepthPeelingPass::DisableStencil()
{
  glDisable(GL_STENCIL_TEST);
}

//------------------------------------------------------------------------------
void vtkDualDepthPeelingPass::BeginFragmentCountTransfer()
{
  EventMarker marker("vtkDDP::BeginFragmentCountTransfer");

  // Detach the stencil texture from the draw fb
  this->Framebuffer->RemoveTexDepthStencilAttachment(GL_DRAW_FRAMEBUFFER);

  const char *desc = NULL;
  if (!this->Framebuffer->GetFrameBufferStatus(GL_DRAW_FRAMEBUFFER, desc))
    {
    vtkErrorMacro("Depth peeling error detected: Draw framebuffer incomplete: "
                  << desc);
    }

  // Reattach it to the read fb
  this->FragmentCountFB->Bind(GL_READ_FRAMEBUFFER);
  this->FragmentCountFB->AddDepthStencilAttachment(
        GL_READ_FRAMEBUFFER, this->Textures[FragmentCount]);

  if (!this->FragmentCountFB->GetFrameBufferStatus(GL_READ_FRAMEBUFFER, desc))
    {
    vtkErrorMacro("Depth peeling error detected: Stencil-read framebuffer "
                  "incomplete: " << desc);
    }

  // Start an async transfer of the stencil data from GPU -> CPU via a PBO.
  // We fetch both the (garbage) depth info and the (useful) stencil info, since
  // async readback via PBO requires the size, type, and order of components to
  // match.
  this->FragmentCountTransfer->Bind(vtkPixelBufferObject::PACKED_BUFFER);

  startEvent("glReadPixels");
  glReadPixels(0, 0, this->ViewportWidth, this->ViewportHeight,
               GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, 0);
  vtkOpenGLCheckErrorMacro("Failed after glReadPixels");
  endEvent("glReadPixels");

  this->FragmentCountTransfer->UnBind();
  this->FragmentCountFB->UnBind(GL_READ_FRAMEBUFFER);

  // Insert the fence into the command stream and flush the commands to GPU:
  startEvent("MarkFence");
  this->FragmentCountFence->Mark();
  endEvent("MarkFence");
  startEvent("FlushFence");
  this->FragmentCountFence->Flush();
  endEvent("FlushFence");

  // Re-attach the stencil to the draw fb for limiting fragments during blends.
  this->Framebuffer->AddDepthStencilAttachment(GL_DRAW_FRAMEBUFFER,
                                               this->Textures[FragmentCount]);

  if (!this->Framebuffer->GetFrameBufferStatus(GL_DRAW_FRAMEBUFFER, desc))
    {
    vtkErrorMacro("Depth peeling error detected: Draw framebuffer incomplete: "
                  << desc);
    }

#ifdef DEBUG_FRAGMENTCOUNT
  std::cout << "Fragment count transfer started\n";
#endif // DEBUG_FRAGMENTCOUNT
}

//------------------------------------------------------------------------------
void vtkDualDepthPeelingPass::CheckFragmentCountTransfer()
{
  if (this->DepthComplexity >= 0)
    { // Already completed.
    return;
    }

  EventMarker marker("vtkDDP::CheckFragmentCountTransfer");

#ifdef DEBUG_FRAGMENTCOUNT
  std::cout << "Checking fragment count transfer status for peel "
            << this->CurrentPeel << "\n";
#endif // DEBUG_FRAGMENTCOUNT

  // Are we done?
  if (!this->FragmentCountFence->IsFinished())
    {
    // If we're less than 5 peels in, just keep waiting.
    if (this->CurrentPeel < 5)
      {
#ifdef DEBUG_FRAGMENTCOUNT
      std::cout << "Fragment count transfer not finished.\n";
#endif // DEBUG_FRAGMENTCOUNT
      return;
      }

#ifdef DEBUG_FRAGMENTCOUNT
    std::cout << "Fragment count transfer not finished after 5 peels. "
                 "Flushing first render pass to force results.\n";
#endif // DEBUG_FRAGMENTCOUNT

    // Otherwise, request that the occlusion query queue flush the first render
    // pass. This ensures that the fence (set in InitializeDepth) will be
    // processed without having to flush the entire command stream.
    this->QueryQueue->FlushToQuery(0);

    // Sanity check:
    if (!this->FragmentCountFence->IsFinished())
      {
      vtkWarningMacro("Fence still not processed after glFinished().");
      return;
      }
    }

  // We only reach this point when the fragment count buffer is ready.
  // Processing this buffer takes a significant amount of time, so before we do,
  // update the occlusion query queue and return early if possible:
  this->UpdateOcclusionQueryQueue();
  if (this->WrittenPixels <= this->OcclusionThreshold)
    {
#ifdef DEBUG_FRAGMENTCOUNT
    std::cout << "Peeling completed before FragmentCount buffer ready.\n";
#endif
    return;
    }
  this->ProcessFragmentCount();
}

namespace {
// Memory layout of the depth/stencil buffer. Helper for processing.
struct DepthStencil
{
  // Despite what everything I've read says, the packed Depth24Stencil8 format
  // actually places the stencil data first...
  unsigned char Stencil;
  unsigned char Depth[3];
};
} // end anon namespace

//------------------------------------------------------------------------------
void vtkDualDepthPeelingPass::ProcessFragmentCount()
{
  EventMarker marker("vtkDDP::ProcessFragmentCount");

  void *dataRaw = this->FragmentCountTransfer->MapBuffer(
        vtkPixelBufferObject::PACKED_BUFFER);

  if (dataRaw == NULL)
    {
    vtkErrorMacro("Unable to map stencil buffer.");
    return;
    }

  DepthStencil *data = static_cast<DepthStencil*>(dataRaw);

  // We'll count how many pixel are at a given complexity level. Map has 256
  // entries, since we're using an 8-bit stencil buffer.
  unsigned long long int pixelsAtComplexity[256];
  std::fill(pixelsAtComplexity, pixelsAtComplexity + 256, 0);

  // Tabulate how many pixels have a given complexity:
  for (int i = 0; i < this->ViewportWidth; ++i)
    {
    for (int j = 0; j < this->ViewportHeight; ++j)
      {
      ++pixelsAtComplexity[data[j * this->ViewportWidth + i].Stencil];
      }
    }

  // Update table to reflect that higher complexity pixels also count against
  // lower complexity pixels (eg. a pixel with complexity 5 is also handled
  // during the pass for complexity 2):
  for (int i = 0; i < 255; ++i)
    {
    for (int j = i + 1; j < 256; ++j)
      {
      pixelsAtComplexity[i] += pixelsAtComplexity[j];
      }
    }

  dataRaw = NULL;
  data = NULL;
  this->FragmentCountTransfer->UnmapBuffer(vtkPixelBufferObject::PACKED_BUFFER);
  this->FragmentCountTransfer->UnBind();

  // Find the first entry in the pixel/complexity map that satisfy the
  // occlusion criteria. This will be our depth complexity.
  for (int i = 0; i < 255; ++i)
    {
    if (pixelsAtComplexity[i] <= this->OcclusionThreshold)
      {
      this->DepthComplexity = i - 1;
      break;
      }
    }

  if (this->DepthComplexity >= 0)
    {
    // + 1 for integer rounding:
    this->DepthComplexityPasses = (this->DepthComplexity + 1) / 2;
    }

#ifdef DEBUG_FRAGMENTCOUNT
  std::cout << "Fragment count transfer completed. Complexity table:\n";
  for (int i = 0; i <= this->DepthComplexity + 1; ++i)
    {
    std::cout << "Complexity: " << i << " pixels: " << pixelsAtComplexity[i]
              << "\n";
    }
  std::cout << "Occlusion threshold of " << this->OcclusionThreshold
            << " should be reached in " << this->DepthComplexityPasses
            << " passes to cover a depth complexity of "
            << this->DepthComplexity << ".\n";
#endif // DEBUG_FRAGMENTCOUNT
}

//------------------------------------------------------------------------------
bool vtkDualDepthPeelingPass::PeelingDone()
{
  // Did we exceed the number of peels specified by either the user, or the
  // depth complexity analysis?
  this->CheckFragmentCountTransfer();
  bool result = (this->CurrentPeel >= this->MaximumNumberOfPeels ||
                 (this->DepthComplexityPasses >= 0 &&
                  this->CurrentPeel >= this->DepthComplexityPasses));

  // Only check the occlusion query queue if we aren't finished:
  if (!result)
    {
    this->UpdateOcclusionQueryQueue();
    result = this->WrittenPixels <= this->OcclusionThreshold;
    }

  return result;
}

//------------------------------------------------------------------------------
void vtkDualDepthPeelingPass::Peel()
{
  std::ostringstream event;
  event << "vtkDDP::Peel (" << (this->CurrentPeel + 1) << ")";
  EventMarker marker(event.str());

  this->InitializeTargets();
  this->PeelRender();
  this->BlendBackBuffer();
  this->SwapTargets();
  ++this->CurrentPeel;

#ifdef DEBUG_PEEL
  std::cout << "Peel " << this->CurrentPeel << ": Pixels written: "
            << this->WrittenPixels << " (threshold: "
            << this->OcclusionThreshold << ")\n";
#endif // DEBUG_PEEL
}

//------------------------------------------------------------------------------
void vtkDualDepthPeelingPass::InitializeTargets()
{
  EventMarker marker("vtkDDP::InitializeTargets");

  // Initialize destination buffers to their minima, since we're MAX blending,
  // this ensures that valid outputs are captured.
  unsigned int destColorBuffers[2] = { this->FrontDestination, BackTemp };
  this->Framebuffer->ActivateDrawBuffers(destColorBuffers, 2);
  glClearColor(0.f, 0.f, 0.f, 0.f);
  glClear(GL_COLOR_BUFFER_BIT);

  this->Framebuffer->ActivateDrawBuffer(this->DepthDestination);
  glClearColor(-1.f, -1.f, 0.f, 0.f);
  glClear(GL_COLOR_BUFFER_BIT);
}

//------------------------------------------------------------------------------
void vtkDualDepthPeelingPass::PeelRender()
{
  EventMarker marker("vtkDDP::PeelRender");

  // Enable the destination targets:
  unsigned int targets[3] = { BackTemp, this->FrontDestination,
                              this->DepthDestination };
  this->Framebuffer->ActivateDrawBuffers(targets, 3);

  // Use MAX blending to capture peels:
  glEnable(GL_BLEND);
  glBlendEquation(GL_MAX);

  this->SetCurrentStage(Peeling);
  this->Textures[this->FrontSource]->Activate();
  this->Textures[this->DepthSource]->Activate();

  this->RenderTranslucentPass();

  this->Textures[this->FrontSource]->Deactivate();
  this->Textures[this->DepthSource]->Deactivate();
}

//------------------------------------------------------------------------------
void vtkDualDepthPeelingPass::BlendBackBuffer()
{
  EventMarker marker("vtkDDP::BlendBackBuffer");

  this->Framebuffer->ActivateDrawBuffer(Back);
  this->Textures[BackTemp]->Activate();

  /* For this step, we blend the last peel's back fragments into a back-
   * accumulation buffer. The full over-blending equations are:
   *
   * (f = front frag (incoming peel); b = back frag (current accum. buffer))
   *
   * a = f.a + (1. - f.a) * b.a
   *
   * if a == 0, C == (0, 0, 0). Otherwise,
   *
   * C = ( f.a * f.rgb + (1. - f.a) * b.a * b.rgb ) / a
   *
   * We use premultiplied alphas to save on computations, resulting in:
   *
   * [a * C] = [f.a * f.rgb] + (1 - f.a) * [ b.a * b.rgb ]
   * a = f.a + (1. - f.a) * b.a
   */

  glEnable(GL_BLEND);
  glBlendEquation(GL_FUNC_ADD);
  glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

  typedef vtkOpenGLRenderUtilities GLUtil;

  vtkOpenGLRenderWindow *renWin = static_cast<vtkOpenGLRenderWindow*>(
        this->RenderState->GetRenderer()->GetRenderWindow());
  if (!this->BackBlendProgram)
    {
    std::string fragShader = GLUtil::GetFullScreenQuadFragmentShaderTemplate();
    vtkShaderProgram::Substitute(
          fragShader, "//VTK::FSQ::Decl",
          "uniform sampler2D newPeel;\n"
          );
    vtkShaderProgram::Substitute(
          fragShader, "//VTK::FSQ::Impl",
          "  vec4 f = texture2D(newPeel, texCoord); // new frag\n"
          "  if (f.a == 0.)\n"
          "    {\n"
          "    discard;\n"
          "    }\n"
          "\n"
          "  gl_FragData[0] = f;\n"
          );
    this->BackBlendProgram = renWin->GetShaderCache()->ReadyShaderProgram(
          GLUtil::GetFullScreenQuadVertexShader().c_str(),
          fragShader.c_str(),
          GLUtil::GetFullScreenQuadGeometryShader().c_str());
    }
  else
    {
    renWin->GetShaderCache()->ReadyShaderProgram(this->BackBlendProgram);
    }

  if (!this->BackBlendVAO)
    {
    this->BackBlendVAO = vtkOpenGLVertexArrayObject::New();
    GLUtil::PrepFullScreenVAO(this->BackBlendVAO, this->BackBlendProgram);
    }

  // Stencil out the regions that aren't important for this pass:
  this->EnableStencilForCurrentPass();

  this->BackBlendProgram->SetUniformi(
        "newPeel", this->Textures[BackTemp]->GetTextureUnit());

  this->BackBlendVAO->Bind();

  this->StartOcclusionQuery();
  GLUtil::DrawFullScreenQuad();
  this->EndOcclusionQuery();

  this->BackBlendVAO->Release();

  this->Textures[BackTemp]->Deactivate();

  this->DisableStencil();
}

//------------------------------------------------------------------------------
void vtkDualDepthPeelingPass::StartOcclusionQuery()
{
  // Unfortunately, the stencil buffer we use to determine depth complexity
  // during InitializeDepth seems to double count some fragments, so we may
  // overestimate the number of passes needed. For this reason, we keep the
  // number of passes needed for the last frame and check occlusion there as
  // well.

  // If we don't have depth complexity information, just use the last frame's
  // info as an estimate:
  int lowEstimate = this->LastFramePassCount;
  int highEstimate = this->LastFramePassCount;

  // Account for depth complexity info if available:
  if (this->DepthComplexityPasses > 0)
    {
    if (this->DepthComplexityPasses <= this->LastFramePassCount)
      {
      // Depth complexity says there are fewer passes needed than last time.
      // The depth complexity value is a hard upper limit, so use it for both
      // estimates:
      lowEstimate = this->DepthComplexityPasses;
      highEstimate = this->DepthComplexityPasses;
      }
    else
      {
      // If it took fewer passes last time than we're estimating now, check
      // both:
      highEstimate = this->DepthComplexityPasses;
      }
    }

  // Update the threshold based on the current pass:
  if (this->CurrentPeel < lowEstimate)
    {
    this->QueryQueue->SetFlushThresholdInTotalQueries(lowEstimate);
    }
  else if (this->CurrentPeel < highEstimate)
    {
    this->QueryQueue->SetFlushThresholdInTotalQueries(highEstimate);
    }
  else
    {
    // If we've exceeded the high estimate, check every three passes:
    this->QueryQueue->SetFlushThreshold(3);
    }

  this->QueryQueue->StartQuery();
}

//------------------------------------------------------------------------------
void vtkDualDepthPeelingPass::EndOcclusionQuery()
{
  this->QueryQueue->EndQuery();
}

//------------------------------------------------------------------------------
void vtkDualDepthPeelingPass::UpdateOcclusionQueryQueue()
{
  EventMarker marker("vtkDDP::UpdateOcclusionQueryQueue");

  // Check to see if any queries finished:
  this->QueryQueue->UpdateQueryStatuses();
  if (this->QueryQueue->GetAnyQueriesFinished())
    {
    this->WrittenPixels = this->QueryQueue->GetNumberOfPixelsWritten();
    }
}

//------------------------------------------------------------------------------
void vtkDualDepthPeelingPass::SwapTargets()
{
  std::swap(this->FrontSource, this->FrontDestination);
  std::swap(this->DepthSource, this->DepthDestination);
}

//------------------------------------------------------------------------------
void vtkDualDepthPeelingPass::Finalize()
{
  EventMarker marker("vtkDDP::Finalize");
  // Mop up any unrendered fragments using simple alpha blending into the back
  // buffer.
  if (this->WrittenPixels > 0 &&
      (this->DepthComplexityPasses < 1 ||
       this->CurrentPeel < this->DepthComplexityPasses))
    {
    this->AlphaBlendRender();
    }

  this->NumberOfRenderedProps =
      this->TranslucentPass->GetNumberOfRenderedProps();

  this->FragmentCountFence->Reset();
  this->Framebuffer->UnBind(GL_DRAW_FRAMEBUFFER);
  this->BlendFinalImage();

  // Restore blending parameters:
  glEnable(GL_BLEND);
  glBlendEquation(GL_FUNC_ADD);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  size_t numProps = this->RenderState->GetPropArrayCount();
  for (size_t i = 0; i < numProps; ++i)
    {
    vtkProp *prop = this->RenderState->GetPropArray()[i];
    vtkInformation *info = prop->GetPropertyKeys();
    if (info)
      {
      info->Remove(vtkOpenGLActor::GLDepthMaskOverride());
      }
    }

  this->RenderState = NULL;
  this->FinalizeOcclusionQuery();
  this->SetCurrentStage(Inactive);

#ifdef DEBUG_FRAME
  std::cout << "Depth peel done:\n"
            << "  - Number of peels: " << this->CurrentPeel << "\n"
            << "  - Number of geometry passes: " << this->RenderCount << "\n"
            << "  - Last Peel Occlusion Ratio: "
            << static_cast<float>(this->WrittenPixels) /
               static_cast<float>(this->ViewportWidth * this->ViewportHeight)
            << " (target: " << this->OcclusionRatio << ")\n"
            << "  - Predicted depth complexity: "  << this->DepthComplexity
            << "\n"
            << "  - Predicted number of passes: " << this->DepthComplexityPasses
            << "\n";
#endif // DEBUG_FRAME
}

//------------------------------------------------------------------------------
void vtkDualDepthPeelingPass::AlphaBlendRender()
{
  EventMarker markerEvent("vtkDDP::AlphaBlendRender");

  /* This pass is mopping up the remaining fragments when we exceed the max
   * number of peels or hit the occlusion limit. We'll simply render all of the
   * remaining fragments into the back destination buffer using the
   * premultiplied-alpha over-blending equations:
   *
   * aC = f.a * f.rgb + (1 - f.a) * b.a * b.rgb
   * a = f.a + (1 - f.a) * b.a
   */
  glEnable(GL_BLEND);
  glBlendEquation(GL_FUNC_ADD);
  glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

  this->EnableStencilForCurrentPass();

  this->SetCurrentStage(AlphaBlending);
  this->Framebuffer->ActivateDrawBuffer(Back);
  this->Textures[this->DepthSource]->Activate();

  this->RenderTranslucentPass();

  this->Textures[this->DepthSource]->Deactivate();

  this->DisableStencil();
}

//------------------------------------------------------------------------------
void vtkDualDepthPeelingPass::BlendFinalImage()
{
  EventMarker marker("vtkDDP::BlendFinalImage");
  this->Textures[this->FrontSource]->Activate();
  this->Textures[Back]->Activate();

  /* The final pixel (including the opaque layer is:
   *
   * C = (1 - b.a) * f.a * o.a * o.rgb + f.a * (b.a * b.rgb) + f.rgb
   *
   * ( C = final color; o = opaque frag; b = back frag; f = front frag )
   *
   * This is obtained from repeatedly applying the underblend equations:
   *
   * C = f.a * b.a * b.rgb + f.rgb
   * a = (1 - b.a) * f.a
   *
   * These blending parameters and fragment shader perform this work.
   * Note that the opaque fragments are assumed to have premultiplied alpha
   * in this implementation. */
  glEnable(GL_BLEND);
  glBlendEquation(GL_FUNC_ADD);
  glBlendFunc(GL_ONE, GL_SRC_ALPHA);

  typedef vtkOpenGLRenderUtilities GLUtil;

  vtkOpenGLRenderWindow *renWin = static_cast<vtkOpenGLRenderWindow*>(
        this->RenderState->GetRenderer()->GetRenderWindow());
  if (!this->BlendProgram)
    {
    std::string fragShader = GLUtil::GetFullScreenQuadFragmentShaderTemplate();
    vtkShaderProgram::Substitute(
          fragShader, "//VTK::FSQ::Decl",
          "uniform sampler2D frontTexture;\n"
          "uniform sampler2D backTexture;\n"
          );
    vtkShaderProgram::Substitute(
          fragShader, "//VTK::FSQ::Impl",
          "  vec4 front = texture2D(frontTexture, texCoord);\n"
          "  vec4 back = texture2D(backTexture, texCoord);\n"
          "  front.a = 1. - front.a; // stored as (1 - alpha)\n"
          "  // Underblend. Back color is premultiplied:\n"
          "  gl_FragData[0].rgb = (front.rgb + back.rgb * front.a);\n"
          "  gl_FragData[0].a = front.a * (1 - back.a);\n"
          );
    this->BlendProgram = renWin->GetShaderCache()->ReadyShaderProgram(
          GLUtil::GetFullScreenQuadVertexShader().c_str(),
          fragShader.c_str(),
          GLUtil::GetFullScreenQuadGeometryShader().c_str());
    }
  else
    {
    renWin->GetShaderCache()->ReadyShaderProgram(this->BlendProgram);
    }

  if (!this->BlendVAO)
    {
    this->BlendVAO = vtkOpenGLVertexArrayObject::New();
    GLUtil::PrepFullScreenVAO(this->BlendVAO, this->BlendProgram);
    }

  this->BlendProgram->SetUniformi(
        "frontTexture", this->Textures[this->FrontSource]->GetTextureUnit());
  this->BlendProgram->SetUniformi(
        "backTexture", this->Textures[Back]->GetTextureUnit());

  this->BlendVAO->Bind();

  GLUtil::DrawFullScreenQuad();

  this->BlendVAO->Release();

  this->Textures[this->FrontSource]->Deactivate();
  this->Textures[Back]->Deactivate();
}

//------------------------------------------------------------------------------
void vtkDualDepthPeelingPass::FinalizeOcclusionQuery()
{
  // Get the number of passes need to reach the occlusion ratio:
  int numPasses = this->QueryQueue->GetQueriesNeededForPixelThreshold();

  // If == 0, we never hit the desired occlusion ratio (this happens when we
  // hit the number of required number of passes as determined by the depth
  // complexity analysis, as we don't do an additional pass to confirm that
  // we've finished, so the query manager thinks we still need more).
  // Alternatively, we may have hit the maximum number of peels specified by
  // the user. In either case, just record the number of peels taken this time.
  if (numPasses < 1)
    {
    numPasses = this->CurrentPeel;
    }

  this->LastFramePassCount = numPasses;

  this->QueryQueue->Reset();
}
