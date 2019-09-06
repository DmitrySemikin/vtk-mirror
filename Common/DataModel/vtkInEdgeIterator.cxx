/*===========================================================================*/
/* Distributed under OSI-approved BSD 3-Clause License.                      */
/* For copyright, see the following accompanying files or https://vtk.org:   */
/* - VTK-Copyright.txt                                                       */
/* - Sandia-Copyright.txt                                                    */
/*===========================================================================*/

#include "vtkInEdgeIterator.h"

#include "vtkObjectFactory.h"
#include "vtkGraph.h"
#include "vtkGraphEdge.h"

vtkCxxSetObjectMacro(vtkInEdgeIterator, Graph, vtkGraph);
vtkStandardNewMacro(vtkInEdgeIterator);
//----------------------------------------------------------------------------
vtkInEdgeIterator::vtkInEdgeIterator()
{
  this->Vertex = 0;
  this->Current = nullptr;
  this->End = nullptr;
  this->Graph = nullptr;
  this->GraphEdge = nullptr;
}

//----------------------------------------------------------------------------
vtkInEdgeIterator::~vtkInEdgeIterator()
{
  if (this->Graph)
  {
    this->Graph->Delete();
  }
  if (this->GraphEdge)
  {
    this->GraphEdge->Delete();
  }
}

//----------------------------------------------------------------------------
void vtkInEdgeIterator::Initialize(vtkGraph *graph, vtkIdType v)
{
  this->SetGraph(graph);
  this->Vertex = v;
  vtkIdType nedges;
  this->Graph->GetInEdges(this->Vertex, this->Current, nedges);
  this->End = this->Current + nedges;
}

//----------------------------------------------------------------------------
vtkGraphEdge *vtkInEdgeIterator::NextGraphEdge()
{
  vtkInEdgeType e = this->Next();
  if (!this->GraphEdge)
  {
    this->GraphEdge = vtkGraphEdge::New();
  }
  this->GraphEdge->SetSource(e.Source);
  this->GraphEdge->SetTarget(this->Vertex);
  this->GraphEdge->SetId(e.Id);
  return this->GraphEdge;
}

//----------------------------------------------------------------------------
void vtkInEdgeIterator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Graph: " << (this->Graph ? "" : "(null)") << endl;
  if (this->Graph)
  {
    this->Graph->PrintSelf(os, indent.GetNextIndent());
  }
  os << indent << "Vertex: " << this->Vertex << endl;
}
