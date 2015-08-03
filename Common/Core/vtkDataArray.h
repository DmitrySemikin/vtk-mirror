/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataArray.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkDataArray - abstract superclass for arrays of numeric data
// .SECTION Description
//
// vtkDataArray is an abstract superclass for data array objects
// containing numeric data.  It extends the API defined in
// vtkAbstractArray.  vtkDataArray is an abstract superclass for data
// array objects. This class defines an API that all array objects
// must support. Note that the concrete subclasses of this class
// represent data in native form (char, int, etc.) and often have
// specialized more efficient methods for operating on this data (for
// example, getting pointers to data or getting/inserting data in
// native form).  Subclasses of vtkDataArray are assumed to contain
// data whose components are meaningful when cast to and from double.
//
// .SECTION See Also
// vtkBitArray vtkCharArray vtkUnsignedCharArray vtkShortArray
// vtkUnsignedShortArray vtkIntArray vtkUnsignedIntArray vtkLongArray
// vtkUnsignedLongArray vtkDoubleArray vtkDoubleArray

#ifndef vtkDataArray_h
#define vtkDataArray_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkAbstractArray.h"

class vtkDoubleArray;
class vtkIdList;
class vtkInformationDoubleVectorKey;
class vtkLookupTable;
class vtkPoints;

class VTKCOMMONCORE_EXPORT vtkDataArray : public vtkAbstractArray
{
public:
  vtkTypeMacro(vtkDataArray,vtkAbstractArray);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Perform a fast, safe cast from a vtkAbstractArray to a vtkDataArray.
  // This method checks if source->GetArrayType() returns DataArrayFastCast
  // or a more derived type, and performs a static_cast to return
  // source as a vtkDataArray pointer. Otherwise, NULL is returned.
  static vtkDataArray* FastDownCast(vtkAbstractArray *source);

  // Description:
  // This method is here to make backward compatibility easier.  It
  // must return true if and only if an array contains numeric data.
  // All vtkDataArray subclasses contain numeric data, hence this method
  // always returns 1(true).
  virtual int IsNumeric()
    { return 1; }

  // Description:
  // Return the size, in bytes, of the lowest-level element of an
  // array.  For vtkDataArray and subclasses this is the size of the
  // data type.
  virtual int GetElementComponentSize()
    { return this->GetDataTypeSize(); }

  // Description:
  // Set the tuple at the ith location using the jth tuple in the source array.
  // This method assumes that the two arrays have the same type
  // and structure. Note that range checking and memory allocation is not
  // performed; use in conjunction with SetNumberOfTuples() to allocate space.
  virtual void SetTuple(vtkIdType i, vtkIdType j, vtkAbstractArray* source) = 0;

  // Description:
  // Insert the jth tuple in the source array, at ith location in this array.
  // Note that memory allocation is performed as necessary to hold the data.
  // This pure virtual function is redeclared here to avoid
  // declaration hidden warnings.
  virtual void InsertTuple(vtkIdType i, vtkIdType j, vtkAbstractArray* source) = 0;

  // Description:
  // Insert the jth tuple in the source array, at the end in this array.
  // Note that memory allocation is performed as necessary to hold the data.
  // Returns the location at which the data was inserted.
  // This pure virtual function is redeclared here to avoid
  // declaration hidden warnings.
  virtual vtkIdType InsertNextTuple(vtkIdType j, vtkAbstractArray* source) = 0;

  // Description:
  // Given a list of point ids, return an array of tuples.
  // You must insure that the output array has been previously
  // allocated with enough space to hold the data.
  virtual void GetTuples(vtkIdList *ptIds, vtkAbstractArray *output);

  // Description:
  // Get the tuples for the range of points ids specified
  // (i.e., p1->p2 inclusive). You must insure that the output array has
  // been previously allocated with enough space to hold the data.
  virtual void GetTuples(vtkIdType p1, vtkIdType p2, vtkAbstractArray *output);


  // Description:
  // Set the ith tuple in this array as the interpolated tuple value,
  // given the ptIndices in the source array and associated
  // interpolation weights.
  // This method assumes that the two arrays are of the same type
  // and strcuture.
  virtual void InterpolateTuple(vtkIdType i, vtkIdList *ptIndices,
    vtkAbstractArray* source,  double* weights);

  // Description
  // Insert the ith tuple in this array as interpolated from the two values,
  // p1 and p2, and an interpolation factor, t.
  // The interpolation factor ranges from (0,1),
  // with t=0 located at p1. This method assumes that the three arrays are of
  // the same type. p1 is value at index id1 in source1, while, p2 is
  // value at index id2 in source2.
  virtual void InterpolateTuple(vtkIdType i,
    vtkIdType id1, vtkAbstractArray* source1,
    vtkIdType id2, vtkAbstractArray* source2, double t);

  // Description:
  // Get the data tuple at ith location. Return it as a pointer to an array.
  // Note: this method is not thread-safe, and the pointer is only valid
  // as long as another method invocation to a vtk object is not performed.
  virtual double *GetTuple(vtkIdType i) = 0;

  // Description:
  // Get the data tuple at ith location by filling in a user-provided array,
  // Make sure that your array is large enough to hold the NumberOfComponents
  // amount of data being returned.
  virtual void GetTuple(vtkIdType i, double * tuple) = 0;

  // Description:
  // These methods are included as convenience for the wrappers.
  // GetTuple() and SetTuple() which return/take arrays can not be
  // used from wrapped languages. These methods can be used instead.
  double GetTuple1(vtkIdType i);
  double* GetTuple2(vtkIdType i);
  double* GetTuple3(vtkIdType i);
  double* GetTuple4(vtkIdType i);
  double* GetTuple6(vtkIdType i);
  double* GetTuple9(vtkIdType i);

  // Description:
  // Set the data tuple at ith location. Note that range checking or
  // memory allocation is not performed; use this method in conjunction
  // with SetNumberOfTuples() to allocate space.
  virtual void SetTuple(vtkIdType i, const float * tuple) = 0;
  virtual void SetTuple(vtkIdType i, const double * tuple) = 0;

  // Description:
  // These methods are included as convenience for the wrappers.
  // GetTuple() and SetTuple() which return/take arrays can not be
  // used from wrapped languages. These methods can be used instead.
  void SetTuple1(vtkIdType i, double value);
  void SetTuple2(vtkIdType i, double val0, double val1);
  void SetTuple3(vtkIdType i, double val0, double val1, double val2);
  void SetTuple4(vtkIdType i, double val0, double val1, double val2,
                 double val3);
  void SetTuple6(vtkIdType i, double val0, double val1, double val2,
                 double val3, double val4, double val5);
  void SetTuple9(vtkIdType i, double val0, double val1, double val2,
                 double val3, double val4, double val5, double val6,
                 double val7, double val8);

  // Description:
  // Insert the data tuple at ith location. Note that memory allocation
  // is performed as necessary to hold the data.
  virtual void InsertTuple(vtkIdType i, const float * tuple) = 0;
  virtual void InsertTuple(vtkIdType i, const double * tuple) = 0;

  // Description:
  // These methods are included as convenience for the wrappers.
  // InsertTuple() which takes arrays can not be
  // used from wrapped languages. These methods can be used instead.
  void InsertTuple1(vtkIdType i, double value);
  void InsertTuple2(vtkIdType i, double val0, double val1);
  void InsertTuple3(vtkIdType i, double val0, double val1, double val2);
  void InsertTuple4(vtkIdType i, double val0, double val1, double val2,
                    double val3);
  void InsertTuple6(vtkIdType i, double val0, double val1, double val2,
                    double val3, double val4, double val5);
  void InsertTuple9(vtkIdType i, double val0, double val1, double val2,
                    double val3, double val4, double val5, double val6,
                    double val7, double val8);

  // Description:
  // Insert the data tuple at the end of the array and return the location at
  // which the data was inserted. Memory is allocated as necessary to hold
  // the data.
  virtual vtkIdType InsertNextTuple(const float * tuple) = 0;
  virtual vtkIdType InsertNextTuple(const double * tuple) = 0;

  // Description:
  // These methods are included as convenience for the wrappers.
  // InsertTuple() which takes arrays can not be
  // used from wrapped languages. These methods can be used instead.
  void InsertNextTuple1(double value);
  void InsertNextTuple2(double val0, double val1);
  void InsertNextTuple3(double val0, double val1, double val2);
  void InsertNextTuple4(double val0, double val1, double val2,
                        double val3);
  void InsertNextTuple6(double val0, double val1, double val2,
                        double val3, double val4, double val5);
  void InsertNextTuple9(double val0, double val1, double val2,
                        double val3, double val4, double val5, double val6,
                        double val7, double val8);

  // Description:
  // These methods remove tuples from the data array. They shift data and
  // resize array, so the data array is still valid after this operation. Note,
  // this operation is fairly slow.
  virtual void RemoveTuple(vtkIdType id) = 0;
  virtual void RemoveFirstTuple() = 0;
  virtual void RemoveLastTuple() = 0;

  // Description:
  // Return the data component at the ith tuple and jth component location.
  // Note that i is less than NumberOfTuples and j is less than
  // NumberOfComponents.
  virtual double GetComponent(vtkIdType i, int j);

  // Description:
  // Set the data component at the ith tuple and jth component location.
  // Note that i is less than NumberOfTuples and j is less than
  //  NumberOfComponents. Make sure enough memory has been allocated
  // (use SetNumberOfTuples() and SetNumberOfComponents()).
  virtual void SetComponent(vtkIdType i, int j, double c);

  // Description:
  // Insert the data component at ith tuple and jth component location.
  // Note that memory allocation is performed as necessary to hold the data.
  virtual void InsertComponent(vtkIdType i, int j, double c);

  // Description:
  // Get the data as a double array in the range (tupleMin,tupleMax) and
  // (compMin, compMax). The resulting double array consists of all data in
  // the tuple range specified and only the component range specified. This
  // process typically requires casting the data from native form into
  // doubleing point values. This method is provided as a convenience for data
  // exchange, and is not very fast.
  virtual void GetData(vtkIdType tupleMin, vtkIdType tupleMax, int compMin,
                       int compMax, vtkDoubleArray* data);

  // Description:
  // Deep copy of data. Copies data from different data arrays even if
  // they are different types (using doubleing-point exchange).
  virtual void DeepCopy(vtkAbstractArray *aa);
  virtual void DeepCopy(vtkDataArray *da);

  // Description:
  // Fill a component of a data array with a specified value. This method
  // sets the specified component to specified value for all tuples in the
  // data array.  This methods can be used to initialize or reinitialize a
  // single component of a multi-component array.
  virtual void FillComponent(int j, double c);

  // Description:
  // Copy a component from one data array into a component on this data array.
  // This method copies the specified component ("fromComponent") from the
  // specified data array ("from") to the specified component ("j") over all
  // the tuples in this data array.  This method can be used to extract
  // a component (column) from one data array and paste that data into
  // a component on this data array.
  virtual void CopyComponent(int j, vtkDataArray *from,
                             int fromComponent);

  // Description:
  // Get the address of a particular data index. Make sure data is allocated
  // for the number of items requested. If needed, increase MaxId to mark any
  // new value ranges as in-use.
  virtual void* WriteVoidPointer(vtkIdType id, vtkIdType number) = 0;

  // Description:
  // Return the memory in kibibytes (1024 bytes) consumed by this data array. Used to
  // support streaming and reading/writing data. The value returned is
  // guaranteed to be greater than or equal to the memory required to
  // actually represent the data represented by this object. The
  // information returned is valid only after the pipeline has
  // been updated.
  virtual unsigned long GetActualMemorySize();

  // Description:
  // Create default lookup table. Generally used to create one when none
  // is available.
  void CreateDefaultLookupTable();

  // Description:
  // Set/get the lookup table associated with this scalar data, if any.
  void SetLookupTable(vtkLookupTable *lut);
  vtkGetObjectMacro(LookupTable,vtkLookupTable);

  // Description:
  // The range of the data array values for the given component will be
  // returned in the provided range array argument. If comp is -1, the range
  // of the magnitude (L2 norm) over all components will be provided. The
  // range is computed and then cached, and will not be re-computed on
  // subsequent calls to GetRange() unless the array is modified or the
  // requested component changes.
  // THIS METHOD IS NOT THREAD SAFE.
  void GetRange(double range[2], int comp)
    {
    this->ComputeRange(range, comp);
    }

  // Description:
  // Return the range of the data array values for the given component. If
  // comp is -1, return the range of the magnitude (L2 norm) over all
  // components.The range is computed and then cached, and will not be
  // re-computed on subsequent calls to GetRange() unless the array is
  // modified or the requested component changes.
  // THIS METHOD IS NOT THREAD SAFE.
  double* GetRange(int comp)
    {
    this->GetRange(this->Range, comp);
    return this->Range;
    }

  // Description:
  // Return the range of the data array. If the array has multiple components,
  // then this will return the range of only the first component (component
  // zero). The range is computed and then cached, and will not be re-computed
  // on subsequent calls to GetRange() unless the array is modified.
  // THIS METHOD IS NOT THREAD SAFE.
  double* GetRange()
    {
    return this->GetRange(0);
    }

  // Description:
  // The the range of the data array values will be returned in the provided
  // range array argument. If the data array has multiple components, then
  // this will return the range of only the first component (component zero).
  // The range is computend and then cached, and will not be re-computed on
  // subsequent calls to GetRange() unless the array is modified.
  // THIS METHOD IS NOT THREAD SAFE.
  void GetRange(double range[2])
    {
    this->GetRange(range,0);
    }

  // Description:
  // These methods return the Min and Max possible range of the native
  // data type. For example if a vtkScalars consists of unsigned char
  // data these will return (0,255).
  void GetDataTypeRange(double range[2]);
  double GetDataTypeMin();
  double GetDataTypeMax();
  static void GetDataTypeRange(int type, double range[2]);
  static double GetDataTypeMin(int type);
  static double GetDataTypeMax(int type);

  // Description:
  // Return the maximum norm for the tuples.
  // Note that the max. is computed every time GetMaxNorm is called.
  virtual double GetMaxNorm();

  // Description:
  // Creates an array for dataType where dataType is one of
  // VTK_BIT, VTK_CHAR, VTK_SIGNED_CHAR, VTK_UNSIGNED_CHAR, VTK_SHORT,
  // VTK_UNSIGNED_SHORT, VTK_INT, VTK_UNSIGNED_INT, VTK_LONG,
  // VTK_UNSIGNED_LONG, VTK_DOUBLE, VTK_DOUBLE, VTK_ID_TYPE.
  // Note that the data array returned has be deleted by the
  // user.
  static vtkDataArray* CreateDataArray(int dataType);

  // Description:
  // This key is used to hold tight bounds on the range of
  // one component over all tuples of the array.
  // Two values (a minimum and maximum) are stored for each component.
  // When GetRange() is called when no tuples are present in the array
  // this value is set to { VTK_DOUBLE_MAX, VTK_DOUBLE_MIN }.
  static vtkInformationDoubleVectorKey* COMPONENT_RANGE();
  // Description:
  // This key is used to hold tight bounds on the $L_2$ norm
  // of tuples in the array.
  // Two values (a minimum and maximum) are stored for each component.
  // When GetRange() is called when no tuples are present in the array
  // this value is set to { VTK_DOUBLE_MAX, VTK_DOUBLE_MIN }.
  static vtkInformationDoubleVectorKey* L2_NORM_RANGE();

  // Description:
  // Copy information instance. Arrays use information objects
  // in a variety of ways. It is important to have flexibility in
  // this regard because certain keys should not be coppied, while
  // others must be. NOTE: Up to the implmeneter to make sure that
  // keys not inteneded to be coppied are excluded here.
  virtual int CopyInformation(vtkInformation *infoFrom, int deep=1);

  // Description:
  // Method for type-checking in FastDownCast implementations.
  virtual int GetArrayType() { return DataArray; }

protected:

  friend class vtkPoints;

  // Description:
  // Compute the range for a specific component. If comp is set -1
  // then L2 norm is computed on all components. Call ClearRange
  // to force a recomputation if it is needed. The range is copied
  // to the range argument.
  // THIS METHOD IS NOT THREAD SAFE.
  virtual void ComputeRange(double range[2], int comp);

  // Description:
  // Computes the range for each component of an array, the length
  // of \a ranges must be two times the number of components.
  // Returns true if the range was computed. Will return false
  // if you try to compute the range of an array of length zero.
  virtual bool ComputeScalarRange(double* ranges);

  // Returns true if the range was computed. Will return false
  // if you try to compute the range of an array of length zero.
  virtual bool ComputeVectorRange(double range[2]);

  // Construct object with default tuple dimension (number of components) of 1.
  vtkDataArray();
  ~vtkDataArray();

  vtkLookupTable *LookupTable;
  double Range[2];

private:
  double* GetTupleN(vtkIdType i, int n);

private:
  vtkDataArray(const vtkDataArray&);  // Not implemented.
  void operator=(const vtkDataArray&);  // Not implemented.
};

//------------------------------------------------------------------------------
inline vtkDataArray* vtkDataArray::FastDownCast(vtkAbstractArray *source)
{
  switch (source->GetArrayType())
    {
    case DataArrayTemplate:
    case TypedDataArray:
    case DataArray:
    case MappedDataArray:
      return static_cast<vtkDataArray*>(source);
    default:
      return NULL;
    }
}

#endif
