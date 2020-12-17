/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestCompositePolyDataMapper2MixedOpacities.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Test for paraview/paraview#20362

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCompositeDataDisplayAttributes.h"
#include "vtkCompositeDataSet.h"
#include "vtkCompositePolyDataMapper2.h"
#include "vtkConeSource.h"
#include "vtkCullerCollection.h"
#include "vtkInformation.h"
#include "vtkMath.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkRenderingOpenGLConfigure.h"
#include "vtkSmartPointer.h"
#include "vtkSphereSource.h"
#include "vtkTestUtilities.h"
#include "vtkTimerLog.h"
#include "vtkTrivialProducer.h"

namespace
{
vtkSmartPointer<vtkMultiBlockDataSet> CreateDataSet()
{
  vtkNew<vtkMultiBlockDataSet> mb;

  vtkNew<vtkSphereSource> sphere;
  sphere->Update();
  mb->SetBlock(0, sphere->GetOutput(0));

  vtkNew<vtkConeSource> cone;
  cone->SetCenter(1, 0, 0);
  cone->Update();
  mb->SetBlock(1, cone->GetOutput(0));

  return mb;
}
}

int TestCompositePolyDataMapper2MixedOpacities(int argc, char* argv[])
{
  auto mb = ::CreateDataSet();

  vtkNew<vtkCompositePolyDataMapper2> mapper;
  mapper->SetInputDataObject(0, mb);

  vtkNew<vtkCompositeDataDisplayAttributes> attrs;
  attrs->SetBlockOpacity(mb->GetBlock(0), 1.0);
  mapper->SetCompositeDataDisplayAttributes(attrs);

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  actor->GetProperty()->SetOpacity(0.5);

  vtkNew<vtkRenderer> ren;
  ren->AddActor(actor);

  vtkNew<vtkRenderWindow> win;
  win->AddRenderer(ren);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(win);

  win->Render();
  ren->ResetCamera();
  win->Render();

  int retVal = vtkRegressionTestImageThreshold(win.GetPointer(), 15);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }
  return !retVal;
}
