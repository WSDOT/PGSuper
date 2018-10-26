///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2016  Washington State Department of Transportation
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
#include <PgsExt\GirderGroupData.h>
#include <PgsExt\BridgeDescription2.h>
#include <PgsExt\PierData2.h>
#include <PgsExt\ClosureJointData.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CGirderGroupData::CGirderGroupData()
{
   m_pBridge                     = NULL;
   m_pPier[pgsTypes::metStart]   = NULL;
   m_pPier[pgsTypes::metEnd]     = NULL;

   m_PierIndex[pgsTypes::metStart] = INVALID_INDEX;
   m_PierIndex[pgsTypes::metEnd]   = INVALID_INDEX;
   m_GroupIdx                      = INVALID_INDEX;
   m_GroupID                       = INVALID_ID;
}

CGirderGroupData::CGirderGroupData(CBridgeDescription2* pBridge)
{
   m_pBridge      = pBridge;

   m_pPier[pgsTypes::metStart]   = NULL;
   m_pPier[pgsTypes::metEnd]     = NULL;

   m_PierIndex[pgsTypes::metStart] = INVALID_INDEX;
   m_PierIndex[pgsTypes::metEnd]   = INVALID_INDEX;
   m_GroupIdx                      = INVALID_INDEX;
   m_GroupID                       = INVALID_ID;
}

CGirderGroupData::CGirderGroupData(CPierData2* pStartPier,CPierData2* pEndPier)
{
   ATLASSERT(pStartPier->GetBridgeDescription() == pEndPier->GetBridgeDescription());
   ATLASSERT(pStartPier->GetIndex() != INVALID_INDEX);
   ATLASSERT(pEndPier->GetIndex()   != INVALID_INDEX);

   m_pBridge = pStartPier->GetBridgeDescription();

   m_PierIndex[pgsTypes::metStart] = INVALID_INDEX;
   m_PierIndex[pgsTypes::metEnd]   = INVALID_INDEX;

   m_pPier[pgsTypes::metStart]   = pStartPier;
   m_pPier[pgsTypes::metEnd]     = pEndPier;

   m_GroupIdx = INVALID_INDEX;
   m_GroupID  = INVALID_ID;
}

CGirderGroupData::CGirderGroupData(const CGirderGroupData& rOther)
{
   m_pBridge                     = NULL;
   m_pPier[pgsTypes::metStart]   = NULL;
   m_pPier[pgsTypes::metEnd]     = NULL;

   m_PierIndex[pgsTypes::metStart] = INVALID_INDEX;
   m_PierIndex[pgsTypes::metEnd]   = INVALID_INDEX;
   m_GroupIdx                      = INVALID_INDEX;
   m_GroupID                       = INVALID_ID;

   MakeCopy(rOther,true/*copy only data*/);
}

CGirderGroupData::~CGirderGroupData()
{
   m_pPier[pgsTypes::metStart]   = NULL;
   m_pPier[pgsTypes::metEnd]     = NULL;

   m_PierIndex[pgsTypes::metStart] = INVALID_INDEX;
   m_PierIndex[pgsTypes::metEnd]   = INVALID_INDEX;

   std::vector<CSplicedGirderData*>::iterator iter(m_Girders.begin());
   std::vector<CSplicedGirderData*>::iterator end(m_Girders.end());
   for ( ; iter != end; iter++ )
   {
      CSplicedGirderData* pGirder = *iter;
      delete pGirder;
   }

   m_Girders.clear();
}

CGirderGroupData& CGirderGroupData::operator=(const CGirderGroupData& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

void CGirderGroupData::CopyGirderGroupData(const CGirderGroupData* pGroup,bool bCopyDataOnly)
{
   MakeCopy(*pGroup,bCopyDataOnly);
}

bool CGirderGroupData::operator==(const CGirderGroupData& rOther) const
{
   if ( GetPierIndex(pgsTypes::metStart) != rOther.GetPierIndex(pgsTypes::metStart) )
   {
      return false;
   }

   if ( GetPierIndex(pgsTypes::metEnd) != rOther.GetPierIndex(pgsTypes::metEnd) )
   {
      return false;
   }

   if ( m_pBridge && !m_pBridge->UseSameNumberOfGirdersInAllGroups() )
   {
      if ( m_Girders.size() != rOther.m_Girders.size() )
      {
         return false;
      }
   }

   bool bCheckSlabOffset = false;
   if ( m_pBridge && (m_pBridge->GetSlabOffsetType() == pgsTypes::sotPier || m_pBridge->GetSlabOffsetType() == pgsTypes::sotGirder))
   {
      bCheckSlabOffset = true;
   }

   GirderIndexType nGirders = m_Girders.size();
   for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
   {
      if ( *m_Girders[gdrIdx] != *rOther.m_Girders[gdrIdx] )
      {
         return false;
      }

      if ( bCheckSlabOffset )
      {
         PierIndexType startPierIdx = GetPierIndex(pgsTypes::metStart);
         PierIndexType endPierIdx   = GetPierIndex(pgsTypes::metEnd);
         for ( PierIndexType pierIdx = startPierIdx; pierIdx <= endPierIdx; pierIdx++ )
         {
            if ( !IsEqual(GetSlabOffset(pierIdx,gdrIdx),rOther.GetSlabOffset(pierIdx,gdrIdx)) )
            {
               return false;
            }
         }
      }
   }

   return true;
}

bool CGirderGroupData::operator!=(const CGirderGroupData& rOther) const
{
   return !operator==(rOther);
}

void CGirderGroupData::SetIndex(GroupIndexType grpIdx)
{
   m_GroupIdx = grpIdx;
}

GroupIndexType CGirderGroupData::GetIndex() const
{
   return m_GroupIdx;
}

void CGirderGroupData::SetID(GroupIDType grpID)
{
   m_GroupID = grpID;
}

GroupIDType CGirderGroupData::GetID() const
{
   return m_GroupID;
}

void CGirderGroupData::SetBridgeDescription(CBridgeDescription2* pBridge)
{
   m_pBridge = pBridge;
   UpdatePiers();
}

CBridgeDescription2* CGirderGroupData::GetBridgeDescription()
{
   return m_pBridge;
}

const CBridgeDescription2* CGirderGroupData::GetBridgeDescription() const
{
   return m_pBridge;
}

void CGirderGroupData::SetPier(pgsTypes::MemberEndType end,CPierData2* pPier)
{
   ATLASSERT(pPier != NULL);

   CGirderSpacing2* pOldSpacing = NULL;
   pgsTypes::PierFaceType pierFace = (end == pgsTypes::metStart ? pgsTypes::Ahead : pgsTypes::Back);
   if ( m_pPier[end] && m_pPier[end]->GetGirderSpacing(pierFace)->GetSpacingCount() != pPier->GetGirderSpacing(pierFace)->GetSpacingCount() )
   {
      // There is a different number of girders framing into the new and old piers.
      // Hold a copy of the old pier's spacing data and then set it back to the old pier
      // after copying the new pier's data
      pOldSpacing = m_pPier[end]->GetGirderSpacing(pierFace);
   }

   m_pPier[end] = pPier;
   m_PierIndex[end] = INVALID_INDEX;

   if ( pOldSpacing )
   {
      m_pPier[end]->SetGirderSpacing(pierFace,*pOldSpacing);
   }

   m_pPier[end]->GetGirderSpacing(pierFace)->SetPier(pPier);

   // Update where the first/last segment starts/ends
   std::vector<CSplicedGirderData*>::iterator iter(m_Girders.begin());
   std::vector<CSplicedGirderData*>::iterator iterEnd(m_Girders.end());
   for ( ; iter != iterEnd; iter++ )
   {
      CSplicedGirderData* pGirder = *iter;
      if ( end == pgsTypes::metStart )
      {
         pGirder->GetSegment(0)->SetSpan(end,pPier->GetNextSpan());
      }
      else
      {
         SegmentIndexType nSegments = pGirder->GetSegmentCount();
         pGirder->GetSegment(nSegments-1)->SetSpan(end,pPier->GetPrevSpan());
      }
   }

#if defined _DEBUG
   if ( m_pBridge != NULL )
   {
      ATLASSERT(m_pBridge == pPier->GetBridgeDescription());
   }
#endif
   m_pBridge = pPier->GetBridgeDescription();
}

void CGirderGroupData::SetPiers(CPierData2* pStartPier,CPierData2* pEndPier)
{
   SetPier(pgsTypes::metStart,pStartPier);
   SetPier(pgsTypes::metEnd,  pEndPier);
}

void CGirderGroupData::AddSpan(PierIndexType refPierIdx,pgsTypes::PierFaceType face)
{
   UpdateSlabOffsets(refPierIdx);

   std::vector<CSplicedGirderData*>::iterator iter(m_Girders.begin());
   std::vector<CSplicedGirderData*>::iterator end(m_Girders.end());
   for ( ; iter != end; iter++ )
   {
      CSplicedGirderData* pGirder = *iter;
      pGirder->InsertSpan(refPierIdx,face);
   }
}

void CGirderGroupData::RemoveSpan(SpanIndexType spanIdx,pgsTypes::RemovePierType rmPierType)
{
   // Adjust the slab offsets
   PierIndexType pierIdx = (PierIndexType)spanIdx + (rmPierType == pgsTypes::PrevPier ? 0 : 1);
   PierIndexType startPierIdx = GetPierIndex(pgsTypes::metStart);
   PierIndexType nPiersToRemove = pierIdx - startPierIdx;
   m_SlabOffsets.erase(m_SlabOffsets.begin()+nPiersToRemove);

   // Adjust the girders in the group for the span that is removed
   // remove span references from the girders before the span is destroyed
   // Segments have pointers to the spans they start and end in
   std::vector<CSplicedGirderData*>::iterator iter(m_Girders.begin());
   std::vector<CSplicedGirderData*>::iterator end(m_Girders.end());
   for ( ; iter != end; iter++ )
   {
      CSplicedGirderData* pGirder = *iter;
      pGirder->RemoveSpan(spanIdx,rmPierType);
   }
}

CPierData2* CGirderGroupData::GetPier(pgsTypes::MemberEndType end)
{
   return m_pPier[end];
}

const CPierData2* CGirderGroupData::GetPier(pgsTypes::MemberEndType end) const
{
   return m_pPier[end];
}

CPierData2* CGirderGroupData::GetPier(PierIndexType pierIdx)
{
   PierIndexType startPierIdx = GetPierIndex(pgsTypes::metStart);
   if ( pierIdx < startPierIdx && GetPierIndex(pgsTypes::metEnd) < pierIdx )
   {
      return NULL; // pier isn't part of this girder group
   }

   CPierData2* pPier = m_pPier[pgsTypes::metStart];
   for ( PierIndexType pi = startPierIdx; pi < pierIdx; pi++ )
   {
      pPier = pPier->GetNextSpan()->GetNextPier();
   }

   return pPier;
}

const CPierData2* CGirderGroupData::GetPier(PierIndexType pierIdx) const
{
   PierIndexType startPierIdx = GetPierIndex(pgsTypes::metStart);
   if ( pierIdx < startPierIdx && GetPierIndex(pgsTypes::metEnd) < pierIdx )
   {
      return NULL; // pier isn't part of this girder group
   }

   const CPierData2* pPier = m_pPier[pgsTypes::metStart];
   for ( PierIndexType pi = startPierIdx; pi < pierIdx; pi++ )
   {
      pPier = pPier->GetNextSpan()->GetNextPier();
   }

   return pPier;
}

PierIndexType CGirderGroupData::GetPierIndex(pgsTypes::MemberEndType end) const
{
   if ( m_pPier[end] )
   {
      ATLASSERT(m_PierIndex[end] == INVALID_INDEX);
      return m_pPier[end]->GetIndex();
   }
   else
   {
      ATLASSERT(m_pPier[end] == NULL);
      return m_PierIndex[end];
   }
}

CGirderGroupData* CGirderGroupData::GetPrevGirderGroup()
{
   if ( m_pPier[pgsTypes::metStart] )
   {
      CSpanData2* pSpan = m_pPier[pgsTypes::metStart]->GetPrevSpan();
      CGirderGroupData* pGroup = m_pBridge->GetGirderGroup(pSpan);
      return pGroup;
   }

   return NULL;
}

const CGirderGroupData* CGirderGroupData::GetPrevGirderGroup() const
{
   if ( m_pPier[pgsTypes::metStart] )
   {
      const CSpanData2* pSpan = m_pPier[pgsTypes::metStart]->GetPrevSpan();
      const CGirderGroupData* pGroup = m_pBridge->GetGirderGroup(pSpan);
      return pGroup;
   }

   return NULL;
}

CGirderGroupData* CGirderGroupData::GetNextGirderGroup()
{
   if ( m_pPier[pgsTypes::metEnd] )
   {
      CSpanData2* pSpan = m_pPier[pgsTypes::metEnd]->GetNextSpan();
      CGirderGroupData* pGroup = m_pBridge->GetGirderGroup(pSpan);
      return pGroup;
   }

   return NULL;
}

const CGirderGroupData* CGirderGroupData::GetNextGirderGroup() const
{
   if ( m_pPier[pgsTypes::metEnd] )
   {
      const CSpanData2* pSpan = m_pPier[pgsTypes::metEnd]->GetNextSpan();
      const CGirderGroupData* pGroup = m_pBridge->GetGirderGroup(pSpan);
      return pGroup;
   }

   return NULL;
}

void CGirderGroupData::SetGirderCount(GirderIndexType nGirders)
{
   if ( m_Girders.size() == 0 && nGirders != 0 )
   {
      ATLASSERT(m_GirderTypeGroups.size() == 0);

      CSplicedGirderData* pGirder = new CSplicedGirderData;
      pGirder->SetIndex(0);
      pGirder->SetID( m_pBridge ? m_pBridge->GetNextGirderID() : INVALID_ID );
      pGirder->SetGirderGroup(this);
      m_Girders.push_back(pGirder);

      GirderTypeGroup group;
      group.first = 0;
      group.second = 0;
      m_GirderTypeGroups.push_back(group);

      std::vector<std::vector<Float64>>::iterator iter(m_SlabOffsets.begin());
      std::vector<std::vector<Float64>>::iterator end(m_SlabOffsets.end());
      for ( ; iter != end; iter++ )
      {
         std::vector<Float64>& vSlabOffset(*iter);
         vSlabOffset.push_back(::ConvertToSysUnits(10.0,unitMeasure::Inch));
      }
   }

   if ( nGirders < m_Girders.size() )
   {
      RemoveGirders(m_Girders.size()-nGirders);
   }
   else if ( m_Girders.size() < nGirders )
   {
      AddGirders(nGirders-m_Girders.size());
   }
   //else
   // do nothing if nGirders == m_Girders.size()

   ATLASSERT(nGirders == m_Girders.size());
   ASSERT_VALID;
}

void CGirderGroupData::Initialize(GirderIndexType nGirders)
{
   Clear();
   std::vector<CSplicedGirderData*>::iterator iter(m_Girders.begin());
   std::vector<CSplicedGirderData*>::iterator end(m_Girders.end());
   for ( ; iter != end; iter++ )
   {
      CSplicedGirderData* pGirder = *iter;
      delete pGirder;
   }

   m_Girders.clear();

   PierIndexType startPierIdx = GetPierIndex(pgsTypes::metStart);
   PierIndexType endPierIdx   = GetPierIndex(pgsTypes::metEnd);
   for ( PierIndexType pierIdx = startPierIdx; pierIdx <= endPierIdx; pierIdx++ )
   {
      m_SlabOffsets.push_back(std::vector<Float64>());
   }

   if ( nGirders == 0 )
   {
      return;
   }

   CGirderGroupData* pPrevGroup = GetPrevGirderGroup();
   CGirderGroupData* pNextGroup = GetNextGirderGroup();

   // create the new girders
   for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
   {
      CSplicedGirderData* pNewGirder = new CSplicedGirderData(this);
      pNewGirder->SetIndex(m_Girders.size());
      m_Girders.push_back(pNewGirder);

      pNewGirder->SetID( m_pBridge->GetNextGirderID() );
      GirderIDType newGdrID = pNewGirder->GetID();

      pNewGirder->Initialize(); // creates the default segment

      std::vector<std::vector<Float64>>::iterator iter(m_SlabOffsets.begin());
      std::vector<std::vector<Float64>>::iterator end(m_SlabOffsets.end());
      for ( ; iter != end; iter++ )
      {
         std::vector<Float64>& vSlabOffset(*iter);
         vSlabOffset.push_back( ::ConvertToSysUnits( 10.0, unitMeasure::Inch ) );
      }
   }

   m_GirderTypeGroups.push_back( GirderTypeGroup(0,m_Girders.size()-1) );
}

void CGirderGroupData::RemoveGirders(GirderIndexType nGirdersToRemove)
{
   ATLASSERT( nGirdersToRemove < (GirderIndexType)m_Girders.size() ); // removing more than the container holds

   std::vector<CSplicedGirderData*>::iterator iter( m_Girders.end() - nGirdersToRemove );
   std::vector<CSplicedGirderData*>::iterator end(  m_Girders.end() );
   for ( ; iter != end; iter++ )
   {
      CSplicedGirderData* pSplicedGirder = *iter;
      pSplicedGirder->Clear();
      delete pSplicedGirder;
      pSplicedGirder = NULL;

      m_GirderTypeGroups.back().second--;
      if ( m_GirderTypeGroups.back().second < m_GirderTypeGroups.back().first )
      {
         m_GirderTypeGroups.pop_back();
      }
   }

   GirderIndexType nGirders = m_Girders.size();
   std::vector<std::vector<Float64>>::iterator soIter(m_SlabOffsets.begin());
   std::vector<std::vector<Float64>>::iterator soIterEnd(m_SlabOffsets.end());
   for ( ; soIter != soIterEnd; soIter++ )
   {
      std::vector<Float64>& vSlabOffset(*soIter);
      vSlabOffset.resize(nGirders-nGirdersToRemove);
   }
   m_Girders.resize(nGirders-nGirdersToRemove);


   // Update the girder spacing (number of girders and number of spaces are related)
   m_pPier[pgsTypes::metStart]->GetGirderSpacing(pgsTypes::Ahead)->RemoveGirders(nGirdersToRemove);
   m_pPier[pgsTypes::metEnd  ]->GetGirderSpacing(pgsTypes::Back )->RemoveGirders(nGirdersToRemove);

   ASSERT_VALID;
}

void CGirderGroupData::AddGirders(GirderIndexType nGirdersToAdd)
{
   ATLASSERT(m_Girders.size() != 0);

   // Collect the construction and erection event information for each
   // segment from the reference girder. We'll use the same events for
   // the new girder and its segments.
   const CSplicedGirderData* pRefGirder = m_Girders.back();
   GirderIDType refGdrID = pRefGirder->GetID();

   SegmentIndexType nSegments = pRefGirder->GetSegmentCount();
   std::vector<EventIndexType> constructionEvents(nSegments);
   std::vector<EventIndexType> erectionEvents(nSegments);
   CTimelineManager* pTimelineManager = m_pBridge->GetTimelineManager();
   for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      const CPrecastSegmentData* pSegment = pRefGirder->GetSegment(segIdx);
      SegmentIDType segID = pSegment->GetID();
      ATLASSERT(segID != INVALID_ID);

      EventIndexType constructionEventIdx = pTimelineManager->GetSegmentConstructionEventIndex( segID );
      constructionEvents[segIdx] = constructionEventIdx;

      EventIndexType erectionEventIdx = pTimelineManager->GetSegmentErectionEventIndex( segID );
      erectionEvents[segIdx] = erectionEventIdx;
   }

   // Collect the post-tensioning event information for the reference girder.
   DuctIndexType nDucts = pRefGirder->GetPostTensioning()->GetDuctCount();
   std::vector<EventIndexType> ptEvents(nDucts);
   for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
   {
      ptEvents[ductIdx] = pTimelineManager->GetStressTendonEventIndex( refGdrID, ductIdx);
   }

   // create the new girders
   for ( GirderIndexType i = 0; i < nGirdersToAdd; i++ )
   {
      GirderIndexType gdrIdx = m_Girders.size();
      GirderIDType gdrID = m_pBridge->GetNextGirderID();
      CSplicedGirderData* pNewGirder = new CSplicedGirderData(this,gdrIdx,gdrID,*pRefGirder);
      m_Girders.push_back(pNewGirder);

      CGirderKey newGirderKey(pNewGirder->GetGirderKey());

      std::vector<std::vector<Float64>>::iterator soIter(m_SlabOffsets.begin());
      std::vector<std::vector<Float64>>::iterator soIterEnd(m_SlabOffsets.end());
      for ( ; soIter != soIterEnd; soIter++ )
      {
         std::vector<Float64>& vSlabOffset(*soIter);
         vSlabOffset.push_back(vSlabOffset.back());
      }

      // set the construction and erection event information for the segments of this new girder
      SegmentIndexType segIdx = 0;
      std::vector<EventIndexType>::iterator constructionEventIter(constructionEvents.begin());
      std::vector<EventIndexType>::iterator constructionEventIterEnd(constructionEvents.end());
      std::vector<EventIndexType>::iterator erectionEventIter(erectionEvents.begin());
      for ( ; constructionEventIter != constructionEventIterEnd; constructionEventIter++, erectionEventIter++, segIdx++)
      {
         CPrecastSegmentData* pNewSegment = pNewGirder->GetSegment(segIdx);
         SegmentIDType segID = pNewSegment->GetID();
         ATLASSERT(segID != INVALID_ID);

         pTimelineManager->SetSegmentConstructionEventByIndex( segID, *constructionEventIter );
         pTimelineManager->SetSegmentErectionEventByIndex( segID, *erectionEventIter );
      }

      // set the pt events
      for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
      {
         ATLASSERT(m_GroupID != INVALID_ID);
         pTimelineManager->SetStressTendonEventByIndex(gdrID,ductIdx,ptEvents[ductIdx]);
      }
   }

   // Update the girder type group
   m_GirderTypeGroups.back().second = m_Girders.size()-1;

   // Update the girder spacing (number of girders and number of spaces are related)
   CPierData2* pStartPier = m_pPier[pgsTypes::metStart];
   CPierData2* pEndPier   = m_pPier[pgsTypes::metEnd];
   PierIndexType endPierIdx = pEndPier->GetIndex();
   for ( CPierData2* pPier = pStartPier; pPier != NULL && pPier->GetIndex() <= endPierIdx;  )
   {
      if ( pPier == pStartPier )
      {
         pPier->GetGirderSpacing(pgsTypes::Ahead)->AddGirders(nGirdersToAdd);
      }
      else if ( pPier == pEndPier )
      {
         pPier->GetGirderSpacing(pgsTypes::Back)->AddGirders(nGirdersToAdd);
      }
      else
      {
         pPier->GetGirderSpacing(pgsTypes::Ahead)->AddGirders(nGirdersToAdd);
         pPier->GetGirderSpacing(pgsTypes::Back)->AddGirders(nGirdersToAdd);
      }

      if ( pPier->GetNextSpan() )
      {
         pPier = pPier->GetNextSpan()->GetNextPier();
      }
      else
      {
         pPier = NULL;
      }
   }
}

GirderIndexType CGirderGroupData::GetGirderCount() const
{
   if ( m_pBridge && m_pBridge->UseSameNumberOfGirdersInAllGroups() )
   {
      return m_pBridge->GetGirderCount();
   }
   else
   {
      return (GirderIndexType)m_Girders.size();
   }
}

void CGirderGroupData::SetGirder(GirderIndexType gdrIdx,CSplicedGirderData* pGirderData)
{
   m_Girders[gdrIdx] = pGirderData;
}

CSplicedGirderData* CGirderGroupData::GetGirder(GirderIndexType gdrIdx)
{
   if ( gdrIdx < 0 || m_Girders.size() <= gdrIdx )
   {
      return NULL;
   }

   return m_Girders[gdrIdx];
}

const CSplicedGirderData* CGirderGroupData::GetGirder(GirderIndexType gdrIdx) const
{
   if ( gdrIdx < 0 || m_Girders.size() <= gdrIdx )
   {
      return NULL;
   }

   return m_Girders[gdrIdx];
}

void CGirderGroupData::SetSlabOffset(PierIndexType pierIdx,Float64 offset)
{
   ATLASSERT(GetPierIndex(pgsTypes::metStart) <= pierIdx && pierIdx <= GetPierIndex(pgsTypes::metEnd));

   // set the slab offset for all girders at this pier
   IndexType idx = pierIdx - GetPierIndex(pgsTypes::metStart);
   std::vector<Float64>& vSlabOffsets(m_SlabOffsets[idx]);
   std::vector<Float64>::iterator iter(vSlabOffsets.begin());
   std::vector<Float64>::iterator iterEnd(vSlabOffsets.end());
   for ( ; iter != iterEnd; iter++ )
   {
      *iter = offset;
   }
}

void CGirderGroupData::SetSlabOffset(PierIndexType pierIdx,GirderIndexType gdrIdx,Float64 offset)
{
   ATLASSERT(GetPierIndex(pgsTypes::metStart) <= pierIdx && pierIdx <= GetPierIndex(pgsTypes::metEnd));

   // set the slab offset for a specific girders at this pier
   IndexType idx = pierIdx - GetPierIndex(pgsTypes::metStart);
   std::vector<Float64>& vSlabOffsets(m_SlabOffsets[idx]);
   ATLASSERT(gdrIdx < vSlabOffsets.size());
   vSlabOffsets[gdrIdx] = offset;
}

Float64 CGirderGroupData::GetSlabOffset(PierIndexType pierIdx,GirderIndexType gdrIdx,bool bGetRawValue) const
{
   Float64 offset;
   if ( bGetRawValue )
   {
      ATLASSERT(GetPierIndex(pgsTypes::metStart) <= pierIdx && pierIdx <= GetPierIndex(pgsTypes::metEnd));

      // set the slab offset for a specific girders at this pier
      IndexType idx = pierIdx - GetPierIndex(pgsTypes::metStart);
      const std::vector<Float64>& vSlabOffsets(m_SlabOffsets[idx]);

      ATLASSERT(gdrIdx < vSlabOffsets.size());
      offset = vSlabOffsets[gdrIdx];
   }
   else
   {
      pgsTypes::SlabOffsetType slabOffsetType = m_pBridge->GetSlabOffsetType();
      if ( slabOffsetType == pgsTypes::sotBridge )
      {
         offset = m_pBridge->GetSlabOffset();
      }
      else if ( slabOffsetType == pgsTypes::sotPier )
      {
         if ( m_pBridge->GetDeckDescription()->DeckType == pgsTypes::sdtNone )
         {
            return 0;
         }

         ATLASSERT(GetPierIndex(pgsTypes::metStart) <= pierIdx && pierIdx <= GetPierIndex(pgsTypes::metEnd));

         // set the slab offset for a specific girders at this pier
         IndexType idx = pierIdx - GetPierIndex(pgsTypes::metStart);
         const std::vector<Float64>& vSlabOffsets(m_SlabOffsets[idx]);

         // slab offsets should be the same across the pier so just use the one for the first girder
         offset = vSlabOffsets.front();
      }
      else
      {
         if ( m_pBridge->GetDeckDescription()->DeckType == pgsTypes::sdtNone )
         {
            return 0;
         }

         ATLASSERT(GetPierIndex(pgsTypes::metStart) <= pierIdx && pierIdx <= GetPierIndex(pgsTypes::metEnd));

         // set the slab offset for a specific girders at this pier
         IndexType idx = pierIdx - GetPierIndex(pgsTypes::metStart);
         const std::vector<Float64>& vSlabOffsets(m_SlabOffsets[idx]);

         ATLASSERT(gdrIdx < vSlabOffsets.size());
         offset = vSlabOffsets[gdrIdx];
      }
   }

   return offset;
}

void CGirderGroupData::CopySlabOffset(GirderIndexType sourceGdrIdx,GirderIndexType targetGdrIdx)
{
   PierIndexType startPierIdx = GetPierIndex(pgsTypes::metStart);
   PierIndexType endPierIdx   = GetPierIndex(pgsTypes::metEnd);
   for ( PierIndexType pierIdx = startPierIdx; pierIdx <= endPierIdx; pierIdx++ )
   {
      IndexType idx = pierIdx - startPierIdx;
      std::vector<Float64>& vSlabOffsets(m_SlabOffsets[idx]);
      vSlabOffsets[targetGdrIdx] = vSlabOffsets[sourceGdrIdx];
   }
}

GroupIndexType CGirderGroupData::CreateGirderTypeGroup(GirderIndexType firstGdrIdx,GirderIndexType lastGdrIdx)
{
   if ( m_pBridge->UseSameGirderForEntireBridge() )
   {
      return 0; // can't create a new group... return group 0
   }

   GroupIndexType newGroupIdx = 9999;

   std::vector<GirderTypeGroup> gdrGroups;

   std::vector<GirderTypeGroup>::const_iterator iter;
   for ( iter = m_GirderTypeGroups.begin(); iter != m_GirderTypeGroups.end(); iter++ )
   {
      GirderTypeGroup gdrGroup = *iter;
      if ( gdrGroup.first == firstGdrIdx && gdrGroup.second == lastGdrIdx )
      {
         // no need to create a new group
         newGroupIdx = iter - m_GirderTypeGroups.begin();
         return newGroupIdx;
      }

      if ( gdrGroup.first < firstGdrIdx && firstGdrIdx <= gdrGroup.second )
      {
         // the new group starts in the middle of this group
         gdrGroup.second = firstGdrIdx-1; // set the end of this group one girder before the 
         gdrGroups.push_back(gdrGroup);
         break;
      }
      else if ( lastGdrIdx <= gdrGroup.second )
      {
         // the new group is totally within this group
         if ( gdrGroup.first == firstGdrIdx )
         {
            GirderTypeGroup newGrp;
            newGrp.first  = firstGdrIdx;
            newGrp.second = lastGdrIdx;
            gdrGroups.push_back(newGrp);
            newGroupIdx = gdrGroups.size()-1;

            newGrp.first = lastGdrIdx+1;
            newGrp.second = gdrGroup.second;
            gdrGroups.push_back(newGrp);
         }
         else if ( gdrGroup.second == lastGdrIdx )
         {
            GirderTypeGroup newGrp;
            newGrp.first  = gdrGroup.first;
            newGrp.second = firstGdrIdx-1;
            gdrGroups.push_back(newGrp);

            newGrp.first = firstGdrIdx;
            newGrp.second = lastGdrIdx;
            gdrGroups.push_back(newGrp);
            newGroupIdx = gdrGroups.size()-1;
         }
         else
         {
            GirderTypeGroup newGrp;
            newGrp.first  = gdrGroup.first;
            newGrp.second = firstGdrIdx-1;
            gdrGroups.push_back(newGrp);

            newGrp.first = firstGdrIdx;
            newGrp.second = lastGdrIdx;
            gdrGroups.push_back(newGrp);
            newGroupIdx = gdrGroups.size()-1;

            newGrp.first = lastGdrIdx+1;
            newGrp.second = gdrGroup.second;
            gdrGroups.push_back(newGrp);
         }
      }
      else
      {
         gdrGroups.push_back(gdrGroup);
      }
   }

   for ( iter; iter != m_GirderTypeGroups.end(); iter++ )
   {
      GirderTypeGroup gdrGroup = *iter;
      if ( gdrGroup.first < lastGdrIdx && lastGdrIdx <= gdrGroup.second )
      {
         // the new group ends in the middle of this group

         // add the new group
         gdrGroup.first = firstGdrIdx;
         gdrGroup.second = lastGdrIdx;
         gdrGroups.push_back(gdrGroup);
         newGroupIdx = gdrGroups.size()-1;

         // get the current group back
         gdrGroup = *iter;
         gdrGroup.first = lastGdrIdx + 1; // group begins one girder after the last group
         if ( gdrGroup.first <= gdrGroup.second ) 
         {
            gdrGroups.push_back(gdrGroup); // if last is < first, then this isn't a group... the new group ends at the last girder
         }
      }
      else
      {
         gdrGroups.push_back(gdrGroup); // save the rest of the groups
      }
   }

   m_GirderTypeGroups = gdrGroups;

   ASSERT_VALID;

   return newGroupIdx;
}

void CGirderGroupData::ExpandAll()
{
   m_GirderTypeGroups.clear();
   GirderIndexType nGirders = m_Girders.size();
   for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
   {
      m_GirderTypeGroups.push_back(std::make_pair(gdrIdx,gdrIdx));
   }
   ASSERT_VALID;
}

void CGirderGroupData::Expand(GroupIndexType girderTypeGroupIdx)
{
   GirderIndexType firstGdrIdx, lastGdrIdx;
   std::_tstring strName;
   GetGirderTypeGroup(girderTypeGroupIdx,&firstGdrIdx,&lastGdrIdx,&strName);

   if ( firstGdrIdx == lastGdrIdx )
   {
      return; // nothing to expand
   }

   std::vector<GirderTypeGroup>::iterator iter = m_GirderTypeGroups.begin() + girderTypeGroupIdx;
   std::vector<GirderTypeGroup>::iterator pos = m_GirderTypeGroups.erase(iter); // returns the iter the element after the one removed

   // inserted element goes before the iterator... insert retuns position of newly inserted item
   // go in reverse order
   for ( GirderIndexType gdrIdx = lastGdrIdx; firstGdrIdx <= gdrIdx; gdrIdx-- )
   {
      GirderTypeGroup group(gdrIdx,gdrIdx);
      pos = m_GirderTypeGroups.insert(pos,group);
   }
   ASSERT_VALID;
}


void CGirderGroupData::JoinAll(GirderIndexType gdrIdx)
{
   if ( m_Girders.size() == 0 )
   {
      return;
   }

   std::_tstring strName = m_Girders[gdrIdx]->GetGirderName();
   const GirderLibraryEntry* pGdrEntry = m_Girders[gdrIdx]->GetGirderLibraryEntry();

   GirderTypeGroup firstGroup = m_GirderTypeGroups.front();
   GirderTypeGroup lastGroup  = m_GirderTypeGroups.back();

   GirderTypeGroup joinedGroup;
   joinedGroup.first  = firstGroup.first;
   joinedGroup.second = lastGroup.second;

   m_GirderTypeGroups.clear();
   m_GirderTypeGroups.push_back(joinedGroup);

   // make girder name and lib entry the same for all members of the group
   std::vector<CSplicedGirderData*>::iterator iter(m_Girders.begin());
   std::vector<CSplicedGirderData*>::iterator iterEnd(m_Girders.end());
   for ( ; iter != iterEnd; iter++ )
   {
      CSplicedGirderData* pGirder = *iter;
      pGirder->SetGirderName(strName.c_str());
      pGirder->SetGirderLibraryEntry(pGdrEntry);
   }
   ASSERT_VALID;
}

void CGirderGroupData::Join(GirderIndexType firstGdrIdx,GirderIndexType lastGdrIdx,GirderIndexType gdrIdx)
{
   // girder index must be in the range
   _ASSERT( firstGdrIdx <= gdrIdx && gdrIdx <= lastGdrIdx );

   // get the girder name for the group
   std::_tstring strName = m_Girders[gdrIdx]->GetGirderName();
   const GirderLibraryEntry* pGdrEntry = m_Girders[gdrIdx]->GetGirderLibraryEntry();

   // assign the name for the group
   for ( GirderIndexType i = firstGdrIdx; i <= lastGdrIdx; i++ )
   {
      m_Girders[i]->SetGirderName(strName.c_str());
      m_Girders[i]->SetGirderLibraryEntry(pGdrEntry);
   }

   // firstGdrIdx must match the "first" parameter of a GirderTypeGroup
   // lastGdrIdx  must match the "last"  parameter of a GirderTypeGroup
   // this is the way the UI grid works so we don't need to allow
   // for any more complicated joining than this.

   // create a local girder groups container. It is easier to fill it up as we go
   // rather than manipulating the class data member... update the class data member
   // at the end of this function
   std::vector<GirderTypeGroup> gdrGroups;

   // loop until the first index in a group matches firstGdrIdx
   // save any group that comes before the first group
   std::vector<GirderTypeGroup>::iterator iter;
   for ( iter = m_GirderTypeGroups.begin(); iter != m_GirderTypeGroups.end(); iter++ )
   {
      GirderTypeGroup gdrGroup = *iter;
      if ( gdrGroup.first == firstGdrIdx )
      {
         break;
      }
      else
      {
         gdrGroups.push_back(gdrGroup);
      }
   }

   _ASSERT(iter != m_GirderTypeGroups.end()); // shouldn't have gone through the full vector

   // loop until the last index in a group matches lastGdrIdx
   // don't save any groups in the middle... these are the groups
   // being joined
   for ( ; iter != m_GirderTypeGroups.end(); iter++ )
   {
      GirderTypeGroup gdrGroup = *iter;
      if ( gdrGroup.second == lastGdrIdx )
      {
         iter++; // move iter to next position... breaking out of the loop skips the incrementer
         break;
      }
   }

   // save the new group
   GirderTypeGroup newGroup(firstGdrIdx,lastGdrIdx);
   gdrGroups.push_back(newGroup);

   // copy the remaining groups
   if ( iter != m_GirderTypeGroups.end() )
   {
      gdrGroups.insert(gdrGroups.end(),iter,m_GirderTypeGroups.end());
   }

   // finally replace the data member with the local girder groups
   m_GirderTypeGroups = gdrGroups;
   ASSERT_VALID;
}

GroupIndexType CGirderGroupData::GetGirderTypeGroupCount() const
{
   if ( m_pBridge && m_pBridge->UseSameGirderForEntireBridge() )
   {
      return 1;
   }
   else
   {
      return m_GirderTypeGroups.size();
   }
}

void CGirderGroupData::GetGirderTypeGroup(GroupIndexType girderTypeGroupIdx,GirderIndexType* pFirstGdrIdx,GirderIndexType* pLastGdrIdx,std::_tstring* pName) const
{
   if ( m_pBridge && m_pBridge->UseSameNumberOfGirdersInAllGroups() &&
        m_pBridge->UseSameGirderForEntireBridge() )
   {
      *pFirstGdrIdx = 0;
      *pLastGdrIdx = m_pBridge->GetGirderCount()-1;
      *pName = m_pBridge->GetGirderName();
   }
   else
   {
      _ASSERT( girderTypeGroupIdx < (SpacingIndexType)m_GirderTypeGroups.size() );
      GirderTypeGroup group = m_GirderTypeGroups[girderTypeGroupIdx];

      *pFirstGdrIdx = group.first;
      *pLastGdrIdx  = group.second;

      *pName = m_Girders[group.first]->GetGirderName();

#if defined _DEBUG
      // make sure every girder in the group has the same name
      for ( GirderIndexType i = group.first + 1; i <= group.second; i++ )
      {
         _ASSERT(*pName == m_Girders[i]->GetGirderName());
      }
#endif
   }
}

GroupIndexType CGirderGroupData::FindGroup(GirderIndexType gdrIdx) const
{
   if ( m_pBridge && m_pBridge->UseSameGirderForEntireBridge() )
   {
      return 0; // can't create a new group... return group 0
   }

   GroupIndexType nGroups = GetGirderTypeGroupCount();
   for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
   {
      GirderIndexType firstGdrIdx, lastGdrIdx;

      GirderTypeGroup group = m_GirderTypeGroups[grpIdx];

      firstGdrIdx = group.first;
      lastGdrIdx  = group.second;

      if ( firstGdrIdx <= gdrIdx && gdrIdx <= lastGdrIdx )
      {
         return grpIdx;
      }
   }

   ATLASSERT(false); // should never get here
   return INVALID_INDEX;
}

void CGirderGroupData::SetGirderName(GroupIndexType grpIdx,LPCTSTR strName)
{
   std::_tstring strGirder;
   GirderIndexType firstGdrIdx, lastGdrIdx;
   GetGirderTypeGroup(grpIdx,&firstGdrIdx,&lastGdrIdx,&strGirder);

   for ( GirderIndexType gdrIdx = firstGdrIdx; gdrIdx <= lastGdrIdx; gdrIdx++ )
   {
      m_Girders[gdrIdx]->SetGirderName(strName);
   }
}

void CGirderGroupData::RenameGirder(GroupIndexType grpIdx,LPCTSTR strName)
{
   std::_tstring strGirder;
   GirderIndexType firstGdrIdx, lastGdrIdx;
   GetGirderTypeGroup(grpIdx,&firstGdrIdx,&lastGdrIdx,&strGirder);

   for ( GirderIndexType gdrIdx = firstGdrIdx; gdrIdx <= lastGdrIdx; gdrIdx++ )
   {
      m_Girders[gdrIdx]->m_GirderType = strName;
   }
}

LPCTSTR CGirderGroupData::GetGirderName(GirderIndexType gdrIdx) const
{
   if ( m_pBridge && m_pBridge->UseSameGirderForEntireBridge() )
   {
      return m_pBridge->GetGirderName();
   }
   else
   {
      return m_Girders[gdrIdx]->GetGirderName();
   }
}

void CGirderGroupData::SetGirderLibraryEntry(GirderIndexType gdrIdx,const GirderLibraryEntry* pEntry)
{
   m_Girders[gdrIdx]->SetGirderLibraryEntry(pEntry);
}

const GirderLibraryEntry* CGirderGroupData::GetGirderLibraryEntry(GirderIndexType gdrIdx) const
{
   return m_Girders[gdrIdx]->GetGirderLibraryEntry();
}

bool CGirderGroupData::IsExteriorGirder(GirderIndexType gdrIdx) const
{
   return (gdrIdx == 0 || gdrIdx == m_Girders.size()-1 ? true : false);
}

bool CGirderGroupData::IsInteriorGirder(GirderIndexType gdrIdx) const
{
   return !IsExteriorGirder(gdrIdx);
}

PierIndexType CGirderGroupData::GetPierCount() const
{
   return GetPierIndex(pgsTypes::metEnd) - GetPierIndex(pgsTypes::metStart) + 1;
}

SpanIndexType CGirderGroupData::GetSpanCount() const
{
   return GetPierCount()-1;
}

Float64 CGirderGroupData::GetLength() const
{
   const CPierData2* pStartPier = GetPier(pgsTypes::metStart);
   const CPierData2* pEndPier   = GetPier(pgsTypes::metEnd);

   return pEndPier->GetStation() - pStartPier->GetStation();
}

HRESULT CGirderGroupData::Load(IStructuredLoad* pStrLoad,IProgress* pProgress)
{
   CComVariant var;

   CHRException hr;
   try
   {
      hr = pStrLoad->BeginUnit(_T("GirderGroup"));

      var.vt = VT_ID;
      hr = pStrLoad->get_Property(_T("ID"),&var);
      m_GroupID = VARIANT2ID(var);

      var.vt = VT_INDEX;
      hr = pStrLoad->get_Property(_T("StartPier"),&var);
      m_PierIndex[pgsTypes::metStart] = VARIANT2INDEX(var);

      hr = pStrLoad->get_Property(_T("EndPier"),&var);
      m_PierIndex[pgsTypes::metEnd] = VARIANT2INDEX(var);

      UpdatePiers();

      PierIndexType startPierIdx = GetPierIndex(pgsTypes::metStart);
      PierIndexType endPierIdx   = GetPierIndex(pgsTypes::metEnd);
      ATLASSERT(m_SlabOffsets.size() == 0);
      for ( PierIndexType pierIdx = startPierIdx; pierIdx <= endPierIdx; pierIdx++ )
      {
         m_SlabOffsets.push_back(std::vector<Float64>());
      }

      hr = pStrLoad->BeginUnit(_T("Girders"));

      var.vt = VT_INDEX;
      hr = pStrLoad->get_Property(_T("Count"),&var);
      GirderIndexType nGirders = VARIANT2INDEX(var);

      for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
      {
         CSplicedGirderData* pGirder = new CSplicedGirderData;
         pGirder->SetIndex(gdrIdx);
         pGirder->SetGirderGroup(this);
         hr = pGirder->Load(pStrLoad,pProgress);
         m_Girders.push_back(pGirder);
         ATLASSERT(pGirder->GetID() != INVALID_ID);
         m_pBridge->UpdateNextGirderID(pGirder->GetID());
      }

      hr = pStrLoad->EndUnit(); // Girders

      hr = pStrLoad->BeginUnit(_T("GirderTypeGroups"));
      var.vt = VT_INDEX;
      hr = pStrLoad->get_Property(_T("Count"),&var);
      GroupIndexType nGirderTypeGroups = VARIANT2INDEX(var);
      for ( GroupIndexType gdrTypeGroupIdx = 0; gdrTypeGroupIdx < nGirderTypeGroups; gdrTypeGroupIdx++ )
      {
         GirderTypeGroup group;

         hr = pStrLoad->BeginUnit(_T("GirderTypeGroup"));
         
         hr = pStrLoad->get_Property(_T("FirstGirderIndex"),&var);
         group.first = VARIANT2INDEX(var);

         hr = pStrLoad->get_Property(_T("LastGirderIndex"),&var);
         group.second = VARIANT2INDEX(var);

         m_GirderTypeGroups.push_back(group);

         hr = pStrLoad->EndUnit(); // GirderTypeGroup
      }
      hr = pStrLoad->EndUnit(); // GirderTypeGroups

      if ( ::IsBridgeSpacing(m_pBridge->GetGirderSpacingType()) )
      {
         // If the girder spacing type is a bridge spacing then individual girder spaces aren't loaded by
         // the CPierData2 objects. The girder spacing objects need to have spacing that is consistent
         // with the number of girders. Set those values here.
         Float64 bridgeSpacing = m_pBridge->GetGirderSpacing();
         CPierData2* pStartPier = m_pPier[pgsTypes::metStart];
         CPierData2* pEndPier   = m_pPier[pgsTypes::metEnd];
         CPierData2* pPier = pStartPier;
         PierIndexType startPierIdx = pStartPier->GetIndex();
         PierIndexType endPierIdx   = pEndPier->GetIndex();
         for ( PierIndexType pierIdx = startPierIdx; pierIdx <= endPierIdx; pierIdx++ )
         {
            if ( pPier == pStartPier )
            {
               pPier->GetGirderSpacing(pgsTypes::Ahead)->InitGirderCount(nGirders);
               pPier->GetGirderSpacing(pgsTypes::Ahead)->SetGirderSpacing(0,bridgeSpacing);
            }
            else if ( pPier == pEndPier )
            {
               pPier->GetGirderSpacing(pgsTypes::Back)->InitGirderCount(nGirders);
               pPier->GetGirderSpacing(pgsTypes::Back)->SetGirderSpacing(0,bridgeSpacing);
            }
            else
            {
               pPier->GetGirderSpacing(pgsTypes::Ahead)->InitGirderCount(nGirders);
               pPier->GetGirderSpacing(pgsTypes::Ahead)->SetGirderSpacing(0,bridgeSpacing);
               pPier->GetGirderSpacing(pgsTypes::Back )->InitGirderCount(nGirders);
               pPier->GetGirderSpacing(pgsTypes::Back )->SetGirderSpacing(0,bridgeSpacing);
            }

            CSpanData2* pSpan = pPier->GetNextSpan();
            if ( pSpan )
            {
               pPier = pSpan->GetNextPier();
            }
            else
            {
               ATLASSERT(pPier->GetIndex() == endPierIdx); // if there isn't next span, we better be at the last pier
            }
         }
      }
#if defined _DEBUG
      else
      {
         // Girder spacing was loaded by CPierData2 objects. Make sure the number of girder spaces is consistent
         // with the number of girders
         ATLASSERT(m_pPier[pgsTypes::metStart]->GetGirderSpacing(pgsTypes::Ahead)->GetSpacingCount()+1 == m_Girders.size());
         ATLASSERT(m_pPier[pgsTypes::metEnd  ]->GetGirderSpacing(pgsTypes::Back )->GetSpacingCount()+1 == m_Girders.size());


         CPierData2* pStartPier = m_pPier[pgsTypes::metStart];
         CPierData2* pEndPier   = m_pPier[pgsTypes::metEnd];
         CPierData2* pPier = pStartPier;
         PierIndexType startPierIdx = pStartPier->GetIndex();
         PierIndexType endPierIdx   = pEndPier->GetIndex();
         for ( PierIndexType pierIdx = startPierIdx; pierIdx <= endPierIdx; pierIdx++ )
         {
            if ( pPier == pStartPier )
            {
               ATLASSERT(pPier->GetGirderSpacing(pgsTypes::Ahead)->GetSpacingCount()+1 == m_Girders.size());
            }
            else if ( pPier == pEndPier )
            {
               ATLASSERT(pPier->GetGirderSpacing(pgsTypes::Back)->GetSpacingCount()+1 == m_Girders.size());
            }
            else
            {
               ATLASSERT(pPier->GetGirderSpacing(pgsTypes::Ahead)->GetSpacingCount()+1 == m_Girders.size());
               ATLASSERT(pPier->GetGirderSpacing(pgsTypes::Back)->GetSpacingCount()+1 == m_Girders.size());
            }

            CSpanData2* pSpan = pPier->GetNextSpan();
            if ( pSpan )
            {
               pPier = pSpan->GetNextPier();
            }
            else
            {
               ATLASSERT(pPier->GetIndex() == endPierIdx); // if there isn't next span, we better be at the last pier
            }
         }
      }
#endif


      // Slab Offset
      if (m_pBridge->GetSlabOffsetType() != pgsTypes::sotBridge )
      {
         hr = pStrLoad->BeginUnit(_T("SlabOffset"));
         if ( m_pBridge->GetSlabOffsetType() == pgsTypes::sotPier )
         {
            // Slab offset per pier (one value for all girders)
            PierIndexType startPierIdx = GetPierIndex(pgsTypes::metStart);
            PierIndexType endPierIdx   = GetPierIndex(pgsTypes::metEnd);
            IndexType index = 0;
            for ( PierIndexType pierIdx = startPierIdx; pierIdx <= endPierIdx; pierIdx++, index++ )
            {
               std::vector<Float64>& vSlabOffset(m_SlabOffsets[index]);
               ATLASSERT(vSlabOffset.size() == 0);

               var.vt = VT_R8;
               hr = pStrLoad->get_Property(_T("SlabOffset"),&var);
               // Insert for all girders so our data structure is sized properly
               vSlabOffset.insert(vSlabOffset.begin(),nGirders,var.dblVal);
            }
         }
         else
         {
            ATLASSERT(m_pBridge->GetSlabOffsetType() == pgsTypes::sotGirder);
            PierIndexType startPierIdx = GetPierIndex(pgsTypes::metStart);
            PierIndexType endPierIdx   = GetPierIndex(pgsTypes::metEnd);
            IndexType index = 0;
            for ( PierIndexType pierIdx = startPierIdx; pierIdx <= endPierIdx; pierIdx++, index++ )
            {
               hr = pStrLoad->BeginUnit(_T("GirderSlabOffsets"));

               std::vector<Float64>& vSlabOffset(m_SlabOffsets[index]);
               ATLASSERT(vSlabOffset.size() == 0);
               for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
               {
                  var.vt = VT_R8;
                  hr = pStrLoad->get_Property(_T("SlabOffset"),&var);
                  vSlabOffset.push_back(var.dblVal);
               }

               hr = pStrLoad->EndUnit();
            }
         }
         hr = pStrLoad->EndUnit(); // SlabOffset
      }
      else
      {
         // make sure slab offset data structures are sized correctly
         std::vector<std::vector<Float64>>::iterator iter(m_SlabOffsets.begin());
         std::vector<std::vector<Float64>>::iterator end(m_SlabOffsets.end());
         for ( ; iter != end; iter++ )
         {
            std::vector<Float64>& vSlabOffsets(*iter);
            vSlabOffsets.resize(nGirders);
         }
      }

      hr = pStrLoad->EndUnit(); // GirderGroup
   }
   catch (HRESULT)
   {
      ATLASSERT(false);
      THROW_LOAD(InvalidFileFormat,pStrLoad);
   }

   return S_OK;
}

HRESULT CGirderGroupData::Save(IStructuredSave* pStrSave,IProgress* pProgress)
{
   pStrSave->BeginUnit(_T("GirderGroup"),1.0);

   pStrSave->put_Property(_T("ID"), CComVariant(m_GroupID) );

   pStrSave->put_Property(_T("StartPier"),CComVariant(GetPierIndex(pgsTypes::metStart)));
   pStrSave->put_Property(_T("EndPier"),CComVariant(GetPierIndex(pgsTypes::metEnd)));

   pStrSave->BeginUnit(_T("Girders"),1.0);
   pStrSave->put_Property(_T("Count"),CComVariant(m_Girders.size()));

   std::vector<CSplicedGirderData*>::iterator gdrIter(m_Girders.begin());
   std::vector<CSplicedGirderData*>::iterator gdrIterEnd(m_Girders.end());
   for ( ; gdrIter != gdrIterEnd; gdrIter++ )
   {
      CSplicedGirderData* pGirder = *gdrIter;
      pGirder->Save(pStrSave,pProgress);
   }
   pStrSave->EndUnit(); // Girders

   pStrSave->BeginUnit(_T("GirderTypeGroups"),1.0);
   pStrSave->put_Property(_T("Count"),CComVariant(m_GirderTypeGroups.size()));
   std::vector<GirderTypeGroup>::iterator grpIter(m_GirderTypeGroups.begin());
   std::vector<GirderTypeGroup>::iterator grpIterEnd(m_GirderTypeGroups.end());
   for ( ; grpIter != grpIterEnd; grpIter++ )
   {
      GirderTypeGroup group = *grpIter;
      pStrSave->BeginUnit(_T("GirderTypeGroup"),1.0);
      pStrSave->put_Property(_T("FirstGirderIndex"),CComVariant(group.first));
      pStrSave->put_Property(_T("LastGirderIndex"),CComVariant(group.second));
      pStrSave->EndUnit(); // GirderTypeGroup
   }
   pStrSave->EndUnit(); // GirderTypeGroups

   // Slab Offset
   if (m_pBridge->GetSlabOffsetType() != pgsTypes::sotBridge )
   {
      pStrSave->BeginUnit(_T("SlabOffset"),1.0);
      if ( m_pBridge->GetSlabOffsetType() == pgsTypes::sotPier )
      {
         // Slab offset per pier (one value for all girders)
         PierIndexType startPierIdx = GetPierIndex(pgsTypes::metStart);
         PierIndexType endPierIdx   = GetPierIndex(pgsTypes::metEnd);
         IndexType index = 0;
         for ( PierIndexType pierIdx = startPierIdx; pierIdx <= endPierIdx; pierIdx++, index++ )
         {
            std::vector<Float64>& vSlabOffset(m_SlabOffsets[index]);
            pStrSave->put_Property(_T("SlabOffset"),CComVariant(vSlabOffset.front()));
         }
      }
      else
      {
         ATLASSERT(m_pBridge->GetSlabOffsetType() == pgsTypes::sotGirder);
         PierIndexType startPierIdx = GetPierIndex(pgsTypes::metStart);
         PierIndexType endPierIdx   = GetPierIndex(pgsTypes::metEnd);
         IndexType index = 0;
         for ( PierIndexType pierIdx = startPierIdx; pierIdx <= endPierIdx; pierIdx++, index++ )
         {
            pStrSave->BeginUnit(_T("GirderSlabOffsets"),1.0);

            std::vector<Float64>& vSlabOffset(m_SlabOffsets[index]);
            std::vector<Float64>::iterator iter(vSlabOffset.begin());
            std::vector<Float64>::iterator end(vSlabOffset.end());
            for ( ; iter != end; iter++ )
            {
               pStrSave->put_Property(_T("SlabOffset"),CComVariant(*iter));
            }

            pStrSave->EndUnit();
         }
      }
      pStrSave->EndUnit(); // SlabOffset
   }

   pStrSave->EndUnit(); // GirderGroup

   return S_OK;
}

void CGirderGroupData::MakeCopy(const CGirderGroupData& rOther,bool bCopyDataOnly)
{
   if ( !bCopyDataOnly )
   {
      m_GroupIdx = rOther.m_GroupIdx;
      m_GroupID  = rOther.m_GroupID;
   }

   m_pPier[pgsTypes::metStart] = NULL;
   m_pPier[pgsTypes::metEnd]   = NULL;
   m_PierIndex[pgsTypes::metStart] = rOther.GetPierIndex(pgsTypes::metStart);
   m_PierIndex[pgsTypes::metEnd]   = rOther.GetPierIndex(pgsTypes::metEnd);
   UpdatePiers();

   m_SlabOffsets = rOther.m_SlabOffsets;

   if ( m_Girders.size() == 0 )
   {
      m_Girders.resize(rOther.m_Girders.size());
   }
   else
   {
      SetGirderCount(rOther.m_Girders.size());
   }
   ATLASSERT(m_Girders.size() == rOther.m_Girders.size());
   std::vector<CSplicedGirderData*>::iterator myGirderIter(m_Girders.begin());
   std::vector<CSplicedGirderData*>::iterator myGirderIterEnd(m_Girders.end());
   std::vector<CSplicedGirderData*>::const_iterator otherGirderIter(rOther.m_Girders.begin());
   std::vector<CSplicedGirderData*>::const_iterator otherGirderIterEnd(rOther.m_Girders.end());
   for ( ; myGirderIter != myGirderIterEnd; myGirderIter++, otherGirderIter++ )
   {
      CSplicedGirderData* pMyGirder = *myGirderIter; // this is my girder... we are replacing it with a copy of the other girder
      GirderIndexType myGirderIdx = INVALID_INDEX;
      GirderIDType myGirderID = INVALID_ID;
      if ( pMyGirder )
      {
         myGirderIdx = pMyGirder->GetIndex();
         myGirderID = pMyGirder->GetID();
         pMyGirder->Clear();
         pMyGirder->SetGirderGroup(NULL); // this removes the girder from its group which removes it from the bridge. it does not alter the timeline events
         delete pMyGirder; // done with this girder
      }
      pMyGirder = NULL;
      *myGirderIter = NULL;

      const CSplicedGirderData* pOtherGirder = *otherGirderIter;
      CSplicedGirderData* pNewGirder = new CSplicedGirderData(this);
      if ( bCopyDataOnly )
      {
         // copies only the data
         pNewGirder->CopySplicedGirderData(pOtherGirder);
         pNewGirder->SetIndex(myGirderIdx);
         pNewGirder->SetID(myGirderID);
      }
      else
      {
         // assignment copies everything (ID, Index, etc)
         *pNewGirder = *pOtherGirder;
      }
      *myGirderIter = pNewGirder;
   }

   m_GirderTypeGroups.clear();
   std::vector<GirderTypeGroup>::const_iterator grpIter(rOther.m_GirderTypeGroups.begin());
   std::vector<GirderTypeGroup>::const_iterator grpIterEnd(rOther.m_GirderTypeGroups.end());
   for ( ; grpIter != grpIterEnd; grpIter++ )
   {
      const GirderTypeGroup& grp = *grpIter;
      m_GirderTypeGroups.push_back(grp);
   }
}

void CGirderGroupData::MakeAssignment(const CGirderGroupData& rOther)
{
   MakeCopy( rOther, false /*assign everything*/ );
}

void CGirderGroupData::RemoveGirder(GirderIndexType gdrIdx)
{
   CSplicedGirderData* pSplicedGirder = m_Girders[gdrIdx];
   m_Girders.erase( m_Girders.begin() + gdrIdx );
   pSplicedGirder->Clear();
   delete pSplicedGirder;
}

void CGirderGroupData::Clear()
{
   std::vector<CSplicedGirderData*>::iterator iter(m_Girders.begin());
   std::vector<CSplicedGirderData*>::iterator end(m_Girders.end());
   for ( ; iter != end; iter++ )
   {
      CSplicedGirderData* pGirder = *iter;
      pGirder->Clear();
   }

   m_GirderTypeGroups.clear();

   m_SlabOffsets.clear();
}

void CGirderGroupData::UpdatePiers()
{
   if ( m_pBridge != NULL && (m_pPier[pgsTypes::metStart] == NULL || m_pPier[pgsTypes::metEnd] == NULL) )
   {
      ATLASSERT(m_PierIndex[pgsTypes::metStart] != INVALID_INDEX);
      ATLASSERT(m_PierIndex[pgsTypes::metEnd]   != INVALID_INDEX);

      m_pPier[pgsTypes::metStart] = m_pBridge->GetPier(m_PierIndex[pgsTypes::metStart]);
      m_pPier[pgsTypes::metEnd]   = m_pBridge->GetPier(m_PierIndex[pgsTypes::metEnd]);

      m_PierIndex[pgsTypes::metStart] = INVALID_INDEX;
      m_PierIndex[pgsTypes::metEnd]   = INVALID_INDEX;
   }
}

void CGirderGroupData::UpdateSlabOffsets(PierIndexType newPierIdx)
{
   PierIndexType startPierIdx = GetPierIndex(pgsTypes::metStart);
   PierIndexType endPierIdx   = GetPierIndex(pgsTypes::metEnd);
   
   ATLASSERT(startPierIdx <= newPierIdx && newPierIdx <= endPierIdx);

   PierIndexType relPierIdx = newPierIdx - startPierIdx; // relative location in the m_SlabOffsets collection
   // for the slab offsets we want to copy and insert into the collection

   // use the reference pier's slab offsets
   m_SlabOffsets.insert(m_SlabOffsets.begin() + relPierIdx + 1,m_SlabOffsets[relPierIdx]);
}

GirderIndexType CGirderGroupData::GetPrivateGirderCount() const
{
   return (GirderIndexType)m_Girders.size();
}


#if defined _DEBUG
void CGirderGroupData::AssertValid()
{
   // All girders must be associated with this group
   std::vector<CSplicedGirderData*>::iterator iter(m_Girders.begin());
   std::vector<CSplicedGirderData*>::iterator end(m_Girders.end());
   for ( ; iter != end; iter++ )
   {
      CSplicedGirderData* pGirder = *iter;
      _ASSERT( pGirder->GetGirderGroup() == this );
      pGirder->AssertValid();

      _ASSERT(pGirder->GetID() != INVALID_ID);
      _ASSERT(pGirder->GetIndex() != INVALID_INDEX);

      // end piers must be the same as for this group
      _ASSERT(pGirder->GetPier(pgsTypes::metStart) == m_pPier[pgsTypes::metStart] );
      _ASSERT(pGirder->GetPier(pgsTypes::metEnd)   == m_pPier[pgsTypes::metEnd] );
   }

   ATLASSERT( m_SlabOffsets.size() == GetPierCount() );

   // Girder type groups must be consistent with number of girders
   if ( 0 < m_Girders.size() )
   {
      ATLASSERT(m_GirderTypeGroups.size() != 0);
      ATLASSERT(m_GirderTypeGroups.back().second - m_GirderTypeGroups.front().first == m_Girders.size()-1);

      if ( m_pBridge && m_pBridge->GetSlabOffsetType() == pgsTypes::sotGirder )
      {
         std::vector<std::vector<Float64>>::iterator iter(m_SlabOffsets.begin());
         std::vector<std::vector<Float64>>::iterator end(m_SlabOffsets.end());
         for ( ; iter != end; iter++ )
         {
            std::vector<Float64>& vSlabOffsets(*iter);
            ATLASSERT(vSlabOffsets.size() == m_Girders.size());
         }
      }
   }

   if ( m_pBridge && m_pPier[pgsTypes::metStart] )
   {
      ATLASSERT( m_pBridge == m_pPier[pgsTypes::metStart]->GetBridgeDescription() );
      ATLASSERT( m_pPier[pgsTypes::metStart]->IsBoundaryPier() );
   }

   if ( m_pBridge && m_pPier[pgsTypes::metEnd] )
   {
      ATLASSERT( m_pBridge == m_pPier[pgsTypes::metEnd]->GetBridgeDescription() );
      ATLASSERT( m_pPier[pgsTypes::metEnd]->IsBoundaryPier() );
   }

   ATLASSERT(m_Girders.size()<=1 || m_pPier[pgsTypes::metStart]->GetGirderSpacing(pgsTypes::Ahead)->GetSpacingCount()+1 == m_Girders.size());
   ATLASSERT(m_Girders.size()<=1 || m_pPier[pgsTypes::metEnd  ]->GetGirderSpacing(pgsTypes::Back )->GetSpacingCount()+1 == m_Girders.size());
}
#endif // _DEBUG
