///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2012  Washington State Department of Transportation
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
#include "PGSuperDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

txnEditPierData::txnEditPierData()
{
}

txnEditPierData::txnEditPierData(const CPierData* pPier)
{
   Station     = pPier->GetStation();
   Orientation = pPier->GetOrientation();

   Connection[pgsTypes::Back]  = pPier->GetConnection(pgsTypes::Back);
   Connection[pgsTypes::Ahead] = pPier->GetConnection(pgsTypes::Ahead);

   ConnectionType  = pPier->GetConnectionType();

   SlabOffsetType = pPier->GetBridgeDescription()->GetSlabOffsetType();

   if ( pPier->GetPrevSpan() )
   {
      GirderSpacing[pgsTypes::Back]  = *pPier->GetPrevSpan()->GetGirderSpacing(pgsTypes::metEnd);
      nGirders[pgsTypes::Back] = pPier->GetPrevSpan()->GetGirderTypes()->GetGirderCount();

      if ( SlabOffsetType == pgsTypes::sotGirder )
         SlabOffset[pgsTypes::Back] = pPier->GetPrevSpan()->GetGirderTypes()->GetSlabOffset(0,pgsTypes::metEnd);
      else
         SlabOffset[pgsTypes::Back] = pPier->GetPrevSpan()->GetSlabOffset(pgsTypes::metEnd);
   }

   if ( pPier->GetNextSpan() )
   {
      GirderSpacing[pgsTypes::Ahead]  = *pPier->GetNextSpan()->GetGirderSpacing(pgsTypes::metStart);
      nGirders[pgsTypes::Ahead] = pPier->GetNextSpan()->GetGirderTypes()->GetGirderCount();

      if ( SlabOffsetType == pgsTypes::sotGirder )
         SlabOffset[pgsTypes::Ahead] = pPier->GetNextSpan()->GetGirderTypes()->GetSlabOffset(0,pgsTypes::metStart);
      else
         SlabOffset[pgsTypes::Ahead] = pPier->GetNextSpan()->GetSlabOffset(pgsTypes::metStart);
   }

   UseSameNumberOfGirdersInAllSpans    = pPier->GetBridgeDescription()->UseSameNumberOfGirdersInAllSpans();

   GirderSpacingType                   = pPier->GetBridgeDescription()->GetGirderSpacingType();
   GirderMeasurementLocation           = pPier->GetBridgeDescription()->GetMeasurementLocation();
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
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker, IEvents, pEvents);
   pEvents->HoldEvents();

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);

   CPierData pierData = *pIBridgeDesc->GetPier(m_PierIdx);

   pierData.SetConnection(pgsTypes::Back,  m_PierData[i].Connection[pgsTypes::Back].c_str() );
   pierData.SetConnection(pgsTypes::Ahead, m_PierData[i].Connection[pgsTypes::Ahead].c_str());

   pierData.SetConnectionType( m_PierData[i].ConnectionType );

   pierData.SetOrientation(m_PierData[i].Orientation.c_str());

   pIBridgeDesc->SetPier(m_PierIdx,pierData);

   GirderIndexType nGirders = 999;

   pIBridgeDesc->UseSameNumberOfGirdersInAllSpans(m_PierData[i].UseSameNumberOfGirdersInAllSpans);

   if ( m_PierData[i].UseSameNumberOfGirdersInAllSpans )
   {
      if ( pIBridgeDesc->GetPier(m_PierIdx)->GetPrevSpan() )
         nGirders = m_PierData[i].nGirders[pgsTypes::Back];
      else
         nGirders = m_PierData[i].nGirders[pgsTypes::Ahead];

      pIBridgeDesc->SetGirderCount( nGirders );
   }

   if ( pIBridgeDesc->GetPier(m_PierIdx)->GetPrevSpan() )
   {
      nGirders = _cpp_min(nGirders,m_PierData[i].nGirders[pgsTypes::Back]);
      pIBridgeDesc->SetGirderCount( pIBridgeDesc->GetPier(m_PierIdx)->GetPrevSpan()->GetSpanIndex(), 
                                    nGirders );
   }

   if ( pIBridgeDesc->GetPier(m_PierIdx)->GetNextSpan() )
   {
      nGirders = m_PierData[i].nGirders[pgsTypes::Ahead];
      pIBridgeDesc->SetGirderCount( pIBridgeDesc->GetPier(m_PierIdx)->GetNextSpan()->GetSpanIndex(), 
                                    nGirders );
   }

   pIBridgeDesc->SetGirderSpacingType(m_PierData[i].GirderSpacingType);
   pIBridgeDesc->SetMeasurementLocation(m_PierData[i].GirderMeasurementLocation);

   if ( 2 <= nGirders )
   {
      if ( IsGirderSpacing(m_PierData[i].GirderSpacingType) )
      {
         pgsTypes::PierFaceType pierFace = (pIBridgeDesc->GetPier(m_PierIdx)->GetPrevSpan() ? pgsTypes::Back : pgsTypes::Ahead);

         pIBridgeDesc->SetGirderSpacing( m_PierData[i].GirderSpacing[pierFace].GetGirderSpacing(0) );
         pIBridgeDesc->SetMeasurementType( m_PierData[i].GirderSpacing[pierFace].GetMeasurementType() );
         pIBridgeDesc->SetMeasurementLocation( m_PierData[i].GirderSpacing[pierFace].GetMeasurementLocation() );
      }

      if ( pIBridgeDesc->GetPier(m_PierIdx)->GetPrevSpan() )
      {
         CGirderSpacing girderSpacing( m_PierData[i].GirderSpacing[pgsTypes::Back] );
         pIBridgeDesc->SetGirderSpacing(m_PierIdx, pgsTypes::Back, girderSpacing);
      }

      if ( pIBridgeDesc->GetPier(m_PierIdx)->GetNextSpan() )
      {
         CGirderSpacing girderSpacing( m_PierData[i].GirderSpacing[pgsTypes::Ahead] );
         pIBridgeDesc->SetGirderSpacing(m_PierIdx, pgsTypes::Ahead, girderSpacing);
      }
   }

   pIBridgeDesc->MovePier(m_PierIdx, m_PierData[i].Station, m_MoveOption);

   if ( m_PierData[i].SlabOffsetType == pgsTypes::sotBridge )
   {
      // changing to whole bridge slab offset
      if ( pIBridgeDesc->GetPier(m_PierIdx)->GetNextSpan() )
         pIBridgeDesc->SetSlabOffset(m_PierData[i].SlabOffset[pgsTypes::Ahead]);
      else
         pIBridgeDesc->SetSlabOffset(m_PierData[i].SlabOffset[pgsTypes::Back]);
   }
   else if ( m_PierData[i].SlabOffsetType == pgsTypes::sotSpan )
   {
      // changing to span by span slab offset
      SpanIndexType nextSpanIdx = m_PierIdx;
      SpanIndexType prevSpanIdx = nextSpanIdx - 1;

      if ( pIBridgeDesc->GetSpan(prevSpanIdx) )
      {
         Float64 start,end;
         pIBridgeDesc->GetSlabOffset(prevSpanIdx,0,&start,&end);
         pIBridgeDesc->SetSlabOffset(prevSpanIdx,start,m_PierData[i].SlabOffset[pgsTypes::Back]);
      }

      if ( pIBridgeDesc->GetSpan(nextSpanIdx) )
      {
         Float64 start,end;
         pIBridgeDesc->GetSlabOffset(nextSpanIdx,0,&start,&end);
         pIBridgeDesc->SetSlabOffset(nextSpanIdx,m_PierData[i].SlabOffset[pgsTypes::Ahead],end);
      }
   }
   else if ( m_PierData[i].SlabOffsetType == pgsTypes::sotGirder )
   {
      // change to girder by girder slab offset
      SpanIndexType nextSpanIdx = m_PierIdx;
      SpanIndexType prevSpanIdx = nextSpanIdx - 1;

      if ( pIBridgeDesc->GetSpan(prevSpanIdx) )
      {
         GirderIndexType nGirders = pIBridgeDesc->GetSpan(prevSpanIdx)->GetGirderCount();
         CGirderTypes girderTypes = *(pIBridgeDesc->GetSpan(prevSpanIdx)->GetGirderTypes());
         for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
         {
            girderTypes.SetSlabOffset(gdrIdx,pgsTypes::metEnd,m_PierData[i].SlabOffset[pgsTypes::Back]);
         }

         pIBridgeDesc->SetGirderTypes(prevSpanIdx,girderTypes);
      }

      if ( pIBridgeDesc->GetSpan(nextSpanIdx) )
      {
         GirderIndexType nGirders = pIBridgeDesc->GetSpan(nextSpanIdx)->GetGirderCount();
         CGirderTypes girderTypes = *(pIBridgeDesc->GetSpan(nextSpanIdx)->GetGirderTypes());
         for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
         {
            girderTypes.SetSlabOffset(gdrIdx,pgsTypes::metEnd,m_PierData[i].SlabOffset[pgsTypes::Ahead]);
         }

         pIBridgeDesc->SetGirderTypes(nextSpanIdx,girderTypes);
      }
   }

   pEvents->FirePendingEvents();
}
