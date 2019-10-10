/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCellTextureToPointTexture.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCellTextureToPointTexture.h"

#include "vtkAbstractTransform.h"
#include "vtkCellData.h"
#include "vtkCellTextureToPointTextureInternal.h"
#include "vtkIncrementalOctreePointLocator.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkFloatArray.h"
#include "vtkLinearTransform.h"
#include "vtkMathUtilities.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolygon.h"


#include <iterator>

vtkStandardNewMacro(vtkCellTextureToPointTexture);

vtkCellTextureToPointTexture::vtkCellTextureToPointTexture()
{
  this->FaceTextureTolerance = 0.000001;
}

vtkCellTextureToPointTexture::~vtkCellTextureToPointTexture()
{
}

int vtkCellTextureToPointTexture::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkPolyData *input = vtkPolyData::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkFloatArray* texCoordsCells = vtkArrayDownCast<vtkFloatArray>(
    input->GetCellData()->GetTCoords());
  if (texCoordsCells)
  {
    vtkSmartPointer<vtkFloatArray> texCoordsPoints =
      vtkSmartPointer<vtkFloatArray>::New();
    vtkIdType numPts = input->GetNumberOfPoints();
    texCoordsPoints->SetName("TCoords");
    texCoordsPoints->SetNumberOfComponents(2);
    texCoordsPoints->SetNumberOfTuples(numPts);
    // initialize texture coordinates with invalid value
    for (int j = 0; j < numPts; ++j)
    {
      texCoordsPoints->SetTuple2(j, -1, -1);
    }
    output->GetPointData()->SetTCoords(texCoordsPoints);


    vtkCellTextureToPointTextureInternal duplicatePoints;
    duplicatePoints.Initialize(input->GetNumberOfPoints(),
                               this->FaceTextureTolerance);
    vtkSmartPointer<vtkPoints> points;
    points.TakeReference(input->GetPoints()->NewInstance());
    output->SetPoints(points);
    output->GetPoints()->DeepCopy(input->GetPoints());
    vtkSmartPointer<vtkCellArray> polys = vtkSmartPointer<vtkCellArray>::New();
    vtkIdType numPolys = input->GetNumberOfCells();
    polys->Allocate(polys->EstimateSize(numPolys,3),numPolys/2);
    vtkNew<vtkPolygon> cell;
    for (int i = 0; i < input->GetNumberOfCells(); ++i)
    {
      vtkCell* inputCell = input->GetCell(i);
      float* texCoordsCell = texCoordsCells->GetPointer(
        i * texCoordsCells->GetNumberOfComponents());
      cell->Initialize(
        inputCell->GetNumberOfPoints(),
        inputCell->GetPointIds()->GetPointer(0), output->GetPoints());
      duplicatePoints.DuplicatePoints(
        cell, texCoordsCell, texCoordsPoints, output);
      polys->InsertNextCell(cell);
    }
    output->SetPolys(polys);
  }
  else
  {
    output->ShallowCopy(input);
  }
  return 1;
}

void vtkCellTextureToPointTexture::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

}
