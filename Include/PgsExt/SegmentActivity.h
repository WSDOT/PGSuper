///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2015  Washington State Department of Transportation
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

#include <PgsExt\PgsExtExp.h>
#include <PgsExt\Keys.h>

/*****************************************************************************
CLASS 
   CSegmentActivityBase

DESCRIPTION
   Encapsulates the data for segment based activities
*****************************************************************************/

class PGSEXTCLASS CSegmentActivityBase
{
public:
   CSegmentActivityBase();
   CSegmentActivityBase(const CSegmentActivityBase& rOther);
   ~CSegmentActivityBase();

   CSegmentActivityBase& operator= (const CSegmentActivityBase& rOther);
   bool operator==(const CSegmentActivityBase& rOther) const;
   bool operator!=(const CSegmentActivityBase& rOther) const;

   void Enable(bool bEnable=true);
   bool IsEnabled() const;

   void Clear();

   void AddSegment(SegmentIDType segmentID);
   void AddSegments(const std::set<SegmentIDType>& segments);
   const std::set<SegmentIDType>& GetSegments() const;
   bool HasSegment(SegmentIDType segmentID) const;
   void RemoveSegment(SegmentIDType segmentID);
   IndexType GetSegmentCount() const;

	HRESULT Load(IStructuredLoad* pStrLoad,IProgress* pProgress);
	HRESULT Save(IStructuredSave* pStrSave,IProgress* pProgress);

protected:
   void MakeCopy(const CSegmentActivityBase& rOther);
   virtual void MakeAssignment(const CSegmentActivityBase& rOther);
   bool m_bEnabled;

   // called by load/save to give subclasses an opportunity to load/save data
   virtual HRESULT LoadSubclassData(IStructuredLoad* pStrLoad,IProgress* pProgress);
   virtual HRESULT SaveSubclassData(IStructuredSave* pStrSave,IProgress* pProgress);

   // Segments must be stored by ID because IDs are unique and they don't change
   // when the index of a segment changes
   std::set<SegmentIDType> m_Segments;

   virtual LPCTSTR GetUnitName() = 0;
};

// This activity represents the construction of a precast segment which includes
// jacking of the prestressing strands, casting of the concrete, curing, release,
// prestress transfer, lifting from the casting bed, and
// placing into a storage configuration.
// It will be assumed that all segments are constucted at the same time
class PGSEXTCLASS CConstructSegmentActivity : public CSegmentActivityBase
{
public:
   CConstructSegmentActivity();
   CConstructSegmentActivity(const CConstructSegmentActivity& rOther);

   CConstructSegmentActivity& operator= (const CConstructSegmentActivity& rOther);
   bool operator==(const CConstructSegmentActivity& rOther) const;
   bool operator!=(const CConstructSegmentActivity& rOther) const;

   // Duration of time from strand stressing to release.
   // Used to determine the amount if initial relaxation.
   void SetRelaxationTime(Float64 r);
   Float64 GetRelaxationTime() const;

   // Age of concrete at release (this is the curing time)
   // Used to compute f'ci and beginning of creep and shrinkage
   void SetAgeAtRelease(Float64 age);
   Float64 GetAgeAtRelease() const;

protected:
   virtual LPCTSTR GetUnitName() { return _T("ConstructSegments"); }
   void MakeCopy(const CConstructSegmentActivity& rOther);
   virtual void MakeAssignment(const CConstructSegmentActivity& rOther);

   virtual HRESULT LoadSubclassData(IStructuredLoad* pStrLoad,IProgress* pProgress);
   virtual HRESULT SaveSubclassData(IStructuredSave* pStrSave,IProgress* pProgress);

   Float64 m_RelaxationTime;
   Float64 m_AgeAtRelease;
};

// This activity represents the erection of a precast segment into its permanent
// location. It is assumed that transportation and erection occur in the same day
// and are thus modeled with this activity.
class PGSEXTCLASS CErectSegmentActivity : public CSegmentActivityBase
{
protected:
   virtual LPCTSTR GetUnitName() { return _T("ErectSegments"); }
};
