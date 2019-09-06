/*===========================================================================*/
/* Distributed under OSI-approved BSD 3-Clause License.                      */
/* For copyright, see the following accompanying files or https://vtk.org:   */
/* - VTK-Copyright.txt                                                       */
/* - Sandia-Copyright.txt                                                    */
/*===========================================================================*/

#include "vtkActor.h"
#include "vtkCircularLayoutStrategy.h"
#include "vtkFast2DLayoutStrategy.h"
#include "vtkGraphLayout.h"
#include "vtkGraphMapper.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSmartPointer.h"
#include "vtkTestUtilities.h"
#include "vtkTulipReader.h"

#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

int TestTulipReader(int argc, char* argv[])
{
  char* file = vtkTestUtilities::ExpandDataFileName(argc, argv,
                                                    "Data/Infovis/small.tlp");
  VTK_CREATE(vtkTulipReader, reader);
  reader->SetFileName(file);
  delete[] file;

  VTK_CREATE(vtkCircularLayoutStrategy, strategy);
  VTK_CREATE(vtkGraphLayout, layout);
  layout->SetInputConnection(reader->GetOutputPort());
  layout->SetLayoutStrategy(strategy);

  VTK_CREATE(vtkGraphMapper, mapper);
  mapper->SetInputConnection(layout->GetOutputPort());
  VTK_CREATE(vtkActor, actor);
  actor->SetMapper(mapper);
  VTK_CREATE(vtkRenderer, ren);
  ren->AddActor(actor);
  VTK_CREATE(vtkRenderWindowInteractor, iren);
  VTK_CREATE(vtkRenderWindow, win);
  win->AddRenderer(ren);
  win->SetInteractor(iren);

  int retVal = vtkRegressionTestImage(win);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Initialize();
    iren->Start();

    retVal = vtkRegressionTester::PASSED;
  }

  return !retVal;
}
