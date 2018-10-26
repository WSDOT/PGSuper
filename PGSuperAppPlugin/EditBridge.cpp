///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2014  Washington State Department of Transportation
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
#include "EditBridge.h"

#include <IFace\Project.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


txnEditBridgeDescription::txnEditBridgeDescription(const CBridgeDescription2& oldBridgeDesc,const CBridgeDescription2& newBridgeDesc)
{
   m_BridgeDescription[0] = oldBridgeDesc;
   m_BridgeDescription[1] = newBridgeDesc;
}

txnEditBridgeDescription::~txnEditBridgeDescription()
{
}

bool txnEditBridgeDescription::Execute()
{
   DoExecute(1);
   return true;
}

void txnEditBridgeDescription::Undo()
{
   DoExecute(0);
}

txnTransaction* txnEditBridgeDescription::CreateClone() const
{
   return new txnEditBridgeDescription(m_BridgeDescription[0],m_BridgeDescription[1]);
}

std::_tstring txnEditBridgeDescription::Name() const
{
   return _T("Edit Bridge");
}

bool txnEditBridgeDescription::IsUndoable()
{
   return true;
}

bool txnEditBridgeDescription::IsRepeatable()
{
   return false;
}

void txnEditBridgeDescription::DoExecute(int i)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IEvents, pEvents);
   pEvents->HoldEvents(); // don't fire any changed events until all changes are done

   GET_IFACE2(pBroker,IBridgeDescription,pBridgeDesc);

   pBridgeDesc->SetBridgeDescription(m_BridgeDescription[i]);

   pEvents->FirePendingEvents();
}
