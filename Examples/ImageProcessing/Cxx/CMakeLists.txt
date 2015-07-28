cmake_minimum_required(VERSION 2.8.5 FATAL_ERROR)
if(POLICY CMP0025)
  cmake_policy(SET CMP0025 NEW) # CMake 3.0
endif()
if(POLICY CMP0053)
  cmake_policy(SET CMP0053 NEW) # CMake 3.1
endif()

PROJECT(ImageProcessing)

find_package(VTK COMPONENTS
  vtkIOImage
  vtkImagingGeneral
  vtkImagingCore
  vtkImagingMath
  vtkFiltersHybrid
  vtkInteractionStyle
  vtkRendering${VTK_RENDERING_BACKEND}
  vtkTestingRendering
  vtkImagingStencil
  vtkImagingStatistics
)
include(${VTK_USE_FILE})

add_executable(ImageSlicing MACOSX_BUNDLE ImageSlicing.cxx)
add_executable(SMPThreadedImageAlgorithm MACOSX_BUNDLE SMPThreadedImageAlgorithm.cxx)
target_link_libraries(ImageSlicing ${VTK_LIBRARIES})
target_link_libraries(SMPThreadedImageAlgorithm ${VTK_LIBRARIES})

if(BUILD_TESTING)
  if(vtkTestingRendering_LOADED)
   ######## Regression Testing ########
   set(vtk-example ImageProcessingExamples)
   set(TestImageSlicing_ARGS ${VTK_TEST_DATA_DIR}/Data/headsq/quarter)
   set(TestSMPThreadedImageAlgorithm_ARGS 4 500 1 5000 true)
   vtk_add_test_cxx(${vtk-example}CxxTests tests
     TestImageSlicing.cxx
     TestSMPThreadedImageAlgorithm.cxx
     )

   vtk_test_cxx_executable(${vtk-example}CxxTests tests
     RENDERING_FACTORY
     )
   configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/SMPBenchMarking.sh.in"
    "${CMAKE_CURRENT_BINARY_DIR}/SMPBenchMarking.sh"
    COPYONLY
   )
   configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/SMPBenchMarkingRunAllTests.sh.in"
    "${CMAKE_CURRENT_BINARY_DIR}/SMPBenchMarkingRunAllTests.sh"
    COPYONLY
   )
  # Unit Tests for vtkExtentTranslator
  set(TESTS  TestSlabMode  Test2DSplitMode  Test3DSplitMode  TestEmptyExtent  TestDefaultMode)
  set(TEST_EXE ExtentTranslatorUnitTest)
  set(TEST_SRCS vtkExtentTranslatorUnitTest.cxx)
  set(TEST_LIBS ${VTK_LIBRARIES})

  # This block creates the tests
  add_executable(${TEST_EXE} ${TEST_SRCS})
  target_link_libraries(${TEST_EXE} ${TEST_LIBS})
  foreach(TEST_NAME ${TESTS})
  add_test(NAME ${TEST_NAME} COMMAND ${TEST_EXE} ${TEST_NAME})
  endforeach()

  endif()
endif()
