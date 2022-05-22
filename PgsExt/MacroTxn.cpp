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

#include <PgsExt\PgsExtLib.h>
#include <PgsExt\MacroTxn.h>

#include <EAF\EAFUtilities.h>
#include <IFace\Project.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

bool pgsMacroTxn::Execute()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IEvents, pEvents);

   // Exception-safe holder for events
   CIEventsHolder event_holder(pEvents);

   bool bResult = CEAFMacroTxn::Execute();

   return bResult;
}

void pgsMacroTxn::Undo()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IEvents, pEvents);

   // Exception-safe holder for events
   CIEventsHolder event_holder(pEvents);

   CEAFMacroTxn::Undo();
}

std::unique_ptr<CEAFTransaction> pgsMacroTxn::CreateClone() const
{
   std::unique_ptr<pgsMacroTxn> clone(std::make_unique<pgsMacroTxn>());
   clone->Name(m_Name);
   std::for_each(std::begin(m_Transactions), std::end(m_Transactions), [&clone](auto& txn) {clone->AddTransaction(*txn); });
   return clone;
}
