/*===========================================================================*/
/* Distributed under OSI-approved BSD 3-Clause License.                      */
/* For copyright, see the following accompanying files or https://vtk.org:   */
/* - VTK-Copyright.txt                                                       */
/*===========================================================================*/
#include "vtkUnstructuredGridVolumeMapper.h"

#include "vtkDataSet.h"
#include "vtkExecutive.h"
#include "vtkInformation.h"
#include "vtkUnstructuredGrid.h"


// Construct a vtkUnstructuredGridVolumeMapper with empty scalar input and
// clipping off.
vtkUnstructuredGridVolumeMapper::vtkUnstructuredGridVolumeMapper()
{
  this->BlendMode = vtkUnstructuredGridVolumeMapper::COMPOSITE_BLEND;
}

vtkUnstructuredGridVolumeMapper::~vtkUnstructuredGridVolumeMapper() = default;

void vtkUnstructuredGridVolumeMapper::SetInputData( vtkDataSet *genericInput )
{
  vtkUnstructuredGridBase *input =
    vtkUnstructuredGridBase::SafeDownCast( genericInput );

  if ( input )
  {
    this->SetInputData( input );
  }
  else
  {
    vtkErrorMacro("The SetInput method of this mapper requires "
                  "vtkUnstructuredGridBase as input");
  }
}

void vtkUnstructuredGridVolumeMapper::SetInputData( vtkUnstructuredGridBase *input )
{
  this->SetInputDataInternal(0, input);
}

vtkUnstructuredGridBase *vtkUnstructuredGridVolumeMapper::GetInput()
{
  if (this->GetNumberOfInputConnections(0) < 1)
  {
    return nullptr;
  }
  return vtkUnstructuredGridBase::SafeDownCast(
    this->GetExecutive()->GetInputData(0, 0));
}


// Print the vtkUnstructuredGridVolumeMapper
void vtkUnstructuredGridVolumeMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Blend Mode: " << this->BlendMode << endl;
}

//----------------------------------------------------------------------------
int vtkUnstructuredGridVolumeMapper::FillInputPortInformation(
  int vtkNotUsed( port ), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(),
            "vtkUnstructuredGridBase");
  return 1;
}

