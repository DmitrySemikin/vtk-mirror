/*===========================================================================*/
/* Distributed under OSI-approved BSD 3-Clause License.                      */
/* For copyright, see the following accompanying files or https://vtk.org:   */
/* - VTK-Copyright.txt                                                       */
/*===========================================================================*/
#include "vtkStdString.h"

//----------------------------------------------------------------------------
ostream& operator<<(ostream& os, const vtkStdString& s)
{
  return os << s.c_str();
}
