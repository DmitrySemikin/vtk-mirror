/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFFMPEGWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkFFMPEGWriter
 * @brief   Uses the FFMPEG library to write video files.
 *
 * vtkFFMPEGWriter is an adapter that allows VTK to use the LGPL'd FFMPEG
 * library to write movie files. FFMPEG can create a variety of multimedia
 * file formats and can use a variety of encoding algorithms (codecs).
 * This class creates .avi or .mp4 files containing MP43 or H264 encoded video
 * without audio.
 *
 * The FFMPEG multimedia library source code can be obtained from
 * the sourceforge web site at http://ffmpeg.sourceforge.net/download.php
 * or is a tarball along with installation instructions at
 * http://www.vtk.org/files/support/ffmpeg_source.tar.gz
 *
 */

#ifndef vtkFFMPEGWriter_h
#define vtkFFMPEGWriter_h

#include "vtkGenericMovieWriter.h"
#include "vtkIOFFMPEGModule.h" // For export macro

class vtkFFMPEGWriterInternal;

class VTKIOFFMPEG_EXPORT vtkFFMPEGWriter : public vtkGenericMovieWriter
{
public:
  static vtkFFMPEGWriter* New();

  vtkTypeMacro(vtkFFMPEGWriter, vtkGenericMovieWriter);

  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * These methods start writing an Movie file, write a frame to the file
   * and then end the writing process.
   */
  void Start() override;

  void Write() override;

  void End() override;
  //@}

  //@{
  /**
   * Set/Get the compression quality.
   * 0 means worst quality and smallest file size
   * 2 means best quality and largest file size
   */
  vtkSetClampMacro(Quality, int, 0, 2);

  vtkGetMacro(Quality, int);
  //@}

  //@{
  /**
   * Turns on(the default) or off compression.
   * Turning off compression overrides quality setting.
   */
  vtkSetMacro(Compression, bool);

  vtkGetMacro(Compression, bool);

  vtkBooleanMacro(Compression, bool);
  //@}

  //@{
  /**
   * Set/Get the frame rate, in frame/s.
   */
  vtkSetClampMacro(Rate, int, 1, 5000);

  vtkGetMacro(Rate, int);
  //@}

  //@{
  /**
   * Set/Get the bit-rate
   */
  vtkSetMacro(BitRate, int);

  vtkGetMacro(BitRate, int);
  //@}

  //@{
  /**
   * Set/Get the bit-rate tolerance
   */
  vtkSetMacro(BitRateTolerance, int);

  vtkGetMacro(BitRateTolerance, int);
  //@}

  enum EncodingMethods
  {
    ENCODING_METHOD_H264,
    ENCODING_METHOD_MJPEG
  };

  //@{
  /**
   * Set/Get the encoding format. e.g h264
   */
  vtkSetMacro(EncodingMethod, EncodingMethods);

  vtkGetMacro(EncodingMethod, EncodingMethods);
  //@}

  enum H264Codecs
  {
    H264_CODEC_LIBOPENH264,
    H264_CODEC_LIBX264
  };

  //@{
  /**
   * Set/Get the name of the encoder. e.g. libopenh264
   */
  vtkSetMacro(h264Codec, H264Codecs);

  vtkGetMacro(h264Codec, H264Codecs);
  //@}

  enum OutputFormats
  {
    OUTPUT_FORMAT_AVI,
    OUTPUT_FORMAT_MP4
  };

  //@{
  /**
   * Set/Get the output file format. e.g. mp4
   */
  vtkSetMacro(OutputFormat, OutputFormats);

  vtkGetMacro(OutputFormat, OutputFormats);
  //@}

protected:
  vtkFFMPEGWriter();

  ~vtkFFMPEGWriter();

  vtkFFMPEGWriterInternal* Internals;

  int Initialized;
  int Quality;
  int Rate;
  int BitRate;
  int BitRateTolerance;
  bool Compression;
  EncodingMethods EncodingMethod;
  H264Codecs h264Codec;
  OutputFormats OutputFormat;

private:
  vtkFFMPEGWriter(const vtkFFMPEGWriter&) = delete;

  void operator=(const vtkFFMPEGWriter&) = delete;
};

#endif
