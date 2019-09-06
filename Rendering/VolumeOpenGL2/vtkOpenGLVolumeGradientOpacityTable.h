/*===========================================================================*/
/* Distributed under OSI-approved BSD 3-Clause License.                      */
/* For copyright, see the following accompanying files or https://vtk.org:   */
/* - VTK-Copyright.txt                                                       */
/*===========================================================================*/

#ifndef vtkOpenGLVolumeGradientOpacityTable_h
#define vtkOpenGLVolumeGradientOpacityTable_h
#ifndef __VTK_WRAP__

#include "vtkOpenGLVolumeLookupTable.h"

// Forward declarations
class vtkOpenGLRenderWindow;

//----------------------------------------------------------------------------
class vtkOpenGLVolumeGradientOpacityTable : public vtkOpenGLVolumeLookupTable
{
public:
  vtkTypeMacro(vtkOpenGLVolumeGradientOpacityTable, vtkOpenGLVolumeLookupTable);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkOpenGLVolumeGradientOpacityTable* New();

protected:
  vtkOpenGLVolumeGradientOpacityTable() = default;

  /**
   * Update the internal texture object using the gradient opacity transfer
   * function
   */
  void InternalUpdate(vtkObject* func,
                      int blendMode,
                      double sampleDistance,
                      double unitDistance,
                      int filterValue) override;

private:
  vtkOpenGLVolumeGradientOpacityTable(
    const vtkOpenGLVolumeGradientOpacityTable&) = delete;
  vtkOpenGLVolumeGradientOpacityTable& operator=(
    const vtkOpenGLVolumeGradientOpacityTable&) = delete;
};

#endif // __VTK_WRAP__
#endif // vtkOpenGLVolumeGradientOpacityTable_h
// VTK-HeaderTest-Exclude: vtkOpenGLVolumeGradientOpacityTable.h
