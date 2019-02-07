///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2019  Washington State Department of Transportation
//                        Bridge and Structures Office
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the Alternate Route Open Source License as 
// published by the Washington State Department of Transportation, 
// Bridge and Structures Office.
//
// This program is distributed in the hope that it will be useful, but 
// distribution is AS IS, WITHOUT ANY WARRANTY; without even the implied 
// warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See 
// the Alternate Route Open Source License for more details.
//
// You should have received a copy of the Alternate Route Open Source 
// License along with this program; if not, write to the Washington 
// State Department of Transportation, Bridge and Structures Office, 
// P.O. Box  47340, Olympia, WA 98503, USA or e-mail 
// Bridge_Support@wsdot.wa.gov
///////////////////////////////////////////////////////////////////////

#pragma once

#include "psgLibLib.h"
#include <psgLib\ISupportIcon.h>
#include <libraryFw\LibraryEntry.h>
#include <System\SubjectT.h>

class pgsLibraryEntryDifferenceItem;
class ConnectionLibraryEntry;
class ConnectionLibraryEntryObserver;
#pragma warning(disable:4231)
PSGLIBTPL sysSubjectT<ConnectionLibraryEntryObserver, ConnectionLibraryEntry>;

/*****************************************************************************
CLASS 
   ConnectionLibraryEntryObserver

   A pure virtual entry class for observing Connection material entries.


DESCRIPTION
   This class may be used to describe observe Connection  materials in a library.

LOG
   rdp : 07.20.1998 : Created file
*****************************************************************************/
class PSGLIBCLASS ConnectionLibraryEntryObserver
{
public:
   //------------------------------------------------------------------------
   // called by our subject to let us now he's changed, along with an optional
   // hint
   virtual void Update(ConnectionLibraryEntry* pSubject, Int32 hint)=0;
};

/*****************************************************************************
CLASS 
   ConnectionLibraryEntry

   A library entry class for girder connections


DESCRIPTION
   This class may be used to describe Connections in a library.

LOG
   rdp : 07.20.1998 : Created file
*****************************************************************************/

class PSGLIBCLASS ConnectionLibraryEntry : public libLibraryEntry, public ISupportIcon,
       public sysSubjectT<ConnectionLibraryEntryObserver, ConnectionLibraryEntry>
{
public:
   // diaphragm loading types
   enum DiaphragmLoadType
   {
      ApplyAtBearingCenterline,
      ApplyAtSpecifiedLocation, // measured from C.L. pier 
      DontApply
   };

   // defines how girder end distances and bearing offsets are measured
   enum EndDistanceMeasurementType
   {
      FromBearingAlongGirder = 0,
      FromBearingNormalToPier = 1, // this is really From and Normal to CL Bearing (don't want to change it because too many things depend on this name)
      FromPierAlongGirder = 2,
      FromPierNormalToPier = 3
   };

   enum BearingOffsetMeasurementType // always measured from CL pier
   {
      AlongGirder = 0, 
      NormalToPier = 1
   };
   static CString GetBearingOffsetMeasurementType(ConnectionLibraryEntry::BearingOffsetMeasurementType measurementType);
   static CString GetEndDistanceMeasurementType(ConnectionLibraryEntry::EndDistanceMeasurementType measurementType);
   static CString GetDiaphragmLoadType(ConnectionLibraryEntry::DiaphragmLoadType loadType);

   static std::_tstring StringForEndDistanceMeasurementType(EndDistanceMeasurementType type);
   static ConnectionLibraryEntry::EndDistanceMeasurementType EndDistanceMeasurementTypeFromString(LPCTSTR strType);

   static std::_tstring StringForBearingOffsetMeasurementType(BearingOffsetMeasurementType type);
   static ConnectionLibraryEntry::BearingOffsetMeasurementType BearingOffsetMeasurementTypeFromString(LPCTSTR strType);


   // GROUP: LIFECYCLE

   //------------------------------------------------------------------------
   // Default constructor
   ConnectionLibraryEntry();

   //------------------------------------------------------------------------
   // Copy constructor
   ConnectionLibraryEntry(const ConnectionLibraryEntry& rOther);

   //------------------------------------------------------------------------
   // Destructor
   virtual ~ConnectionLibraryEntry();

   // GROUP: OPERATORS
   //------------------------------------------------------------------------
   // Assignment operator
   ConnectionLibraryEntry& operator = (const ConnectionLibraryEntry& rOther);

   // GROUP: OPERATIONS

   //------------------------------------------------------------------------
   // Edit the entry
   virtual bool Edit(bool allowEditing,int nPage=0);

   //------------------------------------------------------------------------
   // Get the icon for this entry
   virtual HICON GetIcon() const;

   //------------------------------------------------------------------------
   // Save to structured storage
   virtual bool SaveMe(sysIStructuredSave* pSave);

   //------------------------------------------------------------------------
   // Load from structured storage
   virtual bool LoadMe(sysIStructuredLoad* pLoad);

    // GROUP: ACCESS
   //------------------------------------------------------------------------
   // SetGirderEndDistance - set girder end distance
   void SetGirderEndDistance(Float64 d);

   //------------------------------------------------------------------------
   // GetGirderEndDistance - get girder end distance
   Float64 GetGirderEndDistance() const;

   //------------------------------------------------------------------------
   // SetGirderBearingOffset - set girder bearing offset
   void SetGirderBearingOffset(Float64 d);

   //------------------------------------------------------------------------
   // GetGirderBearingOffset - get girder bearing offset
   Float64 GetGirderBearingOffset() const;

   void SetBearingOffsetMeasurementType(BearingOffsetMeasurementType mt);
   BearingOffsetMeasurementType GetBearingOffsetMeasurementType() const;

   void SetEndDistanceMeasurementType(EndDistanceMeasurementType mt);
   EndDistanceMeasurementType GetEndDistanceMeasurementType() const;

   Float64 GetSupportWidth() const;

   //------------------------------------------------------------------------
   // SetDiaphragmHeight - set diaphragm height
   void SetDiaphragmHeight(Float64 d);

   //------------------------------------------------------------------------
   // GetDiaphragmHeight - get diaphragm height
   Float64 GetDiaphragmHeight() const;

   //------------------------------------------------------------------------
   // SetDiaphragmWidth - set diaphragm width
   void SetDiaphragmWidth(Float64 s);

   //------------------------------------------------------------------------
   // GetDiaphragmWidth - get diaphragm width
   Float64 GetDiaphragmWidth()const;

   //------------------------------------------------------------------------
   // Get how diaphragm loads are applied
   DiaphragmLoadType GetDiaphragmLoadType() const;

   //------------------------------------------------------------------------
   // Set how diaphragm loads are applied
   // Also sets DiaphragmLoadLocation = 0.0
   void SetDiaphragmLoadType(DiaphragmLoadType type);

   //------------------------------------------------------------------------
   // Get diaphgram load location measure from C.L. pier
   // Note: This call only valid if DiaphragmLoadType==ApplyAtSpecifiedLocation 
   Float64 GetDiaphragmLoadLocation() const;

   //------------------------------------------------------------------------
   // Set Diaphragm load location measure from C.L. pier
   // Note: This call only valid if DiaphragmLoadType==ApplyAtSpecifiedLocation 
   void SetDiaphragmLoadLocation(Float64 loc);

   // Compares this library entry with rOther. Returns true if the entries are the same.
   // vDifferences contains a listing of the differences. The caller is responsible for deleting the difference items
   bool Compare(const ConnectionLibraryEntry& rOther, std::vector<pgsLibraryEntryDifferenceItem*>& vDifferences, bool& bMustRename, bool bReturnOnFirstDifference=false, bool considerName=false) const;

   bool IsEqual(const ConnectionLibraryEntry& rOther,bool bConsiderName=false) const;

   // GROUP: INQUIRY

protected:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   void MakeCopy(const ConnectionLibraryEntry& rOther);

   //------------------------------------------------------------------------
   void MakeAssignment(const ConnectionLibraryEntry& rOther);
  // GROUP: ACCESS
  // GROUP: INQUIRY

private:
   // GROUP: DATA MEMBERS
   Float64 m_GirderEndDistance;
   Float64 m_GirderBearingOffset;
   EndDistanceMeasurementType m_EndDistanceMeasure;
   BearingOffsetMeasurementType m_BearingOffsetMeasure;
   Float64 m_SupportWidth;

   Float64 m_DiaphragmHeight;
   Float64 m_DiaphragmWidth;
   DiaphragmLoadType m_DiaphragmLoadType;
   Float64 m_DiaphragmLoadLocation;

   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   // GROUP: ACCESS
   // GROUP: INQUIRY

public:
   // GROUP: DEBUG
   #if defined _DEBUG
   //------------------------------------------------------------------------
   // Returns true if the object is in a valid state, otherwise returns false.
   virtual bool AssertValid() const;

   //------------------------------------------------------------------------
   // Dumps the contents of the object to the given dump context.
   virtual void Dump(dbgDumpContext& os) const;
   #endif // _DEBUG

   #if defined _UNITTEST
   //------------------------------------------------------------------------
   // Runs a self-diagnostic test.  Returns true if the test passed,
   // otherwise false.
   static bool TestMe(dbgLog& rlog);
   #endif // _UNITTEST
};

