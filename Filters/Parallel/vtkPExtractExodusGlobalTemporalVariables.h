/*===========================================================================*/
/* Distributed under OSI-approved BSD 3-Clause License.                      */
/* For copyright, see the following accompanying files or https://vtk.org:   */
/* - VTK-Copyright.txt                                                       */
/*===========================================================================*/
/**
 * @class vtkPExtractExodusGlobalTemporalVariables
 * @brief parallel version of vtkExtractExodusGlobalTemporalVariables.
 *
 * vtkPExtractExodusGlobalTemporalVariables is a parallel version of
 * vtkExtractExodusGlobalTemporalVariables that handles synchronization between
 * multiple ranks. Since vtkPExodusIIReader has explicit sycnchronization
 * between ranks its essential that downstream filters make consistent requests
 * on all ranks to avoid deadlocks. Since global variables need not be provided
 * on all ranks, without explicit coordination
 * vtkExtractExodusGlobalTemporalVariables may end up not making requests on
 * certain ranks causing deadlocks.
 */

#ifndef vtkPExtractExodusGlobalTemporalVariables_h
#define vtkPExtractExodusGlobalTemporalVariables_h

#include "vtkExtractExodusGlobalTemporalVariables.h"
#include "vtkFiltersParallelModule.h" // For export macro

class vtkMultiProcessController;

class VTKFILTERSPARALLEL_EXPORT vtkPExtractExodusGlobalTemporalVariables
  : public vtkExtractExodusGlobalTemporalVariables
{
public:
  static vtkPExtractExodusGlobalTemporalVariables* New();
  vtkTypeMacro(vtkPExtractExodusGlobalTemporalVariables, vtkExtractExodusGlobalTemporalVariables);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Get/Set the controller to use. By default
   * `vtkMultiProcessController::GlobalController` will be used.
   */
  void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  //@}
protected:
  vtkPExtractExodusGlobalTemporalVariables();
  ~vtkPExtractExodusGlobalTemporalVariables() override;

  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

private:
  vtkPExtractExodusGlobalTemporalVariables(
    const vtkPExtractExodusGlobalTemporalVariables&) = delete;
  void operator=(const vtkPExtractExodusGlobalTemporalVariables&) = delete;

  vtkMultiProcessController* Controller;
};

#endif
