/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMP4Writer.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkMP4Writer
 * @brief   Writes MP4 video files
 *
 * vtkMP4Writer writes H.264-encoded MP4 files.
 */

#ifndef vtkMP4Writer_h
#define vtkMP4Writer_h

#include "vtkGenericMovieWriter.h"
#include "vtkIOMovieModule.h" // For export macro

class VTKIOMOVIE_EXPORT vtkMP4Writer : public vtkGenericMovieWriter
{
public:
  vtkTypeMacro(vtkMP4Writer, vtkGenericMovieWriter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Set/Get the frame rate, in frame/s.
   */
  vtkSetClampMacro(FrameRate, int, 1, 5000);
  vtkGetMacro(FrameRate, int);
  //@}

  //@{
  /**
   * Set/Get the average bit rate of the video.
   * Higher produces better quality, but a larger file size.
   */
  vtkSetMacro(BitRate, int);
  vtkGetMacro(BitRate, int);
  //@}

protected:
  vtkMP4Writer();
  ~vtkMP4Writer() override;

  class vtkMP4WriterInternals;
  vtkMP4WriterInternals* Internals;

  int FrameRate = 10;
  int BitRate = 800000;

private:
  vtkMP4Writer(const vtkMP4Writer&) = delete;
  void operator=(const vtkMP4Writer&) = delete;
};

#endif
