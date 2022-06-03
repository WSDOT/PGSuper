///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2022  Washington State Department of Transportation
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

#include "StatusItems.h"
#include <PgsExt\InsertDeleteLoad.h>
#include "DealWithLoadDlg.h"

#include <IFace\EditByUI.h>
#include <IFace\Project.h>
#include <IFace\StatusCenter.h>

#include <EAF\EAFTransactions.h>

////////////////

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

////////////////

pgsAlignmentDescriptionStatusItem::pgsAlignmentDescriptionStatusItem(StatusGroupIDType statusGroupID,StatusCallbackIDType callbackID,long dlgPage,LPCTSTR strDescription) :
CEAFStatusItem(statusGroupID,callbackID,strDescription), m_DlgPage(dlgPage)
{
}

bool pgsAlignmentDescriptionStatusItem::IsEqual(CEAFStatusItem* pOther)
{
   pgsAlignmentDescriptionStatusItem* other = dynamic_cast<pgsAlignmentDescriptionStatusItem*>(pOther);
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
pgsAlignmentDescriptionStatusCallback::pgsAlignmentDescriptionStatusCallback(IBroker* pBroker,eafTypes::StatusSeverityType severity):
m_pBroker(pBroker), m_Severity(severity)
{
}

eafTypes::StatusSeverityType pgsAlignmentDescriptionStatusCallback::GetSeverity() const
{
   return m_Severity;
}

void pgsAlignmentDescriptionStatusCallback::Execute(CEAFStatusItem* pStatusItem)
{
   pgsAlignmentDescriptionStatusItem* pItem = dynamic_cast<pgsAlignmentDescriptionStatusItem*>(pStatusItem);
   ATLASSERT(pItem!=nullptr);

   if ( AfxMessageBox( pStatusItem->GetDescription(), MB_OK ) == IDOK )
   {
      GET_IFACE(IEditByUI,pEdit);
      pEdit->EditAlignmentDescription(pItem->m_DlgPage);
   }
}


////////////////

pgsConcreteStrengthStatusItem::pgsConcreteStrengthStatusItem(pgsConcreteStrengthStatusItem::ConcreteType concType,pgsConcreteStrengthStatusItem::ElementType elemType,const CSegmentKey& segmentKey,StatusGroupIDType statusGroupID,StatusCallbackIDType callbackID,LPCTSTR strDescription) :
pgsSegmentRelatedStatusItem(statusGroupID,callbackID,strDescription,segmentKey), m_ConcreteType(concType),m_ElementType(elemType),m_SegmentKey(segmentKey)
{
}

bool pgsConcreteStrengthStatusItem::IsEqual(CEAFStatusItem* pOther)
{
   pgsConcreteStrengthStatusItem* other = dynamic_cast<pgsConcreteStrengthStatusItem*>(pOther);
   if ( !other )
   {
      return false;
   }

   return (other->m_ConcreteType == m_ConcreteType && other->m_ElementType == m_ElementType && other->m_SegmentKey == m_SegmentKey);
}

//////////////////////////////////////////////////////////
pgsConcreteStrengthStatusCallback::pgsConcreteStrengthStatusCallback(IBroker* pBroker,eafTypes::StatusSeverityType severity):
m_pBroker(pBroker),
m_Severity(severity)
{
}

eafTypes::StatusSeverityType pgsConcreteStrengthStatusCallback::GetSeverity() const
{
   return m_Severity;
}

void pgsConcreteStrengthStatusCallback::Execute(CEAFStatusItem* pStatusItem)
{
   pgsConcreteStrengthStatusItem* pItem = dynamic_cast<pgsConcreteStrengthStatusItem*>(pStatusItem);
   ATLASSERT(pItem!=nullptr);

   GET_IFACE(IEditByUI,pEdit);

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

bool pgsPointLoadStatusItem::IsEqual(CEAFStatusItem* pOther)
{
   pgsPointLoadStatusItem* other = dynamic_cast<pgsPointLoadStatusItem*>(pOther);
   if ( !other )
   {
      return false;
   }

   return (other->m_LoadIndex == m_LoadIndex && other->m_SpanKey == m_SpanKey);
}

///////////////////////////////

pgsPointLoadStatusCallback::pgsPointLoadStatusCallback(IBroker* pBroker,eafTypes::StatusSeverityType severity):
m_pBroker(pBroker),
m_Severity(severity)
{
}

eafTypes::StatusSeverityType pgsPointLoadStatusCallback::GetSeverity() const
{
   return m_Severity;
}

void pgsPointLoadStatusCallback::Execute(CEAFStatusItem* pStatusItem)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   pgsPointLoadStatusItem* pItem = dynamic_cast<pgsPointLoadStatusItem*>(pStatusItem);
   ATLASSERT(pItem!=nullptr);

   CDealWithLoadDlg dlg;
   dlg.m_Message = pItem->GetDescription();

   INT_PTR result = dlg.DoModal();
   if ( result == CDealWithLoadDlg::IDDELETELOAD )
   {
      GET_IFACE(IEAFTransactions,pTxn);
      txnDeletePointLoad txn(pItem->m_LoadIndex);
      pTxn->Execute(txn);

      StatusItemIDType id = pItem->GetID();
      GET_IFACE(IEAFStatusCenter,pStatusCenter);
      pStatusCenter->RemoveByID(id);
   }
   else if (result == CDealWithLoadDlg::IDEDITLOAD)
   {
      GET_IFACE(IEditByUI,pEdit);
      if ( pEdit->EditPointLoad(pItem->m_LoadIndex) )
      {
         StatusItemIDType id = pItem->GetID();
         GET_IFACE(IEAFStatusCenter,pStatusCenter);
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

bool pgsDistributedLoadStatusItem::IsEqual(CEAFStatusItem* pOther)
{
   pgsDistributedLoadStatusItem* other = dynamic_cast<pgsDistributedLoadStatusItem*>(pOther);
   if ( !other )
   {
      return false;
   }

   return (other->m_LoadIndex == m_LoadIndex && other->m_SpanKey == m_SpanKey);
}

//////////////////////////////////////////////////////////

pgsDistributedLoadStatusCallback::pgsDistributedLoadStatusCallback(IBroker* pBroker,eafTypes::StatusSeverityType severity):
m_pBroker(pBroker),
m_Severity(severity)
{
}

eafTypes::StatusSeverityType pgsDistributedLoadStatusCallback::GetSeverity() const
{
   return m_Severity;
}

void pgsDistributedLoadStatusCallback::Execute(CEAFStatusItem* pStatusItem)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   pgsDistributedLoadStatusItem* pItem = dynamic_cast<pgsDistributedLoadStatusItem*>(pStatusItem);
   ATLASSERT(pItem!=nullptr);

   CDealWithLoadDlg dlg;
   dlg.m_Message = pItem->GetDescription();

   INT_PTR result = dlg.DoModal();

   if ( result == CDealWithLoadDlg::IDDELETELOAD )
   {
      GET_IFACE(IEAFTransactions,pTxn);
      txnDeleteDistributedLoad txn(pItem->m_LoadIndex);
      pTxn->Execute(txn);

      StatusItemIDType id = pItem->GetID();
      GET_IFACE(IEAFStatusCenter,pStatusCenter);
      pStatusCenter->RemoveByID(id);
   }
   else if (result == CDealWithLoadDlg::IDEDITLOAD)
   {
      GET_IFACE(IEditByUI,pEdit);
      if ( pEdit->EditDistributedLoad(pItem->m_LoadIndex) )
      {
         StatusItemIDType id = pItem->GetID();
         GET_IFACE(IEAFStatusCenter,pStatusCenter);
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

bool pgsMomentLoadStatusItem::IsEqual(CEAFStatusItem* pOther)
{
   pgsMomentLoadStatusItem* other = dynamic_cast<pgsMomentLoadStatusItem*>(pOther);
   if ( !other )
   {
      return false;
   }

   return (other->m_LoadIndex == m_LoadIndex && other->m_SpanKey == m_SpanKey);
}
///////////////////////////

pgsMomentLoadStatusCallback::pgsMomentLoadStatusCallback(IBroker* pBroker,eafTypes::StatusSeverityType severity):
m_pBroker(pBroker),
m_Severity(severity)
{
}

eafTypes::StatusSeverityType pgsMomentLoadStatusCallback::GetSeverity() const
{
   return m_Severity;
}

void pgsMomentLoadStatusCallback::Execute(CEAFStatusItem* pStatusItem)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   pgsMomentLoadStatusItem* pItem = dynamic_cast<pgsMomentLoadStatusItem*>(pStatusItem);
   ATLASSERT(pItem!=nullptr);

   CDealWithLoadDlg dlg;
   dlg.m_Message = pItem->GetDescription();

   INT_PTR result = dlg.DoModal();

   if ( result == CDealWithLoadDlg::IDDELETELOAD )
   {
      GET_IFACE(IEAFTransactions,pTxn);
      txnDeleteMomentLoad txn(pItem->m_LoadIndex);
      pTxn->Execute(txn);

      StatusItemIDType id = pItem->GetID();
      GET_IFACE(IEAFStatusCenter,pStatusCenter);
      pStatusCenter->RemoveByID(id);
   }
   else if (result == CDealWithLoadDlg::IDEDITLOAD)
   {
      GET_IFACE(IEditByUI,pEdit);
      if ( pEdit->EditMomentLoad(pItem->m_LoadIndex) )
      {
         StatusItemIDType id = pItem->GetID();
         GET_IFACE(IEAFStatusCenter,pStatusCenter);
         pStatusCenter->RemoveByID(id);
      }
   }
   else
   {
      ATLASSERT(result == IDCANCEL);
   }
}
