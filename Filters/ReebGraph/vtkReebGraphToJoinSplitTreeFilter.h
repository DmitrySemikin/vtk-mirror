/*===========================================================================*/
/* Distributed under OSI-approved BSD 3-Clause License.                      */
/* For copyright, see the following accompanying files or https://vtk.org:   */
/* - VTK-Copyright.txt                                                       */
/*===========================================================================*/
/**
 * @class   vtkReebGraphToJoinSplitTreeFilter
 * @brief   converts a given Reeb graph
 * either to a join tree or a split tree (respectively the connectivity of the
 * sub- and sur- level sets).
 * Note: if you want to use simplification filters, do so on the input Reeb
 * graph first.
 *
 * Reference:
 * "Computing contpour trees in all dimensions". H. Carr, J. Snoeyink, U. Axen.
 * SODA 2000, pp. 918-926.
 *
 *
 * The filter takes as an input the underlying mesh (port 0, a vtkPolyData for
 * 2D meshes or a vtkUnstructuredGrid for 3D meshes) with an attached scalar
 * field (identified by FieldId, with setFieldId()) and an input Reeb graph
 * computed on that mesh (port 1).
 * The outputs is vtkReebGraph object describing either a join or split tree.
*/

#ifndef vtkReebGraphToJoinSplitTreeFilter_h
#define vtkReebGraphToJoinSplitTreeFilter_h

#include "vtkFiltersReebGraphModule.h" // For export macro
#include  "vtkDirectedGraphAlgorithm.h"

class vtkReebGraph;

class VTKFILTERSREEBGRAPH_EXPORT vtkReebGraphToJoinSplitTreeFilter :
  public vtkDirectedGraphAlgorithm
{
public:
  static vtkReebGraphToJoinSplitTreeFilter* New();
  vtkTypeMacro(vtkReebGraphToJoinSplitTreeFilter,
    vtkDirectedGraphAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Specify if you want to get a join or a split tree.
   * Default value: false (join tree)
   */
  vtkSetMacro(IsSplitTree, bool);
  vtkGetMacro(IsSplitTree, bool);
  //@}

  //@{
  /**
   * Set the scalar field Id
   * Default value: 0;
   */
  vtkSetMacro(FieldId, vtkIdType);
  vtkGetMacro(FieldId, vtkIdType);
  //@}

  vtkReebGraph* GetOutput();

protected:
  vtkReebGraphToJoinSplitTreeFilter();
  ~vtkReebGraphToJoinSplitTreeFilter();

  bool IsSplitTree;

  vtkIdType FieldId;

  int FillInputPortInformation(int portNumber, vtkInformation *) override;
  int FillOutputPortInformation(int, vtkInformation *) override;

  int RequestData(vtkInformation *request,
    vtkInformationVector **inputVector, vtkInformationVector *outputVector) override;

private:
  vtkReebGraphToJoinSplitTreeFilter(const vtkReebGraphToJoinSplitTreeFilter&) = delete;
  void operator=(const vtkReebGraphToJoinSplitTreeFilter&) = delete;
};

#endif
