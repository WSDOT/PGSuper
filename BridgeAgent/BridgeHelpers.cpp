///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2015  Washington State Department of Transportation
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
#include "BridgeHelpers.h"

#include <EAF\EAFUtilities.h>
#include <IFace\Bridge.h>
#include <IFace\Project.h>

#include <PgsExt\BridgeDescription2.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define TEMPORARY_SUPPORT_ID_OFFSET 10000

HRESULT GetSuperstructureMember(IGenericBridge* pBridge,const CGirderKey& girderKey,ISuperstructureMember* *ssmbr)
{
   GirderIDType gdrID = ::GetSuperstructureMemberID(girderKey.groupIndex,girderKey.girderIndex);
   return pBridge->get_SuperstructureMember(gdrID,ssmbr);
}

HRESULT GetSegment(IGenericBridge* pBridge,const CSegmentKey& segmentKey,ISuperstructureMemberSegment** segment)
{
   CComPtr<ISuperstructureMember> ssmbr;
   GetSuperstructureMember(pBridge,segmentKey,&ssmbr);

   return ssmbr->get_Segment(segmentKey.segmentIndex,segment);
}

HRESULT GetGirder(IGenericBridge* pBridge,const CSegmentKey& segmentKey,IPrecastGirder** girder)
{
   CComPtr<ISuperstructureMemberSegment> segment;
   ::GetSegment(pBridge,segmentKey,&segment);

   CComQIPtr<IItemData> itemdata(segment);
   CComPtr<IUnknown> punk;
   itemdata->GetItemData(CComBSTR("Precast Girder"),&punk);
   CComQIPtr<IPrecastGirder> gdr(punk);

   (*girder) = gdr;
   (*girder)->AddRef();

   return S_OK;
}

PierIDType GetPierLineID(PierIndexType pierIdx)
{
   return pierIdx;
}

PierIndexType GetPierIndex(PierIDType pierLineID)
{
   return pierLineID;
}

SupportIDType GetTempSupportLineID(SupportIndexType tsIdx)
{
   return -(SupportIDType)(tsIdx + TEMPORARY_SUPPORT_ID_OFFSET);
}

SupportIndexType GetTempSupportIndex(SupportIDType tsLineID)
{
   return (SupportIndexType)(-tsLineID - TEMPORARY_SUPPORT_ID_OFFSET);
}

LineIDType GetGirderSegmentLineID(GroupIndexType grpIdx,GirderIndexType gdrIdx,SegmentIndexType segIdx)
{
   ATLASSERT( grpIdx < Int16_Max && gdrIdx < Int8_Max && segIdx < Int8_Max );
   Int16 id = ::make_Int16((Int8)gdrIdx,(Int8)segIdx);
   return ::make_Int32((Int16)grpIdx,(Int16)id);
}

LineIDType GetGirderSegmentLineID(const CSegmentKey& segmentKey)
{
   return GetGirderSegmentLineID(segmentKey.groupIndex,segmentKey.girderIndex,segmentKey.segmentIndex);
}

LineIDType GetGirderLineID(const CSpanKey& spanKey)
{
   ATLASSERT( spanKey.spanIndex < Int16_Max && spanKey.girderIndex < Int16_Max );
   return ::make_Int32((Int16)spanKey.girderIndex,(Int16)spanKey.spanIndex);
}

GirderIDType GetSuperstructureMemberID(GroupIndexType grpIdx,GirderIndexType gdrIdx)
{
   ATLASSERT(grpIdx != INVALID_INDEX);
   ATLASSERT(gdrIdx != INVALID_INDEX);

   return ::GetGirderLineID(CSpanKey(grpIdx,gdrIdx));
}

CSegmentKey GetSegmentKey(GirderIDType gdrID)
{
   GroupIndexType grpIdx  = (GroupIndexType)high_Int16((Int32)gdrID);
   GirderIndexType gdrIdx = (GirderIndexType)low_Int16((Int32)gdrID);

   CSegmentKey segmentKey(grpIdx,gdrIdx,0);
   return segmentKey;
}

void GetSuperstructureMemberIDs(GroupIndexType grpIdx,GirderIndexType gdrIdx,GirderIDType* pLeftID,GirderIDType* pThisID,GirderIDType* pRightID)
{
   *pLeftID = (gdrIdx == 0 ? INVALID_ID : GetSuperstructureMemberID(grpIdx,gdrIdx-1));
   *pThisID = GetSuperstructureMemberID(grpIdx,gdrIdx);

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IBridge,pBridge);

   GirderIndexType nGirders = pBridge->GetGirderCount(grpIdx);
   *pRightID = (gdrIdx == nGirders-1 ? INVALID_ID : GetSuperstructureMemberID(grpIdx,gdrIdx+1));
}

void GetAdjacentSuperstructureMemberIDs(const CSegmentKey& segmentKey,GirderIDType* pLeftID,GirderIDType* pThisID,GirderIDType* pRightID)
{
   GroupIndexType grpIdx = segmentKey.groupIndex;
   GirderIndexType gdrIdx = segmentKey.girderIndex;

   *pLeftID = (gdrIdx == 0 ? INVALID_ID : GetSuperstructureMemberID(grpIdx,gdrIdx-1));

   *pThisID = GetSuperstructureMemberID(grpIdx,gdrIdx);

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IBridgeDescription,pBridgeDesc);

   GirderIndexType nGirders = pBridgeDesc->GetBridgeDescription()->GetGirderGroup(grpIdx)->GetGirderCount();
   *pRightID = (gdrIdx == nGirders-1 ? INVALID_ID : GetSuperstructureMemberID(grpIdx,gdrIdx+1));
}

void GetAdjacentGirderKeys(const CSegmentKey& segmentKey,CSegmentKey* pLeftKey,CSegmentKey* pRightKey)
{
   GroupIndexType grpIdx = segmentKey.groupIndex;
   GirderIndexType gdrIdx = segmentKey.girderIndex;

   if ( gdrIdx == 0 )
   {
      pLeftKey->groupIndex = INVALID_INDEX;
      pLeftKey->girderIndex = INVALID_INDEX;
   }
   else
   {
      pLeftKey->groupIndex = grpIdx;
      pLeftKey->girderIndex = gdrIdx-1;
   }
   pLeftKey->segmentIndex = segmentKey.segmentIndex;;

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IBridgeDescription,pBridgeDesc);

   GirderIndexType nGirders = pBridgeDesc->GetBridgeDescription()->GetGirderCount();
   if ( gdrIdx == nGirders-1 )
   {
      pRightKey->groupIndex = INVALID_INDEX;
      pRightKey->girderIndex = INVALID_INDEX;
   }
   else
   {
      pRightKey->groupIndex = grpIdx;
      pRightKey->girderIndex = gdrIdx+1;
   }
   pRightKey->segmentIndex = segmentKey.segmentIndex;;
}
