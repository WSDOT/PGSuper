///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2015  Washington State Department of Transportation
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

pgsRebarStrengthStatusItem::pgsRebarStrengthStatusItem(SpanIndexType span,GirderIndexType gdr,Type type,StatusGroupIDType statusGroupID,StatusCallbackIDType callbackID,LPCTSTR strDescription) :
pgsSpanGirderRelatedStatusItem(statusGroupID,callbackID,strDescription,span,gdr),m_Span(span),m_Girder(gdr),m_Type(type)
{
}

bool pgsRebarStrengthStatusItem::IsEqual(CEAFStatusItem* pOther)
{
   pgsRebarStrengthStatusItem* other = dynamic_cast<pgsRebarStrengthStatusItem*>(pOther);
   if ( !other )
      return false;

   return (other->m_Type == m_Type && other->m_Span == m_Span && other->m_Girder == m_Girder);
}

pgsRebarStrengthStatusCallback::pgsRebarStrengthStatusCallback()
{
}

eafTypes::StatusSeverityType pgsRebarStrengthStatusCallback::GetSeverity()
{
   return eafTypes::statusWarning;
}

void pgsRebarStrengthStatusCallback::Execute(CEAFStatusItem* pStatusItem)
{
   AfxMessageBox(pStatusItem->GetDescription().c_str());
}
