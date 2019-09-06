/*===========================================================================*/
/* Distributed under OSI-approved BSD 3-Clause License.                      */
/* For copyright, see the following accompanying files or https://vtk.org:   */
/* - VTK-Copyright.txt                                                       */
/*===========================================================================*/

#ifndef vtkWrapPythonEnum_h
#define vtkWrapPythonEnum_h

#include "vtkParse.h"
#include "vtkParseData.h"
#include "vtkParseHierarchy.h"

/* check whether an enum type will be wrapped */
int vtkWrapPython_IsEnumWrapped(
  HierarchyInfo *hinfo, const char *enumname);

/* find and mark all enum parameters by setting IsEnum=1 */
void vtkWrapPython_MarkAllEnums(
  NamespaceInfo *contents, HierarchyInfo *hinfo);

/* write out an enum type wrapped in python */
void vtkWrapPython_GenerateEnumType(
  FILE *fp, const char *module, const char *classname, EnumInfo *data);

/* generate code that adds an enum type to a python dict */
void vtkWrapPython_AddEnumType(
  FILE *fp, const char *indent, const char *dictvar, const char *objvar,
  const char *scope, EnumInfo *cls);

/* generate code that adds all public enum types to a python dict */
void vtkWrapPython_AddPublicEnumTypes(
  FILE *fp, const char *indent, const char *dictvar, const char *objvar,
  NamespaceInfo *data);

#endif /* vtkWrapPythonEnum_h */
/* VTK-HeaderTest-Exclude: vtkWrapPythonEnum.h */
