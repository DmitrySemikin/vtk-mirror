#include <vtkSmartPointer.h>
#include <vtkExtentTranslator.h>
#include "UnitTest.h"

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


TEST(TestSlabMode)
{
  int minimumBlockSize = 8;
  vtkSmartPointer<vtkExtentTranslator> translator = vtkExtentTranslator::New();
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
      CHECK_EQUAL(splitExt[2] == 0 && splitExt[3] == 214 && splitExt[4] == 0 && splitExt[5] == 323,true);
      CHECK_EQUAL(verifyValidExtent(splitExt,allowDuplicate,allowEmptyExtent,minimumBlockSize),true);
      totalCalculatedSize +=(splitExt[1]-splitExt[0]+1) *(splitExt[3]-  splitExt[2] +1)*(splitExt[5]-  splitExt[4] +1);
      }
    //printf("%d,%d\n",totalCalculatedSize,expectedTotalSize);
    CHECK_EQUAL(totalCalculatedSize ,expectedTotalSize);

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
      CHECK_EQUAL (splitExt[0] == 0 && splitExt[1] == 255 && splitExt[4] == 0 && splitExt[5] == 323,true);
      CHECK_EQUAL(verifyValidExtent(splitExt,allowDuplicate,allowEmptyExtent,minimumBlockSize),true);
      totalCalculatedSize +=(splitExt[1]-splitExt[0]+1) *(splitExt[3]-  splitExt[2] +1)*(splitExt[5]-  splitExt[4] +1);
      }
    CHECK_EQUAL(totalCalculatedSize,expectedTotalSize);

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
      CHECK_EQUAL (splitExt[0] == 0 && splitExt[1] == 255 && splitExt[2] == 0 && splitExt[3] == 214,true);
      CHECK_EQUAL(verifyValidExtent(splitExt,allowDuplicate,allowEmptyExtent,minimumBlockSize),true);
      totalCalculatedSize +=(splitExt[1]-splitExt[0]+1) *(splitExt[3]-  splitExt[2] +1)*(splitExt[5]-  splitExt[4] +1);
      }
    CHECK_EQUAL(totalCalculatedSize,expectedTotalSize);
    }
}

TEST(Test2DSplitMode)
{
  int minimumBlockSize = 8;
  vtkSmartPointer<vtkExtentTranslator> translator = vtkExtentTranslator::New();
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
      CHECK_EQUAL(verifyValidExtent(splitExt,allowDuplicate,allowEmptyExtent,minimumBlockSize),true);
      CHECK_EQUAL (splitExt[2] == 33 && splitExt[3] == 235,true);

      totalCalculatedSize +=(splitExt[1]-splitExt[0]+1) *(splitExt[3]-  splitExt[2] +1)*(splitExt[5]-  splitExt[4] +1);
      }
    CHECK_EQUAL(totalCalculatedSize,expectedTotalSize);

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
      CHECK_EQUAL (splitExt[4] == -148 && splitExt[5] == 0,true);
      CHECK_EQUAL(verifyValidExtent(splitExt,allowDuplicate,allowEmptyExtent,minimumBlockSize),true);
      totalCalculatedSize +=(splitExt[1]-splitExt[0]+1) *(splitExt[3]-  splitExt[2] +1)*(splitExt[5]-  splitExt[4] +1);
      }
    CHECK_EQUAL(totalCalculatedSize, expectedTotalSize);

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
      CHECK_EQUAL (splitExt[0] ==-145 && splitExt[1] == 234,true);
      CHECK_EQUAL(verifyValidExtent(splitExt,allowDuplicate,allowEmptyExtent,minimumBlockSize),true);
      totalCalculatedSize +=(splitExt[1]-splitExt[0]+1) *(splitExt[3]-  splitExt[2] +1)*(splitExt[5]-  splitExt[4] +1);
      }
    CHECK_EQUAL(totalCalculatedSize,expectedTotalSize);
    }
}

TEST(Test3DSplitMode)
{
  int minimumBlockSize = 8;
  vtkSmartPointer<vtkExtentTranslator> translator = vtkExtentTranslator::New();

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
      CHECK_EQUAL(verifyValidExtent(splitExt,allowDuplicate,allowEmptyExtent,minimumBlockSize),true);
      totalCalculatedSize +=(splitExt[1]-splitExt[0]+1) *(splitExt[3]-  splitExt[2] +1)*(splitExt[5]-  splitExt[4] +1);
      }
    CHECK_EQUAL(totalCalculatedSize,expectedTotalSize);
    }
}

TEST(TestEmptyExtent)
{
  int blockPercentage = 50;
  bool byPoints = true;
  vtkSmartPointer<vtkExtentTranslator> translator = vtkExtentTranslator::New();
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
  CHECK_EQUAL(totalCalculatedSize,expectedTotalSize && "BLOCKFILLING\n");

}

TEST_MAIN();
