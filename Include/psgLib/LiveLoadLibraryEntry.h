///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2017  Washington State Department of Transportation
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

#ifndef INCLUDED_PSGLIB_USER_LIVE_LOAD_H_
#define INCLUDED_PSGLIB_USER_LIVE_LOAD_H_

// SYSTEM INCLUDES
//

// PROJECT INCLUDES
//

#include "psgLibLib.h"

#include <psgLib\ISupportIcon.h>
#include <libraryFw\LibraryEntry.h>

#if !defined INCLUDED_SYSTEM_SUBJECTT_H_
#include <System\SubjectT.h>
#endif

// FORWARD DECLARATIONS
//
class pgsLibraryEntryDifferenceItem;
class LiveLoadLibraryEntry;
class LiveLoadLibraryEntryObserver;
#pragma warning(disable:4231)
PSGLIBTPL sysSubjectT<LiveLoadLibraryEntryObserver, LiveLoadLibraryEntry>;

// MISCELLANEOUS
//
/*****************************************************************************
CLASS 
   LiveLoadLibraryEntryObserver

   A pure virtual entry class for observing live load entries.


DESCRIPTION
   This class may be used to describe observe live loads in a library.

LOG
   rdp : 01.24.2005 : Created file
*****************************************************************************/
class PSGLIBCLASS LiveLoadLibraryEntryObserver
{
public:

   // GROUP: LIFECYCLE
   //------------------------------------------------------------------------
   // called by our subject to let us now he's changed, along with an optional
   // hint
   virtual void Update(LiveLoadLibraryEntry* pSubject, Int32 hint)=0;
};

/*****************************************************************************
CLASS 
   LiveLoadLibraryEntry

   A library entry class for user-defined live loads


DESCRIPTION
   This class may be used to describe user-defined live loads in a library.

LOG
   rdp : 01.24.2005 : Created file
*****************************************************************************/

class PSGLIBCLASS LiveLoadLibraryEntry : public libLibraryEntry, public ISupportIcon,
       public sysSubjectT<LiveLoadLibraryEntryObserver, LiveLoadLibraryEntry>
{
public:
   struct Axle
   {
      Float64 Weight;
      Float64 Spacing; // Spacing before axle. If Variable axle, this is value of
                       // the minimum spacing. First axle has no spacing.
      Axle():
      Weight(0.0),
      Spacing(0.0)
      {;}
   };

   // ConfigurationType - how truck/lane are combined
   enum LiveLoadConfigurationType {lcTruckOnly,lcLaneOnly,lcTruckPlusLane,lcTruckLaneEnvelope};

   // GROUP: LIFECYCLE
   //------------------------------------------------------------------------
   // Default constructor
   LiveLoadLibraryEntry();

   //------------------------------------------------------------------------
   // Copy constructor
   LiveLoadLibraryEntry(const LiveLoadLibraryEntry& rOther);

   //------------------------------------------------------------------------
   // Destructor
   virtual ~LiveLoadLibraryEntry();

   // GROUP: OPERATORS
   //------------------------------------------------------------------------
   // Assignment operator
   LiveLoadLibraryEntry& operator = (const LiveLoadLibraryEntry& rOther);

   // GROUP: OPERATIONS

   //------------------------------------------------------------------------
   // Edit the entry
   virtual bool Edit(bool allowEditing,int nPage=0);

   //------------------------------------------------------------------------
   // Save to structured storage
   virtual bool SaveMe(sysIStructuredSave* pSave);

   //------------------------------------------------------------------------
   // Load from structured storage
   virtual bool LoadMe(sysIStructuredLoad* pLoad);

   //------------------------------------------------------------------------
   // Get the icon for this entry
   virtual HICON GetIcon() const;

   // GROUP: ACCESS
   //------------------------------------------------------------------------
   // IsNotional - If true, Only apply those axles contributing to extreme 
   //              force under consideration. (LRFD 3.6.1.3.1)
   void SetIsNotional(bool flag);
   bool GetIsNotional() const;

   //------------------------------------------------------------------------
   // LiveLoadConfigurationType - Determines how truck and lane are combined
   void SetLiveLoadConfigurationType(LiveLoadConfigurationType config);
   LiveLoadConfigurationType GetLiveLoadConfigurationType() const;

   //------------------------------------------------------------------------
   // LiveLoadApplicabilityType - Determines how and where results are used
   void SetLiveLoadApplicabilityType(pgsTypes::LiveLoadApplicabilityType applicability);
   pgsTypes::LiveLoadApplicabilityType GetLiveLoadApplicabilityType() const;

   //------------------------------------------------------------------------
   // LaneLoad - Value of distributed lane load
   void SetLaneLoad(Float64 val);
   Float64 GetLaneLoad() const;

   //------------------------------------------------------------------------
   // LaneLoadSpanLength - Minimum span length for which the lane load will be applied
   void SetLaneLoadSpanLength(Float64 val);
   Float64 GetLaneLoadSpanLength() const;

   //------------------------------------------------------------------------
   // GetNumAxles - returns the number of axles
   AxleIndexType GetNumAxles() const;

   //------------------------------------------------------------------------
   // Append a new axle
   void AddAxle(Axle axle);

   //------------------------------------------------------------------------
   // Overwrite an existing axle
   void SetAxle(AxleIndexType idx, Axle axle);

   //------------------------------------------------------------------------
   // Get a particular axle
   Axle GetAxle(AxleIndexType idx) const;

   //------------------------------------------------------------------------
   // Remove all axles from collection
   void ClearAxles();

   //------------------------------------------------------------------------
   // VariableAxleIndex - Index of variable axle.
   //                     if less than zero, there is no variable-spaced axle.
   void SetVariableAxleIndex(AxleIndexType idx);
   AxleIndexType GetVariableAxleIndex() const;

   //------------------------------------------------------------------------
   // MaxVariableAxleSpacing - Value of max spacing for variable axle.
   //                          Must be larger than spacing for axle.
   void SetMaxVariableAxleSpacing(Float64 val);
   Float64 GetMaxVariableAxleSpacing() const;

   // Compares this library entry with rOther. Returns true if the entries are the same.
   // vDifferences contains a listing of the differences. The caller is responsible for deleting the difference items
   bool Compare(const LiveLoadLibraryEntry& rOther, std::vector<pgsLibraryEntryDifferenceItem*>& vDifferences, bool bReturnOnFirstDifference=false,bool considerName=false) const;

   bool IsEqual(const LiveLoadLibraryEntry& rOther,bool bConsiderName=false) const;

   // GROUP: INQUIRY

protected:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   void MakeCopy(const LiveLoadLibraryEntry& rOther);

   //------------------------------------------------------------------------
   void MakeAssignment(const LiveLoadLibraryEntry& rOther);
  // GROUP: ACCESS
  // GROUP: INQUIRY

public:

   typedef std::vector<Axle> AxleContainer;
   typedef AxleContainer::iterator AxleIterator;

   static CString GetConfigurationType(LiveLoadConfigurationType configuration);
   static CString GetApplicabilityType(pgsTypes::LiveLoadApplicabilityType applicability);

private:
   // GROUP: DATA MEMBERS
   bool m_IsNotional;
   LiveLoadConfigurationType m_LiveLoadConfigurationType;
   pgsTypes::LiveLoadApplicabilityType m_LiveLoadApplicabilityType;
   Float64 m_LaneLoad;
   Float64 m_LaneLoadSpanLength;
   Float64 m_MaxVariableAxleSpacing;
   AxleIndexType   m_VariableAxleIndex;

   AxleContainer m_Axles;

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

// INLINE METHODS
//

// EXTERNAL REFERENCES
//

#endif // INCLUDED_PSGLIB_TRAFFIC_BARRIER_H_
