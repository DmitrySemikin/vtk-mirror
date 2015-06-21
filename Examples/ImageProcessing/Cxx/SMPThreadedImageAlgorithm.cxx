#include <vtkSmartPointer.h>

#include <vtkTimerLog.h>
#include <vtkSMPTools.h>

//Test 1
#include <vtkImageCast.h>
//Test 2
#include <vtkImageDotProduct.h>
#include <vtkImageData.h>
//Test 3
#include <vtkImageReader2Factory.h>
#include <vtkImageReader2.h>

#include <vtkImageActor.h>
#include <vtkImageReslice.h>

#include <vtkTransform.h>

#include <vtkImageConvolve.h>
#include <vtkImageMandelbrotSource.h>

#define CSV_RESULT_FILE "executionResult.csv"

struct TestParms
{
  int testCase;
  bool enableSMP;
  bool enableSMPBlockMode;
  int numberOfThreadsToRun;
  int numberOfSMPBlocks;
  int workSize;
  char * additionalData;
};

int WriteResultToCSV(double executionTime,TestParms *parm)
{
  char writeOutput[100];
  sprintf(writeOutput, "%d,%d,%d,%d,%d,%d,%f,%s\n"
  ,parm->testCase
  ,parm->enableSMP
  ,parm->enableSMPBlockMode
  ,parm->numberOfThreadsToRun
  ,parm->numberOfSMPBlocks
  ,parm->workSize
  ,executionTime
  ,parm->additionalData);

  FILE *f = fopen(CSV_RESULT_FILE, "a+");
  if (f == NULL)
  {
    return -1;
  }
  fprintf(f, "%s", writeOutput);
  fclose(f);
  return 0;
}


int main(int argc, char *argv[])
{
  if(argc <7)
    {
    printf ("Not all parms have being passed in\n"
        "parm#1: Test Number.\n"
        "parm#2: Enable/Disabl SMP, use true/false\n"
        "parm#3: Enable block mode splitting, this is only valid if SMP is true. \n"
        "parm#4: Number Of threads to run ~4, this is only valid if SMP is true. \n"
        "parm#5: Number of split SMP blocks ~500.\n"
        "parm#6: The size of the work load ~5000\n"
        );
    return -1;
    }

  TestParms parms;
  parms.additionalData = " ";

  parms.testCase = atoi(argv[1]);


  if(strcmp(argv[2],"true")==0)
    {
    parms.enableSMP =true;
    }
  else
    {
    parms.enableSMP =false;
    }
  if(strcmp(argv[3],"true")==0)
    {
    parms.enableSMPBlockMode = true;
    }
  else
    {
    parms.enableSMPBlockMode = false;
    }

  parms.numberOfThreadsToRun = atoi(argv[4]);
  parms.numberOfSMPBlocks = atoi(argv[5]);
  parms.workSize = atoi(argv[6]);

  // Initilize with passed in number of threads
  vtkTimerLog *tl = vtkTimerLog::New();
  vtkSMPTools::Initialize(parms.numberOfThreadsToRun);

  printf("Running test case %d, with smp %d, blockmode %d, numberOfThreads %d\n"
    ,parms.testCase
    ,parms.enableSMP
    ,parms.enableSMPBlockMode
    ,parms.numberOfThreadsToRun);

  //---------------------------------------------------------------------
  //----Test Case 1: SMP overhead compared with old multi-threader
  switch(parms.testCase)
   {
    case 1:
    {
      int workExtent[6] = {0,parms.workSize,0,parms.workSize,0,0};

      vtkSmartPointer<vtkImageMandelbrotSource> source =
      vtkSmartPointer<vtkImageMandelbrotSource>::New();
      source->SetWholeExtent(workExtent);
      source->Update();

      vtkSmartPointer<vtkImageCast> castFilter =
      vtkSmartPointer<vtkImageCast>::New();
      castFilter->SetInputConnection(source->GetOutputPort());
      castFilter->EnableSMP(parms.enableSMP);
      castFilter->SetSMPBlocks(parms.numberOfSMPBlocks);
      castFilter->SetSMPBlockMode(parms.enableSMPBlockMode);
      castFilter->SetOutputScalarTypeToUnsignedChar();

      tl->StartTimer();
      castFilter->Update();
      tl->StopTimer();

      cerr << "Wall Time = " << tl->GetElapsedTime() << "\n";

      WriteResultToCSV(tl->GetElapsedTime(),&parms);
      break;

    }
  //----Test Case 2: SMP overhead compared with old multi-threader
  case 2:
    {
      // Create an image data
      vtkSmartPointer<vtkImageData> imageData =
      vtkSmartPointer<vtkImageData>::New();

      // Specify the size of the image data
      imageData->SetDimensions(parms.workSize,parms.workSize,parms.workSize);
      imageData->AllocateScalars(VTK_DOUBLE,1);

      int* dims = imageData->GetDimensions();
      // int dims[3]; // can't do this

      // Fill every entry of the image data with "2.0"
      for (int z = 0; z < dims[2]; z++)
        {
        for (int y = 0; y < dims[1]; y++)
          {
          for (int x = 0; x < dims[0]; x++)
            {
            double* pixel = static_cast<double*>(imageData->GetScalarPointer(x,y,z));
            pixel[0] = 2.0;
            }
          }
        }

      // Create an image data 2
      vtkSmartPointer<vtkImageData> imageData2 =
      vtkSmartPointer<vtkImageData>::New();

      // Specify the size of the image data
      imageData2->SetDimensions(parms.workSize,parms.workSize,parms.workSize);
      imageData2->AllocateScalars(VTK_DOUBLE,1);

      dims = imageData2->GetDimensions();
      // int dims[3]; // can't do this

      // Fill every entry of the image data with "2.0"
      for (int z = 0; z < dims[2]; z++)
        {
        for (int y = 0; y < dims[1]; y++)
          {
          for (int x = 0; x < dims[0]; x++)
            {
            double* pixel = static_cast<double*>(imageData2->GetScalarPointer(x,y,z));
            pixel[0] = 2.0;
            }
          }
        }


      vtkSmartPointer<vtkImageDotProduct> dotProductFilter =
      vtkSmartPointer<vtkImageDotProduct>::New();

      dotProductFilter->SetInput1Data(imageData);
      dotProductFilter->SetInput2Data(imageData2);

      dotProductFilter->EnableSMP(parms.enableSMP);
      dotProductFilter->SetSMPBlocks(parms.numberOfSMPBlocks);
      dotProductFilter->SetSMPBlockMode(parms.enableSMPBlockMode);

      tl->StartTimer();
      dotProductFilter->Update();
      tl->StopTimer();

      cerr << "Wall Time = " << tl->GetElapsedTime() << "\n";
      WriteResultToCSV(tl->GetElapsedTime(),&parms);

      break;
    }
  //----Test Case 3: SMP overhead compared with old multi-threader
  case 3:
    {
      // read in additional parm for the image slice data
      if(argc <8)
      {
        cerr << "Additional Data not inputed into argument 7\n";
        break;
      }
      parms.additionalData = argv[7];
      double angle = 90;

      // Read file
      vtkSmartPointer<vtkImageReader2Factory> readerFactory =
        vtkSmartPointer<vtkImageReader2Factory>::New();

      vtkImageReader2 *reader =
        readerFactory->CreateImageReader2(parms.additionalData);
      reader->SetFileName(parms.additionalData);
      reader->Update();
      double bounds[6];
      reader->GetOutput()->GetBounds(bounds);

      // Rotate about the center of the image
      vtkSmartPointer<vtkTransform> transform =
        vtkSmartPointer<vtkTransform>::New();

      // Compute the center of the image
      double center[3];
      center[0] = (bounds[1] + bounds[0]) / 2.0;
      center[1] = (bounds[3] + bounds[2]) / 2.0;
      center[2] = (bounds[5] + bounds[3]) / 2.0;

      // Rotate about the center
      transform->Translate(center[0], center[1], center[2]);
      transform->RotateWXYZ(angle, 0, 0, 1);
      transform->Translate(-center[0], -center[1], -center[2]);

      // Reslice does all of the work
      vtkSmartPointer<vtkImageReslice> reslice =
      vtkSmartPointer<vtkImageReslice>::New();
      reslice->SetInputConnection(reader->GetOutputPort());
      reslice->SetResliceTransform(transform);
      reslice->SetInterpolationModeToCubic();
      reslice->SetOutputSpacing(
      reader->GetOutput()->GetSpacing()[0],
      reader->GetOutput()->GetSpacing()[1],
      reader->GetOutput()->GetSpacing()[2]);
      reslice->SetOutputOrigin(
      reader->GetOutput()->GetOrigin()[0],
      reader->GetOutput()->GetOrigin()[1],
      reader->GetOutput()->GetOrigin()[2]);
      reslice->SetOutputExtent(
      reader->GetOutput()->GetExtent());

      reslice->EnableSMP(parms.enableSMP);
      reslice->SetSMPBlocks(parms.numberOfSMPBlocks);
      reslice->SetSMPBlockMode(parms.enableSMPBlockMode);

      tl->StartTimer();
      reslice->Update();
      tl->StopTimer();

      cerr << "Wall Time = " << tl->GetElapsedTime() << "\n";
      WriteResultToCSV(tl->GetElapsedTime(),&parms);
      break;
    }
  default:
   {
     cerr << "No test case specified\n";
   }
   }


  tl->Delete();
  return EXIT_SUCCESS;
}
