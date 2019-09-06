/*===========================================================================*/
/* Distributed under OSI-approved BSD 3-Clause License.                      */
/* For copyright, see the following accompanying files or https://vtk.org:   */
/* - VTKm-Copyright.txt                                                      */
/* - Sandia-Copyright.txt                                                    */
/*===========================================================================*/
#include "vtkmAverageToPoints.h"

#include "vtkDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"

#include "vtkmlib/Storage.h"
#include "vtkmlib/DataSetConverters.h"
#include "vtkmlib/ArrayConverters.h"

#include "vtkmCellSetExplicit.h"
#include "vtkmCellSetSingleType.h"
#include "vtkmFilterPolicy.h"

#include <vtkm/filter/PointAverage.h>

vtkStandardNewMacro(vtkmAverageToPoints)

//------------------------------------------------------------------------------
vtkmAverageToPoints::vtkmAverageToPoints()
{
}

//------------------------------------------------------------------------------
vtkmAverageToPoints::~vtkmAverageToPoints()
{
}

//------------------------------------------------------------------------------
int vtkmAverageToPoints::RequestData(vtkInformation *vtkNotUsed(request),
                                     vtkInformationVector **inputVector,
                                     vtkInformationVector *outputVector)
{
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkDataSet *input = vtkDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkDataSet* output = vtkDataSet::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  output->ShallowCopy(input);

  //grab the input array to process to determine the field we want to average
  int association = this->GetInputArrayAssociation(0, inputVector);
  auto fieldArray = this->GetInputArrayToProcess(0, inputVector);
  if (association != vtkDataObject::FIELD_ASSOCIATION_CELLS ||
      fieldArray == nullptr ||
      fieldArray->GetName() == nullptr || fieldArray->GetName()[0] == '\0')
  {
    vtkErrorMacro(<< "Invalid field: Requires a cell field with a valid name.");
    return 0;
  }

  const char* fieldName = fieldArray->GetName();

  try
  {
    //convert the input dataset to a vtkm::cont::DataSet
    vtkm::cont::DataSet in = tovtkm::Convert(input);
    auto field = tovtkm::Convert(fieldArray, association);
    in.AddField(field);

    vtkmInputFilterPolicy policy;
    vtkm::filter::PointAverage filter;
    filter.SetActiveField(fieldName, vtkm::cont::Field::Association::CELL_SET);
    filter.SetOutputFieldName(fieldName); //should we expose this control?

    auto result = filter.Execute(in, policy);

    // convert back the dataset to VTK, and add the field as a point field
    vtkDataArray* resultingArray =
      fromvtkm::Convert(result.GetPointField(fieldName));
    if (resultingArray == nullptr)
    {
      vtkErrorMacro(<< "Unable to convert result array from VTK-m to VTK");
      return 0;
    }

    output->GetPointData()->AddArray(resultingArray);
    resultingArray->FastDelete();
  }
  catch (const vtkm::cont::Error& e)
  {
    vtkErrorMacro(<< "VTK-m error: " << e.GetMessage());
    return 0;
  }

  return 1;
}

//------------------------------------------------------------------------------
void vtkmAverageToPoints::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
