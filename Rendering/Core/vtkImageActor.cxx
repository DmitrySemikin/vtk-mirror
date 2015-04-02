/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageActor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageActor.h"

#include "vtkBoundingBox.h"
#include "vtkObjectFactory.h"
#include "vtkImageData.h"
#include "vtkMath.h"
#include "vtkMatrix4x4.h"
#include "vtkRenderer.h"
#include "vtkImageProperty.h"
#include "vtkImageSliceMapper.h"
#include "vtkInformation.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkStandardNewMacro(vtkImageActor);

//----------------------------------------------------------------------------
vtkImageActor::vtkImageActor()
{
  this->DisplayExtent[0] = 0;
  this->DisplayExtent[1] = -1;
  this->DisplayExtent[2] = 0;
  this->DisplayExtent[3] = -1;
  this->DisplayExtent[4] = 0;
  this->DisplayExtent[5] = -1;

  vtkMath::UninitializeBounds(this->DisplayBounds);

  this->Property = vtkImageProperty::New();
  this->Property->SetInterpolationTypeToLinear();
  this->Property->SetAmbient(1.0);
  this->Property->SetDiffuse(0.0);

  vtkImageSliceMapper *mapper = vtkImageSliceMapper::New();
  this->Mapper = mapper;
  mapper->BorderOff();
  mapper->SliceAtFocalPointOff();
  mapper->SliceFacesCameraOff();
  mapper->SetOrientationToZ();
  // For backwards compabilitity, make Streaming the default behavior
  mapper->StreamingOn();
}

//----------------------------------------------------------------------------
vtkImageActor::~vtkImageActor()
{
  if (this->Property)
    {
    this->Property->Delete();
    this->Property = NULL;
    }
  if (this->Mapper)
    {
    this->Mapper->Delete();
    this->Mapper = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkImageActor::SetInputData(vtkImageData *input)
{
  if (this->Mapper && input != this->Mapper->GetInput())
    {
    this->Mapper->SetInputData(input);
    this->Modified();
    }
}

//----------------------------------------------------------------------------
vtkAlgorithm *vtkImageActor::GetInputAlgorithm()
{
  if (!this->Mapper)
    {
    return 0;
    }

  return this->Mapper->GetInputAlgorithm();
}

//----------------------------------------------------------------------------
vtkImageData *vtkImageActor::GetInput()
{
  if (!this->Mapper)
    {
    return 0;
    }

  return this->Mapper->GetInput();
}

//----------------------------------------------------------------------------
void vtkImageActor::SetInterpolate(int i)
{
  if (this->Property)
    {
    if (i)
      {
      if (this->Property->GetInterpolationType() != VTK_LINEAR_INTERPOLATION)
        {
        this->Property->SetInterpolationTypeToLinear();
        this->Modified();
        }
      }
    else
      {
      if (this->Property->GetInterpolationType() != VTK_NEAREST_INTERPOLATION)
        {
        this->Property->SetInterpolationTypeToNearest();
        this->Modified();
        }
      }
    }
}

//----------------------------------------------------------------------------
int vtkImageActor::GetInterpolate()
{
  if (this->Property &&
      this->Property->GetInterpolationType() != VTK_NEAREST_INTERPOLATION)
    {
    return 1;
    }

  return 0;
}

//----------------------------------------------------------------------------
void vtkImageActor::SetOpacity(double o)
{
  if (this->Property && this->Property->GetOpacity() != o)
    {
    this->Property->SetOpacity(o);
    this->Modified();
    }
}

//----------------------------------------------------------------------------
double vtkImageActor::GetOpacity()
{
  if (this->Property)
    {
    return this->Property->GetOpacity();
    }

  return 1.0;
}

//----------------------------------------------------------------------------
int vtkImageActor::GetSliceNumber()
{
  if (!this->Mapper || !this->Mapper->IsA("vtkImageSliceMapper"))
    {
    return 0;
    }

  return static_cast<vtkImageSliceMapper *>(this->Mapper)->GetSliceNumber();
}

//----------------------------------------------------------------------------
int vtkImageActor::GetSliceNumberMax()
{
  if (!this->Mapper || !this->Mapper->IsA("vtkImageSliceMapper"))
    {
    return 0;
    }

  return static_cast<vtkImageSliceMapper *>(this->Mapper)
    ->GetSliceNumberMaxValue();
}

//----------------------------------------------------------------------------
int vtkImageActor::GetSliceNumberMin()
{
  if (!this->Mapper || !this->Mapper->IsA("vtkImageSliceMapper"))
    {
    return 0;
    }

  return static_cast<vtkImageSliceMapper *>(this->Mapper)
    ->GetSliceNumberMinValue();
}

//----------------------------------------------------------------------------
void vtkImageActor::SetDisplayExtent(int extent[6])
{
  int idx, modified = 0;

  for (idx = 0; idx < 6; ++idx)
    {
    if (this->DisplayExtent[idx] != extent[idx])
      {
      this->DisplayExtent[idx] = extent[idx];
      modified = 1;
      }
    }

  if (modified)
    {
    if (this->Mapper && this->Mapper->IsA("vtkImageSliceMapper"))
      {
      if (this->DisplayExtent[0] <= this->DisplayExtent[1])
        {
        static_cast<vtkImageSliceMapper *>(this->Mapper)->CroppingOn();
        static_cast<vtkImageSliceMapper *>(this->Mapper)->
          SetCroppingRegion(this->DisplayExtent);
        static_cast<vtkImageSliceMapper *>(this->Mapper)->
          SetOrientation(this->GetOrientationFromExtent(this->DisplayExtent));
        }
      else
        {
        static_cast<vtkImageSliceMapper *>(this->Mapper)->CroppingOff();
        static_cast<vtkImageSliceMapper *>(this->Mapper)->
          SetOrientationToZ();
        }
      }
    this->Modified();
    }
}
//----------------------------------------------------------------------------
void vtkImageActor::SetDisplayExtent(int minX, int maxX,
                                     int minY, int maxY,
                                     int minZ, int maxZ)
{
  int extent[6];

  extent[0] = minX;  extent[1] = maxX;
  extent[2] = minY;  extent[3] = maxY;
  extent[4] = minZ;  extent[5] = maxZ;
  this->SetDisplayExtent(extent);
}


//----------------------------------------------------------------------------
void vtkImageActor::GetDisplayExtent(int extent[6])
{
  for (int idx = 0; idx < 6; ++idx)
    {
    extent[idx] = this->DisplayExtent[idx];
    }
}

//----------------------------------------------------------------------------
// Get the bounds for this Volume as (Xmin,Xmax,Ymin,Ymax,Zmin,Zmax).
double *vtkImageActor::GetDisplayBounds()
{
  vtkAlgorithm* inputAlg = NULL;

  if (this->Mapper && this->Mapper->GetNumberOfInputConnections(0) > 0)
    {
    inputAlg = this->Mapper->GetInputAlgorithm();
    }

  if (!inputAlg)
    {
    return this->DisplayBounds;
    }

  inputAlg->UpdateInformation();
  int extent[6];
  vtkInformation* inputInfo =
    this->Mapper->GetInputInformation();
  inputInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), extent);
  double spacing[3] = {1, 1, 1};
  if (inputInfo->Has(vtkDataObject::SPACING()))
    {
    inputInfo->Get(vtkDataObject::SPACING(), spacing);
    }
  double origin[3] = {0, 0, 0};
  if (inputInfo->Has(vtkDataObject::ORIGIN()))
    {
    inputInfo->Get(vtkDataObject::ORIGIN(), origin);
    }

  // if the display extent has not been set, use first slice
  extent[5] = extent[4];

  if (this->DisplayExtent[0] <= this->DisplayExtent[1])
    {
    extent[0] = this->DisplayExtent[0];
    extent[1] = this->DisplayExtent[1];
    extent[2] = this->DisplayExtent[2];
    extent[3] = this->DisplayExtent[3];
    extent[4] = this->DisplayExtent[4];
    extent[5] = this->DisplayExtent[5];
    }

  if (spacing[0] >= 0)
    {
    this->DisplayBounds[0] = extent[0]*spacing[0] + origin[0];
    this->DisplayBounds[1] = extent[1]*spacing[0] + origin[0];
    }
  else
    {
    this->DisplayBounds[0] = extent[1]*spacing[0] + origin[0];
    this->DisplayBounds[1] = extent[0]*spacing[0] + origin[0];
    }
  if (spacing[1] >= 0)
    {
    this->DisplayBounds[2] = extent[2]*spacing[1] + origin[1];
    this->DisplayBounds[3] = extent[3]*spacing[1] + origin[1];
    }
  else
    {
    this->DisplayBounds[2] = extent[3]*spacing[1] + origin[1];
    this->DisplayBounds[3] = extent[2]*spacing[1] + origin[1];
    }
  if (spacing[2] >= 0)
    {
    this->DisplayBounds[4] = extent[4]*spacing[2] + origin[2];
    this->DisplayBounds[5] = extent[5]*spacing[2] + origin[2];
    }
  else
    {
    this->DisplayBounds[4] = extent[5]*spacing[2] + origin[2];
    this->DisplayBounds[5] = extent[4]*spacing[2] + origin[2];
    }

  return this->DisplayBounds;
}

//----------------------------------------------------------------------------
// Get the bounds for the displayed data as (Xmin,Xmax,Ymin,Ymax,Zmin,Zmax).
void vtkImageActor::GetDisplayBounds(double bounds[6])
{
  this->GetDisplayBounds();
  for (int i = 0; i < 6; i++)
    {
    bounds[i] = this->DisplayBounds[i];
    }
}


//----------------------------------------------------------------------------
vtkBoundingBox vtkImageActor::ComputeBoundingBox(vtkViewport *)
{
  double bounds[6];
  this->GetDisplayBounds(bounds);

  vtkBoundingBox bbox;
  bbox.AddBounds(bounds);

  if (bbox.IsValid())
    {
    bbox.Transform(this->GetMatrix());
    }

  return bbox;
}

//----------------------------------------------------------------------------
int vtkImageActor::GetOrientationFromExtent(const int extent[6])
{
  int orientation = 2;

  if (extent[4] == extent[5])
    {
    orientation = 2;
    }
  else if (extent[2] == extent[3])
    {
    orientation = 1;
    }
  else if (extent[0] == extent[1])
    {
    orientation = 0;
    }

  return orientation;
}

//----------------------------------------------------------------------------
void vtkImageActor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Input: " << this->GetInput() << "\n";
  os << indent << "Interpolate: " << (this->GetInterpolate() ? "On\n" : "Off\n");
  os << indent << "Opacity: " << this->GetOpacity() << "\n";

  os << indent << "DisplayExtent: (" << this->DisplayExtent[0];
  for (int idx = 1; idx < 6; ++idx)
    {
    os << ", " << this->DisplayExtent[idx];
    }
  os << ")\n";
}

//----------------------------------------------------------------------------
int vtkImageActor::GetWholeZMin()
{
  int *extent;

  if ( ! this->GetInputAlgorithm())
    {
    return 0;
    }
  this->GetInputAlgorithm()->UpdateInformation();
  extent = this->Mapper->GetInputInformation()->Get(
    vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());
  return extent[4];
}

//----------------------------------------------------------------------------
int vtkImageActor::GetWholeZMax()
{
  int *extent;

  if ( ! this->GetInputAlgorithm())
    {
    return 0;
    }
  this->GetInputAlgorithm()->UpdateInformation();
  extent = this->Mapper->GetInputInformation()->Get(
    vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());
  return extent[5];
}

//-----------------------------------------------------------------------------
// Description:
// Does this prop have some translucent polygonal geometry?
int vtkImageActor::HasTranslucentPolygonalGeometry()
{
  vtkImageData *input = this->GetInput();
  if (!input)
    {
    return 0;
    }

  // This requires that Update has been called on the mapper,
  // which the Renderer does immediately before it renders.
  if ( input->GetScalarType() == VTK_UNSIGNED_CHAR )
    {
    if (!(this->GetOpacity() >= 1.0 &&
          input->GetNumberOfScalarComponents() % 2))
      {
      return 1;
      }
    }

  return 0;
}
