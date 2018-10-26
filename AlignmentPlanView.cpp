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

// AlignmentPlanView.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\resource.h"
#include "PGSuperAppPlugin\PGSuperApp.h"
#include "AlignmentPlanView.h"
#include "AlignmentProfileView.h"
#include "PGSuperDocBase.h"
#include <IFace\DrawBridgeSettings.h>
#include "PGSuperColors.h"
#include "AlignmentDisplayObjectEvents.h"
#include "BridgeDisplayObjectEvents.h"

#include <IFace\Alignment.h>
#include <IFace\Bridge.h>
#include <EAF\EAFDisplayUnits.h>
#include <WBFLDManipTools.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define TITLE_DISPLAY_LIST       0
#define ALIGNMENT_DISPLAY_LIST   1
#define BRIDGE_DISPLAY_LIST      2
#define LABEL_DISPLAY_LIST       3
#define NORTH_ARROW_DISPLAY_LIST 4 

#define ALIGNMENT_ID   -300
#define BRIDGE_ID      -400

/////////////////////////////////////////////////////////////////////////////
// CAlignmentPlanView

IMPLEMENT_DYNCREATE(CAlignmentPlanView, CDisplayView)

CAlignmentPlanView::CAlignmentPlanView()
{
}

CAlignmentPlanView::~CAlignmentPlanView()
{
}


BEGIN_MESSAGE_MAP(CAlignmentPlanView, CDisplayView)
	//{{AFX_MSG_MAP(CAlignmentPlanView)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_COMMAND(ID_VIEWSETTINGS, OnViewSettings)
   ON_WM_KEYDOWN()
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
//   ON_COMMAND(ID_ZOOM,OnZoom)
//   ON_COMMAND(ID_SCALETOFIT,OnScaleToFit)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAlignmentPlanView drawing

void CAlignmentPlanView::OnInitialUpdate() 
{
   EnableToolTips();

   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   dispMgr->EnableLBtnSelect(TRUE);
   dispMgr->EnableRBtnSelect(TRUE);
   dispMgr->SetSelectionLineColor(SELECTED_OBJECT_LINE_COLOR);
   dispMgr->SetSelectionFillColor(SELECTED_OBJECT_FILL_COLOR);

   CDisplayView::SetMappingMode(DManip::Isotropic);

   // Setup display lists

   CComPtr<iDisplayList> label_list;
   ::CoCreateInstance(CLSID_DisplayList,NULL,CLSCTX_ALL,IID_iDisplayList,(void**)&label_list);
   label_list->SetID(LABEL_DISPLAY_LIST);
   dispMgr->AddDisplayList(label_list);

   CComPtr<iDisplayList> title_list;
   ::CoCreateInstance(CLSID_DisplayList,NULL,CLSCTX_ALL,IID_iDisplayList,(void**)&title_list);
   title_list->SetID(TITLE_DISPLAY_LIST);
   dispMgr->AddDisplayList(title_list);

   CComPtr<iDisplayList> bridge_list;
   ::CoCreateInstance(CLSID_DisplayList,NULL,CLSCTX_ALL,IID_iDisplayList,(void**)&bridge_list);
   bridge_list->SetID(BRIDGE_DISPLAY_LIST);
   dispMgr->AddDisplayList(bridge_list);

   CComPtr<iDisplayList> alignment_list;
   ::CoCreateInstance(CLSID_DisplayList,NULL,CLSCTX_ALL,IID_iDisplayList,(void**)&alignment_list);
   alignment_list->SetID(ALIGNMENT_DISPLAY_LIST);
   dispMgr->AddDisplayList(alignment_list);

   CComPtr<iDisplayList> north_arrow_list;
   ::CoCreateInstance(CLSID_DisplayList,NULL,CLSCTX_ALL,IID_iDisplayList,(void**)&north_arrow_list);
   north_arrow_list->SetID(NORTH_ARROW_DISPLAY_LIST);
   dispMgr->AddDisplayList(north_arrow_list);

   CDisplayView::OnInitialUpdate();
}

void CAlignmentPlanView::DoPrint(CDC* pDC, CPrintInfo* pInfo,CRect rcDraw)
{
   OnBeginPrinting(pDC, pInfo, rcDraw);
   OnPrepareDC(pDC);
   UpdateDrawingScale();
   OnDraw(pDC);
   OnEndPrinting(pDC, pInfo);
}

void CAlignmentPlanView::OnDraw(CDC* pDC)
{
   CDisplayView::OnDraw(pDC);

   if ( CWnd::GetFocus() == this && !pDC->IsPrinting() )
   {
      DrawFocusRect();
   }
}

/////////////////////////////////////////////////////////////////////////////
// CAlignmentPlanView diagnostics

#ifdef _DEBUG
void CAlignmentPlanView::AssertValid() const
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
	CDisplayView::AssertValid();
}

void CAlignmentPlanView::Dump(CDumpContext& dc) const
{
	CDisplayView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CBridgePlanView message handlers

void CAlignmentPlanView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
   CDisplayView::OnUpdate(pSender,lHint,pHint);

   UpdateDisplayObjects();
   UpdateDrawingScale();
}

void CAlignmentPlanView::OnSize(UINT nType, int cx, int cy) 
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

void CAlignmentPlanView::HandleLButtonDown(UINT nFlags, CPoint logPoint)
{
   CBridgeModelViewChildFrame* pFrame = GetFrame();
   pFrame->ClearSelection();
}

void CAlignmentPlanView::HandleLButtonDblClk(UINT nFlags, CPoint logPoint) 
{
   GetFrame()->SendMessage(WM_COMMAND,ID_PROJECT_ALIGNMENT,0);
}

void CAlignmentPlanView::HandleContextMenu(CWnd* pWnd,CPoint logPoint)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CPGSuperDocBase* pDoc = (CPGSuperDocBase*)GetDocument();
   CEAFMenu* pMenu = CEAFMenu::CreateContextMenu(pDoc->GetPluginCommandManager());
   pMenu->LoadMenu(IDR_ALIGNMENT_PLAN_CTX,NULL);

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

   const std::map<IDType,IAlignmentPlanViewEventCallback*>& callbacks = pDoc->GetAlignmentPlanViewCallbacks();
   std::map<IDType,IAlignmentPlanViewEventCallback*>::const_iterator callbackIter(callbacks.begin());
   std::map<IDType,IAlignmentPlanViewEventCallback*>::const_iterator callbackIterEnd(callbacks.end());
   for ( ; callbackIter != callbackIterEnd; callbackIter++ )
   {
      IAlignmentPlanViewEventCallback* pCallback = callbackIter->second;
      pCallback->OnBackgroundContextMenu(pMenu);
   }


   pMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, logPoint.x, logPoint.y, this);
   delete pMenu;
}

void CAlignmentPlanView::OnViewSettings() 
{
   ((CPGSuperDocBase*)GetDocument())->EditBridgeViewSettings(VS_BRIDGE_ALIGNMENT);
}

void CAlignmentPlanView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
   if ( nChar == VK_DOWN && ::GetKeyState(VK_CONTROL) < 0 )
   {
      // CTRL + down arrow... put focus in Alignment Profile View
      m_pFrame->GetAlignmentProfileView()->SetFocus();
   }

   CDisplayView::OnKeyDown(nChar,nRepCnt,nFlags);
}

void CAlignmentPlanView::UpdateDisplayObjects()
{
   CWaitCursor wait;

   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   CDManipClientDC dc(this);

   dispMgr->ClearDisplayObjects();

   BuildTitleDisplayObjects();
   BuildAlignmentDisplayObjects();
   BuildBridgeDisplayObjects();
   BuildLabelDisplayObjects();
   BuildNorthArrowDisplayObjects();
}

void CAlignmentPlanView::BuildTitleDisplayObjects()
{
   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   CComPtr<iDisplayList> title_list;
   dispMgr->FindDisplayList(TITLE_DISPLAY_LIST,&title_list);

   title_list->Clear();

   CComPtr<iViewTitle> title;
   title.CoCreateInstance(CLSID_ViewTitle);
   title->SetText(_T("Alignment"));
   title_list->AddDisplayObject(title);
}

void CAlignmentPlanView::BuildAlignmentDisplayObjects()
{
   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   CComPtr<iDisplayList> display_list;
   dispMgr->FindDisplayList(ALIGNMENT_DISPLAY_LIST,&display_list);

   display_list->Clear();

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IRoadway,pRoadway);

   Float64 n = 10;
   Float64 start_station, start_elevation, start_grade;
   Float64 end_station, end_elevation, end_grade;
   CComPtr<IPoint2d> pntStart, pntEnd;
   pRoadway->GetStartPoint(n,&start_station,&start_elevation,&start_grade,&pntStart);
   pRoadway->GetEndPoint(  n,&end_station,  &end_elevation,  &end_grade,  &pntEnd);

   // The alignment is represented on the screen by a poly line object
   CComPtr<iPolyLineDisplayObject> doAlignment;
   doAlignment.CoCreateInstance(CLSID_PolyLineDisplayObject);

   // Register an event sink with the alignment object so that we can handle dbl-clicks
   // on the alignment differently then a general dbl-click
   CComPtr<iDisplayObject> dispObj;
   doAlignment->QueryInterface(IID_iDisplayObject,(void**)&dispObj);
   CAlignmentDisplayObjectEvents* pEvents = new CAlignmentDisplayObjectEvents(pBroker,m_pFrame,CAlignmentDisplayObjectEvents::Alignment,dispObj);
   CComPtr<iDisplayObjectEvents> events;
   events.Attach((iDisplayObjectEvents*)pEvents->GetInterface(&IID_iDisplayObjectEvents));
   dispObj->RegisterEventSink(events);

   dispObj->SetToolTipText(_T("Double click to edit alignment.\r\nRight click for more options."));
   dispObj->SetMaxTipWidth(TOOLTIP_WIDTH);
   dispObj->SetTipDisplayTime(TOOLTIP_DURATION);

   // model the alignment as a series of individual points
   CComPtr<IDirection> bearing;
   bearing.CoCreateInstance(CLSID_Direction);
   long nPoints = 50;
   Float64 station_inc = (end_station - start_station)/(nPoints-1);
   Float64 station = start_station;
   for ( long i = 0; i < nPoints; i++, station += station_inc)
   {
      CComPtr<IPoint2d> p;
      pRoadway->GetPoint(station,0.00,bearing,&p);
      doAlignment->AddPoint(p);
   }

   doAlignment->put_Width(ALIGNMENT_LINE_WEIGHT);
   doAlignment->put_Color(ALIGNMENT_COLOR);
   doAlignment->put_PointType(plpNone);
   doAlignment->Commit();

   display_list->AddDisplayObject(dispObj);

   dispObj->SetSelectionType(stAll);
   dispObj->SetID(ALIGNMENT_ID);

   ////////////////


   CComPtr<iCoordinateMap> map;
   dispMgr->GetCoordinateMap(&map);
   CComQIPtr<iMapping> mapping(map);

   // get point on alignment at start
   CComPtr<IDirection> dir;
   pRoadway->GetBearingNormal(start_station,&dir);
   CComPtr<IPoint2d> rotation_center;
   pRoadway->GetPoint(start_station,0.00,dir,&rotation_center);

   // get point on alignment at last pier
   CComPtr<IPoint2d> end_point;
   dir.Release();
   pRoadway->GetBearingNormal(end_station,&dir);
   pRoadway->GetPoint(end_station,0.00,dir,&end_point);

   // get the direction of the line from the start to the end
   // this represents the amount we want to rotate the display

   Float64 x1,y1, x2, y2;
   rotation_center->get_X(&x1);
   rotation_center->get_Y(&y1);
   end_point->get_X(&x2);
   end_point->get_Y(&y2);

   Float64 dx = x2 - x1;
   Float64 dy = y2 - y1;

   Float64 angle = atan2(dy,dx);

   CPGSuperDocBase* pDoc = (CPGSuperDocBase*)GetDocument();
   UINT settings = pDoc->GetAlignmentEditorSettings();
   if ( settings & IDA_AP_NORTH_UP )
   {
      mapping->SetRotation((x1+x2)/2,(y1+y2)/2,0);
   }
   else
   {
      // rotation by negative of the angle
      mapping->SetRotation((x1+x2)/2,(y1+y2)/2,-angle);
   }
}

void CAlignmentPlanView::BuildBridgeDisplayObjects()
{
   CPGSuperDocBase* pDoc = (CPGSuperDocBase*)GetDocument();
   UINT settings = pDoc->GetAlignmentEditorSettings();
   if ( (settings & IDA_AP_DRAW_BRIDGE) == 0 )
   {
      return;
   }

   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   CComPtr<iDisplayList> display_list;
   dispMgr->FindDisplayList(BRIDGE_DISPLAY_LIST,&display_list);

   display_list->Clear();

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IRoadway,pRoadway);

   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 start_station = pBridge->GetPierStation(0);
   Float64 end_station = pBridge->GetPierStation(pBridge->GetPierCount()-1);

   // The bridge is represented on the screen by a poly line object
   CComPtr<iPolyLineDisplayObject> doBridge;
   doBridge.CoCreateInstance(CLSID_PolyLineDisplayObject);

   // Register an event sink with the alignment object so that we can handle Float64 clicks
   // on the alignment differently then a general dbl-click
   CComPtr<iDisplayObject> dispObj;
   doBridge->QueryInterface(IID_iDisplayObject,(void**)&dispObj);
   CBridgeDisplayObjectEvents* pEvents = new CBridgeDisplayObjectEvents(pBroker,m_pFrame,dispObj,CBridgeDisplayObjectEvents::Plan);
   CComPtr<iDisplayObjectEvents> events;
   events.Attach((iDisplayObjectEvents*)pEvents->GetInterface(&IID_iDisplayObjectEvents));
   dispObj->RegisterEventSink(events);
   dispObj->SetToolTipText(_T("Double click to edit bridge.\r\nRight click for more options."));
   dispObj->SetMaxTipWidth(TOOLTIP_WIDTH);
   dispObj->SetTipDisplayTime(TOOLTIP_DURATION);

   Float64 alignment_offset = pBridge->GetAlignmentOffset();

   // model the alignment as a series of individual points
   CComPtr<IDirection> bearing;
   bearing.CoCreateInstance(CLSID_Direction);
   long nPoints = 20;
   Float64 station_inc = (end_station - start_station)/(nPoints-1);
   Float64 station = start_station;
   for ( long i = 0; i < nPoints; i++, station += station_inc)
   {
      CComPtr<IPoint2d> p;
      pRoadway->GetPoint(station,alignment_offset,bearing,&p);
      doBridge->AddPoint(p);
   }

   doBridge->put_Width(BRIDGE_LINE_WEIGHT);
   doBridge->put_Color(BRIDGE_COLOR);
   doBridge->put_PointType(plpNone);
   doBridge->Commit();

   display_list->AddDisplayObject(dispObj);

   dispObj->SetSelectionType(stAll);
   dispObj->SetID(BRIDGE_ID);
}

void CAlignmentPlanView::BuildLabelDisplayObjects()
{
   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   CComPtr<iDisplayList> label_display_list;
   dispMgr->FindDisplayList(LABEL_DISPLAY_LIST,&label_display_list);
   label_display_list->Clear();

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   // Label Start and End of Bridge
   CPGSuperDocBase* pDoc = (CPGSuperDocBase*)GetDocument();
   UINT settings = pDoc->GetAlignmentEditorSettings();
   if ( (settings & IDA_AP_DRAW_BRIDGE) != 0 )
   {
      GET_IFACE2(pBroker,IBridge,pBridge);
      Float64 start_station = pBridge->GetPierStation(0);
      Float64 end_station = pBridge->GetPierStation(pBridge->GetPierCount()-1);
      CreateStationLabel(label_display_list,start_station,_T("Start"),LABEL_NORMAL_TO_ALIGNMENT, TA_BASELINE | TA_RIGHT);
      CreateStationLabel(label_display_list,end_station,_T("End"),    LABEL_NORMAL_TO_ALIGNMENT, TA_BASELINE | TA_RIGHT);
   }

   // Label Horizontal Curve Points
   GET_IFACE2(pBroker,IRoadway,pRoadway);
   IndexType nHC = pRoadway->GetCurveCount();
   for ( IndexType hcIdx = 0; hcIdx < nHC; hcIdx++ )
   {
      CComPtr<IHorzCurve> hc;
      pRoadway->GetCurve(hcIdx,&hc);

      Float64 L;
      hc->get_TotalLength(&L);
      if ( IsZero(L) )
      {
         // This is just a PI
         CComPtr<IPoint2d> pntPI;
         hc->get_PI(&pntPI);
         Float64 station, offset;
         pRoadway->GetStationAndOffset(pntPI,&station,&offset);
         CreateStationLabel(label_display_list,station,_T("PI"));
      }
      else
      {
         Float64 Ls1, Ls2;
         hc->get_SpiralLength(spEntry,&Ls1);
         hc->get_SpiralLength(spExit,&Ls2);

         CComPtr<IPoint2d> pntTS, pntSC, pntCS, pntST;
         hc->get_TS(&pntTS);
         hc->get_SC(&pntSC);
         hc->get_CS(&pntCS);
         hc->get_ST(&pntST);

         Float64 station, offset;
         if ( IsZero(Ls1) )
         {
            pRoadway->GetStationAndOffset(pntTS,&station,&offset);
            CreateStationLabel(label_display_list,station,_T("PC"));
         }
         else
         {
            pRoadway->GetStationAndOffset(pntTS,&station,&offset);
            CreateStationLabel(label_display_list,station,_T("TS"));

            pRoadway->GetStationAndOffset(pntSC,&station,&offset);
            CreateStationLabel(label_display_list,station,_T("SC"));
         }

         if ( IsZero(Ls2) )
         {
            pRoadway->GetStationAndOffset(pntST,&station,&offset);
            CreateStationLabel(label_display_list,station,_T("PT"));
         }
         else
         {
            pRoadway->GetStationAndOffset(pntCS,&station,&offset);
            CreateStationLabel(label_display_list,station,_T("CS"));

            pRoadway->GetStationAndOffset(pntST,&station,&offset);
            CreateStationLabel(label_display_list,station,_T("ST"));
         }
      }
   }
}

void CAlignmentPlanView::CreateStationLabel(iDisplayList* pDisplayList,Float64 station,LPCTSTR strBaseLabel,long angle,UINT textAlign)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   GET_IFACE2(pBroker,IRoadway,pRoadway);


   CComPtr<IPoint2d> p;
   pRoadway->GetPoint(station,0.00,NULL,&p);

   CComPtr<iTextBlock> doLabel;
   doLabel.CoCreateInstance(CLSID_TextBlock);
   doLabel->SetPosition(p);
   
   CString strLabel;
   if ( strBaseLabel == NULL )
   {
      strLabel.Format(_T("%s"),::FormatStation(pDisplayUnits->GetStationFormat(),station));
   }
   else
   {
      strLabel.Format(_T("%s %s"),strBaseLabel,::FormatStation(pDisplayUnits->GetStationFormat(),station));
   }
   doLabel->SetText(strLabel);

   if ( angle == LABEL_NORMAL_TO_ALIGNMENT )
   {
      CComPtr<IDirection> dirNormal;
      pRoadway->GetBearingNormal(station,&dirNormal);
      Float64 dir;
      dirNormal->get_Value(&dir);
      angle = long(1800.*dir/M_PI);
      angle = (900 < angle && angle < 2700 ) ? angle-1800 : angle;
   }
   doLabel->SetAngle(angle);

   doLabel->SetTextAlign(textAlign);
   doLabel->SetBkMode(TRANSPARENT);

   pDisplayList->AddDisplayObject(doLabel);
}

void CAlignmentPlanView::BuildNorthArrowDisplayObjects()
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

void CAlignmentPlanView::UpdateDrawingScale()
{
   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   CComPtr<iDisplayList> title_display_list;
   dispMgr->FindDisplayList(TITLE_DISPLAY_LIST,&title_display_list);

   CComPtr<iDisplayList> na_display_list;
   dispMgr->FindDisplayList(NORTH_ARROW_DISPLAY_LIST,&na_display_list);

   if ( title_display_list == NULL || na_display_list == NULL )
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
//
//
//void CAlignmentPlanView::ClearSelection()
//{
//   CComPtr<iDisplayMgr> dispMgr;
//   GetDisplayMgr(&dispMgr);
//
//   dispMgr->ClearSelectedObjects();
//}

void CAlignmentPlanView::OnSetFocus(CWnd* pOldWnd) 
{
	CDisplayView::OnSetFocus(pOldWnd);
   DrawFocusRect();
}

void CAlignmentPlanView::OnKillFocus(CWnd* pNewWnd) 
{
	CDisplayView::OnKillFocus(pNewWnd);
   DrawFocusRect();
}
//
//void CAlignmentPlanView::OnZoom()
//{
//   CComPtr<iDisplayMgr> dispMgr;
//   GetDisplayMgr(&dispMgr);
//
//   CComPtr<iTaskFactory> taskFactory;
//   dispMgr->GetTaskFactory(&taskFactory);
//
//   CComPtr<iTask> task;
//   taskFactory->CreateZoomTask(this,NULL,LIGHTSTEELBLUE,&task);
//
//   dispMgr->SetTask(task);
//}
//
//void CAlignmentPlanView::OnScaleToFit()
//{
//   UpdateDrawingScale();
//}

int CAlignmentPlanView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CDisplayView::OnCreate(lpCreateStruct) == -1)
   {
		return -1;
   }
	
   m_pFrame = (CBridgeModelViewChildFrame*)GetParent()->GetParent();
   ASSERT( m_pFrame != 0 );
   ASSERT( m_pFrame->IsKindOf( RUNTIME_CLASS( CBridgeModelViewChildFrame ) ) );

	return 0;
}

CBridgeModelViewChildFrame* CAlignmentPlanView::GetFrame()
{
   return m_pFrame;
}

void CAlignmentPlanView::DrawFocusRect()
{
   CClientDC dc(this);
   CRect rClient;
   GetClientRect(&rClient);
   dc.DrawFocusRect(rClient);
}
