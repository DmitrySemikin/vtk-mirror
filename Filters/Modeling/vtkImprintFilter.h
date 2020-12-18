/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImprintFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkImprintFilter
 * @brief   Imprint the contact surface of one object onto another surface
 *
 * This filter imprints the contact surface of one vtkPolyData mesh onto
 * a second, input vtkPolyData mesh. There are two inputs to the filter:
 * the target, which is the surface to be imprinted, and the imprint, which
 * is the object imprinting the target.
 */

#ifndef vtkImprintFilter_h
#define vtkImprintFilter_h

#include "vtkFiltersModelingModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class vtkStaticCellLocator;
class vtkStaticPointLocator;

class VTKFILTERSMODELING_EXPORT vtkImprintFilter : public vtkPolyDataAlgorithm
{
public:
  //@{
  /**
   * Standard methods to instantiate, print and provide type information.
   */
  static vtkImprintFilter* New();
  vtkTypeMacro(vtkImprintFilter, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  //@}

  /**
   * Specify the first vtkPolyData input connection which defines the
   * surface mesh to imprint (i.e., the target).
   */
  void SetTargetConnection(vtkAlgorithmOutput* algOutput);
  vtkAlgorithmOutput* GetTargetConnection();

  //@{
  /**
   * Specify the first vtkPolyData input which defines the surface mesh to
   * imprint (i.e., the taregt). The imprint surface is provided by the
   * second input.
   */
  void SetTargetData(vtkDataObject* target);
  vtkDataObject* GetTarget();
  //@}

  /**
   * Specify the a second vtkPolyData input connection which defines the
   * surface mesh with which to imprint the target (the target is provided by
   * the first input).
   */
  void SetImprintConnection(vtkAlgorithmOutput* algOutput);
  vtkAlgorithmOutput* GetImprintConnection();

  //@{
  /**
   * Specify the a second vtkPolyData input which defines the surface mesh
   * with which to imprint the target (i.e., the first input).
   */
  void SetImprintData(vtkDataObject* imprint);
  vtkDataObject* GetImprint();
  //@}

  //@{
  /**
   * Specify a tolerance which controls how close the imprint surface must be
   * to the target to successfully imprint the surface.
   */
  vtkSetClampMacro(Tolerance,double,0.0,VTK_FLOAT_MAX);
  vtkGetMacro(Tolerance,double);
  //@}

protected:
  vtkImprintFilter();
  ~vtkImprintFilter() override;

  double Tolerance;

  // Used internally to project points from imprint onto target
  vtkSmartPointer<vtkStaticCellLocator> CellLocator;
  vtkSmartPointer<vtkStaticPointLocator> PointLocator;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestUpdateExtent(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int, vtkInformation*) override;

private:
  vtkImprintFilter(const vtkImprintFilter&) = delete;
  void operator=(const vtkImprintFilter&) = delete;
};

#endif
