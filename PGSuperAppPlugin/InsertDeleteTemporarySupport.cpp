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
#include "InsertDeleteTemporarySupport.h"
#include "PGSpliceDoc.h"

#include <EAF\EAFDisplayUnits.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

txnInsertTemporarySupport::txnInsertTemporarySupport(const CTemporarySupportData& tsData,IndexType erectionEventIdx,IndexType removalEventIdx)
{
   m_TSData = tsData;
   m_ErectionEventIdx = erectionEventIdx;
   m_RemovalEventIdx = removalEventIdx;
   m_tsID = INVALID_INDEX; // don't have an ID yet because it hasn't been added to the model
}

std::_tstring txnInsertTemporarySupport::Name() const
{
   return _T("Insert Temporary Support");
}

txnTransaction* txnInsertTemporarySupport::CreateClone() const
{
   return new txnInsertTemporarySupport(m_TSData,m_ErectionEventIdx,m_RemovalEventIdx);
}

bool txnInsertTemporarySupport::IsUndoable()
{
   return true;
}

bool txnInsertTemporarySupport::IsRepeatable()
{
   return false;
}

bool txnInsertTemporarySupport::Execute()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   SupportIndexType nTS = pBridgeDesc->GetTemporarySupportCount();
   for (SupportIndexType tsIdx = 0; tsIdx < nTS; tsIdx++ )
   {
      const CTemporarySupportData* pTS = pBridgeDesc->GetTemporarySupport(tsIdx);
      if ( IsEqual(pTS->GetStation(),m_TSData.GetStation()) )
      {
         CString strMsg;
         strMsg.Format(_T("Temporary support could not be added because one is already defined at %s"),FormatStation(pDisplayUnits->GetStationFormat(),m_TSData.GetStation()));
         AfxMessageBox(strMsg);

         return false;
      }
   }

   PierIndexType nPiers = pBridgeDesc->GetPierCount();
   for ( PierIndexType pierIdx = 0; pierIdx < nPiers; pierIdx++ )
   {
      const CPierData2* pPier = pBridgeDesc->GetPier(pierIdx);
      if ( IsEqual(pPier->GetStation(),m_TSData.GetStation()) )
      {
         CString strMsg;
         strMsg.Format(_T("Temporary support could not be added because a pier is already defined at %s"),FormatStation(pDisplayUnits->GetStationFormat(),m_TSData.GetStation()));
         AfxMessageBox(strMsg);

         return false;
      }
   }

   GET_IFACE2(pBroker,IEvents,pEvents);
   pEvents->HoldEvents();


   CTemporarySupportData* pTSData = new CTemporarySupportData(m_TSData);
   pIBridgeDesc->InsertTemporarySupport(pTSData,m_ErectionEventIdx,m_RemovalEventIdx);

   m_tsID = pTSData->GetID(); // capture the support id so it can be later removed

   pEvents->FirePendingEvents();

   return true;
}

void txnInsertTemporarySupport::Undo()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IEvents,pEvents);
   pEvents->HoldEvents();

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);

   pIBridgeDesc->DeleteTemporarySupportByID(m_tsID);

   pEvents->FirePendingEvents();
}

///////////////////////////////////////////////

txnDeleteTemporarySupport::txnDeleteTemporarySupport(SupportIDType tsID)
{
   m_tsID = tsID;
}

txnDeleteTemporarySupport::~txnDeleteTemporarySupport()
{
}

std::_tstring txnDeleteTemporarySupport::Name() const
{
   return _T("Delete Temporary Support");
}

txnTransaction* txnDeleteTemporarySupport::CreateClone() const
{
   return new txnDeleteTemporarySupport(m_tsID);
}

bool txnDeleteTemporarySupport::IsUndoable()
{
   return true;
}

bool txnDeleteTemporarySupport::IsRepeatable()
{
   return false;
}

bool txnDeleteTemporarySupport::Execute()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   GET_IFACE2(pBroker,IEvents,pEvents);
   pEvents->HoldEvents();

   // save the span/pier that are going to be deleted for undo
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   m_TSData = *pBridgeDesc->FindTemporarySupport(m_tsID);
   pBridgeDesc->GetTimelineManager()->GetTempSupportEvents(m_tsID,&m_ErectionEventIdx,&m_RemovalEventIdx);

   pIBridgeDesc->DeleteTemporarySupportByID(m_tsID);

   GET_IFACE2(pBroker,ISelectionEx,pSelection);
   CSelection selection = pSelection->GetSelection();
   if ( selection.Type == CSelection::TemporarySupport && selection.tsID == m_tsID)
      pSelection->ClearSelection();

   pEvents->FirePendingEvents();

   return true;
}

void txnDeleteTemporarySupport::Undo()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IEvents,pEvents);
   pEvents->HoldEvents();

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);

   CTemporarySupportData* pTSData = new CTemporarySupportData(m_TSData);
   pIBridgeDesc->InsertTemporarySupport(pTSData,m_ErectionEventIdx,m_RemovalEventIdx);
   m_tsID = pTSData->GetID();

   pEvents->FirePendingEvents();
}
