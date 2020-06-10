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
#include <vtkVectorFieldTopology.h>

// VTK includes
#include <vtkAppendPolyData.h>
#include <vtkCellArray.h>
#include <vtkCellData.h>
#include <vtkDataSetTriangleFilter.h>
#include <vtkDoubleArray.h>
#include <vtkGradientFilter.h>
#include <vtkImageData.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkIntersectionPolyDataFilter.h>
#include <vtkLine.h>
#include <vtkMath.h>
#include <vtkMatrix3x3.h>
#include <vtkNew.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkProbeFilter.h>
#include <vtkQuad.h>
#include <vtkRegularPolygonSource.h>
#include <vtkRuledSurfaceFilter.h>
#include <vtkSmartPointer.h>
#include <vtkStreamSurface.h>
#include <vtkStreamTracer.h>
#include <vtkTriangle.h>
#include <vtkUnstructuredGrid.h>
#include <vtkVector.h>
#include <vtkVectorFieldTopology.h>
#include <vtkVertex.h>

// Eigen3
#include <vtk_eigen.h>
#include VTK_EIGEN(Eigenvalues)

#include <cmath>
#include <map>

#define epsilon (1e-10)

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkVectorFieldTopology);

//----------------------------------------------------------------------------
vtkVectorFieldTopology::vtkVectorFieldTopology()
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(3);
}

//----------------------------------------------------------------------------
vtkVectorFieldTopology::~vtkVectorFieldTopology() = default;

//----------------------------------------------------------------------------
void vtkVectorFieldTopology::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "MaxNumSteps =  " << this->MaxNumSteps << "\n";
  os << indent << "IntegrationStepSize =  " << this->IntegrationStepSize << "\n";
  os << indent << "SeparatrixDistance =  " << this->SeparatrixDistance << "\n";
  os << indent << "UseIterativeSeeding =  " << this->UseIterativeSeeding << "\n";
  os << indent << "ComputeSurfaces =  " << this->ComputeSurfaces << "\n";
  os << indent << "vtkStreamSurface: \n";
  this->StreamSurface->PrintSelf(os, indent.GetNextIndent());
}

//----------------------------------------------------------------------------
int vtkVectorFieldTopology::FillInputPortInformation(int port, vtkInformation* info)
{
  if (port == 0)
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
  }
  return 1;
}

//----------------------------------------------------------------------------
int vtkVectorFieldTopology::FillOutputPortInformation(int port, vtkInformation* info)
{
  if (port < 3)
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPolyData");
  }
  return 1;
}

//----------------------------------------------------------------------------
int vtkVectorFieldTopology::Classify2D(
  int vtkNotUsed(countReal), int countComplex, int countPos, int countNeg)
{
  // make simple type that corresponds to the number of positive eigenvalues
  // SOURCE2D 2, SADDLE2D 1, SINK2D 0, (CENTER2D 3)
  // in analogy to ttk, where the type corresponds to the down directions
  int critType = vtkVectorFieldTopology::DEGENERATE2D;
  if (countPos + countNeg == 2)
  {
    switch (countPos)
    {
      case 0:
        critType = vtkVectorFieldTopology::SINK2D;
        break;
      case 1:
        critType = vtkVectorFieldTopology::SADDLE2D;
        break;
      case 2:
        critType = vtkVectorFieldTopology::SOURCE2D;
        break;
      default:
        break;
    }
  }
  if (countComplex == 2)
  {
    critType = vtkVectorFieldTopology::CENTER2D;
  }
  return critType;
}

//----------------------------------------------------------------------------
int vtkVectorFieldTopology::Classify3D(
  int vtkNotUsed(countReal), int countComplex, int countPos, int countNeg)
{
  // make simple type that corresponds to the number of positive eigenvalues
  // SOURCE3D 3, SADDLE23D 2, SADDLE13D 1, SINK3D 0, (CENTER3D 4)
  // in analogy to ttk, where the type corresponds to the down directions
  int critType = vtkVectorFieldTopology::DEGENERATE3D;
  if (countComplex > 0)
  {
    critType = vtkVectorFieldTopology::CENTER3D;
  }
  if (countPos + countNeg == 3)
  {
    switch (countPos)
    {
      case 0:
        critType = vtkVectorFieldTopology::SINK3D;
        break;
      case 1:
        critType = vtkVectorFieldTopology::SADDLE13D;
        break;
      case 2:
        critType = vtkVectorFieldTopology::SADDLE23D;
        break;
      case 3:
        critType = vtkVectorFieldTopology::SOURCE3D;
        break;
      default:
        break;
    }
  }
  return critType;
}

//----------------------------------------------------------------------------
int vtkVectorFieldTopology::ComputeCriticalPoints2D(
  vtkSmartPointer<vtkPolyData> criticalPoints, vtkSmartPointer<vtkUnstructuredGrid> tridataset)
{
  for (int cellId = 0; cellId < tridataset->GetNumberOfCells(); cellId++)
  {
    auto cell = tridataset->GetCell(cellId);
    vtkIdType indices[3] = { cell->GetPointId(0), cell->GetPointId(1), cell->GetPointId(2) };

    vtkVector3d coords[3] = { vtkVector3d(tridataset->GetPoint(indices[0])),
      vtkVector3d(tridataset->GetPoint(indices[1])),
      vtkVector3d(tridataset->GetPoint(indices[2])) };

    vtkVector3d values[3] = { vtkVector3d(
                                tridataset->GetPointData()->GetVectors()->GetTuple(indices[0])),
      vtkVector3d(tridataset->GetPointData()->GetVectors()->GetTuple(indices[1])),
      vtkVector3d(tridataset->GetPointData()->GetVectors()->GetTuple(indices[2])) };

    vtkNew<vtkMatrix3x3> valueMatrix;
    for (int i = 0; i < 2; i++)
    {
      for (int j = 0; j < 2; j++)
      {
        valueMatrix->SetElement(j, i, values[i + 1][j] - values[0][j]);
      }
    }

    if (valueMatrix->Determinant() != 0)
    {
      valueMatrix->Invert();
      double zeroBase[3] = { -values[0][0], -values[0][1], -values[0][2] };
      valueMatrix->MultiplyPoint(zeroBase, zeroBase);

      double zeroPos[3] = { coords[0][0] + zeroBase[0] * (coords[1][0] - coords[0][0]) +
          zeroBase[1] * (coords[2][0] - coords[0][0]),
        coords[0][1] + zeroBase[0] * (coords[1][1] - coords[0][1]) +
          zeroBase[1] * (coords[2][1] - coords[0][1]),
        coords[0][2] + zeroBase[0] * (coords[1][2] - coords[0][2]) +
          zeroBase[1] * (coords[2][2] - coords[0][2]) };

      // Check if zeroPos is inside the cell
      if (valueMatrix->Determinant() != 0 && zeroBase[0] >= -epsilon && zeroBase[1] >= -epsilon &&
        zeroBase[0] + zeroBase[1] <= 1.0 + epsilon)
      {
        bool isNewPoint = 1;
        for (int i = 0; i < criticalPoints->GetNumberOfPoints(); ++i)
        {
          if (vtkMath::Distance2BetweenPoints(zeroPos, criticalPoints->GetPoint(i)) < epsilon)
          {
            isNewPoint = 0;
          }
        }
        if (isNewPoint)
        {
          criticalPoints->GetPoints()->InsertNextPoint(zeroPos);
          vtkNew<vtkVertex> vertex;
          vertex->GetPointIds()->SetId(0, criticalPoints->GetNumberOfPoints() - 1);
          criticalPoints->GetVerts()->InsertNextCell(vertex);
        }
      }
    }
  }
  return 1;
}

//----------------------------------------------------------------------------
int vtkVectorFieldTopology::ComputeCriticalPoints3D(
  vtkSmartPointer<vtkPolyData> criticalPoints, vtkSmartPointer<vtkUnstructuredGrid> tridataset)
{
  for (int cellId = 0; cellId < tridataset->GetNumberOfCells(); cellId++)
  {
    auto cell = tridataset->GetCell(cellId);
    vtkIdType indices[4] = { cell->GetPointId(0), cell->GetPointId(1), cell->GetPointId(2),
      cell->GetPointId(3) };

    vtkVector3d coords[4] = { vtkVector3d(tridataset->GetPoint(indices[0])),
      vtkVector3d(tridataset->GetPoint(indices[1])), vtkVector3d(tridataset->GetPoint(indices[2])),
      vtkVector3d(tridataset->GetPoint(indices[3])) };

    vtkVector3d values[4] = { vtkVector3d(
                                tridataset->GetPointData()->GetVectors()->GetTuple(indices[0])),
      vtkVector3d(tridataset->GetPointData()->GetVectors()->GetTuple(indices[1])),
      vtkVector3d(tridataset->GetPointData()->GetVectors()->GetTuple(indices[2])),
      vtkVector3d(tridataset->GetPointData()->GetVectors()->GetTuple(indices[3])) };

    vtkNew<vtkMatrix3x3> valueMatrix;
    for (int i = 0; i < 3; i++)
    {
      for (int j = 0; j < 3; j++)
      {
        valueMatrix->SetElement(j, i, values[3][j] - values[i][j]);
      }
    }

    if (valueMatrix->Determinant() != 0)
    {
      valueMatrix->Invert();
      double zeroBase[3] = { values[3][0], values[3][1], values[3][2] };
      valueMatrix->MultiplyPoint(zeroBase, zeroBase);

      double zeroPos[3] = { coords[0][0] * zeroBase[0] + coords[1][0] * zeroBase[1] +
          coords[2][0] * zeroBase[2] +
          coords[3][0] * (1.0 - zeroBase[0] - zeroBase[1] - zeroBase[2]),
        coords[0][1] * zeroBase[0] + coords[1][1] * zeroBase[1] + coords[2][1] * zeroBase[2] +
          coords[3][1] * (1.0 - zeroBase[0] - zeroBase[1] - zeroBase[2]),
        coords[0][2] * zeroBase[0] + coords[1][2] * zeroBase[1] + coords[2][2] * zeroBase[2] +
          coords[3][2] * (1.0 - zeroBase[0] - zeroBase[1] - zeroBase[2]) };

      // Check if zeroPos is inside the cell
      if (valueMatrix->Determinant() != 0 && zeroBase[0] >= -epsilon && zeroBase[1] >= -epsilon &&
        zeroBase[2] >= -epsilon && zeroBase[0] + zeroBase[1] + zeroBase[2] <= 1.0 + epsilon)
      {
        bool isNewPoint = 1;
        for (int i = 0; i < criticalPoints->GetNumberOfPoints(); ++i)
        {
          if (vtkMath::Distance2BetweenPoints(zeroPos, criticalPoints->GetPoint(i)) < epsilon)
          {
            isNewPoint = 0;
          }
        }
        if (isNewPoint)
        {
          criticalPoints->GetPoints()->InsertNextPoint(zeroPos);
          vtkNew<vtkVertex> vertex;
          vertex->GetPointIds()->SetId(0, criticalPoints->GetNumberOfPoints() - 1);
          criticalPoints->GetVerts()->InsertNextCell(vertex);
        }
      }
    }
  }
  return 1;
}

//----------------------------------------------------------------------------
int vtkVectorFieldTopology::ComputeSurface(int numberOfSeparatingSurfaces, bool isBackward,
  double normal[3], double zeroPos[3], vtkSmartPointer<vtkPolyData> streamSurfaces,
  vtkSmartPointer<vtkImageData> dataset, int vtkNotUsed(integrationStepUnit), double dist,
  double vtkNotUsed(stepSize), int maxNumSteps, bool useIterativeSeeding)
{
  // generate circle and add first point again in the back to avoid gap
  vtkNew<vtkRegularPolygonSource> circle;
  circle->GeneratePolygonOff();
  circle->SetNumberOfSides(6);
  circle->SetRadius(dist);
  circle->SetCenter(zeroPos);
  circle->SetNormal(normal);
  circle->Update();

  // close circle exactly with a point instead of an edge to correctly treat points exiting the
  // boundary
  circle->GetOutput()->GetPoints()->InsertNextPoint(circle->GetOutput()->GetPoint(0));
  vtkNew<vtkPolyData> currentCircle;
  currentCircle->SetPoints(circle->GetOutput()->GetPoints());
  vtkNew<vtkDoubleArray> integrationTimeArray;
  integrationTimeArray->SetName("IntegrationTime");
  currentCircle->GetPointData()->AddArray(integrationTimeArray);
  for (int i = 0; i < currentCircle->GetNumberOfPoints(); ++i)
  {
    integrationTimeArray->InsertNextTuple1(0);
  }

  this->StreamSurface->SetInputData(0, dataset);
  this->StreamSurface->SetInputData(1, currentCircle);
  this->StreamSurface->SetUseIterativeSeeding(useIterativeSeeding);
  this->StreamSurface->SetIntegratorTypeToRungeKutta4();
  this->StreamSurface->SetIntegrationStepUnit(this->IntegrationStepUnit);
  this->StreamSurface->SetInitialIntegrationStep(this->IntegrationStepSize);
  this->StreamSurface->SetIntegrationDirection(isBackward);
  this->StreamSurface->SetComputeVorticity(0);
  this->StreamSurface->SetMaximumNumberOfSteps(maxNumSteps);
  this->StreamSurface->SetSourceData(currentCircle);
  this->StreamSurface->SetMaximumPropagation(dist * maxNumSteps);
  this->StreamSurface->Update();

  for (int i = 0; i < StreamSurface->GetOutput()->GetNumberOfPoints(); ++i)
  {
    StreamSurface->GetOutput()->GetPointData()->GetArray("index")->SetTuple1(
      i, numberOfSeparatingSurfaces);
  }

  // add current surface to existing surfaces
  vtkNew<vtkAppendPolyData> appendSurfaces;
  appendSurfaces->AddInputData(this->StreamSurface->GetOutput());
  appendSurfaces->AddInputData(streamSurfaces);
  appendSurfaces->Update();
  streamSurfaces->DeepCopy(appendSurfaces->GetOutput());

  return (streamSurfaces != nullptr);
}

//----------------------------------------------------------------------------
int vtkVectorFieldTopology::ComputeSeparatrices(vtkSmartPointer<vtkPolyData> criticalPoints,
  vtkSmartPointer<vtkPolyData> separatrices, vtkSmartPointer<vtkPolyData> surfaces,
  vtkSmartPointer<vtkImageData> dataset, vtkSmartPointer<vtkImageData> graddataset,
  int integrationStepUnit, double dist, double stepSize, int maxNumSteps, bool computeSurfaces,
  bool useIterativeSeeding)
{
  // adapt dist if cell unit was selected
  if (integrationStepUnit == vtkStreamTracer::CELL_LENGTH_UNIT)
  {
    dist *= sqrt(static_cast<double>(dataset->GetCell(0)->GetLength2()));
  }

  // Compute eigenvectors & eigenvalues
  vtkNew<vtkDoubleArray> criticalPointsTypes;
  criticalPointsTypes->SetNumberOfTuples(criticalPoints->GetNumberOfPoints());
  criticalPointsTypes->SetName("type");
  criticalPoints->GetPointData()->AddArray(criticalPointsTypes);

  vtkNew<vtkProbeFilter> probe;
  probe->SetInputData(criticalPoints);
  probe->SetSourceData(graddataset);
  probe->Update();

  vtkNew<vtkStreamTracer> streamTracer;
  streamTracer->SetInputData(dataset);
  streamTracer->SetIntegratorTypeToRungeKutta4();
  streamTracer->SetIntegrationStepUnit(this->IntegrationStepUnit);
  streamTracer->SetInitialIntegrationStep(this->IntegrationStepSize);
  streamTracer->SetComputeVorticity(0);
  streamTracer->SetMaximumNumberOfSteps(maxNumSteps);
  streamTracer->SetMaximumPropagation(dist * maxNumSteps);
  streamTracer->SetTerminalSpeed(epsilon);

  int numberOfSeparatingLines = 0;
  int numberOfSeparatingSurfaces = 0;

  for (int pointId = 0; pointId < criticalPoints->GetNumberOfPoints(); pointId++)
  {
    // classification
    Eigen::Matrix<double, 3, 3> eigenMatrix;
    int k = 0;
    for (int i = 0; i < 3; i++)
    {
      for (int j = 0; j < 3; j++)
      {
        eigenMatrix(i, j) =
          probe->GetOutput()->GetPointData()->GetArray("gradient")->GetTuple(pointId)[k];
        k++;
      }
    }

    Eigen::EigenSolver<Eigen::Matrix<double, 3, 3>> eigenS(eigenMatrix);

    int countReal = 0;
    int countComplex = 0;
    int countPos = 0;
    int countNeg = 0;
    for (int i = 0; i < dataset->GetDataDimension(); i++)
    {
      if (imag(eigenS.eigenvalues()[i]) == 0.0)
      {
        countReal++;
      }
      else
      {
        countComplex++;
      }

      if (real(eigenS.eigenvalues()[i]) < -epsilon)
      {
        countNeg++;
      }
      else if (real(eigenS.eigenvalues()[i]) > epsilon)
      {
        countPos++;
      }
    }
    if (dataset->GetDataDimension() == 2)
    {
      criticalPoints->GetPointData()->GetArray("type")->SetTuple1(
        pointId, Classify2D(countReal, countComplex, countPos, countNeg));
    }
    else
    {
      criticalPoints->GetPointData()->GetArray("type")->SetTuple1(
        pointId, Classify3D(countReal, countComplex, countPos, countNeg));
    }

    // separatrix
    if (criticalPoints->GetPointData()->GetArray("type")->GetTuple1(pointId) == 1 ||
      (dataset->GetDataDimension() == 3 &&
        criticalPoints->GetPointData()->GetArray("type")->GetTuple1(pointId) == 2))
    {
      for (int i = 0; i < dataset->GetDataDimension(); i++)
      {

        double normal[] = { real(eigenS.eigenvectors().col(i)[0]),
          real(eigenS.eigenvectors().col(i)[1]), real(eigenS.eigenvectors().col(i)[2]) };

        bool isForward = real(eigenS.eigenvalues()[i]) > 0 && countPos == 1;
        bool isBackward = real(eigenS.eigenvalues()[i]) < 0 && countNeg == 1;
        if (isForward or isBackward)
        {
          // insert two seeds
          for (int j = 0; j < 2; j++)
          {
            // insert seed with small offset
            vtkNew<vtkPolyData> seeds;
            vtkNew<vtkPoints> seedPoints;
            vtkNew<vtkCellArray> seedCells;
            seeds->SetPoints(seedPoints);
            seeds->SetVerts(seedCells);

            seedPoints->InsertNextPoint(pow(-1, j) * dist * real(eigenS.eigenvectors().col(i)[0]) +
                criticalPoints->GetPoint(pointId)[0],
              pow(-1, j) * dist * real(eigenS.eigenvectors().col(i)[1]) +
                criticalPoints->GetPoint(pointId)[1],
              pow(-1, j) * dist * real(eigenS.eigenvectors().col(i)[2]) +
                criticalPoints->GetPoint(pointId)[2]);
            vtkNew<vtkVertex> vertex;
            vertex->GetPointIds()->SetId(0, seedPoints->GetNumberOfPoints() - 1);
            seedCells->InsertNextCell(vertex);

            // integrate
            streamTracer->SetIntegrationDirection(isBackward);
            streamTracer->SetSourceData(seeds);
            streamTracer->Update();

            // close gap to the critical point at the end
            int closestCriticalPointToEnd = 0;
            double p[3];
            double q[3];
            for (int j = 0; j < criticalPoints->GetNumberOfPoints(); j++)
            {
              vtkMath::Subtract(streamTracer->GetOutput()->GetPoint(
                                  streamTracer->GetOutput()->GetNumberOfPoints() - 1),
                criticalPoints->GetPoint(closestCriticalPointToEnd), p);
              vtkMath::Subtract(streamTracer->GetOutput()->GetPoint(
                                  streamTracer->GetOutput()->GetNumberOfPoints() - 1),
                criticalPoints->GetPoint(j), p);
              if (vtkMath::Norm(p) < dist)
              {
                closestCriticalPointToEnd = j;
              }
            }
            vtkMath::Subtract(streamTracer->GetOutput()->GetPoint(
                                streamTracer->GetOutput()->GetNumberOfPoints() - 1),
              criticalPoints->GetPoint(closestCriticalPointToEnd), p);
            if (vtkMath::Norm(p) < dist)
            {
              streamTracer->GetOutput()->GetPoints()->InsertNextPoint(
                criticalPoints->GetPoint(closestCriticalPointToEnd));
              for (int j = 0; j < streamTracer->GetOutput()->GetPointData()->GetNumberOfArrays();
                   j++)
              {
                streamTracer->GetOutput()->GetPointData()->GetArray(j)->InsertNextTuple(
                  streamTracer->GetOutput()->GetPointData()->GetArray(j)->GetTuple(
                    closestCriticalPointToEnd));
              }
              vtkNew<vtkLine> line;
              line->GetPointIds()->SetId(0, streamTracer->GetOutput()->GetNumberOfPoints() - 1);
              line->GetPointIds()->SetId(1, streamTracer->GetOutput()->GetNumberOfPoints() - 2);
              streamTracer->GetOutput()->GetLines()->InsertNextCell(line);
              for (int j = 0; j < streamTracer->GetOutput()->GetCellData()->GetNumberOfArrays();
                   j++)
              {
                streamTracer->GetOutput()->GetCellData()->GetArray(j)->InsertNextTuple(
                  streamTracer->GetOutput()->GetCellData()->GetArray(j)->GetTuple(0));
              }
            }

            // close gap to the critical point at the beginning
            streamTracer->GetOutput()->GetPoints()->InsertNextPoint(
              criticalPoints->GetPoint(pointId));
            for (int j = 0; j < streamTracer->GetOutput()->GetPointData()->GetNumberOfArrays(); j++)
            {
              streamTracer->GetOutput()->GetPointData()->GetArray(j)->InsertNextTuple(
                streamTracer->GetOutput()->GetPointData()->GetArray(j)->GetTuple(0));
            }
            vtkNew<vtkLine> line;
            line->GetPointIds()->SetId(0, streamTracer->GetOutput()->GetNumberOfPoints() - 1);
            line->GetPointIds()->SetId(1, 0);
            streamTracer->GetOutput()->GetLines()->InsertNextCell(line);
            for (int j = 0; j < streamTracer->GetOutput()->GetCellData()->GetNumberOfArrays(); j++)
            {
              streamTracer->GetOutput()->GetCellData()->GetArray(j)->InsertNextTuple(
                streamTracer->GetOutput()->GetCellData()->GetArray(j)->GetTuple(0));
            }

            // fill arrays
            vtkNew<vtkDoubleArray> iterationArray;
            iterationArray->SetName("iteration");
            iterationArray->SetNumberOfTuples(streamTracer->GetOutput()->GetNumberOfPoints());
            streamTracer->GetOutput()->GetPointData()->AddArray(iterationArray);
            vtkNew<vtkDoubleArray> indexArray;
            indexArray->SetName("index");
            indexArray->SetNumberOfTuples(streamTracer->GetOutput()->GetNumberOfPoints());
            streamTracer->GetOutput()->GetPointData()->AddArray(indexArray);
            for (int i = 0; i < streamTracer->GetOutput()->GetNumberOfPoints(); i++)
            {
              iterationArray->SetTuple1(i, i + 1);
              indexArray->SetTuple1(i, numberOfSeparatingLines);
            }
            iterationArray->SetTuple1(streamTracer->GetOutput()->GetNumberOfPoints() - 1, 0);

            // combine lines
            vtkNew<vtkAppendPolyData> appendFilter;
            appendFilter->AddInputData(separatrices);
            appendFilter->AddInputData(streamTracer->GetOutput());
            appendFilter->Update();
            separatrices->DeepCopy(appendFilter->GetOutput());
            numberOfSeparatingLines++;
          }
          if (computeSurfaces && dataset->GetDataDimension() == 3)
          {
            ComputeSurface(numberOfSeparatingSurfaces++, isForward, normal,
              criticalPoints->GetPoint(pointId), surfaces, dataset, integrationStepUnit, dist,
              stepSize, maxNumSteps, useIterativeSeeding);
          }
        }
      }
    }
  }

  // probe arrays to output surfaces
  if (computeSurfaces && dataset->GetDataDimension() == 3)
  {
    probe->SetInputData(surfaces);
    probe->SetSourceData(dataset);
    probe->Update();
    for (int i = 0; i < dataset->GetPointData()->GetNumberOfArrays(); ++i)
    {
      if (probe->GetOutput()->GetPointData()->GetArray(i)->GetNumberOfComponents() == 3)
      {
        surfaces->GetPointData()->SetVectors(probe->GetOutput()->GetPointData()->GetArray(i));
        break;
      }
    }
  }
  return 1;
}

//----------------------------------------------------------------------------
int vtkVectorFieldTopology::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo0 = outputVector->GetInformationObject(0);
  vtkInformation* outInfo1 = outputVector->GetInformationObject(1);
  vtkInformation* outInfo2 = outputVector->GetInformationObject(2);

  // get the input and output
  vtkImageData* dataset = vtkImageData::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));

  // these things are necessary for probe and the integrator to work properly in the 2D setting
  if (dataset->GetDataDimension() == 2)
  {
    dataset->SetSpacing(dataset->GetSpacing()[0], dataset->GetSpacing()[1], 1);
    dataset->SetOrigin(dataset->GetOrigin()[0], dataset->GetOrigin()[1], 0);
    for (int i = 0; i < dataset->GetNumberOfPoints(); ++i)
    {
      auto vector = dataset->GetPointData()->GetVectors()->GetTuple(i);
      dataset->GetPointData()->GetVectors()->SetTuple3(i, vector[0], vector[1], 0);
    }
  }
  if (dataset->GetPointData()->GetVectors() == nullptr)
  {
    vtkErrorMacro("The field does not contain any vectors.");
    return 0;
  }

  // make output
  vtkPolyData* criticalPoints =
    vtkPolyData::SafeDownCast(outInfo0->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData* separatingLines =
    vtkPolyData::SafeDownCast(outInfo1->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData* separatingSurfaces =
    vtkPolyData::SafeDownCast(outInfo2->Get(vtkDataObject::DATA_OBJECT()));

  // Triangulate the input data
  vtkNew<vtkDataSetTriangleFilter> triangulateFilter3D;
  triangulateFilter3D->SetInputData(dataset);
  triangulateFilter3D->Update();
  vtkSmartPointer<vtkUnstructuredGrid> tridataset = triangulateFilter3D->GetOutput();

  // Compute gradient
  this->GradientFilter->SetInputData(dataset);
  this->GradientFilter->SetInputScalars(
    vtkDataObject::FIELD_ASSOCIATION_POINTS, dataset->GetPointData()->GetVectors()->GetName());
  this->GradientFilter->SetResultArrayName("gradient");
  this->GradientFilter->Update();
  vtkSmartPointer<vtkImageData> graddataset =
    vtkImageData::SafeDownCast(this->GradientFilter->GetOutput());

  // Compute critical points
  vtkNew<vtkPoints> criticalPointsPoints;
  vtkNew<vtkCellArray> criticalPointsCells;
  criticalPoints->SetPoints(criticalPointsPoints);
  criticalPoints->SetVerts(criticalPointsCells);
  if (dataset->GetDataDimension() == 2)
  {
    ComputeCriticalPoints2D(criticalPoints, tridataset);
  }
  else
  {
    ComputeCriticalPoints3D(criticalPoints, tridataset);
  }

  ComputeSeparatrices(criticalPoints, separatingLines, separatingSurfaces, dataset, graddataset,
    this->IntegrationStepUnit, this->SeparatrixDistance, this->IntegrationStepSize,
    this->MaxNumSteps, this->ComputeSurfaces, this->UseIterativeSeeding);

  return 1;
}
