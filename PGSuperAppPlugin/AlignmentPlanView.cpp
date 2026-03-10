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
#include "AlignmentPlanView.h"
#include "AlignmentProfileView.h"
#include "PGSuperDocBase.h"
#include "PGSuperColors.h"
#include "AlignmentDisplayObjectEvents.h"
#include "BridgeDisplayObjectEvents.h"

#include <IFace/Tools.h>
#include <IFace\Alignment.h>
#include <IFace\Bridge.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\DrawBridgeSettings.h>

#include <DManip/PolyLineDisplayObjectImpl.h>
#include <WBFLGeometry/GeomHelpers.h>


#define TITLE_DISPLAY_LIST       0
#define ALIGNMENT_DISPLAY_LIST   1
#define BRIDGE_DISPLAY_LIST      2
#define LABEL_DISPLAY_LIST       3
#define NORTH_ARROW_DISPLAY_LIST 4 

#define ALIGNMENT_ID   -300
#define BRIDGE_ID      -400
#define BRIDGELINE_ID  -500

/////////////////////////////////////////////////////////////////////////////
// CAlignmentPlanView

IMPLEMENT_DYNCREATE(CAlignmentPlanView, CBridgeViewPane)

CAlignmentPlanView::CAlignmentPlanView()
{
}

CAlignmentPlanView::~CAlignmentPlanView()
{
}


BEGIN_MESSAGE_MAP(CAlignmentPlanView, CBridgeViewPane)
	//{{AFX_MSG_MAP(CAlignmentPlanView)
	ON_COMMAND(ID_VIEWSETTINGS, OnViewSettings)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAlignmentPlanView drawing

void CAlignmentPlanView::OnInitialUpdate() 
{
   EnableToolTips();
   CBridgeViewPane::OnInitialUpdate();
}

void CAlignmentPlanView::BuildDisplayLists()
{
   m_pDispMgr->EnableLBtnSelect(TRUE);
   m_pDispMgr->EnableRBtnSelect(TRUE);
   m_pDispMgr->SetSelectionLineColor(SELECTED_OBJECT_LINE_COLOR);
   m_pDispMgr->SetSelectionFillColor(SELECTED_OBJECT_FILL_COLOR);

   CBridgeViewPane::SetMappingMode(WBFL::DManip::MapMode::Isotropic);

   // Setup display lists
   m_pDispMgr->CreateDisplayList(LABEL_DISPLAY_LIST);
   m_pDispMgr->CreateDisplayList(TITLE_DISPLAY_LIST);
   m_pDispMgr->CreateDisplayList(ALIGNMENT_DISPLAY_LIST);
   m_pDispMgr->CreateDisplayList(BRIDGE_DISPLAY_LIST);
   m_pDispMgr->CreateDisplayList(NORTH_ARROW_DISPLAY_LIST);
}

/////////////////////////////////////////////////////////////////////////////
// CAlignmentPlanView diagnostics

#ifdef _DEBUG
void CAlignmentPlanView::AssertValid() const
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
	CBridgeViewPane::AssertValid();
}

void CAlignmentPlanView::Dump(CDumpContext& dc) const
{
	CBridgeViewPane::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CBridgePlanView message handlers

void CAlignmentPlanView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
   CBridgeViewPane::OnUpdate(pSender,lHint,pHint);
   UpdateDisplayObjects();
   UpdateDrawingScale();
   Invalidate();
   UpdateWindow();
}

void CAlignmentPlanView::HandleLButtonDblClk(UINT nFlags, CPoint logPoint) 
{
   GetFrame()->PostMessage(WM_COMMAND,ID_PROJECT_ALIGNMENT,0);
}

void CAlignmentPlanView::HandleContextMenu(CWnd* pWnd,CPoint logPoint)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CPGSDocBase* pDoc = (CPGSDocBase*)GetDocument();
   auto pMenu = WBFL::EAF::Menu::CreateContextMenu(pDoc->GetPluginCommandManager());
   pMenu->LoadMenu(IDR_ALIGNMENT_PLAN_CTX,nullptr);

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
}

void CAlignmentPlanView::OnViewSettings() 
{
   ((CPGSDocBase*)GetDocument())->EditBridgeViewSettings(VS_BRIDGE_ALIGNMENT);
}

void CAlignmentPlanView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
   if ( nChar == VK_DOWN && ::GetKeyState(VK_CONTROL) < 0 )
   {
      // CTRL + down arrow... put focus in Alignment Profile View
      m_pFrame->GetAlignmentProfileView()->SetFocus();
   }

   CBridgeViewPane::OnKeyDown(nChar,nRepCnt,nFlags);
}

void CAlignmentPlanView::UpdateDisplayObjects()
{
   CWaitCursor wait;

   CDManipClientDC dc(this);

   m_pDispMgr->ClearDisplayObjects();

   BuildTitleDisplayObjects();
   BuildAlignmentDisplayObjects();
   BuildBridgeDisplayObjects();
   BuildLabelDisplayObjects();
   BuildNorthArrowDisplayObjects();
}

void CAlignmentPlanView::BuildTitleDisplayObjects()
{
   auto title_list = m_pDispMgr->FindDisplayList(TITLE_DISPLAY_LIST);
   title_list->Clear();

   auto title = WBFL::DManip::ViewTitle::Create();
   title->SetText(_T("Alignment"));
   title_list->AddDisplayObject(title);
}

void CAlignmentPlanView::BuildAlignmentDisplayObjects()
{
   auto display_list = m_pDispMgr->FindDisplayList(ALIGNMENT_DISPLAY_LIST);
   display_list->Clear();

   
   auto pBroker = EAFGetBroker();

   GET_IFACE2(pBroker,IRoadway,pRoadway);
   GET_IFACE2(pBroker, IRoadwayData, pRoadwayData);

   Float64 n = 10;
   Float64 start_station, start_elevation, start_grade;
   Float64 end_station, end_elevation, end_grade;
   CComPtr<IPoint2d> pntStart, pntEnd;
   pRoadway->GetStartPoint(n,&start_station,&start_elevation,&start_grade,&pntStart);
   pRoadway->GetEndPoint(  n,&end_station,  &end_elevation,  &end_grade,  &pntEnd);

   // The alignment is represented on the screen by a poly line object
   auto doAlignment = WBFL::DManip::PolyLineDisplayObject::Create();

   // Register an event sink with the alignment object so that we can handle dbl-clicks
   // on the alignment differently then a general dbl-click
   auto events = std::make_shared<CAlignmentDisplayObjectEvents>(pBroker, m_pFrame, CAlignmentDisplayObjectEvents::Alignment, doAlignment);
   doAlignment->RegisterEventSink(events);

   doAlignment->SetToolTipText(_T("Double click to edit alignment.\r\nRight click for more options."));
   doAlignment->SetMaxTipWidth(TOOLTIP_WIDTH);
   doAlignment->SetTipDisplayTime(TOOLTIP_DURATION);

   // model the alignment as a series of individual points
   CComPtr<IDirection> bearing;
   bearing.CoCreateInstance(CLSID_Direction);
   long nPoints = 50;
   Float64 station_inc = (end_station - start_station)/(nPoints-1);
   Float64 station = start_station;
   for ( long i = 0; i < nPoints; i++, station += station_inc)
   {
      CComPtr<IPoint2d> p;
      pRoadway->GetPoint(station,0.00,bearing,pgsTypes::pcGlobal,&p);
      doAlignment->AddPoint(geomUtil::GetPoint(p));

      if (i == 0)
      {
         auto doText = WBFL::DManip::TextBlock::Create();
         doText->SetPosition(geomUtil::GetPoint(p));
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
      }
   }

   doAlignment->SetWidth(ALIGNMENT_LINE_WEIGHT);
   doAlignment->SetColor(ALIGNMENT_COLOR);
   doAlignment->SetPointType(WBFL::DManip::PointType::None);
   doAlignment->SetSelectionType(WBFL::DManip::SelectionType::All);
   doAlignment->SetID(ALIGNMENT_ID);

   display_list->AddDisplayObject(doAlignment);


   ////////////////
   if (pRoadwayData->GetRoadwaySectionData().AlignmentPointIdx != pRoadwayData->GetRoadwaySectionData().ProfileGradePointIdx)
   {
      // draw the profile grade line
      auto doPGL = WBFL::DManip::PolyLineDisplayObject::Create();

      Float64 station = start_station;
      for (long i = 0; i < nPoints; i++, station += station_inc)
      {
         IndexType alignmentIdx = pRoadway->GetAlignmentPointIndex(station); // get index of crown point corresponding to the alignment
         Float64 offset = pRoadway->GetProfileGradeLineOffset(alignmentIdx, station); // get the offset from the alignment point to the PGL

         ATLASSERT(!IsZero(offset)); // only drawing PGL if it is offset from alignment so this better not be zero

         bearing.Release();
         pRoadway->GetBearingNormal(station, &bearing);

         // use -offset because offset is from PGL to alignment... we need to plot alignemnt to PGL offset
         CComPtr<IPoint2d> p;
         pRoadway->GetPoint(station, -offset, bearing, pgsTypes::pcGlobal, &p);
         doPGL->AddPoint(geomUtil::GetPoint(p));

         if (i == 0)
         {
            auto doText = WBFL::DManip::TextBlock::Create();
            doText->SetPosition(geomUtil::GetPoint(p));
            doText->SetText(_T("PGL"));
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
         }
      }

      doPGL->SetWidth(PROFILE_LINE_WEIGHT);
      doPGL->SetColor(PROFILE_COLOR);
      doPGL->SetPointType(WBFL::DManip::PointType::None);

      display_list->AddDisplayObject(doPGL);
   }

   ////////////////

   // get point on alignment at start
   CComPtr<IDirection> dir;
   pRoadway->GetBearingNormal(start_station,&dir);
   CComPtr<IPoint2d> rotation_center;
   pRoadway->GetPoint(start_station,0.00,dir,pgsTypes::pcGlobal,&rotation_center);

   // get point on alignment at last pier
   CComPtr<IPoint2d> end_point;
   dir.Release();
   pRoadway->GetBearingNormal(end_station,&dir);
   pRoadway->GetPoint(end_station,0.00,dir,pgsTypes::pcGlobal,&end_point);

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

   CPGSDocBase* pDoc = (CPGSDocBase*)GetDocument();
   UINT settings = pDoc->GetAlignmentEditorSettings();
   if ( settings & IDA_AP_NORTH_UP )
   {
      m_pMapping->SetRotation((x1+x2)/2,(y1+y2)/2,0);
   }
   else
   {
      // rotation by negative of the angle
      m_pMapping->SetRotation((x1+x2)/2,(y1+y2)/2,-angle);
   }
}

void CAlignmentPlanView::BuildBridgeDisplayObjects()
{
   CPGSDocBase* pDoc = (CPGSDocBase*)GetDocument();
   UINT settings = pDoc->GetAlignmentEditorSettings();
   if ( (settings & IDA_AP_DRAW_BRIDGE) == 0 )
   {
      return;
   }

   auto display_list = m_pDispMgr->FindDisplayList(BRIDGE_DISPLAY_LIST);
   display_list->Clear();
   
   // Build the deck shape
   CComPtr<IShape> shape;
   CComPtr<IPoint2d> pntShape;
   
   auto pBroker = EAFGetBroker();
   GET_IFACE2(pBroker, IBridge, pBridge);
   pgsTypes::SupportedDeckType deckType = pBridge->GetDeckType();
   if (deckType == pgsTypes::sdtNone)
   {
      GET_IFACE2(pBroker, IGirder, pGirder);

      CComPtr<ICompositeShape> compShape;
      compShape.CoCreateInstance(CLSID_CompositeShape);
      GroupIndexType nGroups = pBridge->GetGirderGroupCount();
      for (GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++)
      {
         GirderIndexType nGirders = pBridge->GetGirderCount(grpIdx);
         for (GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++)
         {
            SegmentIndexType nSegments = pBridge->GetSegmentCount(grpIdx, gdrIdx);
            for (SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++)
            {
               CSegmentKey segmentKey(grpIdx, gdrIdx, segIdx);
               CComPtr<IPoint2d> pntEnd1Left, pntEnd1, pntEnd1Right, pntEnd2Left, pntEnd2, pntEnd2Right;
               pGirder->GetSegmentPlanPoints(segmentKey, pgsTypes::pcGlobal, &pntEnd1Left, &pntEnd1, &pntEnd1Right, &pntEnd2Right, &pntEnd2, &pntEnd2Left);

               CComPtr<IPolyShape> gdrShape;
               gdrShape.CoCreateInstance(CLSID_PolyShape);
               gdrShape->AddPointEx(pntEnd1Left);
               gdrShape->AddPointEx(pntEnd1Right);
               gdrShape->AddPointEx(pntEnd2Right);
               gdrShape->AddPointEx(pntEnd2Left);

               CComQIPtr<IShape> shape(gdrShape);
               compShape->AddShape(shape, VARIANT_FALSE);

               if (grpIdx == 0 && gdrIdx == 0 && segIdx == 0)
               {
                  pntShape = pntEnd1Left;
               }
            }
         }
      }

      compShape.QueryInterface(&shape);
   }
   else
   {
      SpanIndexType nSpans = pBridge->GetSpanCount();
      CComPtr<IPoint2dCollection> points;
      pBridge->GetSlabPerimeter(0, nSpans - 1, 10 * nSpans, pgsTypes::pcGlobal, &points);

      CComPtr<IPolyShape> poly_shape;
      poly_shape.CoCreateInstance(CLSID_PolyShape);
      poly_shape->AddPoints(points);

      poly_shape.QueryInterface(&shape);

      points->get_Item(0, &pntShape);
   }

   auto doBridge = WBFL::DManip::PointDisplayObject::Create();
   doBridge->SetPosition(geomUtil::GetPoint(pntShape),false, false);

   auto strategy = WBFL::DManip::ShapeDrawStrategy::Create();
   strategy->SetShape(geomUtil::ConvertShape(shape));
   strategy->SetSolidLineColor(IsStructuralDeck(deckType) ? DECK_BORDER_COLOR : NONSTRUCTURAL_DECK_BORDER_COLOR);
   strategy->SetSolidFillColor(IsStructuralDeck(deckType) ? DECK_FILL_COLOR : NONSTRUCTURAL_DECK_FILL_COLOR);
   strategy->Fill(true);
   doBridge->SetDrawingStrategy(strategy);

   doBridge->SetSelectionType(WBFL::DManip::SelectionType::All);
   doBridge->SetID(BRIDGE_ID);

   display_list->AddDisplayObject(doBridge);

   // Register an event sink with the alignment object so that we can handle double clicks
   // on the alignment differently then a general dbl-click
   auto events = std::make_shared<CBridgeDisplayObjectEvents>(pBroker, m_pFrame, doBridge, CBridgeDisplayObjectEvents::Plan);
   doBridge->RegisterEventSink(events);
   doBridge->SetToolTipText(_T("Double click to edit bridge.\r\nRight click for more options."));
   doBridge->SetMaxTipWidth(TOOLTIP_WIDTH);
   doBridge->SetTipDisplayTime(TOOLTIP_DURATION);

   // Draw the Bridge Line
   Float64 start_station = pBridge->GetPierStation(0);
   Float64 end_station = pBridge->GetPierStation(pBridge->GetPierCount()-1);

   // The bridge is represented on the screen by a poly line object
   auto doBridgeLine = WBFL::DManip::PolyLineDisplayObject::Create();

   Float64 alignment_offset = pBridge->GetAlignmentOffset();

   // model the alignment as a series of individual points
   GET_IFACE2(pBroker, IRoadway, pRoadway);
   long nPoints = 20;
   Float64 station_inc = (end_station - start_station)/(nPoints-1);
   Float64 station = start_station;
   for ( long i = 0; i < nPoints; i++, station += station_inc)
   {
      CComPtr<IDirection> normal;
      pRoadway->GetBearingNormal(station,&normal);
      CComPtr<IPoint2d> p;
      pRoadway->GetPoint(station,alignment_offset,normal,pgsTypes::pcGlobal,&p);
      doBridgeLine->AddPoint(geomUtil::GetPoint(p));
   }

   doBridgeLine->SetWidth(BRIDGE_LINE_WEIGHT);
   doBridgeLine->SetColor(BRIDGE_COLOR);
   doBridgeLine->SetPointType(WBFL::DManip::PointType::None);

   doBridgeLine->SetSelectionType(WBFL::DManip::SelectionType::None);
   doBridgeLine->SetID(BRIDGELINE_ID);

   display_list->AddDisplayObject(doBridgeLine);
}

void CAlignmentPlanView::BuildLabelDisplayObjects()
{
   auto label_display_list = m_pDispMgr->FindDisplayList(LABEL_DISPLAY_LIST);
   label_display_list->Clear();

   
   auto pBroker = EAFGetBroker();

   // Label Start and End of Bridge
   CPGSDocBase* pDoc = (CPGSDocBase*)GetDocument();
   UINT settings = pDoc->GetAlignmentEditorSettings();
   if ( (settings & IDA_AP_DRAW_BRIDGE) != 0 )
   {
      GET_IFACE2(pBroker,IBridge,pBridge);
      Float64 start_station = pBridge->GetPierStation(0);
      Float64 end_station = pBridge->GetPierStation(pBridge->GetPierCount()-1);
      CreateStationLabel(label_display_list,start_station,_T("Start"),LABEL_NORMAL_TO_ALIGNMENT, TA_BASELINE | TA_RIGHT);
      CreateStationLabel(label_display_list,end_station,_T("End"),    LABEL_NORMAL_TO_ALIGNMENT, TA_BASELINE | TA_RIGHT);
   }

   // Start/End of station range labels
   GET_IFACE2(pBroker, IRoadway, pRoadway);
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

   // Label Horizontal Curve Points
   IndexType nHC = pRoadway->GetCurveCount();
   for ( IndexType hcIdx = 0; hcIdx < nHC; hcIdx++ )
   {
      CComPtr<ICompoundCurve> hc;
      pRoadway->GetCurve(hcIdx,pgsTypes::pcGlobal,&hc);

      Float64 L;
      hc->get_TotalLength(&L);
      if ( IsZero(L) )
      {
         // This is just a PI
         CComPtr<IPoint2d> pntPI;
         hc->get_PI(&pntPI);
         Float64 station, offset;
         pRoadway->GetStationAndOffset(pgsTypes::pcGlobal,pntPI,&station,&offset);
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
            pRoadway->GetStationAndOffset(pgsTypes::pcGlobal,pntTS,&station,&offset);
            ATLASSERT(IsZero(offset)); // point is on the alignment
            CreateStationLabel(label_display_list,station,_T("PC"));
         }
         else
         {
            pRoadway->GetStationAndOffset(pgsTypes::pcGlobal,pntTS,&station,&offset);
            ATLASSERT(IsZero(offset)); // point is on the alignment
            CreateStationLabel(label_display_list,station,_T("TS"));

            pRoadway->GetStationAndOffset(pgsTypes::pcGlobal,pntSC,&station,&offset);
            ATLASSERT(IsZero(offset)); // point is on the alignment
            CreateStationLabel(label_display_list,station,_T("SC"));
         }

         if ( IsZero(Ls2) )
         {
            pRoadway->GetStationAndOffset(pgsTypes::pcGlobal,pntST,&station,&offset);
            ATLASSERT(IsZero(offset)); // point is on the alignment
            CreateStationLabel(label_display_list,station,_T("PT"));
         }
         else
         {
            pRoadway->GetStationAndOffset(pgsTypes::pcGlobal,pntCS,&station,&offset);
            ATLASSERT(IsZero(offset)); // point is on the alignment
            CreateStationLabel(label_display_list,station,_T("CS"));

            pRoadway->GetStationAndOffset(pgsTypes::pcGlobal,pntST,&station,&offset);
            ATLASSERT(IsZero(offset)); // point is on the alignment
            CreateStationLabel(label_display_list,station,_T("ST"));
         }
      }
   }
}

void CAlignmentPlanView::CreateStationLabel(std::shared_ptr<WBFL::DManip::iDisplayList> pDisplayList,Float64 station,LPCTSTR strBaseLabel,long angle,UINT textAlign)
{
   
   auto pBroker = EAFGetBroker();

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   GET_IFACE2(pBroker,IRoadway,pRoadway);


   CComPtr<IPoint2d> p;
   pRoadway->GetPoint(station,0.00,nullptr,pgsTypes::pcGlobal,&p);

   auto doLabel = WBFL::DManip::TextBlock::Create();
   doLabel->SetPosition(geomUtil::GetPoint(p));
   
   CString strLabel;
   if ( strBaseLabel == nullptr )
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
   doLabel->SetPointSize(100);

   pDisplayList->AddDisplayObject(doLabel);
}

void CAlignmentPlanView::BuildNorthArrowDisplayObjects()
{
   
   auto pBroker = EAFGetBroker();

   auto display_list = m_pDispMgr->FindDisplayList(NORTH_ARROW_DISPLAY_LIST);
   display_list->Clear();


   auto doNorth = WBFL::DManip::NorthArrow::Create();

   auto [center,angle] = m_pMapping->GetRotation();

   Float64 direction = PI_OVER_2 + angle;
   doNorth->SetDirection(direction);

   display_list->AddDisplayObject(doNorth);
}

void CAlignmentPlanView::UpdateDrawingScale()
{
   auto title_display_list = m_pDispMgr->FindDisplayList(TITLE_DISPLAY_LIST);
   auto na_display_list = m_pDispMgr->FindDisplayList(NORTH_ARROW_DISPLAY_LIST);

   if ( title_display_list == nullptr || na_display_list == nullptr )
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
