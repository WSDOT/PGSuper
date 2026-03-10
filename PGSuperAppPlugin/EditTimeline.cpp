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
#include "EditTimeline.h"
#include "PGSuperDoc.h"
#include <IFace/Tools.h>


txnEditTimeline::txnEditTimeline(const CTimelineManager& oldTimelineManager,const CTimelineManager& newTimelineManager)
{
   m_TimelineManager[0] = oldTimelineManager;
   m_TimelineManager[1] = newTimelineManager;
}

txnEditTimeline::~txnEditTimeline()
{
}

bool txnEditTimeline::Execute()
{
   DoExecute(1);
   return true;
}

void txnEditTimeline::Undo()
{
   DoExecute(0);
}

std::unique_ptr<WBFL::EAF::Transaction> txnEditTimeline::CreateClone() const
{
   return std::make_unique<txnEditTimeline>(m_TimelineManager[0],m_TimelineManager[1]);
}

std::_tstring txnEditTimeline::Name() const
{
   return _T("Edit Timeline");
}

bool txnEditTimeline::IsUndoable() const
{
   return true;
}

bool txnEditTimeline::IsRepeatable() const
{
   return false;
}

void txnEditTimeline::DoExecute(int i)
{
   
   auto pBroker = EAFGetBroker();

   GET_IFACE2(pBroker,IEvents, pEvents);
   // Exception-safe holder to keep from fireing events until we are done
   CIEventsHolder event_holder(pEvents);

   GET_IFACE2(pBroker,IBridgeDescription,pBridgeDesc);

   pBridgeDesc->SetTimelineManager(m_TimelineManager[i]);
}
