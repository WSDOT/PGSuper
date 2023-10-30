///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2023  Washington State Department of Transportation
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

#include <EAF\EAFMacroTxn.h>

/*****************************************************************************
CLASS 
   pgsMacroTxn

   A macro transaction that holds all events until every sub-transaction
   has executed

DESCRIPTION
   A macro transaction is a collection of other transactions.

KEYWORDS
   CEAFTransaction, CEAFTxnManager
*****************************************************************************/

class PGSEXTCLASS pgsMacroTxn : public CEAFMacroTxn
{
public:
   pgsMacroTxn() = default;
   virtual ~pgsMacroTxn() = default; 
   pgsMacroTxn(const pgsMacroTxn&) = delete;
   pgsMacroTxn& operator=(const pgsMacroTxn&) = delete;

   virtual bool Execute() override;
   virtual void Undo() override;

   virtual std::unique_ptr<CEAFTransaction> CreateClone() const override;
};
