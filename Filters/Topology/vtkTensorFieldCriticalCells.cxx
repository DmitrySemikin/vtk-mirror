/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyDataAlgorithm.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTensorFieldCriticalCells.h"

#include "vtkCell.h"
#include "vtkCellArray.h"
#include "vtkDoubleArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkStructuredGrid.h"
#include <vtkArrayDispatch.h>
#include <vtkCellData.h>
#include <vtkCellTypes.h>
#include <vtkEdgeTable.h>
#include <vtkIdList.h>
#include <vtkIntArray.h>
#include <vtkPointData.h>

vtkStandardNewMacro(vtkTensorFieldCriticalCells);

//----------------------------------------------------------------------------

vtkTensorFieldCriticalCells::vtkTensorFieldCriticalCells()
{
  // number of input ports is 1
  this->SetNumberOfInputPorts(1);
  this->Field = nullptr;
}

//----------------------------------------------------------------------------

void vtkTensorFieldCriticalCells::SetEigenvectorFieldArrayName(const char* nm)
{
  this->Field = nm;
}

//----------------------------------------------------------------------------

void vtkTensorFieldCriticalCells::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------

int vtkTensorFieldCriticalCells::FillInputPortInformation(int, vtkInformation* info)
{
  // port expects a uniform grid containing an array defining the line field
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkStructuredGrid");
  return 1;
}

//----------------------------------------------------------------------------

namespace
{
struct EdgeClassificationFunctor
{

  EdgeClassificationFunctor() = delete;
  EdgeClassificationFunctor(vtkTensorFieldCriticalCells* filter, vtkStructuredGrid* inField,
    vtkIntArray* degenerateCellFlags)
    : filter_(filter)
    , inField_(inField)
    , degenerateCellFlags_(degenerateCellFlags)
  {
  }

  template <typename EigenVectorArray>
  void operator()(EigenVectorArray* eigenvectors)
  {

    using ValueType = vtk::GetAPIType<EigenVectorArray>;

    vtkDataArrayAccessor<EigenVectorArray> eigenvectorsAccessor(eigenvectors);

    const auto numberOfCells = inField_->GetNumberOfCells();
    const auto numberOfPoints = inField_->GetNumberOfPoints();

    auto edgeTable = vtkSmartPointer<vtkEdgeTable>::New();

    // According to Euler-Poincare, E = V + F - 2 (assuming genus of 0 and # of shells := 1)
    edgeTable->InitEdgeInsertion(numberOfPoints + numberOfCells - 2, 1);

    auto edgeValueFn = [](const ValueType* ev1, const ValueType* ev2) -> vtkIdType {
      const auto dotProduct = vtkMath::Dot2D(ev1, ev2);

      if (abs(dotProduct) < std::numeric_limits<ValueType>::epsilon())
      {
        vtkErrorWithObjectMacro(
          nullptr, "Eigenvectors are perpendicular. Field perturbation required.");
        return 1;
      }
      return vtkIdType(dotProduct > ValueType{ 0 }) - vtkIdType(dotProduct < ValueType{ 0 }) +
        vtkIdType(1);
    };

    auto pointIDs = vtkSmartPointer<vtkIdList>::New();

    for (vtkIdType cellIndex{ 0 }; cellIndex < numberOfCells; ++cellIndex)
    {
      pointIDs->Resize(0);
      inField_->GetCellPoints(cellIndex, pointIDs);

      vtkIdType cellValue{ 1 };

      for (vtkIdType i{ 0 }; i < 3; ++i)
      {
        const auto p1 = pointIDs->GetId(i);
        const auto p2 = pointIDs->GetId((i + 1) % 3);

        ValueType ev1[2];
        ValueType ev2[2];

        auto edgeValue{ edgeTable->IsEdge(p1, p2) };

        // Insert unique edge, if not present
        if (edgeValue == -1)
        {
          eigenvectorsAccessor.Get(p1, ev1);
          eigenvectorsAccessor.Get(p2, ev2);

          edgeValue = edgeValueFn(ev1, ev2);

          edgeTable->InsertEdge(p1, p2, edgeValue);
        }

        cellValue *= edgeValue;
      }

      degenerateCellFlags_->InsertNextValue(static_cast<int>(cellValue < 0));

      filter_->UpdateProgress((cellIndex + 1.0) / numberOfCells);
    }
  }

private:
  vtkTensorFieldCriticalCells* filter_;
  vtkStructuredGrid* inField_;
  vtkIntArray* degenerateCellFlags_;
};
}

//----------------------------------------------------------------------------

int vtkTensorFieldCriticalCells::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // obtain the input/output port info
  auto inportData = inputVector[0]->GetInformationObject(0);
  auto outportData = outputVector->GetInformationObject(0);

  auto inField = vtkStructuredGrid::SafeDownCast(inportData->Get(vtkDataObject::DATA_OBJECT()));
  auto outField = vtkStructuredGrid::SafeDownCast(outportData->Get(vtkDataObject::DATA_OBJECT()));

  // auto inCellData = inField->GetCellData();
  auto inPointData = inField->GetPointData();
  auto outCellData = outField->GetCellData();

  auto degenerateCellFlags = vtkSmartPointer<vtkIntArray>::New();
  degenerateCellFlags->SetNumberOfComponents(1);
  degenerateCellFlags->SetName("Degenerate cell flags");

  auto eigenvectors = inPointData->GetArray(Field);

  if (eigenvectors->GetNumberOfComponents() != 2)
  {
    vtkErrorMacro("The specified line field does not contain 2D vectors.");
    return 1;
  }

  auto cellTypes = vtkSmartPointer<vtkCellTypes>::New();
  inField->GetCellTypes(cellTypes);
  if (cellTypes->GetNumberOfTypes() != 1)
  {
    vtkWarningMacro("Cell types not uniform, triangulating...");
    return 1;
  }

  {
    // Do triangulation...
  }

  EdgeClassificationFunctor worker(this, inField, degenerateCellFlags);

  typedef vtkArrayDispatch::DispatchByValueType<vtkArrayDispatch::Reals> Dispatcher;

  if (!Dispatcher::Execute(eigenvectors, worker))
  {
    worker(eigenvectors);
  }

  outCellData->AddArray(degenerateCellFlags);

  return 1;
}
