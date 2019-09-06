/*===========================================================================*/
/* Distributed under OSI-approved BSD 3-Clause License.                      */
/* For copyright, see the following accompanying files or https://vtk.org:   */
/* - VTK-Copyright.txt                                                       */
/* - Sandia-Copyright.txt                                                    */
/*===========================================================================*/
/**
 * @class   vtkGraphLayoutStrategy
 * @brief   abstract superclass for all graph layout strategies
 *
 *
 * All graph layouts should subclass from this class.  vtkGraphLayoutStrategy
 * works as a plug-in to the vtkGraphLayout algorithm.  The Layout()
 * function should perform some reasonable "chunk" of the layout.
 * This allows the user to be able to see the progress of the layout.
 * Use IsLayoutComplete() to tell the user when there is no more layout
 * to perform.
 *
 * @par Thanks:
 * Thanks to Brian Wylie from Sandia National Laboratories for adding incremental
 * layout capabilities.
*/

#ifndef vtkGraphLayoutStrategy_h
#define vtkGraphLayoutStrategy_h

#include "vtkInfovisLayoutModule.h" // For export macro
#include "vtkObject.h"

class vtkGraph;

class VTKINFOVISLAYOUT_EXPORT vtkGraphLayoutStrategy : public vtkObject
{
public:
  vtkTypeMacro(vtkGraphLayoutStrategy,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Setting the graph for the layout strategy
   */
  virtual void SetGraph(vtkGraph *graph);

  /**
   * This method allows the layout strategy to
   * do initialization of data structures
   * or whatever else it might want to do.
   */
  virtual void Initialize() {}

  /**
   * This is the layout method where the graph that was
   * set in SetGraph() is laid out. The method can either
   * entirely layout the graph or iteratively lay out the
   * graph. If you have an iterative layout please implement
   * the IsLayoutComplete() method.
   */
  virtual void Layout()=0;

  /**
   * If your concrete class is iterative than
   * you should overload IsLayoutComplete()
   * otherwise it simply returns 1 by default;
   */
  virtual int IsLayoutComplete() {return 1;}

  //@{
  /**
   * Whether to use edge weights in the layout or not.
   */
  virtual void SetWeightEdges(bool state);
  vtkGetMacro(WeightEdges, bool);
  //@}

  //@{
  /**
   * Set/Get the field to use for the edge weights.
   */
  virtual void SetEdgeWeightField(const char* field);
  vtkGetStringMacro(EdgeWeightField);
  //@}

protected:
  vtkGraphLayoutStrategy();
  ~vtkGraphLayoutStrategy() override;

  vtkGraph *Graph;
  char     *EdgeWeightField;
  bool     WeightEdges;
private:

  vtkGraphLayoutStrategy(const vtkGraphLayoutStrategy&) = delete;
  void operator=(const vtkGraphLayoutStrategy&) = delete;
};

#endif

