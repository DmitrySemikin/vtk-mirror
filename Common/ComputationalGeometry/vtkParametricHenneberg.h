/*===========================================================================*/
/* Distributed under OSI-approved BSD 3-Clause License.                      */
/* For copyright, see the following accompanying files or https://vtk.org:   */
/* - VTK-Copyright.txt                                                       */
/*===========================================================================*/
/**
 * @class   vtkParametricHenneberg
 * @brief   Generate Henneberg's minimal surface.
 *
 * vtkParametricHenneberg generates Henneberg's minimal surface parametrically.
 * Henneberg's minimal surface is discussed further at
 * <a href="http://mathworld.wolfram.com/HennebergsMinimalSurface.html">Math World</a>.
 * @par Thanks:
 * Tim Meehan
*/

#ifndef vtkParametricHenneberg_h
#define vtkParametricHenneberg_h

#include "vtkCommonComputationalGeometryModule.h" // For export macro
#include "vtkParametricFunction.h"

class VTKCOMMONCOMPUTATIONALGEOMETRY_EXPORT vtkParametricHenneberg :
  public vtkParametricFunction
{
  public:

    vtkTypeMacro(vtkParametricHenneberg, vtkParametricFunction);
    void PrintSelf(ostream& os, vtkIndent indent) override;

    /**
     * Construct Henneberg's minimal surface with the following parameters:
     * (MinimumU, MaximumU) = (-1., 1.),
     * (MinimumV, MaximumV) = (-pi/.2, pi/2.),
     * JoinU = 0, JoinV = 0,
     * TwistU = 0, TwistV = 0;
     * ClockwiseOrdering = 0,
     * DerivativesAvailable = 1,
     */
    static vtkParametricHenneberg *New();

    /**
     * Return the parametric dimension of the class.
     */
    int GetDimension() override {return 2;}

    /**
     * Henneberg's minimal surface.

     * This function performs the mapping \f$f(u,v) \rightarrow (x,y,x)\f$, returning it
     * as Pt. It also returns the partial derivatives Du and Dv.
     * \f$Pt = (x, y, z), D_u\vec{f} = (dx/du, dy/du, dz/du), D_v\vec{f} = (dx/dv, dy/dv, dz/dv)\f$ .
     * Then the normal is \f$N = D_u\vec{f} \times D_v\vec{f}\f$ .
     */
    void Evaluate(double uvw[3], double Pt[3], double Duvw[9]) override;

    /**
     * Calculate a user defined scalar using one or all of uvw, Pt, Duvw.
     * This method simply returns 0.
     */
    double EvaluateScalar(double uvw[3], double Pt[3],
                          double Duvw[9]) override;

  protected:
    vtkParametricHenneberg();
    ~vtkParametricHenneberg() override;

  private:
    vtkParametricHenneberg(const vtkParametricHenneberg&) = delete;
    void operator=(const vtkParametricHenneberg&) = delete;
};

#endif
