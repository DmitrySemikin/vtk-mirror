/*===========================================================================*/
/* Distributed under OSI-approved BSD 3-Clause License.                      */
/* For copyright, see the following accompanying files or https://vtk.org:   */
/* - VTK-Copyright.txt                                                       */
/*===========================================================================*/
// .NAME vtkQtLabelRenderStrategyInternals - Internals to render labels with Qt
//
// .SECTION Description
// This class is an implementation detail of vtkQtLabelRenderStrategy
//
// This should only be included in the source file of a class derived from
// vtkQtLabelRenderStrategy.

#ifndef vtkQtLabelRenderStrategyInternals_h
#define vtkQtLabelRenderStrategyInternals_h

#include "vtkTextProperty.h"

#include <QColor>
#include <QFont>
#include <QImage>
#include <QMap>
#include <QString>

class QPainter;

struct vtkQtLabelMapEntry
{
  QString Text;
  QColor Color;
  QFont Font;
};

struct vtkQtLabelMapValue
{
  QImage Image;
  QRectF Bounds;
};

bool operator <(const vtkQtLabelMapEntry& a, const vtkQtLabelMapEntry& other);

class vtkQtLabelRenderStrategy::Internals
{
public:
  QImage* Image;
  QPainter* Painter;
  QMap<vtkQtLabelMapEntry, vtkQtLabelMapValue> Cache;

  QFont TextPropertyToFont(vtkTextProperty* tprop)
  {
    QFont fontSpec(tprop->GetFontFamilyAsString());
    fontSpec.setBold(tprop->GetBold());
    fontSpec.setItalic(tprop->GetItalic());
    fontSpec.setPixelSize(tprop->GetFontSize());
    return fontSpec;
  }

  QColor TextPropertyToColor(double* fc, double opacity)
  {
    QColor textColor(
      static_cast<int>(fc[0]*255),
      static_cast<int>(fc[1]*255),
      static_cast<int>(fc[2]*255),
      static_cast<int>(opacity*255));
    return textColor;
  }
};

#endif // vtkQtLabelRenderStrategyInternals_h
// VTK-HeaderTest-Exclude: vtkQtLabelRenderStrategyInternals.h
