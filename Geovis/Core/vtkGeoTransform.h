/*===========================================================================*/
/* Distributed under OSI-approved BSD 3-Clause License.                      */
/* For copyright, see the following accompanying files or https://vtk.org:   */
/* - VTK-Copyright.txt                                                       */
/* - Sandia-Copyright.txt                                                    */
/*===========================================================================*/
/**
 * @class   vtkGeoTransform
 * @brief   A transformation between two geographic coordinate systems
 *
 * This class takes two geographic projections and transforms point
 * coordinates between them.
*/

#ifndef vtkGeoTransform_h
#define vtkGeoTransform_h

#include "vtkGeovisCoreModule.h" // For export macro
#include "vtkAbstractTransform.h"

class vtkGeoProjection;

class VTKGEOVISCORE_EXPORT vtkGeoTransform : public vtkAbstractTransform
{
public:
  static vtkGeoTransform* New();
  void PrintSelf( ostream& os, vtkIndent indent ) override;
  vtkTypeMacro(vtkGeoTransform,vtkAbstractTransform);

  //@{
  /**
   * The source geographic projection.
   */
  void SetSourceProjection(vtkGeoProjection* source);
  vtkGetObjectMacro(SourceProjection,vtkGeoProjection);
  //@}

  //@{
  /**
   * The target geographic projection.
   */
  void SetDestinationProjection(vtkGeoProjection* dest);
  vtkGetObjectMacro(DestinationProjection,vtkGeoProjection);
  //@}

  /**
   * Transform many points at once.
   */
  void TransformPoints( vtkPoints* src, vtkPoints* dst ) override;

  /**
   * Invert the transformation.
   */
  void Inverse() override;

  //@{
  /**
   * This will calculate the transformation without calling Update.
   * Meant for use only within other VTK classes.
   */
  void InternalTransformPoint( const float in[3], float out[3] ) override;
  void InternalTransformPoint( const double in[3], double out[3] ) override;
  //@}

  //@{
  /**
   * This will transform a point and, at the same time, calculate a
   * 3x3 Jacobian matrix that provides the partial derivatives of the
   * transformation at that point.  This method does not call Update.
   * Meant for use only within other VTK classes.
   */
  void InternalTransformDerivative( const float in[3], float out[3], float derivative[3][3] ) override;
  void InternalTransformDerivative( const double in[3], double out[3], double derivative[3][3] ) override;
  //@}

  /**
   * Make another transform of the same type.
   */
  vtkAbstractTransform* MakeTransform() override;

protected:
  vtkGeoTransform();
  ~vtkGeoTransform() override;

  void InternalTransformPoints( double* ptsInOut, vtkIdType numPts, int stride );

  vtkGeoProjection* SourceProjection;
  vtkGeoProjection* DestinationProjection;

private:
  vtkGeoTransform( const vtkGeoTransform& ) = delete;
  void operator = ( const vtkGeoTransform& ) = delete;
};

#endif // vtkGeoTransform_h
