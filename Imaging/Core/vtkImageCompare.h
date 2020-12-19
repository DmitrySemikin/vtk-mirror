/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageCompare.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef VTKIMAGECOMPARE_H
#define VTKIMAGECOMPARE_H

#include "vtkImageAlgorithm.h"
#include "vtkImagingCoreModule.h" // For export macro

/**
 * @class   vtkImageCompare
 * @brief   Compares images for regression tests.
 *
 * vtkImageCompare takes two rgb unsigned char images and compares them.
 * It computes pixel-channel-wise difference between the images using a
 * selected metric, by default Mean Squared Error (MSE). It also generates
 * a difference image indicating which pixels are different between the two
 * images.
 *
 * The filter also has an option to apply a Gaussian smooth filter before
 * comparing the difference. Currently the Gaussian smooth kernel is fixed
 * and hard coded.
 *
 * This is a symmetric filter and the difference computed is symmetric.
 * The resulting value is the maximum error of the two directions
 * A->B and B->A
 */
class VTKIMAGINGCORE_EXPORT vtkImageCompare : public vtkImageAlgorithm
{
public:
  static vtkImageCompare* New();
  vtkTypeMacro(vtkImageCompare, vtkImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Specify the Image to compare the input to.
   */
  void SetImageConnection(vtkAlgorithmOutput* output)
  {
    this->SetInputConnection(1, output);
  }
  void SetImageData(vtkDataObject* image) { this->SetInputData(1, image); }
  vtkImageData* GetImage();
  //@}

  /**
   * Return the total error in comparing the two images.
   */
  double GetError() { return this->Error; }
  void GetError(double* e) { *e = this->GetError(); }

  /**
   * Return the total thresholded error in comparing the two images.
   * The thresholded error is the error minus the threshold and clamped at a
   * minimum of zero.
   */
  double GetThresholdedError() { return this->ThresholdedError; }
  void GetThresholdedError(double* e) { *e = this->GetThresholdedError(); }

  /**
   * This threshold allows difference tolerated at each channel of each pixel.
   * Note that when computing difference, color values are divided by 255 so the
   * range would be [0...1.]
   */
  vtkGetMacro(FuzzyThreshold, double);
  vtkSetClampMacro(FuzzyThreshold, double, 0., 1.);

  /**
   * Choose the error metric. The default is Mean Squared Error (MSE). Another
   * option is Peak Signal to Noise Ratio (PSNR). MSE should be fine with most
   * cases.
   */
  vtkGetMacro(ErrorMetric, int);
  vtkSetClampMacro(ErrorMetric, int, MeanSquaredErrorMetric,
    PeakSignalToNoiseRatioErrorMetric);

  /**
   * Sets the error threshold. This will affect the ThresholdedError and isEqual
   * flag.
   */
  vtkGetMacro(ErrorThreshold, double);
  vtkSetClampMacro(ErrorThreshold, double, 0., 1000.);

  /**
   * Apply a Gaussian smooth filter to both images before comparing. The
   * Gaussian kernel is currently hard coded.
   */
  vtkGetMacro(SmoothBeforeCompare, bool);
  vtkSetMacro(SmoothBeforeCompare, bool);

protected:
  vtkImageCompare();
  ~vtkImageCompare() override;

  enum MetricType
  {

    MeanSquaredErrorMetric,                // implemented
    PeakSignalToNoiseRatioErrorMetric,     // implemented
    AbsoluteErrorMetric,                   // not implemented
    FuzzErrorMetric,                       // not implemented
    MeanAbsoluteErrorMetric,               // not implemented
    MeanErrorPerPixelErrorMetric,          // not implemented
    NormalizedCrossCorrelationErrorMetric, // not implemented
    PeakAbsoluteErrorMetric,               // not implemented
    PerceptualHashErrorMetric,             // not implemented
    RootMeanSquaredErrorMetric,            // not implemented
    StructuralSimilarityErrorMetric,       // not implemented
    StructuralDissimilarityErrorMetric     // not implemented
  };

  // Parameters
  double ErrorThreshold;
  double FuzzyThreshold;
  int ErrorMetric;
  bool SmoothBeforeCompare;
  int KernelSize;
  double* Kernel;

  // Outputs
  double Error;
  double ThresholdedError;
  bool IsEqual;

  int RequestInformation(vtkInformation* request,
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

private:
  enum GaussianSmoothDirection
  {
    X = 0,
    Y = 1
  };
  enum GaussianSmoothChannel
  {
    R = 0,
    G = 1,
    B = 2
  };

  vtkImageCompare(const vtkImageCompare&) = delete;
  void operator=(const vtkImageCompare&) = delete;
  void ComputeError(unsigned char* in1Pixels, unsigned char* in2Pixels,
    int* extent, vtkIdType* in1Incs, vtkIdType* in2Incs);
  void GaussianSmooth(unsigned char* inPixels, int* extent, vtkIdType* inIncs);

  class GaussianSmooth1D1CFunctor;
  class ComputeDifferenceImageFunctor;
  class ComputeErrorFunctor;
  class ComputeMeanSquaredError;
  class ComputePeakSignalToNoiseRatioError;
};

#endif // VTKIMAGECOMPARE_H
