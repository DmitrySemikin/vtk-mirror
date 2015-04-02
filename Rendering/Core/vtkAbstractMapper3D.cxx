/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAbstractMapper3D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAbstractMapper3D.h"
#include "vtkDataSet.h"
#include "vtkMath.h"
#include "vtkMatrix4x4.h"
#include "vtkPlaneCollection.h"

#include <algorithm>

// Construct with initial range (0,1).
vtkAbstractMapper3D::vtkAbstractMapper3D()
{
#ifndef VTK_LEGACY_REMOVE
  vtkMath::UninitializeBounds(this->LegacyBounds);
  this->LegacyCenter[0] = this->LegacyCenter[1] = this->LegacyCenter[2] = 0.0;
  this->InGetBounds = false;
#endif // VTK_LEGACY_REMOVE
}

// Get the bounds for this Prop as (Xmin,Xmax,Ymin,Ymax,Zmin,Zmax).
#ifndef VTK_LEGACY_REMOVE
double* vtkAbstractMapper3D::GetBounds()
{
  VTK_LEGACY_REPLACED_BODY(
        double *vtkAbstractMapper3D::GetBounds(), "VTK 6.3",
        bool vtkAbstractMapper3D::GetBounds(vtkViewport*, double[6]))

  vtkBoundingBox bbox = this->ComputeBoundingBox(NULL);
  bbox.GetBounds(this->LegacyBounds);
  return bbox.IsValid() ? this->LegacyBounds : NULL;
}

void vtkAbstractMapper3D::GetBounds(double bounds[6])
{
  VTK_LEGACY_REPLACED_BODY(
        void vtkAbstractMapper3D::GetBounds(double[6]), "VTK 6.3",
        bool vtkAbstractMapper3D::GetBounds(vtkViewport*, double[6]))

  this->ComputeBoundingBox(NULL).GetBounds(bounds);
}

double* vtkAbstractMapper3D::GetCenter()
{
  VTK_LEGACY_REPLACED_BODY(
        double* vtkAbstractMapper3D::GetCenter(), "VTK 6.3",
        bool vtkAbstractMapper3D::GetCenter(vtkViewport*, double[3]))

  this->GetCenter(NULL, this->LegacyCenter);
  return this->LegacyCenter;
}

double vtkAbstractMapper3D::GetLength()
{
  VTK_LEGACY_REPLACED_BODY(
        double vtkAbstractMapper3D::GetLength(), "VTK 6.3",
        double vtkAbstractMapper3D::GetLength(vtkViewport*))

  return this->GetLength(NULL);
}
#endif // VTK_LEGACY_REMOVE


vtkBoundingBox vtkAbstractMapper3D::ComputeBoundingBox(vtkViewport *)
{
  vtkBoundingBox result;

#ifndef VTK_LEGACY_REMOVE
  if (this->InGetBounds)
    {
    vtkErrorMacro(<<"Missing ComputeBoundingBox override.");
    return result;
    }

  this->InGetBounds = true;
  // This will cause a compile time deprecation warning. This is needed to
  // support external subclasses that still use the deprecated virtual.
  double *b = this->GetBounds();
  this->InGetBounds = false;

  if (b)
    {
    result.AddBounds(b);
    }
  // If b is NULL, fall back to the new base implementation:
#endif // VTK_LEGACY_REMOVE

  return result;
}

bool vtkAbstractMapper3D::GetCenter(vtkViewport *vp, double center[3])
{
  vtkBoundingBox bounds = this->ComputeBoundingBox(vp);
  if (bounds.IsValid())
    {
    bounds.GetCenter(center);
    return true;
    }
  else
    {
    std::fill(center, center + 3, 0.);
    return false;
    }
}

double vtkAbstractMapper3D::GetLength(vtkViewport *vp)
{
  vtkBoundingBox bounds = this->ComputeBoundingBox(vp);
  return bounds.IsValid() ? bounds.GetDiagonalLength() : 0.;
}

void vtkAbstractMapper3D::GetClippingPlaneInDataCoords(
  vtkMatrix4x4 *propMatrix, int i, double hnormal[4])
{
  vtkPlaneCollection *clipPlanes = this->ClippingPlanes;
  const double *mat = *propMatrix->Element;

  if (clipPlanes)
    {
    int n = clipPlanes->GetNumberOfItems();
    if (i >= 0 && i < n)
      {
      // Get the plane
      vtkPlane *plane = clipPlanes->GetItem(i);
      double *normal = plane->GetNormal();
      double *origin = plane->GetOrigin();

      // Compute the plane equation
      double v1 = normal[0];
      double v2 = normal[1];
      double v3 = normal[2];
      double v4 = -(v1*origin[0] + v2*origin[1] + v3*origin[2]);

      // Transform normal from world to data coords
      hnormal[0] = v1*mat[0] + v2*mat[4] + v3*mat[8]  + v4*mat[12];
      hnormal[1] = v1*mat[1] + v2*mat[5] + v3*mat[9]  + v4*mat[13];
      hnormal[2] = v1*mat[2] + v2*mat[6] + v3*mat[10] + v4*mat[14];
      hnormal[3] = v1*mat[3] + v2*mat[7] + v3*mat[11] + v4*mat[15];

      return;
      }
    }

  vtkErrorMacro("Clipping plane index " << i << " is out of range.");
}

int vtkAbstractMapper3D::GetNumberOfClippingPlanes()
{
  int n = 0;

  if ( this->ClippingPlanes )
    {
    n = this->ClippingPlanes->GetNumberOfItems();
    }

  return n;
}

void vtkAbstractMapper3D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
