/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtentTranslator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkExtentTranslator.h"
#include "vtkObjectFactory.h"
#include "vtkInformationIntegerKey.h"
#include "vtkInformationIntegerRequestKey.h"
#include "vtkLargeInteger.h"
#include "limits.h"

vtkStandardNewMacro(vtkExtentTranslator);

vtkInformationKeyMacro(vtkExtentTranslator, DATA_SPLIT_MODE, Integer);

// Subclass vtkInformationIntegerRequestKey to set the DataKey.
class vtkInformationSplitModeRequestKey : public vtkInformationIntegerRequestKey
{
public:
  vtkInformationSplitModeRequestKey(const char* name, const char* location) :
    vtkInformationIntegerRequestKey(name, location)
    {
    this->DataKey = vtkExtentTranslator::DATA_SPLIT_MODE();
    }
};
vtkInformationKeySubclassMacro(vtkExtentTranslator, UPDATE_SPLIT_MODE,
                               SplitModeRequest, IntegerRequest);

//----------------------------------------------------------------------------
vtkExtentTranslator::vtkExtentTranslator()
{
  this->Piece = 0;
  this->NumberOfPieces = 0;

  this->GhostLevel = 0;

  this->Extent[0] = this->Extent[2] = this->Extent[4] = 0;
  this->Extent[1] = this->Extent[3] = this->Extent[5] = -1;
  this->WholeExtent[0] = this->WholeExtent[2] = this->WholeExtent[4] = 0;
  this->WholeExtent[1] = this->WholeExtent[3] = this->WholeExtent[5] = -1;

  // Set a default split mode to be slabs
  this->SplitMode = vtkExtentTranslator::BLOCK_MODE;

  this->SplitLen = 0;
  this->SplitPath = NULL;

  this->BlockProperties.MinSize[0] = 1;
  this->BlockProperties.MinSize[1] = 1;
  this->BlockProperties.MinSize[2] = 1;

  this->Initialized = false;
}

//----------------------------------------------------------------------------
vtkExtentTranslator::~vtkExtentTranslator()
{
  this->SetSplitPath(0, NULL);
}

//----------------------------------------------------------------------------
void vtkExtentTranslator::SetSplitPath(int len, int *sp)
{
  delete[] this->SplitPath;
  this->SplitPath = NULL;
  this->SplitLen = len;
  if (len && sp)
    {
    this->SplitPath = new int[len];
    memcpy(this->SplitPath, sp, len*sizeof(int));
    }
}
//----------------------------------------------------------------------------
int vtkExtentTranslator::PieceToExtent()
{
  return
    this->PieceToExtentThreadSafe(this->Piece, this->NumberOfPieces,
                                  this->GhostLevel, this->WholeExtent,
                                  this->Extent, this->SplitMode, 0);
}

//----------------------------------------------------------------------------
int vtkExtentTranslator::PieceToExtentByPoints()
{
  return
    this->PieceToExtentThreadSafe(this->Piece, this->NumberOfPieces,
                                  this->GhostLevel, this->WholeExtent,
                                  this->Extent, this->SplitMode, 1);
}

int vtkExtentTranslator::PieceToExtentThreadSafe(int piece, int numPieces,
                                                 int ghostLevel,
                                                 int *wholeExtent,
                                                 int *resultExtent,
                                                 int splitMode,
                                                 int byPoints)
{
  memcpy(resultExtent, wholeExtent, sizeof(int)*6);
  int ret;
  if (byPoints)
    {
    ret = this->SplitExtentByPoints(piece, numPieces, resultExtent, splitMode);
    }
  else
    {
    ret = this->SplitExtent(piece, numPieces, resultExtent, splitMode);
    }

  if (ret == 0)
    {
    // Nothing in this piece.
    resultExtent[0] = resultExtent[2] = resultExtent[4] = 0;
    resultExtent[1] = resultExtent[3] = resultExtent[5] = -1;
    return 0;
    }
  if (ghostLevel > 0)
    {
    resultExtent[0] -= ghostLevel;
    resultExtent[1] += ghostLevel;
    resultExtent[2] -= ghostLevel;
    resultExtent[3] += ghostLevel;
    resultExtent[4] -= ghostLevel;
    resultExtent[5] += ghostLevel;

    if (resultExtent[0] < wholeExtent[0])
      {
      resultExtent[0] = wholeExtent[0];
      }
    if (resultExtent[1] > wholeExtent[1])
      {
      resultExtent[1] = wholeExtent[1];
      }
    if (resultExtent[2] < wholeExtent[2])
      {
      resultExtent[2] = wholeExtent[2];
      }
    if (resultExtent[3] > wholeExtent[3])
      {
      resultExtent[3] = wholeExtent[3];
      }
    if (resultExtent[4] < wholeExtent[4])
      {
      resultExtent[4] = wholeExtent[4];
      }
    if (resultExtent[5] > wholeExtent[5])
      {
      resultExtent[5] = wholeExtent[5];
      }
    }

  return 1;
}
//----------------------------------------------------------------------------
int vtkExtentTranslator::PieceToExtentThreadSafeImaging(int piece, int numPieces,
                                                 int ghostLevel,
                                                 int *wholeExtent,
                                                 int *resultExtent,
                                                 int splitMode,
                                                 int byPoints)
{
  memcpy(resultExtent, wholeExtent, sizeof(int)*6);
  int ret;
  if (byPoints)
    {
    ret = this->SplitExtentImaging(piece, numPieces, resultExtent, splitMode, true);
    }
  else
    {
    ret = this->SplitExtentImaging(piece, numPieces, resultExtent, splitMode, false);
    }

  if (ret == 0)
    {
    // Nothing in this piece.
    resultExtent[0] = resultExtent[2] = resultExtent[4] = 0;
    resultExtent[1] = resultExtent[3] = resultExtent[5] = -1;
    return 0;
    }
  if (ghostLevel > 0)
    {
    resultExtent[0] -= ghostLevel;
    resultExtent[1] += ghostLevel;
    resultExtent[2] -= ghostLevel;
    resultExtent[3] += ghostLevel;
    resultExtent[4] -= ghostLevel;
    resultExtent[5] += ghostLevel;

    if (resultExtent[0] < wholeExtent[0])
      {
      resultExtent[0] = wholeExtent[0];
      }
    if (resultExtent[1] > wholeExtent[1])
      {
      resultExtent[1] = wholeExtent[1];
      }
    if (resultExtent[2] < wholeExtent[2])
      {
      resultExtent[2] = wholeExtent[2];
      }
    if (resultExtent[3] > wholeExtent[3])
      {
      resultExtent[3] = wholeExtent[3];
      }
    if (resultExtent[4] < wholeExtent[4])
      {
      resultExtent[4] = wholeExtent[4];
      }
    if (resultExtent[5] > wholeExtent[5])
      {
      resultExtent[5] = wholeExtent[5];
      }
    }

  return 1;
}
//----------------------------------------------------------------------------
int vtkExtentTranslator::SetUpExtent(int * ext, int splitMode, float splitPercentage, bool byPoints
                                    ,int minBlockSizeX, int minBlockSizeY, int minBlockSizeZ)
{
  vtkTypeInt64 size[3];
  if (byPoints)
    {
    size[0] = ext[1] - ext[0] + 1;
    size[1] = ext[3] - ext[2] + 1;
    size[2] = ext[5] - ext[4] + 1;
    }
  else
    {
    size[0] = ext[1] - ext[0];
    size[1] = ext[3] - ext[2];
    size[2] = ext[5] - ext[4];
    }

  BlockSizeProperties * properties = &this->BlockProperties;

  properties->MinSize[0] = minBlockSizeX;
  properties->MinSize[1] = minBlockSizeY;
  properties->MinSize[2] = minBlockSizeZ;
  switch (splitMode)
    {
    case X_SLAB_MODE:
      {
      properties->MinSize[1] = size[1];
      properties->MinSize[2] = size[2];
      break;
      }
    case Y_SLAB_MODE:
      {
      properties->MinSize[0] = size[0];
      properties->MinSize[2] = size[2];
      break;
      }
    case Z_SLAB_MODE:
      {
      properties->MinSize[0] = size[0];
      properties->MinSize[1] = size[1];
      break;
      }
    case XZ_MODE:
      {
      properties->MinSize[1] = size[1];
      break;
      }
    case XY_MODE:
      {
      properties->MinSize[2] = size[2];
      break;
      }
    case YZ_MODE:
      {
      properties->MinSize[0] = size[0];
      break;
      }
    case DEFAULT_MODE:
      {
      if (size[2] != 1) //Z_SLAB_MODE
        {
        properties->MinSize[0] = size[0];
        properties->MinSize[1] = size[1];
        break;
        }
      else if (size[1] != 1) //Y_SLAB_MODE
        {
        properties->MinSize[0] = size[0];
        properties->MinSize[2] = size[2];
        break;
        }
      else
        {
        properties->MinSize[1] = size[1];
        properties->MinSize[2] = size[2];
        break;
        }
      }
    }

  vtkTypeInt64 minSize[3] = {properties->MinSize[0]
                 ,properties->MinSize[1]
                 ,properties->MinSize[2]};

  int startExt[6]= {ext[0], ext[1], ext[2], ext[3], ext[4], ext[5]};

  vtkTypeInt64 blocks[3];
  float dimensionsToSplit = 0;
  for (int i=0; i < 3; i++)
    {
    vtkTypeInt64 block = size[i] / minSize[i];
    if (block == 0 || block == 1)
      {
      block = 1;
      }
    else
      {
      dimensionsToSplit++;
      }
    blocks[i] = block;
    }
  if (dimensionsToSplit == 0) // there is only 1 piece
    {
    splitPercentage = 100.0;
    }
  else
    {
    splitPercentage = pow(splitPercentage,1.0 / dimensionsToSplit);
    }

  for (int i =0; i < 3; i++)
    {
    properties->NumMicroBlocks[i] = blocks[i];
    vtkTypeInt64 pieces = ceil(splitPercentage / 100.0 * static_cast <float>(blocks[i]));

    if (pieces > INT_MAX)
      {
      vtkErrorMacro("There are too many blocks with the current configuration.");
      return -1;
      }

    int Pieces = pieces;
    properties->NumMacroBlocks[i] = Pieces;
    properties->MacroToMicro[i] = blocks[i] / Pieces;
    }
  vtkTypeInt64 totalPieces =properties->NumMacroBlocks[0] * properties->NumMacroBlocks[1] * properties->NumMacroBlocks[2];
  if (totalPieces > INT_MAX)
      {
      vtkErrorMacro("There are too many blocks with the current configuration.");
      return -1;
      }
  properties->TotalMacroBlocks = totalPieces;

  this->Initialized = true;

  this->NumberOfPieces = properties->TotalMacroBlocks;
  return properties->TotalMacroBlocks;
}

//----------------------------------------------------------------------------
int vtkExtentTranslator::SplitExtentImaging(int piece, int numPieces, int *ext,
                                     int splitMode, bool byPoints)
{
  if (!Initialized)
    {
    vtkErrorMacro("SplitExtent has not being initialized.");
    return -1;
    }

  int sX = ext[1] - ext[0] + 1;
  int sY = ext[3] - ext[2] + 1;
  int sZ = ext[5] - ext[4] + 1;

  int minSize[3] = {this->BlockProperties.MinSize[0]
                 ,this->BlockProperties.MinSize[1]
                 ,this->BlockProperties.MinSize[2]};

  //Rotate axis based on whether blockmode, xy ,xz or yz split
  int planeAxis;
  int strideAxis;
  int blockAxis;

  if ((sX != 1 && sY != 1 && sZ != 1)
    ||(sX != 1 && sY != 1 && sZ == 1))
    {
    planeAxis = 2;
    strideAxis = 1;
    blockAxis = 0;
    }
  else if (sX != 1 && sY == 1 && sZ != 1)
    {
    planeAxis = 1;
    strideAxis = 2;
    blockAxis = 0;
    }
  else if (sX != 1 && sY == 1 && sZ != 1)
    {
    planeAxis = 1;
    strideAxis = 2;
    blockAxis = 0;
    }
  else if (sX == 1 && sY != 1 && sZ != 1)
    {
    planeAxis = 0;
    strideAxis = 2;
    blockAxis = 1;
    }
  else // when there is only 1 piece
    {
    planeAxis = 2;
    strideAxis = 1;
    blockAxis = 0;
    }

  // Find Appropiate split
  int size[3] = {this->BlockProperties.NumMacroBlocks[0]
                ,this->BlockProperties.NumMacroBlocks[1]
                ,this->BlockProperties.NumMacroBlocks[2]};

  int startExt[6] = {ext[0], ext[1], ext[2], ext[3], ext[4], ext[5]};
  int targetPiece = piece;

  //Resize MinBlockSize based on choosen blocks

  //Find Plane Axis Index
  int numberOfBlocksPerPlane = size[strideAxis] * size[blockAxis];
  int planeOffset = targetPiece / numberOfBlocksPerPlane;

  int macroToMicroModifer = this->BlockProperties.MacroToMicro[planeAxis];

  ext[planeAxis * 2] = startExt[planeAxis * 2] + planeOffset * minSize[planeAxis] * macroToMicroModifer;
  if (byPoints)
    {
    ext[planeAxis * 2 + 1] = ext[planeAxis * 2] + minSize[planeAxis] * macroToMicroModifer - 1;
    }
  else
    {
    ext[planeAxis * 2 + 1] = ext[planeAxis * 2] + minSize[planeAxis] * macroToMicroModifer;  // shared points between pieces
    }

  if (planeOffset == this->BlockProperties.NumMacroBlocks[planeAxis] - 1)
    {
    ext[planeAxis * 2 + 1] = startExt[planeAxis * 2 + 1];
    }

  targetPiece = targetPiece - numberOfBlocksPerPlane*planeOffset;
  //Find Stride Axis Index

  int numberOfBlocksPerStride = size[blockAxis];
  int strideOffset = targetPiece / numberOfBlocksPerStride;

  macroToMicroModifer = this->BlockProperties.MacroToMicro[strideAxis];

  ext[strideAxis * 2] = startExt[strideAxis * 2]+ strideOffset * minSize[strideAxis] * macroToMicroModifer;
  if (byPoints)
    {
    ext[strideAxis * 2 + 1] = ext[strideAxis * 2] + minSize[strideAxis] * macroToMicroModifer - 1;
    }
  else
    {
    ext[strideAxis * 2 + 1] = ext[strideAxis * 2] + minSize[strideAxis] * macroToMicroModifer; // shared points between pieces
    }

  if (strideOffset == this->BlockProperties.NumMacroBlocks[strideAxis] - 1)
    {
    ext[strideAxis * 2 + 1] = startExt[strideAxis * 2 + 1];
    }

  targetPiece = targetPiece - numberOfBlocksPerStride*strideOffset;

  // Find BlockOffset
  macroToMicroModifer = this->BlockProperties.MacroToMicro[blockAxis];
  ext[blockAxis * 2] = startExt[blockAxis * 2] + targetPiece * minSize[blockAxis] * macroToMicroModifer;

  if (byPoints)
    {
    ext[blockAxis * 2 + 1] = ext[blockAxis * 2] + minSize[blockAxis] * macroToMicroModifer - 1;
    }
  else
    {
    ext[blockAxis * 2 + 1] = ext[blockAxis * 2] + minSize[blockAxis] * macroToMicroModifer; // shared points between pieces
    }

  if (targetPiece == this->BlockProperties.NumMacroBlocks[blockAxis] - 1)
    {
    ext[blockAxis * 2 + 1] = startExt[blockAxis * 2 + 1];
    }
  return 1;
}

int vtkExtentTranslator::SplitExtent(int piece, int numPieces, int *ext,
                                     int splitMode)
{
  int numPiecesInFirstHalf;
  unsigned long size[3];
  int splitAxis;
  vtkLargeInteger mid;

  if (piece >= numPieces || piece < 0)
    {
    return 0;
    }

  // keep splitting until we have only one piece.
  // piece and numPieces will always be relative to the current ext.
  int cnt = 0;
  while (numPieces > 1)
    {
    // Get the dimensions for each axis.
    size[0] = ext[1]-ext[0];
    size[1] = ext[3]-ext[2];
    size[2] = ext[5]-ext[4];
    // choose what axis to split on based on the SplitMode
    // if the user has requested x, y, or z slabs then try to
    // honor that request. If that axis is already split as
    // far as it can go, then drop to block mode.
    if (this->SplitPath && cnt<this->SplitLen)
      {
      splitMode = this->SplitPath[cnt];
      cnt++;
      }
    if (splitMode < 3 && size[splitMode] > 1)
      {
      splitAxis = splitMode;
      }
    // otherwise use block mode
    else
      {
      // choose the biggest axis
      if (size[2] >= size[1] && size[2] >= size[0] && size[2]/2 >= 1)
        {
        splitAxis = 2;
        }
      else if (size[1] >= size[0] && size[1]/2 >= 1)
        {
        splitAxis = 1;
        }
      else if (size[0]/2 >= 1)
        {
        splitAxis = 0;
        }
      else
        {
        // signal no more splits possible
        splitAxis = -1;
        }
      }

    if (splitAxis == -1)
      {
      // can not split any more.
      if (piece == 0)
        {
        // just return the remaining piece
        numPieces = 1;
        }
      else
        {
        // the rest must be empty
        return 0;
        }
      }
    else
      {
      // split the chosen axis into two pieces.
      numPiecesInFirstHalf = (numPieces / 2);
      mid = size[splitAxis];
      mid = (mid *  numPiecesInFirstHalf) / numPieces + ext[splitAxis*2];
      if (piece < numPiecesInFirstHalf)
        {
        // piece is in the first half
        // set extent to the first half of the previous value.
        ext[splitAxis*2+1] = mid.CastToInt();
        // piece must adjust.
        numPieces = numPiecesInFirstHalf;
        }
      else
        {
        // piece is in the second half.
        // set the extent to be the second half. (two halves share points)
        ext[splitAxis*2] = mid.CastToInt();
        // piece must adjust
        numPieces = numPieces - numPiecesInFirstHalf;
        piece -= numPiecesInFirstHalf;
        }
      }
    } // end of while

  return 1;
}



//----------------------------------------------------------------------------
int vtkExtentTranslator::SplitExtentByPoints(int piece, int numPieces,
                                             int *ext, int splitMode)
{
  int numPiecesInFirstHalf;
  int size[3], splitAxis;
  vtkLargeInteger mid;

  // keep splitting until we have only one piece.
  // piece and numPieces will always be relative to the current ext.
  while (numPieces > 1)
    {
    // Get the dimensions for each axis.
    size[0] = ext[1]-ext[0] + 1;
    size[1] = ext[3]-ext[2] + 1;
    size[2] = ext[5]-ext[4] + 1;
    // choose what axis to split on based on the SplitMode
    // if the user has requested x, y, or z slabs then try to
    // honor that request. If that axis is already split as
    // far as it can go, then drop to block mode.
    if (splitMode < 3 && size[splitMode] > 1)
      {
      splitAxis = splitMode;
      }
    // otherwise use block mode
    else
      {
      if (size[2] >= size[1] && size[2] >= size[0] && size[2]/2 >= 1)
        {
        splitAxis = 2;
        }
      else if (size[1] >= size[0] && size[1]/2 >= 1)
        {
        splitAxis = 1;
        }
      else if (size[0]/2 >= 1)
        {
        splitAxis = 0;
        }
      else
        {
        // signal no more splits possible
        splitAxis = -1;
        }
      }

    if (splitAxis == -1)
      {
      // can not split any more.
      if (piece == 0)
        {
        // just return the remaining piece
        numPieces = 1;
        }
      else
        {
        // the rest must be empty
        return 0;
        }
      }
    else
      {
      // split the chosen axis into two pieces.
      numPiecesInFirstHalf = (numPieces / 2);
      mid = size[splitAxis];
      mid = (mid *  numPiecesInFirstHalf) / numPieces + ext[splitAxis*2];
      if (piece < numPiecesInFirstHalf)
        {
        // piece is in the first half
        // set extent to the first half of the previous value.
        ext[splitAxis*2+1] = mid.CastToInt() - 1;
        // piece must adjust.
        numPieces = numPiecesInFirstHalf;
        }
      else
        {
        // piece is in the second half.
        // set the extent to be the second half.
        ext[splitAxis*2] = mid.CastToInt();
        // piece must adjust
        numPieces = numPieces - numPiecesInFirstHalf;
        piece -= numPiecesInFirstHalf;
        }
      }
    } // end of while

  return 1;
}

//----------------------------------------------------------------------------
void vtkExtentTranslator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Piece: " << this->Piece << endl;
  os << indent << "NumberOfPieces: " << this->NumberOfPieces << endl;

  os << indent << "GhostLevel: " << this->GhostLevel << endl;

  os << indent << "Extent: " << this->Extent[0] << ", "
     << this->Extent[1] << ", " << this->Extent[2] << ", "
     << this->Extent[3] << ", " << this->Extent[4] << ", "
     << this->Extent[5] << endl;

  os << indent << "WholeExtent: " << this->WholeExtent[0] << ", "
     << this->WholeExtent[1] << ", " << this->WholeExtent[2] << ", "
     << this->WholeExtent[3] << ", " << this->WholeExtent[4] << ", "
     << this->WholeExtent[5] << endl;

  os << indent << "SplitMode: ";
  if (this->SplitMode == vtkExtentTranslator::BLOCK_MODE)
    {
    os << "Block\n";
    }
  else if (this->SplitMode == vtkExtentTranslator::X_SLAB_MODE)
    {
    os << "X Slab\n";
    }
  else if (this->SplitMode == vtkExtentTranslator::Y_SLAB_MODE)
    {
    os << "Y Slab\n";
    }
  else if (this->SplitMode == vtkExtentTranslator::Z_SLAB_MODE)
    {
    os << "Z Slab\n";
    }
  else
    {
    os << "Unknown\n";
    }
}
