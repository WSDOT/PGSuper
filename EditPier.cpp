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
#include "EditPier.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

txnEditPierData::txnEditPierData()
{
}

txnEditPierData::txnEditPierData(const CPierData2* pPier)
{
   const CBridgeDescription2* pBridgeDesc = pPier->GetBridgeDescription();

   Station        = pPier->GetStation();
   Orientation    = pPier->GetOrientation();
   ErectionEventIndex = pBridgeDesc->GetTimelineManager()->GetPierErectionEventIndex(pPier->GetIndex());

   PierConnectionType = pPier->GetPierConnectionType();
   SegmentConnectionType = pPier->GetSegmentConnectionType();
   for ( int i = 0; i < 2; i++ )
   {
      pPier->GetBearingOffset((pgsTypes::PierFaceType)i,&BearingOffset[i],&BearingOffsetMeasurementType[i]);
      pPier->GetGirderEndDistance((pgsTypes::PierFaceType)i,&EndDistance[i],&EndDistanceMeasurementType[i]);
      SupportWidth[i] = pPier->GetSupportWidth((pgsTypes::PierFaceType)i);
   }

   const CGirderGroupData* pPrevGroup = pBridgeDesc->GetGirderGroup(pPier->GetPrevSpan());
   const CGirderGroupData* pNextGroup = pBridgeDesc->GetGirderGroup(pPier->GetNextSpan());

   if ( pPrevGroup )
   {
      GirderSpacing[pgsTypes::Back]  = *pPier->GetGirderSpacing(pgsTypes::Back);
      nGirders[pgsTypes::Back] = pPrevGroup->GetGirderCount();

      SlabOffset[pgsTypes::Back] = pPrevGroup->GetSlabOffset(0,pgsTypes::metEnd);
   }

   if ( pNextGroup )
   {
      GirderSpacing[pgsTypes::Ahead]  = *pPier->GetGirderSpacing(pgsTypes::Ahead);

      nGirders[pgsTypes::Ahead] = pNextGroup->GetGirderCount();

      SlabOffset[pgsTypes::Ahead] = pNextGroup->GetSlabOffset(0,pgsTypes::metStart);
   }
   
   SlabOffsetType = pBridgeDesc->GetSlabOffsetType();

   UseSameNumberOfGirdersInAllGroups   = pPier->GetBridgeDescription()->UseSameNumberOfGirdersInAllGroups();
   GirderSpacingType                   = pPier->GetBridgeDescription()->GetGirderSpacingType();
   GirderMeasurementLocation           = pPier->GetBridgeDescription()->GetMeasurementLocation();

   DiaphragmHeight[pgsTypes::Back]       = pPier->GetDiaphragmHeight(pgsTypes::Back);
   DiaphragmWidth[pgsTypes::Back]        = pPier->GetDiaphragmWidth(pgsTypes::Back);
   DiaphragmLoadType[pgsTypes::Back]     = pPier->GetDiaphragmLoadType(pgsTypes::Back);
   DiaphragmLoadLocation[pgsTypes::Back] = pPier->GetDiaphragmLoadLocation(pgsTypes::Back);

   DiaphragmHeight[pgsTypes::Ahead]       = pPier->GetDiaphragmHeight(pgsTypes::Ahead);
   DiaphragmWidth[pgsTypes::Ahead]        = pPier->GetDiaphragmWidth(pgsTypes::Ahead);
   DiaphragmLoadType[pgsTypes::Ahead]     = pPier->GetDiaphragmLoadType(pgsTypes::Ahead);
   DiaphragmLoadLocation[pgsTypes::Ahead] = pPier->GetDiaphragmLoadLocation(pgsTypes::Ahead);
}

txnEditPier::txnEditPier(PierIndexType pierIdx,
                         const txnEditPierData& oldPierData,
                         const txnEditPierData& newPierData,
                         pgsTypes::MovePierOption moveOption)
{
   m_PierIdx = pierIdx;
   m_PierData[0] = oldPierData;
   m_PierData[1] = newPierData;

   m_MoveOption = moveOption;

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IBridgeDescription,pBridgeDesc);
   PierIndexType nPiers = pBridgeDesc->GetBridgeDescription()->GetPierCount();
   if ( m_PierIdx == 0 || m_PierIdx == nPiers-1 )
      m_strPierType = _T("Abutment");
   else
      m_strPierType = _T("Pier");
}

txnEditPier::~txnEditPier()
{
}

bool txnEditPier::Execute()
{
   DoExecute(1);
   return true;
}

void txnEditPier::Undo()
{
   DoExecute(0);
}

txnTransaction* txnEditPier::CreateClone() const
{
   return new txnEditPier(m_PierIdx,
                          m_PierData[0],
                          m_PierData[1],
                          m_MoveOption);
}

std::_tstring txnEditPier::Name() const
{
   std::_tostringstream os;
   os << _T("Edit ") << m_strPierType << _T(" ") << (m_PierIdx+1);
   return os.str();
}

bool txnEditPier::IsUndoable()
{
   return true;
}

bool txnEditPier::IsRepeatable()
{
   return false;
}

void txnEditPier::DoExecute(int i)
{
   // NOTE: This code block is very similar to CBridgeDescFramingGrid::EditPier(PierIndexType pierIdx)
   // If changes are made here, it is likely that similar changes are needed there

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker, IEvents, pEvents);
   pEvents->HoldEvents();

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);

   // Move pier based on new station
   pIBridgeDesc->MovePier(m_PierIdx, m_PierData[i].Station, m_MoveOption);

   // Make a copy of the original pier that is getting edited
   CPierData2 pierData( *pIBridgeDesc->GetPier(m_PierIdx) );

   // Tweak its data
   pierData.SetPierConnectionType( m_PierData[i].PierConnectionType );
   pierData.SetSegmentConnectionType( m_PierData[i].SegmentConnectionType );
   pierData.SetOrientation(m_PierData[i].Orientation.c_str());
   pierData.SetBearingOffset(pgsTypes::Back, m_PierData[i].BearingOffset[pgsTypes::Back],  m_PierData[i].BearingOffsetMeasurementType[pgsTypes::Back]);
   pierData.SetBearingOffset(pgsTypes::Ahead,m_PierData[i].BearingOffset[pgsTypes::Ahead], m_PierData[i].BearingOffsetMeasurementType[pgsTypes::Ahead]);
   pierData.SetGirderEndDistance(pgsTypes::Back, m_PierData[i].EndDistance[pgsTypes::Back], m_PierData[i].EndDistanceMeasurementType[pgsTypes::Back]);
   pierData.SetGirderEndDistance(pgsTypes::Ahead,m_PierData[i].EndDistance[pgsTypes::Ahead],m_PierData[i].EndDistanceMeasurementType[pgsTypes::Ahead]);
   pierData.SetSupportWidth(pgsTypes::Back, m_PierData[i].SupportWidth[pgsTypes::Back]);
   pierData.SetSupportWidth(pgsTypes::Ahead,m_PierData[i].SupportWidth[pgsTypes::Ahead]);

   pierData.SetDiaphragmHeight(pgsTypes::Back,m_PierData[i].DiaphragmHeight[pgsTypes::Back]);
   pierData.SetDiaphragmWidth(pgsTypes::Back,m_PierData[i].DiaphragmWidth[pgsTypes::Back]);
   pierData.SetDiaphragmLoadType(pgsTypes::Back,m_PierData[i].DiaphragmLoadType[pgsTypes::Back]);
   pierData.SetDiaphragmLoadLocation(pgsTypes::Back,m_PierData[i].DiaphragmLoadLocation[pgsTypes::Back]);

   pierData.SetDiaphragmHeight(pgsTypes::Ahead,m_PierData[i].DiaphragmHeight[pgsTypes::Ahead]);
   pierData.SetDiaphragmWidth(pgsTypes::Ahead,m_PierData[i].DiaphragmWidth[pgsTypes::Ahead]);
   pierData.SetDiaphragmLoadType(pgsTypes::Ahead,m_PierData[i].DiaphragmLoadType[pgsTypes::Ahead]);
   pierData.SetDiaphragmLoadLocation(pgsTypes::Ahead,m_PierData[i].DiaphragmLoadLocation[pgsTypes::Ahead]);

   // Replace the pier with the new data
   pIBridgeDesc->SetPierByIndex(m_PierIdx,pierData);

   pIBridgeDesc->SetPierErectionEventByIndex(m_PierIdx,m_PierData[i].ErectionEventIndex);

   // Number of girders in group that is adjacent to this pier is being edited also
   // Edit the number of girders now
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CPierData2* pPier = pBridgeDesc->GetPier(m_PierIdx);
   const CGirderGroupData* pPrevGroup = pBridgeDesc->GetGirderGroup( pPier->GetPrevSpan() );
   const CGirderGroupData* pNextGroup = pBridgeDesc->GetGirderGroup( pPier->GetNextSpan() );

   if ( pPier->IsBoundaryPier() )
   {
      // Pier is at the boundary of a group, so the number of girders in the group can be changed
      
      // Update the bridge level setting
      pIBridgeDesc->UseSameNumberOfGirdersInAllGroups(m_PierData[i].UseSameNumberOfGirdersInAllGroups);

      GirderIndexType nGirders = 999; // initialize with dummy value

      if ( m_PierData[i].UseSameNumberOfGirdersInAllGroups )
      {
         // if there is a group on the back side of the pier
         // use the number of girders on the back side of the pier, otherwise on the ahead side
         if ( pPrevGroup )
            nGirders = m_PierData[i].nGirders[pgsTypes::Back];
         else
            nGirders = m_PierData[i].nGirders[pgsTypes::Ahead];

         // Set the number of girders for the entire bridge
         pIBridgeDesc->SetGirderCount( nGirders );
      }

      // A unique number of girders is used in each group
      if ( pPrevGroup )
      {
         nGirders = _cpp_min(nGirders,m_PierData[i].nGirders[pgsTypes::Back]);
         pIBridgeDesc->SetGirderCount( pPrevGroup->GetIndex(), nGirders );
      }

      if ( pNextGroup )
      {
         nGirders = _cpp_min(nGirders,m_PierData[i].nGirders[pgsTypes::Ahead]);
         pIBridgeDesc->SetGirderCount( pNextGroup->GetIndex(), nGirders );
      }
   }

  // Girder Spacing (spacing is only defined at the ends of segments)
   if ( pPier->IsBoundaryPier() ||
        pPier->IsInteriorPier() && (pPier->GetSegmentConnectionType() == pgsTypes::psctContinousClosurePour || pPier->GetSegmentConnectionType() == pgsTypes::psctIntegralClosurePour) )
   {
      pIBridgeDesc->SetGirderSpacingType(  m_PierData[i].GirderSpacingType);
      pIBridgeDesc->SetMeasurementLocation(m_PierData[i].GirderMeasurementLocation);

      // Set girder spacing at the bridge level
      if ( IsGirderSpacing(m_PierData[i].GirderSpacingType) )
      {
         pgsTypes::PierFaceType pierFace = (pPrevGroup ? pgsTypes::Back : pgsTypes::Ahead);

         pIBridgeDesc->SetGirderSpacing(       m_PierData[i].GirderSpacing[pierFace].GetGirderSpacing(0) );
         pIBridgeDesc->SetMeasurementType(     m_PierData[i].GirderSpacing[pierFace].GetMeasurementType() );
         pIBridgeDesc->SetMeasurementLocation( m_PierData[i].GirderSpacing[pierFace].GetMeasurementLocation() );
      }

      // Set spacing at the pier
      if ( pPrevGroup && 2 <= pPrevGroup->GetGirderCount() )
      {
         CGirderSpacing2 girderSpacing( m_PierData[i].GirderSpacing[pgsTypes::Back] );
         pIBridgeDesc->SetGirderSpacing(m_PierIdx, pgsTypes::Back, girderSpacing);
      }

      if ( pNextGroup && 2 <= pNextGroup->GetGirderCount() )
      {
         CGirderSpacing2 girderSpacing( m_PierData[i].GirderSpacing[pgsTypes::Ahead] );
         pIBridgeDesc->SetGirderSpacing(m_PierIdx, pgsTypes::Ahead, girderSpacing);
      }
   }

   if ( m_PierData[i].SlabOffsetType == pgsTypes::sotBridge )
   {
      // changing to whole bridge slab offset
      if ( pPrevGroup == pNextGroup ) // only a single spacing is needed... and it is stored in Back
         pIBridgeDesc->SetSlabOffset(m_PierData[i].SlabOffset[pgsTypes::Back]);
      else if ( pNextGroup )
         pIBridgeDesc->SetSlabOffset(m_PierData[i].SlabOffset[pgsTypes::Ahead]);
      else
         pIBridgeDesc->SetSlabOffset(m_PierData[i].SlabOffset[pgsTypes::Back]);
   }
   else if ( m_PierData[i].SlabOffsetType == pgsTypes::sotGroup )
   {
      // changing to group by group slab offset
      if ( pPrevGroup )
      {
         CSegmentKey segmentKey(pPrevGroup->GetIndex(),0,0);
         Float64 start,end;
         pIBridgeDesc->GetSlabOffset(segmentKey,&start,&end);
         pIBridgeDesc->SetSlabOffset(pPrevGroup->GetIndex(),start,m_PierData[i].SlabOffset[pgsTypes::Back]);
      }

      if ( pNextGroup )
      {
         CSegmentKey segmentKey(pNextGroup->GetIndex(),0,0);
         Float64 start,end;
         pIBridgeDesc->GetSlabOffset(segmentKey,&start,&end);
         pIBridgeDesc->SetSlabOffset(pNextGroup->GetIndex(),m_PierData[i].SlabOffset[pgsTypes::Ahead],end);
      }
   }
   else if ( m_PierData[i].SlabOffsetType == pgsTypes::sotSegment )
   {
      ATLASSERT(false); // need to finish this
#pragma Reminder("IMPLEMENT: Slab Offset")
   }

   pEvents->FirePendingEvents();
}
