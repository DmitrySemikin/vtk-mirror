/*===========================================================================*/
/* Distributed under OSI-approved BSD 3-Clause License.                      */
/* For copyright, see the following accompanying files or https://vtk.org:   */
/* - VTK-Copyright.txt                                                       */
/*===========================================================================*/
/**
 * @class   vtkUnsignedCharArray
 * @brief   dynamic, self-adjusting array of unsigned char
 *
 * vtkUnsignedCharArray is an array of values of type unsigned char.
 * It provides methods for insertion and retrieval of values and will
 * automatically resize itself to hold new data.
*/

#ifndef vtkUnsignedCharArray_h
#define vtkUnsignedCharArray_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkDataArray.h"
#include "vtkAOSDataArrayTemplate.h" // Real Superclass

// Fake the superclass for the wrappers.
#ifndef __VTK_WRAP__
#define vtkDataArray vtkAOSDataArrayTemplate<unsigned char>
#endif
class VTKCOMMONCORE_EXPORT vtkUnsignedCharArray : public vtkDataArray
{
public:
  vtkTypeMacro(vtkUnsignedCharArray, vtkDataArray)
#ifndef __VTK_WRAP__
#undef vtkDataArray
#endif
  static vtkUnsignedCharArray* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;

  // This macro expands to the set of method declarations that
  // make up the interface of vtkAOSDataArrayTemplate, which is ignored
  // by the wrappers.
#if defined(__VTK_WRAP__) || defined (__WRAP_GCCXML__)
  vtkCreateWrappedArrayInterface(unsigned char);
#endif

  /**
   * A faster alternative to SafeDownCast for downcasting vtkAbstractArrays.
   */
  static vtkUnsignedCharArray* FastDownCast(vtkAbstractArray *source)
  {
    return static_cast<vtkUnsignedCharArray*>(Superclass::FastDownCast(source));
  }

  /**
   * Get the minimum data value in its native type.
   */
  static unsigned char GetDataTypeValueMin() { return VTK_UNSIGNED_CHAR_MIN; }

  /**
   * Get the maximum data value in its native type.
   */
  static unsigned char GetDataTypeValueMax() { return VTK_UNSIGNED_CHAR_MAX; }

protected:
  vtkUnsignedCharArray();
  ~vtkUnsignedCharArray() override;

private:

  typedef vtkAOSDataArrayTemplate<unsigned char> RealSuperclass;

  vtkUnsignedCharArray(const vtkUnsignedCharArray&) = delete;
  void operator=(const vtkUnsignedCharArray&) = delete;
};

// Define vtkArrayDownCast implementation:
vtkArrayDownCast_FastCastMacro(vtkUnsignedCharArray)

#endif
