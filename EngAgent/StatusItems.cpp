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
#include "StatusItems.h"
#include <IFace\EditByUI.h>
#include <IFace\Views.h>
#include <EAF/EAFUtilities.h>

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

pgsLiveLoadStatusCallback::pgsLiveLoadStatusCallback()
{
}

eafTypes::StatusSeverityType pgsLiveLoadStatusCallback::GetSeverity() const
{
   return eafTypes::statusWarning;
}

void pgsLiveLoadStatusCallback::Execute(CEAFStatusItem* pStatusItem)
{
   auto broker = EAFGetBroker();
   EAF_GET_IFACE2(broker,IEditByUI,pEdit);
   pEdit->EditLiveLoads();
}

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
pgsLiftingSupportLocationStatusCallback::pgsLiftingSupportLocationStatusCallback(eafTypes::StatusSeverityType severity):
m_Severity(severity)
{
}

eafTypes::StatusSeverityType pgsLiftingSupportLocationStatusCallback::GetSeverity() const
{
   return m_Severity;
}

void pgsLiftingSupportLocationStatusCallback::Execute(CEAFStatusItem* pStatusItem)
{
   pgsLiftingSupportLocationStatusItem* pItem = dynamic_cast<pgsLiftingSupportLocationStatusItem*>(pStatusItem);
   ATLASSERT(pItem!=nullptr);

   auto broker = EAFGetBroker();
   EAF_GET_IFACE2(broker, IEditByUI, pEdit);

   pEdit->EditSegmentDescription(pItem->m_SegmentKey,EGD_TRANSPORTATION);
}


pgsHaulTruckStatusItem::pgsHaulTruckStatusItem(StatusGroupIDType statusGroupID,StatusCallbackIDType callbackID,LPCTSTR strDescription) :
CEAFStatusItem(statusGroupID,callbackID,strDescription)
{
}

bool pgsHaulTruckStatusItem::IsEqual(CEAFStatusItem* pOther)
{
   pgsHaulTruckStatusItem* other = dynamic_cast<pgsHaulTruckStatusItem*>(pOther);
   if ( !other )
      return false;

   return true;
}

pgsHaulTruckStatusCallback::pgsHaulTruckStatusCallback()
{
}

eafTypes::StatusSeverityType pgsHaulTruckStatusCallback::GetSeverity() const
{
   return eafTypes::statusError;
}

void pgsHaulTruckStatusCallback::Execute(CEAFStatusItem* pStatusItem)
{
   //pgsHaulTruckStatusItem* pItem = dynamic_cast<pgsHaulTruckStatusItem*>(pStatusItem);
   //ATLASSERT(pItem!=nullptr);

   auto broker = EAFGetBroker();
   EAF_GET_IFACE2(broker, IViews, pViews);
   pViews->CreateLibraryEditorView();
}

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

pgsBunkPointLocationStatusCallback::pgsBunkPointLocationStatusCallback()
{
}

eafTypes::StatusSeverityType pgsBunkPointLocationStatusCallback::GetSeverity() const
{
   return eafTypes::statusWarning;
}

void pgsBunkPointLocationStatusCallback::Execute(CEAFStatusItem* pStatusItem)
{
   pgsBunkPointLocationStatusItem* pItem = dynamic_cast<pgsBunkPointLocationStatusItem*>(pStatusItem);
   ATLASSERT(pItem!=nullptr);

   auto broker = EAFGetBroker();
   EAF_GET_IFACE2(broker, IEditByUI, pEdit);
   pEdit->EditSegmentDescription(pItem->m_SegmentKey,EGD_TRANSPORTATION);
}

