/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLightingMapPass.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkLightingMapPass.h"

#include "vtkInformation.h"
#include "vtkInformationIntegerKey.h"
#include "vtkObjectFactory.h"
#include "vtkProp.h"
#include "vtkRenderer.h"
#include "vtkRenderState.h"

#include <cassert>

vtkStandardNewMacro(vtkLightingMapPass);

vtkInformationKeyMacro(vtkLightingMapPass, RENDER_LUMINANCE, Integer);
vtkInformationKeyMacro(vtkLightingMapPass, RENDER_NORMALS, Integer);

// ----------------------------------------------------------------------------
vtkLightingMapPass::vtkLightingMapPass()
{
}

// ----------------------------------------------------------------------------
vtkLightingMapPass::~vtkLightingMapPass()
{
}

// ----------------------------------------------------------------------------
void vtkLightingMapPass::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

// ----------------------------------------------------------------------------
// Description:
// Perform rendering according to a render state \p s.
// \pre s_exists: s!=0
void vtkLightingMapPass::Render(const vtkRenderState *s)
{
  assert("pre: s_exists" && s != 0);

  // Render filtered geometry according to our keys
  this->NumberOfRenderedProps = 0;
  this->RenderFilteredOpaqueGeometry(s);
}

// ----------------------------------------------------------------------------
// Description:
// Opaque pass with key checking.
// \pre s_exists: s!=0
void vtkLightingMapPass::RenderFilteredOpaqueGeometry(const vtkRenderState *s)
{
  assert("pre: s_exists" && s!=0);

  // initialize to false
  this->SetLastRenderingUsedDepthPeeling(s->GetRenderer(), false);

  vtkInformation *luminanceKey = vtkInformation::New();
  luminanceKey->Set(vtkLightingMapPass::RENDER_LUMINANCE(), 1);

  vtkInformation *normalKey = vtkInformation::New();
  normalKey->Set(vtkLightingMapPass::RENDER_NORMALS(), 1);

  int c = s->GetPropArrayCount();
  int i = 0;
  while (i < c)
    {
    vtkProp *p = s->GetPropArray()[i];

    // Render luminance
    if (p->HasKeys(luminanceKey))
      {
      int rendered =
        p->RenderFilteredOpaqueGeometry(s->GetRenderer(), luminanceKey);
      this->NumberOfRenderedProps += rendered;
      }

    // Render normals
    if (p->HasKeys(normalKey))
      {
      int rendered =
        p->RenderFilteredOpaqueGeometry(s->GetRenderer(), normalKey);
      this->NumberOfRenderedProps += rendered;
      }
    ++i;
    }

  luminanceKey->Delete();
  normalKey->Delete();
}
