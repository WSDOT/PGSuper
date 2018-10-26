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

// GirderModelSectionView.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\resource.h"
#include "PGSuperAppPlugin\PGSuperApp.h"
#include "PGSuperDocBase.h"
#include "PGSuperDoc.h"
#include "PGSpliceDoc.h"
#include "PGSuperUnits.h"
#include "PGSuperColors.h"
#include "GirderModelSectionView.h"
#include "GirderModelChildFrame.h"
#include "GMDisplayMgrEventsImpl.h"
#include "DisplayObjectFactory.h"
#include <IFace\Bridge.h>
#include <IFace\DrawBridgeSettings.h>
#include <IFace\Intervals.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\EditByUI.h>

#include <WBFLGenericBridgeTools.h>
#include <WBFLDManip.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define SOCKET_TL   0 // top left (top left of girder or slab)
#define SOCKET_TC   1 // top center (on CL of non-composite girder)
#define SOCKET_TR   2 // top right (top right of girder or slab
#define SOCKET_BL   3 // bottom left of girder bottom flange
#define SOCKET_BC   4 // bottom center of girder
#define SOCKET_BR   5 // bottom right of girder bottom flange
#define SOCKET_CGPS 6 // CG of prestress force

#define SECTION_LIST             1
#define STRAIGHT_STRAND_LIST     2
#define HARPED_STRAND_LIST       3
#define TEMP_STRAND_LIST         4
#define DUCT_LIST                5
#define DIMENSION_LIST           6
#define CG_LIST                  7
#define LONG_REINF_LIST          8

/////////////////////////////////////////////////////////////////////////////
// CGirderModelSectionView

IMPLEMENT_DYNCREATE(CGirderModelSectionView, CDisplayView)

CGirderModelSectionView::CGirderModelSectionView() :
m_GirderKey(0,0),
m_bOnIntialUpdateComplete(false)
{
   m_bUpdateError = false;
}

CGirderModelSectionView::~CGirderModelSectionView()
{
}


BEGIN_MESSAGE_MAP(CGirderModelSectionView, CDisplayView)
	//{{AFX_MSG_MAP(CGirderModelSectionView)
	ON_WM_CREATE()
	ON_COMMAND(ID_EDIT_PRESTRESSING, OnEditPrestressing)
	ON_COMMAND(ID_VIEWSETTINGS, OnViewSettings)
	ON_COMMAND(ID_EDIT_STIRRUPS, OnEditStirrups)
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
   ATLASSERT(m_bOnIntialUpdateComplete == false);
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

   CPGSDocBase* pDoc = (CPGSDocBase*)GetDocument();

   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);
//   dispMgr->EnableLBtnSelectRect(TRUE);
//   dispMgr->EnableLBtnMultiSelect(TRUE,MK_SHIFT);
   dispMgr->SetSelectionLineColor(SELECTED_OBJECT_LINE_COLOR);
   dispMgr->SetSelectionFillColor(SELECTED_OBJECT_FILL_COLOR);

   CDisplayObjectFactory* pFactory = new CDisplayObjectFactory(pDoc);
   CComPtr<iDisplayObjectFactory> factory;
   factory.Attach((iDisplayObjectFactory*)pFactory->GetInterface(&IID_iDisplayObjectFactory));
   dispMgr->AddDisplayObjectFactory(factory);

   // set up default event handler for canvas
   CGMDisplayMgrEventsImpl* pEvents = new CGMDisplayMgrEventsImpl(pDoc, m_pFrame, this, false);
   CComPtr<iDisplayMgrEvents> events;
   events.Attach((iDisplayMgrEvents*)pEvents->GetInterface(&IID_iDisplayMgrEvents));
   dispMgr->RegisterEventSink(events);

   CSelection selection = pDoc->GetSelection();
   if ( selection.Type != CSelection::Girder )
   {
      pDoc->SelectGirder(m_GirderKey);
   }

   m_bOnIntialUpdateComplete = true;
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

   CComPtr<iDisplayList> duct_list;
   ::CoCreateInstance(CLSID_DisplayList,NULL,CLSCTX_ALL,IID_iDisplayList,(void**)&duct_list);
   duct_list->SetID(DUCT_LIST);
   dispMgr->AddDisplayList(duct_list);

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

   pgsPointOfInterest poi(m_pFrame->GetCutLocation());

   if ( poi.GetSegmentKey().girderIndex == INVALID_INDEX )
   {
      return;
   }

   CPGSDocBase* pDoc = (CPGSDocBase*)GetDocument();
   UINT settings = pDoc->GetGirderEditorSettings();

   // Grab hold of the broker so we can pass it as a parameter
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);


   BuildSectionDisplayObjects(pDoc, pBroker, poi, dispMgr);

   if ( settings & IDG_SV_SHOW_STRANDS )
   {
      BuildStrandDisplayObjects(pDoc, pBroker, poi, dispMgr);
      BuildDuctDisplayObjects(pDoc, pBroker, poi, dispMgr);
   }

   if ( settings & IDG_SV_SHOW_LONG_REINF )
   {
      BuildLongReinfDisplayObjects(pDoc, pBroker, poi, dispMgr);
   }

   if (settings & IDG_SV_SHOW_PS_CG)
   {
      BuildCGDisplayObjects(pDoc, pBroker, poi, dispMgr);
   }

   if ( settings & IDG_SV_SHOW_DIMENSIONS )
   {
      BuildDimensionDisplayObjects(pDoc, pBroker, poi, dispMgr);
   }

   SetMappingMode(DManip::Isotropic);
}

void CGirderModelSectionView::BuildSectionDisplayObjects(CPGSDocBase* pDoc,IBroker* pBroker,const pgsPointOfInterest& poi,iDisplayMgr* pDispMgr)
{
   CComPtr<iDisplayList> pDL;
   pDispMgr->FindDisplayList(SECTION_LIST,&pDL);
   ATLASSERT(pDL);
   pDL->Clear();

   const CSegmentKey& segmentKey(poi.GetSegmentKey());

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   GET_IFACE2(pBroker,IShapes,pShapes);
   GET_IFACE2(pBroker,IGirder,pGirder);
   GET_IFACE2(pBroker,IIntervals,pIntervals);

   EventIndexType eventIdx = m_pFrame->GetEvent();
   EventIndexType castDeckEventIdx = pIBridgeDesc->GetCastDeckEventIndex();
   IntervalIndexType intervalIdx = pIntervals->GetInterval(eventIdx);

   CComPtr<iPointDisplayObject> doPnt;
   ::CoCreateInstance(CLSID_PointDisplayObject,NULL,CLSCTX_ALL,IID_iPointDisplayObject,(void**)&doPnt);
   doPnt->SetID(1);

   CComPtr<iShapeDrawStrategy> strategy;
   ::CoCreateInstance(CLSID_ShapeDrawStrategy,NULL,CLSCTX_ALL,IID_iShapeDrawStrategy,(void**)&strategy);
   doPnt->SetDrawingStrategy(strategy);

   // Get the shape in Girder Section Coordinates so that it is in the same coordinate system
   // as the items internal to the section (strand, rebar, etc.) (0,0 is at top center of girder)
   CComPtr<IShape> shape;
   pShapes->GetSegmentShape(intervalIdx,poi,false/*don't orient... shape is always plumb*/,pgsTypes::scGirder,&shape);
   strategy->SetShape(shape);
   strategy->SetSolidLineColor(SEGMENT_BORDER_COLOR);
   strategy->SetSolidFillColor(SEGMENT_FILL_COLOR);
   strategy->SetVoidLineColor(VOID_BORDER_COLOR);
   strategy->SetVoidFillColor(GetSysColor(COLOR_WINDOW));
   strategy->DoFill(true);

   // Set up sockets so dimension lines can plug into the girder shape
   CComPtr<IRect2d> boxGirder, boxSlab;
   CComPtr<IPoint2d> pntTC, pntBC; // top and bottom center
   Float64 top_width = 0;
   if (eventIdx <= castDeckEventIdx)
   {
      top_width = pGirder->GetTopWidth(poi);
      shape->get_BoundingBox(&boxGirder);
      shape->get_BoundingBox(&boxSlab);
   }
   else
   {
      GET_IFACE2(pBroker,ISectionProperties,pSectProp);
      top_width = pSectProp->GetTributaryFlangeWidth(poi);

      CComQIPtr<ICompositeShape> composite(shape);
      IndexType nShapes;
      composite->get_Count(&nShapes);
      CComPtr<ICompositeShapeItem> girderShapeItem;
      composite->get_Item(0,&girderShapeItem); // basic girder is always first in composite
      CComPtr<IShape> girderShape;
      girderShapeItem->get_Shape(&girderShape);
      girderShape->get_BoundingBox(&boxGirder);

      CComPtr<ICompositeShapeItem> slabShapeItem;
      composite->get_Item(nShapes-1,&slabShapeItem); // slab is always last in composite
      CComPtr<IShape> slabShape;
      slabShapeItem->get_Shape(&slabShape);
      slabShape->get_BoundingBox(&boxSlab);
   }

   Float64 bottom_width = pGirder->GetBottomWidth(poi);

   CComPtr<iSocket> socketTL, socketTC, socketTR, socketBL, socketBC, socketBR;
   CComQIPtr<iConnectable> connectable(doPnt);

   // sockets for top flange dimension lines
   boxSlab->get_TopCenter(&pntTC);
   pntTC->Offset(-top_width/2,0);
   connectable->AddSocket(SOCKET_TL,pntTC,&socketTL);
   pntTC->Offset(top_width,0);
   connectable->AddSocket(SOCKET_TR,pntTC,&socketTR);

   Float64 yTop;
   pntTC->get_Y(&yTop);
   pntTC.Release();
   boxGirder->get_TopCenter(&pntTC);
   pntTC->put_Y(yTop);
   connectable->AddSocket(SOCKET_TC,pntTC,&socketTC);

   // sockets for bottom flange dimension line
   boxGirder->get_BottomCenter(&pntBC);
   connectable->AddSocket(SOCKET_BC,pntBC,&socketBC);
   pntBC->Offset(-bottom_width/2,0);
   connectable->AddSocket(SOCKET_BL,pntBC,&socketBL);
   pntBC->Offset(bottom_width,0);
   connectable->AddSocket(SOCKET_BR,pntBC,&socketBR);

   pDL->AddDisplayObject(doPnt);
}

void CGirderModelSectionView::BuildStrandDisplayObjects(CPGSDocBase* pDoc,IBroker* pBroker,const pgsPointOfInterest& poi,iDisplayMgr* pDispMgr)
{
   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeom);

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   // Strands are measured in Girder Section Coordinates
   GET_IFACE2(pBroker,IMaterials,pMaterial);
   const matPsStrand* pStrand = pMaterial->GetStrandMaterial(segmentKey,pgsTypes::Straight);
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
      {
         doPnt->SetDrawingStrategy(debond_strategy);
      }
      else
      {
         doPnt->SetDrawingStrategy(strategy);
      }

      doPnt->SetSelectionType(stAll);

      pStraightDL->AddDisplayObject(doPnt);
   }

   // Harped Strands
   CComPtr<iDisplayList> pHarpedDL;
   pDispMgr->FindDisplayList(HARPED_STRAND_LIST,&pHarpedDL);
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

      CComPtr<iPointDisplayObject> doPnt;
      ::CoCreateInstance(CLSID_PointDisplayObject,NULL,CLSCTX_ALL,IID_iPointDisplayObject,(void**)&doPnt);
      doPnt->SetPosition(p,FALSE,FALSE);
      doPnt->SetID(strandPointIdx);
      doPnt->SetDrawingStrategy(strategy);

      pHarpedDL->AddDisplayObject(doPnt);
   }

   // Temporary Strands
   EventIndexType eventIdx = m_pFrame->GetEvent();
   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType intervalIdx = pIntervals->GetInterval(eventIdx);
   IntervalIndexType tsrIntervalIdx = pIntervals->GetTemporaryStrandRemovalInterval(segmentKey);
   if ( tsrIntervalIdx != INVALID_INDEX && intervalIdx < tsrIntervalIdx )
   {
      // only show temporary strands if they haven't been removed yet
      CComPtr<iDisplayList> pTempDL;
      pDispMgr->FindDisplayList(TEMP_STRAND_LIST,&pTempDL);
      ATLASSERT(pTempDL);
      pTempDL->Clear();

      pStrand = pMaterial->GetStrandMaterial(segmentKey,pgsTypes::Temporary);
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
}

void CGirderModelSectionView::BuildDuctDisplayObjects(CPGSDocBase* pDoc,IBroker* pBroker,const pgsPointOfInterest& poi,iDisplayMgr* pDispMgr)
{
   // The outlines of the ducts are drawn as part of the girder cross section
   // This method adds the text labels to the ducts
   CComPtr<iDisplayList> pDisplayList;
   pDispMgr->FindDisplayList(DUCT_LIST,&pDisplayList);
   ATLASSERT(pDisplayList);
   pDisplayList->Clear();

   const CSegmentKey& segmentKey(poi.GetSegmentKey());
   const CGirderKey& girderKey(segmentKey);

   GET_IFACE2(pBroker,IPointOfInterest,pPoi);
   Float64 Xg = pPoi->ConvertPoiToGirderCoordinate(poi);

#pragma Reminder("REVIEW: assuming tendon starts/ends at start/end face of girder")
   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 Lg = pBridge->GetGirderLength(girderKey);
   if ( Xg < 0 || Lg < Xg )
   {
      return; // poi is not within the extent of the tendon
   }

   GET_IFACE2(pBroker,ITendonGeometry,pTendonGeom);
   DuctIndexType nDucts = pTendonGeom->GetDuctCount(girderKey);
   for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
   {
      StrandIndexType nStrands = pTendonGeom->GetTendonStrandCount(girderKey,ductIdx);
      CString strStrands;
      strStrands.Format(_T("%d"),nStrands);

      CComPtr<IPoint2d> pnt;
      pTendonGeom->GetDuctPoint(girderKey,Xg,ductIdx,&pnt);

      CComPtr<iTextBlock> textBlock;
      ::CoCreateInstance(CLSID_TextBlock,NULL,CLSCTX_ALL,IID_iTextBlock,(void**)&textBlock);

      textBlock->SetText(strStrands);
      textBlock->SetPosition(pnt);
      textBlock->SetTextAlign(TA_BASELINE | TA_CENTER);
      textBlock->SetTextColor(BLACK);
      textBlock->SetBkMode(TRANSPARENT);

      pDisplayList->AddDisplayObject(textBlock);
   }
}

void CGirderModelSectionView::BuildLongReinfDisplayObjects(CPGSDocBase* pDoc,IBroker* pBroker,const pgsPointOfInterest& poi,iDisplayMgr* pDispMgr)
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   CComPtr<iDisplayList> pDL;
   pDispMgr->FindDisplayList(LONG_REINF_LIST,&pDL);
   ATLASSERT(pDL);
   pDL->Clear();

   CComPtr<iSimpleDrawPointStrategy> strategy;
   ::CoCreateInstance(CLSID_SimpleDrawPointStrategy,NULL,CLSCTX_ALL,IID_iSimpleDrawPointStrategy,(void**)&strategy);
   strategy->SetColor(REBAR_COLOR);
   strategy->SetPointType(ptCircle);

   GET_IFACE2(pBroker,ILongRebarGeometry,pLongRebarGeom);

   CComPtr<IRebarSection> rebar_section;
   pLongRebarGeom->GetRebars(poi,&rebar_section);

   CComPtr<IEnumRebarSectionItem> enum_items;
   rebar_section->get__EnumRebarSectionItem(&enum_items);

   IDType id = 0;

   // Bars are measured in Girder Section Coordinates
   CComPtr<IRebarSectionItem> item;
   while ( enum_items->Next(1,&item,NULL) != S_FALSE )
   {
      CComPtr<IPoint2d> p;
      item->get_Location(&p);

      CComPtr<iPointDisplayObject> doPnt;
      ::CoCreateInstance(CLSID_PointDisplayObject,NULL,CLSCTX_ALL,IID_iPointDisplayObject,(void**)&doPnt);
      doPnt->SetPosition(p,FALSE,FALSE);
      doPnt->SetID(id++);
      doPnt->SetDrawingStrategy(strategy);

      pDL->AddDisplayObject(doPnt);

      item.Release();
   }
}

void CGirderModelSectionView::BuildCGDisplayObjects(CPGSDocBase* pDoc,IBroker* pBroker,const pgsPointOfInterest& poi,iDisplayMgr* pDispMgr)
{
   CComPtr<iDisplayList> pDL;
   pDispMgr->FindDisplayList(CG_LIST,&pDL);
   ATLASSERT(pDL);
   pDL->Clear();

   const CSegmentKey& segmentKey(poi.GetSegmentKey());

   // nothing to draw if poi is in a closure joint
   // (poi object won't necessarily have the POI_CLOSURE attribute so just check to see
   // if it is beyond the end of the segment)
   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 segment_length = pBridge->GetSegmentLength(segmentKey);
   if ( segment_length < poi.GetDistFromStart() )
   {
      return;
   }

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   EventIndexType eventIdx = m_pFrame->GetEvent();
   IntervalIndexType intervalIdx = pIntervals->GetInterval(eventIdx);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
   if ( intervalIdx < releaseIntervalIdx )
   {
      intervalIdx = releaseIntervalIdx;
   }

   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeom);
   Float64 nEff;
   Float64 ecc = pStrandGeom->GetEccentricity(intervalIdx, poi,true, &nEff);

   GET_IFACE2(pBroker,ISectionProperties,pSectProp);
   Float64 Yb = pSectProp->GetY(intervalIdx,poi,pgsTypes::BottomGirder);
   Float64 Hg = pSectProp->GetHg(intervalIdx,poi);

   CComPtr<IPoint2d> point;
   point.CoCreateInstance(__uuidof(Point2d));
   point->Move(0,Yb - (Hg+ecc));

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

   CComPtr<iSocket> socketCGPS;
   connectable->AddSocket(SOCKET_CGPS,point,&socketCGPS);

   pDL->AddDisplayObject(doPnt);
}

void CGirderModelSectionView::BuildDimensionDisplayObjects(CPGSDocBase* pDoc,IBroker* pBroker,const pgsPointOfInterest& poi,iDisplayMgr* pDispMgr)
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   GET_IFACE2(pBroker,IGirder,pGirder);
   GET_IFACE2(pBroker,ISectionProperties,pSectProp);
   GET_IFACE2(pBroker,IIntervals,pIntervals);

   EventIndexType eventIdx = m_pFrame->GetEvent();
   EventIndexType castDeckEventIdx = pIBridgeDesc->GetCastDeckEventIndex();

   // need to layout dimension line witness lines in twips
   const long twip_offset = 1440/2;

   CComPtr<iDisplayList> pDL;
   pDispMgr->FindDisplayList(DIMENSION_LIST,&pDL);
   ATLASSERT(pDL);
   pDL->Clear();

   CComPtr<iDimensionLine> doDimLineTopFlangeWidth;
   CComPtr<iDimensionLine> doDimLineBottomFlangeWidth;
   CComPtr<iDimensionLine> doDimLineHeight;
   CComPtr<iDimensionLine> doDimLineCGPS;
   CComPtr<iDimensionLine> doDimLineLeftOverhang;
   CComPtr<iDimensionLine> doDimLineRightOverhang;

   ::CoCreateInstance(CLSID_DimensionLineDisplayObject,NULL,CLSCTX_ALL,IID_iDimensionLine,(void**)&doDimLineTopFlangeWidth);
   ::CoCreateInstance(CLSID_DimensionLineDisplayObject,NULL,CLSCTX_ALL,IID_iDimensionLine,(void**)&doDimLineBottomFlangeWidth);
   ::CoCreateInstance(CLSID_DimensionLineDisplayObject,NULL,CLSCTX_ALL,IID_iDimensionLine,(void**)&doDimLineHeight);
   ::CoCreateInstance(CLSID_DimensionLineDisplayObject,NULL,CLSCTX_ALL,IID_iDimensionLine,(void**)&doDimLineCGPS);
   ::CoCreateInstance(CLSID_DimensionLineDisplayObject,NULL,CLSCTX_ALL,IID_iDimensionLine,(void**)&doDimLineLeftOverhang);
   ::CoCreateInstance(CLSID_DimensionLineDisplayObject,NULL,CLSCTX_ALL,IID_iDimensionLine,(void**)&doDimLineRightOverhang);

   doDimLineTopFlangeWidth->SetID(0);
   doDimLineBottomFlangeWidth->SetID(1);
   doDimLineHeight->SetID(2);
   doDimLineCGPS->SetID(3);
   doDimLineLeftOverhang->SetID(4);
   doDimLineLeftOverhang->SetID(5);

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
   CComPtr<iSocket> socketTL,socketTC,socketTR,socketBL,socketBC,socketBR;
   connectableSection->GetSocket(SOCKET_TL,  atByID, &socketTL);
   connectableSection->GetSocket(SOCKET_TC,  atByID, &socketTC);
   connectableSection->GetSocket(SOCKET_TR,  atByID, &socketTR);
   connectableSection->GetSocket(SOCKET_BL,  atByID, &socketBL);
   connectableSection->GetSocket(SOCKET_BC,  atByID, &socketBC);
   connectableSection->GetSocket(SOCKET_BR,  atByID, &socketBR);

   UINT settings = pDoc->GetGirderEditorSettings();

   CComPtr<iSocket> socketCGPS;
   if (settings & IDG_SV_SHOW_PS_CG)
   {
      CComPtr<iDisplayList> pCGList;
      pDispMgr->FindDisplayList(CG_LIST,&pCGList);

      CComPtr<iDisplayObject> doCGPS;
      pCGList->GetDisplayObject(0,&doCGPS);
      if ( doCGPS )
      {
         CComQIPtr<iConnectable> connectableCGPS(doCGPS);
         connectableCGPS->GetSocket(SOCKET_CGPS, atByID, &socketCGPS);
      }
   }

   // get the connector interface from the dimension lines
   CComQIPtr<iConnector> connectorTopFlangeWidth(doDimLineTopFlangeWidth);
   CComQIPtr<iConnector> connectorBottomFlangeWidth(doDimLineBottomFlangeWidth);
   CComQIPtr<iConnector> connectorHeight(doDimLineHeight);
   CComQIPtr<iConnector> connectorCGPS(doDimLineCGPS);
   CComQIPtr<iConnector> connectorLeftOverhang(doDimLineLeftOverhang);
   CComQIPtr<iConnector> connectorRightOverhang(doDimLineRightOverhang);

   // connect the top flange width dimension line (across the top)
   CComPtr<iPlug> startPlug, endPlug;
   connectorTopFlangeWidth->GetStartPlug(&startPlug);
   connectorTopFlangeWidth->GetEndPlug(&endPlug);
   
   DWORD dwCookie;
   socketTL->Connect(startPlug,&dwCookie);
   socketTR->Connect(endPlug,&dwCookie);

   // connect the overhang dimension lines
   if ( castDeckEventIdx < eventIdx )
   {
      startPlug.Release();
      endPlug.Release();
      connectorLeftOverhang->GetStartPlug(&startPlug);
      connectorLeftOverhang->GetEndPlug(&endPlug);
      socketTL->Connect(startPlug,&dwCookie);
      socketTC->Connect(endPlug,&dwCookie);

      startPlug.Release();
      endPlug.Release();
      connectorRightOverhang->GetStartPlug(&startPlug);
      connectorRightOverhang->GetEndPlug(&endPlug);
      socketTC->Connect(startPlug,&dwCookie);
      socketTR->Connect(endPlug,&dwCookie);
   }

   // connect the bottom flange width dimension line (across the bottom)
   startPlug.Release();
   endPlug.Release();
   connectorBottomFlangeWidth->GetStartPlug(&startPlug);
   connectorBottomFlangeWidth->GetEndPlug(&endPlug);
   socketBR->Connect(startPlug,&dwCookie);
   socketBL->Connect(endPlug,&dwCookie);


   // connect the height dimension line (left side)
   startPlug.Release();
   endPlug.Release();
   connectorHeight->GetStartPlug(&startPlug);
   connectorHeight->GetEndPlug(&endPlug);

   socketBC->Connect(startPlug,&dwCookie);
   socketTC->Connect(endPlug,&dwCookie);

   // connect cg ps dimension line (bottom center to cg symbol)
   if ( (settings & IDG_SV_SHOW_PS_CG) && socketCGPS)
   {
      startPlug.Release();
      endPlug.Release();
      connectorCGPS->GetStartPlug(&startPlug);
      connectorCGPS->GetEndPlug(&endPlug);

      socketBC->Connect(startPlug,&dwCookie);
      socketCGPS->Connect(endPlug,&dwCookie);
   }

   // set the text labels on the dimension lines
   IntervalIndexType intervalIdx = Max(pIntervals->GetInterval(eventIdx), pIntervals->GetPrestressReleaseInterval(segmentKey) );

   Float64 twLeft, twRight;
   Float64 top_width = (eventIdx <= castDeckEventIdx ? pGirder->GetTopWidth(poi) : pSectProp->GetTributaryFlangeWidthEx(poi,&twLeft,&twRight));
   Float64 bottom_width = pGirder->GetBottomWidth(poi);
   Float64 height       = pSectProp->GetHg(intervalIdx,poi);

   CString strDim;
   CComPtr<iTextBlock> textBlock;

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   const unitmgtLengthData& length_unit = pDisplayUnits->GetComponentDimUnit();

   textBlock.CoCreateInstance(CLSID_TextBlock);
   strDim = FormatDimension(top_width,length_unit);
   textBlock->SetText(strDim);
   doDimLineTopFlangeWidth->SetTextBlock(textBlock);

   if ( castDeckEventIdx < eventIdx )
   {
      textBlock.Release();
      textBlock.CoCreateInstance(CLSID_TextBlock);
      strDim = FormatDimension(twLeft,length_unit);
      textBlock->SetText(strDim);
      doDimLineLeftOverhang->SetTextBlock(textBlock);

      textBlock.Release();
      textBlock.CoCreateInstance(CLSID_TextBlock);
      strDim = FormatDimension(twRight,length_unit);
      textBlock->SetText(strDim);
      doDimLineRightOverhang->SetTextBlock(textBlock);

      // move the top flange width line up
      doDimLineTopFlangeWidth->SetWitnessLength(3*twip_offset/2);
   }

   textBlock.Release();
   textBlock.CoCreateInstance(CLSID_TextBlock);
   strDim = FormatDimension(bottom_width,length_unit);
   textBlock->SetText(strDim);
   doDimLineBottomFlangeWidth->SetTextBlock(textBlock);

   textBlock.Release();
   textBlock.CoCreateInstance(CLSID_TextBlock);
   strDim = FormatDimension(height,length_unit);
   textBlock->SetText(strDim);
   textBlock->SetBkMode(TRANSPARENT);
   doDimLineHeight->SetTextBlock(textBlock);

   // adjust the witness line
   long tx0,ty0;
   long tx1,ty1;
   m_pCoordinateMap->WPtoTP(0,0,&tx0,&ty0);
   m_pCoordinateMap->WPtoTP(top_width/2.0,0,&tx1,&ty1);
   doDimLineHeight->SetWitnessLength(tx1-tx0+twip_offset);

   if ( (settings & IDG_SV_SHOW_PS_CG) && socketCGPS )
   {
      GET_IFACE2(pBroker,IStrandGeometry,pStrandGeometry);

      Float64 nEff;
      Float64 ecc = pStrandGeometry->GetEccentricity(intervalIdx, poi,true,&nEff);
      Float64 yps = pSectProp->GetY(intervalIdx,poi,pgsTypes::BottomGirder) - ecc;

      textBlock.Release();
      textBlock.CoCreateInstance(CLSID_TextBlock);
      strDim = FormatDimension(yps,length_unit);
      textBlock->SetText(strDim);
      doDimLineCGPS->SetTextBlock(textBlock);

      // adjust the witness lines on the vertical dimensions
      m_pCoordinateMap->WPtoTP(bottom_width/2.0,0,&tx1,&ty1);
      doDimLineCGPS->SetWitnessLength(tx1-tx0+twip_offset); 
   }

   // add the dimension line display objects to the display list
   pDL->AddDisplayObject(doDimLineTopFlangeWidth);
   pDL->AddDisplayObject(doDimLineBottomFlangeWidth);
   pDL->AddDisplayObject(doDimLineHeight);

   if ( castDeckEventIdx < eventIdx )
   {
      pDL->AddDisplayObject(doDimLineLeftOverhang);
      pDL->AddDisplayObject(doDimLineRightOverhang);
   }


   if ( (settings & IDG_SV_SHOW_PS_CG) && socketCGPS )
   {
      pDL->AddDisplayObject(doDimLineCGPS);
   }
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
   {
		return -1;
   }

   m_pFrame = (CGirderModelChildFrame*)GetParent()->GetParent();
   ASSERT( m_pFrame != 0 );
   ASSERT( m_pFrame->IsKindOf( RUNTIME_CLASS( CGirderModelChildFrame ) ) );

	return 0;
}

void CGirderModelSectionView::OnEditPrestressing() 
{
   int page = EGS_PRESTRESSING;
   if ( GetDocument()->IsKindOf(RUNTIME_CLASS(CPGSuperDoc)) )
   {
      page = EGD_PRESTRESSING;
   }

   pgsPointOfInterest poi(m_pFrame->GetCutLocation());
   ((CPGSDocBase*)GetDocument())->EditGirderSegmentDescription(poi.GetSegmentKey(),page);
}

void CGirderModelSectionView::OnEditStirrups() 
{
   int page = EGS_STIRRUPS;
   if ( GetDocument()->IsKindOf(RUNTIME_CLASS(CPGSuperDoc)) )
   {
      page = EGD_STIRRUPS;
   }

   pgsPointOfInterest poi(m_pFrame->GetCutLocation());
   ((CPGSDocBase*)GetDocument())->EditGirderSegmentDescription(poi.GetSegmentKey(),page);
}

void CGirderModelSectionView::OnViewSettings() 
{
	((CPGSDocBase*)GetDocument())->EditGirderViewSettings(VS_GIRDER_SECTION);
}

void CGirderModelSectionView::OnUserCut() 
{
	m_pFrame->ShowCutDlg();
}

void CGirderModelSectionView::OnSize(UINT nType, int cx, int cy) 
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

   if ( m_bOnIntialUpdateComplete )
   {
      //
      // The witness lines on the dimensions don't automatically scale so we
      // have to re-create them every time the window size changes

      CDManipClientDC dc2(this);


      // get the display manager
      CComPtr<iDisplayMgr> dispMgr;
      GetDisplayMgr(&dispMgr);

      // Get the POI
      pgsPointOfInterest poi(m_pFrame->GetCutLocation());

      if ( poi.GetSegmentKey().girderIndex == INVALID_INDEX )
      {
         return;
      }

      // Get the doc and the view settings
      CPGSDocBase* pDoc = (CPGSDocBase*)GetDocument();
      UINT settings = pDoc->GetGirderEditorSettings();

      // Grab hold of the broker so we can pass it as a parameter
      CComPtr<IBroker> pBroker;
      EAFGetBroker(&pBroker);

      // build the dimension lines
      if ( settings & IDG_SV_SHOW_DIMENSIONS )
      {
         BuildDimensionDisplayObjects(pDoc, pBroker, poi, dispMgr);
      }
   }
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
      {
         pOldFont = pDC->SelectObject(&font);
      }

      MultiLineTextOut(pDC,0,0,msg);

      if ( pOldFont )
      {
         pDC->SelectObject(pOldFont);
      }

      return;
   }


   if ( m_GirderKey.girderIndex != ALL_GIRDERS  )
   {
      CDisplayView::OnDraw(pDC);
   }
   else
   {
      CString msg(_T("Select a girder to display"));
      CFont font;
      CFont* pOldFont = NULL;
      if ( font.CreatePointFont(100,_T("Arial"),pDC) )
      {
         pOldFont = pDC->SelectObject(&font);
      }

      MultiLineTextOut(pDC,0,0,msg);

      if ( pOldFont )
      {
         pDC->SelectObject(pOldFont);
      }
   }
}

bool CGirderModelSectionView::DidGirderSelectionChange()
{
   const CGirderKey& girderKey = m_pFrame->GetSelection();
   if ( girderKey != m_GirderKey )
   {
      m_GirderKey = girderKey;
      return true;
   }

   return false;
}

CGirderKey CGirderModelSectionView::GetGirderKey()
{
   CGirderKey girderKey = m_pFrame->GetSelection();
   if ( girderKey.groupIndex == INVALID_INDEX )
   {
      pgsPointOfInterest poi(m_pFrame->GetCutLocation());
      girderKey = poi.GetSegmentKey();
   }

   ATLASSERT(girderKey.groupIndex != INVALID_INDEX && girderKey.girderIndex != INVALID_INDEX);
   return girderKey;
}
