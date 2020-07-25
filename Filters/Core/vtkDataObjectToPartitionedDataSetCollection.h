/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataObjectToPartitionedDataSetCollection.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkDataObjectToPartitionedDataSetCollection
 * @brief convert any dataset to vtkPartitionedDataSetCollection.
 *
 * vtkDataObjectToPartitionedDataSetCollection converts any dataset to a
 * vtkPartitionedDataSetCollection. If the input is a multiblock dataset or an
 * AMR dataset, it creates a vtkDataAssembly for the output
 * vtkPartitionedDataSetCollection that reflects the input's hierarchical
 * organization.
 */

#ifndef vtkDataObjectToPartitionedDataSetCollection_h
#define vtkDataObjectToPartitionedDataSetCollection_h

#include "vtkDataObjectAlgorithm.h"
#include "vtkFiltersCoreModule.h" // For export macro

class vtkMultiBlockDataSet;
class vtkPartitionedDataSetCollection;
class vtkUniformGridAMR;

class VTKFILTERSCORE_EXPORT vtkDataObjectToPartitionedDataSetCollection
  : public vtkDataObjectAlgorithm
{
public:
  static vtkDataObjectToPartitionedDataSetCollection* New();
  vtkTypeMacro(vtkDataObjectToPartitionedDataSetCollection, vtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkDataObjectToPartitionedDataSetCollection();
  ~vtkDataObjectToPartitionedDataSetCollection() override;

  int FillOutputPortInformation(int port, vtkInformation* info) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int ConvertFromMultiBlock(vtkMultiBlockDataSet* input, vtkPartitionedDataSetCollection* output);
  int ConvertFromAMR(vtkUniformGridAMR* input, vtkPartitionedDataSetCollection* output);

private:
  vtkDataObjectToPartitionedDataSetCollection(
    const vtkDataObjectToPartitionedDataSetCollection&) = delete;
  void operator=(const vtkDataObjectToPartitionedDataSetCollection&) = delete;
};

#endif
