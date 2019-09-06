/*===========================================================================*/
/* Distributed under OSI-approved BSD 3-Clause License.                      */
/* For copyright, see the following accompanying files or https://vtk.org:   */
/* - VTK-Copyright.txt                                                       */
/*===========================================================================*/

#ifndef vtkLookupTableItem_h
#define vtkLookupTableItem_h

#include "vtkChartsCoreModule.h" // For export macro
#include "vtkScalarsToColorsItem.h"

class vtkLookupTable;

// Description:
// vtkPlot::Color, vtkPlot::Brush, vtkScalarsToColors::DrawPolyLine,
// vtkScalarsToColors::MaskAboveCurve have no effect here.
class VTKCHARTSCORE_EXPORT vtkLookupTableItem: public vtkScalarsToColorsItem
{
public:
  static vtkLookupTableItem* New();
  vtkTypeMacro(vtkLookupTableItem, vtkScalarsToColorsItem);
  void PrintSelf(ostream &os, vtkIndent indent) override;

  void SetLookupTable(vtkLookupTable* t);
  vtkGetObjectMacro(LookupTable, vtkLookupTable);

protected:
  vtkLookupTableItem();
  ~vtkLookupTableItem() override;

  // Description:
  // Reimplemented to return the range of the lookup table
  void ComputeBounds(double bounds[4]) override;


  void ComputeTexture() override;
  vtkLookupTable* LookupTable;

private:
  vtkLookupTableItem(const vtkLookupTableItem &) = delete;
  void operator=(const vtkLookupTableItem &) = delete;
};

#endif
