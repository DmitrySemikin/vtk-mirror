/*===========================================================================*/
/* Distributed under OSI-approved BSD 3-Clause License.                      */
/* For copyright, see the following accompanying files or https://vtk.org:   */
/* - VTK-Copyright.txt                                                       */
/* - Sandia-Copyright.txt                                                    */
/*===========================================================================*/
/**
 * @class   vtkPOrderStatistics
 * @brief   A class for parallel univariate order statistics
 *
 * vtkPOrderStatistics is vtkOrderStatistics subclass for parallel datasets.
 * It learns and derives the global statistical model on each node, but assesses each
 * individual data points on the node that owns it.
 *
 * .NOTE: It is assumed that the keys in the histogram table be contained in the set {0,...,n-1}
 * of successive integers, where n is the number of rows of the summary table.
 * If this requirement is not fulfilled, then the outcome of the parallel update of order
 * tables is unpredictable but will most likely be a crash.
 * Note that this requirement is consistent with the way histogram tables are constructed
 * by the (serial) superclass and thus, if you are using this class as it is intended to be ran,
 * then you do not have to worry about this requirement.
 *
 * @par Thanks:
 * Thanks to Philippe Pebay from Sandia National Laboratories for implementing this class.
*/

#ifndef vtkPOrderStatistics_h
#define vtkPOrderStatistics_h

#include "vtkFiltersParallelStatisticsModule.h" // For export macro
#include "vtkOrderStatistics.h"

#include <map> // STL Header

class vtkIdTypeArray;
class vtkMultiBlockDataSet;
class vtkMultiProcessController;

class VTKFILTERSPARALLELSTATISTICS_EXPORT vtkPOrderStatistics : public vtkOrderStatistics
{
 public:
  static vtkPOrderStatistics* New();
  vtkTypeMacro(vtkPOrderStatistics, vtkOrderStatistics);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Get/Set the multiprocess controller. If no controller is set,
   * single process is assumed.
   */
  virtual void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  //@}

  /**
   * Execute the parallel calculations required by the Learn option.
   */
  void Learn( vtkTable*,
              vtkTable*,
              vtkMultiBlockDataSet* ) override;

 protected:
  vtkPOrderStatistics();
  ~vtkPOrderStatistics() override;

  /**
   * Reduce the collection of local histograms to the global one for data inputs
   */
  bool Reduce( vtkIdTypeArray*,
               vtkDataArray* );

  /**
   * Reduce the collection of local histograms to the global one for string inputs
   */
  bool Reduce( vtkIdTypeArray*,
               vtkIdType&,
               char*,
               std::map<vtkStdString,vtkIdType>& );

  /**
   * Broadcast reduced histogram to all processes in the case of string inputs
   */
  bool Broadcast( std::map<vtkStdString,vtkIdType>&,
                  vtkIdTypeArray*,
                  vtkStringArray*,
                  vtkIdType );

  vtkMultiProcessController* Controller;
 private:
  vtkPOrderStatistics(const vtkPOrderStatistics&) = delete;
  void operator=(const vtkPOrderStatistics&) = delete;
};

#endif
