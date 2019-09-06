/*===========================================================================*/
/* Distributed under OSI-approved BSD 3-Clause License.                      */
/* For copyright, see the following accompanying files or https://vtk.org:   */
/* - VTK-Copyright.txt                                                       */
/*===========================================================================*/
#include "vtkTextProperty.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkTextProperty);

//----------------------------------------------------------------------------
vtkTextProperty::vtkTextProperty()
{
  this->Color[0] = 1.0;
  this->Color[1] = 1.0;
  this->Color[2] = 1.0;

  this->Opacity  = 1.0;

  this->BackgroundColor[0] = 0.0;
  this->BackgroundColor[1] = 0.0;
  this->BackgroundColor[2] = 0.0;

  this->BackgroundOpacity  = 0.0;

  this->Frame = 0;
  this->FrameWidth = 1;
  this->FrameColor[0] = 1.0;
  this->FrameColor[1] = 1.0;
  this->FrameColor[2] = 1.0;

  this->FontFamilyAsString = nullptr;
  this->FontFile = nullptr;
  this->SetFontFamilyAsString( "Arial" );
  this->FontSize = 12;

  this->Bold = 0;
  this->Italic = 0;
  this->Shadow = 0;
  this->ShadowOffset[0] = 1;
  this->ShadowOffset[1] = -1;

  this->Justification = VTK_TEXT_LEFT;
  this->VerticalJustification = VTK_TEXT_BOTTOM;
  this->UseTightBoundingBox = 0;

  this->LineOffset = 0.0;
  this->LineSpacing = 1.1; // why not 1.0 ?

  this->Orientation = 0.0;
}

//----------------------------------------------------------------------------
vtkTextProperty::~vtkTextProperty()
{
  this->SetFontFamilyAsString(nullptr);
  this->SetFontFile(nullptr);
}

//----------------------------------------------------------------------------
void vtkTextProperty::ShallowCopy(vtkTextProperty *tprop)
{
  if (!tprop)
  {
    return;
  }

  this->SetColor(tprop->GetColor());
  this->SetOpacity(tprop->GetOpacity());

  this->SetBackgroundColor(tprop->GetBackgroundColor());
  this->SetBackgroundOpacity(tprop->GetBackgroundOpacity());

  this->SetFrame(tprop->GetFrame());
  this->SetFrameWidth(tprop->GetFrameWidth());
  this->SetFrameColor(tprop->GetFrameColor());

  this->SetFontFamilyAsString(tprop->GetFontFamilyAsString());
  this->SetFontFile(tprop->GetFontFile());
  this->SetFontSize(tprop->GetFontSize());

  this->SetBold(tprop->GetBold());
  this->SetItalic(tprop->GetItalic());
  this->SetShadow(tprop->GetShadow());

  this->SetOrientation(tprop->GetOrientation());

  this->SetJustification(tprop->GetJustification());
  this->SetVerticalJustification(tprop->GetVerticalJustification());

  this->SetUseTightBoundingBox(tprop->GetUseTightBoundingBox());

  this->SetLineOffset(tprop->GetLineOffset());
  this->SetLineSpacing(tprop->GetLineSpacing());

  this->SetShadowOffset(tprop->GetShadowOffset());
}

//----------------------------------------------------------------------------
void vtkTextProperty::GetShadowColor(double color[3])
{
#if 1
  double average = (this->Color[0] + this->Color[1] + this->Color[2]) / 3.0;
#else
  double average = (0.30 * this->Color[0] +
                    0.59 * this->Color[1] +
                    0.11 * this->Color[2]);
#endif
  double shadow_i = average > 0.5 ? 0.0 : 1.0;
  color[0] = color[1] = color[2] = shadow_i;
}

//----------------------------------------------------------------------------
void vtkTextProperty::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Color: (" << this->Color[0] << ", "
     << this->Color[1] << ", " << this->Color[2] << ")\n";

  os << indent << "Opacity: " << this->Opacity << "\n";

  os << indent << "BackgroundColor: ("
     << this->BackgroundColor[0] << ", "
     << this->BackgroundColor[1] << ", "
     << this->BackgroundColor[2] << ")\n";

  os << indent << "BackgroundOpacity: " << this->BackgroundOpacity << "\n";

  os << indent << "Frame: " << (this->Frame ? "On\n" : "Off\n");
  os << indent << "FrameWidth: " << this->FrameWidth << "\n";
  os << indent << "FrameColor: ("
     << this->FrameColor[0] << ", "
     << this->FrameColor[1] << ", "
     << this->FrameColor[2] << ")\n";

  os << indent << "FontFamilyAsString: "
     << (this->FontFamilyAsString ? this->FontFamilyAsString : "(null)") << endl;
  os << indent << "FontFile: "
     << (this->FontFile ? this->FontFile : "(null)") << endl;
  os << indent << "FontSize: " << this->FontSize << "\n";

  os << indent << "Bold: " << (this->Bold ? "On\n" : "Off\n");
  os << indent << "Italic: " << (this->Italic ? "On\n" : "Off\n");
  os << indent << "Shadow: " << (this->Shadow ? "On\n" : "Off\n");
  os << indent << "ShadowOffset: (" << this->ShadowOffset[0] << ", "
    << this->ShadowOffset[1] << ")\n";

  os << indent << "Justification: "
     << this->GetJustificationAsString() << "\n";

  os << indent << "Vertical justification: "
     << this->GetVerticalJustificationAsString() << "\n";

  os << indent << "UseTightBoundingBox: " << this->UseTightBoundingBox << "\n";
  os << indent << "Orientation: " << this->Orientation << "\n";
  os << indent << "Line Offset: " << this->LineOffset << "\n";
  os << indent << "Line Spacing: " << this->LineSpacing << "\n";
}
