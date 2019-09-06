/*===========================================================================*/
/* Distributed under OSI-approved BSD 3-Clause License.                      */
/* For copyright, see the following accompanying files or https://vtk.org:   */
/* - VTK-Copyright.txt                                                       */
/* - Sandia-Copyright.txt                                                    */
/*===========================================================================*/

/**
 * @class   vtkBoostRandomSparseArraySource
 * @brief   generates a sparse N-way array containing random values.
 *
 * @par Thanks:
 * Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National Laboratories.
*/

#ifndef vtkBoostRandomSparseArraySource_h
#define vtkBoostRandomSparseArraySource_h

#include "vtkInfovisBoostGraphAlgorithmsModule.h" // For export macro
#include "vtkArrayDataAlgorithm.h"
#include "vtkArrayExtents.h"

class VTKINFOVISBOOSTGRAPHALGORITHMS_EXPORT vtkBoostRandomSparseArraySource : public vtkArrayDataAlgorithm
{
public:
  static vtkBoostRandomSparseArraySource* New();
  vtkTypeMacro(vtkBoostRandomSparseArraySource, vtkArrayDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Sets the extents (dimensionality and size) of the output array
   */
  void SetExtents(const vtkArrayExtents&);

  /**
   * Returns the extents (dimensionality and size) of the output array
   */
  vtkArrayExtents GetExtents();

  //@{
  /**
   * Stores a random-number-seed for determining which elements within
   * the output matrix will have non-zero values
   */
  vtkGetMacro(ElementProbabilitySeed, vtkTypeUInt32);
  vtkSetMacro(ElementProbabilitySeed, vtkTypeUInt32);
  //@}

  //@{
  /**
   * Stores the probability (in the range [0, 1]) that an element within
   * the output matrix will have a non-zero value
   */
  vtkGetMacro(ElementProbability, double);
  vtkSetMacro(ElementProbability, double);
  //@}

  //@{
  /**
   * Stores a random-number-seed for computing random element values
   */
  vtkGetMacro(ElementValueSeed, vtkTypeUInt32);
  vtkSetMacro(ElementValueSeed, vtkTypeUInt32);
  //@}

  //@{
  /**
   * Stores the minimum value of any element
   */
  vtkGetMacro(MinValue, double);
  vtkSetMacro(MinValue, double);
  //@}

  //@{
  /**
   * Stores the maximum value of any element
   */
  vtkGetMacro(MaxValue, double);
  vtkSetMacro(MaxValue, double);
  //@}

protected:
  vtkBoostRandomSparseArraySource();
  ~vtkBoostRandomSparseArraySource();

  int RequestData(
    vtkInformation*,
    vtkInformationVector**,
    vtkInformationVector*) override;

private:
  vtkBoostRandomSparseArraySource(const vtkBoostRandomSparseArraySource&) = delete;
  void operator=(const vtkBoostRandomSparseArraySource&) = delete;

  vtkArrayExtents Extents;

  vtkTypeUInt32 ElementProbabilitySeed;
  double ElementProbability;

  vtkTypeUInt32 ElementValueSeed;
  double MinValue;
  double MaxValue;

};

#endif

// VTK-HeaderTest-Exclude: vtkBoostRandomSparseArraySource.h
