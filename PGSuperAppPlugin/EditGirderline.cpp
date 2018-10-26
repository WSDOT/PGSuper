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
#include "EditGirderline.h"
#include <PgsExt\BridgeDescription2.h>
#include <PgsExt\PTData.h>
#include <PgsExt\PrecastSegmentData.h>
#include <PgsExt\ClosureJointData.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

txnEditGirderline::txnEditGirderline(const CGirderKey& girderKey,const txnEditGirderlineData& newData) :
m_GirderKey(girderKey),m_NewData(newData)
{
}

txnEditGirderline::~txnEditGirderline()
{
}

bool txnEditGirderline::Execute()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IEvents, pEvents);
   pEvents->HoldEvents(); // don't fire any changed events until all changes are done

   m_OldData.clear();

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(m_GirderKey.groupIndex);
   const CTimelineManager* pTimelineMgr = pBridgeDesc->GetTimelineManager();

   GirderIndexType nGirders    = pGroup->GetGirderCount();
   GirderIndexType firstGdrIdx = (m_GirderKey.girderIndex == ALL_GIRDERS ? 0 : m_GirderKey.girderIndex);
   GirderIndexType lastGdrIdx  = (m_GirderKey.girderIndex == ALL_GIRDERS ? nGirders-1 : firstGdrIdx);

   GroupIDType grpID = pGroup->GetID();

   for ( GirderIndexType gdrIdx = firstGdrIdx; gdrIdx <= lastGdrIdx; gdrIdx++ )
   {
      // collect up the old girder data (we will need it for Undo)
      txnEditGirderlineData oldData;
      oldData.m_GirderKey = m_GirderKey;
      oldData.m_GirderKey.girderIndex = gdrIdx;

      const CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);
      CGirderKey girderKey(pGirder->GetGirderKey());

      DuctIndexType nDucts = pGirder->GetPostTensioning()->GetDuctCount();
      for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
      {
         IndexType eventIdx = pTimelineMgr->GetStressTendonEventIndex(girderKey,ductIdx);
         oldData.m_StressingEvent.push_back(eventIdx);
      }

      EventIndexType nClosureJoints = pGirder->GetClosureJointCount();
      for ( EventIndexType idx = 0; idx < nClosureJoints; idx++ )
      {
         const CPrecastSegmentData* pSegment = pGirder->GetSegment(idx);
         SegmentIDType segID = pSegment->GetID();
         EventIndexType eventIdx = pTimelineMgr->GetCastClosureJointEventIndex(segID);
         oldData.m_ClosureEvent.push_back(eventIdx);
      }

      oldData.m_Girder = *pGroup->GetGirder(gdrIdx);
      m_OldData.insert(oldData);

      // Copy the new girder data onto this girder
      SetGirderData(oldData.m_GirderKey,m_NewData);
   }

   pEvents->FirePendingEvents();

   return true;
}

void txnEditGirderline::Undo()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IEvents, pEvents);
   pEvents->HoldEvents(); // don't fire any changed events until all changes are done

   std::set<txnEditGirderlineData>::iterator iter(m_OldData.begin());
   std::set<txnEditGirderlineData>::iterator end(m_OldData.end());
   for ( ; iter != end; iter++ )
   {
      txnEditGirderlineData& oldData = *iter;
      SetGirderData(oldData.m_GirderKey,oldData);
   }

   pEvents->FirePendingEvents();
}

txnTransaction* txnEditGirderline::CreateClone() const
{
   return new txnEditGirderline(m_GirderKey,m_NewData);
}

std::_tstring txnEditGirderline::Name() const
{
   std::_tostringstream os;
   os << "Edit Group " << LABEL_GROUP(m_NewData.m_GirderKey.groupIndex) << " Girder " << LABEL_GIRDER(m_NewData.m_GirderKey.girderIndex);
   return os.str();
}

bool txnEditGirderline::IsUndoable()
{
   return true;
}

bool txnEditGirderline::IsRepeatable()
{
   return false;
}

void txnEditGirderline::SetGirderData(const CGirderKey& girderKey,const txnEditGirderlineData& gdrData)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   pIBridgeDesc->SetGirder( girderKey, gdrData.m_Girder);

   DuctIndexType nDucts = gdrData.m_Girder.GetPostTensioning()->GetDuctCount();
   for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
   {
      pIBridgeDesc->SetStressTendonEventByIndex(girderKey,ductIdx,gdrData.m_StressingEvent[ductIdx]);
   }

   std::vector<EventIndexType>::const_iterator iter(gdrData.m_ClosureEvent.begin());
   std::vector<EventIndexType>::const_iterator end(gdrData.m_ClosureEvent.end());
   for ( ; iter != end; iter++ )
   {
      EventIndexType eventIdx = *iter;
      SegmentIndexType segIdx = iter - gdrData.m_ClosureEvent.begin();
      pIBridgeDesc->SetCastClosureJointEventByIndex(girderKey.groupIndex,segIdx,eventIdx);
   }
}
