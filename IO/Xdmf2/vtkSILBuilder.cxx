/*===========================================================================*/
/* Distributed under OSI-approved BSD 3-Clause License.                      */
/* For copyright, see the following accompanying files or https://vtk.org:   */
/* - VTK-Copyright.txt                                                       */
/*===========================================================================*/

#include "vtkSILBuilder.h"

#include "vtkObjectFactory.h"
#include "vtkMutableDirectedGraph.h"
#include "vtkStringArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkDataSetAttributes.h"

vtkStandardNewMacro(vtkSILBuilder);
vtkCxxSetObjectMacro(vtkSILBuilder, SIL, vtkMutableDirectedGraph);
//----------------------------------------------------------------------------
vtkSILBuilder::vtkSILBuilder()
{
  this->NamesArray = 0;
  this->CrossEdgesArray = 0;
  this->SIL = 0;
  this->RootVertex = -1;
}

//----------------------------------------------------------------------------
vtkSILBuilder::~vtkSILBuilder()
{
  this->SetSIL(0);
}

//----------------------------------------------------------------------------
void vtkSILBuilder::Initialize()
{
  this->SIL->Initialize();
  this->NamesArray = vtkStringArray::New();
  this->NamesArray->SetName("Names");
  this->CrossEdgesArray = vtkUnsignedCharArray::New();
  this->CrossEdgesArray->SetName("CrossEdges");
  this->SIL->GetVertexData()->AddArray(this->NamesArray);
  this->SIL->GetEdgeData()->AddArray(this->CrossEdgesArray);
  this->NamesArray->Delete();
  this->CrossEdgesArray->Delete();

  this->RootVertex = this->AddVertex("SIL");
}

//-----------------------------------------------------------------------------
vtkIdType vtkSILBuilder::AddVertex(const char* name)
{
  vtkIdType vertex = this->SIL->AddVertex();
  this->NamesArray->InsertValue(vertex, name);
  return vertex;
}

//-----------------------------------------------------------------------------
vtkIdType vtkSILBuilder::AddChildEdge(vtkIdType src, vtkIdType dst)
{
  vtkIdType id = this->SIL->AddEdge(src, dst).Id;
  this->CrossEdgesArray->InsertValue(id, 0);
  return id;
}

//-----------------------------------------------------------------------------
vtkIdType vtkSILBuilder::AddCrossEdge(vtkIdType src, vtkIdType dst)
{
  vtkIdType id = this->SIL->AddEdge(src, dst).Id;
  this->CrossEdgesArray->InsertValue(id, 1);
  return id;
}

//----------------------------------------------------------------------------
void vtkSILBuilder::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
