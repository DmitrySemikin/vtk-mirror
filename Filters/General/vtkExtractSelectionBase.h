/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractSelectionBase.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkExtractSelectionBase
 * @brief   abstract base class for all extract selection
 * filters.
 *
 * vtkExtractSelectionBase is an abstract base class for all extract selection
 * filters. It defines some properties common to all extract selection filters.
 */

#ifndef vtkExtractSelectionBase_h
#define vtkExtractSelectionBase_h

#include "vtkDataObjectAlgorithm.h"
#include "vtkFiltersGeneralModule.h" // For export macro

class VTKFILTERSGENERAL_EXPORT vtkExtractSelectionBase : public vtkDataObjectAlgorithm
{
public:
  vtkTypeMacro(vtkExtractSelectionBase, vtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Convenience method to specify the selection connection (2nd input
   * port)
   */
  void SetSelectionConnection(vtkAlgorithmOutput* algOutput)
  {
    this->SetInputConnection(1, algOutput);
  }

  //@{
  /**
   * This flag tells the extraction filter not to convert the selected
   * output into an unstructured grid, but instead to produce a vtkInsidedness
   * array and add it to the input dataset.
   *
   * A significant performance improvement as it prevents copying memory
   * unnecessarily. It needs to be paired with a filter which will read
   * the insidedness array and filter out points or cells as part of an
   * operation which is already iteratively traversing the data.
   *
   * Default value is false(0).
   */
  vtkSetMacro(PreserveTopology, vtkTypeBool);
  vtkGetMacro(PreserveTopology, vtkTypeBool);
  vtkBooleanMacro(PreserveTopology, vtkTypeBool);
  //@}

  //@{
  /**
   * The name for the insidedness array from a previous filter which should be
   * combined with the result of this filter.
   *
   * This is useful when stacking multiple filters in sequence, as filters
   * generating these arrays often pass data with shallow copies for performance
   * reasons. Then each filter needs to create it's own insidedness array
   * with the result up to that point. If only a later filter is modified,
   * that later filter then still has the previous result, and doesn't
   * accidentally recycle its own previous results into a later calculation.
   *
   * Default value is nullptr(0).
   */
  vtkSetStringMacro(TopologyFilterInputArrayName);
  vtkGetStringMacro(TopologyFilterInputArrayName);
  //@}

  //@{
  /**
   * The name for the resulting insidedness array defining which points or cells
   * should be filtered from the dataset. Default value is "vtkInsidedness".
   */
  vtkSetStringMacro(TopologyFilterOutputArrayName);
  vtkGetStringMacro(TopologyFilterOutputArrayName);
  //@}

protected:
  vtkExtractSelectionBase();
  ~vtkExtractSelectionBase() override;

  /**
   * Sets up empty output dataset
   */
  int RequestDataObject(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  int FillInputPortInformation(int port, vtkInformation* info) override;

  vtkTypeBool PreserveTopology;
  char* TopologyFilterInputArrayName;
  char* TopologyFilterOutputArrayName;

private:
  vtkExtractSelectionBase(const vtkExtractSelectionBase&) = delete;
  void operator=(const vtkExtractSelectionBase&) = delete;
};

#endif
