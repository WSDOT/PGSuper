///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2020  Washington State Department of Transportation
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

#include "stdafx.h"
#include "resource.h"
#include "PGSuperApp.h"
#include "AlignmentProfileView.h"
#include "ProfileDisplayObjectEvents.h"
#include "PGSuperDocBase.h"
#include <IFace\DrawBridgeSettings.h>
#include "PGSuperColors.h"
#include "BridgeDisplayObjectEvents.h"

#include <IFace\Alignment.h>
#include <IFace\Bridge.h>
#include <EAF\EAFDisplayUnits.h>
#include <DManipTools\DManipTools.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define TITLE_DISPLAY_LIST       0
#define PROFILE_DISPLAY_LIST     1
#define BRIDGE_DISPLAY_LIST      2
#define LABEL_DISPLAY_LIST       3

#define PROFILE_ID   -300
#define BRIDGE_ID    -400

/////////////////////////////////////////////////////////////////////////////
// CAlignmentProfileView

IMPLEMENT_DYNCREATE(CAlignmentProfileView, CBridgeViewPane)

CAlignmentProfileView::CAlignmentProfileView()
{
}

CAlignmentProfileView::~CAlignmentProfileView()
{
}


BEGIN_MESSAGE_MAP(CAlignmentProfileView, CBridgeViewPane)
	//{{AFX_MSG_MAP(CAlignmentProfileView)
	ON_COMMAND(ID_VIEWSETTINGS, OnViewSettings)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAlignmentProfileView drawing

void CAlignmentProfileView::OnInitialUpdate() 
{
   EnableToolTips();
   CBridgeViewPane::OnInitialUpdate();
}

void CAlignmentProfileView::BuildDisplayLists()
{
   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   dispMgr->EnableLBtnSelect(TRUE);
   dispMgr->EnableRBtnSelect(TRUE);
   dispMgr->SetSelectionLineColor(SELECTED_OBJECT_LINE_COLOR);
   dispMgr->SetSelectionFillColor(SELECTED_OBJECT_FILL_COLOR);

   CPGSDocBase* pDoc = (CPGSDocBase*)GetDocument();
   UINT settings = pDoc->GetAlignmentEditorSettings();
   DManip::MapMode mode = (settings & IDP_AP_DRAW_ISOTROPIC) ? DManip::Isotropic : DManip::Anisotropic;
   CBridgeViewPane::SetMappingMode(mode);

   // Setup display lists

   CComPtr<iDisplayList> label_list;
   ::CoCreateInstance(CLSID_DisplayList,nullptr,CLSCTX_ALL,IID_iDisplayList,(void**)&label_list);
   label_list->SetID(LABEL_DISPLAY_LIST);
   dispMgr->AddDisplayList(label_list);

   CComPtr<iDisplayList> title_list;
   ::CoCreateInstance(CLSID_DisplayList,nullptr,CLSCTX_ALL,IID_iDisplayList,(void**)&title_list);
   title_list->SetID(TITLE_DISPLAY_LIST);
   dispMgr->AddDisplayList(title_list);

   CComPtr<iDisplayList> bridge_list;
   ::CoCreateInstance(CLSID_DisplayList,nullptr,CLSCTX_ALL,IID_iDisplayList,(void**)&bridge_list);
   bridge_list->SetID(BRIDGE_DISPLAY_LIST);
   dispMgr->AddDisplayList(bridge_list);

   CComPtr<iDisplayList> profile_list;
   ::CoCreateInstance(CLSID_DisplayList,nullptr,CLSCTX_ALL,IID_iDisplayList,(void**)&profile_list);
   profile_list->SetID(PROFILE_DISPLAY_LIST);
   dispMgr->AddDisplayList(profile_list);
}

/////////////////////////////////////////////////////////////////////////////
// CAlignmentProfileView diagnostics

#ifdef _DEBUG
void CAlignmentProfileView::AssertValid() const
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
	CBridgeViewPane::AssertValid();
}

void CAlignmentProfileView::Dump(CDumpContext& dc) const
{
	CBridgeViewPane::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CBridgePlanView message handlers

void CAlignmentProfileView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
   CBridgeViewPane::OnUpdate(pSender,lHint,pHint);
   UpdateDisplayObjects();
   UpdateDrawingScale();
   Invalidate();
   UpdateWindow();
}

void CAlignmentProfileView::HandleLButtonDblClk(UINT nFlags, CPoint logPoint) 
{
   GetFrame()->SendMessage(WM_COMMAND,ID_PROJECT_PROFILE,0);
}

void CAlignmentProfileView::HandleContextMenu(CWnd* pWnd,CPoint logPoint)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CPGSDocBase* pDoc = (CPGSDocBase*)GetDocument();
   CEAFMenu* pMenu = CEAFMenu::CreateContextMenu(pDoc->GetPluginCommandManager());
   pMenu->LoadMenu(IDR_ALIGNMENT_PROFILE_CTX,nullptr);

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

   const std::map<IDType,IAlignmentProfileViewEventCallback*>& callbacks = pDoc->GetAlignmentProfileViewCallbacks();
   std::map<IDType,IAlignmentProfileViewEventCallback*>::const_iterator callbackIter(callbacks.begin());
   std::map<IDType,IAlignmentProfileViewEventCallback*>::const_iterator callbackIterEnd(callbacks.end());
   for ( ; callbackIter != callbackIterEnd; callbackIter++ )
   {
      IAlignmentProfileViewEventCallback* pCallback = callbackIter->second;
      pCallback->OnBackgroundContextMenu(pMenu);
   }


   pMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, logPoint.x, logPoint.y, this);
   delete pMenu;
}

void CAlignmentProfileView::OnViewSettings() 
{
   ((CPGSDocBase*)GetDocument())->EditBridgeViewSettings(VS_BRIDGE_PROFILE);
}

void CAlignmentProfileView::UpdateDisplayObjects()
{
   CWaitCursor wait;

   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   CDManipClientDC dc(this);

   dispMgr->ClearDisplayObjects();

   BuildTitleDisplayObjects();
   BuildProfileDisplayObjects();
   BuildBridgeDisplayObjects();
   BuildLabelDisplayObjects();

   CPGSDocBase* pDoc = (CPGSDocBase*)GetDocument();
   UINT settings = pDoc->GetAlignmentEditorSettings();
   DManip::MapMode mode = (settings & IDP_AP_DRAW_ISOTROPIC) ? DManip::Isotropic : DManip::Anisotropic;
   CBridgeViewPane::SetMappingMode(mode);
}

void CAlignmentProfileView::BuildTitleDisplayObjects()
{
   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   CComPtr<iDisplayList> title_list;
   dispMgr->FindDisplayList(TITLE_DISPLAY_LIST,&title_list);

   title_list->Clear();

   CComPtr<iViewTitle> title;
   title.CoCreateInstance(CLSID_ViewTitle);

   title->SetText(_T("Profile"));
   title_list->AddDisplayObject(title);
}

void CAlignmentProfileView::BuildProfileDisplayObjects()
{
   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   CComPtr<iDisplayList> display_list;
   dispMgr->FindDisplayList(PROFILE_DISPLAY_LIST,&display_list);

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

   // The profile is represented on the screen by a poly line object
   CComPtr<iPolyLineDisplayObject> doProfile;
   doProfile.CoCreateInstance(CLSID_PolyLineDisplayObject);
   doProfile->put_Width(PROFILE_LINE_WEIGHT);
   doProfile->put_Color(PROFILE_COLOR);
   doProfile->put_PointType(plpNone);
   doProfile->SetSelectionType(stAll);
   doProfile->SetID(PROFILE_ID);

   CComPtr<iPolyLineDisplayObject> doLeftCurb;
   doLeftCurb.CoCreateInstance(CLSID_PolyLineDisplayObject);
   doLeftCurb->put_Width(1);
   doLeftCurb->put_Color(RED);
   doLeftCurb->put_PointType(plpNone);

   CComPtr<iPolyLineDisplayObject> doRightCurb;
   doRightCurb.CoCreateInstance(CLSID_PolyLineDisplayObject);
   doRightCurb->put_Width(1);
   doRightCurb->put_Color(GREEN);
   doRightCurb->put_PointType(plpNone);

   // Register an event sink with the alignment object so that we can handle Float64 clicks
   // on the alignment differently then a general dbl-click
   CComPtr<iDisplayObject> dispObj;
   doProfile->QueryInterface(IID_iDisplayObject,(void**)&dispObj);
   CProfileDisplayObjectEvents* pEvents = new CProfileDisplayObjectEvents(pBroker,m_pFrame,dispObj);
   CComPtr<iDisplayObjectEvents> events;
   events.Attach((iDisplayObjectEvents*)pEvents->GetInterface(&IID_iDisplayObjectEvents));

   dispObj->RegisterEventSink(events);
   dispObj->SetToolTipText(_T("Double click to edit profile.\r\nRight click for more options."));
   dispObj->SetMaxTipWidth(TOOLTIP_WIDTH);
   dispObj->SetTipDisplayTime(TOOLTIP_DURATION);

   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 leftOffset  = pBridge->GetLeftCurbOffset((PierIndexType)0);
   Float64 rightOffset = pBridge->GetRightCurbOffset((PierIndexType)0);

   // model the profile as a series of individual points
   long nPoints = 50;
   Float64 station_inc = (end_station - start_station)/(nPoints-1);
   Float64 station = start_station;
   for ( long i = 0; i < nPoints; i++, station += station_inc)
   {
      Float64 y = pRoadway->GetElevation(station,0.0);
      CComPtr<IPoint2d> pnt;
      pnt.CoCreateInstance(CLSID_Point2d);
      pnt->Move(station,y);
      doProfile->AddPoint(pnt);

      y = pRoadway->GetElevation(station,leftOffset);
      pnt.Release();
      pnt.CoCreateInstance(CLSID_Point2d);
      pnt->Move(station,y);
      doLeftCurb->AddPoint(pnt);

      y = pRoadway->GetElevation(station,rightOffset);
      pnt.Release();
      pnt.CoCreateInstance(CLSID_Point2d);
      pnt->Move(station,y);
      doRightCurb->AddPoint(pnt);
   }

   doProfile->Commit();
   doLeftCurb->Commit();
   doRightCurb->Commit();

   display_list->AddDisplayObject(dispObj);
   display_list->AddDisplayObject(doLeftCurb);
   display_list->AddDisplayObject(doRightCurb);
}

void CAlignmentProfileView::BuildBridgeDisplayObjects()
{
   CPGSDocBase* pDoc = (CPGSDocBase*)GetDocument();
   UINT settings = pDoc->GetAlignmentEditorSettings();
   if ( (settings & IDP_AP_DRAW_BRIDGE) == 0 )
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

   GET_IFACE2(pBroker,IRoadway,pAlignment);
   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IGirder,pIGirder);
   GET_IFACE2(pBroker, IPointOfInterest, pPoi);

   CComPtr<iCompositeDisplayObject> doBridge;
   doBridge.CoCreateInstance(CLSID_CompositeDisplayObject);
   doBridge->SetSelectionType(stAll);
   doBridge->SetID(BRIDGE_ID);
   display_list->AddDisplayObject(doBridge);

   CComPtr<iDisplayObject> dispObj;
   doBridge->QueryInterface(IID_iDisplayObject,(void**)&dispObj);
   CBridgeDisplayObjectEvents* pEvents = new CBridgeDisplayObjectEvents(pBroker,m_pFrame,dispObj,CBridgeDisplayObjectEvents::Profile);
   CComPtr<iDisplayObjectEvents> events;
   events.Attach((iDisplayObjectEvents*)pEvents->GetInterface(&IID_iDisplayObjectEvents));
   dispObj->RegisterEventSink(events);
   dispObj->SetToolTipText(_T("Double click to edit bridge.\r\nRight click for more options."));
   dispObj->SetMaxTipWidth(TOOLTIP_WIDTH);
   dispObj->SetTipDisplayTime(TOOLTIP_DURATION);

   GroupIndexType nGroups = pBridge->GetGirderGroupCount();
   GirderIndexType nGirderLines = pBridge->GetGirderlineCount();
   for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
   {
      GirderIndexType nGirders = pBridge->GetGirderCount(grpIdx);
      nGirders = Min(nGirders,nGirderLines);
      CGirderKey girderKey(grpIdx,nGirders-1);
      SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);
      for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
      {
         CSegmentKey segmentKey(girderKey,segIdx);

         CComPtr<IShape> segmentShape;
         pIGirder->GetSegmentProfile( segmentKey, false/*dont include closures*/, &segmentShape);

         PoiList vPoi;
         pPoi->GetPointsOfInterest(segmentKey, POI_START_FACE, &vPoi); // don't use 0L at release... the support points may not be at the ends of the segment
         ATLASSERT(vPoi.size() == 1);
         const pgsPointOfInterest& startPoi = vPoi.front();

         Float64 elev = pIGirder->GetTopGirderChordElevation(startPoi);

         CComPtr<IPoint2d> pntPier1, pntEnd1, pntBrg1, pntBrg2, pntEnd2, pntPier2;
         pIGirder->GetSegmentEndPoints(segmentKey,pgsTypes::pcGlobal,&pntPier1,&pntEnd1,&pntBrg1,&pntBrg2,&pntEnd2,&pntPier2);

         Float64 slope = pBridge->GetSegmentSlope(segmentKey);
         Float64 angle = atan(slope);

         Float64 station,offset;
         pAlignment->GetStationAndOffset(pgsTypes::pcGlobal, pntEnd1, &station, &offset);

#if defined _DEBUG
         Float64 _station, _offset;
         pBridge->GetStationAndOffset(startPoi, &_station, &_offset);
         ATLASSERT(IsEqual(station, _station) && IsEqual(offset, _offset));
#endif

         CComQIPtr<IXYPosition> position(segmentShape);
         CComPtr<IPoint2d> pntTopLeft;
         position->get_LocatorPoint(lpTopLeft,&pntTopLeft);
         position->RotateEx(pntTopLeft,angle);

         Float64 x,y;
         pntTopLeft->Location(&x,&y);

         position->Offset(station-x,elev-y);


         CComPtr<iPointDisplayObject> doSegment;
         doSegment.CoCreateInstance(CLSID_PointDisplayObject);

         CComPtr<iShapeDrawStrategy> strategy;
         strategy.CoCreateInstance(CLSID_ShapeDrawStrategy);
         strategy->SetShape(segmentShape);
         strategy->SetSolidLineColor(SEGMENT_BORDER_COLOR);
         strategy->SetSolidFillColor(SEGMENT_FILL_COLOR);
         strategy->DoFill(TRUE);
         doSegment->SetDrawingStrategy(strategy);

         doBridge->AddDisplayObject(doSegment);
      }
   }
}

void CAlignmentProfileView::BuildLabelDisplayObjects()
{
   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   CComPtr<iDisplayList> label_list;
   dispMgr->FindDisplayList(LABEL_DISPLAY_LIST,&label_list);

   label_list->Clear();

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   // Label Start/End of Bridge
   CPGSDocBase* pDoc = (CPGSDocBase*)GetDocument();
   UINT settings = pDoc->GetAlignmentEditorSettings();
   if ( (settings & IDP_AP_DRAW_BRIDGE) != 0 )
   {
      GET_IFACE2(pBroker,IBridge,pBridge);
      Float64 start_station = pBridge->GetPierStation(0);
      Float64 end_station = pBridge->GetPierStation(pBridge->GetPierCount()-1);
      CreateStationLabel(label_list,start_station,_T("Start"),TA_BASELINE | TA_LEFT);
      CreateStationLabel(label_list,end_station,  _T("End"),  TA_BASELINE | TA_LEFT);
   }

   // Label Vertical Curves
   GET_IFACE2(pBroker,IRoadway,pRoadway);
   IndexType nVC = pRoadway->GetVertCurveCount();
   for ( IndexType vcIdx = 0; vcIdx < nVC; vcIdx++ )
   {
      CComPtr<IVertCurve> vc;
      pRoadway->GetVertCurve(vcIdx,&vc);

      Float64 L;
      vc->get_Length(&L);
      if ( IsZero(L) )
      {
         CComPtr<IProfilePoint> pntPVI;
         vc->get_PVI(&pntPVI);
         CComPtr<IStation> objStation;
         pntPVI->get_Station(&objStation);
         Float64 station;
         objStation->get_Value(&station);

         Float64 elevation;
         pntPVI->get_Elevation(&elevation);

         CreateStationLabel(label_list,station,elevation,_T("PVI"));
      }
      else
      {
         CComPtr<IProfilePoint> pntBVC;
         vc->get_BVC(&pntBVC);
         CComPtr<IStation> objStation;
         pntBVC->get_Station(&objStation);
         Float64 station;
         objStation->get_Value(&station);

         Float64 elevation;
         pntBVC->get_Elevation(&elevation);

         CreateStationLabel(label_list,station,elevation,_T("BVC"));

         //CComPtr<IProfilePoint> pntPVI;
         //vc->get_PVI(&pntPVI);
         //objStation.Release();
         //pntPVI->get_Station(&objStation);
         //objStation->get_Value(&station);
         //pntPVI->get_Elevation(&elevation);

         //CreateStationLabel(label_list,station,elevation,_T("PVI"));

         CComPtr<IProfilePoint> pntEVC;
         vc->get_EVC(&pntEVC);
         objStation.Release();
         pntEVC->get_Station(&objStation);
         objStation->get_Value(&station);
         pntEVC->get_Elevation(&elevation);

         CreateStationLabel(label_list,station,elevation,_T("EVC"));
      }
   }
}

void CAlignmentProfileView::UpdateDrawingScale()
{
   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   CComPtr<iDisplayList> title_display_list;
   dispMgr->FindDisplayList(TITLE_DISPLAY_LIST,&title_display_list);

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

   ScaleToFit();

   title_display_list->HideDisplayObjects(false);
}

void CAlignmentProfileView::CreateStationLabel(iDisplayList* pDisplayList,Float64 station,LPCTSTR strBaseLabel,UINT textAlign)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IRoadway,pRoadway);

   Float64 y = pRoadway->GetElevation(station,0.0);
   CreateStationLabel(pDisplayList,station,y,strBaseLabel,textAlign);
}

void CAlignmentProfileView::CreateStationLabel(iDisplayList* pDisplayList,Float64 station,Float64 elevation,LPCTSTR strBaseLabel,UINT textAlign)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   CComPtr<IPoint2d> p;
   p.CoCreateInstance(CLSID_Point2d);
   p->Move(station,elevation);

   CComPtr<iTextBlock> doLabel;
   doLabel.CoCreateInstance(CLSID_TextBlock);
   doLabel->SetPosition(p);
   
   CString strLabel;
   if ( strBaseLabel == nullptr )
   {
      strLabel.Format(_T("%s\nEl. %s"),
      ::FormatStation(pDisplayUnits->GetStationFormat(),station),
      ::FormatDimension(elevation,pDisplayUnits->GetAlignmentLengthUnit()));
   }
   else
   {
      strLabel.Format(_T("%s %s\nEl. %s"),
       strBaseLabel,
      ::FormatStation(pDisplayUnits->GetStationFormat(),station),
      ::FormatDimension(elevation,pDisplayUnits->GetAlignmentLengthUnit()));
   }
   doLabel->SetText(strLabel);

   doLabel->SetAngle(900);
   doLabel->SetTextAlign(textAlign);
   doLabel->SetBkMode(TRANSPARENT);

   pDisplayList->AddDisplayObject(doLabel);
}
