/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAssembly.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAssembly.h"
#include "vtkActor.h"
#include "vtkAssemblyNode.h"
#include "vtkAssemblyPaths.h"
#include "vtkBoundingBox.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkProp3DCollection.h"
#include "vtkRenderWindow.h"
#include "vtkVolume.h"

vtkStandardNewMacro(vtkAssembly);

// Construct object with no children.
vtkAssembly::vtkAssembly()
{
  this->Parts = vtkProp3DCollection::New();
}

vtkAssembly::~vtkAssembly()
{
  vtkCollectionSimpleIterator pit;
  vtkProp *part;
  for (this->Parts->InitTraversal(pit);
    (part=this->Parts->GetNextProp(pit));)
    {
    part->RemoveConsumer(this);
    }

  this->Parts->Delete();
  this->Parts = 0;
}

// Add a part to the list of Parts.
void vtkAssembly::AddPart(vtkProp3D *prop)
{
  if (!this->Parts->IsItemPresent(prop))
    {
    this->Parts->AddItem(prop);
    prop->AddConsumer(this);
    this->Modified();
    }
}

// Remove a part from the list of parts,
void vtkAssembly::RemovePart(vtkProp3D *prop)
{
  if (this->Parts->IsItemPresent(prop))
    {
    prop->RemoveConsumer(this);
    this->Parts->RemoveItem(prop);
    this->Modified();
    }
}

// Shallow copy another assembly.
void vtkAssembly::ShallowCopy(vtkProp *prop)
{
  vtkAssembly *p = vtkAssembly::SafeDownCast(prop);
  if (p && p != this)
    {
    vtkCollectionSimpleIterator pit;
    vtkProp3D *part;
    for (this->Parts->InitTraversal(pit);
      (part=this->Parts->GetNextProp3D(pit));)
      {
      part->RemoveConsumer(this);
      }
    this->Parts->RemoveAllItems();
    for (p->Parts->InitTraversal(pit);
      (part=p->Parts->GetNextProp3D(pit));)
      {
      this->AddPart(part);
      }
    }

  // Now do superclass
  this->vtkProp3D::ShallowCopy(prop);
}

// Render this assembly and all its Parts. The rendering process is recursive.
// Note that a mapper need not be defined. If not defined, then no geometry
// will be drawn for this assembly. This allows you to create "logical"
// assemblies; that is, assemblies that only serve to group and transform
// its Parts.
int vtkAssembly::RenderTranslucentPolygonalGeometry(vtkViewport *ren)
{
  this->UpdatePaths();

  int renderedSomething = 0;

  // for allocating render time between components
  // simple equal allocation
  double fraction = this->AllocatedRenderTime
    / static_cast<double>(this->Paths->GetNumberOfItems());

  // render the Paths
  vtkAssemblyPath *path;
  vtkCollectionSimpleIterator sit;
  for (this->Paths->InitTraversal(sit);
    (path = this->Paths->GetNextPath(sit));)
    {
    vtkProp3D* prop3D = static_cast<vtkProp3D *>(path->GetLastNode()->GetViewProp());
    if ( prop3D->GetVisibility() )
      {
      prop3D->SetAllocatedRenderTime(fraction, ren);
      prop3D->PokeMatrix(path->GetLastNode()->GetMatrix());
      renderedSomething += prop3D->RenderTranslucentPolygonalGeometry(ren);
      prop3D->PokeMatrix(NULL);
      }
    }

  return (renderedSomething > 0) ? 1 : 0;
}

// Description:
// Does this prop have some translucent polygonal geometry?
int vtkAssembly::HasTranslucentPolygonalGeometry()
{
  this->UpdatePaths();

  int result = 0;

  // render the Paths
  vtkAssemblyPath *path;
  vtkCollectionSimpleIterator sit;
  for (this->Paths->InitTraversal(sit);
    !result && (path = this->Paths->GetNextPath(sit));)
    {
    vtkProp3D* prop3D = static_cast<vtkProp3D *>(path->GetLastNode()->GetViewProp());
    if ( prop3D->GetVisibility() )
      {
      result = prop3D->HasTranslucentPolygonalGeometry();
      }
    }

  return result;
}


// Render this assembly and all its Parts. The rendering process is recursive.
// Note that a mapper need not be defined. If not defined, then no geometry
// will be drawn for this assembly. This allows you to create "logical"
// assemblies; that is, assemblies that only serve to group and transform
// its Parts.
int vtkAssembly::RenderVolumetricGeometry(vtkViewport *ren)
{
  this->UpdatePaths();

  // for allocating render time between components
  // simple equal allocation
  double fraction = this->AllocatedRenderTime
    / static_cast<double>(this->Paths->GetNumberOfItems());

  int renderedSomething = 0;

  // render the Paths
  vtkAssemblyPath *path;
  vtkCollectionSimpleIterator sit;
  for ( this->Paths->InitTraversal(sit); (path = this->Paths->GetNextPath(sit)); )
    {
    vtkProp3D* prop3D = static_cast<vtkProp3D *>(path->GetLastNode()->GetViewProp());
    if (prop3D->GetVisibility())
      {
      prop3D->SetAllocatedRenderTime(fraction, ren);
      prop3D->PokeMatrix(path->GetLastNode()->GetMatrix());
      renderedSomething += prop3D->RenderVolumetricGeometry(ren);
      prop3D->PokeMatrix(NULL);
      }
    }

  return (renderedSomething > 0) ? 1 : 0;
}

// Render this assembly and all its Parts. The rendering process is recursive.
// Note that a mapper need not be defined. If not defined, then no geometry
// will be drawn for this assembly. This allows you to create "logical"
// assemblies; that is, assemblies that only serve to group and transform
// its Parts.
int vtkAssembly::RenderOpaqueGeometry(vtkViewport *ren)
{
  this->UpdatePaths();

  // for allocating render time between components
  // simple equal allocation
  double fraction = this->AllocatedRenderTime
    / static_cast<double>(this->Paths->GetNumberOfItems());

  int renderedSomething = 0;

  // render the Paths
  vtkAssemblyPath *path;
  vtkCollectionSimpleIterator sit;
  for (this->Paths->InitTraversal(sit);
    (path = this->Paths->GetNextPath(sit));)
    {
    vtkProp3D* prop3D = static_cast<vtkProp3D *>(path->GetLastNode()->GetViewProp());
    if (prop3D->GetVisibility())
      {
      prop3D->PokeMatrix(path->GetLastNode()->GetMatrix());
      prop3D->SetAllocatedRenderTime(fraction, ren);
      renderedSomething += prop3D->RenderOpaqueGeometry(ren);
      prop3D->PokeMatrix(NULL);
      }
    }

  return (renderedSomething > 0)? 1 : 0;
}

void vtkAssembly::ReleaseGraphicsResources(vtkWindow *renWin)
{
  vtkProp3D *prop3D;

  vtkCollectionSimpleIterator pit;
  for (this->Parts->InitTraversal(pit);
    (prop3D = this->Parts->GetNextProp3D(pit));)
    {
    prop3D->ReleaseGraphicsResources(renWin);
    }
}

void vtkAssembly::GetActors(vtkPropCollection *ac)
{
  this->UpdatePaths();

  vtkAssemblyPath *path;
  vtkCollectionSimpleIterator sit;
  for (this->Paths->InitTraversal(sit);
    (path = this->Paths->GetNextPath(sit));)
    {
    vtkProp3D* prop3D = static_cast<vtkProp3D *>(path->GetLastNode()->GetViewProp());
    vtkActor *actor = vtkActor::SafeDownCast(prop3D);
    if (actor)
      {
      ac->AddItem(actor);
      }
    }
}

void vtkAssembly::GetVolumes(vtkPropCollection *ac)
{
  this->UpdatePaths();

  vtkAssemblyPath *path;
  vtkCollectionSimpleIterator sit;
  for (this->Paths->InitTraversal(sit);
    (path = this->Paths->GetNextPath(sit));)
    {
    vtkProp3D* prop3D = static_cast<vtkProp3D *>(path->GetLastNode()->GetViewProp());
    vtkVolume *volume = vtkVolume::SafeDownCast(prop3D);
    if (volume)
      {
      ac->AddItem(volume);
      }
    }
}

void vtkAssembly::InitPathTraversal()
{
  this->UpdatePaths();
  this->Paths->InitTraversal();
}

// Return the next part in the hierarchy of assembly Parts.  This method
// returns a properly transformed and updated actor.
vtkAssemblyPath *vtkAssembly::GetNextPath()
{
  return this->Paths ? this->Paths->GetNextItem() : 0;
}

int vtkAssembly::GetNumberOfPaths()
{
  this->UpdatePaths();
  return this->Paths->GetNumberOfItems();
}

// Build the assembly paths if necessary. UpdatePaths()
// is only called when the assembly is at the root
// of the hierarchy; otherwise UpdatePaths() is called.
void vtkAssembly::UpdatePaths()
{
  if (this->GetMTime() > this->PathTime ||
    (this->Paths != NULL && this->Paths->GetMTime() > this->PathTime))
    {
    if (this->Paths)
      {
      this->Paths->Delete();
      this->Paths = 0;
      }

    // Create the list to hold all the paths
    this->Paths = vtkAssemblyPaths::New();
    vtkAssemblyPath *path = vtkAssemblyPath::New();

    //add ourselves to the path to start things off
    path->AddNode(this,this->GetMatrix());

    // Add nodes as we proceed down the hierarchy
    vtkProp3D *prop3D;
    vtkCollectionSimpleIterator pit;
    for (this->Parts->InitTraversal(pit);
      (prop3D = this->Parts->GetNextProp3D(pit));)
      {
      path->AddNode(prop3D, prop3D->GetMatrix());

      // dive into the hierarchy
      prop3D->BuildPaths(this->Paths, path);

      // when returned, pop the last node off of the
      // current path
      path->DeleteLastNode();
      }

    path->Delete();
    this->PathTime.Modified();
    }
}

// Build assembly paths from this current assembly. A path consists of
// an ordered sequence of props, with transformations properly concatenated.
void vtkAssembly::BuildPaths(vtkAssemblyPaths *paths, vtkAssemblyPath *path)
{
  vtkProp3D *prop3D;

  vtkCollectionSimpleIterator pit;
  for (this->Parts->InitTraversal(pit);
    (prop3D = this->Parts->GetNextProp3D(pit));)
    {
    path->AddNode(prop3D, prop3D->GetMatrix());

    // dive into the hierarchy
    prop3D->BuildPaths(paths, path);

    // when returned, pop the last node off of the
    // current path
    path->DeleteLastNode();
    }
}

vtkBoundingBox vtkAssembly::ComputeBoundingBox(vtkViewport *vp)
{
  this->UpdatePaths();

  // now calculate the new bounds
  vtkBoundingBox bbox;

  vtkAssemblyPath *path;
  vtkCollectionSimpleIterator sit;
  for (this->Paths->InitTraversal(sit);
    (path = this->Paths->GetNextPath(sit));)
    {
    vtkProp3D* prop3D =
        static_cast<vtkProp3D*>(path->GetLastNode()->GetViewProp());
    if (prop3D->GetVisibility() && prop3D->GetUseBounds())
      {
      prop3D->PokeMatrix(path->GetLastNode()->GetMatrix());
      bbox.AddBox(prop3D->ComputeBoundingBox(vp));
      prop3D->PokeMatrix(NULL);
      }//if visible && prop3d
    }//for each path

  return bbox;
}

unsigned long int vtkAssembly::GetMTime()
{
  unsigned long mTime = this->vtkProp3D::GetMTime();
  vtkProp3D *prop;

  vtkCollectionSimpleIterator pit;
  for (this->Parts->InitTraversal(pit);
    (prop = this->Parts->GetNextProp3D(pit));)
    {
    unsigned long time = prop->GetMTime();
    mTime = ( time > mTime ? time : mTime );
    }

  return mTime;
}

void vtkAssembly::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "There are: " << this->Parts->GetNumberOfItems()
     << " parts in this assembly\n";
}

