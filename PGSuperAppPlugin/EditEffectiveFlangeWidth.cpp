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
#include "EditEffectiveFlangeWidth.h"
#include <IFace\Project.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

txnEditEffectiveFlangeWidth::txnEditEffectiveFlangeWidth(bool bOldSetting,bool bNewSetting)
{
   m_bIgnore[0] = bOldSetting;
   m_bIgnore[1] = bNewSetting;
}

txnEditEffectiveFlangeWidth::~txnEditEffectiveFlangeWidth()
{
}

bool txnEditEffectiveFlangeWidth::Execute()
{
   Execute(1);
   return true;
}

void txnEditEffectiveFlangeWidth::Undo()
{
   Execute(0);
}

void txnEditEffectiveFlangeWidth::Execute(int i)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IEffectiveFlangeWidth, pEFW );
   GET_IFACE2(pBroker,IEvents, pEvents);

   // Exception-safe holder to keep from fireing events until we are done
   CIEventsHolder event_holder(pEvents);

   pEFW->IgnoreEffectiveFlangeWidthLimits(m_bIgnore[i]);
}

std::unique_ptr<CEAFTransaction> txnEditEffectiveFlangeWidth::CreateClone() const
{
   return std::make_unique<txnEditEffectiveFlangeWidth>(m_bIgnore[0],m_bIgnore[1]);
}

std::_tstring txnEditEffectiveFlangeWidth::Name() const
{
   return _T("Edit Effective Flange Width");
}

bool txnEditEffectiveFlangeWidth::IsUndoable() const
{
   return true;
}

bool txnEditEffectiveFlangeWidth::IsRepeatable() const
{
   return false;
}
