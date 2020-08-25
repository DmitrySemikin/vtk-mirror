/*=========================================================================

  Program:   Visualization Toolkit
  Module:    quadraticIntersection.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

  =========================================================================*/
// .NAME
// .SECTION Description
// This program tests quadratic cell IntersectWithLine() methods.

#include <sstream>

#include "vtkDebugLeaks.h"

#include "vtkRegressionTestImage.h"
#include <vtkCylindricalGrid.h>

#include <vtkActor.h>
#include <vtkCamera.h>
#include <vtkCellArray.h>
#include <vtkDataSetSurfaceFilter.h>
#include <vtkFeatureEdges.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkPolyLine.h>
#include <vtkProperty.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>
#include <vtkTubeFilter.h>
#include <vtkVersion.h>

int TestCylindricalGrid(int argc, char* argv[])
{
  std::ostringstream strm;
  strm << "Test vtkCell::TestCylindricalGrid Start" << endl;

  vtkNew<vtkActor> lineActor;
  {
    vtkNew<vtkPoints> points;
    vtkNew<vtkCellArray> cells;

    points->InsertNextPoint(0.0, 0.0, -10.0);
    points->InsertNextPoint(0.0, 0.0, 10.0);

    std::vector<int> cell1 = { 0, 1 };
    std::vector<std::vector<int>> cellMap{ cell1 };

    for (int i = 0; i < cellMap.size(); i++)
    {
      auto cellN = cellMap[i];
      vtkNew<vtkPolyLine> polyline;
      polyline->GetPointIds()->SetNumberOfIds(cellN.size());
      for (int j = 0; j < cellN.size(); j++)
      {
        polyline->GetPointIds()->SetId(j, cellN[j]);
      }
      cells->InsertNextCell(polyline);
    }

    vtkNew<vtkPolyData> line;
    line->SetPoints(points);
    line->SetLines(cells);

    vtkNew<vtkTubeFilter> tubeFilter;
    tubeFilter->SetInputData(line);
    // tubeFilter->SetRadius(.025); //default is .5
    tubeFilter->SetRadius(0.15);
    tubeFilter->SetNumberOfSides(50);

    vtkNew<vtkPolyDataMapper> lineMapper;
    lineMapper->SetInputConnection(tubeFilter->GetOutputPort());

    lineActor->GetProperty()->SetColor(0.0, 1.0, 0.1); // Give some color to the line
    lineActor->SetMapper(lineMapper);
  }

  vtkNew<vtkActor> gridActor;
  vtkNew<vtkActor> outlineActor;
  {
    // Add the grid
    vtkNew<vtkCylindricalGrid> grid;
    grid->SetMaximumAngle(5);
    grid->InsertNextCylindricalCell(0.5, 1, 0, 360, -1, 1);
    grid->InsertNextCylindricalCell(0.5, 1, 0, 90, 1, 2);
    grid->InsertNextCylindricalCell(0.5, 1, 45, 180, 2.5, 3);
    grid->UseDegreesOff();
    grid->InsertNextCylindricalCell(0.5, 1, vtkMath::Pi(), vtkMath::Pi() * 2.0, 4, 5);

    vtkNew<vtkDataSetSurfaceFilter> surface;
    surface->SetInputData(grid);

    vtkNew<vtkPolyDataMapper> mapper;
    mapper->SetInputConnection(surface->GetOutputPort());
    mapper->ScalarVisibilityOn();

    gridActor->GetProperty()->SetColor(1.0, 0.6, 0.0);
    gridActor->SetMapper(mapper);

    // Outline the grid cells
    vtkNew<vtkFeatureEdges> outlineEdges;
    vtkNew<vtkPolyDataMapper> outlineMapper;

    outlineEdges->SetInputConnection(surface->GetOutputPort());
    outlineEdges->SetFeatureAngle(75);
    outlineEdges->ColoringOff();

    outlineMapper->SetInputConnection(outlineEdges->GetOutputPort());
    outlineMapper->ScalarVisibilityOff();

    outlineActor->GetProperty()->SetColor(1.0, 0.0, 1.0);
    outlineActor->GetProperty()->EdgeVisibilityOn();
    outlineActor->GetProperty()->RenderLinesAsTubesOn();
    outlineActor->GetProperty()->SetLineWidth(5);
    outlineActor->SetMapper(outlineMapper);
  }

  strm << "Test vtkCell::TestCylindricalGrid End" << endl;

  vtkSmartPointer<vtkRenderer> renderer = vtkSmartPointer<vtkRenderer>::New();
  renderer->AddActor(lineActor);
  renderer->AddActor(gridActor);
  renderer->AddActor(outlineActor);
  renderer->SetBackground(0.5, 0.5, 0.5);
  vtkSmartPointer<vtkRenderWindow> renderWindow = vtkSmartPointer<vtkRenderWindow>::New();
  renderWindow->SetMultiSamples(0);
  renderWindow->SetSize(500, 500);
  renderWindow->AddRenderer(renderer);
  vtkSmartPointer<vtkRenderWindowInteractor> renderWindowInteractor =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  renderWindowInteractor->SetRenderWindow(renderWindow);

  auto camera = renderer->GetActiveCamera();
  camera->SetPosition(10, 0, 10);
  camera->SetFocalPoint(0, 0, 2.5);
  camera->SetViewUp(0, 0, 1.0);

  renderWindow->Render();

  int retVal = vtkRegressionTestImage(renderWindow);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    renderWindowInteractor->Start();
    retVal = vtkRegressionTester::PASSED;
  }
  return (retVal == vtkRegressionTester::PASSED) ? EXIT_SUCCESS : EXIT_FAILURE;
}
