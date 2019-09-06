/*===========================================================================*/
/* Distributed under OSI-approved BSD 3-Clause License.                      */
/* For copyright, see the following accompanying files or https://vtk.org:   */
/* - VTK-Copyright.txt                                                       */
/*===========================================================================*/
#include "vtkXMLHierarchicalDataReader.h"

#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkXMLHierarchicalDataReader);
//----------------------------------------------------------------------------
vtkXMLHierarchicalDataReader::vtkXMLHierarchicalDataReader() = default;

//----------------------------------------------------------------------------
vtkXMLHierarchicalDataReader::~vtkXMLHierarchicalDataReader() = default;

//----------------------------------------------------------------------------
void vtkXMLHierarchicalDataReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

