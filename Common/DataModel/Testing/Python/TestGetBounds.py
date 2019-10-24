#!/usr/bin/env python
# -*- coding: utf-8 -*-

import vtk
import sys

# Test speed of compute bounds in vtkPolyData, vtkPoints, and
# vtkBoundingBox.

# Control model size
res = 500

# Uncomment if you want to use as a little interactive program
#if len(sys.argv) >= 2 :
#    res = int(sys.argv[1])
#else:
#    res = 500

# Data source
plane = vtk.vtkPlaneSource()
plane.SetResolution(res,res)
plane.Update()

output = plane.GetOutput()
points = output.GetPoints()
box = [0.0,0.0,0.0,0.0,0.0,0.0]

print("Input polydata:")
print("\tNum Points: {0}".format(output.GetNumberOfPoints()))
print("\tNum Cells: {0}".format(output.GetNumberOfCells()))
timer = vtk.vtkTimerLog()

# Currently vtkPolyData takes into account cells that are connected to
# points; hence only connected points (i.e., points used by cells) are
# considered.

# Compute bounds on polydaya
points.Modified()
timer.StartTimer()
output.GetBounds(box)
timer.StopTimer()
time = timer.GetElapsedTime()
print("vtkPolyData::ComputeBounds():")
print("\tTime: {0}".format(time))
print("\tBounds: {0}".format(box))

assert box[0] == -0.5
assert box[1] ==  0.5
assert box[2] == -0.5
assert box[3] ==  0.5
assert box[4] ==  0.0
assert box[5] ==  0.0

# Uses vtkPoints::ComputeBounds() which uses threaded vtkSMPTools and
# vtkArrayDispatch (see vtkDataArrayPrivate.txx). In other words, cell
# connectivity is not taken into account.
points.Modified()
timer.StartTimer()
points.GetBounds(box)
timer.StopTimer()
time = timer.GetElapsedTime()
print("vtkPoints::ComputeBounds():")
print("\tTime: {0}".format(time))
print("\tBounds: {0}".format(box))

assert box[0] == -0.5
assert box[1] ==  0.5
assert box[2] == -0.5
assert box[3] ==  0.5
assert box[4] ==  0.0
assert box[5] ==  0.0

# Uses vtkBoundingBox with vtkSMPTools. This method takes into account
# a pointUses array to only consider selected points.
bbox = vtk.vtkBoundingBox()
timer.StartTimer()
bbox.ComputeBounds(points,box)
timer.StopTimer()
time = timer.GetElapsedTime()
print("vtkBoundingBox::ComputeBounds():")
print("\tTime: {0}".format(time))
print("\tBounds: {0}".format(box))

assert box[0] == -0.5
assert box[1] ==  0.5
assert box[2] == -0.5
assert box[3] ==  0.5
assert box[4] ==  0.0
assert box[5] ==  0.0
