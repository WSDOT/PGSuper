///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2020  Washington State Department of Transportation
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

#include <PgsExt\PgsExtLib.h>
#include <PgsExt\MacroTxn.h>

#include <EAF\EAFUtilities.h>
#include <IFace\Project.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

pgsMacroTxn::pgsMacroTxn()
{
}

pgsMacroTxn::~pgsMacroTxn()
{
}

bool pgsMacroTxn::Execute()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IEvents, pEvents);

   // Exception-safe holder for events
   CIEventsHolder event_holder(pEvents);

   bool bResult = txnMacroTxn::Execute();

   return bResult;
}

void pgsMacroTxn::Undo()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IEvents, pEvents);

   // Exception-safe holder for events
   CIEventsHolder event_holder(pEvents);

   txnMacroTxn::Undo();
}

txnTransaction* pgsMacroTxn::CreateClone() const
{
   txnMacroTxn* pClone = new pgsMacroTxn;

   TxnConstIterator begin = m_Transactions.begin();
   TxnConstIterator end   = m_Transactions.end();
   while ( begin != end )
   {
      TransactionPtr pTxn = *begin++;

      // Use the reference version.
      // We are not giving up ownership of the txn
      pClone->AddTransaction(*pTxn);
   }

   pClone->Name(m_Name);

   return pClone;
}
