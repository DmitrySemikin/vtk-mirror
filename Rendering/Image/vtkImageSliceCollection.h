/*===========================================================================*/
/* Distributed under OSI-approved BSD 3-Clause License.                      */
/* For copyright, see the following accompanying files or https://vtk.org:   */
/* - VTK-Copyright.txt                                                       */
/*===========================================================================*/
/**
 * @class   vtkImageSliceCollection
 * @brief   a sorted list of image slice objects
 *
 * vtkImageSliceCollection is a vtkPropCollection that maintains
 * a list of vtkImageSlice objects that are sorted by LayerNumber.
 * This allows the images to be rendered in the correct order.
 * @sa
 * vtkImageSlice vtkImageAssembly
*/

#ifndef vtkImageSliceCollection_h
#define vtkImageSliceCollection_h

#include "vtkRenderingImageModule.h" // For export macro
#include "vtkPropCollection.h"
#include "vtkImageSlice.h" // to allow inline static-cast

class VTKRENDERINGIMAGE_EXPORT vtkImageSliceCollection : public vtkPropCollection
{
 public:
  static vtkImageSliceCollection *New();
  vtkTypeMacro(vtkImageSliceCollection,vtkPropCollection);

  /**
   * Sorts the vtkImageSliceCollection by layer number.  Smaller layer
   * numbers are first. Layer numbers can be any integer value. Items
   * with the same layer number will be kept in the same relative order
   * as before the sort.
   */
  void Sort();

  /**
   * Add an image to the list.  The new image is inserted in the list
   * according to its layer number.
   */
  void AddItem(vtkImageSlice *a);

  /**
   * Standard Collection methods.  You must call InitTraversal
   * before calling GetNextImage.  If possible, you should use the
   * GetNextImage method that takes a collection iterator instead.
   */
  vtkImageSlice *GetNextImage();

  /**
   * Reentrant safe way to get an object in a collection.
   */
  vtkImageSlice *GetNextImage(vtkCollectionSimpleIterator &cookie);

  /**
   * Access routine provided for compatibility with previous
   * versions of VTK.  Please use the GetNextImage() variant
   * where possible.
   */
  vtkImageSlice *GetNextItem() { return this->GetNextImage(); }

protected:
  vtkImageSliceCollection() {}
  ~vtkImageSliceCollection() override;

  void DeleteElement(vtkCollectionElement *) override;

private:
  // hide the standard AddItem from the user and the compiler.
  void AddItem(vtkObject *o) { this->vtkCollection::AddItem(o); };
  void AddItem(vtkProp *o) { this->vtkPropCollection::AddItem(o); };

private:
  vtkImageSliceCollection(const vtkImageSliceCollection&) = delete;
  void operator=(const vtkImageSliceCollection&) = delete;
};

inline vtkImageSlice *vtkImageSliceCollection::GetNextImage()
{
  return static_cast<vtkImageSlice *>(this->GetNextItemAsObject());
}

inline vtkImageSlice *vtkImageSliceCollection::GetNextImage(
  vtkCollectionSimpleIterator &cookie)
{
  return static_cast<vtkImageSlice *>(this->GetNextItemAsObject(cookie));
}


#endif
