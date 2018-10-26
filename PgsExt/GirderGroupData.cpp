///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2013  Washington State Department of Transportation
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
#include <PgsExt\ClosurePourData.h>

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

   Clear();
}

CGirderGroupData& CGirderGroupData::operator=(const CGirderGroupData& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

void CGirderGroupData::CopyGirderGroupData(const CGirderGroupData* pGroup)
{
   MakeCopy(*pGroup,true/*copy only data*/);
}

bool CGirderGroupData::operator==(const CGirderGroupData& rOther) const
{
   if ( GetPierIndex(pgsTypes::metStart) != rOther.GetPierIndex(pgsTypes::metStart) )
      return false;

   if ( GetPierIndex(pgsTypes::metEnd) != rOther.GetPierIndex(pgsTypes::metEnd) )
      return false;

   if ( m_pBridge && !m_pBridge->UseSameNumberOfGirdersInAllGroups() )
   {
      if ( m_Girders.size() != rOther.m_Girders.size() )
         return false;
   }

   bool bCheckSlabOffset = false;
   if ( m_pBridge && (m_pBridge->GetSlabOffsetType() == pgsTypes::sotGroup || m_pBridge->GetSlabOffsetType() == pgsTypes::sotSegment))
   {
      bCheckSlabOffset = true;
   }

   GirderIndexType nGirders = m_Girders.size();
   for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
   {
      if ( *m_Girders[gdrIdx] != *rOther.m_Girders[gdrIdx] )
         return false;

      if ( bCheckSlabOffset && !IsEqual(GetSlabOffset(gdrIdx,pgsTypes::metStart),rOther.GetSlabOffset(gdrIdx,pgsTypes::metStart)) )
         return false;

      if ( bCheckSlabOffset && !IsEqual(GetSlabOffset(gdrIdx,pgsTypes::metEnd),rOther.GetSlabOffset(gdrIdx,pgsTypes::metEnd)) )
         return false;
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
      m_pPier[end]->SetGirderSpacing(pierFace,*pOldSpacing);

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

CPierData2* CGirderGroupData::GetPier(pgsTypes::MemberEndType end)
{
   return m_pPier[end];
}

const CPierData2* CGirderGroupData::GetPier(pgsTypes::MemberEndType end) const
{
   return m_pPier[end];
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
      pGirder->SetID( m_pBridge->GetNextGirderID() );
      pGirder->SetGirderGroup(this);
      m_Girders.push_back(pGirder);

      GirderTypeGroup group;
      group.first = 0;
      group.second = 0;
      m_GirderTypeGroups.push_back(group);

      m_SlabOffset[pgsTypes::metStart].push_back(::ConvertToSysUnits(10.0,unitMeasure::Inch));
      m_SlabOffset[pgsTypes::metEnd].push_back(::ConvertToSysUnits(10.0,unitMeasure::Inch));
   }

   if ( nGirders < m_Girders.size() )
      RemoveGirders(m_Girders.size()-nGirders);
   else if ( m_Girders.size() < nGirders )
      AddGirders(nGirders-m_Girders.size());
   //else
   // do nothing if nGirders == m_Girders.size()

   ATLASSERT(nGirders == m_Girders.size());
   ATLASSERT(nGirders == m_SlabOffset[pgsTypes::metStart].size());
   ATLASSERT(nGirders == m_SlabOffset[pgsTypes::metEnd].size());
   ASSERT_VALID;
}

void CGirderGroupData::RemoveGirders(GirderIndexType nGirders)
{
   ATLASSERT( nGirders < (GirderIndexType)m_Girders.size() ); // removing more than the container holds

   CTimelineManager* pTimelineMgr = m_pBridge->GetTimelineManager();

   std::vector<CSplicedGirderData*>::iterator iter( m_Girders.end() - nGirders );
   std::vector<CSplicedGirderData*>::iterator end(  m_Girders.end() );
   for ( ; iter != end; iter++ )
   {
      CSplicedGirderData* pSplicedGirder = *iter;

      // remove references to this girder from the timeline manager
      CGirderKey girderKey(pSplicedGirder->GetGirderKey());
      DuctIndexType nDucts = pSplicedGirder->GetPostTensioning()->GetDuctCount();
      for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
      {
#pragma Reminder("REVIEW: seems like we need to remove the duct rather than set the index to invalid")
         pTimelineMgr->SetStressTendonEventByIndex(girderKey, ductIdx, INVALID_INDEX);
      }

      SegmentIndexType nSegments = pSplicedGirder->GetSegmentCount();
      for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
      {
         CPrecastSegmentData* pSegment = pSplicedGirder->GetSegment(segIdx);
         SegmentIDType segID = pSegment->GetID();
         ATLASSERT(segID != INVALID_ID);

#pragma Reminder("REVIEW: seems like we need to remove the segment rather than set the index to invalid")
         pTimelineMgr->SetSegmentErectionEventByIndex(segID,INVALID_INDEX);
      }
      delete pSplicedGirder;

      m_GirderTypeGroups.back().second--;
      if ( m_GirderTypeGroups.back().second < m_GirderTypeGroups.back().first )
         m_GirderTypeGroups.pop_back();
   }

   GirderIndexType ng = m_Girders.size();
   m_SlabOffset[pgsTypes::metStart].resize(ng-nGirders);
   m_SlabOffset[pgsTypes::metEnd].resize(ng-nGirders);
   m_Girders.resize(ng-nGirders);
}

void CGirderGroupData::Initialize(GirderIndexType nGirders)
{
   Clear();

   if ( nGirders == 0 )
      return;

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

      m_SlabOffset[pgsTypes::metStart].push_back( ::ConvertToSysUnits( 10.0, unitMeasure::Inch ) );
      m_SlabOffset[pgsTypes::metEnd].push_back( ::ConvertToSysUnits( 10.0, unitMeasure::Inch ) );
   }

   m_GirderTypeGroups.push_back( GirderTypeGroup(0,m_Girders.size()-1) );
}

void CGirderGroupData::AddGirders(GirderIndexType nGirders)
{
   ATLASSERT(m_Girders.size() != 0);

   // Collect the construction and erection event information for each
   // segment for the reference girder
   const CSplicedGirderData* pRefGirder = m_Girders.back();
   GirderIDType refGdrID = pRefGirder->GetID();

   GirderIndexType gdrIdx = (GirderIndexType)(m_Girders.size()-1);

   CGirderKey girderKey(m_GroupIdx,gdrIdx);

   SegmentIndexType nSegments = pRefGirder->GetSegmentCount();
   std::vector<EventIndexType> segmentEvents(nSegments);

   for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      const CPrecastSegmentData* pSegment = pRefGirder->GetSegment(segIdx);
      SegmentIDType segID = pSegment->GetID();
      ATLASSERT(segID != INVALID_ID);

      EventIndexType erectionEventIdx = m_pBridge->GetTimelineManager()->GetSegmentErectionEventIndex( segID );

      segmentEvents[segIdx] = erectionEventIdx;
   }

   // Collect the post-tensioning event information for the reference girder
   DuctIndexType nDucts = pRefGirder->GetPostTensioning()->GetDuctCount();
   std::vector<EventIndexType> ptEvents(nDucts);
   for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
   {
      ptEvents[ductIdx] = m_pBridge->GetTimelineManager()->GetStressTendonEventIndex( girderKey, ductIdx);
   }

   // create the new girders
   for ( GirderIndexType i = 0; i < nGirders; i++ )
   {
      gdrIdx++;
      CSplicedGirderData* pNewGirder = new CSplicedGirderData(*pRefGirder);
      pNewGirder->SetGirderGroup(this);
      pNewGirder->SetIndex(m_Girders.size());
      m_Girders.push_back(pNewGirder);

      pNewGirder->SetID( m_pBridge->GetNextGirderID() );
      CGirderKey newGirderKey(pNewGirder->GetGirderKey());

      m_SlabOffset[pgsTypes::metStart].push_back(m_SlabOffset[pgsTypes::metStart].back());
      m_SlabOffset[pgsTypes::metEnd].push_back(m_SlabOffset[pgsTypes::metEnd].back());

      // set the construction and erection event information for the segments of this new girder
      std::vector<EventIndexType>::iterator segIterBegin(segmentEvents.begin());
      std::vector<EventIndexType>::iterator segIter(segIterBegin);
      std::vector<EventIndexType>::iterator segIterEnd(segmentEvents.end());
      for ( ; segIter != segIterEnd; segIter++ )
      {
         SegmentIndexType segIdx = segIter-segIterBegin;
         CPrecastSegmentData* pNewSegment = pNewGirder->GetSegment(segIdx);
         SegmentIDType segID = pNewSegment->GetID();
         ATLASSERT(segID != INVALID_ID);

         m_pBridge->GetTimelineManager()->SetSegmentErectionEventByIndex( segID, *segIter );
      }

      // set the pt events
      for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
      {
         ATLASSERT(m_GroupID != INVALID_ID);
         m_pBridge->GetTimelineManager()->SetStressTendonEventByIndex(newGirderKey,ductIdx,ptEvents[ductIdx]);
      }
   }

   // Update the girder type group
   m_GirderTypeGroups.back().second = m_Girders.size()-1;
}

GirderIndexType CGirderGroupData::GetGirderCount() const
{
   ATLASSERT(m_SlabOffset[pgsTypes::metStart].size() == m_Girders.size());
   ATLASSERT(m_SlabOffset[pgsTypes::metEnd].size()   == m_Girders.size());

   if ( m_pBridge && m_pBridge->UseSameNumberOfGirdersInAllGroups() )
      return m_pBridge->GetGirderCount();
   else
      return (GirderIndexType)m_Girders.size();
}

void CGirderGroupData::SetGirder(GirderIndexType gdrIdx,CSplicedGirderData* pGirderData)
{
   m_Girders[gdrIdx] = pGirderData;
}

CSplicedGirderData* CGirderGroupData::GetGirder(GirderIndexType gdrIdx)
{
   return m_Girders[gdrIdx];
}

const CSplicedGirderData* CGirderGroupData::GetGirder(GirderIndexType gdrIdx) const
{
   return m_Girders[gdrIdx];
}

void CGirderGroupData::SetSlabOffset(pgsTypes::MemberEndType end,Float64 offset)
{
//#if defined _DEBUG
//   pgsTypes::SlabOffsetType slabOffsetType = m_pBridge->GetSlabOffsetType();
//   ATLASSERT(slabOffsetType == pgsTypes::sotGroup);
//#endif
//
   std::vector<Float64>::iterator iter(m_SlabOffset[end].begin());
   std::vector<Float64>::iterator iterEnd(m_SlabOffset[end].end());
   for ( ; iter != iterEnd; iter++ )
   {
      *iter = offset;
   }
}

void CGirderGroupData::SetSlabOffset(GirderIndexType gdrIdx,pgsTypes::MemberEndType end,Float64 offset)
{
//#if defined _DEBUG
//   pgsTypes::SlabOffsetType slabOffsetType = m_pBridge->GetSlabOffsetType();
//   ATLASSERT(slabOffsetType == pgsTypes::sotSegment);
//#endif
//
   m_SlabOffset[end][gdrIdx] = offset;
}

Float64 CGirderGroupData::GetSlabOffset(GirderIndexType gdrIdx,pgsTypes::MemberEndType end) const
{
   Float64 offset;
   pgsTypes::SlabOffsetType slabOffsetType = m_pBridge->GetSlabOffsetType();
   if ( slabOffsetType == pgsTypes::sotBridge )
      offset = m_pBridge->GetSlabOffset();
   else if ( slabOffsetType == pgsTypes::sotGroup )
      offset = m_SlabOffset[end].front();
   else
      offset = m_SlabOffset[end][gdrIdx];

   return offset;
}

GroupIndexType CGirderGroupData::CreateGirderTypeGroup(GirderIndexType firstGdrIdx,GirderIndexType lastGdrIdx)
{
   if ( m_pBridge->UseSameGirderForEntireBridge() )
      return 0; // can't create a new group... return group 0

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
            gdrGroups.push_back(gdrGroup); // if last is < first, then this isn't a group... the new group ends at the last girder
      }
      else
      {
         gdrGroups.push_back(gdrGroup); // save the rest of the groups
      }
   }

   m_GirderTypeGroups = gdrGroups;

   IS_VALID;

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
   IS_VALID;
}

void CGirderGroupData::Expand(GroupIndexType girderTypeGroupIdx)
{
   GirderIndexType firstGdrIdx, lastGdrIdx;
   std::_tstring strName;
   GetGirderTypeGroup(girderTypeGroupIdx,&firstGdrIdx,&lastGdrIdx,&strName);

   if ( firstGdrIdx == lastGdrIdx )
      return; // nothing to expand

   std::vector<GirderTypeGroup>::iterator iter = m_GirderTypeGroups.begin() + girderTypeGroupIdx;
   std::vector<GirderTypeGroup>::iterator pos = m_GirderTypeGroups.erase(iter); // returns the iter the element after the one removed

   // inserted element goes before the iterator... insert retuns position of newly inserted item
   // go in reverse order
   for ( GirderIndexType gdrIdx = lastGdrIdx; firstGdrIdx <= gdrIdx; gdrIdx-- )
   {
      GirderTypeGroup group(gdrIdx,gdrIdx);
      pos = m_GirderTypeGroups.insert(pos,group);
   }
   IS_VALID;
}


void CGirderGroupData::JoinAll(GirderIndexType gdrIdx)
{
   if ( m_Girders.size() == 0 )
      return;

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
   IS_VALID;
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
         break;
      else
         gdrGroups.push_back(gdrGroup);
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
      gdrGroups.insert(gdrGroups.end(),iter,m_GirderTypeGroups.end());

   // finally replace the data member with the local girder groups
   m_GirderTypeGroups = gdrGroups;
   IS_VALID;
}

GroupIndexType CGirderGroupData::GetGirderTypeGroupCount() const
{
   if ( m_pBridge && m_pBridge->UseSameGirderForEntireBridge() )
      return 1;
   else
      return m_GirderTypeGroups.size();
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
      return 0; // can't create a new group... return group 0

   GroupIndexType nGroups = GetGirderTypeGroupCount();
   for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
   {
      GirderIndexType firstGdrIdx, lastGdrIdx;

      GirderTypeGroup group = m_GirderTypeGroups[grpIdx];

      firstGdrIdx = group.first;
      lastGdrIdx  = group.second;

      if ( firstGdrIdx <= gdrIdx && gdrIdx <= lastGdrIdx )
         return grpIdx;
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
      return m_pBridge->GetGirderName();
   else
      return m_Girders[gdrIdx]->GetGirderName();
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

      if (m_pBridge->GetSlabOffsetType() != pgsTypes::sotBridge )
      {
         pStrLoad->BeginUnit(_T("SlabOffset"));
         var.vt = VT_R8;
         if ( m_pBridge->GetSlabOffsetType() == pgsTypes::sotGroup )
         {
            hr = pStrLoad->get_Property(_T("SlabOffsetAtStart"),&var);
            m_SlabOffset[pgsTypes::metStart].push_back(var.dblVal);

            hr = pStrLoad->get_Property(_T("SlabOffsetAtEnd"),&var);
            m_SlabOffset[pgsTypes::metEnd].push_back(var.dblVal);
         }
         else
         {
            ATLASSERT(m_pBridge->GetSlabOffsetType() == pgsTypes::sotSegment);
            var.vt = VT_INDEX;
            pStrLoad->get_Property(_T("Count"),&var);
            GirderIndexType nGirders = VARIANT2INDEX(var);

            var.vt = VT_R8;
            for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
            {
               pStrLoad->get_Property(_T("SlabOffsetAtStart"),&var);
               m_SlabOffset[pgsTypes::metStart].push_back(var.dblVal);

               pStrLoad->get_Property(_T("SlabOffsetAtEnd"),&var);
               m_SlabOffset[pgsTypes::metEnd].push_back(var.dblVal);
            }
         }
         pStrLoad->EndUnit(); // SlabOffset
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

      if ( m_pBridge->GetSlabOffsetType() == pgsTypes::sotBridge )
      {
         m_SlabOffset[pgsTypes::metStart].resize(nGirders,m_pBridge->GetSlabOffset());
         m_SlabOffset[pgsTypes::metEnd].resize(nGirders,m_pBridge->GetSlabOffset());
      }
      else if ( m_pBridge->GetSlabOffsetType() == pgsTypes::sotGroup )
      {
         // if slab offset is by group, the we need to fill up the vectors for all the girders
         m_SlabOffset[pgsTypes::metStart].resize(nGirders,m_SlabOffset[pgsTypes::metStart].front());
         m_SlabOffset[pgsTypes::metEnd].resize(nGirders,m_SlabOffset[pgsTypes::metEnd].front());
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

      hr = pStrLoad->EndUnit(); // GirderTypeGroup
   }
   catch(...)
   {
      ATLASSERT(0);
   }

   return S_OK;
}

HRESULT CGirderGroupData::Save(IStructuredSave* pStrSave,IProgress* pProgress)
{
   pStrSave->BeginUnit(_T("GirderGroup"),1.0);

   pStrSave->put_Property(_T("ID"), CComVariant(m_GroupID) );

   pStrSave->put_Property(_T("StartPier"),CComVariant(GetPierIndex(pgsTypes::metStart)));
   pStrSave->put_Property(_T("EndPier"),CComVariant(GetPierIndex(pgsTypes::metEnd)));

   if (m_pBridge->GetSlabOffsetType() != pgsTypes::sotBridge )
   {
      pStrSave->BeginUnit(_T("SlabOffset"),1.0);
      if ( m_pBridge->GetSlabOffsetType() == pgsTypes::sotGroup )
      {
         pStrSave->put_Property(_T("SlabOffsetAtStart"),CComVariant(m_SlabOffset[pgsTypes::metStart].front()));
         pStrSave->put_Property(_T("SlabOffsetAtEnd"),  CComVariant(m_SlabOffset[pgsTypes::metEnd].front()));
      }
      else
      {
         ATLASSERT(m_pBridge->GetSlabOffsetType() == pgsTypes::sotSegment);
         pStrSave->put_Property(_T("Count"),CComVariant(m_Girders.size()));
         std::vector<Float64>::iterator startOffsetIter(m_SlabOffset[pgsTypes::metStart].begin());
         std::vector<Float64>::iterator startOffsetIterEnd(m_SlabOffset[pgsTypes::metStart].end());
         std::vector<Float64>::iterator endOffsetIter(m_SlabOffset[pgsTypes::metEnd].begin());
         std::vector<Float64>::iterator endOffsetIterEnd(m_SlabOffset[pgsTypes::metEnd].end());

         for ( ; startOffsetIter != startOffsetIterEnd && endOffsetIter != endOffsetIterEnd; startOffsetIter++, endOffsetIter++ )
         {
            pStrSave->put_Property(_T("SlabOffsetAtStart"),CComVariant(*startOffsetIter));
            pStrSave->put_Property(_T("SlabOffsetAtEnd"),CComVariant(*endOffsetIter));
         }
      }
      pStrSave->EndUnit(); // SlabOffset
   }

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

   pStrSave->EndUnit(); // GirderGroup

   return S_OK;
}

void CGirderGroupData::MakeCopy(const CGirderGroupData& rOther,bool bCopyDataOnly)
{
   Clear();

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

   m_SlabOffset[pgsTypes::metStart] = rOther.m_SlabOffset[pgsTypes::metStart];
   m_SlabOffset[pgsTypes::metEnd]   = rOther.m_SlabOffset[pgsTypes::metEnd];

   std::vector<CSplicedGirderData*>::const_iterator iter(rOther.m_Girders.begin());
   std::vector<CSplicedGirderData*>::const_iterator end(rOther.m_Girders.end());
   for ( ; iter != end; iter++ )
   {
      const CSplicedGirderData* pGirder = *iter;
      CSplicedGirderData* pNewGirder = new CSplicedGirderData(this);
      if ( bCopyDataOnly )
      {
         // copies only the data
         pNewGirder->CopySplicedGirderData(pGirder);
      }
      else
      {
         // assignment copies everything (ID, Index, etc)
         *pNewGirder = *pGirder;
      }
      m_Girders.push_back(pNewGirder);
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
   delete pSplicedGirder;
}

void CGirderGroupData::Clear()
{
   std::vector<CSplicedGirderData*>::iterator iter(m_Girders.begin());
   std::vector<CSplicedGirderData*>::iterator end(m_Girders.end());
   for ( ; iter != end; iter++ )
   {
      CSplicedGirderData* pGirder = *iter;
      delete pGirder;
   }

   m_Girders.clear();
   m_GirderTypeGroups.clear();
   m_SlabOffset[pgsTypes::metStart].clear();
   m_SlabOffset[pgsTypes::metEnd].clear();
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

   // Girder type groups must be consistent with number of girders
   if ( 0 < m_Girders.size() )
   {
      ATLASSERT(m_GirderTypeGroups.size() != 0);
      ATLASSERT(m_GirderTypeGroups.back().second - m_GirderTypeGroups.front().first == m_Girders.size()-1);
      ATLASSERT(m_SlabOffset[pgsTypes::metStart].size() == m_Girders.size());
      ATLASSERT(m_SlabOffset[pgsTypes::metEnd].size() == m_Girders.size());
   }

   ATLASSERT(m_SlabOffset[pgsTypes::metStart].size() == m_SlabOffset[pgsTypes::metEnd].size());

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
}
#endif // _DEBUG
