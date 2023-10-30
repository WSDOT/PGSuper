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
#include "EditGirder.h"
#include "PGSuperDoc.h"
#include <PgsExt\BridgeDescription2.h>
#include <IFace\GirderHandling.h>

#if defined _DEBUG
#include <IFace\DocumentType.h>
#endif

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// NOTE: This transaction is for use with PGSuper only

txnEditGirderData::txnEditGirderData()
{
}

txnEditGirderData::txnEditGirderData(const txnEditGirderData& rOther)
{
   m_GirderKey = rOther.m_GirderKey;
   m_bUseSameGirder = rOther.m_bUseSameGirder;
   m_strGirderName = rOther.m_strGirderName;

   m_Girder = rOther.m_Girder;

   m_TimelineMgr = rOther.m_TimelineMgr;

   m_SlabOffsetType = rOther.m_SlabOffsetType;
   m_SlabOffset[pgsTypes::metStart] = rOther.m_SlabOffset[pgsTypes::metStart];
   m_SlabOffset[pgsTypes::metEnd] = rOther.m_SlabOffset[pgsTypes::metEnd];

   m_AssumedExcessCamberType = rOther.m_AssumedExcessCamberType;
   m_AssumedExcessCamber = rOther.m_AssumedExcessCamber;

   m_HaunchDepths = rOther.m_HaunchDepths;
}

/////////////////////////////////////////////////////////////
txnEditGirder::txnEditGirder(const CGirderKey& girderKey,const txnEditGirderData& newGirderData)
{
   m_GirderKey = girderKey;
   ATLASSERT(m_GirderKey.groupIndex != INVALID_INDEX);
   m_NewGirderData = newGirderData;

#if defined _DEBUG
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker, IDocumentType, pDocType);
   ATLASSERT(pDocType->IsPGSuperDocument());
#endif
}

txnEditGirder::~txnEditGirder()
{
}

bool txnEditGirder::Execute()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IEvents, pEvents);
   // Exception-safe holder to keep from fireing events until we are done
   CIEventsHolder event_holder(pEvents);

   m_OldGirderData.clear();

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(m_GirderKey.groupIndex);

   GirderIndexType nGirders    = pGroup->GetGirderCount();
   GirderIndexType firstGdrIdx = (m_GirderKey.girderIndex == ALL_GIRDERS ? 0 : m_GirderKey.girderIndex);
   GirderIndexType lastGdrIdx  = (m_GirderKey.girderIndex == ALL_GIRDERS ? nGirders-1 : firstGdrIdx);

   for ( GirderIndexType gdrIdx = firstGdrIdx; gdrIdx <= lastGdrIdx; gdrIdx++ )
   {
      // collect up the old girder data (we will need it for Undo)
      txnEditGirderData oldGirderData;
      oldGirderData.m_GirderKey = m_GirderKey;
      oldGirderData.m_GirderKey.girderIndex = gdrIdx;

      oldGirderData.m_bUseSameGirder = pIBridgeDesc->UseSameGirderForEntireBridge();
      oldGirderData.m_strGirderName = pGroup->GetGirderName(gdrIdx);

      // this is a precast girder (only one segment per girder)
      PierIndexType startPierIdx = pGroup->GetPierIndex(pgsTypes::metStart);
      PierIndexType endPierIdx = pGroup->GetPierIndex(pgsTypes::metEnd);

      ATLASSERT(pGroup->GetGirder(gdrIdx)->GetSegmentCount() == 1); // this is for PGSuper only
      CSegmentKey segmentKey(m_GirderKey.groupIndex, gdrIdx, 0);


      oldGirderData.m_SlabOffsetType = pBridgeDesc->GetSlabOffsetType();
      oldGirderData.m_AssumedExcessCamberType = pBridgeDesc->GetAssumedExcessCamberType();
      if (pIBridgeDesc->GetHaunchInputDepthType() == pgsTypes::hidACamber)
      {
      oldGirderData.m_SlabOffset[pgsTypes::metStart] = pIBridgeDesc->GetSlabOffset(segmentKey,pgsTypes::metStart);
      oldGirderData.m_SlabOffset[pgsTypes::metEnd] = pIBridgeDesc->GetSlabOffset(segmentKey,pgsTypes::metEnd);

      // this is a precast girder (only one segment per girder)
         oldGirderData.m_AssumedExcessCamber = pIBridgeDesc->GetAssumedExcessCamber(m_GirderKey.groupIndex,gdrIdx);
      }
      else
      {
         // Direct input of haunch. Zero out slab offset data
         oldGirderData.m_SlabOffset[pgsTypes::metStart] = 0.0;
         oldGirderData.m_SlabOffset[pgsTypes::metEnd] = 0.0;
         oldGirderData.m_AssumedExcessCamber = 0.0;

         pgsTypes::HaunchInputLocationType haunchInputLocationType = pBridgeDesc->GetHaunchInputLocationType();
         pgsTypes::HaunchLayoutType haunchLayoutType = pBridgeDesc->GetHaunchLayoutType();
         pgsTypes::HaunchInputDistributionType haunchInputDistributionType = pBridgeDesc->GetHaunchInputDistributionType();

         if (haunchLayoutType == pgsTypes::hltAlongSpans && haunchInputLocationType == pgsTypes::hilPerEach &&
            (haunchInputDistributionType == pgsTypes::hidUniform || haunchInputDistributionType == pgsTypes::hidAtEnds))
         {
            // haunch depths are in span object
            oldGirderData.m_HaunchDepths = pBridgeDesc->GetSpan(m_GirderKey.groupIndex)->GetDirectHaunchDepths(m_GirderKey.girderIndex);
         }
      }

      oldGirderData.m_Girder = *pGroup->GetGirder(gdrIdx);
      oldGirderData.m_TimelineMgr = (*pIBridgeDesc->GetTimelineManager());

      oldGirderData.m_BearingType = pIBridgeDesc->GetBearingType();
      oldGirderData.m_BearingData[pgsTypes::metStart] = *pIBridgeDesc->GetBearingData(startPierIdx, pgsTypes::Ahead, gdrIdx);
      oldGirderData.m_BearingData[pgsTypes::metEnd] =   *pIBridgeDesc->GetBearingData(endPierIdx, pgsTypes::Back, gdrIdx);

      m_OldGirderData.insert(oldGirderData);

      // Copy the new girder data onto this girder
      SetGirderData(oldGirderData.m_GirderKey,m_NewGirderData,false);
   }

   return true;
}

void txnEditGirder::Undo()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IEvents, pEvents);
   // Exception-safe holder to keep from fireing events until we are done
   CIEventsHolder event_holder(pEvents);

   std::set<txnEditGirderData>::iterator iter(m_OldGirderData.begin());
   std::set<txnEditGirderData>::iterator end(m_OldGirderData.end());
   for ( ; iter != end; iter++ )
   {
      const txnEditGirderData& oldGirderData = *iter;
      SetGirderData(oldGirderData.m_GirderKey,oldGirderData,true);
   }
}

std::unique_ptr<CEAFTransaction>txnEditGirder::CreateClone() const
{
   return std::make_unique<txnEditGirder>(m_GirderKey,m_NewGirderData);
}

std::_tstring txnEditGirder::Name() const
{
   std::_tostringstream os;
   if ( m_GirderKey.girderIndex == ALL_GIRDERS )
   {
      os << "Edit Span " << LABEL_SPAN(m_NewGirderData.m_Girder.GetGirderGroupIndex()) << " All Girders";
   }
   else
   {
      os << "Edit Span " << LABEL_SPAN(m_NewGirderData.m_Girder.GetGirderGroupIndex()) << " Girder " << LABEL_GIRDER(m_NewGirderData.m_Girder.GetIndex());
   }

   return os.str();
}

bool txnEditGirder::IsUndoable() const
{
   return true;
}

bool txnEditGirder::IsRepeatable() const
{
   return false;
}

void txnEditGirder::SetGirderData(const CGirderKey& girderKey,const txnEditGirderData& gdrData,bool bUndo)
{
   /// this is a precast girder bridge (PGSuper) so there is only the one segment
   CSegmentKey segmentKey(girderKey.groupIndex,girderKey.girderIndex,0);

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);

   pIBridgeDesc->UseSameGirderForEntireBridge( gdrData.m_bUseSameGirder );

   // Set Girder Data and then overwrite using our local information
   pIBridgeDesc->SetGirder(girderKey,gdrData.m_Girder);

   pIBridgeDesc->SetTimelineManager(gdrData.m_TimelineMgr);

   // Haunch and slab offset
   if (pIBridgeDesc->GetHaunchInputDepthType() == pgsTypes::hidACamber)
   {
   // set the slab offset
      if (gdrData.m_SlabOffsetType == pgsTypes::sotBridge)
   {
      // for the entire bridge
         pIBridgeDesc->SetSlabOffset(gdrData.m_SlabOffset[pgsTypes::metStart]);
   }
      else if (gdrData.m_SlabOffsetType == pgsTypes::sotBearingLine)
   {
      // for this bearing line
      const CGirderGroupData* pGroup = pIBridgeDesc->GetGirderGroup(girderKey.groupIndex);
      PierIndexType startPierIdx = pGroup->GetPier(pgsTypes::metStart)->GetIndex();
      PierIndexType endPierIdx   = pGroup->GetPier(pgsTypes::metEnd)->GetIndex();
      ATLASSERT(startPierIdx == endPierIdx - 1);

         pIBridgeDesc->SetSlabOffset(startPierIdx,pgsTypes::Ahead,gdrData.m_SlabOffset[pgsTypes::metStart]);
         pIBridgeDesc->SetSlabOffset(endPierIdx,pgsTypes::Back,gdrData.m_SlabOffset[pgsTypes::metEnd]);
   }
   else
   {
      // by girder
      const CGirderGroupData* pGroup = pIBridgeDesc->GetGirderGroup(segmentKey.groupIndex);
      PierIndexType startPierIdx = pGroup->GetPierIndex(pgsTypes::metStart);
      PierIndexType endPierIdx   = pGroup->GetPierIndex(pgsTypes::metEnd);

         if (!bUndo && gdrData.m_SlabOffsetType != m_NewGirderData.m_SlabOffsetType)
      {
         // we are changing the slab offset type from something to "by girder"

         // need to make sure the "by girder" values are matching the current slab offset for the girder
         // the current value is the value for this girder

         // get the current value of the slab offset
         Float64 start = pGroup->GetGirder(segmentKey.girderIndex)->GetSegment(segmentKey.segmentIndex)->GetSlabOffset(pgsTypes::metStart);
         Float64 end   = pGroup->GetGirder(segmentKey.girderIndex)->GetSegment(segmentKey.segmentIndex)->GetSlabOffset(pgsTypes::metEnd);

         // set the value for each girder to this current value
         GirderIndexType nGirders = pIBridgeDesc->GetBridgeDescription()->GetGirderGroup(girderKey.groupIndex)->GetGirderCount();
            for (GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++)
         {
               CSegmentKey thisSegmentKey(segmentKey.groupIndex,gdrIdx,0);
            pIBridgeDesc->SetSlabOffset(thisSegmentKey,start,end);
         }
      }
      
      // change the girder that was edited
         pIBridgeDesc->SetSlabOffset(segmentKey,gdrData.m_SlabOffset[pgsTypes::metStart],gdrData.m_SlabOffset[pgsTypes::metEnd]);
   }

   // set the Assumed Excess Camber
      if (gdrData.m_AssumedExcessCamberType == pgsTypes::aecBridge)
   {
      // for the entire bridge
         pIBridgeDesc->SetAssumedExcessCamber(gdrData.m_AssumedExcessCamber);
   }
      else if (gdrData.m_AssumedExcessCamberType == pgsTypes::aecSpan)
   {
      // for this span
         pIBridgeDesc->SetAssumedExcessCamber(girderKey.groupIndex,gdrData.m_AssumedExcessCamber);
   }
   else
   {
      // change the girder that was edited
         pIBridgeDesc->SetAssumedExcessCamber(segmentKey.groupIndex,segmentKey.girderIndex,gdrData.m_AssumedExcessCamber);
      }
   }
   else
   {
      // Input by direct haunch
      std::size_t siz = gdrData.m_HaunchDepths.size();
      if (siz > 0)
      {
         ATLASSERT(siz == pIBridgeDesc->GetHaunchInputDistributionType()); // vectors are sized per enum value
         // Haunch values are stored in spans.
         pIBridgeDesc->SetDirectHaunchDepthsPerSpan(segmentKey.groupIndex,segmentKey.girderIndex,gdrData.m_HaunchDepths);
      }
   }

   // set the bearing data
   pIBridgeDesc->SetBearingType( gdrData.m_BearingType );
   if ( gdrData.m_BearingType == pgsTypes::brtBridge )
   {
      // for the entire bridge
      pIBridgeDesc->SetBearingData( &gdrData.m_BearingData[pgsTypes::metStart] );
   }
   else if ( gdrData.m_BearingType == pgsTypes::brtPier )
   {
      // for this span
      const CGirderGroupData* pGroup = pIBridgeDesc->GetGirderGroup(girderKey.groupIndex);
      PierIndexType startPierIdx = pGroup->GetPier(pgsTypes::metStart)->GetIndex();
      PierIndexType endPierIdx   = pGroup->GetPier(pgsTypes::metEnd)->GetIndex();

      pIBridgeDesc->SetBearingData(girderKey.groupIndex, startPierIdx, pgsTypes::Ahead, &gdrData.m_BearingData[pgsTypes::metStart]);
      pIBridgeDesc->SetBearingData(girderKey.groupIndex, endPierIdx,   pgsTypes::Back,  &gdrData.m_BearingData[pgsTypes::metEnd]);
   }
   else
   {
      // by girder
      const CGirderGroupData* pGroup = pIBridgeDesc->GetGirderGroup(segmentKey.groupIndex);
      PierIndexType startPierIdx = pGroup->GetPierIndex(pgsTypes::metStart);
      PierIndexType endPierIdx   = pGroup->GetPierIndex(pgsTypes::metEnd);
      
      // change the girder that was edited
      pIBridgeDesc->SetBearingData(segmentKey.groupIndex, startPierIdx, pgsTypes::Ahead, segmentKey.girderIndex, &gdrData.m_BearingData[pgsTypes::metStart]);
      pIBridgeDesc->SetBearingData(segmentKey.groupIndex, endPierIdx,   pgsTypes::Back,  segmentKey.girderIndex, &gdrData.m_BearingData[pgsTypes::metEnd]  );
   }


   if ( !gdrData.m_bUseSameGirder )
   {
      pIBridgeDesc->SetGirderName( segmentKey, gdrData.m_strGirderName.c_str() );
   }
   else
   {
      pIBridgeDesc->SetGirderName( gdrData.m_strGirderName.c_str() );
   }
}
