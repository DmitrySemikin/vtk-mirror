/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHyperTreeGridEntry.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkHyperTreeGridEntry
 * @brief   Entries are cache data for cursors
 *
 * Hyper Tree Grid Entries are a mechanism used on Hyper Tree Grid
 * to avoid inheritance overhead.
 * Entries are relevant for cursor/supercursor developers. Filter
 * developers should have a look at cursors/supercursors documentation.
 * (cf. vtkHyperTreeGridNonOrientedCursor). When writing a new cursor or
 * supercursor the choice of the entry is very important: it will drive
 * the performance and memory cost. This is even more important for
 * supercursors which have several neighbors: 6x for VonNeuman and 26x for
 * Moore in 3D with a branch factor of 2.
 *
 * Several types of Entries exist:
 *
 * - vtkHyperTreeGridEntry
 *   
 *   This cache only memorizes the current cell index in one HyperTree.
 *   Using the index, this entry provides several services:
 *   + Determine if the current cell is a leaf
 *   + Get or set global index which can be used to interact
 *     with scalar array
 *   + Descend into selected child
 *   + Subdivise the current cell (it needs to be a leaf for it to work)
 *
 *  The same services exist for all entries.
 *
 * - vtkHyperTreeGridGeometryEntry
 *
 *   This entry caches the origin coordinates of the current cell
 *   atop vtkHyperTreeGridEntry services. A getter is provided,
 *   as well as services related
 *   to the bounding box and cell center.
 *
 * - vtkHyperTreeGridLevelEntry
 *
 *   This entry offers the same services as vtkHyperTreeGridEntry, adding:
 *   - A pointer to the vtkHyperTreeGrid owning the current cell.
 *   - Access to the depth (or level) of the current cell.
 *
 * - vtkHyperTreeGridGeometryLevelEntry
 *
 *   This cache is the concatanation of vtkHyperTreeGridGeometryEntry
 *   and vtkHyperTreeGridLevelEntry.
 *
 * @sa
 * vtkHyperTreeGridEntry
 * vtkHyperTreeGridLevelEntry
 * vtkHyperTreeGridGeometryEntry
 * vtkHyperTreeGridGeometryLevelEntry
 * vtkHyperTreeGridOrientedCursor
 * vtkHyperTreeGridNonOrientedCursor
 *
 * @par Thanks:
 * This class was written by Jacques-Bernard Lekien, Jerome Dubois and
 * Guenole Harel, CEA 2018.
 * This work was supported by Commissariat a l'Energie Atomique
 * CEA, DAM, DIF, F-91297 Arpajon, France.
 */

#ifndef vtkHyperTreeGridEntry_h
#define vtkHyperTreeGridEntry_h

#ifndef __VTK_WRAP__

#include "vtkObject.h"

class vtkHyperTree;
class vtkHyperTreeGrid;

class vtkHyperTreeGridEntry
{
public:
  /**
   * Display info about the entry
   */
  void PrintSelf(ostream& os, vtkIndent indent);

  // @deprecated Replaced by PrintSelf as of VTK 9.0
  VTK_LEGACY(void Dump(ostream& os));

  /**
   * Defautl constructor
   */
  vtkHyperTreeGridEntry();

  /**
   * Constructor
   */
  vtkHyperTreeGridEntry(vtkIdType index);

  /**
   * Destructor
   */
  ~vtkHyperTreeGridEntry() = default;

  /**
   * Initialize cursor at root of given tree index in grid.
   */
  vtkHyperTree* Initialize(vtkHyperTreeGrid* grid, vtkIdType treeIndex, bool create = false);

  /**
   * Initialize cursor at root of given tree index in grid.
   */
  void Initialize(vtkIdType index) { this->Index = index; }

  /**
   * Copy function
   */
  void Copy(const vtkHyperTreeGridEntry* entry) { this->Index = entry->Index; }

  /**
   * Return the index of the current vertex in the tree.
   */
  vtkIdType GetVertexId() const { return this->Index; }

  /**
   * Return the global index for the current cell (cf. vtkHyperTree).
   * \pre not_tree: tree
   */
  VTK_LEGACY(vtkIdType GetGlobalNodeIndex(const vtkHyperTree* tree) const);
  vtkIdType GetGlobalNodeIndex(vtkHyperTree* tree) const;

  /**
   * Set the global index for the root cell of the HyperTree.
   * \pre not_tree: tree
   */
  void SetGlobalIndexStart(vtkHyperTree* tree, vtkIdType index);

  /**
   * Set the global index for the current cell of the HyperTree.
   * \pre not_tree: tree
   */
  void SetGlobalIndexFromLocal(vtkHyperTree* tree, vtkIdType index);

  /**
   * Set the blanking mask is empty or not
   * \pre not_tree: tree
   */
  void SetMask(const vtkHyperTreeGrid* grid, const vtkHyperTree* tree, bool state);

  /**
   * Determine whether blanking mask is empty or not
   * \pre not_tree: tree
   */
  bool IsMasked(const vtkHyperTreeGrid* grid, const vtkHyperTree* tree) const;

  /**
   * Is the cursor pointing to a leaf?
   * \pre not_tree: tree
   * Return true if level == grid->GetDepthLimiter()
   */
  bool IsLeaf(const vtkHyperTreeGrid* grid, const vtkHyperTree* tree, unsigned int level) const;

  /**
   * Change the current cell's status: if leaf then becomes coarse and
   * all its children are created, cf. HyperTree.
   * \pre not_tree: tree
   * \pre depth_limiter: level == grid->GetDepthLimiter()
   * \pre is_masked: IsMasked
   */
  void SubdivideLeaf(const vtkHyperTreeGrid* grid, vtkHyperTree* tree, unsigned int level);

  /**
   * Is the cursor pointing to a coarse with all childrens being leaves?
   * \pre not_tree: tree
   */
  bool IsTerminalNode(
    const vtkHyperTreeGrid* grid, const vtkHyperTree* tree, unsigned int level) const;

  /**
   * Is the cursor at HyperTree root?
   */
  bool IsRoot() const { return (this->Index == 0); }

  /**
   * Move the cursor to i-th child of the current cell.
   * \pre not_tree: tree
   * \pre not_leaf: !IsLeaf()
   * \pre valid_child: ichild>=0 && ichild<this->GetNumberOfChildren()
   * \pre depth_limiter: level == grid->GetDepthLimiter()
   * \pre is_masked: !IsMasked()
   */
  void ToChild(const vtkHyperTreeGrid* grid, const vtkHyperTree* tree, unsigned int level,
    unsigned char ichild);

protected:
  /**
   * index of the current cell in the HyperTree.
   */
  vtkIdType Index;
};

#endif // __VTK_WRAP__

#endif // vtkHyperTreeGridEntry_h
// VTK-HeaderTest-Exclude: vtkHyperTreeGridEntry.h
