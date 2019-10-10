/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPLYReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkCellTextureToPointTextureInternal_h
#define vtkCellTextureToPointTextureInternal_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkFloatArray.h"
#include "vtkIncrementalOctreePointLocator.h"
#include "vtkPolygon.h"


/**
 * Go over all cells that have cell textures (each cell has texture
 * coordinates for each point of the cell) and change that into point
 * texture. Duplicate points that have 2 or more different textures
 * comming from different cells.
 */
class VTKFILTERSGENERAL_EXPORT vtkCellTextureToPointTextureInternal
{
public:
  vtkCellTextureToPointTextureInternal();

  void Initialize(int numberOfPoints, float faceTextureTolerance);

  /**
   * Gets a `cell` and `texCoordsCell` and fills in
   * `texCoordsPoints` which is the texture point coordinates array
   * and modifies `cell`. The whole mesh is `output`
   */
  void DuplicatePoints(vtkPolygon* cell, float* texCoordsCell,
                       vtkFloatArray* texCoordsPoints, vtkPolyData *output);

private:
  /**
   * Create an extra point in 'data' with the same coordinates and data as
   * the point at cellPointIndex inside cell. This is to avoid texture artifacts
   * when you have one point with two different texture values (so the latter
   * value override the first. This results in a texture discontinuity which results
   * in artifacts).
   */
  static vtkIdType DuplicatePoint(
    vtkPolyData* data, vtkCell* cell, int cellPointIndex);


  /**
   * Compare two points for equality
   */
  static bool FuzzyEqual(double* f, double* s, double t);

private:
  /**
   * We store a list of pointIds (that have the same texture coordinates)
   * at the texture index returned by texLocator
   */
  std::vector<std::vector<vtkIdType>> PointIds;
  /**
   * Used to detect different texture values at a vertex.
   */
  vtkSmartPointer<vtkIncrementalOctreePointLocator> TexLocator;
};

#endif
// VTK-HeaderTest-Exclude: vtkCellTextureToPointTextureInternal.h
