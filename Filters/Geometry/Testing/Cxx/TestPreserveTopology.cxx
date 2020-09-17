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
#include "vtkClipPolyData.h"
#include "vtkCompositeDataGeometryFilter.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkExtractSelectedFastIds.h"
#include "vtkFloatArray.h"
#include "vtkPlane.h"
#include "vtkPlaneCutter.h"
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

namespace
{
const static int nI = 5;
const static int nJ = 4;
const static int nK = 3;

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

vtkIdType GetPointIndex(vtkIdType i, vtkIdType j, vtkIdType k)
{
  auto kOffset = k * (nJ + 1) * (nI + 1);
  auto jOffset = j * (nI + 1);
  return i + jOffset + kOffset;
}
} // namespace

int TestPreserveTopology(int argc, char* argv[])
{
  const double cellSize = 1.0;
  const int numCells = nI * nJ * nK;

  std::ostringstream strm;
  strm << "TestPreserveTopology Start" << endl;

  vtkNew<vtkUnstructuredGrid> grid;
  {
    vtkNew<vtkFloatArray> pointsArray;
    pointsArray->SetNumberOfComponents(3); // X,Y,Z
    for (int k = 0; k <= nK; k++)
    {
      for (int j = 0; j <= nJ; j++)
      {
        for (int i = 0; i <= nI; i++)
        {
          pointsArray->InsertNextTuple3(i * cellSize, j * cellSize, k * cellSize);
        }
      }
    }

    vtkNew<vtkCellArray> cells;
    for (int k = 0; k < nK; k++)
    {
      for (int j = 0; j < nJ; j++)
      {
        for (int i = 0; i < nI; i++)
        {
          vtkIdType pointIndices[8];

          pointIndices[0] = GetPointIndex(i, j, k);
          pointIndices[1] = GetPointIndex(i + 1, j, k);
          pointIndices[2] = GetPointIndex(i + 1, j + 1, k);
          pointIndices[3] = GetPointIndex(i, j + 1, k);

          pointIndices[4] = GetPointIndex(i, j, k + 1);
          pointIndices[5] = GetPointIndex(i + 1, j, k + 1);
          pointIndices[6] = GetPointIndex(i + 1, j + 1, k + 1);
          pointIndices[7] = GetPointIndex(i, j + 1, k + 1);

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

  vtkNew<vtkPlane> cutPlane;
  vtkNew<vtkClipPolyData> cutGrid;
  {
    cutGrid->SetInputConnection(surface->GetOutputPort());

    cutPlane->SetOrigin(0.5 * cellSize, nJ * cellSize / 2.0, nK * cellSize / 2.0);
    cutPlane->SetNormal(1.0, 0.0, 0.5);
    cutGrid->SetClipFunction(cutPlane);
  }

  vtkNew<vtkPlaneCutter> intersectGrid;
  vtkNew<vtkCompositeDataGeometryFilter> intersectGridMultiPieceDataJoiner;
  {
    intersectGrid->SetInputConnection(filter2->GetOutputPort());
    intersectGrid->FilterTopologyOn();
    intersectGrid->SetTopologyFilterArrayName("filter2");
    intersectGrid->SetPlane(cutPlane);
    intersectGrid->GeneratePolygonsOn();
    intersectGrid->BuildTreeOn();
    intersectGrid->BuildHierarchyOn();

    intersectGridMultiPieceDataJoiner->SetInputConnection(intersectGrid->GetOutputPort());
  }

  vtkNew<vtkActor> actor;
  {
    vtkNew<vtkPolyDataMapper> mapper;
    mapper->SetInputConnection(cutGrid->GetOutputPort());
    mapper->ScalarVisibilityOn();

    actor->GetProperty()->SetRepresentationToSurface();
    actor->GetProperty()->SetColor(0.0, 1.0, 0.1);
    actor->GetProperty()->SetEdgeColor(1.0, 0.0, 0.1);
    actor->GetProperty()->SetLineWidth(3.0);
    actor->GetProperty()->EdgeVisibilityOn();
    actor->SetMapper(mapper);
  }

  vtkNew<vtkActor> intersectActor;
  {
    vtkNew<vtkPolyDataMapper> mapper;
    mapper->SetInputConnection(intersectGridMultiPieceDataJoiner->GetOutputPort());
    mapper->ScalarVisibilityOn();

    intersectActor->GetProperty()->SetRepresentationToSurface();
    intersectActor->GetProperty()->SetColor(0.0, 0.7, 0.5);
    intersectActor->GetProperty()->SetEdgeColor(1.0, 0.0, 1.0);
    intersectActor->GetProperty()->SetLineWidth(3.0);
    intersectActor->GetProperty()->EdgeVisibilityOn();
    intersectActor->SetMapper(mapper);
  }

  strm << "TestPreserveTopology End" << endl;

  vtkNew<vtkRenderer> renderer;
  renderer->AddActor(actor);
  renderer->AddActor(intersectActor);
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
  camera->SetPosition(oI * -4.0, oJ * 4, oK * -4.0);
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
