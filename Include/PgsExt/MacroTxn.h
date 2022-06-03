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

#pragma once

#if !defined INCLUDED_PGSEXTEXP_H_
#include <PgsExt\PgsExtExp.h>
#endif

#include <System\MacroTxn.h>

/*****************************************************************************
CLASS 
   pgsMacroTxn

   A macro transaction that holds all events until every sub-transaction
   has executed

DESCRIPTION
   A macro transaction is a collection of other transactions.

KEYWORDS
   txnTransaction, txnTxnManager
*****************************************************************************/

class PGSEXTCLASS pgsMacroTxn : public txnMacroTxn
{
public:
   pgsMacroTxn();
   virtual ~pgsMacroTxn(); 

   virtual bool Execute() override;
   virtual void Undo() override;

   txnTransaction* CreateClone() const;

private:
   pgsMacroTxn(const pgsMacroTxn& /*rOther*/) = delete;               // Remove to enable copy
   pgsMacroTxn& operator=(const pgsMacroTxn& /*rOther*/) = delete;  // Remove to enable assignment
};
