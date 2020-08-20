/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLPointSetReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLPointSetReader.h"
#include "vtkCellArray.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkPointSet.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnsignedCharArray.h"
#include "vtkXMLDataElement.h"

#include <cassert>

vtkStandardNewMacro(vtkXMLPointSetReader);

//------------------------------------------------------------------------------
vtkXMLPointSetReader::~vtkXMLPointSetReader()
{
  if (this->NumberOfPieces)
  {
    this->DestroyPieces();
  }
}

//------------------------------------------------------------------------------
void vtkXMLPointSetReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
const char* vtkXMLPointSetReader::GetDataSetName()
{
  return "PointSet";
}

//------------------------------------------------------------------------------
vtkPointSet* vtkXMLPointSetReader::GetOutput()
{
  return this->GetOutput(0);
}

//------------------------------------------------------------------------------
vtkPointSet* vtkXMLPointSetReader::GetOutput(int idx)
{
  return vtkPointSet::SafeDownCast(this->GetOutputDataObject(idx));
}

//------------------------------------------------------------------------------
void vtkXMLPointSetReader::GetOutputUpdateExtent(int& piece, int& numberOfPieces, int& ghostLevel)
{
  vtkInformation* outInfo = this->GetCurrentOutputInformation();
  piece = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  numberOfPieces = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  ghostLevel = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());
}

//------------------------------------------------------------------------------
vtkIdType vtkXMLPointSetReader::GetNumberOfCellsInPiece(int vtkNotUsed(piece))
{
  return 0;
}

//------------------------------------------------------------------------------
int vtkXMLPointSetReader::FillOutputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPointSet");
  return 1;
}
