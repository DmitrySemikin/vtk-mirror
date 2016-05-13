/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLFenceSync.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkOpenGLFenceSync - Allows arbitrary queries to ensure the OpenGL
// command stream has reached a certain point.
//
// .SECTION Description
// This class uses glFenceSync and glClientWaitSync to mark a synchronization
// point in the OpenGL command stream and test for when it has been processed.
// This is helpful for monitoring asynchronous operations, such as PBO reads of
// texture data, without blocking the CPU.

#ifndef vtkOpenGLFenceSync_h
#define vtkOpenGLFenceSync_h

#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkObject.h"

class VTKRENDERINGOPENGL2_EXPORT vtkOpenGLFenceSync: public vtkObject
{
public:
  static vtkOpenGLFenceSync* New();
  vtkTypeMacro(vtkOpenGLFenceSync, vtkObject)
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Immediately put a sychronization point into the OpenGL stream.
  void Mark();

  // Description:
  // Test whether the synchronization point has been processed by the GPU.
  // Does not block CPU to wait for the result.
  bool IsFinished();

  // Description:
  // Wait @a timeout_ns nanoseconds for the synchronization point to be reached.
  bool WaitForFinished(vtkTypeUInt64 timeout_ns);

  // Description:
  // Flush the command stream so that the synchronization point is posted to
  // the GPU's command queue.
  // @warning This should only be called once per Mark()!
  void Flush();

  // Description:
  // Deletes the synchronization object and prepares the class for reuse.
  void Reset();

protected:
  vtkOpenGLFenceSync();
  ~vtkOpenGLFenceSync();

private:
  vtkOpenGLFenceSync(const vtkOpenGLFenceSync&); // Not implemented
  void operator=(const vtkOpenGLFenceSync&); // Not implemented

  struct Private;
  Private *P;
};

#endif // vtkOpenGLFenceSync_h
