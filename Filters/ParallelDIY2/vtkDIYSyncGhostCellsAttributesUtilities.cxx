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
#include "vtkCommunicator.h"
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

namespace
{
  struct vtkInternals
  {
    struct vtkInternalAttributes
    {
      int Type;
      vtkDataSetAttributes* DataSetAttributes;
      vtkIdTypeArray* GlobalIds;

      vtkInternalAttributes(int type, vtkDataSetAttributes* dsa)
        : Type(ype), DataSetAttributes(dsa)
      {
        GlobalIds = this->DataSetAttributes->GetGlobalIds();
      }
    }

    std::vector<vtkInternalAttributes> Attributes;
    std::set<int> GhostTypes;
    std::map<int, std::vector<vtkIdType>> GhostsGlobalIdsMap, GhostIdsMap;
    std::map<int, std::unoredered_map<vtkIdType, vtkIdType> GlobalIdMap;
  }
}

//----------------------------------------------------------------------------
vtkDIYSyncGhostCellsAttributesUtilities::vtkDIYSyncGhostCellsAttributesUtilities()
  : Internals(new vtkInternals())
{
}

//----------------------------------------------------------------------------
vtkDIYSyncGhostCellsAttributesUtilities::~vtkDIYSyncGhostCellsAttributesUtilities()
{
  delete this->Internals();
}

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
  if (!this->Internals->GhostTypes.count(type))
  {
    this->Internals->GhostTypes.Insert(type);
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkDIYSyncGhostCellsAttributesUtilities::RemoveGhostType(int type)
{
  if (this->Internals->GhostTypes.count(type))
  {
    this->Internals->GhostTypes.Erase(type);
    this->Modified();
  }
}

//----------------------------------------------------------------------------
bool vtkDIYSyncGhostCellsAttributesUtilities::HasGhostType(int type) const
{
  return this->Internals->GhostTypes.count(type);
}

//----------------------------------------------------------------------------
const std::set<int>& GetGhostTypes() const
{
  return this->Internals->GhostTypes;
}

//----------------------------------------------------------------------------
void vtkDIYSyncGhostCellsAttributesUtilities::PreprocessInput(vtkDataObject* input)
{
  for (int type : this->Internals->GhostTypes)
  {
    if (input->HasAnyGhostElements(type))
    {
      this->Internals->Attributes.emplace_back(
          vtkInternals::vtkInternalsAttributes(type, input->GetDataSetAttributes(type)));
    }
  }
  
  for (auto&& att : attributes)
  {
    vtkUnsignedChar* ghostArray = input->GetGhostArray(att.type);
    std::vector<vtkIdType>& ghostIds = this->Internals->GhostsIdsMap[att.type];
    std::vector<vtkIdType>& ghostGlobalIds = this->Internals->GhostsGlobalIdsMap[att.type];
    for (vtkIdType id = 0; id < ghostArray->GetNumberOfElements(); ++id)
    {
      if (ghostArray->GetTuple1(id))
      {
        ghostGlobalIds.push_back(att.gids.GetTuple1(id));
        ghostIds.push_back(id);
      }
    }
  }
}

//----------------------------------------------------------------------------
void vtkDIYSyncGhostCellsAttributesUtilities::Sync(vtkDataObject* input, vtkMultiProcessController* controller)
{
  diy::mpi::communicator comm = vtkDIYUtilities::GetCommunicator(controller);

  int numberOfProcesses = controller->GetNumberOfProcesses();
  int processId = controller->GetLocalProcessId();
  vtkIdType offset = processId * this->GhostTypes.size();
  std::vector<vtkIdType> localNumberOfGhostsPerType (this->GhostTypes.size() * numberOfProcesses, 0),
                         numberOfGhostsPerType (localNumberOfGhostsPerType.size(), 0);
  for (auto&& att : attributes)
  {
    localNumberOfGhostsPerType[offset + att.type] = ghostIdsMap.size();
  }

  controller->AllReduce(localNumberOfGhostsPerType.data(), numberOfGhostsPerType.data(), numberOfGhostsPerType.size(), vtkCommunicator::MAX_OP);

  // We transform numberOfGhostsPerType numberOfGhostsPerType[aProcessId-1] is the index of the
  // elements belonging to process aProcessId
  for (std::size_t i = 1; i < numberOfGhostsPerType.size(); ++i)
  {
    numberOfGhostsPerType[i] += numberOfGhostsPerType[i-1];
  }
  vtkIdType totalNumberOfGhosts = numberOfGhostsPerType[numberOfGhostsPerType.size()-1];

  std::vector<vtkIdType> localGlobalIds (totalNumberOfGhosts, 0),
                         globalIds (totalNumberOfGhosts, 0);

  // We put the global ids in the correct location in the array we will share with other processes
  vtkIdType startIndex = numberOfGhostsPerType[processId ? processId - 1 : processId];
  std::memcpy(localGlobalIds.data() + startIndex, ghostGlobalIds.data(), ghostGlobalIds.size());

  controller->AllReduce(localGlobalIds.data(), globalIds.data(), globalIds.size(), vtkCommunicator::MAX_OP);


}
