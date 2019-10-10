/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTransformPolyDataFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkCellTextureToPointTexture
 * @brief   Converts cell to point texture coordinates.
 *
 * Transforms a poly data with cell texture (each cell has texture
 * coordinates for each of its points) into a poly data with point
 * texture. It duplicates points where we have two or more different
 * texture coordinates. This works only if all cells in the input
 * polydata have the same number of points.
*/

#ifndef vtkCellTextureToPointTexture_h
#define vtkCellTextureToPointTexture_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

/**
 *
 */
class VTKFILTERSGENERAL_EXPORT vtkCellTextureToPointTexture : public vtkPolyDataAlgorithm
{
public:
  static vtkCellTextureToPointTexture *New();
  vtkTypeMacro(vtkCellTextureToPointTexture,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Tolerance used to detect different texture coordinates for shared
   * points for faces.
   */
  vtkGetMacro(FaceTextureTolerance, float);
  vtkSetMacro(FaceTextureTolerance, float);


protected:
  vtkCellTextureToPointTexture();
  ~vtkCellTextureToPointTexture() override;

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) override;

private:
  vtkCellTextureToPointTexture(const vtkCellTextureToPointTexture&) = delete;
  void operator=(const vtkCellTextureToPointTexture&) = delete;

private:
  float FaceTextureTolerance;
};

#endif
