/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPointSetReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPointSetReader.h"

#include "vtkCellArray.h"
#include "vtkFieldData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointSet.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <vector>

vtkStandardNewMacro(vtkPointSetReader);

//------------------------------------------------------------------------------
vtkPointSetReader::vtkPointSetReader() = default;

//------------------------------------------------------------------------------
vtkPointSetReader::~vtkPointSetReader() = default;

//------------------------------------------------------------------------------
vtkPointSet* vtkPointSetReader::GetOutput()
{
  return this->GetOutput(0);
}

//------------------------------------------------------------------------------
vtkPointSet* vtkPointSetReader::GetOutput(int idx)
{
  return vtkPointSet::SafeDownCast(this->GetOutputDataObject(idx));
}

//------------------------------------------------------------------------------
void vtkPointSetReader::SetOutput(vtkPointSet* output)
{
  this->GetExecutive()->SetOutputData(0, output);
}

//------------------------------------------------------------------------------
int vtkPointSetReader::ReadMeshSimple(const std::string& fname, vtkDataObject* doOutput)
{
  vtkIdType numPts = 0;
  char line[256];
  vtkIdType npts;
  vtkPointSet* output = vtkPointSet::SafeDownCast(doOutput);

  vtkDebugMacro(<< "Reading vtk point data...");

  if (!(this->OpenVTKFile(fname.c_str())) || !this->ReadHeader(fname.c_str()))
  {
    return 1;
  }

  //
  // Read polygonal data specific stuff
  //
  if (!this->ReadString(line))
  {
    vtkErrorMacro(<< "Data file ends prematurely!");
    this->CloseVTKFile();
    return 1;
  }

  if (!strncmp(this->LowerCase(line), "dataset", 7))
  {
    //
    // Make sure we're reading right type of geometry
    //
    if (!this->ReadString(line))
    {
      vtkErrorMacro(<< "Data file ends prematurely!");
      this->CloseVTKFile();
      return 1;
    }

    if (strncmp(this->LowerCase(line), "pointset", 8) != 0)
    {
      vtkErrorMacro(<< "Cannot read dataset type: " << line);
      this->CloseVTKFile();
      return 1;
    }
    //
    // Might find points
    //
    while (true)
    {
      if (!this->ReadString(line))
      {
        break;
      }

      if (!strncmp(this->LowerCase(line), "field", 5))
      {
        vtkFieldData* fd = this->ReadFieldData();
        output->SetFieldData(fd);
        fd->Delete(); // ?
      }
      else if (!strncmp(line, "points", 6))
      {
        if (!this->Read(&numPts))
        {
          vtkErrorMacro(<< "Cannot read number of points!");
          this->CloseVTKFile();
          return 1;
        }

        this->ReadPointCoordinates(output, numPts);
      }
      else if (!strncmp(line, "point_data", 10))
      {
        if (!this->Read(&npts))
        {
          vtkErrorMacro(<< "Cannot read point data!");
          this->CloseVTKFile();
          return 1;
        }

        if (npts != numPts)
        {
          vtkErrorMacro(<< "Number of points don't match number data values!");
          return 1;
        }

        this->ReadPointData(output, npts);
        break; // out of this loop
      }

      else
      {
        vtkErrorMacro(<< "Unrecognized keyword: " << line);
        this->CloseVTKFile();
        return 1;
      }
    }

    if (!output->GetPoints())
      vtkWarningMacro(<< "No points read!");
  }
  else if (!strncmp(line, "point_data", 10))
  {
    vtkWarningMacro(<< "No geometry defined in data file!");
    if (!this->Read(&numPts))
    {
      vtkErrorMacro(<< "Cannot read point data!");
      this->CloseVTKFile();
      return 1;
    }

    this->ReadPointData(output, numPts);
  }
  else
  {
    vtkErrorMacro(<< "Unrecognized keyword: " << line);
  }
  this->CloseVTKFile();

  return 1;
}

//------------------------------------------------------------------------------
int vtkPointSetReader::FillOutputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPointSet");
  return 1;
}

//------------------------------------------------------------------------------
void vtkPointSetReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
