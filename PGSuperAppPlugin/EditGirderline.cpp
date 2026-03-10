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
#include "EditGirderline.h"


txnEditGirderline::txnEditGirderline(const CGirderKey& girderKey,bool bApplyToAllGirderlines,const CBridgeDescription2& oldBridgeDesc,const CBridgeDescription2& newBridgeDesc) :
txnEditBridgeDescription(oldBridgeDesc,newBridgeDesc)
{
   m_GirderKey = girderKey;
   m_bApplyToAllGirderlines = bApplyToAllGirderlines;
   m_bGirderlineCopied = false;
}

txnEditGirderline::~txnEditGirderline()
{
}

std::unique_ptr<WBFL::EAF::Transaction> txnEditGirderline::CreateClone() const
{
   return std::make_unique<txnEditGirderline>(m_GirderKey,m_bApplyToAllGirderlines,m_BridgeDescription[0],m_BridgeDescription[1]);
}

std::_tstring txnEditGirderline::Name() const
{
   std::_tostringstream os;
   os << "Edit Girder " << LABEL_GIRDER(m_GirderKey.girderIndex);
   return os.str();
}

bool txnEditGirderline::Execute()
{
   if ( m_bApplyToAllGirderlines && !m_bGirderlineCopied )
   {
      CGirderGroupData* pGroup = m_BridgeDescription[1].GetGirderGroup(m_GirderKey.groupIndex);
      CSplicedGirderData* pGirder = pGroup->GetGirder(m_GirderKey.girderIndex);

      // capture the events for the segments in this girder so we can apply
      // them to the events for the segments in the other girders
      CTimelineManager* pTimelineMgr = m_BridgeDescription[1].GetTimelineManager();
      std::vector<std::pair<EventIndexType,EventIndexType>> vSegmentEvents;
      std::vector<EventIndexType> vClosureEvents;
      SegmentIndexType nSegments = pGirder->GetSegmentCount();
      for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
      {
         CPrecastSegmentData* pSegment = pGirder->GetSegment(segIdx);
         SegmentIDType segmentID = pSegment->GetID();
         EventIndexType constructEventIdx, erectEventIdx;
         pTimelineMgr->GetSegmentEvents(segmentID,&constructEventIdx,&erectEventIdx);
         vSegmentEvents.emplace_back(constructEventIdx,erectEventIdx);

         if ( segIdx < nSegments-1 )
         {
            CClosureJointData* pClosureJoint = pSegment->GetClosureJoint(pgsTypes::metEnd);
            EventIndexType castClosureEventIdx = pTimelineMgr->GetCastClosureJointEventIndex(pClosureJoint);
            vClosureEvents.push_back(castClosureEventIdx);
         }
      }

      GirderIndexType nGirders = pGroup->GetGirderCount();
      for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
      {
         if ( gdrIdx == m_GirderKey.girderIndex )
         {
            continue;
         }

         CSplicedGirderData* pOtherGirder = pGroup->GetGirder(gdrIdx);
         pOtherGirder->CopySplicedGirderData(pGirder);

         ATLASSERT(nSegments == pOtherGirder->GetSegmentCount());
         for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
         {
            CPrecastSegmentData* pSegment = pOtherGirder->GetSegment(segIdx);
            SegmentIDType segmentID = pSegment->GetID();
            pTimelineMgr->SetSegmentEvents(segmentID, vSegmentEvents[segIdx].first, vSegmentEvents[segIdx].second );

            if ( segIdx < nSegments-1 )
            {
               CClosureJointData* pClosure = pSegment->GetClosureJoint(pgsTypes::metEnd);
               pTimelineMgr->SetCastClosureJointEventByIndex(pClosure,vClosureEvents[segIdx]);
            }
         }
      }
      m_bGirderlineCopied = true;
   }

   return txnEditBridgeDescription::Execute();
}
