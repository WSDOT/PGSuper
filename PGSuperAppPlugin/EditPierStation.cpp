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
#include "PGSuperDoc.h"
#include "EditPierStation.h"

#include <IFace/Tools.h>
#include <IFace\Project.h>


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

std::unique_ptr<WBFL::EAF::Transaction> txnEditPierStation::CreateClone() const
{
   return std::make_unique<txnEditPierStation>(m_PierIdx,m_Station[0],m_Station[1],m_MoveOption);
}

std::_tstring txnEditPierStation::Name() const
{
   std::_tostringstream os;
   os << "Edit Station of Pier " << LABEL_PIER(m_PierIdx);
   return os.str();
}

bool txnEditPierStation::IsUndoable() const
{
   return true;
}

bool txnEditPierStation::IsRepeatable() const
{
   return false;
}

void txnEditPierStation::DoExecute(int i)
{
   
   auto pBroker = EAFGetBroker();

   GET_IFACE2(pBroker,IEvents,pEvents);
   // Exception-safe holder to keep from fireing events until we are done
   CIEventsHolder event_holder(pEvents);

   GET_IFACE2(pBroker,IBridgeDescription,pBridgeDesc);
   pBridgeDesc->MovePier(m_PierIdx,m_Station[i],m_MoveOption);
}
