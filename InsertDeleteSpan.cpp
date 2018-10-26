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

#include "PGSuperAppPlugin\stdafx.h"
#include "InsertDeleteSpan.h"
#include "PGSuperDocBase.h"

#include <PgsExt\ClosureJointData.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

txnInsertSpan::txnInsertSpan(PierIndexType refPierIdx,pgsTypes::PierFaceType pierFace,Float64 spanLength,bool bCreateNewGroup,IndexType pierErectionEventIdx)
{
   m_RefPierIdx = refPierIdx;
   m_PierFace   = pierFace;
   m_SpanLength = spanLength;
   m_bCreateNewGroup = bCreateNewGroup;
   m_PierErectionEventIndex = pierErectionEventIdx;
}

std::_tstring txnInsertSpan::Name() const
{
   return _T("Insert Span");
}

txnTransaction* txnInsertSpan::CreateClone() const
{
   return new txnInsertSpan(m_RefPierIdx, m_PierFace,m_SpanLength,m_bCreateNewGroup,m_PierErectionEventIndex);
}

bool txnInsertSpan::IsUndoable()
{
   return true;
}

bool txnInsertSpan::IsRepeatable()
{
   return false;
}

bool txnInsertSpan::Execute()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IEvents,pEvents);
   pEvents->HoldEvents();

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);

   // sometimes the connection information gets altered when adding a span... capture it here
   // so it can be reset on Undo
   const CPierData2* pPier = pIBridgeDesc->GetPier(m_RefPierIdx);
   for ( int i = 0; i < 2; i++ )
   {
      pgsTypes::PierFaceType face = (pgsTypes::PierFaceType)i;
      pPier->GetBearingOffset(face,&m_BrgOffset[face],&m_BrgOffsetMeasure[face]);
      pPier->GetGirderEndDistance(face,&m_EndDist[face],&m_EndDistMeasure[face]);
   }

   pIBridgeDesc->InsertSpan(m_RefPierIdx,m_PierFace,m_SpanLength,NULL,NULL,m_bCreateNewGroup,m_PierErectionEventIndex);

   pEvents->FirePendingEvents();

   return true;
}

void txnInsertSpan::Undo()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IEvents,pEvents);
   pEvents->HoldEvents();

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);

   PierIndexType pierIdx = m_RefPierIdx + (m_PierFace == pgsTypes::Ahead ? 1 : 0);
   pgsTypes::PierFaceType pierFace = (m_PierFace == pgsTypes::Ahead ? pgsTypes::Back : pgsTypes::Ahead);
   pIBridgeDesc->DeletePier(pierIdx,pierFace);

   // restore the connection geometry
   CBridgeDescription2 bridgeDesc = *(pIBridgeDesc->GetBridgeDescription());
   CPierData2* pPier = bridgeDesc.GetPier(m_RefPierIdx);
   for ( int i = 0; i < 2; i++ )
   {
      pgsTypes::PierFaceType face = (pgsTypes::PierFaceType)i;
      pPier->SetBearingOffset(face,m_BrgOffset[face],m_BrgOffsetMeasure[face]);
      pPier->SetGirderEndDistance(face,m_EndDist[face],m_EndDistMeasure[face]);
   }
   pIBridgeDesc->SetPierByIndex(m_RefPierIdx,*pPier);

   pEvents->FirePendingEvents();
}

///////////////////////////////////////////////

txnDeleteSpan::txnDeleteSpan(PierIndexType refPierIdx,pgsTypes::PierFaceType pierFace)
{
   m_RefPierIdx = refPierIdx;
   m_PierFace   = pierFace;
   m_pDeletedSpan = NULL;
   m_pDeletedPier = NULL;
   m_pDeletedGroup = NULL;
}

txnDeleteSpan::~txnDeleteSpan()
{
   delete m_pDeletedSpan;
   delete m_pDeletedPier;
   delete m_pDeletedGroup;
}

std::_tstring txnDeleteSpan::Name() const
{
   return _T("Delete Span");
}

txnTransaction* txnDeleteSpan::CreateClone() const
{
   return new txnDeleteSpan(m_RefPierIdx, m_PierFace);
}

bool txnDeleteSpan::IsUndoable()
{
   return true;
}

bool txnDeleteSpan::IsRepeatable()
{
   return false;
}

bool txnDeleteSpan::Execute()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   GET_IFACE2(pBroker,IEvents,pEvents);
   pEvents->HoldEvents();

   // save the span/pier that are going to be deleted for undo
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CPierData2* pPier = pBridgeDesc->GetPier(m_RefPierIdx);
   const CSpanData2* pSpan = (m_PierFace == pgsTypes::Back ? pPier->GetPrevSpan() : pPier->GetNextSpan());

   m_LastPierIdx = pBridgeDesc->GetPierCount()-1;

#if defined _DEBUG
   if ( m_RefPierIdx == 0 )
   {
      ATLASSERT(m_PierFace == pgsTypes::Ahead);
   }
   else if ( m_RefPierIdx == m_LastPierIdx )
   {
      ATLASSERT(m_PierFace == pgsTypes::Back);
   }
#endif
   const CPierData2* pOtherPier = (m_PierFace == pgsTypes::Ahead ? pBridgeDesc->GetPier(m_RefPierIdx+1) : pBridgeDesc->GetPier(m_RefPierIdx-1));
   m_OldGirderSpacing = *(pOtherPier->GetGirderSpacing(m_PierFace == pgsTypes::Ahead ? pgsTypes::Back : pgsTypes::Ahead));


   ATLASSERT(pSpan != NULL); // this should not happen

   m_DeletedPierIdx = pPier->GetIndex();
   m_pDeletedPier = new CPierData2(*pPier);

   m_DeletedSpanIdx = pSpan->GetIndex();
   m_pDeletedSpan = new CSpanData2(*pSpan);

   m_LastPierIdx = pBridgeDesc->GetPierCount()-1;

   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(pSpan);
   if ( pGroup->GetSpanCount() == 1 ? true : false )
   {
      // there is only one span in this group so the group is going to go away. need to create a new group for undo
      m_pDeletedGroup = new CGirderGroupData(*pGroup);
      m_DeletedGroupIdx = pGroup->GetIndex();

      const CTimelineManager* pTimelineMgr = pBridgeDesc->GetTimelineManager();

      GirderIndexType nGirders = pGroup->GetGirderCount();
      for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
      {
         const CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);
         SegmentIndexType nSegments = pGirder->GetSegmentCount();
         for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
         {
            const CPrecastSegmentData* pSegment = pGirder->GetSegment(segIdx);
            EventIndexType constructionEventIdx, erectionEventIdx;
            pTimelineMgr->GetSegmentEvents(pSegment->GetID(),&constructionEventIdx,&erectionEventIdx);

            m_SegmentEvents.insert(std::make_pair(CSegmentKey(m_DeletedGroupIdx,gdrIdx,segIdx),std::make_pair(constructionEventIdx,erectionEventIdx)));

            if ( segIdx < nSegments-1 )
            {
               const CClosureJointData* pClosure = pGirder->GetClosureJoint(segIdx);
               EventIndexType castClosureJointEventIdx = pTimelineMgr->GetCastClosureJointEventIndex(pClosure->GetID());
               m_ClosureJointEvents.insert(std::make_pair(CClosureKey(m_DeletedGroupIdx,gdrIdx,segIdx),castClosureJointEventIdx));
            }
         }
      }

      // need to capture events for segments in the group that are removed....
   }
   m_PierErectionEventIdx = pIBridgeDesc->GetPierErectionEvent(m_RefPierIdx);

   // save span length for undo
   m_SpanLength = pSpan->GetSpanLength();

   pIBridgeDesc->DeletePier(m_RefPierIdx,m_PierFace);

   pEvents->FirePendingEvents();

   return true;
}

void txnDeleteSpan::Undo()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IEvents,pEvents);
   pEvents->HoldEvents();

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   if ( m_RefPierIdx == 0 && m_PierFace == pgsTypes::Ahead )
   {
      pIBridgeDesc->InsertSpan(m_RefPierIdx,pgsTypes::Back,m_SpanLength,NULL,NULL,m_pDeletedGroup ? true : false,m_PierErectionEventIdx);
   }
   else if ( m_RefPierIdx == m_LastPierIdx && m_PierFace == pgsTypes::Back )
   {
      pIBridgeDesc->InsertSpan(m_RefPierIdx-1,pgsTypes::Ahead,m_SpanLength,NULL,NULL,m_pDeletedGroup ? true : false,m_PierErectionEventIdx);
   }
   else
   {
      pIBridgeDesc->InsertSpan(m_RefPierIdx - (m_PierFace == pgsTypes::Back ? 1 : 0),m_PierFace == pgsTypes::Ahead ? pgsTypes::Back : pgsTypes::Ahead,m_SpanLength,NULL,NULL,m_pDeletedGroup ? true : false,m_PierErectionEventIdx);
   }

   if ( m_pDeletedGroup )
   {
      pIBridgeDesc->SetGirderGroup(m_DeletedGroupIdx,*m_pDeletedGroup);

      std::map<CSegmentKey,std::pair<EventIndexType,EventIndexType>>::const_iterator segIter(m_SegmentEvents.begin());
      std::map<CSegmentKey,std::pair<EventIndexType,EventIndexType>>::const_iterator segIterEnd(m_SegmentEvents.end());
      for ( ; segIter != segIterEnd; segIter++ )
      {
         pIBridgeDesc->SetSegmentEventsByIndex(segIter->first,segIter->second.first,segIter->second.second);
      }

      std::map<CClosureKey,EventIndexType>::const_iterator cjIter(m_ClosureJointEvents.begin());
      std::map<CClosureKey,EventIndexType>::const_iterator cjIterEnd(m_ClosureJointEvents.end());
      for ( ; cjIter != cjIterEnd; cjIter++ )
      {
         CClosureKey key(cjIter->first);
         pIBridgeDesc->SetCastClosureJointEventByIndex(key.groupIndex,key.segmentIndex,cjIter->second);
      }
   }

   pIBridgeDesc->SetSpan(m_DeletedSpanIdx,*m_pDeletedSpan);
   pIBridgeDesc->SetPierByIndex(m_DeletedPierIdx,*m_pDeletedPier);

   pIBridgeDesc->SetGirderSpacing(m_PierFace == pgsTypes::Ahead ? m_RefPierIdx+1 : m_RefPierIdx-1,m_PierFace == pgsTypes::Ahead ? pgsTypes::Back : pgsTypes::Ahead,m_OldGirderSpacing);

   std::map<CSegmentKey,std::pair<EventIndexType,EventIndexType>>::iterator segIter(m_SegmentEvents.begin());
   std::map<CSegmentKey,std::pair<EventIndexType,EventIndexType>>::iterator segIterEnd(m_SegmentEvents.end());
   for ( ; segIter != segIterEnd; segIter++ )
   {
      pIBridgeDesc->SetSegmentEventsByIndex(segIter->first,segIter->second.first,segIter->second.second);
   }

   std::map<CClosureKey,EventIndexType>::iterator cjIter(m_ClosureJointEvents.begin());
   std::map<CClosureKey,EventIndexType>::iterator cjIterEnd(m_ClosureJointEvents.end());
   for ( ; cjIter != cjIterEnd; cjIter++ )
   {
      CClosureKey closureKey(cjIter->first);
      pIBridgeDesc->SetCastClosureJointEventByIndex(closureKey.groupIndex,closureKey.segmentIndex,cjIter->second);
   }

   pEvents->FirePendingEvents();
}
