/*===========================================================================*/
/* Distributed under OSI-approved BSD 3-Clause License.                      */
/* For copyright, see the following accompanying files or https://vtk.org:   */
/* - VTK-Copyright.txt                                                       */
/*===========================================================================*/
/**
 * @class   vtkExtractSelectedIds
 * @brief   extract a list of cells from a dataset
 *
 * vtkExtractSelectedIds extracts a set of cells and points from within a
 * vtkDataSet. The set of ids to extract are listed within a vtkSelection.
 * This filter adds a scalar array called vtkOriginalCellIds that says what
 * input cell produced each output cell. This is an example of a Pedigree ID
 * which helps to trace back results. Depending on whether the selection has
 * GLOBALIDS, VALUES or INDICES, the selection will use the contents of the
 * array named in the GLOBALIDS DataSetAttribute, and arbitrary array, or the
 * position (tuple id or number) within the cell or point array.
 * @sa
 * vtkSelection vtkExtractSelection
*/

#ifndef vtkExtractSelectedIds_h
#define vtkExtractSelectedIds_h

#include "vtkFiltersExtractionModule.h" // For export macro
#include "vtkExtractSelectionBase.h"

class vtkSelection;
class vtkSelectionNode;

class VTKFILTERSEXTRACTION_EXPORT vtkExtractSelectedIds : public vtkExtractSelectionBase
{
public:
  static vtkExtractSelectedIds *New();
  vtkTypeMacro(vtkExtractSelectedIds, vtkExtractSelectionBase);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkExtractSelectedIds();
  ~vtkExtractSelectedIds() override;

  // Overridden to indicate that the input must be a vtkDataSet.
  int FillInputPortInformation(int port, vtkInformation* info) override;

  // Usual data generation method
  int RequestData(vtkInformation *,
                          vtkInformationVector **,
                          vtkInformationVector *) override;


  int ExtractCells(vtkSelectionNode *sel, vtkDataSet *input,
                   vtkDataSet *output);
  int ExtractPoints(vtkSelectionNode *sel, vtkDataSet *input,
                    vtkDataSet *output);

private:
  vtkExtractSelectedIds(const vtkExtractSelectedIds&) = delete;
  void operator=(const vtkExtractSelectedIds&) = delete;
};

#endif
