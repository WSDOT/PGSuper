///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2010  Washington State Department of Transportation
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
#include "PGSuperDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

txnEditSpanData::txnEditSpanData()
{
}

txnEditSpanData::txnEditSpanData(const CSpanData* pSpan)
{
   bSameNumberOfGirdersInAllSpans = pSpan->GetBridgeDescription()->UseSameNumberOfGirdersInAllSpans();
   bSameGirderType                = pSpan->GetBridgeDescription()->UseSameGirderForEntireBridge();
   bSameGirderSpacingAtEachEnd    = pSpan->UseSameSpacingAtBothEndsOfSpan();

   SpanLength = pSpan->GetSpanLength();

   nGirders = pSpan->GetGirderCount();
   GirderTypes = *pSpan->GetGirderTypes();

   if ( pSpan->GetPrevPier()->GetPrevSpan() )
      PrevPierConnection[pgsTypes::Back] = pSpan->GetPrevPier()->GetConnection(pgsTypes::Back);

   PrevPierConnection[pgsTypes::Ahead] = pSpan->GetPrevPier()->GetConnection(pgsTypes::Ahead);
   NextPierConnection[pgsTypes::Back]  = pSpan->GetNextPier()->GetConnection(pgsTypes::Back);

   if ( pSpan->GetNextPier()->GetNextSpan() )
      NextPierConnection[pgsTypes::Ahead] = pSpan->GetNextPier()->GetConnection(pgsTypes::Ahead);

   ConnectionType[pgsTypes::Ahead] = pSpan->GetPrevPier()->GetConnectionType();
   ConnectionType[pgsTypes::Back]  = pSpan->GetNextPier()->GetConnectionType();

   GirderSpacingType              = pSpan->GetBridgeDescription()->GetGirderSpacingType();
   GirderMeasurementLocation      = pSpan->GetBridgeDescription()->GetMeasurementLocation();
   GirderSpacing[pgsTypes::Ahead] = *pSpan->GetGirderSpacing(pgsTypes::metStart);
   GirderSpacing[pgsTypes::Back]  = *pSpan->GetGirderSpacing(pgsTypes::metEnd);

   SlabOffsetType = pSpan->GetBridgeDescription()->GetSlabOffsetType();
   if ( SlabOffsetType == pgsTypes::sotGirder )
   {
      // slab offset is by girder... which girder do we use???
      // use first girder for now
      SlabOffset[pgsTypes::metStart] = pSpan->GetGirderTypes()->GetSlabOffset(0,pgsTypes::metStart);
      SlabOffset[pgsTypes::metEnd]   = pSpan->GetGirderTypes()->GetSlabOffset(0,pgsTypes::metEnd);
   }
   else
   {
      SlabOffset[pgsTypes::metStart] = pSpan->GetSlabOffset(pgsTypes::metStart);
      SlabOffset[pgsTypes::metEnd]   = pSpan->GetSlabOffset(pgsTypes::metEnd);
   }
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

   pBridgeDesc->UseSameNumberOfGirdersInAllSpans( m_SpanData[i].bSameNumberOfGirdersInAllSpans);
   pBridgeDesc->UseSameGirderForEntireBridge(m_SpanData[i].bSameGirderType);

   pBridgeDesc->SetSpanLength(m_SpanIdx,m_SpanData[i].SpanLength);

   if ( m_SpanData[i].bSameNumberOfGirdersInAllSpans )
   {
      // set number of girder for the entire bridge
      pBridgeDesc->SetGirderCount(m_SpanData[i].nGirders);
   }

   pBridgeDesc->SetGirderCount(m_SpanIdx, m_SpanData[i].nGirders);

   if ( m_SpanData[i].bSameGirderType )
   {
      CGirderTypes girderTypes = m_SpanData[i].GirderTypes;
      pBridgeDesc->SetGirderName(girderTypes.GetGirderName(0));
   }

   CGirderTypes girderTypes = m_SpanData[i].GirderTypes;

   pBridgeDesc->SetGirderSpacingType(   m_SpanData[i].GirderSpacingType );
   pBridgeDesc->SetMeasurementLocation( m_SpanData[i].GirderMeasurementLocation);

   if ( IsGirderSpacing(m_SpanData[i].GirderSpacingType) )
   {
      pBridgeDesc->SetGirderSpacing(       m_SpanData[i].GirderSpacing[pgsTypes::Ahead].GetGirderSpacing(0) );
      pBridgeDesc->SetMeasurementLocation( m_SpanData[i].GirderSpacing[pgsTypes::Ahead].GetMeasurementLocation() );
      pBridgeDesc->SetMeasurementType(     m_SpanData[i].GirderSpacing[pgsTypes::Ahead].GetMeasurementType() );
   }

   pBridgeDesc->UseSameGirderSpacingAtBothEndsOfSpan(m_SpanIdx,m_SpanData[i].bSameGirderSpacingAtEachEnd);

   CGirderSpacing girderSpacingAhead( m_SpanData[i].GirderSpacing[pgsTypes::Ahead] );
   pBridgeDesc->SetGirderSpacingAtStartOfSpan( m_SpanIdx,girderSpacingAhead);
   pBridgeDesc->SetMeasurementLocation(        m_SpanIdx,pgsTypes::Ahead,girderSpacingAhead.GetMeasurementLocation());
   pBridgeDesc->SetMeasurementType(            m_SpanIdx,pgsTypes::Ahead,girderSpacingAhead.GetMeasurementType());

   CGirderSpacing girderSpacingBack( m_SpanData[i].GirderSpacing[pgsTypes::Back] );
   if ( !m_SpanData[i].bSameGirderSpacingAtEachEnd )
   {
      pBridgeDesc->SetGirderSpacingAtEndOfSpan(m_SpanIdx,girderSpacingBack);
   }
   pBridgeDesc->SetMeasurementLocation(     m_SpanIdx+1,pgsTypes::Back,girderSpacingBack.GetMeasurementLocation());
   pBridgeDesc->SetMeasurementType(         m_SpanIdx+1,pgsTypes::Back,girderSpacingBack.GetMeasurementType());

   CPierData prevPier = *pBridgeDesc->GetPier(m_SpanIdx);
   CPierData nextPier = *pBridgeDesc->GetPier(m_SpanIdx+1);

   if ( pBridgeDesc->GetPier(m_SpanIdx)->GetPrevSpan() )
      prevPier.SetConnection(pgsTypes::Back,      m_SpanData[i].PrevPierConnection[pgsTypes::Back].c_str() );

   prevPier.SetConnection(pgsTypes::Ahead,      m_SpanData[i].PrevPierConnection[pgsTypes::Ahead].c_str() );

   nextPier.SetConnection(pgsTypes::Back,      m_SpanData[i].NextPierConnection[pgsTypes::Back].c_str() );

   if ( pBridgeDesc->GetPier(m_SpanIdx+1)->GetNextSpan() )
      nextPier.SetConnection(pgsTypes::Ahead,      m_SpanData[i].NextPierConnection[pgsTypes::Ahead].c_str() );
   
   prevPier.SetConnectionType( m_SpanData[i].ConnectionType[pgsTypes::Ahead] );
   nextPier.SetConnectionType( m_SpanData[i].ConnectionType[pgsTypes::Back] );

   pBridgeDesc->SetPier(m_SpanIdx,  prevPier);
   pBridgeDesc->SetPier(m_SpanIdx+1,nextPier);

   if ( m_SpanData[i].SlabOffsetType == pgsTypes::sotBridge )
   {
      // changing to whole bridge slab offset
      pBridgeDesc->SetSlabOffset(m_SpanData[i].SlabOffset[pgsTypes::metStart]);
   }
   else if ( m_SpanData[i].SlabOffsetType == pgsTypes::sotSpan )
   {
      // changing to span by span slab offset
      pBridgeDesc->SetSlabOffset(m_SpanIdx,m_SpanData[i].SlabOffset[pgsTypes::metStart],m_SpanData[i].SlabOffset[pgsTypes::metEnd]);
   }
   else if ( m_SpanData[i].SlabOffsetType == pgsTypes::sotGirder )
   {
      // change to girder by girder slab offset
      GirderIndexType nGirders = pBridgeDesc->GetSpan(m_SpanIdx)->GetGirderCount();
      for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
      {
         girderTypes.SetSlabOffset(gdrIdx,pgsTypes::metStart,m_SpanData[i].SlabOffset[pgsTypes::metStart]);
         girderTypes.SetSlabOffset(gdrIdx,pgsTypes::metEnd,  m_SpanData[i].SlabOffset[pgsTypes::metEnd]);
      }
   }

   pBridgeDesc->SetGirderTypes(m_SpanIdx, girderTypes);

   pEvents->FirePendingEvents();
}
