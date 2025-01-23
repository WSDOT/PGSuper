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
#include "EditLLDF.h"
#include "PGSuperDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

txnEditLLDF::txnEditLLDF(const CBridgeDescription2& oldBridgeDesc,const CBridgeDescription2& newBridgeDesc,
                         WBFL::LRFD::RangeOfApplicabilityAction oldROA, WBFL::LRFD::RangeOfApplicabilityAction newROA)
{
   m_pBridgeDesc[0] = new CBridgeDescription2(oldBridgeDesc);
   m_pBridgeDesc[1] = new CBridgeDescription2(newBridgeDesc);

   m_ROA[0] = oldROA;
   m_ROA[1] = newROA;
}

txnEditLLDF::~txnEditLLDF()
{
   delete m_pBridgeDesc[0];
   delete m_pBridgeDesc[1];
}

bool txnEditLLDF::Execute()
{
   DoExecute(1);
   return true;
}

void txnEditLLDF::Undo()
{
   DoExecute(0);
}

std::unique_ptr<CEAFTransaction> txnEditLLDF::CreateClone() const
{
   return std::make_unique<txnEditLLDF>(*m_pBridgeDesc[0], *m_pBridgeDesc[1],m_ROA[0],m_ROA[1]);
}

std::_tstring txnEditLLDF::Name() const
{
   return _T("Edit Live Load Distribution Factors");
}

bool txnEditLLDF::IsUndoable() const
{
   return true;
}

bool txnEditLLDF::IsRepeatable() const
{
   return false;
}

void txnEditLLDF::DoExecute(int i)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IEvents, pEvents);
   // Exception-safe holder to keep from fireing events until we are done
   CIEventsHolder event_holder(pEvents);

   GET_IFACE2(pBroker,IBridgeDescription,pBridgeDesc);
   pBridgeDesc->SetBridgeDescription( *m_pBridgeDesc[i] );

   GET_IFACE2(pBroker,ILiveLoads,pLiveLoads);
   pLiveLoads->SetRangeOfApplicabilityAction(m_ROA[i]);
}
