/*===========================================================================*/
/* Distributed under OSI-approved BSD 3-Clause License.                      */
/* For copyright, see the following accompanying files or https://vtk.org:   */
/* - VTK-Copyright.txt                                                       */
/*===========================================================================*/

#include "vtkPolyDataSourceWidget.h"

#include "vtkDataSet.h"
#include "vtkProp3D.h"



vtkPolyDataSourceWidget::vtkPolyDataSourceWidget() : vtk3DWidget()
{
  // child classes should call this constructor so that the vtk3DWidget()
  // constructor can set up some pertinent variables (e.g. Input and Prop3D)
}

void vtkPolyDataSourceWidget::PlaceWidget()
{
  double bounds[6];

  if ( this->Prop3D )
  {
    this->Prop3D->GetBounds(bounds);
  }
  else if ( this->GetInput() )
  {
    this->UpdateInput();
    this->GetInput()->GetBounds(bounds);
  }
  else
  {
    // if Prop3D and Input aren't set, we assume that we're going to
    // look at what the user has already done with our polydata (and this
    // should happen in the child PlaceWidget(bounds), but we have to setup
    // some defaults for misbehaving child classes
    bounds[0] = -1.0;
    bounds[1] = 1.0;
    bounds[2] = -1.0;
    bounds[3] = 1.0;
    bounds[4] = -1.0;
    bounds[5] = 1.0;
  }

  this->PlaceWidget(bounds);
}

void vtkPolyDataSourceWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
