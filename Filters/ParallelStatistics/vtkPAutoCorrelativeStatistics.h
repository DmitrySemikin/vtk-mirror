/*===========================================================================*/
/* Distributed under OSI-approved BSD 3-Clause License.                      */
/* For copyright, see the following accompanying files or https://vtk.org:   */
/* - VTK-Copyright.txt                                                       */
/*===========================================================================*/
/**
 * @class   vtkPAutoCorrelativeStatistics
 * @brief   A class for parallel auto-correlative statistics
 *
 * vtkPAutoCorrelativeStatistics is vtkAutoCorrelativeStatistics subclass for parallel datasets.
 * It learns and derives the global statistical model on each node, but assesses each
 * individual data points on the node that owns it.
 *
 * @par Thanks:
 * This class was written by Philippe Pebay, Kitware SAS 2012.
*/

#ifndef vtkPAutoCorrelativeStatistics_h
#define vtkPAutoCorrelativeStatistics_h

#include "vtkFiltersParallelStatisticsModule.h" // For export macro
#include "vtkAutoCorrelativeStatistics.h"

class vtkMultiBlockDataSet;
class vtkMultiProcessController;

class VTKFILTERSPARALLELSTATISTICS_EXPORT vtkPAutoCorrelativeStatistics : public vtkAutoCorrelativeStatistics
{
public:
  static vtkPAutoCorrelativeStatistics* New();
  vtkTypeMacro(vtkPAutoCorrelativeStatistics, vtkAutoCorrelativeStatistics);
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
  void Learn( vtkTable* inData,
              vtkTable* inParameters,
              vtkMultiBlockDataSet* outMeta ) override;

  /**
   * Execute the calculations required by the Test option.
   * NB: Not implemented for more than 1 processor
   */
  void Test( vtkTable*,
             vtkMultiBlockDataSet*,
             vtkTable* ) override;

protected:
  vtkPAutoCorrelativeStatistics();
  ~vtkPAutoCorrelativeStatistics() override;

  vtkMultiProcessController* Controller;
private:
  vtkPAutoCorrelativeStatistics(const vtkPAutoCorrelativeStatistics&) = delete;
  void operator=(const vtkPAutoCorrelativeStatistics&) = delete;
};

#endif
