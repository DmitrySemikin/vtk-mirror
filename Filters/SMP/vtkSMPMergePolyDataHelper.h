/*===========================================================================*/
/* Distributed under OSI-approved BSD 3-Clause License.                      */
/* For copyright, see the following accompanying files or https://vtk.org:   */
/* - VTK-Copyright.txt                                                       */
/*===========================================================================*/
/**
 * @class   vtkSMPMergePolyDataHelper
 * @brief   Utility class for merging poly data in parallel
 * This class is designed as a utility class to help merging of poly data
 * generated by filters that generate multiple polydata outputs and the associated
 * locators. It requires that the filter uses vtkSMPMergePoints which creates
 * a number of necessary data structures.
*/

#ifndef vtkSMPMergePolyDataHelper_h
#define vtkSMPMergePolyDataHelper_h

#include "vtkConfigure.h"
#include "vtkFiltersSMPModule.h"

#include <vector>

class vtkPolyData;
class vtkSMPMergePoints;
class vtkIdList;

class VTKFILTERSSMP_EXPORT vtkSMPMergePolyDataHelper
{
public:

  //@{
  /**
   * This is the data structure needed by the MergePolyData function.
   * Each input is represented by a polydata (Input), a locator generated
   * using identical binning structure (Locator) and offset structures
   * for each vtkCellArray type. These offsets allow semi-random access
   * to the cell arrays. They should store offsets to where cells start
   * in the cell arrays. Each offset can be for 1 or more cells. The finer
   * the granularity, the better the parallelism.
   */
  struct InputData
  {
    vtkPolyData* Input;
    vtkSMPMergePoints* Locator;
    vtkIdList* VertOffsets;
    vtkIdList* LineOffsets;
    vtkIdList* PolyOffsets;
  //@}

    InputData(vtkPolyData* input,
              vtkSMPMergePoints* locator,
              vtkIdList* vertOffsets,
              vtkIdList* lineOffsets,
              vtkIdList* polyOffsets) : Input(input),
                                        Locator(locator),
                                        VertOffsets(vertOffsets),
                                        LineOffsets(lineOffsets),
                                        PolyOffsets(polyOffsets)
    {
    }
  };

  /**
   * Given a vector of vtkSMPMergePolyDataHelper::InputData, it merges
   * them and returns a new vtkPolyData (which needs to be deleted by the
   * caller). Note that this function uses the first input as a temporary
   * merging target so it will be modified in place. If you need to preserve
   * it, use DeepCopy before passing to MergePolyData.
   */
  static vtkPolyData* MergePolyData(std::vector<InputData>& inputs);

protected:
  vtkSMPMergePolyDataHelper();
  ~vtkSMPMergePolyDataHelper();

private:
  vtkSMPMergePolyDataHelper(const vtkSMPMergePolyDataHelper&) = delete;
  void operator=(const vtkSMPMergePolyDataHelper&) = delete;
};

#endif
// VTK-HeaderTest-Exclude: vtkSMPMergePolyDataHelper.h
