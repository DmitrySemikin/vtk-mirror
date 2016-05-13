/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLFenceSync.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkOpenGLFenceSync.h"

#include "vtkObjectFactory.h"

#include "vtk_glew.h"

vtkStandardNewMacro(vtkOpenGLFenceSync)

struct vtkOpenGLFenceSync::Private
{
  Private() : SyncObject(0), Flushed(false) {}
  GLsync SyncObject;
  bool Flushed;
};

//------------------------------------------------------------------------------
void vtkOpenGLFenceSync::PrintSelf(std::ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void vtkOpenGLFenceSync::Mark()
{
  if (this->P->SyncObject != 0)
    {
    vtkErrorMacro("Mark() called multiple times without Reset()'ing.");
    return;
    }

  this->P->SyncObject = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
}

//------------------------------------------------------------------------------
bool vtkOpenGLFenceSync::IsFinished()
{
  return this->WaitForFinished(0);
}

//------------------------------------------------------------------------------
bool vtkOpenGLFenceSync::WaitForFinished(vtkTypeUInt64 timeout_ns)
{
  if (this->P->SyncObject == 0)
    {
    vtkErrorMacro("Called before Mark()!");
    return false;
    }

  GLenum result = glClientWaitSync(this->P->SyncObject, 0,
                                   static_cast<GLuint64>(timeout_ns));
  switch (result)
    {
    case GL_CONDITION_SATISFIED:
    case GL_ALREADY_SIGNALED:
      return true;
    case GL_TIMEOUT_EXPIRED:
    case GL_WAIT_FAILED:
      return false;
    default:
      vtkErrorMacro("Unknown result from glClientWaitSync: " << result);
      break;
    }

  return false;
}

//------------------------------------------------------------------------------
void vtkOpenGLFenceSync::Flush()
{
  if (this->P->SyncObject == 0)
    {
    vtkErrorMacro("Called before Mark()!");
    return;
    }

  if (this->P->Flushed)
    {
    vtkWarningMacro("Flush() called multiple times on same Mark(). This should "
                    "not be done and may affect performance.");
    }

  glClientWaitSync(this->P->SyncObject, GL_SYNC_FLUSH_COMMANDS_BIT, 0);
  this->P->Flushed = true;
}

//------------------------------------------------------------------------------
void vtkOpenGLFenceSync::Reset()
{
  if (this->P->SyncObject != 0)
    {
    glDeleteSync(this->P->SyncObject);
    this->P->SyncObject = 0;
    }

  this->P->Flushed = false;
}

//------------------------------------------------------------------------------
vtkOpenGLFenceSync::vtkOpenGLFenceSync()
  : P(new Private)
{
}

//------------------------------------------------------------------------------
vtkOpenGLFenceSync::~vtkOpenGLFenceSync()
{
  this->Reset();
  delete P;
}
