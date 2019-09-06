/*===========================================================================*/
/* Distributed under OSI-approved BSD 3-Clause License.                      */
/* For copyright, see the following accompanying files or https://vtk.org:   */
/* - Sandia-Copyright.txt                                                    */
/*===========================================================================*/

// .NAME EasyView - Shows regular way of for linking multiple views.
//
// .SECTION Description
// EasyView shows a way to link various views using vtkAnnotationLink
// shared between views. Selection in a particular view will update the
// selection in all other views associated.

// Other way to get the same functionality is by using vtkEventQtSlotConnect
// and providing corresponding QT slot.

// .SECTION See Also
// CustomLinkView.


#ifndef EasyView_H
#define EasyView_H

#include "vtkSmartPointer.h"    // Required for smart pointer internal ivars.

#include <QMainWindow>

// Forward Qt class declarations
class Ui_EasyView;

// Forward VTK class declarations
class vtkXMLTreeReader;
class vtkGraphLayoutView;
class vtkQtTableView;
class vtkQtTreeView;


class EasyView : public QMainWindow
{
  Q_OBJECT

public:

  // Constructor/Destructor
  EasyView();
  ~EasyView() override;

public slots:

  virtual void slotOpenXMLFile();
  virtual void slotExit();

protected:

protected slots:

private:

  // Methods
  void SetupAnnotationLink();


  // Members
  vtkSmartPointer<vtkXMLTreeReader>       XMLReader;
  vtkSmartPointer<vtkGraphLayoutView>     GraphView;
  vtkSmartPointer<vtkQtTreeView>          TreeView;
  vtkSmartPointer<vtkQtTableView>         TableView;
  vtkSmartPointer<vtkQtTreeView>          ColumnView;

  // Designer form
  Ui_EasyView *ui;
};

#endif // EasyView_H
