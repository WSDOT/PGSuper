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
#include "CopyGirder.h"
#include "PGSuperDoc.h"

#include <EAF\EAFAutoProgress.h>
#include <IFace\Bridge.h>
#include <IFace\GirderHandling.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

txnCopyGirder::txnCopyGirder(SpanGirderHashType fromHash,std::vector<SpanGirderHashType> toHash,BOOL bGirder,BOOL bTransverse,BOOL bLongitudinalRebar,BOOL bPrestress,BOOL bHandling, BOOL bMaterial, BOOL bSlabOffset)
{
   m_From               = fromHash;
   m_To                 = toHash;
   m_bGirder            = bGirder;
   m_bTransverse        = bTransverse;
   m_bLongitudinalRebar = bLongitudinalRebar;
   m_bPrestress         = bPrestress;
   m_bHandling          = bHandling;
   m_bMaterial          = bMaterial;
   m_bSlabOffset        = bSlabOffset;
}

txnCopyGirder::~txnCopyGirder()
{
}

bool txnCopyGirder::Execute()
{
   GirderIndexType from_gdr;
   SpanIndexType from_span;
   UnhashSpanGirder(m_From,&from_span,&from_gdr);
   GetGirderData(from_span,from_gdr,&m_SourceGirderData);

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IGirderData, pGirderData);
   GET_IFACE2(pBroker,IShear, pShear);
   GET_IFACE2(pBroker,ILongitudinalRebar, pLongRebar);
   GET_IFACE2(pBroker,IGirderLifting,pGirderLifting);
   GET_IFACE2(pBroker,IGirderHauling,pGirderHauling);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   GET_IFACE2(pBroker,IProgress,pProgress);
   CEAFAutoProgress ap(pProgress);

   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   GET_IFACE2(pBroker,IEvents,pEvents);
   pEvents->HoldEvents();

   CString strWarningMsg;

   ASSERT(0 < m_To.size());
   m_DestinationGirderData.clear();
   for (std::vector<SpanGirderHashType>::iterator iter = m_To.begin(); iter != m_To.end(); iter++)
   {
      SpanGirderHashType hashval = *iter;
      SpanIndexType to_span;
      GirderIndexType to_gdr;
      UnhashSpanGirder(hashval,&to_span,&to_gdr);

      CString strMsg;
      strMsg.Format(_T("Copying to Span %d Girder %s"),LABEL_SPAN(to_span),LABEL_GIRDER(to_gdr));
      pProgress->UpdateMessage(strMsg);

      txnCopyGirderData copyData;
      GetGirderData(to_span,to_gdr,&copyData);

      if (m_bGirder)
      {
         ASSERT(!pBridgeDesc->UseSameGirderForEntireBridge());
         pIBridgeDesc->SetGirderName( to_span,to_gdr, m_SourceGirderData.m_strGirderName.c_str());
      }

      // Girder data-related stuff
      CGirderData gd = copyData.m_GirderData;

      bool did_gdr_type_change = m_SourceGirderData.m_strGirderName != copyData.m_strGirderName;

      if (m_bTransverse)
      {
          gd.CopyShearDataFrom(m_SourceGirderData.m_GirderData);
      }

      // If girder type changes, we don't know if long rebar or prestressing will fit into the new section
      // Be save and copy this data from source girder. Then tell user
      if (m_bLongitudinalRebar || did_gdr_type_change)
      {
         if (did_gdr_type_change && !m_bLongitudinalRebar)
         {
            CString msg;
            msg.Format(_T("- Longitudinal rebar data was copied to Span %d Girder %s because girder type was changed.\n"),LABEL_SPAN(to_span),LABEL_GIRDER(to_gdr));
            strWarningMsg = strWarningMsg + msg;
      }

         gd.CopyShearDataFrom(m_SourceGirderData.m_GirderData);
      }

      if (m_bPrestress || did_gdr_type_change)
      {
         if (did_gdr_type_change && !m_bPrestress)
         {
            CString msg;
            msg.Format(_T("- Prestressing data was copied to Span %d Girder %s because girder type was changed.\n"),LABEL_SPAN(to_span),LABEL_GIRDER(to_gdr));
            strWarningMsg = strWarningMsg + msg;
         }

          gd.CopyPrestressingFrom(m_SourceGirderData.m_GirderData);
      }

      if (m_bMaterial)
      {
          gd.CopyMaterialFrom(m_SourceGirderData.m_GirderData);
      }

      if (m_bHandling)
      {
          gd.CopyHandlingDataFrom(m_SourceGirderData.m_GirderData);
      }

      bool worked = pGirderData->SetGirderData(gd,to_span,to_gdr);
      ASSERT(worked);

      if (m_bSlabOffset)
      {
         pIBridgeDesc->SetSlabOffset(to_span,to_gdr,m_SourceGirderData.m_SlabOffset[pgsTypes::metStart],m_SourceGirderData.m_SlabOffset[pgsTypes::metEnd]);
      }

      m_DestinationGirderData.push_back(copyData);
   }

   // Issue warning if some data not copied
   if(!strWarningMsg.IsEmpty())
   {
      ::AfxMessageBox(strWarningMsg, MB_OK | MB_ICONWARNING);
   }

   pEvents->FirePendingEvents();
   return true;
}

void txnCopyGirder::Undo()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IGirderData, pGirderData);
   GET_IFACE2(pBroker,IShear, pShear);
   GET_IFACE2(pBroker,ILongitudinalRebar, pLongRebar);
   GET_IFACE2(pBroker,IGirderLifting,pGirderLifting);
   GET_IFACE2(pBroker,IGirderHauling,pGirderHauling);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   GET_IFACE2(pBroker,IProgress,pProgress);
   CEAFAutoProgress ap(pProgress);

   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   GET_IFACE2(pBroker,IEvents,pEvents);
   pEvents->HoldEvents();

   ASSERT(0 < m_To.size());

   for (std::vector<SpanGirderHashType>::iterator iter = m_To.begin(); iter != m_To.end(); iter++)
   {
      SpanGirderHashType hashval = *iter;
      SpanIndexType to_span;
      GirderIndexType to_gdr;
      UnhashSpanGirder(hashval,&to_span,&to_gdr);


      CString strMsg;
      strMsg.Format(_T("Undoing copy for Span %d Girder %s"),LABEL_SPAN(to_span),LABEL_GIRDER(to_gdr));
      pProgress->UpdateMessage(strMsg);

      txnCopyGirderData copyData = m_DestinationGirderData[ iter - m_To.begin() ];

      if (m_bGirder)
      {
         ASSERT(!pBridgeDesc->UseSameGirderForEntireBridge());
         pIBridgeDesc->SetGirderName( to_span,to_gdr, copyData.m_strGirderName.c_str());
      }

      // All girderdata-related stuff can be done in one shot
      bool worked = pGirderData->SetGirderData(copyData.m_GirderData,to_span,to_gdr);
      ASSERT(worked);

      if (m_bSlabOffset)
      {
         pIBridgeDesc->SetSlabOffset(to_span,to_gdr,copyData.m_SlabOffset[pgsTypes::metStart],copyData.m_SlabOffset[pgsTypes::metEnd]);
      }
   }

   pEvents->FirePendingEvents();
}

txnTransaction* txnCopyGirder::CreateClone() const
{
   return new txnCopyGirder(m_From,m_To,m_bGirder,m_bTransverse,m_bLongitudinalRebar,m_bPrestress,m_bHandling,m_bMaterial,m_bSlabOffset);
}

std::_tstring txnCopyGirder::Name() const
{
   return _T("Copy Girder Properties");
}

bool txnCopyGirder::IsUndoable()
{
   return true;
}

bool txnCopyGirder::IsRepeatable()
{
   return false;
}

void txnCopyGirder::GetGirderData(SpanIndexType spanIdx,GirderIndexType gdrIdx,txnCopyGirderData* ptxnGirderData)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   // gather data about the source girder
   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IGirderData, pGirderData);
   GET_IFACE2(pBroker,IShear, pShear);
   GET_IFACE2(pBroker,ILongitudinalRebar, pLongRebar);
   GET_IFACE2(pBroker,IGirderLifting,pGirderLifting);
   GET_IFACE2(pBroker,IGirderHauling,pGirderHauling);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);

   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   ptxnGirderData->m_strGirderName = pBridgeDesc->GetSpan(spanIdx)->GetGirderTypes()->GetGirderName(gdrIdx);
   
   ptxnGirderData->m_GirderData            = pGirderData->GetGirderData(spanIdx,gdrIdx);
   ptxnGirderData->m_LongitudinalRebarData = pLongRebar->GetLongitudinalRebarData(spanIdx,gdrIdx);
   ptxnGirderData->m_ShearData             = pShear->GetShearData(spanIdx,gdrIdx);

   ptxnGirderData->m_LeftLiftPoint  = pGirderLifting->GetLeftLiftingLoopLocation(spanIdx,gdrIdx);
   ptxnGirderData->m_RightLiftPoint = pGirderLifting->GetRightLiftingLoopLocation(spanIdx,gdrIdx);

   ptxnGirderData->m_LeadingOverhang  = pGirderHauling->GetLeadingOverhang(spanIdx,gdrIdx);
   ptxnGirderData->m_TrailingOverhang = pGirderHauling->GetTrailingOverhang(spanIdx,gdrIdx);

   ptxnGirderData->m_SlabOffset[pgsTypes::metStart] = pBridge->GetSlabOffset(spanIdx,gdrIdx,pgsTypes::metStart);
   ptxnGirderData->m_SlabOffset[pgsTypes::metEnd]   = pBridge->GetSlabOffset(spanIdx,gdrIdx,pgsTypes::metEnd);
}