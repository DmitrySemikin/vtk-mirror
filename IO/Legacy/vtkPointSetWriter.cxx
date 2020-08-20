/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPointSetWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPointSetWriter.h"

#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkPointSet.h"

#if !defined(_WIN32) || defined(__CYGWIN__)
#include <unistd.h> /* unlink */
#else
#include <io.h> /* unlink */
#endif

vtkStandardNewMacro(vtkPointSetWriter);

void vtkPointSetWriter::WriteData()
{
  ostream* fp;
  vtkPointSet* input = this->GetInput();

  vtkDebugMacro(<< "Writing vtk point data...");

  if (!(fp = this->OpenVTKFile()) || !this->WriteHeader(fp))
  {
    if (fp)
    {
      if (this->FileName)
      {
        vtkErrorMacro("Ran out of disk space; deleting file: " << this->FileName);
        this->CloseVTKFile(fp);
        unlink(this->FileName);
      }
      else
      {
        this->CloseVTKFile(fp);
        vtkErrorMacro("Could not read memory header. ");
      }
    }
    return;
  }
  //
  // Write polygonal data specific stuff
  //
  *fp << "DATASET POINTSET\n";

  //
  // Write data owned by the dataset
  int errorOccured = 0;
  if (!this->WriteDataSetData(fp, input))
  {
    errorOccured = 1;
  }
  if (!errorOccured && !this->WritePoints(fp, input->GetPoints()))
  {
    errorOccured = 1;
  }

  if (!errorOccured && !this->WritePointData(fp, input))
  {
    errorOccured = 1;
  }

  if (errorOccured)
  {
    if (this->FileName)
    {
      vtkErrorMacro("Ran out of disk space; deleting file: " << this->FileName);
      this->CloseVTKFile(fp);
      unlink(this->FileName);
    }
    else
    {
      vtkErrorMacro("Error writing data set to memory");
      this->CloseVTKFile(fp);
    }
    return;
  }
  this->CloseVTKFile(fp);
}

int vtkPointSetWriter::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPointSet");
  return 1;
}

vtkPointSet* vtkPointSetWriter::GetInput()
{
  return vtkPointSet::SafeDownCast(this->Superclass::GetInput());
}

vtkPointSet* vtkPointSetWriter::GetInput(int port)
{
  return vtkPointSet::SafeDownCast(this->Superclass::GetInput(port));
}

void vtkPointSetWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
