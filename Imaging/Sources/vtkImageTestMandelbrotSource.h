
#ifndef vtkImageTestMandelbrotSource_h
#define vtkImageTestMandelbrotSource_h

#include "vtkImageMandelbrotSource.h"

class VTKIMAGINGSOURCES_EXPORT vtkImageTestMandelbrotSource: public vtkImageMandelbrotSource
{
protected:
  vtkImageTestMandelbrotSource() {};
  ~vtkImageTestMandelbrotSource(){};
public:
  static vtkImageTestMandelbrotSource *New();
  vtkTypeMacro(vtkImageTestMandelbrotSource,vtkImageMandelbrotSource);

  int RequestData(vtkInformation *request,
                          vtkInformationVector** inputVector,
                          vtkInformationVector* outputVector);
  void PrintSelf(ostream& os, vtkIndent indent);
};

#endif
