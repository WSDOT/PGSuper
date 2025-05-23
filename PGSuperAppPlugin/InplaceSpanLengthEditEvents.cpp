///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright � 1999-2025  Washington State Department of Transportation
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
#include "PGSuperApp.h"
#include "InplaceSpanLengthEditEvents.h"
#include "EditSpanLength.h"
#include <PgsExt\BridgeDescription2.h>
#include <IFace\Project.h>
#include <EAF\EAFTxnManager.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CInplaceSpanLengthEditEvents::CInplaceSpanLengthEditEvents(IBroker* pBroker,SpanIndexType spanIdx) :
CInplaceEditDisplayObjectEvents(pBroker), m_SpanIdx(spanIdx)
{
}

void CInplaceSpanLengthEditEvents::Handle_OnChanged(iDisplayObject* pDO)
{
   CComQIPtr<iEditableUnitValueTextBlock> pTextBlock(pDO);
   ATLASSERT(pTextBlock);

   Float64 new_span_length = pTextBlock->GetEditedValue();

   if ( IsLE(new_span_length,0.0) )
   {
      AfxMessageBox(_T("Invalid span length"),MB_OK | MB_ICONEXCLAMATION);
      return;
   }

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   Float64 old_span_length = pBridgeDesc->GetSpan(m_SpanIdx)->GetSpanLength();

   if ( !IsEqual(old_span_length,new_span_length) )
   {
      std::unique_ptr<txnEditSpanLength> pTxn(std::make_unique<txnEditSpanLength>(m_SpanIdx,old_span_length,new_span_length));
      CEAFTxnManager::GetInstance().Execute(std::move(pTxn));
   }
}
