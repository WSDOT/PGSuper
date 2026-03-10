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
#include "EngAgent.h"
#include "StatusItems.h"
#include <IFace\EditByUI.h>
#include <IFace\Views.h>
#include <EAF/EAFUtilities.h>

pgsLiveLoadStatusItem::pgsLiveLoadStatusItem(StatusGroupIDType statusGroupID,StatusCallbackIDType callbackID,LPCTSTR strDescription) :
WBFL::EAF::StatusItem(statusGroupID,callbackID,strDescription)
{
}

bool pgsLiveLoadStatusItem::IsEqual(std::shared_ptr<const WBFL::EAF::StatusItem> pOther) const
{
   auto other = std::dynamic_pointer_cast<const pgsLiveLoadStatusItem>(pOther);
   if ( !other )
      return false;

   return true;
}

pgsLiveLoadStatusCallback::pgsLiveLoadStatusCallback()
{
}

WBFL::EAF::StatusSeverityType pgsLiveLoadStatusCallback::GetSeverity() const
{
   return WBFL::EAF::StatusSeverityType::Warning;
}

void pgsLiveLoadStatusCallback::Execute(std::shared_ptr<WBFL::EAF::StatusItem> pStatusItem)
{
   auto broker = EAFGetBroker();
   GET_IFACE2(broker,IEditByUI,pEdit);
   pEdit->EditLiveLoads();
}

pgsLiftingSupportLocationStatusItem::pgsLiftingSupportLocationStatusItem(const CSegmentKey& segmentKey,pgsTypes::MemberEndType end,StatusGroupIDType statusGroupID,StatusCallbackIDType callbackID,LPCTSTR strDescription) :
WBFL::EAF::StatusItem(statusGroupID,callbackID,strDescription), m_SegmentKey(segmentKey), m_End(end)
{
}

bool pgsLiftingSupportLocationStatusItem::IsEqual(std::shared_ptr<const WBFL::EAF::StatusItem> pOther) const
{
   auto other = std::dynamic_pointer_cast<const pgsLiftingSupportLocationStatusItem>(pOther);
   if ( !other )
      return false;

   return (m_SegmentKey == other->m_SegmentKey) && (m_End == other->m_End);
}

//////////////////////////////////////////////////////////
pgsLiftingSupportLocationStatusCallback::pgsLiftingSupportLocationStatusCallback(WBFL::EAF::StatusSeverityType severity):
m_Severity(severity)
{
}

WBFL::EAF::StatusSeverityType pgsLiftingSupportLocationStatusCallback::GetSeverity() const
{
   return m_Severity;
}

void pgsLiftingSupportLocationStatusCallback::Execute(std::shared_ptr<WBFL::EAF::StatusItem> pStatusItem)
{
   auto pItem = std::dynamic_pointer_cast<pgsLiftingSupportLocationStatusItem>(pStatusItem);
   ATLASSERT(pItem!=nullptr);

   auto broker = EAFGetBroker();
   GET_IFACE2(broker, IEditByUI, pEdit);

   pEdit->EditSegmentDescription(pItem->m_SegmentKey,EGD_TRANSPORTATION);
}


pgsHaulTruckStatusItem::pgsHaulTruckStatusItem(StatusGroupIDType statusGroupID,StatusCallbackIDType callbackID,LPCTSTR strDescription) :
WBFL::EAF::StatusItem(statusGroupID,callbackID,strDescription)
{
}

bool pgsHaulTruckStatusItem::IsEqual(std::shared_ptr<const WBFL::EAF::StatusItem> pOther) const
{
   auto other = std::dynamic_pointer_cast<const pgsHaulTruckStatusItem>(pOther);
   if ( !other )
      return false;

   return true;
}

pgsHaulTruckStatusCallback::pgsHaulTruckStatusCallback()
{
}

WBFL::EAF::StatusSeverityType pgsHaulTruckStatusCallback::GetSeverity() const
{
   return WBFL::EAF::StatusSeverityType::Error;
}

void pgsHaulTruckStatusCallback::Execute(std::shared_ptr<WBFL::EAF::StatusItem> pStatusItem)
{
   //pgsHaulTruckStatusItem* pItem = dynamic_cast<pgsHaulTruckStatusItem*>(pStatusItem);
   //ATLASSERT(pItem!=nullptr);

   auto broker = EAFGetBroker();
   GET_IFACE2(broker, IViews, pViews);
   pViews->CreateLibraryEditorView();
}

pgsBunkPointLocationStatusItem::pgsBunkPointLocationStatusItem(const CSegmentKey& segmentKey,pgsTypes::MemberEndType end,StatusGroupIDType statusGroupID,StatusCallbackIDType callbackID,LPCTSTR strDescription) :
WBFL::EAF::StatusItem(statusGroupID,callbackID,strDescription), m_SegmentKey(segmentKey), m_End(end)
{
}

bool pgsBunkPointLocationStatusItem::IsEqual(std::shared_ptr<const WBFL::EAF::StatusItem> pOther) const
{
   auto other = std::dynamic_pointer_cast<const pgsBunkPointLocationStatusItem>(pOther);
   if ( !other )
      return false;

   return (m_SegmentKey == other->m_SegmentKey) && (m_End == other->m_End);
}

pgsBunkPointLocationStatusCallback::pgsBunkPointLocationStatusCallback()
{
}

WBFL::EAF::StatusSeverityType pgsBunkPointLocationStatusCallback::GetSeverity() const
{
   return WBFL::EAF::StatusSeverityType::Warning;
}

void pgsBunkPointLocationStatusCallback::Execute(std::shared_ptr<WBFL::EAF::StatusItem> pStatusItem)
{
   auto pItem = std::dynamic_pointer_cast<pgsBunkPointLocationStatusItem>(pStatusItem);
   ATLASSERT(pItem!=nullptr);

   auto broker = EAFGetBroker();
   GET_IFACE2(broker, IEditByUI, pEdit);
   pEdit->EditSegmentDescription(pItem->m_SegmentKey,EGD_TRANSPORTATION);
}

