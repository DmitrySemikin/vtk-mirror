/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPLYReaderFaceTextureOnly.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME Test of TestPLYReaderFaceTextureOnly.cxx
// .SECTION Description
// Reads face texture and checks against expected data.

#include "vtkPLYWriter.h"

#include "vtkCellData.h"
#include "vtkFloatArray.h"
#include "vtkPLYReader.h"
#include "vtkPolyData.h"
#include "vtkPointData.h"
#include "vtkTestUtilities.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <fstream>
#include <streambuf>

int TestPLYReaderFaceTextureOnly(int argc, char *argv[])
{
  // Read file name.
  const char* tempFileName = vtkTestUtilities::ExpandDataFileName(
    argc, argv, "Data/squareTexturedFaces.ply");
  std::string filename = tempFileName;
  delete [] tempFileName;

  // Create the reader.
  vtkNew<vtkPLYReader> reader;
  reader->SetFileName(filename.c_str());
  reader->DuplicatePointsForFaceTextureOff();
  reader->ReadFaceTextureOnlyOn();
  reader->Update();
  vtkPolyData* data = reader->GetOutput();
  vtkDataArray* texcoord = data->GetCellData()->GetTCoords();
  double expected[2][8] = {
    {0, 0, 1, 0, 1, 0.25, 0, 0.25},
    {0, 0.75, 1, 0.75, 1, 1, 0, 1}
  };
  double t[2][8];
  double *t0 = texcoord->GetTuple(0);
  std::copy(t0, t0 + 8, &t[0][0]);
  double *t1 = texcoord->GetTuple(1);
  std::copy(t1, t1 + 8, &t[1][0]);
  if (! std::equal(&expected[0][0], &expected[0][8], &t[0][0]) ||
      ! std::equal(&expected[1][0], &expected[1][8], &t[1][0]))
  {
    std::cout << "Texture coordinates are not identical." << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
