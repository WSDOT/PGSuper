///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2025  Washington State Department of Transportation
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

#include <WBFLGeometry/GeomHelpers.h>

#include <algorithm>

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
   m_pDispMgr->EnableLBtnSelect(TRUE);
   m_pDispMgr->EnableRBtnSelect(TRUE);
   m_pDispMgr->SetSelectionLineColor(SELECTED_OBJECT_LINE_COLOR);
   m_pDispMgr->SetSelectionFillColor(SELECTED_OBJECT_FILL_COLOR);

   CPGSDocBase* pDoc = (CPGSDocBase*)GetDocument();
   UINT settings = pDoc->GetAlignmentEditorSettings();
   auto mode = (settings & IDP_AP_DRAW_ISOTROPIC) ? WBFL::DManip::MapMode::Isotropic : WBFL::DManip::MapMode::Anisotropic;
   CBridgeViewPane::SetMappingMode(mode);

   // Setup display lists
   m_pDispMgr->CreateDisplayList(LABEL_DISPLAY_LIST);
   m_pDispMgr->CreateDisplayList(TITLE_DISPLAY_LIST);
   m_pDispMgr->CreateDisplayList(BRIDGE_DISPLAY_LIST);
   m_pDispMgr->CreateDisplayList(PROFILE_DISPLAY_LIST);
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

   CDManipClientDC dc(this);

   m_pDispMgr->ClearDisplayObjects();

   BuildTitleDisplayObjects();
   BuildProfileDisplayObjects();
   BuildBridgeDisplayObjects();
   BuildLabelDisplayObjects();

   CPGSDocBase* pDoc = (CPGSDocBase*)GetDocument();
   UINT settings = pDoc->GetAlignmentEditorSettings();
   auto mode = (settings & IDP_AP_DRAW_ISOTROPIC) ? WBFL::DManip::MapMode::Isotropic : WBFL::DManip::MapMode::Anisotropic;
   CBridgeViewPane::SetMappingMode(mode);
}

void CAlignmentProfileView::BuildTitleDisplayObjects()
{
   auto title_list = m_pDispMgr->FindDisplayList(TITLE_DISPLAY_LIST);

   title_list->Clear();

   auto title = WBFL::DManip::ViewTitle::Create();

   title->SetText(_T("Profile"));
   title_list->AddDisplayObject(title);
}

void CAlignmentProfileView::BuildProfileDisplayObjects()
{
   auto display_list = m_pDispMgr->FindDisplayList(PROFILE_DISPLAY_LIST);
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
   auto doProfile = WBFL::DManip::PolyLineDisplayObject::Create();
   doProfile->SetWidth(PROFILE_LINE_WEIGHT);
   doProfile->SetColor(PROFILE_COLOR);
   doProfile->SetPointType(WBFL::DManip::PointType::None);
   doProfile->SetSelectionType(WBFL::DManip::SelectionType::All);
   doProfile->SetID(PROFILE_ID);

   auto doLeftCurb = WBFL::DManip::PolyLineDisplayObject::Create();
   doLeftCurb->SetWidth(1);
   doLeftCurb->SetColor(RED);
   doLeftCurb->SetPointType(WBFL::DManip::PointType::None);

   auto doRightCurb = WBFL::DManip::PolyLineDisplayObject::Create();
   doRightCurb->SetWidth(1);
   doRightCurb->SetColor(GREEN);
   doRightCurb->SetPointType(WBFL::DManip::PointType::None);

   // Register an event sink with the alignment object so that we can handle double clicks
   // on the alignment differently then a general dbl-click
   auto events = std::make_shared<CProfileDisplayObjectEvents>(pBroker, m_pFrame, doProfile);
   doProfile->RegisterEventSink(events);
   doProfile->SetToolTipText(_T("Double click to edit profile.\r\nRight click for more options."));
   doProfile->SetMaxTipWidth(TOOLTIP_WIDTH);
   doProfile->SetTipDisplayTime(TOOLTIP_DURATION);

   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 leftOffset  = pBridge->GetLeftCurbOffset((PierIndexType)0);
   Float64 rightOffset = pBridge->GetRightCurbOffset((PierIndexType)0);

   // model the profile as a series of individual points

   // get we want the stations at all roadway sections
   GET_IFACE2(pBroker, IRoadwayData, pRoadwayData);
   const auto roadwaySectionData = pRoadwayData->GetRoadwaySectionData();
   auto nSectionTemplates = roadwaySectionData.RoadwaySectionTemplates.size();
   
   // we also want to use a minimum of nPoints
   long nPoints = 50;

   // create a vector of stations
   std::vector<Float64> vStations;
   vStations.resize(nPoints);
   Float64 station_inc = (end_station - start_station)/(nPoints-1);
   Float64 station = start_station - station_inc;
   std::generate(std::begin(vStations), std::end(vStations), [&]() {return station += station_inc;}); // generate nPoint stations
   if (1 < roadwaySectionData.RoadwaySectionTemplates.size())
   {
      for (const auto& sectionTemplate : roadwaySectionData.RoadwaySectionTemplates) // add section template locations, if there is more than one
      {
         vStations.push_back(sectionTemplate.Station);
      }
   }
   std::sort(std::begin(vStations), std::end(vStations)); // sort and remove duplicates
   vStations.erase(std::unique(std::begin(vStations), std::end(vStations), [](auto& a, auto& b) {return IsEqual(a, b);}), std::end(vStations));

   for(Float64 station : vStations)
   {
      IndexType pglIdx = pRoadway->GetProfileGradeLineIndex(station);
      Float64 offset = pRoadway->GetAlignmentOffset(pglIdx, station);

      Float64 y = pRoadway->GetElevation(station,offset);
      CComPtr<IPoint2d> pnt;
      pnt.CoCreateInstance(CLSID_Point2d);
      pnt->Move(station,y);
      doProfile->AddPoint(geomUtil::GetPoint(pnt));

      if (IsEqual(station, vStations.front()))
      {
         Float64 grade = pRoadway->GetProfileGrade(station);
         auto doText = WBFL::DManip::TextBlock::Create();
         doText->SetPosition(geomUtil::GetPoint(pnt));
         doText->SetText(_T("PGL"));
         doText->SetTextAlign(TA_BOTTOM | TA_LEFT);
         doText->SetBkMode(TRANSPARENT);
         long angle = long(1800.*grade / M_PI);
         angle = (900 < angle && angle < 2700) ? angle - 1800 : angle;
         doText->SetAngle(angle);
         display_list->AddDisplayObject(doText);
      }

      y = pRoadway->GetElevation(station,leftOffset);
      pnt.Release();
      pnt.CoCreateInstance(CLSID_Point2d);
      pnt->Move(station,y);
      doLeftCurb->AddPoint(geomUtil::GetPoint(pnt));

      y = pRoadway->GetElevation(station,rightOffset);
      pnt.Release();
      pnt.CoCreateInstance(CLSID_Point2d);
      pnt->Move(station,y);
      doRightCurb->AddPoint(geomUtil::GetPoint(pnt));
   }

   display_list->AddDisplayObject(doProfile);
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

   auto display_list = m_pDispMgr->FindDisplayList(BRIDGE_DISPLAY_LIST);
   display_list->Clear();

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IRoadway,pAlignment);
   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IGirder,pIGirder);
   GET_IFACE2(pBroker, IPointOfInterest, pPoi);

   auto doBridge = WBFL::DManip::CompositeDisplayObject::Create();
   doBridge->SetSelectionType(WBFL::DManip::SelectionType::All);
   doBridge->SetID(BRIDGE_ID);
   display_list->AddDisplayObject(doBridge);

   auto events = std::make_shared<CBridgeDisplayObjectEvents>(pBroker,m_pFrame,doBridge,CBridgeDisplayObjectEvents::Profile);
   doBridge->RegisterEventSink(events);
   doBridge->SetToolTipText(_T("Double click to edit bridge.\r\nRight click for more options."));
   doBridge->SetMaxTipWidth(TOOLTIP_WIDTH);
   doBridge->SetTipDisplayTime(TOOLTIP_DURATION);

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


         auto doSegment = WBFL::DManip::PointDisplayObject::Create();

         auto strategy = WBFL::DManip::ShapeDrawStrategy::Create();
         strategy->SetShape(geomUtil::ConvertShape(segmentShape));
         strategy->SetSolidLineColor(SEGMENT_BORDER_COLOR);
         strategy->SetSolidFillColor(SEGMENT_FILL_COLOR);
         strategy->Fill(true);
         doSegment->SetDrawingStrategy(strategy);

         doBridge->AddDisplayObject(doSegment);
      }
   }
}

void CAlignmentProfileView::BuildLabelDisplayObjects()
{
   auto label_display_list = m_pDispMgr->FindDisplayList(LABEL_DISPLAY_LIST);
   label_display_list->Clear();

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
      CreateStationLabel(label_display_list,start_station,_T("Start"),TA_BASELINE | TA_LEFT);
      CreateStationLabel(label_display_list,end_station,  _T("End"),  TA_BASELINE | TA_LEFT);
   }

   // Start/End of station range labels
   GET_IFACE2(pBroker,IRoadway,pRoadway);
   Float64 n = 10;
   Float64 start_station, start_elevation, start_grade;
   Float64 end_station, end_elevation, end_grade;
   CComPtr<IPoint2d> pntStart, pntEnd;
   pRoadway->GetStartPoint(n, &start_station, &start_elevation, &start_grade, &pntStart);
   pRoadway->GetEndPoint(n, &end_station, &end_elevation, &end_grade, &pntEnd);
   CreateStationLabel(label_display_list, start_station);
   CreateStationLabel(label_display_list, end_station);

   // Even station labels
   Float64 start, end, station_step;
   GetUniformStationingData(pBroker, start_station, end_station, &start, &end, &station_step);
   Float64 station = start;
   do
   {
      CreateStationLabel(label_display_list, station);
      station += station_step;
   } while (station < end);

   // Label Vertical Curves
   IndexType nVC = pRoadway->GetVertCurveCount();
   for ( IndexType vcIdx = 0; vcIdx < nVC; vcIdx++ )
   {
      CComPtr<IVerticalCurve> vc;
      pRoadway->GetVertCurve(vcIdx,&vc);

      Float64 L;
      CComQIPtr<IProfileElement> element(vc);
      element->GetLength(&L);
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

         CreateStationLabel(label_display_list,station,elevation,_T("PVI"));
      }
      else
      {
         CComPtr<IProfilePoint> pntBVC;
         vc->get_BVC(&pntBVC);
         CComPtr<IStation> objStation;
         pntBVC->get_Station(&objStation);
         Float64 bvc_station;
         objStation->get_Value(&bvc_station);
         Float64 elevation;
         pntBVC->get_Elevation(&elevation);
         CreateStationLabel(label_display_list, bvc_station,elevation,_T("BVC"));

         //CComPtr<IProfilePoint> pntPVI;
         //vc->get_PVI(&pntPVI);
         //objStation.Release();
         //pntPVI->get_Station(&objStation);
         //objStation->get_Value(&station);
         //pntPVI->get_Elevation(&elevation);

         //CreateStationLabel(label_display_list,station,elevation,_T("PVI"));

         CComPtr<IProfilePoint> pntEVC;
         vc->get_EVC(&pntEVC);
         objStation.Release();
         pntEVC->get_Station(&objStation);
         Float64 evc_station;
         objStation->get_Value(&evc_station);
         pntEVC->get_Elevation(&elevation);

         CreateStationLabel(label_display_list, evc_station,elevation,_T("EVC"));

         CComPtr<IProfilePoint> low_point;
         vc->get_LowPoint(&low_point);
         objStation.Release();
         low_point->get_Station(&objStation);
         Float64 low_point_station;
         objStation->get_Value(&low_point_station);
         low_point->get_Elevation(&elevation);
         if (!IsEqual(bvc_station, low_point_station) && !IsEqual(low_point_station, evc_station))
         {
            CreateStationLabel(label_display_list, low_point_station, elevation, _T("LP"));
         }

         CComPtr<IProfilePoint> high_point;
         vc->get_HighPoint(&high_point);
         objStation.Release();
         high_point->get_Station(&objStation);
         Float64 high_point_station;
         objStation->get_Value(&high_point_station);
         high_point->get_Elevation(&elevation);
         if (!IsEqual(bvc_station, high_point_station) && !IsEqual(high_point_station, evc_station))
         {
            CreateStationLabel(label_display_list, high_point_station, elevation, _T("HP"));
         }
      }
   }
}

void CAlignmentProfileView::UpdateDrawingScale()
{
   auto title_display_list = m_pDispMgr->FindDisplayList(TITLE_DISPLAY_LIST);

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

void CAlignmentProfileView::CreateStationLabel(std::shared_ptr<WBFL::DManip::iDisplayList> pDisplayList,Float64 station,LPCTSTR strBaseLabel,UINT textAlign)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IRoadway,pRoadway);

   IndexType pglIdx = pRoadway->GetProfileGradeLineIndex(station);
   Float64 offset = pRoadway->GetAlignmentOffset(pglIdx, station);

   Float64 y = pRoadway->GetElevation(station,offset);
   CreateStationLabel(pDisplayList,station,y,strBaseLabel,textAlign);
}

void CAlignmentProfileView::CreateStationLabel(std::shared_ptr<WBFL::DManip::iDisplayList> pDisplayList,Float64 station,Float64 elevation,LPCTSTR strBaseLabel,UINT textAlign)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   WBFL::Geometry::Point2d p(station,elevation);

   auto doLabel = WBFL::DManip::TextBlock::Create();
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
   doLabel->SetPointSize(100);

   pDisplayList->AddDisplayObject(doLabel);
}
