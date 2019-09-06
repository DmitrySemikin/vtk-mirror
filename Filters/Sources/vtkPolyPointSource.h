/*===========================================================================*/
/* Distributed under OSI-approved BSD 3-Clause License.                      */
/* For copyright, see the following accompanying files or https://vtk.org:   */
/* - VTK-Copyright.txt                                                       */
/*===========================================================================*/
/**
 * @class   vtkPolyPointSource
 * @brief   create points from a list of input points
 *
 * vtkPolyPointSource is a source object that creates a vert from
 * user-specified points. The output is a vtkPolyData.
*/

#ifndef vtkPolyPointSource_h
#define vtkPolyPointSource_h

#include "vtkFiltersSourcesModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class vtkPoints;

class VTKFILTERSSOURCES_EXPORT vtkPolyPointSource : public vtkPolyDataAlgorithm
{
public:
  static vtkPolyPointSource* New();
  vtkTypeMacro(vtkPolyPointSource, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Set the number of points in the poly line.
   */
  void SetNumberOfPoints(vtkIdType numPoints);
  vtkIdType GetNumberOfPoints();
  //@}

  /**
   * Resize while preserving data.
   */
  void Resize(vtkIdType numPoints);

  /**
   * Set a point location.
   */
  void SetPoint(vtkIdType id, double x, double y, double z);

  //@{
  /**
   * Get the points.
   */
  void SetPoints(vtkPoints* points);
  vtkGetObjectMacro(Points, vtkPoints);
  //@}

  /**
   * Get the mtime plus consider its Points
   */
  vtkMTimeType GetMTime() override;

protected:
  vtkPolyPointSource();
  ~vtkPolyPointSource() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector *) override;

  vtkPoints* Points;

private:
  vtkPolyPointSource(const vtkPolyPointSource&) = delete;
  void operator=(const vtkPolyPointSource&) = delete;
};

#endif
