cmake_minimum_required(VERSION 3.8...3.12 FATAL_ERROR)


PROJECT (Step5)

find_package(VTK REQUIRED
  COMPONENTS
  CommonCore
  FiltersSources
  InteractionStyle
  RenderingOpenGL2
)

add_executable(Cone5 MACOSX_BUNDLE Cone5.cxx)
vtk_module_autoinit(
  TARGETS Cone5
  MODULES ${VTK_LIBRARIES}
  )

target_link_libraries(Cone5 ${VTK_LIBRARIES})
