/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWin32MP4Writer.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkWin32MP4Writer
 * @brief   Writes Windows MP4 files on Windows platforms.
 *
 * vtkWin32MP4Writer writes H.264-encoded MP4 files. Note that this class in only available
 * on the Microsoft Windows platform.
 *
 * Implementation inspired from the following tutorial:
 * https://docs.microsoft.com/en-us/windows/win32/medfound/tutorial--using-the-sink-writer-to-encode-video?redirectedfrom=MSDN
 * @sa
 * vtkGenericMovieWriter vtkAVIWriter
 */

#ifndef vtkWin32MP4Writer_h
#define vtkWin32MP4Writer_h

#include "vtkGenericMovieWriter.h"
#include "vtkIOMovieModule.h" // For export macro

class VTKIOMOVIE_EXPORT vtkWin32MP4Writer : public vtkGenericMovieWriter
{
public:
  static vtkWin32MP4Writer* New();
  vtkTypeMacro(vtkWin32MP4Writer, vtkGenericMovieWriter);
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
  vtkWin32MP4Writer();
  ~vtkWin32MP4Writer() override;

  class vtkWin32MP4WriterInternals;
  vtkWin32MP4WriterInternals* Internals;

  int FrameRate;
  int BitRate;

private:
  vtkWin32MP4Writer(const vtkWin32MP4Writer&) = delete;
  void operator=(const vtkWin32MP4Writer&) = delete;
};

#endif
