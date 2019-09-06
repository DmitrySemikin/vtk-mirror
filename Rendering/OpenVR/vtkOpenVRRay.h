/*===========================================================================*/
/* Distributed under OSI-approved BSD 3-Clause License.                      */
/* For copyright, see the following accompanying files or https://vtk.org:   */
/* - VTK-Copyright.txt                                                       */
/*===========================================================================*/
/**
* @class   vtkOpenVRRay
* @brief   OpenVR device model

* Represents a ray shooting from a VR controller, used for pointing or picking.
*/

#ifndef vtkOpenVRRay_h
#define vtkOpenVRRay_h

#include "vtkRenderingOpenVRModule.h" // For export macro
#include "vtkObject.h"
#include "vtkOpenGLHelper.h" // ivar
#include "vtkNew.h" // for ivar
#include <openvr.h> // for ivars

class vtkOpenGLRenderWindow;
class vtkRenderWindow;
class vtkOpenGLVertexBufferObject;
class vtkMatrix4x4;


class VTKRENDERINGOPENVR_EXPORT vtkOpenVRRay : public vtkObject
{
public:
  static vtkOpenVRRay *New();
  vtkTypeMacro(vtkOpenVRRay, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  bool Build(vtkOpenGLRenderWindow *win);
  void Render(vtkOpenGLRenderWindow *win,
    vtkMatrix4x4 *poseMatrix);

  // show the model
  vtkSetMacro(Show, bool);
  vtkGetMacro(Show, bool);

  vtkSetMacro(Length, float);

  void ReleaseGraphicsResources(vtkRenderWindow *win);

protected:
  vtkOpenVRRay();
  ~vtkOpenVRRay() override;

  bool Show;
  bool Loaded;

  vtkOpenGLHelper ModelHelper;
  vtkOpenGLVertexBufferObject *ModelVBO;
  vtkNew<vtkMatrix4x4> PoseMatrix;

  float Length;

private:
  vtkOpenVRRay(const vtkOpenVRRay&) = delete;
  void operator=(const vtkOpenVRRay&) = delete;
};

#endif
