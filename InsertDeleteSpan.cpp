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

#include "PGSuperAppPlugin\stdafx.h"
#include "InsertDeleteSpan.h"
#include "PGSuperDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

txnInsertSpan::txnInsertSpan(PierIndexType refPierIdx,pgsTypes::PierFaceType pierFace)
{
   m_RefPierIdx = refPierIdx;
   m_PierFace   = pierFace;
}

std::_tstring txnInsertSpan::Name() const
{
   return _T("Insert Span");
}

txnTransaction* txnInsertSpan::CreateClone() const
{
   return new txnInsertSpan(m_RefPierIdx, m_PierFace);
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

   // Default length for new spans is the length of the
   // previous span
   const CSpanData* pSpan = pIBridgeDesc->GetSpan(m_RefPierIdx);
   if ( pSpan == NULL ) // m_RefPierIdx must be for the last pier... 
      pSpan = pIBridgeDesc->GetSpan(m_RefPierIdx-1); // ... get the previous span

   Float64 span_length = pSpan->GetSpanLength();

   pIBridgeDesc->InsertSpan(m_RefPierIdx,m_PierFace,span_length);

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
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CPierData* pPier = pBridgeDesc->GetPier(m_RefPierIdx);
   const CSpanData* pSpan = (m_PierFace == pgsTypes::Back ? pPier->GetPrevSpan() : pPier->GetNextSpan());

   ASSERT(pSpan != NULL); // I don't think this should happen

   m_pDeletedPier = new CPierData(*pPier);
   if ( pSpan )
      m_pDeletedSpan = new CSpanData(*pSpan);

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
      pIBridgeDesc->InsertSpan(m_RefPierIdx,pgsTypes::Back,m_SpanLength,m_pDeletedSpan,m_pDeletedPier);
   else
      pIBridgeDesc->InsertSpan(m_RefPierIdx,m_PierFace,m_SpanLength,m_pDeletedSpan,m_pDeletedPier);

   pEvents->FirePendingEvents();
}
