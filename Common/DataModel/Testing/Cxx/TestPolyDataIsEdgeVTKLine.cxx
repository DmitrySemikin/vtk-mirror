/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPolyDataIsEdgeVTKLine.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkCellArray.h"
#include "vtkLine.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkSmartPointer.h"
#include <iostream>

/**
 * @brief Test entry point.
 */
int TestPolyDataIsEdgeVTKLine(int argc, char* argv[])
{
  // create points of a square
  const double p0[3] = { 0.0, 0.0, 0.0 };
  const double p1[3] = { 0.0, 1.0, 0.0 };
  const double p2[3] = { 1.0, 1.0, 0.0 };
  const double p3[3] = { 1.0, 0.0, 0.0 };

  // insert points
  auto vtkPointsArray = vtkSmartPointer<vtkPoints>::New();
  vtkPointsArray->SetDataTypeToDouble();
  vtkPointsArray->InsertNextPoint(p0);
  vtkPointsArray->InsertNextPoint(p1);
  vtkPointsArray->InsertNextPoint(p2);
  vtkPointsArray->InsertNextPoint(p3);

  // create lines
  auto line0 = vtkSmartPointer<vtkLine>::New();
  line0->GetPointIds()->SetId(0, 0);
  line0->GetPointIds()->SetId(1, 1);
  auto line1 = vtkSmartPointer<vtkLine>::New();
  line1->GetPointIds()->SetId(0, 1);
  line1->GetPointIds()->SetId(1, 2);
  auto line2 = vtkSmartPointer<vtkLine>::New();
  line2->GetPointIds()->SetId(0, 2);
  line2->GetPointIds()->SetId(1, 3);
  auto line3 = vtkSmartPointer<vtkLine>::New();
  line3->GetPointIds()->SetId(0, 3);
  line3->GetPointIds()->SetId(1, 0);

  // insert lines
  auto lines = vtkSmartPointer<vtkCellArray>::New();
  lines->InsertNextCell(line0);
  lines->InsertNextCell(line1);
  lines->InsertNextCell(line2);
  lines->InsertNextCell(line3);

  // create polydata
  auto polyData = vtkSmartPointer<vtkPolyData>::New();
  polyData->SetPoints(vtkPointsArray);
  polyData->SetLines(lines);

  // create links
  polyData->BuildLinks();

  // test correct edges
  if (!(polyData->IsEdge(0, 1) && polyData->IsEdge(1, 2) && polyData->IsEdge(2, 3) &&
        polyData->IsEdge(3, 0)))
  {
    std::cout << "test correct edges failed" << std::endl;
    return EXIT_FAILURE;
  }

  // test incorrect edges
  if (polyData->IsEdge(1, 3) || polyData->IsEdge(0, 2))
  {
    std::cout << "test incorrect edges failed" << std::endl;
    return EXIT_FAILURE;
  }

  // test edge with itself
  if (polyData->IsEdge(1, 1))
  {
    std::cout << "test edged with itself failed" << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
