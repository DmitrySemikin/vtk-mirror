/*===========================================================================*/
/* Distributed under OSI-approved BSD 3-Clause License.                      */
/* For copyright, see the following accompanying files or https://vtk.org:   */
/* - VTK-Copyright.txt                                                       */
/*===========================================================================*/
#include "vtkParametricFigure8Klein.h"
#include "vtkObjectFactory.h"
#include "vtkMath.h"

vtkStandardNewMacro(vtkParametricFigure8Klein);

//----------------------------------------------------------------------------
vtkParametricFigure8Klein::vtkParametricFigure8Klein()
{
  // Preset triangulation parameters
  this->MinimumU = -vtkMath::Pi();
  this->MinimumV = -vtkMath::Pi();
  this->MaximumU = vtkMath::Pi();
  this->MaximumV =  vtkMath::Pi();

  this->JoinU = 1;
  this->JoinV = 1;
  this->TwistU = 1;
  this->TwistV = 0;
  this->ClockwiseOrdering = 0;
  this->DerivativesAvailable = 1;
  this->Radius = 1;
}

//----------------------------------------------------------------------------
vtkParametricFigure8Klein::~vtkParametricFigure8Klein() = default;

//----------------------------------------------------------------------------
void vtkParametricFigure8Klein::Evaluate(double uvw[3], double Pt[3],
    double Duvw[9])
{
  double u = uvw[0];
  double v = uvw[1];
  double *Du = Duvw;
  double *Dv = Duvw + 3;

  double cu = cos(u);
  double cu2 = cos(u / 2);
  double su = sin(u);
  double su2 = sin(u / 2);
  double cv = cos(v);
  double c2v = cos(2 * v);
  double s2v = sin(2 * v);
  double sv = sin(v);
  double t = this->Radius + sv * cu2 - s2v * su2 / 2;

  // The point
  Pt[0] = cu * t;
  Pt[1] = su * t;
  Pt[2] = su2 * sv + cu2 * s2v / 2;

  //The derivatives are:
  Du[0] = -Pt[1] - cu * (2 * sv * su2 + s2v * cu2) / 4;
  Du[1] =  Pt[0] - su * (2 * sv * su2 + s2v * cu2) / 4;
  Du[2] = cu2 * sv / 2 - su2 * s2v / 4;
  Dv[0] = cu * (cv * cu2 - c2v * su2);
  Dv[1] = su * (cv * cu2 - c2v * su2);
  Dv[2] = su2 * cv / 2 + cu2 * c2v;
}

//----------------------------------------------------------------------------
double vtkParametricFigure8Klein::EvaluateScalar(double*, double*,
    double*)
{
  return 0;
}

//----------------------------------------------------------------------------
void vtkParametricFigure8Klein::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Radius: " << this->Radius << "\n";
}

