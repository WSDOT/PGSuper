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

#include <PgsExt\PgsExtExp.h>

class CBridgeDescription2;

class PGSEXTCLASS CCastingRegion
{
public:
   enum RegionType 
   { 
      Span, // Regions near the center of the span, typically positive moment regions
      Pier  // REgions over piers, typically negative moment regions
   };

   CCastingRegion();
   CCastingRegion(SpanIndexType spanIdx, IndexType sequenceIdx);
   CCastingRegion(PierIndexType pierIdx, Float64 start, Float64 end, IndexType sequenceIdx);
   bool operator==(const CCastingRegion& rOther) const;

   HRESULT Load(IStructuredLoad* pStrLoad, IProgress* pProgress);
   HRESULT Save(IStructuredSave* pStrSave, IProgress* pProgress);

   RegionType m_Type;
   IndexType m_Index; // span or pier index, depending on m_Type

   // start/end are only valid if m_Type == Pier
   // start end can be absolute value or fractional measure
   // fractional measure is [0.0,-1.0)
   Float64 m_Start; // start of region, measured back from CL Pier
   Float64 m_End; // end of region, measured ahead from CL Pier

   IndexType m_SequenceIndex; // casting sequence index. regions with the same sequence are cast at the same time
};

/*****************************************************************************
CLASS 
   CCastDeckActivity

DESCRIPTION
   Encapsulates the data for the cast deck activity
*****************************************************************************/

class PGSEXTCLASS CCastDeckActivity
{
public:
   enum CastingType 
   {
      Continuous, // entire deck is cast at the same time
      Staged // deck is cast in stages
   };

   CCastDeckActivity();
   CCastDeckActivity(const CCastDeckActivity& rOther);
   ~CCastDeckActivity();

   CCastDeckActivity& operator= (const CCastDeckActivity& rOther);
   bool operator==(const CCastDeckActivity& rOther) const;
   bool operator!=(const CCastDeckActivity& rOther) const;

   void Enable(bool bEnable = true);
   bool IsEnabled() const;

   void SetCastingType(CCastDeckActivity::CastingType castingType);
   CCastDeckActivity::CastingType GetCastingType() const;

   void SetConcreteAgeAtContinuity(Float64 age);
   Float64 GetConcreteAgeAtContinuity() const;

   void SetCuringDuration(Float64 duration);
   Float64 GetCuringDuration() const;

   // returns the total duration of this activity
   Float64 GetDuration() const;

   // Set/Get when elapsed time between the start of sequential casting operations
   void SetTimeBetweenCasting(Float64 time);
   Float64 GetTimeBetweenCasting() const;

   // Set/Get region boundary type
   void SetDeckCastingRegionBoundary(pgsTypes::DeckCastingRegionBoundary boundary);
   pgsTypes::DeckCastingRegionBoundary GetDeckCastingRegionBoundary() const;

   // information about the casting regions
   void SetCastingRegions(const std::vector<CCastingRegion>& vRegions);
   const std::vector<CCastingRegion>& GetCastingRegions() const;
   IndexType GetCastingRegionCount() const;

   // information about the casting sequence
   // returns number of deck casting operations (number of unique sequence numbers)
   IndexType GetCastingCount() const;
   // returns the sequence number for the casting index
   IndexType GetSequenceNumber(IndexType castingIdx) const;
   // returns the index of regions cast during a partical casting
   std::vector<IndexType> GetRegions(IndexType castingIdx) const;

   // Called by the framework when a girder group is removed to remove the associated
   // deck casting regions
   void RemoveGirderGroup(const CBridgeDescription2* pBridge, GroupIndexType grpIdx, pgsTypes::RemovePierType rmPierType);
   void RemoveSpan(const CBridgeDescription2* pBridge,SpanIndexType spanIdx, pgsTypes::RemovePierType rmPierType);
   void InsertSpan(const CBridgeDescription2* pBridge, SpanIndexType newSpanIdx, PierIndexType newPierIdx);

   void RemoveNegMomentRegion(PierIndexType pierIdx);
   void AddNegMomentRegion(PierIndexType pierIdx);

   HRESULT Load(IStructuredLoad* pStrLoad,IProgress* pProgress);
	HRESULT Save(IStructuredSave* pStrSave,IProgress* pProgress);

protected:
   void MakeCopy(const CCastDeckActivity& rOther);
   void MakeAssignment(const CCastDeckActivity& rOther);
   bool m_bEnabled;
   
   CastingType m_CastingType;
   Float64 m_TimeBetweenCastings;
   pgsTypes::DeckCastingRegionBoundary m_DeckCastingRegionBoundary;

   Float64 m_Age;
   Float64 m_CuringDuration;

   std::vector<CCastingRegion> m_vCastingRegions;
   std::vector<CCastingRegion> m_vContinuousCastingRegions; // this is kind of a dummy region... it covers the entire bridge

   void UpdateCastings();
   std::vector<IndexType> m_vCastingOrder; // contains the casting sequence number is sorted order
};
