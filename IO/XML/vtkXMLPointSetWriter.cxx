/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLPointSetWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLPointSetWriter.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkErrorCode.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationIntegerKey.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPointSet.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#define vtkXMLOffsetsManager_DoNotInclude
#include "vtkXMLOffsetsManager.h"
#undef vtkXMLOffsetsManager_DoNotInclude

vtkStandardNewMacro(vtkXMLPointSetWriter);

//------------------------------------------------------------------------------
vtkXMLPointSetWriter::vtkXMLPointSetWriter()
{
  this->VertsOM = new OffsetsManagerArray;
  this->LinesOM = new OffsetsManagerArray;
  this->StripsOM = new OffsetsManagerArray;
  this->PolysOM = new OffsetsManagerArray;
}

//------------------------------------------------------------------------------
vtkXMLPointSetWriter::~vtkXMLPointSetWriter()
{
  delete this->VertsOM;
  delete this->LinesOM;
  delete this->StripsOM;
  delete this->PolysOM;
}

//------------------------------------------------------------------------------
void vtkXMLPointSetWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
vtkPointSet* vtkXMLPointSetWriter::GetInput()
{
  return static_cast<vtkPointSet*>(this->Superclass::GetInput());
}

//------------------------------------------------------------------------------
const char* vtkXMLPointSetWriter::GetDataSetName()
{
  return "PointSet";
}

//------------------------------------------------------------------------------
const char* vtkXMLPointSetWriter::GetDefaultFileExtension()
{
  return "vtp";
}

//------------------------------------------------------------------------------
void vtkXMLPointSetWriter::AllocatePositionArrays()
{
  this->Superclass::AllocatePositionArrays();

  this->NumberOfVertsPositions = new unsigned long[this->NumberOfPieces];
  this->NumberOfLinesPositions = new unsigned long[this->NumberOfPieces];
  this->NumberOfStripsPositions = new unsigned long[this->NumberOfPieces];
  this->NumberOfPolysPositions = new unsigned long[this->NumberOfPieces];

  this->VertsOM->Allocate(this->NumberOfPieces, 2, this->NumberOfTimeSteps);
  this->LinesOM->Allocate(this->NumberOfPieces, 2, this->NumberOfTimeSteps);
  this->StripsOM->Allocate(this->NumberOfPieces, 2, this->NumberOfTimeSteps);
  this->PolysOM->Allocate(this->NumberOfPieces, 2, this->NumberOfTimeSteps);
}

//------------------------------------------------------------------------------
void vtkXMLPointSetWriter::DeletePositionArrays()
{
  this->Superclass::DeletePositionArrays();

  delete[] this->NumberOfVertsPositions;
  delete[] this->NumberOfLinesPositions;
  delete[] this->NumberOfStripsPositions;
  delete[] this->NumberOfPolysPositions;
}

//------------------------------------------------------------------------------
vtkIdType vtkXMLPointSetWriter::GetNumberOfInputCells()
{
  return 0;
}

//------------------------------------------------------------------------------
int vtkXMLPointSetWriter::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPointSet");
  return 1;
}
