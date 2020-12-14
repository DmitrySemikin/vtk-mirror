/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataProperties.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkDataProperties_h
#define vtkDataProperties_h

namespace vtkDataProperties
{

enum
{
  Visibility = 0,
  Pickability,
  Color,
  Opacity,
  Material
};

const char* GetStringFromDataProperties(int);
int GetPropertiesFromString(const char*);
}

#endif
