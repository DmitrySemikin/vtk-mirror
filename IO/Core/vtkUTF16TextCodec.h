/*===========================================================================*/
/* Distributed under OSI-approved BSD 3-Clause License.                      */
/* For copyright, see the following accompanying files or https://vtk.org:   */
/* - VTK-Copyright.txt                                                       */
/* - Sandia-Copyright.txt                                                    */
/*===========================================================================*/
/**
 * @class   vtkUTF16TextCodec
 * @brief   Class to read/write ascii text.
 *
 *
 * A virtual class interface for codecs that readers/writers can rely on
 *
 * @par Thanks:
 * Thanks to Tim Shed from Sandia National Laboratories for his work
 * on the concepts and to Marcus Hanwell and Jeff Baumes of Kitware for
 * keeping me out of the weeds
 *
 * @sa
 * vtkUTF16TextCodecFactory
 *
*/

#ifndef vtkUTF16TextCodec_h
#define vtkUTF16TextCodec_h

#include "vtkIOCoreModule.h" // For export macro
#include "vtkTextCodec.h"


class VTKIOCORE_EXPORT vtkUTF16TextCodec : public vtkTextCodec
{
public:
  vtkTypeMacro(vtkUTF16TextCodec, vtkTextCodec);
  static vtkUTF16TextCodec* New() ;
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * The name this codec goes by - should match the string the factory will take to create it
   */
  const char* Name() override;
  bool CanHandle(const char* NameString) override ;
  //@}

  /**
   * Set the endianness - true if Big false is little
   */
  void SetBigEndian(bool) ;

  /**
   * Set the endianness - true if Big false is little
   */
  void FindEndianness(istream& InputStream) ;

  /**
   * is the given sample valid for this codec? - will take endianness into account
   */
  bool IsValid(istream& InputStream) override ;

  /**
   * Iterate through the sequence represented by the begin and end iterators assigning the result
   * to the output iterator.  This is the current pattern in vtkDelimitedTextReader
   */
  void ToUnicode(istream& InputStream,
                         vtkTextCodec::OutputIterator& output) override ;

  /**
   * Return the next code point from the sequence represented by the begin, end iterators
   * advancing begin through however many places needed to assemble that code point
   */
  vtkUnicodeString::value_type  NextUnicode(istream& inputStream) override ;

protected:
  vtkUTF16TextCodec() ;
  ~vtkUTF16TextCodec() override;

  bool _endianExplicitlySet ;
  bool _bigEndian ;

private:
  vtkUTF16TextCodec(const vtkUTF16TextCodec &) = delete;
  void operator=(const vtkUTF16TextCodec &) = delete;

};


#endif
