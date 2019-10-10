/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPLYReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME Test of vtkPLYReader
// .SECTION Description
//

#include "vtkActor.h"
#include "vtkCellTextureToPointTexture.h"
#include "vtkPLYReader.h"
#include "vtkPNGReader.h"
#include "vtkPolyDataMapper.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkTexture.h"
#include "vtkTestUtilities.h"


int TestCellTextureToPointTexture( int argc, char *argv[] )
{
  // Read file name.
  const char* fname = vtkTestUtilities::ExpandDataFileName(
    argc, argv, "Data/squareTexturedFaces.ply");
  const char* fnameImg = vtkTestUtilities::ExpandDataFileName(
    argc, argv, "Data/two_vtk_logos_stacked.png");

  vtkNew<vtkPNGReader> readerImg;
  if (0 == readerImg->CanReadFile(fnameImg))
  {
     std::cout << "The PNG reader can not read the input file." << std::endl;
     return EXIT_FAILURE;
  }
  readerImg->SetFileName(fnameImg);
  readerImg->Update();
  delete[] fnameImg;

  // Create the texture.
  vtkNew<vtkTexture> texture;
  texture->SetInputConnection(readerImg->GetOutputPort());



  // Test if the reader thinks it can open the file.
  int canRead = vtkPLYReader::CanReadFile(fname);
  (void)canRead;

  // Create the reader.
  vtkNew<vtkPLYReader> reader;
  reader->SetFileName(fname);
  reader->DuplicatePointsForFaceTextureOff();
  reader->ReadFaceTextureOnlyOn();
  delete [] fname;

  vtkNew<vtkCellTextureToPointTexture> ctpt;
  ctpt->SetInputConnection(reader->GetOutputPort());
  ctpt->Update();

    // Create a mapper.
  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(ctpt->GetOutputPort());
  mapper->ScalarVisibilityOn();

  // Create the actor.
  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  actor->SetTexture(texture);

  // Basic visualisation.
  vtkNew<vtkRenderWindow> renWin;
  vtkNew<vtkRenderer> ren;
  renWin->AddRenderer(ren);
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  ren->AddActor(actor);
  ren->SetBackground(0,0,0);
  renWin->SetSize(300,300);

  // interact with data
  renWin->Render();

  int retVal = vtkRegressionTestImage( renWin );

  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
