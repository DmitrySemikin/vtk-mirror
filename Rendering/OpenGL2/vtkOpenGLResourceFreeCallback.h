/*===========================================================================*/
/* Distributed under OSI-approved BSD 3-Clause License.                      */
/* For copyright, see the following accompanying files or https://vtk.org:   */
/* - VTK-Copyright.txt                                                       */
/*===========================================================================*/

#ifndef vtkOpenGLResourceFreeCallback_h
#define vtkOpenGLResourceFreeCallback_h

// Description:
// Provide a mechanism for making sure graphics resources are
// freed properly.

class vtkOpenGLRenderWindow;
class vtkWindow;

class vtkGenericOpenGLResourceFreeCallback
{
  public:
    vtkGenericOpenGLResourceFreeCallback() {
        this->VTKWindow = nullptr; this->Releasing = false; }
    virtual ~vtkGenericOpenGLResourceFreeCallback() { }

    // Called when the event is invoked
    virtual void Release() = 0;

    virtual void RegisterGraphicsResources(vtkOpenGLRenderWindow *rw) =  0;

    bool IsReleasing() {
      return this->Releasing; }

  protected:
    vtkOpenGLRenderWindow *VTKWindow;
    bool Releasing;
};

// Description:
// Templated member callback.
template <class T>
class vtkOpenGLResourceFreeCallback : public vtkGenericOpenGLResourceFreeCallback
{
public:
  vtkOpenGLResourceFreeCallback(T* handler, void (T::*method)(vtkWindow *))
  {
    this->Handler = handler;
    this->Method = method;
  }

  ~vtkOpenGLResourceFreeCallback() override { }

  void RegisterGraphicsResources(vtkOpenGLRenderWindow *rw) override {
    if (this->VTKWindow == rw)
    {
      return;
    }
    if (this->VTKWindow)
    {
      this->Release();
    }
    this->VTKWindow = rw;
    if (this->VTKWindow)
    {
      this->VTKWindow->RegisterGraphicsResources(this);
    }
  }

  // Called when the event is invoked
  void Release() override
  {
    if (this->VTKWindow && this->Handler && !this->Releasing)
    {
      this->Releasing = true;
      this->VTKWindow->PushContext();
      (this->Handler->*this->Method)(this->VTKWindow);
      this->VTKWindow->UnregisterGraphicsResources(this);
      this->VTKWindow->PopContext();
      this->VTKWindow = nullptr;
      this->Releasing = false;
    }
  }
protected:
  T* Handler;
  void (T::*Method)(vtkWindow *);
};

#endif
// VTK-HeaderTest-Exclude: vtkOpenGLResourceFreeCallback.h
