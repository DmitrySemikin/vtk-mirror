/*===========================================================================*/
/* Distributed under OSI-approved BSD 3-Clause License.                      */
/* For copyright, see the following accompanying files or https://vtk.org:   */
/* - VTK-Copyright.txt                                                       */
/*===========================================================================*/
#include "vtkXMLPStructuredGridWriter.h"

#include "vtkErrorCode.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkStructuredGrid.h"
#include "vtkXMLStructuredGridWriter.h"

vtkStandardNewMacro(vtkXMLPStructuredGridWriter);

//----------------------------------------------------------------------------
vtkXMLPStructuredGridWriter::vtkXMLPStructuredGridWriter() = default;

//----------------------------------------------------------------------------
vtkXMLPStructuredGridWriter::~vtkXMLPStructuredGridWriter() = default;

//----------------------------------------------------------------------------
void vtkXMLPStructuredGridWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
vtkStructuredGrid* vtkXMLPStructuredGridWriter::GetInput()
{
  return static_cast<vtkStructuredGrid*>(this->Superclass::GetInput());
}

//----------------------------------------------------------------------------
const char* vtkXMLPStructuredGridWriter::GetDataSetName()
{
  return "PStructuredGrid";
}

//----------------------------------------------------------------------------
const char* vtkXMLPStructuredGridWriter::GetDefaultFileExtension()
{
  return "pvts";
}

//----------------------------------------------------------------------------
vtkXMLStructuredDataWriter*
  vtkXMLPStructuredGridWriter::CreateStructuredPieceWriter()
{
  // Create the writer for the piece.
  vtkXMLStructuredGridWriter* pWriter = vtkXMLStructuredGridWriter::New();
  pWriter->SetInputConnection(this->GetInputConnection(0, 0));
  return pWriter;
}

//----------------------------------------------------------------------------
void vtkXMLPStructuredGridWriter::WritePData(vtkIndent indent)
{
  this->Superclass::WritePData(indent);
  if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
  {
    return;
  }
  vtkStructuredGrid* input = this->GetInput();
  this->WritePPoints(input->GetPoints(), indent);
}

//----------------------------------------------------------------------------
int vtkXMLPStructuredGridWriter::FillInputPortInformation(
  int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkStructuredGrid");
  return 1;
}
