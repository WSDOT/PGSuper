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
#include <IFace\EditByUI.h>

////////////////

pgsLiveLoadStatusItem::pgsLiveLoadStatusItem(StatusGroupIDType statusGroupID,StatusCallbackIDType callbackID,const char* strDescription) :
CEAFStatusItem(statusGroupID,callbackID,strDescription)
{
}

bool pgsLiveLoadStatusItem::IsEqual(CEAFStatusItem* pOther)
{
   pgsLiveLoadStatusItem* other = dynamic_cast<pgsLiveLoadStatusItem*>(pOther);
   if ( !other )
      return false;

   return true;
}

//////////////////////////////////////////////////////////
pgsLiveLoadStatusCallback::pgsLiveLoadStatusCallback(IBroker* pBroker):
m_pBroker(pBroker)
{
}

eafTypes::StatusSeverityType pgsLiveLoadStatusCallback::GetSeverity()
{
   return eafTypes::statusWarning;
}

void pgsLiveLoadStatusCallback::Execute(CEAFStatusItem* pStatusItem)
{
   GET_IFACE(IEditByUI,pEdit);
   pEdit->EditLiveLoads();
}

////////////////

pgsLiftingSupportLocationStatusItem::pgsLiftingSupportLocationStatusItem(SpanIndexType span,GirderIndexType gdr,StatusGroupIDType statusGroupID,StatusCallbackIDType callbackID,const char* strDescription) :
CEAFStatusItem(statusGroupID,callbackID,strDescription), m_Span(span),m_Girder(gdr)
{
}

bool pgsLiftingSupportLocationStatusItem::IsEqual(CEAFStatusItem* pOther)
{
   pgsLiftingSupportLocationStatusItem* other = dynamic_cast<pgsLiftingSupportLocationStatusItem*>(pOther);
   if ( !other )
      return false;

   return (other->m_Span == m_Span && other->m_Girder == m_Girder);
}

//////////////////////////////////////////////////////////
pgsLiftingSupportLocationStatusCallback::pgsLiftingSupportLocationStatusCallback(IBroker* pBroker):
m_pBroker(pBroker)
{
}

eafTypes::StatusSeverityType pgsLiftingSupportLocationStatusCallback::GetSeverity()
{
   return eafTypes::statusError;
}

void pgsLiftingSupportLocationStatusCallback::Execute(CEAFStatusItem* pStatusItem)
{
   pgsLiftingSupportLocationStatusItem* pItem = dynamic_cast<pgsLiftingSupportLocationStatusItem*>(pStatusItem);
   ATLASSERT(pItem!=NULL);

   GET_IFACE(IEditByUI,pEdit);
   pEdit->EditGirderDescription(pItem->m_Span,pItem->m_Girder,EGD_TRANSPORTATION);
}

////////////////

pgsTruckStiffnessStatusItem::pgsTruckStiffnessStatusItem(StatusGroupIDType statusGroupID,StatusCallbackIDType callbackID,const char* strDescription) :
CEAFStatusItem(statusGroupID,callbackID,strDescription)
{
}

bool pgsTruckStiffnessStatusItem::IsEqual(CEAFStatusItem* pOther)
{
   pgsTruckStiffnessStatusItem* other = dynamic_cast<pgsTruckStiffnessStatusItem*>(pOther);
   if ( !other )
      return false;

   return true;
}

//////////////////////////////////////////////////////////
pgsTruckStiffnessStatusCallback::pgsTruckStiffnessStatusCallback(IBroker* pBroker):
m_pBroker(pBroker)
{
}

eafTypes::StatusSeverityType pgsTruckStiffnessStatusCallback::GetSeverity()
{
   return eafTypes::statusError;
}

void pgsTruckStiffnessStatusCallback::Execute(CEAFStatusItem* pStatusItem)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   pgsTruckStiffnessStatusItem* pItem = dynamic_cast<pgsTruckStiffnessStatusItem*>(pStatusItem);
   ATLASSERT(pItem!=NULL);

   CString msg;
   msg.Format("%s\n\nThe truck roll stiffness is specified in the Hauling Parameters of the Design Criteria\nDesign Criteria may be viewed in the Library Editor",pStatusItem->GetDescription().c_str());
   AfxMessageBox(msg);
}
