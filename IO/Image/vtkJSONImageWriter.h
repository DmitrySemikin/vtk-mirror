/*===========================================================================*/
/* Distributed under OSI-approved BSD 3-Clause License.                      */
/* For copyright, see the following accompanying files or https://vtk.org:   */
/* - VTK-Copyright.txt                                                       */
/*===========================================================================*/
/**
 * @class   vtkJSONImageWriter
 * @brief   Writes vtkImageData to a JSON file.
 *
 * vtkJSONImageWriter writes a JSON file which will describe the
 * data inside a vtkImageData.
*/

#ifndef vtkJSONImageWriter_h
#define vtkJSONImageWriter_h

#include "vtkIOImageModule.h" // For export macro
#include "vtkImageAlgorithm.h"

class VTKIOIMAGE_EXPORT vtkJSONImageWriter : public vtkImageAlgorithm
{
public:
  static vtkJSONImageWriter *New();
  vtkTypeMacro(vtkJSONImageWriter,vtkImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Specify file name for the image file.
   */
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
  //@}

  //@{
  /**
   * Specify ArrayName to export. By default nullptr which will dump ALL arrays.
   */
  vtkSetStringMacro(ArrayName);
  vtkGetStringMacro(ArrayName);
  //@}

  //@{
  /**
   * Specify Slice in Z to export. By default -1 which will dump the full 3D domain.
   */
  vtkSetMacro(Slice, int);
  vtkGetMacro(Slice, int);
  //@}

  /**
   * The main interface which triggers the writer to start.
   */
  virtual void Write();

protected:
  vtkJSONImageWriter();
  ~vtkJSONImageWriter() override;

  char *FileName;
  char *ArrayName;
  int Slice;

  int RequestData(vtkInformation *request,
                          vtkInformationVector** inputVector,
                          vtkInformationVector* outputVector) override;

private:
  vtkJSONImageWriter(const vtkJSONImageWriter&) = delete;
  void operator=(const vtkJSONImageWriter&) = delete;
};

#endif
