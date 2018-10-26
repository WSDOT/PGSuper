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
#include "EditSpan.h"

#include <IFace\Project.h>
#include <EAF\EAFDocument.h>
#include "PGSuperDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

txnEditSpanData::txnEditSpanData()
{
}

txnEditSpanData::txnEditSpanData(const txnEditSpanData& other)
{
   m_SpanLength = other.m_SpanLength;

   for ( int i = 0; i < 2; i++ )
   {
      m_ConnectionType[i] = other.m_ConnectionType[i];
   

      for ( int j = 0; j < 2; j++ )
      {
         m_EndDistanceMeasurementType[i][j]   = other.m_EndDistanceMeasurementType[i][j];
         m_EndDistance[i][j]                  = other.m_EndDistance[i][j];;
         m_BearingOffsetMeasurementType[i][j] = other.m_BearingOffsetMeasurementType[i][j];
         m_BearingOffset[i][j]                = other.m_BearingOffset[i][j];
         m_SupportWidth[i][j]                 = other.m_SupportWidth[i][j];
      }

      m_DiaphragmHeight[i]       = other.m_DiaphragmHeight[i];
      m_DiaphragmWidth[i]        = other.m_DiaphragmWidth[i];
      m_DiaphragmLoadType[i]     = other.m_DiaphragmLoadType[i];
      m_DiaphragmLoadLocation[i] = other.m_DiaphragmLoadLocation[i];

      m_GirderSpacing[i] = other.m_GirderSpacing[i];
      m_SlabOffset[i] = other.m_SlabOffset[i];
   }

   m_nGirders = other.m_nGirders;
   m_bUseSameNumGirders = other.m_bUseSameNumGirders;
   m_bUseSameGirderType = other.m_bUseSameGirderType;
   m_GirderSpacingType  = other.m_GirderSpacingType;
   m_GirderSpacingMeasurementLocation = other.m_GirderSpacingMeasurementLocation;
   m_GirderGroup = other.m_GirderGroup;

   m_SlabOffsetType = other.m_SlabOffsetType;
}

txnEditSpanData::txnEditSpanData(const CSpanData2* pSpan)
{
   m_SpanLength = pSpan->GetSpanLength();

   ATLASSERT(pSpan->GetPrevPier()->IsBoundaryPier());
   ATLASSERT(pSpan->GetNextPier()->IsBoundaryPier());
   m_ConnectionType[pgsTypes::metStart] = pSpan->GetPrevPier()->GetPierConnectionType();
   m_ConnectionType[pgsTypes::metEnd]   = pSpan->GetNextPier()->GetPierConnectionType();

   pSpan->GetPrevPier()->GetGirderEndDistance(pgsTypes::Back, &m_EndDistance[pgsTypes::metStart][pgsTypes::Back ],&m_EndDistanceMeasurementType[pgsTypes::metStart][pgsTypes::Back]);
   pSpan->GetPrevPier()->GetGirderEndDistance(pgsTypes::Ahead,&m_EndDistance[pgsTypes::metStart][pgsTypes::Ahead],&m_EndDistanceMeasurementType[pgsTypes::metStart][pgsTypes::Ahead]);

   pSpan->GetNextPier()->GetGirderEndDistance(pgsTypes::Back, &m_EndDistance[pgsTypes::metEnd][pgsTypes::Back ],&m_EndDistanceMeasurementType[pgsTypes::metEnd][pgsTypes::Back]);
   pSpan->GetNextPier()->GetGirderEndDistance(pgsTypes::Ahead,&m_EndDistance[pgsTypes::metEnd][pgsTypes::Ahead],&m_EndDistanceMeasurementType[pgsTypes::metEnd][pgsTypes::Ahead]);

   pSpan->GetPrevPier()->GetBearingOffset(pgsTypes::Back, &m_BearingOffset[pgsTypes::metStart][pgsTypes::Back ],&m_BearingOffsetMeasurementType[pgsTypes::metStart][pgsTypes::Back]);
   pSpan->GetPrevPier()->GetBearingOffset(pgsTypes::Ahead,&m_BearingOffset[pgsTypes::metStart][pgsTypes::Ahead],&m_BearingOffsetMeasurementType[pgsTypes::metStart][pgsTypes::Ahead]);

   pSpan->GetNextPier()->GetBearingOffset(pgsTypes::Back, &m_BearingOffset[pgsTypes::metEnd][pgsTypes::Back ],&m_BearingOffsetMeasurementType[pgsTypes::metEnd][pgsTypes::Back]);
   pSpan->GetNextPier()->GetBearingOffset(pgsTypes::Ahead,&m_BearingOffset[pgsTypes::metEnd][pgsTypes::Ahead],&m_BearingOffsetMeasurementType[pgsTypes::metEnd][pgsTypes::Ahead]);

   m_SupportWidth[pgsTypes::Back ][pgsTypes::metStart] = pSpan->GetPrevPier()->GetSupportWidth(pgsTypes::Back);
   m_SupportWidth[pgsTypes::Ahead][pgsTypes::metStart] = pSpan->GetPrevPier()->GetSupportWidth(pgsTypes::Ahead);

   m_SupportWidth[pgsTypes::Back ][pgsTypes::metEnd] = pSpan->GetNextPier()->GetSupportWidth(pgsTypes::Back);
   m_SupportWidth[pgsTypes::Ahead][pgsTypes::metEnd] = pSpan->GetNextPier()->GetSupportWidth(pgsTypes::Ahead);

   m_DiaphragmHeight[pgsTypes::Ahead] = pSpan->GetPrevPier()->GetDiaphragmHeight(pgsTypes::Ahead);
   m_DiaphragmHeight[pgsTypes::Back]  = pSpan->GetNextPier()->GetDiaphragmHeight(pgsTypes::Back);

   m_DiaphragmWidth[pgsTypes::Ahead] = pSpan->GetPrevPier()->GetDiaphragmWidth(pgsTypes::Ahead);
   m_DiaphragmWidth[pgsTypes::Back]  = pSpan->GetNextPier()->GetDiaphragmWidth(pgsTypes::Back);

   m_DiaphragmLoadType[pgsTypes::Ahead] = pSpan->GetPrevPier()->GetDiaphragmLoadType(pgsTypes::Ahead);
   m_DiaphragmLoadType[pgsTypes::Back]  = pSpan->GetNextPier()->GetDiaphragmLoadType(pgsTypes::Back);

   m_DiaphragmLoadLocation[pgsTypes::Ahead] = pSpan->GetPrevPier()->GetDiaphragmLoadLocation(pgsTypes::Ahead);
   m_DiaphragmLoadLocation[pgsTypes::Back]  = pSpan->GetNextPier()->GetDiaphragmLoadLocation(pgsTypes::Back);

   m_GirderSpacing[pgsTypes::Ahead] = *(pSpan->GetPrevPier()->GetGirderSpacing(pgsTypes::Ahead));
   m_GirderSpacing[pgsTypes::Back ] = *(pSpan->GetNextPier()->GetGirderSpacing(pgsTypes::Back));

   m_SlabOffset[pgsTypes::metStart] = pSpan->GetBridgeDescription()->GetGirderGroup(pSpan)->GetSlabOffset(0,pgsTypes::metStart);
   m_SlabOffset[pgsTypes::metEnd  ] = pSpan->GetBridgeDescription()->GetGirderGroup(pSpan)->GetSlabOffset(0,pgsTypes::metEnd);

   m_nGirders = pSpan->GetBridgeDescription()->GetGirderCount();
   m_bUseSameNumGirders = pSpan->GetBridgeDescription()->UseSameNumberOfGirdersInAllGroups();
   m_bUseSameGirderType = pSpan->GetBridgeDescription()->UseSameGirderForEntireBridge();
   m_GirderSpacingType  = pSpan->GetBridgeDescription()->GetGirderSpacingType();
   m_GirderSpacingMeasurementLocation = pSpan->GetBridgeDescription()->GetMeasurementLocation();
   m_GirderGroup = *pSpan->GetBridgeDescription()->GetGirderGroup(pSpan);

   m_SlabOffsetType = pSpan->GetBridgeDescription()->GetSlabOffsetType();
}

///////////////////////////////////////////////////////////////////////////////

txnEditSpan::txnEditSpan(SpanIndexType spanIdx,const txnEditSpanData& oldData,const txnEditSpanData& newData)
{
   m_SpanIdx = spanIdx;
   m_SpanData[0] = oldData;
   m_SpanData[1] = newData;
}

txnEditSpan::~txnEditSpan()
{
}

bool txnEditSpan::Execute()
{
   DoExecute(1);
   return true;
}

void txnEditSpan::Undo()
{
   DoExecute(0);
}

txnTransaction* txnEditSpan::CreateClone() const
{
   return new txnEditSpan(m_SpanIdx,m_SpanData[0],m_SpanData[1]);
}

std::_tstring txnEditSpan::Name() const
{
   std::_tostringstream os;
   os << "Edit Span " << LABEL_SPAN(m_SpanIdx);
   return os.str();
}

bool txnEditSpan::IsUndoable()
{
   return true;
}

bool txnEditSpan::IsRepeatable()
{
   return false;
}

void txnEditSpan::DoExecute(int i)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IEvents, pEvents);
   pEvents->HoldEvents(); // don't fire any changed events until all changes are done

   GET_IFACE2(pBroker,IBridgeDescription,pBridgeDesc);

   // General
   pBridgeDesc->SetSpanLength(m_SpanIdx,m_SpanData[i].m_SpanLength);

   CEAFDocument* pDoc = EAFGetDocument();
   if ( pDoc->IsKindOf(RUNTIME_CLASS(CPGSuperDoc)) )
   {
      // This code is only for PGSuper documents

      PierIndexType prevPierIdx = (PierIndexType)m_SpanIdx;
      PierIndexType nextPierIdx = prevPierIdx + 1;

      // in PGSuper, span and group are the same
      GroupIndexType grpIdx = (GroupIndexType)m_SpanIdx;

      // Girder Spacing (entire bridge)
      pBridgeDesc->UseSameNumberOfGirdersInAllGroups( m_SpanData[i].m_bUseSameNumGirders);
      pBridgeDesc->UseSameGirderForEntireBridge(m_SpanData[i].m_bUseSameGirderType);
      pBridgeDesc->SetGirderSpacingType(   m_SpanData[i].m_GirderSpacingType );
      pBridgeDesc->SetMeasurementLocation( m_SpanData[i].m_GirderSpacingMeasurementLocation);

      pBridgeDesc->SetGirderGroup(grpIdx, m_SpanData[i].m_GirderGroup);

      if ( m_SpanData[i].m_bUseSameNumGirders )
      {
         // set number of girder for the entire bridge
         pBridgeDesc->SetGirderCount(m_SpanData[i].m_nGirders);
      }

      pBridgeDesc->SetGirderCount( grpIdx, m_SpanData[i].m_nGirders);

      if ( m_SpanData[i].m_bUseSameGirderType )
      {
         pBridgeDesc->SetGirderName(m_SpanData[i].m_GirderGroup.GetGirderName(0));
      }

      if ( ::IsBridgeSpacing(m_SpanData[i].m_GirderSpacingType) )
      {
         // girder spacing for the entire bridge
         pBridgeDesc->SetGirderSpacing(       m_SpanData[i].m_GirderSpacing[pgsTypes::metStart].GetGirderSpacing(0) );
         pBridgeDesc->SetMeasurementLocation( m_SpanData[i].m_GirderSpacing[pgsTypes::metStart].GetMeasurementLocation() );
         pBridgeDesc->SetMeasurementType(     m_SpanData[i].m_GirderSpacing[pgsTypes::metStart].GetMeasurementType() );
      }
      else
      {
         pBridgeDesc->SetGirderSpacingAtStartOfGroup( grpIdx, m_SpanData[i].m_GirderSpacing[pgsTypes::metStart] );
         pBridgeDesc->SetGirderSpacingAtEndOfGroup(   grpIdx, m_SpanData[i].m_GirderSpacing[pgsTypes::metEnd]);
      }

      // Connections
      for ( int j = 0; j < 2; j++ )
      {
         pgsTypes::MemberEndType end = (j == 0 ? pgsTypes::metStart : pgsTypes::metEnd);
         PierIndexType pierIdx = (j == 0 ? prevPierIdx : nextPierIdx);
         CPierData2 pier = *pBridgeDesc->GetPier( pierIdx );
         ATLASSERT(pBridgeDesc->GetPier(pierIdx)->IsBoundaryPier());// this is setup for Boundary Piers
         pier.SetPierConnectionType( m_SpanData[i].m_ConnectionType[end] );

         // Diaphragm
         pier.SetDiaphragmHeight(       (pgsTypes::PierFaceType)end, m_SpanData[i].m_DiaphragmHeight[end]);
         pier.SetDiaphragmWidth(        (pgsTypes::PierFaceType)end, m_SpanData[i].m_DiaphragmWidth[end]);
         pier.SetDiaphragmLoadType(     (pgsTypes::PierFaceType)end, m_SpanData[i].m_DiaphragmLoadType[end]);
         pier.SetDiaphragmLoadLocation( (pgsTypes::PierFaceType)end, m_SpanData[i].m_DiaphragmLoadLocation[end]);

         for ( int k = 0; k < 2; k++ )
         {
            pgsTypes::PierFaceType face = (k == 0 ? pgsTypes::Ahead : pgsTypes::Back);

            pier.SetGirderEndDistance( face,m_SpanData[i].m_EndDistance[end][face],  m_SpanData[i].m_EndDistanceMeasurementType[end][face]);
            pier.SetBearingOffset(     face,m_SpanData[i].m_BearingOffset[end][face],m_SpanData[i].m_BearingOffsetMeasurementType[end][face]);
            pier.SetSupportWidth(      face,m_SpanData[i].m_SupportWidth[end][face]);

            pBridgeDesc->SetPierByIndex(pierIdx,pier);
         }
      }

      if ( m_SpanData[i].m_SlabOffsetType == pgsTypes::sotBridge )
      {
         pBridgeDesc->SetSlabOffset(m_SpanData[i].m_SlabOffset[pgsTypes::metStart]);
      }
      else if ( m_SpanData[i].m_SlabOffsetType == pgsTypes::sotGroup )
      {
         pBridgeDesc->SetSlabOffset(grpIdx,m_SpanData[i].m_SlabOffset[pgsTypes::metStart],m_SpanData[i].m_SlabOffset[pgsTypes::metEnd]);
      }
      else if ( m_SpanData[i].m_SlabOffsetType == pgsTypes::sotSegment )
      {
         // can't handle this case right now
         // This case occurs when slab offset type was by segment and it was changed to by group
         // and then undo brings it back to by segment
         ATLASSERT(false);
#pragma Reminder("IMPLEMENT: Slab offset")
      }
   }

   pEvents->FirePendingEvents();
}
