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
//Test 4

#include "vtkImageBSplineInterpolator.h"
#include "vtkImageBSplineCoefficients.h"
#include "vtkImageReslice.h"
#include "vtkBSplineTransform.h"
#include "vtkTransformToGrid.h"
#include "vtkThinPlateSplineTransform.h"
#include "vtkImageGridSource.h"
#include "vtkImageBlend.h"
#include "vtkImageMapToColors.h"
#include "vtkLookupTable.h"
#include "vtkPoints.h"
// Other
#include <vtkTransform.h>
#include <vtkImageConvolve.h>
#include <vtkImageMandelbrotSource.h>

struct TestParms
{
  int numberOfIterationsToRun;
  int testCase;
  bool enableSMP;
  bool enableSMPBlockMode;
  int numberOfThreadsToRun;
  int numberOfSMPBlocks;
  int workSize;
  char * additionalData;
  char * outputCSVFile;
};

int WriteResultToCSV(double executionTime,TestParms *parm)
{
  char writeOutput[100];
  sprintf(writeOutput, "%d,%d,%d,%d,%d,%d,%d,%f,%s\n"
  ,parm->numberOfIterationsToRun
  ,parm->testCase
  ,parm->enableSMP
  ,parm->enableSMPBlockMode
  ,parm->numberOfThreadsToRun
  ,parm->numberOfSMPBlocks
  ,parm->workSize
  ,executionTime
  ,parm->additionalData);

  FILE *f = fopen(parm->outputCSVFile, "a+");
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
  if(argc <9)
    {
    printf ("Not all parms have being passed in\n"
        "parm#1: NumberOfIterationsToRun.\n"
        "parm#2: Test Number.\n"
        "parm#3: Enable/Disabl SMP, use true/false\n"
        "parm#4: Enable block mode splitting, this is only valid if SMP is true. \n"
        "parm#5: Number Of threads to run ~4, this is only valid if SMP is true. \n"
        "parm#6: Number of split SMP blocks ~500.\n"
        "parm#7: The size of the work load ~5000\n"
        "parm#8: Output csv file\n"
        );
    return -1;
    }

  TestParms parms;
  parms.additionalData = " ";
  parms.outputCSVFile = "";

  parms.numberOfIterationsToRun =  atoi(argv[1]);
  parms.testCase = atoi(argv[2]);


  if(strcmp(argv[3],"true")==0)
    {
    parms.enableSMP =true;
    }
  else
    {
    parms.enableSMP =false;
    }
  if(strcmp(argv[4],"true")==0)
    {
    parms.enableSMPBlockMode = true;
    }
  else
    {
    parms.enableSMPBlockMode = false;
    }

  parms.numberOfThreadsToRun = atoi(argv[5]);
  parms.numberOfSMPBlocks = atoi(argv[6]);
  parms.workSize = atoi(argv[7]);
  parms.outputCSVFile = argv[8];

  // Initilize with passed in number of threads
  vtkTimerLog *tl = vtkTimerLog::New();
  vtkSMPTools::Initialize(parms.numberOfThreadsToRun);

  // printf("Running test case %d, with smp %d, blockmode %d, numberOfThreads %d\n"
  //   ,parms.testCase
  //   ,parms.enableSMP
  //   ,parms.enableSMPBlockMode
  //   ,parms.numberOfThreadsToRun);

  //---------------------------------------------------------------------
  //----Test Case 1: SMP overhead compared with old multi-threader
  switch(parms.testCase)
   {
    case 1:
    {
      float totalTime =0.0;
      for(int i=0;i<parms.numberOfIterationsToRun;i++)
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
          totalTime +=tl->GetElapsedTime();
        }

      WriteResultToCSV(totalTime/static_cast<float>(parms.numberOfIterationsToRun),&parms);
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
      if(argc <10)
      {
        cerr << "Additional Data not inputed into argument 8\n";
        break;
      }
      parms.additionalData = argv[9];
      double angle = 45;

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
  case 4:
    {
      // first, create an image that looks like
      // graph paper by combining two image grid
      // sources via vtkImageBlend
      vtkSmartPointer<vtkImageGridSource> imageGrid1 =
        vtkSmartPointer<vtkImageGridSource>::New();
      imageGrid1->SetGridSpacing(4,4,0);
      imageGrid1->SetGridOrigin(0,0,0);
      imageGrid1->SetDataExtent(0,255,0,255,0,0);
      imageGrid1->SetDataScalarTypeToUnsignedChar();

      vtkSmartPointer<vtkImageGridSource> imageGrid2 =
        vtkSmartPointer<vtkImageGridSource>::New();
      imageGrid2->SetGridSpacing(16,16,0);
      imageGrid2->SetGridOrigin(0,0,0);
      imageGrid2->SetDataExtent(0,255,0,255,0,0);
      imageGrid2->SetDataScalarTypeToUnsignedChar();

      vtkSmartPointer<vtkLookupTable> table1 =
        vtkSmartPointer<vtkLookupTable>::New();
      table1->SetTableRange(0,1);
      table1->SetValueRange(1.0,0.7);
      table1->SetSaturationRange(0.0,1.0);
      table1->SetHueRange(0.12,0.12);
      table1->SetAlphaRange(1.0,1.0);
      table1->Build();

      vtkSmartPointer<vtkLookupTable> table2 =
        vtkSmartPointer<vtkLookupTable>::New();
      table2->SetTableRange(0,1);
      table2->SetValueRange(1.0,0.0);
      table2->SetSaturationRange(0.0,0.0);
      table2->SetHueRange(0.0,0.0);
      table2->SetAlphaRange(0.0,1.0);
      table2->Build();

      vtkSmartPointer<vtkImageMapToColors> map1 =
        vtkSmartPointer<vtkImageMapToColors>::New();
      map1->SetInputConnection(imageGrid1->GetOutputPort());
      map1->SetLookupTable(table1);

      vtkSmartPointer<vtkImageMapToColors> map2 =
        vtkSmartPointer<vtkImageMapToColors>::New();
      map2->SetInputConnection(imageGrid2->GetOutputPort());
      map2->SetLookupTable(table2);

      vtkSmartPointer<vtkImageBlend> blend =
        vtkSmartPointer<vtkImageBlend>::New();
      blend->AddInputConnection(map1->GetOutputPort());
      blend->AddInputConnection(map2->GetOutputPort());

      // next, create a ThinPlateSpline transform, which
      // will then be used to create the B-spline transform
      vtkSmartPointer<vtkPoints> p1 =
        vtkSmartPointer<vtkPoints>::New();
      p1->SetNumberOfPoints(8);
      p1->SetPoint(0,0,0,0);
      p1->SetPoint(1,0,255,0);
      p1->SetPoint(2,255,0,0);
      p1->SetPoint(3,255,255,0);
      p1->SetPoint(4,96,96,0);
      p1->SetPoint(5,96,159,0);
      p1->SetPoint(6,159,159,0);
      p1->SetPoint(7,159,96,0);

      vtkSmartPointer<vtkPoints> p2 =
        vtkSmartPointer<vtkPoints>::New();
      p2->SetNumberOfPoints(8);
      p2->SetPoint(0,0,0,0);
      p2->SetPoint(1,0,255,0);
      p2->SetPoint(2,255,0,0);
      p2->SetPoint(3,255,255,0);
      p2->SetPoint(4,96,159,0);
      p2->SetPoint(5,159,159,0);
      p2->SetPoint(6,159,96,0);
      p2->SetPoint(7,96,96,0);

      vtkSmartPointer<vtkThinPlateSplineTransform> thinPlate =
        vtkSmartPointer<vtkThinPlateSplineTransform>::New();
      thinPlate->SetSourceLandmarks(p2);
      thinPlate->SetTargetLandmarks(p1);
      thinPlate->SetBasisToR2LogR();

      // convert the thin plate spline into a B-spline, by
      // sampling it onto a grid and then computing the
      // B-spline coefficients
      vtkSmartPointer<vtkTransformToGrid> transformToGrid =
        vtkSmartPointer<vtkTransformToGrid>::New();
      transformToGrid->SetInput(thinPlate);
      transformToGrid->SetGridSpacing(16.0, 16.0, 1.0);
      transformToGrid->SetGridOrigin(0.0, 0.0, 0.0);
      transformToGrid->SetGridExtent(0,16, 0,16, 0,0);

      vtkSmartPointer<vtkImageBSplineCoefficients> grid =
        vtkSmartPointer<vtkImageBSplineCoefficients>::New();
      grid->SetInputConnection(transformToGrid->GetOutputPort());
      grid->UpdateWholeExtent();

      // create the B-spline transform, scale the deformation by
      // half to demonstrate how deformation scaling works
      vtkSmartPointer<vtkBSplineTransform> transform =
        vtkSmartPointer<vtkBSplineTransform>::New();
      transform->SetCoefficientData(grid->GetOutput());
      transform->SetDisplacementScale(0.5);
      transform->SetBorderModeToZero();

      // invert the transform before passing it to vtkImageReslice
      transform->Inverse();

      // reslice the image through the B-spline transform,
      // using B-spline interpolation and the "Repeat"
      // boundary condition
      vtkSmartPointer<vtkImageBSplineCoefficients> prefilter =
        vtkSmartPointer<vtkImageBSplineCoefficients>::New();
      prefilter->SetInputConnection(blend->GetOutputPort());
      prefilter->SetBorderModeToRepeat();
      prefilter->SetSplineDegree(3);

      vtkSmartPointer<vtkImageBSplineInterpolator> interpolator =
        vtkSmartPointer<vtkImageBSplineInterpolator>::New();
      interpolator->SetSplineDegree(3);

      vtkSmartPointer<vtkImageReslice> reslice =
        vtkSmartPointer<vtkImageReslice>::New();
      reslice->SetInputConnection(prefilter->GetOutputPort());
      reslice->SetResliceTransform(transform);
      reslice->WrapOn();
      reslice->SetInterpolator(interpolator);
      reslice->SetOutputSpacing(1.0, 1.0, 1.0);
      reslice->SetOutputOrigin(-32.0, -32.0, 0.0);
      reslice->SetOutputExtent(0, 319, 0, 319, 0, 0);


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
