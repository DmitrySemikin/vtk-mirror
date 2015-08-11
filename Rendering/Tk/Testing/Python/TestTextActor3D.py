#!/usr/bin/env python

import sys
import math
from functools import partial
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

try:
    # for Python2
    import Tkinter as tk
    from Tkinter import Pack
except ImportError:
    # for Python3
    import tkinter as tk
    from tkinter import Pack

#from vtk.tk.vtkTkRenderWindowInteractor import vtkTkRenderWindowInteractor
from vtk.tk.vtkTkRenderWidget import vtkTkRenderWidget
from vtk.tk.vtkTkImageViewerWidget import vtkTkImageViewerWidget

# Tkinter constants.
E = tk.E
W = tk.W
N = tk.N
S = tk.S
HORIZONTAL = tk.HORIZONTAL
VERTICAL = tk.VERTICAL
RIGHT = tk.RIGHT
LEFT = tk.LEFT
TOP = tk.TOP
BOTTOM = tk.BOTTOM
X = tk.X
BOTH = tk.BOTH
NO = tk.NO
YES = tk.YES
NORMAL = tk.NORMAL
DISABLED = tk.DISABLED
TRUE = tk.TRUE
FALSE = tk.FALSE
GROOVE = tk.GROOVE
INSERT = tk.INSERT
END = tk.END

class TestTextActor3DViewer(Testing.vtkTest):
    '''
    Provide a testing framework for for TestTextActor3D.

    Note:
        root, the top-level widget for Tk,
        tkrw, the vtkTkRenderWidget and
        renWin, the vtkRenderindow
        are accessible from any function in this class
        after SetUp() has run.
    '''

    def SetUp(self):
        '''
        Set up cursor3D
        '''

        def OnClosing():
            self.root.quit()

        def AddSphere(ren):
            objSource = vtk.vtkSphereSource()

            objMapper = vtk.vtkPolyDataMapper()
            objMapper.SetInputConnection(objSource.GetOutputPort())

            objActor = vtk.vtkActor()
            objActor.SetMapper(objMapper)
            objActor.GetProperty().SetRepresentationToWireframe()

            ren.AddActor(objActor)

        def AddOneTextActor(baseTextProp):
            name = "ia"
            self.textActors[name] = vtk.vtkTextActor3D()
            # This adjustment is needed to reduce the difference
            # between the Tcl and Python versions.
            self.textActors[name].SetOrigin(0, -0.127878, 0)


            tprop = self.textActors[name].GetTextProperty()
            tprop.ShallowCopy(baseTextProp)
            tprop.SetColor(1,0,0)

        # Add many text actors.
        def AddManyTextActors(baseTextProp):
            lut = vtk.vtkColorTransferFunction()
            lut.SetColorSpaceToHSV()
            lut.AddRGBPoint(0.0, 0.0, 1.0, 1.0)
            lut.AddRGBPoint(1.0, 1.0, 1.0, 1.0)

            for i in range(0, 10):
                name = "ia" + str(i)

                self.textActors[name] = vtk.vtkTextActor3D()
                self.textActors[name].SetOrientation(0, i*36, 0)
                #self.textActors[name].SetPosition(math.cos(i * 0.0314), 0, 0)
                # This adjustment is needed to reduce the diffierence
                # between the Tcl and Python versions.
                self.textActors[name].SetOrigin(0, -0.127878, 0)

                tprop = self.textActors[name].GetTextProperty()
                tprop.ShallowCopy(baseTextProp)
                value = i / 10.0
                tprop.SetColor(lut.GetColor(value))

            del lut

        # Update all text actors
        def UpdateTextActors(event):
            orientation = self.scaleOrientation.get()
            fontSize = self.scaleFontSize.get()
            scale = self.scaleScale.get() / 10000.0
            text = self.entryText.get("1.0",'end+1c')
            opacity = self.scaleOpacity.get()

            for actor in self.textActors.values():
                actor.SetScale(scale)
                actor.SetInput(text)
                tprop = actor.GetTextProperty()
                tprop.SetFontSize(fontSize)
                tprop.SetOrientation(orientation)
                tprop.SetOpacity(opacity)

            self.renWin.Render()

        ren = vtk.vtkRenderer()
        ren.SetBackground(0.1, 0.2, 0.4)
        self.renWin = vtk.vtkRenderWindow()
        self.renWin.AddRenderer(ren)
        #self.renWin.SetSize(600, 600)

        self.root = tk.Tk()
        self.root.title("TestTextActor3D.py")
        # Define what to do when the user explicitly closes a window.
        self.root.protocol("WM_DELETE_WINDOW", OnClosing)

        # The Tk render widget.
        self.tkrw = vtkTkRenderWidget(
                        self.root, width=450, height=450, rw=self.renWin)
        self.tkrw.BindTkRenderWidget()
        #self.renWin.GetInteractor().GetInteractorStyle().SetCurrentStyleToTrackballCamera()
        self.tkrw.pack(side=LEFT, fill=BOTH, expand=YES)

        # Base text property
        baseTextProp = vtk.vtkTextProperty()
        baseTextProp.SetFontSize(48)
        baseTextProp.ShadowOn()
        baseTextProp.SetColor(1.0, 0.0, 0.0)
        baseTextProp.SetFontFamilyToArial()

        baseScale = 0.0025

        baseText = "This is a test"

        # The text actors
        self.textActors = dict()

        scaleLength = 200
        controls = tk.Frame(self.root, relief=GROOVE, bd=2)
        controls.pack(
                padx=2, pady=2, anchor=N+W, side=LEFT, fill=BOTH, expand=NO)

        # Add control of text.
        self.entryText = tk.Text(controls, height=1, width=25)
        self.entryText.insert(INSERT,baseText)
        self.entryText.pack(padx=4, pady=4, side=TOP, fill=X, expand=NO)
        self.entryText.bind('<Return>',UpdateTextActors)
        self.entryText.bind('<FocusOut>',UpdateTextActors)

        # Add control of orientation.
        self.scaleOrientation = tk.Scale(controls,
                                from_=0, to=360, res=1,
                                length= scaleLength,
                                orient=HORIZONTAL,
                                label="Text orientation:",
                                command=UpdateTextActors)
        self.scaleOrientation.set(baseTextProp.GetOrientation())
        self.scaleOrientation.pack(side=TOP, fill=X, expand=NO)

        # Add control of font size.
        self.scaleFontSize = tk.Scale(controls,
                                from_=5, to=150, res=1,
                                length= scaleLength,
                                orient=HORIZONTAL,
                                label="Font Size:",
                                command=UpdateTextActors)
        self.scaleFontSize.set(baseTextProp.GetFontSize())
        self.scaleFontSize.pack(side=TOP, fill=X, expand=NO)

        # Add control of scale.
        self.scaleScale = tk.Scale(controls,
                                from_=0, to=100, res=1,
                                length= scaleLength,
                                orient=HORIZONTAL,
                                label="Actor scale:",
                                command=UpdateTextActors)
        self.scaleScale.set(baseScale * 10000.0)
        self.scaleScale.pack(side=TOP, fill=X, expand=NO)

        # Add control of scale.
        self.scaleOpacity = tk.Scale(controls,
                                from_=0, to=1.0, res=0.01,
                                length= scaleLength,
                                orient=HORIZONTAL,
                                label="Text opacity:",
                                command=UpdateTextActors)
        self.scaleOpacity.set(baseTextProp.GetOpacity())
        self.scaleOpacity.pack(side=TOP, fill=X, expand=NO)

        # Create and add all the text actors.
        if False:
            AddSphere(ren)
            AddOneTextActor(baseTextProp)
            ren.ResetCamera()
        else:
            AddManyTextActors(baseTextProp)
            ren.ResetCamera()

            ren.GetActiveCamera().Elevation(30.0)
            ren.GetActiveCamera().Dolly(0.40)

        UpdateTextActors(0)

        for actor in self.textActors.itervalues():
            ren.AddActor(actor)

    def DoIt(self):
        self.SetUp()
        self.renWin.Render()
        self.tkrw.Render()
        self.root.update()
        # If you want to interact and use the sliders etc,
        # uncomment the following line.
        #self.root.mainloop()
        img_file = "TestTextActor3D.png"
        Testing.compareImage(self.renWin, Testing.getAbsImagePath(img_file))
        Testing.interact()

if __name__ == '__main__':
    cases = [(TestTextActor3DViewer, 'DoIt')]
    del TestTextActor3DViewer
    Testing.main(cases)
