/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPartitionedDataSetCollectionMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPartitionedDataSetCollectionMapper.h"

#include "vtkActor.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkDataAssembly.h"
#include "vtkDataAssemblyVisitor.h"
#include "vtkDataProperties.h"
#include "vtkExecutive.h"
#include "vtkGarbageCollector.h"
#include "vtkInformation.h"
#include "vtkMapper.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkPartitionedDataSetMapper.h"

#include <map>
#include <stack>
#include <vector>

using std::greater;
using std::less;
using namespace vtkDataProperties;

namespace
{

template <typename F, typename T>
inline void assign(T* A, T* B, int i)
{
  F f;
  A[i] = f(B[i], A[i]) ? B[i] : A[i];
};
}

class vtkPartitionedDataSetCollectionMapper::internals
{
public:
  std::map<int, vtkPartitionedDataSetMapper*> Mappers;
  vtkDataAssembly* currentDataAssembly = nullptr;
};

struct vtkPartitionedDataSetCollectionMapper::DataAssemblyVisitor : public vtkDataAssemblyVisitor
{
  static DataAssemblyVisitor* New();

  vtkPartitionedDataSetCollectionMapper* ParentClass = nullptr;
  std::map<int, std::stack<std::string>> Properties;

  /*
   *
   */
  virtual void BeginSubTree(int nodeid) override
  {
    for (auto prop : this->GetCurrentProperties())
    {
      auto it = Properties.find(prop);
      if (it == Properties.end())
      {
        it = Properties.insert({ prop, {} }).first;
      }
      it->second.push(this->GetCurrentProperty(prop));
    }
  }

  /*
   *
   */
  virtual void EndSubTree(int nodeid) override
  {
    for (auto prop : this->GetCurrentProperties())
    {
      auto it = Properties.find(prop);
      if (it != Properties.end())
      {
        it->second.pop();
      }
    }
  }

  /*
   *
   */
  std::string RetrieveProperty(int prop, std::string _default = {})
  {
    if (!this->GetCurrentProperty(prop).empty())
    {
      return this->GetCurrentProperty(prop);
    }
    else
    {
      auto it = Properties.find(prop);
      if (it != Properties.end())
      {
        return it->second.top();
      }
    }
    return _default;
  }
};

vtkStandardNewMacro(vtkPartitionedDataSetCollectionMapper);

//------------------------------------------------------------------------------
struct vtkPartitionedDataSetCollectionMapper::DataAssemblyVisitorBuildDatasets
  : public vtkPartitionedDataSetCollectionMapper::DataAssemblyVisitor
{
  static DataAssemblyVisitorBuildDatasets* New();

  vtkPartitionedDataSetCollection* Input = nullptr;

  virtual void Visit(int nodeid) override
  {
    for (int node : this->GetCurrentDataSetIndices())
    {
      vtkPartitionedDataSet* pd =
        vtkPartitionedDataSet::SafeDownCast(this->Input->GetPartitionedDataSet(node));
      if (pd)
      {
        this->ParentClass->Internal->Mappers.insert({ node, {} });
        // Make a copy of the data to break the pipeline here
        vtkPartitionedDataSet* newpd = vtkPartitionedDataSet::New();
        newpd->ShallowCopy(pd);

        vtkPartitionedDataSetMapper* pdmapper = vtkPartitionedDataSetMapper::New();
        pdmapper->vtkMapper::ShallowCopy(this->ParentClass);
        pdmapper->Register(this->ParentClass);
        pdmapper->AddDataset(newpd);
        this->ParentClass->Internal->Mappers[node] = pdmapper;
        newpd->Delete();
      }
    }
  }
};

//------------------------------------------------------------------------------
struct vtkPartitionedDataSetCollectionMapper::DataAssemblyVisitorRender
  : public vtkPartitionedDataSetCollectionMapper::DataAssemblyVisitor
{
  static DataAssemblyVisitorRender* New();

  vtkActor* Actor = nullptr;
  vtkRenderer* Renderer = nullptr;

  /*
   *
   */
  virtual void Visit(int nodeid) override
  {
    if (this->RetrieveProperty(Visibility, "true") == "false")
    {
      return;
    }

    for (int node : this->GetCurrentDataSetIndices())
    {
      auto mapper = this->ParentClass->Internal->Mappers[node];

      // skip if we have a mismatch in opaque and translucent
      if (Actor->IsRenderingTranslucentPolygonalGeometry() == mapper->HasOpaqueGeometry())
      {
        continue;
      }

      if (ParentClass->ClippingPlanes != mapper->GetClippingPlanes())
      {
        mapper->SetClippingPlanes(ParentClass->ClippingPlanes);
      }

      mapper->SetLookupTable(ParentClass->GetLookupTable());
      mapper->SetScalarVisibility(ParentClass->GetScalarVisibility());
      mapper->SetUseLookupTableScalarRange(ParentClass->GetUseLookupTableScalarRange());
      mapper->SetScalarRange(ParentClass->GetScalarRange());
      mapper->SetColorMode(ParentClass->GetColorMode());
      mapper->SetInterpolateScalarsBeforeMapping(ParentClass->GetInterpolateScalarsBeforeMapping());

      mapper->SetScalarMode(ParentClass->GetScalarMode());
      if (ParentClass->ScalarMode == VTK_SCALAR_MODE_USE_POINT_FIELD_DATA ||
        ParentClass->ScalarMode == VTK_SCALAR_MODE_USE_CELL_FIELD_DATA)
      {
        if (ParentClass->ArrayAccessMode == VTK_GET_ARRAY_BY_ID)
        {
          mapper->ColorByArrayComponent(ParentClass->ArrayId, this->ParentClass->ArrayComponent);
        }
        else
        {
          mapper->ColorByArrayComponent(ParentClass->ArrayName, this->ParentClass->ArrayComponent);
        }
      }

      mapper->Render(Renderer, Actor);
      ParentClass->TimeToDraw += mapper->GetTimeToDraw();
    }
  }
};

//------------------------------------------------------------------------------
struct vtkPartitionedDataSetCollectionMapper::DataAssemblyVisitorComputeBounds
  : public vtkPartitionedDataSetCollectionMapper::DataAssemblyVisitor
{
  static DataAssemblyVisitorComputeBounds* New();

  virtual void Visit(int nodeid) override
  {
    if (this->RetrieveProperty(Visibility, "true") == "false")
    {
      return;
    }

    for (int node : this->GetCurrentDataSetIndices())
    {
      auto mapper = this->ParentClass->Internal->Mappers[node];

      if (mapper)
      {
        double bounds[6] = { 0 };

        if (vtkMath::AreBoundsInitialized(this->ParentClass->Bounds))
        {
          mapper->GetBounds(bounds);
          if (vtkMath::AreBoundsInitialized(bounds))
          {
            for (int i = 0; i < 3; i++)
            {
              assign<less<double>>(this->ParentClass->Bounds, bounds, i * 2);
              assign<greater<double>>(this->ParentClass->Bounds, bounds, i * 2 + 1);
            }
          }
        }
        else
        {
          mapper->GetBounds(this->ParentClass->Bounds);
        }
      }
    }
  }
};

vtkStandardNewMacro(vtkPartitionedDataSetCollectionMapper::DataAssemblyVisitorBuildDatasets);
vtkStandardNewMacro(vtkPartitionedDataSetCollectionMapper::DataAssemblyVisitorRender);
vtkStandardNewMacro(vtkPartitionedDataSetCollectionMapper::DataAssemblyVisitorComputeBounds);

//------------------------------------------------------------------------------
vtkPartitionedDataSetCollectionMapper::vtkPartitionedDataSetCollectionMapper()
{
  this->Internal.reset(new vtkPartitionedDataSetCollectionMapper::internals);
}

//------------------------------------------------------------------------------
vtkPartitionedDataSetCollectionMapper::~vtkPartitionedDataSetCollectionMapper()
{
  for (auto mappers : this->Internal->Mappers)
  {
    auto& mapper = mappers.second;
    mapper->UnRegister(this);
    mapper->Delete();
  }
  this->Internal->Mappers.clear();
}

//------------------------------------------------------------------------------
int vtkPartitionedDataSetCollectionMapper::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPartitionedDataSetCollection");
  return 1;
}

//------------------------------------------------------------------------------
void vtkPartitionedDataSetCollectionMapper::BuildPartitionedMapperCollection()
{
  for (auto mappers : this->Internal->Mappers)
  {
    auto& mapper = mappers.second;
    mapper->UnRegister(this);
  }
  this->Internal->Mappers.clear();

  vtkInformation* inInfo = this->GetExecutive()->GetInputInformation(0, 0);
  vtkPartitionedDataSetCollection* input =
    vtkPartitionedDataSetCollection::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));

  if (input)
  {
    auto dataAssembly = input->GetDataAssembly();
    if (dataAssembly)
    {
      vtkNew<DataAssemblyVisitorBuildDatasets> visitor;
      visitor->Input = input;
      visitor->ParentClass = this;

      this->Internal->currentDataAssembly = dataAssembly;
      this->Internal->currentDataAssembly->Visit(visitor);
    }
    else
    {
      vtkErrorMacro("This mapper needs a vtkPartitionedDataSet with a valid vtkDataAssembly");
    }
  }
  else
  {
    vtkDataObject* tmpInp = this->GetExecutive()->GetInputData(0, 0);
    vtkErrorMacro(
      "This mapper cannot handle input of type: " << (tmpInp ? tmpInp->GetClassName() : "(none)"));
  }

  this->InternalMappersBuildTime.Modified();
}

//------------------------------------------------------------------------------
void vtkPartitionedDataSetCollectionMapper::Render(vtkRenderer* ren, vtkActor* a)
{
  vtkCompositeDataPipeline* executive =
    vtkCompositeDataPipeline::SafeDownCast(this->GetExecutive());

  if (executive->GetPipelineMTime() > this->InternalMappersBuildTime.GetMTime())
  {
    this->BuildPartitionedMapperCollection();
  }

  vtkNew<DataAssemblyVisitorRender> visitor;
  visitor->ParentClass = this;
  visitor->Renderer = ren;
  visitor->Actor = a;
  this->Internal->currentDataAssembly->Visit(visitor);
}

//------------------------------------------------------------------------------
void vtkPartitionedDataSetCollectionMapper::ComputeBounds()
{
  vtkMath::UninitializeBounds(this->Bounds);

  vtkInformation* inInfo = this->GetExecutive()->GetInputInformation(0, 0);
  vtkPartitionedDataSetCollection* input =
    vtkPartitionedDataSetCollection::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));

  auto dataAssembly = this->Internal->currentDataAssembly;
  if (dataAssembly)
  {
    vtkNew<DataAssemblyVisitorComputeBounds> visitor;
    visitor->ParentClass = this;
    this->Internal->currentDataAssembly->Visit(visitor);
  }

  this->BoundsMTime.Modified();
}

//------------------------------------------------------------------------------
vtkExecutive* vtkPartitionedDataSetCollectionMapper::CreateDefaultExecutive()
{
  return vtkCompositeDataPipeline::New();
}

//------------------------------------------------------------------------------
double* vtkPartitionedDataSetCollectionMapper::GetBounds()
{
  if (!this->GetExecutive()->GetInputData(0, 0))
  {
    vtkMath::UninitializeBounds(this->Bounds);
    return this->Bounds;
  }
  else
  {
    this->Update();

    // only compute bounds when the input data has changed
    vtkCompositeDataPipeline* executive =
      vtkCompositeDataPipeline::SafeDownCast(this->GetExecutive());
    if (executive->GetPipelineMTime() > this->BoundsMTime.GetMTime())
    {
      this->BuildPartitionedMapperCollection();
      this->ComputeBounds();
    }

    return this->Bounds;
  }
}

//------------------------------------------------------------------------------
void vtkPartitionedDataSetCollectionMapper::ReleaseGraphicsResources(vtkWindow* win)
{
  for (auto mappers : this->Internal->Mappers)
  {
    auto& mapper = mappers.second;
    mapper->ReleaseGraphicsResources(win);
  }
}

//------------------------------------------------------------------------------
void vtkPartitionedDataSetCollectionMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
bool vtkPartitionedDataSetCollectionMapper::HasOpaqueGeometry()
{
  vtkCompositeDataPipeline* executive =
    vtkCompositeDataPipeline::SafeDownCast(this->GetExecutive());

  if (executive->GetPipelineMTime() > this->InternalMappersBuildTime.GetMTime())
  {
    this->BuildPartitionedMapperCollection();
  }

  bool hasOpaque = false;
  for (auto mappers : this->Internal->Mappers)
  {
    auto& mapper = mappers.second;
    hasOpaque = hasOpaque || mapper->HasOpaqueGeometry();
  }
  return hasOpaque;
}

//------------------------------------------------------------------------------
bool vtkPartitionedDataSetCollectionMapper::HasTranslucentPolygonalGeometry()
{
  vtkCompositeDataPipeline* executive =
    vtkCompositeDataPipeline::SafeDownCast(this->GetExecutive());

  if (executive->GetPipelineMTime() > this->InternalMappersBuildTime.GetMTime())
  {
    this->BuildPartitionedMapperCollection();
  }

  bool hasTrans = false;
  for (auto mappers : this->Internal->Mappers)
  {
    auto& mapper = mappers.second;
    hasTrans = hasTrans || mapper->HasTranslucentPolygonalGeometry();
  }
  return hasTrans;
}
