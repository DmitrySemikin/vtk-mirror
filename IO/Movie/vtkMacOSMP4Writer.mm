/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMacOSMP4Writer.mm

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMacOSMP4Writer.h"

#include "vtkObjectFactory.h"

#include <iostream>

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkMacOSMP4Writer);

//------------------------------------------------------------------------------
vtkMacOSMP4Writer::vtkMacOSMP4Writer()
{
}

//------------------------------------------------------------------------------
vtkMacOSMP4Writer::~vtkMacOSMP4Writer()
{
}

//------------------------------------------------------------------------------
void vtkMacOSMP4Writer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void vtkMacOSMP4Writer::Start()
{
}

//------------------------------------------------------------------------------
void vtkMacOSMP4Writer::Write()
{
}

//------------------------------------------------------------------------------
void vtkMacOSMP4Writer::End()
{
}
