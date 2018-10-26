///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2014  Washington State Department of Transportation
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

#include <PgsExt\GirderLabel.h>

#include <IFace\DocumentType.h>

pgsVSRatioStatusItem::pgsVSRatioStatusItem(const CSegmentKey& segmentKey,StatusGroupIDType statusGroupID,StatusCallbackIDType callbackID,LPCTSTR strDescription) :
pgsSegmentRelatedStatusItem(statusGroupID,callbackID,strDescription,segmentKey), m_SegmentKey(segmentKey)
{
}

bool pgsVSRatioStatusItem::IsEqual(CEAFStatusItem* pOther)
{
   pgsVSRatioStatusItem* other = dynamic_cast<pgsVSRatioStatusItem*>(pOther);
   if ( !other )
      return false;

   return (m_SegmentKey == other->m_SegmentKey);
}

//////////////////////////////////////////////////////////
pgsVSRatioStatusCallback::pgsVSRatioStatusCallback(IBroker* pBroker):
m_pBroker(pBroker)
{
}

eafTypes::StatusSeverityType pgsVSRatioStatusCallback::GetSeverity()
{
   return eafTypes::statusError;
}

void pgsVSRatioStatusCallback::Execute(CEAFStatusItem* pStatusItem)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   pgsVSRatioStatusItem* pItem = dynamic_cast<pgsVSRatioStatusItem*>(pStatusItem);
   ATLASSERT(pItem!=NULL);

   GET_IFACE(IDocumentType,pDocType);
   CString strMsg;
   if ( pDocType->IsPGSuperDocument() )
   {
      ATLASSERT(pItem->m_SegmentKey.segmentIndex == 0);

      strMsg.Format(_T("Span %d Girder %s: %s"),LABEL_SPAN(pItem->m_SegmentKey.groupIndex),LABEL_GIRDER(pItem->m_SegmentKey.girderIndex),pItem->GetDescription().c_str());
   }
   else
   {
      strMsg.Format(_T("Group %d Girder %s Segment %d: %s"),LABEL_SPAN(pItem->m_SegmentKey.groupIndex),LABEL_GIRDER(pItem->m_SegmentKey.girderIndex),LABEL_SEGMENT(pItem->m_SegmentKey.segmentIndex),pItem->GetDescription().c_str());
   }
   AfxMessageBox(strMsg);
}
