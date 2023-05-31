///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2023  Washington State Department of Transportation
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

// BridgePlanView.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "PGSuperApp.h"
#include "PGSuperDocBase.h"
#include "PGSuperUnits.h"
#include <IFace\DrawBridgeSettings.h>
#include "BridgePlanView.h"
#include "PGSuperColors.h"
#include "AlignmentDisplayObjectEvents.h"
#include "InplaceSpanLengthEditEvents.h"
#include "InplacePierStationEditEvents.h"
#include "InplaceTemporarySupportStationEditEvents.h"
#include "GirderDisplayObjectEvents.h"
#include "PierDisplayObjectEvents.h"
#include "TemporarySupportDisplayObjectEvents.h"
#include "ClosureJointDisplayObjectEvents.h"
#include "ConnectionDisplayObjectEvents.h"
#include "DisplayObjectFactory.h"
#include "BridgeSectionCutDisplayImpl.h"
#include "SlabDisplayObjectEvents.h"
#include "SpanDisplayObjectEvents.h"
#include "BridgeSectionView.h"

#include <IFace\Alignment.h>
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\EditByUI.h>
#include <IFace\DocumentType.h>

#include <PgsExt\BridgeDescription2.h>
#include <WBFLDManip.h>
#include <WBFLDManipTools.h>
#include <DManipTools\DManipTools.h>
#include <Units\Convert.h>

#include <Materials/Materials.h>

#include <EAF\EAFMenu.h>

#include <PgsExt\ClosureJointData.h>
#include <PgsExt\Helpers.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define TITLE_DISPLAY_LIST       0
#define ALIGNMENT_DISPLAY_LIST   1
#define PIER_DISPLAY_LIST        2
#define SEGMENT_DISPLAY_LIST     3
#define JOINT_DISPLAY_LIST       4
#define GIRDER_DISPLAY_LIST      5
#define BEARING_DISPLAY_LIST     6
#define SPAN_DISPLAY_LIST        7
#define SLAB_DISPLAY_LIST        8
#define LABEL_DISPLAY_LIST       9
#define SECTION_CUT_DISPLAY_LIST 10
#define NORTH_ARROW_DISPLAY_LIST 11
#define DIAPHRAGM_DISPLAY_LIST   12
#define TEMPORARY_SUPPORT_DISPLAY_LIST 13
#define CLOSURE_JOINT_DISPLAY_LIST 14

#define SECTION_CUT_ID -200
#define ALIGNMENT_ID   -300

//#define _SHOW_CL_GIRDER

/////////////////////////////////////////////////////////////////////////////
// CBridgePlanView

IMPLEMENT_DYNCREATE(CBridgePlanView, CBridgeViewPane)

CBridgePlanView::CBridgePlanView()
{
   m_StartGroupIdx = 0;
   m_EndGroupIdx  = ALL_GROUPS;
}

CBridgePlanView::~CBridgePlanView()
{
}


BEGIN_MESSAGE_MAP(CBridgePlanView, CBridgeViewPane)
	//{{AFX_MSG_MAP(CBridgePlanView)
	ON_COMMAND(ID_EDIT_DECK, OnEditDeck)
	ON_COMMAND(ID_VIEWSETTINGS, OnViewSettings)
   ON_WM_KEYDOWN()
   ON_WM_MOUSEWHEEL()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CBridgePlanView drawing
bool CBridgePlanView::IsDeckSelected()
{
   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   DisplayObjectContainer displayObjects;
   dispMgr->GetSelectedObjects(&displayObjects);

   ATLASSERT(displayObjects.size() == 0 || displayObjects.size() == 1 );

   if ( displayObjects.size() == 0 )
   {
      return false;
   }

   CComPtr<iDisplayObject> pDO = displayObjects.front().m_T;

   DeckDisplayObjectInfo* pInfo = nullptr;
   pDO->GetItemData((void**)&pInfo);
   if ( pInfo == nullptr || pInfo->ID != DECK_ID || pInfo->DisplayListID != SLAB_DISPLAY_LIST )
   {
      return false;
   }

   return true;
}

bool CBridgePlanView::IsAlignmentSelected()
{
   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   DisplayObjectContainer displayObjects;
   dispMgr->GetSelectedObjects(&displayObjects);

   ATLASSERT(displayObjects.size() == 0 || displayObjects.size() == 1 );

   if ( displayObjects.size() == 0 )
   {
      return false;
   }

   CComPtr<iDisplayObject> pDO = displayObjects.front().m_T;

   if ( pDO->GetID() == ALIGNMENT_ID )
   {
      return true;
   }

   return false;
}

void CBridgePlanView::GetGroupRange(GroupIndexType* pStartGroupIdx,GroupIndexType* pEndGroupIdx)
{
   *pStartGroupIdx = m_StartGroupIdx;
   *pEndGroupIdx   = m_EndGroupIdx;
}

void CBridgePlanView::SetGroupRange(GroupIndexType startGroupIdx,GroupIndexType endGroupIdx,bool bUpdate)
{
   m_StartGroupIdx = startGroupIdx;
   m_EndGroupIdx  = endGroupIdx;

   if ( bUpdate )
   {
      m_pDocument->UpdateAllViews(nullptr,HINT_BRIDGEVIEWSETTINGSCHANGED,nullptr);
      m_pDocument->UpdateAllViews(nullptr,HINT_BRIDGEVIEWSECTIONCUTCHANGED,nullptr);
   }
}

bool CBridgePlanView::GetSelectedSpan(SpanIndexType* pSpanIdx)
{
   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   DisplayObjectContainer displayObjects;
   dispMgr->GetSelectedObjects(&displayObjects);

   ATLASSERT(displayObjects.size() == 0 || displayObjects.size() == 1 );

   if ( displayObjects.size() == 0 )
   {
      return false;
   }

   CComPtr<iDisplayObject> pDO = displayObjects.front().m_T;

   SpanDisplayObjectInfo* pInfo;
   pDO->GetItemData((void**)&pInfo);
   if ( pInfo == nullptr || pInfo->SpanIdx == ALL_SPANS || pInfo->DisplayListID != SPAN_DISPLAY_LIST )
   {
      return false;
   }

   *pSpanIdx = pInfo->SpanIdx;

   return true;
}

bool CBridgePlanView::GetSelectedPier(PierIndexType* pPierIdx)
{
   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   DisplayObjectContainer displayObjects;
   dispMgr->GetSelectedObjects(&displayObjects);

   ATLASSERT(displayObjects.size() == 0 || displayObjects.size() == 1 );

   if ( displayObjects.size() == 0 )
   {
      return false;
   }

   CComPtr<iDisplayObject> pDO = displayObjects.front().m_T;

   PierDisplayObjectInfo* pInfo;
   pDO->GetItemData((void**)&pInfo);

   if ( pInfo == nullptr || pInfo->DisplayListID != PIER_DISPLAY_LIST )
   {
      return false;
   }

   *pPierIdx = pInfo->PierIdx;

   return true;
}

void CBridgePlanView::SelectSpan(SpanIndexType spanIdx,bool bSelect)
{
   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   CComPtr<iDisplayObject> pDO;
   dispMgr->FindDisplayObject(spanIdx,SPAN_DISPLAY_LIST,atByID,&pDO);

   if ( pDO )
   {
      dispMgr->SelectObject(pDO,bSelect);
   }
   else
   {
      dispMgr->ClearSelectedObjects();
   }
}

void CBridgePlanView::SelectPier(PierIndexType pierIdx,bool bSelect)
{
   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   CComPtr<iDisplayObject> pDO;
   dispMgr->FindDisplayObject(pierIdx,PIER_DISPLAY_LIST,atByID,&pDO);

   if ( pDO )
   {
      dispMgr->SelectObject(pDO,bSelect);
   }
   else
   {
      dispMgr->ClearSelectedObjects();
   }
}

bool CBridgePlanView::GetSelectedGirder(CGirderKey* pGirderKey)
{
   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   DisplayObjectContainer displayObjects;
   dispMgr->GetSelectedObjects(&displayObjects);

   ATLASSERT(displayObjects.size() == 0 || displayObjects.size() == 1 );

   if ( displayObjects.size() == 0 )
   {
      return false;
   }

   CComPtr<iDisplayObject> pDO = displayObjects.front().m_T;

   GirderDisplayObjectInfo* pInfo = nullptr;
   pDO->GetItemData((void**)&pInfo);

   if ( pInfo == nullptr || pInfo->DisplayListID != GIRDER_DISPLAY_LIST )
   {
      return false;
   }

   *pGirderKey = pInfo->m_GirderKey;
   return true;
}

void CBridgePlanView::SelectGirder(const CGirderKey& girderKey,bool bSelect)
{
   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   std::map<CGirderKey,IDType>::iterator found = m_GirderIDs.find(girderKey);
   if ( found == m_GirderIDs.end() )
   {
      dispMgr->ClearSelectedObjects();
      return;
   }

   IDType ID = (*found).second;

   CComPtr<iDisplayObject> pDO;
   dispMgr->FindDisplayObject(ID,GIRDER_DISPLAY_LIST,atByID,&pDO);

   if ( pDO )
   {
      dispMgr->SelectObject(pDO,bSelect);
   }
   else
   {
      dispMgr->ClearSelectedObjects();
   }
}

bool CBridgePlanView::GetSelectedSegment(CSegmentKey* pSegmentKey)
{
   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   DisplayObjectContainer displayObjects;
   dispMgr->GetSelectedObjects(&displayObjects);

   ATLASSERT(displayObjects.size() == 0 || displayObjects.size() == 1 );

   if ( displayObjects.size() == 0 )
   {
      return false;
   }

   CComPtr<iDisplayObject> pDO = displayObjects.front().m_T;

   SegmentDisplayObjectInfo* pInfo = nullptr;
   pDO->GetItemData((void**)&pInfo);

   if ( pInfo == nullptr || pInfo->DisplayListID != SEGMENT_DISPLAY_LIST )
   {
      return false;
   }

   *pSegmentKey = pInfo->m_SegmentKey;

   return true;
}

void CBridgePlanView::SelectSegment(const CSegmentKey& segmentKey,bool bSelect)
{
   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   std::map<CSegmentKey,IDType>::iterator found = m_SegmentIDs.find(segmentKey);
   if ( found == m_SegmentIDs.end() )
   {
      dispMgr->ClearSelectedObjects();
      return;
   }

   IDType ID = (*found).second;

   CComPtr<iDisplayObject> pDO;
   dispMgr->FindDisplayObject(ID,SEGMENT_DISPLAY_LIST,atByID,&pDO);

   if ( pDO )
   {
      dispMgr->SelectObject(pDO,bSelect);
   }
   else
   {
      dispMgr->ClearSelectedObjects();
   }
}

bool CBridgePlanView::GetSelectedClosureJoint(CSegmentKey* pClosureKey)
{
   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   DisplayObjectContainer displayObjects;
   dispMgr->GetSelectedObjects(&displayObjects);

   ATLASSERT(displayObjects.size() == 0 || displayObjects.size() == 1 );

   if ( displayObjects.size() == 0 )
   {
      return false;
   }

   CComPtr<iDisplayObject> pDO = displayObjects.front().m_T;

   SegmentDisplayObjectInfo* pInfo = nullptr;
   pDO->GetItemData((void**)&pInfo);

   if ( pInfo == nullptr || pInfo->DisplayListID != CLOSURE_JOINT_DISPLAY_LIST )
   {
      return false;
   }

   *pClosureKey = pInfo->m_SegmentKey;

   return true;
}

void CBridgePlanView::SelectClosureJoint(const CSegmentKey& closureKey,bool bSelect)
{
   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   std::map<CSegmentKey,IDType>::iterator found = m_ClosureJointIDs.find(closureKey);
   if ( found == m_ClosureJointIDs.end() )
   {
      dispMgr->ClearSelectedObjects();
      return;
   }

   IDType ID = (*found).second;

   CComPtr<iDisplayObject> pDO;
   dispMgr->FindDisplayObject(ID,CLOSURE_JOINT_DISPLAY_LIST,atByID,&pDO);

   if ( pDO )
   {
      dispMgr->SelectObject(pDO,bSelect);
   }
   else
   {
      dispMgr->ClearSelectedObjects();
   }
}

void CBridgePlanView::SelectDeck(bool bSelect)
{
   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   CComPtr<iDisplayObject> pDO;
   dispMgr->FindDisplayObject(DECK_ID,SLAB_DISPLAY_LIST,atByID,&pDO);

   if ( pDO )
   {
      dispMgr->SelectObject(pDO,bSelect);
   }
   else
   {
      dispMgr->ClearSelectedObjects();
   }
}

void CBridgePlanView::SelectAlignment(bool bSelect)
{
   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   CComPtr<iDisplayObject> pDO;
   dispMgr->FindDisplayObject(ALIGNMENT_ID,ALIGNMENT_DISPLAY_LIST,atByID,&pDO);

   if ( pDO )
   {
      dispMgr->SelectObject(pDO,bSelect);
   }
   else
   {
      dispMgr->ClearSelectedObjects();
   }
}

bool CBridgePlanView::GetSelectedTemporarySupport(SupportIndexType* ptsIdx)
{
   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   DisplayObjectContainer displayObjects;
   dispMgr->GetSelectedObjects(&displayObjects);

   ATLASSERT(displayObjects.size() == 0 || displayObjects.size() == 1);

   if (displayObjects.size() == 0)
   {
      return false;
   }

   CComPtr<iDisplayObject> pDO = displayObjects.front().m_T;

   TemporarySupportDisplayObjectInfo* pInfo;
   pDO->GetItemData((void**)&pInfo);

   if (pInfo == nullptr || pInfo->DisplayListID != TEMPORARY_SUPPORT_DISPLAY_LIST)
   {
      return false;
   }

   *ptsIdx = pInfo->tsIdx;

   return true;
}

void CBridgePlanView::SelectTemporarySupport(SupportIDType tsID,bool bSelect)
{
   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   CComPtr<iDisplayObject> pDO;
   dispMgr->FindDisplayObject(tsID,TEMPORARY_SUPPORT_DISPLAY_LIST,atByID,&pDO);

   if ( pDO )
   {
      dispMgr->SelectObject(pDO,bSelect);
   }
   else
   {
      dispMgr->ClearSelectedObjects();
   }
}

void CBridgePlanView::OnInitialUpdate() 
{
   EnableToolTips();
   CBridgeViewPane::OnInitialUpdate();

   // Causes the child frame window to initalize the span range selection controls
   m_pFrame->InitGroupRange();
}

void CBridgePlanView::BuildDisplayLists()
{
   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   CPGSDocBase* pDoc = (CPGSDocBase*)GetDocument();
   CDisplayObjectFactory* factory = new CDisplayObjectFactory(pDoc);
   CComPtr<iDisplayObjectFactory> doFactory;
   doFactory.Attach((iDisplayObjectFactory*)factory->GetInterface(&IID_iDisplayObjectFactory));
   dispMgr->AddDisplayObjectFactory(doFactory);

   dispMgr->EnableLBtnSelect(TRUE);
   dispMgr->EnableRBtnSelect(TRUE);
   dispMgr->SetSelectionLineColor(SELECTED_OBJECT_LINE_COLOR);
   dispMgr->SetSelectionFillColor(SELECTED_OBJECT_FILL_COLOR);

   CBridgeViewPane::SetMappingMode(DManip::Isotropic);

   // Setup display lists

   // section cut - add first so it is always on top
   CComPtr<iDisplayList> section_cut_list;
   ::CoCreateInstance(CLSID_DisplayList,nullptr,CLSCTX_ALL,IID_iDisplayList,(void**)&section_cut_list);
   section_cut_list->SetID(SECTION_CUT_DISPLAY_LIST);
   dispMgr->AddDisplayList(section_cut_list);

   CComPtr<iDisplayList> label_list;
   ::CoCreateInstance(CLSID_DisplayList,nullptr,CLSCTX_ALL,IID_iDisplayList,(void**)&label_list);
   label_list->SetID(LABEL_DISPLAY_LIST);
   dispMgr->AddDisplayList(label_list);

   CComPtr<iDisplayList> title_list;
   ::CoCreateInstance(CLSID_DisplayList,nullptr,CLSCTX_ALL,IID_iDisplayList,(void**)&title_list);
   title_list->SetID(TITLE_DISPLAY_LIST);
   dispMgr->AddDisplayList(title_list);

   CComPtr<iDisplayList> alignment_list;
   ::CoCreateInstance(CLSID_DisplayList,nullptr,CLSCTX_ALL,IID_iDisplayList,(void**)&alignment_list);
   alignment_list->SetID(ALIGNMENT_DISPLAY_LIST);
   dispMgr->AddDisplayList(alignment_list);

   CComPtr<iDisplayList> segment_list;
   ::CoCreateInstance(CLSID_DisplayList,nullptr,CLSCTX_ALL,IID_iDisplayList,(void**)&segment_list);
   segment_list->SetID(SEGMENT_DISPLAY_LIST);
   dispMgr->AddDisplayList(segment_list);

   CComPtr<iDisplayList> joint_list;
   ::CoCreateInstance(CLSID_DisplayList, nullptr, CLSCTX_ALL, IID_iDisplayList, (void**)&joint_list);
   joint_list->SetID(JOINT_DISPLAY_LIST);
   dispMgr->AddDisplayList(joint_list);

   CComPtr<iDisplayList> girder_list;
   ::CoCreateInstance(CLSID_DisplayList,nullptr,CLSCTX_ALL,IID_iDisplayList,(void**)&girder_list);
   girder_list->SetID(GIRDER_DISPLAY_LIST);
   dispMgr->AddDisplayList(girder_list);

   CComPtr<iDisplayList> diaphragm_list;
   ::CoCreateInstance(CLSID_DisplayList,nullptr,CLSCTX_ALL,IID_iDisplayList,(void**)&diaphragm_list);
   diaphragm_list->SetID(DIAPHRAGM_DISPLAY_LIST);
   dispMgr->AddDisplayList(diaphragm_list);

   CComPtr<iDisplayList> closure_joint_list;
   ::CoCreateInstance(CLSID_DisplayList, nullptr, CLSCTX_ALL, IID_iDisplayList, (void**)&closure_joint_list);
   closure_joint_list->SetID(CLOSURE_JOINT_DISPLAY_LIST);
   dispMgr->AddDisplayList(closure_joint_list);

   CComPtr<iDisplayList> pier_list;
   ::CoCreateInstance(CLSID_DisplayList,nullptr,CLSCTX_ALL,IID_iDisplayList,(void**)&pier_list);
   pier_list->SetID(PIER_DISPLAY_LIST);
   dispMgr->AddDisplayList(pier_list);

   CComPtr<iDisplayList> temporary_support_list;
   ::CoCreateInstance(CLSID_DisplayList,nullptr,CLSCTX_ALL,IID_iDisplayList,(void**)&temporary_support_list);
   temporary_support_list->SetID(TEMPORARY_SUPPORT_DISPLAY_LIST);
   dispMgr->AddDisplayList(temporary_support_list);

   CComPtr<iDisplayList> bearing_list;
   ::CoCreateInstance(CLSID_DisplayList,nullptr,CLSCTX_ALL,IID_iDisplayList,(void**)&bearing_list);
   bearing_list->SetID(BEARING_DISPLAY_LIST);
   dispMgr->AddDisplayList(bearing_list);

   CComPtr<iDisplayList> slab_list;
   ::CoCreateInstance(CLSID_DisplayList, nullptr, CLSCTX_ALL, IID_iDisplayList, (void**)&slab_list);
   slab_list->SetID(SLAB_DISPLAY_LIST);
   dispMgr->AddDisplayList(slab_list);

   CComPtr<iDisplayList> span_list;
   ::CoCreateInstance(CLSID_DisplayList,nullptr,CLSCTX_ALL,IID_iDisplayList,(void**)&span_list);
   span_list->SetID(SPAN_DISPLAY_LIST);
   dispMgr->AddDisplayList(span_list);

   CComPtr<iDisplayList> north_arrow_list;
   ::CoCreateInstance(CLSID_DisplayList,nullptr,CLSCTX_ALL,IID_iDisplayList,(void**)&north_arrow_list);
   north_arrow_list->SetID(NORTH_ARROW_DISPLAY_LIST);
   dispMgr->AddDisplayList(north_arrow_list);

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IBridge,pBridge);
   GroupIndexType nGroups = pBridge->GetGirderGroupCount();
   m_EndGroupIdx = nGroups - 1;

   // OnInitialUpdate eventually calls OnUpdate... OnUpdate calls the
   // following two methods so there isn't any need to call them here
   //UpdateDisplayObjects();
   //UpdateDrawingScale();
}

/////////////////////////////////////////////////////////////////////////////
// CBridgePlanView diagnostics

#ifdef _DEBUG
void CBridgePlanView::AssertValid() const
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
	CBridgeViewPane::AssertValid();
}

void CBridgePlanView::Dump(CDumpContext& dc) const
{
	CBridgeViewPane::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CBridgePlanView message handlers

void CBridgePlanView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
   CBridgeViewPane::OnUpdate(pSender,lHint,pHint);

	if ( (lHint == 0)                               || 
        (lHint == HINT_BRIDGECHANGED)              || 
        (lHint == HINT_GIRDERFAMILYCHANGED)        ||
        (lHint == HINT_UNITSCHANGED)               ||  
        (lHint == HINT_BRIDGEVIEWSETTINGSCHANGED)  ||
        (lHint == HINT_GIRDERLABELFORMATCHANGED)   ||
        (lHint == HINT_GIRDERCHANGED)
      )
   {
      CComPtr<IBroker> pBroker;
      EAFGetBroker(&pBroker);
      GET_IFACE2(pBroker,IBridge,pBridge);

      if ( lHint == HINT_BRIDGECHANGED )
      {
         if ( pHint )
         {
            // The span configuration of the bridge changed
            CBridgeHint* pBridgeHint = (CBridgeHint*)pHint;

            // We want to know if the span that was added or removed
            // is within the range of spans being displayed. If it is,
            // adjust the display range.
            SpanIndexType nSpans = pBridge->GetSpanCount();
            SpanIndexType nPrevSpans = nSpans + (pBridgeHint->bAdded ? -1 : 1);


            SpanIndexType spanIdx = pBridgeHint->PierIdx + (pBridgeHint->PierFace == pgsTypes::Back ? -1 : 0);
            if ( (m_StartGroupIdx <= spanIdx && spanIdx <= m_EndGroupIdx) || // span in range
                 (m_EndGroupIdx == nPrevSpans-1 && spanIdx == nSpans-1) || // at end
                 (m_StartGroupIdx == 0 && spanIdx == INVALID_INDEX) // at start
               )
            {
               // new span is in the display range
               if ( pBridgeHint->bAdded )
               {
                  m_EndGroupIdx++;
               }
               else
               {
                  if ( spanIdx == m_StartGroupIdx && spanIdx != 0)
                  {
                     m_StartGroupIdx++; // span at start of range was removed, so make the range smaller
                  }
                  else
                  {
                     m_EndGroupIdx--; // span at within or at the end of the range was removed...
                  }
               }
            }
         }
      }

      // Make sure we aren't displaying spans past the end of the bridge
      GroupIndexType nGroups = pBridge->GetGirderGroupCount();
      m_EndGroupIdx = (nGroups <= m_EndGroupIdx ? nGroups -1 : m_EndGroupIdx);

      if (m_EndGroupIdx < m_StartGroupIdx)
      {
         m_StartGroupIdx = 0;
      }

      //// Make sure we aren't splitting a group
      //GroupIndexType startGroupIdx = pBridge->GetGirderGroupIndex(m_StartGroupIdx);
      //GroupIndexType startSpanIdx, endSpanIdx;
      //pBridge->GetGirderGroupSpans(startGroupIdx, &startSpanIdx, &endSpanIdx);
      //if (startSpanIdx != m_StartGroupIdx)
      //{
      //   m_StartGroupIdx = startSpanIdx;
      //}

      //GroupIndexType endGroupIdx = pBridge->GetGirderGroupIndex(m_EndGroupIdx);
      //pBridge->GetGirderGroupSpans(endGroupIdx, &startSpanIdx, &endSpanIdx);
      //if (endSpanIdx != m_EndGroupIdx)
      //{
      //   m_EndGroupIdx = endSpanIdx;
      //}


      if ( lHint != HINT_BRIDGEVIEWSETTINGSCHANGED )
      {
         m_pFrame->InitGroupRange();
      }

      UpdateDisplayObjects();
      UpdateDrawingScale();
   }
   else if ( lHint == HINT_BRIDGEVIEWSECTIONCUTCHANGED )
   {
      UpdateSectionCut();
   }
   else if ( lHint == HINT_SELECTIONCHANGED )
   {
      CSelection* pSelection = (CSelection*)pHint;
      Select(pSelection);
   }
}

void CBridgePlanView::UpdateSegmentTooltips()
{
   if ( m_SegmentIDs.size() == 0 )
   {
      return;
   }

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IMaterials,pMaterial);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeom);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   GET_IFACE2(pBroker, IGirder, pIGirder);

   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CTimelineManager* pTimelineMgr = pBridgeDesc->GetTimelineManager();

   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   CComPtr<iDisplayList> display_list;
   dispMgr->FindDisplayList(SEGMENT_DISPLAY_LIST,&display_list);

   GroupIndexType nGroups = pBridgeDesc->GetGirderGroupCount();
   for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
   {
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(grpIdx);

      GirderIndexType nGirders = pGroup->GetGirderCount();
      for ( GirderIndexType girderIdx = 0; girderIdx < nGirders; girderIdx++ )
      {
         const CSplicedGirderData* pGirder = pGroup->GetGirder(girderIdx);
         IndexType nDucts = pGirder->GetPostTensioning()->GetDuctCount();
#if defined _DEBUG
         GirderIDType girderID = pGirder->GetID();
#endif
         SegmentIndexType nSegments = pGirder->GetSegmentCount();
         for (SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++)
         {
            CSegmentKey segmentKey(grpIdx, girderIdx, segIdx);

            const CPrecastSegmentData* pSegment = pGirder->GetSegment(segIdx);

            std::map<CSegmentKey, IDType>::iterator found = m_SegmentIDs.find(segmentKey);
            if (found == m_SegmentIDs.end())
            {
               continue;
            }

            IDType ID = (*found).second;

            CComPtr<iDisplayObject> pDO;
            display_list->FindDisplayObject(ID, &pDO);
            ATLASSERT(pDO != nullptr);

            CComPtr<IDirection> direction;
            pBridge->GetSegmentBearing(segmentKey, &direction);

            CString strMsg1(_T("Double click to edit.\nRight click for more options."));

            Float64 gdr_length = pBridge->GetSegmentLength(segmentKey);
            Float64 span_length = pBridge->GetSegmentSpanLength(segmentKey);
            CString strMsg2;
            if (nDucts == 0)
            {
               strMsg2.Format(_T("\n\nGirder: %s\nGirder Length: %s\nSpan Length: %s\n\n%s"),
                  pGirder->GetGirderName(),
                  FormatDimension(gdr_length, pDisplayUnits->GetSpanLengthUnit()),
                  FormatDimension(span_length, pDisplayUnits->GetSpanLengthUnit()),
                  FormatDirection(direction)
               );
            }
            else
            {
               strMsg2.Format(_T("\n\nGirder: %s\nSegment Length: %s\n\n%s"),
                  pGirder->GetGirderName(),
                  FormatDimension(gdr_length, pDisplayUnits->GetSpanLengthUnit()),
                  FormatDirection(direction)
               );
            }

            Float64 fc = pSegment->Material.Concrete.Fc;
            Float64 fci = pSegment->Material.Concrete.Fci;

            CString strMsg3;
            strMsg3.Format(_T("\n\n%s\nf'ci: %s\nf'c: %s"),
               lrfdConcreteUtil::GetTypeName((WBFL::Materials::ConcreteType)pMaterial->GetSegmentConcreteType(segmentKey), true).c_str(),
               FormatDimension(fci, pDisplayUnits->GetStressUnit()),
               FormatDimension(fc, pDisplayUnits->GetStressUnit())
            );

            const auto* pStraightStrand = pMaterial->GetStrandMaterial(segmentKey, pgsTypes::Straight);
            const auto* pHarpedStrand = pMaterial->GetStrandMaterial(segmentKey, pgsTypes::Harped);
            const auto* pTempStrand = pMaterial->GetStrandMaterial(segmentKey, pgsTypes::Temporary);

            StrandIndexType Ns = pStrandGeom->GetStrandCount(segmentKey, pgsTypes::Straight);
            StrandIndexType Nh = pStrandGeom->GetStrandCount(segmentKey, pgsTypes::Harped);
            StrandIndexType Nt = pStrandGeom->GetStrandCount(segmentKey, pgsTypes::Temporary);
            StrandIndexType Nsd = pStrandGeom->GetNumDebondedStrands(segmentKey, pgsTypes::Straight, pgsTypes::dbetEither);

            std::_tstring harp_type(LABEL_HARP_TYPE(pStrandGeom->GetAreHarpedStrandsForcedStraight(segmentKey)));

            CString strMsg4;
            if (Nsd == 0)
            {
               strMsg4.Format(_T("\n\nStraight Strands\n%s\n# Straight: %2d"), pStraightStrand->GetName().c_str(), Ns);
            }
            else
            {
               strMsg4.Format(_T("\n\nStraight Strands\n%s\n# Straight: %2d (%2d Debonded)"), pStraightStrand->GetName().c_str(), Ns, Nsd);
            }
            CString strHarped;
            strHarped.Format(_T("\n\n%s Strands\n%s\n# %s: %2d"), harp_type.c_str(), pHarpedStrand->GetName().c_str(), harp_type.c_str(), Nh);
            strMsg4 += strHarped;

            if (pStrandGeom->GetMaxStrands(segmentKey, pgsTypes::Temporary) != 0)
            {
               CString strTemp;
               strTemp.Format(_T("\n\nTemporary Strands\n%s\n# Temporary: %2d"),pTempStrand->GetName().c_str(), Nt);
               strMsg4 += strTemp;
            }

            CString strMsg5;
            if (pBridge->GetDeckType() != pgsTypes::sdtNone && pBridge->GetHaunchInputDepthType() == pgsTypes::hidACamber)
            {
               // Slab Offset
               PierIndexType startPierIdx, endPierIdx;
               pBridge->GetGirderGroupPiers(segmentKey.groupIndex, &startPierIdx, &endPierIdx);

               Float64 startOffset = pBridge->GetSlabOffset(segmentKey, pgsTypes::metStart);
               Float64 endOffset = pBridge->GetSlabOffset(segmentKey, pgsTypes::metEnd);

               strMsg5.Format(_T("\n\nSlab Offset\nStart: %s\nEnd: %s"),
                  FormatDimension(startOffset, pDisplayUnits->GetComponentDimUnit()),
                  FormatDimension(endOffset, pDisplayUnits->GetComponentDimUnit())
               );
            }

            CString strMsg = strMsg1 + strMsg2 + strMsg3 + strMsg4 + strMsg5;

            // Precamber
            Float64 precamber = pIGirder->GetPrecamber(segmentKey);
            if (pIGirder->CanPrecamber(segmentKey) && !IsZero(precamber))
            {
               CString strPrecamber;
               strPrecamber.Format(_T("\n\nPrecamber: %s"), FormatDimension(precamber, pDisplayUnits->GetComponentDimUnit()));
               strMsg += strPrecamber;
            }

            // Top Flange Thickening
            pgsTypes::TopFlangeThickeningType tftType = pIGirder->GetTopFlangeThickeningType(segmentKey);
            if (pIGirder->CanTopFlangeBeLongitudinallyThickened(segmentKey) && tftType != pgsTypes::tftNone)
            {
               Float64 tft = pIGirder->GetTopFlangeThickening(segmentKey);
               CString strTFT;
               strTFT.Format(_T("\n\nTop Flange Thickening: %s at %s"), FormatDimension(tft, pDisplayUnits->GetComponentDimUnit()), (tftType == pgsTypes::tftEnds ? _T("Ends") : _T("Middle")));
               strMsg += strTFT;
            }


            // Top Width
            if (IsTopWidthSpacing(pIBridgeDesc->GetGirderSpacingType()))
            {
               const CSplicedGirderData* pSplicedGirder = pIBridgeDesc->GetGirder(segmentKey);
               Float64 wLeft, wRight;
               Float64 topWidthStart = pSplicedGirder->GetTopWidth(pgsTypes::metStart, &wLeft, &wRight);
               Float64 topWidthEnd = pSplicedGirder->GetTopWidth(pgsTypes::metEnd, &wLeft, &wRight);

               CString strTopWidth;
               if (IsEqual(topWidthStart, topWidthEnd))
               {
                  strTopWidth.Format(_T("\n\nTop Width: %s"), FormatDimension(topWidthStart, pDisplayUnits->GetXSectionDimUnit()));
               }
               else
               {
                  strTopWidth.Format(_T("\n\nTop Width: Start %s, End %s"), FormatDimension(topWidthStart, pDisplayUnits->GetXSectionDimUnit()), FormatDimension(topWidthEnd, pDisplayUnits->GetXSectionDimUnit()));
               }
               strMsg += strTopWidth;
            }

            SegmentIDType segmentID = pSegment->GetID();
            EventIndexType constructionEventIdx = pTimelineMgr->GetSegmentConstructionEventIndex(segmentID);
            EventIndexType erectionEventIdx = pTimelineMgr->GetSegmentErectionEventIndex(segmentID);
            const CTimelineEvent* pConstructionEvent = nullptr;
            const CTimelineEvent* pErectionEvent = nullptr;
            if (constructionEventIdx != INVALID_INDEX)
            {
               pConstructionEvent = pTimelineMgr->GetEventByIndex(constructionEventIdx);
            }
            if (erectionEventIdx != INVALID_INDEX)
            {
               pErectionEvent = pTimelineMgr->GetEventByIndex(erectionEventIdx);
            }
            CString strEvents;
            strEvents.Format(_T("\n\nConstruction: Event %d, %s\nErection: Event %d, %s"),
               LABEL_EVENT(constructionEventIdx),
               pConstructionEvent == nullptr ? _T("Construction event not defined") : pConstructionEvent->GetDescription(),
               LABEL_EVENT(erectionEventIdx),
               pErectionEvent == nullptr ? _T("Erection event not defined") : pErectionEvent->GetDescription()
               );
            strMsg += strEvents;
#if defined _DEBUG
            CString strSegID;
            strSegID.Format(_T("\n\nGirder ID: %d\nSegment ID: %d"),girderID,ID);

            strMsg += strSegID;
#endif // _DEBUG

            pDO->SetMaxTipWidth(TOOLTIP_WIDTH);
            pDO->SetTipDisplayTime(TOOLTIP_DURATION);
            pDO->SetToolTipText(strMsg);
         } // segment loop
      } // girder loop
   } // group loop
}

void CBridgePlanView::UpdateClosureJointTooltips()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);

   // only used if there is a closure joint
   GET_IFACE2_NOCHECK(pBroker,IMaterials,pMaterial);
   GET_IFACE2_NOCHECK(pBroker,IEAFDisplayUnits,pDisplayUnits);

   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CTimelineManager* pTimelineMgr = pBridgeDesc->GetTimelineManager();

   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   CComPtr<iDisplayList> display_list;
   dispMgr->FindDisplayList(CLOSURE_JOINT_DISPLAY_LIST,&display_list);

   GroupIndexType nGroups = pBridgeDesc->GetGirderGroupCount();
   for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
   {
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(grpIdx);

      GirderIndexType nGirders = pGroup->GetGirderCount();
      for ( GirderIndexType girderIdx = 0; girderIdx < nGirders; girderIdx++ )
      {
         const CSplicedGirderData* pGirder = pGroup->GetGirder(girderIdx);
         IndexType nClosures = pGirder->GetClosureJointCount();
         for ( IndexType cpIdx = 0; cpIdx < nClosures; cpIdx++ )
         {
            CSegmentKey closureKey(grpIdx,girderIdx,cpIdx);

            const CClosureJointData* pClosureJoint = pGirder->GetClosureJoint(cpIdx);

            std::map<CSegmentKey,IDType>::iterator found = m_ClosureJointIDs.find(closureKey);
            if ( found == m_ClosureJointIDs.end() )
            {
               continue;
            }

            IDType ID = (*found).second;

            CComPtr<iDisplayObject> pDO;
            display_list->FindDisplayObject(ID,&pDO);
            ATLASSERT(pDO != nullptr);

            EventIndexType eventIdx = pTimelineMgr->GetCastClosureJointEventIndex(pClosureJoint);
            const CTimelineEvent* pEvent = nullptr;
            if (eventIdx != INVALID_INDEX)
            {
               pEvent = pTimelineMgr->GetEventByIndex(eventIdx);
            }

            CString strMsg1(_T("Double click to edit.\nRight click for more options."));

            Float64 fc  = pClosureJoint->GetConcrete().Fc;
            Float64 fci = pClosureJoint->GetConcrete().Fci;

            CString strMsg2;
            strMsg2.Format(_T("\n\n%s\nf'ci: %s\nf'c: %s\n\nInstallation: Event %d, %s"),
                           lrfdConcreteUtil::GetTypeName((WBFL::Materials::ConcreteType)pClosureJoint->GetConcrete().Type,true).c_str(),
                           FormatDimension(fci,pDisplayUnits->GetStressUnit()),
                           FormatDimension(fc, pDisplayUnits->GetStressUnit()),
                           LABEL_EVENT(eventIdx),
                           pEvent == nullptr ? _T("Installation event not defined") : pEvent->GetDescription()
                           );

            CString strMsg = strMsg1 + strMsg2;


#if defined _DEBUG
            CString strClosureID;
            strClosureID.Format(_T("\r\nID: %d"), pClosureJoint->GetID());

            strMsg += strClosureID;
#endif // _DEBUG

            pDO->SetMaxTipWidth(TOOLTIP_WIDTH);
            pDO->SetTipDisplayTime(TOOLTIP_DURATION);
            pDO->SetToolTipText(strMsg);
         } // segment loop
      } // girder loop
   } // group loop
}

void CBridgePlanView::UpdateSectionCut()
{
   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   CComPtr<iDisplayList> display_list;
   dispMgr->FindDisplayList(SECTION_CUT_DISPLAY_LIST,&display_list);

   CComPtr<iDisplayObject> dispObj;
   display_list->FindDisplayObject(SECTION_CUT_ID,&dispObj);

   CComQIPtr<iPointDisplayObject> pntDO(dispObj);
   
   UpdateSectionCut(pntDO,TRUE);
}

void CBridgePlanView::UpdateSectionCut(iPointDisplayObject* pntDO,BOOL bRedraw)
{
   Float64 station = m_pFrame->GetCurrentCutLocation();

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   
   GET_IFACE2(pBroker,IRoadway,pRoadway);
   CComPtr<IDirection> bearing;
   pRoadway->GetBearing(station,&bearing);

   CComPtr<IPoint2d> point;
   pRoadway->GetPoint(station,0.00,bearing,pgsTypes::pcGlobal,&point);

   pntDO->SetPosition(point,bRedraw,FALSE);
}

void CBridgePlanView::HandleLButtonDblClk(UINT nFlags, CPoint logPoint) 
{
   GetFrame()->PostMessage(WM_COMMAND,ID_PROJECT_BRIDGEDESC,0);
}

void CBridgePlanView::HandleContextMenu(CWnd* pWnd,CPoint logPoint)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CPGSDocBase* pDoc = (CPGSDocBase*)GetDocument();
   CEAFMenu* pMenu = CEAFMenu::CreateContextMenu(pDoc->GetPluginCommandManager());
   pMenu->LoadMenu(IDR_BRIDGE_PLAN_CTX,nullptr);

   if ( logPoint.x < 0 || logPoint.y < 0 )
   {
      // the context menu key or Shift+F10 was pressed
      // need some real coordinates (how about the center of the client area)
      CRect rClient;
      GetClientRect(&rClient);
      CPoint center = rClient.TopLeft();
      ClientToScreen(&center);
      logPoint = center;
   }

   const std::map<IDType,IBridgePlanViewEventCallback*>& callbacks = pDoc->GetBridgePlanViewCallbacks();
   std::map<IDType,IBridgePlanViewEventCallback*>::const_iterator callbackIter(callbacks.begin());
   std::map<IDType,IBridgePlanViewEventCallback*>::const_iterator callbackIterEnd(callbacks.end());
   for ( ; callbackIter != callbackIterEnd; callbackIter++ )
   {
      IBridgePlanViewEventCallback* pCallback = callbackIter->second;
      pCallback->OnBackgroundContextMenu(pMenu);
   }


   pMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, logPoint.x, logPoint.y, this);
   delete pMenu;
}

void CBridgePlanView::OnEditDeck() 
{
   ((CPGSDocBase*)GetDocument())->EditBridgeDescription(EBD_DECK);
}

void CBridgePlanView::OnViewSettings() 
{
   ((CPGSDocBase*)GetDocument())->EditBridgeViewSettings(VS_BRIDGE_PLAN);
}

void CBridgePlanView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
   if ( nChar == VK_LEFT || nChar == VK_RIGHT )
   {
      CComPtr<iDisplayMgr> dispMgr;
      GetDisplayMgr(&dispMgr);
      DisplayObjectContainer selObjs;
      dispMgr->GetSelectedObjects(&selObjs);
      bool bSectionCutSelected = false;
      if ( 0 < selObjs.size() )
      {
         CComPtr<iDisplayObject> pDO = selObjs[0].m_T;
         if ( pDO->GetID() == SECTION_CUT_ID )
         {
            bSectionCutSelected = true;
         }
      }

      if ( ::GetKeyState(VK_CONTROL) < 0 || bSectionCutSelected )
      {
         CBridgeModelViewChildFrame* pFrame = GetFrame();
         if ( nChar == VK_LEFT )
         {
            pFrame->CutAtPrev();
         }
         else if ( nChar == VK_RIGHT )
         {
            pFrame->CutAtNext();
         }
         return; // don't send this on to the display view
      }
      else
      {
         // otherwise select a pier
         if ( selObjs.size() == 0 )
         {
            CComPtr<IBroker> pBroker;
            EAFGetBroker(&pBroker);
            GET_IFACE2(pBroker,IBridge,pBridge);

            PierIndexType nPiers = pBridge->GetPierCount();

            if (nChar == VK_LEFT)
            {
               m_pFrame->SelectPier(nPiers-1);
            }
            else
            {
               m_pFrame->SelectPier(0);
            }

            return;
         }
      }
   }
   else if (nChar == VK_UP || nChar == VK_DOWN )
   {
      if ( nChar == VK_DOWN && ::GetKeyState(VK_CONTROL) < 0 )
      {
         // CTRL + down arrow... put focus in Bridge Section View
         m_pFrame->GetBridgeSectionView()->SetFocus();
      }
      else
      {
         CComPtr<iDisplayMgr> dispMgr;
         GetDisplayMgr(&dispMgr);
         DisplayObjectContainer selObjs;
         dispMgr->GetSelectedObjects(&selObjs);
         if ( 0 == selObjs.size() )
         {
            SelectAlignment(true);
            return;
         }
      }
   }

   CBridgeViewPane::OnKeyDown(nChar,nRepCnt,nFlags);
}

BOOL CBridgePlanView::OnMouseWheel(UINT nFlags,short zDelta,CPoint pt)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);
   DisplayObjectContainer selObjs;
   dispMgr->GetSelectedObjects(&selObjs);
   bool bLeftRight = true;
   if ( 0 < selObjs.size() )
   {
      CComPtr<iDisplayObject> pDO = selObjs[0].m_T;
      CComPtr<iDisplayList> pDispList;
      pDO->GetDisplayList(&pDispList);
      IDType dispListID = pDispList->GetID();
      if ( pDO->GetID() == SECTION_CUT_ID )
      {
         bLeftRight = true;
      }
      else if ( dispListID == SPAN_DISPLAY_LIST || dispListID == PIER_DISPLAY_LIST || dispListID == TEMPORARY_SUPPORT_DISPLAY_LIST || dispListID == SLAB_DISPLAY_LIST )
      {
         bLeftRight = true;
      }
      else
      {
         // a girder or the alignment is selected
         bLeftRight = false;
      }
   }

   UINT nChar;
   if ( bLeftRight )
   {
      nChar = (zDelta < 0 ? VK_RIGHT : VK_LEFT);
   }
   else
   {
      nChar = (zDelta < 0 ? VK_DOWN : VK_UP);
   }

   OnKeyDown(nChar, 1, nFlags);

   return TRUE;
}

void CBridgePlanView::SetModelToWorldSpacingMapping()
{
   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   CComPtr<iCoordinateMap> map;
   dispMgr->GetCoordinateMap(&map);
   CComQIPtr<iMapping> mapping(map);

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker, IBridge, pBridge);
   GET_IFACE2(pBroker, IRoadway, pAlignment);

   PierIndexType startPierIdx = pBridge->GetGirderGroupStartPier(m_StartGroupIdx);
   PierIndexType endPierIdx = pBridge->GetGirderGroupEndPier(m_EndGroupIdx == ALL_GROUPS ? pBridge->GetPierCount() - 1 : m_EndGroupIdx);
   
   // get point on alignment at first pier
   CComPtr<IDirection> dir;
   Float64 station = pBridge->GetPierStation(startPierIdx);
   pBridge->GetPierDirection(startPierIdx, &dir);
   CComPtr<IPoint2d> rotation_center;
   pAlignment->GetPoint(station, 0.00, dir, pgsTypes::pcGlobal, &rotation_center);

   // get point on alignment at last pier
   CComPtr<IPoint2d> end_point;
   dir.Release();
   station = pBridge->GetPierStation(endPierIdx);
   pBridge->GetPierDirection(endPierIdx, &dir);
   pAlignment->GetPoint(station, 0.00, dir, pgsTypes::pcGlobal, &end_point);

   // get the direction of the line from the start of the bridge to the end
   // this represents the amount we want to rotate the display

   Float64 x1, y1, x2, y2;
   rotation_center->get_X(&x1);
   rotation_center->get_Y(&y1);
   end_point->get_X(&x2);
   end_point->get_Y(&y2);

   Float64 dx = x2 - x1;
   Float64 dy = y2 - y1;

   Float64 angle = atan2(dy, dx);

   CPGSDocBase* pDoc = (CPGSDocBase*)GetDocument();
   UINT settings = pDoc->GetBridgeEditorSettings();
   if (settings & IDB_PV_NORTH_UP)
   {
      mapping->SetRotation((x1 + x2) / 2, (y1 + y2) / 2, 0);
   }
   else
   {
      // rotation by negative of the angle
      mapping->SetRotation((x1 + x2) / 2, (y1 + y2) / 2, -angle);
   }
}

void CBridgePlanView::UpdateDisplayObjects()
{
   CWaitCursor wait;

   // capture the current selection
   CPGSDocBase* pDoc = (CPGSDocBase*)GetDocument();
   CSelection selection = pDoc->GetSelection();

   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   CDManipClientDC dc(this);

   dispMgr->ClearDisplayObjects();

   CComPtr<iDisplayList> label_display_list;
   dispMgr->FindDisplayList(LABEL_DISPLAY_LIST, &label_display_list);
   label_display_list->Clear();

   SetModelToWorldSpacingMapping(); // must do this before building display objects

   BuildTitleDisplayObjects();
   BuildAlignmentDisplayObjects();

   BuildPierDisplayObjects();

   BuildSegmentDisplayObjects();
   BuildLongitudinalJointDisplayObject();
   BuildGirderDisplayObjects();

   BuildTemporarySupportDisplayObjects();
   BuildClosureJointDisplayObjects();

   BuildSpanDisplayObjects();

   BuildSlabDisplayObjects();
   BuildSectionCutDisplayObjects();
   BuildNorthArrowDisplayObjects();

   BuildDiaphragmDisplayObjects();
   
   UpdateSegmentTooltips();
   UpdateClosureJointTooltips();

   // restore the selection
   Select(&selection);
}

void CBridgePlanView::BuildTitleDisplayObjects()
{
   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   CComPtr<iDisplayList> title_list;
   dispMgr->FindDisplayList(TITLE_DISPLAY_LIST,&title_list);

   title_list->Clear();

   CComPtr<iViewTitle> title;
   title.CoCreateInstance(CLSID_ViewTitle);

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IBridge,pBridge);
   GroupIndexType nGroups = pBridge->GetGirderGroupCount();

   GET_IFACE2(pBroker, IDocumentType, pDocType);
   bool isPGSuper = pDocType->IsPGSuperDocument();

   CString strTitle;
   if ( m_StartGroupIdx == 0 && (m_EndGroupIdx == nGroups-1 || m_EndGroupIdx == ALL_GROUPS) )
   {
      strTitle = _T("Plan View");
   }
   else if ( m_StartGroupIdx == m_EndGroupIdx )
   {
      if (isPGSuper)
      {
         strTitle.Format(_T("Plan View: Span %s"), LABEL_SPAN(m_StartGroupIdx));
      }
      else
      {
         strTitle.Format(_T("Plan View: Group %d"), LABEL_GROUP(m_StartGroupIdx));
      }
   }
   else
   {
      if (isPGSuper)
      {
         strTitle.Format(_T("Plan View: Spans %s - %s"), LABEL_SPAN(m_StartGroupIdx), LABEL_SPAN(m_EndGroupIdx));
      }
      else
      {
         strTitle.Format(_T("Plan View: Groups %d - %d"), LABEL_GROUP(m_StartGroupIdx), LABEL_GROUP(m_EndGroupIdx));
      }
   }

   title->SetText(strTitle);
   title_list->AddDisplayObject(title);
}

void CBridgePlanView::BuildAlignmentDisplayObjects()
{
   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   CComPtr<iDisplayList> display_list;
   dispMgr->FindDisplayList(ALIGNMENT_DISPLAY_LIST, &display_list);

   display_list->Clear();

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker, IBridge, pBridge);
   GET_IFACE2(pBroker, IRoadway, pRoadway);
   GET_IFACE2(pBroker, IRoadwayData, pRoadwayData);
   bool bIsPGLOffsetFromAlignment = (pRoadwayData->GetRoadwaySectionData().AlignmentPointIdx != pRoadwayData->GetRoadwaySectionData().ProfileGradePointIdx) ? true : false;

   // show that part of the alignment from 1/n of the first span length before the start of the bridge
   // to 1/n of the last span length beyond the end of the bridge
   Float64 n = 10;
   GroupIndexType nGroups = pBridge->GetGirderGroupCount();
   GroupIndexType startGroupIdx = (m_StartGroupIdx == ALL_GROUPS ? 0 : m_StartGroupIdx);
   GroupIndexType endGroupIdx = (m_EndGroupIdx == ALL_GROUPS ? nGroups - 1 : m_EndGroupIdx);

   PierIndexType startPierIdx = pBridge->GetGirderGroupStartPier(startGroupIdx);
   PierIndexType endPierIdx   = pBridge->GetGirderGroupEndPier(endGroupIdx);

   Float64 start_station = pBridge->GetPierStation(startPierIdx);
   Float64 end_station = pBridge->GetPierStation(endPierIdx);
   Float64 length1 = pBridge->GetPierStation(startPierIdx + 1) - start_station;
   Float64 length2 = end_station - pBridge->GetPierStation(endPierIdx - 1);

   // project the edges of the first and last pier onto the alignment
   // use the min/max station as the start and end of the bridge for purposes
   // of defining the alignment station range
   CComPtr<IPoint2d> start_left, start_alignment, start_bridge, start_right;
   pBridge->GetPierPoints(startPierIdx, pgsTypes::pcGlobal, &start_left, &start_alignment, &start_bridge, &start_right);
   Float64 start1, start2, start3, start4, offset;
   pRoadway->GetStationAndOffset(pgsTypes::pcGlobal, start_left, &start1, &offset);
   pRoadway->GetStationAndOffset(pgsTypes::pcGlobal, start_alignment, &start2, &offset);
   pRoadway->GetStationAndOffset(pgsTypes::pcGlobal, start_bridge, &start3, &offset);
   pRoadway->GetStationAndOffset(pgsTypes::pcGlobal, start_right, &start4, &offset);
   start_station = Min(start1, start2, start3, start4);
   start_station -= length1 / n;

   CComPtr<IPoint2d> end_left, end_alignment, end_bridge, end_right;
   pBridge->GetPierPoints(endPierIdx, pgsTypes::pcGlobal, &end_left, &end_alignment, &end_bridge, &end_right);
   Float64 end1, end2, end3, end4;
   pRoadway->GetStationAndOffset(pgsTypes::pcGlobal, end_left, &end1, &offset);
   pRoadway->GetStationAndOffset(pgsTypes::pcGlobal, end_alignment, &end2, &offset);
   pRoadway->GetStationAndOffset(pgsTypes::pcGlobal, end_bridge, &end3, &offset);
   pRoadway->GetStationAndOffset(pgsTypes::pcGlobal, end_right, &end4, &offset);
   end_station = Max(end1, end2, end3, end4);
   end_station += length2 / n;

   // The alignment is represented on the screen by a poly line object
   CComPtr<iPolyLineDisplayObject> doAlignment;
   doAlignment.CoCreateInstance(CLSID_PolyLineDisplayObject);

   // Register an event sink with the alignment object so that we can handle double clicks
   // on the alignment differently then a general dbl-click
   CAlignmentDisplayObjectEvents* pEvents = new CAlignmentDisplayObjectEvents(pBroker, m_pFrame, CAlignmentDisplayObjectEvents::BridgePlan);
   CComPtr<iDisplayObjectEvents> events;
   events.Attach((iDisplayObjectEvents*)pEvents->GetInterface(&IID_iDisplayObjectEvents));

   CComPtr<iDisplayObject> dispObj;
   doAlignment->QueryInterface(IID_iDisplayObject, (void**)&dispObj);
   dispObj->RegisterEventSink(events);
   dispObj->SetToolTipText(_T("Double click to edit alignment.\nRight click for more options."));
   dispObj->SetMaxTipWidth(TOOLTIP_WIDTH);
   dispObj->SetTipDisplayTime(TOOLTIP_DURATION);

   // display object for CL bridge
   CComPtr<iPolyLineDisplayObject> doCLBridge;
   doCLBridge.CoCreateInstance(CLSID_PolyLineDisplayObject);

   Float64 alignment_offset = pBridge->GetAlignmentOffset();

   // display object for PGL
   CComPtr<iPolyLineDisplayObject> doPGL;
   doPGL.CoCreateInstance(CLSID_PolyLineDisplayObject);

   // model the alignment as a series of individual points
   CComPtr<IDirection> bearing;
   bearing.CoCreateInstance(CLSID_Direction);
   IndexType nPoints = 50;
   Float64 station_inc = (end_station - start_station) / (nPoints - 1);
   std::vector<Float64> vStations;
   PierIndexType nPiers = pBridge->GetPierCount();
   vStations.reserve(nPoints + nPiers);
   Float64 station = start_station;
   for (long i = 0; i < nPoints; i++, station += station_inc)
   {
      vStations.push_back(station);
   }
   for (PierIndexType pierIdx = startPierIdx; pierIdx <= endPierIdx; pierIdx++)
   {
      Float64 pierStation = pBridge->GetPierStation(pierIdx);
      vStations.push_back(pierStation);
   }
   std::sort(vStations.begin(), vStations.end());
   vStations.erase(std::unique(vStations.begin(), vStations.end(), [](const auto& v1, const auto& v2) {return IsEqual(v1, v2);}), vStations.end());

   for ( const auto& station : vStations)
   {
      CComPtr<IPoint2d> pntAlignment;
      pRoadway->GetPoint(station,0.00,bearing,pgsTypes::pcGlobal,&pntAlignment);
      doAlignment->AddPoint(pntAlignment);

      Float64 pgl_offset = 0;
      if (bIsPGLOffsetFromAlignment)
      {
         IndexType alignmentIdx = pRoadway->GetAlignmentPointIndex(station); // get index of crown point corresponding to the alignment
         Float64 offset = pRoadway->GetProfileGradeLineOffset(alignmentIdx, station); // get the offset from the alignment point to the PGL

         ATLASSERT(!IsZero(offset)); // only drawing PGL if it is offset from alignment so this better not be zero
         pgl_offset = -offset;
      }

      CComPtr<IDirection> normal;
      if (!IsZero(alignment_offset) || !IsZero(pgl_offset))
      {
         pRoadway->GetBearingNormal(station, &normal);
      }

      if ( !IsZero(alignment_offset) )
      {
         CComPtr<IPoint2d> pntBridgeLine;
         pRoadway->GetPoint(station,alignment_offset,normal,pgsTypes::pcGlobal,&pntBridgeLine);
         doCLBridge->AddPoint(pntBridgeLine);
      }

      CComPtr<IPoint2d> pntPGL;
      if (!IsZero(pgl_offset))
      {
         pRoadway->GetPoint(station, pgl_offset, normal, pgsTypes::pcGlobal, &pntPGL);
         doPGL->AddPoint(pntPGL);
      }

      if (IsEqual(station, vStations.front()))
      {
         CComPtr<iTextBlock> doText;
         doText.CoCreateInstance(CLSID_TextBlock);
         doText->SetPosition(pntAlignment);
         doText->SetText(pRoadwayData->GetAlignmentData2().Name.c_str());
         doText->SetTextAlign(TA_BOTTOM | TA_LEFT);
         doText->SetBkMode(TRANSPARENT);
         CComPtr<IDirection> bearing;
         pRoadway->GetBearing(station, &bearing);
         Float64 dir;
         bearing->get_Value(&dir);
         long angle = long(1800.*dir / M_PI);
         angle = (900 < angle && angle < 2700) ? angle - 1800 : angle;
         doText->SetAngle(angle);
         display_list->AddDisplayObject(doText);

         if (bIsPGLOffsetFromAlignment)
         {
            doText.Release();
            doText.CoCreateInstance(CLSID_TextBlock);
            doText->SetPosition(pntPGL);
            doText->SetText(_T("PGL"));
            doText->SetTextAlign(TA_BOTTOM | TA_LEFT);
            doText->SetBkMode(TRANSPARENT);
            doText->SetAngle(angle);
            display_list->AddDisplayObject(doText);
         }
      }
   }

   doAlignment->put_Width(ALIGNMENT_LINE_WEIGHT);
   doAlignment->put_Color(ALIGNMENT_COLOR);
   doAlignment->put_PointType(plpNone);
   doAlignment->Commit();

   doPGL->put_Width(PROFILE_LINE_WEIGHT);
   doPGL->put_Color(PROFILE_COLOR);
   doPGL->put_PointType(plpNone);
   doPGL->Commit();

   doCLBridge->put_Width(BRIDGELINE_LINE_WEIGHT);
   doCLBridge->put_Color(BRIDGE_COLOR);
   doCLBridge->put_PointType(plpNone);
   doCLBridge->Commit();

   display_list->AddDisplayObject(dispObj);

   if ( !IsZero(alignment_offset) )
   {
      CComQIPtr<iDisplayObject> dispObj2(doCLBridge);
      display_list->AddDisplayObject(dispObj2);
   }

   if(bIsPGLOffsetFromAlignment)
   {
      CComQIPtr<iDisplayObject> dispObj3(doPGL);
      display_list->AddDisplayObject(dispObj3);
   }

   dispObj->SetSelectionType(stAll);
   dispObj->SetID(ALIGNMENT_ID);

   // the alignment is going to be the drop site for the section cut object
   CComQIPtr<iDropSite> drop_site(events);
   dispObj->RegisterDropSite(drop_site);
}

void CBridgePlanView::BuildSegmentDisplayObjects()
{
   USES_CONVERSION;

   CBridgeModelViewChildFrame* pFrame = GetFrame();

   CPGSDocBase* pDoc = (CPGSDocBase*)GetDocument();
   CComPtr<IBroker> pBroker;
   pDoc->GetBroker(&pBroker);

   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   CComPtr<iDisplayList> display_list;
   dispMgr->FindDisplayList(SEGMENT_DISPLAY_LIST,&display_list);
   display_list->Clear();

   CComPtr<iDisplayList> bearing_display_list;
   dispMgr->FindDisplayList(BEARING_DISPLAY_LIST,&bearing_display_list);
   bearing_display_list->Clear();

   CComPtr<iDisplayList> label_display_list;
   dispMgr->FindDisplayList(LABEL_DISPLAY_LIST,&label_display_list);

   //
   // set up some drawing strategies
   //
   UINT settings = pDoc->GetBridgeEditorSettings();

   // drawing the bearings
   CComPtr<iSimpleDrawLineStrategy> strategy_brg;
   if ( settings & IDB_PV_LABEL_BEARINGS )
   {
      strategy_brg.CoCreateInstance(CLSID_SimpleDrawLineStrategy);
      strategy_brg->SetColor(RGB(0,0,255));
      strategy_brg->SetLineStyle(lsCenterline);
   }

   // restart the display object ids
   m_SegmentIDs.clear();
   m_NextSegmentID = 0;


#if defined _SHOW_CL_GIRDER
   // girder line
   CComPtr<iSimpleDrawLineStrategy> strategy_girder;
   strategy_girder.CoCreateInstance(CLSID_SimpleDrawLineStrategy);
   strategy_girder->SetColor(RGB(0,128,0));
   strategy_girder->SetLineStyle(lsCenterline);
#endif // _SHOW_CL_GIRDER

   GET_IFACE2_NOCHECK(pBroker,IBridge,pBridge);
   GET_IFACE2_NOCHECK(pBroker,IGirder,pIGirder);

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   GroupIndexType nGroups = pBridgeDesc->GetGirderGroupCount();
   GroupIndexType startGroupIdx = (m_StartGroupIdx == ALL_GROUPS ? 0 : m_StartGroupIdx);
   GroupIndexType endGroupIdx = (m_EndGroupIdx == ALL_GROUPS ? nGroups - 1 : m_EndGroupIdx);

   for ( GroupIndexType grpIdx = startGroupIdx; grpIdx <= endGroupIdx; grpIdx++ )
   {
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(grpIdx);

      GirderIndexType nGirders = pGroup->GetGirderCount();
      for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
      {
         const CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);
         SegmentIndexType nSegments = pGirder->GetSegmentCount();
         for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
         {
            CSegmentKey segmentKey(grpIdx,gdrIdx,segIdx);

            CComPtr<IShape> shape;
            pIGirder->GetSegmentPlan(segmentKey, &shape);
            CComQIPtr<IPolyShape> polyShape(shape);
            CComPtr<IPoint2d> pntStart, pntEnd;
            IndexType nPoints;
            polyShape->get_Count(&nPoints);
            polyShape->get_Point(1, &pntStart);
            polyShape->get_Point(2+(nPoints-6)/2+2, &pntEnd);

            CComPtr<iPointDisplayObject> doSegment;
            doSegment.CoCreateInstance(CLSID_PointDisplayObject);
            doSegment->SetPosition(pntStart, FALSE, FALSE);

            IDType ID = m_NextSegmentID++;
            m_SegmentIDs.insert( std::make_pair(segmentKey,ID) );
            doSegment->SetID(ID);
            SegmentDisplayObjectInfo* pInfo = new SegmentDisplayObjectInfo(segmentKey,SEGMENT_DISPLAY_LIST);
            doSegment->SetItemData((void*)pInfo,true);
            doSegment->SetSelectionType(stAll);
            
            if (1 < nSegments)
            {
               // for spliced girder bridges, if a segment is selected, clicking a second time on the segments should move the selection
               // to the next DO which is the entire girder.
               doSegment->RetainSelection(FALSE);
            }

            CComPtr<iShapeDrawStrategy> shapeDrawStrategy;
            shapeDrawStrategy.CoCreateInstance(CLSID_ShapeDrawStrategy);
            shapeDrawStrategy->SetShape(shape);
            shapeDrawStrategy->DoFill(true);
            shapeDrawStrategy->SetSolidFillColor(SEGMENT_FILL_COLOR);
            shapeDrawStrategy->SetSolidLineColor(SEGMENT_BORDER_COLOR);

            doSegment->SetDrawingStrategy(shapeDrawStrategy);

            CComQIPtr<iGravityWellStrategy> gravity_well(shapeDrawStrategy);
            doSegment->SetGravityWellStrategy(gravity_well);

            display_list->AddDisplayObject(doSegment);

            // direction to output text
            CComPtr<IDirection> direction;
            pBridge->GetSegmentBearing(segmentKey,&direction);
            Float64 dir;
            direction->get_Value(&dir);
            long angle = long(1800.*dir/M_PI);
            angle = (900 < angle && angle < 2700 ) ? angle-1800 : angle;

            if ( settings & IDB_PV_LABEL_BEARINGS )
            {
               // Labels the girder bearings            
               CComPtr<IDirectionDisplayUnitFormatter> direction_formatter;
               direction_formatter.CoCreateInstance(CLSID_DirectionDisplayUnitFormatter);
               direction_formatter->put_BearingFormat(VARIANT_TRUE);
               CComPtr<iTextBlock> doText2;
               doText2.CoCreateInstance(CLSID_TextBlock);

               Float64 x1,y1, x2,y2;
               pntStart->Location(&x1,&y1);
               pntEnd->Location(&x2,&y2);

               Float64 x = x1 + (x2-x1)/2;
               Float64 y = y1 + (y2-y1)/2;
               CComPtr<IPoint2d> pntText2;
               pntText2.CoCreateInstance(CLSID_Point2d);
               pntText2->Move(x,y);
               doText2->SetPosition(pntText2);

               CComBSTR bstrBearing;
               direction_formatter->Format(dir,CComBSTR("°,\',\""),&bstrBearing);
               doText2->SetText(OLE2T(bstrBearing));
               doText2->SetAngle(angle);
               doText2->SetTextAlign(TA_BOTTOM | TA_LEFT);
               doText2->SetBkMode(TRANSPARENT);
               label_display_list->AddDisplayObject(doText2);
            }

            // Register an event sink with the segment display object so that we can handle double clicks
            // on the segment differently then a general double click
            CBridgePlanViewSegmentDisplayObjectEvents* pEvents = new CBridgePlanViewSegmentDisplayObjectEvents(segmentKey,pFrame);
            CComPtr<iDisplayObjectEvents> events;
            events.Attach((iDisplayObjectEvents*)pEvents->GetInterface(&IID_iDisplayObjectEvents));

            CComQIPtr<iDisplayObject,&IID_iDisplayObject> dispObj(doSegment);
            dispObj->RegisterEventSink(events);
         } // segment loop
      } // girder loop
   } // group loop
}

void CBridgePlanView::BuildLongitudinalJointDisplayObject()
{
   CPGSDocBase* pDoc = (CPGSDocBase*)GetDocument();
   CComPtr<IBroker> pBroker;
   pDoc->GetBroker(&pBroker);

   GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   if (!pBridgeDesc->HasLongitudinalJoints() || pBridgeDesc->GetDeckDescription()->TransverseConnectivity == pgsTypes::atcConnectedRelativeDisplacement )
   {
      return;
   }

   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   CComPtr<iDisplayList> display_list;
   dispMgr->FindDisplayList(JOINT_DISPLAY_LIST, &display_list);
   display_list->Clear();

   GET_IFACE2_NOCHECK(pBroker, IGirder, pIGirder);

   GroupIndexType nGroups = pBridgeDesc->GetGirderGroupCount();
   GroupIndexType firstGroupIdx = (m_StartGroupIdx == ALL_GROUPS ? 0 : m_StartGroupIdx);
   GroupIndexType lastGroupIdx = (m_EndGroupIdx == ALL_GROUPS ? nGroups - 1 : m_EndGroupIdx);
   for (GroupIndexType groupIdx = firstGroupIdx; groupIdx <= lastGroupIdx; groupIdx++)
   {
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(groupIdx);

      GirderIndexType nGirders = pGroup->GetGirderCount();
      for (GirderIndexType gdrIdx = 0; gdrIdx < nGirders-1; gdrIdx++)
      {
         const CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);
         SegmentIndexType nSegments = pGirder->GetSegmentCount();
         for (SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++)
         {
            CSegmentKey leftSegmentKey(groupIdx, gdrIdx, segIdx);
            CSegmentKey rightSegmentKey(groupIdx, gdrIdx+1, segIdx);

            const int left = 0;
            const int right = 1;
            std::array<CComPtr<IPoint2d>, 2> pntEnd1Left, pntEnd1, pntEnd1Right, pntEnd2Right, pntEnd2, pntEnd2Left;
            pIGirder->GetSegmentPlanPoints(leftSegmentKey, pgsTypes::pcGlobal, &pntEnd1Left[left], &pntEnd1[left], &pntEnd1Right[left], &pntEnd2Right[left], &pntEnd2[left], &pntEnd2Left[left]);
            pIGirder->GetSegmentPlanPoints(rightSegmentKey, pgsTypes::pcGlobal, &pntEnd1Left[right], &pntEnd1[right], &pntEnd1Right[right], &pntEnd2Right[right], &pntEnd2[right], &pntEnd2Left[right]);

            CComPtr<iPointDisplayObject> doJoint;
            doJoint.CoCreateInstance(CLSID_PointDisplayObject);
            doJoint->SetPosition(pntEnd1[left], FALSE, FALSE);

            CComPtr<IPolyShape> polyShape;
            polyShape.CoCreateInstance(CLSID_PolyShape);
            polyShape->AddPointEx(pntEnd1Left[right]);
            polyShape->AddPointEx(pntEnd1Right[left]);
            polyShape->AddPointEx(pntEnd2Right[left]);
            polyShape->AddPointEx(pntEnd2Left[right]);

            CComQIPtr<IShape> shape(polyShape);

            CComPtr<iShapeDrawStrategy> shapeDrawStrategy;
            shapeDrawStrategy.CoCreateInstance(CLSID_ShapeDrawStrategy);
            shapeDrawStrategy->SetShape(shape);
            shapeDrawStrategy->DoFill(true);
            COLORREF fillColor = pBridgeDesc->HasStructuralLongitudinalJoints() ? JOINT_FILL_COLOR : JOINT_FILL_GHOST_COLOR;
            shapeDrawStrategy->SetSolidFillColor(fillColor);
            shapeDrawStrategy->SetSolidLineColor(JOINT_BORDER_COLOR);

            doJoint->SetDrawingStrategy(shapeDrawStrategy);

            display_list->AddDisplayObject(doJoint);
         }
      }
   }
}

void CBridgePlanView::BuildGirderDisplayObjects()
{
   // this is a composite display object that is simply a container for all the individual girder
   // segment display objects. it will treat all the segment display objects as a single object.
   CBridgeModelViewChildFrame* pFrame = GetFrame();

   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   CComPtr<iDisplayList> girderline_display_list;
   dispMgr->FindDisplayList(GIRDER_DISPLAY_LIST,&girderline_display_list);
   girderline_display_list->Clear();

   CComPtr<iDisplayList> segment_display_list;
   dispMgr->FindDisplayList(SEGMENT_DISPLAY_LIST,&segment_display_list);

   CComPtr<iDisplayList> label_display_list;
   dispMgr->FindDisplayList(LABEL_DISPLAY_LIST,&label_display_list);


   m_NextGirderID = 0;
   m_GirderIDs.clear();

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2_NOCHECK(pBroker,IBridge,pBridge);
   GET_IFACE2_NOCHECK(pBroker,IGirder,pIGirder);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   CPGSDocBase* pDoc = (CPGSDocBase*)GetDocument();
   UINT settings = pDoc->GetBridgeEditorSettings();

   GroupIndexType nGroups = pBridgeDesc->GetGirderGroupCount();
   GroupIndexType firstGroupIdx = (m_StartGroupIdx == ALL_GROUPS ? 0 : m_StartGroupIdx);
   GroupIndexType lastGroupIdx = (m_EndGroupIdx == ALL_GROUPS ? nGroups - 1 : m_EndGroupIdx);
   for (GroupIndexType groupIdx = firstGroupIdx; groupIdx <= lastGroupIdx; groupIdx++)
   {
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(groupIdx);

      GirderIndexType nGirdersThisGroup = pGroup->GetGirderCount();

      for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirdersThisGroup; gdrIdx++ )
      {
         CComPtr<iCompositeDisplayObject> doGirderLine;
         doGirderLine.CoCreateInstance(CLSID_CompositeDisplayObject);
         doGirderLine->SetSelectionType(stAll);

         CGirderKey girderKey(groupIdx,gdrIdx);

         IDType ID = m_NextGirderID++;
         m_GirderIDs.insert( std::make_pair(girderKey,ID) );

         doGirderLine->SetID(ID);

         GirderDisplayObjectInfo* pInfo = new GirderDisplayObjectInfo(girderKey,GIRDER_DISPLAY_LIST);
         doGirderLine->SetItemData((void*)pInfo,true);

         const CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);
         SegmentIndexType nSegments = pGirder->GetSegmentCount();
         for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
         {
            CSegmentKey segmentKey(groupIdx,gdrIdx,segIdx);
            std::map<CSegmentKey,IDType>::iterator found = m_SegmentIDs.find(segmentKey);
            ASSERT(found != m_SegmentIDs.end() );

            IDType ID = (*found).second;

            CComPtr<iDisplayObject> doSegment;
            segment_display_list->FindDisplayObject(ID,&doSegment);
            ASSERT(doSegment);

            if ( settings & IDB_PV_LABEL_GIRDERS )
            {
               // direction to output text
               CComPtr<IDirection> direction;
               pBridge->GetSegmentBearing(segmentKey,&direction);
               Float64 dir;
               direction->get_Value(&dir);
               long angle = long(1800.*dir/M_PI);
               angle = (900 < angle && angle < 2700 ) ? angle-1800 : angle;

               // segment end points
               CComPtr<IPoint2d> pntSupport1,pntEnd1,pntBrg1,pntBrg2,pntEnd2,pntSupport2;
               pIGirder->GetSegmentEndPoints(segmentKey,pgsTypes::pcGlobal,&pntSupport1,&pntEnd1,&pntBrg1,&pntBrg2,&pntEnd2,&pntSupport2);

               // girder labels
               Float64 x1,y1, x2,y2;
               pntEnd1->get_X(&x1);
               pntEnd1->get_Y(&y1);
               pntEnd2->get_X(&x2);
               pntEnd2->get_Y(&y2);
               Float64 x = x1 + (x2-x1)/4;
               Float64 y = y1 + (y2-y1)/4;
               CComPtr<IPoint2d> pntText;
               pntText.CoCreateInstance(CLSID_Point2d);
               pntText->Move(x,y);
               CComPtr<iTextBlock> doText;
               doText.CoCreateInstance(CLSID_TextBlock);
               doText->SetPosition(pntText);

               CString strText;
               strText.Format(_T("%s"),LABEL_GIRDER(gdrIdx));
               doText->SetText(strText);
               doText->SetTextAlign(TA_BOTTOM | TA_CENTER);
               doText->SetBkMode(TRANSPARENT);
               doText->SetAngle(angle);

               label_display_list->AddDisplayObject(doText);
            }

            doGirderLine->AddDisplayObject(doSegment);
         }

         // Register an event sink with the girder display object so that we can handle double clicks
         // on the girder differently then a general double click
         CBridgePlanViewGirderDisplayObjectEvents* pEvents = new CBridgePlanViewGirderDisplayObjectEvents(girderKey,nGroups,nGirdersThisGroup,pFrame);
         CComPtr<iDisplayObjectEvents> events;
         events.Attach((iDisplayObjectEvents*)pEvents->GetInterface(&IID_iDisplayObjectEvents));

         CComQIPtr<iDisplayObject,&IID_iDisplayObject> dispObj(doGirderLine);
         dispObj->RegisterEventSink(events);

         girderline_display_list->AddDisplayObject(doGirderLine);
      } // girder loop
   } // group loop
}

void CBridgePlanView::BuildPierDisplayObjects()
{
   CBridgeModelViewChildFrame* pFrame = GetFrame();

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   CComPtr<iDisplayList> display_list;
   dispMgr->FindDisplayList(PIER_DISPLAY_LIST,&display_list);
   display_list->Clear();

   CComPtr<iDisplayList> label_display_list;
   dispMgr->FindDisplayList(LABEL_DISPLAY_LIST,&label_display_list);

   CPGSDocBase* pDoc = (CPGSDocBase*)GetDocument();
   UINT settings = pDoc->GetBridgeEditorSettings();

   GET_IFACE2(pBroker,IRoadway,pAlignment);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   GET_IFACE2(pBroker,IGirder,pGirder);
   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IPointOfInterest,pPoi);

   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CTimelineManager* pTimelineMgr = pBridgeDesc->GetTimelineManager();

   bool bNoDeck = IsNonstructuralDeck(pBridgeDesc->GetDeckDescription()->GetDeckType());

   CComPtr<IDocUnitSystem> docUnitSystem;
   pDoc->GetDocUnitSystem(&docUnitSystem);

   GroupIndexType nGroups = pBridge->GetGirderGroupCount();
   PierIndexType nPiers = pBridge->GetPierCount();
   SpanIndexType nSpans = pBridge->GetSpanCount();
   Float64 last_station;
   GroupIndexType firstGroupIdx = (m_StartGroupIdx == ALL_GROUPS ? 0 : m_StartGroupIdx);
   GroupIndexType lastGroupIdx  = (m_EndGroupIdx  == ALL_GROUPS ? nGroups-1 : m_EndGroupIdx);
   for ( GroupIndexType groupIdx = firstGroupIdx; groupIdx <= lastGroupIdx; groupIdx++ )
   {
      PierIndexType firstPierIdx, lastPierIdx;
      pBridge->GetGirderGroupPiers(groupIdx, &firstPierIdx, &lastPierIdx);
      for (PierIndexType pierIdx = firstPierIdx; pierIdx <= lastPierIdx; pierIdx++)
      {
         const CPierData2* pPier = pBridgeDesc->GetPier(pierIdx);
         PierIDType pierID = pPier->GetID();

         // get station of the pier
         Float64 station = pPier->GetStation();

         CComPtr<IDirection> direction;
         pBridge->GetPierDirection(pierIdx, &direction);

         // skew the pier so it parallels the alignment
         CComPtr<IAngle> objSkew;
         pBridge->GetPierSkew(pierIdx, &objSkew);
         Float64 skew;
         objSkew->get_Value(&skew);

         // get the pier control points
         CComPtr<IPoint2d> left, alignment_pt, bridge_pt, right;
         pBridge->GetPierPoints(pierIdx, pgsTypes::pcGlobal, &left, &alignment_pt, &bridge_pt, &right);

         // create a point display object for the left side of the pier
         // add a socket to it
         CComPtr<iPointDisplayObject> doLeft;
         doLeft.CoCreateInstance(CLSID_PointDisplayObject);
         doLeft->SetPosition(left, FALSE, FALSE);
         CComQIPtr<iConnectable> connectable1(doLeft);
         CComPtr<iSocket> socket1;
         connectable1->AddSocket(0, left, &socket1);

         // create a point display object for the right side of the pier
         // add a socket to it
         CComPtr<iPointDisplayObject> doRight;
         doRight.CoCreateInstance(CLSID_PointDisplayObject);
         doRight->SetPosition(right, FALSE, FALSE);
         CComQIPtr<iConnectable> connectable2(doRight);
         CComPtr<iSocket> socket2;
         connectable2->AddSocket(0, right, &socket2);

         // create a line display object for the pier centerline
         CComPtr<iLineDisplayObject> doCenterLine;
         doCenterLine.CoCreateInstance(CLSID_LineDisplayObject);

         CString strMsg1;
         strMsg1.Format(_T("Double click to edit %s\nRight click for more options."), LABEL_PIER_EX(pPier->IsAbutment(),pierIdx));

         CString strMsg2;
         strMsg2.Format(_T("Station: %s\nDirection: %s\nSkew: %s"), FormatStation(pDisplayUnits->GetStationFormat(), station), FormatDirection(direction), FormatAngle(objSkew));

         CString strConnectionTip;
         if (pPier->IsBoundaryPier())
         {
            strConnectionTip.Format(_T("Boundary Condition: %s"), CPierData2::AsString(pPier->GetBoundaryConditionType(), bNoDeck));
         }
         else
         {
            strConnectionTip.Format(_T("Boundary Condition: %s"), CPierData2::AsString(pPier->GetSegmentConnectionType()));
         }

         CString strMsg = strMsg1 + _T("\n\n") + strMsg2 + _T("\n") + strConnectionTip;

         CString strModelType(pPier->GetPierModelType() == pgsTypes::pmtIdealized ? _T("Idealized") : _T("Physical"));
         strMsg += _T("\nModel Type: ") + strModelType;

         EventIndexType eventIdx = pTimelineMgr->GetPierErectionEventIndex(pierID);

         CString strEvent;
         if (eventIdx != INVALID_INDEX)
         {
            const CTimelineEvent* pTimelineEventData = pTimelineMgr->GetEventByIndex(eventIdx);

            strEvent.Format(_T("Erection: Event %d: %s"), LABEL_EVENT(eventIdx), pTimelineEventData->GetDescription());
         }
         else
         {
            strEvent.Format(_T("Erection: Erection event not defined"));
         }

         strMsg += _T("\n") + strEvent;

#if defined _DEBUG
         CString strDebugMsg;
         strDebugMsg.Format(_T("\n\nID: %d"), pPier->GetID());
         strMsg += strDebugMsg;
#endif

         doCenterLine->SetToolTipText(strMsg);
         doCenterLine->SetMaxTipWidth(TOOLTIP_WIDTH);
         doCenterLine->SetTipDisplayTime(TOOLTIP_DURATION);

         doCenterLine->SetID(pierIdx);
         PierDisplayObjectInfo* pInfo = new PierDisplayObjectInfo(pierIdx, PIER_DISPLAY_LIST);
         doCenterLine->SetItemData((void*)pInfo, true);

         // get the connectors from the line
         CComQIPtr<iConnector> connector(doCenterLine);
         CComQIPtr<iPlug> startPlug, endPlug;
         connector->GetStartPlug(&startPlug);
         connector->GetEndPlug(&endPlug);

         // connect the line to the points
         DWORD dwCookie;
         connectable1->Connect(0, atByID, startPlug, &dwCookie);
         connectable2->Connect(0, atByID, endPlug, &dwCookie);

         // Register an event sink with the pier centerline display object so that we can handle dbl-clicks
         // on the piers differently then a general dbl-click
         CPierDisplayObjectEvents* pEvents = new CPierDisplayObjectEvents(pierIdx,
            pBridgeDesc,
            pFrame);
         CComPtr<iDisplayObjectEvents> events;
         events.Attach((iDisplayObjectEvents*)pEvents->GetInterface(&IID_iDisplayObjectEvents));

         CComQIPtr<iDisplayObject, &IID_iDisplayObject> dispObj(doCenterLine);
         dispObj->RegisterEventSink(events);

         // create line drawing strategies... use a compound strategy so
         // we can draw both the centerline and the pseudo-outline of the pier
         CComPtr<iCompoundDrawLineStrategy> strategy;
         strategy.CoCreateInstance(CLSID_CompoundDrawLineStrategy);

         // strategy for pseudo-outline of the pier
         CComPtr<iExtRectangleDrawLineStrategy> strategy_pier;
         strategy_pier.CoCreateInstance(CLSID_ExtRectangleDrawLineStrategy);
         strategy_pier->SetColor(PIER_BORDER_COLOR);
         strategy_pier->SetFillColor(PIER_FILL_COLOR);
         strategy_pier->DoFill(TRUE);

         // make the pier outline just a bit wider
         SpanIndexType prev_span_idx = pierIdx - 1;
         SpanIndexType next_span_idx = pierIdx == nPiers - 1 ? INVALID_INDEX : pierIdx;

         Float64 left_offset = 0;
         Float64 right_offset = 0;

         CPierData2::PierConnectionFlags conFlag = pPier->IsConnectionDataAvailable();

         if (pPier->GetPierModelType() == pgsTypes::pmtIdealized)
         {
            if (CPierData2::pcfBothFaces==conFlag || CPierData2::pcfBackOnly == conFlag)
            {
               ConnectionLibraryEntry::BearingOffsetMeasurementType left_brg_offset_measure_type;
               pPier->GetBearingOffset(pgsTypes::Back, &left_offset, &left_brg_offset_measure_type);
               if (left_brg_offset_measure_type == ConnectionLibraryEntry::NormalToPier)
               {
                  left_offset /= cos(skew);
               }
            }

            if (CPierData2::pcfBothFaces == conFlag || CPierData2::pcfAheadOnly == conFlag)
            {
               ConnectionLibraryEntry::BearingOffsetMeasurementType right_brg_offset_measure_type;
               pPier->GetBearingOffset(pgsTypes::Ahead, &right_offset, &right_brg_offset_measure_type);
               if (right_brg_offset_measure_type == ConnectionLibraryEntry::NormalToPier)
               {
                  right_offset /= cos(skew);
               }
            }
         }
         else
         {
            if (CPierData2::pcfBothFaces == conFlag || CPierData2::pcfBackOnly == conFlag)
            {
               left_offset = pPier->GetXBeamWidth() / 2;
            }

            if (CPierData2::pcfBothFaces == conFlag || CPierData2::pcfAheadOnly == conFlag)
            {
               right_offset = pPier->GetXBeamWidth() / 2;
            }
         }

         left_offset = (IsZero(left_offset) ? right_offset / 2 : left_offset);
         right_offset = (IsZero(right_offset) ? left_offset / 2 : right_offset);

         if (pPier->GetPierModelType() == pgsTypes::pmtIdealized)
         {
            left_offset *= 1.05;
            right_offset *= 1.05;
         }

         strategy_pier->SetLeftOffset(left_offset);
         strategy_pier->SetRightOffset(right_offset);

         // Draw the piers so that the are wider than the width of the girders... compute the overhang
         // from the left and right exterior girder
         GroupIndexType backGroupIdx, aheadGroupIdx;
         pBridge->GetGirderGroupIndex(pierIdx, &backGroupIdx, &aheadGroupIdx);

         // Left side of pier
         pgsPointOfInterest backPoi, aheadPoi;
         if (backGroupIdx != INVALID_INDEX)
         {
            CGirderKey girderKey(backGroupIdx, 0);
            backPoi = pPoi->GetPierPointOfInterest(girderKey, pierIdx);
         }

         if (aheadGroupIdx != INVALID_INDEX)
         {
            CGirderKey girderKey(aheadGroupIdx, 0);
            aheadPoi = pPoi->GetPierPointOfInterest(girderKey, pierIdx);
         }

         Float64 back_top_width = (backGroupIdx == INVALID_INDEX ? 0 : pGirder->GetTopWidth(backPoi));
         Float64 back_bot_width = (backGroupIdx == INVALID_INDEX ? 0 : pGirder->GetBottomWidth(backPoi));
         Float64 ahead_top_width = (aheadGroupIdx == INVALID_INDEX ? 0 : pGirder->GetTopWidth(aheadPoi));
         Float64 ahead_bot_width = (aheadGroupIdx == INVALID_INDEX ? 0 : pGirder->GetBottomWidth(aheadPoi));
         Float64 left_overhang = Max(back_top_width, back_bot_width, ahead_top_width, ahead_bot_width) / 2;
         left_overhang /= cos(fabs(skew));
         left_overhang *= 1.10;

         // Right Side of Pier
         if (backGroupIdx != INVALID_INDEX)
         {
            GirderIndexType nGirders = pBridge->GetGirderCount(backGroupIdx);
            CGirderKey girderKey(backGroupIdx, nGirders - 1);
            backPoi = pPoi->GetPierPointOfInterest(girderKey, pierIdx);
         }

         if (aheadGroupIdx != INVALID_INDEX)
         {
            GirderIndexType nGirders = pBridge->GetGirderCount(aheadGroupIdx);
            CGirderKey girderKey(aheadGroupIdx, nGirders - 1);
            aheadPoi = pPoi->GetPierPointOfInterest(girderKey, pierIdx);
         }

         back_top_width = (backGroupIdx == INVALID_INDEX ? 0 : pGirder->GetTopWidth(backPoi));
         back_bot_width = (backGroupIdx == INVALID_INDEX ? 0 : pGirder->GetBottomWidth(backPoi));
         ahead_top_width = (aheadGroupIdx == INVALID_INDEX ? 0 : pGirder->GetTopWidth(aheadPoi));
         ahead_bot_width = (aheadGroupIdx == INVALID_INDEX ? 0 : pGirder->GetBottomWidth(aheadPoi));
         Float64 right_overhang = Max(back_top_width, back_bot_width, ahead_top_width, ahead_bot_width) / 2;
         right_overhang /= cos(fabs(skew));
         right_overhang *= 1.10;

         strategy_pier->SetStartExtension(left_overhang);
         strategy_pier->SetEndExtension(right_overhang);

         strategy_pier->SetStartSkew(-skew);
         strategy_pier->SetEndSkew(-skew);

         // this strategy doubles as a gravity well.. get its interface and give it to 
         // the line display object. this will make the entire pier the clickable part
         // of the display object
         strategy_pier->PerimeterGravityWell(TRUE);
         CComQIPtr<iGravityWellStrategy> gravity_well(strategy_pier);
         doCenterLine->SetGravityWellStrategy(gravity_well);
         doCenterLine->SetSelectionType(stAll);

         strategy->AddStrategy(strategy_pier);

         // pier centerline
         CComPtr<iExtRectangleDrawLineStrategy> strategy_centerline;
         strategy_centerline.CoCreateInstance(CLSID_ExtRectangleDrawLineStrategy);
         strategy_centerline->SetColor(BLUE);
         strategy_centerline->SetLineStyle(lsCenterline);
         strategy_centerline->SetStartExtension(left_overhang);
         strategy_centerline->SetEndExtension(right_overhang);
         strategy->AddStrategy(strategy_centerline);

         doCenterLine->SetDrawLineStrategy(strategy);

         Float64 alignmentOffset = pBridge->GetAlignmentOffset();
         alignmentOffset /= cos(skew);

         CComPtr<IPoint2d> ahead_point;
         pAlignment->GetPoint(station, -alignmentOffset, direction, pgsTypes::pcGlobal, &ahead_point);

         CComPtr<IPoint2d> back_point;
         pAlignment->GetPoint(station, -alignmentOffset, direction, pgsTypes::pcGlobal, &back_point);

         if (settings & IDB_PV_LABEL_PIERS)
         {
            // pier label
            CComPtr<iTextBlock> doPierName;
            doPierName.CoCreateInstance(CLSID_TextBlock);

            CString strText;
            strText.Format(_T("%s"), LABEL_PIER_EX(pPier->IsAbutment(),pierIdx));

            doPierName->SetPosition(ahead_point);
            doPierName->SetTextAlign(TA_BASELINE | TA_CENTER);
            doPierName->SetText(strText);
            doPierName->SetTextColor(BLACK);
            doPierName->SetBkMode(OPAQUE);

            Float64 dir;
            direction->get_Value(&dir);
            long angle = long(1800.*dir / M_PI);
            angle = (900 < angle && angle < 2700) ? angle - 1800 : angle;
            doPierName->SetAngle(angle);
            label_display_list->AddDisplayObject(doPierName);

            // pier station
            CComPtr<iEditableUnitValueTextBlock> doStation;
            doStation.CoCreateInstance(CLSID_EditableUnitValueTextBlock);
            doStation->SetUnitSystem(docUnitSystem);
            doStation->SetDisplayUnitGroupName(_T("Station"));
            doStation->IsStation(true);
            doStation->SetValue(station);
            doStation->SetPosition(back_point);
            doStation->SetTextAlign(TA_TOP | TA_CENTER);
            doStation->SetTextColor(BLACK);
            doStation->SetBkMode(OPAQUE);
            doStation->SetAngle(angle);
            doStation->SetSelectionType(stAll);

            doStation->SetToolTipText(_T("Click to edit"));
            doStation->SetMaxTipWidth(TOOLTIP_WIDTH);
            doStation->SetTipDisplayTime(TOOLTIP_DURATION);

            CInplacePierStationEditEvents* pPierStationEvents = new CInplacePierStationEditEvents(pBroker, pierIdx);
            CComPtr<iDisplayObjectEvents> pierStationEvents;
            pierStationEvents.Attach((iDisplayObjectEvents*)pPierStationEvents->GetInterface(&IID_iDisplayObjectEvents));

            doStation->RegisterEventSink(pierStationEvents);

            label_display_list->AddDisplayObject(doStation);

            // connection
            Float64 right_slab_edge_offset = pBridge->GetRightSlabEdgeOffset(pierIdx);
            CComPtr<IPoint2d> connection_label_point;
            pAlignment->GetPoint(station, -right_slab_edge_offset / cos(skew), direction, pgsTypes::pcGlobal, &connection_label_point);

            // make the baseline of the connection text parallel to the alignment
            direction.Release();
            pAlignment->GetBearing(station, &direction);
            direction->get_Value(&dir);

            angle = long(1800.*dir / M_PI);
            angle = (900 < angle && angle < 2700) ? angle - 1800 : angle;

            CComPtr<iTextBlock> doConnection;
            doConnection.CoCreateInstance(CLSID_TextBlock);
            doConnection->SetSelectionType(stNone);
            doConnection->SetPosition(connection_label_point);
            doConnection->SetTextColor(CONNECTION_LABEL_COLOR);
            doConnection->SetBkMode(TRANSPARENT);
            doConnection->SetAngle(angle);

            if (pierIdx == 0) // first pier
            {
               doConnection->SetTextAlign(TA_TOP | TA_LEFT);
            }
            else if (pierIdx == nPiers - 1) // last pier
            {
               doConnection->SetTextAlign(TA_TOP | TA_RIGHT);
            }
            else // intermediate pier
            {
               doConnection->SetTextAlign(TA_TOP | TA_CENTER);
            }

            doConnection->SetText(GetConnectionString(pPier).c_str());

            doConnection->SetToolTipText(GetFullConnectionString(pPier).c_str());
            doConnection->SetMaxTipWidth(TOOLTIP_WIDTH);
            doConnection->SetTipDisplayTime(TOOLTIP_DURATION);

            // Register an event sink with the connection text display object so that we can handle double clicks
            // differently then a general double click
            CConnectionDisplayObjectEvents* pEvents = new CConnectionDisplayObjectEvents(pierIdx);
            CComPtr<iDisplayObjectEvents> events;
            events.Attach((iDisplayObjectEvents*)pEvents->GetInterface(&IID_iDisplayObjectEvents));

            CComQIPtr<iDisplayObject, &IID_iDisplayObject> dispObj(doConnection);
            dispObj->RegisterEventSink(events);

            label_display_list->AddDisplayObject(doConnection);

            if (firstPierIdx < pierIdx)
            {
               ATLASSERT(pierIdx != ALL_PIERS);
               // label span length

               Float64 span_length = station - last_station;

               CComPtr<IPoint2d> pntInSpan;
               pAlignment->GetPoint(last_station + span_length / 2, 0.00, direction, pgsTypes::pcGlobal, &pntInSpan);

               CComPtr<IDirection> dirParallel;
               pAlignment->GetBearing(last_station + span_length / 2, &dirParallel);

               dirParallel->get_Value(&dir);
               angle = long(1800.*dir / M_PI);
               angle = (900 < angle && angle < 2700) ? angle - 1800 : angle;

               CComPtr<iEditableUnitValueTextBlock> doSpanLength;
               doSpanLength.CoCreateInstance(CLSID_EditableUnitValueTextBlock);
               doSpanLength->SetUnitSystem(docUnitSystem);
               doSpanLength->SetDisplayUnitGroupName(_T("SpanLength"));
               doSpanLength->SetValue(span_length);
               doSpanLength->SetPosition(pntInSpan);
               doSpanLength->SetTextAlign(TA_BASELINE | TA_CENTER);
               doSpanLength->SetTextColor(BLACK);
               doSpanLength->SetBkMode(OPAQUE);
               doSpanLength->SetAngle(angle);
               doSpanLength->SetSelectionType(stAll);

               doSpanLength->SetToolTipText(_T("Click to edit"));
               doSpanLength->SetMaxTipWidth(TOOLTIP_WIDTH);
               doSpanLength->SetTipDisplayTime(TOOLTIP_DURATION);

               // Register an event sink with the text block object so that we can handle change events
               CInplaceSpanLengthEditEvents* pSpanLengthEvents = new CInplaceSpanLengthEditEvents(pBroker, pierIdx - 1);
               CComPtr<iDisplayObjectEvents> spanLengthEvents;
               spanLengthEvents.Attach((iDisplayObjectEvents*)pSpanLengthEvents->GetInterface(&IID_iDisplayObjectEvents));

               doSpanLength->RegisterEventSink(spanLengthEvents);

               label_display_list->AddDisplayObject(doSpanLength);
            }
         }

         // create a line display object for the pier centerline
         // this centerline differs from the one created above in that
         // it is an extension line from the center of the pier to the intersection
         // with the alignment. This line shows where the centerline of the pier (extended)
         // intersects the alignment.
         CComPtr<iLineDisplayObject> doCenterLine2;
         doCenterLine2.CoCreateInstance(CLSID_LineDisplayObject);

         // create a point display object for the center of the pier
         // add a socket to it
         CComPtr<iPointDisplayObject> doAlignment;
         doAlignment.CoCreateInstance(CLSID_PointDisplayObject);
         doAlignment->SetPosition(alignment_pt, FALSE, FALSE);
         CComQIPtr<iConnectable> connectable3(doAlignment);
         CComPtr<iSocket> socket3;
         connectable3->AddSocket(0, alignment_pt, &socket3);

         CComPtr<iPointDisplayObject> doBridge;
         doBridge.CoCreateInstance(CLSID_PointDisplayObject);
         doBridge->SetPosition(bridge_pt, FALSE, FALSE);
         CComQIPtr<iConnectable> connectable4(doBridge);
         CComPtr<iSocket> socket4;
         connectable4->AddSocket(0, bridge_pt, &socket4);

         // get the connectors from the line
         CComQIPtr<iConnector> connector2(doCenterLine2);
         startPlug.Release();
         endPlug.Release();
         connector2->GetStartPlug(&startPlug);
         connector2->GetEndPlug(&endPlug);

         // connect the line to the points
         connectable3->Connect(0, atByID, startPlug, &dwCookie);
         connectable4->Connect(0, atByID, endPlug, &dwCookie);

         CComPtr<iSimpleDrawLineStrategy> offset_line_strategy;
         offset_line_strategy.CoCreateInstance(CLSID_SimpleDrawLineStrategy);
         offset_line_strategy->SetColor(RGB(120, 120, 120));
         offset_line_strategy->SetLineStyle(lsCenterline);

         doCenterLine2->SetDrawLineStrategy(offset_line_strategy);


         display_list->AddDisplayObject(doLeft);
         display_list->AddDisplayObject(doAlignment);
         display_list->AddDisplayObject(doBridge);
         display_list->AddDisplayObject(doRight);
         display_list->AddDisplayObject(doCenterLine);
         display_list->AddDisplayObject(doCenterLine2);

         last_station = station;
      } // next pier
   } // next group
}

void CBridgePlanView::BuildTemporarySupportDisplayObjects()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   SupportIndexType nTS = pBridgeDesc->GetTemporarySupportCount();
   if ( nTS == 0 )
   {
      return;
   }

   CBridgeModelViewChildFrame* pFrame = GetFrame();

   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   CComPtr<iDisplayList> display_list;
   dispMgr->FindDisplayList(TEMPORARY_SUPPORT_DISPLAY_LIST,&display_list);
   display_list->Clear();

   CComPtr<iDisplayList> label_display_list;
   dispMgr->FindDisplayList(LABEL_DISPLAY_LIST,&label_display_list);

   CPGSDocBase* pDoc = (CPGSDocBase*)GetDocument();
   UINT settings = pDoc->GetBridgeEditorSettings();

   const CTimelineManager* pTimelineMgr = pBridgeDesc->GetTimelineManager();

   CComPtr<IDocUnitSystem> docUnitSystem;
   pDoc->GetDocUnitSystem(&docUnitSystem);

   GET_IFACE2_NOCHECK(pBroker,ITempSupport,pTemporarySupport); // only needed if there are temporary supports
   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2_NOCHECK(pBroker,IGirder,pGirder);
   GET_IFACE2_NOCHECK(pBroker,IRoadway,pAlignment);
   GET_IFACE2_NOCHECK(pBroker,IEAFDisplayUnits,pDisplayUnits);
   GET_IFACE2_NOCHECK(pBroker,IPointOfInterest,pPoi);

   GroupIndexType nGroups = pBridge->GetGirderGroupCount();
   GroupIndexType firstGroupIdx = (m_StartGroupIdx == ALL_GROUPS ? 0 : m_StartGroupIdx);
   GroupIndexType lastGroupIdx = (m_EndGroupIdx == ALL_GROUPS ? nGroups - 1 : m_EndGroupIdx);

   for ( SupportIndexType tsIdx = 0; tsIdx < nTS; tsIdx++ )
   {
      const CTemporarySupportData* pTS = pBridgeDesc->GetTemporarySupport(tsIdx);

      SpanIndexType spanIdx = pTS->GetSpan()->GetIndex();
      GroupIndexType grpIdx = pBridge->GetGirderGroupIndex(spanIdx);

      if (grpIdx < firstGroupIdx || lastGroupIdx < grpIdx)
      {
         continue;
      }

      SupportIDType tsID = pTS->GetID();

      Float64 station = pTS->GetStation();

      pgsTypes::TemporarySupportType tsSupportType = pTS->GetSupportType();
      pgsTypes::TempSupportSegmentConnectionType segConnectionType = pTS->GetConnectionType();

      EventIndexType erectionEventIdx, removalEventIdx;
      pTimelineMgr->GetTempSupportEvents(tsID,&erectionEventIdx,&removalEventIdx);
      const CTimelineEvent* pErectionEvent = nullptr;
      const CTimelineEvent* pRemovalEvent  = nullptr;

      if ( erectionEventIdx != INVALID_INDEX )
      {
         pErectionEvent = pTimelineMgr->GetEventByIndex(erectionEventIdx);
      }

      if ( removalEventIdx != INVALID_INDEX )
      {
         pRemovalEvent  = pTimelineMgr->GetEventByIndex(removalEventIdx);
      }

      // get the control points
      CComPtr<IPoint2d> left,alignment_pt,bridge_pt,right;
      pTemporarySupport->GetControlPoints(tsIdx,pgsTypes::pcGlobal,&left,&alignment_pt,&bridge_pt,&right);

      CComPtr<IDirection> direction;
      pTemporarySupport->GetDirection(tsIdx,&direction);

      CComPtr<IAngle> objSkew;
      pTemporarySupport->GetSkew(tsIdx,&objSkew);
      Float64 skew;
      objSkew->get_Value(&skew);

      // create a point display object for the left side of the support
      // add a socket to it
      CComPtr<iPointDisplayObject> doLeft;
      doLeft.CoCreateInstance(CLSID_PointDisplayObject);
      doLeft->SetPosition(left,FALSE,FALSE);
      display_list->AddDisplayObject(doLeft);

      CComQIPtr<iConnectable> connectable1(doLeft);
      CComPtr<iSocket> socket1;
      connectable1->AddSocket(0,left,&socket1);

      // create a point display object for the right side of the support
      // add a socket to it
      CComPtr<iPointDisplayObject> doRight;
      doRight.CoCreateInstance(CLSID_PointDisplayObject);
      doRight->SetPosition(right,FALSE,FALSE);
      display_list->AddDisplayObject(doRight);

      CComQIPtr<iConnectable> connectable2(doRight);
      CComPtr<iSocket> socket2;
      connectable2->AddSocket(0,right,&socket2);

      // create a line display object for the centerline
      CComPtr<iLineDisplayObject> doCenterLine;
      doCenterLine.CoCreateInstance(CLSID_LineDisplayObject);
      doCenterLine->SetSelectionType(stAll);
      doCenterLine->SetID(tsID);

      TemporarySupportDisplayObjectInfo* pInfo = new TemporarySupportDisplayObjectInfo(tsIdx, TEMPORARY_SUPPORT_DISPLAY_LIST);
      doCenterLine->SetItemData((void*)pInfo, true);

      // Register an event sink with the centerline display object so that we can handle double clicks
      // on the temporary supports differently then a general double click
      CTemporarySupportDisplayObjectEvents* pEvents = new CTemporarySupportDisplayObjectEvents(pTS,pFrame);
      CComPtr<iDisplayObjectEvents> events;
      events.Attach((iDisplayObjectEvents*)pEvents->GetInterface(&IID_iDisplayObjectEvents));

      CComQIPtr<iDisplayObject,&IID_iDisplayObject> dispObj(doCenterLine);
      dispObj->RegisterEventSink(events);

      // get the connectors from the line
      CComQIPtr<iConnector> connector(doCenterLine);
      CComQIPtr<iPlug> startPlug, endPlug;
      connector->GetStartPlug(&startPlug);
      connector->GetEndPlug(&endPlug);

      // connect the line to the points
      DWORD dwCookie;
      connectable1->Connect(0,atByID,startPlug,&dwCookie);
      connectable2->Connect(0,atByID,endPlug,  &dwCookie);

      CString strMsg1;
      strMsg1.Format(_T("Double click to edit Temporary Support %d\nRight click for more options."),LABEL_TEMPORARY_SUPPORT(tsIdx));

      CString strMsg2;
      strMsg2.Format(_T("Type: %s\nStation: %s\nDirection: %s\nSkew: %s\nConnection Type: %s\nErection: Event %d, %s\nRemoval: Event %d, %s"),
                     tsSupportType == pgsTypes::ErectionTower ? _T("Erection Tower") : _T("Strong Back"),
                     FormatStation(pDisplayUnits->GetStationFormat(),station),
                     FormatDirection(direction),
                     FormatAngle(objSkew),
                     segConnectionType == pgsTypes::tsctClosureJoint ? _T("Closure Joint") : _T("Continuous Segment"),
                     LABEL_EVENT(erectionEventIdx),
                     pErectionEvent == nullptr ? _T("Erection event not defined") : pErectionEvent->GetDescription(),
                     LABEL_EVENT(removalEventIdx),
                     pRemovalEvent ==  nullptr ? _T("Removal event not defined") : pRemovalEvent->GetDescription());

      CString strMsg = strMsg1 + _T("\n\n") + strMsg2 + _T("\n");

#if defined _DEBUG
            CString strID;
            strID.Format(_T("\n\nID: %d"),tsID);

            strMsg += strID;
#endif // _DEBUG

      doCenterLine->SetToolTipText(strMsg);
      doCenterLine->SetMaxTipWidth(TOOLTIP_WIDTH);
      doCenterLine->SetTipDisplayTime(TOOLTIP_DURATION);
      display_list->AddDisplayObject(doCenterLine);

      // create line drawing strategies... use a compound strategy so
      // we can draw both the centerline and the pseudo-outline of the pier
      CComPtr<iCompoundDrawLineStrategy> strategy;
      strategy.CoCreateInstance(CLSID_CompoundDrawLineStrategy);

      // strategy for pseudo-outline of the pier
      CComPtr<iExtRectangleDrawLineStrategy> strategy_pier;
      strategy_pier.CoCreateInstance(CLSID_ExtRectangleDrawLineStrategy);

      if ( pTS->GetSupportType() == pgsTypes::ErectionTower )
      {
         strategy_pier->SetColor(TS_BORDER_COLOR);
         strategy_pier->SetFillColor(TS_FILL_COLOR);
      }
      else
      {
         strategy_pier->SetColor(SB_BORDER_COLOR);
         strategy_pier->SetFillColor(SB_FILL_COLOR);
      }
      strategy_pier->DoFill(TRUE);

      Float64 support_width = WBFL::Units::ConvertToSysUnits(6.0, WBFL::Units::Measure::Inch); // a reasonable default. No other data
      
      Float64 brg_offset;
      ConnectionLibraryEntry::BearingOffsetMeasurementType brg_offset_measurement_type;
      pTS->GetBearingOffset(&brg_offset,&brg_offset_measurement_type);
      if ( brg_offset_measurement_type == ConnectionLibraryEntry::NormalToPier )
      {
         brg_offset /= cos(skew);
      }

      Float64 left_offset  = 1.05*(brg_offset + support_width);
      Float64 right_offset = 1.05*(brg_offset + support_width);

      // need POI at intersection of TS and CL Girder
      pgsPointOfInterest leftPoi = pPoi->GetTemporarySupportPointOfInterest(CGirderKey(grpIdx,0),tsIdx);
      Float64 top_width = pGirder->GetTopWidth(leftPoi);
      Float64 bot_width = pGirder->GetBottomWidth(leftPoi);
      Float64 left_overhang = Max(top_width,bot_width);
      left_overhang /= cos(fabs(skew));
      left_overhang *= 1.10;

      GirderIndexType nGirders = pTS->GetSpan()->GetGirderCount();
      pgsPointOfInterest rightPoi = pPoi->GetTemporarySupportPointOfInterest(CGirderKey(grpIdx,nGirders-1),tsIdx);
      top_width = pGirder->GetTopWidth(rightPoi);
      bot_width = pGirder->GetBottomWidth(rightPoi);
      Float64 right_overhang = Max(top_width,bot_width);
      right_overhang /= cos(fabs(skew));
      right_overhang *= 1.10;

      strategy_pier->SetLeftOffset(left_offset);
      strategy_pier->SetRightOffset(right_offset);
      strategy_pier->SetStartExtension(left_overhang);
      strategy_pier->SetEndExtension(right_overhang);
      strategy_pier->SetStartSkew(-skew);
      strategy_pier->SetEndSkew(-skew);

      // this strategy doubles as a gravity well.. get its interface and give it to 
      // the line display object. this will make the entire pier the clickable part
      // of the display object
      strategy_pier->PerimeterGravityWell(TRUE);
      CComQIPtr<iGravityWellStrategy> gravity_well(strategy_pier);
      doCenterLine->SetGravityWellStrategy(gravity_well);
      strategy->AddStrategy(strategy_pier);

      // centerline
      CComPtr<iExtRectangleDrawLineStrategy> strategy_centerline;
      strategy_centerline.CoCreateInstance(CLSID_ExtRectangleDrawLineStrategy);
      strategy_centerline->SetColor(BLUE);
      strategy_centerline->SetLineStyle(lsCenterline);
      strategy_centerline->SetStartExtension(left_overhang);
      strategy_centerline->SetEndExtension(right_overhang);
      strategy->AddStrategy(strategy_centerline);

      doCenterLine->SetDrawLineStrategy(strategy);

      Float64 alignmentOffset = pBridgeDesc->GetAlignmentOffset();
      alignmentOffset /= cos(skew);

      CComPtr<IPoint2d> ahead_point;
      pAlignment->GetPoint(station,-alignmentOffset,direction,pgsTypes::pcGlobal,&ahead_point);

      CComPtr<IPoint2d> back_point;
      pAlignment->GetPoint(station,-alignmentOffset,direction,pgsTypes::pcGlobal,&back_point);

      if ( settings & IDB_PV_LABEL_PIERS )
      {
         // label
         CComPtr<iTextBlock> doPierName;
         doPierName.CoCreateInstance(CLSID_TextBlock);

         CString strText;
         strText.Format(_T("TS %d"),LABEL_TEMPORARY_SUPPORT(tsIdx));
         doPierName->SetPosition(ahead_point);
         doPierName->SetTextAlign(TA_BASELINE | TA_CENTER);
         doPierName->SetText(strText);
         doPierName->SetTextColor(BLACK);
         doPierName->SetBkMode(OPAQUE);

         Float64 dir;
         direction->get_Value(&dir);
         long angle = long(1800.*dir/M_PI);
         angle = (900 < angle && angle < 2700 ) ? angle-1800 : angle;
         doPierName->SetAngle(angle);
         label_display_list->AddDisplayObject(doPierName);

         // label temporary support station
         CComPtr<iEditableUnitValueTextBlock> doStation;
         doStation.CoCreateInstance(CLSID_EditableUnitValueTextBlock);
         doStation->SetUnitSystem(docUnitSystem);
         doStation->SetDisplayUnitGroupName(_T("Station"));
         doStation->IsStation(true);
         doStation->SetValue(station);
         doStation->SetPosition(back_point);
         doStation->SetTextAlign(TA_TOP | TA_CENTER);
         doStation->SetTextColor(BLACK);
         doStation->SetBkMode(OPAQUE);
         doStation->SetAngle(angle);
         doStation->SetSelectionType(stAll);
         
         doStation->SetToolTipText(_T("Click to edit"));
         doStation->SetMaxTipWidth(TOOLTIP_WIDTH);
         doStation->SetTipDisplayTime(TOOLTIP_DURATION);

         CInplaceTemporarySupportStationEditEvents* pTempSupportStationEvents = new CInplaceTemporarySupportStationEditEvents(pBroker,tsIdx);
         CComPtr<iDisplayObjectEvents> tsStationEvents;
         tsStationEvents.Attach((iDisplayObjectEvents*)pTempSupportStationEvents->GetInterface(&IID_iDisplayObjectEvents));

         doStation->RegisterEventSink(tsStationEvents);

         label_display_list->AddDisplayObject(doStation);

         // connection
         Float64 Xb = station - pBridgeDesc->GetPier(0)->GetStation();
         Float64 left_slab_edge_offset = pBridge->GetLeftSlabEdgeOffset(Xb);
         CComPtr<IPoint2d> connection_label_point;
         pAlignment->GetPoint(station,-left_slab_edge_offset/cos(skew),direction,pgsTypes::pcGlobal,&connection_label_point);

         // make the baseline of the connection text parallel to the alignment
         direction.Release();
         pAlignment->GetBearing(station,&direction);
         direction->get_Value(&dir);

         angle = long(1800.*dir/M_PI);
         angle = (900 < angle && angle < 2700 ) ? angle-1800 : angle;

         CComPtr<iTextBlock> doConnection;
         doConnection.CoCreateInstance(CLSID_TextBlock);
         doConnection->SetSelectionType(stNone);
         doConnection->SetPosition(connection_label_point);
         if ( pTS->GetSupportType() == pgsTypes::ErectionTower )
         {
            doConnection->SetTextColor(TS_LABEL_COLOR);
         }
         else
         {
            doConnection->SetTextColor(SB_LABEL_COLOR);
         }

         doConnection->SetBkMode(TRANSPARENT);
         doConnection->SetAngle(angle);
         doConnection->SetTextAlign(TA_BOTTOM | TA_CENTER);

         doConnection->SetText(GetConnectionString(pTS).c_str());

         doConnection->SetToolTipText(GetFullConnectionString(pTS).c_str());
         doConnection->SetMaxTipWidth(TOOLTIP_WIDTH);
         doConnection->SetTipDisplayTime(TOOLTIP_DURATION);

         label_display_list->AddDisplayObject(doConnection);
      }
   }
}

void CBridgePlanView::BuildClosureJointDisplayObjects()
{
   CBridgeModelViewChildFrame* pFrame = GetFrame();

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   CComPtr<iDisplayList> display_list;
   dispMgr->FindDisplayList(CLOSURE_JOINT_DISPLAY_LIST,&display_list);
   display_list->Clear();

   // restart the display object ids
   m_ClosureJointIDs.clear();
   m_NextClosureJointID = 0;

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);

   // this are only used if there are closure joints
   GET_IFACE2_NOCHECK(pBroker,IBridge,pIBridge);
   GET_IFACE2_NOCHECK(pBroker,IGirder,pIGirder);
   GET_IFACE2_NOCHECK(pBroker,ITempSupport,pITemporarySupport);

   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   GroupIndexType nGroups = pBridgeDesc->GetGirderGroupCount();
   GroupIndexType firstGroupIdx = (m_StartGroupIdx == ALL_GROUPS ? 0 : m_StartGroupIdx);
   GroupIndexType lastGroupIdx = (m_EndGroupIdx == ALL_GROUPS ? nGroups - 1 : m_EndGroupIdx);
   for(GroupIndexType grpIdx = firstGroupIdx; grpIdx <= lastGroupIdx; grpIdx++)
   {
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(grpIdx);
      GirderIndexType nGirders = pGroup->GetGirderCount();
      for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
      {
         const CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);
         IndexType nClosures = pGirder->GetClosureJointCount();
         for ( IndexType cpIdx = 0; cpIdx < nClosures; cpIdx++ )
         {
            const CClosureJointData* pClosure = pGirder->GetClosureJoint(cpIdx);
            CSegmentKey closureKey(pClosure->GetClosureKey());

            const CPrecastSegmentData* pLeftSegment  = pClosure->GetLeftSegment();
            const CPrecastSegmentData* pRightSegment = pClosure->GetRightSegment();

            CSegmentKey leftSegmentKey(pLeftSegment->GetSegmentKey());
            CSegmentKey rightSegmentKey(pRightSegment->GetSegmentKey());

            Float64 top_width = pIGirder->GetTopWidth( pgsPointOfInterest(leftSegmentKey,0.0) );

            CComPtr<IDirection> objDirection;
            if( pClosure )
            {
               if ( pClosure->GetTemporarySupport() )
               {
                  pITemporarySupport->GetDirection(pClosure->GetTemporarySupport()->GetIndex(),&objDirection);
               }
               else
               {
                  pIBridge->GetPierDirection(pClosure->GetPier()->GetIndex(),&objDirection);
               }
            }
            else
            {
               pIBridge->GetPierDirection(pGirder->GetPier(pgsTypes::metStart)->GetIndex(),&objDirection);
            }

            Float64 direction;
            objDirection->get_Value(&direction);


            // get the segment geometry points
            // array index is pgsTypes::Back = back side of closure, pgsTypes::Ahead = ahead side of closure
            // we want left segment (back) end 2 and right segment (ahead) end 1 points to 
            // build the display object
            std::array<CComPtr<IPoint2d>, 2> pntSupport1,pntEnd1,pntBrg1,pntBrg2,pntEnd2,pntSupport2;
            pIGirder->GetSegmentEndPoints(leftSegmentKey,  pgsTypes::pcGlobal,&pntSupport1[pgsTypes::Back], &pntEnd1[pgsTypes::Back], &pntBrg1[pgsTypes::Back], &pntBrg2[pgsTypes::Back], &pntEnd2[pgsTypes::Back], &pntSupport2[pgsTypes::Back]);
            pIGirder->GetSegmentEndPoints(rightSegmentKey, pgsTypes::pcGlobal,&pntSupport1[pgsTypes::Ahead],&pntEnd1[pgsTypes::Ahead],&pntBrg1[pgsTypes::Ahead],&pntBrg2[pgsTypes::Ahead],&pntEnd2[pgsTypes::Ahead],&pntSupport2[pgsTypes::Ahead]);

            // create display objects for points at ends of segments
            // also add a socket for each point
            CComPtr<iPointDisplayObject> doEnd1, doEnd2;
            doEnd1.CoCreateInstance(CLSID_PointDisplayObject);
            doEnd1->SetPosition(pntEnd2[pgsTypes::Back],FALSE,FALSE); // end of left segment
            doEnd1->Visible(FALSE);
            CComQIPtr<iConnectable> connectable_end1(doEnd1);
            CComPtr<iSocket> socket_end1;
            connectable_end1->AddSocket(0,pntEnd2[pgsTypes::Back],&socket_end1);

            doEnd2.CoCreateInstance(CLSID_PointDisplayObject);
            doEnd2->SetPosition(pntEnd1[pgsTypes::Ahead],FALSE,FALSE); // start of right segment
            doEnd2->Visible(FALSE);
            CComQIPtr<iConnectable> connectable_end2(doEnd2);
            CComPtr<iSocket> socket_end2;
            connectable_end2->AddSocket(0,pntEnd1[pgsTypes::Ahead],&socket_end2);

            // create a display object for the closure
            CComPtr<iLineDisplayObject> doClosure;
            doClosure.CoCreateInstance(CLSID_LineDisplayObject);

            IDType ID = m_NextClosureJointID++;
            m_ClosureJointIDs.insert( std::make_pair(closureKey,ID) );
            doClosure->SetID(ID);
            SegmentDisplayObjectInfo* pInfo = new SegmentDisplayObjectInfo(closureKey,CLOSURE_JOINT_DISPLAY_LIST);
            doClosure->SetItemData((void*)pInfo,true);

            // Register an event sink with the display object so that we can handle double clicks
            // differently then a general double click
            CClosureJointDisplayObjectEvents* pEvents = new CClosureJointDisplayObjectEvents(closureKey,leftSegmentKey,rightSegmentKey,pFrame);
            CComPtr<iDisplayObjectEvents> events;
            events.Attach((iDisplayObjectEvents*)pEvents->GetInterface(&IID_iDisplayObjectEvents));

            CComQIPtr<iDisplayObject,&IID_iDisplayObject> dispObj(doClosure);
            dispObj->RegisterEventSink(events);

            // connect the closure display object to the display objects
            // at the segment ends
            CComQIPtr<iConnector> connector(doClosure);
            CComQIPtr<iPlug> startPlug, endPlug;
            connector->GetStartPlug(&startPlug);
            connector->GetEndPlug(&endPlug);

            DWORD dwCookie;
            connectable_end1->Connect(0,atByID,startPlug,&dwCookie);
            connectable_end2->Connect(0,atByID,endPlug,  &dwCookie);


            display_list->AddDisplayObject(doEnd1);
            display_list->AddDisplayObject(doEnd2);

            display_list->AddDisplayObject(doClosure);

            // drawing strategy
            // use a compound strategy so we can draw both centerline and the top flange rectangle
            CComPtr<iCompoundDrawLineStrategy> strategy;
            strategy.CoCreateInstance(CLSID_CompoundDrawLineStrategy);

            CComPtr<iExtRectangleDrawLineStrategy> strategy1;
            strategy1.CoCreateInstance(CLSID_ExtRectangleDrawLineStrategy);

            strategy1->SetLeftOffset(top_width/2);
            strategy1->SetRightOffset(top_width/2);

            CComPtr<IDirection> objLeftSegmentBearing, objRightSegmentBearing;
            pIGirder->GetSegmentDirection(leftSegmentKey, &objLeftSegmentBearing);
            pIGirder->GetSegmentDirection(rightSegmentKey,&objRightSegmentBearing);

            CComPtr<IDirection> objLeftNormal, objRightNormal;
            objLeftSegmentBearing->Increment(CComVariant(PI_OVER_2),&objLeftNormal);
            objRightSegmentBearing->Increment(CComVariant(PI_OVER_2),&objRightNormal);
            Float64 left_normal, right_normal;
            objLeftNormal->get_Value(&left_normal);
            objLeftNormal->get_Value(&right_normal);
            Float64 start_skew = direction - left_normal;
            Float64 end_skew   = direction   - right_normal;
            strategy1->SetStartSkew(start_skew);
            strategy1->SetEndSkew(end_skew);

            strategy1->SetColor(CLOSURE_BORDER_COLOR);
            strategy1->DoFill(TRUE);
            strategy1->SetFillColor(CLOSURE_FILL_COLOR);

            // this strategy doubles as a gravity well.. get its interface and give it to 
            // the line display object. this will make the entire top flange the clickable part
            // of the display object
            strategy1->PerimeterGravityWell(TRUE);
            CComQIPtr<iGravityWellStrategy> gravity_well(strategy1);
            doClosure->SetGravityWellStrategy(gravity_well);
            doClosure->SetSelectionType(stAll);

            strategy->AddStrategy(strategy1);

   #if defined _SHOW_CL_GIRDER
            strategy->AddStrategy(strategy_girder);
   #endif
            doClosure->SetDrawLineStrategy(strategy);
         }
      }
   }
}

void CBridgePlanView::BuildSpanDisplayObjects()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IBridge,pBridge);
   pgsTypes::SupportedDeckType deckType = pBridge->GetDeckType();
   if ( deckType == pgsTypes::sdtNone )
   {
      return;
   }

   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   CComPtr<iDisplayList> display_list;
   dispMgr->FindDisplayList(SPAN_DISPLAY_LIST,&display_list);

   display_list->Clear();

   GroupIndexType nGroups = pBridge->GetGirderGroupCount();
   GroupIndexType firstGroupIdx = (m_StartGroupIdx == ALL_GROUPS ? 0 : m_StartGroupIdx);
   GroupIndexType lastGroupIdx  = (m_EndGroupIdx  == ALL_GROUPS ? nGroups -1 : m_EndGroupIdx);
   for ( GroupIndexType groupIdx = firstGroupIdx; groupIdx <= lastGroupIdx; groupIdx++ )
   {
      SpanIndexType startSpanIdx, endSpanIdx;
      pBridge->GetGirderGroupSpans(groupIdx, &startSpanIdx, &endSpanIdx);
      for (SpanIndexType spanIdx = startSpanIdx; spanIdx <= endSpanIdx; spanIdx++)
      {
         CComPtr<IPoint2dCollection> points;
         pBridge->GetSpanPerimeter(spanIdx, 10, pgsTypes::pcGlobal, &points);

         CComPtr<IPolyShape> poly_shape;
         poly_shape.CoCreateInstance(CLSID_PolyShape);
         poly_shape->AddPoints(points);


         CComPtr<iPointDisplayObject> doPnt;
         doPnt.CoCreateInstance(CLSID_PointDisplayObject);
         CComPtr<IPoint2d> p;
         points->get_Item(0, &p);
         doPnt->SetPosition(p, FALSE, FALSE);

         CComPtr<iShapeDrawStrategy> strategy;
         strategy.CoCreateInstance(CLSID_ShapeDrawStrategy);
         CComQIPtr<IShape> shape(poly_shape);
         strategy->SetShape(shape);
         strategy->SetSolidLineColor(IsStructuralDeck(deckType) ? DECK_BORDER_COLOR : NONSTRUCTURAL_DECK_BORDER_COLOR);
         strategy->SetSolidFillColor(IsStructuralDeck(deckType) ? DECK_FILL_COLOR : NONSTRUCTURAL_DECK_FILL_COLOR);
         strategy->DoFill(TRUE);
         doPnt->SetDrawingStrategy(strategy);

         doPnt->SetSelectionType(stAll);
         doPnt->SetID(spanIdx);

         SpanDisplayObjectInfo* pInfo = new SpanDisplayObjectInfo(spanIdx, SPAN_DISPLAY_LIST);
         doPnt->SetItemData((void*)pInfo, true);

         CComPtr<iShapeGravityWellStrategy> gravity_well;
         gravity_well.CoCreateInstance(CLSID_ShapeGravityWellStrategy);
         gravity_well->SetShape(shape);

         doPnt->SetGravityWellStrategy(gravity_well);

         CBridgePlanViewSpanDisplayObjectEvents* pEvents = new CBridgePlanViewSpanDisplayObjectEvents(spanIdx, m_pFrame);
         CComPtr<iDisplayObjectEvents> events;
         events.Attach((iDisplayObjectEvents*)pEvents->GetInterface(&IID_iDisplayObjectEvents));

         CComQIPtr<iDisplayObject, &IID_iDisplayObject> dispObj(doPnt);
         dispObj->RegisterEventSink(events);

         CString strMsg(_T("Double click to edit span.\nRight click for more options."));

         dispObj->SetToolTipText(strMsg);
         dispObj->SetMaxTipWidth(TOOLTIP_WIDTH);
         dispObj->SetTipDisplayTime(TOOLTIP_DURATION);

         display_list->AddDisplayObject(doPnt);
      } // next span
   } // next group
}

void CBridgePlanView::BuildSlabDisplayObjects()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker, IBridge, pBridge);
   if (pBridge->GetDeckType() == pgsTypes::sdtNone)
   {
      return;
   }

   GET_IFACE2(pBroker, IEAFDisplayUnits, pDisplayUnits);

   GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CDeckDescription2* pDeck = pBridgeDesc->GetDeckDescription();

   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   CComPtr<iDisplayList> display_list;
   dispMgr->FindDisplayList(SLAB_DISPLAY_LIST, &display_list);

   display_list->Clear();

   // setup the display object for the slab. use a composite object
   // and composite it of the individual deck casting regions
   CComPtr<iCompositeDisplayObject> doSlab;
   doSlab.CoCreateInstance(CLSID_CompositeDisplayObject);
   doSlab->SetSelectionType(stAll);
   doSlab->SetID(DECK_ID);
   display_list->AddDisplayObject(doSlab);

   // put some item data on the slab object for others to use
   DeckDisplayObjectInfo* pInfo = new DeckDisplayObjectInfo(DECK_ID, SLAB_DISPLAY_LIST);
   doSlab->SetItemData((void*)pInfo, true);

   // deal with events at the composite display object level
   CPGSDocBase* pDoc = (CPGSDocBase*)GetDocument();
   CBridgePlanViewSlabDisplayObjectEvents* pEvents = new CBridgePlanViewSlabDisplayObjectEvents(pDoc, pBroker, m_pFrame, true);
   CComPtr<iDisplayObjectEvents> events;
   events.Attach((iDisplayObjectEvents*)pEvents->GetInterface(&IID_iDisplayObjectEvents));

   CComQIPtr<iDisplayObject, &IID_iDisplayObject> dispObj(doSlab);
   dispObj->RegisterEventSink(events);


   // display object tool tip messages
   CString strMsg1(_T("Double click to edit deck.\nRight click for more options."));
   CString strMsg2;
   if (pDeck->GetDeckType() != pgsTypes::sdtNone)
   {
      strMsg2.Format(_T("\n\nDeck: %s\nSlab Thickness: %s\nf'c: %s"),
         GetDeckTypeName(pDeck->GetDeckType()),
         FormatDimension(pDeck->GrossDepth, pDisplayUnits->GetComponentDimUnit()),
         FormatDimension(pDeck->Concrete.Fc, pDisplayUnits->GetStressUnit())
      );
   }

   CString strMsg3;
   Float64 overlay_weight = pBridge->GetOverlayWeight();
   if (pBridge->HasOverlay())
   {
      strMsg3.Format(_T("\n\n%s: %s"),
         pBridge->IsFutureOverlay() ? _T("Future Overlay") : _T("Overlay"),
         FormatDimension(overlay_weight, pDisplayUnits->GetOverlayWeightUnit()));
   }

   CString strBaseMsg = strMsg1 + strMsg2 + strMsg3;

   // build display objects for each deck casting region
   GroupIndexType nGroups = pBridge->GetGirderGroupCount();
   GroupIndexType firstGroupIdx = (m_StartGroupIdx == ALL_GROUPS ? 0 : m_StartGroupIdx);
   GroupIndexType lastGroupIdx = (m_EndGroupIdx == ALL_GROUPS ? nGroups - 1 : m_EndGroupIdx);

   IndexType nRegions = pBridge->GetDeckCastingRegionCount();
   for(IndexType regionIdx = 0; regionIdx < nRegions; regionIdx++)
   {
      for (GroupIndexType grpIdx = firstGroupIdx; grpIdx <= lastGroupIdx; grpIdx++)
      {
         SpanIndexType firstSpanIdx, lastSpanIdx;
         pBridge->GetGirderGroupSpans(grpIdx, &firstSpanIdx, &lastSpanIdx);

         PierIndexType startPierIdx, endPierIdx;
         Float64 Xstart, Xend;
         CCastingRegion::RegionType regionType;
         IndexType sequenceIdx;
         pBridge->GetDeckCastingRegionLimits(regionIdx, &startPierIdx, &Xstart, &endPierIdx, &Xend, &regionType, &sequenceIdx);

         IndexType nPoints = 10;
         CComPtr<IPoint2dCollection> points;
         pBridge->GetDeckCastingRegionPerimeter(regionIdx, firstSpanIdx, lastSpanIdx, nPoints, pgsTypes::pcGlobal, &regionType, &sequenceIdx, nullptr, &points);

         COLORREF deck_fill_color = (regionType == CCastingRegion::Span ? DECK_FILL_POS_MOMENT_REGION_COLOR : DECK_FILL_NEG_MOMENT_REGION_COLOR);

         CComPtr<IPolyShape> poly_shape;
         poly_shape.CoCreateInstance(CLSID_PolyShape);
         poly_shape->AddPoints(points);

         CComPtr<iPointDisplayObject> doDeckRegion;
         doDeckRegion.CoCreateInstance(CLSID_PointDisplayObject);
         CComPtr<IPoint2d> p;
         points->get_Item(0, &p);
         doDeckRegion->SetPosition(p, FALSE, FALSE);
         doDeckRegion->SetSelectionType(stAll);

         CComPtr<iShapeDrawStrategy> strategy;
         strategy.CoCreateInstance(CLSID_ShapeDrawStrategy);
         CComQIPtr<IShape> shape(poly_shape);
         strategy->SetShape(shape);
         strategy->SetSolidLineColor(DECK_BORDER_COLOR);
         strategy->SetSolidFillColor(deck_fill_color);
         strategy->DoFill(TRUE);
         doDeckRegion->SetDrawingStrategy(strategy);

         CComPtr<iShapeGravityWellStrategy> gravity_well;
         gravity_well.CoCreateInstance(CLSID_ShapeGravityWellStrategy);
         gravity_well->SetShape(shape);

         doDeckRegion->SetGravityWellStrategy(gravity_well);

         CString strMsg;
         strMsg.Format(_T("%s\n\nRegion %d\nSequence %d"), strBaseMsg, LABEL_INDEX(regionIdx), LABEL_INDEX(sequenceIdx));

         doDeckRegion->SetToolTipText(strMsg);
         doDeckRegion->SetMaxTipWidth(TOOLTIP_WIDTH);
         doDeckRegion->SetTipDisplayTime(TOOLTIP_DURATION);

         doSlab->AddDisplayObject(doDeckRegion);
      } // next group
   } // next region
}

void CBridgePlanView::BuildSectionCutDisplayObjects()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   CComPtr<iDisplayList> display_list;
   dispMgr->FindDisplayList(SECTION_CUT_DISPLAY_LIST,&display_list);

   CComPtr<iDisplayObjectFactory> factory;
   dispMgr->GetDisplayObjectFactory(0, &factory);

   CComPtr<iDisplayObject> disp_obj;
   factory->Create(CBridgeSectionCutDisplayImpl::ms_Format,nullptr,&disp_obj);

   CComPtr<iDisplayObjectEvents> sink;
   disp_obj->GetEventSink(&sink);

   disp_obj->SetSelectionType(stAll);

   CComQIPtr<iPointDisplayObject,&IID_iPointDisplayObject> point_disp(disp_obj);
   point_disp->SetMaxTipWidth(TOOLTIP_WIDTH);
   point_disp->SetToolTipText(_T("Drag me along the alignment to move section cut.\nDouble click to enter the cut station\nPress CTRL + -> to move ahead\nPress CTRL + <- to move back"));
   point_disp->SetTipDisplayTime(TOOLTIP_DURATION);

   GET_IFACE2_NOCHECK(pBroker,IRoadway,pRoadway); // this interface is simply stored in the section_cut_strategy object.. no usage here
   GET_IFACE2(pBroker,IBridge,pBridge);
   CComQIPtr<iBridgeSectionCutDrawStrategy,&IID_iBridgeSectionCutDrawStrategy> section_cut_strategy(sink);
   section_cut_strategy->Init(m_pFrame, point_disp, pRoadway, pBridge, m_pFrame);
   section_cut_strategy->SetColor(CUT_COLOR);

   point_disp->SetID(SECTION_CUT_ID);

   display_list->Clear();
   UpdateSectionCut(point_disp,FALSE);
   display_list->AddDisplayObject(disp_obj);

   GroupIndexType nGroups = pBridge->GetGirderGroupCount();
   GroupIndexType firstGroupIdx = (m_StartGroupIdx == ALL_GROUPS ? 0 : m_StartGroupIdx);
   GroupIndexType lastGroupIdx = (m_EndGroupIdx == ALL_GROUPS ? nGroups - 1 : m_EndGroupIdx);

   SpanIndexType startSpanIdx, dummySpanIdx, endSpanIdx;
   pBridge->GetGirderGroupSpans(firstGroupIdx, &startSpanIdx, &dummySpanIdx);
   pBridge->GetGirderGroupSpans(lastGroupIdx, &dummySpanIdx, &endSpanIdx);
   PierIndexType startPierIdx = (PierIndexType)startSpanIdx;
   PierIndexType endPierIdx   = (PierIndexType)(endSpanIdx+1);
   Float64 first_station = pBridge->GetPierStation(startPierIdx);
   Float64 last_station  = pBridge->GetPierStation(endPierIdx);
   Float64 cut_station = m_pFrame->GetCurrentCutLocation();

   if ( !InRange(first_station,cut_station,last_station) )
   {
      m_pFrame->InvalidateCutLocation();
      UpdateSectionCut();
   }
}

void CBridgePlanView::BuildNorthArrowDisplayObjects()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   CComPtr<iDisplayList> display_list;
   dispMgr->FindDisplayList(NORTH_ARROW_DISPLAY_LIST,&display_list);
   display_list->Clear();


   CComPtr<iNorthArrow> doNorth;
   doNorth.CoCreateInstance(CLSID_NorthArrow);

   CComPtr<iCoordinateMap> map;
   dispMgr->GetCoordinateMap(&map);

   CComQIPtr<iMapping> mapping(map);
   Float64 cx,cy,angle;
   mapping->GetRotation(&cx,&cy,&angle);
   Float64 direction = PI_OVER_2 + angle;
   doNorth->SetDirection(direction);

   display_list->AddDisplayObject(doNorth);
}

void CBridgePlanView::BuildDiaphragmDisplayObjects()
{
   // This method draws the CIP diaphragms. Precast diaphragms are not drawn
#pragma Reminder("UPDATE: this method assumes diaphragms are between girders")
   // CIP diaphragms can be cast between girders or within a girder (eg, in a U-beam)

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   CComPtr<iDisplayList> display_list;
   dispMgr->FindDisplayList(DIAPHRAGM_DISPLAY_LIST,&display_list);
   display_list->Clear();

   GET_IFACE2(pBroker,IBridge,pBridge);

   // only used if there are diaphragms
   GET_IFACE2_NOCHECK(pBroker,IRoadway,pAlignment);
   GET_IFACE2_NOCHECK(pBroker,IEAFDisplayUnits,pDisplayUnits);
   GET_IFACE2_NOCHECK(pBroker,IPointOfInterest,pPoi);

   GroupIndexType nGroups = pBridge->GetGirderGroupCount();
   GroupIndexType firstGroupIdx = (m_StartGroupIdx == ALL_GROUPS ? 0 : m_StartGroupIdx);
   GroupIndexType lastGroupIdx  = (m_EndGroupIdx  == ALL_GROUPS ? nGroups-1 : m_EndGroupIdx);
   for(GroupIndexType grpIdx = firstGroupIdx; grpIdx <= lastGroupIdx; grpIdx++)
   {
      SpanIndexType firstSpanIdx, lastSpanIdx;
      pBridge->GetGirderGroupSpans(grpIdx, &firstSpanIdx, &lastSpanIdx);
      for ( SpanIndexType spanIdx = firstSpanIdx; spanIdx <= lastSpanIdx; spanIdx++ )
      {
         GirderIndexType nGirders = pBridge->GetGirderCountBySpan(spanIdx);
         for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders-1; gdrIdx++ )
         {
            CSpanKey spanKey1(spanIdx,gdrIdx);
            CSpanKey spanKey2(spanIdx,gdrIdx+1);

            std::vector<IntermedateDiaphragm> left_diaphragms  = pBridge->GetCastInPlaceDiaphragms(spanKey1);
            std::vector<IntermedateDiaphragm> right_diaphragms = pBridge->GetCastInPlaceDiaphragms(spanKey2);

            std::vector<IntermedateDiaphragm>::iterator left_iter, right_iter;
            for ( left_iter = left_diaphragms.begin(), right_iter = right_diaphragms.begin(); 
                  left_iter != left_diaphragms.end() && right_iter != right_diaphragms.end(); 
                  left_iter++, right_iter++ )
            {
               IntermedateDiaphragm left_diaphragm  = *left_iter;
               IntermedateDiaphragm right_diaphragm = *right_iter;
            
               // Only add the diaphragm if it has width
               if (!IsZero(left_diaphragm.W) && !IsZero(right_diaphragm.W))
               {
                  pgsPointOfInterest poi = pPoi->ConvertSpanPointToPoi(spanKey1, left_diaphragm.Location);

                  Float64 station, offset;
                  pBridge->GetStationAndOffset(poi, &station, &offset);

                  CComPtr<IDirection> normal;
                  pAlignment->GetBearingNormal(station, &normal);
                  CComPtr<IPoint2d> pntLeft;
                  pAlignment->GetPoint(station, offset, normal, pgsTypes::pcGlobal, &pntLeft);

                  poi = pPoi->ConvertSpanPointToPoi(spanKey2, right_diaphragm.Location);
                  pBridge->GetStationAndOffset(poi, &station, &offset);
                  normal.Release();
                  pAlignment->GetBearingNormal(station, &normal);
                  CComPtr<IPoint2d> pntRight;
                  pAlignment->GetPoint(station, offset, normal, pgsTypes::pcGlobal, &pntRight);

                  // create a point on the left side of the diaphragm
                  CComPtr<iPointDisplayObject> doLeft;
                  doLeft.CoCreateInstance(CLSID_PointDisplayObject);
                  doLeft->SetPosition(pntLeft, FALSE, FALSE);
                  CComQIPtr<iConnectable> connectable1(doLeft);
                  CComPtr<iSocket> socket1;
                  connectable1->AddSocket(0, pntLeft, &socket1);

                  // create a point on the right side of the diaphragm
                  CComPtr<iPointDisplayObject> doRight;
                  doRight.CoCreateInstance(CLSID_PointDisplayObject);
                  doRight->SetPosition(pntRight, FALSE, FALSE);
                  CComQIPtr<iConnectable> connectable2(doRight);
                  CComPtr<iSocket> socket2;
                  connectable2->AddSocket(0, pntRight, &socket2);

                  // create a line for the diaphragm
                  CComPtr<iLineDisplayObject> doDiaphragmLine;
                  doDiaphragmLine.CoCreateInstance(CLSID_LineDisplayObject);

                  CComQIPtr<iConnector> connector(doDiaphragmLine);
                  CComQIPtr<iPlug> startPlug, endPlug;
                  connector->GetStartPlug(&startPlug);
                  connector->GetEndPlug(&endPlug);

                  // connect the line to the points
                  DWORD dwCookie;
                  connectable1->Connect(0, atByID, startPlug, &dwCookie);
                  connectable2->Connect(0, atByID, endPlug, &dwCookie);

                  CComPtr<iExtRectangleDrawLineStrategy> strategy;
                  strategy.CoCreateInstance(CLSID_ExtRectangleDrawLineStrategy);
                  strategy->SetColor(DIAPHRAGM_BORDER_COLOR);
                  strategy->SetFillColor(DIAPHRAGM_FILL_COLOR);
                  strategy->DoFill(TRUE);

                  Float64 width = (left_diaphragm.T + right_diaphragm.T) / 2;
                  strategy->SetLeftOffset(width / 2);
                  strategy->SetRightOffset(width / 2);

                  strategy->SetStartExtension(0);
                  strategy->SetEndExtension(0);

                  strategy->SetStartSkew(0);
                  strategy->SetEndSkew(0);

                  doDiaphragmLine->SetDrawLineStrategy(strategy);

                  CString strTip;
                  strTip.Format(_T("%s x %s intermediate diaphragm"), ::FormatDimension(left_diaphragm.T, pDisplayUnits->GetComponentDimUnit()), ::FormatDimension(left_diaphragm.H, pDisplayUnits->GetComponentDimUnit()));
                  doDiaphragmLine->SetMaxTipWidth(TOOLTIP_WIDTH);
                  doDiaphragmLine->SetTipDisplayTime(TOOLTIP_DURATION);
                  doDiaphragmLine->SetToolTipText(strTip);

                  CComQIPtr<iDisplayObject> dispObj(doDiaphragmLine);
                  display_list->AddDisplayObject(dispObj);
               }
	         }
         }
      } // next span
   } // next group
}

void CBridgePlanView::UpdateDrawingScale()
{
   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   CComPtr<iDisplayList> title_display_list;
   dispMgr->FindDisplayList(TITLE_DISPLAY_LIST,&title_display_list);

   CComPtr<iDisplayList> na_display_list;
   dispMgr->FindDisplayList(NORTH_ARROW_DISPLAY_LIST,&na_display_list);

   if ( title_display_list == nullptr )
   {
      return;
   }

   CDManipClientDC dc(this);

   // before scaling the drawing to fit, hide the title display objects
   // if they aren't hidden, they factor into the bounding box and they
   // mess up the scaling of the drawing.
   //
   // this is the best solution I've been able to come up with.
   title_display_list->HideDisplayObjects(true);
   na_display_list->HideDisplayObjects(true);

   ScaleToFit();

   title_display_list->HideDisplayObjects(false);
   na_display_list->HideDisplayObjects(false);
}

void CBridgePlanView::ClearSelection()
{
   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   dispMgr->ClearSelectedObjects();
}

DROPEFFECT CBridgePlanView::CanDrop(COleDataObject* pDataObject,DWORD dwKeyState,IPoint2d* point)
{
   // This override has to determine if the thing being dragged over it can
   // be dropped. In order to do that, it must unpackage the OleDataObject.
   //
   // The stuff in the data object is just from the display object. The display
   // objects need to be updated so that the client can attach an object to it
   // that knows how to package up domain specific information. At the same
   // time, this view needs to be able to get some domain specific hint 
   // as to the type of data that is going to be dropped.

   if ( pDataObject->IsDataAvailable(CBridgeSectionCutDisplayImpl::ms_Format) )
   {
      // need to peek at our object first and make sure it's coming from the local process
      // this is ugly because it breaks encapsulation of CBridgeSectionCutDisplayImpl
      CComPtr<iDragDataSource> source;               
      ::CoCreateInstance(CLSID_DragDataSource,nullptr,CLSCTX_ALL,IID_iDragDataSource,(void**)&source);
      source->SetDataObject(pDataObject);
      source->PrepareFormat(CBridgeSectionCutDisplayImpl::ms_Format);

      CWinThread* thread = ::AfxGetThread( );
      DWORD threadid = thread->m_nThreadID;

      DWORD threadl;
      // know (by voodoo) that the first member of this data source is the thread id
      source->Read(CBridgeSectionCutDisplayImpl::ms_Format,&threadl,sizeof(DWORD));

      if (threadl == threadid)
      {
        return DROPEFFECT_MOVE;
      }
   }

   return DROPEFFECT_NONE;
}

void CBridgePlanView::OnDropped(COleDataObject* pDataObject,DROPEFFECT dropEffect,IPoint2d* point)
{
   AfxMessageBox(_T("CBridgePlanView::OnDropped"));
}

std::_tstring CBridgePlanView::GetConnectionString(const CPierData2* pPierData)
{
   std::_tstring strConnection;
   bool bNoDeck = IsNonstructuralDeck(pPierData->GetBridgeDescription()->GetDeckDescription()->GetDeckType());
   if ( pPierData->IsBoundaryPier() )
   {
      pgsTypes::BoundaryConditionType connectionType = pPierData->GetBoundaryConditionType();

      switch( connectionType )
      {
      case pgsTypes::bctHinge:
         strConnection = _T("H");
         break;

      case pgsTypes::bctRoller:
         strConnection = _T("R");
         break;

      case pgsTypes::bctContinuousAfterDeck:
         if ( bNoDeck )
            strConnection = _T("C");
         else
            strConnection = _T("Ca");
         break;

      case pgsTypes::bctContinuousBeforeDeck:
         ATLASSERT(bNoDeck == false);
         strConnection = _T("Cb");
         break;

      case pgsTypes::bctIntegralAfterDeck:
         if ( bNoDeck)
            strConnection = _T("I");
         else
            strConnection = _T("Ia");
         break;

      case pgsTypes::bctIntegralBeforeDeck:
         ATLASSERT(bNoDeck == false);
         strConnection = _T("Ib");
         break;

      case pgsTypes::bctIntegralAfterDeckHingeBack:
         if ( bNoDeck )
            strConnection = _T("H I");
         else
            strConnection = _T("H Ia");
         break;

      case pgsTypes::bctIntegralBeforeDeckHingeBack:
         ATLASSERT(bNoDeck == false);
         strConnection = _T("H Ib");
         break;

      case pgsTypes::bctIntegralAfterDeckHingeAhead:
         if ( bNoDeck )
            strConnection = _T("I H");
         else
            strConnection = _T("Ia H");
         break;

      case pgsTypes::bctIntegralBeforeDeckHingeAhead:
         ATLASSERT(bNoDeck == false);
         strConnection = _T("Ib H");
         break;

      default:
         ATLASSERT(false); // who added a new connection type?
         strConnection = _T("?");
      }
   }
   else
   {
      pgsTypes::PierSegmentConnectionType segmentConnectionType = pPierData->GetSegmentConnectionType();
      switch(segmentConnectionType)
      {
      case pgsTypes::psctContinousClosureJoint:
         strConnection = _T("C-CJ");
         break;

      case pgsTypes::psctContinuousSegment:
         strConnection = _T("C");
         break;

      case pgsTypes::psctIntegralClosureJoint:
         strConnection = _T("I-CJ");
         break;

      case pgsTypes::psctIntegralSegment:
         strConnection = _T("I");
         break;

      default:
         ATLASSERT(false); // who added a new connection type?
         strConnection = _T("?");
      }
   }

   return strConnection;
}

std::_tstring CBridgePlanView::GetFullConnectionString(const CPierData2* pPierData)
{
   std::_tstring strConnection;
   if ( pPierData->IsBoundaryPier() )
   {
      bool bNoDeck = IsNonstructuralDeck(pPierData->GetBridgeDescription()->GetDeckDescription()->GetDeckType());
      pgsTypes::BoundaryConditionType connectionType = pPierData->GetBoundaryConditionType();
      return CPierData2::AsString(connectionType, bNoDeck);
   }
   else
   {
      pgsTypes::PierSegmentConnectionType connectionType = pPierData->GetSegmentConnectionType();
      return CPierData2::AsString(connectionType);
   }
}

std::_tstring CBridgePlanView::GetConnectionString(const CTemporarySupportData* pTS)
{
   pgsTypes::TemporarySupportType connectionType = pTS->GetSupportType();

   std::_tstring strConnection;
   switch( connectionType )
   {
   case pgsTypes::ErectionTower:
      strConnection = _T("ET");
      break;

   case pgsTypes::StrongBack:
      strConnection = _T("SB");
      break;

   default:
      ATLASSERT(false); // who added a new connection type?
   }

   return strConnection;
}

std::_tstring CBridgePlanView::GetFullConnectionString(const CTemporarySupportData* pTS)
{
   pgsTypes::TemporarySupportType connectionType = pTS->GetSupportType();

   std::_tstring strConnection;
   switch( connectionType )
   {
   case pgsTypes::ErectionTower:
      strConnection = _T("Erection Tower");
      break;

   case pgsTypes::StrongBack:
      strConnection = _T("Strong Back");
      break;

   default:
      ATLASSERT(false); // who added a new connection type?
   }

   return strConnection;
}

void CBridgePlanView::OnZoom()
{
   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   CComPtr<iTaskFactory> taskFactory;
   dispMgr->GetTaskFactory(&taskFactory);

   CComPtr<iTask> task;
   taskFactory->CreateZoomTask(this,nullptr,LIGHTSTEELBLUE,&task);

   dispMgr->SetTask(task);
}

void CBridgePlanView::OnScaleToFit()
{
   UpdateDrawingScale();
}

void CBridgePlanView::Select(const CSelection* pSelection)
{
   switch (pSelection->Type)
   {
   case CSelection::None:
      ClearSelection();
      break;

   case CSelection::Span:
      SelectSpan(pSelection->SpanIdx, true);
      break;

   case CSelection::Girder:
      SelectGirder(CGirderKey(pSelection->GroupIdx, pSelection->GirderIdx), true);
      break;

   case CSelection::Segment:
      SelectSegment(CSegmentKey(pSelection->GroupIdx, pSelection->GirderIdx, pSelection->SegmentIdx), true);
      break;

   case CSelection::ClosureJoint:
      SelectClosureJoint(CSegmentKey(pSelection->GroupIdx, pSelection->GirderIdx, pSelection->SegmentIdx), true);
      break;

   case CSelection::Pier:
      SelectPier(pSelection->PierIdx, true);
      break;

   case CSelection::Deck:
      SelectDeck(true);
      break;

   case CSelection::Alignment:
      SelectAlignment(true);
      break;

   case CSelection::LeftRailingSystem:
   case CSelection::RightRailingSystem:
      ClearSelection();
      break;

   case CSelection::TemporarySupport:
      SelectTemporarySupport(pSelection->tsID, true);
      break;

   default:
      ATLASSERT(FALSE); // is there a new type of object to be selected?
      ClearSelection();
      break;
   }
}
