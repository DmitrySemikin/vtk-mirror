/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkThreadedSynchronizedTemplatesCutter3D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkThreadedSynchronizedTemplatesCutter3D.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCharArray.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkIdList.h"
#include "vtkIdListCollection.h"
#include "vtkImplicitFunction.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkLongArray.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolygonBuilder.h"
#include "vtkShortArray.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStructuredPoints.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnsignedIntArray.h"
#include "vtkUnsignedLongArray.h"
#include "vtkUnsignedShortArray.h"

#include <cmath>

#include "vtkExtentTranslator.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkPlane.h"
#include "vtkSMPThreadLocalObject.h"
#include "vtkSMPTools.h"

vtkStandardNewMacro(vtkThreadedSynchronizedTemplatesCutter3D);
vtkCxxSetObjectMacro(vtkThreadedSynchronizedTemplatesCutter3D,CutFunction,vtkImplicitFunction);

//----------------------------------------------------------------------------
// Description:
// Construct object with initial scalar range (0,1) and single contour value
// of 0.0. The ImageRange are set to extract the first k-plane.
vtkThreadedSynchronizedTemplatesCutter3D::vtkThreadedSynchronizedTemplatesCutter3D()
{
  VTK_LEGACY_BODY(
    vtkThreadedSynchronizedTemplatesCutter3D::vtkThreadedSynchronizedTemplatesCutter3D,
    "VTK 8.1");

  this->CutFunction = nullptr;
  this->OutputPointsPrecision = vtkAlgorithm::DEFAULT_PRECISION;
}

//----------------------------------------------------------------------------
vtkThreadedSynchronizedTemplatesCutter3D::~vtkThreadedSynchronizedTemplatesCutter3D()
{
  this->SetCutFunction(nullptr);
}

//----------------------------------------------------------------------------
// Overload standard modified time function. If cut functions is modified,
// or contour values modified, then this object is modified as well.
vtkMTimeType vtkThreadedSynchronizedTemplatesCutter3D::GetMTime()
{
  vtkMTimeType mTime=this->Superclass::GetMTime();

  if ( this->CutFunction != nullptr )
  {
    vtkMTimeType time = this->CutFunction->GetMTime();
    mTime = ( time > mTime ? time : mTime );
  }

  return mTime;
}

//----------------------------------------------------------------------------
int vtkThreadedSynchronizedTemplatesCutter3D::FillOutputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkMultiBlockDataSet");
  return 1;
}

//----------------------------------------------------------------------------
static void vtkThreadedSynchronizedTemplatesCutter3DInitializeOutput(
  int *ext, int precision, int estimatedNumberOfPieces, vtkImageData *input,
  vtkPolyData *o)
{
  vtkPoints *newPts;
  vtkCellArray *newPolys;
  long estimatedSize;

  estimatedSize = (int) pow ((double)
      ((ext[1]-ext[0]+1)*(ext[3]-ext[2]+1)*(ext[5]-ext[4]+1)), .75);
  if (estimatedSize < 1024)
  {
    estimatedSize = 1024;
  }
  estimatedSize /= estimatedNumberOfPieces;

  newPts = vtkPoints::New();

  // set precision for the points in the output
  if(precision == vtkAlgorithm::DEFAULT_PRECISION)
  {
    vtkPointSet *inputPointSet = vtkPointSet::SafeDownCast(input);
    if(inputPointSet)
    {
      newPts->SetDataType(inputPointSet->GetPoints()->GetDataType());
    }
    else
    {
      newPts->SetDataType(VTK_FLOAT);
    }
  }
  else if(precision == vtkAlgorithm::SINGLE_PRECISION)
  {
    newPts->SetDataType(VTK_FLOAT);
  }
  else if(precision == vtkAlgorithm::DOUBLE_PRECISION)
  {
    newPts->SetDataType(VTK_DOUBLE);
  }

  newPts->Allocate(estimatedSize,estimatedSize);
  newPolys = vtkCellArray::New();
  newPolys->Allocate(newPolys->EstimateSize(estimatedSize,3));

  o->GetPointData()->CopyAllOn();

  o->GetPointData()->InterpolateAllocate(input->GetPointData(),
                                         estimatedSize,estimatedSize/2);
  o->GetCellData()->CopyAllocate(input->GetCellData(),
                                 estimatedSize,estimatedSize/2);

  o->SetPoints(newPts);
  newPts->Delete();

  o->SetPolys(newPolys);
  newPolys->Delete();
}

inline void PointIndexToPosition(vtkImageData *img, int xidx, int yidx,
                                 int zidx, double position[3])
{
  double *origin = img->GetOrigin();
  double *spacing = img->GetSpacing();

  position[0] = origin[0] + (static_cast<double>(xidx) * spacing[0]);
  position[1] = origin[1] + (static_cast<double>(yidx) * spacing[1]);
  position[2] = origin[2] + (static_cast<double>(zidx) * spacing[2]);
}

inline int GetSideOfSurface(vtkImplicitFunction *func, double value,
                            const double pos[3])
{
  double diff = func->FunctionValue(pos) - value;
  return (diff < 0.0) ? -1 : ((diff > 0.0) ? 1 : 0);
}

//----------------------------------------------------------------------------
//
// Contouring filter specialized for images
//
template <class T>
void ContourImage(vtkThreadedSynchronizedTemplatesCutter3D *self, int *exExt,
                  vtkImageData *data, vtkPolyData *output, T *ptr,
                  bool outputTriangles)
{
  int *inExt = data->GetExtent();
  vtkIdType xdim = exExt[1] - exExt[0] + 1;
  vtkIdType ydim = exExt[3] - exExt[2] + 1;
  double *values = self->GetValues();
  vtkIdType numContours = self->GetNumberOfContours();
  T *inPtrX, *inPtrY, *inPtrZ;
  T *s0, *s1, *s2, *s3;
  int xMin, xMax, yMin, yMax, zMin, zMax;
  vtkIdType xInc, yInc, zInc;
  int xIncFunc, yIncFunc, zIncFunc, scalarZIncFunc;
  double *origin = data->GetOrigin();
  double *spacing = data->GetSpacing();
  vtkIdType *isect1Ptr, *isect2Ptr;
  double y, z, t;
  int i, j, k;
  vtkIdType zstep, yisectstep;
  vtkIdType offsets[12];
  int *tablePtr;
  vtkIdType idx;
  int vidx;
  double x[3], xz[3];
  vtkIdType v0, v1, v2, v3;
  vtkIdType ptIds[3];
  double value;
  // We need to know the edgePointId's for interpolating attributes.
  vtkIdType edgePtId, inCellId, outCellId;
  vtkPointData *inPD = data->GetPointData();
  vtkCellData *inCD = data->GetCellData();
  vtkPointData *outPD = output->GetPointData();
  vtkCellData *outCD = output->GetCellData();
  // Use to be arguments
  vtkPoints *newPts;
  vtkCellArray *newPolys;
  ptr += self->GetArrayComponent();
  vtkPolygonBuilder polyBuilder;
  vtkSmartPointer<vtkIdListCollection> polys =
    vtkSmartPointer<vtkIdListCollection>::New();

  newPts = output->GetPoints();
  newPolys = output->GetPolys();

  xMin = exExt[0];
  xMax = exExt[1];
  yMin = exExt[2];
  yMax = exExt[3];
  zMin = exExt[4];
  zMax = exExt[5];

  vtkImplicitFunction *func = self->GetCutFunction();
  if (!func)
  {
    return;
  }

  xInc = 1;
  yInc = xInc*(inExt[1]-inExt[0]+1);
  zInc = yInc*(inExt[3]-inExt[2]+1);

  // Note that the implicit functions are specified
  //over exExt so we need to compute the steps differently
  xIncFunc = 1;
  yIncFunc = xIncFunc*xdim;
  zIncFunc = yIncFunc*ydim;
  scalarZIncFunc = zIncFunc;

  // Kens increments, probably to do with edge array
  zstep = xdim*ydim;
  yisectstep = xdim*3;
  // compute offsets probably how to get to the edges in the edge array.
  offsets[0] = -xdim*3;
  offsets[1] = -xdim*3 + 1;
  offsets[2] = -xdim*3 + 2;
  offsets[3] = -xdim*3 + 4;
  offsets[4] = -xdim*3 + 5;
  offsets[5] = 0;
  offsets[6] = 2;
  offsets[7] = 5;
  offsets[8] = (zstep - xdim)*3;
  offsets[9] = (zstep - xdim)*3 + 1;
  offsets[10] = (zstep - xdim)*3 + 4;
  offsets[11] = zstep*3;

  double bboxCorners[8][3];
  PointIndexToPosition(data, exExt[0], exExt[2], exExt[4], bboxCorners[0]);
  PointIndexToPosition(data, exExt[1], exExt[2], exExt[4], bboxCorners[1]);
  PointIndexToPosition(data, exExt[0], exExt[3], exExt[4], bboxCorners[2]);
  PointIndexToPosition(data, exExt[0], exExt[2], exExt[5], bboxCorners[3]);
  PointIndexToPosition(data, exExt[1], exExt[3], exExt[4], bboxCorners[4]);
  PointIndexToPosition(data, exExt[0], exExt[3], exExt[5], bboxCorners[5]);
  PointIndexToPosition(data, exExt[1], exExt[2], exExt[5], bboxCorners[6]);
  PointIndexToPosition(data, exExt[1], exExt[3], exExt[5], bboxCorners[7]);

  // allocate storage array
  vtkIdType *isect1 = new vtkIdType [xdim*ydim*3*2];
  // set impossible edges to -1
  for (i = 0; i < ydim; i++)
  {
    isect1[(i+1)*xdim*3-3] = -1;
    isect1[(i+1)*xdim*3*2-3] = -1;
  }
  for (i = 0; i < xdim; i++)
  {
    isect1[((ydim-1)*xdim + i)*3 + 1] = -1;
    isect1[((ydim-1)*xdim + i)*3*2 + 1] = -1;
  }

  // allocate scalar storage for two slices
  T *scalars = new T [xdim*ydim*2];
  T *scalars1 = scalars;
  T *scalars2 = scalars + xdim*ydim;

  // for each contour
  for (vidx = 0; vidx < numContours; vidx++)
  {
    value = values[vidx];
    inPtrZ = ptr;

    // can the block be skipped?
    int total = 0;
    for (int ii = 0; ii < 8; ++ii)
    {
      total += GetSideOfSurface(func, value, bboxCorners[ii]);
    }

    vtkPlane *plane = vtkPlane::SafeDownCast(func);
    if (plane && (total == 8 || total == -8))
    {
      continue;
    }

    // fill the first slice
    z = origin[2] + spacing[2]*zMin;
    x[2] = z;
    T *scalarsTmp = scalars1;
    scalars1 = scalars2;
    scalars2 = scalarsTmp;
    for (j = yMin; j <= yMax; j++)
    {
      x[1] = origin[1] + spacing[1]*j;
      for (i = xMin; i <= xMax; i++)
      {
        x[0] = origin[0] + spacing[0]*i;
        *scalarsTmp = func->FunctionValue(x);
        scalarsTmp++;
      }
    }
    scalarZIncFunc = -scalarZIncFunc;

    //==================================================================
    for (k = zMin; k <= zMax; k++)
    {
      // self->UpdateProgress((double)vidx/numContours +
      //                      (k-zMin)/((zMax - zMin+1.0)*numContours));
      inPtrY = inPtrZ;

      // for each slice compute the scalars
      z = origin[2] + spacing[2]*(k+1);
      x[2] = z;
      scalarsTmp = scalars1;
      scalars1 = scalars2;
      scalars2 = scalarsTmp;
      // if not the last slice then get more scalars
      if (k < zMax)
      {
        for (j = yMin; j <= yMax; j++)
        {
          x[1] = origin[1] + spacing[1]*j;
          for (i = xMin; i <= xMax; i++)
          {
            x[0] = origin[0] + spacing[0]*i;
            *scalarsTmp = func->FunctionValue(x);
            scalarsTmp++;
          }
        }
      }
      inPtrY = scalars1;
      scalarZIncFunc = -scalarZIncFunc;

      z = origin[2] + spacing[2]*k;
      x[2] = z;

      // swap the buffers
      if (k%2)
      {
        offsets[8] = (zstep - xdim)*3;
        offsets[9] = (zstep - xdim)*3 + 1;
        offsets[10] = (zstep - xdim)*3 + 4;
        offsets[11] = zstep*3;
        isect1Ptr = isect1;
        isect2Ptr = isect1 + xdim*ydim*3;
      }
      else
      {
        offsets[8] = (-zstep - xdim)*3;
        offsets[9] = (-zstep - xdim)*3 + 1;
        offsets[10] = (-zstep - xdim)*3 + 4;
        offsets[11] = -zstep*3;
        isect1Ptr = isect1 + xdim*ydim*3;
        isect2Ptr = isect1;
      }

      for (j = yMin; j <= yMax; j++)
      {
        // Should not impact performance here/
        edgePtId = (xMin-inExt[0])*xInc + (j-inExt[2])*yInc + (k-inExt[4])*zInc;

        // Increments are different for cells.  Since the cells are not
        // contoured until the second row of templates, subtract 1 from
        // i,j,and k.  Note: first cube is formed when i=0, j=1, and k=1.
        inCellId =
          (xMin-inExt[0]) + (inExt[1]-inExt[0])*
          ( (j-inExt[2]-1) + (k-inExt[4]-1)*(inExt[3]-inExt[2]) );

        y = origin[1] + j*spacing[1];
        xz[1] = y;

        s1 = inPtrY;
        v1 = (*s1 < value ? 0 : 1);

        inPtrX = inPtrY;
        for (i = xMin; i <= xMax; i++)
        {
          s0 = s1;
          v0 = v1;
          *isect2Ptr = -1;
          *(isect2Ptr + 1) = -1;
          *(isect2Ptr + 2) = -1;
          if (i < xMax)
          {
            s1 = (inPtrX + xIncFunc);
            v1 = (*s1 < value ? 0 : 1);
            if (v0 ^ v1)
            {
              // watch for degenerate points
              if (*s0 == value)
              {
                if (i > xMin && *(isect2Ptr-3) > -1)
                {
                  *isect2Ptr = *(isect2Ptr-3);
                }
                else if (j > yMin && *(isect2Ptr - yisectstep + 1) > -1)
                {
                  *isect2Ptr = *(isect2Ptr - yisectstep + 1);
                }
                else if (k > zMin && *(isect1Ptr+2) > -1)
                {
                  *isect2Ptr = *(isect1Ptr+2);
                }
              }
              else if (*s1 == value)
              {
                if (j > yMin && *(isect2Ptr - yisectstep +4) > -1)
                {
                  *isect2Ptr = *(isect2Ptr - yisectstep + 4);
                }
                else if (k > zMin && i < xMax && *(isect1Ptr + 5) > -1)
                {
                  *isect2Ptr = *(isect1Ptr + 5);
                }
              }
              // if the edge has not been set yet then it is a new point
              if (*isect2Ptr == -1)
              {
                t = (value - (double)(*s0)) / ((double)(*s1) - (double)(*s0));
                x[0] = origin[0] + spacing[0]*(i+t);
                x[1] = y;
                *isect2Ptr = newPts->InsertNextPoint(x);
                outPD->InterpolateEdge(inPD, *isect2Ptr, edgePtId, edgePtId+1, t);
              }
            }
          }
          if (j < yMax)
          {
            s2 = (inPtrX + yIncFunc);
            v2 = (*s2 < value ? 0 : 1);
            if (v0 ^ v2)
            {
              if (*s0 == value)
              {
                if (*isect2Ptr > -1)
                {
                  *(isect2Ptr + 1) = *isect2Ptr;
                }
                else if (i > xMin && *(isect2Ptr-3) > -1)
                {
                  *(isect2Ptr + 1) = *(isect2Ptr-3);
                }
                else if (j > yMin && *(isect2Ptr - yisectstep + 1) > -1)
                {
                  *(isect2Ptr + 1) = *(isect2Ptr - yisectstep + 1);
                }
                else if (k > zMin && *(isect1Ptr+2) > -1)
                {
                  *(isect2Ptr + 1) = *(isect1Ptr+2);
                }
              }
              else if (*s2 == value && k > zMin && *(isect1Ptr + yisectstep + 2) > -1)
              {
                *(isect2Ptr+1) = *(isect1Ptr + yisectstep + 2);
              }
              // if the edge has not been set yet then it is a new point
              if (*(isect2Ptr + 1) == -1)
              {
                t = (value - (double)(*s0)) / ((double)(*s2) - (double)(*s0));
                x[0] = origin[0] + spacing[0]*i;
                x[1] = y + spacing[1]*t;
                *(isect2Ptr + 1) = newPts->InsertNextPoint(x);
                outPD->InterpolateEdge(inPD, *(isect2Ptr+1), edgePtId, edgePtId+yInc, t);
              }
            }
          }
          if (k < zMax)
          {
            s3 = (inPtrX + scalarZIncFunc);
            v3 = (*s3 < value ? 0 : 1);
            if (v0 ^ v3)
            {
              if (*s0 == value)
              {
                if (*isect2Ptr > -1)
                {
                  *(isect2Ptr + 2) = *isect2Ptr;
                }
                else if (*(isect2Ptr+1) > -1)
                {
                  *(isect2Ptr + 2) = *(isect2Ptr+1);
                }
                else if (i > xMin && *(isect2Ptr-3) > -1)
                {
                  *(isect2Ptr + 2) = *(isect2Ptr-3);
                }
                else if (j > yMin && *(isect2Ptr - yisectstep + 1) > -1)
                {
                  *(isect2Ptr + 2) = *(isect2Ptr - yisectstep + 1);
                }
                else if (k > zMin && *(isect1Ptr+2) > -1)
                {
                  *(isect2Ptr + 2) = *(isect1Ptr+2);
                }
              }
              if (*(isect2Ptr + 2) == -1)
              {
                t = (value - (double)(*s0)) / ((double)(*s3) - (double)(*s0));
                xz[0] = origin[0] + spacing[0]*i;
                xz[2] = z + spacing[2]*t;
                *(isect2Ptr + 2) = newPts->InsertNextPoint(xz);
                outPD->InterpolateEdge(inPD, *(isect2Ptr+2), edgePtId, edgePtId+zInc, t);
              }
            }
          }
          // To keep track of ids for interpolating attributes.
          ++edgePtId;

          // now add any polys that need to be added
          // basically look at the isect values,
          // form an index and lookup the polys
          if (j > yMin && i < xMax && k > zMin)
          {
            idx = (v0 ? 4096 : 0);
            idx = idx + (*(isect1Ptr - yisectstep) > -1 ? 2048 : 0);
            idx = idx + (*(isect1Ptr -yisectstep +1) > -1 ? 1024 : 0);
            idx = idx + (*(isect1Ptr -yisectstep +2) > -1 ? 512 : 0);
            idx = idx + (*(isect1Ptr -yisectstep +4) > -1 ? 256 : 0);
            idx = idx + (*(isect1Ptr -yisectstep +5) > -1 ? 128 : 0);
            idx = idx + (*(isect1Ptr) > -1 ? 64 : 0);
            idx = idx + (*(isect1Ptr + 2) > -1 ? 32 : 0);
            idx = idx + (*(isect1Ptr + 5) > -1 ? 16 : 0);
            idx = idx + (*(isect2Ptr -yisectstep) > -1 ? 8 : 0);
            idx = idx + (*(isect2Ptr -yisectstep +1) > -1 ? 4 : 0);
            idx = idx + (*(isect2Ptr -yisectstep +4) > -1 ? 2 : 0);
            idx = idx + (*(isect2Ptr) > -1 ? 1 : 0);

            tablePtr = VTK_TSYNCHRONIZED_TEMPLATES_3D_TABLE_2
              + VTK_TSYNCHRONIZED_TEMPLATES_3D_TABLE_1[idx];

            if (!outputTriangles)
            {
              polyBuilder.Reset();
            }
            while (*tablePtr != -1)
            {
              ptIds[0] = *(isect1Ptr + offsets[*tablePtr]);
              tablePtr++;
              ptIds[1] = *(isect1Ptr + offsets[*tablePtr]);
              tablePtr++;
              ptIds[2] = *(isect1Ptr + offsets[*tablePtr]);
              tablePtr++;
              if (ptIds[0] != ptIds[1] &&
                  ptIds[0] != ptIds[2] &&
                  ptIds[1] != ptIds[2])
              {
                if(outputTriangles)
                {
                  outCellId = newPolys->InsertNextCell(3,ptIds);
                  outCD->CopyData(inCD, inCellId, outCellId);
                }
                else
                {
                  polyBuilder.InsertTriangle(ptIds);
                }
              }
            }
            if(!outputTriangles)
            {
              polyBuilder.GetPolygons(polys);
              int nPolys = polys->GetNumberOfItems();
              for (int polyId = 0; polyId < nPolys; ++polyId)
              {
                vtkIdList* poly = polys->GetItem(polyId);
                if(poly->GetNumberOfIds()!=0)
                {
                  outCellId = newPolys->InsertNextCell(poly);
                  outCD->CopyData(inCD, inCellId, outCellId);
                }
                poly->Delete();
              }
              polys->RemoveAllItems();
            }
          }
          inPtrX += xIncFunc;
          isect2Ptr += 3;
          isect1Ptr += 3;
          // To keep track of ids for copying cell attributes..
          ++inCellId;
        }
        inPtrY += yIncFunc;
      }
      inPtrZ += zIncFunc;
    }
  }
  delete [] isect1;

  delete [] scalars;
}

class DoThreadedCut
{
public:
  DoThreadedCut(vtkThreadedSynchronizedTemplatesCutter3D* filter, int* exExt,
    vtkImageData* input, int numPieces)
    : Filter(filter), ExExt(exExt), Input(input), Outputs(nullptr),
      NumberOfPieces(numPieces), NumOutputPieces(0)
  {
  }

  ~DoThreadedCut()
  {
    delete[] this->Outputs;
  }

  int GetNumberOfOutputPieces() const
  {
    return this->NumOutputPieces;
  }

  vtkPolyData* GetOutputPiece(int pieceIdx)
  {
    return this->Outputs[pieceIdx];
  }

  void Initialize()
  {
  vtkThreadedSynchronizedTemplatesCutter3DInitializeOutput(ExExt,
    Filter->GetOutputPointsPrecision(),
    vtkSMPTools::GetEstimatedNumberOfThreads(), Input, this->tlsPoly.Local());
  }

  void Reduce()
  {
  this->NumOutputPieces = static_cast<int>(tlsPoly.size());
  this->Outputs = new vtkPolyData* [this->NumOutputPieces];

  int p = 0;
  for (ThreadLocalPolyObject::iterator i = this->tlsPoly.begin();
       i != this->tlsPoly.end(); ++i)
  {
    vtkPolyData *thisPiece = vtkPolyData::New();
    thisPiece->ShallowCopy(*i);
    this->Outputs[p++] = thisPiece;
  }
  }

  void operator()( vtkIdType begin, vtkIdType end )
  {
    vtkNew<vtkExtentTranslator> et;
    et->SetWholeExtent(this->ExExt);
    et->SetNumberOfPieces(this->NumberOfPieces);
    for(int i=begin; i<end; i++)
    {
      int exExt2[6];
      et->SetPiece(i);
      et->PieceToExtent();
      et->GetExtent(exExt2);
      ContourImage(this->Filter, exExt2, this->Input, this->tlsPoly.Local(),
                   (double*)nullptr, true);
    }
  }

private:
  vtkThreadedSynchronizedTemplatesCutter3D* Filter;
  int* ExExt;
  vtkImageData* Input;
  vtkPolyData** Outputs;
  int NumberOfPieces;
  int NumOutputPieces;

  typedef vtkSMPThreadLocalObject<vtkPolyData> ThreadLocalPolyObject;
  ThreadLocalPolyObject tlsPoly;
};

//----------------------------------------------------------------------------
//
// Contouring filter specialized for images (or slices from images)
//
void vtkThreadedSynchronizedTemplatesCutter3D::ThreadedExecute(
  vtkImageData *data, vtkInformation *outInfo, int)
{
  vtkMultiBlockDataSet *output;

  vtkDebugMacro(<< "Executing Cutter3D structured contour");

  output = vtkMultiBlockDataSet::GetData(outInfo);

  int* exExt = data->GetExtent();
  if ( exExt[0] >= exExt[1] || exExt[2] >= exExt[3] || exExt[4] >= exExt[5] )
  {
    vtkDebugMacro(<<"Cutter3D structured contours requires Cutter3D data");
    return;
  }

  int ncells = (exExt[1] - exExt[0]) * (exExt[3] - exExt[2]) *
               (exExt[5] - exExt[4]);
  const int grainSize = 64 * 64 * 64;
  int nPieces = (ncells == 0) ? 1 : (ncells + grainSize - 1)/grainSize;
  DoThreadedCut functor(this, exExt, data, nPieces);
  vtkSMPTools::For( 0, nPieces, functor );

  int count = 0;
  for(int i = 0; i < functor.GetNumberOfOutputPieces(); ++i)
  {
    vtkPolyData* contour = functor.GetOutputPiece(i);
    if (contour->GetNumberOfCells() > 0)
    {
      output->SetBlock(count++, contour);
    }
    contour->Delete();
  }
}

//----------------------------------------------------------------------------
int vtkThreadedSynchronizedTemplatesCutter3D::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkImageData *input = vtkImageData::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Just call the threaded execute directly.
  this->ThreadedExecute(input, outInfo, 0);

  return 1;
}


//----------------------------------------------------------------------------
void vtkThreadedSynchronizedTemplatesCutter3D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Cut Function: " << this->CutFunction << "\n";
  os << indent << "Precision of the output points: "
     << this->OutputPointsPrecision << "\n";
}
