/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDualDepthPeelingPass.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkDualDepthPeelingPass - Implements the dual depth peeling algorithm.
//
// .SECTION Description
// Dual depth peeling is an augmentatation of the standard depth peeling
// algorithm that peels two layers (front and back) for each render pass. The
// technique is described in "Order independent transparency with dual depth
// peeling" (February 2008) by L. Bavoil, K. Myers.
//
// The pass occurs in several stages:
//
// 1. Copy the current (opaque geometry) depth buffer into a texture.
// 2. Initialize the min-max depth buffer from the opaque depth texture and the
//    translucent geometry.
// 3. Peel the nearest and farthest fragments:
// 3a. Blend fragments that match the nearest depth of the min-max depth buffer
//     into the front buffer.
// 3b. Write the far depth fragments into a temporary buffer.
// 3c. Extract the next set of min/max depth values for the next peel.
// 3d. Blend the temporary far fragment texture (3b) into an accumulation
//     texture.
// 3e. Go back to 3a and repeat until the maximum number of peels is met, or
//     the desired occlusion ratio is satisfied.
// 4. If the occlusion ratio != 0 (i.e. we hit the maximum number of peels
//    before finishing), alpha blend the remaining fragments in-between the
//    near and far accumulation textures.
// 5. Blend all accumulation buffers over the opaque color buffer to produce the
//    final image.
//
// There are a few new improvements over the published method that are used
// to improve performance:
//
// - Delayed occlusion queries: Rather than check the occlusion ratio (which
//   causes a full pipeline stall) after every pass, a
//   vtkOpenGLOcclusionQueryQueue object is used to track the occlusion queries.
//   These are only checked after significant numbers of passes; for example,
//   queries are checked after the number of passes needed to complete the last
//   frame, as there is typically little variation in depth complexity between
//   frames.
//
// - Depth complexity analysis: During the pre-peeling initialization pass
//   through the geometry, the stencil buffer is used to count the number of
//   non-occluded transluscent fragments that will be rendered to each pixel.
//   This information is asynchronously transferred to system memory while
//   the first few peeling passes occur. When it is available, it is inspected
//   to determine the exact number of passes needed to fully process the scene.
//
// - Stenciled fullscreen blend passes: At several points during peeling,
//   full-screen textures need to be blended to produce either intermediate or
//   final renderings. These passes re-use the stencil buffer used for depth
//   complexity analysis to limit the blending operations to only those pixels
//   which should have fragments from the current peel layers.


#ifndef vtkDualDepthPeelingPass_h
#define vtkDualDepthPeelingPass_h

#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkDepthPeelingPass.h"
#include "vtkNew.h" // For what it says.

class vtkFrameBufferObject2;
class vtkOpenGLBufferObject;
class vtkOpenGLFenceSync;
class vtkOpenGLOcclusionQueryQueue;
class vtkOpenGLVertexArrayObject;
class vtkPixelBufferObject;
class vtkShaderProgram;
class vtkTextureObject;

class VTKRENDERINGOPENGL2_EXPORT vtkDualDepthPeelingPass:
    public vtkDepthPeelingPass
{
public:
  static vtkDualDepthPeelingPass* New();
  vtkTypeMacro(vtkDualDepthPeelingPass, vtkDepthPeelingPass)
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  virtual void Render(const vtkRenderState *s);
  virtual void ReleaseGraphicsResources(vtkWindow *w);

  // vtkOpenGLRenderPass virtuals:
  virtual bool ReplaceShaderValues(std::string &vertexShader,
                                   std::string &geometryShader,
                                   std::string &fragmentShader,
                                   vtkAbstractMapper *mapper,
                                   vtkProp *prop);
  virtual bool SetShaderParameters(vtkShaderProgram *program,
                                   vtkAbstractMapper *mapper, vtkProp *prop);
  virtual unsigned long int GetShaderStageMTime();

protected:

  // Name the textures used by this render pass. These are indexes into
  // this->Textures
  enum TextureName
    {
    BackTemp = 0, // RGBA8 back-to-front peeling buffer
    Back, // RGBA8 back-to-front accumulation buffer
    FrontA, // RGBA8 front-to-back accumulation buffer
    FrontB, // RGBA8 front-to-back accumulation buffer
    DepthA, // RG32F min-max depth buffer
    DepthB, // RG32F min-max depth buffer
    OpaqueDepth, // Stores the depth map from the opaque passes
    FragmentCount, // Depth24Stencil8. Counts the number of fragments per-pixel

    NumberOfTextures
    };

  // The stages of this multipass render pass:
  enum ShaderStage
    {
    InitializingDepth,
    Peeling,
    AlphaBlending,

    NumberOfPasses,
    Inactive = -1,
    };

  vtkDualDepthPeelingPass();
  ~vtkDualDepthPeelingPass();

  void SetCurrentStage(ShaderStage stage);

  // Description:
  // Release all FBOs and textures.
  void FreeGLObjects();

  // Description:
  // Render the translucent pass geometry, counting number of render calls.
  void RenderTranslucentPass();

  // Description:
  // Allocate and configure FBOs and textures.
  void Initialize(const vtkRenderState *s);

  // Description:
  // Initialize helpers.
  void InitColorTexture(vtkTextureObject *tex, const vtkRenderState *s);
  void InitDepthTexture(vtkTextureObject *tex, const vtkRenderState *s);
  void InitOpaqueDepthTexture(vtkTextureObject *tex, const vtkRenderState *s);
  void InitFragmentCountTexture(vtkTextureObject *tex, const vtkRenderState *s);
  void InitFramebuffer(const vtkRenderState *s);
  void InitFragmentCountPBO(const vtkRenderState *s);

  // Description:
  // Fill textures with initial values, bind the framebuffer.
  void Prepare();
  void InitializeOcclusionQuery();
  void CopyOpaqueDepthBuffer();
  void InitializeDepth();

  // Setup the stencil buffer for reading
  void EnableStencilForCurrentPass();
  void DisableStencil();

  // Manage the fragment count fetch.
  void BeginFragmentCountTransfer();
  void CheckFragmentCountTransfer();
  void ProcessFragmentCount();

  bool PeelingDone();

  // Description:
  // Render the scene to produce the next set of peels.
  void Peel();

  void InitializeTargets();

  void PeelRender();

  void BlendBackBuffer();
  void StartOcclusionQuery();
  void EndOcclusionQuery();
  void UpdateOcclusionQueryQueue();

  // Description:
  // Swap the src/dest render targets:
  void SwapTargets();

  void Finalize();

  void AlphaBlendRender();

  void BlendFinalImage();
  void FinalizeOcclusionQuery();

  const vtkRenderState *RenderState;

  vtkShaderProgram *CopyDepthProgram;
  vtkOpenGLVertexArrayObject *CopyDepthVAO;

  vtkShaderProgram *BackBlendProgram;
  vtkOpenGLVertexArrayObject *BackBlendVAO;

  vtkShaderProgram *BlendProgram;
  vtkOpenGLVertexArrayObject *BlendVAO;

  vtkFrameBufferObject2 *FragmentCountFB;
  vtkPixelBufferObject *FragmentCountTransfer;
  vtkOpenGLFenceSync *FragmentCountFence;

  vtkFrameBufferObject2 *Framebuffer;
  vtkTextureObject *Textures[NumberOfTextures];

  TextureName FrontSource; // The current front source buffer
  TextureName FrontDestination; // The current front destination buffer
  TextureName DepthSource; // The current depth source buffer
  TextureName DepthDestination; // The current depth destination buffer

  ShaderStage CurrentStage;
  vtkTimeStamp CurrentStageTimeStamp;

  vtkNew<vtkOpenGLOcclusionQueryQueue> QueryQueue;
  int LastFramePassCount;

  // Note that these are not necessarily the full depth complexity of the
  // scene, they're just the depth complexity needed to hit the occlusion
  // thresholds.
  int DepthComplexity; // determined by reading the pixel buffer, -1 = uninit'd
  int DepthComplexityPasses; // Number of passes req based on depth complexity

  int CurrentPeel;
  unsigned int WrittenPixels;
  unsigned int OcclusionThreshold;

  int RenderCount; // Debug info, counts number of geometry passes.

private:
  vtkDualDepthPeelingPass(const vtkDualDepthPeelingPass&); // Not implemented
  void operator=(const vtkDualDepthPeelingPass&); // Not implemented
};

#endif // vtkDualDepthPeelingPass_h
