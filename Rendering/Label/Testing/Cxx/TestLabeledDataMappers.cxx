/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestLabeledDataMappers.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// this test verifies that vtkFastLabelDataMapper works as expected

#include "vtkTestUtilities.h"

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkFastLabeledDataMapper.h"
#include "vtkFloatArray.h"
#include "vtkIdFilter.h"
#include "vtkNew.h"
#include "vtkPlaneSource.h"
#include "vtkPointData.h"
#include "vtkPolyDataMapper.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSelectVisiblePoints.h"
#include "vtkSphereSource.h"
#include "vtkStringArray.h"
#include "vtkTextActor.h"
#include "vtkTextProperty.h"
#include "vtkTransform.h"
#include "vtkTransformPolyDataFilter.h"
#include "vtkTrivialProducer.h"

#include "vtkDataSetAttributes.h"
#include "vtkDataSetMapper.h"
#include "vtkExtractSelectionLegacy.h"
#include "vtkHardwareSelector.h"
#include "vtkIdTypeArray.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkProperty.h"
#include "vtkRendererCollection.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"

#include <array>

namespace
{

vtkNew<vtkFastLabeledDataMapper> ldm;
vtkNew<vtkTransform> matrix;
vtkNew<vtkPlaneSource> plane;
vtkNew<vtkIdFilter> ids;
vtkNew<vtkTextActor> tactor;

class KeyPressInteractorStyle : public vtkInteractorStyleTrackballCamera
{
public:
  static KeyPressInteractorStyle* New();
  vtkTypeMacro(KeyPressInteractorStyle, vtkInteractorStyleTrackballCamera);

  virtual void OnKeyPress()
  {
    // Get the keypress
    std::string key = this->Interactor->GetKeySym();

    // "s" for "s"elect
    if (key.compare("s") == 0)
    {
      vtkNew<vtkHardwareSelector> selector;
      selector->SetRenderer(
        this->Interactor->GetRenderWindow()->GetRenderers()->GetFirstRenderer());
      int* temp = this->Interactor->GetRenderWindow()->GetSize();
      unsigned int windowSize[4];
      windowSize[0] = temp[2] + 1;
      windowSize[1] = temp[3] + 1;
      windowSize[2] = temp[0] - 1;
      windowSize[3] = temp[1] - 1;
      selector->SetArea(windowSize);
      selector->SetFieldAssociation(vtkDataObject::FIELD_ASSOCIATION_POINTS);
      vtkSelection* selection = selector->Select();
      std::cout << "Selection has " << selection->GetNumberOfNodes() << " nodes." << std::endl;
      // selection->PrintSelf(cout, vtkIndent(0));
#if 0
        for (unsigned int cnt = 0; cnt < selection->GetNumberOfNodes(); cnt++)
        {
          auto n = selection->GetNode(cnt);
          vtkIdTypeArray *da = vtkIdTypeArray::SafeDownCast(n->GetSelectionData()->GetArray("SelectedIds"));
          cout << da->GetClassName() << endl;
          for (int tup = 0; tup < da->GetNumberOfTuples(); tup++)
          {
            cout << "  ID[" << tup << "]=" << da->GetValue(tup) << endl;
          }
        }
#endif
      ids->Update();
      extractSelection->SetInputData(0, ids->GetOutput());
      extractSelection->SetInputData(1, selection);
      extractSelection->Update();
      selection->Delete();
      mapper->ScalarVisibilityOff();
      mapper->SetInputConnection(extractSelection->GetOutputPort());

      actor->SetMapper(mapper);
      actor->GetProperty()->SetColor(1, 0, 0);
      actor->GetProperty()->SetPointSize(40);
      static bool hasset = false;
      if (!hasset)
      {
        this->Renderer->AddActor(actor);
        hasset = true;
      }
    }

    static int cnt = -1;
    if (key.compare("c") == 0) // change font selection
    {
      cnt = (cnt - 1) % 7;
    }
    if (key.compare("C") == 0) // change font selection
    {
      cnt = (cnt + 1) % 7;
    }
    if (key.compare("C") == 0 || key.compare("c") == 0) // change font selection
    {
      vtkNew<vtkTextProperty> p;
      p->SetBackgroundColor(0.5, 0.5, 0.5);
      // p->ItalicOn();
      // p->SetOrientation(45.0);
      switch (cnt)
      {
        case 0:
          cout << "Arial" << endl;
          p->SetFontFamilyAsString("Arial");
          p->SetColor(1.0, 1.0, 1.0);
          p->SetBackgroundColor(1.0, 0.0, 0.0);
          p->SetFontSize(24);
          break;
        case 1:
          /*
          cout << "Arial Red Italic" << endl;
          p->SetFontFamilyAsString("Arial");
          p->SetColor(1.0, 0.0, 0.0);
          p->SetBackgroundColor(1.0, 0.0, 0.0);
          p->SetFontSize(24);
          p->ItalicOn();
          */
          cout << "Arial grey w blue frame" << endl;
          p->SetFontFamilyAsString("Arial");
          p->SetColor(0.5, 0.5, 0.5);
          p->SetBackgroundColor(1.0, 1.0, 1.0);
          p->SetBackgroundOpacity(1.0);
          p->SetFontSize(24);
          p->FrameOn();
          p->SetFrameWidth(4);
          p->SetFrameColor(0.0, 0.0, 0.8);
          break;
        case 2:
          cout << "Courier" << endl;
          p->SetFontFamilyAsString("Courier");
          p->SetColor(0.0, 1.0, 0.0);
          p->SetBackgroundColor(0.5, 0.0, 0.5);
          p->SetBackgroundOpacity(0.9);
          p->SetFontSize(32);
          break;
        case 3:
          cout << "Times" << endl;
          p->SetFontFamilyAsString("Times");
          p->SetColor(0.0, 0.0, 1.0);
          p->SetBackgroundColor(1.0, 0.7, 0.4);
          p->SetBackgroundOpacity(0.1);
          p->SetFontSize(38);
          break;
        case 4:
          cout << "Courier Frames" << endl;
          p->SetFontFamilyAsString("Courier");
          p->SetColor(0.0, 0.0, 1.0);
          p->SetFontSize(36);
          p->SetBackgroundColor(1.0, 0.5, 1.0);
          p->SetBackgroundOpacity(1.0);
          p->FrameOn();
          p->SetFrameWidth(2);
          break;
        case 5:
          cout << "Courier Frames BIGGER" << endl;
          p->SetFontFamilyAsString("Courier");
          p->SetColor(0.0, 0.0, 1.0);
          p->SetFontSize(64);
          p->SetBackgroundColor(1.0, 0.5, 1.0);
          p->SetFrameColor(0.0, 0.5, 0.5);
          p->SetBackgroundOpacity(1.0);
          p->FrameOn();
          p->SetFrameWidth(4);
          break;
        case 6:
          cout << "Inconsolata" << endl;
          p->SetFontFamily(VTK_FONT_FILE);
          // relative path in this example, so has to be in same directory user runs from
          p->SetFontFile("Inconsolata.otf");
          // see vtkResourceFileLocator for a pattern to follow to manufacture an
          // absolute path to look for instead for distributable binaries etc
          p->SetColor(1.0, 1.0, 1.0);
          p->SetFontSize(32);
          p->FrameOff();
          p->SetBackgroundColor(0.8, 0.0, 0.8);
          p->SetBackgroundOpacity(1.0);
          break;
      }
      tactor->SetTextProperty(p);
      ldm->SetLabelTextProperty(p, 1);
      this->Renderer->GetRenderWindow()->Render();
    }

    // "M" for "M"ore data
    if (key.compare("M") == 0)
    {
      int res[2];
      plane->GetResolution(res[0], res[1]);
      res[1] = res[1] * 2;
      plane->SetResolution(res[0], res[1]);
      this->Renderer->GetRenderWindow()->Render();
    }
    // "m" for "l"ess data?
    if (key.compare("m") == 0)
    {
      int res[2];
      plane->GetResolution(res[0], res[1]);
      res[1] = res[1] / 2;
      plane->SetResolution(res[0], res[1]);
      this->Renderer->GetRenderWindow()->Render();
    }
    // "B" for "B"igger transform
    if (key.compare("B") == 0)
    {
      scale[0] = scale[0] * 2;
      matrix->Scale(scale);
      this->Renderer->GetRenderWindow()->Render();
    }
    // "b" for "s"maller tranform?
    if (key.compare("b") == 0)
    {
      scale[0] = scale[0] / 2;
      matrix->Scale(scale);
      this->Renderer->GetRenderWindow()->Render();
    }
    if (key.compare("G") == 0)
    {
      vtkTextProperty* p = ldm->GetLabelTextProperty(0);
      int fsize = p->GetFontSize();
      fsize += 2;
      cerr << fsize << endl;
      p->SetFontSize(fsize);
      tactor->SetTextProperty(p);
      ldm->SetLabelTextProperty(p, 0);
      this->Renderer->GetRenderWindow()->Render();
    }
    if (key.compare("g") == 0)
    {
      vtkTextProperty* p = ldm->GetLabelTextProperty(0);
      int fsize = p->GetFontSize();
      fsize -= 2;
      cerr << fsize << endl;
      p->SetFontSize(fsize);
      tactor->SetTextProperty(p);
      ldm->SetLabelTextProperty(p, 0);
      this->Renderer->GetRenderWindow()->Render();
    }

    // "P" or "p" to toggle Perspective and Projection.
    if (key.compare("P") == 0 || key.compare("p") == 0)
    {
      vtkCamera* cam = this->Renderer->GetActiveCamera();
      cam->SetParallelProjection(!cam->GetParallelProjection());
      this->Renderer->GetRenderWindow()->Render();
    }

    // Forward events
    vtkInteractorStyleTrackballCamera::OnKeyPress();
  }

  vtkRenderer* Renderer;
  vtkNew<vtkActor> actor;
  vtkNew<vtkDataSetMapper> mapper;
  vtkNew<vtkExtractSelectionLegacy> extractSelection;
  double scale[3] = { 1.0, 1.0, 1.0 };
};
vtkStandardNewMacro(KeyPressInteractorStyle);

void AddTextProperty(int idx, int font, int fontSize, int frameWidth, std::array<double, 4> color,
  std::array<double, 4> bgColor, std::array<double, 4> frameColor)
{
  vtkNew<vtkTextProperty> tprop;
  tprop->SetFontFamily(font);
  tprop->SetColor(color.data());
  tprop->SetOpacity(color[3]);
  tprop->SetBackgroundColor(bgColor.data());
  tprop->SetBackgroundOpacity(bgColor[3]);
  tprop->SetFontSize(fontSize);
  tprop->SetFrame(frameWidth > 0);
  tprop->SetFrameWidth(frameWidth);
  tprop->SetFrameColor(frameColor.data()); // alpha unused
  ldm->SetLabelTextProperty(tprop, idx);
}

} // end anon namespace

int TestLabeledDataMappers(int, char* [])
{
  // Create some data to label
  plane->SetResolution(10, 10);

  vtkNew<vtkTransformPolyDataFilter> xform;
  xform->SetInputConnection(plane->GetOutputPort());

  // matrix->RotateZ(90);
  // matrix->Scale(100, 1000, 1);
  // matrix->Translate(1000, 10, 10000);
  xform->SetTransform(matrix);

  // Generate ids for labeling
  ids->SetInputConnection(xform->GetOutputPort());
  ids->PointIdsOn();
  ids->Update();

  vtkNew<vtkIntArray> types;
  types->SetNumberOfComponents(1);
  types->SetName("types");
  vtkNew<vtkStringArray> names;
  names->SetName("names");
  char buf[30];
  vtkNew<vtkFloatArray> frames;
  frames->SetNumberOfComponents(3);
  frames->SetName("frames");
  vtkDataSet* ds = ids->GetOutput();
  for (int i = 0; i < ds->GetNumberOfPoints(); i++)
  {
    types->InsertNextValue(i % 10);
    snprintf(buf, 30, "Z_%d_a", i);
    names->InsertNextValue(buf);
    double v = i / (double)ds->GetNumberOfPoints();
    frames->InsertNextTuple3(v, v, v);
  }
  ds->GetPointData()->AddArray(types);
  ds->GetPointData()->AddArray(names);
  ds->GetPointData()->AddArray(frames);

  AddTextProperty(0, VTK_TIMES, 24, 2, { 1., 0., 0., 1. }, { 0., 1., .0, 1. }, { .0, .0, .1, 1. });
  AddTextProperty(1, VTK_ARIAL, 24, 4, { 1., 1., 1., 1. }, { .2, 1., .2, 1. }, { .1, .6, .6, 1. });
  AddTextProperty(
    2, VTK_COURIER, 24, 8, { 0., 0., 0., 1. }, { .8, 1., .8, 1. }, { .8, .2, .2, 1. });
  AddTextProperty(3, VTK_ARIAL, 12, 1, { .8, 1., .2, 1. }, { .1, .4, .2, 1. }, { 0., 0., 0., 1. });
  AddTextProperty(4, VTK_ARIAL, 32, 4, { .5, .5, .2, 1. }, { 0., 0., 1., 1. }, { .8, .5, .3, 1. });
  AddTextProperty(5, VTK_TIMES, 16, 3, { 1., .2, 1., 1. }, { .2, 1., .6, 1. }, { .1, 0., .3, 1. });
  AddTextProperty(
    6, VTK_COURIER, 18, 0, { 1., 1., 1., 1. }, { 0., 0., 0., 0. }, { 0., 0., 0., 0. });
  AddTextProperty(
    7, VTK_COURIER, 22, 1, { 0., 0., 0., 1. }, { .2, 1., .2, 1. }, { 0., 0., 0., 1. });
  AddTextProperty(8, VTK_TIMES, 18, 1, { 0., 1., 1., 1. }, { 0., 0., 0., 1. }, { 1., 1., 1., 1. });
  AddTextProperty(9, VTK_ARIAL, 24, 4, { 1., 5., 5., 1. }, { .5, .5, 1., 1. }, { .5, 1., .5, 1. });

  ldm->SetLabelModeToLabelFieldData();
  ldm->SetFieldDataName("names");
  ldm->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "types");
  // ldm->SetFrameColorsName("frames"); //comment this out to use TextProperty colors
#define INSRC 0
#if INSRC == 0
  ldm->SetInputConnection(ids->GetOutputPort());
#endif
#if INSRC == 1
  vtkNew<vtkPolyData> pd;
  pd->ShallowCopy(ids->GetOutput());
  ldm->SetInputData(pd);
  // ldm->SetInputData(ids->GetOutput());
#endif
#if INSRC == 2
  vtkNew<vtkTrivialProducer> tp;
  tp->SetOutput(ids->GetOutput());
  ldm->SetInputConnection(tp->GetOutputPort());
#endif
  vtkNew<vtkActor> pointLabels;
  pointLabels->SetMapper(ldm);

  vtkNew<vtkPolyDataMapper> boundsmapper;
  boundsmapper->SetInputConnection(ids->GetOutputPort());
  vtkNew<vtkActor> boundsactor;
  boundsactor->GetProperty()->SetRepresentationToWireframe();
  boundsactor->SetMapper(boundsmapper);

  tactor->SetInput("0 1 3 12 Z_61_a 102");
  vtkNew<vtkTextProperty> p;
  p->SetFontFamilyAsString("Arial");
  p->SetFontSize(24);
  p->SetColor(1.0, 1.0, 1.0);
  p->SetBackgroundColor(1.0, 0.0, 0.0);
  tactor->SetTextProperty(p);

  vtkNew<vtkPolyDataMapper> originMapper;
  originMapper->SetInputConnection(ids->GetOutputPort());

  vtkNew<vtkActor> originActor;
  originActor->SetMapper(originMapper);
  originActor->GetProperty()->SetRepresentationToPoints();
  originActor->GetProperty()->RenderPointsAsSpheresOn();
  originActor->GetProperty()->SetPointSize(5);

  // Rendering setup
  vtkNew<vtkRenderer> ren;
  ren->AddActor(pointLabels);
  ren->AddActor(originActor);
  // ren->AddActor(boundsactor);
  ren->AddActor(tactor);

  ren->SetBackground(.5, .5, 6.);

  vtkNew<vtkRenderWindow> renWin;
  renWin->AddRenderer(ren);
  // vtkOpenGLRenderWindow::SetGlobalMaximumNumberOfMultiSamples(128);
  renWin->SetMultiSamples(0);
  renWin->SetSize(500, 500);

  vtkNew<vtkRenderWindowInteractor> renderWindowInteractor;
  renderWindowInteractor->SetRenderWindow(renWin);

  vtkNew<KeyPressInteractorStyle> style;
  style->Renderer = ren;
  renderWindowInteractor->SetInteractorStyle(style);
  style->SetCurrentRenderer(ren);

  renWin->Render();
  ldm->ReleaseGraphicsResources(renWin);
  renWin->Render();

  int retVal = 0; // vtkRegressionTestImage( renWin );
  // if ( retVal == vtkRegressionTester::DO_INTERACTOR)
  //{
  renderWindowInteractor->Start();
  //}

  style->SetCurrentRenderer(nullptr);

  pointLabels->SetMapper(nullptr);

  return !retVal;
}
