/*===========================================================================*/
/* Distributed under OSI-approved BSD 3-Clause License.                      */
/* For copyright, see the following accompanying files or https://vtk.org:   */
/* - VTK-Copyright.txt                                                       */
/*===========================================================================*/

#ifndef vtkWrapPythonType_h
#define vtkWrapPythonType_h

#include "vtkParse.h"
#include "vtkParseData.h"
#include "vtkParseHierarchy.h"

/* check whether a non-vtkObjectBase class is wrappable */
int vtkWrapPython_IsSpecialTypeWrappable(ClassInfo *data);

/* write out a python type object */
void vtkWrapPython_GenerateSpecialType(
  FILE *fp, const char *module, const char *classname,
  ClassInfo *data, FileInfo *finfo, HierarchyInfo *hinfo);

#endif /* vtkWrapPythonType_h */
/* VTK-HeaderTest-Exclude: vtkWrapPythonType.h */
