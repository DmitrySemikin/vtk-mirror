/*===========================================================================*/
/* Distributed under OSI-approved BSD 3-Clause License.                      */
/* For copyright, see the following accompanying files or https://vtk.org:   */
/* - VTK-Copyright.txt                                                       */
/*===========================================================================*/
#include "vtkActor.h"
#include "vtkArrayCalculator.h"
#include "vtkDataSetMapper.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkProperty.h"
#include "vtkPolyData.h"
#include "vtkPoints.h"
#include "vtkPointSource.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRTAnalyticSource.h"
#include "vtkStreamTracer.h"
#include "vtkWarpScalar.h"

int TestStreamTracerSurface(int argc, char* argv[])
{

  vtkNew<vtkRTAnalyticSource> wavelet;
  wavelet->SetWholeExtent(-10, 100, -10, 100, 0, 0);

  vtkNew<vtkWarpScalar> warp;
  warp->SetScaleFactor(0.1);
  warp->SetInputConnection(wavelet->GetOutputPort());

  vtkNew<vtkArrayCalculator> calc;
  calc->AddScalarArrayName("RTData");
  calc->SetFunction("abs(RTData)*iHat + abs(RTData)*jHat");
  calc->SetInputConnection(warp->GetOutputPort());
  calc->Update();

  vtkNew<vtkPoints> points;
  vtkDataSet* calcData = vtkDataSet::SafeDownCast(calc->GetOutput());
  vtkIdType nLine = static_cast<vtkIdType>(sqrt(static_cast<double>(calcData->GetNumberOfPoints())));
  for (vtkIdType i = 0; i < nLine; i += 10)
  {
    points->InsertNextPoint(calcData->GetPoint(i * (nLine - 1) + nLine));
  }

  vtkNew<vtkPolyData> pointsPolydata;
  pointsPolydata->SetPoints(points);

  vtkNew<vtkStreamTracer> stream;
  stream->SurfaceStreamlinesOn();
  stream->SetMaximumPropagation(210);
  stream->SetIntegrationDirection(vtkStreamTracer::BOTH);
  stream->SetInputConnection(calc->GetOutputPort());
  stream->SetSourceData(pointsPolydata);

  vtkNew<vtkDataSetMapper> streamMapper;
  streamMapper->SetInputConnection(stream->GetOutputPort());
  streamMapper->ScalarVisibilityOff();

  vtkNew<vtkDataSetMapper> surfaceMapper;
  surfaceMapper->SetInputConnection(calc->GetOutputPort());

  vtkNew<vtkActor> streamActor;
  streamActor->SetMapper(streamMapper);
  streamActor->GetProperty()->SetColor(1, 1, 1);
  streamActor->GetProperty()->SetLineWidth(4.);
  streamActor->SetPosition(0, 0, 1);

  vtkNew<vtkActor> surfaceActor;
  surfaceActor->SetMapper(surfaceMapper);
  surfaceActor->GetProperty()->SetRepresentationToSurface();

  vtkNew<vtkRenderer> renderer;
  renderer->AddActor(surfaceActor);
  renderer->AddActor(streamActor);
  renderer->ResetCamera();
  renderer->SetBackground(1., 1., 1.);

  vtkNew<vtkRenderWindow> renWin;
  renWin->AddRenderer(renderer);
  renWin->SetMultiSamples(0);
  renWin->SetSize(300, 300);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }
  return !retVal;
}
