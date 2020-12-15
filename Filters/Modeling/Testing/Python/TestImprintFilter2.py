#!/usr/bin/env python
import vtk

# Control test size
res = 6

# create planes
# Create the RenderWindow, Renderer
#
ren = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer( ren )

iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# create pipeline. Use two plane sources:
# one plane imprints on the other plane.
#
plane1 = vtk.vtkPlaneSource()
plane1.SetXResolution(res)
plane1.SetYResolution(res)
plane1.SetOrigin(0,0,0)
plane1.SetPoint1(10,0,0)
plane1.SetPoint2(0,10,0)

plane2 = vtk.vtkPlaneSource()
plane2.SetXResolution(25)
plane2.SetYResolution(25)
plane2.SetOrigin(5,5,0)
plane2.SetPoint1(7.5,5,0)
plane2.SetPoint2(5,7.5,0)

tri = vtk.vtkDelaunay2D()
tri.SetInputConnection(plane1.GetOutputPort())
tri.Update()

mapper = vtk.vtkPolyDataMapper()
mapper.SetInputConnection(tri.GetOutputPort())

actor = vtk.vtkActor()
actor.SetMapper(mapper)

tri2 = vtk.vtkDelaunay2D()
tri2.SetInputConnection(plane2.GetOutputPort())
tri2.Update()

mapper2 = vtk.vtkPolyDataMapper()
mapper2.SetInputConnection(tri2.GetOutputPort())

actor2 = vtk.vtkActor()
actor2.SetMapper(mapper2)
actor2.GetProperty().SetColor(1,0,0)

append = vtk.vtkAppendPolyData()
append.AddInputConnection(tri.GetOutputPort())
append.AddInputConnection(tri2.GetOutputPort())
append.Update()

tri3 = vtk.vtkDelaunay2D()
tri3.SetInputConnection(append.GetOutputPort())
tri3.Update()

mapper3 = vtk.vtkPolyDataMapper()
mapper3.SetInputConnection(tri3.GetOutputPort())

actor3 = vtk.vtkActor()
actor3.SetMapper(mapper3)
actor3.GetProperty().SetColor(1,0,0)

ren.AddActor(actor)
ren.AddActor(actor2)
#ren.AddActor(actor3)

renWin.Render()
iren.Start()
