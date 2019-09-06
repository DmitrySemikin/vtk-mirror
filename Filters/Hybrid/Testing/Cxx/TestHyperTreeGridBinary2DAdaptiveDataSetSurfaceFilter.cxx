/*===========================================================================*/
/* Distributed under OSI-approved BSD 3-Clause License.                      */
/* For copyright, see the following accompanying files or https://vtk.org:   */
/* - VTK-Copyright.txt                                                       */
/*===========================================================================*/
// .SECTION Thanks
// This test was written by Rogeli Grima and Philippe Pebay, 2016
// This work was supported by Commissariat a l'Energie Atomique (CEA/DIF)

#include "vtkAdaptiveDataSetSurfaceFilter.h"

#include "vtkCamera.h"
#include "vtkDataSetMapper.h"
#include "vtkCellData.h"
#include "vtkHyperTreeGridSource.h"
#include "vtkNew.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"

int TestHyperTreeGridBinary2DAdaptiveDataSetSurfaceFilter( int argc, char* argv[] )
{
  // Hyper tree grid
  vtkNew<vtkHyperTreeGridSource> htGrid;
  int maxLevel = 6;
  htGrid->SetMaxDepth(maxLevel);
  htGrid->SetDimensions( 3, 4, 1 ); //Dimension 2 in xy plane GridCell 2, 3, 1
  htGrid->SetGridScale( 1.5, 1., 10. );  // this is to test that orientation fixes scale
  htGrid->SetBranchFactor( 2 );
  htGrid->SetDescriptor( "RRRRR.|.... .R.. RRRR R... R...|.R.. ...R ..RR .R.. R... .... ....|.... ...R ..R. .... .R.. R...|.... .... .R.. ....|...." );

  // Data set surface
  vtkNew<vtkAdaptiveDataSetSurfaceFilter> surface;
  vtkNew<vtkRenderer> renderer;
  surface->SetRenderer( renderer );
  surface->SetInputConnection( htGrid->GetOutputPort() );
  surface->Update();
  vtkPolyData* pd = surface->GetOutput();
  double* range = pd->GetCellData()->GetScalars()->GetRange();

  // Mappers
  vtkMapper::SetResolveCoincidentTopologyToPolygonOffset();
  vtkNew<vtkDataSetMapper> mapper1;
  mapper1->SetInputConnection( surface->GetOutputPort() );
  mapper1->SetScalarRange( range );
  vtkNew<vtkDataSetMapper> mapper2;
  mapper2->SetInputConnection( surface->GetOutputPort() );
  mapper2->ScalarVisibilityOff();

  // Actors
  vtkNew<vtkActor> actor1;
  actor1->SetMapper( mapper1 );
  vtkNew<vtkActor> actor2;
  actor2->SetMapper( mapper2 );
  actor2->GetProperty()->SetRepresentationToWireframe();
  actor2->GetProperty()->SetColor( .7, .7, .7 );

  // Camera
  vtkNew<vtkCamera> camera;
  double point[3] = { pd->GetCenter()[0]-0.75, pd->GetCenter()[1], pd->GetCenter()[2] };
  camera->SetClippingRange( 1., 100. );
  camera->SetFocalPoint( point );
  point[2] += 10.0;
  camera->SetPosition(point);
  camera->ParallelProjectionOn();
  camera->SetParallelScale(1);

  // Renderer
  renderer->SetActiveCamera( camera );
  renderer->SetBackground( 1., 1., 1. );
  renderer->AddActor( actor1 );
  renderer->AddActor( actor2 );

  // Render window
  vtkNew<vtkRenderWindow> renWin;
  renWin->AddRenderer( renderer );
  renWin->SetSize( 400, 400 );
  renWin->SetMultiSamples( 0 );

  // Interactor
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow( renWin );

  // Render and test
  renWin->Render();

  int retVal = vtkRegressionTestImageThreshold( renWin, 120 );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR )
  {
    iren->Start();
  }

  return !retVal;
}
