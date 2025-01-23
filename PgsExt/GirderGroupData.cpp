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
   m_pBridgeDesc                     = nullptr;
   m_pPier[pgsTypes::metStart]   = nullptr;
   m_pPier[pgsTypes::metEnd]     = nullptr;

   m_PierIndex[pgsTypes::metStart] = INVALID_INDEX;
   m_PierIndex[pgsTypes::metEnd]   = INVALID_INDEX;
   m_GroupIdx                      = INVALID_INDEX;
   m_GroupID                       = INVALID_ID;
}

CGirderGroupData::CGirderGroupData(CBridgeDescription2* pBridge)
{
   m_pBridgeDesc      = pBridge;

   m_pPier[pgsTypes::metStart]   = nullptr;
   m_pPier[pgsTypes::metEnd]     = nullptr;

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

   m_pBridgeDesc = pStartPier->GetBridgeDescription();

   m_PierIndex[pgsTypes::metStart] = INVALID_INDEX;
   m_PierIndex[pgsTypes::metEnd]   = INVALID_INDEX;

   m_pPier[pgsTypes::metStart]   = pStartPier;
   m_pPier[pgsTypes::metEnd]     = pEndPier;

   m_GroupIdx = INVALID_INDEX;
   m_GroupID  = INVALID_ID;
}

CGirderGroupData::CGirderGroupData(const CGirderGroupData& rOther)
{
   m_pBridgeDesc                     = nullptr;
   m_pPier[pgsTypes::metStart]   = nullptr;
   m_pPier[pgsTypes::metEnd]     = nullptr;

   m_PierIndex[pgsTypes::metStart] = INVALID_INDEX;
   m_PierIndex[pgsTypes::metEnd]   = INVALID_INDEX;
   m_GroupIdx                      = INVALID_INDEX;
   m_GroupID                       = INVALID_ID;

   MakeCopy(rOther,true/*copy only data*/);
}

CGirderGroupData::~CGirderGroupData()
{
   m_pPier[pgsTypes::metStart]   = nullptr;
   m_pPier[pgsTypes::metEnd]     = nullptr;

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

   if ( m_pBridgeDesc && !m_pBridgeDesc->UseSameNumberOfGirdersInAllGroups() )
   {
      if ( m_Girders.size() != rOther.m_Girders.size() )
      {
         return false;
      }
   }

   if ( m_pBridgeDesc && !m_pBridgeDesc->UseSameGirderForEntireBridge() )
   {
      if (m_GirderTypeGroups != rOther.m_GirderTypeGroups)
      {
         return false;
      }
   }

   if (m_pBridgeDesc && !IsBridgeSpacing(m_pBridgeDesc->GetGirderSpacingType()) && IsTopWidthSpacing(m_pBridgeDesc->GetGirderSpacingType()))
   {
      if (m_GirderTopWidthGroups != rOther.m_GirderTopWidthGroups)
      {
         return false;
      }
   }

   GirderIndexType nGirders = m_Girders.size();
   for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
   {
      if ( *m_Girders[gdrIdx] != *rOther.m_Girders[gdrIdx] )
      {
         return false;
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
   m_pBridgeDesc = pBridge;
   UpdatePiers();
}

CBridgeDescription2* CGirderGroupData::GetBridgeDescription()
{
   return m_pBridgeDesc;
}

const CBridgeDescription2* CGirderGroupData::GetBridgeDescription() const
{
   return m_pBridgeDesc;
}

void CGirderGroupData::SetPier(pgsTypes::MemberEndType end,CPierData2* pPier)
{
   ATLASSERT(pPier != nullptr);

   CGirderSpacing2* pOldSpacing = nullptr;
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
   if ( m_pBridgeDesc != nullptr )
   {
      ATLASSERT(m_pBridgeDesc == pPier->GetBridgeDescription());
   }
#endif
   m_pBridgeDesc = pPier->GetBridgeDescription();
}

void CGirderGroupData::SetPiers(CPierData2* pStartPier,CPierData2* pEndPier)
{
   SetPier(pgsTypes::metStart,pStartPier);
   SetPier(pgsTypes::metEnd,  pEndPier);
}

void CGirderGroupData::AddSpan(PierIndexType refPierIdx,pgsTypes::PierFaceType face)
{
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
   // Adjust the girders in the group for the span that is removed
   // remove span references from the girders before the span is destroyed
   // Segments have pointers to the spans they start and end in
   for(auto* pGirder : m_Girders)
   {
      pGirder->RemoveSpan(spanIdx, rmPierType);
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
      return nullptr; // pier isn't part of this girder group
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
      return nullptr; // pier isn't part of this girder group
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
      ATLASSERT(m_pPier[end] == nullptr);
      return m_PierIndex[end];
   }
}

CGirderGroupData* CGirderGroupData::GetPrevGirderGroup()
{
   if ( m_pPier[pgsTypes::metStart] )
   {
      CSpanData2* pSpan = m_pPier[pgsTypes::metStart]->GetPrevSpan();
      CGirderGroupData* pGroup = m_pBridgeDesc->GetGirderGroup(pSpan);
      return pGroup;
   }

   return nullptr;
}

const CGirderGroupData* CGirderGroupData::GetPrevGirderGroup() const
{
   if ( m_pPier[pgsTypes::metStart] )
   {
      const CSpanData2* pSpan = m_pPier[pgsTypes::metStart]->GetPrevSpan();
      const CGirderGroupData* pGroup = m_pBridgeDesc->GetGirderGroup(pSpan);
      return pGroup;
   }

   return nullptr;
}

CGirderGroupData* CGirderGroupData::GetNextGirderGroup()
{
   if ( m_pPier[pgsTypes::metEnd] )
   {
      CSpanData2* pSpan = m_pPier[pgsTypes::metEnd]->GetNextSpan();
      CGirderGroupData* pGroup = m_pBridgeDesc->GetGirderGroup(pSpan);
      return pGroup;
   }

   return nullptr;
}

const CGirderGroupData* CGirderGroupData::GetNextGirderGroup() const
{
   if ( m_pPier[pgsTypes::metEnd] )
   {
      const CSpanData2* pSpan = m_pPier[pgsTypes::metEnd]->GetNextSpan();
      const CGirderGroupData* pGroup = m_pBridgeDesc->GetGirderGroup(pSpan);
      return pGroup;
   }

   return nullptr;
}

void CGirderGroupData::SetGirderCount(GirderIndexType nGirders)
{
   if ( m_Girders.size() == 0 && nGirders != 0 )
   {
      ATLASSERT(m_GirderTypeGroups.size() == 0);

      CSplicedGirderData* pGirder = new CSplicedGirderData;
      pGirder->SetIndex(0);
      pGirder->SetID( m_pBridgeDesc ? m_pBridgeDesc->GetNextGirderID() : INVALID_ID );
      pGirder->SetGirderGroup(this);
      m_Girders.push_back(pGirder);

      GirderGroup group;
      group.first = 0;
      group.second = 0;
      m_GirderTypeGroups.push_back(group);
      m_GirderTopWidthGroups.push_back(group);
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
   PGS_ASSERT_VALID;
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

      pNewGirder->SetID( m_pBridgeDesc->GetNextGirderID() );
      GirderIDType newGdrID = pNewGirder->GetID();

      pNewGirder->Initialize(); // creates the default segment
   }

   m_GirderTypeGroups.push_back( GirderGroup(0,m_Girders.size()-1) );
   m_GirderTopWidthGroups.push_back( GirderGroup(0, m_Girders.size() - 1));
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
      pSplicedGirder = nullptr;

      m_GirderTypeGroups.back().second--;
      if (m_GirderTypeGroups.back().second < m_GirderTypeGroups.back().first)
      {
         m_GirderTypeGroups.pop_back();
      }

      m_GirderTopWidthGroups.back().second--;
      if (m_GirderTopWidthGroups.back().second < m_GirderTopWidthGroups.back().first)
      {
         m_GirderTopWidthGroups.pop_back();
      }
   }

   GirderIndexType nGirders = m_Girders.size();
   m_Girders.resize(nGirders-nGirdersToRemove);


   // Update the girder spacing (number of girders and number of spaces are related)
   CPierData2* pStartPier = m_pPier[pgsTypes::metStart];
   CPierData2* pEndPier = m_pPier[pgsTypes::metEnd];
   PierIndexType endPierIdx = pEndPier->GetIndex();
   for (CPierData2* pPier = pStartPier; pPier != nullptr && pPier->GetIndex() <= endPierIdx; )
   {
      if (pPier == pStartPier)
      {
         pPier->GetGirderSpacing(pgsTypes::Ahead)->RemoveGirders(nGirdersToRemove);
      }
      else if (pPier == pEndPier)
      {
         pPier->GetGirderSpacing(pgsTypes::Back)->RemoveGirders(nGirdersToRemove);
      }
      else
      {
         pPier->GetGirderSpacing(pgsTypes::Ahead)->RemoveGirders(nGirdersToRemove);
         pPier->GetGirderSpacing(pgsTypes::Back)->RemoveGirders(nGirdersToRemove);
      }

      // advance to next pier
      if (pPier->GetNextSpan())
      {
         pPier = pPier->GetNextSpan()->GetNextPier();
      }
      else
      {
         pPier = nullptr;
      }
   }


   PGS_ASSERT_VALID;
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
   CTimelineManager* pTimelineManager = m_pBridgeDesc->GetTimelineManager();
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
      GirderIDType gdrID = m_pBridgeDesc->GetNextGirderID();
      CSplicedGirderData* pNewGirder = new CSplicedGirderData(this,gdrIdx,gdrID,*pRefGirder);
      m_Girders.push_back(pNewGirder);

      CGirderKey newGirderKey(pNewGirder->GetGirderKey());

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
   m_GirderTopWidthGroups.back().second = m_Girders.size() - 1;

   // Update the girder spacing (number of girders and number of spaces are related)
   CPierData2* pStartPier = m_pPier[pgsTypes::metStart];
   CPierData2* pEndPier   = m_pPier[pgsTypes::metEnd];
   PierIndexType endPierIdx = pEndPier->GetIndex();
   for ( CPierData2* pPier = pStartPier; pPier != nullptr && pPier->GetIndex() <= endPierIdx;  )
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

      // advance to next pier
      if ( pPier->GetNextSpan() )
      {
         pPier = pPier->GetNextSpan()->GetNextPier();
      }
      else
      {
         pPier = nullptr;
      }
   }
}

GirderIndexType CGirderGroupData::GetGirderCount() const
{
   if ( m_pBridgeDesc && m_pBridgeDesc->UseSameNumberOfGirdersInAllGroups() )
   {
      return m_pBridgeDesc->GetGirderCount();
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
      return nullptr;
   }

   return m_Girders[gdrIdx];
}

const CSplicedGirderData* CGirderGroupData::GetGirder(GirderIndexType gdrIdx) const
{
   if ( gdrIdx < 0 || m_Girders.size() <= gdrIdx )
   {
      return nullptr;
   }

   return m_Girders[gdrIdx];
}

GroupIndexType CGirderGroupData::CreateGirderTypeGroup(GirderIndexType firstGdrIdx,GirderIndexType lastGdrIdx)
{
   if ( m_pBridgeDesc->UseSameGirderForEntireBridge() )
   {
      return 0; // can't create a new group... return group 0
   }

   GroupIndexType newGroupIdx;
   CreateGirderGroup(firstGdrIdx, lastGdrIdx, &m_GirderTypeGroups, &newGroupIdx);
   return newGroupIdx;

}

void CGirderGroupData::ExpandAll()
{
   ExpandAll(&m_GirderTypeGroups);
}

void CGirderGroupData::Expand(GroupIndexType girderTypeGroupIdx)
{
   Expand(girderTypeGroupIdx, &m_GirderTypeGroups);
}


void CGirderGroupData::JoinAll(GirderIndexType gdrIdx)
{
   if ( m_Girders.size() == 0 )
   {
      return;
   }

   std::_tstring strName = m_Girders[gdrIdx]->GetGirderName();
   const GirderLibraryEntry* pGdrEntry = m_Girders[gdrIdx]->GetGirderLibraryEntry();

   GirderGroup firstGroup = m_GirderTypeGroups.front();
   GirderGroup lastGroup  = m_GirderTypeGroups.back();

   GirderGroup joinedGroup;
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
   PGS_ASSERT_VALID;
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

   Join(firstGdrIdx, lastGdrIdx, gdrIdx, &m_GirderTypeGroups);
}

GroupIndexType CGirderGroupData::GetGirderTypeGroupCount() const
{
   if ( m_pBridgeDesc && m_pBridgeDesc->UseSameGirderForEntireBridge() )
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
   if ( m_pBridgeDesc && m_pBridgeDesc->UseSameNumberOfGirdersInAllGroups() &&
        m_pBridgeDesc->UseSameGirderForEntireBridge() )
   {
      *pFirstGdrIdx = 0;
      *pLastGdrIdx = m_pBridgeDesc->GetGirderCount()-1;
      *pName = m_pBridgeDesc->GetGirderName();
   }
   else
   {
      _ASSERT( girderTypeGroupIdx < (SpacingIndexType)m_GirderTypeGroups.size() );
      GirderGroup group = m_GirderTypeGroups[girderTypeGroupIdx];

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

CGirderTypeGroup CGirderGroupData::GetGirderTypeGroup(GroupIndexType girderTypeGroupIdx) const
{
   CGirderTypeGroup group;
   GetGirderTypeGroup(girderTypeGroupIdx, &group.firstGdrIdx, &group.lastGdrIdx, &group.strName);
   return group;
}

std::vector<CGirderTypeGroup> CGirderGroupData::GetGirderTypeGroups() const
{
   std::vector<CGirderTypeGroup> groups;
   for (const auto& p : m_GirderTypeGroups)
   {
      CGirderTypeGroup group;
      group.firstGdrIdx = p.first;
      group.lastGdrIdx = p.second;
      group.strName = m_Girders[p.first]->GetGirderName();
      group.pGdrEntry = m_Girders[p.first]->GetGirderLibraryEntry();
      groups.push_back(group);
   }
   return groups;
}

void CGirderGroupData::SetGirderTypeGroups(const std::vector<CGirderTypeGroup>& girderTypeGroups)
{
#if defined _DEBUG
   GirderIndexType firstGdrIdx = girderTypeGroups.front().firstGdrIdx;
   GirderIndexType lastGdrIdx = girderTypeGroups.back().lastGdrIdx;
   GirderIndexType nGirders = lastGdrIdx - firstGdrIdx + 1;
   ATLASSERT(nGirders == m_Girders.size());
   ATLASSERT(girderTypeGroups.size() <= m_Girders.size());
#endif

   m_GirderTypeGroups.clear();
   for (const auto& typeGroup : girderTypeGroups)
   {
      ATLASSERT(typeGroup.firstGdrIdx <= typeGroup.lastGdrIdx);
      GirderGroup group;
      group.first = typeGroup.firstGdrIdx;
      group.second = typeGroup.lastGdrIdx;
      m_GirderTypeGroups.push_back(group);
      for (GirderIndexType gdrIdx = typeGroup.firstGdrIdx; gdrIdx <= typeGroup.lastGdrIdx; gdrIdx++)
      {
         m_Girders[gdrIdx]->SetGirderName(typeGroup.strName.c_str());
         m_Girders[gdrIdx]->SetGirderLibraryEntry(typeGroup.pGdrEntry);
      }
   }
}

GroupIndexType CGirderGroupData::FindGroup(GirderIndexType gdrIdx) const
{
   if ( m_pBridgeDesc && m_pBridgeDesc->UseSameGirderForEntireBridge() )
   {
      return 0; // can't create a new group... return group 0
   }

   return FindGroup(gdrIdx, m_GirderTypeGroups);
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
   if ( m_pBridgeDesc && m_pBridgeDesc->UseSameGirderForEntireBridge() )
   {
      return m_pBridgeDesc->GetGirderName();
   }
   else
   {
      return m_Girders[gdrIdx]->GetGirderName();
   }
}

GroupIndexType CGirderGroupData::CreateGirderTopWidthGroup(GirderIndexType firstGdrIdx, GirderIndexType lastGdrIdx)
{
   if (!IsTopWidthSpacing(m_pBridgeDesc->GetGirderSpacingType()))
   {
      return INVALID_INDEX; // top flange width is not applicable
   }

   if (IsBridgeSpacing(m_pBridgeDesc->GetGirderSpacingType()))
   {
      return 0; // can't create a new group... return 0
   }

   GroupIndexType newGroupIdx;
   CreateGirderGroup(firstGdrIdx, lastGdrIdx, &m_GirderTopWidthGroups, &newGroupIdx);
   return newGroupIdx;
}

GroupIndexType CGirderGroupData::GetGirderTopWidthGroupCount() const
{
   if (m_pBridgeDesc && IsBridgeSpacing(m_pBridgeDesc->GetGirderSpacingType()))
   {
      ATLASSERT(IsTopWidthSpacing(m_pBridgeDesc->GetGirderSpacingType()));
      return 1;
   }
   else
   {
      return m_GirderTopWidthGroups.size();
   }
}

void CGirderGroupData::GetGirderTopWidthGroup(GroupIndexType groupIdx, GirderIndexType* pFirstGdrIdx, GirderIndexType* pLastGdrIdx,pgsTypes::TopWidthType* pType,Float64* pLeftStart, Float64* pRightStart,Float64* pLeftEnd,Float64* pRightEnd) const
{
   if (m_pBridgeDesc && IsBridgeSpacing(m_pBridgeDesc->GetGirderSpacingType()))
   {
      *pFirstGdrIdx = 0;
      *pLastGdrIdx = m_pBridgeDesc->GetGirderCount() - 1;
      m_pBridgeDesc->GetGirderTopWidth(pType,pLeftStart,pRightStart);
      *pLeftEnd = *pLeftStart;
      *pRightEnd = *pRightStart;
   }
   else
   {
      _ASSERT(groupIdx < (SpacingIndexType)m_GirderTopWidthGroups.size());
      GirderGroup group = m_GirderTopWidthGroups[groupIdx];

      *pFirstGdrIdx = group.first;
      *pLastGdrIdx = group.second;
      m_Girders[group.first]->GetTopWidth(pType,pLeftStart,pRightStart,pLeftEnd,pRightEnd);

#if defined _DEBUG
      // make sure every girder in the group has the same name
      for (GirderIndexType i = group.first + 1; i <= group.second; i++)
      {
         pgsTypes::TopWidthType type;
         Float64 leftStart, rightStart, leftEnd, rightEnd;
         m_Girders[i]->GetTopWidth(&type, &leftStart, &rightStart, &leftEnd, &rightEnd);
         ATLASSERT(type == *pType);
         ATLASSERT(IsEqual(*pLeftStart, leftStart));
         ATLASSERT(IsEqual(*pLeftEnd, leftEnd));
         if (type == pgsTypes::twtAsymmetric)
         {
            ATLASSERT(IsEqual(*pRightStart, rightStart));
            ATLASSERT(IsEqual(*pRightEnd, rightEnd));
         }
      }
#endif
   }
}

CGirderTopWidthGroup CGirderGroupData::GetGirderTopWidthGroup(GroupIndexType groupIdx) const
{
   CGirderTopWidthGroup group;
   GetGirderTopWidthGroup(groupIdx, &group.firstGdrIdx, &group.lastGdrIdx, &group.type, &group.left[pgsTypes::metStart], &group.right[pgsTypes::metStart], &group.left[pgsTypes::metEnd], &group.right[pgsTypes::metEnd]);
   return group;
}

std::vector<CGirderTopWidthGroup>CGirderGroupData::GetGirderTopWidthGroups() const
{
   std::vector<CGirderTopWidthGroup> groups;
   for (const auto& p : m_GirderTopWidthGroups)
   {
      CGirderTopWidthGroup group;
      group.firstGdrIdx = p.first;
      group.lastGdrIdx = p.second;
      m_Girders[p.first]->GetTopWidth(&group.type, &group.left[pgsTypes::metStart], &group.right[pgsTypes::metStart], &group.left[pgsTypes::metEnd], &group.right[pgsTypes::metEnd]);
      groups.push_back(group);
   }
   return groups;
}

void CGirderGroupData::SetGirderTopWidthGroups(const std::vector<CGirderTopWidthGroup>& girderTopWidthGroups)
{
#if defined _DEBUG
   GirderIndexType firstGdrIdx = girderTopWidthGroups.front().firstGdrIdx;
   GirderIndexType lastGdrIdx = girderTopWidthGroups.back().lastGdrIdx;
   GirderIndexType nGirders = lastGdrIdx - firstGdrIdx + 1;
   ATLASSERT(nGirders == m_Girders.size());
#endif

   m_GirderTopWidthGroups.clear();
   for (const auto& typeGroup : girderTopWidthGroups)
   {
      GirderGroup group;
      group.first = typeGroup.firstGdrIdx;
      group.second = typeGroup.lastGdrIdx;
      m_GirderTopWidthGroups.push_back(group);
      for (GirderIndexType gdrIdx = typeGroup.firstGdrIdx; gdrIdx <= typeGroup.lastGdrIdx; gdrIdx++)
      {
         m_Girders[gdrIdx]->SetTopWidth(typeGroup.type, typeGroup.left[pgsTypes::metStart], typeGroup.right[pgsTypes::metStart], typeGroup.left[pgsTypes::metEnd], typeGroup.right[pgsTypes::metEnd]);
      }
   }
}

GroupIndexType CGirderGroupData::FindGirderTopWidthGroup(GirderIndexType gdrIdx) const
{
   if (m_pBridgeDesc && IsBridgeSpacing(m_pBridgeDesc->GetGirderSpacingType()))
   {
      return 0; // can't create a new group... return group 0
   }

   return FindGroup(gdrIdx, m_GirderTopWidthGroups);
}

void CGirderGroupData::ExpandAllGirderTopWidthGroups()
{
   ExpandAll(&m_GirderTopWidthGroups);
}

void CGirderGroupData::ExpandGirderTopWidthGroup(GroupIndexType girderTopWidthGroupIdx)
{
   Expand(girderTopWidthGroupIdx, &m_GirderTopWidthGroups);
}

void CGirderGroupData::JoinAllGirderTopWidthGroups(GirderIndexType gdrIdx)
{
   // this is almost identical to JoinAll for girder type(name).... 
   // the functions could problem be combined into one if I could figure out how
   // to pass a lambda expression to get the name/libentry/wtf and set them in the loop at the end
   if (m_Girders.size() == 0)
   {
      return;
   }

   pgsTypes::TopWidthType type;
   Float64 leftStart, rightStart, leftEnd, rightEnd;
   m_Girders[gdrIdx]->GetTopWidth(&type,&leftStart,&rightStart,&leftEnd,&rightEnd);

   GirderGroup firstGroup = m_GirderTopWidthGroups.front();
   GirderGroup lastGroup = m_GirderTopWidthGroups.back();

   GirderGroup joinedGroup;
   joinedGroup.first = firstGroup.first;
   joinedGroup.second = lastGroup.second;

   m_GirderTopWidthGroups.clear();
   m_GirderTopWidthGroups.push_back(joinedGroup);

   // make girder name and lib entry the same for all members of the group
   std::vector<CSplicedGirderData*>::iterator iter(m_Girders.begin());
   std::vector<CSplicedGirderData*>::iterator iterEnd(m_Girders.end());
   for (; iter != iterEnd; iter++)
   {
      CSplicedGirderData* pGirder = *iter;
      pGirder->SetTopWidth(type,leftStart,rightStart,leftEnd,rightEnd);
   }
   PGS_ASSERT_VALID;
}

void CGirderGroupData::JoinGirderTopWidthGroup(GirderIndexType firstGdrIdx, GirderIndexType lastGdrIdx, GirderIndexType gdrIdx)
{
   // girder index must be in the range
   _ASSERT(firstGdrIdx <= gdrIdx && gdrIdx <= lastGdrIdx);

   // get the girder name for the group
   pgsTypes::TopWidthType type;
   Float64 leftStart, rightStart, leftEnd, rightEnd;
   m_Girders[gdrIdx]->GetTopWidth(&type,&leftStart,&rightStart,&leftEnd,&rightEnd);

   // assign the name for the group
   for (GirderIndexType i = firstGdrIdx; i <= lastGdrIdx; i++)
   {
      m_Girders[i]->SetTopWidth(type,leftStart,rightStart,leftEnd,rightEnd);
   }

   Join(firstGdrIdx, lastGdrIdx, gdrIdx, &m_GirderTopWidthGroups);
}

void CGirderGroupData::SetGirderTopWidth(GroupIndexType groupIdx, pgsTypes::TopWidthType type,Float64 leftStart,Float64 rightStart,Float64 leftEnd,Float64 rightEnd)
{
   pgsTypes::TopWidthType sourceGroupType;
   Float64 sourceGroupLeftStart, sourceGroupRightStart, sourceGroupLeftEnd, sourceGroupRightEnd;
   GirderIndexType firstGdrIdx, lastGdrIdx;
   GetGirderTopWidthGroup(groupIdx, &firstGdrIdx, &lastGdrIdx, &sourceGroupType, &sourceGroupLeftStart, &sourceGroupRightStart, &sourceGroupLeftEnd, &sourceGroupRightEnd);

   for (GirderIndexType gdrIdx = firstGdrIdx; gdrIdx <= lastGdrIdx; gdrIdx++)
   {
      m_Girders[gdrIdx]->SetTopWidth(type,leftStart,rightStart,leftEnd,rightEnd);
   }
}

void CGirderGroupData::GetGirderTopWidth(GirderIndexType gdrIdx, pgsTypes::TopWidthType* pType, Float64* pLeftStart, Float64* pRightStart,Float64* pLeftEnd,Float64* pRightEnd) const
{
   if (m_pBridgeDesc && IsBridgeSpacing(m_pBridgeDesc->GetGirderSpacingType()) )
   {
      m_pBridgeDesc->GetGirderTopWidth(pType, pLeftStart, pRightStart);
      *pLeftEnd = *pLeftStart;
      *pRightEnd = *pRightStart;
   }
   else
   {
      m_Girders[gdrIdx]->GetTopWidth(pType, pLeftStart, pRightStart, pLeftEnd, pRightEnd);
   }
}

//-----------------------------------------------
void CGirderGroupData::CreateGirderGroup(GirderIndexType firstGdrIdx, GirderIndexType lastGdrIdx,std::vector<GirderGroup>* pGroups,GroupIndexType* pNewGroupIdx)
{
   GroupIndexType newGroupIdx = INVALID_INDEX;
   
   std::vector<GirderGroup> gdrGroups;
   
   std::vector<GirderGroup>::const_iterator iter;
   for (iter = pGroups->cbegin(); iter != pGroups->cend(); iter++)
   {
      GirderGroup gdrGroup = *iter; // we want a copy so do a direct assignment, don't use a reference
      if (gdrGroup.first == firstGdrIdx && gdrGroup.second == lastGdrIdx)
      {
         // no need to create a new group
         *pNewGroupIdx = iter - pGroups->cbegin();
         return;
      }

      if (gdrGroup.first < firstGdrIdx && firstGdrIdx <= gdrGroup.second)
      {
         // the new group starts in the middle of this group
         gdrGroup.second = firstGdrIdx - 1; // set the end of this group one girder before the 
         gdrGroups.push_back(gdrGroup);
         break;
      }
      else if (lastGdrIdx <= gdrGroup.second)
      {
         // the new group is totally within this group
         if (gdrGroup.first == firstGdrIdx)
         {
            GirderGroup newGrp;
            newGrp.first = firstGdrIdx;
            newGrp.second = lastGdrIdx;
            gdrGroups.push_back(newGrp);
            newGroupIdx = gdrGroups.size() - 1;

            newGrp.first = lastGdrIdx + 1;
            newGrp.second = gdrGroup.second;
            gdrGroups.push_back(newGrp);
         }
         else if (gdrGroup.second == lastGdrIdx)
         {
            GirderGroup newGrp;
            newGrp.first = gdrGroup.first;
            newGrp.second = firstGdrIdx - 1;
            gdrGroups.push_back(newGrp);

            newGrp.first = firstGdrIdx;
            newGrp.second = lastGdrIdx;
            gdrGroups.push_back(newGrp);
            newGroupIdx = gdrGroups.size() - 1;
         }
         else
         {
            GirderGroup newGrp;
            newGrp.first = gdrGroup.first;
            newGrp.second = firstGdrIdx - 1;
            gdrGroups.push_back(newGrp);

            newGrp.first = firstGdrIdx;
            newGrp.second = lastGdrIdx;
            gdrGroups.push_back(newGrp);
            newGroupIdx = gdrGroups.size() - 1;

            newGrp.first = lastGdrIdx + 1;
            newGrp.second = gdrGroup.second;
            gdrGroups.push_back(newGrp);
         }
         iter++;
         break;
      }
      else
      {
         gdrGroups.push_back(gdrGroup);
      }
   }

   for (iter; iter != pGroups->end(); iter++)
   {
      GirderGroup gdrGroup = *iter;
      if (gdrGroup.first < lastGdrIdx && lastGdrIdx <= gdrGroup.second)
      {
         // the new group ends in the middle of this group

         // add the new group
         gdrGroup.first = firstGdrIdx;
         gdrGroup.second = lastGdrIdx;
         gdrGroups.push_back(gdrGroup);
         newGroupIdx = gdrGroups.size() - 1;

         // get the current group back
         gdrGroup = *iter;
         gdrGroup.first = lastGdrIdx + 1; // group begins one girder after the last group
         if (gdrGroup.first <= gdrGroup.second)
         {
            gdrGroups.push_back(gdrGroup); // if last is < first, then this isn't a group... the new group ends at the last girder
         }
      }
      else
      {
         gdrGroups.push_back(gdrGroup); // save the rest of the groups
      }
   }

   *pGroups = gdrGroups;
   *pNewGroupIdx = newGroupIdx;
   PGS_ASSERT_VALID;
}

void CGirderGroupData::ExpandAll(std::vector<GirderGroup>* pGroups)
{
   pGroups->clear();
   GirderIndexType nGirders = m_Girders.size();
   for (GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++)
   {
      pGroups->emplace_back(gdrIdx, gdrIdx);
   }
   PGS_ASSERT_VALID;
}

void CGirderGroupData::Expand(GroupIndexType groupIdx, std::vector<GirderGroup>* pGroups)
{
   GirderIndexType firstGdrIdx, lastGdrIdx;
   firstGdrIdx = (*pGroups)[groupIdx].first;
   lastGdrIdx = (*pGroups)[groupIdx].second;

   if (firstGdrIdx == lastGdrIdx)
   {
      return; // nothing to expand
   }

   std::vector<GirderGroup>::iterator iter = pGroups->begin() + groupIdx;
   std::vector<GirderGroup>::iterator pos = pGroups->erase(iter); // returns the iter the element after the one removed

                                                                                // inserted element goes before the iterator... insert retuns position of newly inserted item
                                                                                // go in reverse order
   for (GirderIndexType gdrIdx = lastGdrIdx; firstGdrIdx <= gdrIdx && gdrIdx != INVALID_INDEX; gdrIdx--)
   {
      GirderGroup group(gdrIdx, gdrIdx);
      pos = pGroups->insert(pos, group);
   }
   PGS_ASSERT_VALID;
}

void CGirderGroupData::Join(GirderIndexType firstGdrIdx, GirderIndexType lastGdrIdx, GirderIndexType gdrIdx, std::vector<GirderGroup>* pGroups)
{
   // girder index must be in the range
   _ASSERT(firstGdrIdx <= gdrIdx && gdrIdx <= lastGdrIdx);

   // firstGdrIdx must match the "first" parameter of a GirderTypeGroup
   // lastGdrIdx  must match the "last"  parameter of a GirderTypeGroup
   // this is the way the UI grid works so we don't need to allow
   // for any more complicated joining than this.

   // create a local girder groups container. It is easier to fill it up as we go
   // rather than manipulating the class data member... update the class data member
   // at the end of this function
   std::vector<GirderGroup> gdrGroups;

   // loop until the first index in a group matches firstGdrIdx
   // save any group that comes before the first group
   std::vector<GirderGroup>::iterator iter = pGroups->begin();
   std::vector<GirderGroup>::iterator end(pGroups->end());
   for (; iter != end; iter++)
   {
      GirderGroup gdrGroup = *iter;
      if (gdrGroup.first == firstGdrIdx)
      {
         break;
      }
      else
      {
         gdrGroups.push_back(gdrGroup);
      }
   }

   _ASSERT(iter != end); // shouldn't have gone through the full vector

                                              // loop until the last index in a group matches lastGdrIdx
                                              // don't save any groups in the middle... these are the groups
                                              // being joined
   for (; iter != end; iter++)
   {
      GirderGroup gdrGroup = *iter;
      if (gdrGroup.second == lastGdrIdx)
      {
         iter++; // move iter to next position... breaking out of the loop skips the incrementer
         break;
      }
   }

   // save the new group
   GirderGroup newGroup(firstGdrIdx, lastGdrIdx);
   gdrGroups.push_back(newGroup);

   // copy the remaining groups
   if (iter != end)
   {
      gdrGroups.insert(gdrGroups.end(), iter, end);
   }

   // finally replace the data member with the local girder groups
   *pGroups = gdrGroups;
   PGS_ASSERT_VALID;
}

GroupIndexType CGirderGroupData::FindGroup(GirderIndexType gdrIdx, const std::vector<GirderGroup>& groups) const
{
   GroupIndexType nGroups = groups.size();
   for (GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++)
   {
      GirderIndexType firstGdrIdx, lastGdrIdx;

      GirderGroup group = groups[grpIdx];

      firstGdrIdx = group.first;
      lastGdrIdx = group.second;

      if (firstGdrIdx <= gdrIdx && gdrIdx <= lastGdrIdx)
      {
         return grpIdx;
      }
   }

   ATLASSERT(false); // should never get here
   return INVALID_INDEX;
}

//-----------------------------------------------

void CGirderGroupData::SetGirderLibraryEntry(GroupIndexType grpIdx,const GirderLibraryEntry* pEntry)
{
   std::_tstring strGirder;
   GirderIndexType firstGdrIdx, lastGdrIdx;
   GetGirderTypeGroup(grpIdx, &firstGdrIdx, &lastGdrIdx, &strGirder);

   for (GirderIndexType gdrIdx = firstGdrIdx; gdrIdx <= lastGdrIdx; gdrIdx++)
   {
      m_Girders[gdrIdx]->SetGirderLibraryEntry(pEntry);
   }
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

      Float64 version;
      pStrLoad->get_Version(&version);

      var.vt = VT_ID;
      hr = pStrLoad->get_Property(_T("ID"),&var);
      m_GroupID = VARIANT2ID(var);

      var.vt = VT_INDEX;
      hr = pStrLoad->get_Property(_T("StartPier"),&var);
      m_PierIndex[pgsTypes::metStart] = VARIANT2INDEX(var);

      hr = pStrLoad->get_Property(_T("EndPier"),&var);
      m_PierIndex[pgsTypes::metEnd] = VARIANT2INDEX(var);

      UpdatePiers();

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
         m_pBridgeDesc->UpdateNextGirderID(pGirder->GetID());
      }

      hr = pStrLoad->EndUnit(); // Girders

      hr = pStrLoad->BeginUnit(_T("GirderTypeGroups"));
      var.vt = VT_INDEX;
      hr = pStrLoad->get_Property(_T("Count"),&var);
      GroupIndexType nGirderTypeGroups = VARIANT2INDEX(var);
      for ( GroupIndexType gdrTypeGroupIdx = 0; gdrTypeGroupIdx < nGirderTypeGroups; gdrTypeGroupIdx++ )
      {
         GirderGroup group;

         hr = pStrLoad->BeginUnit(_T("GirderTypeGroup"));
         
         hr = pStrLoad->get_Property(_T("FirstGirderIndex"),&var);
         group.first = VARIANT2INDEX(var);

         hr = pStrLoad->get_Property(_T("LastGirderIndex"),&var);
         group.second = VARIANT2INDEX(var);

         m_GirderTypeGroups.push_back(group);

         hr = pStrLoad->EndUnit(); // GirderTypeGroup
      }
      hr = pStrLoad->EndUnit(); // GirderTypeGroups

      if (1 < version && !IsBridgeSpacing(m_pBridgeDesc->GetGirderSpacingType()) && IsTopWidthSpacing(m_pBridgeDesc->GetGirderSpacingType()) )
      {
         // added in version
         hr = pStrLoad->BeginUnit(_T("GirderTopWidthGroups"));
         var.vt = VT_INDEX;
         hr = pStrLoad->get_Property(_T("Count"), &var);
         GroupIndexType nGirderTypeGroups = VARIANT2INDEX(var);
         for (GroupIndexType gdrTypeGroupIdx = 0; gdrTypeGroupIdx < nGirderTypeGroups; gdrTypeGroupIdx++)
         {
            GirderGroup group;

            hr = pStrLoad->BeginUnit(_T("GirderTopWidthGroup"));

            hr = pStrLoad->get_Property(_T("FirstGirderIndex"), &var);
            group.first = VARIANT2INDEX(var);

            hr = pStrLoad->get_Property(_T("LastGirderIndex"), &var);
            group.second = VARIANT2INDEX(var);

            m_GirderTopWidthGroups.push_back(group);

            hr = pStrLoad->EndUnit(); // GirderTopWidthGroup
         }
         hr = pStrLoad->EndUnit(); // GirderTopWidthGroups
      }
      else
      {
         GirderIndexType nGirders = m_Girders.size();
         for (GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++)
         {
            m_GirderTopWidthGroups.push_back(GirderGroup(gdrIdx,gdrIdx));
         }
      }

      if ( ::IsBridgeSpacing(m_pBridgeDesc->GetGirderSpacingType()) )
      {
         // If the girder spacing type is a bridge spacing then individual girder spaces aren't loaded by
         // the CPierData2 objects. The girder spacing objects need to have spacing that is consistent
         // with the number of girders. Set those values here.
         Float64 bridgeSpacing = m_pBridgeDesc->GetGirderSpacing();
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
      // Slab offset removed from girder group data in version 3 of this data block
      // map the old data into the new locations where slab offsets are stored
      if (version < 3)
      {
         if (m_pBridgeDesc->GetSlabOffsetType() != pgsTypes::sotBridge)
         {
            hr = pStrLoad->BeginUnit(_T("SlabOffset"));
            if (m_pBridgeDesc->GetSlabOffsetType() == pgsTypes::sotBearingLine)
            {
               // Slab offset per pier (one value for all girders)
               PierIndexType startPierIdx = GetPierIndex(pgsTypes::metStart);
               PierIndexType endPierIdx = GetPierIndex(pgsTypes::metEnd);
               for (PierIndexType pierIdx = startPierIdx; pierIdx <= endPierIdx; pierIdx++)
               {
                  var.vt = VT_R8;
                  hr = pStrLoad->get_Property(_T("SlabOffset"), &var);
                  Float64 slabOffset = var.dblVal;

                  CPierData2* pPier = m_pBridgeDesc->GetPier(pierIdx);

                  if (pierIdx == startPierIdx)
                  {
                     pPier->SetSlabOffset(pgsTypes::Ahead, slabOffset);

                     // the old implementation copied the slab offset down to the individual girders... we do the same here
                     GirderIndexType nGirders = GetGirderCount();
                     for (GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++)
                     {
                        CSplicedGirderData* pGirder = GetGirder(gdrIdx);
                        pGirder->GetSegment(0)->SetSlabOffset(pgsTypes::metStart, slabOffset);
                     }
                  }
                  else if (pierIdx == endPierIdx)
                  {
                     pPier->SetSlabOffset(pgsTypes::Back, slabOffset);

                     // the old implementation copied the slab offset down to the individual girders... we do the same here
                     GirderIndexType nGirders = GetGirderCount();
                     for (GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++)
                     {
                        CSplicedGirderData* pGirder = GetGirder(gdrIdx);
                        SegmentIndexType nSegments = pGirder->GetSegmentCount();
                        pGirder->GetSegment(nSegments-1)->SetSlabOffset(pgsTypes::metEnd, slabOffset);
                     }
                  }
                  else
                  {
                     pPier->SetSlabOffset(slabOffset, slabOffset);

                     // the old implementation copied the slab offset down to the individual girders... we do the same here
                     // we know all girders in the group have the same number of segments so we will work by segment along
                     // girder 0 until we find a place that the end of the segment is supported by pPier. Once we find
                     // that segment, set the slab offset at the appropreate end of that segment in all girders across the group
                     GirderIndexType nGirders = GetGirderCount();
                     CSplicedGirderData* pGirder = GetGirder(0); // girder girder 0
                     SegmentIndexType nSegments = pGirder->GetSegmentCount();
                     for (SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++) // work our way down the segments in girder 0
                     {
                        CPrecastSegmentData* pSegment = pGirder->GetSegment(segIdx);
                        for (int i = 0; i < 2; i++) // check both ends
                        {
                           pgsTypes::MemberEndType end = pgsTypes::MemberEndType(i);
                           CPierData2* pSupportingPier;
                           CTemporarySupportData* pTS;
                           pSegment->GetSupport(end, &pSupportingPier, &pTS);
                           if (pSupportingPier && pSupportingPier->GetIndex() == pierIdx)
                           {
                              // the current end is supported by pPier so set the slab offset for this segment in all girders
                              for (GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++)
                              {
                                 GetGirder(gdrIdx)->GetSegment(segIdx)->SetSlabOffset(end, slabOffset);
                              } // next girder
                           } // supported by our pier
                        } // next end
                     } // next segment
                  }
               }
            }
            else
            {
               ATLASSERT(m_pBridgeDesc->GetSlabOffsetType() == pgsTypes::sotSegment);
               PierIndexType startPierIdx = GetPierIndex(pgsTypes::metStart);
               PierIndexType endPierIdx = GetPierIndex(pgsTypes::metEnd);
               for (PierIndexType pierIdx = startPierIdx; pierIdx <= endPierIdx; pierIdx++)
               {
                  hr = pStrLoad->BeginUnit(_T("GirderSlabOffsets"));

                  for (GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++)
                  {
                     var.vt = VT_R8;
                     hr = pStrLoad->get_Property(_T("SlabOffset"), &var);
                     Float64 slabOffset = var.dblVal;

                     CSplicedGirderData* pGirder = GetGirder(gdrIdx);

                     SegmentIndexType nSegments = pGirder->GetSegmentCount();
                     for (SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++)
                     {
                        CPrecastSegmentData* pSegment = pGirder->GetSegment(segIdx);

                        if (pierIdx == startPierIdx)
                        {
                           pSegment->SetSlabOffset(pgsTypes::metStart, slabOffset);

                           if (gdrIdx == 0)
                           {
                              CPierData2* pSupportingPier;
                              CTemporarySupportData* pTS;
                              pSegment->GetSupport(pgsTypes::metStart, &pSupportingPier, &pTS);
                              if (pSupportingPier && pSupportingPier->GetIndex() == pierIdx)
                              {
                                 pSupportingPier->SetSlabOffset(pgsTypes::Ahead, slabOffset);
                              }
                           }
                        }
                        else if (pierIdx == endPierIdx)
                        {
                           pSegment->SetSlabOffset(pgsTypes::metEnd, slabOffset);

                           if (gdrIdx == 0)
                           {
                              CPierData2* pSupportingPier;
                              CTemporarySupportData* pTS;
                              pSegment->GetSupport(pgsTypes::metEnd, &pSupportingPier, &pTS);
                              if (pSupportingPier && pSupportingPier->GetIndex() == pierIdx)
                              {
                                 pSupportingPier->SetSlabOffset(pgsTypes::Back, slabOffset);
                              }
                           }
                        }
                        else
                        {
                           pSegment->SetSlabOffset(slabOffset, slabOffset);

                           if (gdrIdx == 0)
                           {
                              for (int i = 0; i < 2; i++)
                              {
                                 pgsTypes::MemberEndType end = (pgsTypes::MemberEndType)i;
                                 CPierData2* pSupportingPier;
                                 CTemporarySupportData* pTS;
                                 pSegment->GetSupport(end, &pSupportingPier, &pTS);
                                 if (pSupportingPier && pSupportingPier->GetIndex() == pierIdx)
                                 {
                                    pSupportingPier->SetSlabOffset((pgsTypes::PierFaceType)end, slabOffset); // NOTE: metStart = Ahead and metEnd = Back
                                 }
                              }
                           }
                        }
                     }
                  }

                  hr = pStrLoad->EndUnit();
               }
            }
            hr = pStrLoad->EndUnit(); // SlabOffset
         }
      }

      hr = pStrLoad->EndUnit(); // GirderGroup
   }
   catch (HRESULT)
   {
      ATLASSERT(false);
      THROW_LOAD(InvalidFileFormat,pStrLoad);
   }

   RepairGirderTypeGroups(); // some sequence in editing causes the girder group data to get messed up and the bad data is stored... this methods attempts to fix bad data

   return S_OK;
}

HRESULT CGirderGroupData::Save(IStructuredSave* pStrSave,IProgress* pProgress)
{
   pStrSave->BeginUnit(_T("GirderGroup"),3.0);

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
   std::vector<GirderGroup>::iterator grpIter(m_GirderTypeGroups.begin());
   std::vector<GirderGroup>::iterator grpIterEnd(m_GirderTypeGroups.end());
   for ( ; grpIter != grpIterEnd; grpIter++ )
   {
      GirderGroup group = *grpIter;
      pStrSave->BeginUnit(_T("GirderTypeGroup"),1.0);
      pStrSave->put_Property(_T("FirstGirderIndex"),CComVariant(group.first));
      pStrSave->put_Property(_T("LastGirderIndex"),CComVariant(group.second));
      pStrSave->EndUnit(); // GirderTypeGroup
   }
   pStrSave->EndUnit(); // GirderTypeGroups

   // added in version 2.0
   if ( !IsBridgeSpacing(m_pBridgeDesc->GetGirderSpacingType()) && IsTopWidthSpacing(m_pBridgeDesc->GetGirderSpacingType()))
   {
      pStrSave->BeginUnit(_T("GirderTopWidthGroups"), 1.0);
      pStrSave->put_Property(_T("Count"), CComVariant(m_GirderTopWidthGroups.size()));
      std::vector<GirderGroup>::iterator grpIter(m_GirderTopWidthGroups.begin());
      std::vector<GirderGroup>::iterator grpIterEnd(m_GirderTopWidthGroups.end());
      for (; grpIter != grpIterEnd; grpIter++)
      {
         GirderGroup group = *grpIter;
         pStrSave->BeginUnit(_T("GirderTopWidthGroup"), 1.0);
         pStrSave->put_Property(_T("FirstGirderIndex"), CComVariant(group.first));
         pStrSave->put_Property(_T("LastGirderIndex"), CComVariant(group.second));
         pStrSave->EndUnit(); // GirderTopWidthGroup
      }
      pStrSave->EndUnit(); // GirderTopWidthGroups
   }

   // Slab Offset
   // Removed in version 3
   //if (m_pBridgeDesc->GetSlabOffsetType() != pgsTypes::sotBridge )
   //{
   //   pStrSave->BeginUnit(_T("SlabOffset"),1.0);
   //   if ( m_pBridgeDesc->GetSlabOffsetType() == pgsTypes::sotBearingLine )
   //   {
   //      // Slab offset per pier (one value for all girders)
   //      PierIndexType startPierIdx = GetPierIndex(pgsTypes::metStart);
   //      PierIndexType endPierIdx   = GetPierIndex(pgsTypes::metEnd);
   //      IndexType index = 0;
   //      for ( PierIndexType pierIdx = startPierIdx; pierIdx <= endPierIdx; pierIdx++, index++ )
   //      {
   //         std::vector<Float64>& vSlabOffset(m_SlabOffsets[index]);
   //         pStrSave->put_Property(_T("SlabOffset"),CComVariant(vSlabOffset.front()));
   //      }
   //   }
   //   else
   //   {
   //      ATLASSERT(m_pBridgeDesc->GetSlabOffsetType() == pgsTypes::sotSegment);
   //      PierIndexType startPierIdx = GetPierIndex(pgsTypes::metStart);
   //      PierIndexType endPierIdx   = GetPierIndex(pgsTypes::metEnd);
   //      IndexType index = 0;
   //      for ( PierIndexType pierIdx = startPierIdx; pierIdx <= endPierIdx; pierIdx++, index++ )
   //      {
   //         pStrSave->BeginUnit(_T("GirderSlabOffsets"),1.0);

   //         std::vector<Float64>& vSlabOffset(m_SlabOffsets[index]);
   //         std::vector<Float64>::iterator iter(vSlabOffset.begin());
   //         std::vector<Float64>::iterator end(vSlabOffset.end());
   //         for ( ; iter != end; iter++ )
   //         {
   //            pStrSave->put_Property(_T("SlabOffset"),CComVariant(*iter));
   //         }

   //         pStrSave->EndUnit();
   //      }
   //   }
   //   pStrSave->EndUnit(); // SlabOffset
   //}

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

   m_pPier[pgsTypes::metStart] = nullptr;
   m_pPier[pgsTypes::metEnd]   = nullptr;
   m_PierIndex[pgsTypes::metStart] = rOther.GetPierIndex(pgsTypes::metStart);
   m_PierIndex[pgsTypes::metEnd]   = rOther.GetPierIndex(pgsTypes::metEnd);
   UpdatePiers();

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
   std::vector<CSplicedGirderData*> vOldGirders;
   for ( ; myGirderIter != myGirderIterEnd; myGirderIter++, otherGirderIter++ )
   {
      CSplicedGirderData* pMyGirder = *myGirderIter; // this is my girder... we are replacing it with a copy of the other girder
      GirderIndexType myGirderIdx = INVALID_INDEX;
      GirderIDType myGirderID = INVALID_ID;
      if ( pMyGirder )
      {
         myGirderIdx = pMyGirder->GetIndex();
         myGirderID = pMyGirder->GetID();
         vOldGirders.push_back(pMyGirder);
      }
      pMyGirder = nullptr;
      *myGirderIter = nullptr;

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

   m_GirderTypeGroups = rOther.m_GirderTypeGroups;
   //m_GirderTypeGroups.clear();
   //std::vector<GirderGroup>::const_iterator grpIter(rOther.m_GirderTypeGroups.begin());
   //std::vector<GirderGroup>::const_iterator grpIterEnd(rOther.m_GirderTypeGroups.end());
   //for ( ; grpIter != grpIterEnd; grpIter++ )
   //{
   //   const GirderGroup& grp = *grpIter;
   //   m_GirderTypeGroups.push_back(grp);
   //}

   m_GirderTopWidthGroups = rOther.m_GirderTopWidthGroups;

   // delete the old girders
   std::vector<CSplicedGirderData*>::iterator oldGirderIter(vOldGirders.begin());
   std::vector<CSplicedGirderData*>::iterator oldGirderIterEnd(vOldGirders.end());
   for ( ; oldGirderIter != oldGirderIterEnd; oldGirderIter++ )
   {
      CSplicedGirderData* pOldGirder = *oldGirderIter;
      delete pOldGirder; // done with this girder
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

void CGirderGroupData::RepairGirderTypeGroups()
{
   // There is something that causes girder type group data to get messed up
   // this method repairs the problem
   bool bNeedsRepair = false;

   if (m_Girders.size() < m_GirderTypeGroups.size())
   {
      bNeedsRepair = true;
   }

   if (1 < m_GirderTypeGroups.size() && bNeedsRepair == false)
   {
      auto iter1 = m_GirderTypeGroups.begin();
      auto iter2 = iter1 + 1;
      auto end = m_GirderTypeGroups.end();
      for (; iter2 != end; iter1++, iter2++)
      {
         if ((iter1->second < iter1->first) || (iter2->second < iter2->first) || (iter2->first <= iter1->second))
         {
            bNeedsRepair = true;
            break;
         }
      }
   }

   if (bNeedsRepair)
   {
      ExpandAll();
   }
}

void CGirderGroupData::Clear()
{
   std::for_each(std::begin(m_Girders), std::end(m_Girders), [](auto* pGirder) {pGirder->Clear(); });
   // NOTE: the girder pointers are deleted and the m_Girders collection cleared in the destructor

   m_GirderTypeGroups.clear();
   m_GirderTopWidthGroups.clear();
}

void CGirderGroupData::UpdatePiers()
{
   if ( m_pBridgeDesc != nullptr && (m_pPier[pgsTypes::metStart] == nullptr || m_pPier[pgsTypes::metEnd] == nullptr) )
   {
      ATLASSERT(m_PierIndex[pgsTypes::metStart] != INVALID_INDEX);
      ATLASSERT(m_PierIndex[pgsTypes::metEnd]   != INVALID_INDEX);

      m_pPier[pgsTypes::metStart] = m_pBridgeDesc->GetPier(m_PierIndex[pgsTypes::metStart]);
      m_pPier[pgsTypes::metEnd]   = m_pBridgeDesc->GetPier(m_PierIndex[pgsTypes::metEnd]);

      m_PierIndex[pgsTypes::metStart] = INVALID_INDEX;
      m_PierIndex[pgsTypes::metEnd]   = INVALID_INDEX;
   }
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

   // Girder type groups must be consistent with number of girders
   if ( 0 < m_Girders.size() )
   {
      ATLASSERT(m_GirderTypeGroups.size() != 0);
      ATLASSERT(m_GirderTypeGroups.size() <= m_Girders.size());
      ATLASSERT(m_GirderTypeGroups.back().second - m_GirderTypeGroups.front().first == m_Girders.size() - 1);
      ATLASSERT(m_GirderTopWidthGroups.size() != 0);
      ATLASSERT(m_GirderTopWidthGroups.size() <= m_Girders.size());
      ATLASSERT(m_GirderTopWidthGroups.back().second - m_GirderTopWidthGroups.front().first == m_Girders.size() - 1);

      if (1 < m_GirderTypeGroups.size())
      {
         auto iter1 = m_GirderTypeGroups.begin();
         auto iter2 = iter1 + 1;
         auto end = m_GirderTypeGroups.end();
         for (; iter2 != end; iter1++, iter2++)
         {
            ATLASSERT(iter1->first <= iter1->second);
            ATLASSERT(iter2->first <= iter2->second);
            ATLASSERT(iter1->second < iter2->first);
         }
      }

      if (1 < m_GirderTopWidthGroups.size())
      {
         auto iter1 = m_GirderTopWidthGroups.begin();
         auto iter2 = iter1 + 1;
         auto end = m_GirderTopWidthGroups.end();
         for (; iter2 != end; iter1++, iter2++)
         {
            ATLASSERT(iter1->first <= iter1->second);
            ATLASSERT(iter2->first <= iter2->second);
            ATLASSERT(iter1->second < iter2->first);
         }
      }
   }

   if ( m_pBridgeDesc && m_pPier[pgsTypes::metStart] )
   {
      ATLASSERT( m_pBridgeDesc == m_pPier[pgsTypes::metStart]->GetBridgeDescription() );
      ATLASSERT( m_pPier[pgsTypes::metStart]->IsBoundaryPier() );
   }

   if ( m_pBridgeDesc && m_pPier[pgsTypes::metEnd] )
   {
      ATLASSERT( m_pBridgeDesc == m_pPier[pgsTypes::metEnd]->GetBridgeDescription() );
      ATLASSERT( m_pPier[pgsTypes::metEnd]->IsBoundaryPier() );
   }

   ATLASSERT(m_Girders.size()<=1 || m_pPier[pgsTypes::metStart]->GetGirderSpacing(pgsTypes::Ahead)->GetSpacingCount()+1 == m_Girders.size());
   ATLASSERT(m_Girders.size()<=1 || m_pPier[pgsTypes::metEnd  ]->GetGirderSpacing(pgsTypes::Back )->GetSpacingCount()+1 == m_Girders.size());
}
#endif // _DEBUG
