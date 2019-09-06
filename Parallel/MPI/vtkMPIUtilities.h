/*===========================================================================*/
/* Distributed under OSI-approved BSD 3-Clause License.                      */
/* For copyright, see the following accompanying files or https://vtk.org:   */
/* - VTKm-Copyright.txt                                                      */
/*===========================================================================*/
#ifndef vtkMPIUtilities_h
#define vtkMPIUtilities_h

#include "vtkParallelMPIModule.h" // For export macro

// Forward declarations
class vtkMPIController;

namespace vtkMPIUtilities
{

// Description:
// Rank 0 prints the user-supplied formatted message to stdout.
// This method works just like printf, but, requires an additional
// argument to specify the MPI controller for the application.
// NOTE: This is a collective operation, all ranks in the given communicator
// must call this method.
VTKPARALLELMPI_EXPORT
void Printf(vtkMPIController* comm, const char* format, ...);

// Description:
// Each rank, r_0 to r_{N-1}, prints the formatted message to stdout in
// rank order. That is, r_i prints the supplied message right after r_{i-1}.
// NOTE: This is a collective operation, all ranks in the given communicator
// must call this method.
VTKPARALLELMPI_EXPORT
void SynchronizedPrintf(vtkMPIController* comm, const char* format, ...);

} // END namespace vtkMPIUtilities

#endif // vtkMPIUtilities_h
// VTK-HeaderTest-Exclude: vtkMPIUtilities.h
