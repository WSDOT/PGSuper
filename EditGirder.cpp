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

   m_SlabOffsetType = rOther.m_SlabOffsetType;
   m_SlabOffset[pgsTypes::metStart] = rOther.m_SlabOffset[pgsTypes::metStart];
   m_SlabOffset[pgsTypes::metEnd] = rOther.m_SlabOffset[pgsTypes::metEnd];
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
   pEvents->HoldEvents(); // don't fire any changed events until all changes are done

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
      oldGirderData.m_SlabOffset[pgsTypes::metStart] = pIBridgeDesc->GetSlabOffset(m_GirderKey.groupIndex,pGroup->GetPierIndex(pgsTypes::metStart),gdrIdx);
      oldGirderData.m_SlabOffset[pgsTypes::metEnd]   = pIBridgeDesc->GetSlabOffset(m_GirderKey.groupIndex,pGroup->GetPierIndex(pgsTypes::metEnd),  gdrIdx);

      oldGirderData.m_Girder = *pGroup->GetGirder(gdrIdx);

      m_OldGirderData.insert(oldGirderData);

      // Copy the new girder data onto this girder
      SetGirderData(oldGirderData.m_GirderKey,m_NewGirderData,false);
   }

   pEvents->FirePendingEvents();

   return true;
}

void txnEditGirder::Undo()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IEvents, pEvents);
   pEvents->HoldEvents(); // don't fire any changed events until all changes are done

   std::set<txnEditGirderData>::iterator iter(m_OldGirderData.begin());
   std::set<txnEditGirderData>::iterator end(m_OldGirderData.end());
   for ( ; iter != end; iter++ )
   {
      txnEditGirderData& oldGirderData = *iter;
      SetGirderData(oldGirderData.m_GirderKey,oldGirderData,true);
   }

   pEvents->FirePendingEvents();
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
   /// this is a precast girder bridge (PGSuepr) so there is only the one segment
   CSegmentKey segmentKey(girderKey.groupIndex,girderKey.girderIndex,0);

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);

   pIBridgeDesc->UseSameGirderForEntireBridge( gdrData.m_bUseSameGirder );

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

   if ( !gdrData.m_bUseSameGirder )
   {
      pIBridgeDesc->SetGirderName( segmentKey, gdrData.m_strGirderName.c_str() );
   }
   else
   {
      pIBridgeDesc->SetGirderName( gdrData.m_strGirderName.c_str() );
   }

   // Girder Data
   pIBridgeDesc->SetGirder(girderKey,gdrData.m_Girder);
}
