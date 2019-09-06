/*===========================================================================*/
/* Distributed under OSI-approved BSD 3-Clause License.                      */
/* For copyright, see the following accompanying files or https://vtk.org:   */
/* - VTK-Copyright.txt                                                       */
/*===========================================================================*/
/**
 * @class   vtkVolumetricPass
 * @brief   Render the volumetric geometry with property key
 * filtering.
 *
 * vtkVolumetricPass renders the volumetric geometry of all the props that
 * have the keys contained in vtkRenderState.
 *
 * This pass expects an initialized depth buffer and color buffer.
 * Initialized buffers means they have been cleared with farest z-value and
 * background color/gradient/transparent color.
 *
 * @sa
 * vtkRenderPass vtkDefaultPass
*/

#ifndef vtkVolumetricPass_h
#define vtkVolumetricPass_h

#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkDefaultPass.h"

class VTKRENDERINGOPENGL2_EXPORT vtkVolumetricPass : public vtkDefaultPass
{
public:
  static vtkVolumetricPass *New();
  vtkTypeMacro(vtkVolumetricPass,vtkDefaultPass);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Perform rendering according to a render state \p s.
   * \pre s_exists: s!=0
   */
  void Render(const vtkRenderState *s) override;

 protected:
  /**
   * Default constructor.
   */
  vtkVolumetricPass();

  /**
   * Destructor.
   */
  ~vtkVolumetricPass() override;

 private:
  vtkVolumetricPass(const vtkVolumetricPass&) = delete;
  void operator=(const vtkVolumetricPass&) = delete;
};

#endif
