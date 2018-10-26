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
#include "PGSuperDoc.h"
#include "EditPierStation.h"
#include <IFace\Project.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

txnEditPierStation::txnEditPierStation(PierIndexType pierIdx,Float64 oldStation,Float64 newStation,pgsTypes::MovePierOption moveOption)
{
   m_PierIdx = pierIdx;
   m_Station[0] = oldStation;
   m_Station[1] = newStation;
   m_MoveOption = moveOption;
}

txnEditPierStation::~txnEditPierStation()
{
}

bool txnEditPierStation::Execute()
{
   DoExecute(1);
   return true;
}

void txnEditPierStation::Undo()
{
   DoExecute(0);
}

txnTransaction* txnEditPierStation::CreateClone() const
{
   return new txnEditPierStation(m_PierIdx,m_Station[0],m_Station[1],m_MoveOption);
}

std::_tstring txnEditPierStation::Name() const
{
   std::_tostringstream os;
   os << "Edit Station of Pier " << (m_PierIdx+1);
   return os.str();
}

bool txnEditPierStation::IsUndoable()
{
   return true;
}

bool txnEditPierStation::IsRepeatable()
{
   return false;
}

void txnEditPierStation::DoExecute(int i)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IEvents,pEvents);
   pEvents->HoldEvents();

   GET_IFACE2(pBroker,IBridgeDescription,pBridgeDesc);
   pBridgeDesc->MovePier(m_PierIdx,m_Station[i],m_MoveOption);

   pEvents->FirePendingEvents();
}
