/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkClipPolyData.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkCylindricalGrid
 * @brief   a cylindrical dataset oriented along the z axis
 *
 * vtkCylindricalGrid is a data object representing a combination of cells which have a radial
 * curvature, up to and including a complete ring. Cells are represented by a cartesian radius,
 * polar theta rotation angle about the origin, and a cartesian z height.
 *
 * @sa
 * vtkUnstructuredGrid
 */

#ifndef vtkCylindricalGrid_h
#define vtkCylindricalGrid_h

#include <vtkUnstructuredGrid.h>

class VTKCOMMONDATAMODEL_EXPORT vtkCylindricalGrid : public vtkUnstructuredGrid
{
public:
  static vtkCylindricalGrid* New();
  vtkTypeMacro(vtkCylindricalGrid, vtkUnstructuredGrid);

  //@{
  /**
   * The maximum angle which can be rendered without inserting intermediate points.
   *
   * If a cell's polar angle becomes too large, intermediate points will be inserted into the inner
   * and outer curved surfaces of the cell. This preserves the visual rendering of the cell's
   * curvature.
   *
   * @warning
   * A polygonal representation of a cylindrical cell is built as soon as the cell is added, so this
   * value should be set beforehand.
   */
  void SetMaximumAngle(double maxAngle);
  double GetMaximumAngle();
  //@}

  //@{
  /**
   * Should any radial coordinates be represented in degrees (true) or radians (false)?
   * Default value is true(1).
   */
  vtkSetMacro(UseDegrees, vtkTypeBool);
  vtkGetMacro(UseDegrees, vtkTypeBool);
  vtkBooleanMacro(UseDegrees, vtkTypeBool);
  //@}

  //@{
  /**
   * Create a new cylindrically shaped polygonal cell.
   * Defined by an inner and outer cartesian radius, starting and ending polar rotation, and
   * cartesian z height. In 2D or 3D depending on how many z coordinates are supplied.
   */
  void InsertNextCylindricalCell(double r1, double r2, double p1, double p2, double z1);
  void InsertNextCylindricalCell(double r1, double r2, double p1, double p2, double z1, double z2);
  //@}

protected:
  vtkCylindricalGrid();

  double MaximumAngle;
  vtkTypeBool UseDegrees;

private:
  class Impl;
};

#endif // vtkCylindricalGrid_h
