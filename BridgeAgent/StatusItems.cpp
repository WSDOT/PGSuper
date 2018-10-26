///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2010  Washington State Department of Transportation
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
#include "DealWithLoadDlg.h"

#include <IFace\EditByUI.h>
#include <IFace\Project.h>
#include <IFace\StatusCenter.h>

////////////////

////////////////

pgsAlignmentDescriptionStatusItem::pgsAlignmentDescriptionStatusItem(AgentIDType agentID,StatusCallbackIDType callbackID,long dlgPage,const char* strDescription) :
pgsStatusItem(agentID,callbackID,strDescription), m_DlgPage(dlgPage)
{
}

bool pgsAlignmentDescriptionStatusItem::IsEqual(pgsStatusItem* pOther)
{
   pgsAlignmentDescriptionStatusItem* other = dynamic_cast<pgsAlignmentDescriptionStatusItem*>(pOther);
   if ( !other )
      return false;

   if ( this->GetDescription() != other->GetDescription() )
      return false;

   if ( m_DlgPage != other->m_DlgPage )
      return false;

   return true;
}

//////////////////////////////////////////////////////////
pgsAlignmentDescriptionStatusCallback::pgsAlignmentDescriptionStatusCallback(IBroker* pBroker,pgsTypes::StatusSeverityType severity):
m_pBroker(pBroker), m_Severity(severity)
{
}

pgsTypes::StatusSeverityType pgsAlignmentDescriptionStatusCallback::GetSeverity()
{
   return m_Severity;
}

void pgsAlignmentDescriptionStatusCallback::Execute(pgsStatusItem* pStatusItem)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   pgsAlignmentDescriptionStatusItem* pItem = dynamic_cast<pgsAlignmentDescriptionStatusItem*>(pStatusItem);
   ATLASSERT(pItem!=NULL);

   if ( AfxMessageBox( pStatusItem->GetDescription().c_str(), MB_OK ) == IDOK )
   {
      GET_IFACE(IEditByUI,pEdit);
      pEdit->EditAlignmentDescription(pItem->m_DlgPage);
   }
}


////////////////

pgsConcreteStrengthStatusItem::pgsConcreteStrengthStatusItem(pgsConcreteStrengthStatusItem::ConcreteType concType,pgsConcreteStrengthStatusItem::ElementType elemType,SpanIndexType span,GirderIndexType gdr,AgentIDType agentID,StatusCallbackIDType callbackID,const char* strDescription) :
pgsStatusItem(agentID,callbackID,strDescription), m_ConcreteType(concType),m_ElementType(elemType),m_Span(span),m_Girder(gdr)
{
}

bool pgsConcreteStrengthStatusItem::IsEqual(pgsStatusItem* pOther)
{
   pgsConcreteStrengthStatusItem* other = dynamic_cast<pgsConcreteStrengthStatusItem*>(pOther);
   if ( !other )
      return false;

   return (other->m_ConcreteType == m_ConcreteType && other->m_ElementType == m_ElementType && other->m_Span == m_Span && other->m_Girder == m_Girder);
}

//////////////////////////////////////////////////////////
pgsConcreteStrengthStatusCallback::pgsConcreteStrengthStatusCallback(IBroker* pBroker,pgsTypes::StatusSeverityType severity):
m_pBroker(pBroker),
m_Severity(severity)
{
}

pgsTypes::StatusSeverityType pgsConcreteStrengthStatusCallback::GetSeverity()
{
   return m_Severity;
}

void pgsConcreteStrengthStatusCallback::Execute(pgsStatusItem* pStatusItem)
{
   pgsConcreteStrengthStatusItem* pItem = dynamic_cast<pgsConcreteStrengthStatusItem*>(pStatusItem);
   ATLASSERT(pItem!=NULL);

   GET_IFACE(IEditByUI,pEdit);

   if ( pItem->m_ConcreteType == pgsConcreteStrengthStatusItem::Slab )
   {
      pEdit->EditBridgeDescription(EBD_DECK);
   }
   else if ( pItem->m_ConcreteType == pgsConcreteStrengthStatusItem::Girder )
   {
      pEdit->EditGirderDescription(pItem->m_Span,pItem->m_Girder,EGD_CONCRETE);
   }
   else
   {
      pEdit->EditBridgeDescription(EBD_RAILING);
   }
}

//////////////////////

pgsPointLoadStatusItem::pgsPointLoadStatusItem(Uint32 value,AgentIDType agentID,StatusCallbackIDType callbackID,const char* strDescription) :
pgsStatusItem(agentID,callbackID,strDescription), m_LoadIndex(value)
{
}

bool pgsPointLoadStatusItem::IsEqual(pgsStatusItem* pOther)
{
   pgsPointLoadStatusItem* other = dynamic_cast<pgsPointLoadStatusItem*>(pOther);
   if ( !other )
      return false;

   return (other->m_LoadIndex == m_LoadIndex);
}

///////////////////////////////

pgsPointLoadStatusCallback::pgsPointLoadStatusCallback(IBroker* pBroker,pgsTypes::StatusSeverityType severity):
m_pBroker(pBroker),
m_Severity(severity)
{
}

pgsTypes::StatusSeverityType pgsPointLoadStatusCallback::GetSeverity()
{
   return m_Severity;
}

void pgsPointLoadStatusCallback::Execute(pgsStatusItem* pStatusItem)
{
   pgsPointLoadStatusItem* pItem = dynamic_cast<pgsPointLoadStatusItem*>(pStatusItem);
   ATLASSERT(pItem!=NULL);

   CDealWithLoadDlg dlg;
   dlg.m_Message = pItem->GetDescription().c_str();

   int result = dlg.DoModal();
   GET_IFACE(IUserDefinedLoadData, pUserDefinedLoads);
   GET_IFACE(IStatusCenter,pStatusCenter);

   if ( result == CDealWithLoadDlg::IDDELETELOAD )
   {
      pUserDefinedLoads->DeletePointLoad(pItem->m_LoadIndex);

      StatusItemIDType id = pItem->GetID();
      pStatusCenter->RemoveByID(id);
   }
   else if (result == CDealWithLoadDlg::IDEDITLOAD)
   {
      GET_IFACE(IEditByUI,pEdit);
      if ( pEdit->EditPointLoad(pItem->m_LoadIndex) )
      {
         StatusItemIDType id = pItem->GetID();
         pStatusCenter->RemoveByID(id);
      }
   }
   else
   {
      ATLASSERT(result == IDCANCEL);
   }
}

//////////////////////////////////////////////////////////

pgsDistributedLoadStatusItem::pgsDistributedLoadStatusItem(Uint32 value,AgentIDType agentID,StatusCallbackIDType callbackID,const char* strDescription) :
pgsStatusItem(agentID,callbackID,strDescription), m_LoadIndex(value)
{
}

bool pgsDistributedLoadStatusItem::IsEqual(pgsStatusItem* pOther)
{
   pgsDistributedLoadStatusItem* other = dynamic_cast<pgsDistributedLoadStatusItem*>(pOther);
   if ( !other )
      return false;

   return (other->m_LoadIndex == m_LoadIndex);
}

//////////////////////////////////////////////////////////

pgsDistributedLoadStatusCallback::pgsDistributedLoadStatusCallback(IBroker* pBroker,pgsTypes::StatusSeverityType severity):
m_pBroker(pBroker),
m_Severity(severity)
{
}

pgsTypes::StatusSeverityType pgsDistributedLoadStatusCallback::GetSeverity()
{
   return m_Severity;
}

void pgsDistributedLoadStatusCallback::Execute(pgsStatusItem* pStatusItem)
{
   pgsDistributedLoadStatusItem* pItem = dynamic_cast<pgsDistributedLoadStatusItem*>(pStatusItem);
   ATLASSERT(pItem!=NULL);

   CDealWithLoadDlg dlg;
   dlg.m_Message = pItem->GetDescription().c_str();

   int result = dlg.DoModal();
   GET_IFACE(IUserDefinedLoadData, pUserDefinedLoads);
   GET_IFACE(IStatusCenter,pStatusCenter);

   if ( result == CDealWithLoadDlg::IDDELETELOAD )
   {
      pUserDefinedLoads->DeleteDistributedLoad(pItem->m_LoadIndex);

      StatusItemIDType id = pItem->GetID();
      pStatusCenter->RemoveByID(id);
   }
   else if (result == CDealWithLoadDlg::IDEDITLOAD)
   {
      GET_IFACE(IEditByUI,pEdit);
      if ( pEdit->EditDistributedLoad(pItem->m_LoadIndex) )
      {
         StatusItemIDType id = pItem->GetID();
         pStatusCenter->RemoveByID(id);
      }
   }
   else
   {
      ATLASSERT(result == IDCANCEL);
   }
}

//////////////////////////////////////////////

pgsMomentLoadStatusItem::pgsMomentLoadStatusItem(Uint32 value,AgentIDType agentID,StatusCallbackIDType callbackID,const char* strDescription) :
pgsStatusItem(agentID,callbackID,strDescription), m_LoadIndex(value)
{
}

bool pgsMomentLoadStatusItem::IsEqual(pgsStatusItem* pOther)
{
   pgsMomentLoadStatusItem* other = dynamic_cast<pgsMomentLoadStatusItem*>(pOther);
   if ( !other )
      return false;

   return (other->m_LoadIndex == m_LoadIndex);
}
///////////////////////////

pgsMomentLoadStatusCallback::pgsMomentLoadStatusCallback(IBroker* pBroker,pgsTypes::StatusSeverityType severity):
m_pBroker(pBroker),
m_Severity(severity)
{
}

pgsTypes::StatusSeverityType pgsMomentLoadStatusCallback::GetSeverity()
{
   return m_Severity;
}

void pgsMomentLoadStatusCallback::Execute(pgsStatusItem* pStatusItem)
{
   pgsMomentLoadStatusItem* pItem = dynamic_cast<pgsMomentLoadStatusItem*>(pStatusItem);
   ATLASSERT(pItem!=NULL);

   CDealWithLoadDlg dlg;
   dlg.m_Message = pItem->GetDescription().c_str();

   int result = dlg.DoModal();
   GET_IFACE(IUserDefinedLoadData, pUserDefinedLoads);
   GET_IFACE(IStatusCenter,pStatusCenter);

   if ( result == CDealWithLoadDlg::IDDELETELOAD )
   {
      pUserDefinedLoads->DeleteMomentLoad(pItem->m_LoadIndex);

      StatusItemIDType id = pItem->GetID();
      pStatusCenter->RemoveByID(id);
   }
   else if (result == CDealWithLoadDlg::IDEDITLOAD)
   {
      GET_IFACE(IEditByUI,pEdit);
      if ( pEdit->EditMomentLoad(pItem->m_LoadIndex) )
      {
         StatusItemIDType id = pItem->GetID();
         pStatusCenter->RemoveByID(id);
      }
   }
   else
   {
      ATLASSERT(result == IDCANCEL);
   }
}
