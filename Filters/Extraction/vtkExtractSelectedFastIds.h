/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractSelectedFastIds.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkExtractSelectedFastIds
 * @brief   extract a list of cells from a dataset
 *
 * vtkExtractSelectedFastIds extracts a set of cells and points from within a
 * vtkDataSet. The set of ids to extract are listed within a vtkSelection.
 *
 * Designed for use with PreserveTopology as a higher performance filter for
 * selecting cells by ID when that id can be specified by a dense lookup table
 * array of boolean values.
 *
 * The filter will get the selection of GLOBALIDS, PEDIGREEIDS, or VALUES
 * add look up that value in the boolean (1, or -1) selection list array to
 * determine if the cell should be filtered or not.
 *
 * @sa
 * vtkSelection vtkExtractSelection vtkExtractSelectedIds
 */

#ifndef vtkExtractSelectedFastIds_h
#define vtkExtractSelectedFastIds_h

#include "vtkExtractSelectedIds.h"
#include "vtkFiltersExtractionModule.h" // For export macro

class vtkSelection;
class vtkSelectionNode;

class VTKFILTERSEXTRACTION_EXPORT vtkExtractSelectedFastIds : public vtkExtractSelectedIds
{
public:
  static vtkExtractSelectedFastIds* New();
  vtkTypeMacro(vtkExtractSelectedFastIds, vtkExtractSelectedIds);

protected:
  vtkExtractSelectedFastIds();
  ~vtkExtractSelectedFastIds() override;

  int ExtractCells(vtkSelectionNode* sel, vtkDataSet* input, vtkDataSet* output) override;

private:
  vtkExtractSelectedFastIds(const vtkExtractSelectedFastIds&) = delete;
  void operator=(const vtkExtractSelectedFastIds&) = delete;
};

#endif
