/*===========================================================================*/
/* Distributed under OSI-approved BSD 3-Clause License.                      */
/* For copyright, see the following accompanying files or https://vtk.org:   */
/* - VTK-Copyright.txt                                                       */
/*===========================================================================*/
/**
 * @class   vtkFileOutputWindow
 * @brief   File Specific output window class
 *
 * Writes debug/warning/error output to a log file instead of the console.
 * To use this class, instantiate it and then call SetInstance(this).
 *
*/

#ifndef vtkFileOutputWindow_h
#define vtkFileOutputWindow_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkOutputWindow.h"


class VTKCOMMONCORE_EXPORT vtkFileOutputWindow : public vtkOutputWindow
{
public:
  vtkTypeMacro(vtkFileOutputWindow, vtkOutputWindow);

  static vtkFileOutputWindow* New();

  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Put the text into the log file.
   * New lines are converted to carriage return new lines.
   */
  void DisplayText(const char*) override;

  //@{
  /**
   * Sets the name for the log file.
   */
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
  //@}

  //@{
  /**
   * Turns on buffer flushing for the output
   * to the log file.
   */
  vtkSetMacro(Flush, vtkTypeBool);
  vtkGetMacro(Flush, vtkTypeBool);
  vtkBooleanMacro(Flush, vtkTypeBool);
  //@}

  //@{
  /**
   * Setting append will cause the log file to be
   * opened in append mode.  Otherwise, if the log file exists,
   * it will be overwritten each time the vtkFileOutputWindow
   * is created.
   */
  vtkSetMacro(Append, vtkTypeBool);
  vtkGetMacro(Append, vtkTypeBool);
  vtkBooleanMacro(Append, vtkTypeBool);
  //@}

protected:
  vtkFileOutputWindow();
  ~vtkFileOutputWindow() override;
  void Initialize();

  char* FileName;
  ofstream* OStream;
  vtkTypeBool Flush;
  vtkTypeBool Append;

private:
  vtkFileOutputWindow(const vtkFileOutputWindow&) = delete;
  void operator=(const vtkFileOutputWindow&) = delete;
};


#endif
