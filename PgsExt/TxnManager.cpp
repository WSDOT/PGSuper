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

#include <PgsExt\PgsExtLib.h>
#include <PgsExt\TxnManager.h>
#include <EAF\EAFUtilities.h>


std::unique_ptr<WBFL::EAF::TxnManager> pgsTxnManagerFactory::CreateTransactionManager()
{
   return std::make_unique<pgsTxnManager>();
}

void pgsTxnManager::Execute(std::unique_ptr<WBFL::EAF::Transaction>&& pTxn)
{
   try
   {
      WBFL::EAF::TxnManager::Execute(std::move(pTxn));
   }
   catch (...)
   {
      m_TxnHistory.emplace_back(std::move(pTxn));
      m_Mode = WBFL::EAF::TxnManager::Mode::Repeat;
      throw;
   }
}
