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

#include "PGSuperAppPlugin\stdafx.h"
#include "EditBoundaryConditions.h"
#include "PGSuperAppPlugin\PGSuperApp.h"
#include <IFace\Project.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

txnEditBoundaryConditions::txnEditBoundaryConditions(PierIndexType pierIdx,CPGSuperDoc* pDoc,pgsTypes::PierConnectionType oldBC,pgsTypes::PierConnectionType newBC)
{
   m_PierIdx = pierIdx;
   m_pPGSuperDoc = pDoc;
   m_ConnectionType[0] = oldBC;
   m_ConnectionType[1] = newBC;
}

std::_tstring txnEditBoundaryConditions::Name() const
{
   return _T("Change Boundary Conditions");
}

txnTransaction* txnEditBoundaryConditions::CreateClone() const
{
   return new txnEditBoundaryConditions(m_PierIdx,m_pPGSuperDoc,m_ConnectionType[0],m_ConnectionType[1]);
}

bool txnEditBoundaryConditions::IsUndoable()
{
   return true;
}

bool txnEditBoundaryConditions::IsRepeatable()
{
   return false;
}

bool txnEditBoundaryConditions::Execute()
{
   return DoExecute(1);
}

void txnEditBoundaryConditions::Undo()
{
   DoExecute(0);
}

bool txnEditBoundaryConditions::DoExecute(int i)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IEvents,pEvents);
   pEvents->HoldEvents();

   GET_IFACE2(pBroker,IBridgeDescription,pBridgeDesc);
   pBridgeDesc->SetBoundaryCondition( m_PierIdx, m_ConnectionType[i] );

   pEvents->FirePendingEvents();

   return true;
}
