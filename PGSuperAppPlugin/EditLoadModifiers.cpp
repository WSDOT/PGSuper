///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2021  Washington State Department of Transportation
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
#include "EditLoadModifiers.h"
#include <IFace\Project.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
txnEditLoadModifiers::txnEditLoadModifiers(const txnEditLoadModifiers::LoadModifiers& oldLoadModifiers,const txnEditLoadModifiers::LoadModifiers& newLoadFactors)
{
   m_LoadModifiers[0] = oldLoadModifiers;
   m_LoadModifiers[1] = newLoadFactors;
}

txnEditLoadModifiers::~txnEditLoadModifiers()
{
}

bool txnEditLoadModifiers::Execute()
{
   DoExecute(1);
   return true;
}

void txnEditLoadModifiers::Undo()
{
   DoExecute(0);
}

txnTransaction* txnEditLoadModifiers::CreateClone() const
{
   return new txnEditLoadModifiers(m_LoadModifiers[0],m_LoadModifiers[1]);
}

std::_tstring txnEditLoadModifiers::Name() const
{
   std::_tostringstream os;
   os << "Edit Load Modifiers" << std::endl;
   return os.str();
}

bool txnEditLoadModifiers::IsUndoable()
{
   return true;
}

bool txnEditLoadModifiers::IsRepeatable()
{
   return false;
}

void txnEditLoadModifiers::DoExecute(int i)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IEvents, pEvents);
   // Exception-safe holder to keep from fireing events until we are done
   CIEventsHolder event_holder(pEvents);

   GET_IFACE2(pBroker,ILoadModifiers,pLoadModifiers);

   pLoadModifiers->SetDuctilityFactor(  m_LoadModifiers[i].DuctilityLevel,  m_LoadModifiers[i].DuctilityFactor );
   pLoadModifiers->SetRedundancyFactor( m_LoadModifiers[i].RedundancyLevel, m_LoadModifiers[i].RedundancyFactor );
   pLoadModifiers->SetImportanceFactor( m_LoadModifiers[i].ImportanceLevel, m_LoadModifiers[i].ImportanceFactor );
}
