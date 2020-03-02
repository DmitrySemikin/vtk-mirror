/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDIYSyncGhostCellsAttributesUtilities.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkDIYSyncGhostCellsAttributesUtilities
 * @brief collection of utility functions for DIY-based KdTree algorithm
 *
 * vtkDIYSyncGhostCellsAttributesUtilities is intended for use by vtkRedistributeDataSetFilter. It
 * encapsulates invocation of DIY algorithms for various steps in the
 * vtkRedistributeDataSetFilter.
 */

#ifndef vtkDIYSyncGhostCellsAttributesUtilities_h
#define vtkDIYSyncGhostCellsAttributesUtilities_h

#include "vtkFiltersParallelDIY2Module.h" // for export macros
#include "vtkObject.h"

#include <set>

class vtkDataObject;
class vtkMultiProcessController;
class vtkPartitionedDataSet;
class vtkPoints;
class vtkUnstructuredGrid;

class VTKFILTERSPARALLELDIY2_EXPORT vtkDIYSyncGhostCellsAttributesUtilities : public vtkObject
{
public:
  vtkTypeMacro(vtkDIYSyncGhostCellsAttributesUtilities, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Resets all attributes of this object.
   */
  void Initialize();

  void Sync(vtkDataObject* input, vtkMultiProcessController* controller = nullptr);

  //@{
  /**
   * Accessors / Setters to ghost type. Ghost types have to match the ghost type id of
   * the underlying data object, which are enumerated in vtkDataObject::AttributeTypes
   */
  void AddGhostType(int type);
  void RemoveGhostType(int type);
  bool HasGhostType(int type) const;
  const std::set<int>& GetGhostTypes() const;
  //@}

protected:
  vtkDIYSyncGhostCellsAttributesUtilities();
  ~vtkDIYSyncGhostCellsAttributesUtilities() override;

  /**
   * List of ghost types to update.
   */
  std::set<int> GhostTypes;

private:
  vtkDIYSyncGhostCellsAttributesUtilities(const vtkDIYSyncGhostCellsAttributesUtilities&) = delete;
  void operator=(const vtkDIYSyncGhostCellsAttributesUtilities&) = delete;
};

#endif
