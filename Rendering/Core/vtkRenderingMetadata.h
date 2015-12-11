/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRenderingMetadata.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkRenderingMetadata - Retrieve data about VTK itself
//
// .SECTION Description
// Use the static methods on vtkRenderingMetadata to query at run-time various
// aspects of the VTK in use.

#ifndef vtkRenderingMetadata_h
#define vtkRenderingMetadata_h

#include "vtkObject.h"
#include "vtkRenderingCoreModule.h" // For export macro

enum
{
  VTK_OPENGL_BACKEND = 1,
  VTK_OPENGL2_BACKEND = 2,
  VTK_UNKNOWN_BACKEND
};

class VTKRENDERINGCORE_EXPORT vtkRenderingMetadata : public vtkObject
{
public:
  vtkTypeMacro(vtkRenderingMetadata, vtkObject);
  static vtkRenderingMetadata *New();

  static int GetBackend();

protected:
  vtkRenderingMetadata();

private:
  vtkRenderingMetadata(const vtkRenderingMetadata&);  // Not implemented.
  void operator=(const vtkRenderingMetadata&);  // Not implemented.
};

#endif
