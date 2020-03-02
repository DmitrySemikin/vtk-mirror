/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDIYSyncGhostCellsAttributesUtilities.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkDIYSyncGhostCellsAttributesUtilities.h"

#include "vtkCellData.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataObject.h"
#include "vtkIdTypeArray.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkMultiProcessController.h"
#include "vtkUnsignedCharArray.h"

#include <map>
#include <set>
#include <vector>

// clang-format off
#include "vtk_diy2.h"
#include VTK_DIY2(diy/mpi.hpp)
#include VTK_DIY2(diy/master.hpp)
#include VTK_DIY2(diy/link.hpp)
#include VTK_DIY2(diy/reduce.hpp)
#include VTK_DIY2(diy/reduce-operations.hpp)
#include VTK_DIY2(diy/partners/swap.hpp)
#include VTK_DIY2(diy/assigner.hpp)
#include VTK_DIY2(diy/algorithms.hpp)
// clang-format on

//----------------------------------------------------------------------------
vtkDIYSyncGhostCellsAttributesUtilities::vtkDIYSyncGhostCellsAttributesUtilities() {}

//----------------------------------------------------------------------------
vtkDIYSyncGhostCellsAttributesUtilities::~vtkDIYSyncGhostCellsAttributesUtilities() {}

//----------------------------------------------------------------------------
void vtkDIYSyncGhostCellsAttributesUtilities::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkDIYSyncGhostCellsAttributesUtilities::Initialize()
{
  if (this->GhostTypes.size())
  {
    this->GhostTypes.clear();
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkDIYSyncGhostCellsAttributesUtilities::AddGhostType(int type)
{
  if (!this->GhostTypes.count(type))
  {
    this->GhostTypes.Insert(type);
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkDIYSyncGhostCellsAttributesUtilities::RemoveGhostType(int type)
{
  if (this->GhostTypes.count(type))
  {
    this->GhostTypes.Erase(type);
    this->Modified();
  }
}

//----------------------------------------------------------------------------
bool vtkDIYSyncGhostCellsAttributesUtilities::HasGhostType(int type) const
{
  return this->GhostTypes.count(type);
}

//----------------------------------------------------------------------------
const std::set<int>& GetGhostTypes() const
{
  return this->GhostTypes;
}

//----------------------------------------------------------------------------
void vtkDIYSyncGhostCellsAttributesUtilities::Sync(vtkDataObject* input, vtkMultiProcessController* controller)
{
  std::map<int, bool> hasGhosts;
  for (type : this->GhostTypes)
  {
    hasGhosts[type] = input->HasAnyGhostElements(type);
  }

  std::vector<std::vector<vtkIdType>>
  diy::mpi::communicator comm = vtkDIYUtilities::GetCommunicator(controller);
}
