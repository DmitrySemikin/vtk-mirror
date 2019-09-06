/*===========================================================================*/
/* Distributed under OSI-approved BSD 3-Clause License.                      */
/* For copyright, see the following accompanying files or https://vtk.org:   */
/* - VTK-Copyright.txt                                                       */
/*===========================================================================*/
#include "vtkXMLStructuredGridWriter.h"

#include "vtkCellData.h"
#include "vtkErrorCode.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkStructuredGrid.h"
#define vtkXMLOffsetsManager_DoNotInclude
#include "vtkXMLOffsetsManager.h"
#undef  vtkXMLOffsetsManager_DoNotInclude

vtkStandardNewMacro(vtkXMLStructuredGridWriter);

//----------------------------------------------------------------------------
vtkXMLStructuredGridWriter::vtkXMLStructuredGridWriter()
{
  this->PointsOM = new OffsetsManagerGroup;
}

//----------------------------------------------------------------------------
vtkXMLStructuredGridWriter::~vtkXMLStructuredGridWriter()
{
  delete this->PointsOM;
}

//----------------------------------------------------------------------------
void vtkXMLStructuredGridWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
vtkStructuredGrid* vtkXMLStructuredGridWriter::GetInput()
{
  return static_cast<vtkStructuredGrid*>(this->Superclass::GetInput());
}

//----------------------------------------------------------------------------
void vtkXMLStructuredGridWriter::GetInputExtent(int* extent)
{
  this->GetInput()->GetExtent(extent);
}

//----------------------------------------------------------------------------
const char* vtkXMLStructuredGridWriter::GetDataSetName()
{
  return "StructuredGrid";
}

//----------------------------------------------------------------------------
const char* vtkXMLStructuredGridWriter::GetDefaultFileExtension()
{
  return "vts";
}

//----------------------------------------------------------------------------
void vtkXMLStructuredGridWriter::AllocatePositionArrays()
{
  this->Superclass::AllocatePositionArrays();
  this->PointsOM->Allocate(this->NumberOfPieces,this->NumberOfTimeSteps);
}

//----------------------------------------------------------------------------
void vtkXMLStructuredGridWriter::DeletePositionArrays()
{
  this->Superclass::DeletePositionArrays();
}

//----------------------------------------------------------------------------
void vtkXMLStructuredGridWriter::WriteAppendedPiece(int index,
                                                    vtkIndent indent)
{
  this->Superclass::WriteAppendedPiece(index, indent);
  if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
  {
    return;
  }
  this->WritePointsAppended(this->GetInput()->GetPoints(), indent,
    &this->PointsOM->GetPiece(index));
}

//----------------------------------------------------------------------------
void vtkXMLStructuredGridWriter::WriteAppendedPieceData(int index)
{
  // Split progress range by the approximate fractions of data written
  // by each step in this method.
  float progressRange[2] = { 0.f, 0.f };
  this->GetProgressRange(progressRange);
  float fractions[3];
  this->CalculateSuperclassFraction(fractions);

  // Set the range of progress for the superclass.
  this->SetProgressRange(progressRange, 0, fractions);

  // Let the superclass write its data.
  this->Superclass::WriteAppendedPieceData(index);
  if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
  {
    return;
  }

  // Set the range of progress for the points array.
  this->SetProgressRange(progressRange, 1, fractions);

  // Write the points array.
  this->WritePointsAppendedData(this->GetInput()->GetPoints(),
                                this->CurrentTimeIndex,
                                &this->PointsOM->GetPiece(index));
}

//----------------------------------------------------------------------------
void vtkXMLStructuredGridWriter::WriteInlinePiece(vtkIndent indent)
{
  // Split progress range by the approximate fractions of data written
  // by each step in this method.
  float progressRange[2] = { 0.f, 0.f };
  this->GetProgressRange(progressRange);
  float fractions[3];
  this->CalculateSuperclassFraction(fractions);

  // Set the range of progress for the superclass.
  this->SetProgressRange(progressRange, 0, fractions);

  // Let the superclass write its data.
  this->Superclass::WriteInlinePiece(indent);
  if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
  {
    return;
  }

  // Set the range of progress for the points array.
  this->SetProgressRange(progressRange, 1, fractions);

  // Write the points array.
  this->WritePointsInline(this->GetInput()->GetPoints(), indent);
}

//----------------------------------------------------------------------------
void vtkXMLStructuredGridWriter::CalculateSuperclassFraction(float* fractions)
{
  // The amount of data written by the superclass comes from the
  // point/cell data arrays.
  vtkIdType superclassPieceSize = GetNumberOfValues(this->GetInput());
  // The total data written includes the points array.
  vtkIdType totalPieceSize = superclassPieceSize + this->GetInput()->GetNumberOfPoints()*3;
  if (totalPieceSize == 0)
  {
    totalPieceSize = 1;
  }
  fractions[0] = 0.0f;
  fractions[1] = static_cast<float>(superclassPieceSize)/totalPieceSize;
  fractions[2] = 1.0f;
}

//----------------------------------------------------------------------------
int vtkXMLStructuredGridWriter::FillInputPortInformation(
  int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkStructuredGrid");
  return 1;
}
