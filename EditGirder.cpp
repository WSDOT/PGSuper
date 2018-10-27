///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2018  Washington State Department of Transportation
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
#include "EditGirder.h"
#include "PGSuperDoc.h"
#include <PgsExt\BridgeDescription2.h>
#include <IFace\GirderHandling.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

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

   m_AssExcessCamberType = rOther.m_AssExcessCamberType;
   m_AssExcessCamber = rOther.m_AssExcessCamber;
}

/////////////////////////////////////////////////////////////
txnEditGirder::txnEditGirder(const CGirderKey& girderKey,const txnEditGirderData& newGirderData)
{
   m_GirderKey = girderKey;
   ATLASSERT(m_GirderKey.groupIndex != INVALID_INDEX);
   m_NewGirderData = newGirderData;
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

      oldGirderData.m_SlabOffsetType = pBridgeDesc->GetSlabOffsetType();

      // this is a precast girder (only one segment per girder)
      PierIndexType startPierIdx = pGroup->GetPierIndex(pgsTypes::metStart);
      PierIndexType endPierIdx = pGroup->GetPierIndex(pgsTypes::metEnd);

      oldGirderData.m_SlabOffset[pgsTypes::metStart] = pIBridgeDesc->GetSlabOffset(m_GirderKey.groupIndex,startPierIdx,gdrIdx);
      oldGirderData.m_SlabOffset[pgsTypes::metEnd]   = pIBridgeDesc->GetSlabOffset(m_GirderKey.groupIndex,endPierIdx,  gdrIdx);

      oldGirderData.m_AssExcessCamberType = pBridgeDesc->GetAssExcessCamberType();
      // this is a precast girder (only one segment per girder)
      oldGirderData.m_AssExcessCamber = pIBridgeDesc->GetAssExcessCamber(m_GirderKey.groupIndex, gdrIdx);

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

txnTransaction* txnEditGirder::CreateClone() const
{
   return new txnEditGirder(m_GirderKey,m_NewGirderData);
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

bool txnEditGirder::IsUndoable()
{
   return true;
}

bool txnEditGirder::IsRepeatable()
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

   // set the slab offset
   pIBridgeDesc->SetSlabOffsetType( gdrData.m_SlabOffsetType );
   if ( gdrData.m_SlabOffsetType == pgsTypes::sotBridge )
   {
      // for the entire bridge
      pIBridgeDesc->SetSlabOffset( gdrData.m_SlabOffset[pgsTypes::metStart] );
   }
   else if ( gdrData.m_SlabOffsetType == pgsTypes::sotPier )
   {
      // for this span
      const CGirderGroupData* pGroup = pIBridgeDesc->GetGirderGroup(girderKey.groupIndex);
      PierIndexType startPierIdx = pGroup->GetPier(pgsTypes::metStart)->GetIndex();
      PierIndexType endPierIdx   = pGroup->GetPier(pgsTypes::metEnd)->GetIndex();

      pIBridgeDesc->SetSlabOffset(girderKey.groupIndex, startPierIdx, gdrData.m_SlabOffset[pgsTypes::metStart]);
      pIBridgeDesc->SetSlabOffset(girderKey.groupIndex, endPierIdx,   gdrData.m_SlabOffset[pgsTypes::metEnd]);
   }
   else
   {
      // by girder
      const CGirderGroupData* pGroup = pIBridgeDesc->GetGirderGroup(segmentKey.groupIndex);
      PierIndexType startPierIdx = pGroup->GetPierIndex(pgsTypes::metStart);
      PierIndexType endPierIdx   = pGroup->GetPierIndex(pgsTypes::metEnd);

      if ( !bUndo && gdrData.m_SlabOffsetType != m_NewGirderData.m_SlabOffsetType )
      {
         // we are changing the slab offset type from something to "by girder"

         // need to make sure the "by girder" values are match the current slab offset for the girder
         // the current value is the value for this girder

         // get the current value of the slab offset
         Float64 start = pGroup->GetSlabOffset(startPierIdx,segmentKey.girderIndex);
         Float64 end   = pGroup->GetSlabOffset(endPierIdx,  segmentKey.girderIndex);

         // set the value for each girder to this current value
         GirderIndexType nGirders = pIBridgeDesc->GetBridgeDescription()->GetGirderGroup(girderKey.groupIndex)->GetGirderCount();
         for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
         {
            pIBridgeDesc->SetSlabOffset(girderKey.groupIndex,startPierIdx,gdrIdx,start);
            pIBridgeDesc->SetSlabOffset(girderKey.groupIndex,endPierIdx,  gdrIdx,end);
         }
      }
      
      // change the girder that was edited
      pIBridgeDesc->SetSlabOffset(segmentKey.groupIndex, startPierIdx, segmentKey.girderIndex, gdrData.m_SlabOffset[pgsTypes::metStart]);
      pIBridgeDesc->SetSlabOffset(segmentKey.groupIndex, endPierIdx,   segmentKey.girderIndex, gdrData.m_SlabOffset[pgsTypes::metEnd]  );
   }

   // set the Assumed Excess Camber
   pIBridgeDesc->SetAssExcessCamberType( gdrData.m_AssExcessCamberType );
   if ( gdrData.m_AssExcessCamberType == pgsTypes::aecBridge )
   {
      // for the entire bridge
      pIBridgeDesc->SetAssExcessCamber( gdrData.m_AssExcessCamber );
   }
   else if ( gdrData.m_AssExcessCamberType == pgsTypes::aecSpan )
   {
      // for this span
      pIBridgeDesc->SetAssExcessCamber(girderKey.groupIndex, gdrData.m_AssExcessCamber );
   }
   else
   {
      // change the girder that was edited
      pIBridgeDesc->SetAssExcessCamber(segmentKey.groupIndex, segmentKey.girderIndex, gdrData.m_AssExcessCamber);
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
