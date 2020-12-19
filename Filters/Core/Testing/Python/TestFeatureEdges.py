#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

def CreateUnconnectedQuads():
  poly = vtk.vtkPolyData()
  pts  = vtk.vtkPoints()
  ids = vtk.vtkIdTypeArray()
  ids.SetName('point-ids')
  poly.GetPointData().AddArray(ids)
  poly.SetPoints(pts)
  quads = vtk.vtkCellArray()
  poly.SetPolys(quads)

  for y in [0,1]:
    for x in [0,1,1,2]:
      pid = pts.InsertNextPoint(float(x),float(y),0.0)
      ids.InsertNextValue(pid+101)
  for quad in [[0,1,5,4],[2,3,7,6]]:
    quads.InsertNextCell(4)
    for p in quad:
      quads.InsertCellPoint(p)
  return poly


class TestFeatureEdges(Testing.vtkTest):

  def testMerging(self):
    """ Merging mode copies only one of the coincident points at [1,0,0] """
    quads = CreateUnconnectedQuads()

    edges = vtk.vtkFeatureEdges()
    edges.SetInputData(quads)

    edges.Update()

    out=edges.GetOutput()
    self.assertEqual(6, out.GetNumberOfPoints())
    pid102=out.GetPointData().GetArray('point-ids').LookupValue(102)
    pid103=out.GetPointData().GetArray('point-ids').LookupValue(103)
    self.assertTrue(pid102 < 0 or pid103 < 0)

  def testNonMerging(self):
    """ Non-merging mode copies both the coincident points at [1,0,0] """
    quads = CreateUnconnectedQuads()

    edges = vtk.vtkFeatureEdges()
    edges.SetInputData(quads)

    edges.MergingOff()
    edges.Update()

    out=edges.GetOutput()
    self.assertEqual(8, out.GetNumberOfPoints())
    pid102=out.GetPointData().GetArray('point-ids').LookupValue(102)
    pid103=out.GetPointData().GetArray('point-ids').LookupValue(103)
    self.assertTrue(pid102 > -1 and pid103 > -1)

if __name__ == "__main__":
  Testing.main([(TestFeatureEdges, 'test')])
