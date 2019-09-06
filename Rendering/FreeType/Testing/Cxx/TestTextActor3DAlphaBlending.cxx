/*===========================================================================*/
/* Distributed under OSI-approved BSD 3-Clause License.                      */
/* For copyright, see the following accompanying files or https://vtk.org:   */
/* - VTK-Copyright.txt                                                       */
/*===========================================================================*/
// This tests vtkTextActor3D with default alpha blending.
// As this actor uses vtkImageActor underneath, it also tests vtkImageActor
// with alpha blending.
//
// The command line arguments are:
// -I        => run in interactive mode; unless this is used, the program will
//              not allow interaction and exit

#include "vtkTestUtilities.h"
#include "vtkRegressionTestImage.h"

#include "vtkRenderWindowInteractor.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkCamera.h"
#include "vtkTextActor3D.h"
#include "vtkTextProperty.h"

int TestTextActor3DAlphaBlending(int argc, char* argv[])
{
  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
  vtkRenderWindow *renWin = vtkRenderWindow::New();
  iren->SetRenderWindow(renWin);
  renWin->Delete();

  vtkRenderer *renderer = vtkRenderer::New();
  renWin->AddRenderer(renderer);
  renderer->Delete();

  renderer->SetBackground(0.0,0.0,0.5);
  renWin->SetSize(300,300);

  vtkTextActor3D *actor=vtkTextActor3D::New();
  renderer->AddActor(actor);
  actor->Delete();

  actor->SetInput("0123456789.");

  vtkTextProperty *textProperty=vtkTextProperty::New();
  actor->SetTextProperty(textProperty);
  textProperty->Delete();

  actor->SetPosition(3,4,5);
  actor->SetScale(0.05,0.05,1);
  textProperty->SetJustificationToCentered();
  textProperty->SetVerticalJustificationToCentered(); // default
  textProperty->SetFontFamilyToArial(); // default.

  renWin->Render();
  renderer->ResetCamera();

  renWin->Render();

  int retVal = vtkRegressionTestImage( renWin );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  // Cleanup
  iren->Delete();
  return !retVal;
}
