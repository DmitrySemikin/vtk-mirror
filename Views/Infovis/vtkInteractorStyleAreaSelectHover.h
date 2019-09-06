/*===========================================================================*/
/* Distributed under OSI-approved BSD 3-Clause License.                      */
/* For copyright, see the following accompanying files or https://vtk.org:   */
/* - VTK-Copyright.txt                                                       */
/* - Sandia-Copyright.txt                                                    */
/*===========================================================================*/
/**
 * @class   vtkInteractorStyleAreaSelectHover
 * @brief   An interactor style for an area tree view
 *
 *
 * The vtkInteractorStyleAreaSelectHover specifically works with pipelines
 * that create a hierarchical tree.  Such pipelines will have a vtkAreaLayout
 * filter which must be passed to this interactor style for it to function
 * correctly.
 * This interactor style allows only 2D panning and zooming,
 * rubber band selection and provides a balloon containing the name of the
 * vertex hovered over.
*/

#ifndef vtkInteractorStyleAreaSelectHover_h
#define vtkInteractorStyleAreaSelectHover_h

#include "vtkViewsInfovisModule.h" // For export macro
#include "vtkInteractorStyleRubberBand2D.h"

class vtkAreaLayout;
class vtkBalloonRepresentation;
class vtkPoints;
class vtkRenderer;
class vtkTree;
class vtkWorldPointPicker;
class vtkPolyData;

class VTKVIEWSINFOVIS_EXPORT vtkInteractorStyleAreaSelectHover : public vtkInteractorStyleRubberBand2D
{
public:
  static vtkInteractorStyleAreaSelectHover* New();
  vtkTypeMacro(vtkInteractorStyleAreaSelectHover,vtkInteractorStyleRubberBand2D);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Must be set to the vtkAreaLayout used to compute the bounds of
   * each vertex.
   */
  void SetLayout(vtkAreaLayout* layout);
  vtkGetObjectMacro(Layout, vtkAreaLayout);
  //@}

  //@{
  /**
   * The name of the field to use when displaying text in the hover balloon.
   */
  vtkSetStringMacro(LabelField);
  vtkGetStringMacro(LabelField);
  //@}

  //@{
  /**
   * Determine whether or not to use rectangular coordinates instead of
   * polar coordinates.
   */
  vtkSetMacro(UseRectangularCoordinates, bool);
  vtkGetMacro(UseRectangularCoordinates, bool);
  vtkBooleanMacro(UseRectangularCoordinates, bool);
  //@}

  /**
   * Overridden from vtkInteractorStyleImage to provide the desired
   * interaction behavior.
   */
  void OnMouseMove() override;

  /**
   * Set the interactor that this interactor style works with.
   */
  void SetInteractor(vtkRenderWindowInteractor *rwi) override;

  /**
   * Set the color used to highlight the hovered vertex.
   */
  void SetHighLightColor(double r, double g, double b);

  //@{
  /**
   * The width of the line around the hovered vertex.
   */
  void SetHighLightWidth(double lw);
  double GetHighLightWidth();
  //@}

  /**
   * Obtain the tree vertex id at the position specified.
   */
  vtkIdType GetIdAtPos(int x, int y);

protected:
  vtkInteractorStyleAreaSelectHover();
  ~vtkInteractorStyleAreaSelectHover() override;

private:
  vtkInteractorStyleAreaSelectHover(const vtkInteractorStyleAreaSelectHover&) = delete;
  void operator=(const vtkInteractorStyleAreaSelectHover&) = delete;

  // These methods are used internally
  void GetBoundingAreaForItem(vtkIdType id, float *sinfo);

  vtkWorldPointPicker* Picker;
  vtkBalloonRepresentation* Balloon;
  vtkPolyData *HighlightData;
  vtkActor *HighlightActor;
  vtkAreaLayout* Layout;
  char *LabelField;
  bool UseRectangularCoordinates;
};

#endif
