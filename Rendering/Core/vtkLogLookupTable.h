/*===========================================================================*/
/* Distributed under OSI-approved BSD 3-Clause License.                      */
/* For copyright, see the following accompanying files or https://vtk.org:   */
/* - VTK-Copyright.txt                                                       */
/*===========================================================================*/
/**
 * @class   vtkLogLookupTable
 * @brief   map scalars into colors using log (base 10) scale
 *
 * This class is an empty shell.  Use vtkLookupTable with SetScaleToLog10()
 * instead.
 *
 * @sa
 * vtkLookupTable
*/

#ifndef vtkLogLookupTable_h
#define vtkLogLookupTable_h

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkLookupTable.h"

class VTKRENDERINGCORE_EXPORT vtkLogLookupTable : public vtkLookupTable
{
public:
  static vtkLogLookupTable *New();

  vtkTypeMacro(vtkLogLookupTable, vtkLookupTable);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkLogLookupTable(int sze = 256, int ext = 256);
  ~vtkLogLookupTable() override {}
private:
  vtkLogLookupTable(const vtkLogLookupTable&) = delete;
  void operator=(const vtkLogLookupTable&) = delete;
};

#endif
