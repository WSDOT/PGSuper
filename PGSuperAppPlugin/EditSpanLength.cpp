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
#include "EditSpanLength.h"
#include "PGSuperDoc.h"

#include <IFace/Tools.h>
#include <IFace\Project.h>


txnEditSpanLength::txnEditSpanLength(SpanIndexType spanIdx,Float64 oldSpanLength,Float64 newSpanLength)
{
   m_SpanIdx = spanIdx;
   m_SpanLength[0] = oldSpanLength;
   m_SpanLength[1] = newSpanLength;
}

txnEditSpanLength::~txnEditSpanLength()
{
}

bool txnEditSpanLength::Execute()
{
   DoExecute(1);
   return true;
}

void txnEditSpanLength::Undo()
{
   DoExecute(0);
}

std::unique_ptr<WBFL::EAF::Transaction> txnEditSpanLength::CreateClone() const
{
   return std::make_unique<txnEditSpanLength>(m_SpanIdx,m_SpanLength[0],m_SpanLength[1]);
}

std::_tstring txnEditSpanLength::Name() const
{
   std::_tostringstream os;
   os << "Edit Length of Span " << LABEL_SPAN(m_SpanIdx);
   return os.str();
}

bool txnEditSpanLength::IsUndoable() const
{
   return true;
}

bool txnEditSpanLength::IsRepeatable() const
{
   return false;
}

void txnEditSpanLength::DoExecute(int i)
{
   
   auto pBroker = EAFGetBroker();

   GET_IFACE2(pBroker,IEvents, pEvents);
   // Exception-safe holder to keep from fireing events until we are done
   CIEventsHolder event_holder(pEvents);

   GET_IFACE2(pBroker,IBridgeDescription,pBridgeDesc);
   pBridgeDesc->SetSpanLength(m_SpanIdx,m_SpanLength[i]);
}
