
set(classes
  vtkTDXDevice
  vtkTDXinteractorStyleGeo)

if (WIN32)
  list(APPEND classes vtkTDxWinDevice)
endif ()
set(classes
  vtkTDXDevice
  vtkTDXinteractorStyleGeo)

if (WIN32)
  list(APPEND classes vtkTDxWinDevice)
endif ()

if (APPLE)
  list(APPEND classes vtkTDxMacDevice)
endif ()

if (VTK_USE_X)
  list(APPEND classes vtkTDxUnixDevice)
  vtk_module_find_package(
    PACKAGE X11)
  vtk_module_link(VTK::RenderingUI
    PUBLIC
      X11::X11)
endif ()
  
vtk_module_add_module(VTK::RenderingTDx
  CLASSES ${classes}
)

if (WIN32)
#required because of the #import command needed on windows to allow #import of stuff from TDX. This page is pretty clear on the subject.
# https://helgeklein.com/blog/2019/12/enabling-multi-processor-parallel-builds-in-visual-studio/ or just google "#import /MP1"
set_property(SOURCE vtkTDxWindowsDevice.cxx APPEND
  PROPERTY
    COMPILE_FLAGS "$<$<CXX_COMPILER_ID:MSVC>:/MP1>")
endif()
