#!/usr/bin/env python
import vtk

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
plane1.SetXResolution(12)
plane1.SetYResolution(12)
plane1.SetOrigin(0,0,0)
plane1.SetPoint1(10,0,0)
plane1.SetPoint2(0,10,0)

plane2 = vtk.vtkPlaneSource()
plane2.SetXResolution(25)
plane2.SetYResolution(25)
plane2.SetOrigin(2.5,2.5,0)
plane2.SetPoint1(7.5,2.5,0)
plane2.SetPoint2(2.5,7.5,0)

imp = vtk.vtkImprintFilter()
imp.SetTargetConnection(plane1.GetOutputPort())
imp.SetImprintConnection(plane2.GetOutputPort())
imp.Update()

mapper = vtk.vtkPolyDataMapper()
mapper.SetInputConnection(imp.GetOutputPort())

actor = vtk.vtkActor()
actor.SetMapper(mapper)

ren.AddActor(actor)

renWin.Render()
iren.Start()
