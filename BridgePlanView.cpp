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

// BridgePlanView.cpp : implementation file
//

#pragma Reminder("UPDATE")
// Recently (2/2012) RAB added ID's to all girders, piers, segments, etc... these object ID's
// could be the display object id. currently this view keeps a mapping of key values to
// display object ids. this may not be necessary

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\resource.h"
#include "PGSuperAppPlugin\PGSuperApp.h"
#include "PGSuperDocBase.h"
#include "PGSuperUnits.h"
#include <IFace\DrawBridgeSettings.h>
#include "BridgePlanView.h"
#include "PGSuperColors.h"
#include "AlignmentDisplayObjectEvents.h"
#include "InplaceSpanLengthEditEvents.h"
#include "InplacePierStationEditEvents.h"
#include "PGSuperAppPlugin\InplaceTemporarySupportStationEditEvents.h"
#include "GirderDisplayObjectEvents.h"
#include "PierDisplayObjectEvents.h"
#include "PGSuperAppPlugin\TemporarySupportDisplayObjectEvents.h"
#include "PGSuperAppPlugin\ClosureJointDisplayObjectEvents.h"
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

#include <PgsExt\BridgeDescription2.h>
#include <WBFLDManip.h>
#include <WBFLDManipTools.h>

#include <Material\Material.h>

#include <EAF\EAFMenu.h>

#include <PgsExt\ClosureJointData.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define TITLE_DISPLAY_LIST       0
#define ALIGNMENT_DISPLAY_LIST   1
#define PIER_DISPLAY_LIST        2
#define SEGMENT_DISPLAY_LIST     3
#define GIRDER_DISPLAY_LIST      4
#define BEARING_DISPLAY_LIST     5
#define SPAN_DISPLAY_LIST        6
#define SLAB_DISPLAY_LIST        7
#define LABEL_DISPLAY_LIST       8
#define SECTION_CUT_DISPLAY_LIST 9
#define NORTH_ARROW_DISPLAY_LIST 10
#define DIAPHRAGM_DISPLAY_LIST   11
#define TEMPORARY_SUPPORT_DISPLAY_LIST 12
#define CLOSURE_JOINT_DISPLAY_LIST 13

#define SECTION_CUT_ID -200
#define ALIGNMENT_ID   -300

//#define _SHOW_CL_GIRDER

/////////////////////////////////////////////////////////////////////////////
// CBridgePlanView

IMPLEMENT_DYNCREATE(CBridgePlanView, CDisplayView)

CBridgePlanView::CBridgePlanView()
{
   m_StartSpanIdx = 0;
   m_EndSpanIdx  = ALL_SPANS;
}

CBridgePlanView::~CBridgePlanView()
{
}


BEGIN_MESSAGE_MAP(CBridgePlanView, CDisplayView)
	//{{AFX_MSG_MAP(CBridgePlanView)
	ON_COMMAND(ID_EDIT_ROADWAY, OnEditRoadway)
	ON_COMMAND(ID_EDIT_BRIDGE, OnEditBridge)
	ON_COMMAND(ID_EDIT_DECK, OnEditDeck)
	ON_COMMAND(ID_VIEWSETTINGS, OnViewSettings)
	ON_WM_SIZE()
	ON_WM_CREATE()
   ON_WM_KEYDOWN()
   ON_WM_MOUSEWHEEL()
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
//   ON_COMMAND(ID_ZOOM,OnZoom)
//   ON_COMMAND(ID_SCALETOFIT,OnScaleToFit)
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
      return false;

   CComPtr<iDisplayObject> pDO = displayObjects.front().m_T;

   DeckDisplayObjectInfo* pInfo = NULL;
   pDO->GetItemData((void**)&pInfo);
   if ( pInfo == NULL || pInfo->ID != DECK_ID || pInfo->DisplayListID != SLAB_DISPLAY_LIST )
      return false;

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
      return false;

   CComPtr<iDisplayObject> pDO = displayObjects.front().m_T;

   if ( pDO->GetID() == ALIGNMENT_ID )
      return true;

   return false;
}

void CBridgePlanView::GetSpanRange(SpanIndexType* pStartSpanIdx,SpanIndexType* pEndSpanIdx)
{
   *pStartSpanIdx = m_StartSpanIdx;
   *pEndSpanIdx   = m_EndSpanIdx;
}

void CBridgePlanView::SetSpanRange(SpanIndexType startSpanIdx,SpanIndexType endSpanIdx)
{
   m_StartSpanIdx = startSpanIdx;
   m_EndSpanIdx  = endSpanIdx;

   m_pDocument->UpdateAllViews(NULL,HINT_BRIDGEVIEWSETTINGSCHANGED,NULL);
   m_pDocument->UpdateAllViews(NULL,HINT_BRIDGEVIEWSECTIONCUTCHANGED,NULL);
}

bool CBridgePlanView::GetSelectedSpan(SpanIndexType* pSpanIdx)
{
   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   DisplayObjectContainer displayObjects;
   dispMgr->GetSelectedObjects(&displayObjects);

   ATLASSERT(displayObjects.size() == 0 || displayObjects.size() == 1 );

   if ( displayObjects.size() == 0 )
      return false;

   CComPtr<iDisplayObject> pDO = displayObjects.front().m_T;

   SpanDisplayObjectInfo* pInfo;
   pDO->GetItemData((void**)&pInfo);
   if ( pInfo == NULL || pInfo->SpanIdx == ALL_SPANS || pInfo->DisplayListID != SPAN_DISPLAY_LIST )
      return false;

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
      return false;

   CComPtr<iDisplayObject> pDO = displayObjects.front().m_T;

   PierDisplayObjectInfo* pInfo;
   pDO->GetItemData((void**)&pInfo);

   if ( pInfo == NULL || pInfo->DisplayListID != PIER_DISPLAY_LIST )
      return false;

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
      dispMgr->SelectObject(pDO,bSelect);
   else
      dispMgr->ClearSelectedObjects();
}

void CBridgePlanView::SelectPier(PierIndexType pierIdx,bool bSelect)
{
   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   CComPtr<iDisplayObject> pDO;
   dispMgr->FindDisplayObject(pierIdx,PIER_DISPLAY_LIST,atByID,&pDO);

   if ( pDO )
      dispMgr->SelectObject(pDO,bSelect);
   else
      dispMgr->ClearSelectedObjects();
}

bool CBridgePlanView::GetSelectedGirder(CGirderKey* pGirderKey)
{
   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   DisplayObjectContainer displayObjects;
   dispMgr->GetSelectedObjects(&displayObjects);

   ATLASSERT(displayObjects.size() == 0 || displayObjects.size() == 1 );

   if ( displayObjects.size() == 0 )
      return false;

   CComPtr<iDisplayObject> pDO = displayObjects.front().m_T;

   GirderDisplayObjectInfo* pInfo = NULL;
   pDO->GetItemData((void**)&pInfo);

   if ( pInfo == NULL || pInfo->DisplayListID != GIRDER_DISPLAY_LIST )
      return false;

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
      dispMgr->SelectObject(pDO,bSelect);
   else
      dispMgr->ClearSelectedObjects();
}

bool CBridgePlanView::GetSelectedSegment(CSegmentKey* pSegmentKey)
{
   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   DisplayObjectContainer displayObjects;
   dispMgr->GetSelectedObjects(&displayObjects);

   ATLASSERT(displayObjects.size() == 0 || displayObjects.size() == 1 );

   if ( displayObjects.size() == 0 )
      return false;

   CComPtr<iDisplayObject> pDO = displayObjects.front().m_T;

   SegmentDisplayObjectInfo* pInfo = NULL;
   pDO->GetItemData((void**)&pInfo);

   if ( pInfo == NULL || pInfo->DisplayListID != SEGMENT_DISPLAY_LIST )
      return false;

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
      dispMgr->SelectObject(pDO,bSelect);
   else
      dispMgr->ClearSelectedObjects();
}

bool CBridgePlanView::GetSelectedClosureJoint(CSegmentKey* pClosureKey)
{
   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   DisplayObjectContainer displayObjects;
   dispMgr->GetSelectedObjects(&displayObjects);

   ATLASSERT(displayObjects.size() == 0 || displayObjects.size() == 1 );

   if ( displayObjects.size() == 0 )
      return false;

   CComPtr<iDisplayObject> pDO = displayObjects.front().m_T;

   SegmentDisplayObjectInfo* pInfo = NULL;
   pDO->GetItemData((void**)&pInfo);

   if ( pInfo == NULL || pInfo->DisplayListID != CLOSURE_JOINT_DISPLAY_LIST )
      return false;

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
      dispMgr->SelectObject(pDO,bSelect);
   else
      dispMgr->ClearSelectedObjects();
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

void CBridgePlanView::SelectTemporarySupport(SupportIDType tsID,bool bSelect)
{
   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   CComPtr<iDisplayObject> pDO;
   dispMgr->FindDisplayObject(tsID,TEMPORARY_SUPPORT_DISPLAY_LIST,atByID,&pDO);

   if ( pDO )
      dispMgr->SelectObject(pDO,bSelect);
   else
      dispMgr->ClearSelectedObjects();
}

void CBridgePlanView::OnInitialUpdate() 
{
   EnableToolTips();

   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   CPGSuperDocBase* pDoc = (CPGSuperDocBase*)GetDocument();
   CDisplayObjectFactory* factory = new CDisplayObjectFactory(pDoc);
   CComPtr<iDisplayObjectFactory> doFactory;
   doFactory.Attach((iDisplayObjectFactory*)factory->GetInterface(&IID_iDisplayObjectFactory));
   dispMgr->AddDisplayObjectFactory(doFactory);

   dispMgr->EnableLBtnSelect(TRUE);
   dispMgr->EnableRBtnSelect(TRUE);
   dispMgr->SetSelectionLineColor(SELECTED_OBJECT_LINE_COLOR);
   dispMgr->SetSelectionFillColor(SELECTED_OBJECT_FILL_COLOR);

   CDisplayView::SetMappingMode(DManip::Isotropic);

   // Setup display lists

   // section cut - add first so it is always on top
   CComPtr<iDisplayList> section_cut_list;
   ::CoCreateInstance(CLSID_DisplayList,NULL,CLSCTX_ALL,IID_iDisplayList,(void**)&section_cut_list);
   section_cut_list->SetID(SECTION_CUT_DISPLAY_LIST);
   dispMgr->AddDisplayList(section_cut_list);

   CComPtr<iDisplayList> label_list;
   ::CoCreateInstance(CLSID_DisplayList,NULL,CLSCTX_ALL,IID_iDisplayList,(void**)&label_list);
   label_list->SetID(LABEL_DISPLAY_LIST);
   dispMgr->AddDisplayList(label_list);

   CComPtr<iDisplayList> title_list;
   ::CoCreateInstance(CLSID_DisplayList,NULL,CLSCTX_ALL,IID_iDisplayList,(void**)&title_list);
   title_list->SetID(TITLE_DISPLAY_LIST);
   dispMgr->AddDisplayList(title_list);

   CComPtr<iDisplayList> alignment_list;
   ::CoCreateInstance(CLSID_DisplayList,NULL,CLSCTX_ALL,IID_iDisplayList,(void**)&alignment_list);
   alignment_list->SetID(ALIGNMENT_DISPLAY_LIST);
   dispMgr->AddDisplayList(alignment_list);

   CComPtr<iDisplayList> segment_list;
   ::CoCreateInstance(CLSID_DisplayList,NULL,CLSCTX_ALL,IID_iDisplayList,(void**)&segment_list);
   segment_list->SetID(SEGMENT_DISPLAY_LIST);
   dispMgr->AddDisplayList(segment_list);

   CComPtr<iDisplayList> girder_list;
   ::CoCreateInstance(CLSID_DisplayList,NULL,CLSCTX_ALL,IID_iDisplayList,(void**)&girder_list);
   girder_list->SetID(GIRDER_DISPLAY_LIST);
   dispMgr->AddDisplayList(girder_list);

   CComPtr<iDisplayList> diaphragm_list;
   ::CoCreateInstance(CLSID_DisplayList,NULL,CLSCTX_ALL,IID_iDisplayList,(void**)&diaphragm_list);
   diaphragm_list->SetID(DIAPHRAGM_DISPLAY_LIST);
   dispMgr->AddDisplayList(diaphragm_list);

   CComPtr<iDisplayList> pier_list;
   ::CoCreateInstance(CLSID_DisplayList,NULL,CLSCTX_ALL,IID_iDisplayList,(void**)&pier_list);
   pier_list->SetID(PIER_DISPLAY_LIST);
   dispMgr->AddDisplayList(pier_list);

   CComPtr<iDisplayList> closure_joint_list;
   ::CoCreateInstance(CLSID_DisplayList,NULL,CLSCTX_ALL,IID_iDisplayList,(void**)&closure_joint_list);
   closure_joint_list->SetID(CLOSURE_JOINT_DISPLAY_LIST);
   dispMgr->AddDisplayList(closure_joint_list);

   CComPtr<iDisplayList> temporary_support_list;
   ::CoCreateInstance(CLSID_DisplayList,NULL,CLSCTX_ALL,IID_iDisplayList,(void**)&temporary_support_list);
   temporary_support_list->SetID(TEMPORARY_SUPPORT_DISPLAY_LIST);
   dispMgr->AddDisplayList(temporary_support_list);

   CComPtr<iDisplayList> bearing_list;
   ::CoCreateInstance(CLSID_DisplayList,NULL,CLSCTX_ALL,IID_iDisplayList,(void**)&bearing_list);
   bearing_list->SetID(BEARING_DISPLAY_LIST);
   dispMgr->AddDisplayList(bearing_list);

   CComPtr<iDisplayList> span_list;
   ::CoCreateInstance(CLSID_DisplayList,NULL,CLSCTX_ALL,IID_iDisplayList,(void**)&span_list);
   span_list->SetID(SPAN_DISPLAY_LIST);
   dispMgr->AddDisplayList(span_list);

   CComPtr<iDisplayList> slab_list;
   ::CoCreateInstance(CLSID_DisplayList,NULL,CLSCTX_ALL,IID_iDisplayList,(void**)&slab_list);
   slab_list->SetID(SLAB_DISPLAY_LIST);
   dispMgr->AddDisplayList(slab_list);

   CComPtr<iDisplayList> north_arrow_list;
   ::CoCreateInstance(CLSID_DisplayList,NULL,CLSCTX_ALL,IID_iDisplayList,(void**)&north_arrow_list);
   north_arrow_list->SetID(NORTH_ARROW_DISPLAY_LIST);
   dispMgr->AddDisplayList(north_arrow_list);

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IBridge,pBridge);
   SpanIndexType nSpans = pBridge->GetSpanCount();
   m_EndSpanIdx = nSpans-1;

   CDisplayView::OnInitialUpdate();

   // OnInitialUpdate eventually calls OnUpdate... OnUpdate calls the
   // following two methods so there isn't any need to call them here
   //UpdateDisplayObjects();
   //UpdateDrawingScale();

   // Causes the child frame window to initalize the span range selection controls
   m_pFrame->InitSpanRange();
}

void CBridgePlanView::DoPrint(CDC* pDC, CPrintInfo* pInfo,CRect rcDraw)
{
   OnBeginPrinting(pDC, pInfo, rcDraw);
   OnPrepareDC(pDC);
   UpdateDrawingScale();
   OnDraw(pDC);
   OnEndPrinting(pDC, pInfo);
}

void CBridgePlanView::OnDraw(CDC* pDC)
{
   CDisplayView::OnDraw(pDC);

   if ( CWnd::GetFocus() == this && !pDC->IsPrinting() )
   {
      DrawFocusRect();
   }
}

/////////////////////////////////////////////////////////////////////////////
// CBridgePlanView diagnostics

#ifdef _DEBUG
void CBridgePlanView::AssertValid() const
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
	CDisplayView::AssertValid();
}

void CBridgePlanView::Dump(CDumpContext& dc) const
{
	CDisplayView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CBridgePlanView message handlers

void CBridgePlanView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
   CDisplayView::OnUpdate(pSender,lHint,pHint);

	if ( (lHint == 0)                               || 
        (lHint == HINT_BRIDGECHANGED)              || 
        (lHint == HINT_GIRDERFAMILYCHANGED)        ||
        (lHint == HINT_UNITSCHANGED)               ||  
        (lHint == HINT_BRIDGEVIEWSETTINGSCHANGED)  ||
        (lHint == HINT_GIRDERLABELFORMATCHANGED)  
      )
   {
      if ( lHint == HINT_BRIDGECHANGED )
      {
         CComPtr<IBroker> pBroker;
         EAFGetBroker(&pBroker);
         GET_IFACE2(pBroker,IBridge,pBridge);

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
            if ( (m_StartSpanIdx <= spanIdx && spanIdx <= m_EndSpanIdx) || // span in range
                 (m_EndSpanIdx == nPrevSpans-1 && spanIdx == nSpans-1) || // at end
                 (m_StartSpanIdx == 0 && spanIdx == INVALID_INDEX) // at start
               )
            {
               // new span is in the display range
               if ( pBridgeHint->bAdded )
               {
                  m_EndSpanIdx++;
               }
               else
               {
                  if ( spanIdx == m_StartSpanIdx && spanIdx != 0)
                     m_StartSpanIdx++; // span at start of range was removed, so make the range smaller
                  else
                     m_EndSpanIdx--; // span at within or at the end of the range was removed...
               }
            }
         }

         // Make sure we aren't displaying spans past the end of the bridge
         SpanIndexType nSpans = pBridge->GetSpanCount();
         m_EndSpanIdx = (nSpans <= m_EndSpanIdx ? nSpans-1 : m_EndSpanIdx);

         m_pFrame->InitSpanRange();
      }

      UpdateDisplayObjects();
      UpdateDrawingScale();
   }
   else if ( lHint == HINT_GIRDERCHANGED )
   {
      UpdateSegmentTooltips();
   }
   else if ( lHint == HINT_BRIDGEVIEWSECTIONCUTCHANGED )
   {
      UpdateSectionCut();
   }
   else if ( lHint == HINT_SELECTIONCHANGED )
   {
      CSelection* pSelection = (CSelection*)pHint;
      switch( pSelection->Type )
      {
      case CSelection::None:
         this->ClearSelection();
         break;

      case CSelection::Span:
         this->SelectSpan( pSelection->SpanIdx, true );
         break;

      case CSelection::Girder:
         this->SelectGirder( CSegmentKey(pSelection->GroupIdx,pSelection->GirderIdx,INVALID_INDEX),true);
         break;

      case CSelection::Segment:
         this->SelectSegment( CSegmentKey(pSelection->GroupIdx, pSelection->GirderIdx, pSelection->SegmentIdx), true );
         break;

      case CSelection::ClosureJoint:
         this->SelectClosureJoint( CSegmentKey(pSelection->GroupIdx, pSelection->GirderIdx, pSelection->SegmentIdx), true );
         break;

      case CSelection::Pier:
         this->SelectPier( pSelection->PierIdx, true );
         break;

      case CSelection::Deck:
         this->SelectDeck(true);
         break;

      case CSelection::Alignment:
         this->SelectAlignment(true);
         break;

      case CSelection::TemporarySupport:
         this->SelectTemporarySupport(pSelection->tsID,true);
         break;

      default:
         ATLASSERT(FALSE); // is there a new type of object to be selected?
         this->ClearSelection();
         break;
      }
   }
}

void CBridgePlanView::UpdateSegmentTooltips()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IMaterials,pMaterial);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeom);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);

   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

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
         SegmentIndexType nSegments = pGirder->GetSegmentCount();
         for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
         {
            CSegmentKey segmentKey(grpIdx,girderIdx,segIdx);

            const CPrecastSegmentData* pSegment = pGirder->GetSegment(segIdx);

            std::map<CSegmentKey,IDType>::iterator found = m_SegmentIDs.find(segmentKey);
            if ( found == m_SegmentIDs.end() )
               continue;

            IDType ID = (*found).second;

            CComPtr<iDisplayObject> pDO;
            display_list->FindDisplayObject(ID,&pDO);
            ATLASSERT(pDO != NULL);

            CComPtr<IDirection> direction;
            pBridge->GetSegmentBearing(segmentKey,&direction);

#pragma Reminder("UPDATE: girder/segment label in tool tip") // using generic text for now
            //CString strMsg1;
            //strMsg1.Format(_T("Double click to edit Span %d Girder %s\r\nRight click for more options."),LABEL_SPAN(spanIdx),LABEL_GIRDER(girderIdx));
            CString strMsg1(_T("Double click to edit.\r\nRight click for more options."));

            Float64 gdr_length, span_length;
            gdr_length  = pBridge->GetSegmentLength(segmentKey);
            span_length = pBridge->GetSegmentSpanLength(segmentKey);
            CString strMsg2;
            strMsg2.Format(_T("\r\n\r\nGirder: %s\r\nGirder Length: %s\r\nSpan Length: %s\r\n\r\n%s"),
                           pGirder->GetGirderName(),
                           FormatDimension(gdr_length,pDisplayUnits->GetSpanLengthUnit()),
                           FormatDimension(span_length,pDisplayUnits->GetSpanLengthUnit()),
                           FormatDirection(direction)
                           );

            Float64 fc  = pSegment->Material.Concrete.Fc;
            Float64 fci = pSegment->Material.Concrete.Fci;

            CString strMsg3;
            strMsg3.Format(_T("\r\n\r\n%s\r\nf'ci: %s\r\nf'c: %s"),
                           matConcrete::GetTypeName((matConcrete::Type)pMaterial->GetSegmentConcreteType(segmentKey),true).c_str(),
                           FormatDimension(fci,pDisplayUnits->GetStressUnit()),
                           FormatDimension(fc, pDisplayUnits->GetStressUnit())
                           );

            const matPsStrand* pStrand     = pMaterial->GetStrandMaterial(segmentKey,pgsTypes::Permanent);
            const matPsStrand* pTempStrand = pMaterial->GetStrandMaterial(segmentKey,pgsTypes::Temporary);

            StrandIndexType Ns, Nh, Nt, Nsd;
            Ns = pStrandGeom->GetNumStrands(segmentKey,pgsTypes::Straight);
            Nh = pStrandGeom->GetNumStrands(segmentKey,pgsTypes::Harped);
            Nt = pStrandGeom->GetNumStrands(segmentKey,pgsTypes::Temporary);
            Nsd= pStrandGeom->GetNumDebondedStrands(segmentKey,pgsTypes::Straight);
       
            std::_tstring harp_type(LABEL_HARP_TYPE(pStrandGeom->GetAreHarpedStrandsForcedStraight(segmentKey)));
	
	         CString strMsg4;
	         if ( pStrandGeom->GetMaxStrands(segmentKey,pgsTypes::Temporary) != 0 )
	         {
	            if ( Nsd == 0 )
	            {
	               strMsg4.Format(_T("\r\n\r\nStrand: %s\r\n# Straight: %2d\r\n# %s: %2d\r\n\r\n%s\r\n# Temporary: %2d"),
	                               pStrand->GetName().c_str(),Ns,harp_type.c_str(),Nh,pTempStrand->GetName().c_str(),Nt);
	            }
	            else
	            {
	               strMsg4.Format(_T("\r\n\r\nStrand: %s\r\n# Straight: %2d (%2d Debonded)\r\n# %s: %2d\r\n\r\n%s\r\n# Temporary: %2d"),
	                               pStrand->GetName().c_str(),Ns,Nsd,harp_type.c_str(),Nh,pTempStrand->GetName().c_str(),Nt);
	            }
	         }
	         else
	         {
	            if ( Nsd == 0 )
	            {
	               strMsg4.Format(_T("\r\n\r\nStrand: %s\r\n# Straight: %2d\r\n# %s: %2d"),
	                               pStrand->GetName().c_str(),Ns,harp_type.c_str(),Nh);
	            }
	            else
	            {
	               strMsg4.Format(_T("\r\n\r\nStrand: %s\r\n# Straight: %2d (%2d Debonded)\r\n# %s: %2d"),
	                               pStrand->GetName().c_str(),Ns,Nsd,harp_type.c_str(),Nh);
	            }
	         }


            CString strMsg = strMsg1 + strMsg2 + strMsg3 + strMsg4;

#if defined _DEBUG
            CString strSegID;
            strSegID.Format(_T("\r\n\r\nSegment ID: %d"),ID);

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

   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IMaterials,pMaterial);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeom);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);

   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

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
               continue;

            IDType ID = (*found).second;

            CComPtr<iDisplayObject> pDO;
            display_list->FindDisplayObject(ID,&pDO);
            ATLASSERT(pDO != NULL);

#pragma Reminder("UPDATE: girder/segment label in tool tip") // using generic text for now
            //CString strMsg1;
            //strMsg1.Format(_T("Double click to edit Span %d Girder %s\r\nRight click for more options."),LABEL_SPAN(spanIdx),LABEL_GIRDER(girderIdx));
            CString strMsg1(_T("Double click to edit.\r\nRight click for more options."));

            Float64 fc  = pClosureJoint->GetConcrete().Fc;
            Float64 fci = pClosureJoint->GetConcrete().Fci;

            CString strMsg2;
            strMsg2.Format(_T("\r\n\r\n%s\r\nf'ci: %s\r\nf'c: %s"),
                           matConcrete::GetTypeName((matConcrete::Type)pClosureJoint->GetConcrete().Type,true).c_str(),
                           FormatDimension(fci,pDisplayUnits->GetStressUnit()),
                           FormatDimension(fc, pDisplayUnits->GetStressUnit())
                           );

            CString strMsg = strMsg1 + strMsg2;

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
   pRoadway->GetPoint(station,0.00,bearing,&point);

   pntDO->SetPosition(point,bRedraw,FALSE);
}

void CBridgePlanView::OnSize(UINT nType, int cx, int cy) 
{
	CDisplayView::OnSize(nType, cx, cy);

   CRect rect;
   GetClientRect(&rect);
   rect.DeflateRect(15,15,15,15);

   CSize size = rect.Size();
   size.cx = Max(0L,size.cx);
   size.cy = Max(0L,size.cy);

   SetLogicalViewRect(MM_TEXT,rect);

   SetScrollSizes(MM_TEXT,size,CScrollView::sizeDefault,CScrollView::sizeDefault);

   UpdateDrawingScale();
}

void CBridgePlanView::HandleLButtonDown(UINT nFlags, CPoint logPoint)
{
   CBridgeModelViewChildFrame* pFrame = GetFrame();
   pFrame->ClearSelection();
}

void CBridgePlanView::HandleLButtonDblClk(UINT nFlags, CPoint logPoint) 
{
   GetFrame()->SendMessage(WM_COMMAND,ID_PROJECT_BRIDGEDESC,0);
}

void CBridgePlanView::HandleContextMenu(CWnd* pWnd,CPoint logPoint)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CPGSuperDocBase* pDoc = (CPGSuperDocBase*)GetDocument();
   CEAFMenu* pMenu = CEAFMenu::CreateContextMenu(pDoc->GetPluginCommandManager());
   pMenu->LoadMenu(IDR_BRIDGE_PLAN_CTX,NULL);

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

   std::map<IDType,IBridgePlanViewEventCallback*> callbacks = pDoc->GetBridgePlanViewCallbacks();
   std::map<IDType,IBridgePlanViewEventCallback*>::iterator iter;
   for ( iter = callbacks.begin(); iter != callbacks.end(); iter++ )
   {
      IBridgePlanViewEventCallback* callback = iter->second;
      callback->OnBackgroundContextMenu(pMenu);
   }


   pMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, logPoint.x, logPoint.y, this);
   delete pMenu;
}

void CBridgePlanView::OnEditRoadway() 
{
   ((CPGSuperDocBase*)GetDocument())->EditAlignmentDescription(EBD_ROADWAY);
}

void CBridgePlanView::OnEditBridge() 
{
   GetFrame()->SendMessage(WM_COMMAND,ID_PROJECT_BRIDGEDESC,0);
}

void CBridgePlanView::OnEditDeck() 
{
   ((CPGSuperDocBase*)GetDocument())->EditBridgeDescription(EBD_DECK);
}

void CBridgePlanView::OnViewSettings() 
{
   ((CPGSuperDocBase*)GetDocument())->EditBridgeViewSettings(VS_BRIDGE_PLAN);
}

void CBridgePlanView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
   if ( nChar == VK_LEFT || nChar == VK_RIGHT )
   {
      CComPtr<IBroker> pBroker;
      EAFGetBroker(&pBroker);
      GET_IFACE2(pBroker,IBridge,pBridge);

      CComPtr<iDisplayMgr> dispMgr;
      GetDisplayMgr(&dispMgr);
      DisplayObjectContainer selObjs;
      dispMgr->GetSelectedObjects(&selObjs);
      bool bSectionCutSelected = false;
      if ( 0 < selObjs.size() )
      {
         CComPtr<iDisplayObject> pDO = selObjs[0].m_T;
         if ( pDO->GetID() == SECTION_CUT_ID )
            bSectionCutSelected = true;
      }

      if ( ::GetKeyState(VK_CONTROL) < 0 || bSectionCutSelected )
      {
         // if control key is down... move the section cut

         CBridgeModelViewChildFrame* pFrame = GetFrame();
         Float64 station = pFrame->GetCurrentCutLocation();

         SpanIndexType spanIdx;
         if ( !pBridge->GetSpan(station,&spanIdx) )
            return;

         Float64 back_pier = pBridge->GetPierStation(spanIdx);
         Float64 ahead_pier = pBridge->GetPierStation(spanIdx+1);
         Float64 span_length = ahead_pier - back_pier;
         Float64 inc = span_length/10;

         station = station + (nChar == VK_LEFT ? -1 : 1)*inc*nRepCnt;

         pFrame->CutAt( station );

         return; // don't send this on to the display view
      }
      else
      {
         // otherwise select a pier
         if ( selObjs.size() == 0 )
         {
            PierIndexType nPiers = pBridge->GetPierCount();

            if (nChar == VK_LEFT)
               m_pFrame->SelectPier(nPiers-1);
            else
               m_pFrame->SelectPier(0);

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

   CDisplayView::OnKeyDown(nChar,nRepCnt,nFlags);
}

BOOL CBridgePlanView::OnMouseWheel(UINT nFlags,short zDelta,CPoint pt)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IBridge,pBridge);

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
      else if ( dispListID == SPAN_DISPLAY_LIST || dispListID == PIER_DISPLAY_LIST || dispListID == SLAB_DISPLAY_LIST )
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
      nChar = (zDelta < 0 ? VK_RIGHT : VK_LEFT);
   else
      nChar = (zDelta < 0 ? VK_DOWN : VK_UP);

   OnKeyDown(nChar, 1, nFlags);

   return TRUE;
}

void CBridgePlanView::UpdateDisplayObjects()
{
   CWaitCursor wait;

   // capture the current selection
   CPGSuperDocBase* pDoc = (CPGSuperDocBase*)GetDocument();
   CSelection selection = pDoc->GetSelection();

   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   CDManipClientDC dc(this);

   dispMgr->ClearDisplayObjects();

   BuildTitleDisplayObjects();
   BuildAlignmentDisplayObjects();

   BuildGirderSegmentDisplayObjects();
   BuildGirderDisplayObjects();

   BuildPierDisplayObjects();
   BuildTemporarySupportDisplayObjects();
   BuildClosureJointDisplayObjects();

   BuildSpanDisplayObjects();

   BuildSlabDisplayObjects();
   BuildSectionCutDisplayObjects();
   BuildNorthArrowDisplayObjects();

#pragma Reminder("Fix problem drawing diaphragms for spliced girder bridges")
   //BuildDiaphragmDisplayObjects();
   
   UpdateSegmentTooltips();
   UpdateClosureJointTooltips();

   // restore the selection
   pDoc->SetSelection(selection);
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
   SpanIndexType nSpans = pBridge->GetSpanCount();

   CString strTitle;
   if ( m_StartSpanIdx == 0 && (m_EndSpanIdx == nSpans-1 || m_EndSpanIdx == ALL_SPANS) )
      strTitle = _T("Plan View");
   else if ( m_StartSpanIdx == m_EndSpanIdx )
      strTitle.Format(_T("Plan View: Span %d of %d"),LABEL_SPAN(m_StartSpanIdx),nSpans);
   else
      strTitle.Format(_T("Plan View: Span %d - %d of %d"),LABEL_SPAN(m_StartSpanIdx),LABEL_SPAN(m_EndSpanIdx),nSpans);

   title->SetText(strTitle);
   title_list->AddDisplayObject(title);
}

void CBridgePlanView::BuildAlignmentDisplayObjects()
{
   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   CComPtr<iDisplayList> display_list;
   dispMgr->FindDisplayList(ALIGNMENT_DISPLAY_LIST,&display_list);

   display_list->Clear();

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IRoadway,pRoadway);

   // show that part of the alignment from 1/n of the first span length before the start of the bridge
   // to 1/n of the last span length beyond the end of the bridge
   PierIndexType nPiers = pBridge->GetPierCount();
   PierIndexType startPierIdx = (m_StartSpanIdx == ALL_SPANS ? 0 : m_StartSpanIdx);
   PierIndexType endPierIdx   = (m_EndSpanIdx  == ALL_SPANS ? nPiers-1 : m_EndSpanIdx+1);

   Float64 start_station = pBridge->GetPierStation(startPierIdx);
   Float64 end_station = pBridge->GetPierStation(endPierIdx);
   Float64 length1 = pBridge->GetPierStation(startPierIdx+1) - start_station;
   Float64 length2 = end_station - pBridge->GetPierStation(endPierIdx-1);

   // project the edges of the first and last pier onto the alignment
   // use the min/max station as the start and end of the bridge for purposes
   // of defining the alignment station range
   CComPtr<IPoint2d> start_left, start_alignment, start_bridge, start_right;
   pBridge->GetPierPoints(startPierIdx,&start_left,&start_alignment,&start_bridge,&start_right);
   Float64 start1,start2,start3,start4,offset;
   pRoadway->GetStationAndOffset(start_left,&start1,&offset);
   pRoadway->GetStationAndOffset(start_alignment,&start2,&offset);
   pRoadway->GetStationAndOffset(start_bridge,&start3,&offset);
   pRoadway->GetStationAndOffset(start_right,&start4,&offset);
   start_station = Min(start1,start2,start3,start4);
   start_station -= length1/10;

   CComPtr<IPoint2d> end_left, end_alignment, end_bridge, end_right;
   pBridge->GetPierPoints(endPierIdx,&end_left,&end_alignment,&end_bridge,&end_right);
   Float64 end1,end2,end3,end4;
   pRoadway->GetStationAndOffset(end_left,&end1,&offset);
   pRoadway->GetStationAndOffset(end_alignment,&end2,&offset);
   pRoadway->GetStationAndOffset(end_bridge,&end3,&offset);
   pRoadway->GetStationAndOffset(end_right,&end4,&offset);
   end_station = Max(end1,end2,end3,end4);
   end_station += length2/10;

   // The alignment is represented on the screen by a poly line object
   CComPtr<iPolyLineDisplayObject> doAlignment;
   doAlignment.CoCreateInstance(CLSID_PolyLineDisplayObject);

   // Register an event sink with the alignment object so that we can handle Float64 clicks
   // on the alignment differently then a general dbl-click
   CAlignmentDisplayObjectEvents* pEvents = new CAlignmentDisplayObjectEvents(pBroker,m_pFrame);
   CComPtr<iDisplayObjectEvents> events;
   events.Attach((iDisplayObjectEvents*)pEvents->GetInterface(&IID_iDisplayObjectEvents));

   CComPtr<iDisplayObject> dispObj;
   doAlignment->QueryInterface(IID_iDisplayObject,(void**)&dispObj);
   dispObj->RegisterEventSink(events);
   dispObj->SetToolTipText(_T("Double click to edit alignment.\r\nRight click for more options."));
   dispObj->SetMaxTipWidth(TOOLTIP_WIDTH);
   dispObj->SetTipDisplayTime(TOOLTIP_DURATION);

   // display object for CL bridge
   CComPtr<iPolyLineDisplayObject> doCLBridge;
   doCLBridge.CoCreateInstance(CLSID_PolyLineDisplayObject);

   Float64 alignment_offset = pBridge->GetAlignmentOffset();

   // model the alignment as a series of individual points
   CComPtr<IDirection> bearing;
   bearing.CoCreateInstance(CLSID_Direction);
   long nPoints = 50;
   Float64 station_inc = (end_station - start_station)/nPoints;
   Float64 station = start_station;
   for ( long i = 0; i < nPoints; i++, station += station_inc)
   {
      CComPtr<IPoint2d> p;
      pRoadway->GetPoint(station,0.00,bearing,&p);
      doAlignment->AddPoint(p);

      if ( alignment_offset != 0 )
      {
         p.Release();
         CComPtr<IDirection> normal;
         pRoadway->GetBearingNormal(station,&normal);
         pRoadway->GetPoint(station,alignment_offset,normal,&p);
         doCLBridge->AddPoint(p);
      }
   }

   doAlignment->put_Width(3);
   doAlignment->put_Color(ALIGNMENT_COLOR);
   doAlignment->put_PointType(plpNone);
   doAlignment->Commit();

   doCLBridge->put_Width(1);
   doCLBridge->put_Color(CLBRIDGE_COLOR);
   doCLBridge->put_PointType(plpNone);
   doCLBridge->Commit();

   display_list->AddDisplayObject(dispObj);

   if ( alignment_offset != 0 )
   {
      CComQIPtr<iDisplayObject> dispObj2(doCLBridge);
      display_list->AddDisplayObject(dispObj2);
   }

   dispObj->SetSelectionType(stAll);
   dispObj->SetID(ALIGNMENT_ID);

   // the alignment is going to be the drop site for the section cut object
   CComQIPtr<iDropSite> drop_site(events);
   dispObj->RegisterDropSite(drop_site);
}

void CBridgePlanView::BuildGirderSegmentDisplayObjects()
{
   USES_CONVERSION;

   CBridgeModelViewChildFrame* pFrame = GetFrame();

   CPGSuperDocBase* pDoc = (CPGSuperDocBase*)GetDocument();
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
   label_display_list->Clear();

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

   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IGirder,pIGirder);
   GET_IFACE2(pBroker,ITempSupport,pTempSupport);

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   GroupIndexType nGroups = pBridgeDesc->GetGirderGroupCount();
   for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
   {
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(grpIdx);
      
      if ( pGroup->GetPierIndex(pgsTypes::metStart) < m_StartSpanIdx || m_EndSpanIdx+1 < pGroup->GetPierIndex(pgsTypes::metEnd) )
         continue;

      GirderIndexType nGirders = pGroup->GetGirderCount();
      for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
      {
         const CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);
         SegmentIndexType nSegments = pGirder->GetSegmentCount();
         for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
         {
            CSegmentKey segmentKey(grpIdx,gdrIdx,segIdx);

            // assumes top flange of girder is a rectangle (parallelogram) in plan view
            // a future version may go back to the beam factory to get a shape
            // that represents the top flange (curved girders???)
            Float64 top_width = pIGirder->GetTopWidth( pgsPointOfInterest(segmentKey,0.0) );

            // get the segment geometry points
            CComPtr<IPoint2d> pntSupport1,pntEnd1,pntBrg1,pntBrg2,pntEnd2,pntSupport2;
            pIGirder->GetSegmentEndPoints(segmentKey,&pntSupport1,&pntEnd1,&pntBrg1,&pntBrg2,&pntEnd2,&pntSupport2);

            const CPrecastSegmentData* pSegment   = pGirder->GetSegment(segIdx);
            const CClosureJointData* pLeftClosure  = pSegment->GetLeftClosure();
            const CClosureJointData* pRightClosure = pSegment->GetRightClosure();

            CComPtr<IDirection> objStartDirection,objEndDirection;
            if( pLeftClosure )
            {
               if ( pLeftClosure->GetTemporarySupport() )
                  pTempSupport->GetDirection(pLeftClosure->GetTemporarySupport()->GetIndex(),&objStartDirection);
               else
                  pBridge->GetPierDirection(pLeftClosure->GetPier()->GetIndex(),&objStartDirection);
            }
            else
            {
               pBridge->GetPierDirection(pGirder->GetPier(pgsTypes::metStart)->GetIndex(),&objStartDirection);
            }

            if( pRightClosure )
            {
               if ( pRightClosure->GetTemporarySupport() )
                  pTempSupport->GetDirection(pRightClosure->GetTemporarySupport()->GetIndex(),&objEndDirection);
               else
                  pBridge->GetPierDirection(pRightClosure->GetPier()->GetIndex(),&objEndDirection);
            }
            else
            {
               pBridge->GetPierDirection(pGirder->GetPier(pgsTypes::metEnd)->GetIndex(),&objEndDirection);
            }

            Float64 start_direction, end_direction;
            objStartDirection->get_Value(&start_direction);
            objEndDirection->get_Value(&end_direction);

            // create display objects for all the points
            // also add a socket for each point
            CComPtr<iPointDisplayObject> doEnd1, doBrg1, doBrg2, doEnd2;
            doEnd1.CoCreateInstance(CLSID_PointDisplayObject);
            doEnd1->SetPosition(pntEnd1,FALSE,FALSE);
            doEnd1->Visible(FALSE);
            CComQIPtr<iConnectable> connectable_end1(doEnd1);
            CComPtr<iSocket> socket_end1;
            connectable_end1->AddSocket(0,pntEnd1,&socket_end1);

            doBrg1.CoCreateInstance(CLSID_PointDisplayObject);
            doBrg1->SetPosition(pntBrg1,FALSE,FALSE);
            doBrg1->Visible(FALSE);
            CComQIPtr<iConnectable> connectable_brg1(doBrg1);
            CComPtr<iSocket> socket_brg1;
            connectable_brg1->AddSocket(0,pntBrg1,&socket_brg1);

            doBrg2.CoCreateInstance(CLSID_PointDisplayObject);
            doBrg2->SetPosition(pntBrg2,FALSE,FALSE);
            doBrg2->Visible(FALSE);
            CComQIPtr<iConnectable> connectable_brg2(doBrg2);
            CComPtr<iSocket> socket_brg2;
            connectable_brg2->AddSocket(0,pntBrg2,&socket_brg2);

            doEnd2.CoCreateInstance(CLSID_PointDisplayObject);
            doEnd2->SetPosition(pntEnd2,FALSE,FALSE);
            doEnd2->Visible(FALSE);
            CComQIPtr<iConnectable> connectable_end2(doEnd2);
            CComPtr<iSocket> socket_end2;
            connectable_end2->AddSocket(0,pntEnd2,&socket_end2);

            // create a display object for the segment
            CComPtr<iLineDisplayObject> doSegment;
            doSegment.CoCreateInstance(CLSID_LineDisplayObject);

            IDType ID = m_NextSegmentID++;
            m_SegmentIDs.insert( std::make_pair(segmentKey,ID) );
            doSegment->SetID(ID);
            SegmentDisplayObjectInfo* pInfo = new SegmentDisplayObjectInfo(segmentKey,SEGMENT_DISPLAY_LIST);
            doSegment->SetItemData((void*)pInfo,true);

            // connect the segment display object to the display objects
            // at the segment ends
            CComQIPtr<iConnector> connector(doSegment);
            CComQIPtr<iPlug> startPlug, endPlug;
            connector->GetStartPlug(&startPlug);
            connector->GetEndPlug(&endPlug);

            DWORD dwCookie;
            connectable_end1->Connect(0,atByID,startPlug,&dwCookie);
            connectable_end2->Connect(0,atByID,endPlug,  &dwCookie);


            display_list->AddDisplayObject(doEnd1);
            display_list->AddDisplayObject(doEnd2);

            display_list->AddDisplayObject(doBrg1);
            display_list->AddDisplayObject(doBrg2);

            display_list->AddDisplayObject(doSegment);

            // drawing strategy
            // use a compound strategy so we can draw both centerline and the top flange rectangle
            CComPtr<iCompoundDrawLineStrategy> strategy;
            strategy.CoCreateInstance(CLSID_CompoundDrawLineStrategy);

            CComPtr<iExtRectangleDrawLineStrategy> strategy1;
            strategy1.CoCreateInstance(CLSID_ExtRectangleDrawLineStrategy);

            strategy1->SetLeftOffset(top_width/2);
            strategy1->SetRightOffset(top_width/2);

            CComPtr<IDirection> objBearing;
            pIGirder->GetSegmentDirection(segmentKey,&objBearing);

            CComPtr<IDirection> objNormal;
            objBearing->Increment(CComVariant(PI_OVER_2),&objNormal);
            Float64 normal;
            objNormal->get_Value(&normal);
            Float64 start_skew = start_direction - normal;
            Float64 end_skew   = end_direction   - normal;
            strategy1->SetStartSkew(start_skew);
            strategy1->SetEndSkew(end_skew);

            strategy1->SetColor(SEGMENT_BORDER_COLOR);
            strategy1->SetDoFill(TRUE);
            strategy1->SetFillColor(SEGMENT_FILL_COLOR);

            // this strategy doubles as a gravity well.. get its interface and give it to 
            // the line display object. this will make the entire top flange the clickable part
            // of the display object
            strategy1->PerimeterGravityWell(TRUE);
            CComQIPtr<iGravityWellStrategy> gravity_well(strategy1);
            doSegment->SetGravityWellStrategy(gravity_well);
            doSegment->SetSelectionType(stAll);

            strategy->AddStrategy(strategy1);

   #if defined _SHOW_CL_GIRDER
            strategy->AddStrategy(strategy_girder);
   #endif
            doSegment->SetDrawLineStrategy(strategy);

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
               pntEnd1->get_X(&x1);
               pntEnd1->get_Y(&y1);
               pntEnd2->get_X(&x2);
               pntEnd2->get_Y(&y2);

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

            // Register an event sink with the segment display object so that we can handle Float64 clicks
            // on the segment differently then a general Float64 click
            CBridgePlanViewSegmentDisplayObjectEvents* pEvents = new CBridgePlanViewSegmentDisplayObjectEvents(segmentKey,pFrame);
            CComPtr<iDisplayObjectEvents> events;
            events.Attach((iDisplayObjectEvents*)pEvents->GetInterface(&IID_iDisplayObjectEvents));

            CComQIPtr<iDisplayObject,&IID_iDisplayObject> dispObj(doSegment);
            dispObj->RegisterEventSink(events);
         } // segment loop
      } // girder loop
   } // group loop
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
   //label_display_list->Clear(); // don't clear... BuildGirderSegments puts things in this list that we don't want to erase


   m_NextGirderID = 0;
   m_GirderIDs.clear();

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IGirder,pIGirder);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   CPGSuperDocBase* pDoc = (CPGSuperDocBase*)GetDocument();
   UINT settings = pDoc->GetBridgeEditorSettings();

   GroupIndexType nGroups = pBridgeDesc->GetGirderGroupCount();
   for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
   {
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(grpIdx);
      
      if ( pGroup->GetPierIndex(pgsTypes::metStart) < m_StartSpanIdx || m_EndSpanIdx+1 < pGroup->GetPierIndex(pgsTypes::metEnd) )
         continue;

      GirderIndexType nGirdersThisGroup = pGroup->GetGirderCount();

      for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirdersThisGroup; gdrIdx++ )
      {
         CComPtr<iCompositeDisplayObject> doGirderLine;
         doGirderLine.CoCreateInstance(CLSID_CompositeDisplayObject);
         doGirderLine->SetSelectionType(stAll);

         CGirderKey girderKey(grpIdx,gdrIdx);

         IDType ID = m_NextGirderID++;
         m_GirderIDs.insert( std::make_pair(girderKey,ID) );

         doGirderLine->SetID(ID);

         GirderDisplayObjectInfo* pInfo = new GirderDisplayObjectInfo(girderKey,GIRDER_DISPLAY_LIST);
         doGirderLine->SetItemData((void*)pInfo,true);

         const CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);
         SegmentIndexType nSegments = pGirder->GetSegmentCount();
         for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
         {
            CSegmentKey segmentKey(grpIdx,gdrIdx,segIdx);
            std::map<CSegmentKey,IDType>::iterator found = m_SegmentIDs.find(segmentKey);
            ASSERT(found != m_SegmentIDs.end() );

            IDType ID = (*found).second;

            CComPtr<iDisplayObject> doSegment;
            segment_display_list->FindDisplayObject(ID,&doSegment);
            ASSERT(doSegment);

            if ( settings & IDB_PV_LABEL_GIRDERS && segIdx == 0 )
            {
               // Put girder label on first segment

               // direction to output text
               CComPtr<IDirection> direction;
               pBridge->GetSegmentBearing(segmentKey,&direction);
               Float64 dir;
               direction->get_Value(&dir);
               long angle = long(1800.*dir/M_PI);
               angle = (900 < angle && angle < 2700 ) ? angle-1800 : angle;

               // segment end points
               CComPtr<IPoint2d> pntSupport1,pntEnd1,pntBrg1,pntBrg2,pntEnd2,pntSupport2;
               pIGirder->GetSegmentEndPoints(segmentKey,&pntSupport1,&pntEnd1,&pntBrg1,&pntBrg2,&pntEnd2,&pntSupport2);

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

         // Register an event sink with the girder display object so that we can handle Float64 clicks
         // on the girder differently then a general Float64 click
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
   //label_display_list->Clear(); // Don't clear it...BuildGirderDisplayObjects put some stuff in here

   CPGSuperDocBase* pDoc = (CPGSuperDocBase*)GetDocument();
   UINT settings = pDoc->GetBridgeEditorSettings();

   GET_IFACE2(pBroker,IRoadway,pAlignment);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   GET_IFACE2(pBroker,IGirder,pGirder);
   GET_IFACE2(pBroker,IBridge,pBridge);

   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CTimelineManager* pTimelineMgr = pBridgeDesc->GetTimelineManager();

   CComPtr<IDocUnitSystem> docUnitSystem;
   pDoc->GetDocUnitSystem(&docUnitSystem);

   PierIndexType nPiers = pBridge->GetPierCount();
   SpanIndexType nSpans = pBridge->GetSpanCount();
   Float64 last_station;
   PierIndexType firstPierIdx = (m_StartSpanIdx == ALL_SPANS ? 0 : m_StartSpanIdx);
   PierIndexType lastPierIdx  = (m_EndSpanIdx  == ALL_SPANS ? nPiers-1 : m_EndSpanIdx+1);
   for ( PierIndexType pierIdx = firstPierIdx; pierIdx <= lastPierIdx; pierIdx++ )
   {
      const CPierData2* pPier = pBridgeDesc->GetPier(pierIdx);

      CString strPierLabel(pPier->IsAbutment() ? _T("Abutment") : _T("Pier"));

      // get station of the pier
      Float64 station = pPier->GetStation();
   
      CComPtr<IDirection> direction;
      pBridge->GetPierDirection(pierIdx,&direction);

      // skew the pier so it parallels the alignment
      CComPtr<IAngle> objSkew;
      pBridge->GetPierSkew(pierIdx,&objSkew);
      Float64 skew;
      objSkew->get_Value(&skew);

      // get the pier control points
      CComPtr<IPoint2d> left,alignment_pt,bridge_pt,right;
      pBridge->GetPierPoints(pierIdx,&left,&alignment_pt,&bridge_pt,&right);

      // create a point display object for the left side of the pier
      // add a socket to it
      CComPtr<iPointDisplayObject> doLeft;
      doLeft.CoCreateInstance(CLSID_PointDisplayObject);
      doLeft->SetPosition(left,FALSE,FALSE);
      CComQIPtr<iConnectable> connectable1(doLeft);
      CComPtr<iSocket> socket1;
      connectable1->AddSocket(0,left,&socket1);

      // create a point display object for the right side of the pier
      // add a socket to it
      CComPtr<iPointDisplayObject> doRight;
      doRight.CoCreateInstance(CLSID_PointDisplayObject);
      doRight->SetPosition(right,FALSE,FALSE);
      CComQIPtr<iConnectable> connectable2(doRight);
      CComPtr<iSocket> socket2;
      connectable2->AddSocket(0,right,&socket2);

      // create a line display object for the pier centerline
      CComPtr<iLineDisplayObject> doCenterLine;
      doCenterLine.CoCreateInstance(CLSID_LineDisplayObject);

      CString strMsg1;
      strMsg1.Format(_T("Double click to edit %s %d\r\nRight click for more options."),strPierLabel,LABEL_PIER(pierIdx));

      CString strMsg2;
      strMsg2.Format(_T("Station: %s\r\nDirection: %s\r\nSkew: %s"),FormatStation(pDisplayUnits->GetStationFormat(),station),FormatDirection(direction),FormatAngle(objSkew));

      CString strConnectionTip;
      if ( pPier->IsBoundaryPier() )
         strConnectionTip.Format(_T("Boundary Condition: %s"),CPierData2::AsString(pPier->GetPierConnectionType()));
      else
         strConnectionTip.Format(_T("Boundary Condition: %s"),CPierData2::AsString(pPier->GetSegmentConnectionType()));

      CString strMsg = strMsg1 + _T("\r\n\r\n") + strMsg2 + _T("\r\n") + strConnectionTip;

      EventIndexType eventIdx = pTimelineMgr->GetPierErectionEventIndex(pierIdx);

      CString strEvent;
      if ( eventIdx != INVALID_INDEX )
      {
         const CTimelineEvent* pTimelineEventData = pTimelineMgr->GetEventByIndex(eventIdx);

         strEvent.Format(_T("Erection Event: Event %d: %s"),LABEL_EVENT(eventIdx),pTimelineEventData->GetDescription());
      }
      else
      {
         strEvent.Format(_T("Erection Event: No defined"));
      }

      strMsg += _T("\r\n") + strEvent;

      doCenterLine->SetToolTipText(strMsg);
      doCenterLine->SetMaxTipWidth(TOOLTIP_WIDTH);
      doCenterLine->SetTipDisplayTime(TOOLTIP_DURATION);

      doCenterLine->SetID(pierIdx);
      PierDisplayObjectInfo* pInfo = new PierDisplayObjectInfo(pierIdx,PIER_DISPLAY_LIST);
      doCenterLine->SetItemData((void*)pInfo,true);

      // get the connectors from the line
      CComQIPtr<iConnector> connector(doCenterLine);
      CComQIPtr<iPlug> startPlug, endPlug;
      connector->GetStartPlug(&startPlug);
      connector->GetEndPlug(&endPlug);

      // connect the line to the points
      DWORD dwCookie;
      connectable1->Connect(0,atByID,startPlug,&dwCookie);
      connectable2->Connect(0,atByID,endPlug,  &dwCookie);

      // Register an event sink with the pier centerline display object so that we can handle dbl-clicks
      // on the piers differently then a general dbl-click
      CPierDisplayObjectEvents* pEvents = new CPierDisplayObjectEvents(pierIdx,
                                                                       pBridgeDesc,
                                                                       pFrame);
      CComPtr<iDisplayObjectEvents> events;
      events.Attach((iDisplayObjectEvents*)pEvents->GetInterface(&IID_iDisplayObjectEvents));

      CComQIPtr<iDisplayObject,&IID_iDisplayObject> dispObj(doCenterLine);
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
      strategy_pier->SetDoFill(TRUE);

      // make the pier outline just a bit wider
      SpanIndexType prev_span_idx = pierIdx - 1;
      SpanIndexType next_span_idx = pierIdx == nPiers-1 ? INVALID_INDEX : pierIdx;

      Float64 left_offset = 0;
      Float64 right_offset = 0;

      if ( pPier->GetPrevSpan() )
      {
         ConnectionLibraryEntry::BearingOffsetMeasurementType left_brg_offset_measure_type;
         pPier->GetBearingOffset(pgsTypes::Back,&left_offset,&left_brg_offset_measure_type);
         if ( left_brg_offset_measure_type == ConnectionLibraryEntry::NormalToPier )
            left_offset /= cos(skew);
      }

      if ( pPier->GetNextSpan() )
      {
         ConnectionLibraryEntry::BearingOffsetMeasurementType right_brg_offset_measure_type;
         pPier->GetBearingOffset(pgsTypes::Ahead,&right_offset,&right_brg_offset_measure_type);
         if ( right_brg_offset_measure_type == ConnectionLibraryEntry::NormalToPier )
            right_offset /= cos(skew);
      }
 
      left_offset  = ( IsZero(left_offset)  ? right_offset/2 : left_offset );
      right_offset = ( IsZero(right_offset) ? left_offset/2  : right_offset );

      left_offset *= 1.05;
      right_offset *= 1.05;

      strategy_pier->SetLeftOffset(left_offset);
      strategy_pier->SetRightOffset(right_offset);

#pragma Reminder("UPDATE: These are dummy vales...")
      Float64 left_overhang = 0.5;
      Float64 right_overhang = 0.5;
      //// make the pier overhang the exterior girders
      //Float64 prev_girder_length = (prev_span_idx == INVALID_INDEX ? 0 : pBridge->GetGirderLength(prev_span_idx,0));
      //Float64 prev_top_width = (prev_span_idx == INVALID_INDEX ? 0 : pGirder->GetTopWidth(pgsPointOfInterest(::HashSpanGirder(prev_span_idx,0),prev_girder_length)));
      //Float64 prev_bot_width = (prev_span_idx == INVALID_INDEX ? 0 : pGirder->GetBottomWidth(pgsPointOfInterest(::HashSpanGirder(prev_span_idx,0),prev_girder_length)));
      //Float64 next_top_width = (next_span_idx == INVALID_INDEX ? 0 : pGirder->GetTopWidth(pgsPointOfInterest(::HashSpanGirder(next_span_idx,0),0)));
      //Float64 next_bot_width = (next_span_idx == INVALID_INDEX ? 0 : pGirder->GetBottomWidth(pgsPointOfInterest(::HashSpanGirder(next_span_idx,0),0)));
      //Float64 left_overhang = Max(prev_top_width,prev_bot_width,next_top_width,next_bot_width)/2;
      //left_overhang /= cos(fabs(skew));
      //left_overhang *= 1.10;

      //GirderIndexType nGirdersPrevSpan = (prev_span_idx == INVALID_INDEX ? 0 : pBridge->GetGirderCount(prev_span_idx));
      //GirderIndexType nGirdersNextSpan = (next_span_idx == INVALID_INDEX ? 0 : pBridge->GetGirderCount(next_span_idx));
      //prev_girder_length = (prev_span_idx == INVALID_INDEX ? 0 : pBridge->GetGirderLength(prev_span_idx,nGirdersPrevSpan-1));
      //prev_top_width = (prev_span_idx == INVALID_INDEX ? 0 : pGirder->GetTopWidth(pgsPointOfInterest(::HashSpanGirder(prev_span_idx,nGirdersPrevSpan-1),prev_girder_length)));
      //prev_bot_width = (prev_span_idx == INVALID_INDEX ? 0 : pGirder->GetBottomWidth(pgsPointOfInterest(::HashSpanGirder(prev_span_idx,nGirdersPrevSpan-1),prev_girder_length)));
      //next_top_width = (next_span_idx == INVALID_INDEX ? 0 : pGirder->GetTopWidth(pgsPointOfInterest(::HashSpanGirder(next_span_idx,nGirdersNextSpan-1),0)));
      //next_bot_width = (next_span_idx == INVALID_INDEX ? 0 : pGirder->GetBottomWidth(pgsPointOfInterest(::HashSpanGirder(next_span_idx,nGirdersNextSpan-1),0)));
      //Float64 right_overhang = 1.10*Max(prev_top_width,prev_bot_width,next_top_width,next_bot_width)/2;
      //right_overhang /= cos(fabs(skew));
      //right_overhang *= 1.10;

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
      pAlignment->GetPoint(station,-alignmentOffset,direction,&ahead_point);

      CComPtr<IPoint2d> back_point;
      pAlignment->GetPoint(station,-alignmentOffset,direction,&back_point);

      if ( settings & IDB_PV_LABEL_PIERS )
      {
         // pier label
         CComPtr<iTextBlock> doPierName;
         doPierName.CoCreateInstance(CLSID_TextBlock);

         CString strText;
         strText.Format(_T("%s %d"),strPierLabel,LABEL_PIER(pierIdx));

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

         CInplacePierStationEditEvents* pPierStationEvents = new CInplacePierStationEditEvents(pBroker,pierIdx);
         CComPtr<iDisplayObjectEvents> pierStationEvents;
         pierStationEvents.Attach((iDisplayObjectEvents*)pPierStationEvents->GetInterface(&IID_iDisplayObjectEvents));

         doStation->RegisterEventSink(pierStationEvents);

         label_display_list->AddDisplayObject(doStation);

         // connection
         Float64 right_slab_edge_offset = pBridge->GetRightSlabEdgeOffset(pierIdx);
         CComPtr<IPoint2d> connection_label_point;
         pAlignment->GetPoint(station,-right_slab_edge_offset/cos(skew),direction,&connection_label_point);

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
         doConnection->SetTextColor(CONNECTION_LABEL_COLOR);
         doConnection->SetBkMode(TRANSPARENT);
         doConnection->SetAngle(angle);

         if ( pierIdx == 0 ) // first pier
         {
            doConnection->SetTextAlign(TA_TOP | TA_LEFT);
         }
         else if ( pierIdx == nPiers-1 ) // last pier
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

         // Register an event sink with the connection text display object so that we can handle Float64 clicks
         // differently then a general Float64 click
         CConnectionDisplayObjectEvents* pEvents = new CConnectionDisplayObjectEvents(pierIdx);
         CComPtr<iDisplayObjectEvents> events;
         events.Attach((iDisplayObjectEvents*)pEvents->GetInterface(&IID_iDisplayObjectEvents));

         CComQIPtr<iDisplayObject,&IID_iDisplayObject> dispObj(doConnection);
         dispObj->RegisterEventSink(events);

         label_display_list->AddDisplayObject(doConnection);

         if ( firstPierIdx < pierIdx )
         {
            ATLASSERT(pierIdx != ALL_PIERS);
            // label span length

            Float64 span_length = station - last_station;

            CComPtr<IPoint2d> pntInSpan;
            pAlignment->GetPoint(last_station + span_length/2,0.00,direction,&pntInSpan);

            CComPtr<IDirection> dirParallel;
            pAlignment->GetBearing(last_station + span_length/2,&dirParallel);

            dirParallel->get_Value(&dir);
            angle = long(1800.*dir/M_PI);
            angle = (900 < angle && angle < 2700 ) ? angle-1800 : angle;

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
            CInplaceSpanLengthEditEvents* pSpanLengthEvents = new CInplaceSpanLengthEditEvents(pBroker,pierIdx-1);
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
      doAlignment->SetPosition(alignment_pt,FALSE,FALSE);
      CComQIPtr<iConnectable> connectable3(doAlignment);
      CComPtr<iSocket> socket3;
      connectable3->AddSocket(0,alignment_pt,&socket3);

      CComPtr<iPointDisplayObject> doBridge;
      doBridge.CoCreateInstance(CLSID_PointDisplayObject);
      doBridge->SetPosition(bridge_pt,FALSE,FALSE);
      CComQIPtr<iConnectable> connectable4(doBridge);
      CComPtr<iSocket> socket4;
      connectable4->AddSocket(0,bridge_pt,&socket4);

      // get the connectors from the line
      CComQIPtr<iConnector> connector2(doCenterLine2);
      startPlug.Release();
      endPlug.Release();
      connector2->GetStartPlug(&startPlug);
      connector2->GetEndPlug(&endPlug);

      // connect the line to the points
      connectable3->Connect(0,atByID,startPlug,&dwCookie);
      connectable4->Connect(0,atByID,endPlug,  &dwCookie);

      CComPtr<iSimpleDrawLineStrategy> offset_line_strategy;
      offset_line_strategy.CoCreateInstance(CLSID_SimpleDrawLineStrategy);
      offset_line_strategy->SetColor(RGB(120,120,120));
      offset_line_strategy->SetLineStyle(lsCenterline);

      doCenterLine2->SetDrawLineStrategy(offset_line_strategy);


      display_list->AddDisplayObject(doLeft);
      display_list->AddDisplayObject(doAlignment);
      display_list->AddDisplayObject(doBridge);
      display_list->AddDisplayObject(doRight);
      display_list->AddDisplayObject(doCenterLine);
      display_list->AddDisplayObject(doCenterLine2);

      last_station = station;
   }

   CComPtr<iCoordinateMap> map;
   dispMgr->GetCoordinateMap(&map);
   CComQIPtr<iMapping> mapping(map);

   // get point on alignment at first pier
   CComPtr<IDirection> dir;
   Float64 station = pBridge->GetPierStation(0);
   pBridge->GetPierDirection(0,&dir);
   CComPtr<IPoint2d> rotation_center;
   pAlignment->GetPoint(station,0.00,dir,&rotation_center);

   // get point on alignment at last pier
   CComPtr<IPoint2d> end_point;
   dir.Release();
   station = pBridge->GetPierStation(nPiers-1);
   pBridge->GetPierDirection(nPiers-1,&dir);
   pAlignment->GetPoint(station,0.00,dir,&end_point);

   // get the direction of the line from the start of the bridge to the end
   // this represents the amount we want to rotate the display

   Float64 x1,y1, x2, y2;
   rotation_center->get_X(&x1);
   rotation_center->get_Y(&y1);
   end_point->get_X(&x2);
   end_point->get_Y(&y2);

   Float64 dx = x2 - x1;
   Float64 dy = y2 - y1;

   Float64 angle = atan2(dy,dx);

   if ( settings & IDB_PV_NORTH_UP )
   {
      mapping->SetRotation((x1+x2)/2,(y1+y2)/2,0);
   }
   else
   {
      // rotation by negative of the angle
      mapping->SetRotation((x1+x2)/2,(y1+y2)/2,-angle);
   }
}

void CBridgePlanView::BuildTemporarySupportDisplayObjects()
{
   CBridgeModelViewChildFrame* pFrame = GetFrame();

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   CComPtr<iDisplayList> display_list;
   dispMgr->FindDisplayList(TEMPORARY_SUPPORT_DISPLAY_LIST,&display_list);
   display_list->Clear();

   CComPtr<iDisplayList> label_display_list;
   dispMgr->FindDisplayList(LABEL_DISPLAY_LIST,&label_display_list);
   //label_display_list->Clear(); // Don't clear it...BuildGirderDisplayObjects put some stuff in here

   CPGSuperDocBase* pDoc = (CPGSuperDocBase*)GetDocument();
   UINT settings = pDoc->GetBridgeEditorSettings();

   GET_IFACE2(pBroker,IRoadway,pAlignment);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   GET_IFACE2(pBroker,IBridge,pBridge);

   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CTimelineManager* pTimelineMgr = pBridgeDesc->GetTimelineManager();

   CComPtr<IDocUnitSystem> docUnitSystem;
   pDoc->GetDocUnitSystem(&docUnitSystem);

   GET_IFACE2(pBroker,ITempSupport,pTemporarySupport);

   SupportIndexType nTS = pBridgeDesc->GetTemporarySupportCount();
   for ( SupportIndexType tsIdx = 0; tsIdx < nTS; tsIdx++ )
   {
      const CTemporarySupportData* pTS = pBridgeDesc->GetTemporarySupport(tsIdx);

      SpanIndexType spanIdx = pTS->GetSpan()->GetIndex();
      if ( spanIdx < m_StartSpanIdx || m_EndSpanIdx < spanIdx )
         continue;

      SupportIDType tsID = pTS->GetID();

      Float64 station = pTS->GetStation();

      pgsTypes::TemporarySupportType tsSupportType = pTS->GetSupportType();
      pgsTypes::SegmentConnectionType segConnectionType = pTS->GetConnectionType();

      EventIndexType erectionEventIdx, removalEventIdx;
      pTimelineMgr->GetTempSupportEvents(tsID,&erectionEventIdx,&removalEventIdx);
      const CTimelineEvent* pErectionEvent = NULL;
      const CTimelineEvent* pRemovalEvent  = NULL;

      if ( erectionEventIdx != INVALID_INDEX )
         pErectionEvent = pTimelineMgr->GetEventByIndex(erectionEventIdx);

      if ( removalEventIdx != INVALID_INDEX )
         pRemovalEvent  = pTimelineMgr->GetEventByIndex(removalEventIdx);

      // get the control points
      CComPtr<IPoint2d> left,alignment_pt,bridge_pt,right;
      pTemporarySupport->GetControlPoints(tsIdx,&left,&alignment_pt,&bridge_pt,&right);

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

      // Register an event sink with the centerline display object so that we can handle Float64 clicks
      // on the temporary supports differently then a general Float64 click
      CTemporarySupportDisplayObjectEvents* pEvents = new CTemporarySupportDisplayObjectEvents(tsID,pFrame);
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
      strMsg1.Format(_T("Double click to edit Temporary Support %d\r\nRight click for more options."),LABEL_TEMPORARY_SUPPORT(tsIdx));

      CString strMsg2;
      strMsg2.Format(_T("Type: %s\r\nStation: %s\r\nDirection: %s\r\nSkew: %s\r\nConnection Type: %s\r\nErection Event: Event: %d %s\r\nRemoval Event: Event: %d %s"),
                     tsSupportType == pgsTypes::ErectionTower ? _T("Erection Tower") : _T("Strong Back"),
                     FormatStation(pDisplayUnits->GetStationFormat(),station),
                     FormatDirection(direction),
                     FormatAngle(objSkew),
                     segConnectionType == pgsTypes::sctClosureJoint ? _T("Closure Joint") : _T("Continuous Segment"),
                     LABEL_EVENT(erectionEventIdx),
                     pErectionEvent == NULL ? _T("Erection stage not defined") : pErectionEvent->GetDescription(),
                     LABEL_EVENT(removalEventIdx),
                     pRemovalEvent ==  NULL ? _T("Removal stage not defined") : pRemovalEvent->GetDescription());

      CString strMsg = strMsg1 + _T("\r\n\r\n") + strMsg2 + _T("\r\n");

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
      strategy_pier->SetDoFill(TRUE);

#pragma Reminder("FIX: These are dummy vales...")
      // See commented code block below
      Float64 left_offset = 0;
      Float64 right_offset = 0;
      Float64 left_overhang = 0.5;
      Float64 right_overhang = 0.5;

      Float64 support_width = pTS->GetSupportWidth();
      
      Float64 brg_offset;
      ConnectionLibraryEntry::BearingOffsetMeasurementType brg_offset_measurement_type;
      pTS->GetBearingOffset(&brg_offset,&brg_offset_measurement_type);
      if ( brg_offset_measurement_type == ConnectionLibraryEntry::NormalToPier )
         brg_offset /= cos(skew);

      left_offset  = 1.05*(brg_offset + support_width);
      right_offset = 1.05*(brg_offset + support_width);

      // this is how the offset and overhang values are computed for precast girder bridges
      // when the values are fixed above, delete this code block
   //   // make the pier overhang the exterior girders
   //   Float64 prev_girder_length = (prev_span_idx == ALL_SPANS ? 0 : pBridge->GetGirderLength(prev_span_idx,0));
   //   Float64 prev_top_width = (prev_span_idx == ALL_SPANS ? 0 : pGirder->GetTopWidth(pgsPointOfInterest(::HashSpanGirder(prev_span_idx,0),prev_girder_length)));
   //   Float64 prev_bot_width = (prev_span_idx == ALL_SPANS ? 0 : pGirder->GetBottomWidth(pgsPointOfInterest(::HashSpanGirder(prev_span_idx,0),prev_girder_length)));
   //   Float64 next_top_width = (next_span_idx == ALL_SPANS ? 0 : pGirder->GetTopWidth(pgsPointOfInterest(::HashSpanGirder(next_span_idx,0),0)));
   //   Float64 next_bot_width = (next_span_idx == ALL_SPANS ? 0 : pGirder->GetBottomWidth(pgsPointOfInterest(::HashSpanGirder(next_span_idx,0),0)));
   //   Float64 left_overhang = Max(prev_top_width,prev_bot_width,next_top_width,next_bot_width)/2;
   //   left_overhang /= cos(fabs(skew));
   //   left_overhang *= 1.10;

   //   GirderIndexType nGirdersPrevSpan = (prev_span_idx == ALL_SPANS ? 0 : pBridge->GetGirderCount(prev_span_idx));
   //   GirderIndexType nGirdersNextSpan = (next_span_idx == ALL_SPANS ? 0 : pBridge->GetGirderCount(next_span_idx));
   //   prev_girder_length = (prev_span_idx == ALL_SPANS ? 0 : pBridge->GetGirderLength(prev_span_idx,nGirdersPrevSpan-1));
   //   prev_top_width = (prev_span_idx == ALL_SPANS ? 0 : pGirder->GetTopWidth(pgsPointOfInterest(::HashSpanGirder(prev_span_idx,nGirdersPrevSpan-1),prev_girder_length)));
   //   prev_bot_width = (prev_span_idx == ALL_SPANS ? 0 : pGirder->GetBottomWidth(pgsPointOfInterest(::HashSpanGirder(prev_span_idx,nGirdersPrevSpan-1),prev_girder_length)));
   //   next_top_width = (next_span_idx == ALL_SPANS ? 0 : pGirder->GetTopWidth(pgsPointOfInterest(::HashSpanGirder(next_span_idx,nGirdersNextSpan-1),0)));
   //   next_bot_width = (next_span_idx == ALL_SPANS ? 0 : pGirder->GetBottomWidth(pgsPointOfInterest(::HashSpanGirder(next_span_idx,nGirdersNextSpan-1),0)));
   //   Float64 right_overhang = 1.10*Max(prev_top_width,prev_bot_width,next_top_width,next_bot_width)/2;
   //   right_overhang /= cos(fabs(skew));
   //   right_overhang *= 1.10;

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
      pAlignment->GetPoint(station,-alignmentOffset,direction,&ahead_point);

      CComPtr<IPoint2d> back_point;
      pAlignment->GetPoint(station,-alignmentOffset,direction,&back_point);

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
         Float64 dist_from_start_of_bridge = station - pBridgeDesc->GetPier(0)->GetStation();
         Float64 left_slab_edge_offset = pBridge->GetLeftSlabEdgeOffset(dist_from_start_of_bridge);
         CComPtr<IPoint2d> connection_label_point;
         pAlignment->GetPoint(station,-left_slab_edge_offset/cos(skew),direction,&connection_label_point);

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
            doConnection->SetTextColor(TS_LABEL_COLOR);
         else
            doConnection->SetTextColor(SB_LABEL_COLOR);

         doConnection->SetBkMode(TRANSPARENT);
         doConnection->SetAngle(angle);
         doConnection->SetTextAlign(TA_BOTTOM | TA_CENTER);

         doConnection->SetText(GetConnectionString(pTS).c_str());

         doConnection->SetToolTipText(GetFullConnectionString(pTS).c_str());
         doConnection->SetMaxTipWidth(TOOLTIP_WIDTH);
         doConnection->SetTipDisplayTime(TOOLTIP_DURATION);

#pragma Reminder("TODO: Need connection display object for temporary supports")
         //// Register an event sink with the connection text display object so that we can handle Float64 clicks
         //// differently then a general Float64 click
         //CConnectionDisplayObjectEvents* pEvents = new CConnectionDisplayObjectEvents(pierIdx);

         //IUnknown* unk = pEvents->GetInterface(&IID_iDisplayObjectEvents);
         //CComQIPtr<iDisplayObjectEvents,&IID_iDisplayObjectEvents> events(unk);
         //CComQIPtr<iDisplayObject,&IID_iDisplayObject> dispObj(doConnection);
         //dispObj->RegisterEventSink(events);

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
   GET_IFACE2(pBroker,IBridge,pIBridge);
   GET_IFACE2(pBroker,IGirder,pIGirder);
   GET_IFACE2(pBroker,ITempSupport,pITemporarySupport);

   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   GroupIndexType nGroups = pBridgeDesc->GetGirderGroupCount();
   for (GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
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
                  pITemporarySupport->GetDirection(pClosure->GetTemporarySupport()->GetIndex(),&objDirection);
               else
                  pIBridge->GetPierDirection(pClosure->GetPier()->GetIndex(),&objDirection);
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
            CComPtr<IPoint2d> pntSupport1[2],pntEnd1[2],pntBrg1[2],pntBrg2[2],pntEnd2[2],pntSupport2[2];
            pIGirder->GetSegmentEndPoints(leftSegmentKey,  &pntSupport1[pgsTypes::Back], &pntEnd1[pgsTypes::Back], &pntBrg1[pgsTypes::Back], &pntBrg2[pgsTypes::Back], &pntEnd2[pgsTypes::Back], &pntSupport2[pgsTypes::Back]);
            pIGirder->GetSegmentEndPoints(rightSegmentKey, &pntSupport1[pgsTypes::Ahead],&pntEnd1[pgsTypes::Ahead],&pntBrg1[pgsTypes::Ahead],&pntBrg2[pgsTypes::Ahead],&pntEnd2[pgsTypes::Ahead],&pntSupport2[pgsTypes::Ahead]);

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

            // Register an event sink with the display object so that we can handle Float64 clicks
            // differently then a general Float64 click
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
            strategy1->SetDoFill(TRUE);
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
   if ( pBridge->GetDeckType() == pgsTypes::sdtNone )
      return;

   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   CComPtr<iDisplayList> display_list;
   dispMgr->FindDisplayList(SPAN_DISPLAY_LIST,&display_list);

   display_list->Clear();

   SpanIndexType nSpans = pBridge->GetSpanCount();
   SpanIndexType firstSpanIdx = (m_StartSpanIdx == ALL_SPANS ? 0 : m_StartSpanIdx);
   SpanIndexType lastSpanIdx  = (m_EndSpanIdx  == ALL_SPANS ? nSpans-1 : m_EndSpanIdx);
   for ( SpanIndexType spanIdx = firstSpanIdx; spanIdx <= lastSpanIdx; spanIdx++ )
   {
      CComPtr<IPoint2dCollection> points;
      pBridge->GetSpanPerimeter(spanIdx,10,&points);

      CComPtr<IPolyShape> poly_shape;
      poly_shape.CoCreateInstance(CLSID_PolyShape);
      poly_shape->AddPoints(points);


      CComPtr<iPointDisplayObject> doPnt;
      doPnt.CoCreateInstance(CLSID_PointDisplayObject);
      CComPtr<IPoint2d> p;
      points->get_Item(0,&p);
      doPnt->SetPosition(p,FALSE,FALSE);

      CComPtr<iShapeDrawStrategy> strategy;
      strategy.CoCreateInstance(CLSID_ShapeDrawStrategy);
      CComQIPtr<IShape> shape(poly_shape);
      strategy->SetShape(shape);
      strategy->SetSolidLineColor(DECK_BORDER_COLOR);
      strategy->SetSolidFillColor(DECK_FILL_COLOR);
      strategy->DoFill(TRUE);
      doPnt->SetDrawingStrategy(strategy);

      doPnt->SetSelectionType(stAll);
      doPnt->SetID(spanIdx);

      SpanDisplayObjectInfo* pInfo = new SpanDisplayObjectInfo(spanIdx,SPAN_DISPLAY_LIST);
      doPnt->SetItemData((void*)pInfo,true);

      CComPtr<iShapeGravityWellStrategy> gravity_well;
      gravity_well.CoCreateInstance(CLSID_ShapeGravityWellStrategy);
      gravity_well->SetShape(shape);

      doPnt->SetGravityWellStrategy(gravity_well);

      CBridgePlanViewSpanDisplayObjectEvents* pEvents = new CBridgePlanViewSpanDisplayObjectEvents(spanIdx,m_pFrame);
      CComPtr<iDisplayObjectEvents> events;
      events.Attach((iDisplayObjectEvents*)pEvents->GetInterface(&IID_iDisplayObjectEvents));

      CComQIPtr<iDisplayObject,&IID_iDisplayObject> dispObj(doPnt);
      dispObj->RegisterEventSink(events);

      CString strMsg(_T("Double click to edit span.\r\nRight click for more options."));

      dispObj->SetToolTipText(strMsg);
      dispObj->SetMaxTipWidth(TOOLTIP_WIDTH);
      dispObj->SetTipDisplayTime(TOOLTIP_DURATION);

      display_list->AddDisplayObject(doPnt);
   }
}

void CBridgePlanView::BuildSlabDisplayObjects()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IBridge,pBridge);
   if ( pBridge->GetDeckType() == pgsTypes::sdtNone )
      return;

   GET_IFACE2(pBroker,IMaterials,pMaterial);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CDeckDescription2* pDeck = pBridgeDesc->GetDeckDescription();

   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   CComPtr<iDisplayList> display_list;
   dispMgr->FindDisplayList(SLAB_DISPLAY_LIST,&display_list);

   display_list->Clear();

   SpanIndexType nSpans = pBridge->GetSpanCount();
   SpanIndexType firstSpanIdx = (m_StartSpanIdx == ALL_SPANS ? 0 : m_StartSpanIdx);
   SpanIndexType lastSpanIdx  = (m_EndSpanIdx  == ALL_SPANS ? nSpans-1 : m_EndSpanIdx);
   CComPtr<IPoint2dCollection> points;
   pBridge->GetSlabPerimeter(firstSpanIdx,lastSpanIdx,30,&points);

   CComPtr<IPolyShape> poly_shape;
   poly_shape.CoCreateInstance(CLSID_PolyShape);
   poly_shape->AddPoints(points);


   CComPtr<iPointDisplayObject> doPnt;
   doPnt.CoCreateInstance(CLSID_PointDisplayObject);
   CComPtr<IPoint2d> p;
   points->get_Item(0,&p);
   doPnt->SetPosition(p,FALSE,FALSE);

   CComPtr<iShapeDrawStrategy> strategy;
   strategy.CoCreateInstance(CLSID_ShapeDrawStrategy);
   CComQIPtr<IShape> shape(poly_shape);
   strategy->SetShape(shape);
   strategy->SetSolidLineColor(DECK_BORDER_COLOR);
   strategy->SetSolidFillColor(DECK_FILL_COLOR);
   strategy->DoFill(TRUE);
   doPnt->SetDrawingStrategy(strategy);

   doPnt->SetSelectionType(stAll);
   doPnt->SetID(DECK_ID);

   DeckDisplayObjectInfo* pInfo = new DeckDisplayObjectInfo(DECK_ID,SLAB_DISPLAY_LIST);
   doPnt->SetItemData((void*)pInfo,true);

   CComPtr<iShapeGravityWellStrategy> gravity_well;
   gravity_well.CoCreateInstance(CLSID_ShapeGravityWellStrategy);
   gravity_well->SetShape(shape);

   doPnt->SetGravityWellStrategy(gravity_well);

   CPGSuperDocBase* pDoc = (CPGSuperDocBase*)GetDocument();
   CBridgePlanViewSlabDisplayObjectEvents* pEvents = new CBridgePlanViewSlabDisplayObjectEvents(pDoc, pBroker,m_pFrame,strategy->DoFill());
   CComPtr<iDisplayObjectEvents> events;
   events.Attach((iDisplayObjectEvents*)pEvents->GetInterface(&IID_iDisplayObjectEvents));

   CComQIPtr<iDisplayObject,&IID_iDisplayObject> dispObj(doPnt);
   dispObj->RegisterEventSink(events);

   CString strMsg1(_T("Double click to edit slab.\r\nRight click for more options."));

   CString strMsg2;

   if ( pDeck->DeckType != pgsTypes::sdtNone )
   {
      strMsg2.Format(_T("\r\n\r\nDeck: %s\r\nSlab Thickness: %s\r\nf'c: %s"),
                     m_pFrame->GetDeckTypeName(pDeck->DeckType),
                     FormatDimension(pDeck->GrossDepth,pDisplayUnits->GetComponentDimUnit()),
                     FormatDimension(pDeck->Concrete.Fc,pDisplayUnits->GetStressUnit())
                     );
   }

   CString strMsg3;
   Float64 overlay_weight = pBridge->GetOverlayWeight();
   if ( pBridge->HasOverlay() )
   {
      strMsg3.Format(_T("\r\n\r\n%s: %s"),
         pBridge->IsFutureOverlay() ? _T("Future Overlay") : _T("Overlay"),
         FormatDimension(overlay_weight,pDisplayUnits->GetOverlayWeightUnit()));
   }

   CString strMsg = strMsg1 + strMsg2 + strMsg3;

   dispObj->SetToolTipText(strMsg);
   dispObj->SetMaxTipWidth(TOOLTIP_WIDTH);
   dispObj->SetTipDisplayTime(TOOLTIP_DURATION);

   display_list->AddDisplayObject(doPnt);
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
   factory->Create(CBridgeSectionCutDisplayImpl::ms_Format,NULL,&disp_obj);

   CComPtr<iDisplayObjectEvents> sink;
   disp_obj->GetEventSink(&sink);

   disp_obj->SetSelectionType(stAll);

   CComQIPtr<iPointDisplayObject,&IID_iPointDisplayObject> point_disp(disp_obj);
   point_disp->SetMaxTipWidth(TOOLTIP_WIDTH);
   point_disp->SetToolTipText(_T("Drag me along the alignment to move section cut.\r\nDouble click to enter the cut station\r\nPress CTRL + -> to move ahead\r\nPress CTRL + <- to move back"));
   point_disp->SetTipDisplayTime(TOOLTIP_DURATION);

   GET_IFACE2(pBroker,IRoadway,pRoadway);
   GET_IFACE2(pBroker,IBridge,pBridge);
   CComQIPtr<iBridgeSectionCutDrawStrategy,&IID_iBridgeSectionCutDrawStrategy> section_cut_strategy(sink);
   section_cut_strategy->Init(m_pFrame, point_disp, pRoadway, pBridge, m_pFrame);
   section_cut_strategy->SetColor(CUT_COLOR);

   point_disp->SetID(SECTION_CUT_ID);

   display_list->Clear();
   UpdateSectionCut(point_disp,FALSE);
   display_list->AddDisplayObject(disp_obj);

   PierIndexType startPierIdx = (PierIndexType)m_StartSpanIdx;
   PierIndexType endPierIdx   = (PierIndexType)(m_EndSpanIdx+1);
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
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   CComPtr<iDisplayList> display_list;
   dispMgr->FindDisplayList(DIAPHRAGM_DISPLAY_LIST,&display_list);
   display_list->Clear();

   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IRoadway,pAlignment);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   SpanIndexType nSpans = pBridge->GetSpanCount();
   SpanIndexType firstSpanIdx = (m_StartSpanIdx == ALL_SPANS ? 0 : m_StartSpanIdx);
   SpanIndexType lastSpanIdx  = (m_EndSpanIdx  == ALL_SPANS ? nSpans-1 : m_EndSpanIdx);
   for ( SpanIndexType spanIdx = firstSpanIdx; spanIdx <= lastSpanIdx; spanIdx++ )
   {
      GirderIndexType nGirders = pBridge->GetGirderCount(spanIdx);
      for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders-1; gdrIdx++ )
      {
         CSegmentKey segmentKey1(spanIdx,gdrIdx,  0);
         CSegmentKey segmentKey2(spanIdx,gdrIdx+1,0);

         std::vector<IntermedateDiaphragm> left_diaphragms  = pBridge->GetIntermediateDiaphragms(pgsTypes::dtCastInPlace,segmentKey1);
         std::vector<IntermedateDiaphragm> right_diaphragms = pBridge->GetIntermediateDiaphragms(pgsTypes::dtCastInPlace,segmentKey2);

         std::vector<IntermedateDiaphragm>::iterator left_iter, right_iter;
         for ( left_iter = left_diaphragms.begin(), right_iter = right_diaphragms.begin(); 
               left_iter != left_diaphragms.end() && right_iter != right_diaphragms.end(); 
               left_iter++, right_iter++ )
         {
            IntermedateDiaphragm left_diaphragm  = *left_iter;
            IntermedateDiaphragm right_diaphragm = *right_iter;
            
            // Only add the diaphragm if it has width
            if ( !IsZero(left_diaphragm.W) && !IsZero(right_diaphragm.W) )
            {
	            Float64 station, offset;
	            pBridge->GetStationAndOffset(pgsPointOfInterest(segmentKey1,left_diaphragm.Location),&station,&offset);
	
	            CComPtr<IDirection> normal;
	            pAlignment->GetBearingNormal(station,&normal);
	            CComPtr<IPoint2d> pntLeft;
	            pAlignment->GetPoint(station,offset,normal,&pntLeft);
	
	            pBridge->GetStationAndOffset(pgsPointOfInterest(segmentKey2,right_diaphragm.Location),&station,&offset);
	            normal.Release();
	            pAlignment->GetBearingNormal(station,&normal);
	            CComPtr<IPoint2d> pntRight;
	            pAlignment->GetPoint(station,offset,normal,&pntRight);
	
	            // create a point on the left side of the diaphragm
	            CComPtr<iPointDisplayObject> doLeft;
	            doLeft.CoCreateInstance(CLSID_PointDisplayObject);
	            doLeft->SetPosition(pntLeft,FALSE,FALSE);
	            CComQIPtr<iConnectable> connectable1(doLeft);
	            CComPtr<iSocket> socket1;
	            connectable1->AddSocket(0,pntLeft,&socket1);
	
	            // create a point on the right side of the diaphragm
	            CComPtr<iPointDisplayObject> doRight;
	            doRight.CoCreateInstance(CLSID_PointDisplayObject);
	            doRight->SetPosition(pntRight,FALSE,FALSE);
	            CComQIPtr<iConnectable> connectable2(doRight);
	            CComPtr<iSocket> socket2;
	            connectable2->AddSocket(0,pntRight,&socket2);
	
	            // create a line for the diaphragm
	            CComPtr<iLineDisplayObject> doDiaphragmLine;
	            doDiaphragmLine.CoCreateInstance(CLSID_LineDisplayObject);
	
	            CComQIPtr<iConnector> connector(doDiaphragmLine);
	            CComQIPtr<iPlug> startPlug, endPlug;
	            connector->GetStartPlug(&startPlug);
	            connector->GetEndPlug(&endPlug);
	
	            // connect the line to the points
	            DWORD dwCookie;
	            connectable1->Connect(0,atByID,startPlug,&dwCookie);
	            connectable2->Connect(0,atByID,endPlug,  &dwCookie);
	
	            CComPtr<iExtRectangleDrawLineStrategy> strategy;
	            strategy.CoCreateInstance(CLSID_ExtRectangleDrawLineStrategy);
	            strategy->SetColor(DIAPHRAGM_BORDER_COLOR);
	            strategy->SetFillColor(DIAPHRAGM_FILL_COLOR);
	            strategy->SetDoFill(TRUE);
	
	            Float64 width = (left_diaphragm.T + right_diaphragm.T)/2;
	            strategy->SetLeftOffset(width/2);
	            strategy->SetRightOffset(width/2);
	
	            strategy->SetStartExtension(0);
	            strategy->SetEndExtension(0);
	
	            strategy->SetStartSkew(0);
	            strategy->SetEndSkew(0);
	
	            doDiaphragmLine->SetDrawLineStrategy(strategy);
	
	            CString strTip;
	            strTip.Format(_T("%s x %s intermediate diaphragm"),::FormatDimension(left_diaphragm.T,pDisplayUnits->GetComponentDimUnit()),::FormatDimension(left_diaphragm.H,pDisplayUnits->GetComponentDimUnit()));
	            doDiaphragmLine->SetMaxTipWidth(TOOLTIP_WIDTH);
	            doDiaphragmLine->SetTipDisplayTime(TOOLTIP_DURATION);
	            doDiaphragmLine->SetToolTipText(strTip);
	
	
	
	
	
	            CComQIPtr<iDisplayObject> dispObj(doDiaphragmLine);
	            display_list->AddDisplayObject(dispObj);
	
	         }
         }
      }
   }
}

int CBridgePlanView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CDisplayView::OnCreate(lpCreateStruct) == -1)
		return -1;
	
   m_pFrame = (CBridgeModelViewChildFrame*)GetParent()->GetParent();
   ASSERT( m_pFrame != 0 );
   ASSERT( m_pFrame->IsKindOf( RUNTIME_CLASS( CBridgeModelViewChildFrame ) ) );

	return 0;
}

void CBridgePlanView::UpdateDrawingScale()
{
   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   CComPtr<iDisplayList> title_display_list;
   dispMgr->FindDisplayList(TITLE_DISPLAY_LIST,&title_display_list);

   CComPtr<iDisplayList> na_display_list;
   dispMgr->FindDisplayList(NORTH_ARROW_DISPLAY_LIST,&na_display_list);

   if ( title_display_list == NULL )
      return;

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

CBridgeModelViewChildFrame* CBridgePlanView::GetFrame()
{
   return m_pFrame;
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
      ::CoCreateInstance(CLSID_DragDataSource,NULL,CLSCTX_ALL,IID_iDragDataSource,(void**)&source);
      source->SetDataObject(pDataObject);
      source->PrepareFormat(CBridgeSectionCutDisplayImpl::ms_Format);

      CWinThread* thread = ::AfxGetThread( );
      DWORD threadid = thread->m_nThreadID;

      DWORD threadl;
      // know (by voodoo) that the first member of this data source is the thread id
      source->Read(CBridgeSectionCutDisplayImpl::ms_Format,&threadl,sizeof(DWORD));

      if (threadl == threadid)
        return DROPEFFECT_MOVE;
   }

   return DROPEFFECT_NONE;
}

void CBridgePlanView::OnDropped(COleDataObject* pDataObject,DROPEFFECT dropEffect,IPoint2d* point)
{
   AfxMessageBox(_T("CBridgePlanView::OnDropped"));
}

void CBridgePlanView::OnSetFocus(CWnd* pOldWnd) 
{
	CDisplayView::OnSetFocus(pOldWnd);
   DrawFocusRect();
}

void CBridgePlanView::OnKillFocus(CWnd* pNewWnd) 
{
	CDisplayView::OnKillFocus(pNewWnd);
   DrawFocusRect();
}

void CBridgePlanView::DrawFocusRect()
{
   CClientDC dc(this);
   CRect rClient;
   GetClientRect(&rClient);
   dc.DrawFocusRect(rClient);
}

std::_tstring CBridgePlanView::GetConnectionString(const CPierData2* pPierData)
{
   std::_tstring strConnection;
   if ( pPierData->IsBoundaryPier() )
   {
      pgsTypes::PierConnectionType connectionType = pPierData->GetPierConnectionType();

      switch( connectionType )
      {
      case pgsTypes::Hinge:
         strConnection = _T("H");
         break;

      case pgsTypes::Roller:
         strConnection = _T("R");
         break;

      case pgsTypes::ContinuousAfterDeck:
         strConnection = _T("Ca");
         break;

      case pgsTypes::ContinuousBeforeDeck:
         strConnection = _T("Cb");
         break;

      case pgsTypes::IntegralAfterDeck:
         strConnection = _T("Ia");
         break;

      case pgsTypes::IntegralBeforeDeck:
         strConnection = _T("Ib");
         break;

      case pgsTypes::IntegralAfterDeckHingeBack:
         strConnection = _T("H Ia");
         break;

      case pgsTypes::IntegralBeforeDeckHingeBack:
         strConnection = _T("H Ib");
         break;

      case pgsTypes::IntegralAfterDeckHingeAhead:
         strConnection = _T("Ia H");
         break;

      case pgsTypes::IntegralBeforeDeckHingeAhead:
         strConnection = _T("Ib H");
         break;

      default:
         ATLASSERT(0); // who added a new connection type?
         strConnection = _T("?");
      }
   }
   else
   {
      pgsTypes::PierSegmentConnectionType connectionType = pPierData->GetSegmentConnectionType();
      switch(connectionType)
      {
      case pgsTypes::psctContinousClosureJoint:
         strConnection = _T("C-CP");
         break;

      case pgsTypes::psctContinuousSegment:
         strConnection = _T("C");
         break;

      case pgsTypes::psctIntegralClosureJoint:
         strConnection = _T("I-CP");
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
      pgsTypes::PierConnectionType connectionType = pPierData->GetPierConnectionType();

      switch( connectionType )
      {
      case pgsTypes::Hinge:
         strConnection = _T("Hinge");
         break;

      case pgsTypes::Roller:
         strConnection = _T("Roller");
         break;

      case pgsTypes::ContinuousAfterDeck:
         strConnection = _T("Continuous after deck placement");
         break;

      case pgsTypes::ContinuousBeforeDeck:
         strConnection = _T("Continuous before deck placement");
         break;

      case pgsTypes::IntegralAfterDeck:
         strConnection = _T("Integral after deck placement");
         break;

      case pgsTypes::IntegralBeforeDeck:
         strConnection = _T("Integral before deck placement");
         break;

      case pgsTypes::IntegralAfterDeckHingeBack:
         strConnection = _T("Hinge | Integral after deck placement");
         break;

      case pgsTypes::IntegralBeforeDeckHingeBack:
         strConnection = _T("Hinge | Integral before deck placement");
         break;

      case pgsTypes::IntegralAfterDeckHingeAhead:
         strConnection = _T("Integral after deck placement | Hinge");
         break;

      case pgsTypes::IntegralBeforeDeckHingeAhead:
         strConnection = _T("Integral before deck placement | Hinge");
         break;

      default:
         ATLASSERT(0); // who added a new connection type?
         strConnection = _T("?????");
      }
   }
   else
   {
      pgsTypes::PierSegmentConnectionType connectionType = pPierData->GetSegmentConnectionType();
      switch(connectionType)
      {
      case pgsTypes::psctContinousClosureJoint:
         strConnection = _T("Continuous Closure Joint");
         break;

      case pgsTypes::psctContinuousSegment:
         strConnection = _T("Continuous Segment");
         break;

      case pgsTypes::psctIntegralClosureJoint:
         strConnection = _T("Integral Closure Joint");
         break;

      case pgsTypes::psctIntegralSegment:
         strConnection = _T("Integral");
         break;

      default:
         ATLASSERT(false); // who added a new connection type?
         strConnection = _T("?");
      }
   }

   return strConnection;
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
      ATLASSERT(0); // who added a new connection type?
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
      ATLASSERT(0); // who added a new connection type?
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
   taskFactory->CreateZoomTask(this,NULL,LIGHTSTEELBLUE,&task);

   dispMgr->SetTask(task);
}

void CBridgePlanView::OnScaleToFit()
{
   UpdateDrawingScale();
}
