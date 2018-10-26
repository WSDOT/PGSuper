///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2013  Washington State Department of Transportation
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
#include "EditClosurePour.h"
#include "PGSpliceDoc.h"

#include <IFACE\Project.h>
#include <PgsExt\BridgeDescription2.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


txnEditClosurePour::txnEditClosurePour(const CSegmentKey& closureKey,const txnEditClosurePourData& newData)
{
   m_ClosureKey = closureKey;
   m_NewData = newData;
}

txnEditClosurePour::~txnEditClosurePour()
{
}

bool txnEditClosurePour::Execute()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IEvents, pEvents);
   pEvents->HoldEvents(); // don't fire any changed events until all changes are done

   m_OldData.clear();

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(m_ClosureKey.groupIndex);

   GirderIndexType nGirders    = pGroup->GetGirderCount();
   GirderIndexType firstGdrIdx = (m_ClosureKey.girderIndex == ALL_GIRDERS ? 0 : m_ClosureKey.girderIndex);
   GirderIndexType lastGdrIdx  = (m_ClosureKey.girderIndex == ALL_GIRDERS ? nGirders-1 : firstGdrIdx);

   for ( GirderIndexType gdrIdx = firstGdrIdx; gdrIdx <= lastGdrIdx; gdrIdx++ )
   {
      // collect up the old girder data (we will need it for Undo)
      txnEditClosurePourData oldData;
      oldData.m_ClosureKey = m_ClosureKey;
      oldData.m_ClosureKey.girderIndex = gdrIdx;

      oldData.m_ClosurePour = *pBridgeDesc->GetClosurePour(oldData.m_ClosureKey);
      oldData.m_ClosureEventIdx = pIBridgeDesc->GetCastClosurePourEventIndex(oldData.m_ClosureKey.groupIndex,oldData.m_ClosureKey.segmentIndex);

      m_OldData.insert(oldData);

      // Copy the new closure pour data onto this closure pour
      SetClosurePourData(oldData.m_ClosureKey,m_NewData);
   }

   pEvents->FirePendingEvents();

   return true;
}

void txnEditClosurePour::Undo()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IEvents, pEvents);
   pEvents->HoldEvents(); // don't fire any changed events until all changes are done

   std::set<txnEditClosurePourData>::iterator iter(m_OldData.begin());
   std::set<txnEditClosurePourData>::iterator end(m_OldData.end());
   for ( ; iter != end; iter++ )
   {
      txnEditClosurePourData& oldData = *iter;
      SetClosurePourData(oldData.m_ClosureKey,oldData);
   }

   pEvents->FirePendingEvents();
}

txnTransaction* txnEditClosurePour::CreateClone() const
{
   return new txnEditClosurePour(m_ClosureKey,m_NewData);
}

std::_tstring txnEditClosurePour::Name() const
{
#pragma Reminder("UPDATE: updated menu text")
   // should say
   // Edit Closure Pour for Girder <?> at Temporary Support <?>
   // Edit Closure Pour for Girder <?> at Pier <?>
   std::_tostringstream os;
   os << "Edit Closure Pour";

   return os.str();
}

bool txnEditClosurePour::IsUndoable()
{
   return true;
}

bool txnEditClosurePour::IsRepeatable()
{
   return false;
}

void txnEditClosurePour::SetClosurePourData(const CSegmentKey& closureKey,const txnEditClosurePourData& data)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker, IEvents, pEvents);
   pEvents->HoldEvents();

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   pIBridgeDesc->SetClosurePourData(closureKey,data.m_ClosurePour);
   pIBridgeDesc->SetCastClosurePourEventByIndex(closureKey.groupIndex,closureKey.segmentIndex,data.m_ClosureEventIdx);

   pEvents->FirePendingEvents();
}
