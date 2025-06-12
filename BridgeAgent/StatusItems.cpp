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

#include "stdafx.h"
#include "BridgeAgent.h"
#include "StatusItems.h"
#include <PgsExt\InsertDeleteLoad.h>
#include "DealWithLoadDlg.h"

#include <IFace\EditByUI.h>
#include <IFace\Project.h>
#include <EAF/EAFStatusCenter.h>

#include <EAF\EAFTransactions.h>
#include <EAF\EAFUtilities.h>

pgsAlignmentDescriptionStatusItem::pgsAlignmentDescriptionStatusItem(StatusGroupIDType statusGroupID,StatusCallbackIDType callbackID,long dlgPage,LPCTSTR strDescription) :
WBFL::EAF::StatusItem(statusGroupID,callbackID,strDescription), m_DlgPage(dlgPage)
{
}

bool pgsAlignmentDescriptionStatusItem::IsEqual(std::shared_ptr<const WBFL::EAF::StatusItem> pOther) const
{
   auto other = std::dynamic_pointer_cast<const pgsAlignmentDescriptionStatusItem>(pOther);
   if ( !other )
   {
      return false;
   }

   if ( CString(this->GetDescription()) != CString(other->GetDescription()) )
   {
      return false;
   }

   if ( m_DlgPage != other->m_DlgPage )
   {
      return false;
   }

   return true;
}

//////////////////////////////////////////////////////////
pgsAlignmentDescriptionStatusCallback::pgsAlignmentDescriptionStatusCallback(WBFL::EAF::StatusSeverityType severity):
m_Severity(severity)
{
}

WBFL::EAF::StatusSeverityType pgsAlignmentDescriptionStatusCallback::GetSeverity() const
{
   return m_Severity;
}

void pgsAlignmentDescriptionStatusCallback::Execute(std::shared_ptr<WBFL::EAF::StatusItem> pStatusItem)
{
   auto pItem = std::dynamic_pointer_cast<pgsAlignmentDescriptionStatusItem>(pStatusItem);
   ATLASSERT(pItem!=nullptr);

   if ( AfxMessageBox( pStatusItem->GetDescription(), MB_OK ) == IDOK )
   {
      auto broker = EAFGetBroker();
      GET_IFACE2(broker,IEditByUI,pEdit);
      pEdit->EditAlignmentDescription(pItem->m_DlgPage);
   }
}


////////////////

pgsConcreteStrengthStatusItem::pgsConcreteStrengthStatusItem(pgsConcreteStrengthStatusItem::ConcreteType concType,pgsConcreteStrengthStatusItem::ElementType elemType,const CSegmentKey& segmentKey,StatusGroupIDType statusGroupID,StatusCallbackIDType callbackID,LPCTSTR strDescription) :
pgsSegmentRelatedStatusItem(statusGroupID,callbackID,strDescription,segmentKey), m_ConcreteType(concType),m_ElementType(elemType),m_SegmentKey(segmentKey)
{
}

bool pgsConcreteStrengthStatusItem::IsEqual(std::shared_ptr<const WBFL::EAF::StatusItem> pOther) const
{
   auto other = std::dynamic_pointer_cast<const pgsConcreteStrengthStatusItem>(pOther);
   if ( !other )
   {
      return false;
   }

   return (other->m_ConcreteType == m_ConcreteType && other->m_ElementType == m_ElementType && other->m_SegmentKey == m_SegmentKey);
}

//////////////////////////////////////////////////////////
pgsConcreteStrengthStatusCallback::pgsConcreteStrengthStatusCallback(WBFL::EAF::StatusSeverityType severity):
m_Severity(severity)
{
}

WBFL::EAF::StatusSeverityType pgsConcreteStrengthStatusCallback::GetSeverity() const
{
   return m_Severity;
}

void pgsConcreteStrengthStatusCallback::Execute(std::shared_ptr<WBFL::EAF::StatusItem> pStatusItem)
{
   auto pItem = std::dynamic_pointer_cast<pgsConcreteStrengthStatusItem>(pStatusItem);
   ATLASSERT(pItem!=nullptr);

   auto broker = EAFGetBroker();
   GET_IFACE2(broker,IEditByUI,pEdit);

   if ( pItem->m_ConcreteType == pgsConcreteStrengthStatusItem::Slab )
   {
      pEdit->EditBridgeDescription(EBD_DECK);
   }
   else if ( pItem->m_ConcreteType == pgsConcreteStrengthStatusItem::GirderSegment )
   {
      pEdit->EditSegmentDescription(pItem->m_SegmentKey,EGD_CONCRETE);
   }
   else if ( pItem->m_ConcreteType == pgsConcreteStrengthStatusItem::ClosureJoint )
   {
      pEdit->EditClosureJointDescription(pItem->m_SegmentKey,EGD_CONCRETE);
   }
   else
   {
      pEdit->EditBridgeDescription(EBD_RAILING);
   }
}

//////////////////////

pgsPointLoadStatusItem::pgsPointLoadStatusItem(IndexType value,StatusGroupIDType statusGroupID,StatusCallbackIDType callbackID,LPCTSTR strDescription,const CSpanKey& spanKey) :
pgsSpanGirderRelatedStatusItem(statusGroupID,callbackID,strDescription,spanKey), 
m_LoadIndex(value), m_SpanKey(spanKey)
{
}

bool pgsPointLoadStatusItem::IsEqual(std::shared_ptr<const WBFL::EAF::StatusItem> pOther) const
{
   auto other = std::dynamic_pointer_cast<const pgsPointLoadStatusItem>(pOther);
   if ( !other )
   {
      return false;
   }

   return (other->m_LoadIndex == m_LoadIndex && other->m_SpanKey == m_SpanKey);
}

///////////////////////////////

pgsPointLoadStatusCallback::pgsPointLoadStatusCallback(WBFL::EAF::StatusSeverityType severity):
m_Severity(severity)
{
}

WBFL::EAF::StatusSeverityType pgsPointLoadStatusCallback::GetSeverity() const
{
   return m_Severity;
}

void pgsPointLoadStatusCallback::Execute(std::shared_ptr<WBFL::EAF::StatusItem> pStatusItem)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   auto pItem = std::dynamic_pointer_cast<pgsPointLoadStatusItem>(pStatusItem);
   ATLASSERT(pItem!=nullptr);

   CDealWithLoadDlg dlg;
   dlg.m_Message = pItem->GetDescription();

   INT_PTR result = dlg.DoModal();
   if ( result == CDealWithLoadDlg::IDDELETELOAD )
   {
      auto broker = EAFGetBroker();
      GET_IFACE2(broker,IEAFTransactions,pTxn);
      txnDeletePointLoad txn(pItem->m_LoadIndex);
      pTxn->Execute(txn);

      StatusItemIDType id = pItem->GetID();
      GET_IFACE2(broker,IEAFStatusCenter,pStatusCenter);
      pStatusCenter->RemoveByID(id);
   }
   else if (result == CDealWithLoadDlg::IDEDITLOAD)
   {
      auto broker = EAFGetBroker();
      GET_IFACE2(broker,IEditByUI,pEdit);
      if ( pEdit->EditPointLoad(pItem->m_LoadIndex) )
      {
         StatusItemIDType id = pItem->GetID();
         GET_IFACE2(broker,IEAFStatusCenter,pStatusCenter);
         pStatusCenter->RemoveByID(id);
      }
   }
   else
   {
      ATLASSERT(result == IDCANCEL);
   }
}

//////////////////////////////////////////////////////////

pgsDistributedLoadStatusItem::pgsDistributedLoadStatusItem(IndexType value,StatusGroupIDType statusGroupID,StatusCallbackIDType callbackID,LPCTSTR strDescription,const CSpanKey& spanKey) :
pgsSpanGirderRelatedStatusItem(statusGroupID,callbackID,strDescription,spanKey), 
m_LoadIndex(value), m_SpanKey(spanKey)
{
}

bool pgsDistributedLoadStatusItem::IsEqual(std::shared_ptr<const WBFL::EAF::StatusItem> pOther) const
{
   auto other = std::dynamic_pointer_cast<const pgsDistributedLoadStatusItem>(pOther);
   if ( !other )
   {
      return false;
   }

   return (other->m_LoadIndex == m_LoadIndex && other->m_SpanKey == m_SpanKey);
}

//////////////////////////////////////////////////////////

pgsDistributedLoadStatusCallback::pgsDistributedLoadStatusCallback(WBFL::EAF::StatusSeverityType severity):
m_Severity(severity)
{
}

WBFL::EAF::StatusSeverityType pgsDistributedLoadStatusCallback::GetSeverity() const
{
   return m_Severity;
}

void pgsDistributedLoadStatusCallback::Execute(std::shared_ptr<WBFL::EAF::StatusItem> pStatusItem)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   auto pItem = std::dynamic_pointer_cast<pgsDistributedLoadStatusItem>(pStatusItem);
   ATLASSERT(pItem!=nullptr);

   CDealWithLoadDlg dlg;
   dlg.m_Message = pItem->GetDescription();

   INT_PTR result = dlg.DoModal();

   if ( result == CDealWithLoadDlg::IDDELETELOAD )
   {
      auto broker = EAFGetBroker();
      GET_IFACE2(broker,IEAFTransactions,pTxn);
      txnDeleteDistributedLoad txn(pItem->m_LoadIndex);
      pTxn->Execute(txn);

      StatusItemIDType id = pItem->GetID();
      GET_IFACE2(broker,IEAFStatusCenter,pStatusCenter);
      pStatusCenter->RemoveByID(id);
   }
   else if (result == CDealWithLoadDlg::IDEDITLOAD)
   {
      auto broker = EAFGetBroker();
      GET_IFACE2(broker,IEditByUI,pEdit);
      if ( pEdit->EditDistributedLoad(pItem->m_LoadIndex) )
      {
         StatusItemIDType id = pItem->GetID();
         GET_IFACE2(broker,IEAFStatusCenter,pStatusCenter);
         pStatusCenter->RemoveByID(id);
      }
   }
   else
   {
      ATLASSERT(result == IDCANCEL);
   }
}

//////////////////////////////////////////////

pgsMomentLoadStatusItem::pgsMomentLoadStatusItem(IndexType value,StatusGroupIDType statusGroupID,StatusCallbackIDType callbackID,LPCTSTR strDescription,const CSpanKey& spanKey):
pgsSpanGirderRelatedStatusItem(statusGroupID,callbackID,strDescription,spanKey), 
m_LoadIndex(value), m_SpanKey(spanKey)
{
}

bool pgsMomentLoadStatusItem::IsEqual(std::shared_ptr<const WBFL::EAF::StatusItem> pOther) const
{
   auto other = std::dynamic_pointer_cast<const pgsMomentLoadStatusItem>(pOther);
   if ( !other )
   {
      return false;
   }

   return (other->m_LoadIndex == m_LoadIndex && other->m_SpanKey == m_SpanKey);
}
///////////////////////////

pgsMomentLoadStatusCallback::pgsMomentLoadStatusCallback(WBFL::EAF::StatusSeverityType severity):
m_Severity(severity)
{
}

WBFL::EAF::StatusSeverityType pgsMomentLoadStatusCallback::GetSeverity() const
{
   return m_Severity;
}

void pgsMomentLoadStatusCallback::Execute(std::shared_ptr<WBFL::EAF::StatusItem> pStatusItem)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   auto pItem = std::dynamic_pointer_cast<pgsMomentLoadStatusItem>(pStatusItem);
   ATLASSERT(pItem!=nullptr);

   CDealWithLoadDlg dlg;
   dlg.m_Message = pItem->GetDescription();

   INT_PTR result = dlg.DoModal();

   if ( result == CDealWithLoadDlg::IDDELETELOAD )
   {
      auto broker = EAFGetBroker();
      GET_IFACE2(broker,IEAFTransactions,pTxn);
      txnDeleteMomentLoad txn(pItem->m_LoadIndex);
      pTxn->Execute(txn);

      StatusItemIDType id = pItem->GetID();
      GET_IFACE2(broker,IEAFStatusCenter,pStatusCenter);
      pStatusCenter->RemoveByID(id);
   }
   else if (result == CDealWithLoadDlg::IDEDITLOAD)
   {
      auto broker = EAFGetBroker();
      GET_IFACE2(broker,IEditByUI,pEdit);
      if ( pEdit->EditMomentLoad(pItem->m_LoadIndex) )
      {
         StatusItemIDType id = pItem->GetID();
         GET_IFACE2(broker,IEAFStatusCenter,pStatusCenter);
         pStatusCenter->RemoveByID(id);
      }
   }
   else
   {
      ATLASSERT(result == IDCANCEL);
   }
}
