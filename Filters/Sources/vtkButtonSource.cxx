/*===========================================================================*/
/* Distributed under OSI-approved BSD 3-Clause License.                      */
/* For copyright, see the following accompanying files or https://vtk.org:   */
/* - VTK-Copyright.txt                                                       */
/*===========================================================================*/
#include "vtkButtonSource.h"


// Construct
vtkButtonSource::vtkButtonSource()
{
  this->Center[0] = this->Center[1] = this->Center[2] = 0.0;
  this->ShoulderTextureCoordinate[0] = 0.0;
  this->ShoulderTextureCoordinate[1] = 0.0;
  this->TextureStyle = VTK_TEXTURE_STYLE_PROPORTIONAL;
  this->TextureDimensions[0] = 100;
  this->TextureDimensions[1] = 100;
  this->TwoSided = 0;

  this->SetNumberOfInputPorts(0);
}

void vtkButtonSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Center: (" << this->Center[0] << ", "
                              << this->Center[1] << ", "
                              << this->Center[2] << ")\n";

  os << indent << "Shoulder Texture Coordinate: ("
     << this->ShoulderTextureCoordinate[0] << ", "
     << this->ShoulderTextureCoordinate[1] << ")\n";

  os << indent << "Texture Style: ";
  if ( this->TextureStyle == VTK_TEXTURE_STYLE_FIT_IMAGE )
  {
    os << "Fit\n";
  }
  else
  {
    os << "Proportional\n";
  }

  os << indent << "Texture Dimensions: ("
     << this->TextureDimensions[0] << ", "
     << this->TextureDimensions[1] << ")\n";

  os << indent << "Two Sided: "
     << (this->TwoSided ? "On\n" : "Off\n");
}
