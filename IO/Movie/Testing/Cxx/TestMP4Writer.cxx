/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestMP4Writer.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME Test of vtkMP4Writer
// .SECTION Description
//

#include "vtkImageCast.h"
#include "vtkImageData.h"
#include "vtkImageMandelbrotSource.h"
#include "vtkImageMapToColors.h"
#include "vtkLookupTable.h"
#include "vtkMP4Writer.h"
#include "vtksys/SystemTools.hxx"

int TestMP4Writer(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  int err = 0;
  int cc = 0;
  int exists = 0;
  unsigned long length = 0;
  vtkImageMandelbrotSource* Fractal0 = vtkImageMandelbrotSource::New();
  Fractal0->SetWholeExtent(0, 247, 0, 247, 0, 0);
  Fractal0->SetProjectionAxes(0, 1, 2);
  Fractal0->SetOriginCX(-1.75, -1.25, 0, 0);
  Fractal0->SetSizeCX(2.5, 2.5, 2, 1.5);
  Fractal0->SetMaximumNumberOfIterations(100);

  vtkImageCast* cast = vtkImageCast::New();
  cast->SetInputConnection(Fractal0->GetOutputPort());
  cast->SetOutputScalarTypeToUnsignedChar();

  vtkLookupTable* table = vtkLookupTable::New();
  table->SetTableRange(0, 100);
  table->SetNumberOfColors(100);
  table->Build();
  table->SetTableValue(99, 0, 0, 0);

  vtkImageMapToColors* colorize = vtkImageMapToColors::New();
  colorize->SetOutputFormatToRGB();
  colorize->SetLookupTable(table);
  colorize->SetInputConnection(cast->GetOutputPort());

  vtkMP4Writer* w = vtkMP4Writer::New();
  w->SetInputConnection(colorize->GetOutputPort());
  w->SetFileName("TestMP4Writer.mp4");
  cout << "Writing file TestMP4Writer.mp4..." << endl;
  w->Start();
  for (cc = 2; cc < 99; cc++)
  {
    cout << ".";
    Fractal0->SetMaximumNumberOfIterations(cc);
    table->SetTableRange(0, cc);
    table->SetNumberOfColors(cc);
    table->ForceBuild();
    table->SetTableValue(cc - 1, 0, 0, 0);
    w->Write();
  }
  w->End();
  cout << endl;
  cout << "Done writing file TestMP4Writer.mp4..." << endl;
  w->Delete();

  exists = (int)vtksys::SystemTools::FileExists("TestMP4Writer.mp4");
  length = vtksys::SystemTools::FileLength("TestMP4Writer.mp4");
  cout << "TestMP4Writer.mp4 file exists: " << exists << endl;
  cout << "TestMP4Writer.mp4 file length: " << length << endl;
  if (!exists)
  {
    err = 3;
    cerr << "ERROR: 3 - Test failing because TestMP4Writer.mp4 file doesn't exist..." << endl;
  }
  if (0 == length)
  {
    err = 4;
    cerr << "ERROR: 4 - Test failing because TestMP4Writer.mp4 file has zero length..." << endl;
  }

  colorize->Delete();
  table->Delete();
  cast->Delete();
  Fractal0->Delete();

  // err == 0 means test passes...
  //
  return err;
}
