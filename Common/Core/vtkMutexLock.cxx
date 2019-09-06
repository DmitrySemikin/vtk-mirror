/*===========================================================================*/
/* Distributed under OSI-approved BSD 3-Clause License.                      */
/* For copyright, see the following accompanying files or https://vtk.org:   */
/* - VTKm-Copyright.txt                                                      */
/*===========================================================================*/
#include "vtkMutexLock.h"
#include "vtkObjectFactory.h"

#ifdef VTK_USE_WIN32_THREADS
# include "vtkWindows.h"
#endif

vtkStandardNewMacro(vtkMutexLock);

// New for the SimpleMutex
vtkSimpleMutexLock *vtkSimpleMutexLock::New()
{
  return new vtkSimpleMutexLock;
}

// Construct a new vtkMutexLock
vtkSimpleMutexLock::vtkSimpleMutexLock()
{
#ifdef VTK_USE_WIN32_THREADS
  this->MutexLock = CreateMutex( nullptr, FALSE, nullptr );
#endif

#ifdef VTK_USE_PTHREADS
  pthread_mutex_init(&(this->MutexLock), nullptr);
#endif
}

// Destruct the vtkMutexVariable
vtkSimpleMutexLock::~vtkSimpleMutexLock()
{
#ifdef VTK_USE_WIN32_THREADS
  CloseHandle(this->MutexLock);
#endif

#ifdef VTK_USE_PTHREADS
  pthread_mutex_destroy( &this->MutexLock);
#endif
}

// Lock the vtkMutexLock
void vtkSimpleMutexLock::Lock()
{
#ifdef VTK_USE_WIN32_THREADS
  WaitForSingleObject( this->MutexLock, INFINITE );
#endif

#ifdef VTK_USE_PTHREADS
  pthread_mutex_lock( &this->MutexLock);
#endif
}

// Unlock the vtkMutexLock
void vtkSimpleMutexLock::Unlock()
{
#ifdef VTK_USE_WIN32_THREADS
  ReleaseMutex( this->MutexLock );
#endif

#ifdef VTK_USE_PTHREADS
  pthread_mutex_unlock( &this->MutexLock);
#endif
}

void vtkMutexLock::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

