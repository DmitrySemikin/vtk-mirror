/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTerrainInterpolator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkTerrainInterpolator - interpolate point cloud attribute data
// onto x-y plane using various kernels

// .SECTION Description
// vtkTerrainInterpolator probes a point cloud Pc (the filter Source) with a
// set of points P (the filter Input), interpolating the data values from Pc
// onto P. Note however that the descriptive phrase "point cloud" is a
// misnomer: Pc can be represented by any vtkDataSet type, with the points of
// the dataset forming Pc. Similary, the output P can also be represented by
// any vtkDataSet type; and the topology/geometry structure of P is passed
// through to the output along with the newly interpolated arrays. However,
// this filter presumes that P lies on a plane z=constant, thus z-coordinates
// are set to z = constant during the interpolation process. (The z-constant
// value is user specified.)
//
// A key input to this filter is the specification of the interpolation
// kernel, and the parameters which control the associated interpolation
// process. Interpolation kernels include Voronoi, Gaussian, Shepard, and SPH
// (smoothed particle hydrodynamics), with additional kernels to be added in
// the future.
//
// See vtkPoitnInterpolator for more information.

// .SECTION Caveats
// This class has been threaded with vtkSMPTools. Using TBB or other
// non-sequential type (set in the CMake variable
// VTK_SMP_IMPLEMENTATION_TYPE) may improve performance significantly.
//
// For widely spaced points in Pc, or when p is located outside the bounding
// region of Pc, the interpolation may behave badly and the interpolation
// process will adapt as necessary to produce output. For example, if the N
// closest points within R are requested to interpolate p, if N=0 then the
// interpolation will switch to a different strategy (which can be controlled
// as in the NullPointsStrategy).

// .SECTION See Also
// vtkPointInterpolator vtkShepardMethod vtkVoronoiKernel vtkShepardKernel
// vtkGaussianKernel vtkSPHKernel

#ifndef vtkTerrainInterpolator_h
#define vtkTerrainInterpolator_h

#include "vtkFiltersPointsModule.h" // For export macro
#include "vtkDataSetAlgorithm.h"

class vtkAbstractPointLocator;
class vtkIdList;
class vtkDoubleArray;
class vtkInterpolationKernel;
class vtkCharArray;


class VTKFILTERSPOINTS_EXPORT vtkTerrainInterpolator : public vtkDataSetAlgorithm
{
public:
  // Description:
  // Standard methods for instantiating, obtaining type information, and
  // printing.
  static vtkTerrainInterpolator *New();
  vtkTypeMacro(vtkTerrainInterpolator,vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify the dataset Pc that will be probed by the input points P.  The
  // Input P defines the dataset structure (the points and cells) for the
  // output, while the Source Pc is probed (interpolated) to generate the
  // scalars, vectors, etc. for the output points based on the point
  // locations.
  void SetSourceData(vtkDataObject *source);
  vtkDataObject *GetSource();

  // Description:
  // Specify the dataset Pc that will be probed by the input points P.  The
  // Input P defines the structure (the points and cells) for the output,
  // while the Source Pc is probed (interpolated) to generate the scalars,
  // vectors, etc. for the output points based on the point locations.
  void SetSourceConnection(vtkAlgorithmOutput* algOutput);

  // Description:
  // Specify a point locator. By default a vtkStaticPointLocator is
  // used. The locator performs efficient searches to locate near a
  // specified interpolation position.
  void SetLocator(vtkAbstractPointLocator *locator);
  vtkGetObjectMacro(Locator,vtkAbstractPointLocator);

  // Description:
  // Specify an interpolation kernel. By default a vtkVoronoiKernel is used
  // (i.e., closest point). The interpolation kernel changes the basis of the
  // interpolation.
  void SetKernel(vtkInterpolationKernel *kernel);
  vtkGetObjectMacro(Kernel,vtkInterpolationKernel);

  enum NullStrategy
  {
    MASK_POINTS=0,
    NULL_VALUE=1,
    CLOSEST_POINT=2
  };

  // Description:
  // Specify a strategy to use when encountering a "null" point during the
  // interpolation process. Null points occur when the local neighborhood (of
  // nearby points to interpolate from) is empty. If the strategy is set to
  // MaskPoints, then an output array is created that marks points as being
  // valid (=1) or null (invalid =0) (and the NullValue is set as well). If
  // the strategy is set to NullValue, then the output data value(s) are set
  // to the NullPoint value (specified in the output point data). Finally,
  // the default strategy ClosestPoint is to simply use the closest point to
  // perform the interpolation.
  vtkSetMacro(NullPointsStrategy,int);
  vtkGetMacro(NullPointsStrategy,int);
  void SetNullPointsStrategyToMaskPoints()
    { this->SetNullPointsStrategy(MASK_POINTS); }
  void SetNullPointsStrategyToNullValue()
    { this->SetNullPointsStrategy(NULL_VALUE); }
  void SetNullPointsStrategyToClosestPoint()
    { this->SetNullPointsStrategy(CLOSEST_POINT); }

  // Description:
  // If the NullPointsStrategy == MASK_POINTS, then an array is generated for
  // each input point. This vtkCharArray is placed into the output of the filter,
  // with a non-zero value for a valid point, and zero otherwise. The name of
  // this masking array is specified here.
  vtkSetStringMacro(ValidPointsMaskArrayName);
  vtkGetStringMacro(ValidPointsMaskArrayName);

  // Description:
  // Specify the null point value. When a null point is encountered then all
  // components of each null tuple are set to this value. By default the
  // null value is set to zero.
  vtkSetMacro(NullValue,double);
  vtkGetMacro(NullValue,double);

  // Description:
  // Specify the constant z value. (The filter presumes the input points, and
  // points to be interpolated are on the plane z = constant.) By default
  // z=0.0.
  vtkSetMacro(Z,double);
  vtkGetMacro(Z,double);

  // Description:
  // Indicate whether to shallow copy the input point data arrays to the
  // output.  On by default.
  vtkSetMacro(PassPointArrays, bool);
  vtkBooleanMacro(PassPointArrays, bool);
  vtkGetMacro(PassPointArrays, bool);

  // Description:
  // Indicate whether to shallow copy the input cell data arrays to the
  // output.  On by default.
  vtkSetMacro(PassCellArrays, bool);
  vtkBooleanMacro(PassCellArrays, bool);
  vtkGetMacro(PassCellArrays, bool);

  // Description:
  // Indicate whether to pass the field-data arrays from the input to the
  // output. On by default.
  vtkSetMacro(PassFieldArrays, bool);
  vtkBooleanMacro(PassFieldArrays, bool);
  vtkGetMacro(PassFieldArrays, bool);

protected:
  vtkTerrainInterpolator();
  ~vtkTerrainInterpolator();

  vtkAbstractPointLocator *Locator;
  vtkInterpolationKernel *Kernel;

  double Z;
  int NullPointsStrategy;
  double NullValue;
  char* ValidPointsMaskArrayName;
  vtkCharArray *ValidPointsMask;

  bool PassCellArrays;
  bool PassPointArrays;
  bool PassFieldArrays;

  virtual int RequestData(vtkInformation *, vtkInformationVector **,
    vtkInformationVector *);
  virtual int RequestInformation(vtkInformation *, vtkInformationVector **,
    vtkInformationVector *);
  virtual int RequestUpdateExtent(vtkInformation *, vtkInformationVector **,
    vtkInformationVector *);

  // Description:
  // Equivalent to calling BuildFieldList(); InitializeForProbing(); DoProbing().
  void Probe(vtkDataSet *input, vtkDataSet *source, vtkDataSet *output);

  // Description:
  // Call at end of RequestData() to pass attribute data respecting the
  // PassCellArrays, PassPointArrays, PassFieldArrays flags.
  void PassAttributeData(
    vtkDataSet* input, vtkDataObject* source, vtkDataSet* output);

  // Description:
  // Internal method to extract image metadata
  void ExtractImageDescription(vtkImageData *input, int dims[3],
                               double origin[3], double spacing[3]);

private:
  vtkTerrainInterpolator(const vtkTerrainInterpolator&);  // Not implemented.
  void operator=(const vtkTerrainInterpolator&);  // Not implemented.

};

#endif
