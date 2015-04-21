/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestMultiblockDisplayProperties.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCompositePolyDataMapper2.h"
#include "vtkDataObject.h"
#include "vtkNew.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkTestUtilities.h"
#include "vtkXMLMultiBlockDataReader.h"

int TestMultiblockPartialArrayColoring(int argc, char* argv[])
{
  vtkNew<vtkXMLMultiBlockDataReader> reader;
  char * fname = vtkTestUtilities::ExpandDataFileName(
    argc, argv, "Data/partial_array_blocks/partial_array_blocks.vtm");
  reader->SetFileName(fname);
  delete[] fname;

  vtkNew<vtkRenderWindow> renWin;

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin.GetPointer());

  vtkNew<vtkRenderer> renderer;
  renWin->AddRenderer(renderer.GetPointer());

  vtkNew<vtkActor> actor;
  renderer->AddActor(actor.GetPointer());

  vtkNew<vtkCompositePolyDataMapper2> mapper;
  mapper->SetInputConnection(reader->GetOutputPort());
  mapper->SetScalarModeToUsePointData();
  mapper->SelectColorArray("Ids");
  mapper->ScalarVisibilityOn();
  mapper->SetScalarRange(0, 49.0);
  mapper->InterpolateScalarsBeforeMappingOn();
  actor->SetMapper(mapper.GetPointer());

  renWin->SetSize(400,400);
  renderer->SetBackground(0.0,0.0,0.0);
  renderer->ResetCamera();
  renWin->Render();

  int retVal = vtkRegressionTestImage(renWin.GetPointer());
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Start();
    }
  return !retVal;
}
