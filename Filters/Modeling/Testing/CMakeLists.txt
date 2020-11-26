vtk_module_test_data(
  Data/in_out_true.vtm
  Data/in_out_true/,REGEX:.*
  Data/in_out_false.vtm
  Data/in_out_false/,REGEX:.*
  Data/SemiDisk/SemiDisk-0.vtp
  Data/SemiDisk/SemiDisk-1.vtp)

add_subdirectory(Cxx)

if (VTK_WRAP_PYTHON)
  vtk_module_test_data(
    Data/SainteHelens.dem
    Data/headsq/,REGEX:.*
    Data/ironProt.vtk
    Data/nut.slc
    Data/ruler.png
    Data/usa.vtk
    Data/vtk.vtk)

  add_subdirectory(Python)
endif ()
