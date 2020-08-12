/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLPointSetWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkXMLPointSetWriter
 * @brief   Write VTK XML PointSet files.
 *
 * vtkXMLPointSetWriter writes the VTK XML PointSet file format.  One
 * polygonal data input can be written into one file in any number of
 * streamed pieces (if supported by the rest of the pipeline).  The
 * standard extension for this writer's file format is "vtp".  This
 * writer is also used to write a single piece of the parallel file
 * format.
 *
 * @sa
 * vtkXMLPPointSetWriter
 */

#ifndef vtkXMLPointSetWriter_h
#define vtkXMLPointSetWriter_h

#include "vtkIOXMLModule.h" // For export macro
#include "vtkXMLUnstructuredDataWriter.h"

class vtkPointSet;

class VTKIOXML_EXPORT vtkXMLPointSetWriter : public vtkXMLUnstructuredDataWriter
{
public:
  vtkTypeMacro(vtkXMLPointSetWriter, vtkXMLUnstructuredDataWriter);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkXMLPointSetWriter* New();

  /**
   * Get/Set the writer's input.
   */
  vtkPointSet* GetInput();

  /**
   * Get the default file extension for files written by this writer.
   */
  const char* GetDefaultFileExtension() override;

protected:
  vtkXMLPointSetWriter();
  ~vtkXMLPointSetWriter() override;

  // see algorithm for more info
  int FillInputPortInformation(int port, vtkInformation* info) override;

  const char* GetDataSetName() override;

  void AllocatePositionArrays() override;
  void DeletePositionArrays() override;

  void WriteInlinePieceAttributes() override;
  void WriteInlinePiece(vtkIndent indent) override;

  void WriteAppendedPieceAttributes(int index) override;
  void WriteAppendedPiece(int index, vtkIndent indent) override;
  void WriteAppendedPieceData(int index) override;

  vtkIdType GetNumberOfInputCells() override;
  void CalculateSuperclassFraction(float* fractions);

  // Positions of attributes for each piece.
  unsigned long* NumberOfVertsPositions;
  unsigned long* NumberOfLinesPositions;
  unsigned long* NumberOfStripsPositions;
  unsigned long* NumberOfPolysPositions;

  OffsetsManagerArray* VertsOM;
  OffsetsManagerArray* LinesOM;
  OffsetsManagerArray* StripsOM;
  OffsetsManagerArray* PolysOM;

private:
  vtkXMLPointSetWriter(const vtkXMLPointSetWriter&) = delete;
  void operator=(const vtkXMLPointSetWriter&) = delete;
};

#endif
