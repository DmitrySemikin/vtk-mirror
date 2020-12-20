/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPExtractDataArraysOverTime.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkPExtractDataArraysOverTime
 * @brief parallel version of vtkExtractDataArraysOverTime.
 *
 * vtkPExtractDataArraysOverTime adds distributed data support to
 * vtkExtractDataArraysOverTime.
 * This filter is ghost aware, i.e. it ignores ghost cells / ghost points, which
 * is needed to compute correct statistics in a distributed data set.
 *
 * @warning Point ghosts are needed to correctly
 * compute statistics on points on distributed data. Using
 * `vtkGenerateGlobalIds` bedore this filter is a way to produce such ghosts.
 */

#ifndef vtkPExtractDataArraysOverTime_h
#define vtkPExtractDataArraysOverTime_h

#include "vtkExtractDataArraysOverTime.h"
#include "vtkFiltersParallelModule.h" // For export macro
#include "vtkSmartPointer.h"          // For vtkSmartPointer

class vtkDataSetAttributes;
class vtkInformation;
class vtkMultiProcessController;
class vtkTable;

class VTKFILTERSPARALLEL_EXPORT vtkPExtractDataArraysOverTime : public vtkExtractDataArraysOverTime
{
public:
  static vtkPExtractDataArraysOverTime* New();
  vtkTypeMacro(vtkPExtractDataArraysOverTime, vtkExtractDataArraysOverTime);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Set and get the controller.
   */
  virtual void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  //@}

  vtkSmartPointer<vtkDescriptiveStatistics> NewDescriptiveStatistics() override;
  vtkSmartPointer<vtkOrderStatistics> NewOrderStatistics() override;

protected:
  vtkPExtractDataArraysOverTime();
  ~vtkPExtractDataArraysOverTime() override;

  /**
   * This method returns the total amount of non ghost tuples on which
   * statistics are computed, accumulated over each ranks.
   */
  vtkIdType SynchronizeNumberOfTotalInputTuples(vtkDataSetAttributes* fd) override;

  /**
   * See superclass documentation.
   */
  void SynchronizeBlocksMetaData(vtkTable* splits) override;

  vtkMultiProcessController* Controller;

  void PostExecute(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  void ReorganizeData(vtkMultiBlockDataSet* dataset);

private:
  vtkPExtractDataArraysOverTime(const vtkPExtractDataArraysOverTime&) = delete;
  void operator=(const vtkPExtractDataArraysOverTime&) = delete;
};

#endif
