/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractSelectedFastIds.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkExtractSelectedFastIds.h"
#include "vtkExtractSelectedIdUtils.h"

#include "vtkArrayDispatch.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCellType.h"
#include "vtkDataArrayRange.h"
#include "vtkExtractCells.h"
#include "vtkIdList.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSignedCharArray.h"
#include "vtkSmartPointer.h"
#include "vtkSortDataArray.h"
#include "vtkStdString.h"
#include "vtkStringArray.h"
#include "vtkUnstructuredGrid.h"

vtkStandardNewMacro(vtkExtractSelectedFastIds);

//----------------------------------------------------------------------------
vtkExtractSelectedFastIds::vtkExtractSelectedFastIds()
{
  this->SetNumberOfInputPorts(2);
}

//----------------------------------------------------------------------------
vtkExtractSelectedFastIds::~vtkExtractSelectedFastIds() = default;

//----------------------------------------------------------------------------
int vtkExtractSelectedFastIds::ExtractCells(
  vtkSelectionNode* sel, vtkDataSet* input, vtkDataSet* output)
{
  int passThrough = 0;
  if (this->PreserveTopology)
  {
    passThrough = 1;
  }

  int invert = 0;
  if (sel->GetProperties()->Has(vtkSelectionNode::INVERSE()))
  {
    invert = sel->GetProperties()->Get(vtkSelectionNode::INVERSE());
  }

  signed char flagDefault = invert ? 1 : -1;
  signed char flagActive = invert ? -1 : 1;

  vtkIdType numPts = input->GetNumberOfPoints();

  vtkIdType numCells = input->GetNumberOfCells();
  vtkSmartPointer<vtkSignedCharArray> cellInArray = vtkSmartPointer<vtkSignedCharArray>::New();
  cellInArray->SetNumberOfComponents(1);
  cellInArray->SetNumberOfTuples(numCells);
  for (vtkIdType i = 0; i < numCells; i++)
  {
    cellInArray->SetValue(i, flagDefault);
  }

  if (passThrough)
  {
    output->ShallowCopy(input);

    cellInArray->SetName(this->GetTopologyFilterOutputArrayName());
    vtkCellData* outCD = output->GetCellData();
    outCD->AddArray(cellInArray);
  }

  // If there are no cells in the input, there is nothing to extract
  if (input->GetCellData()->GetNumberOfTuples() == 0)
  {
    return 1;
  }

  // decide what the IDS mean
  vtkIdTypeArray* labelArray = nullptr;
  int selType = sel->GetProperties()->Get(vtkSelectionNode::CONTENT_TYPE());
  if (selType == vtkSelectionNode::GLOBALIDS)
  {
    labelArray = vtkArrayDownCast<vtkIdTypeArray>(input->GetCellData()->GetGlobalIds());
  }
  else if (selType == vtkSelectionNode::PEDIGREEIDS)
  {
    labelArray = vtkArrayDownCast<vtkIdTypeArray>(input->GetCellData()->GetPedigreeIds());
  }
  else if (selType == vtkSelectionNode::VALUES && sel->GetSelectionList()->GetName())
  {
    // user chose a specific label array
    labelArray = vtkArrayDownCast<vtkIdTypeArray>(
      input->GetCellData()->GetAbstractArray(sel->GetSelectionList()->GetName()));
  }
  else
  {
    vtkErrorMacro("Invalid Label Array Content Type");
    return 1;
  }

  if (labelArray == nullptr)
  {
    vtkErrorMacro("Undefined Label Array");
    return 1;
  }

  // Get the list of cells included
  vtkSignedCharArray* idArray = vtkArrayDownCast<vtkSignedCharArray>(sel->GetSelectionList());
  if (idArray == nullptr)
  {
    vtkErrorMacro("Selection List is Invalid");
    return 1;
  }

  vtkSignedCharArray* previousCellInArray = vtkArrayDownCast<vtkSignedCharArray>(
    input->GetCellData()->GetArray(this->GetTopologyFilterInputArrayName()));
  vtkIdType numPreviousCellInArray =
    (previousCellInArray != nullptr ? previousCellInArray->GetNumberOfValues() : 0);

  vtkIdType numSelectionIds = idArray->GetNumberOfTuples();
  vtkIdType numLabels = labelArray->GetNumberOfTuples();
  if (numLabels > cellInArray->GetNumberOfTuples())
  {
    vtkErrorMacro("Number of IDs (" << numLabels << ") and number of cells ("
                                    << cellInArray->GetNumberOfTuples() << ") mismatched.");
    numLabels = cellInArray->GetNumberOfTuples();
  }

  for (vtkIdType i = 0; i < numLabels; i++)
  {
    vtkIdType label = labelArray->GetValue(i);
    if (label >= 0 && label < numSelectionIds)
    {
      bool passedByPreviousFilter = (previousCellInArray == nullptr) ||
        (i < numPreviousCellInArray && previousCellInArray->GetValue(i) == 1);
      bool passedByThisFilter = idArray->GetValue(label) == 1;

      if (passedByThisFilter && passedByPreviousFilter)
      {
        cellInArray->SetValue(i, flagActive);
      }
    }
  }

  if (!passThrough)
  {
    vtkIdType* pointMap = new vtkIdType[numPts]; // maps old point ids into new
    vtkExtractSelectedIdsCopyPoints(input, output, nullptr, pointMap);
    this->UpdateProgress(0.75);
    if (output->GetDataObjectType() == VTK_POLY_DATA)
    {
      vtkExtractSelectedIdsCopyCells<vtkPolyData>(
        input, vtkPolyData::SafeDownCast(output), cellInArray->GetPointer(0), pointMap);
    }
    else
    {
      vtkExtractSelectedIdsCopyCells<vtkUnstructuredGrid>(
        input, vtkUnstructuredGrid::SafeDownCast(output), cellInArray->GetPointer(0), pointMap);
    }
    delete[] pointMap;
    this->UpdateProgress(1.0);
  }

  output->Squeeze();

  return 1;
}
