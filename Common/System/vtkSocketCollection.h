/*===========================================================================*/
/* Distributed under OSI-approved BSD 3-Clause License.                      */
/* For copyright, see the following accompanying files or https://vtk.org:   */
/* - VTK-Copyright.txt                                                       */
/*===========================================================================*/
/**
 * @class   vtkSocketCollection
 * @brief    a collection for sockets.
 *
 * Apart from being vtkCollection subclass for sockets, this class
 * provides means to wait for activity on all the sockets in the
 * collection simultaneously.
*/

#ifndef vtkSocketCollection_h
#define vtkSocketCollection_h

#include "vtkCommonSystemModule.h" // For export macro
#include "vtkCollection.h"

class vtkSocket;
class VTKCOMMONSYSTEM_EXPORT vtkSocketCollection : public vtkCollection
{
public:
  static vtkSocketCollection* New();
  vtkTypeMacro(vtkSocketCollection, vtkCollection);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  // Add Socket to the collection.
  void AddItem(vtkSocket* soc);

  /**
   * Select all Connected sockets in the collection. If msec is specified,
   * it timesout after msec milliseconds on inactivity.
   * Returns 0 on timeout, -1 on error; 1 is a socket was selected.
   * The selected socket can be retrieved by GetLastSelectedSocket().
   */
  int SelectSockets(unsigned long msec =0);

  /**
   * Returns the socket selected during the last SelectSockets(), if any.
   * nullptr otherwise.
   */
  vtkSocket* GetLastSelectedSocket()
    {return this->SelectedSocket; }

  //@{
  /**
   * Overridden to unset SelectedSocket.
   */
  void ReplaceItem(int i, vtkObject *);
  void RemoveItem(int i);
  void RemoveItem(vtkObject *);
  void RemoveAllItems();
  //@}

protected:
  vtkSocketCollection();
  ~vtkSocketCollection() override;

  vtkSocket* SelectedSocket;
private:
  // Hide the standard AddItem.
  void AddItem(vtkObject* o) { this->Superclass::AddItem(o); }

private:
  vtkSocketCollection(const vtkSocketCollection&) = delete;
  void operator=(const vtkSocketCollection&) = delete;
};

#endif

