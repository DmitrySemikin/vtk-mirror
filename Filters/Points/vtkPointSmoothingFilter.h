/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPointSmoothingFilter.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See LICENSE file for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPointSmoothingFilter
 * @brief   adjust point positions to form a pleasing, packed arrangement
 *
 *
 * vtkPointSmoothingFilter modifies the coordinates of the input points of a
 * vtkPointSet by adjusting their position to create a smooth distribution
 * (and thereby form a pleasing packing of the points). Smoothing in its
 * simplest form is simply a variant of Laplacian smoothing (i.e., smoothing
 * based on nearby point neighbors). However the smoothing can be further
 * controlled either by a scalar field, by a tensor field, or a frame field
 * (the user can specify the nature of the smoothing operation). If
 * controlled by a scalar field, then each input point is assumed to be
 * surrounded by a isotropic sphere scaled by the scalar field; if controlled
 * by a tensor field, then each input point is assumed to be surrounded by an
 * anisotropic, oriented ellipsoid aligned to the the tensor eigenvectors and
 * scaled by the determinate of the tensor. A frame field also assumes a
 * surrounding, ellipsoidal shape except that the inversion of the ellipsoid
 * tensor is already performed. If no scalar, tensor, or frame field, the
 * smoothing is simply akin to Laplacian smoothing (see
 * vtkSmoothPolyDataFilter). Typical usage of this filter is to perform the
 * smoothing (or packing) operation (i.e., first execute this filter) and
 * then use a glyph filter (e.g., vtkTensorGlyph or vtkGlyph3D) to visualize
 * the packed points.
 *
 * Any vtkPointSet type can be provided as input, and the output will contain
 * the same number of new points each of which is adjusted to a new position.
 *
 * Note that the algorithm requires the use of a spatial point locator. The
 * point locator is used to build a local neighborhood of the points
 * surrounding each point. It is also used to perform interpolation as the
 * point positions are adjusted.
 *
 * @warning
 * This class has been loosely inspired by the paper by Kindlmann and Westin
 * "Diffusion Tensor Visualization with Glyph Packing". However, several
 * computational shortcuts, and generalizations have been used for performance
 * and utility reasons.
 *
 * @warning
 * This class has been threaded with vtkSMPTools. Using TBB or other
 * non-sequential type (set in the CMake variable
 * VTK_SMP_IMPLEMENTATION_TYPE) may improve performance significantly.
 *
 * @sa
 * vtkTensorWidget vtkTensorGlyph vtkSmoothPolyDataFilter
 */

#ifndef vtkPointSmoothingFilter_h
#define vtkPointSmoothingFilter_h

#include "vtkFiltersPointsModule.h" // For export macro
#include "vtkPointSetAlgorithm.h"

class vtkAbstractPointLocator;
class vtkDataArray;


class VTKFILTERSPOINTS_EXPORT vtkPointSmoothingFilter : public vtkPointSetAlgorithm
{
public:
  //@{
  /**
   * Standard methods for instantiation, obtaining type information, and
   * printing information.
   */
  static vtkPointSmoothingFilter* New();
  vtkTypeMacro(vtkPointSmoothingFilter, vtkPointSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  //@}

  /**
   * Specify how smoothing is to be controlled.
   */
  enum
  {
    DEFAULT_SMOOTHING,
    GEOMETRIC_SMOOTHING,
    SCALAR_SMOOTHING,
    TENSOR_SMOOTHING,
    FRAME_FIELD_SMOOTHING
  };

  //@{
  /**
   * Control how smoothing is to be performed. By default, if a point frame
   * field is available then frame field smoothing will be performed; then if
   * point tensors are available then anisotropic tensor smoothing will be
   * used; the next choice is to use isotropic scalar smoothing; and finally
   * if no frame field, tensors, or scalars are available, geometric
   * smoothing (i.e, Laplacian smoothing) will be used. If both scalars,
   * tensors, and /or a frame field are present, the user can specifiy which
   * to use, or to use geometric smoothing.
   */
  vtkSetClampMacro(SmoothingMode, int, DEFAULT_SMOOTHING, FRAME_FIELD_SMOOTHING);
  vtkGetMacro(SmoothingMode, int);
  void SetSmoothingModeToDefault() { this->SetSmoothingMode(DEFAULT_SMOOTHING); }
  void SetSmoothingModeToGeometric() { this->SetSmoothingMode(GEOMETRIC_SMOOTHING); }
  void SetSmoothingModeToScalars() { this->SetSmoothingMode(SCALAR_SMOOTHING); }
  void SetSmoothingModeToTensors() { this->SetSmoothingMode(TENSOR_SMOOTHING); }
  void SetSmoothingModeToFrameField() { this->SetSmoothingMode(FRAME_FIELD_SMOOTHING); }
  //@}

  //@{
  /**
   * Specify the name of the frame field to use for smoothing. This
   * information is only necessary if a frame field smoothing is enabled.
   */
  virtual void SetFrameFieldArray(vtkDataArray*);
  vtkGetObjectMacro(FrameFieldArray, vtkDataArray);
  //@}

  //@{
  /**
   * Specify a convergence criterion for the iteration
   * process. Smaller numbers result in more smoothing iterations.
   */
  vtkSetClampMacro(Convergence, double, 0.0, 1.0);
  vtkGetMacro(Convergence, double);
  //@}

  //@{
  /**
   * Specify the number of smoothing iterations,
   */
  vtkSetClampMacro(NumberOfIterations, int, 0, VTK_INT_MAX);
  vtkGetMacro(NumberOfIterations, int);
  //@}

  //@{
  /**
   * Specify the relaxation factor for smoothing iterations. As in all
   * iterative methods, the stability of the process is sensitive to
   * this parameter. In general, small relaxation factors and large
   * numbers of iterations are more stable than larger relaxation
   * factors and smaller numbers of iterations.
   */
  vtkSetMacro(RelaxationFactor, double);
  vtkGetMacro(RelaxationFactor, double);
  //@}

  //@{
  /**
   * Specify a point locator. By default a vtkStaticPointLocator is
   * used. The locator performs efficient searches to locate points
   * around a sample point.
   */
  void SetLocator(vtkAbstractPointLocator* locator);
  vtkGetObjectMacro(Locator, vtkAbstractPointLocator);
  //@}

protected:
  vtkPointSmoothingFilter();
  ~vtkPointSmoothingFilter() override;

  // Control the smoothing
  int SmoothingMode;
  double Convergence;
  int NumberOfIterations;
  double RelaxationFactor;
  vtkDataArray* FrameFieldArray;

  // Support the algorithm
  vtkAbstractPointLocator* Locator;

  // Pipeline support
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;

private:
  vtkPointSmoothingFilter(const vtkPointSmoothingFilter&) = delete;
  void operator=(const vtkPointSmoothingFilter&) = delete;
};

#endif
