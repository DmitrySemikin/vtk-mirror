/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCellData.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCellData.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkCellData);

void vtkCellData::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

int vtkCellData::CheckNumberOfComponents(vtkAbstractArray* aa,
                                         int attributeType)
{
  if (attributeType == TCOORDS)
  {
    // same as NOLIMIT
    return 1;
  }
  else
  {
    return vtkDataSetAttributes::CheckNumberOfComponents(aa, attributeType);
  }
}
