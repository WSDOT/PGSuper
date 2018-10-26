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
#include "PGSuperAppPlugin\EditTimeline.h"
#include "PGSuperDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

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

txnTransaction* txnEditTimeline::CreateClone() const
{
   return new txnEditTimeline(m_TimelineManager[0],m_TimelineManager[1]);
}

std::_tstring txnEditTimeline::Name() const
{
   return _T("Edit Timeline");
}

bool txnEditTimeline::IsUndoable()
{
   return true;
}

bool txnEditTimeline::IsRepeatable()
{
   return false;
}

void txnEditTimeline::DoExecute(int i)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IEvents, pEvents);
   pEvents->HoldEvents(); // don't fire any changed events until all changes are done

   GET_IFACE2(pBroker,IBridgeDescription,pBridgeDesc);

   pBridgeDesc->SetTimelineManager(m_TimelineManager[i]);

   pEvents->FirePendingEvents();
}
