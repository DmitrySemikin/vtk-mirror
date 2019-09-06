/*===========================================================================*/
/* Distributed under OSI-approved BSD 3-Clause License.                      */
/* For copyright, see the following accompanying files or https://vtk.org:   */
/* - VTK-Copyright.txt                                                       */
/*===========================================================================*/
/**
 * @class   vtkImageHSVToRGB
 * @brief   Converts HSV components to RGB.
 *
 * For each pixel with hue, saturation and value components this filter
 * outputs the color coded as red, green, blue.  Output type must be the same
 * as input type.
 *
 * @sa
 * vtkImageRGBToHSV
*/

#ifndef vtkImageHSVToRGB_h
#define vtkImageHSVToRGB_h


#include "vtkImagingColorModule.h" // For export macro
#include "vtkThreadedImageAlgorithm.h"

class VTKIMAGINGCOLOR_EXPORT vtkImageHSVToRGB : public vtkThreadedImageAlgorithm
{
public:
  static vtkImageHSVToRGB *New();
  vtkTypeMacro(vtkImageHSVToRGB,vtkThreadedImageAlgorithm);

  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Hue is an angle. Maximum specifies when it maps back to 0.
   * HueMaximum defaults to 255 instead of 2PI, because unsigned char
   * is expected as input.
   * Maximum also specifies the maximum of the Saturation, and R, G, B.
   */
  vtkSetMacro(Maximum,double);
  vtkGetMacro(Maximum,double);
  //@}

protected:
  vtkImageHSVToRGB();
  ~vtkImageHSVToRGB() override {}

  double Maximum;

  void ThreadedExecute (vtkImageData *inData, vtkImageData *outData,
                       int ext[6], int id) override;
private:
  vtkImageHSVToRGB(const vtkImageHSVToRGB&) = delete;
  void operator=(const vtkImageHSVToRGB&) = delete;
};

#endif



