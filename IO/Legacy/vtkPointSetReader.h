/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPointSetReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPointSetReader
 * @brief   read vtk polygonal data file
 *
 * vtkPointSetReader is a source object that reads ASCII or binary
 * point set data files in vtk format (see text for format details).
 * The output of this reader is a single vtkPointSet data object.
 * The superclass of this class, vtkDataReader, provides many methods for
 * controlling the reading of the data file, see vtkDataReader for more
 * information.
 * @warning
 * Binary files written on one system may not be readable on other systems.
 * @sa
 * vtkPointSet vtkDataReader
 */

#ifndef vtkPointSetReader_h
#define vtkPointSetReader_h

#include "vtkDataReader.h"
#include "vtkIOLegacyModule.h" // For export macro

class vtkPointSet;

class VTKIOLEGACY_EXPORT vtkPointSetReader : public vtkDataReader
{
public:
  static vtkPointSetReader* New();
  vtkTypeMacro(vtkPointSetReader, vtkDataReader);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Get the output of this reader.
   */
  vtkPointSet* GetOutput();
  vtkPointSet* GetOutput(int idx);
  void SetOutput(vtkPointSet* output);
  //@}

  /**
   * Actual reading happens here
   */
  int ReadMeshSimple(const std::string& fname, vtkDataObject* output) override;

protected:
  vtkPointSetReader();
  ~vtkPointSetReader() override;

  int FillOutputPortInformation(int, vtkInformation*) override;

private:
  vtkPointSetReader(const vtkPointSetReader&) = delete;
  void operator=(const vtkPointSetReader&) = delete;
};

#endif
