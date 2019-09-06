/*===========================================================================*/
/* Distributed under OSI-approved BSD 3-Clause License.                      */
/* For copyright, see the following accompanying files or https://vtk.org:   */
/* - VTK-Copyright.txt                                                       */
/* - Sandia-Copyright.txt                                                    */
/*===========================================================================*/

#include "vtkDataObjectToTable.h"

#include "vtkCellData.h"
#include "vtkDataObject.h"
#include "vtkDataSet.h"
#include "vtkDataSetAttributes.h"
#include "vtkGraph.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkTable.h"

vtkStandardNewMacro(vtkDataObjectToTable);
//---------------------------------------------------------------------------
vtkDataObjectToTable::vtkDataObjectToTable()
{
  this->FieldType = POINT_DATA;
}

//---------------------------------------------------------------------------
vtkDataObjectToTable::~vtkDataObjectToTable() = default;

//---------------------------------------------------------------------------
int vtkDataObjectToTable::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Remove(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkGraph");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkTable");
  return 1;
}

//---------------------------------------------------------------------------
int vtkDataObjectToTable::RequestData(
  vtkInformation*,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  // Get input data
  vtkInformation* inputInfo = inputVector[0]->GetInformationObject(0);
  vtkDataObject* input = inputInfo->Get(vtkDataObject::DATA_OBJECT());

  // Get output table
  vtkInformation* outputInfo = outputVector->GetInformationObject(0);
  vtkTable* output = vtkTable::SafeDownCast(
    outputInfo->Get(vtkDataObject::DATA_OBJECT()));

  // If the input is a table, just copy it into the output.
  if (vtkTable::SafeDownCast(input))
  {
    output->ShallowCopy(input);
    return 1;
  }

  vtkDataSetAttributes* data = vtkDataSetAttributes::New();

  switch(this->FieldType)
  {
    case FIELD_DATA:
      if(input->GetFieldData())
      {
        data->ShallowCopy(input->GetFieldData());
      }
      break;
    case POINT_DATA:
      if(vtkDataSet* const dataset = vtkDataSet::SafeDownCast(input))
      {
        if(dataset->GetPointData())
        {
          data->ShallowCopy(dataset->GetPointData());
        }
      }
      break;
    case CELL_DATA:
      if(vtkDataSet* const dataset = vtkDataSet::SafeDownCast(input))
      {
        if(dataset->GetCellData())
        {
          data->ShallowCopy(dataset->GetCellData());
        }
      }
      break;
    case VERTEX_DATA:
      if(vtkGraph* const graph = vtkGraph::SafeDownCast(input))
      {
        if(graph->GetVertexData())
        {
          data->ShallowCopy(graph->GetVertexData());
        }
      }
      break;
    case EDGE_DATA:
      if(vtkGraph* const graph = vtkGraph::SafeDownCast(input))
      {
        if(graph->GetEdgeData())
        {
          data->ShallowCopy(graph->GetEdgeData());
        }
      }
      break;
  }

  output->SetRowData(data);
  data->Delete();
  return 1;
}

//---------------------------------------------------------------------------
void vtkDataObjectToTable::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FieldType: " << this->FieldType << endl;
}
