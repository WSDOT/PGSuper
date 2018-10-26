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
#include "CopyGirder.h"
#include "PGSuperDoc.h"

#include <EAF\EAFAutoProgress.h>
#include <IFace\Bridge.h>
#include <IFace\GirderHandling.h>

#include <PgsExt\BridgeDescription2.h>
#include <PgsExt\TimelineManager.h>
#include <PgsExt\ClosureJointData.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

txnCopyGirder::txnCopyGirder(const CGirderKey& fromGirder,const std::vector<CGirderKey>& toGirders,BOOL bGirder,BOOL bTransverse,BOOL bLongitudinalRebar,BOOL bPrestress,BOOL bHandling, BOOL bMaterial, BOOL bSlabOffset)
{
   m_From               = fromGirder;
   m_To                 = toGirders;
   m_bGirder            = bGirder;
   m_bTransverse        = bTransverse;
   m_bLongitudinalRebar = bLongitudinalRebar;
   m_bPrestress         = bPrestress;
   m_bHandling          = bHandling;
   m_bMaterial          = bMaterial;
   m_bSlabOffset        = bSlabOffset;

   EAFGetBroker(&m_pBroker);
}

txnCopyGirder::~txnCopyGirder()
{
}

bool txnCopyGirder::Execute()
{
   GetGirderData(m_From,&m_SourceGirderData);
   DoCopyGirderData(true);

   return true;
}

void txnCopyGirder::Undo()
{
   DoCopyGirderData(false);
}

txnTransaction* txnCopyGirder::CreateClone() const
{
   return new txnCopyGirder(m_From,m_To,m_bGirder,m_bTransverse,m_bLongitudinalRebar,m_bPrestress,m_bHandling,m_bMaterial,m_bSlabOffset);
}

std::_tstring txnCopyGirder::Name() const
{
   return _T("Copy Girder Properties");
}

bool txnCopyGirder::IsUndoable()
{
   return true;
}

bool txnCopyGirder::IsRepeatable()
{
   return false;
}

void txnCopyGirder::DoCopyGirderData(bool bExecute)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);

   GET_IFACE(IEvents,pEvents);
   pEvents->HoldEvents();

   GET_IFACE(IProgress,pProgress);
   CEAFAutoProgress ap(pProgress);

   if ( bExecute )
      m_DestinationGirderData.clear();

   std::vector<CGirderKey>::iterator begin(m_To.begin());
   std::vector<CGirderKey>::iterator hashIter(m_To.begin());
   std::vector<CGirderKey>::iterator hashIterEnd(m_To.end());
   for ( ; hashIter != hashIterEnd; hashIter++ )
   {
      const CGirderKey& toGirderKey = *hashIter;
      GroupIndexType to_group = toGirderKey.groupIndex;
      GirderIndexType to_girder = toGirderKey.girderIndex;

      CString strMsg;
      strMsg.Format(_T("Copying to Girder %s"),LABEL_GIRDER(to_girder));
      pProgress->UpdateMessage(strMsg);

      // keep a copy of what the spliced girder looked like before modifications
      txnCopyGirderData* pSourceData = NULL;
      if ( bExecute )
      {
         txnCopyGirderData copyData;
         GetGirderData(toGirderKey,&copyData);
         m_DestinationGirderData.push_back(copyData);
         pSourceData = &m_SourceGirderData;
      }
      else
      {
         pSourceData = &m_DestinationGirderData[hashIter-begin];
      }

      // start with a copy of the source data and then tweak it
      txnCopyGirderData targetData = *pSourceData;

      SegmentIndexType nSegments = pSourceData->m_Girder.GetSegmentCount();
      for (SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
      {
         const CPrecastSegmentData* pSegment = pSourceData->m_Girder.GetSegment(segIdx);
         CPrecastSegmentData* pTargetSegment = targetData.m_Girder.GetSegment(segIdx);

         const CClosureJointData* pClosure = NULL;
         CClosureJointData* pTargetClosure = NULL;
         if ( segIdx < nSegments-1 )
         {
            pClosure = pSourceData->m_Girder.GetClosureJoint(segIdx);
            pTargetClosure = targetData.m_Girder.GetClosureJoint(segIdx);
         }
         

         if (m_bGirder)
         {
            // this parameters meets copy segment variations for spliced girders
            pTargetSegment->CopyVariationFrom(*pSegment);
         }

         if (m_bTransverse)
         {
            pTargetSegment->CopyTransverseReinforcementFrom(*pSegment);
            if ( pTargetClosure )
               pTargetClosure->CopyTransverseReinforcementFrom(*pClosure);
         }

         if (m_bLongitudinalRebar)
         {
            pTargetSegment->CopyLongitudinalReinforcementFrom(*pSegment);
            if ( pTargetClosure )
               pTargetClosure->CopyLongitudinalReinforcementFrom(*pClosure);
         }

         if (m_bPrestress)
         {
            // copy PS
            pTargetSegment->CopyPrestressingFrom(*pSegment);
         }

         if (m_bMaterial)
         {
            pTargetSegment->CopyMaterialFrom(*pSegment);
            if ( pTargetClosure )
               pTargetClosure->CopyMaterialFrom(*pClosure);
         }

         if (m_bHandling)
         {
            // handling of the individual segment
            pTargetSegment->CopyHandlingFrom(*pSegment);
         }
      } // end of segment loop

      if (m_bPrestress)
      {
         // copy PT for entire spliced girder
         targetData.m_Girder.SetPostTensioning(*pSourceData->m_Girder.GetPostTensioning());
      }

      if (m_bHandling)
      {
         // handling of fully assembled spliced girder
         // this isn't supported yet (segment handling is taken care of above)
#pragma Reminder("UPDATE: Implement this once handling of a fully assembled spliced girder is supported")
      }

      if (m_bSlabOffset)
      {
         // ????
#pragma Reminder("UPDATE: Implement this once spliced girder \"A\" Dimensions are defined")
#pragma Reminder("UPDATE: slab offset")
      }

      // Set the girder data
      pIBridgeDesc->SetGirder(toGirderKey,targetData.m_Girder);

      // Set the staging information
      std::map<CSegmentKey,EventIndexType>::iterator iter(targetData.m_ErectSegmentEvent.begin());
      std::map<CSegmentKey,EventIndexType>::iterator iterEnd(targetData.m_ErectSegmentEvent.end());
      for ( ; iter != iterEnd; iter++ )
      {
         const CSegmentKey& segmentKey = iter->first;
         EventIndexType eventIdx = iter->second;
         pIBridgeDesc->SetSegmentErectionEventByIndex(segmentKey,eventIdx);
      }

      std::map<DuctIndexType,EventIndexType>::iterator ductIter(targetData.m_StressTendonEvent.begin());
      std::map<DuctIndexType,EventIndexType>::iterator ductIterEnd(targetData.m_StressTendonEvent.end());
      for ( ; ductIter != ductIterEnd; ductIter++ )
      {
         pIBridgeDesc->SetStressTendonEventByIndex(toGirderKey,ductIter->first,ductIter->second);
      }
   } // end of vector loop

   pEvents->FirePendingEvents();
}

void txnCopyGirder::GetGirderData(const CGirderKey& girderKey,txnCopyGirderData* ptxnGirderData)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);

   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   ptxnGirderData->m_Girder = *pBridgeDesc->GetGirderGroup(girderKey.groupIndex)->GetGirder(girderKey.girderIndex);

   GirderIDType gdrID = pIBridgeDesc->GetGirderID(girderKey);

   // Get segment construction and erection stages as well as closure joint casting stage
   const CTimelineManager* pTimelineMgr = pBridgeDesc->GetTimelineManager();
   SegmentIndexType nSegments = ptxnGirderData->m_Girder.GetSegmentCount();
   for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      CSegmentKey segmentKey(girderKey,segIdx);

      SegmentIDType segID = pIBridgeDesc->GetSegmentID(segmentKey);

      EventIndexType eventIdx = pTimelineMgr->GetSegmentErectionEventIndex(segID);
      ptxnGirderData->m_ErectSegmentEvent.insert(std::make_pair(segmentKey,eventIdx));
   }

   // Get tendon stressing stages
   CPTData* pPTData = ptxnGirderData->m_Girder.GetPostTensioning();
   DuctIndexType nDucts = pPTData->GetDuctCount();
   for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
   {
      EventIndexType eventIdx = pTimelineMgr->GetStressTendonEventIndex(girderKey,ductIdx);
      ptxnGirderData->m_StressTendonEvent.insert( std::make_pair(ductIdx,eventIdx) );
   }
}
