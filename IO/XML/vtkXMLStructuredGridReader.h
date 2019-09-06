/*===========================================================================*/
/* Distributed under OSI-approved BSD 3-Clause License.                      */
/* For copyright, see the following accompanying files or https://vtk.org:   */
/* - VTK-Copyright.txt                                                       */
/*===========================================================================*/
/**
 * @class   vtkXMLStructuredGridReader
 * @brief   Read VTK XML StructuredGrid files.
 *
 * vtkXMLStructuredGridReader reads the VTK XML StructuredGrid file
 * format.  One structured grid file can be read to produce one
 * output.  Streaming is supported.  The standard extension for this
 * reader's file format is "vts".  This reader is also used to read a
 * single piece of the parallel file format.
 *
 * @sa
 * vtkXMLPStructuredGridReader
*/

#ifndef vtkXMLStructuredGridReader_h
#define vtkXMLStructuredGridReader_h

#include "vtkIOXMLModule.h" // For export macro
#include "vtkXMLStructuredDataReader.h"

class vtkStructuredGrid;

class VTKIOXML_EXPORT vtkXMLStructuredGridReader : public vtkXMLStructuredDataReader
{
public:
  vtkTypeMacro(vtkXMLStructuredGridReader,vtkXMLStructuredDataReader);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkXMLStructuredGridReader *New();

  //@{
  /**
   * Get the reader's output.
   */
  vtkStructuredGrid *GetOutput();
  vtkStructuredGrid *GetOutput(int idx);
  //@}

protected:
  vtkXMLStructuredGridReader();
  ~vtkXMLStructuredGridReader() override;

  const char* GetDataSetName() override;
  void SetOutputExtent(int* extent) override;

  void SetupPieces(int numPieces) override;
  void DestroyPieces() override;
  void SetupOutputData() override;

  int ReadPiece(vtkXMLDataElement* ePiece) override;
  int ReadPieceData() override;
  int FillOutputPortInformation(int, vtkInformation*) override;

  // The elements representing the points for each piece.
  vtkXMLDataElement** PointElements;

private:
  vtkXMLStructuredGridReader(const vtkXMLStructuredGridReader&) = delete;
  void operator=(const vtkXMLStructuredGridReader&) = delete;
};

#endif
