/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkActor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkActor.h"

#include "vtkBoundingBox.h"
#include "vtkDataArray.h"
#include "vtkObjectFactory.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationDoubleVectorKey.h"
#include "vtkMapper.h"
#include "vtkMath.h"
#include "vtkMatrix4x4.h"
#include "vtkPointData.h"
#include "vtkPropCollection.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkScalarsToColors.h"
#include "vtkTexture.h"
#include "vtkTransform.h"

#include <math.h>

vtkCxxSetObjectMacro(vtkActor,Texture,vtkTexture);
vtkCxxSetObjectMacro(vtkActor,Mapper,vtkMapper);
vtkCxxSetObjectMacro(vtkActor,BackfaceProperty,vtkProperty);
vtkCxxSetObjectMacro(vtkActor,Property,vtkProperty);

//----------------------------------------------------------------------------
// Return NULL if no override is supplied.
vtkAbstractObjectFactoryNewMacro(vtkActor)

// Creates an actor with the following defaults: origin(0,0,0)
// position=(0,0,0) scale=(1,1,1) visibility=1 pickable=1 dragable=1
// orientation=(0,0,0). No user defined matrix and no texture map.
vtkActor::vtkActor()
{
  this->Mapper = NULL;
  this->Property = NULL;
  this->BackfaceProperty = NULL;
  this->Texture = NULL;

  // The mapper bounds are cache to know when the bounds must be recomputed
  // from the mapper bounds.
  vtkMath::UninitializeBounds(this->MapperBounds);
}

//----------------------------------------------------------------------------
vtkActor::~vtkActor()
{
  if ( this->Property != NULL)
    {
    this->Property->UnRegister(this);
    this->Property = NULL;
    }

  if ( this->BackfaceProperty != NULL)
    {
    this->BackfaceProperty->UnRegister(this);
    this->BackfaceProperty = NULL;
    }

  if (this->Mapper)
    {
    this->Mapper->UnRegister(this);
    this->Mapper = NULL;
    }
  this->SetTexture(NULL);
}

//----------------------------------------------------------------------------
// Shallow copy of an actor.
void vtkActor::ShallowCopy(vtkProp *prop)
{
  vtkActor *a = vtkActor::SafeDownCast(prop);
  if ( a != NULL )
    {
    this->SetMapper(a->GetMapper());
    this->SetProperty(a->GetProperty());
    this->SetBackfaceProperty(a->GetBackfaceProperty());
    this->SetTexture(a->GetTexture());
    }

  // Now do superclass
  this->vtkProp3D::ShallowCopy(prop);
}

//----------------------------------------------------------------------------
void vtkActor::GetActors(vtkPropCollection *ac)
{
  ac->AddItem(this);
}

//----------------------------------------------------------------------------
// should be called from the render methods only
int vtkActor::GetIsOpaque()
{
  // make sure we have a property
  if(!this->Property)
    {
    // force creation of a property
    this->GetProperty();
    }
  bool is_opaque = (this->Property->GetOpacity() >= 1.0);

  // are we using an opaque texture, if any?
  is_opaque = is_opaque &&
    (this->Texture ==NULL || this->Texture->IsTranslucent() == 0);

  // are we using an opaque LUT, if any?
  is_opaque = is_opaque &&
    (this->Mapper == NULL || this->Mapper->GetLookupTable() == NULL ||
     this->Mapper->GetLookupTable()->IsOpaque() == 1);

  // are we using an opaque scalar array, if any?
  is_opaque = is_opaque &&
    (this->Mapper == NULL || this->Mapper->GetIsOpaque());

  return is_opaque? 1 : 0;
}

//----------------------------------------------------------------------------
// This causes the actor to be rendered. It in turn will render the actor's
// property, texture map and then mapper. If a property hasn't been
// assigned, then the actor will create one automatically. Note that a
// side effect of this method is that the visualization network is updated.
int vtkActor::RenderOpaqueGeometry(vtkViewport *vp)
{
  int          renderedSomething = 0;
  vtkRenderer* ren = static_cast<vtkRenderer*>(vp);

  if ( ! this->Mapper )
    {
    return 0;
    }

  // make sure we have a property
  if (!this->Property)
    {
    // force creation of a property
    this->GetProperty();
    }

  // is this actor opaque
  // Do this check only when not in selection mode
  if (this->GetIsOpaque() ||
    (ren->GetSelector() && this->Property->GetOpacity() > 0.0))
    {
    this->Property->Render(this, ren);

    // render the backface property
    if (this->BackfaceProperty)
      {
      this->BackfaceProperty->BackfaceRender(this, ren);
      }

    // render the texture
    if (this->Texture)
      {
      this->Texture->Render(ren);
      if (this->Texture->GetTransform())
        {
        vtkInformation *info = this->GetPropertyKeys();
        if (!info)
          {
          info = vtkInformation::New();
          this->SetPropertyKeys(info);
          info->Delete();
          }
        info->Set(vtkProp::GeneralTextureTransform(),
          &(this->Texture->GetTransform()->GetMatrix()->Element[0][0])
          ,16);
        }
      }
    this->Render(ren,this->Mapper);
    this->Property->PostRender(this, ren);
    if (this->Texture)
      {
      this->Texture->PostRender(ren);
      if (this->Texture->GetTransform())
        {
        vtkInformation *info = this->GetPropertyKeys();
        info->Remove(vtkProp::GeneralTextureTransform());
        }
      }
    this->EstimatedRenderTime += this->Mapper->GetTimeToDraw();
    renderedSomething = 1;
    }

  return renderedSomething;
}

//-----------------------------------------------------------------------------
int vtkActor::RenderTranslucentPolygonalGeometry(vtkViewport *vp)
{
  int          renderedSomething = 0;
  vtkRenderer* ren = static_cast<vtkRenderer*>(vp);

  if ( ! this->Mapper )
    {
    return 0;
    }

  // make sure we have a property
  if (!this->Property)
    {
    // force creation of a property
    this->GetProperty();
    }

  // is this actor opaque ?
  if (!this->GetIsOpaque())
    {
    this->Property->Render(this, ren);

    // render the backface property
    if (this->BackfaceProperty)
      {
      this->BackfaceProperty->BackfaceRender(this, ren);
      }

    // render the texture
    if (this->Texture)
      {
      this->Texture->Render(ren);
      if (this->Texture->GetTransform())
        {
        vtkInformation *info = this->GetPropertyKeys();
        if (!info)
          {
          info = vtkInformation::New();
          this->SetPropertyKeys(info);
          info->Delete();
          }
        info->Set(vtkProp::GeneralTextureTransform(),
          &(this->Texture->GetTransform()->GetMatrix()->Element[0][0])
          ,16);
        }
      }
    this->Render(ren,this->Mapper);
    this->Property->PostRender(this, ren);
    if (this->Texture)
      {
      this->Texture->PostRender(ren);
      if (this->Texture->GetTransform())
        {
        vtkInformation *info = this->GetPropertyKeys();
        info->Remove(vtkProp::GeneralTextureTransform());
        }
      }
    this->EstimatedRenderTime += this->Mapper->GetTimeToDraw();

    renderedSomething = 1;
    }

  return renderedSomething;
}

//-----------------------------------------------------------------------------
// Description:
// Does this prop have some translucent polygonal geometry?
int vtkActor::HasTranslucentPolygonalGeometry()
{
  if ( ! this->Mapper )
    {
    return 0;
    }
  // make sure we have a property
  if (!this->Property)
    {
    // force creation of a property
    this->GetProperty();
    }

  // is this actor opaque ?
  return !this->GetIsOpaque();
}

//----------------------------------------------------------------------------
void vtkActor::ReleaseGraphicsResources(vtkWindow *win)
{
  vtkRenderWindow *renWin = static_cast<vtkRenderWindow *>(win);

  // pass this information onto the mapper
  if (this->Mapper)
    {
    this->Mapper->ReleaseGraphicsResources(renWin);
    }

  // pass this information onto the texture
  if (this->Texture)
    {
    this->Texture->ReleaseGraphicsResources(renWin);
    }

  // pass this information to the properties
  if (this->Property)
    {
    this->Property->ReleaseGraphicsResources(renWin);
    }
  if (this->BackfaceProperty)
    {
    this->BackfaceProperty->ReleaseGraphicsResources(renWin);
    }
}

//----------------------------------------------------------------------------
vtkProperty* vtkActor::MakeProperty()
{
  return vtkProperty::New();
}

//----------------------------------------------------------------------------
vtkProperty *vtkActor::GetProperty()
{
  if ( this->Property == NULL )
    {
    vtkProperty *p = this->MakeProperty();
    this->SetProperty(p);
    p->Delete();
    }
  return this->Property;
}

//----------------------------------------------------------------------------
vtkBoundingBox vtkActor::ComputeBoundingBox(vtkViewport *vp)
{
  vtkDebugMacro( << "Computing Bounding Box" );

  vtkBoundingBox bbox;

  if (!this->Mapper)
    {
    return bbox;
    }

  bbox = this->Mapper->ComputeBoundingBox(vp);

  // Check for the special case when the actor is empty.
  if (!bbox.IsValid())
    {
    bbox.GetBounds(this->MapperBounds);
    this->BoundsMTime.Modified();
    return bbox;
    }

  // Check if we have cached values for these bounds - we cache the
  // values returned by this->Mapper->GetBounds() and we store the time
  // of caching. If the values returned this time are different, or
  // the modified time of this class is newer than the cached time,
  // then we need to rebuild.
  if (bbox != vtkBoundingBox(this->MapperBounds) ||
      (this->GetMTime() > this->BoundsMTime))
    {
    vtkDebugMacro( << "Recomputing bounds..." );
    bbox.GetBounds(this->MapperBounds);
    bbox.Transform(this->GetMatrix());
    bbox.GetBounds(this->Bounds);
    this->BoundsMTime.Modified();
    }
  else
    {
    bbox.Reset();
    bbox.AddBounds(this->Bounds);
    }

  return bbox;
}

//----------------------------------------------------------------------------
unsigned long int vtkActor::GetMTime()
{
  unsigned long mTime=this->Superclass::GetMTime();
  unsigned long time;

  if ( this->Property != NULL )
    {
    time = this->Property->GetMTime();
    mTime = ( time > mTime ? time : mTime );
    }

  if ( this->BackfaceProperty != NULL )
    {
    time = this->BackfaceProperty->GetMTime();
    mTime = ( time > mTime ? time : mTime );
    }

  if ( this->Texture != NULL )
    {
    time = this->Texture->GetMTime();
    mTime = ( time > mTime ? time : mTime );
    }

  return mTime;
}

//----------------------------------------------------------------------------
unsigned long int vtkActor::GetRedrawMTime()
{
  unsigned long mTime=this->GetMTime();
  unsigned long time;

  if ( this->Mapper != NULL )
    {
    time = this->Mapper->GetMTime();
    mTime = ( time > mTime ? time : mTime );
    if (this->GetMapper()->GetInput() != NULL)
      {
      this->GetMapper()->GetInputAlgorithm()->Update();
      time = this->Mapper->GetInput()->GetMTime();
      mTime = ( time > mTime ? time : mTime );
      }
    }

  return mTime;
}

//----------------------------------------------------------------------------
void vtkActor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  if ( this->Mapper )
    {
    os << indent << "Mapper:\n";
    this->Mapper->PrintSelf(os,indent.GetNextIndent());
    }
  else
    {
    os << indent << "Mapper: (none)\n";
    }

  if ( this->Property )
    {
    os << indent << "Property:\n";
    this->Property->PrintSelf(os,indent.GetNextIndent());
    }
  else
    {
    os << indent << "Property: (none)\n";
    }

  if ( this->BackfaceProperty )
    {
    os << indent << "BackfaceProperty:\n";
    this->BackfaceProperty->PrintSelf(os,indent.GetNextIndent());
    }
  else
    {
    os << indent << "BackfaceProperty: (none)\n";
    }

  if ( this->Texture )
    {
    os << indent << "Texture: " << this->Texture << "\n";
    }
  else
    {
    os << indent << "Texture: (none)\n";
    }

}

//----------------------------------------------------------------------------
bool vtkActor::GetSupportsSelection()
{
  if (this->Mapper)
    {
    return this->Mapper->GetSupportsSelection();
    }

  return false;
}
