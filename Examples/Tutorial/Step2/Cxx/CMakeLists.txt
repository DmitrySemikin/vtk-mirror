cmake_minimum_required(VERSION 3.8...3.12 FATAL_ERROR)


project (Step2)
find_package(VTK REQUIRED
  COMPONENTS
      CommonCore
      FiltersSources
      FiltersSources
      InteractionStyle
      RenderingOpenGL2
)

add_executable(Cone2 MACOSX_BUNDLE Cone2.cxx)
vtk_module_autoinit(
  TARGETS Cone2
  MODULES ${VTK_LIBRARIES}
  )
target_link_libraries(Cone2 ${VTK_LIBRARIES})
