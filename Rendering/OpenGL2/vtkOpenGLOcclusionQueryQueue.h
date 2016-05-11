/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLOcclusionQueryQueue.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkOpenGLOcclusionQueryQueue - Manages a series of related occlusion
// queries.
//
// .SECTION Description
// This class manages multiple GL_SAMPLES_PASSED queries. It is written with
// vtkDualDepthPeelingPass in mind, but may be useful elsewhere.
//
// The intent of this class is to allow a multi-pass rendering code to perform
// occlusion queries without blocking on a query result between each pass. The
// queries are held in a queue, and the most recent GL_SAMPLES_PASSED result is
// available via GetNumberOfPixelsWritten(). This allows the rendering code to
// continue making progress while the queried draw commands are working their
// way though the graphics pipeline.
//
// In practice, the OpenGL implementation may buffer commands for quite some
// time -- testing shows that the occlusion query from the first render pass
// may not complete until after 50 (or more) passes. To counteract this, the
// FlushThreshold parameter may be set to force a call to glFinish() every so X
// queries. By default, this is set to 0, indicating that glFinish() should
// never be called. Note that while glFlush sounds like a better option, in
// practice it does not work very well for this usecase.
//
// By setting PixelThreshold, this class can return the number of queries
// required before PixelsWritten fell below PixelThreshold. This can be used
// to make intelligent choices for FlushThreshold in the next frame.
//
// Use StartQuery() and EndQuery() around the set of draw commands that you
// wish to query. There may be multiple queries pending at any given time. Use
// UpdateQueryStatuses() to ask OpenGL if any of the pending queries have
// completed, and check GetAnyQueriesFinished() to see if they have. If so,
// GetNumberOfPixelsWritten() returns the GL_SAMPLES_PASSED result of the most
// recent query to finish. Call Reset() to re-initialize the manager at the end
// of the series of passes.
//
// .SECTION Caveats
// This class assumes that the number of written pixels will always decrease,
// and only uses the most recently completed query to set
// NumberOfPixelsWritten.
//
// For OpenGL ES 3, GL_SAMPLES_PASSED is not available, but
// GL_ANY_SAMPLES_PASSED is. In this case, GetAnyQueriesFinished() returns
// false as long as any samples passed (even if the queries finish), but when
// no samples pass AnyQueriesFinished is set to true, and number of written
// pixels is set to 0.
//
// For OpenGL ES 2, neither query is available, and this class will not be
// built on such systems.

#ifndef vtkOpenGLOcclusionQueryQueue_h
#define vtkOpenGLOcclusionQueryQueue_h

#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkObject.h"

#include <deque> // For deque...

class VTKRENDERINGOPENGL2_EXPORT vtkOpenGLOcclusionQueryQueue: public vtkObject
{
public:
  static vtkOpenGLOcclusionQueryQueue* New();
  vtkTypeMacro(vtkOpenGLOcclusionQueryQueue, vtkObject)
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Delete any pending queries and reset state (including thresholds!).
  void Reset();

  // Description:
  // Call to initiate a GL_SAMPLES_PASSED query (or GL_ANY_SAMPLES_PASSED on
  // OpenGL ES 3).
  // Must not be called again until after EndQuery().
  // @pre QueryIsActive == false
  void StartQuery();

  // Description:
  // Call to terminate the previous query.
  // Must be called after StartQuery().
  // @pre QueryIsActive == true
  void EndQuery();

  // Description:
  // Returns true if StartQuery() has been called without a closing EndQuery().
  vtkGetMacro(QueryIsActive, bool)

  // Description:
  // Returns the number of queries that have not yet finished.
  int GetNumberOfPendingQueries();

  // Description:
  // Check all pending queries to see if they are complete, and update
  // AnyQueriesFinished, NumberOfPixelsWritten, QueriesNeededForPixelThreshold,
  // NumberOfPendingQueries, and QueriesCompleted appropriately.
  void UpdateQueryStatuses();

  // Description:
  // Block until the nth query is finished. n is a zero-based index into all
  // submitted queries since the last Reset().
  void FlushToQuery(int n);

  // Description:
  // Returns true if any queries have finished.
  vtkGetMacro(AnyQueriesFinished, bool)

  // Description:
  // Returns the number of pixels written by the most recently completed
  // query.
  vtkGetMacro(NumberOfPixelsWritten, int)

  // Description:
  // Force a flush when QueriesCompleted reaches thresh. This is different from
  // SetFlushThreshold, whose argument specifies queries since last flush.
  // This modifies FlushThreshold based on internal state.
  void SetFlushThresholdInTotalQueries(int thresh);

  // Description:
  // How frequently (in number of querys) to call glFinish. Default is 0,
  // meaning "Never". See class docs for more info. See also
  // SetFlushThresholdInTotalQueries.
  vtkGetMacro(FlushThreshold, int)
  vtkSetMacro(FlushThreshold, int)

  // Description:
  // Set QueriesNeededForPixelThreshold when NumberOfPixelsWritten falls
  // below this value. See class docs for more info.
  vtkGetMacro(PixelThreshold, int)
  vtkSetMacro(PixelThreshold, int)

  // Description:
  // Set QueriesNeededForPixelThreshold when NumberOfPixelsWritten falls
  // below PixelThreshold. See class docs for more info.
  vtkGetMacro(QueriesNeededForPixelThreshold, int)

  // Description:
  // Returns the number of queries that have completed.
  vtkGetMacro(QueriesCompleted, int)

protected:
  vtkOpenGLOcclusionQueryQueue();
  ~vtkOpenGLOcclusionQueryQueue();

  bool QueryIsActive;
  unsigned int ActiveQuery;

  typedef std::deque<unsigned int> QueueT;
  QueueT Queue;
  bool AnyQueriesFinished;
  int NumberOfPixelsWritten;

  int FlushThreshold;
  int PixelThreshold;

  int LastFlushedQuery;
  int QueriesSinceFlush;
  int QueriesCompleted;
  int QueriesNeededForPixelThreshold;

private:
  vtkOpenGLOcclusionQueryQueue(const vtkOpenGLOcclusionQueryQueue&); // Not implemented
  void operator=(const vtkOpenGLOcclusionQueryQueue&); // Not implemented
};

#endif // vtkOpenGLOcclusionQueryQueue_h
