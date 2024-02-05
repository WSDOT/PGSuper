///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2024  Washington State Department of Transportation
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

// TogaGirderModelSectionView.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "TxDOTOptionalDesignDoc.h"
#include "PGSuperColors.h"
#include "TogaGirderModelSectionView.h"
#include "TxDOTOptionalDesignGirderViewPage.h"
#include "TogaGMDisplayMgrEventsImpl.h"
#include "TogaDisplayObjectFactory.h"

#include <IFace\Bridge.h>
#include <IFace\DrawBridgeSettings.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\EditByUI.h>
#include <MfcTools\Text.h>
#include <IFace\Intervals.h>

#include <WBFLGenericBridgeTools.h>
#include <WBFLGeometry/GeomHelpers.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define SOCKET_HT   0
#define SOCKET_HB   1
#define SOCKET_TFL  2
#define SOCKET_TFR  3
#define SOCKET_BFL  4
#define SOCKET_BFR  5
#define SOCKET_BC   6
#define SOCKET_CGPS 7

#define SECTION_LIST             1
#define STRAIGHT_STRAND_LIST     2
#define HARPED_STRAND_LIST       3
#define TEMP_STRAND_LIST         4
#define DIMENSION_LIST           5
#define CG_LIST                  6
#define LONG_REINF_LIST          7

/////////////////////////////////////////////////////////////////////////////
// CTogaGirderModelSectionView

IMPLEMENT_DYNCREATE(CTogaGirderModelSectionView, CDisplayView)

CTogaGirderModelSectionView::CTogaGirderModelSectionView()
{
   m_bUpdateError = false;
}

CTogaGirderModelSectionView::~CTogaGirderModelSectionView()
{
}


BEGIN_MESSAGE_MAP(CTogaGirderModelSectionView, CDisplayView)
	//{{AFX_MSG_MAP(CTogaGirderModelSectionView)
	ON_WM_CREATE()
	ON_COMMAND(ID_VIEWSETTINGS, OnViewSettings)
	ON_COMMAND(ID_LEFTEND, OnLeftEnd)
	ON_COMMAND(ID_LEFT_HP, OnLeftHp)
	ON_COMMAND(ID_CENTER, OnCenter)
	ON_COMMAND(ID_RIGHT_HP, OnRightHp)
	ON_COMMAND(ID_RIGHTEND, OnRightEnd)
	ON_COMMAND(ID_USER_CUT, OnUserCut)
	ON_WM_SIZE()
	ON_WM_PAINT()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTogaGirderModelSectionView drawing
void CTogaGirderModelSectionView::DoPrint(CDC* pDC, CPrintInfo* pInfo)
{
   OnBeginPrinting(pDC, pInfo);
   OnPrepareDC(pDC);
   ScaleToFit();
   OnDraw(pDC);
   OnEndPrinting(pDC, pInfo);
}

/////////////////////////////////////////////////////////////////////////////
// CTogaGirderModelSectionView diagnostics

#ifdef _DEBUG
void CTogaGirderModelSectionView::AssertValid() const
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
	CDisplayView::AssertValid();
}

void CTogaGirderModelSectionView::Dump(CDumpContext& dc) const
{
	CDisplayView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CTogaGirderModelSectionView message handlers

void CTogaGirderModelSectionView::OnInitialUpdate() 
{
   EnableToolTips();

   CreateDisplayLists();

	CDisplayView::OnInitialUpdate();

   CTxDOTOptionalDesignDoc* pDoc = (CTxDOTOptionalDesignDoc*)GetDocument();

   m_pDispMgr->SetSelectionLineColor(SELECTED_OBJECT_LINE_COLOR);
   m_pDispMgr->SetSelectionFillColor(SELECTED_OBJECT_FILL_COLOR);

   auto factory = std::make_shared<CTogaDisplayObjectFactory>(pDoc);
   m_pDispMgr->AddDisplayObjectFactory(factory);

   // set up default event handler for canvas
   auto events = std::make_shared<CTogaGMDisplayMgrEventsImpl>(pDoc, m_pFrame, this);
   m_pDispMgr->RegisterEventSink(events);
}

void CTogaGirderModelSectionView::CreateDisplayLists()
{
   m_pDispMgr->EnableLBtnSelect(TRUE);
   m_pDispMgr->EnableRBtnSelect(TRUE);
   m_pDispMgr->SetSelectionLineColor(SELECTED_OBJECT_LINE_COLOR);
   m_pDispMgr->SetSelectionFillColor(SELECTED_OBJECT_FILL_COLOR);

   // Create display lists
   m_pDispMgr->CreateDisplayList(STRAIGHT_STRAND_LIST);
   m_pDispMgr->CreateDisplayList(HARPED_STRAND_LIST);
   m_pDispMgr->CreateDisplayList(TEMP_STRAND_LIST);
   m_pDispMgr->CreateDisplayList(LONG_REINF_LIST);
   m_pDispMgr->CreateDisplayList(CG_LIST);
   m_pDispMgr->CreateDisplayList(DIMENSION_LIST);
   m_pDispMgr->CreateDisplayList(SECTION_LIST);
}

void CTogaGirderModelSectionView::UpdateDisplayObjects()
{
   // clean out all the display objects
   m_pDispMgr->ClearDisplayObjects();

   CTxDOTOptionalDesignDoc* pDoc = (CTxDOTOptionalDesignDoc*)GetDocument();

   SpanIndexType span;
   GirderIndexType girder;
   m_pFrame->GetSpanAndGirderSelection(&span,&girder);

   if ( span == ALL_SPANS || girder == ALL_GIRDERS)
      return;

   CSegmentKey segmentKey(span,girder,0);

   // Grab hold of the broker so we can pass it as a parameter
   try
   {
      CComPtr<IBroker> pBroker = pDoc->GetUpdatedBroker();

      UINT settings = pDoc->GetGirderEditorSettings();

      BuildSectionDisplayObjects(pDoc, pBroker, segmentKey);

      if ( settings & IDG_SV_SHOW_STRANDS )
         BuildStrandDisplayObjects(pDoc, pBroker, segmentKey);

      if ( settings & IDG_SV_SHOW_LONG_REINF )
         BuildLongReinfDisplayObjects(pDoc, pBroker, segmentKey);

      if (settings & IDG_SV_SHOW_PS_CG)
         BuildCGDisplayObjects(pDoc, pBroker, segmentKey);

      if ( settings & IDG_SV_SHOW_DIMENSIONS )
         BuildDimensionDisplayObjects(pDoc, pBroker, segmentKey);

      SetMappingMode(WBFL::DManip::MapMode::Isotropic);
   }
   catch(...)
   {
      // shouldn't matter here - page should present error
   }
}

void CTogaGirderModelSectionView::BuildSectionDisplayObjects(CTxDOTOptionalDesignDoc* pDoc,IBroker* pBroker,const CSegmentKey& segmentKey)
{
   auto pDL = m_pDispMgr->FindDisplayList(SECTION_LIST);
   ATLASSERT(pDL);
   pDL->Clear();

   pgsPointOfInterest poi(segmentKey,m_pFrame->GetCurrentCutLocation());

   GET_IFACE2(pBroker,IShapes,pShapes);
   GET_IFACE2(pBroker,IGirder,pGirder);

   Float64 top_width = pGirder->GetTopWidth(poi);
   Float64 bottom_width = pGirder->GetBottomWidth(poi);

   auto doPnt = WBFL::DManip::PointDisplayObject::Create(1);
   auto strategy = WBFL::DManip::ShapeDrawStrategy::Create();
   doPnt->SetDrawingStrategy(strategy);

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);

   CComPtr<IShape> shape;
   pShapes->GetSegmentShape(releaseIntervalIdx,poi,false,pgsTypes::scGirder,&shape);

   strategy->SetShape(geomUtil::ConvertShape(shape));
   strategy->SetSolidLineColor(SEGMENT_BORDER_COLOR);
   strategy->SetSolidFillColor(SEGMENT_FILL_COLOR);
   strategy->SetVoidLineColor(VOID_BORDER_COLOR);
   strategy->SetVoidFillColor(GetSysColor(COLOR_WINDOW));
   strategy->Fill(true);

   // Set up sockets so dimension lines can plug into the girder shape
   CComPtr<IRect2d> box;
   shape->get_BoundingBox(&box);
   CComPtr<IPoint2d> pntTC, pntBC; // top and bottom center

   auto connectable = std::dynamic_pointer_cast<WBFL::DManip::iConnectable>(doPnt);

   // sockets for top flange dimension line
   box->get_TopCenter(&pntTC);
   pntTC->Offset(-top_width/2,0);
   auto socketTFL = connectable->AddSocket(SOCKET_TFL,geomUtil::GetPoint(pntTC));
   pntTC->Offset(top_width,0);
   auto socketTFR = connectable->AddSocket(SOCKET_TFR,geomUtil::GetPoint(pntTC));

   // sockets for bottom flange dimension line
   box->get_BottomCenter(&pntBC);
   pntBC->Offset(-bottom_width/2,0);
   auto socketBFL = connectable->AddSocket(SOCKET_BFL,geomUtil::GetPoint(pntBC));
   pntBC->Offset(bottom_width,0);
   auto socketBFR = connectable->AddSocket(SOCKET_BFR,geomUtil::GetPoint(pntBC));

   // sockets for height dimension line
   pntTC.Release();
   pntBC.Release();
   box->get_TopCenter(&pntTC);
   box->get_BottomCenter(&pntBC);
   Float64 dx = -Max(top_width,bottom_width)/2.0;
   pntTC->Offset(dx,0);
   pntBC->Offset(dx,0);
   auto socketHT = connectable->AddSocket(SOCKET_HT, geomUtil::GetPoint(pntTC));
   auto socketHB = connectable->AddSocket(SOCKET_HB, geomUtil::GetPoint(pntBC));

   // sockets for center of prestressing
   pntBC.Release();
   box->get_BottomCenter(&pntBC);
   auto socketBC = connectable->AddSocket(SOCKET_BC,geomUtil::GetPoint(pntBC));

   pDL->AddDisplayObject(doPnt);
}

void CTogaGirderModelSectionView::BuildStrandDisplayObjects(CTxDOTOptionalDesignDoc* pDoc,IBroker* pBroker,const CSegmentKey& segmentKey)
{
   pgsPointOfInterest poi(segmentKey,m_pFrame->GetCurrentCutLocation());

   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeom);


   GET_IFACE2(pBroker,IMaterials,pMaterial);
   const auto* pStrand = pMaterial->GetStrandMaterial(segmentKey,pgsTypes::Straight);
   Float64 diameter = pStrand->GetNominalDiameter();

   auto strategy = WBFL::DManip::SimpleDrawPointStrategy::Create();
   strategy->SetColor(STRAND_FILL_COLOR);
   strategy->SetPointType(WBFL::DManip::PointType::Circle);
   strategy->SetPointSize(diameter);

   auto debond_strategy = WBFL::DManip::SimpleDrawPointStrategy::Create();
   debond_strategy->SetColor(DEBOND_FILL_COLOR);
   debond_strategy->SetPointType(WBFL::DManip::PointType::Circle);
   debond_strategy->SetPointSize(diameter);

   // Straight strands
   auto pStraightDL = m_pDispMgr->FindDisplayList(STRAIGHT_STRAND_LIST);
   ATLASSERT(pStraightDL);
   pStraightDL->Clear();

   CComPtr<IPoint2dCollection> points;
   pStrandGeom->GetStrandPositions(poi, pgsTypes::Straight,&points);
   IndexType nStrandPoints;
   points->get_Count(&nStrandPoints);
   IndexType strandPointIdx;
   for ( strandPointIdx = 0; strandPointIdx < nStrandPoints; strandPointIdx++ )
   {
      CComPtr<IPoint2d> p;
      points->get_Item(strandPointIdx,&p);

      auto doPnt = WBFL::DManip::PointDisplayObject::Create(strandPointIdx);
      doPnt->SetPosition(geomUtil::GetPoint(p),false,false);

      if ( pStrandGeom->IsStrandDebonded(poi,strandPointIdx,pgsTypes::Straight) )
         doPnt->SetDrawingStrategy(debond_strategy);
      else
         doPnt->SetDrawingStrategy(strategy);

      doPnt->SetSelectionType(WBFL::DManip::SelectionType::All);

      pStraightDL->AddDisplayObject(doPnt);
   }

   // Harped Strands
   auto pHarpedDL = m_pDispMgr->FindDisplayList(HARPED_STRAND_LIST);
   ATLASSERT(pHarpedDL);
   pHarpedDL->Clear();

   pStrand = pMaterial->GetStrandMaterial(segmentKey,pgsTypes::Harped);
   diameter = pStrand->GetNominalDiameter();

   points.Release();
   pStrandGeom->GetStrandPositions(poi, pgsTypes::Harped,&points);
   points->get_Count(&nStrandPoints);
   for ( strandPointIdx = 0; strandPointIdx < nStrandPoints; strandPointIdx++ )
   {
      CComPtr<IPoint2d> p;
      points->get_Item(strandPointIdx,&p);

      auto doPnt = WBFL::DManip::PointDisplayObject::Create(strandPointIdx);
      doPnt->SetPosition(geomUtil::GetPoint(p), false, false);
      doPnt->SetDrawingStrategy(strategy);

      pHarpedDL->AddDisplayObject(doPnt);
   }

   // Temporary Strands
   auto pTempDL = m_pDispMgr->FindDisplayList(TEMP_STRAND_LIST);
   ATLASSERT(pTempDL);
   pTempDL->Clear();

   pStrand  = pMaterial->GetStrandMaterial(segmentKey,pgsTypes::Temporary);
   diameter = pStrand->GetNominalDiameter();

   points.Release();
   pStrandGeom->GetStrandPositions(poi, pgsTypes::Temporary,&points);
   points->get_Count(&nStrandPoints);
   for ( strandPointIdx = 0; strandPointIdx < nStrandPoints; strandPointIdx++ )
   {
      CComPtr<IPoint2d> p;
      points->get_Item(strandPointIdx,&p);

      auto doPnt = WBFL::DManip::PointDisplayObject::Create(strandPointIdx);
      doPnt->SetPosition(geomUtil::GetPoint(p), false, false);
      doPnt->SetDrawingStrategy(strategy);

      pTempDL->AddDisplayObject(doPnt);
   }
}

void CTogaGirderModelSectionView::BuildLongReinfDisplayObjects(CTxDOTOptionalDesignDoc* pDoc,IBroker* pBroker,const CSegmentKey& segmentKey)
{
   auto pDL = m_pDispMgr->FindDisplayList(LONG_REINF_LIST);
   ATLASSERT(pDL);
   pDL->Clear();

   pgsPointOfInterest poi(segmentKey,m_pFrame->GetCurrentCutLocation());

   auto strategy = WBFL::DManip::SimpleDrawPointStrategy::Create();
   strategy->SetColor(REBAR_COLOR);
   strategy->SetPointType(WBFL::DManip::PointType::Circle);

   GET_IFACE2(pBroker,ILongRebarGeometry,pLongRebarGeom);

   CComPtr<IRebarSection> rebar_section;
   pLongRebarGeom->GetRebars(poi,&rebar_section);

   CComPtr<IEnumRebarSectionItem> enum_items;
   rebar_section->get__EnumRebarSectionItem(&enum_items);

   long id = 0;

   CComPtr<IRebarSectionItem> item;
   while ( enum_items->Next(1,&item,nullptr) != S_FALSE )
   {
      CComPtr<IPoint2d> location;
      item->get_Location(&location);

      Float64 x, y;
      location->get_X(&x);
      location->get_Y(&y);

      item.Release();

      WBFL::Geometry::Point2d p(x, y);

      auto doPnt = WBFL::DManip::PointDisplayObject::Create(id++);
      doPnt->SetPosition(p,false,false);
      doPnt->SetDrawingStrategy(strategy);

      pDL->AddDisplayObject(doPnt);
   }
}

void CTogaGirderModelSectionView::BuildCGDisplayObjects(CTxDOTOptionalDesignDoc* pDoc,IBroker* pBroker,const CSegmentKey& segmentKey)
{
   auto pDL = m_pDispMgr->FindDisplayList(CG_LIST);
   ATLASSERT(pDL);
   pDL->Clear();

   pgsPointOfInterest poi(segmentKey,m_pFrame->GetCurrentCutLocation());

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);

   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeom);
   Float64 ecc = pStrandGeom->GetEccentricity(releaseIntervalIdx, poi,true).Y();

   GET_IFACE2(pBroker,ISectionProperties,pSectProp);
   Float64 Yb = pSectProp->GetY(releaseIntervalIdx,poi,pgsTypes::BottomGirder);
   Float64 Hg = pSectProp->GetHg(releaseIntervalIdx,poi);

   WBFL::Geometry::Point2d point(0,Yb - (Hg+ecc));

   auto doPnt = WBFL::DManip::PointDisplayObject::Create(1);
   doPnt->SetPosition(point,false,false);

   auto strategy = WBFL::DManip::TargetDrawStrategy::Create();
   CRect rc;
   GetClientRect(&rc);
   strategy->SetRadius(rc.Width()/80);

   doPnt->SetDrawingStrategy(strategy);

   // setup socket for dimension line
   auto connectable = std::dynamic_pointer_cast<WBFL::DManip::iConnectable>(doPnt);

   // sockets for top flange dimension line
   auto socketCGPS = connectable->AddSocket(SOCKET_CGPS,point);

   pDL->AddDisplayObject(doPnt);
}

void CTogaGirderModelSectionView::BuildDimensionDisplayObjects(CTxDOTOptionalDesignDoc* pDoc,IBroker* pBroker,const CSegmentKey& segmentKey)
{
   auto pDL = m_pDispMgr->FindDisplayList(DIMENSION_LIST);
   ATLASSERT(pDL);
   pDL->Clear();

   auto doDimLineTopFlangeWidth = WBFL::DManip::DimensionLine::Create(0);
   auto doDimLineBottomFlangeWidth = WBFL::DManip::DimensionLine::Create(1);
   auto doDimLineHeight = WBFL::DManip::DimensionLine::Create(2);
   auto doDimLineCGPS = WBFL::DManip::DimensionLine::Create(3);

   // Connect the dimension lines to the sockets in the section display object
   auto pSectionDL = m_pDispMgr->FindDisplayList(SECTION_LIST);
   ATLASSERT(pSectionDL);

   // get the girder section display object
   auto doSection = pSectionDL->GetDisplayObject(0);

   // get it's iConnectable interface
   auto connectableSection = std::dynamic_pointer_cast<WBFL::DManip::iConnectable>(doSection);

   // get the sockets
   auto socketTFL = connectableSection->GetSocket(SOCKET_TFL, WBFL::DManip::AccessType::ByID);
   auto socketTFR = connectableSection->GetSocket(SOCKET_TFR, WBFL::DManip::AccessType::ByID);
   auto socketBFL = connectableSection->GetSocket(SOCKET_BFL, WBFL::DManip::AccessType::ByID);
   auto socketBFR = connectableSection->GetSocket(SOCKET_BFR, WBFL::DManip::AccessType::ByID);
   auto socketHT = connectableSection->GetSocket(SOCKET_HT, WBFL::DManip::AccessType::ByID);
   auto socketHB = connectableSection->GetSocket(SOCKET_HB, WBFL::DManip::AccessType::ByID);
   auto socketBC = connectableSection->GetSocket(SOCKET_BC,   WBFL::DManip::AccessType::ByID);

   UINT settings = pDoc->GetGirderEditorSettings();

   std::shared_ptr<WBFL::DManip::iSocket> socketCGPS;
   if (settings & IDG_SV_SHOW_PS_CG)
   {
      auto pCGList = m_pDispMgr->FindDisplayList(CG_LIST);

      auto doCGPS = pCGList->GetDisplayObject(0);
      auto connectableCGPS = std::dynamic_pointer_cast<WBFL::DManip::iConnectable>(doCGPS);
      socketCGPS = connectableCGPS->GetSocket(SOCKET_CGPS, WBFL::DManip::AccessType::ByID);
   }

   // get the connector interface from the dimension lines
   auto connectorTopFlangeWidth = std::dynamic_pointer_cast<WBFL::DManip::iConnector>(doDimLineTopFlangeWidth);
   auto connectorBottomFlangeWidth = std::dynamic_pointer_cast<WBFL::DManip::iConnector>(doDimLineBottomFlangeWidth);
   auto connectorHeight = std::dynamic_pointer_cast<WBFL::DManip::iConnector>(doDimLineHeight);
   auto connectorCGPS = std::dynamic_pointer_cast<WBFL::DManip::iConnector>(doDimLineCGPS);

   // connect the top flange width dimension line (across the top)
   auto startPlug = connectorTopFlangeWidth->GetStartPlug();
   auto endPlug = connectorTopFlangeWidth->GetEndPlug();
   
   socketTFL->Connect(startPlug);
   socketTFR->Connect(endPlug);

   // connect the bottom flange width dimension line (across the bottom)
   startPlug = connectorBottomFlangeWidth->GetStartPlug();
   endPlug = connectorBottomFlangeWidth->GetEndPlug();
   socketBFR->Connect(startPlug);
   socketBFL->Connect(endPlug);

   // connect the height dimension line (left side)
   startPlug = connectorHeight->GetStartPlug();
   endPlug = connectorHeight->GetEndPlug();
   socketHB->Connect(startPlug);
   socketHT->Connect(endPlug);

   // connect cg ps dimension line (bottom center to cg symbol)
   if (settings & IDG_SV_SHOW_PS_CG)
   {
      startPlug = connectorCGPS->GetStartPlug();
      endPlug = connectorCGPS->GetEndPlug();
      socketBC->Connect(startPlug);
      socketCGPS->Connect(endPlug);
   }

   // set the text labels on the dimension lines
   pgsPointOfInterest poi(segmentKey,m_pFrame->GetCurrentCutLocation());
   
   GET_IFACE2(pBroker,IGirder,pGirder);
   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeometry);
   GET_IFACE2(pBroker,ISectionProperties,pSectProp);

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);

   Float64 top_width = pGirder->GetTopWidth(poi);
   Float64 bottom_width = pGirder->GetBottomWidth(poi);
   Float64 height = pGirder->GetHeight(poi);
   Float64 ecc = pStrandGeometry->GetEccentricity(releaseIntervalIdx, poi,true).Y();
   Float64 yps = pSectProp->GetY(releaseIntervalIdx,poi,pgsTypes::BottomGirder) - ecc;

   CString strDim;

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   const WBFL::Units::LengthData& length_unit = pDisplayUnits->GetComponentDimUnit();

   auto textBlock = WBFL::DManip::TextBlock::Create();
   strDim = FormatDimension(top_width,length_unit);
   textBlock->SetText(strDim);
   doDimLineTopFlangeWidth->SetTextBlock(textBlock);

   textBlock = WBFL::DManip::TextBlock::Create();
   strDim = FormatDimension(bottom_width,length_unit);
   textBlock->SetText(strDim);
   doDimLineBottomFlangeWidth->SetTextBlock(textBlock);

   textBlock = WBFL::DManip::TextBlock::Create();
   strDim = FormatDimension(height,length_unit);
   textBlock->SetText(strDim);
   textBlock->SetBkMode(TRANSPARENT);
   doDimLineHeight->SetTextBlock(textBlock);

   if (settings & IDG_SV_SHOW_PS_CG)
   {
      textBlock = WBFL::DManip::TextBlock::Create();
      strDim = FormatDimension(yps,length_unit);
      textBlock->SetText(strDim);
      doDimLineCGPS->SetTextBlock(textBlock);

      // adjust the witness lines on the vertical dimensions
      long tx0,ty0;
      long tx1,ty1;
      m_pCoordinateMap->WPtoTP(0,0,&tx0,&ty0);
      m_pCoordinateMap->WPtoTP(Max(top_width,bottom_width)/2,0,&tx1,&ty1);
      doDimLineCGPS->SetWitnessLength(tx1-tx0);
      doDimLineHeight->SetWitnessLength(tx1-tx0+50);
   }


   // add the dimension line display objects to the display list
   pDL->AddDisplayObject(doDimLineTopFlangeWidth);
   pDL->AddDisplayObject(doDimLineBottomFlangeWidth);
   pDL->AddDisplayObject(doDimLineHeight);

   if (settings & IDG_SV_SHOW_PS_CG)
      pDL->AddDisplayObject(doDimLineCGPS);
}

void CTogaGirderModelSectionView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
   m_bUpdateError = false;

   CDisplayView::OnUpdate(pSender,lHint,pHint);


   if ( lHint == 0 ||
        lHint == HINT_GIRDERVIEWSETTINGSCHANGED ||
        lHint == HINT_GIRDERVIEWSECTIONCUTCHANGED ||
        lHint == HINT_GIRDERCHANGED )
   {
      // set up a valid dc first
      CDManipClientDC dc(this);

      UpdateDisplayObjects();
      ScaleToFit(false);
   }
   else if ( lHint == EAF_HINT_UPDATEERROR )
   {
      CString* pmsg = (CString*)pHint;
      m_ErrorMsg = *pmsg;
      m_bUpdateError = true;
      Invalidate();
   }
   else 
      ASSERT(0);
}

int CTogaGirderModelSectionView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CDisplayView::OnCreate(lpCreateStruct) == -1)
		return -1;

   m_pFrame = (CTxDOTOptionalDesignGirderViewPage*)GetParent();
   ASSERT( m_pFrame != 0 );
   ASSERT( m_pFrame->IsKindOf( RUNTIME_CLASS( CTxDOTOptionalDesignGirderViewPage ) ) );

	return 0;
}


void CTogaGirderModelSectionView::OnViewSettings() 
{
	// TODO: Add your command handler code here
	((CTxDOTOptionalDesignDoc*)GetDocument())->EditGirderViewSettings(VS_GIRDER_SECTION);
}

void CTogaGirderModelSectionView::OnLeftEnd() 
{
   m_pFrame->CutAtLeftEnd();
}

void CTogaGirderModelSectionView::OnLeftHp() 
{
   m_pFrame->CutAtLeftHp();
}

void CTogaGirderModelSectionView::OnCenter() 
{
   m_pFrame->CutAtCenter();
}

void CTogaGirderModelSectionView::OnRightHp() 
{
   m_pFrame->CutAtRightHp();
}

void CTogaGirderModelSectionView::OnRightEnd() 
{
   m_pFrame->CutAtRightEnd();
}

void CTogaGirderModelSectionView::OnUserCut() 
{
	m_pFrame->CutAtLocation();
}

void CTogaGirderModelSectionView::OnSize(UINT nType, int cx, int cy) 
{
	CDisplayView::OnSize(nType, cx, cy);

   CRect rect;
   GetClientRect(&rect);
   rect.DeflateRect(5,5,5,5);

   CSize size = rect.Size();
   size.cx = Max(0L,size.cx);
   size.cy = Max(0L,size.cy);

   SetLogicalViewRect(MM_TEXT,rect);

   SetScrollSizes(MM_TEXT,size,CScrollView::sizeDefault,CScrollView::sizeDefault);

   ScaleToFit();
}

void CTogaGirderModelSectionView::OnDraw(CDC* pDC) 
{
   if ( m_bUpdateError )
   {
      CString msg("Cannot draw girder section. See Status Center for more information");
      MultiLineTextOut(pDC,0,0,msg);
      return;
   }

   CDisplayView::OnDraw(pDC);
}
