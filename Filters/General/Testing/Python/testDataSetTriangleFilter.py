#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Create the RenderWindow, Renderer, and RenderWindowInteractor
#
ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.SetMultiSamples(0)
renWin.AddRenderer(ren1)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
# create pipeline
#
PIECE = 0
NUMBER_OF_PIECES = 8

reader = vtk.vtkImageReader()
reader.SetDataByteOrderToLittleEndian()
reader.SetDataExtent(0,63,0,63,1,64)
reader.SetFilePrefix("" + str(VTK_DATA_ROOT) + "/Data/headsq/quarter")
reader.SetDataMask(0x7fff)
reader.SetDataSpacing(1.6,1.6,1.5)
clipper = vtk.vtkImageClip()
clipper.SetInputConnection(reader.GetOutputPort())
clipper.SetOutputWholeExtent(30,36,30,36,30,36)
tris = vtk.vtkDataSetTriangleFilter()
tris.SetInputConnection(clipper.GetOutputPort())
geom = vtk.vtkGeometryFilter()
geom.SetInputConnection(tris.GetOutputPort())
mapper1 = vtk.vtkPolyDataMapper()
mapper1.SetInputConnection(geom.GetOutputPort())
mapper1.ScalarVisibilityOn()
mapper1.SetScalarRange(0,1200)
mapper1.SetPiece(PIECE)
mapper1.SetNumberOfPieces(NUMBER_OF_PIECES)
actor1 = vtk.vtkActor()
actor1.SetMapper(mapper1)

reader2 = vtk.vtkImageReader()
reader2.SetDataByteOrderToLittleEndian()
reader2.SetDataExtent(0,63,0,63,1,64)
reader2.SetFilePrefix("" + str(VTK_DATA_ROOT) + "/Data/headsq/quarter")
reader2.SetDataMask(0x7fff)
reader2.SetDataSpacing(1.6,1.6,1.5)
clipper2 = vtk.vtkImageClip()
clipper2.SetInputConnection(reader2.GetOutputPort())
clipper2.SetOutputWholeExtent(30,36,30,36,30,36)
tris2 = vtk.vtkDataSetTriangleFilter()
tris2.SetInputConnection(clipper2.GetOutputPort())
pf = vtk.vtkProgrammableFilter()
pf.SetInputConnection(tris2.GetOutputPort())
def remove_ghosts():
    input = pf.GetInput()
    output = pf.GetOutputDataObject(0)
    output.ShallowCopy(input)
    output.RemoveGhostCells()
pf.SetExecuteMethod(remove_ghosts)
edges = vtk.vtkExtractEdges()
edges.SetInputConnection(pf.GetOutputPort())
mapper2 = vtk.vtkPolyDataMapper()
mapper2.SetInputConnection(edges.GetOutputPort())
mapper2.SetPiece(PIECE)
mapper2.SetNumberOfPieces(NUMBER_OF_PIECES)
mapper2.GetInput().RemoveGhostCells()
actor2 = vtk.vtkActor()
actor2.SetMapper(mapper2)
# add the actor to the renderer; set the size
#
ren1.AddActor(actor1)
ren1.AddActor(actor2)
renWin.SetSize(450,450)
ren1.SetBackground(1,1,1)
renWin.Render()

print(reader.GetOutputDataObject(0))
print(clipper.GetOutputDataObject(0))
print(tris.GetOutputDataObject(0))
print(geom.GetOutputDataObject(0))

# render the image
#
iren.Initialize()
# prevent the tk window from showing up then start the event loop
# --- end of script --
