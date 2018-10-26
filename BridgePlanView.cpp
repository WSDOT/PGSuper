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

// BridgePlanView.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\resource.h"
#include "PGSuperAppPlugin\PGSuperApp.h"
#include "PGSuperDoc.h"
#include "PGSuperUnits.h"
#include <IFace\DrawBridgeSettings.h>
#include "BridgePlanView.h"
#include "PGSuperColors.h"
#include "AlignmentDisplayObjectEvents.h"
#include "InplaceSpanLengthEditEvents.h"
#include "InplacePierStationEditEvents.h"
#include "GirderDisplayObjectEvents.h"
#include "PierDisplayObjectEvents.h"
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

#include <PgsExt\BridgeDescription.h>
#include <MfcTools\Text.h>
#include <WBFLDManip.h>
#include <WBFLDManipTools.h>

#include <Material\Material.h>

#include <EAF\EAFMenu.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define TITLE_DISPLAY_LIST       0
#define ALIGNMENT_DISPLAY_LIST   1
#define PIER_DISPLAY_LIST        2
#define GIRDER_DISPLAY_LIST      3
#define BEARING_DISPLAY_LIST     4
#define SPAN_DISPLAY_LIST        5
#define SLAB_DISPLAY_LIST        6
#define LABEL_DISPLAY_LIST       7
#define SECTION_CUT_DISPLAY_LIST 8
#define NORTH_ARROW_DISPLAY_LIST 9
#define DIAPHRAGM_DISPLAY_LIST   10

#define SECTION_CUT_ID -200
#define ALIGNMENT_ID   -300

//#define _SHOW_CL_GIRDER

/////////////////////////////////////////////////////////////////////////////
// CBridgePlanView

IMPLEMENT_DYNCREATE(CBridgePlanView, CDisplayView)

CBridgePlanView::CBridgePlanView()
{
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
bool CBridgePlanView::GetSelectedGirder(SpanIndexType* pSpanIdx,GirderIndexType* pGirderIdx)
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

   UnhashSpanGirder(pInfo->SpanGirderHash,pSpanIdx,pGirderIdx);
   return true;
}

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

void CBridgePlanView::SelectGirder(SpanIndexType spanIdx,GirderIndexType gdrIdx,bool bSelect)
{
   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   SpanGirderHashType hash = HashSpanGirder(spanIdx,gdrIdx);
   std::map<SpanGirderHashType,long>::iterator found = m_GirderIDs.find(hash);
   if ( found == m_GirderIDs.end() )
   {
      dispMgr->ClearSelectedObjects();
      return;
   }

   long ID = (*found).second;

   CComPtr<iDisplayObject> pDO;
   dispMgr->FindDisplayObject(ID,GIRDER_DISPLAY_LIST,atByID,&pDO);

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

void CBridgePlanView::OnInitialUpdate() 
{
   EnableToolTips();

   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   CPGSuperDoc* pDoc = (CPGSuperDoc*)GetDocument();
   CDisplayObjectFactory* factory = new CDisplayObjectFactory(pDoc);
   IUnknown* unk = factory->GetInterface(&IID_iDisplayObjectFactory);
   dispMgr->AddDisplayObjectFactory((iDisplayObjectFactory*)unk);

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

   CDisplayView::OnInitialUpdate();
	
   UpdateDisplayObjects();
   UpdateDrawingScale();
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
      UpdateDisplayObjects();
      UpdateDrawingScale();
   }
   else if ( lHint == HINT_GIRDERCHANGED )
   {
      UpdateGirderTooltips();
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
         this->SelectGirder( pSelection->SpanIdx,pSelection->GirderIdx,true);
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

      default:
         ATLASSERT(FALSE); // is there a new type of object to be selected?
         this->ClearSelection();
         break;
      }
   }
}

void CBridgePlanView::UpdateGirderTooltips()
{
   CPGSuperDoc* pDoc = (CPGSuperDoc*)GetDocument();
   CComPtr<IBroker> pBroker;
   pDoc->GetBroker(&pBroker);

   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IBridgeMaterialEx,pBridgeMaterial);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeom);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);

   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   CComPtr<iDisplayList> display_list;
   dispMgr->FindDisplayList(GIRDER_DISPLAY_LIST,&display_list);

   SpanIndexType nSpans = pBridge->GetSpanCount();
   for ( SpanIndexType spanIdx = 0; spanIdx < nSpans; spanIdx++ )
   {
      GirderIndexType nGirders = pBridge->GetGirderCount(spanIdx);
      for ( GirderIndexType girderIdx = 0; girderIdx < nGirders; girderIdx++ )
      {
         SpanGirderHashType hash = HashSpanGirder(spanIdx,girderIdx);
         std::map<SpanGirderHashType,long>::iterator found = m_GirderIDs.find(hash);
         if ( found == m_GirderIDs.end() )
            continue;

         long ID = (*found).second;

         CComPtr<iDisplayObject> pDO;
         display_list->FindDisplayObject(ID,&pDO);

         CComPtr<IDirection> direction;
         pBridge->GetGirderBearing(spanIdx,girderIdx,&direction);

         CString strMsg1;
         strMsg1.Format(_T("Double click to edit Span %d Girder %s\r\nRight click for more options."),LABEL_SPAN(spanIdx),LABEL_GIRDER(girderIdx));

         double gdr_length, span_length;
         gdr_length  = pBridge->GetGirderLength(spanIdx,girderIdx);
         span_length = pBridge->GetSpanLength(spanIdx,girderIdx);
         CString strMsg2;
         strMsg2.Format(_T("\r\n\r\nGirder: %s\r\nGirder Length: %s\r\nSpan Length: %s\r\n\r\n%s"),
                        pBridgeDesc->GetSpan(spanIdx)->GetGirderTypes()->GetGirderName(girderIdx),
                        FormatDimension(gdr_length,pDisplayUnits->GetSpanLengthUnit()),
                        FormatDimension(span_length,pDisplayUnits->GetSpanLengthUnit()),
                        FormatDirection(direction)
                        );

         double fc, fci;
         fc = pBridgeMaterial->GetFcGdr(spanIdx,girderIdx);
         fci = pBridgeMaterial->GetFciGdr(spanIdx,girderIdx);

         CString strMsg3;
         strMsg3.Format(_T("\r\n\r\n%s\r\nf'ci: %s\r\nf'c: %s"),
                        matConcrete::GetTypeName((matConcrete::Type)pBridgeMaterial->GetGdrConcreteType(spanIdx,girderIdx),true).c_str(),
                        FormatDimension(fci,pDisplayUnits->GetStressUnit()),
                        FormatDimension(fc, pDisplayUnits->GetStressUnit())
                        );

         const matPsStrand* pStrand = pBridgeMaterial->GetStrand(spanIdx,girderIdx,pgsTypes::Permanent);
         const matPsStrand* pTempStrand = pBridgeMaterial->GetStrand(spanIdx,girderIdx,pgsTypes::Temporary);

         StrandIndexType Ns, Nh, Nt, Nsd;
         Ns = pStrandGeom->GetNumStrands(spanIdx,girderIdx,pgsTypes::Straight);
         Nh = pStrandGeom->GetNumStrands(spanIdx,girderIdx,pgsTypes::Harped);
         Nt = pStrandGeom->GetNumStrands(spanIdx,girderIdx,pgsTypes::Temporary);
         Nsd= pStrandGeom->GetNumDebondedStrands(spanIdx,girderIdx,pgsTypes::Straight);

         CString strMsg4;
         if ( pStrandGeom->GetMaxStrands(spanIdx,girderIdx,pgsTypes::Temporary) != 0 )
         {
            if ( Nsd == 0 )
            {
               strMsg4.Format(_T("\r\n\r\nStrand: %s\r\n# Straight: %2d\r\n# Harped: %2d\r\n\r\n%s\r\n# Temporary: %2d"),
                               pStrand->GetName().c_str(),Ns,Nh,pTempStrand->GetName().c_str(),Nt);
            }
            else
            {
               strMsg4.Format(_T("\r\n\r\nStrand: %s\r\n# Straight: %2d (%2d Debonded)\r\n# Harped: %2d\r\n\r\n%s\r\n# Temporary: %2d"),
                               pStrand->GetName().c_str(),Ns,Nsd,Nh,pTempStrand->GetName().c_str(),Nt);
            }
         }
         else
         {
            if ( Nsd == 0 )
            {
               strMsg4.Format(_T("\r\n\r\nStrand: %s\r\n# Straight: %2d\r\n# Harped: %2d"),
                               pStrand->GetName().c_str(),Ns,Nh);
            }
            else
            {
               strMsg4.Format(_T("\r\n\r\nStrand: %s\r\n# Straight: %2d (%2d Debonded)\r\n# Harped: %2d"),
                               pStrand->GetName().c_str(),Ns,Nsd,Nh);
            }
         }

         CString strMsg = strMsg1 + strMsg2 + strMsg3 + strMsg4;

         pDO->SetMaxTipWidth(TOOLTIP_WIDTH);
         pDO->SetTipDisplayTime(TOOLTIP_DURATION);
         pDO->SetToolTipText(strMsg);
      }
   }
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
   double station = m_pFrame->GetCurrentCutLocation();

   CPGSuperDoc* pDoc = (CPGSuperDoc*)GetDocument();
   CComPtr<IBroker> pBroker;
   pDoc->GetBroker(&pBroker);
   
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
   size.cx = max(0,size.cx);
   size.cy = max(0,size.cy);

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
   ((CPGSuperDoc*)GetDocument())->EditBridgeDescription(EBD_GENERAL);
}

void CBridgePlanView::HandleContextMenu(CWnd* pWnd,CPoint logPoint)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CPGSuperDoc* pDoc = (CPGSuperDoc*)GetDocument();
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
   ((CPGSuperDoc*)GetDocument())->EditAlignmentDescription(EBD_ROADWAY);
}

void CBridgePlanView::OnEditBridge() 
{
   ((CPGSuperDoc*)GetDocument())->EditBridgeDescription(EBD_GENERAL);
}

void CBridgePlanView::OnEditDeck() 
{
   ((CPGSuperDoc*)GetDocument())->EditBridgeDescription(EBD_DECK);
}

void CBridgePlanView::OnViewSettings() 
{
   ((CPGSuperDoc*)GetDocument())->EditBridgeViewSettings(VS_BRIDGE_PLAN);
}

void CBridgePlanView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
   if ( nChar == VK_LEFT || nChar == VK_RIGHT )
   {
      CPGSuperDoc* pDoc = (CPGSuperDoc*)GetDocument();
      CComPtr<IBroker> pBroker;
      pDoc->GetBroker(&pBroker);
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
         double station = pFrame->GetCurrentCutLocation();

         SpanIndexType spanIdx;
         if ( !pBridge->GetSpan(station,&spanIdx) )
            return;

         double back_pier = pBridge->GetPierStation(spanIdx);
         double ahead_pier = pBridge->GetPierStation(spanIdx+1);
         double span_length = ahead_pier - back_pier;
         double inc = span_length/10;

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
   CPGSuperDoc* pDoc = (CPGSuperDoc*)GetDocument();
   CComPtr<IBroker> pBroker;
   pDoc->GetBroker(&pBroker);
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

   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   // capture current selection
   SpanIndexType   span;
   GirderIndexType gdr;
   PierIndexType   pier;
   bool bGirderSelected = GetSelectedGirder(&span,&gdr);
   bool bDeckSelected   = IsDeckSelected();
   bool bPierSelected   = GetSelectedPier(&pier);
   bool bSpanSelected   = GetSelectedSpan(&span);
   bool bAlignmentSelected = IsAlignmentSelected();

   CDManipClientDC dc(this);

   dispMgr->ClearDisplayObjects();

   BuildTitleDisplayObjects();
   BuildAlignmentDisplayObjects();

   BuildGirderDisplayObjects();
   BuildPierDisplayObjects();
   BuildSpanDisplayObjects();
   BuildSlabDisplayObjects();
   BuildSectionCutDisplayObjects();
   BuildNorthArrowDisplayObjects();
   BuildDiaphragmDisplayObjects();
   
   UpdateGirderTooltips();

   // restore the selection
   if ( bGirderSelected )
      SelectGirder(span,gdr,TRUE);
   else if ( bDeckSelected )
      SelectDeck(true);
   else if ( bPierSelected )
      SelectPier(pier,TRUE);
   else if ( bSpanSelected )
      SelectSpan(span,TRUE);
   else if ( bAlignmentSelected )
      SelectAlignment(true);
   else
      ClearSelection();

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

   title->SetText(_T("Plan View"));
   title_list->AddDisplayObject(title);
}

void CBridgePlanView::BuildAlignmentDisplayObjects()
{
   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   CComPtr<iDisplayList> display_list;
   dispMgr->FindDisplayList(ALIGNMENT_DISPLAY_LIST,&display_list);

   display_list->Clear();

   CPGSuperDoc* pDoc = (CPGSuperDoc*)GetDocument();
   CComPtr<IBroker> pBroker;
   pDoc->GetBroker(&pBroker);

   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IRoadway,pRoadway);

   // show that part of the alignment from 1/n of the first span length before the start of the bridge
   // to 1/n of the last span length beyond the end of the bridge
   PierIndexType nPiers = pBridge->GetPierCount();
   double start_station = pBridge->GetPierStation(0);
   double end_station = pBridge->GetPierStation(nPiers-1);
   double length1 = pBridge->GetPierStation(1) - start_station;
   double length2 = end_station - pBridge->GetPierStation(nPiers-2);

   // project the edges of the first and last pier onto the alignment
   // use the min/max station as the start and end of the bridge for purposes
   // of defining the alignment station range
   CComPtr<IPoint2d> start_left, start_alignment, start_bridge, start_right;
   pBridge->GetPierPoints(0,&start_left,&start_alignment,&start_bridge,&start_right);
   double start1,start2,start3,start4,offset;
   pRoadway->GetStationAndOffset(start_left,&start1,&offset);
   pRoadway->GetStationAndOffset(start_alignment,&start2,&offset);
   pRoadway->GetStationAndOffset(start_bridge,&start3,&offset);
   pRoadway->GetStationAndOffset(start_right,&start4,&offset);
   start_station = Min4(start1,start2,start3,start4);
   start_station -= length1/10;

   CComPtr<IPoint2d> end_left, end_alignment, end_bridge, end_right;
   pBridge->GetPierPoints(nPiers-1,&end_left,&end_alignment,&end_bridge,&end_right);
   double end1,end2,end3,end4;
   pRoadway->GetStationAndOffset(end_left,&end1,&offset);
   pRoadway->GetStationAndOffset(end_alignment,&end2,&offset);
   pRoadway->GetStationAndOffset(end_bridge,&end3,&offset);
   pRoadway->GetStationAndOffset(end_right,&end4,&offset);
   end_station = Max4(end1,end2,end3,end4);
   end_station += length2/10;

   // The alignment is represented on the screen by a poly line object
   CComPtr<iPolyLineDisplayObject> doAlignment;
   doAlignment.CoCreateInstance(CLSID_PolyLineDisplayObject);

   // Register an event sink with the alignment object so that we can handle double clicks
   // on the alignment differently then a general double click
   CAlignmentDisplayObjectEvents* pEvents = new CAlignmentDisplayObjectEvents(pBroker,m_pFrame);
   IUnknown* unk = pEvents->GetInterface(&IID_iDisplayObjectEvents);
   CComQIPtr<iDisplayObjectEvents,&IID_iDisplayObjectEvents> events(unk);
   CComPtr<iDisplayObject> dispObj;
   doAlignment->QueryInterface(IID_iDisplayObject,(void**)&dispObj);
   dispObj->RegisterEventSink(events);
   dispObj->SetToolTipText(_T("Double click to edit alignment.\r\nRight click for more options."));
   dispObj->SetMaxTipWidth(TOOLTIP_WIDTH);
   dispObj->SetTipDisplayTime(TOOLTIP_DURATION);

   // display object for CL bridge
   CComPtr<iPolyLineDisplayObject> doCLBridge;
   doCLBridge.CoCreateInstance(CLSID_PolyLineDisplayObject);

   double alignment_offset = pBridge->GetAlignmentOffset();

   // model the alignment as a series of individual points
   CComPtr<IDirection> bearing;
   bearing.CoCreateInstance(CLSID_Direction);
   long nPoints = 50;
   double station_inc = (end_station - start_station)/nPoints;
   double station = start_station;
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
   dispObj->SetDropSite(drop_site);
}

void CBridgePlanView::BuildGirderDisplayObjects()
{
   USES_CONVERSION;

   CBridgeModelViewChildFrame* pFrame = GetFrame();

   CPGSuperDoc* pDoc = (CPGSuperDoc*)GetDocument();
   CComPtr<IBroker> pBroker;
   pDoc->GetBroker(&pBroker);

   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   CComPtr<iDisplayList> display_list;
   dispMgr->FindDisplayList(GIRDER_DISPLAY_LIST,&display_list);
   display_list->Clear();

   CComPtr<iDisplayList> bearing_display_list;
   dispMgr->FindDisplayList(BEARING_DISPLAY_LIST,&bearing_display_list);
   bearing_display_list->Clear();

   CComPtr<iDisplayList> label_display_list;
   dispMgr->FindDisplayList(LABEL_DISPLAY_LIST,&label_display_list);
   label_display_list->Clear();

   GET_IFACE2(pBroker,IBridge,pBridge);

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
   m_GirderIDs.clear();
   m_NextGirderID = 0;


#if defined _SHOW_CL_GIRDER
   // girder line
   CComPtr<iSimpleDrawLineStrategy> strategy_girder;
   strategy_girder.CoCreateInstance(CLSID_SimpleDrawLineStrategy);
   strategy_girder->SetColor(RGB(0,128,0));
   strategy_girder->SetLineStyle(lsCenterline);
#endif // _SHOW_CL_GIRDER


   GET_IFACE2(pBroker,IGirder,pGirder);
   SpanIndexType nSpans = pBridge->GetSpanCount();
   for ( SpanIndexType spanIdx = 0; spanIdx < nSpans; spanIdx++ )
   {
      CComPtr<IDirection> objStartDirection,objEndDirection;
      pBridge->GetPierDirection(spanIdx,  &objStartDirection);
      pBridge->GetPierDirection(spanIdx+1,&objEndDirection);

      double start_direction, end_direction;
      objStartDirection->get_Value(&start_direction);
      objEndDirection->get_Value(&end_direction);

      // start and end pier connectables for the previous time through the loop
      CComPtr<iConnectable> prev_connectable_brg1;
      CComPtr<iConnectable> prev_connectable_brg2;

      GirderIndexType nGirders = pBridge->GetGirderCount(spanIdx);
      for ( GirderIndexType girderIdx = 0; girderIdx < nGirders; girderIdx++ )
      {
         // get the girder geometry points
         CComPtr<IPoint2d> pntPier1,pntEnd1,pntBrg1,pntBrg2,pntEnd2,pntPier2;
         pGirder->GetGirderEndPoints(spanIdx,girderIdx,&pntPier1,&pntEnd1,&pntBrg1,&pntBrg2,&pntEnd2,&pntPier2);

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

         // create a display object for the girder line
         CComPtr<iLineDisplayObject> doGirderLine;
         doGirderLine.CoCreateInstance(CLSID_LineDisplayObject);

         SpanGirderHashType hash = HashSpanGirder(spanIdx,girderIdx);
         long ID = m_NextGirderID++;
         m_GirderIDs.insert( std::make_pair(hash,ID) );

         doGirderLine->SetID(ID);

         GirderDisplayObjectInfo* pInfo = new GirderDisplayObjectInfo(hash,GIRDER_DISPLAY_LIST);
         doGirderLine->SetItemData((void*)pInfo,true);

         // connect the girder line display object to the display objects
         // at the girder ends
         CComQIPtr<iConnector> connector(doGirderLine);
         CComQIPtr<iPlug> startPlug, endPlug;
         connector->GetStartPlug(&startPlug);
         connector->GetEndPlug(&endPlug);

         DWORD dwCookie;
         connectable_end1->Connect(0,atByID,startPlug,&dwCookie);
         connectable_end2->Connect(0,atByID,endPlug,  &dwCookie);

         // drawing strategy
         // use a compound strategy so we can draw both centerline and the top flange rectangle
         CComPtr<iCompoundDrawLineStrategy> strategy;
         strategy.CoCreateInstance(CLSID_CompoundDrawLineStrategy);

         CComPtr<iExtRectangleDrawLineStrategy> strategy1;
         strategy1.CoCreateInstance(CLSID_ExtRectangleDrawLineStrategy);

#pragma Reminder("UPDATE: Assuming constant top width of girder")
         // assumes top flange of girder is a rectangle (parallelogram) in plan view
         // a future version may go back to the beam factory to get a shape
         // that represents the top flange (curved girders???)
         double top_width = pGirder->GetTopWidth(pgsPointOfInterest(spanIdx,girderIdx,0.00));
         strategy1->SetLeftOffset(top_width/2);
         strategy1->SetRightOffset(top_width/2);

         strategy1->SetColor(GIRDER_BORDER_COLOR);
         strategy1->SetDoFill(TRUE);
         strategy1->SetFillColor(GIRDER_FILL_COLOR);

         // this strategy doubles as a gravity well.. get its interface and give it to 
         // the line display object. this will make the entire top flange the clickable part
         // of the display object
         strategy1->PerimeterGravityWell(TRUE);
         CComQIPtr<iGravityWellStrategy> gravity_well(strategy1);
         doGirderLine->SetGravityWellStrategy(gravity_well);
         doGirderLine->SetSelectionType(stAll);

         CComPtr<IDirection> objBearing;
         pBridge->GetGirderBearing(spanIdx,girderIdx,&objBearing);
         objBearing->IncrementBy(CComVariant(PI_OVER_2));
         double bearing;
         objBearing->get_Value(&bearing);
         double start_skew = start_direction - bearing;
         double end_skew = end_direction - bearing;
         strategy1->SetStartSkew(start_skew);
         strategy1->SetEndSkew(end_skew);
         strategy->AddStrategy(strategy1);

#if defined _SHOW_CL_GIRDER
         strategy->AddStrategy(strategy_girder);
#endif
         doGirderLine->SetDrawLineStrategy(strategy);

         display_list->AddDisplayObject(doEnd1);
         display_list->AddDisplayObject(doEnd2);
         display_list->AddDisplayObject(doGirderLine);

         display_list->AddDisplayObject(doBrg1);
         display_list->AddDisplayObject(doBrg2);


         // direction to output text
         CComPtr<IDirection> direction;
         pBridge->GetGirderBearing(spanIdx,girderIdx,&direction);
         double dir;
         direction->get_Value(&dir);
         long angle = long(1800.*dir/M_PI);
         angle = (900 < angle && angle < 2700 ) ? angle-1800 : angle;

         if ( settings & IDB_PV_LABEL_GIRDERS )
         {
            // girder labels
            double x1,y1, x2,y2;
            pntEnd1->get_X(&x1);
            pntEnd1->get_Y(&y1);
            pntEnd2->get_X(&x2);
            pntEnd2->get_Y(&y2);
            double x = x1 + (x2-x1)/4;
            double y = y1 + (y2-y1)/4;
            CComPtr<IPoint2d> pntText;
            pntText.CoCreateInstance(CLSID_Point2d);
            pntText->Move(x,y);
            CComPtr<iTextBlock> doText;
            doText.CoCreateInstance(CLSID_TextBlock);
            doText->SetPosition(pntText);

            CString strText;
            strText.Format(_T("%s"),LABEL_GIRDER(girderIdx));
            doText->SetText(strText);
            doText->SetTextAlign(TA_BOTTOM | TA_CENTER);
            doText->SetBkMode(TRANSPARENT);
            doText->SetAngle(angle);

            label_display_list->AddDisplayObject(doText);
         }

         if ( settings & IDB_PV_LABEL_BEARINGS )
         {
            // Labels the girder bearings            
            CComPtr<IDirectionDisplayUnitFormatter> direction_formatter;
            direction_formatter.CoCreateInstance(CLSID_DirectionDisplayUnitFormatter);
            direction_formatter->put_BearingFormat(VARIANT_TRUE);
            CComPtr<iTextBlock> doText2;
            doText2.CoCreateInstance(CLSID_TextBlock);

            double x1,y1, x2,y2;
            pntEnd1->get_X(&x1);
            pntEnd1->get_Y(&y1);
            pntEnd2->get_X(&x2);
            pntEnd2->get_Y(&y2);

            double x = x1 + (x2-x1)/2;
            double y = y1 + (y2-y1)/2;
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

#if defined _SHOW_CL_GIRDER
         // layout centerline of bearing lines
         if ( girderIdx != 0 )
         {
            CComPtr<iLineDisplayObject> doStartBearingLine;
            doStartBearingLine.CoCreateInstance(CLSID_LineDisplayObject);
            CComQIPtr<iConnector> start_connector(doStartBearingLine);

            startPlug.Release();
            endPlug.Release();
            start_connector->GetStartPlug(&startPlug);
            start_connector->GetEndPlug(&endPlug);

            prev_connectable_brg1->Connect(0,atByID,startPlug,&dwCookie);
            connectable_brg1->Connect(0,atByID,endPlug,  &dwCookie);

            doStartBearingLine->SetDrawLineStrategy(strategy_brg);
            display_list->AddDisplayObject(doStartBearingLine);


            CComPtr<iLineDisplayObject> doEndBearingLine;
            doEndBearingLine.CoCreateInstance(CLSID_LineDisplayObject);
            CComQIPtr<iConnector> end_connector(doEndBearingLine);

            startPlug.Release();
            endPlug.Release();
            end_connector->GetStartPlug(&startPlug);
            end_connector->GetEndPlug(&endPlug);

            prev_connectable_brg2->Connect(0,atByID,startPlug,&dwCookie);
            connectable_brg2->Connect(0,atByID,endPlug,  &dwCookie);


            doEndBearingLine->SetDrawLineStrategy(strategy_brg);
            display_list->AddDisplayObject(doEndBearingLine);
         }
#endif // _SHOW_CL_GIRDER

         // Register an event sink with the girder display object so that we can handle double clicks
         // on the girder differently then a general double click
         CBridgePlanViewGirderDisplayObjectEvents* pEvents = new CBridgePlanViewGirderDisplayObjectEvents(spanIdx,girderIdx,nSpans,nGirders,pFrame);
         IUnknown* unk = pEvents->GetInterface(&IID_iDisplayObjectEvents);
         CComQIPtr<iDisplayObjectEvents,&IID_iDisplayObjectEvents> events(unk);
         CComQIPtr<iDisplayObject,&IID_iDisplayObject> dispObj(doGirderLine);
         dispObj->RegisterEventSink(events);
         unk->Release();
         events.Release();

         prev_connectable_brg1 = connectable_brg1;
         prev_connectable_brg2 = connectable_brg2;
      }
   }
}

void CBridgePlanView::BuildPierDisplayObjects()
{
   CBridgeModelViewChildFrame* pFrame = GetFrame();

   CPGSuperDoc* pDoc = (CPGSuperDoc*)GetDocument();
   CComPtr<IBroker> pBroker;
   pDoc->GetBroker(&pBroker);

   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   CComPtr<iDisplayList> display_list;
   dispMgr->FindDisplayList(PIER_DISPLAY_LIST,&display_list);
   display_list->Clear();

   CComPtr<iDisplayList> label_display_list;
   dispMgr->FindDisplayList(LABEL_DISPLAY_LIST,&label_display_list);
   //label_display_list->Clear(); // Don't clear it...BuildGirderDisplayObjects put some stuff in here

   UINT settings = pDoc->GetBridgeEditorSettings();

   GET_IFACE2(pBroker,IRoadway,pAlignment);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   GET_IFACE2(pBroker,IGirder,pGirder);

   CComPtr<IDocUnitSystem> docUnitSystem;
   pDoc->GetDocUnitSystem(&docUnitSystem);

   GET_IFACE2(pBroker,IBridge,pBridge);
   PierIndexType nPiers = pBridge->GetPierCount();
   SpanIndexType nSpans = pBridge->GetSpanCount();
   double last_station;
   for ( PierIndexType pierIdx = 0; pierIdx < nPiers; pierIdx++ )
   {
      const CPierData* pPier = pIBridgeDesc->GetPier(pierIdx);

      // get station of the pier
      double station = pBridge->GetPierStation(pierIdx);
   
      CComPtr<IDirection> direction;
      pBridge->GetPierDirection(pierIdx,&direction);

      // skew the pier so it parallels the alignment
      CComPtr<IAngle> objSkew;
      pBridge->GetPierSkew(pierIdx,&objSkew);
      double skew;
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
      strMsg1.Format(_T("Double click to edit %s %d\r\nRight click for more options."),(pierIdx == 0 || pierIdx == nPiers-1 ? _T("Abutment") : _T("Pier")),pierIdx+1);

      CString strMsg2;
      strMsg2.Format(_T("Station: %s\r\nDirection: %s\r\nSkew: %s"),FormatStation(pDisplayUnits->GetStationFormat(),station),FormatDirection(direction),FormatAngle(objSkew));

      CString strConnectionTip;
      if ( pierIdx == 0 ) // first pier
      {
         strConnectionTip.Format(_T("Ahead Connection: %s\nBoundary Conditions: %s"),pPier->GetConnection(pgsTypes::Ahead),CPierData::AsString(pPier->GetConnectionType()));
      }
      else if ( pierIdx == nPiers-1 ) // last pier
      {
         strConnectionTip.Format(_T("Back Connection: %s\nBoundary Conditions: %s"),pPier->GetConnection(pgsTypes::Back),CPierData::AsString(pPier->GetConnectionType()));
      }
      else // intermediate pier
      {
         strConnectionTip.Format(_T("Back Connection: %s\nAhead Connection: %s\nBoundary Conditions: %s"),pPier->GetConnection(pgsTypes::Back),pPier->GetConnection(pgsTypes::Ahead),CPierData::AsString(pPier->GetConnectionType()));
      }

      CString strMsg = strMsg1 + _T("\r\n\r\n") + strMsg2 + _T("\r\n") + strConnectionTip;

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

      // Register an event sink with the pier centerline display object so that we can handle double clicks
      // on the piers differently then a general double click
      CPierDisplayObjectEvents* pEvents = new CPierDisplayObjectEvents(pierIdx,
                                                                       nPiers,
                                                                       pBridge->GetDeckType() != pgsTypes::sdtNone,
                                                                       pFrame);
      IUnknown* unk = pEvents->GetInterface(&IID_iDisplayObjectEvents);
      CComQIPtr<iDisplayObjectEvents,&IID_iDisplayObjectEvents> events(unk);
      CComQIPtr<iDisplayObject,&IID_iDisplayObject> dispObj(doCenterLine);
      dispObj->RegisterEventSink(events);
      unk->Release();
      events.Release();

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
      SpanIndexType next_span_idx = pierIdx == nSpans ? INVALID_INDEX : pierIdx;

      double left_offset  = 0;
      double right_offset = 0;
      if ( prev_span_idx != ALL_SPANS)
         left_offset  = 1.05*(pBridge->GetGirderEndBearingOffset(prev_span_idx,0) + pBridge->GetGirderEndSupportWidth(prev_span_idx,0));

      if ( next_span_idx != ALL_SPANS)
         right_offset = 1.05*(pBridge->GetGirderStartBearingOffset(next_span_idx,0) + pBridge->GetGirderStartSupportWidth(next_span_idx,0));

      left_offset  = ( IsZero(left_offset)  ? right_offset/2 : left_offset );
      right_offset = ( IsZero(right_offset) ? left_offset/2  : right_offset );
      strategy_pier->SetLeftOffset(left_offset);
      strategy_pier->SetRightOffset(right_offset);

      // make the pier overhang the exterior girders
      double prev_girder_length = (prev_span_idx == ALL_SPANS ? 0 : pBridge->GetGirderLength(prev_span_idx,0));
      double prev_top_width = (prev_span_idx == ALL_SPANS ? 0 : pGirder->GetTopWidth(pgsPointOfInterest(prev_span_idx,0,prev_girder_length)));
      double prev_bot_width = (prev_span_idx == ALL_SPANS ? 0 : pGirder->GetBottomWidth(pgsPointOfInterest(prev_span_idx,0,prev_girder_length)));
      double next_top_width = (next_span_idx == ALL_SPANS ? 0 : pGirder->GetTopWidth(pgsPointOfInterest(next_span_idx,0,0)));
      double next_bot_width = (next_span_idx == ALL_SPANS ? 0 : pGirder->GetBottomWidth(pgsPointOfInterest(next_span_idx,0,0)));
      double left_overhang = Max4(prev_top_width,prev_bot_width,next_top_width,next_bot_width)/2;
      left_overhang /= cos(fabs(skew));
      left_overhang *= 1.10;

      GirderIndexType nGirdersPrevSpan = (prev_span_idx == ALL_SPANS ? 0 : pBridge->GetGirderCount(prev_span_idx));
      GirderIndexType nGirdersNextSpan = (next_span_idx == ALL_SPANS ? 0 : pBridge->GetGirderCount(next_span_idx));
      prev_girder_length = (prev_span_idx == ALL_SPANS ? 0 : pBridge->GetGirderLength(prev_span_idx,nGirdersPrevSpan-1));
      prev_top_width = (prev_span_idx == ALL_SPANS ? 0 : pGirder->GetTopWidth(pgsPointOfInterest(prev_span_idx,nGirdersPrevSpan-1,prev_girder_length)));
      prev_bot_width = (prev_span_idx == ALL_SPANS ? 0 : pGirder->GetBottomWidth(pgsPointOfInterest(prev_span_idx,nGirdersPrevSpan-1,prev_girder_length)));
      next_top_width = (next_span_idx == ALL_SPANS ? 0 : pGirder->GetTopWidth(pgsPointOfInterest(next_span_idx,nGirdersNextSpan-1,0)));
      next_bot_width = (next_span_idx == ALL_SPANS ? 0 : pGirder->GetBottomWidth(pgsPointOfInterest(next_span_idx,nGirdersNextSpan-1,0)));
      double right_overhang = 1.10*Max4(prev_top_width,prev_bot_width,next_top_width,next_bot_width)/2;
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


      CComPtr<IPoint2d> ahead_point;
      pAlignment->GetPoint(station,0.00,direction,&ahead_point);

      CComPtr<IPoint2d> back_point;
      pAlignment->GetPoint(station,0.00,direction,&back_point);

      if ( settings & IDB_PV_LABEL_PIERS )
      {
         // pier label
         CComPtr<iTextBlock> doPierName;
         doPierName.CoCreateInstance(CLSID_TextBlock);

         CString strText;
         if ( pierIdx == 0 || pierIdx == nPiers-1 )
            strText.Format(_T("Abutment %d"),LABEL_PIER(pierIdx));
         else
            strText.Format(_T("Pier %d"),LABEL_PIER(pierIdx));

         doPierName->SetPosition(ahead_point);
         doPierName->SetTextAlign(TA_BASELINE | TA_CENTER);
         doPierName->SetText(strText);
         doPierName->SetTextColor(BLACK);
         doPierName->SetBkMode(OPAQUE);

         double dir;
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
         IUnknown* unkPierStationEvents = pPierStationEvents->GetInterface(&IID_iDisplayObjectEvents);
         CComQIPtr<iDisplayObjectEvents,&IID_iDisplayObjectEvents> pierStationEvents(unkPierStationEvents);
         doStation->RegisterEventSink(pierStationEvents);
         unkPierStationEvents->Release();
         pierStationEvents.Release();

         label_display_list->AddDisplayObject(doStation);

         // connection
         double right_slab_edge_offset = pBridge->GetRightSlabEdgeOffset(pierIdx);
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
         doConnection->SetTextColor(DARKGREEN);
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

         // Register an event sink with the connection text display object so that we can handle double clicks
         // differently then a general double click
         CConnectionDisplayObjectEvents* pEvents = new CConnectionDisplayObjectEvents(pierIdx);

         IUnknown* unk = pEvents->GetInterface(&IID_iDisplayObjectEvents);
         CComQIPtr<iDisplayObjectEvents,&IID_iDisplayObjectEvents> events(unk);
         CComQIPtr<iDisplayObject,&IID_iDisplayObject> dispObj(doConnection);
         dispObj->RegisterEventSink(events);

         label_display_list->AddDisplayObject(doConnection);

         if ( 0 < pierIdx )
         {
            ATLASSERT(pierIdx != ALL_PIERS);
            // label span length

            double span_length = station - last_station;

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
            IUnknown* unkSpanLength = pSpanLengthEvents->GetInterface(&IID_iDisplayObjectEvents);
            CComQIPtr<iDisplayObjectEvents,&IID_iDisplayObjectEvents> spanLengthEvents(unkSpanLength);
            doSpanLength->RegisterEventSink(spanLengthEvents);
            unkSpanLength->Release();
            spanLengthEvents.Release();

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
   double station = pBridge->GetPierStation(0);
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

   double x1,y1, x2, y2;
   rotation_center->get_X(&x1);
   rotation_center->get_Y(&y1);
   end_point->get_X(&x2);
   end_point->get_Y(&y2);

   double dx = x2 - x1;
   double dy = y2 - y1;

   double angle = atan2(dy,dx);

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

void CBridgePlanView::BuildSpanDisplayObjects()
{
   CPGSuperDoc* pDoc = (CPGSuperDoc*)GetDocument();
   CComPtr<IBroker> pBroker;
   pDoc->GetBroker(&pBroker);

   GET_IFACE2(pBroker,IBridge,pBridge);
   if ( pBridge->GetDeckType() == pgsTypes::sdtNone )
      return;

   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   CComPtr<iDisplayList> display_list;
   dispMgr->FindDisplayList(SPAN_DISPLAY_LIST,&display_list);

   display_list->Clear();

   SpanIndexType nSpans = pBridge->GetSpanCount();
   for ( SpanIndexType spanIdx = 0; spanIdx < nSpans; spanIdx++ )
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
      IUnknown* unk = pEvents->GetInterface(&IID_iDisplayObjectEvents);
      CComQIPtr<iDisplayObjectEvents,&IID_iDisplayObjectEvents> events(unk);
      unk->Release(); // releases ref count from new above

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
   CPGSuperDoc* pDoc = (CPGSuperDoc*)GetDocument();
   CComPtr<IBroker> pBroker;
   pDoc->GetBroker(&pBroker);

   GET_IFACE2(pBroker,IBridge,pBridge);
   if ( pBridge->GetDeckType() == pgsTypes::sdtNone )
      return;

   GET_IFACE2(pBroker,IBridgeMaterial,pBridgeMaterial);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CDeckDescription* pDeck = pBridgeDesc->GetDeckDescription();

   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   CComPtr<iDisplayList> display_list;
   dispMgr->FindDisplayList(SLAB_DISPLAY_LIST,&display_list);

   display_list->Clear();

   CComPtr<IPoint2dCollection> points;
   pBridge->GetSlabPerimeter(30,&points);

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

   CBridgePlanViewSlabDisplayObjectEvents* pEvents = new CBridgePlanViewSlabDisplayObjectEvents(pDoc, pBroker,m_pFrame,strategy->DoFill());
   IUnknown* unk = pEvents->GetInterface(&IID_iDisplayObjectEvents);
   CComQIPtr<iDisplayObjectEvents,&IID_iDisplayObjectEvents> events(unk);

   CComQIPtr<iDisplayObject,&IID_iDisplayObject> dispObj(doPnt);
   dispObj->RegisterEventSink(events);

   CString strMsg1(_T("Double click to edit slab.\r\nRight click for more options."));

   CString strMsg2;

   if ( pDeck->DeckType != pgsTypes::sdtNone )
   {
      strMsg2.Format(_T("\r\n\r\nDeck: %s\r\nSlab Thickness: %s\r\nSlab Offset: ????\r\nf'c: %s"),
                     m_pFrame->GetDeckTypeName(pDeck->DeckType),
                     FormatDimension(pDeck->GrossDepth,pDisplayUnits->GetComponentDimUnit()),
                     //FormatDimension(pDeck->SlabOffset,pDisplayUnits->GetComponentDimUnit()), // replace ???? with %s in the format string
                     FormatDimension(pBridgeMaterial->GetFcSlab(),pDisplayUnits->GetStressUnit())
                     );
   }

   CString strMsg3;
   double overlay_weight = pBridge->GetOverlayWeight();
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
//   EAFGetBroker(&pBroker); // this call doesn't work during initial startup
   CPGSuperDoc* pDoc = (CPGSuperDoc*)(GetDocument());
   CComPtr<IBroker> pBroker;
   pDoc->GetBroker(&pBroker);

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

   double first_station = pBridge->GetPierStation(0);
   double last_station  = pBridge->GetPierStation(pBridge->GetPierCount()-1);
   double cut_station = m_pFrame->GetCurrentCutLocation();

   if ( !InRange(first_station,cut_station,last_station) )
   {
      m_pFrame->InvalidateCutLocation();
      UpdateSectionCut();
   }
}

void CBridgePlanView::BuildNorthArrowDisplayObjects()
{
   CPGSuperDoc* pDoc = (CPGSuperDoc*)GetDocument();
   CComPtr<IBroker> pBroker;
   pDoc->GetBroker(&pBroker);

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
   double cx,cy,angle;
   mapping->GetRotation(&cx,&cy,&angle);
   double direction = PI_OVER_2 + angle;
   doNorth->SetDirection(direction);

   display_list->AddDisplayObject(doNorth);
}

void CBridgePlanView::BuildDiaphragmDisplayObjects()
{
   CPGSuperDoc* pDoc = (CPGSuperDoc*)GetDocument();
   CComPtr<IBroker> pBroker;
   pDoc->GetBroker(&pBroker);

   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   CComPtr<iDisplayList> display_list;
   dispMgr->FindDisplayList(DIAPHRAGM_DISPLAY_LIST,&display_list);
   display_list->Clear();

   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IRoadway,pAlignment);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   SpanIndexType nSpans = pBridge->GetSpanCount();
   for ( SpanIndexType spanIdx = 0; spanIdx < nSpans; spanIdx++ )
   {
      GirderIndexType nGirders = pBridge->GetGirderCount(spanIdx);
      for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders-1; gdrIdx++ )
      {
         std::vector<IntermedateDiaphragm> left_diaphragms  = pBridge->GetIntermediateDiaphragms(pgsTypes::BridgeSite1,spanIdx,gdrIdx);
         std::vector<IntermedateDiaphragm> right_diaphragms = pBridge->GetIntermediateDiaphragms(pgsTypes::BridgeSite1,spanIdx,gdrIdx+1);

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
               double station, offset;
               pBridge->GetStationAndOffset(pgsPointOfInterest(spanIdx,gdrIdx,left_diaphragm.Location),&station,&offset);

               CComPtr<IDirection> normal;
               pAlignment->GetBearingNormal(station,&normal);
               CComPtr<IPoint2d> pntLeft;
               pAlignment->GetPoint(station,offset,normal,&pntLeft);

               pBridge->GetStationAndOffset(pgsPointOfInterest(spanIdx,gdrIdx+1,right_diaphragm.Location),&station,&offset);
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

               double width = (left_diaphragm.T + right_diaphragm.T)/2;
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

std::_tstring CBridgePlanView::GetConnectionString(const CPierData* pPierData)
{
   pgsTypes::PierConnectionType connectionType = pPierData->GetConnectionType();

   std::_tstring strConnection;
   switch( connectionType )
   {
   case pgsTypes::Hinged:
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
