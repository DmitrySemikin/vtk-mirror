/*===========================================================================*/
/* Distributed under OSI-approved BSD 3-Clause License.                      */
/* For copyright, see the following accompanying files or https://vtk.org:   */
/* - VTK-Copyright.txt                                                       */
/*===========================================================================*/
/**
 * @class   vtkGeneralizedKernel
 * @brief   flexible, general interpolation kernels
 *
 *
 * vtkGeneralizedKernel is an abstract class that defines an API for concrete
 * general-purpose, kernel subclasses. vtkGeneralizedKernels has important
 * properties that make them useful in a variety of interpolation
 * applications:
 * <pre>
 * 1. The weights are normalized.
 * 2. The footprint of the basis is configurable.
 * 3. Probabilistic weighting functions can be used to favor certain weights.
 * </pre>
 * The following paragraphs describe each of these properties in more detail.
 *
 * Normalized weightings simple mean Sum(w_i) = 1. This ensures that the
 * interpolation process is well behaved.
 *
 * The interpolation footprint is the set of points that are used to perform
 * the interpolation process. For example, it is possible to choose between a
 * radius-based kernel selection, and one based on the N nearest
 * neighbors. Note that the performance and mathematical properties of
 * kernels may vary greatly depending on which kernel style is selected. For
 * example, if a radius-based kernel footprint is used, and the radius is too
 * big, the algorithm can perform in n^3 fashion.
 *
 * Finally, in advanced usage, probability functions can be applied to the
 * interpolation weights (prior to normalization). These probability
 * functions are confidence estimates that the data at a particular point is
 * accurate. A typical application is when laser scans are used to acquire
 * point measurements, which return normals that indicate glancing returns
 * versus direct, near orthogonal hits. Another use is when point clouds are
 * combined, where some clouds are acquired with more accurate, detailed
 * devices versus a broad, potentially coarser acquisition process.
 *
 * @warning
 * Some kernels, like the Voronoi kernel, cannot be subclasses of this class
 * because their definition inherently defines the basis style. For example,
 * the Voronoi kernel is simply the single closest point. SPH kernels are
 * similar, because they implicitly depend on a particle distribution
 * consistent with simulation constraints such as conservation of mass, etc.
 *
 * @sa
 * vtkPointInterpolator vtkPointInterpolator2D vtkGaussianKernel vtkSPHKernel
 * vtkShepardKernel vtkLinearKernel vtkVoronoiKernel
*/

#ifndef vtkGeneralizedKernel_h
#define vtkGeneralizedKernel_h

#include "vtkFiltersPointsModule.h" // For export macro
#include "vtkInterpolationKernel.h"


class VTKFILTERSPOINTS_EXPORT vtkGeneralizedKernel : public vtkInterpolationKernel
{
public:
  //@{
  /**
   * Standard methods for type and printing.
   */
  vtkTypeMacro(vtkGeneralizedKernel, vtkInterpolationKernel)
  void PrintSelf(ostream& os, vtkIndent indent) override;
  //@}

  /**
   * Based on the kernel style, invoke the appropriate locator method to
   * obtain the points making up the basis. Given a point x (and optional
   * associated point id), determine the points around x which form an
   * interpolation basis. The user must provide the vtkIdList pIds, which
   * will be dynamically resized as necessary. The method returns the number
   * of points in the basis. Typically this method is called before
   * ComputeWeights(). Note that ptId is optional in most cases, although in
   * some kernels it is used to facilitate basis computation.
   */
  vtkIdType ComputeBasis(double x[3], vtkIdList *pIds, vtkIdType ptId=0) override;

  /**
   * Given a point x, a list of basis points pIds, and a probability
   * weighting function prob, compute interpolation weights associated with
   * these basis points.  Note that basis points list pIds, the probability
   * weighting prob, and the weights array are provided by the caller of the
   * method, and may be dynamically resized as necessary. The method returns
   * the number of weights (pIds may be resized in some cases). Typically
   * this method is called after ComputeBasis(), although advanced users can
   * invoke ComputeWeights() and provide the interpolation basis points pIds
   * directly. The probably weighting prob are numbers 0<=prob<=1 which are
   * multiplied against the interpolation weights before normalization. They
   * are estimates of local confidence of weights. The prob may be nullptr in
   * which all probabilities are considered =1.
   */
  virtual vtkIdType ComputeWeights(double x[3], vtkIdList *pIds,
                                   vtkDoubleArray *prob, vtkDoubleArray *weights) = 0;

  /**
   * Given a point x, and a list of basis points pIds, compute interpolation
   * weights associated with these basis points.  Note that both the nearby
   * basis points list pIds and the weights array are provided by the caller
   * of the method, and may be dynamically resized as necessary. Typically
   * this method is called after ComputeBasis(), although advanced users can
   * invoke ComputeWeights() and provide the interpolation basis points pIds
   * directly.
   */
  vtkIdType ComputeWeights(double x[3], vtkIdList *pIds, vtkDoubleArray *weights) override
  {
    return this->ComputeWeights(x,pIds,nullptr,weights);
  }

  /**
   * Enum used to select the interpolation basis form. By default, a Radius
   * form is used (i.e., the basis is defined from all points within a
   * specified radius). However, it is also possible to select the N closest
   * points (NClosest).
   */
  enum KernelStyle
  {
    RADIUS=0,
    N_CLOSEST=1
  };

  //@{
  /**
   * Specify the interpolation basis style. By default, a Radius style is
   * used (i.e., the basis is defined from all points within a specified
   * radius). However, it is also possible to select the N closest points
   * (NClosest). Note that in most formulations the Radius style is assumed
   * as it provides better mathematical properties. However, for convenience
   * some bases are easier to use when the N closest points are taken.
   */
  vtkSetMacro(KernelFootprint,int);
  vtkGetMacro(KernelFootprint,int);
  void SetKernelFootprintToRadius()
    { this->SetKernelFootprint(RADIUS); }
  void SetKernelFootprintToNClosest()
    { this->SetKernelFootprint(N_CLOSEST); }
  //@}

  //@{
  /**
   * If the interpolation basis style is Radius, then this method specifies
   * the radius within which the basis points must lie.
   */
  vtkSetClampMacro(Radius,double,0.0,VTK_FLOAT_MAX);
  vtkGetMacro(Radius,double);
  //@}

  //@{
  /**
   * If the interpolation basis style is NClosest, then this method specifies
   * the number of the closest points used to form the interpolation basis.
   */
  vtkSetClampMacro(NumberOfPoints,int,1,VTK_INT_MAX);
  vtkGetMacro(NumberOfPoints,int);
  //@}

  //@{
  /**
   * Indicate whether the interpolation weights should be normalized after they
   * are computed. Generally this is left on as it results in more reasonable
   * behavior.
   */
  vtkSetMacro(NormalizeWeights,bool);
  vtkGetMacro(NormalizeWeights,bool);
  vtkBooleanMacro(NormalizeWeights,bool);
  //@}

protected:
  vtkGeneralizedKernel();
  ~vtkGeneralizedKernel() override;

  int KernelFootprint;
  double Radius;
  int NumberOfPoints;
  bool NormalizeWeights;

private:
  vtkGeneralizedKernel(const vtkGeneralizedKernel&) = delete;
  void operator=(const vtkGeneralizedKernel&) = delete;
};

#endif
