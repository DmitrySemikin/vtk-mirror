/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPartitionedDataSetMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPartitionedDataSetMapper.h"

#include "vtkActor.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkDataSet.h"
#include "vtkDataSetMapper.h"
#include "vtkExecutive.h"
#include "vtkGarbageCollector.h"
#include "vtkInformation.h"
#include "vtkMapper.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPartitionedDataSet.h"

#include <map>
#include <vector>

using std::greater;
using std::less;

namespace
{

template <typename F, typename T>
inline void assign(T* A, T* B, int i)
{
  F f;
  A[i] = f(B[i], A[i]) ? B[i] : A[i];
};

}

vtkStandardNewMacro(vtkPartitionedDataSetMapper);

class vtkPartitionedDataSetMapperInternals
{
public:
  std::vector<vtkDataSetMapper*> Mappers;
};

//------------------------------------------------------------------------------
vtkPartitionedDataSetMapper::vtkPartitionedDataSetMapper()
{
  this->Internal = new vtkPartitionedDataSetMapperInternals;
}

//------------------------------------------------------------------------------
vtkPartitionedDataSetMapper::~vtkPartitionedDataSetMapper()
{
  for (auto mapper : this->Internal->Mappers)
  {
    mapper->UnRegister(this);
  }
  this->Internal->Mappers.clear();

  delete this->Internal;
}

//------------------------------------------------------------------------------
int vtkPartitionedDataSetMapper::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPartitionedDataSet");
  return 1;
}

//------------------------------------------------------------------------------
void vtkPartitionedDataSetMapper::AddDataset(vtkPartitionedDataSet* partitionedDataset)
{
  if (partitionedDataset)
  {
    for (int j = 0; j < partitionedDataset->GetNumberOfPartitions(); j++)
    {
      vtkDataSetMapper* ds = this->MakeAMapper();
      ds->Register(this);
      ds->SetInputData(partitionedDataset->GetPartition(j));
      this->Internal->Mappers.push_back(ds);
    }
    this->InternalMappersBuildTime.Modified();
  }
}

//------------------------------------------------------------------------------
vtkDataSetMapper* vtkPartitionedDataSetMapper::MakeAMapper()
{
  vtkDataSetMapper* m = vtkDataSetMapper::New();
  m->vtkMapper::ShallowCopy(this);
  return m;
}

//------------------------------------------------------------------------------
void vtkPartitionedDataSetMapper::Render(vtkRenderer* ren, vtkActor* a)
{
  this->TimeToDraw = 0;
  for (auto mapper : this->Internal->Mappers)
  {
    // skip if we have a mismatch in opaque and translucent
    if (a->IsRenderingTranslucentPolygonalGeometry() == mapper->HasOpaqueGeometry())
    {
      continue;
    }

    if (this->ClippingPlanes != mapper->GetClippingPlanes())
    {
      mapper->SetClippingPlanes(this->ClippingPlanes);
    }

    mapper->SetLookupTable(this->GetLookupTable());
    mapper->SetScalarVisibility(this->GetScalarVisibility());
    mapper->SetUseLookupTableScalarRange(this->GetUseLookupTableScalarRange());
    mapper->SetScalarRange(this->GetScalarRange());
    mapper->SetColorMode(this->GetColorMode());
    mapper->SetInterpolateScalarsBeforeMapping(this->GetInterpolateScalarsBeforeMapping());

    mapper->SetScalarMode(this->GetScalarMode());
    if (this->ScalarMode == VTK_SCALAR_MODE_USE_POINT_FIELD_DATA ||
      this->ScalarMode == VTK_SCALAR_MODE_USE_CELL_FIELD_DATA)
    {
      if (this->ArrayAccessMode == VTK_GET_ARRAY_BY_ID)
      {
        mapper->ColorByArrayComponent(this->ArrayId, ArrayComponent);
      }
      else
      {
        mapper->ColorByArrayComponent(this->ArrayName, ArrayComponent);
      }
    }

    mapper->Render(ren, a);
    this->TimeToDraw += mapper->GetTimeToDraw();
  }
}

//------------------------------------------------------------------------------
void vtkPartitionedDataSetMapper::ComputeBounds()
{
  vtkMath::UninitializeBounds(this->Bounds);

  double bounds[6] = { 0 };
  for (auto mapper : this->Internal->Mappers)
  {
    if (vtkMath::AreBoundsInitialized(this->Bounds))
    {
      mapper->GetBounds(bounds);
      if (vtkMath::AreBoundsInitialized(bounds))
      {
        for (int i = 0; i < 3; i++)
        {
          assign<less<double>>(this->Bounds, bounds, i * 2);
          assign<greater<double>>(this->Bounds, bounds, i * 2 + 1);
        }
      }
    }
    else
    {
      mapper->GetBounds(this->Bounds);
    }
  }
  this->BoundsMTime.Modified();
}

//------------------------------------------------------------------------------
vtkExecutive* vtkPartitionedDataSetMapper::CreateDefaultExecutive()
{
  return vtkCompositeDataPipeline::New();
}

//------------------------------------------------------------------------------
double* vtkPartitionedDataSetMapper::GetBounds()
{
  if (this->BoundsMTime.GetMTime() < this->InternalMappersBuildTime.GetMTime())
  {
    this->ComputeBounds();
  }

  return this->Bounds;
}

//------------------------------------------------------------------------------
void vtkPartitionedDataSetMapper::ReleaseGraphicsResources(vtkWindow* win)
{
  for (auto mapper : this->Internal->Mappers)
  {
    mapper->ReleaseGraphicsResources(win);
  }
}

//------------------------------------------------------------------------------
void vtkPartitionedDataSetMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
bool vtkPartitionedDataSetMapper::HasOpaqueGeometry()
{
  bool hasOpaque = false;
  for (auto mapper : this->Internal->Mappers)
  {
    hasOpaque = hasOpaque || mapper->HasOpaqueGeometry();
  }
  return hasOpaque;
}

//------------------------------------------------------------------------------
bool vtkPartitionedDataSetMapper::HasTranslucentPolygonalGeometry()
{
  bool hasTrans = false;
  for (auto mapper : this->Internal->Mappers)
  {
    hasTrans = hasTrans || mapper->HasTranslucentPolygonalGeometry();
  }
  return hasTrans;
}
