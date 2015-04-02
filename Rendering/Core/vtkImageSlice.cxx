/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageSlice.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageSlice.h"

#include "vtkBoundingBox.h"
#include "vtkImageMapper3D.h"
#include "vtkImageProperty.h"
#include "vtkCamera.h"
#include "vtkDataArray.h"
#include "vtkImageData.h"
#include "vtkLookupTable.h"
#include "vtkLinearTransform.h"
#include "vtkMath.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkRenderer.h"
#include "vtkTransform.h"

#include <math.h>

vtkStandardNewMacro(vtkImageSlice);

//----------------------------------------------------------------------------
class vtkImageToImageMapper3DFriendship
{
public:
  static void SetCurrentProp(vtkImageMapper3D *mapper, vtkImageSlice *prop)
    {
    mapper->CurrentProp = prop;
    }
  static void SetCurrentRenderer(vtkImageMapper3D *mapper, vtkRenderer *ren)
    {
    mapper->CurrentRenderer = ren;
    }
  static void SetStackedImagePass(vtkImageMapper3D *mapper, int pass)
    {
    switch (pass)
      {
      case 0:
        mapper->MatteEnable = true;
        mapper->ColorEnable = false;
        mapper->DepthEnable = false;
        break;
      case 1:
        mapper->MatteEnable = false;
        mapper->ColorEnable = true;
        mapper->DepthEnable = false;
        break;
      case 2:
        mapper->MatteEnable = false;
        mapper->ColorEnable = false;
        mapper->DepthEnable = true;
        break;
      default:
        mapper->MatteEnable = true;
        mapper->ColorEnable = true;
        mapper->DepthEnable = true;
        break;
      }
    }

};

//----------------------------------------------------------------------------
vtkImageSlice::vtkImageSlice()
{
  this->Mapper = NULL;
  this->Property = NULL;
}

//----------------------------------------------------------------------------
vtkImageSlice::~vtkImageSlice()
{
  if (this->Property)
    {
    this->Property->UnRegister(this);
    }

  this->SetMapper(NULL);
}

//----------------------------------------------------------------------------
void vtkImageSlice::GetImages(vtkPropCollection *vc)
{
  vc->AddItem(this);
}

//----------------------------------------------------------------------------
void vtkImageSlice::ShallowCopy(vtkProp *prop)
{
  vtkImageSlice *v = vtkImageSlice::SafeDownCast(prop);

  if (v != NULL)
    {
    this->SetMapper(v->GetMapper());
    this->SetProperty(v->GetProperty());
    }

  // Now do superclass
  this->vtkProp3D::ShallowCopy(prop);
}

//----------------------------------------------------------------------------
void vtkImageSlice::SetMapper(vtkImageMapper3D *mapper)
{
  if (this->Mapper != mapper)
    {
    if (this->Mapper != NULL)
      {
      vtkImageToImageMapper3DFriendship::SetCurrentProp(this->Mapper, NULL);
      this->Mapper->UnRegister(this);
      }
    this->Mapper = mapper;
    if (this->Mapper != NULL)
      {
      this->Mapper->Register(this);
      vtkImageToImageMapper3DFriendship::SetCurrentProp(mapper, this);
      }
    this->Modified();
    }
}

//----------------------------------------------------------------------------
vtkBoundingBox vtkImageSlice::ComputeBoundingBox(vtkViewport *vp)
{
  // get the bounds of the Mapper if we have one
  vtkBoundingBox bbox = this->Mapper ? this->Mapper->ComputeBoundingBox(vp)
                                     : vtkBoundingBox();

  if (bbox.IsValid())
    {
    bbox.Transform(this->GetMatrix());
    }

  return bbox;
}

//----------------------------------------------------------------------------
// Does this prop have some translucent polygonal geometry?
int vtkImageSlice::HasTranslucentPolygonalGeometry()
{
  // Always render during opaque pass, to keep the behavior
  // predictable and because depth-peeling kills alpha-blending.
  // In the future, the Renderer should render images in layers,
  // i.e. where each image will have a layer number assigned to it,
  // and the Renderer will do the images in their own pass.
  return 0;
}

//----------------------------------------------------------------------------
int vtkImageSlice::RenderTranslucentPolygonalGeometry(vtkViewport* viewport)
{
  vtkDebugMacro(<< "vtkImageSlice::RenderTranslucentPolygonalGeometry");

  if (this->HasTranslucentPolygonalGeometry())
    {
    this->Render(vtkRenderer::SafeDownCast(viewport));
    return 1;
    }

  return 0;
}

//----------------------------------------------------------------------------
int vtkImageSlice::RenderOpaqueGeometry(vtkViewport* viewport)
{
  vtkDebugMacro(<< "vtkImageSlice::RenderOpaqueGeometry");

  if (!this->HasTranslucentPolygonalGeometry())
    {
    this->Render(vtkRenderer::SafeDownCast(viewport));
    return 1;
    }

  return 0;
}

//----------------------------------------------------------------------------
int vtkImageSlice::RenderOverlay(vtkViewport* vtkNotUsed(viewport))
{
  vtkDebugMacro(<< "vtkImageSlice::RenderOverlay");

  // Render the image as an underlay

  return 0;
}

//----------------------------------------------------------------------------
void vtkImageSlice::Render(vtkRenderer *ren)
{
  // Force the creation of a property
  if (!this->Property)
    {
    this->GetProperty();
    }

  if (!this->Property)
    {
    vtkErrorMacro( << "Error generating a property!\n" );
    return;
    }

  if (!this->Mapper)
    {
    vtkErrorMacro( << "You must specify a mapper!\n" );
    return;
    }

  vtkImageToImageMapper3DFriendship::SetCurrentRenderer(this->Mapper, ren);

  this->Update();

  // only call the mapper if it has an input
  if (this->Mapper->GetInput())
    {
    this->Mapper->Render(ren, this);
    this->EstimatedRenderTime += this->Mapper->GetTimeToDraw();
    }

  vtkImageToImageMapper3DFriendship::SetCurrentRenderer(this->Mapper, NULL);
}

//----------------------------------------------------------------------------
void vtkImageSlice::ReleaseGraphicsResources(vtkWindow *win)
{
  // pass this information onto the mapper
  if (this->Mapper)
    {
    this->Mapper->ReleaseGraphicsResources(win);
    }
}

//----------------------------------------------------------------------------
void vtkImageSlice::Update()
{
  if (this->Mapper)
    {
    vtkImageToImageMapper3DFriendship::SetCurrentProp(this->Mapper, this);
    this->Mapper->Update();
    }
}

//----------------------------------------------------------------------------
void vtkImageSlice::SetProperty(vtkImageProperty *property)
{
  if (this->Property != property)
    {
    if (this->Property != NULL)
      {
      this->Property->UnRegister(this);
      }
    this->Property = property;
    if (this->Property != NULL)
      {
      this->Property->Register(this);
      }
    this->Modified();
    }
}

//----------------------------------------------------------------------------
vtkImageProperty *vtkImageSlice::GetProperty()
{
  if (this->Property == NULL)
    {
    this->Property = vtkImageProperty::New();
    this->Property->Register(this);
    this->Property->Delete();
    }
  return this->Property;
}

//----------------------------------------------------------------------------
unsigned long int vtkImageSlice::GetMTime()
{
  unsigned long mTime = this->Superclass::GetMTime();
  unsigned long time;

  if ( this->Property != NULL )
    {
    time = this->Property->GetMTime();
    mTime = ( time > mTime ? time : mTime );
    }

  if ( this->UserMatrix != NULL )
    {
    time = this->UserMatrix->GetMTime();
    mTime = ( time > mTime ? time : mTime );
    }

  if ( this->UserTransform != NULL )
    {
    time = this->UserTransform->GetMTime();
    mTime = ( time > mTime ? time : mTime );
    }

  return mTime;
}

//----------------------------------------------------------------------------
unsigned long vtkImageSlice::GetRedrawMTime()
{
  unsigned long mTime = this->GetMTime();
  unsigned long time;

  if ( this->Mapper != NULL )
    {
    time = this->Mapper->GetMTime();
    mTime = ( time > mTime ? time : mTime );
    if (this->GetMapper()->GetInputAlgorithm() != NULL)
      {
      this->GetMapper()->GetInputAlgorithm()->Update();
      time = this->Mapper->GetInput()->GetMTime();
      mTime = ( time > mTime ? time : mTime );
      }
    }

  if ( this->Property != NULL )
    {
    time = this->Property->GetMTime();
    mTime = ( time > mTime ? time : mTime );

    if ( this->Property->GetLookupTable() != NULL )
      {
      // check the lookup table mtime
      time = this->Property->GetLookupTable()->GetMTime();
      mTime = ( time > mTime ? time : mTime );
      }
    }

  return mTime;
}

//----------------------------------------------------------------------------
void vtkImageSlice::SetStackedImagePass(int pass)
{
  if (this->Mapper)
    {
    vtkImageToImageMapper3DFriendship::SetStackedImagePass(
      this->Mapper, pass);
    }
}

//----------------------------------------------------------------------------
void vtkImageSlice::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  if( this->Property )
    {
    os << indent << "Property:\n";
    this->Property->PrintSelf(os,indent.GetNextIndent());
    }
  else
    {
    os << indent << "Property: (not defined)\n";
    }

  if( this->Mapper )
    {
    os << indent << "Mapper:\n";
    this->Mapper->PrintSelf(os,indent.GetNextIndent());
    }
  else
    {
    os << indent << "Mapper: (not defined)\n";
    }

  // make sure our bounds are up to date
  vtkBoundingBox bbox = this->ComputeBoundingBox(NULL);
  if (bbox.IsValid())
    {
    os << indent << "Bounds (without viewport): "
       << "(" << bbox.GetBound(0) << ", " << bbox.GetBound(1) << ") "
       << "(" << bbox.GetBound(2) << ", " << bbox.GetBound(3) << ") "
       << "(" << bbox.GetBound(4) << ", " << bbox.GetBound(5) << ")\n";
    }
  else
    {
    os << indent << "Bounds: (not defined, invalid, or requires viewport)\n";
    }
}
