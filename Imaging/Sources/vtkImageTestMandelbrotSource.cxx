#include "vtkImageTestMandelbrotSource.h"
#include "vtkObjectFactory.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkImageData.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkStandardNewMacro(vtkImageTestMandelbrotSource);


int vtkImageTestMandelbrotSource::RequestData(vtkInformation *request,
                        vtkInformationVector** inputVector,
                        vtkInformationVector* outputVector)
{
  // get the output
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkImageData *data = vtkImageData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // We need to allocate our own scalars since we are overriding
  // the superclasses "Execute()" method.
  int *ext = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT());
  data->SetExtent(ext);
  data->AllocateScalars(outInfo);

  float *ptr = static_cast<float *>(data->GetScalarPointerForExtent(ext));

  int pieces = (ext[5]-ext[4]+1)*(ext[3]-ext[2]+1)*(ext[1]-ext[0]+1);

  for(int i=0;i<pieces;i++)
    {
    *ptr=3.0;
    ptr++;
    }

  return 1;
}
