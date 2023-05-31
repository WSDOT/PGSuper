///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2023  Washington State Department of Transportation
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
#include "DesignGirder.h"
#include "PGSuperDoc.h"
#include <PgsExt\DesignConfigUtil.h>
#include <PgsExt\BridgeDescription2.h>
#include <IFace\Project.h>
#include <IFace\Bridge.h>

#include <LRFD\ConcreteUtil.h>

#include <algorithm>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

txnDesignGirder::txnDesignGirder( std::vector<const pgsGirderDesignArtifact*>& artifacts, SlabOffsetDesignSelectionType slabOffsetDType, SpanIndexType fromSpan, GirderIndexType fromGirder)
{
   std::vector<const pgsGirderDesignArtifact*>::const_iterator iter(artifacts.begin());
   std::vector<const pgsGirderDesignArtifact*>::const_iterator iterEnd(artifacts.end());
   for ( ; iter != iterEnd; iter++ )
   {
      const pgsGirderDesignArtifact& gdrDesignArtifact(*(*iter));
      DesignData data(gdrDesignArtifact);

      m_DesignDataColl.push_back(data);
   }

   m_NewSlabOffsetType = slabOffsetDType;
   m_FromSpanIdx = fromSpan;
   m_FromGirderIdx = fromGirder;

   // AssumedExcessCamber design uses same input as slab offset
   if (m_NewSlabOffsetType == sodtBridge)
   {
      m_NewAssumedExcessCamberType = pgsTypes::aecBridge;
   }
   else if (m_NewSlabOffsetType == sodtPier)
   {
      m_NewAssumedExcessCamberType = pgsTypes::aecSpan;
   }
   else if (m_NewSlabOffsetType == sodtGirder || m_NewSlabOffsetType == sodtAllSelectedGirders)
   {
      m_NewAssumedExcessCamberType = pgsTypes::aecGirder;
   }

   m_DidSlabOffsetDesign = false; // until we determine otherwise
   m_DidAssumedExcessCamberDesign = false;

   m_bInit = false;
}

txnDesignGirder::~txnDesignGirder()
{
}

bool txnDesignGirder::Execute()
{
   if ( !m_bInit )
      Init();

   DoExecute(1);
   return true;
}

void txnDesignGirder::Undo()
{
   DoExecute(0);
}

std::unique_ptr<CEAFTransaction> txnDesignGirder::CreateClone() const
{
   std::vector<const pgsGirderDesignArtifact*> artifacts;
   for (DesignDataConstIter iter = m_DesignDataColl.begin(); iter!=m_DesignDataColl.end(); iter++)
   {
      artifacts.push_back(&(iter->m_DesignArtifact));
   }

   return std::make_unique<txnDesignGirder>(artifacts, m_NewSlabOffsetType, m_FromSpanIdx, m_FromGirderIdx);
}

std::_tstring txnDesignGirder::Name() const
{
   std::_tostringstream os;
   if ( m_DesignDataColl.size() == 1 )
   {
      const CGirderKey& girderKey = m_DesignDataColl.front().m_DesignArtifact.GetGirderKey();
      SpanIndexType spanIdx = girderKey.groupIndex;
      GirderIndexType gdrIdx = girderKey.girderIndex;
      os << _T("Design for Span ") << LABEL_SPAN(spanIdx) << _T(", Girder ") << LABEL_GIRDER(gdrIdx);
   }
   else
   {
	   os << _T("Design for (Span, Girder) =");
	   for (DesignDataConstIter iter = m_DesignDataColl.begin(); iter!=m_DesignDataColl.end(); iter++)
	   {
	      const CGirderKey& girderKey = iter->m_DesignArtifact.GetGirderKey();
	      SpanIndexType spanIdx = girderKey.groupIndex;
	      GirderIndexType gdrIdx = girderKey.girderIndex;
	      os << _T(" (") << LABEL_SPAN(spanIdx) << _T(", ") << LABEL_GIRDER(gdrIdx)<< _T(")");
	   }
   }

   return os.str();
}

bool txnDesignGirder::IsUndoable() const
{
   return true;
}

bool txnDesignGirder::IsRepeatable() const
{
   return false;
}

void txnDesignGirder::Init()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);

   for (auto& rdata : m_DesignDataColl)
   {
#pragma Reminder("UPDATE: assuming precast girder bridge")
      SegmentIndexType segIdx = 0;
      const pgsSegmentDesignArtifact* pSegmentDesignArtifact = rdata.m_DesignArtifact.GetSegmentDesignArtifact(segIdx);
      const CSegmentKey& segmentKey = pSegmentDesignArtifact->GetSegmentKey();

      // old (existing) girder data
      rdata.m_SegmentData[0] = *(pIBridgeDesc->GetPrecastSegmentData(segmentKey));

      // new girder data. Artifact does the work
      rdata.m_SegmentData[1] = pSegmentDesignArtifact->GetSegmentData();

      // Slab offset design data
      if ( m_NewSlabOffsetType!=sodtDoNotDesign && pSegmentDesignArtifact->GetDesignOptions().doDesignSlabOffset != sodPreserveHaunch )
      {
         if (m_NewSlabOffsetType==sodtAllSelectedGirders ||
             (rdata.m_DesignArtifact.GetGirderKey().groupIndex == m_FromSpanIdx && rdata.m_DesignArtifact.GetGirderKey().girderIndex == m_FromGirderIdx))
         {
            m_DidSlabOffsetDesign = true;
            m_DesignSlabOffset[pgsTypes::metStart] = pSegmentDesignArtifact->GetSlabOffset(pgsTypes::metStart);
            m_DesignSlabOffset[pgsTypes::metEnd]   = pSegmentDesignArtifact->GetSlabOffset(pgsTypes::metEnd);

            GET_IFACE2(pBroker,ISpecification,pSpec);
            if ( pSegmentDesignArtifact->GetDesignOptions().doDesignSlabOffset == sodDesignHaunch &&
                 pSpec->IsAssumedExcessCamberForLoad())
            {
               // AssumedExcessCamber was done too - store it
               m_DidAssumedExcessCamberDesign = true;
               m_DesignAssumedExcessCamber = pSegmentDesignArtifact->GetAssumedExcessCamber();
            }
         }
      }
   }

   // Store original slab offset data. Format depends on type
   if (m_DidSlabOffsetDesign)
   {
      GET_IFACE2_NOCHECK(pBroker,IBridge,pBridge);

      const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
      m_OldSlabOffsetType = pIBridgeDesc->GetSlabOffsetType();
      if (m_OldSlabOffsetType==pgsTypes::sotBridge)
      {
         m_OldBridgeSlabOffset = pBridgeDesc->GetSlabOffset();
      }
      else if (m_OldSlabOffsetType==pgsTypes::sotBearingLine)
      {
         // Store per pier
         PierIndexType nPiers = pBridgeDesc->GetPierCount();
         for (PierIndexType pierIdx = 0; pierIdx < nPiers; pierIdx++)
         {
            const CPierData2* pPier = pBridgeDesc->GetPier(pierIdx);
            Float64 back, ahead;
            pPier->GetSlabOffset(&back, &ahead);
            m_OldPierSlabOffsets.push_back(std::make_pair(back, ahead));
         }
      }
      else if (m_OldSlabOffsetType==pgsTypes::sotSegment)
      {
         // Store per girder
         GroupIndexType nGroups = pIBridgeDesc->GetGirderGroupCount();
         for (GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++)
         {
            const CGirderGroupData* pGroup = pIBridgeDesc->GetGirderGroup(grpIdx);
            GirderIndexType nGirders = pGroup->GetGirderCount();

            for (GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++)
            {
               ATLASSERT(pGroup->GetGirder(gdrIdx)->GetSegmentCount() == 1); // design is only for PGSuper which is one segment per girder
               CSegmentKey segmentKey(grpIdx, gdrIdx, 0);
               const CPrecastSegmentData* pSegment = pGroup->GetGirder(gdrIdx)->GetSegment(0);
               Float64 start, end;
               pSegment->GetSlabOffset(&start, &end);
               m_OldSegmentSlabOffsets.insert(std::make_pair(segmentKey, std::make_pair(start, end)));
            }
         }
      }
      else
      {
         ATLASSERT(0);
         m_DidSlabOffsetDesign = false; // this is very bad
      }
   }

   // Store original AssumedExcessCamber data. Format depends on type
   if (m_DidAssumedExcessCamberDesign)
   {
      GET_IFACE2_NOCHECK(pBroker,IBridge,pBridge);

      m_OldAssumedExcessCamberType = pIBridgeDesc->GetAssumedExcessCamberType();
      if (m_OldAssumedExcessCamberType==pgsTypes::aecBridge)
      {
         m_OldBridgeAssumedExcessCamber = pIBridgeDesc->GetAssumedExcessCamber(0,0); // same for all
      }
      else if (m_OldAssumedExcessCamberType==pgsTypes::aecSpan)
      {
         // Store per span
         GroupIndexType ngrp = pIBridgeDesc->GetGirderGroupCount();
         for (GroupIndexType igrp=0; igrp<ngrp; igrp++)
         {
            const CGirderGroupData* pGroup = pIBridgeDesc->GetGirderGroup(igrp);
            Float64 assumedExcessCamber = pIBridgeDesc->GetAssumedExcessCamber(igrp,0);
            m_OldAssumedExcessCamberData.push_back( OldAssumedExcessCamberData(igrp,INVALID_INDEX,assumedExcessCamber) );
         }
      }
      else if (m_OldAssumedExcessCamberType==pgsTypes::aecGirder)
      {
         // Store per girder
         GroupIndexType ngrp = pIBridgeDesc->GetGirderGroupCount();
         for (GroupIndexType igrp=0; igrp<ngrp; igrp++)
         {
            const CGirderGroupData* pGroup = pIBridgeDesc->GetGirderGroup(igrp);
            GirderIndexType ngdrs = pGroup->GetGirderCount();
            for (GirderIndexType igdr=0; igdr<ngdrs; igdr++)
            {
               Float64 assumedExcessCamber = pIBridgeDesc->GetAssumedExcessCamber(igrp,igdr);
               m_OldAssumedExcessCamberData.push_back( OldAssumedExcessCamberData(igrp,igdr,assumedExcessCamber) );
            }
         }
      }
      else
      {
         ATLASSERT(0);
         m_DidAssumedExcessCamberDesign = false;
      }
   }

   m_bInit = true; // initialization is complete, don't do it again
}

void txnDesignGirder::DoExecute(int i)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);

   GET_IFACE2(pBroker,IEvents, pEvents);
   // don't fire any changed events until all changes are done
   // Exception-safe holder for events
   CIEventsHolder event_holder(pEvents);

   // Loop over all girder designs
   for (const auto& rdata : m_DesignDataColl)
   {
#pragma Reminder("UPDATE: assuming precast girder bridge")
      SegmentIndexType segIdx = 0;
      const pgsSegmentDesignArtifact* pSegmentDesignArtifact = rdata.m_DesignArtifact.GetSegmentDesignArtifact(segIdx);
      const CSegmentKey& segmentKey = pSegmentDesignArtifact->GetSegmentKey();

      const arDesignOptions& design_options = pSegmentDesignArtifact->GetDesignOptions();

      // Set girder data for either flexural or shear design
      if (design_options.doDesignForFlexure != dtNoDesign || design_options.doDesignForShear)
      {
         pIBridgeDesc->SetPrecastSegmentData(segmentKey,rdata.m_SegmentData[i]);
      }
   }

   if (m_DidSlabOffsetDesign)
   {
      if (i==0)
      {
         // restore old data
         pIBridgeDesc->SetSlabOffsetType(m_OldSlabOffsetType);

         if (m_OldSlabOffsetType==pgsTypes::sotBridge)
         {
            pIBridgeDesc->SetSlabOffset(m_OldBridgeSlabOffset);
         }
         else if (m_OldSlabOffsetType==pgsTypes::sotBearingLine)
         {
            PierIndexType pierIdx = 0;
            for (const auto& offsets : m_OldPierSlabOffsets)
            {
               pIBridgeDesc->SetSlabOffset(pierIdx, offsets.first, offsets. second);
               pierIdx++;
            }
         }
         else if (m_OldSlabOffsetType==pgsTypes::sotSegment)
         {
            for (const auto& item : m_OldSegmentSlabOffsets)
            {
               pIBridgeDesc->SetSlabOffset(item.first, item.second.first, item.second.second);
            }
         }
         else
         {
            ATLASSERT(0);
         }
      }
      else
      {
         // Data for new design
         if (m_NewSlabOffsetType == sodtAllSelectedGirders)
         {
            pIBridgeDesc->SetSlabOffsetType(pgsTypes::sotSegment);

            for (DesignDataConstIter itdd=m_DesignDataColl.begin(); itdd!=m_DesignDataColl.end(); itdd++)
            {
               const pgsSegmentDesignArtifact* pArtifact = itdd->m_DesignArtifact.GetSegmentDesignArtifact(0);
               Float64 aStart = pArtifact->GetSlabOffset(pgsTypes::metStart);
               Float64 aEnd = pArtifact->GetSlabOffset(pgsTypes::metEnd);

               CSegmentKey segmentKey(itdd->m_DesignArtifact.GetGirderKey(), 0);
               pIBridgeDesc->SetSlabOffset(segmentKey, aStart, aEnd);
            }
         }
         else if (m_NewSlabOffsetType==sodtBridge)
         {
            pIBridgeDesc->SetSlabOffsetType(pgsTypes::sotBridge);
            pIBridgeDesc->SetSlabOffset(m_DesignSlabOffset[0]);
         }
         else if (m_NewSlabOffsetType==sodtPier)
         {
            pIBridgeDesc->SetSlabOffsetType(pgsTypes::sotBearingLine);
            pIBridgeDesc->SetSlabOffset((PierIndexType)m_FromSpanIdx, pgsTypes::Ahead,   m_DesignSlabOffset[pgsTypes::metStart]);
            pIBridgeDesc->SetSlabOffset((PierIndexType)(m_FromSpanIdx+1), pgsTypes::Back, m_DesignSlabOffset[pgsTypes::metEnd]);
         }
         else if (m_NewSlabOffsetType==sodtGirder)
         {
            pIBridgeDesc->SetSlabOffsetType(pgsTypes::sotSegment);
            CSegmentKey segmentKey((GroupIndexType)m_FromSpanIdx, m_FromGirderIdx, 0);
            pIBridgeDesc->SetSlabOffset(segmentKey, m_DesignSlabOffset[pgsTypes::metStart], m_DesignSlabOffset[pgsTypes::metEnd]);
         }
         else
         {
            ATLASSERT(0);
         }
      }
   }

   if (m_DidAssumedExcessCamberDesign)
   {
      if (i==0)
      {
         // restore old data
         pIBridgeDesc->SetAssumedExcessCamberType(m_OldAssumedExcessCamberType);

         if (m_OldAssumedExcessCamberType==pgsTypes::aecBridge)
         {
            pIBridgeDesc->SetAssumedExcessCamber(m_OldBridgeAssumedExcessCamber);
         }
         else if (m_OldAssumedExcessCamberType==pgsTypes::aecSpan)
         {
            for(const auto& rdata : m_OldAssumedExcessCamberData)
            {
               pIBridgeDesc->SetAssumedExcessCamber(rdata.GroupIdx, rdata.AssumedExcessCamber);
            }
         }
         else if (m_OldAssumedExcessCamberType==pgsTypes::aecGirder)
         {
            for (const auto& rdata : m_OldAssumedExcessCamberData)
            {
               pIBridgeDesc->SetAssumedExcessCamber(rdata.GroupIdx, rdata.GirderIdx, rdata.AssumedExcessCamber);
            }
         }
         else
         {
            ATLASSERT(0);
         }
      }
      else
      {
         // Data for new design
         if (m_NewSlabOffsetType == sodtAllSelectedGirders)
         {
            pIBridgeDesc->SetAssumedExcessCamberType(pgsTypes::aecGirder);

            for (DesignDataConstIter itdd=m_DesignDataColl.begin(); itdd!=m_DesignDataColl.end(); itdd++)
            {
               const pgsSegmentDesignArtifact* pArtifact = itdd->m_DesignArtifact.GetSegmentDesignArtifact(0);
               Float64 assumedExcessCamber = pArtifact->GetAssumedExcessCamber();

               SpanIndexType span = itdd->m_DesignArtifact.GetGirderKey().groupIndex;
               SpanIndexType gdr  = itdd->m_DesignArtifact.GetGirderKey().girderIndex;

               pIBridgeDesc->SetAssumedExcessCamber(span, gdr, assumedExcessCamber);
            }
         }
         else if (m_NewAssumedExcessCamberType==pgsTypes::aecBridge)
         {
            pIBridgeDesc->SetAssumedExcessCamberType(m_NewAssumedExcessCamberType);
            pIBridgeDesc->SetAssumedExcessCamber(m_DesignAssumedExcessCamber);
         }
         else if (m_NewAssumedExcessCamberType==pgsTypes::aecSpan)
         {
            pIBridgeDesc->SetAssumedExcessCamberType(m_NewAssumedExcessCamberType);
            pIBridgeDesc->SetAssumedExcessCamber(m_FromSpanIdx,m_DesignAssumedExcessCamber);
         }
         else if (m_NewAssumedExcessCamberType==pgsTypes::aecGirder)
         {
            pIBridgeDesc->SetAssumedExcessCamberType(m_NewAssumedExcessCamberType);
            pIBridgeDesc->SetAssumedExcessCamber(m_FromSpanIdx, m_FromGirderIdx, m_DesignAssumedExcessCamber);
         }
      }
   }
}
