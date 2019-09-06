/*===========================================================================*/
/* Distributed under OSI-approved BSD 3-Clause License.                      */
/* For copyright, see the following accompanying files or https://vtk.org:   */
/* - VTK-Copyright.txt                                                       */
/* - Sandia-Copyright.txt                                                    */
/*===========================================================================*/

#include "vtkIcicleView.h"

#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkStackedTreeLayoutStrategy.h"
#include "vtkTreeMapToPolyData.h"

vtkStandardNewMacro(vtkIcicleView);
//----------------------------------------------------------------------------
vtkIcicleView::vtkIcicleView()
{
  vtkSmartPointer<vtkStackedTreeLayoutStrategy> strategy =
    vtkSmartPointer<vtkStackedTreeLayoutStrategy>::New();
  double shrink = this->GetShrinkPercentage();
  strategy->SetUseRectangularCoordinates(true);
  strategy->SetRootStartAngle(0.0);
  strategy->SetRootEndAngle(15.0);
  strategy->SetReverse(true);
  strategy->SetShrinkPercentage(shrink);
  this->SetLayoutStrategy(strategy);
  vtkSmartPointer<vtkTreeMapToPolyData> poly =
    vtkSmartPointer<vtkTreeMapToPolyData>::New();
  this->SetAreaToPolyData(poly);
  this->SetUseRectangularCoordinates(true);
}

//----------------------------------------------------------------------------
vtkIcicleView::~vtkIcicleView() = default;

//----------------------------------------------------------------------------
void vtkIcicleView::SetTopToBottom(bool reversed)
{
  vtkStackedTreeLayoutStrategy* st =
    vtkStackedTreeLayoutStrategy::SafeDownCast(this->GetLayoutStrategy());
  if (st)
  {
    st->SetReverse(reversed);
  }
}

//----------------------------------------------------------------------------
bool vtkIcicleView::GetTopToBottom()
{
  vtkStackedTreeLayoutStrategy* st =
    vtkStackedTreeLayoutStrategy::SafeDownCast(this->GetLayoutStrategy());
  if (st)
  {
    return st->GetReverse();
  }
  return false;
}

//----------------------------------------------------------------------------
void vtkIcicleView::SetRootWidth(double width)
{
  vtkStackedTreeLayoutStrategy* st =
    vtkStackedTreeLayoutStrategy::SafeDownCast(this->GetLayoutStrategy());
  if (st)
  {
    st->SetRootStartAngle(0.0);
    st->SetRootEndAngle(width);
  }
}

//----------------------------------------------------------------------------
double vtkIcicleView::GetRootWidth()
{
  vtkStackedTreeLayoutStrategy* st =
    vtkStackedTreeLayoutStrategy::SafeDownCast(this->GetLayoutStrategy());
  if (st)
  {
    return st->GetRootEndAngle();
  }
  return 0.0;
}

//----------------------------------------------------------------------------
void vtkIcicleView::SetLayerThickness(double thickness)
{
  vtkStackedTreeLayoutStrategy* st =
    vtkStackedTreeLayoutStrategy::SafeDownCast(this->GetLayoutStrategy());
  if (st)
  {
    st->SetRingThickness(thickness);
  }
}

//----------------------------------------------------------------------------
double vtkIcicleView::GetLayerThickness()
{
  vtkStackedTreeLayoutStrategy* st =
    vtkStackedTreeLayoutStrategy::SafeDownCast(this->GetLayoutStrategy());
  if (st)
  {
    return st->GetRingThickness();
  }
  return 0.0;
}

//----------------------------------------------------------------------------
void vtkIcicleView::SetUseGradientColoring(bool value)
{
  vtkTreeMapToPolyData* tm =
    vtkTreeMapToPolyData::SafeDownCast(this->GetAreaToPolyData());
  if (tm)
  {
    tm->SetAddNormals(value);
  }
}

//----------------------------------------------------------------------------
bool vtkIcicleView::GetUseGradientColoring()
{
  vtkTreeMapToPolyData* tm =
    vtkTreeMapToPolyData::SafeDownCast(this->GetAreaToPolyData());
  if (tm)
  {
    return tm->GetAddNormals();
  }
  return false;
}

//----------------------------------------------------------------------------
void vtkIcicleView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

