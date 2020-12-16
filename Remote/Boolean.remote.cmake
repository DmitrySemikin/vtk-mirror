#
# A new boolean operations filter for VTK by Ronald Romer
#

vtk_fetch_module(Boolean
  "A new boolean operation filter by Ronald Romer"
  GIT_REPOSITORY https://github.com/lorensen/vtkbool
  # 12 September - fix warnings
  GIT_TAG 33ba44c57ad2cca726389102a2eea1a6259c47df
  )
