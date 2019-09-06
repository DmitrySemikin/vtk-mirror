/*===========================================================================*/
/* Distributed under OSI-approved BSD 3-Clause License.                      */
/* For copyright, see the following accompanying files or https://vtk.org:   */
/* - VTK-Copyright.txt                                                       */
/*===========================================================================*/
/**
 * @class   vtkImageEuclideanToPolar
 * @brief   Converts 2D Euclidean coordinates to polar.
 *
 * For each pixel with vector components x,y, this filter outputs
 * theta in component0, and radius in component1.
*/

#ifndef vtkImageEuclideanToPolar_h
#define vtkImageEuclideanToPolar_h


#include "vtkImagingGeneralModule.h" // For export macro
#include "vtkThreadedImageAlgorithm.h"

class VTKIMAGINGGENERAL_EXPORT vtkImageEuclideanToPolar : public vtkThreadedImageAlgorithm
{
public:
  static vtkImageEuclideanToPolar *New();
  vtkTypeMacro(vtkImageEuclideanToPolar,
                       vtkThreadedImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Theta is an angle. Maximum specifies when it maps back to 0.
   * ThetaMaximum defaults to 255 instead of 2PI, because unsigned char
   * is expected as input. The output type must be the same as input type.
   */
  vtkSetMacro(ThetaMaximum,double);
  vtkGetMacro(ThetaMaximum,double);
  //@}

protected:
  vtkImageEuclideanToPolar();
  ~vtkImageEuclideanToPolar() override {}

  double ThetaMaximum;

  void ThreadedExecute (vtkImageData *inData, vtkImageData *outData,
                       int ext[6], int id) override;
private:
  vtkImageEuclideanToPolar(const vtkImageEuclideanToPolar&) = delete;
  void operator=(const vtkImageEuclideanToPolar&) = delete;
};

#endif



