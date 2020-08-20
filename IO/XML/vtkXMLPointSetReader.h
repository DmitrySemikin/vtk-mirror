/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLPointSetReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkXMLPointSetReader
 * @brief   Read VTK XML PointSet files.
 *
 * vtkXMLPointSetReader reads the VTK XML PointSet file format.  One
 * polygonal data file can be read to produce one output.  Streaming
 * is supported.  The standard extension for this reader's file format
 * is "vtp".  This reader is also used to read a single piece of the
 * parallel file format.
 *
 * @sa
 * vtkXMLPPointSetReader
 */

#ifndef vtkXMLPointSetReader_h
#define vtkXMLPointSetReader_h

#include "vtkIOXMLModule.h" // For export macro
#include "vtkXMLUnstructuredDataReader.h"

class vtkPointSet;

class VTKIOXML_EXPORT vtkXMLPointSetReader : public vtkXMLUnstructuredDataReader
{
public:
  vtkTypeMacro(vtkXMLPointSetReader, vtkXMLUnstructuredDataReader);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkXMLPointSetReader* New();

  //@{
  /**
   * Get the reader's output.
   */
  vtkPointSet* GetOutput();
  vtkPointSet* GetOutput(int idx);
  //@}

protected:
  vtkXMLPointSetReader() = default;
  ~vtkXMLPointSetReader() override;

  const char* GetDataSetName() override;
  void GetOutputUpdateExtent(int& piece, int& numberOfPieces, int& ghostLevel) override;

  // Get the number of cells in the given piece.  Valid after
  // UpdateInformation.
  vtkIdType GetNumberOfCellsInPiece(int piece) override;

  int FillOutputPortInformation(int, vtkInformation*) override;

private:
  vtkXMLPointSetReader(const vtkXMLPointSetReader&) = delete;
  void operator=(const vtkXMLPointSetReader&) = delete;
};

#endif
