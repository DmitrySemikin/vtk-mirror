set(classes
  vtkAutoCorrelativeStatistics
  vtkBivariateLinearTableThreshold
  vtkComputeQuartiles
  vtkContingencyStatistics
  vtkCorrelativeStatistics
  vtkDescriptiveStatistics
  vtkHighestDensityRegionsStatistics
  vtkExtractFunctionalBagPlot
  vtkKMeansDistanceFunctorCalculator
  vtkKMeansDistanceFunctor
  vtkKMeansStatistics
  vtkMultiCorrelativeStatistics
  vtkOrderStatistics
  vtkPCAStatistics
  vtkStatisticsAlgorithm
  vtkStrahlerMetric
  vtkStreamingStatistics)

set(headers
  vtkStatisticsAlgorithmPrivate.h)

vtk_module_add_module(VTK::FiltersStatistics
  CLASSES ${classes}
  HEADERS ${headers})

if(MINGW AND CMAKE_BUILD_TYPE STREQUAL "Debug")
  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set_source_files_properties(
      vtkPCAStatistics.cxx
      PROPERTIES
        COMPILE_FLAGS -Wa,-mbig-obj)
  endif()
endif()
