/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkRegressionTestImage.h"
#include "vtkTestUtilities.h"

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkDataAssembly.h"
#include "vtkDataProperties.h"
#include "vtkExtractBlock.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkPartitionedDataSetCollectionMapper.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"
#include <vtkExtractBlockUsingDataAssembly.h>

namespace
{

// Extracted from TextExtractBlock
vtkSmartPointer<vtkDataObject> GetSphere(double x, double y, double z)
{
  vtkNew<vtkSphereSource> sphere;
  sphere->SetCenter(x, y, z);
  sphere->Update();
  return sphere->GetOutputDataObject(0);
}

// Extracted from TextExtractBlock
vtkSmartPointer<vtkExtractBlockUsingDataAssembly> CreateSamplePartitionedDataSetCollection()
{
  using namespace vtkDataProperties;
  vtkNew<vtkPartitionedDataSetCollection> pdc;

  for (unsigned int part = 0; part < 4; ++part)
  {
    vtkNew<vtkPartitionedDataSet> pd;
    for (unsigned int cc = 0; cc < 3; ++cc)
    {
      pd->SetPartition(cc, ::GetSphere(cc, part, 0));
    }
    pdc->SetPartitionedDataSet(part, pd);
  }

  vtkNew<vtkDataAssembly> assembly;
  const auto base = assembly->AddNodes({ "left", "right" });
  const auto right = assembly->AddNodes({ "r1", "r2" }, base[1]);
  const auto r1 = assembly->AddNodes({ "r1", "r2" }, right[1]);

  assembly->AddDataSetIndices(base[0], { 0, 1 });
  assembly->AddDataSetIndices(right[0], { 2 });
  assembly->AddDataSetIndices(r1[1], { 3 });

  // BlockVisibility
  // BlockPickability
  // BlockColor
  // BlockOpacity
  // BlockMaterial
  // Every dataset contained in the base[1] subtree would inherit
  // those properties when rendering.
  auto ret = assembly->GetProperty(right[1], Visibility);

  if (!ret.empty())
  {
    throw std::runtime_error("vtkDataAssembly::GetProperty faulty");
  }

  assembly->SetProperty(right[1], Visibility, "true");
  assembly->SetProperty(right[1], Visibility, "false");
  ret = assembly->GetProperty(right[1], Visibility);

  if (ret != "false")
  {
    throw std::runtime_error("vtkDataAssembly::SetProperty faulty");
  }

  assembly->UnSetProperty(right[1], Visibility);
  ret = assembly->GetProperty(right[1], Visibility);

  if (!ret.empty())
  {
    throw std::runtime_error("vtkDataAssembly::UnSetProperty faulty");
  }

  assembly->SetProperty(right[1], Visibility, "false");
  pdc->SetDataAssembly(assembly);

  vtkNew<vtkExtractBlockUsingDataAssembly> extractor;
  extractor->SetInputDataObject(pdc);
  extractor->AddNodePath("//left");
  extractor->AddNodePath("//right");
  extractor->Update();

  extractor->SetInputDataObject(pdc);
  return extractor;
}

} // Anon namespace

int TestPartitionedDataSetCollectionMapper(int argc, char* argv[])
{
  auto eb = CreateSamplePartitionedDataSetCollection();
  vtkNew<vtkPartitionedDataSetCollectionMapper> mapper;

  mapper->SetInputConnection(eb->GetOutputPort());

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);

  vtkNew<vtkRenderWindow> win;
  vtkNew<vtkRenderer> ren;
  win->AddRenderer(ren);

  ren->AddActor(actor);
  ren->SetBackground(0, 0, 0);
  win->SetSize(450, 450);
  ren->ResetCamera();
  ren->GetActiveCamera()->Zoom(1);
  ren->ResetCameraClippingRange();
  win->Render();

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(win);

  int retVal = vtkRegressionTestImage(win);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
