///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2013  Washington State Department of Transportation
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

#ifndef INCLUDED_EDITBOUNDARYCONDITIONS_H_
#define INCLUDED_EDITBOUNDARYCONDITIONS_H_

#include <System\Transaction.h>
#include <PGSuperTypes.h>
#include "PGSuperDoc.h"
#include <PgsExt\BridgeDescription.h>

class txnEditBoundaryConditions : public txnTransaction
{
public:
   txnEditBoundaryConditions(PierIndexType pierIdx,CPGSuperDoc* pDoc,pgsTypes::PierConnectionType oldBC,pgsTypes::PierConnectionType newBC);
   virtual std::_tstring Name() const;
   virtual txnTransaction* CreateClone() const;
   virtual bool Execute();
   virtual void Undo();
   virtual bool IsUndoable();
   virtual bool IsRepeatable();

private:
   bool DoExecute(int i);
   PierIndexType m_PierIdx;
   CPGSuperDoc* m_pPGSuperDoc;
   pgsTypes::PierConnectionType m_ConnectionType[2];
};

#endif // INCLUDED_EDITBOUNDARYCONDITIONS_H_