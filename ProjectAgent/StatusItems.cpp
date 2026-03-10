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

#include "StatusItems.h"


pgsRebarStrengthStatusItem::pgsRebarStrengthStatusItem(const CSegmentKey& segmentKey,Type type,StatusGroupIDType statusGroupID,StatusCallbackIDType callbackID,LPCTSTR strDescription) :
pgsSegmentRelatedStatusItem(statusGroupID,callbackID,strDescription,segmentKey),m_SegmentKey(segmentKey),m_Type(type)
{
}

bool pgsRebarStrengthStatusItem::IsEqual(std::shared_ptr<const WBFL::EAF::StatusItem> pOther) const
{
   auto other = std::dynamic_pointer_cast<const pgsRebarStrengthStatusItem>(pOther);
   if ( !other )
      return false;

   return (other->m_Type == m_Type && other->m_SegmentKey == m_SegmentKey);
}

pgsRebarStrengthStatusCallback::pgsRebarStrengthStatusCallback()
{
}

WBFL::EAF::StatusSeverityType pgsRebarStrengthStatusCallback::GetSeverity() const
{
   return WBFL::EAF::StatusSeverityType::Warning;
}

void pgsRebarStrengthStatusCallback::Execute(std::shared_ptr<WBFL::EAF::StatusItem> pStatusItem)
{
   AfxMessageBox(pStatusItem->GetDescription());
}
