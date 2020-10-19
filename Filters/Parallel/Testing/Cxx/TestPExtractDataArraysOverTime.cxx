/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestExtractDataArraysOverTime.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPExtractDataArraysOverTime.h"

#include "vtkDataArray.h"
#include "vtkDataSetAttributes.h"
#include "vtkExtractDataArraysOverTime.h"
#include "vtkExtractSelection.h"
#include "vtkExtractTimeSteps.h"
#include "vtkGenerateGlobalIds.h"
#include "vtkInformation.h"
#include "vtkMPI.h"
#include "vtkMPIController.h"
#include "vtkMathUtilities.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkPExodusIIReader.h"
#include "vtkRedistributeDataSetFilter.h"
#include "vtkSelectionNode.h"
#include "vtkSelectionSource.h"
#include "vtkTable.h"
#include "vtkTestUtilities.h"

#include <cmath>
#include <sstream>
#include <string>

#define expect(x, msg)                                                                             \
  if (!(x))                                                                                        \
  {                                                                                                \
    cerr << "rank=" << rank << ", line=" << __LINE__ << ": " msg << endl;                          \
    return false;                                                                                  \
  }

namespace
{
bool TablesAreTheSame(vtkMultiBlockDataSet* singleProcessMBS, vtkMultiBlockDataSet* multiProcessMBS)
{
  int count = 0;
  for (unsigned int blockId = 0; blockId < singleProcessMBS->GetNumberOfBlocks(); ++blockId)
  {
    vtkTable* singleProcessTable = vtkTable::SafeDownCast(singleProcessMBS->GetBlock(blockId));
    vtkTable* multiProcessTable = vtkTable::SafeDownCast(multiProcessMBS->GetBlock(blockId));
    auto singleRowData = singleProcessTable->GetRowData();
    auto multiRowData = multiProcessTable->GetRowData();
    for (int arrayId = 0; arrayId < singleRowData->GetNumberOfArrays(); ++arrayId)
    {
      vtkDataArray* singleArray =
        vtkArrayDownCast<vtkDataArray>(singleRowData->GetAbstractArray(arrayId));
      vtkDataArray* multiArray =
        vtkArrayDownCast<vtkDataArray>(multiRowData->GetAbstractArray(singleArray->GetName()));

      // There is a mismatch between global id array names being overriden by
      // vtkGenerateGlobalIds
      if (!singleArray || !multiArray)
      {
        continue;
      }

      if (!multiArray || singleArray->GetNumberOfValues() != multiArray->GetNumberOfValues())
      {
        return false;
      }
      for (vtkIdType id = 0; id < singleArray->GetNumberOfTuples(); ++id)
      {
        // We arbitrarely use 8 significative digits.
        // The way stats are currently computed result in numeric imprecision...
        // Reason is that stats are incrementally computed. At each step i of the
        // computation, the resulting statistic of the i first inputs is
        // computed. To add a new element, one need to do a lot of numeric
        // operations to retrieve a numeric state such that an element can be added.
        //
        // A better solution would be to accumulate inputs in a form such that
        // accumulating data requires a small amount of operations (three
        // maximum, ideally), and with a routine to compute the wanted
        // statistics with this accumulated data. In other words, one does not
        // carry the whole result at each step, but rather some buffer that can
        // be used to easily compute the wanted statistics. This can be easily done for
        // most statistics by expanding whatever is expandable in the formula
        // in term of monomials, then computing those monomials one by one in some
        // small buffer. The wanted statistic can be easily retrieved from it.
        //
        // In the case of the median / quantiles (any statistics not having a
        // closed form), a more involved process is
        // needed, but it can still be worked around.
        if (std::fabs(singleArray->GetTuple1(id) - multiArray->GetTuple1(id)) >
          1e-8 * std::fabs(std::max(singleArray->GetTuple1(id), multiArray->GetTuple1(id))))
        {
          return false;
        }
      }
      ++count;
    }
  }

  // We are supposed to check at least 230 arrays in our setup
  return count >= 230;
}
bool ValidateStats(vtkMultiBlockDataSet* mb, int num_timesteps, int rank)
{
  if (rank != 0)
  {
    // expecting MB with 2 empty blocks.
    expect(mb != nullptr, "expecting a vtkMultiBlockDataSet.");
    expect(mb->GetNumberOfBlocks() == 2, "expecting 2 blocks, got " << mb->GetNumberOfBlocks());
    for (int cc = 0; cc < 2; ++cc)
    {
      expect(mb->GetBlock(cc) == nullptr, "expecting null block at index : " << cc);
    }
    return true;
  }

  expect(mb != nullptr, "expecting a vtkMultiBlockDataSet.");
  expect(mb->GetNumberOfBlocks() == 2, "expecting 2 blocks, got " << mb->GetNumberOfBlocks());
  for (int cc = 0; cc < 2; ++cc)
  {
    vtkTable* b0 = vtkTable::SafeDownCast(mb->GetBlock(0));
    expect(b0 != nullptr, "expecting a vtkTable for block " << cc);
    expect(b0->GetNumberOfRows() == num_timesteps,
      "mismatched rows, expecting " << num_timesteps << ", got " << b0->GetNumberOfRows()
                                    << "for block " << cc);
    expect(b0->GetNumberOfColumns() > 100, "mismatched columns in block " << cc);
    expect(b0->GetColumnByName("max(DISPL (0))") != nullptr,
      "missing 'max(DISPL (0))' for block " << cc);
  }
  return true;
}

bool ValidateGID(vtkMultiBlockDataSet* mb, int num_timesteps, const char* bname, int rank)
{
  if (rank != 0)
  {
    // expecting MB with 1 empty blocks.
    expect(mb != nullptr, "expecting a vtkMultiBlockDataSet.");
    expect(mb->GetNumberOfBlocks() == 1, "expecting 1 blocks, got " << mb->GetNumberOfBlocks());
    expect(mb->GetBlock(0) == nullptr, "expecting null block at index 0.");
    return true;
  }

  expect(mb != nullptr, "expecting a vtkMultiBlockDataSet.");
  expect(mb->GetNumberOfBlocks() == 1, "expecting 1 block, got " << mb->GetNumberOfBlocks());

  vtkTable* b0 = vtkTable::SafeDownCast(mb->GetBlock(0));
  expect(b0 != nullptr, "expecting a vtkTable for block 0");
  expect(b0->GetNumberOfRows() == num_timesteps,
    "mismatched rows, expecting " << num_timesteps << ", got " << b0->GetNumberOfRows());
  expect(b0->GetNumberOfColumns() >= 5, "mismatched columns");
  expect(b0->GetColumnByName("EQPS") != nullptr, "missing EQPS.");

  const char* name = mb->GetMetaData(0u)->Get(vtkCompositeDataSet::NAME());
  expect(name != nullptr, "expecting non-null name.");
  expect(strcmp(name, bname) == 0,
    "block name not matching, expected '" << bname << "', got '" << name << "'");
  return true;
}

bool ValidateID(vtkMultiBlockDataSet* mb, int num_timesteps, const char* bname, int rank)
{
  if (rank != 0)
  {
    // expecting MB with 1 empty blocks.
    expect(mb != nullptr, "expecting a vtkMultiBlockDataSet.");
    expect(mb->GetNumberOfBlocks() == 1, "expecting 1 blocks, got " << mb->GetNumberOfBlocks());
    expect(mb->GetBlock(0) == nullptr, "expecting null block at index 0.");
    return true;
  }

  expect(mb != nullptr, "expecting a vtkMultiBlockDataSet.");
  expect(mb->GetNumberOfBlocks() == 1, "expecting 1 block, got " << mb->GetNumberOfBlocks());

  for (int cc = 0; cc < 1; ++cc)
  {
    vtkTable* b0 = vtkTable::SafeDownCast(mb->GetBlock(cc));
    expect(b0 != nullptr, "expecting a vtkTable for block " << cc);
    expect(b0->GetNumberOfRows() == num_timesteps,
      "mismatched rows, expecting " << num_timesteps << ", got " << b0->GetNumberOfRows());
    expect(b0->GetNumberOfColumns() >= 5, "mismatched columns");
    expect(b0->GetColumnByName("EQPS") != nullptr, "missing EQPS.");

    const char* name = mb->GetMetaData(cc)->Get(vtkCompositeDataSet::NAME());
    expect(name != nullptr, "expecting non-null name.");

    std::ostringstream stream;
    stream << bname;
    expect(stream.str() == name,
      "block name not matching, expected '" << stream.str() << "', got '" << name << "'");
  }
  return true;
}

class Initializer
{
public:
  Initializer(int* argc, char*** argv)
  {
    MPI_Init(argc, argv);
    vtkMPIController* controller = vtkMPIController::New();
    controller->Initialize(argc, argv, 1);
    vtkMultiProcessController::SetGlobalController(controller);
  }

  ~Initializer()
  {
    vtkMultiProcessController::GetGlobalController()->Finalize();
    vtkMultiProcessController::GetGlobalController()->Delete();
    vtkMultiProcessController::SetGlobalController(nullptr);
  }
};

bool AllRanksSucceeded(bool status)
{
  vtkMultiProcessController* contr = vtkMultiProcessController::GetGlobalController();
  int success = status ? 1 : 0;
  int allsuccess = 0;
  contr->AllReduce(&success, &allsuccess, 1, vtkCommunicator::MIN_OP);
  return (allsuccess == 1);
}
}

int TestPExtractDataArraysOverTime(int argc, char* argv[])
{
  Initializer init(&argc, &argv);

  int ret = EXIT_SUCCESS;

  vtkMultiProcessController* contr = vtkMultiProcessController::GetGlobalController();
  if (contr == nullptr || contr->GetNumberOfProcesses() != 2)
  {
    cerr << "TestPExtractDataArraysOverTime requires 2 ranks." << endl;
    return EXIT_FAILURE;
  }

  const int myrank = contr->GetLocalProcessId();

  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/can.ex2");

  vtkNew<vtkPExodusIIReader> reader;
  reader->SetFileName(fname);
  reader->SetController(contr);
  reader->UpdateInformation();
  reader->SetAllArrayStatus(vtkExodusIIReader::NODAL, 1);
  reader->SetAllArrayStatus(vtkExodusIIReader::ELEM_BLOCK, 1);
  reader->SetGenerateGlobalElementIdArray(true);
  reader->SetGenerateGlobalNodeIdArray(true);

  // lets limit to 10 timesteps to reduce test time.
  vtkNew<vtkExtractTimeSteps> textracter;
  textracter->SetInputConnection(reader->GetOutputPort());
  textracter->UpdateInformation();
  textracter->GenerateTimeStepIndices(1, 11, 1);
  const int num_timesteps = 10;

  /**
   * Those 2 following filters compute statistics on distributed data
   */
  vtkNew<vtkRedistributeDataSetFilter> redistributeFilter;
  redistributeFilter->SetInputConnection(textracter->GetOutputPort());

  vtkNew<vtkGenerateGlobalIds> generateGlobalIds;
  generateGlobalIds->SetInputConnection(redistributeFilter->GetOutputPort());

  vtkNew<vtkPExtractDataArraysOverTime> distributedExtractorWithGhosts;
  distributedExtractorWithGhosts->SetReportStatisticsOnly(true);
  distributedExtractorWithGhosts->SetInputConnection(generateGlobalIds->GetOutputPort());
  distributedExtractorWithGhosts->Update();

  std::cout << "Computing stats " << std::endl;

  vtkNew<vtkPExtractDataArraysOverTime> extractor;
  extractor->SetReportStatisticsOnly(true);
  extractor->SetInputConnection(textracter->GetOutputPort());
  extractor->Update();

  if (myrank == 0)
  {
    // We check computed statistics in different setups:
    //
    // - P filter between distributed memory vs rank 0 has all the data
    // - Non P filter and P filter with rank 0 having all the data.

    std::cout << "Comparing computed stats between distributed and non-distributed memory"
              << std::endl;

    vtkNew<vtkExtractDataArraysOverTime> singleProcessExtractor;
    singleProcessExtractor->SetReportStatisticsOnly(true);
    singleProcessExtractor->SetInputConnection(textracter->GetOutputPort());
    singleProcessExtractor->Update();

    if (!TablesAreTheSame(
          vtkMultiBlockDataSet::SafeDownCast(singleProcessExtractor->GetOutputDataObject(0)),
          vtkMultiBlockDataSet::SafeDownCast(
            distributedExtractorWithGhosts->GetOutputDataObject(0))))
    {
      std::cerr << "Single process and multiple process with distributed data"
                << " do not compute the same statistics." << std::endl;
      ret = EXIT_FAILURE;
    }
    std::cout << "end " << std::endl;
    if (!TablesAreTheSame(
          vtkMultiBlockDataSet::SafeDownCast(singleProcessExtractor->GetOutputDataObject(0)),
          vtkMultiBlockDataSet::SafeDownCast(extractor->GetOutputDataObject(0))))
    {
      std::cerr << "Single process and multiple process with empty ranks"
                << " do not compute the same statistics." << std::endl;
      ret = EXIT_FAILURE;
    }
  }

  std::cout << "Checking if rank " << myrank << " has correct memory layout on output" << std::endl;
  if (!AllRanksSucceeded(
        ValidateStats(vtkMultiBlockDataSet::SafeDownCast(extractor->GetOutputDataObject(0)),
          num_timesteps, myrank)))
  {
    cerr << "ERROR: Failed to validate dataset at line: " << __LINE__ << endl;
    ret = EXIT_FAILURE;
  }

  // let's try non-summary extraction.
  vtkNew<vtkSelectionSource> selSource;
  selSource->SetContentType(vtkSelectionNode::GLOBALIDS);
  selSource->SetFieldType(vtkSelectionNode::CELL);
  selSource->AddID(0, 100);

  vtkNew<vtkExtractSelection> iextractor;
  iextractor->SetInputConnection(0, textracter->GetOutputPort());
  iextractor->SetInputConnection(1, selSource->GetOutputPort());

  extractor->SetReportStatisticsOnly(false);
  extractor->SetInputConnection(iextractor->GetOutputPort());
  extractor->SetFieldAssociation(vtkDataObject::CELL);
  extractor->Update();
  if (!AllRanksSucceeded(
        ValidateGID(vtkMultiBlockDataSet::SafeDownCast(extractor->GetOutputDataObject(0)),
          num_timesteps, "gid=100", myrank)))
  {
    cerr << "Failed to validate dataset at line: " << __LINE__ << endl;
    ret = EXIT_FAILURE;
  }

  // this time, simply use element id.
  extractor->SetUseGlobalIDs(false);
  extractor->Update();
  if (!AllRanksSucceeded(
        ValidateID(vtkMultiBlockDataSet::SafeDownCast(extractor->GetOutputDataObject(0)),
          num_timesteps, "originalId=99 block=2", myrank)))
  {
    cerr << "Failed to validate dataset at line: " << __LINE__ << endl;
    ret = EXIT_FAILURE;
  }

  delete[] fname;
  return ret;
}
