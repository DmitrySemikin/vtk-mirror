/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCylindricalGridBuilder.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkCylindricalGridBuilder
 * @brief   a utility for building cylindrical cells around the z axis
 *
 * vtkCylindricalGridBuilder is a utility class for building cylindrical cells for an unstructured
 * grid. These cells have a radial curvature, up to and including a complete ring about the z axis.
 * Cells are represented by a cartesian radius, polar theta rotation angle about the origin, and a
 * cartesian z height.
 *
 * @sa
 * vtkUnstructuredGrid
 */

#ifndef vtkCylindricalGridBuilder_h
#define vtkCylindricalGridBuilder_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkObject.h"
#include "vtkSmartPointer.h"
#include "vtkUnstructuredGrid.h"

class vtkUnstructuredGrid;

class VTKCOMMONDATAMODEL_EXPORT vtkCylindricalGridBuilder : public vtkObject
{
public:
  static vtkCylindricalGridBuilder* New();
  vtkTypeMacro(vtkCylindricalGridBuilder, vtkObject);

  //@{
  /**
   * The grid to which the cells should be added.
   *
   * A default grid will be created which cells can be added to. If the grid the cells should be
   * added to needs to be customized outside this class, a different grid can always be supplied.
   */
  void SetGrid(vtkSmartPointer<vtkUnstructuredGrid> grid) { this->Grid = grid; }
  vtkSmartPointer<vtkUnstructuredGrid> GetGrid() { return this->Grid; }
  //@}

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
  vtkSetMacro(UseDegrees, bool);
  vtkGetMacro(UseDegrees, bool);
  vtkBooleanMacro(UseDegrees, bool);
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
  vtkCylindricalGridBuilder();

  vtkSmartPointer<vtkUnstructuredGrid> Grid;

  double MaximumAngle;
  bool UseDegrees;

private:
  class Impl;
};

#endif // vtkCylindricalGridBuilder_h
