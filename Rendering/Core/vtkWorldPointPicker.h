/*===========================================================================*/
/* Distributed under OSI-approved BSD 3-Clause License.                      */
/* For copyright, see the following accompanying files or https://vtk.org:   */
/* - VTK-Copyright.txt                                                       */
/*===========================================================================*/
/**
 * @class   vtkWorldPointPicker
 * @brief   find world x,y,z corresponding to display x,y,z
 *
 * vtkWorldPointPicker is used to find the x,y,z world coordinate of a
 * screen x,y,z. This picker cannot pick actors and/or mappers, it
 * simply determines an x-y-z coordinate in world space. (It will always
 * return a x-y-z, even if the selection point is not over a prop/actor.)
 *
 * @warning
 * The PickMethod() is not invoked, but StartPickMethod() and EndPickMethod()
 * are.
 *
 * @sa
 * vtkPropPicker vtkPicker vtkCellPicker vtkPointPicker
*/

#ifndef vtkWorldPointPicker_h
#define vtkWorldPointPicker_h

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkAbstractPicker.h"

class VTKRENDERINGCORE_EXPORT vtkWorldPointPicker : public vtkAbstractPicker
{
public:
  static vtkWorldPointPicker *New();
  vtkTypeMacro(vtkWorldPointPicker,vtkAbstractPicker);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Perform the pick. (This method overload's the superclass.)
   */
  int Pick(double selectionX, double selectionY, double selectionZ,
           vtkRenderer *renderer) override;
  int Pick(double selectionPt[3], vtkRenderer *renderer)
    { return this->vtkAbstractPicker::Pick( selectionPt, renderer); };
  //@}

protected:
  vtkWorldPointPicker ();
  ~vtkWorldPointPicker() override {}

private:
  vtkWorldPointPicker(const vtkWorldPointPicker&) = delete;
  void operator=(const vtkWorldPointPicker&) = delete;
};

#endif


