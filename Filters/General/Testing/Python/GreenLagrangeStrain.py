#!/usr/bin/env python
import math
import numpy
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Hexahedron
points = vtk.vtkPoints()
points.SetNumberOfPoints(8)
points.InsertPoint(0, 0, 0, 0)
points.InsertPoint(1, 1, 0, 0)
points.InsertPoint(2, 1, 1, 0)
points.InsertPoint(3, 0, 1, 0)
points.InsertPoint(4, 0, 0, 1)
points.InsertPoint(5, 1, 0, 1)
points.InsertPoint(6, 1, 1, 1)
points.InsertPoint(7, 0, 1, 1)
#for k_point in xrange(8): print points.GetPoint(k_point)

hexahedron = vtk.vtkHexahedron()
for k_point in xrange(8):
    hexahedron.GetPointIds().SetId(k_point, k_point)

cell_array = vtk.vtkCellArray()
cell_array.InsertNextCell(hexahedron)

farray_disp = vtk.vtkFloatArray()
farray_disp.SetName("displacement")
farray_disp.SetNumberOfComponents(3)
farray_disp.SetNumberOfTuples(8)

ugrid_hex = vtk.vtkUnstructuredGrid()
ugrid_hex.SetPoints(points)
ugrid_hex.SetCells(hexahedron.GetCellType(), cell_array)
ugrid_hex.GetPointData().AddArray(farray_disp)
ugrid_hex.GetPointData().SetActiveVectors("displacement")

# deformation gradient
F_lst  = []
F_lst += [numpy.array([[2,0,0],
                       [0,1,0],
                       [0,0,1]])]
F_lst += [numpy.array([[1,1,0],
                       [0,1,0],
                       [0,0,1]])]
alpha = math.pi/2
F_lst += [numpy.array([[+math.cos(alpha),-math.sin(alpha),0],
                       [+math.sin(alpha),+math.cos(alpha),0],
                       [               0,               0,1]])]

for F in F_lst:
    print "F = " + str(F)

    # displacement gradient
    GU = F - numpy.eye(3)
    print "GU = " + str(GU)

    # small strain
    e = (GU + numpy.transpose(GU))/2
    print "e = " + str(e)

    # green lagrange strain
    E = e + numpy.dot(numpy.transpose(GU), GU)/2
    print "E = " + str(E)

    for k_point in xrange(8):
        farray_disp.SetTuple(k_point, numpy.dot(GU, points.GetPoint(k_point)))
        #print farray_disp.GetTuple(k_point)

    cell_derivatives = vtk.vtkCellDerivatives()
    cell_derivatives.SetVectorModeToPassVectors()
    cell_derivatives.SetTensorModeToComputeGradient()
    cell_derivatives.SetInputData(ugrid_hex)
    cell_derivatives.Update()
    vector_gradient = numpy.reshape(cell_derivatives.GetOutput().GetCellData().GetArray("VectorGradient").GetTuple(0), (3,3), "F")
    print "VectorGradient = " + str(vector_gradient)
    assert numpy.allclose(vector_gradient, GU)

    cell_derivatives = vtk.vtkCellDerivatives()
    cell_derivatives.SetVectorModeToPassVectors()
    cell_derivatives.SetTensorModeToComputeStrain()
    cell_derivatives.SetInputData(ugrid_hex)
    cell_derivatives.Update()
    strain = numpy.reshape(cell_derivatives.GetOutput().GetCellData().GetArray("Strain").GetTuple(0), (3,3), "F")
    print "Strain = " + str(strain)
    assert numpy.allclose(strain, e)

    cell_derivatives = vtk.vtkCellDerivatives()
    cell_derivatives.SetVectorModeToPassVectors()
    cell_derivatives.SetTensorModeToComputeGreenLagrangeStrain()
    cell_derivatives.SetInputData(ugrid_hex)
    cell_derivatives.Update()
    green_lagrange_strain = numpy.reshape(cell_derivatives.GetOutput().GetCellData().GetArray("GreenLagrangeStrain").GetTuple(0), (3,3), "F")
    print "GreenLagrangeStrain = " + str(green_lagrange_strain)
    assert numpy.allclose(green_lagrange_strain, E)
