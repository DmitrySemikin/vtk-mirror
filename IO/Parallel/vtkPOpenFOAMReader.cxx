/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPOpenFOAMReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This class was developed by Takuya Oshima at Niigata University,
// Japan (oshima@eng.niigata-u.ac.jp).

#include "vtkPOpenFOAMReader.h"

#include "vtkAppendCompositeDataLeaves.h"
#include "vtkCharArray.h"
#include "vtkCollection.h"
#include "vtkDataArraySelection.h"
#include "vtkDirectory.h"
#include "vtkDoubleArray.h"
#include "vtkFieldData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkSortDataArray.h"
#include "vtkStdString.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStringArray.h"
#include <sstream>

vtkStandardNewMacro(vtkPOpenFOAMReader);
vtkCxxSetObjectMacro(vtkPOpenFOAMReader, Controller, vtkMultiProcessController);

namespace
{
// Determing the number of processor<digit>/ directories
int GuessNprocsDecomposed(vtkDirectory *dir, vtkStringArray *procNames)
{
  // search and list processor subdirectories
  vtkIntArray *procNos = vtkIntArray::New();
  for (int fileI = 0; fileI < dir->GetNumberOfFiles(); fileI++)
  {
    const vtkStdString subDir(dir->GetFile(fileI));
    if (subDir.substr(0, 9) == "processor")
    {
      const vtkStdString procNoStr(subDir.substr(9));
      size_t pos = 0;
      const int procNo = std::stoi(procNoStr, &pos);
      if (procNoStr.length() == pos && procNo >= 0)
      {
        procNos->InsertNextValue(procNo);
        procNames->InsertNextValue(subDir);
      }
    }
  }
  procNos->Squeeze();
  procNames->Squeeze();
  // sort processor subdirectories by processor numbers
  vtkSortDataArray::Sort(procNos, procNames);
  procNos->Delete();

  return procNames->GetNumberOfTuples();
}

// Determing the number of decomposed blocks
int NextTokenHead(std::ifstream& ifs)
{
  for (;;)
  {
    int c;
    while (isspace(c = ifs.get())); // isspace() accepts -1 as EOF
    if (c == '/')
    {
      if ((c = ifs.get()) == '/')
      {
        while ((c = ifs.get()) != EOF && c != '\n');
        if (c == EOF)
        {
          return c;
        }
      }
      else if (c == '*')
      {
        for (;;)
        {
          while ((c = ifs.get()) != EOF && c != '*');
          if (c == EOF)
          {
            return c;
          }
          else if ((c = ifs.get()) == '/')
          {
            break;
          }
          ifs.putback(c);
        }
      }
      else
      {
        ifs.putback(c); // may be an EOF
        return '/';
      }
    }
    else // may be an EOF
    {
      return c;
    }
  }
    #if defined(__hpux)
  return EOF; // this line should not be executed; workaround for HP-UXia64-aCC
    #endif
}

bool ReadExpecting(std::ifstream& ifs, const char expected)
{
  int c;
  while (isspace(c = ifs.get()));
  if (c == '/')
  {
    ifs.putback(c);
    c = NextTokenHead(ifs);
  }
  if (c != expected)
  {
    return false;
  }
  return true;
}

// Read entry until semicolon and return true.
// if closing brace found, put_back the character and return false
bool ReadEntry(std::ifstream& ifs, std::string& keyword, std::string& element)
{
  std::stringstream ss;
  while(char c = ifs.get())
  {
    if (c == '}')
    {
      ifs.putback(c);
      return false;
    }
    else if (c == ';')
    {
      ss >> keyword >> element;
      return true;
    }
    else
    {
      ss << c;
    }
  }
}

int GuessNprocsCollated(vtkDirectory *dir, vtkStdString& masterCasePath, vtkStringArray *procNames)
{
  // search and list processors subdirectory
  for (int fileI = 0; fileI < dir->GetNumberOfFiles(); fileI++)
  {
    const vtkStdString subDir(dir->GetFile(fileI));
    if ("processors" == subDir)
    {
      const vtkStdString boundaryFile = masterCasePath + vtkStdString("processors/constant/polyMesh/boundary");
      std::ifstream ifs(boundaryFile);
      vtkStdString word;
      int procNo = 0;

      // skip the comment block and reach to the first meaningful token, FoamFile
      int c = NextTokenHead(ifs);
      ifs.putback(c);
      // FoamFile Header begin
      ifs >> word; // word should contain FoamFile

      // read FoamFile subDict entries
      std::string keyword;
      std::string element;
      ReadExpecting(ifs, '{');
      while(ReadEntry(ifs, keyword, element));
      ReadExpecting(ifs, '}');
      // FoamFile Header end

      // Parsing the decomposed block data which consists of series of list as follows:
      // intNumber(byteList)
      // intNumber(byteList)
      // ...
      // where intNumber is exactly the size of byteList between the surrounding brackets.
      // each byteList is nothing but an ordinary dictionary file which would be written
      // to each processor directory in decomposed case.
      while(1)
      {
        // skip any commend statement or block
        c = NextTokenHead(ifs);
        if (c == EOF)
        {
          break;
        }
        // get the first token
        ifs.putback(c);
        ifs >> word;
        int byteSize;
        size_t pos = 0;

        // first token must be intNumber
        byteSize = std::stoi(word, &pos);
        if (word.length() != pos)
        {
          std::cerr << "Invalid byte size" << std::endl;
          return -1;
        }
        // skip any comment statement or block and reach to an opening brace.
        c = NextTokenHead(ifs);
        // move the stream pointer by intNumber. it might work with seekg but not sure.
        for (auto i=0; i<byteSize; ++i)
        {
          ifs.get();
        }
        // reach to the closing brace.
        c = NextTokenHead(ifs);
        // found decomposed block
        procNames->InsertNextValue(vtkStdString("processor") + std::to_string(procNo));
        ++procNo;
      }
      ifs.close();
      break;
    }
  }
  procNames->Squeeze();
  return procNames->GetNumberOfTuples();
}
}
//-----------------------------------------------------------------------------
vtkPOpenFOAMReader::vtkPOpenFOAMReader()
{
  this->Controller = nullptr;
  this->SetController(vtkMultiProcessController::GetGlobalController());
  if (this->Controller == nullptr)
  {
    this->NumProcesses = 1;
    this->ProcessId = 0;
  }
  else
  {
    this->NumProcesses = this->Controller->GetNumberOfProcesses();
    this->ProcessId = this->Controller->GetLocalProcessId();
  }
  this->CaseType = RECONSTRUCTED_CASE;
  this->MTimeOld = 0;
}

//-----------------------------------------------------------------------------
vtkPOpenFOAMReader::~vtkPOpenFOAMReader()
{
  this->SetController(nullptr);
}

//-----------------------------------------------------------------------------
void vtkPOpenFOAMReader::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Case Type: " << this->CaseType << endl;
  os << indent << "MTimeOld: " << this->MTimeOld << endl;
  os << indent << "Number of Processes: " << this->NumProcesses << endl;
  os << indent << "Process Id: " << this->ProcessId << endl;
  os << indent << "Controller: " << this->Controller << endl;
}

//-----------------------------------------------------------------------------
void vtkPOpenFOAMReader::SetCaseType(const int t)
{
  if (this->CaseType != t)
  {
    this->CaseType = static_cast<caseType>(t);
    this->Refresh = true;
    this->Modified();
  }
}

//-----------------------------------------------------------------------------
int vtkPOpenFOAMReader::RequestInformation(vtkInformation *request,
    vtkInformationVector **inputVector, vtkInformationVector *outputVector)
{
  if (this->CaseType == RECONSTRUCTED_CASE)
  {
    int ret = 1;
    if (this->ProcessId == 0)
    {
      ret = this->Superclass::RequestInformation(request, inputVector,
          outputVector);
    }
    if (this->NumProcesses > 1)
    {
      // if there was an error in process 0 abort all processes
      this->BroadcastStatus(ret);
      if (ret == 0)
      {
        vtkErrorMacro(<< "The master process returned an error.");
        return 0;
      }

      vtkDoubleArray *timeValues;
      if (this->ProcessId == 0)
      {
        timeValues = this->Superclass::GetTimeValues();
      }
      else
      {
        timeValues = vtkDoubleArray::New();
      }
      this->Controller->Broadcast(timeValues, 0);
      if (this->ProcessId != 0)
      {
        this->Superclass::SetTimeInformation(outputVector, timeValues);
        timeValues->Delete();
        this->Superclass::Refresh = false;
      }
      this->GatherMetaData(); // pvserver deadlocks without this
    }

    return ret;
  }

  if (!this->Superclass::FileName || strlen(this->Superclass::FileName) == 0)
  {
    vtkErrorMacro("FileName has to be specified!");
    return 0;
  }

  if (*this->Superclass::FileNameOld != this->Superclass::FileName
      || this->Superclass::ListTimeStepsByControlDict != this->Superclass::ListTimeStepsByControlDictOld
      || this->Superclass::SkipZeroTime != this->Superclass::SkipZeroTimeOld
      || this->Superclass::Refresh)
  {
    // retain selection status when just refreshing a case
    if (!this->Superclass::FileNameOld->empty() && *this->Superclass::FileNameOld != this->Superclass::FileName)
    {
      // clear selections
      this->Superclass::CellDataArraySelection->RemoveAllArrays();
      this->Superclass::PointDataArraySelection->RemoveAllArrays();
      this->Superclass::LagrangianDataArraySelection->RemoveAllArrays();
      this->Superclass::PatchDataArraySelection->RemoveAllArrays();
    }

    *this->Superclass::FileNameOld = vtkStdString(this->FileName);
    this->Superclass::Readers->RemoveAllItems();
    this->Superclass::NumberOfReaders = 0;

    vtkStringArray *procNames = vtkStringArray::New();
    vtkDoubleArray *timeValues;

    // recreate case information
    vtkStdString masterCasePath, controlDictPath;
    this->Superclass::CreateCasePath(masterCasePath, controlDictPath);

    this->Superclass::CreateCharArrayFromString(this->Superclass::CasePath,
        "CasePath", masterCasePath);

    int ret = 1;
    if (this->ProcessId == 0)
    {
      // search and list processor subdirectories
      vtkDirectory *dir = vtkDirectory::New();
      if (!dir->Open(masterCasePath.c_str()))
      {
        vtkErrorMacro(<< "Can't open " << masterCasePath.c_str());
        dir->Delete();
        this->BroadcastStatus(ret = 0);
        return 0;
      }
      if (this->CaseType == DECOMPOSED_CASE)
      {
        GuessNprocsDecomposed(dir, procNames);
      }
      else if (this->CaseType == COLLATED_CASE)
      {
        GuessNprocsCollated(dir, masterCasePath, procNames);
      }
      dir->Delete();


      // get time directories from the first processor subdirectory
      if (procNames->GetNumberOfTuples() > 0)
      {
        vtkOpenFOAMReader *masterReader = vtkOpenFOAMReader::New();
        masterReader->SetFileName(this->FileName);
        masterReader->SetParent(this);
        masterReader->SetSkipZeroTime(this->SkipZeroTime);
        masterReader->SetUse64BitLabels(this->Use64BitLabels);
        masterReader->SetUse64BitFloats(this->Use64BitFloats);
        int procNo = std::stoi(procNames->GetValue(0).substr(9));
        masterReader->SetProcNo(procNo);

        if (this->CaseType == DECOMPOSED_CASE)
        {
          if (!masterReader->MakeInformationVector(outputVector, procNames
              ->GetValue(0)) || !masterReader->MakeMetaDataAtTimeStep(true))
          {
            procNames->Delete();
            masterReader->Delete();
            this->BroadcastStatus(ret = 0);
            return 0;
          }
        }
        else if (this->CaseType == COLLATED_CASE)
        {
          if (!masterReader->MakeInformationVector(outputVector, "processors")
            || !masterReader->MakeMetaDataAtTimeStep(true))
          {
            procNames->Delete();
            masterReader->Delete();
            this->BroadcastStatus(ret = 0);
            return 0;
          }
        }
        this->Superclass::Readers->AddItem(masterReader);
        timeValues = masterReader->GetTimeValues();
        masterReader->Delete();
      }
      else
      {
        timeValues = vtkDoubleArray::New();
        this->Superclass::SetTimeInformation(outputVector, timeValues);
      }
    }
    else
    {
      timeValues = vtkDoubleArray::New();
    }

    if (this->NumProcesses > 1)
    {
      // if there was an error in process 0 abort all processes
      this->BroadcastStatus(ret);
      if (ret == 0)
      {
        vtkErrorMacro(<< "The master process returned an error.");
        timeValues->Delete(); // don't have to care about process 0
        return 0;
      }

      this->Broadcast(procNames);
      this->Controller->Broadcast(timeValues, 0);
      if (this->ProcessId != 0)
      {
        this->Superclass::SetTimeInformation(outputVector, timeValues);
        timeValues->Delete();
      }
    }

    if (this->ProcessId == 0 && procNames->GetNumberOfTuples() == 0)
    {
      timeValues->Delete();
    }

    // create reader instances for other processor subdirectories
    // skip processor0 since it's already created
    for (int procI = (this->ProcessId ? this->ProcessId : this->NumProcesses); procI
        < procNames->GetNumberOfTuples(); procI += this->NumProcesses)
    {
      vtkOpenFOAMReader *subReader = vtkOpenFOAMReader::New();
      subReader->SetFileName(this->FileName);
      subReader->SetParent(this);
      subReader->SetUse64BitLabels(this->Use64BitLabels);
      subReader->SetUse64BitFloats(this->Use64BitFloats);
      int procNo = std::stoi(procNames->GetValue(procI).substr(9));
      subReader->SetProcNo(procNo);

      // if getting metadata failed simply delete the reader instance
      if (this->CaseType == DECOMPOSED_CASE)
      {
        if (subReader->MakeInformationVector(nullptr, procNames->GetValue(procI))
            && subReader->MakeMetaDataAtTimeStep(true))
        {
          this->Superclass::Readers->AddItem(subReader);
        }
        else
        {
          vtkWarningMacro(<<"Removing reader for processor subdirectory "
            << procNames->GetValue(procI).c_str());
        }
      }
      else if (this->CaseType == COLLATED_CASE)
      {
        if (subReader->MakeInformationVector(NULL, "processors")
            && subReader->MakeMetaDataAtTimeStep(true))
        {
          this->Superclass::Readers->AddItem(subReader);
        }
        else
        {
          vtkWarningMacro(<<"Removing reader for processor subdirectory "
            << procNames->GetValue(procI).c_str());
        }
      }
      subReader->Delete();
    }

    procNames->Delete();

    this->GatherMetaData();
    this->Superclass::Refresh = false;
  }

  outputVector->GetInformationObject(0)->Set(
    CAN_HANDLE_PIECE_REQUEST(),
    1);

  return 1;
}

//-----------------------------------------------------------------------------
int vtkPOpenFOAMReader::RequestData(vtkInformation *request,
    vtkInformationVector **inputVector, vtkInformationVector *outputVector)
{
  if (this->CaseType == RECONSTRUCTED_CASE)
  {
    int ret = 1;
    if (this->ProcessId == 0)
    {
      ret = this->Superclass::RequestData(request, inputVector, outputVector);
    }
    this->BroadcastStatus(ret);
    this->GatherMetaData();
    return ret;
  }

  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkMultiBlockDataSet
      *output =
          vtkMultiBlockDataSet::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  int ret = 1;
  if (this->Superclass::Readers->GetNumberOfItems() > 0)
  {
    int nSteps = 0;
    double requestedTimeValue(0);
    if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()))
    {
      requestedTimeValue
          = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
      nSteps = outInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
      if (nSteps > 0)
      {
        outInfo->Set(vtkDataObject::DATA_TIME_STEP(), requestedTimeValue);
      }
    }

    vtkAppendCompositeDataLeaves *append = vtkAppendCompositeDataLeaves::New();
    // append->AppendFieldDataOn();

    vtkOpenFOAMReader *reader;
    this->Superclass::CurrentReaderIndex = 0;
    this->Superclass::Readers->InitTraversal();
    while ((reader
        = vtkOpenFOAMReader::SafeDownCast(this->Superclass::Readers->GetNextItemAsObject()))
        != nullptr)
    {
      // even if the child readers themselves are not modified, mark
      // them as modified if "this" has been modified, since they
      // refer to the property of "this"
      if ((nSteps > 0 && reader->SetTimeValue(requestedTimeValue))
          || this->MTimeOld != this->GetMTime())
      {
        reader->Modified();
      }
      if (reader->MakeMetaDataAtTimeStep(false))
      {
        append->AddInputConnection(reader->GetOutputPort());
      }
    }

    this->GatherMetaData();

    if (append->GetNumberOfInputConnections(0) == 0)
    {
      output->Initialize();
      ret = 0;
    }
    else
    {
      // reader->RequestInformation() and RequestData() are called
      // for all reader instances without setting UPDATE_TIME_STEPS
      append->Update();
      output->ShallowCopy(append->GetOutput());
    }
    append->Delete();

    // known issue: output for process without sub-reader will not have CasePath
    output->GetFieldData()->AddArray(this->Superclass::CasePath);
  }
  else
  {
    this->GatherMetaData();
    // page 322 of The ParaView Guide says the output must be initialized
    output->Initialize();
  }

  this->Superclass::UpdateStatus();
  this->MTimeOld = this->GetMTime();

  return ret;
}

//-----------------------------------------------------------------------------
void vtkPOpenFOAMReader::BroadcastStatus(int &status)
{
  if (this->NumProcesses > 1)
  {
    this->Controller->Broadcast(&status, 1, 0);
  }
}

//-----------------------------------------------------------------------------
void vtkPOpenFOAMReader::GatherMetaData()
{
  if (this->NumProcesses > 1)
  {
    this->AllGather(this->Superclass::PatchDataArraySelection);
    this->AllGather(this->Superclass::CellDataArraySelection);
    this->AllGather(this->Superclass::PointDataArraySelection);
    this->AllGather(this->Superclass::LagrangianDataArraySelection);
    // omit removing duplicated entries of LagrangianPaths as well
    // when the number of processes is 1 assuming there's no duplicate
    // entry within a process
    this->AllGather(this->Superclass::LagrangianPaths);
  }
}

//-----------------------------------------------------------------------------
// Broadcast a vtkStringArray in process 0 to all processes
void vtkPOpenFOAMReader::Broadcast(vtkStringArray *sa)
{
  vtkIdType lengths[2];
  if (this->ProcessId == 0)
  {
    lengths[0] = sa->GetNumberOfTuples();
    lengths[1] = 0;
    for (int strI = 0; strI < sa->GetNumberOfTuples(); strI++)
    {
      lengths[1] += static_cast<vtkIdType>(sa->GetValue(strI).length()) + 1;
    }
  }
  this->Controller->Broadcast(lengths, 2, 0);
  char *contents = new char [lengths[1]];
  if (this->ProcessId == 0)
  {
    for (int strI = 0, idx = 0; strI < sa->GetNumberOfTuples(); strI++)
    {
      const int len = static_cast<int>(sa->GetValue(strI).length()) + 1;
      memmove(contents + idx, sa->GetValue(strI).c_str(), len);
      idx += len;
    }
  }
  this->Controller->Broadcast(contents, lengths[1], 0);
  if (this->ProcessId != 0)
  {
    sa->Initialize();
    sa->SetNumberOfTuples(lengths[0]);
    for (int strI = 0, idx = 0; strI < lengths[0]; strI++)
    {
      sa->SetValue(strI, contents + idx);
      idx += static_cast<int>(sa->GetValue(strI).length()) + 1;
    }
  }
  delete [] contents;
}

//-----------------------------------------------------------------------------
// AllGather vtkStringArray from and to all processes
void vtkPOpenFOAMReader::AllGather(vtkStringArray *s)
{
  vtkIdType length = 0;
  for (int strI = 0; strI < s->GetNumberOfTuples(); strI++)
  {
    length += static_cast<vtkIdType>(s->GetValue(strI).length()) + 1;
  }
  vtkIdType *lengths = new vtkIdType [this->NumProcesses];
  this->Controller->AllGather(&length, lengths, 1);
  vtkIdType totalLength = 0;
  vtkIdType *offsets = new vtkIdType [this->NumProcesses];
  for (int procI = 0; procI < this->NumProcesses; procI++)
  {
    offsets[procI] = totalLength;
    totalLength += lengths[procI];
  }
  char *allContents = new char [totalLength], *contents = new char [length];
  for (int strI = 0, idx = 0; strI < s->GetNumberOfTuples(); strI++)
  {
    const int len = static_cast<int>(s->GetValue(strI).length()) + 1;
    memmove(contents + idx, s->GetValue(strI).c_str(), len);
    idx += len;
  }
  this->Controller->AllGatherV(contents, allContents, length, lengths, offsets);
  delete [] contents;
  delete [] lengths;
  delete [] offsets;
  s->Initialize();
  for (int idx = 0; idx < totalLength; idx += static_cast<int>(strlen(allContents + idx)) + 1)
  {
    const char *str = allContents + idx;
    // insert only when the same string is not found
    if (s->LookupValue(str) == -1)
    {
      s->InsertNextValue(str);
    }
  }
  s->Squeeze();
  delete [] allContents;
}

//-----------------------------------------------------------------------------
// AllGather vtkDataArraySelections from and to all processes
void vtkPOpenFOAMReader::AllGather(vtkDataArraySelection *s)
{
  vtkIdType length = 0;
  for (int strI = 0; strI < s->GetNumberOfArrays(); strI++)
  {
    length += static_cast<vtkIdType>(strlen(s->GetArrayName(strI))) + 2;
  }
  vtkIdType *lengths = new vtkIdType [this->NumProcesses];
  this->Controller->AllGather(&length, lengths, 1);
  vtkIdType totalLength = 0;
  vtkIdType *offsets = new vtkIdType [this->NumProcesses];
  for (int procI = 0; procI < this->NumProcesses; procI++)
  {
    offsets[procI] = totalLength;
    totalLength += lengths[procI];
  }
  char *allContents = new char [totalLength], *contents = new char [length];
  for (int strI = 0, idx = 0; strI < s->GetNumberOfArrays(); strI++)
  {
    const char *arrayName = s->GetArrayName(strI);
    contents[idx] = s->ArrayIsEnabled(arrayName);
    const int len = static_cast<int>(strlen(arrayName)) + 1;
    memmove(contents + idx + 1, arrayName, len);
    idx += len + 1;
  }
  this->Controller->AllGatherV(contents, allContents, length, lengths, offsets);
  delete [] contents;
  delete [] lengths;
  delete [] offsets;
  // do not RemoveAllArray so that the previous arrays are preserved
  // s->RemoveAllArrays();
  for (int idx = 0; idx < totalLength; idx += static_cast<int>(strlen(allContents + idx + 1)) + 2)
  {
    const char *arrayName = allContents + idx + 1;
    s->AddArray(arrayName);
    if (allContents[idx] == 0)
    {
      s->DisableArray(arrayName);
    }
    else
    {
      s->EnableArray(arrayName);
    }
  }
  delete [] allContents;
}
