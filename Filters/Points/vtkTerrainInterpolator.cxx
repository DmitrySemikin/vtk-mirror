/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTerrainInterpolator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTerrainInterpolator.h"

#include "vtkObjectFactory.h"
#include "vtkVoronoiKernel.h"
#include "vtkAbstractPointLocator.h"
#include "vtkStaticPointLocator.h"
#include "vtkDataSet.h"
#include "vtkDataArray.h"
#include "vtkImageData.h"
#include "vtkPolyData.h"
#include "vtkPoints.h"
#include "vtkCharArray.h"
#include "vtkDoubleArray.h"
#include "vtkCellData.h"
#include "vtkPointData.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkMath.h"
#include "vtkSMPTools.h"
#include "vtkSMPThreadLocalObject.h"

vtkStandardNewMacro(vtkTerrainInterpolator);
vtkCxxSetObjectMacro(vtkTerrainInterpolator,Locator,vtkAbstractPointLocator);
vtkCxxSetObjectMacro(vtkTerrainInterpolator,Kernel,vtkInterpolationKernel);

//----------------------------------------------------------------------------
// Helper classes to support efficient computing, and threaded execution.
namespace {

#include "vtkArrayListTemplate.h"

// Project points onto plane
struct ProjectPoints
{
  vtkDataSet *Input;
  double *OutPoints;

  ProjectPoints(vtkDataSet *input, double *outPts) :
    Input(input), OutPoints(outPts)
    {
    }

  // Threaded projection
  void operator() (vtkIdType ptId, vtkIdType endPtId)
    {
      double *p = this->OutPoints + 3*ptId;
      double x[3];
      for ( ; ptId < endPtId; ++ptId)
        {
        this->Input->GetPoint(ptId,x);
        *p++ = x[0];
        *p++ = x[1];
        *p++ = 0.0; //x-y projection
        }
    }
};

// The threaded core of the algorithm
struct ProbePoints
{
  vtkDataSet *Input;
  vtkInterpolationKernel *Kernel;
  vtkAbstractPointLocator *Locator;
  vtkPointData *InPD;
  vtkPointData *OutPD;
  ArrayList Arrays;
  char *Valid;
  int Strategy;

  // Don't want to allocate these working arrays on every thread invocation,
  // so make them thread local.
  vtkSMPThreadLocalObject<vtkIdList> PIds;
  vtkSMPThreadLocalObject<vtkDoubleArray> Weights;

  ProbePoints(vtkDataSet *input, vtkInterpolationKernel *kernel,vtkAbstractPointLocator *loc,
              vtkPointData *inPD, vtkPointData *outPD, int strategy, char *valid, double nullV) :
    Input(input), Kernel(kernel), Locator(loc), InPD(inPD), OutPD(outPD),
    Valid(valid), Strategy(strategy)
    {
      this->Arrays.AddArrays(input->GetNumberOfPoints(), inPD, outPD, nullV);
    }

  // Just allocate a little bit of memory to get started.
  void Initialize()
    {
    vtkIdList*& pIds = this->PIds.Local();
    pIds->Allocate(128); //allocate some memory
    vtkDoubleArray*& weights = this->Weights.Local();
    weights->Allocate(128);
    }

  // When null point is encountered
  void AssignNullPoint(const double x[3], vtkIdList *pIds,
                       vtkDoubleArray *weights, vtkIdType ptId)
    {
      if ( this->Strategy == vtkTerrainInterpolator::MASK_POINTS)
        {
        this->Valid[ptId] = 0;
        this->Arrays.AssignNullValue(ptId);
        }
      else if ( this->Strategy == vtkTerrainInterpolator::NULL_VALUE)
        {
        this->Arrays.AssignNullValue(ptId);
        }
      else //vtkTerrainInterpolator::CLOSEST_POINT:
        {
        pIds->SetNumberOfIds(1);
        vtkIdType pId = this->Locator->FindClosestPoint(x);
        pIds->SetId(0,pId);
        weights->SetNumberOfTuples(1);
        weights->SetValue(0,1.0);
        this->Arrays.Interpolate(1, pIds->GetPointer(0),
                                 weights->GetPointer(0), ptId);
        }
    }

  // Threaded interpolation method
  void operator() (vtkIdType ptId, vtkIdType endPtId)
    {
      double x[3];
      vtkIdList*& pIds = this->PIds.Local();
      vtkIdType numWeights;
      vtkDoubleArray*& weights = this->Weights.Local();

      for ( ; ptId < endPtId; ++ptId)
        {
        this->Input->GetPoint(ptId,x);
        x[2] = 0.0; //x-y projection

        if ( this->Kernel->ComputeBasis(x, pIds) > 0 )
          {
          numWeights = this->Kernel->ComputeWeights(x, pIds, weights);
          this->Arrays.Interpolate(numWeights, pIds->GetPointer(0),
                                   weights->GetPointer(0), ptId);
          }
        else
          {
          this->AssignNullPoint(x, pIds, weights, ptId);
          }// null point
        }//for all dataset points
    }

  void Reduce()
    {
    }

}; //ProbePoints


} //anonymous namespace

//================= Begin class proper =======================================
//----------------------------------------------------------------------------
vtkTerrainInterpolator::vtkTerrainInterpolator()
{
  this->SetNumberOfInputPorts(2);

  this->Locator = vtkStaticPointLocator::New();

  this->Kernel = vtkVoronoiKernel::New();

  this->NullPointsStrategy = vtkTerrainInterpolator::CLOSEST_POINT;
  this->NullValue = 0.0;

  this->ValidPointsMask = NULL;
  this->ValidPointsMaskArrayName = 0;
  this->SetValidPointsMaskArrayName("vtkValidPointMask");

  this->PassPointArrays = true;
  this->PassCellArrays = true;
  this->PassFieldArrays = true;
}

//----------------------------------------------------------------------------
vtkTerrainInterpolator::~vtkTerrainInterpolator()
{
  this->SetLocator(NULL);
  this->SetKernel(NULL);
}

//----------------------------------------------------------------------------
void vtkTerrainInterpolator::SetSourceConnection(vtkAlgorithmOutput* algOutput)
{
  this->SetInputConnection(1, algOutput);
}

//----------------------------------------------------------------------------
void vtkTerrainInterpolator::SetSourceData(vtkDataObject *input)
{
  this->SetInputData(1, input);
}

//----------------------------------------------------------------------------
vtkDataObject *vtkTerrainInterpolator::GetSource()
{
  if (this->GetNumberOfInputConnections(1) < 1)
    {
    return NULL;
    }

  return this->GetExecutive()->GetInputData(1, 0);
}

//----------------------------------------------------------------------------
// The driver of the algorithm
void vtkTerrainInterpolator::
Probe(vtkDataSet *input, vtkDataSet *source, vtkDataSet *output)
{
  // Make sure there is a kernel
  if ( !this->Kernel )
    {
    vtkErrorMacro(<<"Interpolation kernel required\n");
    return;
    }

  // Start by building the locator
  if ( !this->Locator )
    {
    vtkErrorMacro(<<"Point locator required\n");
    return;
    }

  // We need to project the source points to the z=0.0 plane
  vtkIdType numSourcePts = source->GetNumberOfPoints();
  vtkPolyData *projSource = vtkPolyData::New();
  projSource->ShallowCopy(source);
  vtkPoints *projPoints = vtkPoints::New();
  projPoints->SetDataTypeToDouble();
  projPoints->SetNumberOfPoints(numSourcePts);
  projSource->SetPoints(projPoints);
  projPoints->UnRegister(this);

  ProjectPoints project(source,static_cast<double*>(projPoints->GetVoidPointer(0)));
  vtkSMPTools::For(0, numSourcePts, project);

  this->Locator->SetDataSet(projSource);
  this->Locator->BuildLocator();

  // Set up the interpolation process
  vtkIdType numPts = input->GetNumberOfPoints();
  vtkPointData *inPD = source->GetPointData();
  vtkPointData *outPD = output->GetPointData();
  outPD->InterpolateAllocate(inPD,numPts);

  // Masking if requested
  char *mask=NULL;
  if ( this->NullPointsStrategy == vtkTerrainInterpolator::MASK_POINTS )
    {
    this->ValidPointsMask = vtkCharArray::New();
    this->ValidPointsMask->SetNumberOfTuples(numPts);
    mask = this->ValidPointsMask->GetPointer(0);
    std::fill_n(mask, numPts, 1);
    }

  // Now loop over input points, finding closest points and invoking kernel.
  if ( this->Kernel->GetRequiresInitialization() )
    {
    this->Kernel->Initialize(this->Locator, source, inPD);
    }

  // If the input is image data then there is a faster path
  ProbePoints probe(input,this->Kernel,this->Locator,inPD,outPD,
                    this->NullPointsStrategy,mask,this->NullValue);
  vtkSMPTools::For(0, numPts, probe);

  // Clean up
  if ( mask )
    {
    this->ValidPointsMask->SetName(this->ValidPointsMaskArrayName);
    outPD->AddArray(this->ValidPointsMask);
    this->ValidPointsMask->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkTerrainInterpolator::
PassAttributeData(vtkDataSet* input, vtkDataObject* vtkNotUsed(source),
                  vtkDataSet* output)
{
  // copy point data arrays
  if (this->PassPointArrays)
    {
    int numPtArrays = input->GetPointData()->GetNumberOfArrays();
    for (int i=0; i<numPtArrays; ++i)
      {
      output->GetPointData()->AddArray(input->GetPointData()->GetArray(i));
      }
    }

  // copy cell data arrays
  if (this->PassCellArrays)
    {
    int numCellArrays = input->GetCellData()->GetNumberOfArrays();
    for (int i=0; i<numCellArrays; ++i)
      {
      output->GetCellData()->AddArray(input->GetCellData()->GetArray(i));
      }
    }

  if (this->PassFieldArrays)
    {
    // nothing to do, vtkDemandDrivenPipeline takes care of that.
    }
  else
    {
    output->GetFieldData()->Initialize();
    }
}

//----------------------------------------------------------------------------
int vtkTerrainInterpolator::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *sourceInfo = inputVector[1]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkDataSet *input = vtkDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkDataSet *source = vtkDataSet::SafeDownCast(
    sourceInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkDataSet *output = vtkDataSet::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  if (!source)
    {
    return 0;
    }

  // Copy the input geometry and topology to the output
  output->CopyStructure(input);

  // Perform the probing
  this->Probe(input, source, output);

  // Pass attribute data as requested
  this->PassAttributeData(input, source, output);

  return 1;
}

//----------------------------------------------------------------------------
int vtkTerrainInterpolator::RequestInformation(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *sourceInfo = inputVector[1]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  outInfo->CopyEntry(sourceInfo,
                     vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  outInfo->CopyEntry(sourceInfo,
                     vtkStreamingDemandDrivenPipeline::TIME_RANGE());

  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
               inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()),
               6);

  // Make sure that the scalar type and number of components
  // are propagated from the source not the input.
  if (vtkImageData::HasScalarType(sourceInfo))
    {
    vtkImageData::SetScalarType(vtkImageData::GetScalarType(sourceInfo),
                                outInfo);
    }
  if (vtkImageData::HasNumberOfScalarComponents(sourceInfo))
    {
    vtkImageData::SetNumberOfScalarComponents(
      vtkImageData::GetNumberOfScalarComponents(sourceInfo),
      outInfo);
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkTerrainInterpolator::RequestUpdateExtent(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *sourceInfo = inputVector[1]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), 0);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), 1);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), 0);
  sourceInfo->Set(
    vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(),
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()));
  sourceInfo->Set(
    vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()));
  sourceInfo->Set(
    vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS()));
  sourceInfo->Set(
    vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
    sourceInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()),
    6);

  return 1;
}

//----------------------------------------------------------------------------
void vtkTerrainInterpolator::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataObject *source = this->GetSource();

  this->Superclass::PrintSelf(os,indent);
  os << indent << "Source: " << source << "\n";
  os << indent << "Locator: " << this->Locator << "\n";
  os << indent << "Kernel: " << this->Kernel << "\n";
  os << indent << "Null Points Strategy: " << this->NullPointsStrategy << endl;
  os << indent << "Null Value: " << this->NullValue << "\n";
  os << indent << "Valid Points Mask Array Name: "
     << (this->ValidPointsMaskArrayName ? this->ValidPointsMaskArrayName : "(none)") << "\n";
  os << indent << "Pass Point Arrays: "
     << (this->PassPointArrays? "On" : " Off") << "\n";
  os << indent << "Pass Cell Arrays: "
     << (this->PassCellArrays? "On" : " Off") << "\n";
  os << indent << "Pass Field Arrays: "
     << (this->PassFieldArrays? "On" : " Off") << "\n";
}
