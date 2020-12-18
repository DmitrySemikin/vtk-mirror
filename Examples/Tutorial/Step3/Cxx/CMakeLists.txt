cmake_minimum_required(VERSION 3.8...3.12 FATAL_ERROR)


project (Step3)
find_package(VTK REQUIRED
  COMPONENTS
      CommonCore
      FiltersSources
      InteractionStyle
      RenderingOpenGL2
)

add_executable(Cone3 MACOSX_BUNDLE Cone3.cxx)

vtk_module_autoinit(
  TARGETS Cone3
  MODULES ${VTK_LIBRARIES}
  )
target_link_libraries(Cone3 ${VTK_LIBRARIES})
