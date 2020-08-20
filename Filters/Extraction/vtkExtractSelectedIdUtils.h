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

#include "vtkExtractSelectionBase.h"

#include "vtkIdTypeArray.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"

// Copy the points marked as "in" and build a pointmap
static void vtkExtractSelectedIdsCopyPoints(
  vtkDataSet* input, vtkDataSet* output, signed char* inArray, vtkIdType* pointMap)
{
  vtkPoints* newPts = vtkPoints::New();

  vtkIdType i, numPts = input->GetNumberOfPoints();

  vtkIdTypeArray* originalPtIds = vtkIdTypeArray::New();
  originalPtIds->SetNumberOfComponents(1);
  originalPtIds->SetName("vtkOriginalPointIds");

  vtkPointData* inPD = input->GetPointData();
  vtkPointData* outPD = output->GetPointData();
  outPD->SetCopyGlobalIds(1);
  outPD->CopyAllocate(inPD);

  for (i = 0; i < numPts; i++)
  {
    if (inArray == nullptr || inArray[i] > 0)
    {
      pointMap[i] = newPts->InsertNextPoint(input->GetPoint(i));
      outPD->CopyData(inPD, i, pointMap[i]);
      originalPtIds->InsertNextValue(i);
    }
    else
    {
      pointMap[i] = -1;
    }
  }

  outPD->AddArray(originalPtIds);
  originalPtIds->Delete();

  // outputDS must be either vtkPolyData or vtkUnstructuredGrid
  vtkPointSet::SafeDownCast(output)->SetPoints(newPts);
  newPts->Delete();
}

// Copy the cells marked as "in" using the given pointmap
template <class T>
void vtkExtractSelectedIdsCopyCells(
  vtkDataSet* input, T* output, signed char* inArray, vtkIdType* pointMap)
{
  vtkIdType numCells = input->GetNumberOfCells();
  output->AllocateEstimate(numCells / 4, 1);

  vtkCellData* inCD = input->GetCellData();
  vtkCellData* outCD = output->GetCellData();
  outCD->SetCopyGlobalIds(1);
  outCD->CopyAllocate(inCD);

  vtkIdTypeArray* originalIds = vtkIdTypeArray::New();
  originalIds->SetNumberOfComponents(1);
  originalIds->SetName("vtkOriginalCellIds");

  vtkIdType i, j, newId = 0;
  vtkIdList* ptIds = vtkIdList::New();
  for (i = 0; i < numCells; i++)
  {
    if (inArray[i] > 0)
    {
      // special handling for polyhedron cells
      if (vtkUnstructuredGrid::SafeDownCast(input) && vtkUnstructuredGrid::SafeDownCast(output) &&
        input->GetCellType(i) == VTK_POLYHEDRON)
      {
        ptIds->Reset();
        vtkUnstructuredGrid::SafeDownCast(input)->GetFaceStream(i, ptIds);
        vtkUnstructuredGrid::ConvertFaceStreamPointIds(ptIds, pointMap);
      }
      else
      {
        input->GetCellPoints(i, ptIds);
        for (j = 0; j < ptIds->GetNumberOfIds(); j++)
        {
          ptIds->SetId(j, pointMap[ptIds->GetId(j)]);
        }
      }
      output->InsertNextCell(input->GetCellType(i), ptIds);
      outCD->CopyData(inCD, i, newId++);
      originalIds->InsertNextValue(i);
    }
  }

  outCD->AddArray(originalIds);
  originalIds->Delete();
  ptIds->Delete();
}
