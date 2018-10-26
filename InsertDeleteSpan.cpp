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

#include "PGSuperAppPlugin\stdafx.h"
#include "InsertDeleteSpan.h"
#include "PGSuperDocBase.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

txnInsertSpan::txnInsertSpan(PierIndexType refPierIdx,pgsTypes::PierFaceType pierFace,Float64 spanLength,bool bCreateNewGroup,IndexType eventIdx)
{
   m_RefPierIdx = refPierIdx;
   m_PierFace   = pierFace;
   m_SpanLength = spanLength;
   m_bCreateNewGroup = bCreateNewGroup;
   m_EventIdx = eventIdx;
}

std::_tstring txnInsertSpan::Name() const
{
   return _T("Insert Span");
}

txnTransaction* txnInsertSpan::CreateClone() const
{
   return new txnInsertSpan(m_RefPierIdx, m_PierFace,m_SpanLength,m_bCreateNewGroup,m_EventIdx);
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

   // save the connection type, sometimes it doesn't get automatically reverted during an UNDO
   const CPierData2* pPier = pIBridgeDesc->GetPier(m_RefPierIdx);
   ATLASSERT(pPier->IsBoundaryPier());
   m_RefPierConnectionType = pPier->GetPierConnectionType();

   // Default length for new spans
   pIBridgeDesc->InsertSpan(m_RefPierIdx,m_PierFace,m_SpanLength,NULL,NULL,m_bCreateNewGroup,m_EventIdx);

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

   PierIndexType newPierIdx = m_RefPierIdx + (m_PierFace == pgsTypes::Ahead ? 1 : 0);
   pgsTypes::PierFaceType newPierFace = (m_PierFace == pgsTypes::Ahead ? pgsTypes::Back : pgsTypes::Ahead);
   pIBridgeDesc->DeletePier(newPierIdx,newPierFace);

   CPierData2 pier = *pIBridgeDesc->GetPier(m_RefPierIdx);
   pier.SetPierConnectionType(m_RefPierConnectionType);
   pIBridgeDesc->SetPierByIndex(m_RefPierIdx,pier);


   pEvents->FirePendingEvents();
}

///////////////////////////////////////////////

txnDeleteSpan::txnDeleteSpan(PierIndexType refPierIdx,pgsTypes::PierFaceType pierFace)
{
   m_RefPierIdx = refPierIdx;
   m_PierFace   = pierFace;

   m_pDeletedSpan = NULL;
   m_pDeletedPier = NULL;
}

txnDeleteSpan::~txnDeleteSpan()
{
   delete m_pDeletedSpan;
   delete m_pDeletedPier;
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
   const CPierData2* pRefPier = pBridgeDesc->GetPier(m_RefPierIdx);
   const CSpanData2* pSpan = (m_PierFace == pgsTypes::Back ? pRefPier->GetPrevSpan() : pRefPier->GetNextSpan());

   ASSERT(pSpan != NULL); // I don't think this should happen

   // save any temporary supports that are in the span being removed
   std::vector<const CTemporarySupportData*> tempSupports(pSpan->GetTemporarySupports());
   std::vector<const CTemporarySupportData*>::iterator tsIter(tempSupports.begin());
   std::vector<const CTemporarySupportData*>::iterator tsIterEnd(tempSupports.end());
   for ( ; tsIter != tsIterEnd; tsIter++ )
   {
      const CTemporarySupportData* pTS = *tsIter;
      TSItem item;
      item.TempSupport = *pTS;

      SupportIDType tsID = pTS->GetID();

      pBridgeDesc->GetTimelineManager()->GetTempSupportEvents(tsID,&item.ErectionEventIdx,&item.RemovalEventIdx);
      
      m_TempSupports.push_back(item);
   }

   m_pDeletedPier = new CPierData2(*pRefPier);
   if ( pSpan )
      m_pDeletedSpan = new CSpanData2(*pSpan);

   m_nGirderGroups = pBridgeDesc->GetGirderGroupCount();

   // save span length for undo
   m_SpanLength = pSpan->GetSpanLength();

   // save pier stage
   m_EventIdx = pBridgeDesc->GetTimelineManager()->GetPierErectionEventIndex(pRefPier->GetID());

   // save the connection type, sometimes it doesn't get automatically reverted during an UNDO
   const CPierData2* pPier = pIBridgeDesc->GetPier(m_RefPierIdx);
   ATLASSERT(pPier->IsBoundaryPier());
   m_RefPierConnectionType = pPier->GetPierConnectionType();

   // Delete the pier, span is deleted along with it
   pIBridgeDesc->DeletePier(m_RefPierIdx,m_PierFace);
   pRefPier = NULL;
   pSpan = NULL;


   GET_IFACE2(pBroker,ISelectionEx,pSelection);
   CSelection selection = pSelection->GetSelection();
   if ( selection.Type == CSelection::Span && selection.SpanIdx == m_pDeletedSpan->GetIndex() )
      pSelection->ClearSelection();

   if ( selection.Type == CSelection::Pier && selection.PierIdx == m_RefPierIdx )
      pSelection->ClearSelection();

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

   PierIndexType pierIdx = (m_PierFace == pgsTypes::Back ? m_RefPierIdx-1 : m_RefPierIdx);
   pgsTypes::PierFaceType face = (m_PierFace == pgsTypes::Back ? pgsTypes::Ahead : pgsTypes::Back);

   // create a new group if one was removed (the girder group count would have changed)
   bool bCreateNewGroup = (m_nGirderGroups != pIBridgeDesc->GetBridgeDescription()->GetGirderGroupCount());
   pIBridgeDesc->InsertSpan(pierIdx,face,m_SpanLength,m_pDeletedSpan,m_pDeletedPier,bCreateNewGroup,m_EventIdx);

   std::vector<TSItem>::iterator iter(m_TempSupports.begin());
   std::vector<TSItem>::iterator loop_end(m_TempSupports.end());
   for ( ; iter != loop_end; iter++ )
   {
      TSItem& item = *iter;
      CTemporarySupportData* pTS = new CTemporarySupportData(item.TempSupport);
      pIBridgeDesc->InsertTemporarySupport(pTS,item.ErectionEventIdx,item.RemovalEventIdx);
   }


   CPierData2 pier = *pIBridgeDesc->GetPier(m_RefPierIdx);
   pier.SetPierConnectionType(m_RefPierConnectionType);
   pIBridgeDesc->SetPierByIndex(m_RefPierIdx,pier);

   delete m_pDeletedSpan;
   m_pDeletedSpan = NULL;
   
   delete m_pDeletedPier;
   m_pDeletedPier = NULL;

   m_TempSupports.clear();

   pEvents->FirePendingEvents();
}
