///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2016  Washington State Department of Transportation
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

pgsVSRatioStatusItem::pgsVSRatioStatusItem(SpanIndexType span,GirderIndexType gdr,StatusGroupIDType statusGroupID,StatusCallbackIDType callbackID,LPCTSTR strDescription) :
pgsSpanGirderRelatedStatusItem(statusGroupID,callbackID,strDescription,span,gdr), m_Span(span),m_Girder(gdr)
{
}

bool pgsVSRatioStatusItem::IsEqual(CEAFStatusItem* pOther)
{
   pgsVSRatioStatusItem* other = dynamic_cast<pgsVSRatioStatusItem*>(pOther);
   if ( !other )
      return false;

   return (other->m_Span == m_Span && other->m_Girder == m_Girder);
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

   CString msg;
   msg.Format(_T("Span %d Girder %s: %s"),LABEL_SPAN(pItem->m_Span),LABEL_GIRDER(pItem->m_Girder),pItem->GetDescription().c_str());
   AfxMessageBox(msg);
}
