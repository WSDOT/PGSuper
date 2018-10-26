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

#ifndef INCLUDED_PSGLIB_TRAFFIC_BARRIER_H_
#define INCLUDED_PSGLIB_TRAFFIC_BARRIER_H_

// SYSTEM INCLUDES
//

// PROJECT INCLUDES
//

#include "psgLibLib.h"

#include <psgLib\ISupportIcon.h>
#include <libraryFw\LibraryEntry.h>

#include <WBFLTools.h>
#include <WBFLGeometry.h>

#if !defined INCLUDED_SYSTEM_SUBJECTT_H_
#include <System\SubjectT.h>
#endif

// FORWARD DECLARATIONS
//
class pgsLibraryEntryDifferenceItem;
class TrafficBarrierEntry;
class TrafficBarrierEntryObserver;
#pragma warning(disable:4231)
PSGLIBTPL sysSubjectT<TrafficBarrierEntryObserver, TrafficBarrierEntry>;

interface ISidewalkBarrier;

// MISCELLANEOUS
//
/*****************************************************************************
CLASS 
   TrafficBarrierEntryObserver

   A pure virtual entry class for observing concrete material entries.


DESCRIPTION
   This class may be used to describe observe concrete  materials in a library.

LOG
   rdp : 07.20.1998 : Created file
*****************************************************************************/
class PSGLIBCLASS TrafficBarrierEntryObserver
{
public:

   // GROUP: LIFECYCLE
   //------------------------------------------------------------------------
   // called by our subject to let us now he's changed, along with an optional
   // hint
   virtual void Update(TrafficBarrierEntry* pSubject, Int32 hint)=0;
};

/*****************************************************************************
CLASS 
   TrafficBarrierEntry

   A library entry class for traffic barriers


DESCRIPTION
   This class may be used to describe traffic barriers in a library.

LOG
   rdp : 07.20.1998 : Created file
*****************************************************************************/

class PSGLIBCLASS TrafficBarrierEntry : public libLibraryEntry, public ISupportIcon,
       public sysSubjectT<TrafficBarrierEntryObserver, TrafficBarrierEntry>
{
public:
   // GROUP: LIFECYCLE
   enum WeightMethod
   {
      Compute,
      Input
   };
   static CString GetWeightMethodType(TrafficBarrierEntry::WeightMethod weightMethod);

   //------------------------------------------------------------------------
   // Default constructor
   TrafficBarrierEntry();

   //------------------------------------------------------------------------
   // Copy constructor
   TrafficBarrierEntry(const TrafficBarrierEntry& rOther);

   //------------------------------------------------------------------------
   // Destructor
   virtual ~TrafficBarrierEntry();

   // GROUP: OPERATORS
   //------------------------------------------------------------------------
   // Assignment operator
   TrafficBarrierEntry& operator = (const TrafficBarrierEntry& rOther);

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

   void SetBarrierPoints(IPoint2dCollection* points);
   void GetBarrierPoints(IPoint2dCollection** points) const;

   void CreatePolyShape(pgsTypes::TrafficBarrierOrientation orientation,IPolyShape** polyShape) const;

   void SetWeightMethod(WeightMethod method);
   WeightMethod GetWeightMethod() const;
   void SetWeight(Float64 w);
   Float64 GetWeight() const;
   void SetEc(Float64 ec);
   Float64 GetEc() const;

   bool IsBarrierStructurallyContinuous() const;
   void IsBarrierStructurallyContinuous(bool bContinuous);

   void SetCurbOffset(Float64 curbOffset);
   Float64 GetCurbOffset() const;

   // Compares this library entry with rOther. Returns true if the entries are the same.
   // vDifferences contains a listing of the differences. The caller is responsible for deleting the difference items
   bool Compare(const TrafficBarrierEntry& rOther, std::vector<pgsLibraryEntryDifferenceItem*>& vDifferences, bool bReturnOnFirstDifference=false,bool considerName=false) const;

   bool IsEqual(const TrafficBarrierEntry& rOther,bool bConsiderName=false) const;

   // GROUP: INQUIRY

protected:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   void MakeCopy(const TrafficBarrierEntry& rOther);

   //------------------------------------------------------------------------
   void MakeAssignment(const TrafficBarrierEntry& rOther);
  // GROUP: ACCESS
  // GROUP: INQUIRY

private:
   // GROUP: DATA MEMBERS
   CComPtr<IPoint2dCollection> m_BarrierPoints;
   WeightMethod m_WeightMethod;
   Float64 m_Weight;
   Float64 m_Ec;
   Float64 m_CurbOffset; // offset from face of barrier to "notional" curb line
   bool m_bStructurallyContinuous;

   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   bool ComparePoints(IPoint2dCollection* points1,IPoint2dCollection* points2) const;
   void CopyPoints(IPoint2dCollection* points1,IPoint2dCollection* points2);
   void CopyPoints(IPoint2dCollection* points1,IPoint2dCollection* points2) const;
   void ConvertDimensionsToPoints(Float64 x1,Float64 x2,Float64 x3,Float64 x4,Float64 x5,Float64 y1,Float64 y2,Float64 y3);
   void CreatePolyShape(pgsTypes::TrafficBarrierOrientation orientation,IPoint2dCollection* points,IPolyShape** polyShape) const;

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
