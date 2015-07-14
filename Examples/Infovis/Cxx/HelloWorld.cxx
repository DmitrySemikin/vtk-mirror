#include <vtkSmartPointer.h>
#include <vtkExtentTranslator.h>
#include <assert.h>
#include <vtkTimerLog.h>


int MinimumBlockSize[3] = {8,8,8};

bool verifyValidExtent(int *ext, bool allowDuplicate, int minimumBlockSize, bool allowEmptyBlocks)
{
  //printf("%d,%d x %d,%d x %d,%d\n",ext[0],ext[1],ext[2],ext[3],ext[4],ext[5]);
  if(!allowEmptyBlocks)
    {
      if ((ext[0] == 0 && ext[1] == -1)
      || (ext[2] == 0 && ext[3] == -1)
      ||(ext[4] == 0 && ext[5] == -1) )
      {
      printf("%d,%d x %d,%d x %d,%d\n",ext[0],ext[1],ext[2],ext[3],ext[4],ext[5]);
      return false;
      }
    }
  if(!allowDuplicate)
    {
    if(ext[0] >= ext[1]
     || ext[2] >= ext[3]
     || ext[4] >= ext[5])
      {
      printf("%d,%d x %d,%d x %d,%d\n",ext[0],ext[1],ext[2],ext[3],ext[4],ext[5]);
      return false;
      }
    }
  else
    {
    if(ext[0] > ext[1]
     || ext[2] > ext[3]
     || ext[4] > ext[5])
      {
      printf("%d,%d x %d,%d x %d,%d\n",ext[0],ext[1],ext[2],ext[3],ext[4],ext[5]);
      return false;
      }
    }

  int xExt = ext[1]-ext[0] +1;
  int yExt = ext[3]-ext[2] +1;
  int zExt = ext[5]-ext[4] +1;
  if(xExt<minimumBlockSize ||
     yExt <minimumBlockSize ||
     zExt <  minimumBlockSize)
    {
    return false;
    }

  return true;
}

void TestSlabMode()
{
  int minimumBlockSize = 8;
  vtkExtentTranslator* translator = vtkExtentTranslator::New();
  bool byPoints = true;
  int startExt[6] = {0,255,0,214,0,323};
  int splitExt[6];
  bool allowDuplicate= true;
  bool allowEmptyExtent = false;
  //translator->SetUpExtent(startExt);

  int expectedTotalSize = (startExt[1]-startExt[0]+1) *(startExt[3]-  startExt[2] +1)*(startExt[5]-  startExt[4] +1);

  float blockSizesToTest[4] = {10,30,50,90};
  for(int i =0;i < 4;i++)
    {
    float blockPercentage = blockSizesToTest[i];
    //X_SLAB_MODE
    int blocks = translator->SetUpExtent(startExt,vtkExtentTranslator::X_SLAB_MODE
                                         ,blockPercentage
                                         ,MinimumBlockSize[0]
                                         ,MinimumBlockSize[1]
                                         ,MinimumBlockSize[2]);
    int totalCalculatedSize = 0;
    for(int i =0;i< blocks;i++)
      {
      translator->PieceToExtentThreadSafe(i,blocks,0,startExt,splitExt,vtkExtentTranslator::X_SLAB_MODE, static_cast<int>(byPoints));
      assert (splitExt[2] == 0 && splitExt[3] == 214 && splitExt[4] == 0 && splitExt[5] == 323 && "X_SLAB_MODE\n");
      assert(verifyValidExtent(splitExt,allowDuplicate,allowEmptyExtent,minimumBlockSize)==true && "X_SLAB_MODE\n");
      totalCalculatedSize +=(splitExt[1]-splitExt[0]+1) *(splitExt[3]-  splitExt[2] +1)*(splitExt[5]-  splitExt[4] +1);
      }
    //printf("%d,%d\n",totalCalculatedSize,expectedTotalSize);
    assert(totalCalculatedSize == expectedTotalSize && "X_SLAB_MODE\n");

    //Y_SLAB_MODE
    blocks = translator->SetUpExtent(startExt,vtkExtentTranslator::Y_SLAB_MODE
                                     ,blockPercentage
                                     ,MinimumBlockSize[0]
                                     ,MinimumBlockSize[1]
                                     ,MinimumBlockSize[2]);
    totalCalculatedSize = 0;
    for(int i =0;i< blocks;i++)
      {
      translator->PieceToExtentThreadSafe(i,blocks,0,startExt,splitExt,vtkExtentTranslator::Y_SLAB_MODE, static_cast<int>(byPoints));
      assert (splitExt[0] == 0 && splitExt[1] == 255 && splitExt[4] == 0 && splitExt[5] == 323  && "Y_SLAB_MODE\n");
      assert(verifyValidExtent(splitExt,allowDuplicate,allowEmptyExtent,minimumBlockSize)==true && "Y_SLAB_MODE\n");
      totalCalculatedSize +=(splitExt[1]-splitExt[0]+1) *(splitExt[3]-  splitExt[2] +1)*(splitExt[5]-  splitExt[4] +1);
      }
    assert(totalCalculatedSize == expectedTotalSize && "Y_SLAB_MODE\n");

    //Z_SLAB_MODE
    blocks = translator->SetUpExtent(startExt,vtkExtentTranslator::Z_SLAB_MODE
                                     ,blockPercentage
                                     ,MinimumBlockSize[0]
                                     ,MinimumBlockSize[1]
                                     ,MinimumBlockSize[2]);
    totalCalculatedSize = 0;
    for(int i =0;i< blocks;i++)
      {
      translator->PieceToExtentThreadSafe(i,blocks,0,startExt,splitExt,vtkExtentTranslator::Z_SLAB_MODE, static_cast<int>(byPoints));
      assert (splitExt[0] == 0 && splitExt[1] == 255 && splitExt[2] == 0 && splitExt[3] == 214 && "Z_SLAB_MODE\n");
      assert(verifyValidExtent(splitExt,allowDuplicate,allowEmptyExtent,minimumBlockSize)==true && "Z_SLAB_MODE\n");
      totalCalculatedSize +=(splitExt[1]-splitExt[0]+1) *(splitExt[3]-  splitExt[2] +1)*(splitExt[5]-  splitExt[4] +1);
      }
    assert(totalCalculatedSize == expectedTotalSize && "Z_SLAB_MODE\n");
    }
}

void Test2DSplitMode()
{
  int minimumBlockSize = 8;
  vtkExtentTranslator* translator = vtkExtentTranslator::New();
  bool allowDuplicate= true;
  bool byPoints = true;
  int startExt[6] = {-145,234,33,235,-148,0};
  int splitExt[6];
  int expectedTotalSize = (startExt[1]-startExt[0]+1) *(startExt[3]-  startExt[2] +1)*(startExt[5]-  startExt[4] +1);
  bool allowEmptyExtent = false;

  float blockSizesToTest[4] = {10,30,50,90};
  for(int i =0;i < 4;i++)
    {
    float blockPercentage = blockSizesToTest[i];

    //XZ_MODE
    int blocks = translator->SetUpExtent(startExt,vtkExtentTranslator::XZ_MODE
                                       ,blockPercentage
                                       ,MinimumBlockSize[0]
                                       ,MinimumBlockSize[1]
                                       ,MinimumBlockSize[2]);
    int totalCalculatedSize = 0;
    for(int i =0;i< blocks;i++)
      {
      translator->PieceToExtentThreadSafe(i,blocks,0,startExt,splitExt,vtkExtentTranslator::XZ_MODE, static_cast<int>(byPoints));
      assert(verifyValidExtent(splitExt,allowDuplicate,allowEmptyExtent,minimumBlockSize)==true && "XZ_MODE\n");
      assert (splitExt[2] == 33 && splitExt[3] == 235 && "XZ_MODE\n");

      totalCalculatedSize +=(splitExt[1]-splitExt[0]+1) *(splitExt[3]-  splitExt[2] +1)*(splitExt[5]-  splitExt[4] +1);
      }
    assert(totalCalculatedSize == expectedTotalSize && "XZ_MODE\n");

    //XY_MODE
    blocks = translator->SetUpExtent(startExt,vtkExtentTranslator::XY_MODE
                                    ,blockPercentage
                                    ,MinimumBlockSize[0]
                                    ,MinimumBlockSize[1]
                                    ,MinimumBlockSize[2]);
    totalCalculatedSize = 0;
    for(int i =0;i< blocks;i++)
      {
      translator->PieceToExtentThreadSafe(i,blocks,0,startExt,splitExt,vtkExtentTranslator::XY_MODE,static_cast<int>(byPoints));
      assert (splitExt[4] == -148 && splitExt[5] == 0 &&"XY_MODE\n");
      assert(verifyValidExtent(splitExt,allowDuplicate,allowEmptyExtent,minimumBlockSize)==true  &&"XY_MODE\n");
      totalCalculatedSize +=(splitExt[1]-splitExt[0]+1) *(splitExt[3]-  splitExt[2] +1)*(splitExt[5]-  splitExt[4] +1);
      }
    assert(totalCalculatedSize == expectedTotalSize &&"XY_MODE\n");

    //YZ_MODE
    blocks = translator->SetUpExtent(startExt,vtkExtentTranslator::YZ_MODE
                                    ,blockPercentage
                                    ,MinimumBlockSize[0]
                                    ,MinimumBlockSize[1]
                                    ,MinimumBlockSize[2]);
    totalCalculatedSize = 0;
    for(int i =0;i< blocks;i++)
      {
      translator->PieceToExtentThreadSafe(i,blocks,0,startExt,splitExt,vtkExtentTranslator::YZ_MODE,static_cast<int>(byPoints));
      assert (splitExt[0] ==-145 && splitExt[1] == 234 && "YZ_MODE\n");
      assert(verifyValidExtent(splitExt,allowDuplicate,allowEmptyExtent,minimumBlockSize)==true && "YZ_MODE\n");
      totalCalculatedSize +=(splitExt[1]-splitExt[0]+1) *(splitExt[3]-  splitExt[2] +1)*(splitExt[5]-  splitExt[4] +1);
      }
    assert(totalCalculatedSize == expectedTotalSize && "YZ_MODE\n");
    }
}

void Test3DSplitMode()
{
  int minimumBlockSize = 8;
  vtkExtentTranslator* translator = vtkExtentTranslator::New();

  bool allowDuplicate= true;
  bool byPoints = true;
  int startExt[6] = {-323, 511, -323, 127, -323, 255};
  int splitExt[6];
  float blockPercentage = 0.5;

  int expectedTotalSize = (startExt[1]-startExt[0]+1) *(startExt[3]-  startExt[2] +1)*(startExt[5]-  startExt[4] +1);

  float blockSizesToTest[4] = {10,30,50,90};
  bool allowEmptyExtent = false;

  for(int i =0;i < 1;i++)
    {
    float blockPercentage = blockSizesToTest[i];
    int blocks = translator->SetUpExtent(startExt,vtkExtentTranslator::BLOCK_MODE
                                       ,blockPercentage
                                       ,MinimumBlockSize[0]
                                       ,MinimumBlockSize[1]
                                       ,MinimumBlockSize[2]);

    int totalCalculatedSize = 0;
    for(int i =0;i< blocks;i++)
      {
      translator->PieceToExtentThreadSafe(i,blocks,0,startExt,splitExt,vtkExtentTranslator::BLOCK_MODE, static_cast<int>(byPoints));
      assert(verifyValidExtent(splitExt,allowDuplicate,allowEmptyExtent,minimumBlockSize)==true  && "BLOCKFILLING\n");
      totalCalculatedSize +=(splitExt[1]-splitExt[0]+1) *(splitExt[3]-  splitExt[2] +1)*(splitExt[5]-  splitExt[4] +1);
      }
    assert(totalCalculatedSize == expectedTotalSize && "BLOCKFILLING\n");
    }
}

void TestEmptyExtent()
{
  int blockPercentage = 50;
  bool byPoints = true;
  vtkExtentTranslator* translator = vtkExtentTranslator::New();
  int startExt[6] = {0, 0, 0, 0, 0, 0};
  int splitExt[6];
  int blocks = translator->SetUpExtent(startExt,vtkExtentTranslator::BLOCK_MODE
                                      ,blockPercentage
                                      ,MinimumBlockSize[0]
                                      ,MinimumBlockSize[1]
                                      ,MinimumBlockSize[2]);

  int expectedTotalSize = 1;
  int totalCalculatedSize = 0;
    for(int i =0;i< blocks;i++)
      {
      translator->PieceToExtentThreadSafe(i,blocks,0,startExt,splitExt,vtkExtentTranslator::BLOCK_MODE, static_cast<int>(byPoints));
      totalCalculatedSize +=(splitExt[1]-splitExt[0]+1) *(splitExt[3]-  splitExt[2] +1)*(splitExt[5]-  splitExt[4] +1);
      }
    assert(totalCalculatedSize == expectedTotalSize && "BLOCKFILLING\n");

}

int main(int, char *[] )
{
  TestSlabMode();
  Test2DSplitMode();
  Test3DSplitMode();
  TestEmptyExtent();

  printf("Testing Done\n");

  return  EXIT_SUCCESS;
}
