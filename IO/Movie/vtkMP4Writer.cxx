/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMP4Writer.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMP4Writer.h"

#include "vtkObjectFactory.h"

//------------------------------------------------------------------------------
vtkObjectFactoryNewMacro(vtkMP4Writer);

//------------------------------------------------------------------------------
vtkMP4Writer::vtkMP4Writer()
{
}

//------------------------------------------------------------------------------
vtkMP4Writer::~vtkMP4Writer()
{
}

//------------------------------------------------------------------------------
void vtkMP4Writer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FrameRate: " << this->FrameRate << endl;
  os << indent << "BitRate: " << this->BitRate << endl;
}
