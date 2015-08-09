/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtentTranslator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkExtentTranslator - Generates a structured extent from unstructured.

// .SECTION Description
// vtkExtentTranslator generates a structured extent from an unstructured
// extent.  It uses a recursive scheme that splits the largest axis.  A hard
// coded extent can be used for a starting point.


#ifndef vtkExtentTranslator_h
#define vtkExtentTranslator_h

#include "vtkCommonExecutionModelModule.h" // For export macro
#include "vtkObject.h"

class vtkInformationIntegerRequestKey;
class vtkInformationIntegerKey;

class VTKCOMMONEXECUTIONMODEL_EXPORT vtkExtentTranslator : public vtkObject
{
public:
  static vtkExtentTranslator *New();

  vtkTypeMacro(vtkExtentTranslator,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the Piece/NumPieces. Set the WholeExtent and then call PieceToExtent.
  // The result can be obtained from the Extent ivar.
  vtkSetVector6Macro(WholeExtent, int);
  vtkGetVector6Macro(WholeExtent, int);
  vtkSetVector6Macro(Extent, int);
  vtkGetVector6Macro(Extent, int);
  vtkSetMacro(Piece,int);
  vtkGetMacro(Piece,int);
  vtkSetMacro(NumberOfPieces,int);
  vtkGetMacro(NumberOfPieces,int);
  vtkSetMacro(GhostLevel, int);
  vtkGetMacro(GhostLevel, int);

  // Description:
  // These are the main methods that should be called. These methods
  // are responsible for converting a piece to an extent. The signatures
  // without arguments are only thread safe when each thread accesses a
  // different instance. The signatures with arguments are fully thread
  // safe.
  virtual int PieceToExtent();
  virtual int PieceToExtentByPoints();
  virtual int PieceToExtentThreadSafe(int piece, int numPieces,
                                      int ghostLevel, int *wholeExtent,
                                      int *resultExtent, int splitMode,
                                      int byPoints);

  // Description:
  // This must be called to before the call to splitExtent.
  // This function calculates the appropiate number of blocks to split by and
  // the correct indexing scheme.
  virtual int SetUpExtent(int * ext,int splitMode, float splitPercentage, bool byPoints, int minBlockSizeX,int minBlockSizeY, int minBlockSizeZ);

  // Description:
  // How should the streamer break up extents. Block mode
  // tries to break an extent up into cube blocks.  It always chooses
  // the largest axis to split.
  // Slab mode first breaks up the Z axis.  If it gets to one slice,
  // then it starts breaking up other axes.
  void SetSplitModeToBlock()
    {this->SplitMode = vtkExtentTranslator::BLOCK_MODE;}
  void SetSplitModeToXSlab()
    {this->SplitMode = vtkExtentTranslator::X_SLAB_MODE;}
  void SetSplitModeToYSlab()
    {this->SplitMode = vtkExtentTranslator::Y_SLAB_MODE;}
  void SetSplitModeToZSlab()
    {this->SplitMode = vtkExtentTranslator::Z_SLAB_MODE;}
  vtkGetMacro(SplitMode,int);

  //Description:
  // By default the translator creates N structured subextents by repeatedly
  // splitting the largest current dimension until there are N pieces.
  // If you do not want it always split the largest dimension, for instance when the
  // shortest dimension is the slowest changing and thus least coherent in memory,
  // use this to tell the translator which dimensions to split.
  void SetSplitPath(int len, int *splitpath);

  // Don't change the numbers here - they are used in the code
  // to indicate array indices.
  enum Modes
  {
    X_SLAB_MODE =0,
    Y_SLAB_MODE =1,
    Z_SLAB_MODE =2,
    BLOCK_MODE  =3,
    XZ_MODE     =4,
    XY_MODE     =5,
    YZ_MODE     =6,
    DEFAULT_MODE=7
  };

  // Description:
  // Key used to request a particular split mode.
  // This is used by vtkStreamingDemandDrivenPipeline.
  static vtkInformationIntegerRequestKey* UPDATE_SPLIT_MODE();

protected:
  vtkExtentTranslator();
  ~vtkExtentTranslator();

  static vtkInformationIntegerKey* DATA_SPLIT_MODE();

  friend class vtkInformationSplitModeRequestKey;

  // Description:
  // Returns 0 if no data exist for a piece.
  // The whole extent Should be passed in as the extent.
  // It is modified to return the result.
  int SplitExtent(int piece, int numPieces, int *extent, int splitMode,bool byPoints );

  int Piece;
  int NumberOfPieces;
  int GhostLevel;
  int Extent[6];
  int WholeExtent[6];
  int SplitMode;

  int* SplitPath;
  int SplitLen;

  bool Initialized;

  struct BlockSizeProperties
  {
      int SplitMode;
      unsigned long MinSize[3];      // The minimum size for each block
      unsigned long  NumMicroBlocks[3]; // Number of minimum size blocks
      int NumMacroBlocks[3];    // Number of macro blocks taking into consideration the split Percentage
      int TotalMacroBlocks;     // Total macro blocks taking into consideration the split Percentage
      int MacroToMicro[3];// Number of micro blocks per macro block
  };
  struct BlockSizeProperties BlockProperties;

private:
  vtkExtentTranslator(const vtkExtentTranslator&);  // Not implemented.
  void operator=(const vtkExtentTranslator&);  // Not implemented.
};

#endif
