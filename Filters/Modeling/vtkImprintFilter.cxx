/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImprintFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImprintFilter.h"

#include "vtkArrayDispatch.h"
#include "vtkBoundingBox.h"
#include "vtkExecutive.h"
#include "vtkGenericCell.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkSMPThreadLocal.h"
#include "vtkSMPTools.h"
#include "vtkStaticCellLocator.h"
#include "vtkStaticPointLocator.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkStandardNewMacro(vtkImprintFilter);


//------------------------------------------------------------------------------
// Instantiate object
vtkImprintFilter::vtkImprintFilter()
{
  this->Tolerance = 0.001;
  this->CellLocator = vtkSmartPointer<vtkStaticCellLocator>::New();
  this->PointLocator = vtkSmartPointer<vtkStaticPointLocator>::New();
  this->SetNumberOfInputPorts(2);
}

//------------------------------------------------------------------------------
vtkImprintFilter::~vtkImprintFilter()
{
}

//------------------------------------------------------------------------------
void vtkImprintFilter::SetTargetConnection(vtkAlgorithmOutput* algOutput)
{
  this->SetInputConnection(0, algOutput);
}

//------------------------------------------------------------------------------
vtkAlgorithmOutput* vtkImprintFilter::GetTargetConnection()
{
  return this->GetInputConnection(0, 0);
}

//------------------------------------------------------------------------------
void vtkImprintFilter::SetTargetData(vtkDataObject* input)
{
  this->SetInputData(0, input);
}

//------------------------------------------------------------------------------
vtkDataObject* vtkImprintFilter::GetTarget()
{
  if (this->GetNumberOfInputConnections(0) < 1)
  {
    return nullptr;
  }

  return this->GetExecutive()->GetInputData(0, 0);
}

//------------------------------------------------------------------------------
void vtkImprintFilter::SetImprintConnection(vtkAlgorithmOutput* algOutput)
{
  this->SetInputConnection(1, algOutput);
}

//------------------------------------------------------------------------------
vtkAlgorithmOutput* vtkImprintFilter::GetImprintConnection()
{
  return this->GetInputConnection(1, 0);
}

//------------------------------------------------------------------------------
void vtkImprintFilter::SetImprintData(vtkDataObject* input)
{
  this->SetInputData(1, input);
}

//------------------------------------------------------------------------------
vtkDataObject* vtkImprintFilter::GetImprint()
{
  if (this->GetNumberOfInputConnections(1) < 1)
  {
    return nullptr;
  }

  return this->GetExecutive()->GetInputData(1, 0);
}

namespace {

  enum PtClass
  {
    Unknown = 0,
    Interior = 1,
    OnVertex = 2,
    OnEdge = 3,
    EdgeIntersection = 4
  };

  // Struct retains information relative to the projection
  // of the imprint points onto the target.
  struct vtkImprintMap
  {
    vtkIdType CellId; //Which cell does this point project to? <0 if misses target
    vtkIdType PtMap; //Which point does this map to? (might be a target point)
    double X[3]; //Projection coordinates
    double PC[3]; //Parametric coordinates
    double T; //Perimeter coordinate
    char Classification; //Type of point
  };

  // Project points onto the target
  template <typename DataT>
  struct ProjPoints
  {
    DataT *Pts;
    vtkStaticCellLocator *CellLocator;
    vtkStaticPointLocator *PointLocator;
    double Tol;
    vtkImprintMap *ImprintMap;
    vtkSMPThreadLocal<vtkSmartPointer<vtkGenericCell>> Cell;

    ProjPoints(DataT *pts, vtkStaticCellLocator *cellLoc, vtkStaticPointLocator *ptLoc,
               double tol, vtkImprintMap *iMap) :
      Pts(pts), CellLocator(cellLoc), PointLocator(ptLoc), Tol(tol), ImprintMap(iMap) {}

    void Initialize()
    {
      this->Cell.Local().TakeReference(vtkGenericCell::New());
    }

    void operator()(vtkIdType ptId, vtkIdType endPtId)
    {
      auto& cell = this->Cell.Local();
      const auto pts = vtk::DataArrayTupleRange<3>(this->Pts);
      vtkStaticCellLocator *cellLoc=this->CellLocator;
      vtkStaticPointLocator *ptLoc=this->PointLocator;
      vtkIdType cellId, closestPt;
      int subId, inside;
      double x[3], dist2, closest[3], tol=this->Tol;
      vtkImprintMap *iMap = this->ImprintMap + ptId;

      for ( ; ptId < endPtId; ptId++, iMap++)
      {
        auto xTuple = pts[ptId];
        x[0] = xTuple[0];
        x[1] = xTuple[1];
        x[2] = xTuple[2];

        // See if the point projects onto the target
        if ( ! cellLoc->FindClosestPointWithinRadius(x,tol,closest,cell,cellId,subId,dist2,inside) )
        {
          iMap[ptId].CellId = (-1);
          iMap[ptId].PtMap = ptId;
          iMap[ptId].Classification = PtClass::Unknown;
          // Default constructor takes care of the rest
        }
        else //The point projects onto the target. See if it hits a vertex or edge.
        {
          iMap[ptId].CellId = cellId;
          iMap[ptId].X[0] = closest[0]; iMap[ptId].X[1] = closest[1]; iMap[ptId].X[2] = closest[2];
          if ( (closestPt = ptLoc->FindClosestPointWithinRadius(tol,x,dist2)) >= 0 )
            {//on vertex
            iMap[ptId].PtMap = closestPt;
            iMap[ptId].Classification = PtClass::OnVertex;
            continue;
          }

          // See if an edge is hit
        }
      }
    }

    void Reduce() {}
  };

  struct ProjPointsWorker
  {
    template <typename DataT>
    void operator()(DataT* pts, vtkStaticCellLocator *cellLoc,
                    vtkStaticPointLocator *ptLoc, double tol,
                    vtkImprintMap *iMap)
    {
      vtkIdType numPts = pts->GetNumberOfTuples();

      ProjPoints<DataT> pp(pts, cellLoc, ptLoc, tol, iMap);

      vtkSMPTools::For(0,numPts, pp);
    }
  };

} //anonymous

//------------------------------------------------------------------------------
int vtkImprintFilter::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* targetInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* imprintInfo = inputVector[1]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkPolyData* target = vtkPolyData::SafeDownCast(targetInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData* imprint = vtkPolyData::SafeDownCast(imprintInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData* output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Initialize and check data
  vtkDebugMacro(<< "Imprinting...");

  vtkIdType numTargetPts = target->GetNumberOfPoints();
  vtkIdType numTargetCells = target->GetPolys()->GetNumberOfCells();
  if ( numTargetPts < 1 || numTargetCells < 1 )
  {
    vtkErrorMacro("Target is empty");
    return 1;
  }
  vtkPoints *targetPts = target->GetPoints();
  vtkCellArray *targetCells = target->GetPolys();

  if ( !imprint )
  {
    vtkErrorMacro("Imprint is empty");
    return 1;
  }
  vtkIdType numImprintPts = imprint->GetNumberOfPoints();
  vtkIdType numImprintCells = imprint->GetNumberOfCells();
  if ( numImprintPts < 1 || numImprintCells < 1 )
  {
    vtkErrorMacro("Please define an imprint");
    return 1;
  }
  vtkPoints *imprintPts = imprint->GetPoints();
  vtkCellArray *imprintCells = imprint->GetPolys();

  // Begin by separating out the target cells that may be imprinted (the
  // "imprint" cells), from those that won't be (the "kept" cells). Also copy
  // the target points.  This creates two outputs: 1) the actual filter
  // output - initially it contains the input target points and the kept
  // cells; and 2) the imprint cells that are operated on. Eventually, the
  // imprint cells and any newly generated points are appended to the output
  // #1. This is done to improve performance.
  vtkNew<vtkPoints> outPts;
  outPts->SetDataType(targetPts->GetDataType());
  outPts->SetNumberOfPoints(numTargetPts); //expanded later
  for ( auto i=0; i<numTargetPts; ++i)
  {
    outPts->SetPoint(i,targetPts->GetPoint(i));
  }
  output->SetPoints(outPts);
  vtkNew<vtkPolyData> imprintOutput;
  imprintOutput->SetPoints(outPts);

  // Now separate out the kept cells from the imprint cells. We are throwing
  // out any non-polygon cells.
  output->AllocateEstimate(numTargetCells,3);
  imprintOutput->AllocateEstimate(numImprintCells,3);
  vtkBoundingBox targetBounds;
  vtkBoundingBox imprintBounds;
  double imprintBds[6], targetCellBounds[6];
  imprint->GetBounds(imprintBds);
  imprintBounds.SetBounds(imprintBds);
  imprintBounds.Inflate(this->Tolerance);
  vtkIdType npts;
  const vtkIdType *pts;
  for ( auto i=0; i < numTargetCells; ++i )
  {
    int cellType = target->GetCellType(i);
    if ( cellType == VTK_TRIANGLE || cellType == VTK_QUAD ||
         cellType == VTK_POLYGON )
    {
      target->GetCellBounds(i,targetCellBounds);
      targetBounds.SetBounds(targetCellBounds);
      target->GetCellPoints(i,npts,pts);
      if ( ! targetBounds.Intersects(imprintBounds) )
      {
        // This cell is kept
        output->InsertNextCell(cellType,npts,pts);
      }
      else
      {
        // Otherwise this gets shunted to the imprint output for further
        // processing.
        imprintOutput->InsertNextCell(cellType,npts,pts);
      }
    }
  }

  // Build some locators to project imprint points onto the target, where the target is
  // a subset of the original input target.
  this->CellLocator->SetDataSet(imprintOutput);
  this->CellLocator->BuildLocator();
  this->PointLocator->SetDataSet(imprintOutput);
  this->PointLocator->BuildLocator();

  // Now project all points onto target
  vtkImprintMap *iMap = new vtkImprintMap[numImprintPts];
  using ProjPointsDispatch = vtkArrayDispatch::DispatchByValueType<vtkArrayDispatch::Reals>;
  ProjPointsWorker ppWorker;
  if ( !ProjPointsDispatch::Execute(imprintPts->GetData(), ppWorker, this->CellLocator,
                                    this->PointLocator, this->Tolerance, iMap) )
  {
    ppWorker(imprintPts->GetData(), this->CellLocator, this->PointLocator,
             this->Tolerance, iMap);
  }

  // Imprint points that successfully project are inserted into the output points.


  return 1;
}

//------------------------------------------------------------------------------
int vtkImprintFilter::RequestUpdateExtent(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* targetInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* imprintInfo = inputVector[1]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  if (imprintInfo)
  {
    imprintInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), 0);
    imprintInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), 1);
    imprintInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), 0);
  }
  targetInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(),
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()));
  targetInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()));
  targetInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS()));
  targetInfo->Set(vtkStreamingDemandDrivenPipeline::EXACT_EXTENT(), 1);

  return 1;
}

//------------------------------------------------------------------------------
int vtkImprintFilter::FillInputPortInformation(int port, vtkInformation* info)
{
  if (port == 0)
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPolyData");
    return 1;
  }
  else if (port == 1)
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPolyData");
    return 1;
  }
  return 0;
}

//------------------------------------------------------------------------------
void vtkImprintFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Tolerance: " << this->Tolerance << "\n";
}
