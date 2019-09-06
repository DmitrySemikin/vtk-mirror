/*===========================================================================*/
/* Distributed under OSI-approved BSD 3-Clause License.                      */
/* For copyright, see the following accompanying files or https://vtk.org:   */
/* - VTK-Copyright.txt                                                       */
/*===========================================================================*/
/**
 * @class   vtkContextView
 * @brief   provides a view of the vtkContextScene.
 *
 *
 * This class is derived from vtkRenderViewBase and provides a view of a
 * vtkContextScene, with a default interactor style, renderer etc. It is
 * the simplest way to create a vtkRenderWindow and display a 2D scene inside
 * of it.
 *
 * By default the scene has a white background.
*/

#ifndef vtkContextView_h
#define vtkContextView_h

#include "vtkViewsContext2DModule.h" // For export macro
#include "vtkRenderViewBase.h"
#include "vtkSmartPointer.h" // Needed for SP ivars

class vtkContext2D;
class vtkContextScene;

class VTKVIEWSCONTEXT2D_EXPORT vtkContextView : public vtkRenderViewBase
{
public:
  void PrintSelf(ostream& os, vtkIndent indent) override;
  vtkTypeMacro(vtkContextView, vtkRenderViewBase);

  static vtkContextView* New();

  /**
   * Set the vtkContext2D for the view.
   */
  virtual void SetContext(vtkContext2D *context);

  /**
   * Get the vtkContext2D for the view.
   */
  virtual vtkContext2D* GetContext();

  /**
   * Set the scene object for the view.
   */
  virtual void SetScene(vtkContextScene *scene);

  /**
   * Get the scene of the view.
   */
  virtual vtkContextScene* GetScene();

protected:
  vtkContextView();
  ~vtkContextView() override;

  vtkSmartPointer<vtkContextScene> Scene;
  vtkSmartPointer<vtkContext2D> Context;

private:
  vtkContextView(const vtkContextView&) = delete;
  void operator=(const vtkContextView&) = delete;
};

#endif
