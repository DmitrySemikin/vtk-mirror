/*===========================================================================*/
/* Distributed under OSI-approved BSD 3-Clause License.                      */
/* For copyright, see the following accompanying files or https://vtk.org:   */
/* - VTK-Copyright.txt                                                       */
/*===========================================================================*/
/**
 * @class   vtkExplicitStructuredGridCrop
 * @brief   Filter which extracts a piece of explicit structured
 * grid changing its extents.
 */

#ifndef vtkExplicitStructuredGridCrop_h
#define vtkExplicitStructuredGridCrop_h

#include "vtkExplicitStructuredGridAlgorithm.h"
#include "vtkFiltersCoreModule.h" // For export macro

class VTKFILTERSCORE_EXPORT vtkExplicitStructuredGridCrop
  : public vtkExplicitStructuredGridAlgorithm
{
public:
  static vtkExplicitStructuredGridCrop* New();
  vtkTypeMacro(vtkExplicitStructuredGridCrop, vtkExplicitStructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * The whole extent of the output has to be set explicitly.
   */
  void SetOutputWholeExtent(int extent[6], vtkInformation* outInfo = 0);
  void SetOutputWholeExtent(int minX, int maxX, int minY, int maxY, int minZ, int maxZ);
  void GetOutputWholeExtent(int extent[6]);
  int* GetOutputWholeExtent() { return this->OutputWholeExtent; }
  //@}

  void ResetOutputWholeExtent();

protected:
  vtkExplicitStructuredGridCrop();
  ~vtkExplicitStructuredGridCrop() override = default;

  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestUpdateExtent(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  int Initialized;
  int OutputWholeExtent[6];

private:
  vtkExplicitStructuredGridCrop(const vtkExplicitStructuredGridCrop&) = delete;
  void operator=(const vtkExplicitStructuredGridCrop&) = delete;
};

#endif
