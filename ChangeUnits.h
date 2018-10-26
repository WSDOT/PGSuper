///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright (C) 2008  Washington State Department of Transportation
//                     Bridge and Structures Office
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

#ifndef INCLUDED_CHANGEUNITS_H_
#define INCLUDED_CHANGEUNITS_H_

#include <System\Transaction.h>
#include <PGSuperTypes.h>
#include "PGSuperDoc.h"

class txnChangeUnits : public txnTransaction
{
public:
   txnChangeUnits(CPGSuperDoc* pDoc,pgsTypes::UnitMode oldUnits,pgsTypes::UnitMode newUnits);
   virtual std::string Name() const;
   virtual txnTransaction* CreateClone() const;
   virtual bool Execute();
   virtual void Undo();
   virtual bool IsUndoable();
   virtual bool IsRepeatable();

private:
   bool DoExecute(int i);
   CPGSuperDoc* m_pPGSuperDoc;
   pgsTypes::UnitMode m_UnitMode[2];
};

#endif // INCLUDED_CHANGEUNITS_H_