/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkThreadedImageAlgorithm.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkThreadedImageAlgorithm.h"

#include "vtkCellData.h"
#include "vtkCommand.h"
#include "vtkDataArray.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiThreader.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkSMPTools.h"
#include "vtkExtentTranslator.h"


static bool GlobalEnableSMP = true;

static VTK_THREAD_RETURN_TYPE vtkThreadedImageAlgorithmThreadedExecute( void *arg );

struct vtkImageThreadStruct
{
  vtkThreadedImageAlgorithm *Filter;
  vtkInformation *Request;
  vtkInformationVector **InputsInfo;
  vtkInformationVector *OutputsInfo;
  vtkImageData   ***Inputs;
  vtkImageData   **Outputs;
};

class vtkThreadedImageAlgorithmFunctor
{
  vtkMultiThreader::ThreadInfo * ThreadInfo;
  int Ext[6];
  vtkSMPThreadLocal<vtkImageData * > ThreadLocalImageData;
  vtkThreadedImageAlgorithm * Algo;

public:
  vtkThreadedImageAlgorithmFunctor(vtkMultiThreader::ThreadInfo * info, int * ext, vtkThreadedImageAlgorithm *algo)
  {
    memcpy(Ext,ext,sizeof (int)*6);
    this->ThreadInfo = info;
    this->Algo = algo;
  }

  void Initialize()
  {
    this->Algo->SMPInit();
  }

  void Reduce()
  {
    this->Algo->SMPReduce();
  }

  VTK_THREAD_RETURN_TYPE Execute(int thread)
  {
    vtkMultiThreader::ThreadInfo *ti = this->ThreadInfo;
    vtkImageThreadStruct *str =
      static_cast<vtkImageThreadStruct *>(ti->UserData);

    int splitExt[6] = {0,-1,0,-1,0,-1};
    // execute the actual method with appropriate extent
    // first find out how many pieces extent can be split into.
    int total = str->Filter->SplitExtent(splitExt, this->Ext, thread, ti->NumberOfThreads);

    if (thread < total)
      {
      // return if nothing to do
      if (splitExt[1] < splitExt[0] ||
          splitExt[3] < splitExt[2] ||
          splitExt[5] < splitExt[4])
        {
        return VTK_THREAD_RETURN_VALUE;
        }
      str->Filter->ThreadedRequestData(str->Request,
                                       str->InputsInfo, str->OutputsInfo,
                                       str->Inputs, str->Outputs,
                                       splitExt, thread);
      }
  }

  void operator()(vtkIdType begin, vtkIdType end)
  {
    for (int i =begin; i < end; i++)
      {
      this->Execute(i);
      }
  }
};

//----------------------------------------------------------------------------
vtkThreadedImageAlgorithm::vtkThreadedImageAlgorithm()
{
  this->Threader = vtkMultiThreader::New();
  this->Translator = vtkExtentTranslator::New();
  this->NumberOfThreads = this->Threader->GetNumberOfThreads();

  //SMP default settings
  this->EnableSMP = true;
  this->SplitMode = vtkExtentTranslator::BLOCK_MODE;
  this->SMPSplitPercentage = 3.0;
  this->SplitByPoints = true;
  this->MinimumBlockSize[0] = 1;
  this->MinimumBlockSize[1] = 1;
  this->MinimumBlockSize[2] = 1;
}

//----------------------------------------------------------------------------
void vtkThreadedImageAlgorithm::SetSMPMinimumBlockSize(int * minBlockSizes)
{
  if(minBlockSizes[0]>0
    &&minBlockSizes[1]>0
    &&minBlockSizes[2]>0)
    {
    memcpy(this->MinimumBlockSize, minBlockSizes, 3 * sizeof(int));
    }
}

//----------------------------------------------------------------------------
vtkThreadedImageAlgorithm::~vtkThreadedImageAlgorithm()
{
  this->Threader->Delete();
  this->Translator->Delete();
}

void vtkThreadedImageAlgorithm::SetGlobalEnableSMP(bool enable)
{
  if (enable == GlobalEnableSMP)
    {
    return;
    }
  GlobalEnableSMP = enable;
}
bool vtkThreadedImageAlgorithm::GetGlobalEnableSMP()
{
  return GlobalEnableSMP;
}


//----------------------------------------------------------------------------
int * vtkThreadedImageAlgorithm::GetSMPMinimumBlockSize()
{
  return this->MinimumBlockSize;
}

//----------------------------------------------------------------------------
// Derived class will override these if they have custom ThreadLocal initlization/reduction
void vtkThreadedImageAlgorithm::SMPInit()
{
}
void vtkThreadedImageAlgorithm::SMPReduce()
{
}

//----------------------------------------------------------------------------
void vtkThreadedImageAlgorithm::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "NumberOfThreads: " << this->NumberOfThreads << "\n";
}

//----------------------------------------------------------------------------
// For streaming and threads.  Splits output update extent into num pieces.
// This method needs to be called num times.  Results must not overlap for
// consistent starting extent.  Subclass can override this method.
// This method returns the number of peices resulting from a successful split.
// This can be from 1 to "total".
// If 1 is returned, the extent cannot be split.
int vtkThreadedImageAlgorithm::SplitExtent(int splitExt[6],
                                           int startExt[6],
                                           int num, int total)
{
  int splitAxis;
  int min, max;

  if ((startExt[0] == 0 && startExt[1] == -1)
    || (startExt[2] == 0 && startExt[3] == -1)
    ||(startExt[4] == 0 && startExt[5] == -1) )
    {
    return -1;
    }

  int ret;
  if (this->EnableSMP && GlobalEnableSMP) // this is block mode splitting
    {
    ret = this->Translator->PieceToExtentThreadSafeImaging(num,total,0,startExt,splitExt);
    }
  else
    {
    // start with same extent
    memcpy(splitExt, startExt, 6 * sizeof(int));

    splitAxis = 2;
    min = startExt[4];
    max = startExt[5];
    while (min >= max)
      {
      // empty extent so cannot split
      if (min > max)
        {
        return 1;
        }
      --splitAxis;
      if (splitAxis < 0)
        { // cannot split
        vtkDebugMacro("  Cannot Split");
        return 1;
        }
      min = startExt[splitAxis*2];
      max = startExt[splitAxis*2+1];
      }

    // determine the actual number of pieces that will be generated
    int range = max - min + 1;
    int valuesPerThread = static_cast<int>(ceil(range/static_cast<double>(total)));
    int maxThreadIdUsed = static_cast<int>(ceil(range/static_cast<double>(valuesPerThread))) - 1;
    if (num < maxThreadIdUsed)
      {
      splitExt[splitAxis*2] = splitExt[splitAxis*2] + num*valuesPerThread;
      splitExt[splitAxis*2+1] = splitExt[splitAxis*2] + valuesPerThread - 1;
      }
    if (num == maxThreadIdUsed)
      {
      splitExt[splitAxis*2] = splitExt[splitAxis*2] + num*valuesPerThread;
      }

    vtkDebugMacro("  Split Piece: ( " <<splitExt[0]<< ", " <<splitExt[1]<< ", "
                  << splitExt[2] << ", " << splitExt[3] << ", "
                  << splitExt[4] << ", " << splitExt[5] << ")");

    return maxThreadIdUsed + 1;
    }
    if(ret == 1)//there is a return extent
      {
      return num+1;
      }
    else // there was no piece returned
      {
      return 0;
      }
}

// this mess is really a simple function. All it does is call
// the ThreadedExecute method after setting the correct
// extent for this thread. Its just a pain to calculate
// the correct extent.
static VTK_THREAD_RETURN_TYPE vtkThreadedImageAlgorithmThreadedExecute( void *arg )
{
  vtkImageThreadStruct *str;
  int ext[6], splitExt[6], total;
  int threadId, threadCount;

  threadId = static_cast<vtkMultiThreader::ThreadInfo *>(arg)->ThreadID;
  threadCount = static_cast<vtkMultiThreader::ThreadInfo *>(arg)->NumberOfThreads;

  str = static_cast<vtkImageThreadStruct *>
    (static_cast<vtkMultiThreader::ThreadInfo *>(arg)->UserData);

  // if we have an output
  if (str->Filter->GetNumberOfOutputPorts())
    {
    // which output port did the request come from
    int outputPort =
      str->Request->Get(vtkDemandDrivenPipeline::FROM_OUTPUT_PORT());

    // if output port is negative then that means this filter is calling the
    // update directly, for now an error
    if (outputPort == -1)
      {
      return VTK_THREAD_RETURN_VALUE;
      }

    // get the update extent from the output port
    vtkInformation *outInfo =
      str->OutputsInfo->GetInformationObject(outputPort);
    int updateExtent[6];
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
                 updateExtent);
    memcpy(ext,updateExtent, sizeof(int)*6);
    }
  else
    {
    // if there is no output, then use UE from input, use the first input
    int inPort;
    bool found = false;
    for (inPort = 0; inPort < str->Filter->GetNumberOfInputPorts(); ++inPort)
      {
      if (str->Filter->GetNumberOfInputConnections(inPort))
        {
        int updateExtent[6];
        str->InputsInfo[inPort]
          ->GetInformationObject(0)
          ->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
                updateExtent);
        memcpy(ext,updateExtent, sizeof(int)*6);
        found = true;
        break;
        }
      }
    if (!found)
      {
      return VTK_THREAD_RETURN_VALUE;
      }
    }

  // execute the actual method with appropriate extent
  // first find out how many pieces extent can be split into.
  total = str->Filter->SplitExtent(splitExt, ext, threadId, threadCount);

  if (threadId < total)
    {
    // return if nothing to do
    if (splitExt[1] < splitExt[0] ||
        splitExt[3] < splitExt[2] ||
        splitExt[5] < splitExt[4])
      {
      return VTK_THREAD_RETURN_VALUE;
      }
    str->Filter->ThreadedRequestData(str->Request,
                                     str->InputsInfo, str->OutputsInfo,
                                     str->Inputs, str->Outputs,
                                     splitExt, threadId);
    }
  // else
  //   {
  //   otherwise don't use this thread. Sometimes the threads dont
  //   break up very well and it is just as efficient to leave a
  //   few threads idle.
  //   }

  return VTK_THREAD_RETURN_VALUE;
}


//----------------------------------------------------------------------------
// This is the superclasses style of Execute method.  Convert it into
// an imaging style Execute method.
int vtkThreadedImageAlgorithm::RequestData(
  vtkInformation* request,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  int i;

  // setup the threasd structure
  vtkImageThreadStruct str;
  str.Filter = this;
  str.Request = request;
  str.InputsInfo = inputVector;
  str.OutputsInfo = outputVector;

  // now we must create the output array
  str.Outputs = 0;
  if (this->GetNumberOfOutputPorts())
    {
    str.Outputs = new vtkImageData * [this->GetNumberOfOutputPorts()];
    for (i = 0; i < this->GetNumberOfOutputPorts(); ++i)
      {
      vtkInformation* info = outputVector->GetInformationObject(i);
      vtkImageData *outData = vtkImageData::SafeDownCast(
        info->Get(vtkDataObject::DATA_OBJECT()));
      str.Outputs[i] = outData;
      if (outData)
        {
        int updateExtent[6];
        info->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
                  updateExtent);

        // unlike geometry filters, for image filters data is pre-allocated
        // in the superclass (which means, in this class)
        this->AllocateOutputData(outData, info, updateExtent);
        }
      }
    }

  // now create the inputs array
  str.Inputs = 0;
  if (this->GetNumberOfInputPorts())
    {
    str.Inputs = new vtkImageData ** [this->GetNumberOfInputPorts()];
    for (i = 0; i < this->GetNumberOfInputPorts(); ++i)
      {
      str.Inputs[i] = 0;
      vtkInformationVector* portInfo = inputVector[i];

      if (portInfo->GetNumberOfInformationObjects())
        {
        int j;
        str.Inputs[i] =
          new vtkImageData *[portInfo->GetNumberOfInformationObjects()];
        for (j = 0; j < portInfo->GetNumberOfInformationObjects(); ++j)
          {
          vtkInformation* info = portInfo->GetInformationObject(j);
          str.Inputs[i][j] = vtkImageData::SafeDownCast(
            info->Get(vtkDataObject::DATA_OBJECT()));
          }
        }
      }
    }

  // copy other arrays
  if (str.Inputs && str.Inputs[0] && str.Outputs)
    {
    this->CopyAttributeData(str.Inputs[0][0],str.Outputs[0],inputVector);
    }

  if (this->EnableSMP && GlobalEnableSMP)
    {
    // verify that the request number of blocks is valid
    int updateExtent[6];
    if (str.Filter->GetNumberOfOutputPorts())
      {
      int outputPort =
        str.Request->Get(vtkDemandDrivenPipeline::FROM_OUTPUT_PORT());
      vtkInformation *outInfo =
      str.OutputsInfo->GetInformationObject(outputPort);

      outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
                 updateExtent);
      }
    else
      {
      for (int inPort = 0; inPort < str.Filter->GetNumberOfInputPorts(); ++inPort)
        {
        if (str.Filter->GetNumberOfInputConnections(inPort))
          {
          str.InputsInfo[inPort]
            ->GetInformationObject(0)
            ->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
                  updateExtent);
          }
        }
      }
    if (updateExtent[1] < updateExtent[0] ||
        updateExtent[3] < updateExtent[2] ||
        updateExtent[5] < updateExtent[4])
      {
      return -1;
      }

    int blocks = this->Translator->SetUpExtent(updateExtent,this->SplitMode
                                              ,this->SMPSplitPercentage
                                              ,this->SplitByPoints
                                              ,this->MinimumBlockSize[0]
                                              ,this->MinimumBlockSize[1]
                                              ,this->MinimumBlockSize[2]);

    this->NumberOfThreads = blocks;
    bool debug = this->Debug;
    this->Debug = false;

    vtkMultiThreader::ThreadInfo threadInfo;
    threadInfo.NumberOfThreads = blocks;
    threadInfo.UserData = &str;
    threadInfo.ThreadID = -1;

    vtkThreadedImageAlgorithmFunctor functor(&threadInfo,updateExtent,this);
    vtkSMPTools::For(0, blocks, functor);

    this->Debug = debug;
    }
  else
    {
    this->Threader->SetNumberOfThreads(this->NumberOfThreads);
    this->Threader->SetSingleMethod(vtkThreadedImageAlgorithmThreadedExecute, &str);
    // always shut off debugging to avoid threading problems with GetMacros
    bool debug = this->Debug;
    this->Debug = false;
    this->Threader->SingleMethodExecute();
    this->Debug = debug;
    }

  // free up the arrays
  for (i = 0; i < this->GetNumberOfInputPorts(); ++i)
    {
    delete [] str.Inputs[i];
    }
  delete [] str.Inputs;
  delete [] str.Outputs;

  return 1;
}

//----------------------------------------------------------------------------
// The execute method created by the subclass.
void vtkThreadedImageAlgorithm::ThreadedRequestData(
  vtkInformation* vtkNotUsed( request ),
  vtkInformationVector** vtkNotUsed( inputVector ),
  vtkInformationVector* vtkNotUsed( outputVector ),
  vtkImageData ***inData,
  vtkImageData **outData,
  int extent[6],
  int threadId)
{
  this->ThreadedExecute(inData[0][0], outData[0], extent, threadId);
}

//----------------------------------------------------------------------------
// The execute method created by the subclass.
void vtkThreadedImageAlgorithm::ThreadedExecute(
  vtkImageData * inData,
  vtkImageData * outData,
  int extent[6],
  int threadId)
{
  (void)inData;
  (void)outData;
  (void)extent;
  (void)threadId;
  vtkErrorMacro("Subclass should override this method!!!");
}

