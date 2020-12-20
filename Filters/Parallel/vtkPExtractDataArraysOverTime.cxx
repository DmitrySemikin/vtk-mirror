/*=========================================================================

  Program:   ParaView
  Module:    vtkPExtractDataArraysOverTime.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPExtractDataArraysOverTime.h"

#include "vtkAbstractArray.h"
#include "vtkDataSetAttributes.h"
#include "vtkInformation.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkMultiProcessStream.h"
#include "vtkObjectFactory.h"
#include "vtkPDescriptiveStatistics.h"
#include "vtkPOrderStatistics.h"
#include "vtkSmartPointer.h"
#include "vtkTable.h"
#include "vtkUnsignedCharArray.h"

#include <map>
#include <set>
#include <sstream>
#include <vector>

namespace
{
vtkSmartPointer<vtkTable> vtkMergeTable(vtkTable* dest, vtkTable* src)
{
  if (dest == nullptr)
  {
    return src;
  }

  const auto numRows = dest->GetNumberOfRows();
  if (numRows != src->GetNumberOfRows())
  {
    return dest;
  }

  auto srcRowData = src->GetRowData();
  auto destRowData = dest->GetRowData();
  auto srcMask = vtkUnsignedCharArray::SafeDownCast(srcRowData->GetArray("vtkValidPointMask"));
  for (vtkIdType cc = 0; srcMask != nullptr && cc < numRows; ++cc)
  {
    if (srcMask->GetTypedComponent(cc, 0) == 0)
    {
      continue;
    }

    // Copy arrays from remote to current
    const int numArray = srcRowData->GetNumberOfArrays();
    for (int aidx = 0; aidx < numArray; aidx++)
    {
      auto srcArray = srcRowData->GetAbstractArray(aidx);
      if (const char* name = srcArray ? srcArray->GetName() : nullptr)
      {
        auto destArray = destRowData->GetAbstractArray(name);
        if (destArray == nullptr)
        {
          // add new array array if necessary
          destRowData->AddArray(destArray);
        }
        else
        {
          destArray->InsertTuple(cc, cc, srcArray);
        }
      }
    }
  }
  return dest;
}
static constexpr int NUMBER_OF_COLUMNS_COM = 25096;
static constexpr int ARRAY_NAME_LENGTH_COM = 25097;
static constexpr int BUFFER_COM = 25098;
} // anonymous namespace

vtkStandardNewMacro(vtkPExtractDataArraysOverTime);
vtkCxxSetObjectMacro(vtkPExtractDataArraysOverTime, Controller, vtkMultiProcessController);
//------------------------------------------------------------------------------
vtkPExtractDataArraysOverTime::vtkPExtractDataArraysOverTime()
{
  this->Controller = nullptr;
  this->SetController(vtkMultiProcessController::GetGlobalController());
}

//------------------------------------------------------------------------------
vtkPExtractDataArraysOverTime::~vtkPExtractDataArraysOverTime()
{
  this->SetController(nullptr);
}

//------------------------------------------------------------------------------
void vtkPExtractDataArraysOverTime::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Controller: " << this->Controller << endl;
}

//------------------------------------------------------------------------------
void vtkPExtractDataArraysOverTime::SynchronizeBlocksMetaData(vtkTable* splits)
{
  // We share among processes array information (type, name and number of
  // components) so MPI calls can be done for each array
  // The final buffer layout is
  // ArrayType - NumberOfComponents - ArrayName
  int localNumberOfColumns = splits->GetNumberOfColumns();
  int numberOfProcesses = this->Controller->GetNumberOfProcesses();
  std::vector<int> globalNumberOfColumns(numberOfProcesses);
  int localProcessId = this->Controller->GetLocalProcessId();
  this->Controller->AllGather(&localNumberOfColumns, globalNumberOfColumns.data(), 1);

  int maxId = 0;
  {
    int processId = 0;
    for (; processId < numberOfProcesses; ++processId)
    {
      if (localNumberOfColumns < globalNumberOfColumns[processId])
      {
        maxId = processId;
        break;
      }
      else if (localNumberOfColumns > globalNumberOfColumns[processId])
      {
        break;
      }
      else
      {
        maxId = processId;
      }
    }
    // No need to exchange data, no process are empty
    if (processId == numberOfProcesses)
    {
      return;
    }
  }

  // I have to send array information to processes lacking some.
  if (maxId == localProcessId)
  {
    std::set<int> emptyProcessIds;
    for (int processId = 0; processId < numberOfProcesses; ++processId)
    {
      if (localNumberOfColumns > globalNumberOfColumns[processId])
      {
        emptyProcessIds.insert(processId);
      }
    }
    vtkIdType numberOfColumns = splits->GetNumberOfColumns();
    for (int processId : emptyProcessIds)
    {
      this->Controller->Send(&numberOfColumns, 1, processId, NUMBER_OF_COLUMNS_COM);
    }

    vtkIdType numberOfChars = 0;
    std::vector<std::size_t> arrayNameLength(numberOfColumns);
    for (vtkIdType colId = 0; colId < splits->GetNumberOfColumns(); ++colId)
    {
      arrayNameLength[colId] = strlen(splits->GetColumnName(colId));
      numberOfChars += arrayNameLength[colId];
    }

    for (int processId : emptyProcessIds)
    {
      this->Controller->Send(
        arrayNameLength.data(), numberOfColumns, processId, ARRAY_NAME_LENGTH_COM);
    }

    vtkIdType bufferSize = numberOfChars + (2 + sizeof(int)) * numberOfColumns;
    std::vector<char> sendBuffer(bufferSize);
    char* sendBufferCurrent = sendBuffer.data();
    for (int processId : emptyProcessIds)
    {
      for (vtkIdType colId = 0; colId < splits->GetNumberOfColumns(); ++colId)
      {
        *(sendBufferCurrent++) = static_cast<char>(splits->GetColumn(colId)->GetDataType());
        *(reinterpret_cast<int*>(sendBufferCurrent)) =
          splits->GetColumn(colId)->GetNumberOfComponents();
        sendBufferCurrent += sizeof(int);
        std::size_t len = strlen(splits->GetColumnName(colId));
        memcpy(sendBufferCurrent, splits->GetColumnName(colId), len + 1);
        sendBufferCurrent += len + 1;
      }
      this->Controller->Send(sendBuffer.data(), bufferSize, processId, BUFFER_COM);
    }
  }
  // I need array informations from process of id maxId
  else if (maxId != numberOfProcesses)
  {
    vtkIdType numberOfColumns;
    this->Controller->Receive(&numberOfColumns, 1, maxId, NUMBER_OF_COLUMNS_COM);

    std::vector<std::size_t> arrayNameLength(numberOfColumns);
    this->Controller->Receive(
      arrayNameLength.data(), numberOfColumns, maxId, ARRAY_NAME_LENGTH_COM);

    vtkIdType numberOfChars = 0;
    for (vtkIdType colId = 0; colId < numberOfColumns; ++colId)
    {
      numberOfChars += arrayNameLength[colId];
    }

    vtkIdType bufferSize = numberOfChars + (2 + sizeof(int)) * numberOfColumns;
    std::vector<char> receiveBuffer(bufferSize);
    this->Controller->Receive(receiveBuffer.data(), bufferSize, maxId, BUFFER_COM);

    char* receiveBufferCurrent = receiveBuffer.data();
    for (vtkIdType colId = 0; colId < numberOfColumns; ++colId)
    {
      auto array = vtkSmartPointer<vtkAbstractArray>::Take(
        vtkAbstractArray::CreateArray(*(receiveBufferCurrent++)));
      array->SetNumberOfComponents(*(reinterpret_cast<int*>(receiveBufferCurrent)));
      receiveBufferCurrent += sizeof(int);
      array->SetName(receiveBufferCurrent);
      receiveBufferCurrent += arrayNameLength[colId] + 1;
      splits->AddColumn(array);
    }
  }
}

void vtkPExtractDataArraysOverTime::PostExecute(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  this->Superclass::PostExecute(request, inputVector, outputVector);
  vtkMultiBlockDataSet* output = vtkMultiBlockDataSet::GetData(outputVector, 0);
  assert(output);
  this->ReorganizeData(output);
}

//------------------------------------------------------------------------------
void vtkPExtractDataArraysOverTime::ReorganizeData(vtkMultiBlockDataSet* dataset)
{
  // If we only report statistics, we empty blocks of rank different than 0 to
  // eliminate duplicate information.
  // Else:
  // 1. Send all blocks to 0.
  // 2. Rank 0 then reorganizes blocks. This is done as follows:
  //    i. The tables of blocks of same id are merged
  //       into one.
  // 3. Rank 0 send info about number blocks and their names to everyone
  // 4. Satellites, then, simply initialize their output to and make it match
  //    the structure reported by rank 0.

  const int myRank = this->Controller->GetLocalProcessId();
  if (this->ReportStatisticsOnly)
  {
    if (myRank)
    {
      for (unsigned int blockId = 0; blockId < dataset->GetNumberOfBlocks(); ++blockId)
      {
        dataset->SetBlock(blockId, nullptr);
      }
    }
    return;
  }
  const int numRanks = this->Controller->GetNumberOfProcesses();
  if (myRank != 0)
  {
    std::vector<vtkSmartPointer<vtkDataObject>> recvBuffer;
    this->Controller->Gather(dataset, recvBuffer, 0);

    vtkMultiProcessStream stream;
    this->Controller->Broadcast(stream, 0);

    dataset->Initialize();
    while (!stream.Empty())
    {
      std::string name;
      stream >> name;

      auto idx = dataset->GetNumberOfBlocks();
      dataset->SetBlock(idx, nullptr);
      dataset->GetMetaData(idx)->Set(vtkCompositeDataSet::NAME(), name.c_str());
    }
  }
  else
  {
    std::vector<vtkSmartPointer<vtkDataObject>> recvBuffer;
    this->Controller->Gather(dataset, recvBuffer, 0);

    assert(static_cast<int>(recvBuffer.size()) == numRanks);

    recvBuffer[myRank] = dataset;

    std::map<std::string, std::map<int, vtkSmartPointer<vtkTable>>> collection;
    for (int rank = 0; rank < numRanks; ++rank)
    {
      if (auto mb = vtkMultiBlockDataSet::SafeDownCast(recvBuffer[rank]))
      {
        for (unsigned int cc = 0, max = mb->GetNumberOfBlocks(); cc < max; ++cc)
        {
          const char* name = mb->GetMetaData(cc)->Get(vtkCompositeDataSet::NAME());
          vtkTable* subblock = vtkTable::SafeDownCast(mb->GetBlock(cc));
          if (name && subblock)
          {
            collection[name][rank] = subblock;
          }
        }
      }
    }

    vtkMultiProcessStream stream;
    vtkNew<vtkMultiBlockDataSet> mb;
    for (auto& item : collection)
    {
      const std::string& name = item.first;
      vtkSmartPointer<vtkTable> mergedTable;
      for (auto& sitem : item.second)
      {
        mergedTable = vtkMergeTable(mergedTable, sitem.second);
      }

      auto idx = mb->GetNumberOfBlocks();
      mb->SetBlock(idx, mergedTable);
      mb->GetMetaData(idx)->Set(vtkCompositeDataSet::NAME(), name.c_str());
      stream << name;
    }

    this->Controller->Broadcast(stream, 0);
    dataset->ShallowCopy(mb);
  } // end rank 0
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkDescriptiveStatistics> vtkPExtractDataArraysOverTime::NewDescriptiveStatistics()
{
  return vtkSmartPointer<vtkPDescriptiveStatistics>::New();
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkOrderStatistics> vtkPExtractDataArraysOverTime::NewOrderStatistics()
{
  return vtkSmartPointer<vtkPOrderStatistics>::New();
}

//------------------------------------------------------------------------------
vtkIdType vtkPExtractDataArraysOverTime::SynchronizeNumberOfTotalInputTuples(
  vtkDataSetAttributes* dsa)
{
  vtkIdType localNumberOfTotalInputTuples = Superclass::SynchronizeNumberOfTotalInputTuples(dsa);
  vtkIdType numberOfTotalInputTuples = 0;
  this->Controller->AllReduce(
    &localNumberOfTotalInputTuples, &numberOfTotalInputTuples, 1, vtkCommunicator::SUM_OP);
  return numberOfTotalInputTuples;
}
