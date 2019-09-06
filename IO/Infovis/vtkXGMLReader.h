/*===========================================================================*/
/* Distributed under OSI-approved BSD 3-Clause License.                      */
/* For copyright, see the following accompanying files or https://vtk.org:   */
/* - VTK-Copyright.txt                                                       */
/*===========================================================================*/
//-------------------------------------------------------------------------
//Copyright 2008 Sandia Corporation.
//Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//the U.S. Government retains certain rights in this software.
//-------------------------------------------------------------------------

/**
 * @class   vtkXGMLReader
 * @brief   Reads XGML graph files.
 * This reader is developed for a simple graph file format based
 * loosely on the "GML" notation.  This implementation is based
 * heavily on the vtkTulipReader class that forms part of the
 * Titan toolkit.
 *
 * @par Thanks:
 * Thanks to David Duke from the University of Leeds for providing this
 * implementation.
*/

#ifndef vtkXGMLReader_h
#define vtkXGMLReader_h

#include "vtkIOInfovisModule.h" // For export macro
#include "vtkUndirectedGraphAlgorithm.h"

class VTKIOINFOVIS_EXPORT vtkXGMLReader : public vtkUndirectedGraphAlgorithm
{
public:
  static vtkXGMLReader *New();
  vtkTypeMacro(vtkXGMLReader, vtkUndirectedGraphAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * The XGML file name.
   */
  vtkGetStringMacro(FileName);
  vtkSetStringMacro(FileName);
  //@}

protected:
  vtkXGMLReader();
  ~vtkXGMLReader() override;

  int RequestData(
    vtkInformation *,
    vtkInformationVector **,
    vtkInformationVector *) override;

private:
  char* FileName;

  vtkXGMLReader(const vtkXGMLReader&) = delete;
  void operator=(const vtkXGMLReader&) = delete;
};

#endif // vtkXGMLReader_h
