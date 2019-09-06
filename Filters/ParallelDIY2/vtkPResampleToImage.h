/*===========================================================================*/
/* Distributed under OSI-approved BSD 3-Clause License.                      */
/* For copyright, see the following accompanying files or https://vtk.org:   */
/* - VTK-Copyright.txt                                                       */
/*===========================================================================*/
/**
 * @class   vtkPResampleToImage
 * @brief   sample dataset on a uniform grid in parallel
 *
 * vtkPResampleToImage is a parallel filter that resamples the input dataset on
 * a uniform grid. It internally uses vtkProbeFilter to do the probing.
 * @sa
 * vtkResampleToImage vtkProbeFilter
*/

#ifndef vtkPResampleToImage_h
#define vtkPResampleToImage_h

#include "vtkFiltersParallelDIY2Module.h" // For export macro
#include "vtkResampleToImage.h"

class vtkDataSet;
class vtkImageData;
class vtkMultiProcessController;

class VTKFILTERSPARALLELDIY2_EXPORT vtkPResampleToImage : public vtkResampleToImage
{
public:
  vtkTypeMacro(vtkPResampleToImage, vtkResampleToImage);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkPResampleToImage *New();

  //@{
  /**
   * By default this filter uses the global controller,
   * but this method can be used to set another instead.
   */
  virtual void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  //@}

protected:
  vtkPResampleToImage();
  ~vtkPResampleToImage();

  virtual int RequestData(vtkInformation *, vtkInformationVector **,
                          vtkInformationVector *) override;

  vtkMultiProcessController *Controller;

private:
  vtkPResampleToImage(const vtkPResampleToImage&) = delete;
  void operator=(const vtkPResampleToImage&) = delete;
};

#endif
