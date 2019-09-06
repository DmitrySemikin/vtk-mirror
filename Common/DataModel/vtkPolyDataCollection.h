/*===========================================================================*/
/* Distributed under OSI-approved BSD 3-Clause License.                      */
/* For copyright, see the following accompanying files or https://vtk.org:   */
/* - VTK-Copyright.txt                                                       */
/*===========================================================================*/
/**
 * @class   vtkPolyDataCollection
 * @brief   maintain a list of polygonal data objects
 *
 * vtkPolyDataCollection is an object that creates and manipulates ordered
 * lists of datasets of type vtkPolyData.
 *
 * @sa
 * vtkDataSetCollection vtkCollection
*/

#ifndef vtkPolyDataCollection_h
#define vtkPolyDataCollection_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkCollection.h"

#include "vtkPolyData.h" // Needed for static cast

class VTKCOMMONDATAMODEL_EXPORT vtkPolyDataCollection : public vtkCollection
{
public:
  static vtkPolyDataCollection *New();
  vtkTypeMacro(vtkPolyDataCollection,vtkCollection);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Add a poly data to the bottom of the list.
   */
  void AddItem(vtkPolyData *pd)
  {
      this->vtkCollection::AddItem(pd);
  }

  /**
   * Get the next poly data in the list.
   */
  vtkPolyData *GetNextItem() {
    return static_cast<vtkPolyData *>(this->GetNextItemAsObject());};

  /**
   * Reentrant safe way to get an object in a collection. Just pass the
   * same cookie back and forth.
   */
  vtkPolyData *GetNextPolyData(vtkCollectionSimpleIterator &cookie) {
    return static_cast<vtkPolyData *>(this->GetNextItemAsObject(cookie));};

protected:
  vtkPolyDataCollection() {}
  ~vtkPolyDataCollection() override {}

private:
  // hide the standard AddItem from the user and the compiler.
  void AddItem(vtkObject *o) { this->vtkCollection::AddItem(o); };

private:
  vtkPolyDataCollection(const vtkPolyDataCollection&) = delete;
  void operator=(const vtkPolyDataCollection&) = delete;
};


#endif
