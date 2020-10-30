/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMacOSMP4Writer.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkMacOSMP4Writer
 * @brief   Writes Windows MP4 files on macos platforms.
 *
 * vtkMacOSMP4Writer writes H.264-encoded MP4 files. Note that this class in only available
 * on macos.
 */

#ifndef vtkMacOSMP4Writer_h
#define vtkMacOSMP4Writer_h

#include "vtkMP4Writer.h"
#include "vtkIOMovieModule.h" // For export macro

class VTKIOMOVIE_EXPORT vtkMacOSMP4Writer : public vtkMP4Writer
{
public:
  static vtkMacOSMP4Writer* New();
  vtkTypeMacro(vtkMacOSMP4Writer, vtkMP4Writer);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * These methods start writing an MP4 file, write a frame to the file
   * and then end the writing process.
   */
  void Start() override;
  void Write() override;
  void End() override;
  //@}

protected:
  vtkMacOSMP4Writer();
  ~vtkMacOSMP4Writer() override;

private:
  vtkMacOSMP4Writer(const vtkMacOSMP4Writer&) = delete;
  void operator=(const vtkMacOSMP4Writer&) = delete;
};

#endif
