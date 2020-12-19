set(classes
  vtkGenericMovieWriter
  vtkMP4Writer)

if (WIN32)
  # Check if VideoForWindows is supported. This also adds an option
  # VTK_USE_VIDEO_FOR_WINDOWS that is set to ON/OFF by default based on
  # whether VideoForWindows was found.
  include(vtkTestVideoForWindows)

  if (VTK_USE_VIDEO_FOR_WINDOWS)
    list(APPEND classes vtkAVIWriter)
  endif ()

  # Check if Media Foundation is supported. This also adds an option
  # VTK_USE_MICROSOFT_MEDIA_FOUNDATION that is set to ON/OFF by default based
  # whether Microsoft Media Foundation was found.
  include(vtkTestMicrosoftMediaFoundation)

  if (VTK_USE_MICROSOFT_MEDIA_FOUNDATION)
    vtk_object_factory_declare(
      BASE vtkMP4Writer
      OVERRIDE vtkWin32MP4Writer)
    list(APPEND classes vtkWin32MP4Writer)
  endif ()
elseif (APPLE)
  vtk_object_factory_declare(
    BASE vtkMP4Writer
    OVERRIDE vtkMacOSMP4Writer)
  set_source_files_properties(
    vtkMacOSMP4Writer.mm
    PROPERTIES
      COMPILE_FLAGS "${VTK_REQUIRED_OBJCXX_FLAGS}")
  list(APPEND sources vtkMacOSMP4Writer.mm)
  list(APPEND headers vtkMacOSMP4Writer.h)
endif ()

vtk_object_factory_configure(
  SOURCE_FILE vtk_object_factory_source
  HEADER_FILE vtk_object_factory_header
  EXPORT_MACRO "VTKIOMOVIE_EXPORT")

message(STATUS "${vtk_object_factory_source}")

vtk_module_add_module(VTK::IOMovie
  CLASSES ${classes}
  SOURCES ${sources} ${vtk_object_factory_source}
  HEADERS ${headers}
  PRIVATE_HEADERS ${vtk_object_factory_header}
)

set(libs)
if (WIN32)
  if (VTK_USE_VIDEO_FOR_WINDOWS)
    list(APPEND libs vfw32)
  endif ()

  if (VTK_USE_MICROSOFT_MEDIA_FOUNDATION)
    list(APPEND libs mfreadwrite mfplat mfuuid)
  endif()

  vtk_module_link(VTK::IOMovie
    PRIVATE ${libs})
endif ()
