/*============================================================================
  MetaIO
  Copyright 2000-2010 Insight Software Consortium

  Distributed under the OSI-approved BSD License (the "License");
  see accompanying file Copyright.txt for details.

  This software is distributed WITHOUT ANY WARRANTY; without even the
  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the License for more information.
============================================================================*/
#include "metaTypes.h"

#ifndef ITKMetaIO_METAOBJECT_H
#define ITKMetaIO_METAOBJECT_H

#include "metaUtils.h"
#include "metaEvent.h"

#ifdef _MSC_VER
#pragma warning ( disable: 4251 )
#endif


#if (METAIO_USE_NAMESPACE)
namespace METAIO_NAMESPACE {
#endif

class METAIO_EXPORT MetaObject
  {
  ////
  //
  // PROTECTED
  //
  ////
  protected:

      typedef METAIO_STL::vector<MET_FieldRecordType *> FieldsContainerType;

      METAIO_STREAM::ifstream * m_ReadStream;
      METAIO_STREAM::ofstream * m_WriteStream;

      FieldsContainerType m_Fields;
      FieldsContainerType m_UserDefinedWriteFields;
      FieldsContainerType m_UserDefinedReadFields;
      FieldsContainerType m_AdditionalReadFields;

      char  m_FileName[255];

      char  m_Comment[255];            // "Comment = "       ""

      char  m_ObjectTypeName[255];     // "ObjectType = "    defined by suffix
      char  m_ObjectSubTypeName[255];  // "ObjectSubType = " defined by suffix

      int   m_NDims;                   // "NDims = "         required

      double m_Offset[10];             // "Offset = "          0,0,0
      double m_TransformMatrix[100];   // "TransformMatrix = " 1,0,0,0,1,0,0,0,1
      double m_CenterOfRotation[10];   // "CenterOfRotation = "  0 0 0

      MET_OrientationEnumType m_AnatomicalOrientation[10];

      MET_DistanceUnitsEnumType m_DistanceUnits;   // "DistanceUnits = mm"

      float m_ElementSpacing[10];   // "ElementSpacing = "   0,0,0

      float m_Color[4];             // "Color = "            1.0, 0.0, 0.0, 1.0

      char  m_AcquisitionDate[255]; // "AcquisitionDate = "  "2007.03.21"

      int   m_ID;                   // "ID = "               0

      int   m_ParentID;             // "ParentID = "         -1

      char  m_Name[255];            // "Name = "             ""

      bool  m_BinaryData;           // "BinaryData = "      False

      bool  m_BinaryDataByteOrderMSB;

      METAIO_STL::streamoff m_CompressedDataSize;
      // Used internally to set if the dataSize should be written
      bool m_WriteCompressedDataSize;
      bool m_CompressedData;

      virtual void M_Destroy(void);

      virtual void M_SetupReadFields(void);

      virtual void M_SetupWriteFields(void);

      virtual bool M_Read(void);

      virtual bool M_Write(void);

      virtual void M_PrepareNewReadStream();

      MetaEvent*     m_Event;
      //MET_FieldRecordType * M_GetFieldRecord(const char * fieldName);
      //int   M_GetFieldRecordNumber(const char * fieldName);

      unsigned int m_DoublePrecision;

  /////
  //
  // PUBLIC
  //
  ////
  public:

      ////
      // Constructors & Destructor
      ////
      MetaObject(void);
      MetaObject(const char * fileName);
      MetaObject(unsigned int dim);

      virtual ~MetaObject(void);

      void  FileName(const char *_fileName);
      const char  * FileName(void) const;

      virtual void  CopyInfo(const MetaObject * object);

      bool  Read(const char * fileName=NULL);

      bool  ReadStream(int nDims, METAIO_STREAM::ifstream * stream);

      bool  Write(const char * fileName=NULL);

      virtual bool  Append(const char *_headName=NULL);

      ////
      //
      // Common fields
      //
      ////

      //    PrintMetaInfo()
      //       Writes image parameters to stdout
      virtual void  PrintInfo(void) const;

      //    Comment(...)
      //       Optional Field
      //       Arbitrary string
      const char  * Comment(void) const;
      void    Comment(const char * comment);

      const char  * ObjectTypeName(void) const;
      void    ObjectTypeName(const char * objectTypeName);
      const char  * ObjectSubTypeName(void) const;
      void    ObjectSubTypeName(const char * objectSubTypeName);

      //    NDims()
      //       REQUIRED Field
      //       Number of dimensions to the image
      int   NDims(void) const;

      //    Offset(...)
      //       Optional Field
      //       Physical location (in millimeters and wrt machine coordinate
      //         system or the patient) of the first element in the image
      const double * Offset(void) const;
      double Offset(int i) const;
      void  Offset(const double * position);
      void  Offset(int i, double value);
      const double * Position(void) const;
      double Position(int i) const;
      void  Position(const double * position);
      void  Position(int i, double value);
      const double * Origin(void) const;
      double Origin(int i) const;
      void  Origin(const double * position);
      void  Origin(int i, double value);

      //    TransformMatrix(...)
      //       Optional Field
      //       Physical orientation of the object as an NDims x NDims matrix
      const double * TransformMatrix(void) const;
      double TransformMatrix(int i, int j) const;
      void  TransformMatrix(const double * orientation);
      void  TransformMatrix(int i, int j, double value);
      const double * Rotation(void) const;
      double Rotation(int i, int j) const;
      void  Rotation(const double * orientation);
      void  Rotation(int i, int j, double value);
      const double * Orientation(void) const;
      double Orientation(int i, int j) const;
      void  Orientation(const double * orientation);
      void  Orientation(int i, int j, double value);

      //
      //
      //
      const double * CenterOfRotation(void) const;
      double CenterOfRotation(int i) const;
      void  CenterOfRotation(const double * position);
      void  CenterOfRotation(int i, double value);

      //
      //
      //
      const char * DistanceUnitsName(void) const;
      MET_DistanceUnitsEnumType DistanceUnits(void) const;
      void DistanceUnits(MET_DistanceUnitsEnumType distanceUnits);
      void DistanceUnits(const char * distanceUnits);

      const char * AnatomicalOrientationAcronym(void) const;
      const MET_OrientationEnumType * AnatomicalOrientation(void) const;
      MET_OrientationEnumType AnatomicalOrientation(int dim) const;
      void AnatomicalOrientation(const char *_ao);
      void AnatomicalOrientation(const MET_OrientationEnumType *_ao);
      void AnatomicalOrientation(int dim, MET_OrientationEnumType ao);
      void AnatomicalOrientation(int dim, char ao);


      //    ElementSpacing(...)
      //       Optional Field
      //       Physical Spacing (in same units as position)
      const float * ElementSpacing(void) const;
      float ElementSpacing(int i) const;
      void  ElementSpacing(const float * elementSpacing);
      void  ElementSpacing(int i, float value);

      //    Name(...)
      //       Optional Field
      //       Name of the current metaObject
      void  Name(const char *_name);
      const char  * Name(void) const;

      //    Color(...)
      //       Optional Field
      //       Color of the current metaObject
      const float * Color(void) const;
      void  Color(float r, float g, float b, float a);
      void  Color(const float * color);

      //    ID(...)
      //       Optional Field
      //       ID number of the current metaObject
      void ID(int id);
      int  ID(void) const;

      //    ParentID(...)
      //       Optional Field
      //       ID number of the parent  metaObject
      void  ParentID(int parentId);
      int   ParentID(void) const;

      //    AcquisitionDate(...)
      //       Optional Field
      //       YYYY.MM.DD is the recommended format
      void  AcquisitionDate(const char * acquisitionDate);
      const char *  AcquisitionDate(void) const;

      //    BinaryData(...)
      //       Optional Field
      //       Data is binary or not
      void  BinaryData(bool binaryData);
      bool  BinaryData(void) const;

      void  BinaryDataByteOrderMSB(bool binaryDataByteOrderMSB);
      bool  BinaryDataByteOrderMSB(void) const;


      void  CompressedData(bool compressedData);
      bool  CompressedData(void) const;


      virtual void Clear(void);

      void ClearFields(void);

      void ClearAdditionalFields(void);

      bool InitializeEssential(int nDims);

      //
      //
      // User's field definitions
      bool AddUserField(const char* fieldName, MET_ValueEnumType type,
                        int length=0, bool required=true,
                        int dependsOn=-1);

      // find a field record in a field vector
      MET_FieldRecordType *FindFieldRecord(FieldsContainerType &container,
                                           const char *fieldName)
      {
        FieldsContainerType::iterator it;
        for(it = container.begin();
            it != container.end();
            it++)
          {
          if(strcmp((*it)->name,fieldName) == 0)
            {
            return (*it);
            }
          }
        return 0;
      }

      // Add a user's field
      template <class T>
      bool AddUserField(const char* fieldName, MET_ValueEnumType type,
                        int length, T* v, bool required=true,
                        int dependsOn=-1 )
        {
        // don't add the same field twice. In the unlikely event
        // a field of the same name gets added more than once,
        // over-write the existing FieldRecord
        bool duplicate(true);
        MET_FieldRecordType* mFw =
          this->FindFieldRecord(m_UserDefinedWriteFields,
                                fieldName);
        if(mFw == 0)
          {
          duplicate = false;
          mFw = new MET_FieldRecordType;
          }
        MET_InitWriteField(mFw, fieldName, type, length, v);
        if(!duplicate)
          {
          m_UserDefinedWriteFields.push_back(mFw);
          }

        duplicate = true;
        MET_FieldRecordType* mFr =
          this->FindFieldRecord(m_UserDefinedReadFields,
                                fieldName);
        if(mFr == 0)
          {
          duplicate = false;
          mFr = new MET_FieldRecordType;
          }

        MET_InitReadField(mFr, fieldName, type, required, dependsOn,
          length);
        if(!duplicate)
          {
          m_UserDefinedReadFields.push_back(mFr);
          }
        return true;
        }

      // Clear UserFields
      void ClearUserFields();

      // Get the user field
      void* GetUserField(const char* name);

      int GetNumberOfAdditionalReadFields();
      char * GetAdditionalReadFieldName( int i );
      char * GetAdditionalReadFieldValue( int i );
      int GetAdditionalReadFieldValueLength( int i );

      //
      void SetEvent(MetaEvent* event) {m_Event = event;}

      // Set the double precision for writing
      void SetDoublePrecision(unsigned int precision)
        {
        m_DoublePrecision = precision;
        }
      unsigned int GetDoublePrecision()
        {
        return m_DoublePrecision;
        }

  };

#if (METAIO_USE_NAMESPACE)
};
#endif

#endif
