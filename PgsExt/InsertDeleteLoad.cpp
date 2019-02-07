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

#include <PgsExt\PgsExtLib.h>
#include <PgsExt\InsertDeleteLoad.h>
#include <EAF\EAFUtilities.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//////////////////////////////////////////
//////////////////////////////////////////
//////////////////////////////////////////

txnInsertPointLoad::txnInsertPointLoad(const CPointLoadData& loadData,EventIDType loadingEventID,CTimelineManager* pTimelineMgr)
{
   m_pTimelineMgr = nullptr;
   if ( pTimelineMgr )
   {
      m_pTimelineMgr = new CTimelineManager(*pTimelineMgr);
   }

   m_LoadIdx = INVALID_INDEX;
   m_LoadData = loadData;
   m_LoadingEventID = loadingEventID;
}

txnInsertPointLoad::~txnInsertPointLoad()
{
   if ( m_pTimelineMgr )
   {
      delete m_pTimelineMgr;
   }
}

std::_tstring txnInsertPointLoad::Name() const
{
   return _T("Insert Point Load");
}

txnTransaction* txnInsertPointLoad::CreateClone() const
{
   return new txnInsertPointLoad(m_LoadData,m_LoadingEventID,m_pTimelineMgr);
}

bool txnInsertPointLoad::IsUndoable()
{
   return true;
}

bool txnInsertPointLoad::IsRepeatable()
{
   return false;
}

bool txnInsertPointLoad::Execute()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEvents,pEvents);
   // Exception-safe holder for events
   CIEventsHolder event_holder(pEvents);

   if ( m_pTimelineMgr )
   {
      // if we have a timeline manager object, it means that the
      // timeline was changed as part of this load creation
      // update the timeline in the main bridge model before
      // updating the loading
      GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
      m_OldTimelineMgr = *pIBridgeDesc->GetTimelineManager();
      pIBridgeDesc->SetTimelineManager(*m_pTimelineMgr);
   }

   GET_IFACE2(pBroker,IUserDefinedLoadData, pUdl);
   m_LoadIdx = pUdl->AddPointLoad(m_LoadingEventID,m_LoadData);

   return true;
}

void txnInsertPointLoad::Undo()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IEvents,pEvents);
   // Exception-safe holder for events
   CIEventsHolder event_holder(pEvents);

   GET_IFACE2(pBroker,IUserDefinedLoadData, pUdl);
   pUdl->DeletePointLoad(m_LoadIdx);

   if ( m_pTimelineMgr )
   {
      // if we have a timeline manager object, it means that the
      // timeline was changed as part of this load creation...
      // the timeline was updated when we exectued the transaction
      // so now it needs to be put back the way it was because
      // we are undoing the transaction
      GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
      pIBridgeDesc->SetTimelineManager(m_OldTimelineMgr);
   }
}

///////////////////////////////////////////////

txnDeletePointLoad::txnDeletePointLoad(LoadIDType loadID)
{
   m_LoadID = loadID;
}

std::_tstring txnDeletePointLoad::Name() const
{
   return _T("Delete Point Load");
}

txnTransaction* txnDeletePointLoad::CreateClone() const
{
   return new txnDeletePointLoad(m_LoadID);
}

bool txnDeletePointLoad::IsUndoable()
{
   return true;
}

bool txnDeletePointLoad::IsRepeatable()
{
   return false;
}

bool txnDeletePointLoad::Execute()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEvents,pEvents);
   // Exception-safe holder for events
   CIEventsHolder event_holder(pEvents);

   // keep copies of the loading and the event when it is applied
   // for undo
   GET_IFACE2(pBroker,IUserDefinedLoadData, pUdl);
   m_LoadData = *(pUdl->FindPointLoad(m_LoadID));

   ATLASSERT(m_LoadData.m_ID == m_LoadID);

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CTimelineManager* pTimelineMgr = pIBridgeDesc->GetTimelineManager();
   m_LoadingEventID = pTimelineMgr->FindUserLoadEventID(m_LoadData.m_ID);

   pUdl->DeletePointLoadByID(m_LoadID);

   return true;
}

void txnDeletePointLoad::Undo()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IEvents,pEvents);
   // Exception-safe holder for events
   CIEventsHolder event_holder(pEvents);

   GET_IFACE2(pBroker,IUserDefinedLoadData, pUdl);
   IndexType loadIdx = pUdl->AddPointLoad(m_LoadingEventID,m_LoadData);
   ATLASSERT(pUdl->GetPointLoad(loadIdx)->m_ID == m_LoadID);
}

///////////////////////////////////////////////

txnEditPointLoad::txnEditPointLoad(LoadIDType loadID,const CPointLoadData& oldLoadData,EventIDType oldLoadingEventID,const CPointLoadData& newLoadData,EventIDType newLoadingEventID,CTimelineManager* pTimelineMgr)
{
   m_pTimelineMgr = nullptr;
   if ( pTimelineMgr )
   {
      m_pTimelineMgr = new CTimelineManager(*pTimelineMgr);
   }

   m_LoadID = loadID;
   m_LoadData[0] = oldLoadData;
   m_LoadData[1] = newLoadData;
   m_LoadingEventID[0] = oldLoadingEventID;
   m_LoadingEventID[1] = newLoadingEventID;
}

txnEditPointLoad::~txnEditPointLoad()
{
   if ( m_pTimelineMgr )
   {
      delete m_pTimelineMgr;
   }
}

std::_tstring txnEditPointLoad::Name() const
{
   return _T("Edit Point Load");
}

txnTransaction* txnEditPointLoad::CreateClone() const
{
   return new txnEditPointLoad(m_LoadID,m_LoadData[0],m_LoadingEventID[0],m_LoadData[1],m_LoadingEventID[1],m_pTimelineMgr);
}

bool txnEditPointLoad::Execute()
{
   DoExecute(1);
   return true;
}

void txnEditPointLoad::Undo()
{
   DoExecute(0);
}

bool txnEditPointLoad::IsUndoable()
{
   return true;
}

bool txnEditPointLoad::IsRepeatable()
{
   return true;
}

void txnEditPointLoad::DoExecute(int i)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEvents,pEvents);
   // Exception-safe holder for events
   CIEventsHolder event_holder(pEvents);

   if ( m_pTimelineMgr )
   {
      // if we have a timeline manager object, it means that the
      // timeline was changed as part of this load creation
      // update the timeline in the main bridge model before
      // updating the loading
      GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
      if ( i == 1 )
      {
         m_OldTimelineMgr = *pIBridgeDesc->GetTimelineManager();
         pIBridgeDesc->SetTimelineManager(*m_pTimelineMgr);
      }
      else
      {
         pIBridgeDesc->SetTimelineManager(m_OldTimelineMgr);
      }
   }

   GET_IFACE2(pBroker,IUserDefinedLoadData, pUdl);
   pUdl->UpdatePointLoadByID(m_LoadID,m_LoadingEventID[i],m_LoadData[i]);
}

//////////////////////////////////////////
//////////////////////////////////////////
//////////////////////////////////////////

txnInsertDistributedLoad::txnInsertDistributedLoad(const CDistributedLoadData& loadData,EventIDType loadingEventID,CTimelineManager* pTimelineMgr)
{
   m_pTimelineMgr = nullptr;
   if ( pTimelineMgr )
   {
      m_pTimelineMgr = new CTimelineManager(*pTimelineMgr);
   }

   m_LoadIdx = INVALID_INDEX;
   m_LoadData = loadData;
   m_LoadingEventID = loadingEventID;
}

txnInsertDistributedLoad::~txnInsertDistributedLoad()
{
   if ( m_pTimelineMgr )
   {
      delete m_pTimelineMgr;
   }
}

std::_tstring txnInsertDistributedLoad::Name() const
{
   return _T("Insert Distributed Load");
}

txnTransaction* txnInsertDistributedLoad::CreateClone() const
{
   return new txnInsertDistributedLoad(m_LoadData,m_LoadingEventID,m_pTimelineMgr);
}

bool txnInsertDistributedLoad::IsUndoable()
{
   return true;
}

bool txnInsertDistributedLoad::IsRepeatable()
{
   return false;
}

bool txnInsertDistributedLoad::Execute()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEvents,pEvents);
   // Exception-safe holder for events
   CIEventsHolder event_holder(pEvents);

   if ( m_pTimelineMgr )
   {
      // if we have a timeline manager object, it means that the
      // timeline was changed as part of this load creation
      // update the timeline in the main bridge model before
      // updating the loading
      GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
      m_OldTimelineMgr = *pIBridgeDesc->GetTimelineManager();
      pIBridgeDesc->SetTimelineManager(*m_pTimelineMgr);
   }

   GET_IFACE2(pBroker,IUserDefinedLoadData, pUdl);
   m_LoadIdx = pUdl->AddDistributedLoad(m_LoadingEventID,m_LoadData);

   return true;
}

void txnInsertDistributedLoad::Undo()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IEvents,pEvents);
   // Exception-safe holder for events
   CIEventsHolder event_holder(pEvents);

   GET_IFACE2(pBroker,IUserDefinedLoadData, pUdl);
   pUdl->DeleteDistributedLoad(m_LoadIdx);

   if ( m_pTimelineMgr )
   {
      // if we have a timeline manager object, it means that the
      // timeline was changed as part of this load creation...
      // the timeline was updated when we exectued the transaction
      // so now it needs to be put back the way it was because
      // we are undoing the transaction
      GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
      pIBridgeDesc->SetTimelineManager(m_OldTimelineMgr);
   }
}

///////////////////////////////////////////////

txnDeleteDistributedLoad::txnDeleteDistributedLoad(LoadIDType loadID)
{
   m_LoadID = loadID;
}

std::_tstring txnDeleteDistributedLoad::Name() const
{
   return _T("Delete Distributed Load");
}

txnTransaction* txnDeleteDistributedLoad::CreateClone() const
{
   return new txnDeleteDistributedLoad(m_LoadID);
}

bool txnDeleteDistributedLoad::IsUndoable()
{
   return true;
}

bool txnDeleteDistributedLoad::IsRepeatable()
{
   return false;
}

bool txnDeleteDistributedLoad::Execute()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IEvents,pEvents);
   // Exception-safe holder for events
   CIEventsHolder event_holder(pEvents);

   GET_IFACE2(pBroker,IUserDefinedLoadData, pUdl);
   m_LoadData = *(pUdl->FindDistributedLoad(m_LoadID));

   ATLASSERT(m_LoadData.m_ID == m_LoadID);

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CTimelineManager* pTimelineMgr = pIBridgeDesc->GetTimelineManager();
   m_LoadingEventID = pTimelineMgr->FindUserLoadEventID(m_LoadData.m_ID);

   pUdl->DeleteDistributedLoadByID(m_LoadID);

   return true;
}

void txnDeleteDistributedLoad::Undo()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IEvents,pEvents);
   // Exception-safe holder for events
   CIEventsHolder event_holder(pEvents);

   GET_IFACE2(pBroker,IUserDefinedLoadData, pUdl);
   IndexType loadIdx = pUdl->AddDistributedLoad(m_LoadingEventID,m_LoadData);
   ATLASSERT(pUdl->GetDistributedLoad(loadIdx)->m_ID == m_LoadID);
}

///////////////////////////////////////////////

txnEditDistributedLoad::txnEditDistributedLoad(LoadIDType loadID,const CDistributedLoadData& oldLoadData,EventIDType oldLoadingEventID,const CDistributedLoadData& newLoadData,EventIDType newLoadingEventID,CTimelineManager* pTimelineMgr)
{
   m_pTimelineMgr = nullptr;
   if ( pTimelineMgr )
   {
      m_pTimelineMgr = new CTimelineManager(*pTimelineMgr);
   }

   m_LoadID = loadID;
   m_LoadData[0] = oldLoadData;
   m_LoadData[1] = newLoadData;
   m_LoadingEventID[0] = oldLoadingEventID;
   m_LoadingEventID[1] = newLoadingEventID;
}

txnEditDistributedLoad::~txnEditDistributedLoad()
{
   if (m_pTimelineMgr)
   {
      delete m_pTimelineMgr;
   }
}

std::_tstring txnEditDistributedLoad::Name() const
{
   return _T("Edit Distributed Load");
}

txnTransaction* txnEditDistributedLoad::CreateClone() const
{
   return new txnEditDistributedLoad(m_LoadID,m_LoadData[0],m_LoadingEventID[0],m_LoadData[1],m_LoadingEventID[1],m_pTimelineMgr);
}

bool txnEditDistributedLoad::Execute()
{
   DoExecute(1);
   return true;
}

void txnEditDistributedLoad::Undo()
{
   DoExecute(0);
}

bool txnEditDistributedLoad::IsUndoable()
{
   return true;
}

bool txnEditDistributedLoad::IsRepeatable()
{
   return true;
}

void txnEditDistributedLoad::DoExecute(int i)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEvents,pEvents);
   // Exception-safe holder for events
   CIEventsHolder event_holder(pEvents);

   if ( m_pTimelineMgr )
   {
      // if we have a timeline manager object, it means that the
      // timeline was changed as part of this load creation
      // update the timeline in the main bridge model before
      // updating the loading
      GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
      if ( i == 1 )
      {
         m_OldTimelineMgr = *pIBridgeDesc->GetTimelineManager();
         pIBridgeDesc->SetTimelineManager(*m_pTimelineMgr);
      }
      else
      {
         pIBridgeDesc->SetTimelineManager(m_OldTimelineMgr);
      }
   }

   GET_IFACE2(pBroker,IUserDefinedLoadData, pUdl);
   pUdl->UpdateDistributedLoadByID(m_LoadID,m_LoadingEventID[i],m_LoadData[i]);
}

//////////////////////////////////////////
//////////////////////////////////////////
//////////////////////////////////////////

txnInsertMomentLoad::txnInsertMomentLoad(const CMomentLoadData& loadData,EventIDType loadingEventID,CTimelineManager* pTimelineMgr)
{
   m_pTimelineMgr = nullptr;
   if ( pTimelineMgr )
   {
      m_pTimelineMgr = new CTimelineManager(*pTimelineMgr);
   }
   m_LoadIdx = INVALID_INDEX;
   m_LoadData = loadData;
   m_LoadingEventID = loadingEventID;
}

txnInsertMomentLoad::~txnInsertMomentLoad()
{
   if ( m_pTimelineMgr )
   {
      delete m_pTimelineMgr;
   }
}

std::_tstring txnInsertMomentLoad::Name() const
{
   return _T("Insert Moment Load");
}

txnTransaction* txnInsertMomentLoad::CreateClone() const
{
   return new txnInsertMomentLoad(m_LoadData,m_LoadingEventID,m_pTimelineMgr);
}

bool txnInsertMomentLoad::IsUndoable()
{
   return true;
}

bool txnInsertMomentLoad::IsRepeatable()
{
   return false;
}

bool txnInsertMomentLoad::Execute()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEvents,pEvents);
   // Exception-safe holder for events
   CIEventsHolder event_holder(pEvents);

   if ( m_pTimelineMgr )
   {
      // if we have a timeline manager object, it means that the
      // timeline was changed as part of this load creation
      // update the timeline in the main bridge model before
      // updating the loading
      GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
      m_OldTimelineMgr = *pIBridgeDesc->GetTimelineManager();
      pIBridgeDesc->SetTimelineManager(*m_pTimelineMgr);
   }

   GET_IFACE2(pBroker,IUserDefinedLoadData, pUdl);
   m_LoadIdx = pUdl->AddMomentLoad(m_LoadingEventID,m_LoadData);

   return true;
}

void txnInsertMomentLoad::Undo()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IEvents,pEvents);
   // Exception-safe holder for events
   CIEventsHolder event_holder(pEvents);

   GET_IFACE2(pBroker,IUserDefinedLoadData, pUdl);
   pUdl->DeleteMomentLoad(m_LoadIdx);

   if ( m_pTimelineMgr )
   {
      // if we have a timeline manager object, it means that the
      // timeline was changed as part of this load creation...
      // the timeline was updated when we exectued the transaction
      // so now it needs to be put back the way it was because
      // we are undoing the transaction
      GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
      pIBridgeDesc->SetTimelineManager(m_OldTimelineMgr);
   }
}

///////////////////////////////////////////////

txnDeleteMomentLoad::txnDeleteMomentLoad(LoadIDType loadID)
{
   m_LoadID = loadID;
}

std::_tstring txnDeleteMomentLoad::Name() const
{
   return _T("Delete Moment Load");
}

txnTransaction* txnDeleteMomentLoad::CreateClone() const
{
   return new txnDeleteMomentLoad(m_LoadID);
}

bool txnDeleteMomentLoad::IsUndoable()
{
   return true;
}

bool txnDeleteMomentLoad::IsRepeatable()
{
   return false;
}

bool txnDeleteMomentLoad::Execute()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEvents,pEvents);
   // Exception-safe holder for events
   CIEventsHolder event_holder(pEvents);

   GET_IFACE2(pBroker,IUserDefinedLoadData, pUdl);
   m_LoadData = *(pUdl->FindMomentLoad(m_LoadID));

   ATLASSERT(m_LoadData.m_ID == m_LoadID);

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CTimelineManager* pTimelineMgr = pIBridgeDesc->GetTimelineManager();
   m_LoadingEventID = pTimelineMgr->FindUserLoadEventID(m_LoadData.m_ID);

   pUdl->DeleteMomentLoadByID(m_LoadID);

   return true;
}

void txnDeleteMomentLoad::Undo()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IEvents,pEvents);
   // Exception-safe holder for events
   CIEventsHolder event_holder(pEvents);

   GET_IFACE2(pBroker,IUserDefinedLoadData, pUdl);
   IndexType loadIdx = pUdl->AddMomentLoad(m_LoadingEventID,m_LoadData);
   ATLASSERT(pUdl->GetMomentLoad(loadIdx)->m_ID == m_LoadID);
}

///////////////////////////////////////////////

txnEditMomentLoad::txnEditMomentLoad(LoadIDType loadID,const CMomentLoadData& oldLoadData,EventIDType oldLoadingEventID,const CMomentLoadData& newLoadData,EventIDType newLoadingEventID,CTimelineManager* pTimelineMgr)
{
   m_pTimelineMgr = nullptr;
   if ( pTimelineMgr )
   {
      m_pTimelineMgr = new CTimelineManager(*pTimelineMgr);
   }

   m_LoadID = loadID;
   m_LoadData[0] = oldLoadData;
   m_LoadData[1] = newLoadData;
   m_LoadingEventID[0] = oldLoadingEventID;
   m_LoadingEventID[1] = newLoadingEventID;
}

txnEditMomentLoad::~txnEditMomentLoad()
{
   if ( m_pTimelineMgr )
   {
      delete m_pTimelineMgr;
   }
}

std::_tstring txnEditMomentLoad::Name() const
{
   return _T("Edit Moment Load");
}

txnTransaction* txnEditMomentLoad::CreateClone() const
{
   return new txnEditMomentLoad(m_LoadID,m_LoadData[0],m_LoadingEventID[0],m_LoadData[1],m_LoadingEventID[1],m_pTimelineMgr);
}

bool txnEditMomentLoad::Execute()
{
   DoExecute(1);
   return true;
}

void txnEditMomentLoad::Undo()
{
   DoExecute(0);
}

bool txnEditMomentLoad::IsUndoable()
{
   return true;
}

bool txnEditMomentLoad::IsRepeatable()
{
   return true;
}

void txnEditMomentLoad::DoExecute(int i)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEvents,pEvents);
   // Exception-safe holder for events
   CIEventsHolder event_holder(pEvents);

   if ( m_pTimelineMgr )
   {
      // if we have a timeline manager object, it means that the
      // timeline was changed as part of this load creation
      // update the timeline in the main bridge model before
      // updating the loading
      GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
      if ( i == 1 )
      {
         m_OldTimelineMgr = *pIBridgeDesc->GetTimelineManager();
         pIBridgeDesc->SetTimelineManager(*m_pTimelineMgr);
      }
      else
      {
         pIBridgeDesc->SetTimelineManager(m_OldTimelineMgr);
      }
   }

   GET_IFACE2(pBroker,IUserDefinedLoadData, pUdl);
   pUdl->UpdateMomentLoadByID(m_LoadID,m_LoadingEventID[i],m_LoadData[i]);
}

