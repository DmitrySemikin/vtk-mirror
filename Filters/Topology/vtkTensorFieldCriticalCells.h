/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyDataAlgorithm.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkTensorFieldCriticalCells_h
#define vtkTensorFieldCriticalCells_h

#include "vtkFiltersTopologyModule.h" // For export macro
#include "vtkImageAlgorithm.h"
#include <iterator>
#include <unordered_set>

class VTKFILTERSTOPOLOGY_EXPORT vtkTensorFieldCriticalCells : public vtkImageAlgorithm
{
public:
  static vtkTensorFieldCriticalCells* New();
  vtkTypeMacro(vtkTensorFieldCriticalCells, vtkImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Specify the eigenvector field name to be used in the filter.
   */
  void SetEigenvectorFieldArrayName(const char* fieldName);

protected:
  vtkTensorFieldCriticalCells();
  int FillInputPortInformation(int port, vtkInformation* info) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  // name of the input array names.
  const char* Field;

private:
  vtkTensorFieldCriticalCells(const vtkTensorFieldCriticalCells&) = delete;
  void operator=(const vtkTensorFieldCriticalCells&) = delete;
};

#endif // vtkTensorFieldCriticalCells_h
