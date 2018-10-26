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
#include "EditGirder.h"
#include "PGSuperDoc.h"
#include <PgsExt\BridgeDescription.h>
#include <IFace\GirderHandling.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

txnEditGirder::txnEditGirder(SpanIndexType spanIdx,GirderIndexType gdrIdx,
                             bool bOldUseSameGirder, bool bNewUseSameGirder,
                             const std::_tstring& strOldGirderName, const std::_tstring& strNewGirderName,
                             const CGirderData& oldGirderData,const CGirderData& newGirderData,
                             const CShearData& oldShearData,const CShearData& newShearData,
                             const CLongitudinalRebarData& oldRebarData,const CLongitudinalRebarData& newRebarData,
                             double oldLiftingLocation,  double newLiftingLocation,
                             double oldTrailingOverhang, double newTrailingOverhang,
                             double oldLeadingOverhang,  double newLeadingOverhang,
                             pgsTypes::SlabOffsetType oldSlabOffsetType,pgsTypes::SlabOffsetType newSlabOffsetType,
                             Float64 oldSlabOffsetStart,Float64 newSlabOffsetStart,
                             Float64 oldSlabOffsetEnd, Float64 newSlabOffsetEnd)
{
   m_SpanIdx   = spanIdx;
   m_GirderIdx = gdrIdx;

   m_bUseSameGirder[0] = bOldUseSameGirder;
   m_bUseSameGirder[1] = bNewUseSameGirder;

   m_strGirderName[0] = strOldGirderName;
   m_strGirderName[1] = strNewGirderName;

   m_GirderData[0] = oldGirderData;
   m_GirderData[1] = newGirderData;

   m_ShearData[0] = oldShearData;
   m_ShearData[1] = newShearData;

   m_RebarData[0] = oldRebarData;
   m_RebarData[1] = newRebarData;

   m_LiftingLocation[0] = oldLiftingLocation;
   m_LiftingLocation[1] = newLiftingLocation;

   m_TrailingOverhang[0] = oldTrailingOverhang;
   m_TrailingOverhang[1] = newTrailingOverhang;

   m_LeadingOverhang[0] = oldLeadingOverhang;
   m_LeadingOverhang[1] = newLeadingOverhang;

   m_SlabOffsetType[0] = oldSlabOffsetType;
   m_SlabOffsetType[1] = newSlabOffsetType;

   m_SlabOffset[0][pgsTypes::metStart] = oldSlabOffsetStart;
   m_SlabOffset[1][pgsTypes::metStart] = newSlabOffsetStart;

   m_SlabOffset[0][pgsTypes::metEnd] = oldSlabOffsetEnd;
   m_SlabOffset[1][pgsTypes::metEnd] = newSlabOffsetEnd;
}

txnEditGirder::~txnEditGirder()
{
}

bool txnEditGirder::Execute()
{
   DoExecute(1);
   return true;
}

void txnEditGirder::Undo()
{
   DoExecute(0);
}

txnTransaction* txnEditGirder::CreateClone() const
{
   return new txnEditGirder(m_SpanIdx, m_GirderIdx,
                            m_bUseSameGirder[0],   m_bUseSameGirder[1],
                            m_strGirderName[0],    m_strGirderName[1],
                            m_GirderData[0],       m_GirderData[1],
                            m_ShearData[0],        m_ShearData[1],
                            m_RebarData[0],        m_RebarData[1],
                            m_LiftingLocation[0],  m_LiftingLocation[1],
                            m_TrailingOverhang[0], m_TrailingOverhang[1],
                            m_LeadingOverhang[0],  m_LeadingOverhang[1],
                            m_SlabOffsetType[0],   m_SlabOffsetType[1],
                            m_SlabOffset[0][pgsTypes::metStart],m_SlabOffset[1][pgsTypes::metStart],
                            m_SlabOffset[0][pgsTypes::metEnd],  m_SlabOffset[1][pgsTypes::metEnd]
                            );
}

std::_tstring txnEditGirder::Name() const
{
   std::_tostringstream os;
   os << "Edit Span " << LABEL_SPAN(m_SpanIdx) << " Girder " << LABEL_GIRDER(m_GirderIdx);
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

void txnEditGirder::DoExecute(int i)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IEvents, pEvents);
   pEvents->HoldEvents(); // don't fire any changed events until all changes are done

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);

   pIBridgeDesc->UseSameGirderForEntireBridge( m_bUseSameGirder[i] );

   // set the slab offset
   if ( m_SlabOffsetType[i] == pgsTypes::sotBridge )
   {
      // for the entire bridge
      pIBridgeDesc->SetSlabOffset( m_SlabOffset[i][pgsTypes::metStart] );
   }
   else if ( m_SlabOffsetType[i] == pgsTypes::sotSpan )
   {
      // for this span
      pIBridgeDesc->SetSlabOffset(m_SpanIdx,m_SlabOffset[i][pgsTypes::metStart], m_SlabOffset[i][pgsTypes::metEnd] );
   }
   else
   {
      // by girder
      if ( i == 1 && m_SlabOffsetType[0] != m_SlabOffsetType[1] )
      {
         // we are changing the slab offset type from something to "by girder"

         // need to make sure the "by girder" values are match the current slab offset for the girder
         // the current value is the value for this girder

         // get the current value of the slab offset
         Float64 start, end;
         pIBridgeDesc->GetSlabOffset(m_SpanIdx,m_GirderIdx,&start,&end);

         // set the value for each girder to this current value
         GirderIndexType nGirders = pIBridgeDesc->GetSpan(m_SpanIdx)->GetGirderCount();
         for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
            pIBridgeDesc->SetSlabOffset( m_SpanIdx, gdrIdx, start, end );
      }
      
      // change the girder that was edited
      pIBridgeDesc->SetSlabOffset( m_SpanIdx, m_GirderIdx, m_SlabOffset[i][pgsTypes::metStart], m_SlabOffset[i][pgsTypes::metEnd] );
   }

   if ( !m_bUseSameGirder[i] )
      pIBridgeDesc->SetGirderName( m_SpanIdx, m_GirderIdx, m_strGirderName[i].c_str() );
   else
      pIBridgeDesc->SetGirderName( m_strGirderName[i].c_str() );

   // Prestress Data
   GET_IFACE2(pBroker,IGirderData,pGirderData);
   pGirderData->SetGirderData( m_GirderData[i], m_SpanIdx, m_GirderIdx );

   // Shear Data
   GET_IFACE2(pBroker,IShear,pShear);
   pShear->SetShearData( m_ShearData[i], m_SpanIdx, m_GirderIdx );

   // Longitudinal Rebar Data
   GET_IFACE2(pBroker,ILongitudinalRebar,pLongitudinalRebar);
   pLongitudinalRebar->SetLongitudinalRebarData( m_RebarData[i], m_SpanIdx, m_GirderIdx );


   // lifting location
   GET_IFACE2(pBroker,IGirderLifting,pGirderLifting);
   pGirderLifting->SetLiftingLoopLocations( m_SpanIdx, m_GirderIdx, m_LiftingLocation[i], m_LiftingLocation[i]);

   // truck support location
   GET_IFACE2(pBroker,IGirderHauling,pGirderHauling);
   pGirderHauling->SetTruckSupportLocations( m_SpanIdx, m_GirderIdx, m_TrailingOverhang[i], m_LeadingOverhang[i]);

   pEvents->FirePendingEvents();
}
