cmake_minimum_required(VERSION 3.8...3.12 FATAL_ERROR)

project (Step1)

find_package(VTK REQUIRED
  COMPONENTS
      CommonCore
      FiltersSources
      FiltersSources
      InteractionStyle
      RenderingOpenGL2
    )

add_executable(Cone MACOSX_BUNDLE Cone.cxx)

vtk_module_autoinit(
  TARGETS Cone
  MODULES ${VTK_LIBRARIES}
  )
target_link_libraries(Cone ${VTK_LIBRARIES})
