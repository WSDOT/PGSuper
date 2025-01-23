///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2025  Washington State Department of Transportation
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

#include <PgsExt\PgsExtLib.h>
#include <PgsExt\CastDeckActivity.h>

#include <PgsExt\BridgeDescription2.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CCastingRegion::CCastingRegion()
{
   m_Index = INVALID_INDEX;
}

CCastingRegion::CCastingRegion(SpanIndexType spanIdx, IndexType sequenceIdx)
{
   m_Type = Span;
   m_Index = spanIdx;
   m_Start = -99999;
   m_End = -99999;
   m_SequenceIndex = sequenceIdx;
}

CCastingRegion::CCastingRegion(PierIndexType pierIdx, Float64 start, Float64 end, IndexType sequenceIdx)
{
   m_Type = Pier;
   m_Index = pierIdx;
   m_Start = start;
   m_End = end;
   m_SequenceIndex = sequenceIdx;
}

bool CCastingRegion::operator==(const CCastingRegion& rOther) const
{
   if (m_Type != rOther.m_Type)
   {
      return false;
   }

   if (m_Index != rOther.m_Index)
   {
      return false;
   }

   if (m_SequenceIndex != rOther.m_SequenceIndex)
   {
      return false;
   }

   if (m_Type == Pier)
   {
      if (!IsEqual(m_Start, rOther.m_Start))
      {
         return false;
      }

      if (!IsEqual(m_End, rOther.m_End))
      {
         return false;
      }
   }

   return true;
}

HRESULT CCastingRegion::Save(IStructuredSave* pStrSave, IProgress* pProgress)
{
   pStrSave->BeginUnit(_T("CastingRegion"), 1.0);
   pStrSave->put_Property(_T("Type"), CComVariant(m_Type));
   pStrSave->put_Property(_T("Index"), CComVariant(m_Index));
   if (m_Type == Pier)
   {
      pStrSave->put_Property(_T("Start"), CComVariant(m_Start));
      pStrSave->put_Property(_T("End"), CComVariant(m_End));
   }
   pStrSave->put_Property(_T("Sequence"), CComVariant(m_SequenceIndex));
   pStrSave->EndUnit(); // CastingRegion
   return S_OK;
}

HRESULT CCastingRegion::Load(IStructuredLoad* pStrLoad, IProgress* pProgress)
{
   CHRException hr;

#if defined _DEBUG
   // Calling BeginUnit will fail when we are out of units
   // Don't assert when this happens. It is a normal and expected
   // failure. If we leave the asserts on, we will get them everytime
   // a file is loaded (what a PINTA)
   hr.AssertOnFailure(false);
#endif

   try
   {
      hr = pStrLoad->BeginUnit(_T("CastingRegion"));
      //Float64 version;
      //pStrLoad->get_Version(&version);

      CComVariant var;
      var.vt = VT_I4;
      hr = pStrLoad->get_Property(_T("Type"), &var);
      m_Type = (RegionType)var.lVal;

      var.vt = VT_INDEX;
      hr = pStrLoad->get_Property(_T("Index"), &var);
      m_Index = VARIANT2INDEX(var);

      if (m_Type == Pier)
      {
         var.vt = VT_R8;
         hr = pStrLoad->get_Property(_T("Start"), &var);
         m_Start = var.dblVal;

         hr = pStrLoad->get_Property(_T("End"), &var);
         m_End = var.dblVal;
      }

      var.vt = VT_INDEX;
      hr = pStrLoad->get_Property(_T("Sequence"), &var);
      m_SequenceIndex = VARIANT2INDEX(var);

      hr = pStrLoad->EndUnit(); // CastingRegion
   }
   catch (HRESULT _hr_)
   {
      return _hr_;
   };

   return S_OK;
}


//////////////////////////////////////////////////////////////

CCastDeckActivity::CCastDeckActivity()
{
   m_bEnabled = false;

   m_CastingType = Continuous;

   m_ActiveCuringDuration = 14.0; // days (WSDOT Std. Specs 6-02.3(11)B2
   m_TotalCuringDuration = 14.0; // days
   m_TimeBetweenCastings = 14.0;

   m_DeckCastingRegionBoundary = pgsTypes::dcrbParallelToPier;

   m_vContinuousCastingRegions.emplace_back(ALL_SPANS, 0);

   m_ClosureJointCastingRegion = INVALID_INDEX;

   UpdateCastings();
}

CCastDeckActivity::~CCastDeckActivity()
{
}

bool CCastDeckActivity::operator==(const CCastDeckActivity& rOther) const
{
   if ( m_bEnabled != rOther.m_bEnabled )
   {
      return false;
   }

   if (m_CastingType != rOther.m_CastingType)
   {
      return false;
   }


   if (!IsEqual(m_TotalCuringDuration, rOther.m_TotalCuringDuration))
   {
      return false;
   }

   if (!IsEqual(m_ActiveCuringDuration, rOther.m_ActiveCuringDuration))
   {
      return false;
   }

   if (m_CastingType == Staged)
   {
      if (m_DeckCastingRegionBoundary != rOther.m_DeckCastingRegionBoundary)
      {
         return false;
      }

      if (!IsEqual(m_TimeBetweenCastings, rOther.m_TimeBetweenCastings))
      {
         return false;
      }

      if (m_vCastingRegions != rOther.m_vCastingRegions)
      {
         return false;
      }

      if (m_ClosureJointCastingRegion != rOther.m_ClosureJointCastingRegion)
      {
         return false;
      }
   }

   return true;
}

bool CCastDeckActivity::operator!=(const CCastDeckActivity& rOther) const
{
   return !operator==(rOther);
}

void CCastDeckActivity::Enable(bool bEnable)
{
   m_bEnabled = bEnable;
}

bool CCastDeckActivity::IsEnabled() const
{
   return m_bEnabled;
}

void CCastDeckActivity::SetCastingType(CCastDeckActivity::CastingType castingType)
{
   m_CastingType = castingType;
   UpdateCastings();
}

CCastDeckActivity::CastingType CCastDeckActivity::GetCastingType() const
{
   return m_CastingType;
}

void CCastDeckActivity::SetTotalCuringDuration(Float64 duration)
{
   m_TotalCuringDuration = duration;
}

Float64 CCastDeckActivity::GetTotalCuringDuration() const
{
   return m_TotalCuringDuration;
}

void CCastDeckActivity::SetActiveCuringDuration(Float64 duration)
{
   m_ActiveCuringDuration = duration;
}

Float64 CCastDeckActivity::GetActiveCuringDuration() const
{
   return m_ActiveCuringDuration;
}

Float64 CCastDeckActivity::GetDuration() const
{
   if (m_CastingType == Continuous)
   {
      return m_TotalCuringDuration;
   }
   else
   {
      // the time between castings is measured from start to start
      // the total duration of this activity is the the sum of all the
      // times between starting each casting plus the age to continuity of the
      // last casting

      // |cast sequence 0
      // +-- age at continuity --|
      // |
      // |                       |cast sequence 1
      // +-time between casting--+-- age at continuity --|
      // |                       |
      // |                       |                       |cast sequence 2
      // |                       +-time between casting--+-- age at continuity --|
      // |                                                                       |
      // |---------------------- DURATION ---------------------------------------|

      IndexType nCastings = GetCastingCount();
      return (nCastings - 1)*m_TimeBetweenCastings + m_TotalCuringDuration;
   }
}

void CCastDeckActivity::SetTimeBetweenCasting(Float64 time)
{
   m_TimeBetweenCastings = time;
}

Float64 CCastDeckActivity::GetTimeBetweenCasting() const
{
   return m_TimeBetweenCastings;
}

void CCastDeckActivity::SetDeckCastingRegionBoundary(pgsTypes::DeckCastingRegionBoundary boundary)
{
   m_DeckCastingRegionBoundary = boundary;
}

pgsTypes::DeckCastingRegionBoundary CCastDeckActivity::GetDeckCastingRegionBoundary() const
{
   return m_DeckCastingRegionBoundary;
}

void CCastDeckActivity::SetCastingRegions(const std::vector<CCastingRegion>& vRegions)
{
   m_vCastingRegions = vRegions;
   if (m_CastingType == Staged)
   {
      UpdateCastings();
   }
}

const std::vector<CCastingRegion>& CCastDeckActivity::GetCastingRegions() const
{
   if (m_CastingType == Continuous)
   {
      return m_vContinuousCastingRegions;
   }
   else
   {
      return m_vCastingRegions;
   }
}

IndexType CCastDeckActivity::GetCastingRegionCount() const
{
   if (m_CastingType == Continuous)
   {
      return m_vContinuousCastingRegions.size();
   }
   else
   {
      return m_vCastingRegions.size();
   }
}

IndexType CCastDeckActivity::GetCastingCount() const
{
   // returns the number of unique deck casting operations
   return (IndexType)m_vCastingOrder.size();
}

IndexType CCastDeckActivity::GetSequenceNumber(IndexType castingIdx) const
{
   // returns the sequence number of a partical deck casting operation
   return m_vCastingOrder[castingIdx];
}

std::vector<IndexType> CCastDeckActivity::GetRegions(IndexType castingIdx) const
{
   // returns a vector of region indexes that are cast during a partical deck casting operation
   IndexType sequenceIdx = GetSequenceNumber(castingIdx);
   const std::vector<CCastingRegion>* pvRegions = (m_CastingType == Continuous ? &m_vContinuousCastingRegions : &m_vCastingRegions);

   std::vector<IndexType> vRegions;
   IndexType regionIdx = 0;
   for (const auto& region : *pvRegions)
   {
      if (region.m_SequenceIndex == sequenceIdx)
      {
         vRegions.push_back(regionIdx);
      }
      regionIdx++;
   }

   return vRegions;
}

void CCastDeckActivity::RemoveGirderGroup(const CBridgeDescription2* pBridge, GroupIndexType grpIdx, pgsTypes::RemovePierType rmPierType)
{
   if (m_CastingType == Continuous)
      return; // there aren't any casting regions for continuous casting... just return

   const CGirderGroupData* pGroup = pBridge->GetGirderGroup(grpIdx);
   PierIndexType startPierIdx = pGroup->GetPierIndex(pgsTypes::metStart);
   SpanIndexType startSpanIdx = (SpanIndexType)startPierIdx;
   SpanIndexType nSpans = pGroup->GetSpanCount();
   // working backwards, remove the casting regions for each span in the groupo
   for (SpanIndexType idx = nSpans-1; 0 <= idx && idx != INVALID_INDEX; idx--)
   {
      SpanIndexType spanIdx = startSpanIdx + idx;
      RemoveSpan(pBridge, spanIdx, rmPierType);
   }
}

void CCastDeckActivity::RemoveSpan(const CBridgeDescription2* pBridge, SpanIndexType spanIdx, pgsTypes::RemovePierType rmPierType)
{
   if (m_CastingType == Continuous)
      return; // there aren't any casting regions for continuous casting... just return

   // Get the pier that is being removed with the span
   const CSpanData2* pSpan = pBridge->GetSpan(spanIdx);
   const CPierData2* pPrevPier = pSpan->GetPrevPier();
   const CPierData2* pNextPier = pSpan->GetNextPier();
   PierIndexType removePierIdx = (rmPierType == pgsTypes::PrevPier ? pPrevPier->GetIndex() : pNextPier->GetIndex());

   // remove all casting regions for the span and pier that are being removed
   // remove if, its a span region and index == spanIdx or if it is a pier region and index == removePierIdx
   m_vCastingRegions.erase(
      std::remove_if(std::begin(m_vCastingRegions), std::end(m_vCastingRegions), 
         [spanIdx, removePierIdx](auto& region) {return (region.m_Type == CCastingRegion::Span && region.m_Index == spanIdx) || 
                                                        (region.m_Type == CCastingRegion::Pier && region.m_Index == removePierIdx);}), 
      std::end(m_vCastingRegions));

   for (auto& region : m_vCastingRegions)
   {
      // for all spans and piers after the span and pier that were removed, reduce the index by one
      if ((region.m_Type == CCastingRegion::Span && spanIdx < region.m_Index) || (region.m_Type == CCastingRegion::Pier && removePierIdx < region.m_Index))
      {
         region.m_Index--;
      }
   }

   UpdateCastings();
}

void CCastDeckActivity::RemoveNegMomentRegion(PierIndexType pierIdx)
{
   m_vCastingRegions.erase(
      std::remove_if(std::begin(m_vCastingRegions), std::end(m_vCastingRegions), [pierIdx](auto& region) {return region.m_Type == CCastingRegion::Pier && region.m_Index == pierIdx;}),
      std::end(m_vCastingRegions)
   );
}

void CCastDeckActivity::AddNegMomentRegion(PierIndexType pierIdx)
{
   auto iter = std::begin(m_vCastingRegions);
   auto end = std::end(m_vCastingRegions);
   for (; iter != end; iter++)
   {
      auto& region(*iter);
      if (region.m_Type == CCastingRegion::Span && region.m_Index == pierIdx)
      {
         m_vCastingRegions.insert(iter, CCastingRegion(pierIdx, -0.10, -0.10, 0));
         break;
      }
   }
}

void CCastDeckActivity::SetClosureJointCastingRegion(IndexType regionIdx)
{
   m_ClosureJointCastingRegion = regionIdx;
}

IndexType CCastDeckActivity::GetClosureJointCastingRegion() const
{
   return m_ClosureJointCastingRegion;
}

Float64 CCastDeckActivity::GetTimeOfClosureJointCasting() const
{
   if (m_ClosureJointCastingRegion == INVALID_INDEX || m_CastingType == Continuous)
      return 0.0;

   // get information about the region that is cast at the same time as the closure joints
   const CCastingRegion& region = m_vCastingRegions[m_ClosureJointCastingRegion];

   // sequence numbers are not sequential, they could be 1, 3, 21, 99. Only the relatve order matters.
   // m_vCastingOrder has all the sequnce numbers in sorted order. search for the sequence number
   // for the region of interest
   auto found = std::find(std::cbegin(m_vCastingOrder), std::cend(m_vCastingOrder), region.m_SequenceIndex);
   ATLASSERT(found != m_vCastingOrder.end()); // if it isn't found, there is a bug somewhere
   auto idx = std::distance(std::cbegin(m_vCastingOrder), found); // get the index in m_vCastingOrder where the region of interest's sequence number is found
   Float64 time = idx*m_TimeBetweenCastings; // sum of the castimg times for all previous casting operations
   return time;
}

IndexType CCastDeckActivity::GetClosureJointCasting() const
{
   if (m_ClosureJointCastingRegion == INVALID_INDEX)
      return INVALID_INDEX; // there isn't a closure joint associated with this deck casting

   if (m_CastingType == Continuous)
   {
      // for continuous casting, there is only one casting
      return 0;
   }
   else
   {
      // check all the castings until we find the region
      IndexType nCastings = GetCastingCount();
      for (IndexType castingIdx = 0; castingIdx < nCastings; castingIdx++)
      {
         std::vector<IndexType> vRegions = GetRegions(castingIdx);
         if (std::find(std::begin(vRegions), std::end(vRegions), m_ClosureJointCastingRegion) != vRegions.end())
         {
            return castingIdx;
         }
      }
   }

   ATLASSERT(false); // should never get here
   return INVALID_INDEX;
}

void CCastDeckActivity::InsertSpan(const CBridgeDescription2* pBridge, SpanIndexType newSpanIdx, PierIndexType newPierIdx)
{
   if (m_CastingType == Continuous)
      return; // there aren't any casting regions for continuous casting... just return

   const CPierData2* pNewPier = pBridge->GetPier(newPierIdx);

   auto iter = std::begin(m_vCastingRegions);
   auto end = std::end(m_vCastingRegions);
   for (; iter != end; iter++)
   {
      auto& region(*iter);
      if (region.m_Type == CCastingRegion::Span && region.m_Index == newSpanIdx)
      {
         // the new span is inserted at this location
         break;
      }
   }

   // if this isn't the first region, check the previous region
   // if the previous region is a pier region it needs to stay
   // adjacent to the current span region so back the iteratator
   // up one position so the inserts that follow will move the pier
   // forward
   if (iter != std::begin(m_vCastingRegions))
   {
      auto& prev_region = *(iter - 1);
      if (prev_region.m_Type == CCastingRegion::Pier)
      {
         iter--;
      }
   }

   if (newSpanIdx == newPierIdx)
   {
      // new pier is before new span... see if we need to create a pier region
      if (IsContinuousBoundaryCondition(pNewPier->GetBoundaryConditionType()))
      {
         // the pier is continuous so we need to insert a pier region
         iter = m_vCastingRegions.insert(iter, CCastingRegion(newPierIdx, -0.10, -0.10, 0));
      }
   }

   // insert the new span region
   iter = m_vCastingRegions.insert(iter, CCastingRegion(newSpanIdx, 0));
   iter++; // one past where the new span was inserted

   // increment the span and pier indices for all the remaining regions
   end = std::end(m_vCastingRegions); // the length of the container has changed so we need the new end point
   for (; iter != end; iter++)
   {
      auto& region(*iter);
      region.m_Index++;
   }
}

HRESULT CCastDeckActivity::Load(IStructuredLoad* pStrLoad,IProgress* pProgress)
{
   CHRException hr;

   try
   {
      hr = pStrLoad->BeginUnit(_T("CastDeck"));

      Float64 version;
      pStrLoad->get_Version(&version);

      CComVariant var;
      var.vt = VT_BOOL;
      hr = pStrLoad->get_Property(_T("Enabled"), &var);
      m_bEnabled = (var.boolVal == VARIANT_TRUE ? true : false);

      if (m_bEnabled)
      {
         if (2 < version)
         {
            // added in version 3
            var.vt = VT_I4;
            hr = pStrLoad->get_Property(_T("CastingType"), &var);
            m_CastingType = (CastingType)var.lVal;
         }

         var.vt = VT_R8;
         hr = pStrLoad->get_Property(_T("AgeAtContinuity"), &var);
         m_TotalCuringDuration = var.dblVal;

         if (1 < version)
         {
            // added in version 2
            hr = pStrLoad->get_Property(_T("CuringDuration"), &var);
            m_ActiveCuringDuration = var.dblVal;
         }
         else
         {
            m_ActiveCuringDuration = m_TotalCuringDuration;
         }

         if (m_CastingType == Staged)
         {
            // added in version 3
            var.vt = VT_R8;
            hr = pStrLoad->get_Property(_T("TimeBetweenCasting"), &var);
            m_TimeBetweenCastings = var.dblVal;

            var.vt = VT_I4;
            hr = pStrLoad->get_Property(_T("CastingRegionBoundary"), &var);
            m_DeckCastingRegionBoundary = (pgsTypes::DeckCastingRegionBoundary)(var.iVal);
            
            hr = pStrLoad->BeginUnit(_T("CastingRegions"));
            m_vCastingRegions.clear();
            HRESULT _hr_ = S_OK;
            while (_hr_ == S_OK)
            {
               CCastingRegion region;
               _hr_ = region.Load(pStrLoad, pProgress);
               if (SUCCEEDED(_hr_))
               {
                  m_vCastingRegions.push_back(region);
               }
            }
            hr = pStrLoad->EndUnit(); // CastingRegions

            if (3 < version) // added in version 4
            {
               var.vt = VT_INDEX;
               hr = pStrLoad->get_Property(_T("ClosureJointCastingRegion"), &var);
               m_ClosureJointCastingRegion = VARIANT2INDEX(var);
            }
         }
      }

      pStrLoad->EndUnit();
   }
   catch (HRESULT)
   {
      ATLASSERT(false);
      THROW_LOAD(InvalidFileFormat, pStrLoad);
   };

   UpdateCastings();

   return S_OK;
}

HRESULT CCastDeckActivity::Save(IStructuredSave* pStrSave,IProgress* pProgress)
{
   pStrSave->BeginUnit(_T("CastDeck"), 4.0);
   pStrSave->put_Property(_T("Enabled"), CComVariant(m_bEnabled));
   if (m_bEnabled)
   {
      pStrSave->put_Property(_T("CastingType"), CComVariant(m_CastingType)); // added in version 3
      pStrSave->put_Property(_T("AgeAtContinuity"), CComVariant(m_TotalCuringDuration));
      pStrSave->put_Property(_T("CuringDuration"), CComVariant(m_ActiveCuringDuration)); // added in version 2

      if (m_CastingType == Staged)
      {
         // added in version 3
         pStrSave->put_Property(_T("TimeBetweenCasting"), CComVariant(m_TimeBetweenCastings));
         pStrSave->put_Property(_T("CastingRegionBoundary"), CComVariant(m_DeckCastingRegionBoundary));
         pStrSave->BeginUnit(_T("CastingRegions"), 1.0);
         for (auto& castingRegion : m_vCastingRegions)
         {
            castingRegion.Save(pStrSave, pProgress);
         }
         pStrSave->EndUnit(); // CastingRegions

         // added in version 4
         pStrSave->put_Property(_T("ClosureJointCastingRegion"), CComVariant(m_ClosureJointCastingRegion));
      }
   }
   pStrSave->EndUnit();

   return S_OK;
}

void CCastDeckActivity::UpdateCastings()
{
   m_vCastingOrder.clear();
   std::vector<CCastingRegion>* pvRegions = (m_CastingType == Continuous ? &m_vContinuousCastingRegions : &m_vCastingRegions);

   for (const auto& region : *pvRegions)
   {
      m_vCastingOrder.emplace_back(region.m_SequenceIndex);
   }
   std::sort(std::begin(m_vCastingOrder), std::end(m_vCastingOrder));
   m_vCastingOrder.erase(std::unique(std::begin(m_vCastingOrder), std::end(m_vCastingOrder)), std::end(m_vCastingOrder));
}