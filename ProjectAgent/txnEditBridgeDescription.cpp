///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2024  Washington State Department of Transportation
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
#include "txnEditBridgeDescription.h"

#include <IFace\Project.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


txnEditBridgeDescription::txnEditBridgeDescription(IBroker* pBroker,const CBridgeDescription2& oldBridgeDesc,const CBridgeDescription2& newBridgeDesc) :
m_pBroker(pBroker)
{
   m_BridgeDesc[0] = oldBridgeDesc;
   m_BridgeDesc[1] = newBridgeDesc;
}

txnEditBridgeDescription::~txnEditBridgeDescription(void)
{
}

bool txnEditBridgeDescription::Execute()
{
   Execute(1);
   return true;
}

void txnEditBridgeDescription::Undo()
{
   Execute(0);
}

void txnEditBridgeDescription::Execute(int i)
{
   GET_IFACE(IEvents, pEvents);
   // Exception-safe holder to keep from fireing events until we are done
   CIEventsHolder event_holder(pEvents);

   GET_IFACE(IBridgeDescription,pBridgeDesc);
   pBridgeDesc->SetBridgeDescription( m_BridgeDesc[i] );
}

std::unique_ptr<CEAFTransaction> txnEditBridgeDescription::CreateClone() const
{
   return std::make_unique<txnEditBridgeDescription>(m_pBroker,m_BridgeDesc[0],m_BridgeDesc[1]);
}

std::_tstring txnEditBridgeDescription::Name() const
{
   return _T("Modify bridge for Post-Tensioning");
}

bool txnEditBridgeDescription::IsUndoable() const
{
   return true;
}

bool txnEditBridgeDescription::IsRepeatable() const
{
   return false;
}
