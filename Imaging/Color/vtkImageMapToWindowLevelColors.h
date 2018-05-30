/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageMapToWindowLevelColors.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkImageMapToWindowLevelColors
 * @brief   Map an image through a lookup table and/or a window/level.
 *
 * The vtkImageMapToWindowLevelColors filter can be used to perform
 * the following operations depending on its settings:
 * -# If no lookup table is provided, and if the input data has a single
 *    component (any numerical scalar type is allowed), then the data is
 *    mapped through the specified Window/Level.  The type of the output
 *    scalars will be "unsigned char" with a range of (0,255).
 * -# If no lookup table is provided, and if the input data is already
 *    unsigned char, and if the Window/Level is set to 255.0/127.5, then
 *    the input data will be passed directly to the output.
 * -# If a lookup table is provided, then the first component of the
 *    input data is mapped through the lookup table using the Range of
 *    the lookup table.  The Window and Level are ignored in this case.
 * @sa
 * vtkLookupTable vtkScalarsToColors
*/

#ifndef vtkImageMapToWindowLevelColors_h
#define vtkImageMapToWindowLevelColors_h


#include "vtkImagingColorModule.h" // For export macro
#include "vtkImageMapToColors.h"

class VTKIMAGINGCOLOR_EXPORT vtkImageMapToWindowLevelColors : public vtkImageMapToColors
{
public:
  static vtkImageMapToWindowLevelColors *New();
  vtkTypeMacro(vtkImageMapToWindowLevelColors,vtkImageMapToColors);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Set / Get the Window to use. The output will be a gray value based
   * based on (S - (L - W/2))/W where S is the scalar value, L is the
   * level and W is the window.
   */
  vtkSetMacro( Window, double );
  vtkGetMacro( Window, double );
  //@}

  //@{
  /**
   * Set / Get the Level to use. The output will be a gray value based
   * based on (S - (L - W/2))/W where S is the scalar value, L is the
   * level and W is the window.
   */
  vtkSetMacro( Level, double );
  vtkGetMacro( Level, double );
  //@}

protected:
  vtkImageMapToWindowLevelColors();
  ~vtkImageMapToWindowLevelColors() override;

  int RequestInformation (vtkInformation *, vtkInformationVector **, vtkInformationVector *) override;
  void ThreadedRequestData(vtkInformation *request,
                           vtkInformationVector **inputVector,
                           vtkInformationVector *outputVector,
                           vtkImageData ***inData, vtkImageData **outData,
                           int extent[6], int id) override;
  int RequestData(vtkInformation *request,
                          vtkInformationVector **inputVector,
                          vtkInformationVector *outputVector) override;

  double Window;
  double Level;

private:
  vtkImageMapToWindowLevelColors(const vtkImageMapToWindowLevelColors&) = delete;
  void operator=(const vtkImageMapToWindowLevelColors&) = delete;
};

#endif







