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

#include "PGSuperAppPlugin\stdafx.h"
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

txnDesignGirder::txnDesignGirder( std::vector<const pgsGirderDesignArtifact*>& artifacts, pgsTypes::SlabOffsetType slabOffsetType)
{
   std::vector<const pgsGirderDesignArtifact*>::const_iterator iter(artifacts.begin());
   std::vector<const pgsGirderDesignArtifact*>::const_iterator iterEnd(artifacts.end());
   for ( ; iter != iterEnd; iter++ )
   {
      const pgsGirderDesignArtifact& gdrDesignArtifact(*(*iter));
      DesignData data(gdrDesignArtifact);
      data.m_SlabOffsetType[1] = slabOffsetType;

      m_DesignDataColl.push_back(data);
   }

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

txnTransaction* txnDesignGirder::CreateClone() const
{
   pgsTypes::SlabOffsetType slabtype;
   std::vector<const pgsGirderDesignArtifact*> artifacts;
   bool first = true;
   for (DesignDataConstIter iter = m_DesignDataColl.begin(); iter!=m_DesignDataColl.end(); iter++)
   {
      artifacts.push_back(&(iter->m_DesignArtifact));

      if (first)
         slabtype = iter->m_SlabOffsetType[1];

      first = false;
   }

   return new txnDesignGirder(artifacts, slabtype);
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

bool txnDesignGirder::IsUndoable()
{
   return true;
}

bool txnDesignGirder::IsRepeatable()
{
   return false;
}

void txnDesignGirder::Init()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);

   BOOST_FOREACH(DesignData& rdata,m_DesignDataColl)
   {
#pragma Reminder("UPDATE: assuming precast girder bridge")
      SegmentIndexType segIdx = 0;
      const pgsSegmentDesignArtifact* pSegmentDesignArtifact = rdata.m_DesignArtifact.GetSegmentDesignArtifact(segIdx);
      const CSegmentKey& segmentKey = pSegmentDesignArtifact->GetSegmentKey();

      // old (existing) girder data
      rdata.m_SegmentData[0] = *(pIBridgeDesc->GetPrecastSegmentData(segmentKey));

      // new girder data. Artifact does the work
      rdata.m_SegmentData[1] = pSegmentDesignArtifact->GetSegmentData();

      // deck offset
      const CGirderGroupData* pGroup = pIBridgeDesc->GetGirderGroup(segmentKey.groupIndex);
      PierIndexType startPierIdx = pGroup->GetPierIndex(pgsTypes::metStart);
      PierIndexType endPierIdx   = pGroup->GetPierIndex(pgsTypes::metStart);

      rdata.m_SlabOffset[pgsTypes::metStart][0] = pIBridgeDesc->GetSlabOffset(segmentKey.groupIndex,startPierIdx,segmentKey.segmentIndex);
      rdata.m_SlabOffset[pgsTypes::metEnd][0]   = pIBridgeDesc->GetSlabOffset(segmentKey.groupIndex,endPierIdx,  segmentKey.segmentIndex);
      rdata.m_SlabOffset[pgsTypes::metStart][1] = pSegmentDesignArtifact->GetSlabOffset(pgsTypes::metStart);
      rdata.m_SlabOffset[pgsTypes::metEnd][1]   = pSegmentDesignArtifact->GetSlabOffset(pgsTypes::metEnd);

      // get old deck offset type... new type was set in constructor
      rdata.m_SlabOffsetType[0] = pIBridgeDesc->GetSlabOffsetType();
   }

   m_bInit = true; // initialization is complete, don't do it again
}

void txnDesignGirder::DoExecute(int i)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,ISegmentData,pSegmentData);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);

   GET_IFACE2(pBroker,IEvents, pEvents);
   pEvents->HoldEvents(); // don't fire any changed events until all changes are done

   // Loop over all girder designs
   BOOST_FOREACH(DesignData& rdata,m_DesignDataColl)
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

         // Slab offset set only for flexural design
         if ( design_options.doDesignForFlexure!=dtNoDesign && design_options.doDesignSlabOffset )
         {
            if ( rdata.m_SlabOffsetType[i] == pgsTypes::sotBridge )
            {
               pIBridgeDesc->SetSlabOffset(rdata.m_SlabOffset[pgsTypes::metStart][i]);
            }
            else
            {
               GET_IFACE2(pBroker,IBridge,pBridge);
               PierIndexType startPierIdx, endPierIdx;
               pBridge->GetGirderGroupPiers(segmentKey.groupIndex,&startPierIdx,&endPierIdx);
               pIBridgeDesc->SetSlabOffset(segmentKey.groupIndex,startPierIdx,segmentKey.girderIndex,rdata.m_SlabOffset[pgsTypes::metStart][i]);
               pIBridgeDesc->SetSlabOffset(segmentKey.groupIndex,endPierIdx,  segmentKey.girderIndex,rdata.m_SlabOffset[pgsTypes::metEnd][i]);
            }
         }
      }
   }

  pEvents->FirePendingEvents();
}
