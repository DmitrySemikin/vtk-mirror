/*===========================================================================*/
/* Distributed under OSI-approved BSD 3-Clause License.                      */
/* For copyright, see the following accompanying files or https://vtk.org:   */
/* - VTK-Copyright.txt                                                       */
/*===========================================================================*/
/**
 * @class   vtkImageNoiseSource
 * @brief   Create an image filled with noise.
 *
 * vtkImageNoiseSource just produces images filled with noise.  The only
 * option now is uniform noise specified by a min and a max.  There is one
 * major problem with this source. Every time it executes, it will output
 * different pixel values.  This has important implications when a stream
 * requests overlapping regions.  The same pixels will have different values
 * on different updates.
*/

#ifndef vtkImageNoiseSource_h
#define vtkImageNoiseSource_h


#include "vtkImagingSourcesModule.h" // For export macro
#include "vtkImageAlgorithm.h"


class VTKIMAGINGSOURCES_EXPORT vtkImageNoiseSource : public vtkImageAlgorithm
{
public:
  static vtkImageNoiseSource *New();
  vtkTypeMacro(vtkImageNoiseSource,vtkImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Set/Get the minimum and maximum values for the generated noise.
   */
  vtkSetMacro(Minimum, double);
  vtkGetMacro(Minimum, double);
  vtkSetMacro(Maximum, double);
  vtkGetMacro(Maximum, double);
  //@}

  //@{
  /**
   * Set how large of an image to generate.
   */
  void SetWholeExtent(int xMinx, int xMax, int yMin, int yMax,
                      int zMin, int zMax);
  void SetWholeExtent(const int ext[6])
  {
    this->SetWholeExtent(ext[0], ext[1], ext[2], ext[3], ext[4], ext[5]);
  }
  //@}

protected:
  vtkImageNoiseSource();
  ~vtkImageNoiseSource() override {}

  double Minimum;
  double Maximum;
  int WholeExtent[6];

  int RequestInformation (vtkInformation *, vtkInformationVector**, vtkInformationVector *) override;
  void ExecuteDataWithInformation(vtkDataObject *data, vtkInformation* outInfo) override;
private:
  vtkImageNoiseSource(const vtkImageNoiseSource&) = delete;
  void operator=(const vtkImageNoiseSource&) = delete;
};


#endif


