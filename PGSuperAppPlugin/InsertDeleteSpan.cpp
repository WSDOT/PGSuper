///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2022  Washington State Department of Transportation
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
#include "InsertDeleteSpan.h"
#include "PGSuperDocBase.h"

#include <IFace\Bridge.h>
#include <IFace\Views.h>
#include <BridgeModelViewController.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

txnInsertSpan::txnInsertSpan(PierIndexType refPierIdx,pgsTypes::PierFaceType pierFace,Float64 spanLength,bool bCreateNewGroup,IndexType pierErectionEventIdx)
{
   m_RefPierIdx = refPierIdx;
   m_PierFace   = pierFace;
   m_SpanLength = spanLength;
   m_bCreateNewGroup = bCreateNewGroup;
   m_PierErectionEventIndex = pierErectionEventIdx;
}

std::_tstring txnInsertSpan::Name() const
{
   return _T("Insert Span");
}

std::unique_ptr<CEAFTransaction> txnInsertSpan::CreateClone() const
{
   return std::make_unique<txnInsertSpan>(m_RefPierIdx, m_PierFace,m_SpanLength,m_bCreateNewGroup,m_PierErectionEventIndex);
}

bool txnInsertSpan::IsUndoable() const
{
   return true;
}

bool txnInsertSpan::IsRepeatable() const
{
   return false;
}

bool txnInsertSpan::Execute()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IEvents,pEvents);
   // Exception-safe holder to keep from fireing events until we are done
   CIEventsHolder event_holder(pEvents);

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);

   // sometimes the connection information gets altered when adding a span... capture it here
   // so it can be reset on Undo
   const CPierData2* pPier = pIBridgeDesc->GetPier(m_RefPierIdx);
   for ( int i = 0; i < 2; i++ )
   {
      pgsTypes::PierFaceType face = (pgsTypes::PierFaceType)i;
      pPier->GetBearingOffset(face,&m_BrgOffset[face],&m_BrgOffsetMeasure[face],true);
      pPier->GetGirderEndDistance(face,&m_EndDist[face],&m_EndDistMeasure[face],true);
   }

   pIBridgeDesc->InsertSpan(m_RefPierIdx,m_PierFace,m_SpanLength,nullptr,nullptr,m_bCreateNewGroup,m_PierErectionEventIndex);

   return true;
}

void txnInsertSpan::Undo()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IEvents,pEvents);
   // Exception-safe holder to keep from fireing events until we are done
   CIEventsHolder event_holder(pEvents);

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);

   PierIndexType pierIdx = m_RefPierIdx + (m_PierFace == pgsTypes::Ahead ? 1 : 0);
   pgsTypes::PierFaceType pierFace = (m_PierFace == pgsTypes::Ahead ? pgsTypes::Back : pgsTypes::Ahead);
   pIBridgeDesc->DeletePier(pierIdx,pierFace);

   // restore the connection geometry
   CBridgeDescription2 bridgeDesc = *(pIBridgeDesc->GetBridgeDescription());
   CPierData2* pPier = bridgeDesc.GetPier(m_RefPierIdx);
   for ( int i = 0; i < 2; i++ )
   {
      pgsTypes::PierFaceType face = (pgsTypes::PierFaceType)i;
      pPier->SetBearingOffset(face,m_BrgOffset[face],m_BrgOffsetMeasure[face]);
      pPier->SetGirderEndDistance(face,m_EndDist[face],m_EndDistMeasure[face]);
   }
   pIBridgeDesc->SetPierByIndex(m_RefPierIdx,*pPier);
}

///////////////////////////////////////////////

txnDeleteSpan::txnDeleteSpan(PierIndexType refPierIdx,pgsTypes::PierFaceType pierFace,pgsTypes::BoundaryConditionType boundaryCondition)
{
   m_RefPierIdx = refPierIdx;
   m_PierFace   = pierFace;
   m_BoundaryCondition = boundaryCondition;
   m_bIsBoundaryPier = true;
}

txnDeleteSpan::txnDeleteSpan(PierIndexType refPierIdx, pgsTypes::PierFaceType pierFace, pgsTypes::PierSegmentConnectionType segmentConnection,EventIndexType castClosureEventIdx)
{
   m_RefPierIdx = refPierIdx;
   m_PierFace = pierFace;
   m_SegmentConnection = segmentConnection;
   m_CastClosureEventIdx = castClosureEventIdx;
   m_bIsBoundaryPier = false;
}

txnDeleteSpan::txnDeleteSpan(const txnDeleteSpan& other)
{
   m_RefPierIdx = other.m_RefPierIdx;
   m_PierFace = other.m_PierFace;
   m_BoundaryCondition = other.m_BoundaryCondition;
   m_SegmentConnection = other.m_SegmentConnection;
   m_CastClosureEventIdx = other.m_CastClosureEventIdx;
   m_bIsBoundaryPier = other.m_bIsBoundaryPier;

   m_BridgeDescription = other.m_BridgeDescription;
   m_StartGroupIdx = other.m_StartGroupIdx;
   m_EndGroupIdx = other.m_EndGroupIdx;

   m_PTData = other.m_PTData;;
}

txnDeleteSpan::~txnDeleteSpan()
{
}

std::_tstring txnDeleteSpan::Name() const
{
   return _T("Delete Span");
}

std::unique_ptr<CEAFTransaction> txnDeleteSpan::CreateClone() const
{
   return std::make_unique<txnDeleteSpan>(*this);
}

bool txnDeleteSpan::IsUndoable() const
{
   return true;
}

bool txnDeleteSpan::IsRepeatable() const
{
   return false;
}

bool txnDeleteSpan::Execute()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   GET_IFACE2(pBroker,IEvents,pEvents);
   // Exception-safe holder to keep from fireing events until we are done
   CIEventsHolder event_holder(pEvents);

   GET_IFACE2(pBroker,IViews,pViews);
   CComPtr<IBridgeModelViewController> pViewController;
   pViews->CreateBridgeModelView(&pViewController);
   pViewController->GetGroupRange(&m_StartGroupIdx,&m_EndGroupIdx);

   // save the span/pier that are going to be deleted for undo
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   m_BridgeDescription = *pBridgeDesc;

   // The geometry of linear ducts get messed up when we delete spans
   // Alter the duct geometry by removing all points beyond the end of the new girder length
   // Capture the original linear duct geomety for undo
   GET_IFACE2(pBroker,IBridge,pBridge);
   const CPierData2* pPier = pBridgeDesc->GetPier(m_RefPierIdx);
   const CSpanData2* pSpan = (m_PierFace == pgsTypes::Back ? pPier->GetPrevSpan() : pPier->GetNextSpan());
   SpanIndexType spanIdx = pSpan->GetIndex();
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(pSpan);
   GirderIndexType nGirders = pGroup->GetGirderCount();
   for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
   {
      const CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);

      CGirderKey girderKey = pGirder->GetGirderKey();
      Float64 Lg = pBridge->GetGirderLength(girderKey); // end to end length of girder
      Float64 Ls = pBridge->GetSpanLength(spanIdx,gdrIdx); // span length of girder in the span that is going to be deleted
      Float64 L = Lg - Ls; // this will be the length of the girder once the span is removed

      bool bPTChanged = false;
      CPTData ptData = *(pGirder->GetPostTensioning());
      DuctIndexType nDucts = ptData.GetDuctCount();
      for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
      {
         CDuctData* pDuct = ptData.GetDuct(ductIdx);
         if ( pDuct->DuctGeometryType == CDuctGeometry::Linear )
         {
            bPTChanged = true;
            pDuct->LinearDuctGeometry.RemovePoints(L,Lg);
         }
      }

      if ( bPTChanged )
      {
         m_PTData.insert(std::make_pair(girderKey,*pGirder->GetPostTensioning())); // capture the old PTData for undo
         pIBridgeDesc->SetPostTensioning(girderKey,ptData);
      }
   }

   pIBridgeDesc->DeletePier(m_RefPierIdx, m_PierFace);

   // index of the pier that is not deleted
   PierIndexType pierIdx = (m_PierFace == pgsTypes::Back ? m_RefPierIdx - 1 : m_RefPierIdx/* + 1 don't add one because m_RefPierIdx has already been deleted so the index is one less than normal*/);
   if (m_bIsBoundaryPier)
   {
      pIBridgeDesc->SetBoundaryCondition(pierIdx, m_BoundaryCondition);
   }
   else
   {
      PierIndexType nPiers = pBridge->GetPierCount();
      if (m_RefPierIdx == 0 || m_RefPierIdx == nPiers/*-1 don't subtract one because the pier was already deleted above*/)
      {
         // the first or last pier in the bridge is being removed... this makes the
         // the new first/last pier a boundary pier so change its boundary condition
         pIBridgeDesc->SetBoundaryCondition(pierIdx, pgsTypes::bctRoller);
      }
      else
      {
         pIBridgeDesc->SetBoundaryCondition(pierIdx, m_SegmentConnection, m_CastClosureEventIdx);
      }
   }

   return true;
}

void txnDeleteSpan::Undo()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IEvents,pEvents);
   // Exception-safe holder to keep from fireing events until we are done
   CIEventsHolder event_holder(pEvents);

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   pIBridgeDesc->SetBridgeDescription(m_BridgeDescription);

   // if we alterated PT data, restore it
   if ( 0 < m_PTData.size() )
   {
      std::map<CGirderKey,CPTData>::iterator iter(m_PTData.begin());
      std::map<CGirderKey,CPTData>::iterator end(m_PTData.end());
      for ( ; iter != end; iter++ )
      {
         pIBridgeDesc->SetPostTensioning(iter->first,iter->second);
      }
   }


   GET_IFACE2(pBroker, IViews, pViews);
   CComPtr<IBridgeModelViewController> pViewController;
   pViews->CreateBridgeModelView(&pViewController);
   pViewController->SetGroupRange(m_StartGroupIdx,m_EndGroupIdx);
}
