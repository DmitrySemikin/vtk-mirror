/*===========================================================================*/
/* Distributed under OSI-approved BSD 3-Clause License.                      */
/* For copyright, see the following accompanying files or https://vtk.org:   */
/* - VTK-Copyright.txt                                                       */
/*===========================================================================*/
#include "vtkParametricDini.h"
#include "vtkObjectFactory.h"
#include "vtkMath.h"

vtkStandardNewMacro(vtkParametricDini);

//----------------------------------------------------------------------------
vtkParametricDini::vtkParametricDini()
{
  // Preset triangulation parameters
  this->MinimumU = 0;
  this->MaximumU = 4 * vtkMath::Pi();
  this->MinimumV = 0.001;
  this->MaximumV = 2.0;

  this->JoinU = 0;
  this->JoinV = 0;
  this->TwistU = 0;
  this->TwistV = 0;
  this->ClockwiseOrdering = 0;
  this->DerivativesAvailable = 1;

  this->A = 1.0;
  this->B = 0.2;
}

//----------------------------------------------------------------------------
vtkParametricDini::~vtkParametricDini() = default;

//----------------------------------------------------------------------------
void vtkParametricDini::Evaluate(double uvw[3], double Pt[3],
                                 double Duvw[9])
{

  double u = uvw[0];
  double v = uvw[1];
  double *Du = Duvw;
  double *Dv = Duvw + 3;

  double cu = cos(u);
  double cv = cos(v);
  double su = sin(u);
  double sv = sin(v);

  // The point
  Pt[0] = this->A * cu * sv;
  Pt[1] = this->A * su * sv;
  Pt[2] = this->A * (cos(v) + log(tan((v / 2)))) + this->B * u;

  //The derivatives are:
  Du[0] = -Pt[1];
  Dv[0] = this->A * cu * cv;
  Du[1] = Pt[0];
  Dv[1] = this->A * su * cv;
  Du[2] = this->B;
  double tv2 = tan(0.5 * v);
  if (tv2 != 0)
  {
    Dv[2] = this->A * (-sv + (0.5 + 0.5 * tv2 * tv2) / tv2);
  }
  else // Avoid division by zero at the singularity.
  {
    Dv[2] = this->A * (-sv + 0.00005);
  }
}

//----------------------------------------------------------------------------
double vtkParametricDini::EvaluateScalar(double *, double *, double *)
{
  return 0;
}

//----------------------------------------------------------------------------
void vtkParametricDini::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "A: " << this->A << "\n";
  os << indent << "B: " << this->B << "\n";

}
