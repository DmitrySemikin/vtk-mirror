#include <vtkSmartPointer.h>
#include <vtkImageConvolve.h>
#include <vtkImageMandelbrotSource.h>
#include <vtkTimerLog.h>

int main(int argc, char *argv[])
{
  if(argc <6)
  {
    printf ("Not all parms have being passed in\n"
        "parm#1: Number Of threads to run ~4. \n"
        "parm#2: Number of split SMP blocks ~500.\n"
        "parm#3: Number of test iterations.\n"
        "parm#4: The size of the work load ~5000\n"
        "parm#5: Enable/Disabl SMP, use true/false\n");
    return -1;
  }

  int numberOfThreadsToRun = atoi(argv[1]);
  int numberOfSMPBlocks = atoi(argv[2]);
  int testIterations = atoi(argv[3]);
  int workSize = atoi(argv[4]);
  bool enableSMP;
  if(strcmp(argv[5],"true")==0)
  {
    enableSMP = true;
  }
  else
  {
    enableSMP = false;
  }

  if(enableSMP)
  {
    printf("Running test for %d Number of threads"
          ", %d number Of SMP Blocks"
          ", %d number Of iterations"
          ",and %d number Of work size", numberOfThreadsToRun
          ,numberOfSMPBlocks,testIterations,workSize);
  }
  else
  {
    printf("Running test with SMP off.");
  }

  int workExtent[6] = {0,workSize,0,workSize,0,0};

  vtkSmartPointer<vtkImageMandelbrotSource> source =
    vtkSmartPointer<vtkImageMandelbrotSource>::New();
  source->SetWholeExtent(workExtent);
  source->Update();

  // Initilize with passed in number of threads
  vtkTimerLog *tl = vtkTimerLog::New();
  for(int i =0;i <testIterations;i++)
  {
    tl->StartTimer();


    vtkSmartPointer<vtkImageConvolve> convolveFilter =
    vtkSmartPointer<vtkImageConvolve>::New();
    convolveFilter->SetInputConnection(source->GetOutputPort());
    double kernel[9] = {1,1,1,1,1,1,1,1,1};
    convolveFilter->EnableSMP(enableSMP);
    convolveFilter->SetSMPBlocks(numberOfSMPBlocks);
    convolveFilter->SetSMPProcessorCount(numberOfThreadsToRun);
    convolveFilter->SetKernel3x3(kernel);
    convolveFilter->Update();

    tl->StopTimer();
    cerr << "Wall Time = " << tl->GetElapsedTime() << "\n";
  }
  //Clean Up
  tl->Delete();

  return EXIT_SUCCESS;
}
