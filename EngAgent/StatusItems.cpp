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

#include "stdafx.h"
#include "StatusItems.h"
#include <IFace\EditByUI.h>

////////////////

pgsLiveLoadStatusItem::pgsLiveLoadStatusItem(StatusGroupIDType statusGroupID,StatusCallbackIDType callbackID,LPCTSTR strDescription) :
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

pgsLiftingSupportLocationStatusItem::pgsLiftingSupportLocationStatusItem(const CSegmentKey& segmentKey,pgsTypes::MemberEndType end,StatusGroupIDType statusGroupID,StatusCallbackIDType callbackID,LPCTSTR strDescription) :
CEAFStatusItem(statusGroupID,callbackID,strDescription), m_SegmentKey(segmentKey), m_End(end)
{
}

bool pgsLiftingSupportLocationStatusItem::IsEqual(CEAFStatusItem* pOther)
{
   pgsLiftingSupportLocationStatusItem* other = dynamic_cast<pgsLiftingSupportLocationStatusItem*>(pOther);
   if ( !other )
      return false;

   return (m_SegmentKey == other->m_SegmentKey) && (m_End == other->m_End);
}

//////////////////////////////////////////////////////////
pgsLiftingSupportLocationStatusCallback::pgsLiftingSupportLocationStatusCallback(IBroker* pBroker,eafTypes::StatusSeverityType severity):
m_pBroker(pBroker),
m_Severity(severity)
{
}

eafTypes::StatusSeverityType pgsLiftingSupportLocationStatusCallback::GetSeverity()
{
   return m_Severity;
}

void pgsLiftingSupportLocationStatusCallback::Execute(CEAFStatusItem* pStatusItem)
{
   pgsLiftingSupportLocationStatusItem* pItem = dynamic_cast<pgsLiftingSupportLocationStatusItem*>(pStatusItem);
   ATLASSERT(pItem!=NULL);

   GET_IFACE(IEditByUI,pEdit);

   pEdit->EditSegmentDescription(pItem->m_SegmentKey,EGD_TRANSPORTATION);
}

////////////////

pgsTruckStiffnessStatusItem::pgsTruckStiffnessStatusItem(StatusGroupIDType statusGroupID,StatusCallbackIDType callbackID,LPCTSTR strDescription) :
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
   msg.Format(_T("%s\n\nThe truck roll stiffness is specified in the Hauling Parameters of the Design Criteria\nDesign Criteria may be viewed in the Library Editor"),pStatusItem->GetDescription());
   AfxMessageBox(msg);
}

////////////////

pgsBunkPointLocationStatusItem::pgsBunkPointLocationStatusItem(const CSegmentKey& segmentKey,pgsTypes::MemberEndType end,StatusGroupIDType statusGroupID,StatusCallbackIDType callbackID,LPCTSTR strDescription) :
CEAFStatusItem(statusGroupID,callbackID,strDescription), m_SegmentKey(segmentKey), m_End(end)
{
}

bool pgsBunkPointLocationStatusItem::IsEqual(CEAFStatusItem* pOther)
{
   pgsBunkPointLocationStatusItem* other = dynamic_cast<pgsBunkPointLocationStatusItem*>(pOther);
   if ( !other )
      return false;

   return (m_SegmentKey == other->m_SegmentKey) && (m_End == other->m_End);
}

//////////////////////////////////////////////////////////
pgsBunkPointLocationStatusCallback::pgsBunkPointLocationStatusCallback(IBroker* pBroker):
m_pBroker(pBroker)
{
}

eafTypes::StatusSeverityType pgsBunkPointLocationStatusCallback::GetSeverity()
{
   return eafTypes::statusWarning;
}

void pgsBunkPointLocationStatusCallback::Execute(CEAFStatusItem* pStatusItem)
{
   pgsBunkPointLocationStatusItem* pItem = dynamic_cast<pgsBunkPointLocationStatusItem*>(pStatusItem);
   ATLASSERT(pItem!=NULL);

   GET_IFACE(IEditByUI,pEdit);
   pEdit->EditSegmentDescription(pItem->m_SegmentKey,EGD_TRANSPORTATION);
}

