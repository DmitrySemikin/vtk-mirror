/*===========================================================================*/
/* Distributed under OSI-approved BSD 3-Clause License.                      */
/* For copyright, see the following accompanying files or https://vtk.org:   */
/* - VTK-Copyright.txt                                                       */
/* - Sandia-Copyright.txt                                                    */
/*===========================================================================*/
// -*- c++ -*-

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCompositeDataGeometryFilter.h"
#include "vtkLookupTable.h"
#include "vtkPolyDataMapper.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSLACReader.h"
#include "vtkTestUtilities.h"

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

int SLACReaderLinear(int argc, char *argv[])
{
  // Set up reader.
  VTK_CREATE(vtkSLACReader, reader);

  char *meshFileName = vtkTestUtilities::ExpandDataFileName(argc, argv,
                                  "Data/SLAC/ll-9cell-f523/ll-9cell-f523.ncdf");
  char *modeFileName = vtkTestUtilities::ExpandDataFileName(argc, argv,
              "Data/SLAC/ll-9cell-f523/mode0.l0.R2.457036E+09I2.778314E+04.m3");
  reader->SetMeshFileName(meshFileName);
  delete[] meshFileName;
  reader->AddModeFileName(modeFileName);
  delete[] modeFileName;

  reader->ReadInternalVolumeOn();
  reader->ReadExternalSurfaceOff();
  reader->ReadMidpointsOff();

  // Extract geometry that we can render.
  VTK_CREATE(vtkCompositeDataGeometryFilter, geometry);
  geometry->SetInputConnection(
                           reader->GetOutputPort(vtkSLACReader::VOLUME_OUTPUT));

  // Set up rendering stuff.
  VTK_CREATE(vtkPolyDataMapper, mapper);
  mapper->SetInputConnection(geometry->GetOutputPort());
  mapper->SetScalarModeToUsePointFieldData();
  mapper->ColorByArrayComponent("bfield", 1);
  mapper->UseLookupTableScalarRangeOff();
  mapper->SetScalarRange(0.0, 1e-08);

  VTK_CREATE(vtkLookupTable, lut);
  lut->SetHueRange(0.66667, 0.0);
  mapper->SetLookupTable(lut);

  VTK_CREATE(vtkActor, actor);
  actor->SetMapper(mapper);

  VTK_CREATE(vtkRenderer, renderer);
  renderer->AddActor(actor);
  vtkCamera *camera = renderer->GetActiveCamera();
  camera->SetPosition(-0.75, 0.0, 0.7);
  camera->SetFocalPoint(0.0, 0.0, 0.7);
  camera->SetViewUp(0.0, 1.0, 0.0);

  VTK_CREATE(vtkRenderWindow, renwin);
  renwin->SetSize(600, 150);
  renwin->AddRenderer(renderer);
  VTK_CREATE(vtkRenderWindowInteractor, iren);
  iren->SetRenderWindow(renwin);
  renwin->Render();

  // Do the test comparison.
  int retVal = vtkRegressionTestImage(renwin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
    retVal = vtkRegressionTester::PASSED;
  }

  return (retVal == vtkRegressionTester::PASSED) ? 0 : 1;
}
