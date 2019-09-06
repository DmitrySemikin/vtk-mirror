/*===========================================================================*/
/* Distributed under OSI-approved BSD 3-Clause License.                      */
/* For copyright, see the following accompanying files or https://vtk.org:   */
/* - VTK-Copyright.txt                                                       */
/*===========================================================================*/
/**
 * @brief  Type traits for adios2 types(Native types) to vtk types
 *
*/
#ifndef vtkADIOS2CoreTypeTraits_h
#define vtkADIOS2CoreTypeTraits_h

#include "vtkType.h"

#include "vtkIOADIOS2Module.h" // For export macro

template<typename T>
struct NativeToVTKType
{
  static constexpr int VTKType = 0;
};

template<>
struct NativeToVTKType<char>
{
  static constexpr int VTKType = VTK_CHAR;
};

template<>
struct NativeToVTKType<float>
{
  static constexpr int VTKType = VTK_FLOAT;
};

template<>
struct NativeToVTKType<double>
{
  static constexpr int VTKType = VTK_DOUBLE;
};

template<>
struct NativeToVTKType<int8_t>
{
  static constexpr int VTKType = VTK_TYPE_INT8;
};

template<>
struct NativeToVTKType<uint8_t>
{
  static constexpr int VTKType = VTK_TYPE_UINT8;
};

template<>
struct NativeToVTKType<int16_t>
{
  static constexpr int VTKType = VTK_TYPE_INT16;
};

template<>
struct NativeToVTKType<uint16_t>
{
  static constexpr int VTKType = VTK_TYPE_UINT16;
};

template<>
struct NativeToVTKType<int32_t>
{
  static constexpr int VTKType = VTK_TYPE_INT32;
};

template<>
struct NativeToVTKType<uint32_t>
{
  static constexpr int VTKType = VTK_TYPE_UINT32;
};

template<>
struct NativeToVTKType<int64_t>
{
  static constexpr int VTKType = VTK_TYPE_INT64;
};

template<>
struct NativeToVTKType<uint64_t>
{
  static constexpr int VTKType = VTK_TYPE_UINT64;
};

#endif
// VTK-HeaderTest-Exclude: vtkADIOS2CoreTypeTraits.h
