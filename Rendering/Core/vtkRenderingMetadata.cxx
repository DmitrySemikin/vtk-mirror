/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRenderingMetadata.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkRenderingMetadata.h"

#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkRenderingMetadata);

vtkRenderingMetadata::vtkRenderingMetadata()
{
}

int vtkRenderingMetadata::GetBackend()
{
#ifdef VTK_OPENGL2
  return VTK_OPENGL2_BACKEND;
#else
  return VTK_OPENGL_BACKEND;
#endif
}
