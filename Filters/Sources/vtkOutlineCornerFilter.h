/*===========================================================================*/
/* Distributed under OSI-approved BSD 3-Clause License.                      */
/* For copyright, see the following accompanying files or https://vtk.org:   */
/* - VTK-Copyright.txt                                                       */
/*===========================================================================*/
/**
 * @class   vtkOutlineCornerFilter
 * @brief   create wireframe outline corners for arbitrary data set
 *
 * vtkOutlineCornerFilter is a filter that generates wireframe outline corners of any
 * data set. The outline consists of the eight corners of the dataset
 * bounding box.
*/

#ifndef vtkOutlineCornerFilter_h
#define vtkOutlineCornerFilter_h

#include "vtkFiltersSourcesModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"
class vtkOutlineCornerSource;

class VTKFILTERSSOURCES_EXPORT vtkOutlineCornerFilter : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkOutlineCornerFilter,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Construct outline corner filter with default corner factor = 0.2
   */
  static vtkOutlineCornerFilter *New();

  //@{
  /**
   * Set/Get the factor that controls the relative size of the corners
   * to the length of the corresponding bounds
   */
  vtkSetClampMacro(CornerFactor, double, 0.001, 0.5);
  vtkGetMacro(CornerFactor, double);
  //@}

protected:
  vtkOutlineCornerFilter();
  ~vtkOutlineCornerFilter() override;

  vtkOutlineCornerSource *OutlineCornerSource;
  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) override;
  int FillInputPortInformation(int port, vtkInformation *info) override;

  double CornerFactor;
private:
  vtkOutlineCornerFilter(const vtkOutlineCornerFilter&) = delete;
  void operator=(const vtkOutlineCornerFilter&) = delete;
};

#endif
