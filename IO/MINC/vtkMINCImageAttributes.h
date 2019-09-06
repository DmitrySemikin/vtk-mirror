/*===========================================================================*/
/* Distributed under OSI-approved BSD 3-Clause License.                      */
/* For copyright, see the following accompanying files or https://vtk.org:   */
/* - VTKm-Copyright.txt                                                      */
/*===========================================================================*/
/**
 * @class   vtkMINCImageAttributes
 * @brief   A container for a MINC image header.
 *
 * This class provides methods to access all of the information
 * contained in the MINC header.  If you read a MINC file into
 * VTK and then write it out again, you can use
 * writer->SetImageAttributes(reader->GetImageAttributes) to
 * ensure that all of the medical information contained in the
 * file is transferred from the reader to the writer.  If you
 * want to change any of the header information, you must
 * use ShallowCopy to make a copy of the reader's attributes
 * and then modify only the copy.
 * @sa
 * vtkMINCImageReader vtkMINCImageWriter
 * @par Thanks:
 * Thanks to David Gobbi for writing this class and Atamai Inc. for
 * contributing it to VTK.
*/

#ifndef vtkMINCImageAttributes_h
#define vtkMINCImageAttributes_h

#include "vtkIOMINCModule.h" // For export macro
#include "vtkObject.h"

class vtkDataArray;
class vtkStringArray;
class vtkIdTypeArray;
class vtkDoubleArray;
class vtkMatrix4x4;

// A special class that holds the attributes
class vtkMINCImageAttributeMap;

class VTKIOMINC_EXPORT vtkMINCImageAttributes : public vtkObject
{
public:
  vtkTypeMacro(vtkMINCImageAttributes,vtkObject);

  static vtkMINCImageAttributes *New();
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Reset all the attributes in preparation for loading
   * new information.
   */
  virtual void Reset();

  //@{
  /**
   * Get the name of the image, not including the path or
   * the extension.  This is only needed for printing the
   * header and there is usually no need to set it.
   */
  vtkSetStringMacro(Name);
  vtkGetStringMacro(Name);
  //@}

  //@{
  /**
   * Get the image data type, as stored on disk.  This information
   * is useful if the file was converted to floating-point when it
   * was loaded.  When writing a file from float or double image data,
   * you can use this method to prescribe the output type.
   */
  vtkSetMacro(DataType, int);
  vtkGetMacro(DataType, int);
  //@}

  //@{
  /**
   * Add the names of up to five dimensions. The ordering of these
   * dimensions will determine the dimension order of the file.  If
   * no DimensionNames are set, the writer will set the dimension
   * order of the file to be the same as the dimension order in memory.
   */
  virtual void AddDimension(const char *dimension) {
    this->AddDimension(dimension, 0); };
  virtual void AddDimension(const char *dimension, vtkIdType length);
  //@}

  /**
   * Get the dimension names.  The dimension names are same order
   * as written in the file, starting with the slowest-varying
   * dimension.  Use this method to get the array if you need to
   * change "space" dimensions to "frequency" after performing a
   * Fourier transform.
   */
  virtual vtkStringArray *GetDimensionNames() {
    return this->DimensionNames; };

  /**
   * Get the lengths of all the dimensions.  The dimension lengths
   * are informative, the vtkMINCImageWriter does not look at these values
   * but instead uses the dimension sizes of its input.
   */
  virtual vtkIdTypeArray *GetDimensionLengths() {
    return this->DimensionLengths; };

  /**
   * Get the names of all the variables.
   */
  virtual vtkStringArray *GetVariableNames() {
    return this->VariableNames; };

  /**
   * List the attribute names for a variable.  Set the variable
   * to the empty string to get a list of the global attributes.
   */
  virtual vtkStringArray *GetAttributeNames(const char *variable);

  //@{
  /**
   * Get the image min and max arrays. These are set by the reader,
   * but they aren't used by the writer except to compute the full
   * real data range of the original file.
   */
  virtual void SetImageMin(vtkDoubleArray *imageMin);
  virtual void SetImageMax(vtkDoubleArray *imageMax);
  virtual vtkDoubleArray *GetImageMin() { return this->ImageMin; };
  virtual vtkDoubleArray *GetImageMax() { return this->ImageMax; };
  //@}

  //@{
  /**
   * Get the number of ImageMinMax dimensions.
   */
  vtkGetMacro(NumberOfImageMinMaxDimensions, int);
  vtkSetMacro(NumberOfImageMinMaxDimensions, int);
  //@}

  /**
   * Check to see if a particular attribute exists.
   */
  virtual int HasAttribute(const char *variable, const char *attribute);

  //@{
  /**
   * Set attribute values for a variable as a vtkDataArray.
   * Set the variable to the empty string to access global attributes.
   */
  virtual void SetAttributeValueAsArray(const char *variable,
                                        const char *attribute,
                                        vtkDataArray *array);
  virtual vtkDataArray *GetAttributeValueAsArray(const char *variable,
                                                 const char *attribute);
  //@}

  //@{
  /**
   * Set an attribute value as a string.  Set the variable
   * to the empty string to access global attributes.
   * If you specify a variable that does not exist, it will be
   * created.
   */
  virtual void SetAttributeValueAsString(const char *variable,
                                         const char *attribute,
                                         const char *value);
  virtual const char *GetAttributeValueAsString(const char *variable,
                                                const char *attribute);
  //@}

  //@{
  /**
   * Set an attribute value as an int. Set the variable
   * to the empty string to access global attributes.
   * If you specify a variable that does not exist, it will be
   * created.
   */
  virtual void SetAttributeValueAsInt(const char *variable,
                                      const char *attribute,
                                      int value);
  virtual int GetAttributeValueAsInt(const char *variable,
                                     const char *attribute);
  //@}

  //@{
  /**
   * Set an attribute value as a double.  Set the variable
   * to the empty string to access global attributes.
   * If you specify a variable that does not exist, it will be
   * created.
   */
  virtual void SetAttributeValueAsDouble(const char *variable,
                                         const char *attribute,
                                         double value);
  virtual double GetAttributeValueAsDouble(const char *variable,
                                           const char *attribute);
  //@}


  /**
   * Validate a particular attribute.  This involves checking
   * that the attribute is a MINC standard attribute, and checking
   * whether it can be set (as opposed to being set automatically
   * from the image information).  The return values is 0 if
   * the attribute is set automatically and therefore should not
   * be copied from here, 1 if this attribute is valid and should
   * be set, and 2 if the attribute is non-standard.
   */
  virtual int ValidateAttribute(const char *varname,
                                const char *attname,
                                vtkDataArray *array);

  //@{
  /**
   * Set this to Off if you do not want to automatically validate
   * every attribute that is set.
   */
  vtkSetMacro(ValidateAttributes, vtkTypeBool);
  vtkBooleanMacro(ValidateAttributes, vtkTypeBool);
  vtkGetMacro(ValidateAttributes, vtkTypeBool);
  //@}

  /**
   * Do a shallow copy.  This will copy all the attributes
   * from the source.  It is much more efficient than a DeepCopy
   * would be, since it only copies pointers to the attribute values
   * instead of copying the arrays themselves.  You must use this
   * method to make a copy if you want to modify any MINC attributes
   * from a MINCReader before you pass them to a MINCWriter.
   */
  virtual void ShallowCopy(vtkMINCImageAttributes *source);

  /**
   * Find the valid range of the data from the information stored
   * in the attributes.
   */
  virtual void FindValidRange(double range[2]);

  /**
   * Find the image range of the data from the information stored
   * in the attributes.
   */
  virtual void FindImageRange(double range[2]);

  //@{
  /**
   * A diagnostic function.  Print the header of the file in
   * the same format as ncdump or mincheader.
   */
  virtual void PrintFileHeader();
  virtual void PrintFileHeader(ostream &os);
  //@}

protected:
  vtkMINCImageAttributes();
  ~vtkMINCImageAttributes() override;

  const char *ConvertDataArrayToString(vtkDataArray *array);

  virtual int ValidateGlobalAttribute(const char *attrib,
                                      vtkDataArray *array);
  virtual int ValidateGeneralAttribute(const char *varname,
                                       const char *attname,
                                       vtkDataArray *array);
  virtual int ValidateDimensionAttribute(const char *varname,
                                         const char *attname,
                                         vtkDataArray *array);
  virtual int ValidateImageAttribute(const char *varname,
                                     const char *attname,
                                     vtkDataArray *array);
  virtual int ValidateImageMinMaxAttribute(const char *varname,
                                           const char *attname,
                                           vtkDataArray *array);
  virtual int ValidatePatientAttribute(const char *varname,
                                       const char *attname,
                                       vtkDataArray *array);
  virtual int ValidateStudyAttribute(const char *varname,
                                     const char *attname,
                                     vtkDataArray *array);
  virtual int ValidateAcquisitionAttribute(const char *varname,
                                           const char *attname,
                                           vtkDataArray *array);

  vtkStringArray *DimensionNames;
  vtkIdTypeArray *DimensionLengths;

  vtkStringArray *VariableNames;
  vtkMINCImageAttributeMap *AttributeNames;
  vtkMINCImageAttributeMap *AttributeValues;

  vtkStringArray *StringStore;

  vtkDoubleArray *ImageMin;
  vtkDoubleArray *ImageMax;
  int NumberOfImageMinMaxDimensions;

  int DataType;
  char *Name;

  vtkTypeBool ValidateAttributes;

private:
  vtkMINCImageAttributes(const vtkMINCImageAttributes&) = delete;
  void operator=(const vtkMINCImageAttributes&) = delete;

};

#endif /* vtkMINCImageAttributes_h */
