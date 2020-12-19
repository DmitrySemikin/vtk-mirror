#include "vtkImageCompare.h"
#include "vtkDataObject.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkSMPTools.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkStandardNewMacro(vtkImageCompare);

class vtkImageCompare::GaussianSmooth1D1CFunctor
{
public:
  unsigned char* inPixels;
  int* extent;
  vtkIdType* inIncs;
  int kernelSize;
  double* kernel;
  int direction;
  int channel;

  GaussianSmooth1D1CFunctor(unsigned char* inPixels, int* extent,
    vtkIdType* inIncs, int kernelSize, double* kernel, int direction,
    int channel)
    : inPixels(inPixels)
    , extent(extent)
    , inIncs(inIncs)
    , kernelSize(kernelSize)
    , kernel(kernel)
    , direction(direction)
    , channel(channel)
  {
  }

  void operator()(vtkIdType i, vtkIdType endI)
  {
    int minJ = direction ? extent[2] : extent[0];
    int maxJ = direction ? extent[3] + 1 : extent[1] + 1;
    for (; i < endI; i++)
    {
      double tempNew[maxJ - minJ];
      for (int j = minJ; j < maxJ; j++)
      {
        double sum = 0.;
        for (int k = 0; k < kernelSize; k++)
        {
          int pos = j + (k - kernelSize / 2);
          // Padding
          if (pos < minJ)
          {
            pos = maxJ - (minJ - pos) - 1;
          }
          if (pos >= maxJ)
          {
            pos = minJ + (pos - maxJ);
          }
          unsigned char* pixel =
            inPixels + inIncs[1 - direction] * i + inIncs[direction] * pos;
          double color = static_cast<double>(pixel[channel]) / 255.;
          sum += color * kernel[k];
        }
        tempNew[j] = sum;
      }
      for (int j = minJ; j < maxJ; j++)
      {
        unsigned char* pixel =
          inPixels + inIncs[1 - direction] * i + inIncs[direction] * j;
        pixel[channel] =
          static_cast<unsigned char>(std::floor(tempNew[j] * 255.));
      }
    }
  }
};

class vtkImageCompare::ComputeDifferenceImageFunctor
{
public:
  unsigned char* in1Pixels;
  unsigned char* in2Pixels;
  unsigned char* outPixels;
  int* extent;
  vtkIdType *in1Incs, *in2Incs, *outIncs;
  double fuzzyThreshold;

  ComputeDifferenceImageFunctor(unsigned char* in1Pixels,
    unsigned char* in2Pixels, unsigned char* outPixels, int* extent,
    vtkIdType* in1Incs, vtkIdType* in2Incs, vtkIdType* outIncs,
    double fuzzyThreshold)
    : in1Pixels(in1Pixels)
    , in2Pixels(in2Pixels)
    , outPixels(outPixels)
    , extent(extent)
    , in1Incs(in1Incs)
    , in2Incs(in2Incs)
    , outIncs(outIncs)
    , fuzzyThreshold(fuzzyThreshold)
  {
  }
  void operator()(vtkIdType row, vtkIdType endRow)
  {
    for (; row < endRow; row++)
    {
      for (int col = extent[0]; col <= extent[1]; col++)
      {
        unsigned char* in1Pixel =
          in1Pixels + row * in1Incs[1] + col * in1Incs[0];
        unsigned char* in2Pixel =
          in2Pixels + row * in2Incs[1] + col * in2Incs[0];
        bool different = false;
        for (int channel = 0; channel < 3; channel++)
        {
          double in1Color = static_cast<int>(in1Pixel[channel]) / 255.;
          double in2Color = static_cast<int>(in2Pixel[channel]) / 255.;
          double distance = in1Color - in2Color;
          if (distance * distance > fuzzyThreshold)
          {
            different = true;
            break;
          }
        }
        unsigned char* outPixel =
          outPixels + row * outIncs[1] + col * outIncs[0];
        if (different)
        {
          outPixel[0] = static_cast<unsigned char>(0xf1);
          outPixel[1] = static_cast<unsigned char>(0x00);
          outPixel[2] = static_cast<unsigned char>(0x1e);
        }
        else
        {
          outPixel[0] = static_cast<unsigned char>(0x88);
          outPixel[1] = static_cast<unsigned char>(0x88);
          outPixel[2] = static_cast<unsigned char>(0x88);
        }
      }
    }
  }
};

class vtkImageCompare::ComputeErrorFunctor
{
public:
  unsigned char* in1Pixels;
  unsigned char* in2Pixels;
  int* extent;
  vtkIdType *in1Incs, *in2Incs;
  double* error;
  vtkSMPThreadLocal<double> localError;

  ComputeErrorFunctor(unsigned char* in1Pixels, unsigned char* in2Pixels,
    int* extent, vtkIdType* in1Incs, vtkIdType* in2Incs, double* error)
    : in1Pixels(in1Pixels)
    , in2Pixels(in2Pixels)
    , extent(extent)
    , in1Incs(in1Incs)
    , in2Incs(in2Incs)
    , error(error)
  {
  }

  void Initialize()
  {
    double& e = localError.Local();
    e = 0.;
  }

  virtual void Reduce() = 0;

  virtual void operator()(vtkIdType row, vtkIdType endRow) = 0;
};

class vtkImageCompare::ComputeMeanSquaredError
  : public vtkImageCompare::ComputeErrorFunctor
{
public:
  ComputeMeanSquaredError(unsigned char* in1Pixels, unsigned char* in2Pixels,
    int* extent, vtkIdType* in1Incs, vtkIdType* in2Incs, double* error)
    : vtkImageCompare::ComputeErrorFunctor(
        in1Pixels, in2Pixels, extent, in1Incs, in2Incs, error)
  {
  }

  void Reduce() override
  {
    for (auto it = localError.begin(); it != localError.end(); it++)
    {
      *error += *it;
    }
    *error /= static_cast<double>((extent[1] + 1) * (extent[3] + 1));
  }

  void operator()(vtkIdType row, vtkIdType endRow) override
  {
    double& e = localError.Local();
    for (; row < endRow; row++)
    {
      double rowError = 0.;
      for (int col = extent[0]; col <= extent[1]; col++)
      {
        unsigned char* in1Pixel =
          in1Pixels + row * in1Incs[1] + col * in1Incs[0];
        unsigned char* in2Pixel =
          in2Pixels + row * in2Incs[1] + col * in2Incs[0];
        for (int channel = 0; channel < 3; channel++)
        {
          double in1Color = static_cast<double>(in1Pixel[channel]);
          double in2Color = static_cast<double>(in2Pixel[channel]);
          double distance = in1Color - in2Color;
          rowError += distance * distance;
        }
      }
      e += rowError;
    }
  }
};

class vtkImageCompare::ComputePeakSignalToNoiseRatioError
  : public vtkImageCompare::ComputeErrorFunctor
{
public:
  ComputePeakSignalToNoiseRatioError(unsigned char* in1Pixels,
    unsigned char* in2Pixels, int* extent, vtkIdType* in1Incs,
    vtkIdType* in2Incs, double* error)
    : vtkImageCompare::ComputeErrorFunctor(
        in1Pixels, in2Pixels, extent, in1Incs, in2Incs, error)
  {
  }

  void Reduce() override
  {
    for (auto it = localError.begin(); it != localError.end(); it++)
    {
      *error += *it;
    }
    *error /= static_cast<double>((extent[1] + 1) * (extent[3] + 1));
    *error = -10. * std::log10(*error / (255. * 255.));
  }

  void operator()(vtkIdType row, vtkIdType endRow) override
  {
    double& e = localError.Local();
    for (; row < endRow; row++)
    {
      double rowError = 0.;
      for (int col = extent[0]; col <= extent[1]; col++)
      {
        unsigned char* in1Pixel =
          in1Pixels + row * in1Incs[1] + col * in1Incs[0];
        unsigned char* in2Pixel =
          in2Pixels + row * in2Incs[1] + col * in2Incs[0];
        for (int channel = 0; channel < 3; channel++)
        {
          double in1Color = static_cast<double>(in1Pixel[channel]);
          double in2Color = static_cast<double>(in2Pixel[channel]);
          double distance = in1Color - in2Color;
          rowError += distance * distance;
        }
      }
      e += rowError;
    }
  }
};

vtkImageCompare::vtkImageCompare()
{
  ErrorThreshold = 200.;
  FuzzyThreshold = 0.;
  ErrorMetric = MeanSquaredErrorMetric;
  Error = 0.;
  ThresholdedError = 0.;
  IsEqual = false;
  this->SetNumberOfInputPorts(2);
  SmoothBeforeCompare = true;
  KernelSize = 5;
  Kernel = new double[KernelSize];
  // default sigma = 1.0
  Kernel[0] = 0.090404;
  Kernel[1] = 0.241812;
  Kernel[2] = 0.335568;
  Kernel[3] = 0.241812;
  Kernel[4] = 0.090404;
}

vtkImageCompare::~vtkImageCompare()
{
  delete[] Kernel;
}

void vtkImageCompare::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Error Threshold: " << this->ErrorThreshold;
  os << indent << "Fuzzy Threshold: " << this->FuzzyThreshold;
  switch (this->ErrorMetric)
  {
    case MeanSquaredErrorMetric:
      os << indent << "Error Metric: "
         << "Mean Squared Error";
      break;
    case PeakSignalToNoiseRatioErrorMetric:
      os << indent << "Error Metric: "
         << "Peak Signal to Noise Ratio";
    default:
      os << indent << "Error Metric: "
         << "Unsupported error metric";
      break;
  }
  os << indent << "Smooth before compare: " << this->SmoothBeforeCompare;
  os << indent << "Error: " << this->Error;
  os << indent << "Thresholded Error: " << this->ThresholdedError;
  os << indent << "Is Equal: " << this->IsEqual;
}

void vtkImageCompare::ComputeError(unsigned char* in1Pixels,
  unsigned char* in2Pixels, int* extent, vtkIdType* in1Incs, vtkIdType* in2Incs)
{
  Error = 0.;
  vtkImageCompare::ComputeErrorFunctor* errorFunctor = nullptr;
  switch (this->ErrorMetric)
  {
    case MeanSquaredErrorMetric:
      errorFunctor = new vtkImageCompare::ComputeMeanSquaredError(
        in1Pixels, in2Pixels, extent, in1Incs, in2Incs, &Error);
      break;
    case PeakAbsoluteErrorMetric:
    default:
      errorFunctor = new vtkImageCompare::ComputePeakSignalToNoiseRatioError(
        in1Pixels, in2Pixels, extent, in1Incs, in2Incs, &Error);
  }
  if (errorFunctor)
  {
    vtkSMPTools::For(extent[2], extent[3] + 1, *errorFunctor);
    delete errorFunctor;
  }
}

void vtkImageCompare::GaussianSmooth(
  unsigned char* inPixels, int* extent, vtkIdType* inIncs)
{
  GaussianSmooth1D1CFunctor gaussianXR(inPixels, extent, inIncs, KernelSize,
    Kernel, GaussianSmoothDirection::X, GaussianSmoothChannel::R);
  GaussianSmooth1D1CFunctor gaussianXG(inPixels, extent, inIncs, KernelSize,
    Kernel, GaussianSmoothDirection::X, GaussianSmoothChannel::G);
  GaussianSmooth1D1CFunctor gaussianXB(inPixels, extent, inIncs, KernelSize,
    Kernel, GaussianSmoothDirection::X, GaussianSmoothChannel::B);
  GaussianSmooth1D1CFunctor gaussianYR(inPixels, extent, inIncs, KernelSize,
    Kernel, GaussianSmoothDirection::Y, GaussianSmoothChannel::R);
  GaussianSmooth1D1CFunctor gaussianYG(inPixels, extent, inIncs, KernelSize,
    Kernel, GaussianSmoothDirection::Y, GaussianSmoothChannel::G);
  GaussianSmooth1D1CFunctor gaussianYB(inPixels, extent, inIncs, KernelSize,
    Kernel, GaussianSmoothDirection::Y, GaussianSmoothChannel::B);

  vtkSMPTools::For(extent[2], extent[3] + 1, gaussianXR);
  vtkSMPTools::For(extent[2], extent[3] + 1, gaussianXG);
  vtkSMPTools::For(extent[2], extent[3] + 1, gaussianXB);

  vtkSMPTools::For(extent[0], extent[1] + 1, gaussianYR);
  vtkSMPTools::For(extent[0], extent[1] + 1, gaussianYG);
  vtkSMPTools::For(extent[0], extent[1] + 1, gaussianYB);
}

int vtkImageCompare::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkInformation* inInfo1 = inputVector[0]->GetInformationObject(0);
  vtkInformation* inInfo2 = inputVector[1]->GetInformationObject(0);

  int* in1Ext = inInfo1->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());
  int* in2Ext = inInfo2->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());

  int i;
  if (in1Ext[0] != in2Ext[0] || in1Ext[1] != in2Ext[1] ||
    in1Ext[2] != in2Ext[2] || in1Ext[3] != in2Ext[3] ||
    in1Ext[4] != in2Ext[4] || in1Ext[5] != in2Ext[5])
  {
    this->Error = 1000.0;
    this->ThresholdedError = 1000.0;
    this->IsEqual = false;

    vtkErrorMacro("ExecuteInformation: Input are not the same size.\n"
      << " Input1 is: " << in1Ext[0] << "," << in1Ext[1] << "," << in1Ext[2]
      << "," << in1Ext[3] << "," << in1Ext[4] << "," << in1Ext[5] << "\n"
      << " Input2 is: " << in2Ext[0] << "," << in2Ext[1] << "," << in2Ext[2]
      << "," << in2Ext[3] << "," << in2Ext[4] << "," << in2Ext[5]);
  }

  // We still need to set the whole extent to be the intersection.
  // Otherwise the execute may crash.
  int ext[6];
  for (i = 0; i < 3; ++i)
  {
    ext[i * 2] = in1Ext[i * 2];
    if (ext[i * 2] < in2Ext[i * 2])
    {
      ext[i * 2] = in2Ext[i * 2];
    }
    ext[i * 2 + 1] = in1Ext[i * 2 + 1];
    if (ext[i * 2 + 1] > in2Ext[i * 2 + 1])
    {
      ext[i * 2 + 1] = in2Ext[i * 2 + 1];
    }
  }
  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), ext, 6);

  return 1;
}

int vtkImageCompare::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkImageData* inData[2];
  vtkImageData* outData;

  // prepare input and output data
  vtkInformation* info = outputVector->GetInformationObject(0);
  outData = vtkImageData::SafeDownCast(info->Get(vtkDataObject::DATA_OBJECT()));
  outData->SetExtent(
    info->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()));
  outData->AllocateScalars(info);
  for (int i = 0; i < 2; i++)
  {
    info = inputVector[i]->GetInformationObject(0);
    inData[i] =
      vtkImageData::SafeDownCast(info->Get(vtkDataObject::DATA_OBJECT()));
  }
  if (inData[0]->GetNumberOfScalarComponents() < 3 ||
    inData[1]->GetNumberOfScalarComponents() < 3 ||
    outData->GetNumberOfScalarComponents() < 3)
  {
    vtkErrorMacro("Expecting at least 3 components (RGB or RGBA)");
    return 0;
  }
  // this filter expects that input is the same type as output.
  if (inData[0]->GetScalarType() != VTK_UNSIGNED_CHAR ||
    inData[1]->GetScalarType() != VTK_UNSIGNED_CHAR ||
    outData->GetScalarType() != VTK_UNSIGNED_CHAR)
  {
    vtkErrorMacro("All ScalarTypes must be unsigned char");
    return 0;
  }

  int extent[6];
  outData->GetExtent(extent);

  unsigned char *inPixels[2], *outPixels;

  for (int i = 0; i < 2; i++)
  {
    inPixels[i] =
      static_cast<unsigned char*>(inData[i]->GetScalarPointerForExtent(extent));
  }
  outPixels =
    static_cast<unsigned char*>(outData->GetScalarPointerForExtent(extent));

  vtkIdType* inIncs[2];

  for (int i = 0; i < 2; i++)
  {
    inIncs[i] = new vtkIdType[3];
    inData[i]->GetIncrements(inIncs[i]);
  }

  vtkIdType* outIncs = new vtkIdType[3];
  outData->GetIncrements(outIncs);

  vtkImageData* inDataSmoothed[2];
  if (this->SmoothBeforeCompare)
  {
    unsigned char* inSmoothedPixels[2];
    vtkIdType* inSmoothedIncs[2];

    for (int i = 0; i < 2; i++)
    {
      inDataSmoothed[i] = vtkImageData::New();
      inDataSmoothed[i]->DeepCopy(inData[i]);
      inSmoothedPixels[i] = static_cast<unsigned char*>(
        inDataSmoothed[i]->GetScalarPointerForExtent(extent));
      inSmoothedIncs[i] = new vtkIdType[3];
      inDataSmoothed[i]->GetIncrements(inSmoothedIncs[i]);
      GaussianSmooth(inSmoothedPixels[i], extent, inSmoothedIncs[i]);
      inPixels[i] = inSmoothedPixels[i];
      inIncs[i] = inSmoothedIncs[i];
    }
  }

  ComputeError(inPixels[0], inPixels[1], extent, inIncs[0], inIncs[1]);

  ThresholdedError = Error - ErrorThreshold;
  if (ThresholdedError < 0.)
  {
    ThresholdedError = 0.;
  }

  ComputeDifferenceImageFunctor diff(inPixels[0], inPixels[1], outPixels,
    extent, inIncs[0], inIncs[1], outIncs, this->FuzzyThreshold);

  vtkSMPTools::For(extent[2], extent[3] + 1, diff);

  IsEqual = Error > ErrorThreshold;

  if (SmoothBeforeCompare)
  {
    for (int i = 0; i < 2; i++)
    {
      inDataSmoothed[i]->Delete();
    }
  }

  return 1;
}

vtkImageData* vtkImageCompare::GetImage()
{
  if (this->GetNumberOfInputConnections(1) < 1)
  {
    return nullptr;
  }
  return vtkImageData::SafeDownCast(this->GetExecutive()->GetInputData(1, 0));
}
