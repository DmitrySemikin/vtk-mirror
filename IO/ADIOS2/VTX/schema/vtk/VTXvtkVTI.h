/*===========================================================================*/
/* Distributed under OSI-approved BSD 3-Clause License.                      */
/* For copyright, see the following accompanying files or https://vtk.org:   */
/* - VTK-Copyright.txt                                                       */
/*===========================================================================*/


#ifndef VTK_IO_ADIOS2_VTX_SCHEMA_VTK_VTXvtkVTI_h
#define VTK_IO_ADIOS2_VTX_SCHEMA_VTK_VTXvtkVTI_h

#include <map>
#include <string>
#include <vector>

#include "VTX/schema/vtk/VTXvtkBase.h"

#include "vtkImageData.h"
#include "vtkNew.h"

namespace vtx
{
namespace schema
{
class VTXvtkVTI : public VTXvtkBase
{
public:
  VTXvtkVTI(const std::string& schema, adios2::IO& io, adios2::Engine& engine);
  ~VTXvtkVTI();

private:
  /** Could be extended in a container, this is a per-rank ImageData */
  vtkNew<vtkImageData> ImageData;
  /** Store the Whole Extent in physical dimensions, row-major */
  adios2::Dims WholeExtent;

  adios2::Dims GetShape(const types::DataSetType type);
  adios2::Box<adios2::Dims> GetSelection(const types::DataSetType type);

  void DoFill(vtkMultiBlockDataSet* multiBlock, const size_t step) final;
  void ReadPiece(const size_t step, const size_t pieceID) final;

  void Init() final;

#define declare_type(T)                                                                            \
  void SetDimensions(                                                                              \
    adios2::Variable<T> variable, const types::DataArray& dataArray, const size_t step) final;
  VTK_IO_ADIOS2_VTX_ARRAY_TYPE(declare_type)
#undef declare_type

  template<class T>
  void SetDimensionsCommon(
    adios2::Variable<T> variable, const types::DataArray& dataArray, const size_t step);
};

} // end namespace schema
} // end namespace vtx

#endif /* VTK_IO_ADIOS2_VTX_SCHEMA_VTK_VTXvtkVTI_h */
