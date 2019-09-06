/*===========================================================================*/
/* Distributed under OSI-approved BSD 3-Clause License.                      */
/* For copyright, see the following accompanying files or https://vtk.org:   */
/* - VTKm-Copyright.txt                                                      */
/*===========================================================================*/
/**
 * @class   vtkMultiBlockMergeFilter
 * @brief   merges multiblock inputs into a single multiblock output
 *
 * vtkMultiBlockMergeFilter is an M to 1 filter similar to
 * vtkMultiBlockDataGroupFilter. However where as that class creates N groups
 * in the output for N inputs, this creates 1 group in the output with N
 * datasets inside it. In actuality if the inputs have M blocks, this will
 * produce M blocks, each of which has N datasets. Inside the merged group,
 * the i'th data set comes from the i'th data set in the i'th input.
*/

#ifndef vtkMultiBlockMergeFilter_h
#define vtkMultiBlockMergeFilter_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkMultiBlockDataSetAlgorithm.h"

class VTKFILTERSGENERAL_EXPORT vtkMultiBlockMergeFilter
: public vtkMultiBlockDataSetAlgorithm
{
public:
  vtkTypeMacro(vtkMultiBlockMergeFilter,vtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Construct object with PointIds and CellIds on; and ids being generated
   * as scalars.
   */
  static vtkMultiBlockMergeFilter *New();

  //@{
  /**
   * Assign a data object as input. Note that this method does not
   * establish a pipeline connection. Use AddInputConnection() to
   * setup a pipeline connection.
   */
  void AddInputData(vtkDataObject *);
  void AddInputData(int, vtkDataObject*);
  //@}

protected:
  vtkMultiBlockMergeFilter();
  ~vtkMultiBlockMergeFilter() override;

  int RequestData(vtkInformation *,
                  vtkInformationVector **,
                  vtkInformationVector *) override;

  int FillInputPortInformation(int port, vtkInformation *info) override;

  int IsMultiPiece(vtkMultiBlockDataSet*);

  int Merge(unsigned int numPieces, unsigned int pieceNo,
    vtkMultiBlockDataSet* output,
    vtkMultiBlockDataSet* input);
private:
  vtkMultiBlockMergeFilter(const vtkMultiBlockMergeFilter&) = delete;
  void operator=(const vtkMultiBlockMergeFilter&) = delete;
};

#endif


