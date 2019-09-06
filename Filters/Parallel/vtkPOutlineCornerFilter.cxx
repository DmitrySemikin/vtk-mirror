/*===========================================================================*/
/* Distributed under OSI-approved BSD 3-Clause License.                      */
/* For copyright, see the following accompanying files or https://vtk.org:   */
/* - VTK-Copyright.txt                                                       */
/*===========================================================================*/
#include "vtkPOutlineCornerFilter.h"

#include "vtkCompositeDataSet.h"
#include "vtkDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationDataObjectKey.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkOutlineSource.h"
#include "vtkOutlineCornerSource.h"
#include "vtkPolyData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkOverlappingAMR.h"
#include "vtkDataObjectTree.h"
#include "vtkCompositeDataIterator.h"
#include "vtkDataObjectTreeIterator.h"
#include "vtkSmartPointer.h"
#include "vtkAppendPolyData.h"
#include "vtkBoundingBox.h"
#include "vtkUniformGrid.h"
#include "vtkAMRInformation.h"
#include "vtkNew.h"
#include "vtkPOutlineFilterInternals.h"
#include <vector>

vtkStandardNewMacro(vtkPOutlineCornerFilter);
vtkCxxSetObjectMacro(vtkPOutlineCornerFilter, Controller, vtkMultiProcessController);

vtkPOutlineCornerFilter::vtkPOutlineCornerFilter ()
{
  this->Controller = nullptr;
  this->SetController(vtkMultiProcessController::GetGlobalController());
  this->CornerFactor = 0.2;
  this->Internals = new vtkPOutlineFilterInternals;
  this->Internals->SetController(vtkMultiProcessController::GetGlobalController());
}

vtkPOutlineCornerFilter::~vtkPOutlineCornerFilter ()
{
  this->SetController(nullptr);
  this->Internals->SetController(nullptr);
  delete this->Internals;
}

// ----------------------------------------------------------------------------
void vtkPOutlineCornerFilter::SetCornerFactor(double cornerFactor)
{
  vtkDebugMacro(<< this->GetClassName()
                << " ("
                << this
                << "): setting "
                << "CornerFactor to "
                << CornerFactor );
  double tempCornerFactor =  (cornerFactor < 0.001
                              ? 0.001
                              : (cornerFactor > 0.5
                                 ? 0.5
                                 : cornerFactor));

  if ( this->CornerFactor != tempCornerFactor)
  {
    std::cerr << "CornerFactor: " << tempCornerFactor
              << std::endl;
    this->CornerFactor = tempCornerFactor;
    this->Internals->SetCornerFactor(tempCornerFactor);
    this->Modified();
  }
}

// ----------------------------------------------------------------------------
int vtkPOutlineCornerFilter::RequestData(
  vtkInformation *request,
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  this->Internals->SetIsCornerSource(true);
  return this->Internals->RequestData(request,inputVector,outputVector);
}

int vtkPOutlineCornerFilter::FillInputPortInformation(int, vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCompositeDataSet");
  return 1;
}

void vtkPOutlineCornerFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "CornerFactor: " << this->CornerFactor << "\n";
  os << indent << "Controller: " << this->Controller << endl;
}
