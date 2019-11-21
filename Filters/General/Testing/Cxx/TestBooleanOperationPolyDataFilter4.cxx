/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestBooleanOperationPolyDataFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/

#include "vtkTesting.h"
#include <vtkActor.h>
#include <vtkBooleanOperationPolyDataFilter.h>
#include <vtkPolyDataMapper.h>
#include <vtkPolyDataReader.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>

#include <string>

int TestBooleanOperationPolyDataFilter4(int argc, char* argv[])
{
  vtkSmartPointer<vtkRenderer> renderer = vtkSmartPointer<vtkRenderer>::New();

  vtkSmartPointer<vtkRenderWindow> renWin = vtkSmartPointer<vtkRenderWindow>::New();
  renWin->AddRenderer(renderer);

  vtkSmartPointer<vtkRenderWindowInteractor> renWinInteractor =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  renWinInteractor->SetRenderWindow(renWin);

  vtkSmartPointer<vtkTesting> testHelper = vtkSmartPointer<vtkTesting>::New();
  testHelper->AddArguments(argc, argv);

  if (!testHelper->IsFlagSpecified("-D"))
  {
    std::cerr << "Error: -D /path/to/data was not specified.";
    return EXIT_FAILURE;
  }

  std::string dataRoot = testHelper->GetDataRoot();

  std::string skull = dataRoot + "/Data/BooleanClipping/skull_for_cc7_rc3.vtk";
  std::string chamber = dataRoot + "/Data/BooleanClipping/chamber__cc7.vtk";

  // The chamber
  vtkSmartPointer<vtkPolyData> input1;

  // Human skull
  vtkSmartPointer<vtkPolyData> input2;

  // 1st Input
  //=====================================================================
  vtkSmartPointer<vtkPolyDataReader> reader1 = vtkSmartPointer<vtkPolyDataReader>::New();
  reader1->SetFileName(chamber.c_str());
  reader1->Update();
  input1 = reader1->GetOutput();

  // 2nd Input
  //=====================================================================
  vtkSmartPointer<vtkPolyDataReader> reader2 = vtkSmartPointer<vtkPolyDataReader>::New();
  reader2->SetFileName(skull.c_str());
  reader2->Update();
  input2 = reader2->GetOutput();

  vtkSmartPointer<vtkBooleanOperationPolyDataFilter> booleanOperation =
    vtkSmartPointer<vtkBooleanOperationPolyDataFilter>::New();

  booleanOperation->SetOperationToDifference();

  booleanOperation->SetInputData(0, input1);
  booleanOperation->SetInputData(1, input2);

  vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
  mapper->SetInputConnection(booleanOperation->GetOutputPort());
  mapper->ScalarVisibilityOff();

  vtkSmartPointer<vtkActor> differenceActor = vtkSmartPointer<vtkActor>::New();
  differenceActor->SetMapper(mapper);

  // vtkSmartPointer<vtkActor> differenceActor = GetBooleanOperationActor(argc, argv);
  renderer->AddActor(differenceActor);
  // differenceActor->Delete();

  renWin->Render();
  renWinInteractor->Start();

  return EXIT_SUCCESS;
}
