/*===========================================================================*/
/* Distributed under OSI-approved BSD 3-Clause License.                      */
/* For copyright, see the following accompanying files or https://vtk.org:   */
/* - VTK-Copyright.txt                                                       */
/*===========================================================================*/
/**
 * @class   vtkDataArrayCollectionIterator
 * @brief   iterator through a vtkDataArrayCollection.
 *
 * vtkDataArrayCollectionIterator provides an implementation of
 * vtkCollectionIterator which allows the items to be retrieved with
 * the proper subclass pointer type for vtkDataArrayCollection.
*/

#ifndef vtkDataArrayCollectionIterator_h
#define vtkDataArrayCollectionIterator_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkCollectionIterator.h"

class vtkDataArray;
class vtkDataArrayCollection;

class VTKCOMMONCORE_EXPORT vtkDataArrayCollectionIterator : public vtkCollectionIterator
{
public:
  vtkTypeMacro(vtkDataArrayCollectionIterator,vtkCollectionIterator);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkDataArrayCollectionIterator* New();

  //@{
  /**
   * Set the collection over which to iterate.
   */
  void SetCollection(vtkCollection*) override;
  void SetCollection(vtkDataArrayCollection*);
  //@}

  /**
   * Get the item at the current iterator position.  Valid only when
   * IsDoneWithTraversal() returns 1.
   */
  vtkDataArray* GetDataArray();

protected:
  vtkDataArrayCollectionIterator();
  ~vtkDataArrayCollectionIterator() override;

private:
  vtkDataArrayCollectionIterator(const vtkDataArrayCollectionIterator&) = delete;
  void operator=(const vtkDataArrayCollectionIterator&) = delete;
};

#endif
