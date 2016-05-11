/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLOcclusionQueryQueue.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkOpenGLOcclusionQueryQueue.h"

#include "vtkObjectFactory.h"

#include "vtk_glew.h"

#include <cassert>

// Define to enable debugging output
//#define vtkOGLOQQ_DEBUG

vtkStandardNewMacro(vtkOpenGLOcclusionQueryQueue)

//------------------------------------------------------------------------------
void vtkOpenGLOcclusionQueryQueue::PrintSelf(std::ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void vtkOpenGLOcclusionQueryQueue::Reset()
{
  typedef QueueT::iterator Iter;
  for (Iter it = this->Queue.begin(), itEnd = this->Queue.end(); it != itEnd;
       ++it)
    {
    GLuint query = static_cast<GLuint>(*it);
    glDeleteQueries(1, &query);
    }
  this->Queue.clear();

  this->QueryIsActive = false;
  this->ActiveQuery = 0;
  this->AnyQueriesFinished = false;
  this->NumberOfPixelsWritten = -1;
  this->FlushThreshold = 0;
  this->PixelThreshold = 0;
  this->LastFlushedQuery = 0;
  this->QueriesSinceFlush = 0;
  this->QueriesCompleted = 0;
  this->QueriesNeededForPixelThreshold = 0;

#ifdef vtkOGLOQQ_DEBUG
  std::cerr << "Queries reset.\n";
#endif
}

//------------------------------------------------------------------------------
void vtkOpenGLOcclusionQueryQueue::StartQuery()
{
  assert(!this->QueryIsActive);

  GLuint query;
  glGenQueries(1, &query);

#if GL_ES_VERSION_3_0 == 1
  glBeginQuery(GL_ANY_SAMPLES_PASSED, query);
#else // GL ES 3.0
  glBeginQuery(GL_SAMPLES_PASSED, query);
#endif // GL ES 3.0

  this->ActiveQuery = static_cast<unsigned int>(query);
  this->QueryIsActive = true;

#ifdef vtkOGLOQQ_DEBUG
  std::cerr << "Start Query: " << query << "\n";
#endif
}

//------------------------------------------------------------------------------
void vtkOpenGLOcclusionQueryQueue::EndQuery()
{
  assert(this->QueryIsActive);

#if GL_ES_VERSION_3_0 == 1
  glEndQuery(GL_ANY_SAMPLES_PASSED);
#else // GL ES 3.0
  glEndQuery(GL_SAMPLES_PASSED);
#endif // GL ES 3.0

  this->Queue.push_back(this->ActiveQuery);
  this->QueryIsActive = false;
  ++this->QueriesSinceFlush;

#ifdef vtkOGLOQQ_DEBUG
  std::cerr << "End Query: " << this->ActiveQuery << "\n";
#endif
}

//------------------------------------------------------------------------------
int vtkOpenGLOcclusionQueryQueue::GetNumberOfPendingQueries()
{
  return static_cast<int>(this->Queue.size());
}

//------------------------------------------------------------------------------
void vtkOpenGLOcclusionQueryQueue::UpdateQueryStatuses()
{
  // Determine whether to force the result or not:
  if (this->FlushThreshold > 0 &&
      this->QueriesSinceFlush >= this->FlushThreshold)
    {
#ifdef vtkOGLOQQ_DEBUG
    std::cerr << "Syncing to query results (QueriesSinceFlush: "
              << this->QueriesSinceFlush << ", FlushThreshold: "
              << this->FlushThreshold << ").\n";
#endif

    // Flush the queue by requesting a block on the last query in the queue.
    this->FlushToQuery(this->QueriesCompleted + this->Queue.size() - 1);

    this->LastFlushedQuery = this->QueriesCompleted + this->Queue.size();
    this->QueriesSinceFlush = 0;
    }

  typedef QueueT::iterator Iter;
  Iter it = this->Queue.begin();
  while (it != this->Queue.end())
    {
    GLuint query = static_cast<GLuint>(*it);

    // Check if the result is available:
    GLint status;
    glGetQueryObjectiv(query, GL_QUERY_RESULT_AVAILABLE, &status);
    if (status == GL_FALSE)
      {
      // If not available, just return. None of the later queries will be
      // ready yet, either.
      return;
      }
    // If the result is ready, grab it:
    glGetQueryObjectiv(query, GL_QUERY_RESULT, &status);

#if GL_ES_VERSION_3_0 == 1
    // GL ES 3.0 uses ANY_SAMPLES_PASSED, so false means 0 samples passed:
    if (status == GL_FALSE)
      {
      this->AnyQueriesFinished = true;
      this->NumberOfPixelsWritten = 0;
      }
#else // GL ES 3.0
    // OpenGL 3.2+ queries SAMPLES_PASSED. Use this result:
    this->AnyQueriesFinished = true;
    this->NumberOfPixelsWritten = static_cast<int>(status);
#endif // GL ES 3.0

    ++this->QueriesCompleted;
    // Have we reached the requested pixel threshold?
    if (this->QueriesNeededForPixelThreshold == 0 &&
        this->NumberOfPixelsWritten <= this->PixelThreshold)
      { // If so, record it:
      this->QueriesNeededForPixelThreshold = this->QueriesCompleted;
      }

#ifdef vtkOGLOQQ_DEBUG
    std::cerr << "Query " << this->QueriesCompleted << " complete. id: "
              << query << ". " << this->NumberOfPixelsWritten
              << " samples passed.\n";
#endif

    // Clean up the completed query:
    glDeleteQueries(1, &query);
    it = this->Queue.erase(it);
    }
}

//------------------------------------------------------------------------------
void vtkOpenGLOcclusionQueryQueue::FlushToQuery(int n)
{
  if (n < this->QueriesCompleted)
    { // Already done!
    return;
    }

  if (n >= this->QueriesCompleted + static_cast<int>(this->Queue.size()))
    {
    vtkErrorMacro("Requested flush to a query that does not exist!");
    return;
    }

  GLuint query = static_cast<GLuint>(
        this->Queue.at(n - this->QueriesCompleted));

  // Block for the result, but we don't use it.
  GLint dummy;
  glGetQueryObjectiv(query, GL_QUERY_RESULT, &dummy);
}

//------------------------------------------------------------------------------
void vtkOpenGLOcclusionQueryQueue::SetFlushThresholdInTotalQueries(int thresh)
{
  thresh -= this->LastFlushedQuery;
  // If we've passed the requested queries, check every frame by default:
  this->SetFlushThreshold(std::max(1, thresh));
}

//------------------------------------------------------------------------------
vtkOpenGLOcclusionQueryQueue::vtkOpenGLOcclusionQueryQueue()
  : QueryIsActive(false),
    ActiveQuery(0),
    AnyQueriesFinished(false),
    NumberOfPixelsWritten(-1),
    FlushThreshold(0),
    PixelThreshold(0),
    LastFlushedQuery(0),
    QueriesSinceFlush(0),
    QueriesCompleted(0),
    QueriesNeededForPixelThreshold(0)
{
}

//------------------------------------------------------------------------------
vtkOpenGLOcclusionQueryQueue::~vtkOpenGLOcclusionQueryQueue()
{
  this->Reset();
}
