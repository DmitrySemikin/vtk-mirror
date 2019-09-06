/*===========================================================================*/
/* Distributed under OSI-approved BSD 3-Clause License.                      */
/* For copyright, see the following accompanying files or https://vtk.org:   */
/* - VTK-Copyright.txt                                                       */
/*===========================================================================*/

#ifndef vtkRangeIterableTraits_h
#define vtkRangeIterableTraits_h

class vtkCollection;
class vtkCompositeDataSet;
class vtkDataObjectTree;

namespace vtk
{
namespace detail
{

template <typename CollectionType> struct CollectionRange;

struct CompositeDataSetRange;
struct DataObjectTreeRange;

//------------------------------------------------------------------------------
// DeduceRangeType:
// These function signatures define a mapping from an Iterable (e.g. vtkObject)
// to a RangeType (e.g. the for-range iterable object).
// They are not implemented, as only the signatures are important. Classes used
// should only be forward declared in this header.
// Since classes are only forward declared, the argument type should be const&
// qualified, and the return type should be a reference.

// vtkCollection subclasses --> CollectionRange
template <typename CollectionType,
          typename = typename std::enable_if<
            std::is_base_of<vtkCollection, CollectionType>::value
            >::type>
CollectionRange<CollectionType>& DeduceRangeType(const CollectionType&);

// vtkCompositeDataSet --> CompositeDataSetRange
CompositeDataSetRange& DeduceRangeType(const vtkCompositeDataSet&);

// vtkDataObjectTree --> DataObjectTreeRange
DataObjectTreeRange& DeduceRangeType(const vtkDataObjectTree&);

// Traits class that defines a RangeType corresponding to the iterable range
// type most appropriate for Iterable.
template <typename Iterable>
struct IterableTraits
{
private:
  using RangeTypeInternal =
  decltype(vtk::detail::DeduceRangeType(std::declval<Iterable>()));
public:
  using RangeType = typename std::decay<RangeTypeInternal>::type;
};

}
}

#endif // vtkRangeIterableTraits_h

// VTK-HeaderTest-Exclude: vtkRangeIterableTraits.h
