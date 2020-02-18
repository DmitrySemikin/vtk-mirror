/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHyperTreeGrid.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkHyperTreeGrid
 * @brief   A dataset containing a grid of vtkHyperTree instances
 * arranged as a rectilinear grid.
 *
 * vtkHyperTreeGrid is an implementation of the following paper:
 * 
 * Visualization and Analysis of Large-Scale, Tree-Based, Adaptive
 * MeshRefinement Simulations with Arbitrary Rectilinear Geometry.
 *
 * Guénolé Harel, Jacques-Bernard Lekien, Philippe P. Pébaÿ
 *
 * A vtkHyperTreeGrid is a vtkDataObject containing a rectilinear grid of root nodes,
 * each of which can be refined as a vtkHyperTree. Please refer to the vtkHyperTree
 * documentation for deeper insight on hypertrees. This organization of the
 * root nodes allows for the definition of tree-based AMR grids that do not have
 * uniform geometry.
 * Usually, filters need a specific implementation for hyper tree grids.
 * In VTK, they all start with the prefix vtkHyperTreeGrid in their name.
 *
 * The size of a vtkHyperTreeGrid is set by its dimensions. The dimensions that the user
 * can set actually refer to the underlying vertices of the grid of hypertrees.
 * One can infer the number of hypertrees per dimensiosn by considering the dual
 * of the grid: there are one less hypertrees than points per dimension. One has
 * a handle on the number of hypertrees with the method GetCellDims.
 *
 * By convention, if the hypertree grid is not 3D, each unused dimension has its
 * cell dimensions as well as point dimensions set to one. The user should not
 * worry about updating cell dimensions, they are automatically updated
 * when one sets the point dimneions (by calling SetDimensions).
 *
 * The ordering of the hypertree grid is the following: x grows faster than y,
 * which grows faster than z.
 *
 * @warning
 * It is not a spatial search object. If you are looking for this kind of
 * octree see vtkCellLocator instead.
 *
 * @sa
 * vtkHyperTree vtkRectilinearGrid
 *
 * @par Thanks:
 * This class was written by Philippe Pebay, Joachim Pouderoux, and Charles Law, Kitware 2013
 * This class was modified by Guenole Harel and Jacques-Bernard Lekien 2014
 * This class was rewritten by Philippe Pebay, 2016
 * This class was modified by Jacques-Bernard Lekien 2018
 * This work was supported by Commissariat a l'Energie Atomique
 * CEA, DAM, DIF, F-91297 Arpajon, France.
 */

#ifndef vtkHyperTreeGrid_h
#define vtkHyperTreeGrid_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkDataObject.h"
#include "vtkNew.h"          // vtkSmartPointer
#include "vtkSmartPointer.h" // vtkSmartPointer

#include <cassert> // std::assert
#include <limits>  // For vtkHyperTreeGrid::InvalidIndex
#include <map>     // For iterators
#include <memory>  // std::shared_ptr

class vtkBitArray;
class vtkBoundingBox;
class vtkCellLinks;
class vtkCollection;
class vtkDataArray;
class vtkHyperTree;
class vtkHyperTreeGridNonOrientedCursor;
class vtkHyperTreeGridNonOrientedGeometryCursor;
class vtkHyperTreeGridNonOrientedVonNeumannSuperCursor;
class vtkHyperTreeGridNonOrientedVonNeumannSuperCursorLight;
class vtkHyperTreeGridNonOrientedMooreSuperCursor;
class vtkHyperTreeGridNonOrientedMooreSuperCursorLight;
class vtkHyperTreeGridOrientedCursor;
class vtkHyperTreeGridOrientedGeometryCursor;
class vtkDoubleArray;
class vtkDataSetAttributes;
class vtkIdTypeArray;
class vtkLine;
class vtkPixel;
class vtkPoints;
class vtkPointData;
class vtkUnsignedCharArray;

class VTKCOMMONDATAMODEL_EXPORT vtkHyperTreeGrid : public vtkDataObject
{
public:
  static vtkInformationIntegerKey* LEVELS();
  static vtkInformationIntegerKey* DIMENSION();
  static vtkInformationIntegerKey* ORIENTATION();
  static vtkInformationDoubleVectorKey* SIZES();
  static vtkHyperTreeGrid* New();

  vtkTypeMacro(vtkHyperTreeGrid, vtkDataObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Invalid index that is returned for undefined nodes, for example for nodes that are out of
   * bounds (they can exist with the super cursors).
   */
  static constexpr vtkIdType InvalidIndex = std::numeric_limits<vtkIdType>::min();

  /**
   * Set/Get mode squeeze
   */
  vtkSetStringMacro(ModeSqueeze); // By copy
  vtkGetStringMacro(ModeSqueeze);

  /**
   * Squeeze this representation.
   */
  virtual void Squeeze();

  /**
   * Return what type of dataset this is.
   */
  int GetDataObjectType() override { return VTK_HYPER_TREE_GRID; }

  /**
   * Copy the internal geometric and topological structure of a
   * vtkHyperTreeGrid object.
   */
  virtual void CopyStructure(vtkDataObject*);

  /**
   * @deprecated Replaced by CopyStructure as of VTK 10.0
   */
  VTK_LEGACY(virtual void CopyEmptyStructure(vtkDataObject*));

  // --------------------------------------------------------------------------
  // RectilinearGrid common API
  // --------------------------------------------------------------------------

  //@{
  /**
   * Set/Get sizes of this rectilinear grid dataset
   *
   * @warning The actual number of hypertrees in the hypertree grid relies
   * on the dual grid (of cells) of this grid (of points) being set here.
   */
  void SetDimensions(const unsigned int dims[3]);
  void SetDimensions(const int dims[3]);
  void SetDimensions(unsigned int i, unsigned int j, unsigned int k);
  void SetDimensions(int i, int j, int k);
  //@}

  //@{
  /**
   * Get dimensions of this rectilinear grid dataset.
   * The dimensions correspond to the number of points, i.e. the dual grid dimension
   * of the hypertree grid.
   */
  const int* GetDimensions() const VTK_SIZEHINT(3);
  void GetDimensions(int dim[3]) const;
  VTK_LEGACY(void GetDimensions(unsigned int dim[3]) const);
  //@}

  //@{
  /**
   * Different ways to set the extent of the data array. The extent
   * should be set before the data fields are set or allocated.
   * The Extent is stored in the order (X, Y, Z).
   * Set/Get extent of this rectilinear grid dataset.
   */
  void SetExtent(const int extent[6]);
  void SetExtent(int x1, int x2, int y1, int y2, int z1, int z2);
  vtkGetVector6Macro(Extent, int);
  //@}

  //@{
  /**
   * Get hypertree grid dimensions, which is the dual grid of the grid
   * set by vtkHyperTreeGrid::SetDimensions(int[3]).
   * By convension, if the hypertree grid is of dimensions less than 3D, the
   * corresponding CellDims have a dimension of one.
   *
   * @warning Do not confuse these values with the ones of vtkHyperTreeGrid::GetDimensions()
   */
  const int* GetCellDims() const VTK_SIZEHINT(3);
  void GetCellDims(int cellDims[3]) const;
  VTK_LEGACY(void GetCellDims(unsigned int cellDims[3]) const);
  //@}

  /**
   * Get the dimensionality of the grid deduced when setting
   * Dimensions or Extent. Given 0<i<=3, For each Dimensions[i]
   * equal to 1, or for each Extent[2*i+1] - Extent[2*i] equal to 0,
   * the hypertree grid dimension is reduced by one.
   */
  unsigned int GetDimension() const { return this->Dimension; }

  /**
   * @deprecated Use GetAxes instead.
   */
  VTK_LEGACY(void Get1DAxis(unsigned int& axis) const);

  /**
   * @deprecated Use GetAxes instead
   */
  VTK_LEGACY(void Get2DAxes(unsigned int& axis1, unsigned int& axis2) const);

  /**
   * Returns the array of axes used to span the hypertree grid.
   * The returned pointer is an array of 2 unsigned integers in [0,2].
   * These integers tell which dimension in (x,y,z) is used to span the hypertree grid.
   *
   * If the inner dimension is 1D, only GetAxes()[0] should be regarded.
   * If the inner dimension is 2D, (GetAxes()[0], GetAxes()[1]) form a direct frame.
   *
   * @note GetDimension() gives a handle on the inner dimension of the hypertree grid.
   */
  const unsigned* GetAxes() const { return this->Axis; }

  /**
   * Returns the number of children each node can have.
   * This number will vary depending of the inner dimension of the hypertree grid as well as the
   * branch factor of its hypertrees.
   */
  unsigned int GetNumberOfChildren() const { return this->NumberOfChildren; }

  //@{
  /**
   * Specify whether indexing mode of grid root cells must be transposed to
   * x-axis first, z-axis last, instead of the default z-axis first, x-axis last
   */
  vtkSetMacro(TransposedRootIndexing, bool);
  vtkGetMacro(TransposedRootIndexing, bool);
  void SetIndexingModeToKJI() { this->SetTransposedRootIndexing(false); }
  void SetIndexingModeToIJK() { this->SetTransposedRootIndexing(true); }
  //@}

  /**
   * Get the orientation of 1D or 2D grids:
   * . in 1D: 0, 1, 2 = aligned along X, Y, Z axis
   * . in 2D: 0, 1, 2 = normal to X, Y, Z axis
   *
   * @warning This method is irrelevant in 3D.
   */
  unsigned int GetOrientation() const { return this->Orientation; }

  /**
   * Getter on the frozen state of the hypertree grid.
   */
  vtkGetMacro(FreezeState, bool);

  //@{
  /**
   * Set/Get the subdivision factor in the grid refinement scheme
   */
  void SetBranchFactor(unsigned int);
  unsigned int GetBranchFactor() const { return this->BranchFactor; }
  //@}

  /**
   * Return the maximum number of trees in the depth 0 grid.
   */
  vtkIdType GetMaxNumberOfTrees();

  /**
   * Get the number of vertices in the primal tree grid.
   */
  vtkIdType GetNumberOfVertices();

  /**
   * Get the number of leaves in the primal tree grid.
   */
  vtkIdType GetNumberOfLeaves();

  /**
   * Return the number of levels in an individual (primal) tree.
   */
  unsigned int GetNumberOfLevels(vtkIdType);

  /**
   * Return the number of levels in the hyper tree grid.
   */
  unsigned int GetNumberOfLevels();

  //@{
  /**
   * Set/Get the grid coordinates in the x-direction.
   */
  virtual void SetXCoordinates(vtkDataArray*);
  vtkGetObjectMacro(XCoordinates, vtkDataArray);
  //@}

  //@{
  /**
   * Set/Get the grid coordinates in the y-direction.
   */
  virtual void SetYCoordinates(vtkDataArray*);
  vtkGetObjectMacro(YCoordinates, vtkDataArray);
  //@}

  //@{
  /**
   * Set/Get the grid coordinates in the z-direction.
   */
  virtual void SetZCoordinates(vtkDataArray*);
  vtkGetObjectMacro(ZCoordinates, vtkDataArray);
  //@}

  //@{
  /**
   * JB Augented services on Coordinates.
   */
  virtual void CopyCoordinates(const vtkHyperTreeGrid* output);
  virtual void SetFixedCoordinates(unsigned int axis, double value);
  //@}

  //@{
  /**
   * Set/Get the blanking mask of primal leaf cells
   */
  void SetMask(vtkBitArray*);
  vtkGetObjectMacro(Mask, vtkBitArray);
  //@}

  /**
   * Determine whether blanking mask is empty or not
   */
  bool HasMask();

  //@{
  /**
   * Set/Get presence or absence of interface
   */
  vtkSetMacro(HasInterface, bool);
  vtkGetMacro(HasInterface, bool);
  vtkBooleanMacro(HasInterface, bool);
  //@}

  //@{
  /**
   * Set/Get names of interface normal vectors arrays
   */
  vtkSetStringMacro(InterfaceNormalsName);
  vtkGetStringMacro(InterfaceNormalsName);
  //@}

  //@{
  /**
   * Set/Get names of interface intercepts arrays
   */
  vtkSetStringMacro(InterfaceInterceptsName);
  vtkGetStringMacro(InterfaceInterceptsName);
  //@}

  //@{
  /**
   * Set/Get depth limiter value
   */
  vtkSetMacro(DepthLimiter, unsigned int);
  vtkGetMacro(DepthLimiter, unsigned int);
  //@}

  //@{
  /**
   * This method initializes a cursor. A cursor is a tool allowing to walk through a hypertree or
   * a hypertree grid. There are different types of cursors:
   *
   * * An oriented cursor. This is the simplest and the lightest one. It allows to walk in deeper
   * levels, while forgetting how to go back to the parents of the nodes.
   * * An oriented geometry cursor. This cursor has the same properties as the oriented cursor plus
   * added features. One can get the bounding box of the current node pointed by the cursor.
   * * A non oriented cursor. This cursor allows to go back and forth in a hyper tree.
   * * A non oriented geometry cursor. This cursors is a geometry cursor allowing to go back and
   * forth in a hypertree.
   * * Super cursors. Super cursors are cursors allowing to jump / search in the
   * neighboring hypertrees. The computation is a lot heavier than regular cursors, and ghost
   * trees should be computed beforehand in a multi processing environnement with
   * vtkHyperTreeGridGhostCellsGenerator. A super cursor has all the properties of all regular cursors
   * + A Von Neumann super cursor. A super cursor giving access to a one-ring neighborhood within
   * a distance of Manhattan. It is basically a "+" shaped neighborhood.
   * + A Moore super cursor. A super cursor giving access to a one-ring neighborhood withing distance
   * computed w.r.t. the infinity norm. It is basically a hypercube shaped neighborhood.
   * 
   * @param cursor An allocated cursor where all the information will be held.
   * @param index The index of the hypertree where this cursor will be created.
   * @param create A boolean flag which should be set to true if and only if one is attempting to
   * create a hypertree. This is done when constructing a hypertree grid, not when processing
   * an already made one.
   */
  void InitializeOrientedCursor(
    vtkHyperTreeGridOrientedCursor* cursor, vtkIdType index, bool create = false);
  void InitializeOrientedGeometryCursor(
    vtkHyperTreeGridOrientedGeometryCursor* cursor, vtkIdType index, bool create = false);
  void InitializeNonOrientedCursor(
    vtkHyperTreeGridNonOrientedCursor* cursor, vtkIdType index, bool create = false);
  void InitializeNonOrientedGeometryCursor(
    vtkHyperTreeGridNonOrientedGeometryCursor* cursor, vtkIdType index, bool create = false);
  void InitializeNonOrientedVonNeumannSuperCursor(
    vtkHyperTreeGridNonOrientedVonNeumannSuperCursor* cursor, vtkIdType index, bool create = false);
  void InitializeNonOrientedVonNeumannSuperCursorLight(
    vtkHyperTreeGridNonOrientedVonNeumannSuperCursorLight* cursor, vtkIdType index,
    bool create = false);
  void InitializeNonOrientedMooreSuperCursor(
    vtkHyperTreeGridNonOrientedMooreSuperCursor* cursor, vtkIdType index, bool create = false);
  void InitializeNonOrientedMooreSuperCursorLight(
    vtkHyperTreeGridNonOrientedMooreSuperCursorLight* cursor, vtkIdType index, bool create = false);
  //@}

  //@{
  /**
   * Methoes equivalent with the methods Initialize(Non)Oriented(Geometry)Cursor. Instead
   * of taking an already allocated cursor as input, it returns a pointer on a newly
   * allocated cursor.
   */
  vtkHyperTreeGridOrientedCursor* NewOrientedCursor(vtkIdType index, bool create = false);
  vtkHyperTreeGridOrientedGeometryCursor* NewOrientedGeometryCursor(
    vtkIdType index, bool create = false);
  vtkHyperTreeGridNonOrientedCursor* NewNonOrientedCursor(vtkIdType index, bool create = false);
  vtkHyperTreeGridNonOrientedGeometryCursor* NewNonOrientedGeometryCursor(
    vtkIdType index, bool create = false);
  vtkHyperTreeGridNonOrientedVonNeumannSuperCursor* NewNonOrientedVonNeumannSuperCursor(
    vtkIdType index, bool create = false);
  vtkHyperTreeGridNonOrientedVonNeumannSuperCursorLight* NewNonOrientedVonNeumannSuperCursorLight(
    vtkIdType index, bool create = false);
  vtkHyperTreeGridNonOrientedMooreSuperCursor* NewNonOrientedMooreSuperCursor(
    vtkIdType index, bool create = false);
  vtkHyperTreeGridNonOrientedMooreSuperCursorLight* NewNonOrientedMooreSuperCursorLight(
    vtkIdType index, bool create = false);
  //@}

  /**
   * @warning DO NOT USE THIS FUNCTION, work in progress. Does not work in 3D.
   */
  vtkHyperTreeGridNonOrientedGeometryCursor* FindNonOrientedGeometryCursor(double x[3]);

  //@{
  /**
   * Work in progress, ignore this code.
   */
  virtual unsigned int FindDichotomicX(double value) const;
  virtual unsigned int FindDichotomicY(double value) const;
  virtual unsigned int FindDichotomicZ(double value) const;
  //@}

  /**
   * Restore data object to initial state.
   */
  void Initialize() override;

  /**
   * Return tree located at given index of hyper tree grid
   * @note This will construct a new HyperTree if grid slot is empty.
   */
  virtual vtkHyperTree* GetTree(vtkIdType, bool create = false);

  /**
   * Assign given tree to given index of hyper tree grid
   * @note This will create a new slot in the grid if needed.
   */
  void SetTree(vtkIdType, vtkHyperTree*);

  /**
   * Create shallow copy of hyper tree grid.
   */
  void ShallowCopy(vtkDataObject*) override;

  /**
   * Create deep copy of hyper tree grid.
   */
  void DeepCopy(vtkDataObject*) override;

  /**
   * Structured extent. The extent type is a 3D extent.
   */
  int GetExtentType() override { return VTK_3D_EXTENT; }

  /**
   * Return the actual size of the data in kibibytes (1024 bytes). This number
   * is valid only after the pipeline has updated. The memory size
   * returned is guaranteed to be greater than or equal to the
   * memory required to represent the data (e.g., extra space in
   * arrays, etc. are not included in the return value). THIS METHOD
   * IS THREAD SAFE.
   */
  virtual unsigned long GetActualMemorySizeBytes();

  /**
   * Return the actual size of the data in kibibytes (1024 bytes). This number
   * is valid only after the pipeline has updated. The memory size
   * returned is guaranteed to be greater than or equal to the
   * memory required to represent the data (e.g., extra space in
   * arrays, etc. are not included in the return value). THIS METHOD
   * IS THREAD SAFE.
   */
  unsigned long GetActualMemorySize() override;

  /**
   * Recursively initialize pure material mask
   */
  bool RecursivelyInitializePureMask(
    vtkHyperTreeGridNonOrientedCursor* cursor, vtkDataArray* normale);

  /**
   * Get or create pure material mask.
   */
  vtkBitArray* GetPureMask();

  /**
   * Return hard-coded bitcode correspondng to child mask
   *
   * * Dimension 1:
   * + Factor 2:
   *
   * 0: 100, 1: 001
   * + Factor 3:
   *
   * 0: 100, 1: 010, 2: 001
   * * Dimension 2:
   * + Factor 2:
   *
   * 0: 1101 0000 0, 1: 0110 0100 0
   *
   * 2: 0001 0011 0, 3: 0000 0101 1
   * + Factor 3:
   * 0: 1101 0000 0, 1: 0100 0000 0, 2: 0110 0100 0
   *
   * 3: 0001 0000 0, 4: 0000 1000 0, 5: 0000 0100 0
   *
   * 6: 0001 0011 0, 7: 0000 0001 0, 8: 0000 0101 1
   * * Dimension 3:
   * + Factor 2:
   *
   * 0: 1101 1000 0110 1000 0000 0000 000, 1: 0110 1100 0011 0010 0000 0000 000
   *
   * 2: 0001 1011 0000 1001 1000 0000 000, 3: 0000 1101 1000 0010 1100 0000 000
   *
   * 4: 0000 0000 0110 1000 0011 0110 000, 5: 0000 0000 0011 0010 0001 1011 000
   *
   * 6: 0000 0000 0000 1001 1000 0110 110, 7: 0000 0000 0000 0010 1100 0011 011
   * + Factor 3:
   *
   * 0: 1101 1000 0110 1000 0000 0000 000
   *
   * 1: 0100 1000 0010 0000 0000 0000 000
   *
   * 2: 0110 1100 0011 0010 0000 0000 000
   *
   * 3: 0001 1000 0000 1000 0000 0000 000
   *
   * 4: 0000 1000 0000 0000 0000 0000 000
   *
   * 5: 0000 1100 0000 0010 0000 0000 000
   *
   * 6: 0001 1011 0000 1001 1000 0000 000
   *
   * 7: 0000 1001 0000 0000 1000 0000 000
   *
   * 8: 0000 1101 1000 0010 1100 0000 000
   *
   * 9: 0000 0000 0110 1000 0000 0000 000
   *
   * 10: 0000 0000 0010 0000 0000 0000 000
   *
   * 11: 0000 0000 0011 0010 0000 0000 000
   *
   * 12: 0000 0000 0000 1000 0000 0000 000
   *
   * 13: 0000 0000 0000 0100 0000 0000 000
   *
   * 14: 0000 0000 0000 0010 0000 0000 000
   *
   * 15: 0000 0000 0000 1001 1000 0000 000
   *
   * 16: 0000 0000 0000 0000 1000 0000 000
   *
   * 17: 0000 0000 0000 0010 1100 0000 000
   *
   * 18: 0000 0000 0110 1000 0011 0110 000
  *
   * 19: 0000 0000 0010 0000 0001 0010 000
   *
   * 20: 0000 0000 0011 0010 0001 1011 000
   *
   * 21: 0000 0000 0000 1000 0000 0110 000
   *
   * 22: 0000 0000 0000 0000 0000 0010 000
   *
   * 23: 0000 0000 0000 0010 0000 0011 000
   *
   * 24: 0000 0000 0000 1001 1000 0110 110
   *
   * 25: 0000 0000 0000 0000 1000 0010 010
   *
   * 26: 0000 0000 0000 0010 1100 0011 011
   */
  unsigned int GetChildMask(unsigned int);

  /**
   * Convert the Cartesian coordinates of a root in the grid  to its global index.
   * 
   * @param treeOffsetIdx Offset index pointing to the beginning of the hypertree located at (i,j,k)
   */
  void GetIndexFromLevelZeroCoordinates(vtkIdType& treeOffsetIdx, int i, int j, int k) const;

  /**
   * Return the root index of a root cell with given index displaced.
   * by a Cartesian vector in the grid.
   *
   * @param treeOffsetIdx Offset index pointing to the beginning of the hypertree located at (i,j,k)
   * @note No boundary checks are performed.
   */
  vtkIdType GetShiftedLevelZeroIndex(vtkIdType treeOffsetIdx, int i,  int j, int k) const;

  /**
   * Inverse function of GetIndexFromZeroCoordinates
   */
  void GetLevelZeroCoordinatesFromIndex(
    vtkIdType treeOffsetIdx, int& i, int& j, int& k) const;

  /**
   * Returns the size and origin of the hypertree located at (i,j,k), such that the bounding box
   * of the corresponding hypertree is (origin[0], origin[0]+size[0], origin[1], origin[1]+size[1],
   * origin[2], origin[2]+size[2]), where the ordering correspond of the one of vtkBox.
   */
  virtual void GetLevelZeroOriginAndSizeFromCoordinates(int i, int j, int k, double* origin, double* size);

  /**
   * Same method has GetLevelZeroOriginAndSizeFromCoordinates, where the coordinates (i,j,k) are replaced
   * by the tree offset index.
   */
  virtual void GetLevelZeroOriginAndSizeFromIndex(vtkIdType treeOffsetIdx, double* origin, double* size);

  /**
   * Same method as GetLevelZeroOriginAndSizeFromCoordinates, although no size is computed
   */
  virtual void GetLevelZeroOriginFromCoordinates(int i, int j, int k, double* origin);

  /**
   * Same method has GetLevelZeroOriginFromCoordinates, where the coordinates (i,j,k) are replaced
   * by the tree offset index.
   */
  virtual void GetLevelZeroOriginFromIndex(vtkIdType treeOffsetIdx, double* origin);

  /**
   * JB Retourne la valeur maximale du global index.
   * Cette information est indispensable pour construire une nouvelle
   * grandeur puisqu'elle devra au moins etre de cette taille.
   * Pour les memes raisons, dans le cas de la construction du maillage dual,
   * afin de reutiliser les grandeurs de l'HTG, le nombre de sommets
   * sera dimensionne a cette valeur.
   */
  vtkIdType GetGlobalNodeIndexMax();

  /**
   * JB Permet d'initialiser les index locaux de chacun des HT de cet HTG
   * une fois que TOUS les HTs aient ete COMPLETEMENT construits/raffines !
   * A l'utilisateur ensuite de fournir les grandeurs suivant cet ordre.
   */
  void InitializeLocalIndexNode();

  /**
   * Returns 1 if there are any ghost cells
   * 0 otherwise.
   */
  bool HasAnyGhostCells() const;

  /**
   * Accessor on ghost cells
   */
  vtkUnsignedCharArray* GetGhostCells();

  /**
   * Gets the array that defines the ghost type of each point.
   * We cache the pointer to the array to save a lookup involving string comparisons
   */
  vtkUnsignedCharArray* GetTreeGhostArray();

  /**
   * Allocate ghost array for points.
   */
  vtkUnsignedCharArray* AllocateTreeGhostArray();

  /**
   * An iterator object to iteratively access trees in the grid.
   */
  class VTKCOMMONDATAMODEL_EXPORT vtkHyperTreeGridIterator
  {
  public:
    vtkHyperTreeGridIterator() {}

    /**
     * Initialize the iterator on the tree set of the given grid.
     */
    void Initialize(vtkHyperTreeGrid*);

    /**
     * Get the next tree and set its index then increment the iterator.
     * Returns 0 at the end.
     */
    vtkHyperTree* GetNextTree(vtkIdType& index);

    /**
     * Get the next tree and set its index then increment the iterator.
     * Returns 0 at the end.
     */
    vtkHyperTree* GetNextTree();

  protected:
    std::map<vtkIdType, vtkSmartPointer<vtkHyperTree> >::iterator Iterator;
    vtkHyperTreeGrid* Grid;
  };

  /**
   * Initialize an iterator to browse level 0 trees.
   * FIXME: this method is completely unnecessary.
   */
  void InitializeTreeIterator(vtkHyperTreeGridIterator&);

  //@{
  /**
   * Retrieve an instance of this class from an information object
   */
  static vtkHyperTreeGrid* GetData(vtkInformation* info);
  static vtkHyperTreeGrid* GetData(vtkInformationVector* v, int i = 0);
  //@}

  /**
   * Return a pointer to the geometry bounding box in the form
   * (xmin,xmax, ymin,ymax, zmin,zmax).
   * THIS METHOD IS NOT THREAD SAFE.
   */
  virtual double* GetBounds() VTK_SIZEHINT(6);

  /**
   * Return a pointer to the geometry bounding box in the form
   * (xmin,xmax, ymin,ymax, zmin,zmax).
   * THIS METHOD IS NOT THREAD SAFE.
   */
  void GetBounds(double bounds[6]);

  /**
   * Get the center of the bounding box.
   * THIS METHOD IS NOT THREAD SAFE.
   */
  double* GetCenter() VTK_SIZEHINT(3);

  /**
   * Get the center of the bounding box.
   * THIS METHOD IS NOT THREAD SAFE.
   */
  void GetCenter(double center[3]);

  //@{
  /**
   * Return a pointer to this dataset's point/tree data.
   * THIS METHOD IS THREAD SAFE
   */
  vtkPointData* GetPointData();
  //@}

protected:
  /**
   * Constructor with default bounds (0,1, 0,1, 0,1).
   */
  vtkHyperTreeGrid();

  /**
   * Destructor
   */
  virtual ~vtkHyperTreeGrid() override;

  /**
   * JB ModeSqueeze
   */
  char* ModeSqueeze;

  double Bounds[6]; // (xmin,xmax, ymin,ymax, zmin,zmax) geometric bounds
  double Center[3]; // geometric center

  bool FreezeState;
  unsigned int BranchFactor; // 2 or 3
  unsigned int Dimension;    // 1, 2, or 3

  //@{
  /**
   * These arrays pointers are caches used to avoid a string comparison (when
   * getting ghost arrays using GetArray(name))
   */
  vtkUnsignedCharArray* TreeGhostArray;
  bool TreeGhostArrayCached;
  //@}
private:
  unsigned int Orientation; // 0, 1, or 2
  unsigned int Axis[2];

protected:
  unsigned int NumberOfChildren;
  bool TransposedRootIndexing;

  // --------------------------------
  // RectilinearGrid common fields
  // --------------------------------
protected:
  int DataDescription;

  bool WithCoordinates;
  vtkDataArray* XCoordinates;
  vtkDataArray* YCoordinates;
  vtkDataArray* ZCoordinates;
  // --------------------------------

  vtkBitArray* Mask;
  vtkBitArray* PureMask;
  bool InitPureMask;

  bool HasInterface;
  char* InterfaceNormalsName;
  char* InterfaceInterceptsName;


  std::map<vtkIdType, vtkSmartPointer<vtkHyperTree> > HyperTrees;

  vtkNew<vtkPointData> PointData; // Scalars, vectors, etc. associated w/ each point

  unsigned int DepthLimiter;

private:
  vtkHyperTreeGrid(const vtkHyperTreeGrid&) = delete;
  void operator=(const vtkHyperTreeGrid&) = delete;

  //@{
  /**
   * Those methods are at the proof of concept for point search in the hyper tree grid.
   * Still in "beta" version, thus private.
   */
  unsigned int RecurseDichotomic(
    double value, vtkDoubleArray* coord, unsigned int ideb, unsigned int ifin) const;
  unsigned int FindDichotomic(double value, vtkDataArray* coord) const;
  //@}

  //@{
  /**
   * Each cell maps to a hypertree. CellDims is the dimension of the dual grid of the one
   * set using SetDimensions method. Dimensions is manipulated through SetDimensions.
   * Those are private members because they work together and should not be messed with.
   * The only way to change those attributes is through SetDimensions and SetExtent.
   */
  int CellDims[3];
  int Dimensions[3];
  int Extent[6];
  //@}
};

#endif
