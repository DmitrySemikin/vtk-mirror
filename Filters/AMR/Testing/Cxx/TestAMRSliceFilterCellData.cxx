/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestAMRSliceFilterCellData.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Test vtkAMRSliceFilter filter.

#include <vtkActor.h>
#include <vtkAMRSliceFilter.h>
#include <vtkCamera.h>
#include <vtkColorTransferFunction.h>
#include <vtkCompositePolyDataMapper2.h>
#include <vtkDataSetSurfaceFilter.h>
#include <vtkImageToAMR.h>
#include <vtkLookupTable.h>
#include <vtkNamedColors.h>
#include <vtkNew.h>
#include <vtkOverlappingAMR.h>
#include <vtkPointDataToCellData.h>
#include <vtkRegressionTestImage.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRTAnalyticSource.h>

#include <array>

int TestAMRSliceFilterCellData(int argc, char *argv[])
{
    vtkNew<vtkRTAnalyticSource> imgSrc;

    vtkNew<vtkPointDataToCellData> cdSrc;
    cdSrc->SetInputConnection(imgSrc->GetOutputPort());

    vtkNew<vtkImageToAMR> amr;
    amr->SetInputConnection(cdSrc->GetOutputPort());
    amr->SetNumberOfLevels(3);

    vtkNew<vtkAMRSliceFilter> slicer;
    slicer->SetInputConnection(amr->GetOutputPort());
    slicer->SetNormal(1);
    slicer->SetOffsetFromOrigin(10);
    slicer->SetMaxResolution(2);

    vtkNew<vtkDataSetSurfaceFilter> surface;
    surface->SetInputConnection(slicer->GetOutputPort());
    surface->Update();

    // color map
    vtkNew<vtkNamedColors> colors;

    vtkNew<vtkColorTransferFunction> colormap;
    colormap->SetColorSpaceToDiverging();
    colormap->AddRGBPoint(0.0, 1.0, 0.0, 0.0);
    colormap->AddRGBPoint(1.0, 0.0, 0.0, 1.0);

    vtkNew<vtkLookupTable> lut;
    lut->SetNumberOfColors(256);
    for (int i = 0; i < lut->GetNumberOfColors(); ++i)
    {
        std::array<double, 4> color;
        colormap->GetColor(static_cast<double>(i) / lut->GetNumberOfColors(), color.data());
        color[3] = 1.0;
        lut->SetTableValue(i, color.data());
    }
    lut->Build();

    // Rendering
    vtkNew<vtkCompositePolyDataMapper2> mapper;
    mapper->SetInputConnection(surface->GetOutputPort());
    mapper->SetLookupTable(lut);
    mapper->SetScalarRange(37.3531, 276.829);
    mapper->SetScalarModeToUseCellFieldData();
    mapper->SetInterpolateScalarsBeforeMapping(1);
    mapper->SelectColorArray("RTData");

    vtkNew<vtkActor> actor;
    actor->SetMapper(mapper);

    vtkNew<vtkRenderer> ren;
    vtkNew<vtkRenderWindow> rwin;
    rwin->AddRenderer(ren);
    vtkNew<vtkRenderWindowInteractor> iren;
    iren->SetRenderWindow(rwin);

    ren->AddActor(actor);
    ren->GetActiveCamera()->SetPosition(15, 0, 0);
    ren->GetActiveCamera()->SetFocalPoint(0, 0, 0);
    ren->ResetCamera();
    rwin->SetSize(300, 300);
    iren->Initialize();
    rwin->Render();

    int retVal = vtkRegressionTestImage(rwin);
    if (retVal == vtkRegressionTester::DO_INTERACTOR)
    {
        iren->Start();
    }

    return !retVal;
}
