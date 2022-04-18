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

#include <IFace\EditByUI.h>
#include <IFace\StatusCenter.h>
#include <IFace\DocumentType.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



pgsHaulTruckStatusItem::pgsHaulTruckStatusItem(StatusGroupIDType statusGroupID,StatusCallbackIDType callbackID,LPCTSTR strDescription, const CSegmentKey& segmentKey) :
pgsSegmentRelatedStatusItem(statusGroupID,callbackID,strDescription,segmentKey), m_SegmentKey(segmentKey)
{
}

bool pgsHaulTruckStatusItem::IsEqual(CEAFStatusItem* pOther)
{
   pgsHaulTruckStatusItem* other = dynamic_cast<pgsHaulTruckStatusItem*>(pOther);
   if ( !other )
   {
      return false;
   }

   return (other->m_SegmentKey == m_SegmentKey);
}

//////////////////////////////////////////////////////////
pgsHaulTruckStatusCallback::pgsHaulTruckStatusCallback(IBroker* pBroker,eafTypes::StatusSeverityType severity):
m_pBroker(pBroker),
m_Severity(severity)
{
}

eafTypes::StatusSeverityType pgsHaulTruckStatusCallback::GetSeverity() const
{
   return m_Severity;
}

void pgsHaulTruckStatusCallback::Execute(CEAFStatusItem* pStatusItem)
{
   pgsHaulTruckStatusItem* pItem = dynamic_cast<pgsHaulTruckStatusItem*>(pStatusItem);
   ATLASSERT(pItem!=nullptr);

   GET_IFACE(IDocumentType, pDocType);
   GET_IFACE(IEditByUI,pEdit);
   if (pDocType->IsPGSuperDocument())
   {
      pEdit->EditGirderDescription(pItem->m_SegmentKey, EGD_TRANSPORTATION);
   }
   else
   {
      pEdit->EditSegmentDescription(pItem->m_SegmentKey, EGS_TRANSPORTATION);
   }
}
