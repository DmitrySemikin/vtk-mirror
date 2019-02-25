/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGPURayCastVolumeRotation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test checks if direct ospray volume mapper intermixes with
// surface geometry in the scene.

#include <vtkCamera.h>
#include <vtkClipPolyData.h>
#include <vtkColorTransferFunction.h>
#include <vtkDataArray.h>
#include <vtkDataSetSurfaceFilter.h>
#include <vtkImageData.h>
#include <vtkImageReader.h>
#include <vtkImageShiftScale.h>
#include <vtkNew.h>
#include <vtkOSPRayVolumeMapper.h>
#include <vtkPlane.h>
#include <vtkPiecewiseFunction.h>
#include <vtkPlane.h>
#include <vtkPointData.h>
#include <vtkPolyDataMapper.h>
#include <vtkRegressionTestImage.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>
#include <vtkStructuredPointsReader.h>
#include <vtkTestUtilities.h>
#include <vtkTimerLog.h>
#include <vtkVolumeProperty.h>
#include <vtkXMLImageDataReader.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkOSPRayPass.h>
#include <vtkProperty.h>

#include <vtkAutoInit.h>
VTK_MODULE_INIT(vtkRenderingOSPRay);

int TestOSPRayVolumeRenderer(int argc, char *argv[])
{
  double scalarRange[2];

  vtkNew<vtkActor> dssActor;
  vtkNew<vtkPolyDataMapper> dssMapper;
  vtkNew<vtkOSPRayVolumeMapper> volumeMapper;

  vtkNew<vtkXMLImageDataReader> reader;
  const char* volumeFile = vtkTestUtilities::ExpandDataFileName(
                            argc, argv, "Data/vase_1comp.vti");
  reader->SetFileName(volumeFile);
  volumeMapper->SetInputConnection(reader->GetOutputPort());
  // Put inside an open box to evaluate composite order
  vtkNew<vtkDataSetSurfaceFilter> dssFilter;
  dssFilter->SetInputConnection(reader->GetOutputPort());
  vtkNew<vtkClipPolyData> clip;
  vtkNew<vtkPlane> plane;
  plane->SetOrigin(0,50,0);
  plane->SetNormal(0,-1,0);
  clip->SetInputConnection(dssFilter->GetOutputPort());
  clip->SetClipFunction(plane);
  dssMapper->SetInputConnection(clip->GetOutputPort());
  dssMapper->ScalarVisibilityOff();
  dssActor->SetMapper(dssMapper);
  vtkProperty* property = dssActor->GetProperty();
  property->SetDiffuseColor(0.5, 0.5, 0.5);

  reader->Update();
  volumeMapper->GetInput()->GetScalarRange(scalarRange);
  volumeMapper->SetBlendModeToComposite();

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetMultiSamples(0);
  vtkNew<vtkRenderer> ren;
  renWin->AddRenderer(ren);
  ren->SetBackground(0.2, 0.2, 0.5);
  renWin->SetSize(400, 400);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);
//  vtkNew<vtkInteractorStyleTrackballCamera> style;
//  iren->SetInteractorStyle(style);

  vtkNew<vtkPiecewiseFunction> scalarOpacity;
  scalarOpacity->AddPoint(50, 0.0);
  scalarOpacity->AddPoint(75, 0.1);

  vtkNew<vtkVolumeProperty> volumeProperty;
  volumeProperty->ShadeOff();
  volumeProperty->SetInterpolationType(VTK_LINEAR_INTERPOLATION);

  volumeProperty->SetScalarOpacity(scalarOpacity);

  vtkSmartPointer<vtkColorTransferFunction> colorTransferFunction =
    volumeProperty->GetRGBTransferFunction(0);
  colorTransferFunction->RemoveAllPoints();
  colorTransferFunction->AddRGBPoint(scalarRange[0], 0.0, 0.8, 0.1);
  colorTransferFunction->AddRGBPoint(scalarRange[1], 0.0, 0.8, 0.1);
  //colorTransferFunction->SetRange(scalarRange[0], scalarRange[1]);
  cerr << scalarRange[0] << " to " << scalarRange[1] << endl;

  vtkNew<vtkVolume> volume;
  volume->SetMapper(volumeMapper);
  volume->SetProperty(volumeProperty);

  ren->AddViewProp(volume);
  ren->AddActor(dssActor);
  renWin->Render();
  ren->ResetCamera();

  iren->Initialize();
  iren->SetDesiredUpdateRate(30.0);

  int retVal = vtkRegressionTestImageThreshold( renWin, 50.0 );
  if( retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
