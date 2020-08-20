/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPointSetWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPointSetWriter
 * @brief   write vtk polygonal data
 *
 * vtkPointSetWriter is a source object that writes ASCII or binary
 * point set data files in vtk format. See text for format details.
 * @warning
 * Binary files written on one system may not be readable on other systems.
 */

#ifndef vtkPointSetWriter_h
#define vtkPointSetWriter_h

#include "vtkDataWriter.h"
#include "vtkIOLegacyModule.h" // For export macro

class vtkPointSet;

class VTKIOLEGACY_EXPORT vtkPointSetWriter : public vtkDataWriter
{
public:
  static vtkPointSetWriter* New();
  vtkTypeMacro(vtkPointSetWriter, vtkDataWriter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Get the input to this writer.
   */
  vtkPointSet* GetInput();
  vtkPointSet* GetInput(int port);
  //@}

protected:
  vtkPointSetWriter() = default;
  ~vtkPointSetWriter() override = default;

  void WriteData() override;

  int FillInputPortInformation(int port, vtkInformation* info) override;

private:
  vtkPointSetWriter(const vtkPointSetWriter&) = delete;
  void operator=(const vtkPointSetWriter&) = delete;
};

#endif
