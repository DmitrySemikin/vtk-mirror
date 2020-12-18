set(classes
  vtkFiberSurface
  vtkTensorFieldCriticalCells)

vtk_module_add_module(VTK::FiltersTopology
  CLASSES ${classes})
