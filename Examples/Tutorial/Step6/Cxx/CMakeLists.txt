cmake_minimum_required(VERSION 3.8...3.12 FATAL_ERROR)


PROJECT (Step6)

find_package(VTK REQUIRED
  COMPONENTS
      CommonCore
      FiltersSources
      InteractionStyle
      InteractionWidgets
      RenderingOpenGL2
    )

add_executable(Cone6 MACOSX_BUNDLE Cone6.cxx)

vtk_module_autoinit(
  TARGETS Cone6
  MODULES ${VTK_LIBRARIES}
  )
target_link_libraries(Cone6 ${VTK_LIBRARIES})
