///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2016  Washington State Department of Transportation
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

// GirderModelSectionView.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\resource.h"
#include "PGSuperAppPlugin\PGSuperApp.h"
#include "PGSuperDoc.h"
#include "PGSuperUnits.h"
#include "PGSuperColors.h"
#include "GirderModelSectionView.h"
#include "GirderModelChildFrame.h"
#include "GMDisplayMgrEventsImpl.h"
#include "DisplayObjectFactory.h"
#include <IFace\Bridge.h>
#include <IFace\DrawBridgeSettings.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\EditByUI.h>
#include <MfcTools\Text.h>

#include <WBFLGenericBridgeTools.h>
#include <WBFLDManip.h>

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
// CGirderModelSectionView

IMPLEMENT_DYNCREATE(CGirderModelSectionView, CDisplayView)

CGirderModelSectionView::CGirderModelSectionView():
m_CurrentSpanIdx(INVALID_INDEX),
m_CurrentGirderIdx(INVALID_INDEX)
{
   m_bUpdateError = false;
}

CGirderModelSectionView::~CGirderModelSectionView()
{
}


BEGIN_MESSAGE_MAP(CGirderModelSectionView, CDisplayView)
	//{{AFX_MSG_MAP(CGirderModelSectionView)
	ON_WM_CREATE()
	ON_COMMAND(ID_EDIT_GIRDER, OnEditGirder)
	ON_COMMAND(ID_EDIT_PRESTRESSING, OnEditPrestressing)
	ON_COMMAND(ID_VIEWSETTINGS, OnViewSettings)
	ON_COMMAND(ID_EDIT_STIRRUPS, OnEditStirrups)
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
// CGirderModelSectionView drawing
void CGirderModelSectionView::DoPrint(CDC* pDC, CPrintInfo* pInfo)
{
   OnBeginPrinting(pDC, pInfo);
   OnPrepareDC(pDC);
   ScaleToFit();
   OnDraw(pDC);
   OnEndPrinting(pDC, pInfo);
}

/////////////////////////////////////////////////////////////////////////////
// CGirderModelSectionView diagnostics

#ifdef _DEBUG
void CGirderModelSectionView::AssertValid() const
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
	CDisplayView::AssertValid();
}

void CGirderModelSectionView::Dump(CDumpContext& dc) const
{
	CDisplayView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CGirderModelSectionView message handlers

void CGirderModelSectionView::OnInitialUpdate() 
{
   EnableToolTips();

   CreateDisplayLists();

	CDisplayView::OnInitialUpdate();

   // set girder
   DidGirderSelectionChange();

   // build display objects
   // set up a valid dc first
   CDManipClientDC dc2(this);

   UpdateDisplayObjects();

   ScaleToFit();

   CPGSuperDoc* pDoc = (CPGSuperDoc*)GetDocument();

   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);
//   dispMgr->EnableLBtnSelectRect(TRUE);
//   dispMgr->EnableLBtnMultiSelect(TRUE,MK_SHIFT);
   dispMgr->SetSelectionLineColor(SELECTED_OBJECT_LINE_COLOR);
   dispMgr->SetSelectionFillColor(SELECTED_OBJECT_FILL_COLOR);

   CDisplayObjectFactory* factory = new CDisplayObjectFactory(pDoc);
   IUnknown* unk = factory->GetInterface(&IID_iDisplayObjectFactory);
   dispMgr->AddDisplayObjectFactory((iDisplayObjectFactory*)unk);

   // set up default event handler for canvas
   CGMDisplayMgrEventsImpl* events = new CGMDisplayMgrEventsImpl(pDoc, m_pFrame, this, false);
   unk = events->GetInterface(&IID_iDisplayMgrEvents);
   dispMgr->RegisterEventSink((iDisplayMgrEvents*)unk);

   CSelection selection = pDoc->GetSelection();
   if ( selection.Type != CSelection::Girder )
   {
      pDoc->SelectGirder(m_CurrentSpanIdx,m_CurrentGirderIdx);
   }
}

void CGirderModelSectionView::CreateDisplayLists()
{
   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   // Create display lists
   CComPtr<iDisplayList> straight_strand_list;
   ::CoCreateInstance(CLSID_DisplayList,NULL,CLSCTX_ALL,IID_iDisplayList,(void**)&straight_strand_list);
   straight_strand_list->SetID(STRAIGHT_STRAND_LIST);
   dispMgr->AddDisplayList(straight_strand_list);

   CComPtr<iDisplayList> harped_strand_list;
   ::CoCreateInstance(CLSID_DisplayList,NULL,CLSCTX_ALL,IID_iDisplayList,(void**)&harped_strand_list);
   harped_strand_list->SetID(HARPED_STRAND_LIST);
   dispMgr->AddDisplayList(harped_strand_list);

   CComPtr<iDisplayList> temp_strand_list;
   ::CoCreateInstance(CLSID_DisplayList,NULL,CLSCTX_ALL,IID_iDisplayList,(void**)&temp_strand_list);
   temp_strand_list->SetID(TEMP_STRAND_LIST);
   dispMgr->AddDisplayList(temp_strand_list);

   CComPtr<iDisplayList> long_reinf_list;
   ::CoCreateInstance(CLSID_DisplayList,NULL,CLSCTX_ALL,IID_iDisplayList,(void**)&long_reinf_list);
   long_reinf_list->SetID(LONG_REINF_LIST);
   dispMgr->AddDisplayList(long_reinf_list);

   CComPtr<iDisplayList> cg_list;
   ::CoCreateInstance(CLSID_DisplayList,NULL,CLSCTX_ALL,IID_iDisplayList,(void**)&cg_list);
   cg_list->SetID(CG_LIST);
   dispMgr->AddDisplayList(cg_list);

   CComPtr<iDisplayList> dimension_list;
   ::CoCreateInstance(CLSID_DisplayList,NULL,CLSCTX_ALL,IID_iDisplayList,(void**)&dimension_list);
   dimension_list->SetID(DIMENSION_LIST);
   dispMgr->AddDisplayList(dimension_list);

   CComPtr<iDisplayList> section_list;
   ::CoCreateInstance(CLSID_DisplayList,NULL,CLSCTX_ALL,IID_iDisplayList,(void**)&section_list);
   section_list->SetID(SECTION_LIST);
   dispMgr->AddDisplayList(section_list);
}

void CGirderModelSectionView::UpdateDisplayObjects()
{
   // get the display manager
   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   // clean out all the display objects
   dispMgr->ClearDisplayObjects();

   CPGSuperDoc* pDoc = (CPGSuperDoc*)GetDocument();

   // Grab hold of the broker so we can pass it as a parameter
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   UINT settings = pDoc->GetGirderEditorSettings();

   BuildSectionDisplayObjects(pDoc, pBroker, m_CurrentSpanIdx, m_CurrentGirderIdx, dispMgr);

   if ( settings & IDG_SV_SHOW_STRANDS )
      BuildStrandDisplayObjects(pDoc, pBroker,m_CurrentSpanIdx, m_CurrentGirderIdx, dispMgr);

   if ( settings & IDG_SV_SHOW_LONG_REINF )
      BuildLongReinfDisplayObjects(pDoc, pBroker,m_CurrentSpanIdx, m_CurrentGirderIdx, dispMgr);

   if (settings & IDG_SV_SHOW_PS_CG)
      BuildCGDisplayObjects(pDoc, pBroker,m_CurrentSpanIdx, m_CurrentGirderIdx, dispMgr);

   if ( settings & IDG_SV_SHOW_DIMENSIONS )
      BuildDimensionDisplayObjects(pDoc, pBroker,m_CurrentSpanIdx, m_CurrentGirderIdx, dispMgr);

   SetMappingMode(DManip::Isotropic);
}

void CGirderModelSectionView::BuildSectionDisplayObjects(CPGSuperDoc* pDoc,IBroker* pBroker,SpanIndexType span,GirderIndexType girder,iDisplayMgr* pDispMgr)
{
   CComPtr<iDisplayList> pDL;
   pDispMgr->FindDisplayList(SECTION_LIST,&pDL);
   ATLASSERT(pDL);
   pDL->Clear();

   pgsPointOfInterest poi(span,girder,m_pFrame->GetCurrentCutLocation());

   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,ISectProp2,pSectProp);
   GET_IFACE2(pBroker,IGirder,pGirder);

   CComPtr<iPointDisplayObject> doPnt;
   ::CoCreateInstance(CLSID_PointDisplayObject,NULL,CLSCTX_ALL,IID_iPointDisplayObject,(void**)&doPnt);
   doPnt->SetID(1);

   CComPtr<iShapeDrawStrategy> strategy;
   ::CoCreateInstance(CLSID_ShapeDrawStrategy,NULL,CLSCTX_ALL,IID_iShapeDrawStrategy,(void**)&strategy);
   doPnt->SetDrawingStrategy(strategy);

   CComPtr<IShape> shape;
   pSectProp->GetGirderShape(poi,pgsTypes::CastingYard,false,&shape);

   // Girder shape is positioned at its real coordiantes in the cross section
   // The strands (see BuildStrandDisplayObjects) are position relative to the bottom
   // center of the girder.
   // Move the girder so that it is in the same coordinate system as the strands.
   
   CComPtr<IPoint2d> point;
   point.CoCreateInstance(__uuidof(Point2d));
   point->Move(0,0);
   CComQIPtr<IXYPosition> position(shape);
   position->put_LocatorPoint(lpBottomCenter,point);

   strategy->SetShape(shape);
   strategy->SetSolidLineColor(GIRDER_BORDER_COLOR);
   strategy->SetSolidFillColor(GIRDER_FILL_COLOR);
   strategy->SetVoidLineColor(VOID_BORDER_COLOR);
   strategy->SetVoidFillColor(GetSysColor(COLOR_WINDOW));
   strategy->DoFill(true);

   // Set up sockets so dimension lines can plug into the girder shape
   CComPtr<IRect2d> box;
   shape->get_BoundingBox(&box);
   CComPtr<IPoint2d> pntTC, pntBC; // top and bottom center
   Float64 top_width = pGirder->GetTopWidth(poi);
   Float64 bottom_width = pGirder->GetBottomWidth(poi);

   CComPtr<iSocket> socketHT, socketHB, socketTFL, socketTFR, socketBFL, socketBFR, socketBC;
   CComQIPtr<iConnectable> connectable(doPnt);

   // sockets for top flange dimension line
   box->get_TopCenter(&pntTC);
   pntTC->Offset(-top_width/2,0);
   connectable->AddSocket(SOCKET_TFL,pntTC,&socketTFL);
   pntTC->Offset(top_width,0);
   connectable->AddSocket(SOCKET_TFR,pntTC,&socketTFR);

   // sockets for bottom flange dimension line
   box->get_BottomCenter(&pntBC);
   pntBC->Offset(-bottom_width/2,0);
   connectable->AddSocket(SOCKET_BFL,pntBC,&socketBFL);
   pntBC->Offset(bottom_width,0);
   connectable->AddSocket(SOCKET_BFR,pntBC,&socketBFR);

   // sockets for height dimension line
   pntTC.Release();
   pntBC.Release();
   box->get_TopCenter(&pntTC);
   box->get_BottomCenter(&pntBC);
   Float64 dx = -_cpp_max(top_width,bottom_width)/2.0;
   pntTC->Offset(dx,0);
   pntBC->Offset(dx,0);
   connectable->AddSocket(SOCKET_HT,pntTC,&socketHT);
   connectable->AddSocket(SOCKET_HB,pntBC,&socketHB);

   // sockets for center of prestressing
   pntBC.Release();
   box->get_BottomCenter(&pntBC);
   connectable->AddSocket(SOCKET_BC,pntBC,&socketBC);

   pDL->AddDisplayObject(doPnt);
}

void CGirderModelSectionView::BuildStrandDisplayObjects(CPGSuperDoc* pDoc,IBroker* pBroker,SpanIndexType span,GirderIndexType girder,iDisplayMgr* pDispMgr)
{
   pgsPointOfInterest poi(span,girder,m_pFrame->GetCurrentCutLocation());

   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeom);


   GET_IFACE2(pBroker,IBridgeMaterial,pBridgeMaterial);
   const matPsStrand* pStrand = pBridgeMaterial->GetStrand(span,girder,pgsTypes::Straight);
   Float64 diameter = pStrand->GetNominalDiameter();

   CComPtr<iSimpleDrawPointStrategy> strategy;
   ::CoCreateInstance(CLSID_SimpleDrawPointStrategy,NULL,CLSCTX_ALL,IID_iSimpleDrawPointStrategy,(void**)&strategy);
   strategy->SetColor(STRAND_FILL_COLOR);
   strategy->SetPointType(ptCircle);
   strategy->SetPointSize(diameter);

   CComPtr<iSimpleDrawPointStrategy> debond_strategy;
   ::CoCreateInstance(CLSID_SimpleDrawPointStrategy,NULL,CLSCTX_ALL,IID_iSimpleDrawPointStrategy,(void**)&debond_strategy);
   debond_strategy->SetColor(DEBOND_FILL_COLOR);
   debond_strategy->SetPointType(ptCircle);
   debond_strategy->SetPointSize(diameter);

   // Straight strands
   CComPtr<iDisplayList> pStraightDL;
   pDispMgr->FindDisplayList(STRAIGHT_STRAND_LIST,&pStraightDL);
   ATLASSERT(pStraightDL);
   pStraightDL->Clear();

   CComPtr<IPoint2dCollection> points;
   pStrandGeom->GetStrandPositions(poi, pgsTypes::Straight,&points);
   CollectionIndexType nStrandPoints;
   points->get_Count(&nStrandPoints);
   CollectionIndexType strandPointIdx;
   for ( strandPointIdx = 0; strandPointIdx < nStrandPoints; strandPointIdx++ )
   {
      CComPtr<IPoint2d> p;
      points->get_Item(strandPointIdx,&p);

      CComPtr<iPointDisplayObject> doPnt;
      ::CoCreateInstance(CLSID_PointDisplayObject,NULL,CLSCTX_ALL,IID_iPointDisplayObject,(void**)&doPnt);
      doPnt->SetPosition(p,FALSE,FALSE);
      doPnt->SetID(strandPointIdx);

      if ( pStrandGeom->IsStrandDebonded(poi,strandPointIdx,pgsTypes::Straight) )
         doPnt->SetDrawingStrategy(debond_strategy);
      else
         doPnt->SetDrawingStrategy(strategy);

      doPnt->SetSelectionType(stAll);

      pStraightDL->AddDisplayObject(doPnt);
   }

   // Harped Strands
   CComPtr<iDisplayList> pHarpedDL;
   pDispMgr->FindDisplayList(HARPED_STRAND_LIST,&pHarpedDL);
   ATLASSERT(pHarpedDL);
   pHarpedDL->Clear();

   pStrand = pBridgeMaterial->GetStrand(span,girder,pgsTypes::Harped);
   diameter = pStrand->GetNominalDiameter();

   points.Release();
   pStrandGeom->GetStrandPositions(poi, pgsTypes::Harped,&points);
   points->get_Count(&nStrandPoints);
   for ( strandPointIdx = 0; strandPointIdx < nStrandPoints; strandPointIdx++ )
   {
      CComPtr<IPoint2d> p;
      points->get_Item(strandPointIdx,&p);

      CComPtr<iPointDisplayObject> doPnt;
      ::CoCreateInstance(CLSID_PointDisplayObject,NULL,CLSCTX_ALL,IID_iPointDisplayObject,(void**)&doPnt);
      doPnt->SetPosition(p,FALSE,FALSE);
      doPnt->SetID(strandPointIdx);
      doPnt->SetDrawingStrategy(strategy);

      pHarpedDL->AddDisplayObject(doPnt);
   }

   // Temporary Strands
   CComPtr<iDisplayList> pTempDL;
   pDispMgr->FindDisplayList(TEMP_STRAND_LIST,&pTempDL);
   ATLASSERT(pTempDL);
   pTempDL->Clear();

   pStrand = pBridgeMaterial->GetStrand(span,girder,pgsTypes::Temporary);
   diameter = pStrand->GetNominalDiameter();

   points.Release();
   pStrandGeom->GetStrandPositions(poi, pgsTypes::Temporary,&points);
   points->get_Count(&nStrandPoints);
   for ( strandPointIdx = 0; strandPointIdx < nStrandPoints; strandPointIdx++ )
   {
      CComPtr<IPoint2d> p;
      points->get_Item(strandPointIdx,&p);

      CComPtr<iPointDisplayObject> doPnt;
      ::CoCreateInstance(CLSID_PointDisplayObject,NULL,CLSCTX_ALL,IID_iPointDisplayObject,(void**)&doPnt);
      doPnt->SetPosition(p,FALSE,FALSE);
      doPnt->SetID(strandPointIdx);
      doPnt->SetDrawingStrategy(strategy);

      pTempDL->AddDisplayObject(doPnt);
   }
}

void CGirderModelSectionView::BuildLongReinfDisplayObjects(CPGSuperDoc* pDoc,IBroker* pBroker,SpanIndexType span,GirderIndexType girder,iDisplayMgr* pDispMgr)
{
   CComPtr<iDisplayList> pDL;
   pDispMgr->FindDisplayList(LONG_REINF_LIST,&pDL);
   ATLASSERT(pDL);
   pDL->Clear();

   pgsPointOfInterest poi(span,girder,m_pFrame->GetCurrentCutLocation());

   CComPtr<iSimpleDrawPointStrategy> strategy;
   ::CoCreateInstance(CLSID_SimpleDrawPointStrategy,NULL,CLSCTX_ALL,IID_iSimpleDrawPointStrategy,(void**)&strategy);
   strategy->SetColor(REBAR_COLOR);
   strategy->SetPointType(ptCircle);

   GET_IFACE2(pBroker,ILongRebarGeometry,pLongRebarGeom);

   CComPtr<IRebarSection> rebar_section;
   pLongRebarGeom->GetRebars(poi,&rebar_section);

   CComPtr<IEnumRebarSectionItem> enum_items;
   rebar_section->get__EnumRebarSectionItem(&enum_items);

   long id = 0;

   CComPtr<IRebarSectionItem> item;
   while ( enum_items->Next(1,&item,NULL) != S_FALSE )
   {
      CComPtr<IPoint2d> location;
      item->get_Location(&location);

      Float64 x, y;
      location->get_X(&x);
      location->get_Y(&y);

      item.Release();
      CComPtr<IPoint2d> p;
      p.CoCreateInstance(CLSID_Point2d);
      p->Move(x,y);

      CComPtr<iPointDisplayObject> doPnt;
      ::CoCreateInstance(CLSID_PointDisplayObject,NULL,CLSCTX_ALL,IID_iPointDisplayObject,(void**)&doPnt);
      doPnt->SetPosition(p,FALSE,FALSE);
      doPnt->SetID(id++);
      doPnt->SetDrawingStrategy(strategy);

      pDL->AddDisplayObject(doPnt);
   }
}

void CGirderModelSectionView::BuildCGDisplayObjects(CPGSuperDoc* pDoc,IBroker* pBroker,SpanIndexType span,GirderIndexType girder,iDisplayMgr* pDispMgr)
{
   CComPtr<iDisplayList> pDL;
   pDispMgr->FindDisplayList(CG_LIST,&pDL);
   ATLASSERT(pDL);
   pDL->Clear();

   pgsPointOfInterest poi(span,girder,m_pFrame->GetCurrentCutLocation());

   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeom);
   Float64 nEff;
   Float64 ecc = pStrandGeom->GetEccentricity(poi,true, &nEff);

   GET_IFACE2(pBroker,ISectProp2,pSectProp);
   Float64 Yb = pSectProp->GetYb(pgsTypes::CastingYard,poi);

   CComPtr<IPoint2d> point;
   point.CoCreateInstance(__uuidof(Point2d));
   point->Move(0,Yb - ecc);

   CComPtr<iPointDisplayObject> doPnt;
   ::CoCreateInstance(CLSID_PointDisplayObject,NULL,CLSCTX_ALL,IID_iPointDisplayObject,(void**)&doPnt);
   doPnt->SetID(1);
   doPnt->SetPosition(point,FALSE,FALSE);

   CComPtr<iTargetDrawStrategy> strategy;
   ::CoCreateInstance(CLSID_TargetDrawStrategy,NULL,CLSCTX_ALL,IID_iTargetDrawStrategy,(void**)&strategy);
   CRect rc;
   GetClientRect(&rc);
   strategy->SetRadius(rc.Width()/80);

   doPnt->SetDrawingStrategy(strategy);

   // setup socket for dimension line
   CComQIPtr<iConnectable> connectable(doPnt);

   // sockets for top flange dimension line
   CComPtr<iSocket> socketCGPS;
   connectable->AddSocket(SOCKET_CGPS,point,&socketCGPS);

   pDL->AddDisplayObject(doPnt);
}

void CGirderModelSectionView::BuildDimensionDisplayObjects(CPGSuperDoc* pDoc,IBroker* pBroker,SpanIndexType span,GirderIndexType girder,iDisplayMgr* pDispMgr)
{
   CComPtr<iDisplayList> pDL;
   pDispMgr->FindDisplayList(DIMENSION_LIST,&pDL);
   ATLASSERT(pDL);
   pDL->Clear();

   CComPtr<iDimensionLine> doDimLineTopFlangeWidth;
   CComPtr<iDimensionLine> doDimLineBottomFlangeWidth;
   CComPtr<iDimensionLine> doDimLineHeight;
   CComPtr<iDimensionLine> doDimLineCGPS;

   ::CoCreateInstance(CLSID_DimensionLineDisplayObject,NULL,CLSCTX_ALL,IID_iDimensionLine,(void**)&doDimLineTopFlangeWidth);
   ::CoCreateInstance(CLSID_DimensionLineDisplayObject,NULL,CLSCTX_ALL,IID_iDimensionLine,(void**)&doDimLineBottomFlangeWidth);
   ::CoCreateInstance(CLSID_DimensionLineDisplayObject,NULL,CLSCTX_ALL,IID_iDimensionLine,(void**)&doDimLineHeight);
   ::CoCreateInstance(CLSID_DimensionLineDisplayObject,NULL,CLSCTX_ALL,IID_iDimensionLine,(void**)&doDimLineCGPS);

   doDimLineTopFlangeWidth->SetID(0);
   doDimLineBottomFlangeWidth->SetID(1);
   doDimLineHeight->SetID(2);
   doDimLineCGPS->SetID(3);

   // Connect the dimension lines to the sockets in the section display object
   CComPtr<iDisplayList> pSectionDL;
   pDispMgr->FindDisplayList(SECTION_LIST,&pSectionDL);
   ATLASSERT(pSectionDL);

   // get the girder section display object
   CComPtr<iDisplayObject> doSection;
   pSectionDL->GetDisplayObject(0,&doSection);

   // get it's iConnectable interface
   CComQIPtr<iConnectable> connectableSection(doSection);

   // get the sockets
   CComPtr<iSocket> socketTFL,socketTFR,socketBFL,socketBFR,socketHT,socketHB,socketBC;
   connectableSection->GetSocket(SOCKET_TFL,  atByID, &socketTFL);
   connectableSection->GetSocket(SOCKET_TFR,  atByID, &socketTFR);
   connectableSection->GetSocket(SOCKET_BFL,  atByID, &socketBFL);
   connectableSection->GetSocket(SOCKET_BFR,  atByID, &socketBFR);
   connectableSection->GetSocket(SOCKET_HT,   atByID, &socketHT);
   connectableSection->GetSocket(SOCKET_HB,   atByID, &socketHB);
   connectableSection->GetSocket(SOCKET_BC,   atByID, &socketBC);

   UINT settings = pDoc->GetGirderEditorSettings();

   CComPtr<iSocket> socketCGPS;
   if (settings & IDG_SV_SHOW_PS_CG)
   {
      CComPtr<iDisplayList> pCGList;
      pDispMgr->FindDisplayList(CG_LIST,&pCGList);

      CComPtr<iDisplayObject> doCGPS;
      pCGList->GetDisplayObject(0,&doCGPS);
      CComQIPtr<iConnectable> connectableCGPS(doCGPS);
      connectableCGPS->GetSocket(SOCKET_CGPS, atByID, &socketCGPS);
   }

   // get the connector interface from the dimension lines
   CComQIPtr<iConnector> connectorTopFlangeWidth(doDimLineTopFlangeWidth);
   CComQIPtr<iConnector> connectorBottomFlangeWidth(doDimLineBottomFlangeWidth);
   CComQIPtr<iConnector> connectorHeight(doDimLineHeight);
   CComQIPtr<iConnector> connectorCGPS(doDimLineCGPS);

   // connect the top flange width dimension line (across the top)
   CComPtr<iPlug> startPlug, endPlug;
   connectorTopFlangeWidth->GetStartPlug(&startPlug);
   connectorTopFlangeWidth->GetEndPlug(&endPlug);
   
   DWORD dwCookie;
   socketTFL->Connect(startPlug,&dwCookie);
   socketTFR->Connect(endPlug,&dwCookie);

   // connect the bottom flange width dimension line (across the bottom)
   startPlug.Release();
   endPlug.Release();
   connectorBottomFlangeWidth->GetStartPlug(&startPlug);
   connectorBottomFlangeWidth->GetEndPlug(&endPlug);
   socketBFR->Connect(startPlug,&dwCookie);
   socketBFL->Connect(endPlug,&dwCookie);

   // connect the height dimension line (left side)
   startPlug.Release();
   endPlug.Release();
   connectorHeight->GetStartPlug(&startPlug);
   connectorHeight->GetEndPlug(&endPlug);

   socketHB->Connect(startPlug,&dwCookie);
   socketHT->Connect(endPlug,&dwCookie);

   // connect cg ps dimension line (bottom center to cg symbol)
   if (settings & IDG_SV_SHOW_PS_CG)
   {
      startPlug.Release();
      endPlug.Release();
      connectorCGPS->GetStartPlug(&startPlug);
      connectorCGPS->GetEndPlug(&endPlug);

      socketBC->Connect(startPlug,&dwCookie);
      socketCGPS->Connect(endPlug,&dwCookie);
   }

   // set the text labels on the dimension lines
   pgsPointOfInterest poi(span,girder,m_pFrame->GetCurrentCutLocation());
   
   GET_IFACE2(pBroker,IGirder,pGirder);
   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeometry);
   GET_IFACE2(pBroker,ISectProp2,pSectProp);

   Float64 top_width = pGirder->GetTopWidth(poi);
   Float64 bottom_width = pGirder->GetBottomWidth(poi);
   Float64 height = pGirder->GetHeight(poi);
   Float64 nEff;
   Float64 ecc = pStrandGeometry->GetEccentricity(poi,true, &nEff);
   Float64 yps = pSectProp->GetYb(pgsTypes::CastingYard,poi) - ecc;

   CString strDim;
   CComPtr<iTextBlock> textBlock;

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   const unitmgtLengthData& length_unit = pDisplayUnits->GetComponentDimUnit();

   textBlock.CoCreateInstance(CLSID_TextBlock);
   strDim = FormatDimension(top_width,length_unit);
   textBlock->SetText(strDim);
   doDimLineTopFlangeWidth->SetTextBlock(textBlock);

   textBlock.Release();
   textBlock.CoCreateInstance(CLSID_TextBlock);
   strDim = FormatDimension(bottom_width,length_unit);
   textBlock->SetText(strDim);
   doDimLineBottomFlangeWidth->SetTextBlock(textBlock);

   textBlock.Release();
   textBlock.CoCreateInstance(CLSID_TextBlock);
   strDim = FormatDimension(height,length_unit);
   textBlock->SetText(strDim);
   doDimLineHeight->SetTextBlock(textBlock);

   if (settings & IDG_SV_SHOW_PS_CG)
   {
      textBlock.Release();
      textBlock.CoCreateInstance(CLSID_TextBlock);
      strDim = FormatDimension(yps,length_unit);
      textBlock->SetText(strDim);
      doDimLineCGPS->SetTextBlock(textBlock);

      // adjust the witness lines on the vertical dimensions
      long tx0,ty0;
      long tx1,ty1;
      m_pCoordinateMap->WPtoTP(0,0,&tx0,&ty0);
      m_pCoordinateMap->WPtoTP(bottom_width/2.0,0,&tx1,&ty1);
      long dimwit = doDimLineHeight->GetWitnessLength();
      doDimLineCGPS->SetWitnessLength(tx1-tx0+dimwit); 
      doDimLineHeight->SetWitnessLength(dimwit*2);
   }

   // add the dimension line display objects to the display list
   pDL->AddDisplayObject(doDimLineTopFlangeWidth);
   pDL->AddDisplayObject(doDimLineBottomFlangeWidth);
   pDL->AddDisplayObject(doDimLineHeight);

   if (settings & IDG_SV_SHOW_PS_CG)
      pDL->AddDisplayObject(doDimLineCGPS);
}

void CGirderModelSectionView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{

   m_bUpdateError = false;

   CDisplayView::OnUpdate(pSender,lHint,pHint);

   // Let our frame deal with updates as well
   m_pFrame->OnUpdate(this,lHint,pHint);

   if ( lHint == 0 ||
        lHint == HINT_GIRDERVIEWSETTINGSCHANGED ||
        lHint == HINT_GIRDERVIEWSECTIONCUTCHANGED ||
        lHint == HINT_UNITSCHANGED || 
        lHint == HINT_BRIDGECHANGED ||
        lHint == HINT_GIRDERFAMILYCHANGED ||
        lHint == HINT_GIRDERCHANGED || 
        lHint == HINT_SELECTIONCHANGED )
   {

      if (!DidGirderSelectionChange() && lHint == HINT_SELECTIONCHANGED)
      {
         // don't bother updating if selection hint and our girder selection didn't change
         return;
      }

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
}

int CGirderModelSectionView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CDisplayView::OnCreate(lpCreateStruct) == -1)
		return -1;

   m_pFrame = (CGirderModelChildFrame*)GetParent()->GetParent();
   ASSERT( m_pFrame != 0 );
   ASSERT( m_pFrame->IsKindOf( RUNTIME_CLASS( CGirderModelChildFrame ) ) );

	return 0;
}

void CGirderModelSectionView::OnEditGirder() 
{
   ((CPGSuperDoc*)GetDocument())->EditGirderDescription(m_CurrentSpanIdx,m_CurrentGirderIdx,EBD_GENERAL);
}

void CGirderModelSectionView::OnEditPrestressing() 
{
   ((CPGSuperDoc*)GetDocument())->EditGirderDescription(m_CurrentSpanIdx,m_CurrentGirderIdx,EGD_PRESTRESSING);
}

void CGirderModelSectionView::OnViewSettings() 
{
	((CPGSuperDoc*)GetDocument())->EditGirderViewSettings(VS_GIRDER_SECTION);
}

void CGirderModelSectionView::OnEditStirrups() 
{
   ((CPGSuperDoc*)GetDocument())->EditGirderDescription(m_CurrentSpanIdx,m_CurrentGirderIdx,EGD_STIRRUPS);
}

void CGirderModelSectionView::OnLeftEnd() 
{
   m_pFrame->CutAtLeftEnd();
}

void CGirderModelSectionView::OnLeftHp() 
{
   m_pFrame->CutAtLeftHp();
}

void CGirderModelSectionView::OnCenter() 
{
   m_pFrame->CutAtCenter();
}

void CGirderModelSectionView::OnRightHp() 
{
   m_pFrame->CutAtRightHp();
}

void CGirderModelSectionView::OnRightEnd() 
{
   m_pFrame->CutAtRightEnd();
}

void CGirderModelSectionView::OnUserCut() 
{
	m_pFrame->CutAtLocation();
}

void CGirderModelSectionView::OnSize(UINT nType, int cx, int cy) 
{
	CDisplayView::OnSize(nType, cx, cy);

   CRect rect;
   GetClientRect(&rect);
   rect.DeflateRect(5,5,5,5);

   CSize size = rect.Size();
   size.cx = max(0,size.cx);
   size.cy = max(0,size.cy);

   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   SetLogicalViewRect(MM_TEXT,rect);

   SetScrollSizes(MM_TEXT,size,CScrollView::sizeDefault,CScrollView::sizeDefault);

   ScaleToFit();
}

void CGirderModelSectionView::OnDraw(CDC* pDC) 
{
   if ( m_bUpdateError )
   {
      AFX_MANAGE_STATE(AfxGetStaticModuleState());

      CString msg;
      AfxFormatString1(msg,IDS_E_UPDATE,m_ErrorMsg.c_str());

      CFont font;
      CFont* pOldFont = NULL;
      if ( font.CreatePointFont(100,_T("Arial"),pDC) )
         pOldFont = pDC->SelectObject(&font);

      MultiLineTextOut(pDC,0,0,msg);

      if ( pOldFont )
         pDC->SelectObject(pOldFont);

      return;
   }


   if (  m_CurrentSpanIdx != ALL_SPANS && m_CurrentGirderIdx != ALL_GIRDERS  )
   {
      CDisplayView::OnDraw(pDC);
   }
   else
   {
      CString msg("Select a girder to display");
      CFont font;
      CFont* pOldFont = NULL;
      if ( font.CreatePointFont(100,_T("Arial"),pDC) )
         pOldFont = pDC->SelectObject(&font);

      MultiLineTextOut(pDC,0,0,msg);

      if ( pOldFont )
         pDC->SelectObject(pOldFont);
   }
}

bool CGirderModelSectionView::DidGirderSelectionChange()
{
   SpanIndexType span;
   GirderIndexType gdr;

   m_pFrame->GetSpanAndGirderSelection(&span,&gdr);

   if (m_CurrentSpanIdx!=span || m_CurrentGirderIdx!=gdr)
   {
      m_CurrentSpanIdx=span;
      m_CurrentGirderIdx=gdr;
      return true;
   }

   return false;
}
