/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPreserveTopology.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

  =========================================================================*/

#include <sstream>

#include "vtkDebugLeaks.h"

#include "vtkRegressionTestImage.h"

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCellData.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkExtractSelectedFastIds.h"
#include "vtkFloatArray.h"
#include "vtkPoints.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSignedCharArray.h"
#include "vtkSmartPointer.h"
#include "vtkUnstructuredGrid.h"

vtkSmartPointer<vtkSelection> BuildSelection(vtkIdType numCells, std::vector<vtkIdType> cellsToHide)
{
  vtkNew<vtkSignedCharArray> idSet;
  for (vtkIdType n = 0; n < numCells; n++)
  {
    idSet->InsertNextValue(1);
  }
  for (const auto& cellToHide : cellsToHide)
  {
    if (cellToHide >= 0 && cellToHide < numCells)
    {
      idSet->SetValue(cellToHide, -1);
    }
  }

  vtkNew<vtkSelectionNode> selectionNode;
  selectionNode->SetFieldType(vtkSelectionNode::CELL);
  selectionNode->SetContentType(vtkSelectionNode::PEDIGREEIDS);
  selectionNode->SetSelectionList(idSet);
  vtkNew<vtkSelection> selection;
  selection->AddNode(selectionNode);
  return selection;
}

int TestPreserveTopology(int argc, char* argv[])
{
  const int nI = 5;
  const int nJ = 4;
  const int nK = 3;
  const double cellSize = 1;
  const int numCells = nI * nJ * nK;

  std::ostringstream strm;
  strm << "TestPreserveTopology Start" << endl;

  vtkNew<vtkUnstructuredGrid> grid;
  {
    vtkNew<vtkFloatArray> pointsArray;
    pointsArray->SetNumberOfComponents(3); // X,Y,Z

    vtkNew<vtkCellArray> cells;

    for (int k = 0; k < nK; k++)
    {
      for (int j = 0; j < nJ; j++)
      {
        for (int i = 0; i < nI; i++)
        {
          vtkIdType startingPointIndex = pointsArray->GetNumberOfTuples();

          pointsArray->InsertNextTuple3(i * cellSize, j * cellSize, k * cellSize);
          pointsArray->InsertNextTuple3((i + 1) * cellSize, j * cellSize, k * cellSize);
          pointsArray->InsertNextTuple3((i + 1) * cellSize, (j + 1) * cellSize, k * cellSize);
          pointsArray->InsertNextTuple3(i * cellSize, (j + 1) * cellSize, k * cellSize);
          pointsArray->InsertNextTuple3(i * cellSize, j * cellSize, (k + 1) * cellSize);
          pointsArray->InsertNextTuple3((i + 1) * cellSize, j * cellSize, (k + 1) * cellSize);
          pointsArray->InsertNextTuple3((i + 1) * cellSize, (j + 1) * cellSize, (k + 1) * cellSize);
          pointsArray->InsertNextTuple3(i * cellSize, (j + 1) * cellSize, (k + 1) * cellSize);

          vtkIdType pointIndices[8];
          for (int n = 0; n < 8; n++)
          {
            pointIndices[n] = startingPointIndex + n;
          }
          cells->InsertNextCell(8, pointIndices);
        }
      }
    }

    vtkNew<vtkPoints> points;
    points->SetData(pointsArray);
    grid->SetPoints(points);

    grid->SetCells(VTK_HEXAHEDRON, cells);

    vtkNew<vtkIdTypeArray> pedigreeIds;
    for (vtkIdType n = 0; n < numCells; n++)
    {
      pedigreeIds->InsertNextValue(n);
    }
    grid->GetCellData()->SetPedigreeIds(pedigreeIds);
  }

  vtkNew<vtkExtractSelectedFastIds> filter1;
  {
    filter1->PreserveTopologyOn();
    filter1->SetTopologyFilterOutputArrayName("filter1");
    filter1->SetInputData(0, grid);
    filter1->SetInputData(1, BuildSelection(numCells, { 9, 29 }));
  }

  vtkNew<vtkExtractSelectedFastIds> filter2;
  {
    filter2->PreserveTopologyOn();
    filter2->SetTopologyFilterInputArrayName("filter1");
    filter2->SetTopologyFilterOutputArrayName("filter2");
    filter2->SetInputConnection(0, filter1->GetOutputPort());
    filter2->SetInputData(1, BuildSelection(numCells, { 5, 25, 45, 6, 26, 10, 30, 11, 31 }));
  }

  vtkNew<vtkDataSetSurfaceFilter> surface;
  {
    surface->SetInputConnection(filter2->GetOutputPort());
    surface->FilterTopologyOn();
    surface->SetTopologyFilterArrayName("filter2");
  }

  vtkNew<vtkActor> actor;
  {
    vtkNew<vtkPolyDataMapper> mapper;
    mapper->SetInputConnection(surface->GetOutputPort());
    mapper->ScalarVisibilityOn();

    actor->GetProperty()->SetRepresentationToSurface();
    actor->GetProperty()->SetColor(0.0, 1.0, 0.1);
    actor->GetProperty()->SetEdgeColor(1.0, 0.0, 0.1);
    actor->GetProperty()->SetLineWidth(3.0);
    actor->GetProperty()->EdgeVisibilityOn();
    actor->SetMapper(mapper);
  }

  strm << "TestPreserveTopology End" << endl;

  vtkNew<vtkRenderer> renderer;
  renderer->AddActor(actor);
  renderer->SetBackground(0.5, 0.5, 0.5);
  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->SetMultiSamples(0);
  renderWindow->SetSize(500, 500);
  renderWindow->AddRenderer(renderer);
  vtkNew<vtkRenderWindowInteractor> renderWindowInteractor;
  renderWindowInteractor->SetRenderWindow(renderWindow);

  auto camera = renderer->GetActiveCamera();
  double oI = static_cast<double>(nI * cellSize) / 2.0;
  double oJ = static_cast<double>(nJ * cellSize) / 2.0;
  double oK = static_cast<double>(nK * cellSize) / 2.0;
  camera->SetPosition(oI, oJ, oK * -10.0);
  camera->SetFocalPoint(oI, oJ, oK);
  camera->SetViewUp(0.0, 1.0, 0.0);

  renderWindow->Render();

  int retVal = vtkRegressionTestImage(renderWindow);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    renderWindowInteractor->Start();
    retVal = vtkRegressionTester::PASSED;
  }
  return (retVal == vtkRegressionTester::PASSED) ? EXIT_SUCCESS : EXIT_FAILURE;
}
