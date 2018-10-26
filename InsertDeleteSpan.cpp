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
}

txnDeleteSpan::~txnDeleteSpan()
{
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

   ATLASSERT(pSpan != NULL); // this should not happen

   m_pDeletedPier = new CPierData2(*pPier);
   m_pDeletedSpan = new CSpanData2(*pSpan);

   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(pSpan);
   m_bCreateNewGroup = (pGroup->GetSpanCount() == 1 ? true : false); // there is only one span in this group so the group is going to go away. need to create a new group for undo
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
      pIBridgeDesc->InsertSpan(m_RefPierIdx,pgsTypes::Back,m_SpanLength,m_pDeletedSpan,m_pDeletedPier,m_bCreateNewGroup,m_PierErectionEventIdx);
   }
   else
   {
      pIBridgeDesc->InsertSpan(m_RefPierIdx,m_PierFace,m_SpanLength,m_pDeletedSpan,m_pDeletedPier,m_bCreateNewGroup,m_PierErectionEventIdx);
   }

   pEvents->FirePendingEvents();
}
