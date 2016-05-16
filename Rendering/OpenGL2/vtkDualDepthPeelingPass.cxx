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
#include "vtkTypeTraits.h"

#include <algorithm>
#include <cmath>

// Define to print debug statements to the OpenGL CS stream (useful for e.g.
// apitrace debugging):
//#define ANNOTATE_STREAM

// Define to output details about each peel:
//#define DEBUG_PEEL

// Define to output details about each frame:
//#define DEBUG_FRAME

// Define to debug fragment counting:
//#define DEBUG_FRAGMENTCOUNT

vtkStandardNewMacro(vtkDualDepthPeelingPass)

namespace
{
void annotate(const std::string &str)
{
#ifdef ANNOTATE_STREAM
  vtkOpenGLStaticCheckErrorMacro("Error before glDebug.")
  glDebugMessageInsert(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_OTHER,
                       GL_DEBUG_SEVERITY_NOTIFICATION,
                       0, str.size(), str.c_str());
  vtkOpenGLClearErrorMacro();
#else // ANNOTATE_STREAM
  (void)str;
#endif // ANNOTATE_STREAM
}
}

//------------------------------------------------------------------------------
void vtkDualDepthPeelingPass::PrintSelf(std::ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void vtkDualDepthPeelingPass::Render(const vtkRenderState *s)
{
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
    vtkOpenGLRenderWindow *context =
        static_cast<vtkOpenGLRenderWindow*>(
          s->GetRenderer()->GetRenderWindow());
    size_t numPixels = this->ViewportWidth * this->ViewportHeight;
    this->FragmentCountFB = vtkFrameBufferObject2::New();
    this->FragmentCountFB->SetContext(context);
    this->FragmentCountTransfer = vtkPixelBufferObject::New();
    this->FragmentCountTransfer->SetContext(context);
    this->FragmentCountTransfer->Allocate(numPixels * sizeof(GLubyte),
                                          vtkPixelBufferObject::PACKED_BUFFER);
    this->FragmentCountFence = vtkOpenGLFenceSync::New();

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
void vtkDualDepthPeelingPass::Prepare()
{
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

  annotate("Copying opaque depth!");
  GLUtil::DrawFullScreenQuad();
  annotate("Opaque depth copied!");

  this->CopyDepthVAO->Release();

  this->Textures[OpaqueDepth]->Deactivate();
}

//------------------------------------------------------------------------------
void vtkDualDepthPeelingPass::InitializeDepth()
{
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
  annotate("Initializing depth.");
  this->RenderTranslucentPass();
  annotate("Depth initialized");

  this->Textures[this->DepthDestination]->Deactivate();

  // Detach the depth-stencil texture so we can query it asynchronously.
  glDisable(GL_STENCIL_TEST);
  this->Framebuffer->RemoveTexDepthStencilAttachment(GL_DRAW_FRAMEBUFFER);

  if (!this->Framebuffer->GetFrameBufferStatus(GL_DRAW_FRAMEBUFFER, desc))
    {
    vtkErrorMacro("Depth peeling error detected: Draw framebuffer incomplete: "
                  << desc);
    }

  // Start the stencil buffer transfer:
  this->BeginFragmentCountTransfer();
}

//------------------------------------------------------------------------------
void vtkDualDepthPeelingPass::BeginFragmentCountTransfer()
{
  this->FragmentCountFB->Bind(GL_READ_FRAMEBUFFER);
  this->FragmentCountFB->AddDepthStencilAttachment(
        GL_READ_FRAMEBUFFER, this->Textures[FragmentCount]);
  this->FragmentCountTransfer->Bind(vtkPixelBufferObject::PACKED_BUFFER);

  const char *desc = NULL;
  if (!this->FragmentCountFB->GetFrameBufferStatus(GL_READ_FRAMEBUFFER, desc))
    {
    vtkErrorMacro("Depth peeling error detected: Stencil-read framebuffer "
                  "incomplete: " << desc);
    }


  // Begin the async transfer GL --> PBO
  glReadPixels(0, 0, this->ViewportWidth, this->ViewportHeight,
               GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, 0);
  vtkOpenGLCheckErrorMacro("Failed after glReadPixels");

  this->FragmentCountTransfer->UnBind();

  // Insert the fence into the command stream and flush the commands to GPU:
  this->FragmentCountFence->Mark();
  this->FragmentCountFence->Flush();

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
                   "Calling glFinish() to force results.\n";
#endif // DEBUG_FRAGMENTCOUNT

    // Otherwise, force a pipeline sync:
    glFinish();
    // Sanity check:
    if (!this->FragmentCountFence->IsFinished())
      {
      vtkWarningMacro("Fence still not processed after glFinished().");
      return;
      }
    }

  this->ProcessFragmentCount();
}

//------------------------------------------------------------------------------
void vtkDualDepthPeelingPass::ProcessFragmentCount()
{
  void *dataRaw = this->FragmentCountTransfer->MapBuffer(
        vtkPixelBufferObject::PACKED_BUFFER);

  if (dataRaw == NULL)
    {
    vtkErrorMacro("Unable to map stencil buffer.");
    return;
    }

  unsigned char *data = static_cast<unsigned char*>(dataRaw);

  // We'll count how many pixel are at a given complexity level. Map has 256
  // entries, since we're using an 8-bit stencil buffer.
  unsigned long long int pixelsAtComplexity[256];
  std::fill(pixelsAtComplexity, pixelsAtComplexity + 256, 0);

  // Tabulate how many pixels have a given complexity:
  for (int i = 0; i < this->ViewportWidth; ++i)
    {
    for (int j = 0; j < this->ViewportHeight; ++j)
      {
      ++pixelsAtComplexity[data[j * this->ViewportWidth + i]];
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
  return (this->CurrentPeel >= this->MaximumNumberOfPeels ||
          this->WrittenPixels <= this->OcclusionThreshold ||
          (this->DepthComplexityPasses >= 0 &&
           this->CurrentPeel >= this->DepthComplexityPasses));
}

//------------------------------------------------------------------------------
void vtkDualDepthPeelingPass::Peel()
{
  this->InitializeTargets();
  this->PeelRender();
  this->BlendBackBuffer();
  this->SwapTargets();
  this->CheckFragmentCountTransfer();
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

  annotate("Start peeling!");
  this->RenderTranslucentPass();
  annotate("Peeling done!");

  this->Textures[this->FrontSource]->Deactivate();
  this->Textures[this->DepthSource]->Deactivate();
}

//------------------------------------------------------------------------------
void vtkDualDepthPeelingPass::BlendBackBuffer()
{
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

  this->BackBlendProgram->SetUniformi(
        "newPeel", this->Textures[BackTemp]->GetTextureUnit());

  this->BackBlendVAO->Bind();

  this->StartOcclusionQuery();
  annotate("Start blending back!");
  GLUtil::DrawFullScreenQuad();
  annotate("Back blended!");
  this->EndOcclusionQuery();

  this->BackBlendVAO->Release();

  this->Textures[BackTemp]->Deactivate();
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
  // Mop up any unrendered fragments using simple alpha blending into the back
  // buffer.
  if (this->WrittenPixels > 0)
    {
    this->AlphaBlendRender();
    }

  this->NumberOfRenderedProps =
      this->TranslucentPass->GetNumberOfRenderedProps();

  this->FragmentCountFB->UnBind(GL_READ_FRAMEBUFFER);
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
            << "  - Occlusion Ratio: "
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

  this->SetCurrentStage(AlphaBlending);
  this->Framebuffer->ActivateDrawBuffer(Back);
  this->Textures[this->DepthSource]->Activate();

  annotate("Alpha blend render start");
  this->RenderTranslucentPass();
  annotate("Alpha blend render end");

  this->Textures[this->DepthSource]->Deactivate();
}

//------------------------------------------------------------------------------
void vtkDualDepthPeelingPass::BlendFinalImage()
{
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

  annotate("blending final!");
  GLUtil::DrawFullScreenQuad();
  annotate("final blended!");

  this->BlendVAO->Release();

  this->Textures[this->FrontSource]->Deactivate();
  this->Textures[Back]->Deactivate();
}

//------------------------------------------------------------------------------
void vtkDualDepthPeelingPass::FinalizeOcclusionQuery()
{
  // Set the number of passes before we flush for the next frame.
  int passesNeeded = this->QueryQueue->GetQueriesNeededForPixelThreshold();
  this->LastFramePassCount = passesNeeded;

  this->QueryQueue->Reset();
}
