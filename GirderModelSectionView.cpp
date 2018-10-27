///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2018  Washington State Department of Transportation
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

#include <PgsExt\BridgeDescription2.h>
#include <PgsExt\DeckDescription2.h>

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
#define SOCKET_CGPS_TC 6 // top center for CG of prestress force dimension line
#define SOCKET_CGPS 7 // CG of prestress force
#define SOCKET_CGPS_BC 8 // bottom center for CG of prestress force dimension line
#define SOCKET_TLJ  9 // top left joint
#define SOCKET_TRJ  10 // top right joint
#define SOCKET_CG_TOP 11 // top center for CG of section dimension line
#define SOCKET_CG    12 // CG of section
#define SOCKET_CG_BOTTOM 13 // bottom center for CG of section dimension line
#define SOCKET_CGX_LEFT 14 
#define SOCKET_CGX     15
#define SOCKET_CGX_RIGHT 16

#define SECTION_LIST             1
#define JOINT_LIST               2
#define STRAIGHT_STRAND_LIST     3
#define HARPED_STRAND_LIST       4
#define TEMP_STRAND_LIST         5
#define DUCT_LIST                6
#define DIMENSION_LIST           7
#define STRAND_CG_LIST           8
#define LONG_REINF_LIST          9
#define CG_LIST                 10
#define PROPERTIES_LIST         11

#define FONT_POINT_SIZE  80 // 8 point font

/////////////////////////////////////////////////////////////////////////////
// CGirderModelSectionView

IMPLEMENT_DYNCREATE(CGirderModelSectionView, CDisplayView)

CGirderModelSectionView::CGirderModelSectionView() :
m_GirderKey(0, 0),
m_bOnIntialUpdateComplete(false),
m_pFrame(nullptr)
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

CGirderModelChildFrame* CGirderModelSectionView::GetFrame()
{
   return m_pFrame;
}

/////////////////////////////////////////////////////////////////////////////
// CGirderModelSectionView drawing
void CGirderModelSectionView::DoPrint(CDC* pDC, CPrintInfo* pInfo)
{
   OnBeginPrinting(pDC, pInfo);
   OnPrepareDC(pDC);
   UpdateDrawingScale();
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
   UpdateDrawingScale();

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
   ::CoCreateInstance(CLSID_DisplayList,nullptr,CLSCTX_ALL,IID_iDisplayList,(void**)&straight_strand_list);
   straight_strand_list->SetID(STRAIGHT_STRAND_LIST);
   dispMgr->AddDisplayList(straight_strand_list);

   CComPtr<iDisplayList> harped_strand_list;
   ::CoCreateInstance(CLSID_DisplayList,nullptr,CLSCTX_ALL,IID_iDisplayList,(void**)&harped_strand_list);
   harped_strand_list->SetID(HARPED_STRAND_LIST);
   dispMgr->AddDisplayList(harped_strand_list);

   CComPtr<iDisplayList> temp_strand_list;
   ::CoCreateInstance(CLSID_DisplayList,nullptr,CLSCTX_ALL,IID_iDisplayList,(void**)&temp_strand_list);
   temp_strand_list->SetID(TEMP_STRAND_LIST);
   dispMgr->AddDisplayList(temp_strand_list);

   CComPtr<iDisplayList> duct_list;
   ::CoCreateInstance(CLSID_DisplayList,nullptr,CLSCTX_ALL,IID_iDisplayList,(void**)&duct_list);
   duct_list->SetID(DUCT_LIST);
   dispMgr->AddDisplayList(duct_list);

   CComPtr<iDisplayList> long_reinf_list;
   ::CoCreateInstance(CLSID_DisplayList,nullptr,CLSCTX_ALL,IID_iDisplayList,(void**)&long_reinf_list);
   long_reinf_list->SetID(LONG_REINF_LIST);
   dispMgr->AddDisplayList(long_reinf_list);

   CComPtr<iDisplayList> strand_cg_list;
   ::CoCreateInstance(CLSID_DisplayList, nullptr, CLSCTX_ALL, IID_iDisplayList, (void**)&strand_cg_list);
   strand_cg_list->SetID(STRAND_CG_LIST);
   dispMgr->AddDisplayList(strand_cg_list);

   CComPtr<iDisplayList> cg_list;
   ::CoCreateInstance(CLSID_DisplayList, nullptr, CLSCTX_ALL, IID_iDisplayList, (void**)&cg_list);
   cg_list->SetID(CG_LIST);
   dispMgr->AddDisplayList(cg_list);

   CComPtr<iDisplayList> dimension_list;
   ::CoCreateInstance(CLSID_DisplayList,nullptr,CLSCTX_ALL,IID_iDisplayList,(void**)&dimension_list);
   dimension_list->SetID(DIMENSION_LIST);
   dispMgr->AddDisplayList(dimension_list);

   CComPtr<iDisplayList> section_list;
   ::CoCreateInstance(CLSID_DisplayList, nullptr, CLSCTX_ALL, IID_iDisplayList, (void**)&section_list);
   section_list->SetID(SECTION_LIST);
   dispMgr->AddDisplayList(section_list);

   CComPtr<iDisplayList> joint_list;
   ::CoCreateInstance(CLSID_DisplayList, nullptr, CLSCTX_ALL, IID_iDisplayList, (void**)&joint_list);
   joint_list->SetID(JOINT_LIST);
   dispMgr->AddDisplayList(joint_list);

   CComPtr<iDisplayList> properties_list;
   ::CoCreateInstance(CLSID_DisplayList, nullptr, CLSCTX_ALL, IID_iDisplayList, (void**)&properties_list);
   properties_list->SetID(PROPERTIES_LIST);
   dispMgr->AddDisplayList(properties_list);
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

   // Grab hold of the broker so we can pass it as a parameter
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   EventIndexType eventIdx = m_pFrame->GetEvent();
   EventIndexType erectionEventIdx = pIBridgeDesc->GetSegmentErectionEventIndex(poi.GetSegmentKey());
   if ( eventIdx < erectionEventIdx )
   {
      return;
   }


   CPGSDocBase* pDoc = (CPGSDocBase*)GetDocument();
   UINT settings = pDoc->GetGirderEditorSettings();

   if (settings & IDG_SV_PROPERTIES)
   {
      BuildPropertiesDisplayObjects(pDoc, pBroker, poi, dispMgr);
   }

   BuildSectionDisplayObjects(pDoc, pBroker, poi, dispMgr);
   BuildLongitudinalJointDisplayObject(pDoc, pBroker, poi, dispMgr);

   if (settings & IDG_SV_GIRDER_CG)
   {
      BuildCGDisplayObjects(pDoc, pBroker, poi, dispMgr);
   }

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
      BuildStrandCGDisplayObjects(pDoc, pBroker, poi, dispMgr);
   }

   if ( settings & IDG_SV_SHOW_DIMENSIONS )
   {
      BuildDimensionDisplayObjects(pDoc, pBroker, poi, dispMgr);
   }

   SetMappingMode(DManip::Isotropic);
}

void CGirderModelSectionView::BuildPropertiesDisplayObjects(CPGSDocBase* pDoc, IBroker* pBroker, const pgsPointOfInterest& poi, iDisplayMgr* pDispMgr)
{
   CComPtr<iDisplayList> pDL;
   pDispMgr->FindDisplayList(PROPERTIES_LIST, &pDL);
   ATLASSERT(pDL);
   pDL->Clear();

   if (poi.GetSegmentKey().girderIndex != INVALID_INDEX)
   {
      EventIndexType eventIdx = m_pFrame->GetEvent();

      GET_IFACE2(pBroker, IIntervals, pIntervals);
      IntervalIndexType intervalIdx = pIntervals->GetInterval(eventIdx);
      IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(poi.GetSegmentKey());
      IntervalIndexType compositeIntervalIdx = pIntervals->GetLastCompositeInterval();

      GET_IFACE2(pBroker, ISectionProperties, pSectProps);
      pgsTypes::SectionPropertyMode spMode = pSectProps->GetSectionPropertiesMode();
      Float64 A = pSectProps->GetAg(intervalIdx, poi);
      Float64 Ix = pSectProps->GetIxx(intervalIdx, poi);
      Float64 Iy = pSectProps->GetIyy(intervalIdx, poi);
      Float64 Ixy = pSectProps->GetIxy(intervalIdx, poi);
      Float64 Xl = pSectProps->GetXleft(intervalIdx, poi);
      Float64 Xr = pSectProps->GetXright(intervalIdx, poi);
      Float64 Yt = pSectProps->GetY(intervalIdx, poi, intervalIdx < compositeIntervalIdx ? pgsTypes::TopGirder : pgsTypes::TopDeck);
      Float64 Yb = pSectProps->GetY(intervalIdx, poi, pgsTypes::BottomGirder);
      Float64 St = -pSectProps->GetS(intervalIdx, poi, intervalIdx < compositeIntervalIdx ? pgsTypes::TopGirder : pgsTypes::TopDeck);
      Float64 Sb = pSectProps->GetS(intervalIdx, poi, pgsTypes::BottomGirder);

      GET_IFACE2(pBroker, IEAFDisplayUnits, pDisplayUnits);

      CString strSectPropMode(spMode == pgsTypes::spmGross ? _T("Gross") : _T("Transformed"));
      
      CString strProps;
      if (erectionIntervalIdx < intervalIdx)
      {
         strProps.Format(_T("%s Properties\nA = %s\nIx = %s\nYtop = %s\nYbottom = %s\nStop = %s\nSbottom = %s"),
            strSectPropMode,
            FormatDimension(A, pDisplayUnits->GetAreaUnit()),
            FormatDimension(Ix, pDisplayUnits->GetMomentOfInertiaUnit()),
            FormatDimension(Yt, pDisplayUnits->GetComponentDimUnit()),
            FormatDimension(Yb, pDisplayUnits->GetComponentDimUnit()),
            FormatDimension(St, pDisplayUnits->GetSectModulusUnit()),
            FormatDimension(Sb, pDisplayUnits->GetSectModulusUnit())
         );
      }
      else
      {
         GET_IFACE2(pBroker, IStrandGeometry, pStrandGeom);
         Float64 nEffectiveStrands, ex, ey;
         pStrandGeom->GetEccentricity(intervalIdx, poi, true /*include temp strands*/, &nEffectiveStrands, &ex, &ey);

         GET_IFACE2(pBroker, IBridge, pBridge);
         if (pBridge->HasAsymmetricGirders())
         {
            strProps.Format(_T("%s Properties\nA = %s\nIx = %s\nIy = %s\nIxy = %s\nXleft = %s\nXright = %s\nYtop = %s\nYbottom = %s\nex = %s\ney = %s"),
               strSectPropMode,
               FormatDimension(A, pDisplayUnits->GetAreaUnit()),
               FormatDimension(Ix, pDisplayUnits->GetMomentOfInertiaUnit()),
               FormatDimension(Iy, pDisplayUnits->GetMomentOfInertiaUnit()),
               FormatDimension(Ixy, pDisplayUnits->GetMomentOfInertiaUnit()),
               FormatDimension(Xl, pDisplayUnits->GetComponentDimUnit()),
               FormatDimension(Xr, pDisplayUnits->GetComponentDimUnit()),
               FormatDimension(Yt, pDisplayUnits->GetComponentDimUnit()),
               FormatDimension(Yb, pDisplayUnits->GetComponentDimUnit()),
               FormatDimension(ex, pDisplayUnits->GetComponentDimUnit()),
               FormatDimension(ey, pDisplayUnits->GetComponentDimUnit())
            );
         }
         else
         {
            // ignoring Ixy for non-asymmetric girders
            strProps.Format(_T("%s Properties\nA = %s\nIx = %s\nIy = %s\nXleft = %s\nXright = %s\nYtop = %s\nYbottom = %s\nStop = %s\nSbottom = %s\nex = %s\ney = %s"),
               strSectPropMode,
               FormatDimension(A, pDisplayUnits->GetAreaUnit()),
               FormatDimension(Ix, pDisplayUnits->GetMomentOfInertiaUnit()),
               FormatDimension(Iy, pDisplayUnits->GetMomentOfInertiaUnit()),
               FormatDimension(Xl, pDisplayUnits->GetComponentDimUnit()),
               FormatDimension(Xr, pDisplayUnits->GetComponentDimUnit()),
               FormatDimension(Yt, pDisplayUnits->GetComponentDimUnit()),
               FormatDimension(Yb, pDisplayUnits->GetComponentDimUnit()),
               FormatDimension(St, pDisplayUnits->GetSectModulusUnit()),
               FormatDimension(Sb, pDisplayUnits->GetSectModulusUnit()),
               FormatDimension(ex, pDisplayUnits->GetComponentDimUnit()),
               FormatDimension(ey, pDisplayUnits->GetComponentDimUnit())
            );
         }
      }

      CComPtr<iAnchoredTextBlock> textBlock;
      textBlock.CoCreateInstance(CLSID_AnchoredTextBlock);
      textBlock->SetLocation(CPoint(5,5)); // location of text block relative to top left corner

      textBlock->SetText(strProps);

      pDL->AddDisplayObject(textBlock);
   }
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
   GET_IFACE2(pBroker,IIntervals,pIntervals);
   GET_IFACE2(pBroker,ISectionProperties,pSectProps);

   EventIndexType eventIdx = m_pFrame->GetEvent();
   EventIndexType castDeckEventIdx = pIBridgeDesc->GetCastDeckEventIndex();
   IntervalIndexType intervalIdx = pIntervals->GetInterval(eventIdx);

   pgsTypes::SupportedDeckType deckType = pIBridgeDesc->GetDeckDescription()->GetDeckType();

   CComPtr<iPointDisplayObject> doPnt;
   ::CoCreateInstance(CLSID_PointDisplayObject,nullptr,CLSCTX_ALL,IID_iPointDisplayObject,(void**)&doPnt);
   doPnt->SetID(1);

   CComPtr<iShapeDrawStrategy> strategy;
   ::CoCreateInstance(CLSID_ShapeDrawStrategy,nullptr,CLSCTX_ALL,IID_iShapeDrawStrategy,(void**)&strategy);
   doPnt->SetDrawingStrategy(strategy);

   // Get the shape in Girder Section Coordinates so that it is in the same coordinate system
   // as the items internal to the section (strand, rebar, etc.) (0,0 is at top center of girder)
   CComPtr<IShape> shape;
   pShapes->GetSegmentShape(intervalIdx,poi,false/*don't orient... shape is always plumb*/,pgsTypes::scGirder,pSectProps->GetHaunchAnalysisSectionPropertiesType(),&shape);
   strategy->SetShape(shape);
   strategy->SetSolidLineColor(SEGMENT_BORDER_COLOR);
   strategy->SetSolidFillColor(segmentKey.girderIndex == m_pFrame->GetSelection().girderIndex ? SEGMENT_FILL_COLOR : SEGMENT_FILL_GHOST_COLOR);
   strategy->SetVoidLineColor(VOID_BORDER_COLOR);
   strategy->SetVoidFillColor(GetSysColor(COLOR_WINDOW));
   strategy->DoFill(true);

   // Set up sockets so dimension lines can plug into the girder shape
   CComPtr<IRect2d> boxGirder, boxSlab;
   CComPtr<IPoint2d> pntTC, pntBC; // top and bottom center
   Float64 wLeft, wRight;
   if (eventIdx <= castDeckEventIdx || IsNonstructuralDeck(deckType))
   {
      GET_IFACE2(pBroker, IGirder, pGirder);
      pGirder->GetTopWidth(poi,&wLeft,&wRight);
      shape->get_BoundingBox(&boxGirder);
      boxSlab = boxGirder;
   }
   else
   {
      Float64 top_width = pSectProps->GetTributaryFlangeWidth(poi);
      wLeft = top_width / 2;
      wRight = wLeft;

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

   CComPtr<iSocket> socketTL, socketTC, socketTR, socketBL, socketBC, socketBR;
   CComQIPtr<iConnectable> connectable(doPnt);

   // sockets for top flange dimension lines
   boxSlab->get_TopCenter(&pntTC);
   pntTC->Offset(0.5*(wLeft - wRight), 0); // now pntTC is at the centerline girder

   pntTC->Offset(-wLeft,0); // move pntTC to left edge and create a socket
   connectable->AddSocket(SOCKET_TL,pntTC,&socketTL);

   pntTC->Offset(wLeft+wRight,0); // move pntTC to right edge and create a socket
   connectable->AddSocket(SOCKET_TR,pntTC,&socketTR);

   // recycle pntTC for top center of girder shape bounding box
   pntTC.Release();
   boxGirder->get_TopCenter(&pntTC);
   pntTC->Offset(0.5*(wLeft - wRight), 0); // now pntTC is at the centerline girder

   boxGirder->get_BottomCenter(&pntBC);
   Float64 yBot;
   pntBC->get_Y(&yBot); // elevation of bottom of girder
   Float64 Hg = pSectProps->GetHg(intervalIdx, poi); // CL height of girder
   Float64 yTop = yBot + Hg; // elevation of top of girder
   pntTC->put_Y(yTop);
   connectable->AddSocket(SOCKET_TC,pntTC,&socketTC);

   // sockets for bottom flange dimension line
   CComQIPtr<IGirderSection> section(shape);
   FlangeIndexType nBottomFlanges, nWebs;
   section->get_BottomFlangeCount(&nBottomFlanges);
   section->get_WebCount(&nWebs);

   pntBC->Offset(0.5*(wLeft - wRight), 0);
   connectable->AddSocket(SOCKET_BC,pntBC,&socketBC);

   if (0 < nBottomFlanges)
   {
      Float64 x;
      section->get_BottomFlangeLocation(0, &x);
      Float64 wx;
      section->get_BottomFlangeWidth(0, &wx);
      CComPtr<IPoint2d> pntBL;
      pntBL.CoCreateInstance(CLSID_Point2d);
      pntBL->MoveEx(pntBC);
      pntBL->Offset(x - 0.5*(wLeft - wRight + wx), 0);
      connectable->AddSocket(SOCKET_BL, pntBL, &socketBL);

      section->get_BottomFlangeLocation(nBottomFlanges - 1, &x);
      section->get_BottomFlangeWidth(nBottomFlanges - 1, &wx);
      CComPtr<IPoint2d> pntBR;
      pntBR.CoCreateInstance(CLSID_Point2d);
      pntBR->MoveEx(pntBC);
      pntBR->Offset(x - 0.5*(wLeft - wRight - wx), 0);
      connectable->AddSocket(SOCKET_BR, pntBR, &socketBR);
   }
   else if (0 < nWebs)
   {
      Float64 x;
      section->get_WebLocation(0, &x);
      Float64 wx;
      section->get_WebThickness(0, &wx);
      CComPtr<IPoint2d> pntBL;
      pntBL.CoCreateInstance(CLSID_Point2d);
      pntBL->MoveEx(pntBC);
      pntBL->Offset(x - 0.5*(wLeft - wRight + wx), 0);
      connectable->AddSocket(SOCKET_BL, pntBL, &socketBL);

      section->get_WebLocation(nWebs-1, &x);
      section->get_WebThickness(nWebs - 1, &wx);
      CComPtr<IPoint2d> pntBR;
      pntBR.CoCreateInstance(CLSID_Point2d);
      pntBR->MoveEx(pntBC);
      pntBR->Offset(x - 0.5*(wLeft - wRight - wx), 0);
      connectable->AddSocket(SOCKET_BR, pntBR, &socketBR);
   }
   else
   {
      GET_IFACE2(pBroker, IGirder, pGirder);
      Float64 bottom_width = pGirder->GetBottomWidth(poi);
      pntBC->Offset(-bottom_width / 2, 0);
      connectable->AddSocket(SOCKET_BL, pntBC, &socketBL);
      pntBC->Offset(bottom_width, 0);
      connectable->AddSocket(SOCKET_BR, pntBC, &socketBR);
   }

   pDL->AddDisplayObject(doPnt);
}

void CGirderModelSectionView::BuildLongitudinalJointDisplayObject(CPGSDocBase* pDoc, IBroker* pBroker, const pgsPointOfInterest& poi, iDisplayMgr* pDispMgr)
{
   GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   if ( !pBridgeDesc->HasStructuralLongitudinalJoints() )
   {
      return;
   }

   GET_IFACE2(pBroker, IIntervals, pIntervals);

   EventIndexType eventIdx = m_pFrame->GetEvent();
   IntervalIndexType intervalIdx = pIntervals->GetInterval(eventIdx);
   IntervalIndexType compositeJointIntervalIdx = pIntervals->GetCompositeLongitudinalJointInterval();
   if (intervalIdx < compositeJointIntervalIdx)
   {
      // if joints aren't part of the section yet, there is nothing to draw
      return;
   }

   CComPtr<iDisplayList> pDL;
   pDispMgr->FindDisplayList(JOINT_LIST, &pDL);
   ATLASSERT(pDL);
   pDL->Clear();

   const CSegmentKey& segmentKey(poi.GetSegmentKey());

   GET_IFACE2(pBroker, IShapes, pShapes);
   //GET_IFACE2(pBroker, IGirder, pGirder);

   CComPtr<iPointDisplayObject> doPnt;
   ::CoCreateInstance(CLSID_PointDisplayObject, nullptr, CLSCTX_ALL, IID_iPointDisplayObject, (void**)&doPnt);
   doPnt->SetID(1);

   CComPtr<iShapeDrawStrategy> strategy;
   ::CoCreateInstance(CLSID_ShapeDrawStrategy, nullptr, CLSCTX_ALL, IID_iShapeDrawStrategy, (void**)&strategy);
   doPnt->SetDrawingStrategy(strategy);

   // Get the shape in Girder Section Coordinates so that it is in the same coordinate system
   // as the items internal to the section (strand, rebar, etc.) (0,0 is at top center of girder)
   CComPtr<IShape> leftJointShape, rightJointShape;
   pShapes->GetJointShapes(intervalIdx, poi, false/*don't orient... shape is always plumb*/, pgsTypes::scGirder, &leftJointShape, &rightJointShape);

   if (leftJointShape == nullptr && rightJointShape == nullptr)
   {
      // this girder supports joints, but they are so small they don't  have a shape
      // joint width is probably zero
      return;
   }

   CComPtr<ICompositeShape> compShape;
   compShape.CoCreateInstance(CLSID_CompositeShape);
   if (leftJointShape)
   {
      compShape->AddShape(leftJointShape, VARIANT_FALSE);
   }

   if (rightJointShape)
   {
      compShape->AddShape(rightJointShape, VARIANT_FALSE);
   }

   CComQIPtr<IShape> shape(compShape);

   strategy->SetShape(shape);
   strategy->SetSolidLineColor(JOINT_BORDER_COLOR);
   strategy->SetSolidFillColor(segmentKey.girderIndex == m_pFrame->GetSelection().girderIndex ? JOINT_FILL_COLOR : SEGMENT_FILL_GHOST_COLOR);
   strategy->DoFill(true);

   // Set up sockets so dimension lines can plug into the joint shapes
   CComPtr<IRect2d> boxLeftJoint, boxRightJoint;
   if (leftJointShape)
   {
      leftJointShape->get_BoundingBox(&boxLeftJoint);
   }

   if (rightJointShape)
   {
      rightJointShape->get_BoundingBox(&boxRightJoint);
   }

   CComPtr<iSocket> socketLeftJoint, socketRightJoint;
   CComQIPtr<iConnectable> connectable(doPnt);

   // sockets for top flange dimension lines
   CComPtr<IPoint2d> pntTL, pntTR;
   if (leftJointShape)
   {
      boxLeftJoint->get_TopLeft(&pntTL);
      Float64 x, y;
      pntTL->Location(&x, &y);
      pntTL->Move(x, 0);
      connectable->AddSocket(SOCKET_TLJ, pntTL, &socketLeftJoint);
   }

   if (rightJointShape)
   {
      boxRightJoint->get_TopRight(&pntTR);
      Float64 x, y;
      pntTR->Location(&x, &y);
      pntTR->Move(x, 0);
      connectable->AddSocket(SOCKET_TRJ, pntTR, &socketRightJoint);
   }

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
   ::CoCreateInstance(CLSID_SimpleDrawPointStrategy,nullptr,CLSCTX_ALL,IID_iSimpleDrawPointStrategy,(void**)&strategy);
   strategy->SetColor(STRAND_FILL_COLOR);
   strategy->SetPointType(ptCircle);
   strategy->SetPointSize(diameter);

   CComPtr<iSimpleDrawPointStrategy> debond_strategy;
   ::CoCreateInstance(CLSID_SimpleDrawPointStrategy,nullptr,CLSCTX_ALL,IID_iSimpleDrawPointStrategy,(void**)&debond_strategy);
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
      ::CoCreateInstance(CLSID_PointDisplayObject,nullptr,CLSCTX_ALL,IID_iPointDisplayObject,(void**)&doPnt);
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
      ::CoCreateInstance(CLSID_PointDisplayObject,nullptr,CLSCTX_ALL,IID_iPointDisplayObject,(void**)&doPnt);
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
         ::CoCreateInstance(CLSID_PointDisplayObject,nullptr,CLSCTX_ALL,IID_iPointDisplayObject,(void**)&doPnt);
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
      ::CoCreateInstance(CLSID_TextBlock,nullptr,CLSCTX_ALL,IID_iTextBlock,(void**)&textBlock);

      textBlock->SetText(strStrands);
      textBlock->SetPosition(pnt);
      textBlock->SetTextAlign(TA_BASELINE | TA_CENTER);
      textBlock->SetTextColor(BLACK);
      textBlock->SetBkMode(TRANSPARENT);
      textBlock->SetPointSize(FONT_POINT_SIZE);

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
   ::CoCreateInstance(CLSID_SimpleDrawPointStrategy,nullptr,CLSCTX_ALL,IID_iSimpleDrawPointStrategy,(void**)&strategy);
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
   while ( enum_items->Next(1,&item,nullptr) != S_FALSE )
   {
      CComPtr<IPoint2d> p;
      item->get_Location(&p);

      CComPtr<iPointDisplayObject> doPnt;
      ::CoCreateInstance(CLSID_PointDisplayObject,nullptr,CLSCTX_ALL,IID_iPointDisplayObject,(void**)&doPnt);
      doPnt->SetPosition(p,FALSE,FALSE);
      doPnt->SetID(id++);
      doPnt->SetDrawingStrategy(strategy);

      pDL->AddDisplayObject(doPnt);

      item.Release();
   }
}

void CGirderModelSectionView::BuildStrandCGDisplayObjects(CPGSDocBase* pDoc,IBroker* pBroker,const pgsPointOfInterest& poi,iDisplayMgr* pDispMgr)
{
   CComPtr<iDisplayList> pDL;
   pDispMgr->FindDisplayList(STRAND_CG_LIST,&pDL);
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

   GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc);
   const CPrecastSegmentData* pSegment = pIBridgeDesc->GetPrecastSegmentData(segmentKey);
   SegmentIDType segID = pSegment->GetID();
   EventIndexType erectSegmentEventIdx = pIBridgeDesc->GetTimelineManager()->GetSegmentErectionEventIndex(segID);

   GET_IFACE2(pBroker, IStrandGeometry, pStrandGeometry);
   StrandIndexType nStrands = pStrandGeometry->GetStrandCount(segmentKey, pgsTypes::Straight);
   nStrands += pStrandGeometry->GetStrandCount(segmentKey, pgsTypes::Harped);
   nStrands += pStrandGeometry->GetStrandCount(segmentKey, pgsTypes::Temporary);

   if (eventIdx < erectSegmentEventIdx || nStrands == 0)
   {
      return;
   }


   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeom);
   Float64 nEffective, cgx, cgy;
   pStrandGeom->GetStrandCG(intervalIdx, poi, true, &nEffective, &cgx, &cgy);

   CComPtr<IPoint2d> point;
   point.CoCreateInstance(__uuidof(Point2d));
   point->Move(cgx,cgy);

   CComPtr<iPointDisplayObject> doPnt;
   ::CoCreateInstance(CLSID_PointDisplayObject,nullptr,CLSCTX_ALL,IID_iPointDisplayObject,(void**)&doPnt);
   doPnt->SetID(1);
   doPnt->SetPosition(point,FALSE,FALSE);

   CComPtr<iTargetDrawStrategy> strategy;
   ::CoCreateInstance(CLSID_TargetDrawStrategy,nullptr,CLSCTX_ALL,IID_iTargetDrawStrategy,(void**)&strategy);
   CRect rc;
   GetClientRect(&rc);
   strategy->SetRadius(rc.Width()/80);

   strategy->SetFgColor(STRAND_CG_COLOR_1);
   strategy->SetBgColor(STRAND_CG_COLOR_2);

   doPnt->SetDrawingStrategy(strategy);

   // setup socket for dimension line
   CComQIPtr<iConnectable> connectable(doPnt);

   CComPtr<iSocket> socketCGPS;
   connectable->AddSocket(SOCKET_CGPS,point,&socketCGPS);

   // we don't know where the top/bottom of the dimensions are going to be
   // but we will create the sockets for them now. We will move these sockets
   // when we make the dimensions lines
   CComPtr<iSocket> socketCGPS_TC, socketCGPS_BC;
   connectable->AddSocket(SOCKET_CGPS_TC, point, &socketCGPS_TC);
   connectable->AddSocket(SOCKET_CGPS_BC, point, &socketCGPS_BC);

   pDL->AddDisplayObject(doPnt);
}

void CGirderModelSectionView::BuildCGDisplayObjects(CPGSDocBase* pDoc, IBroker* pBroker, const pgsPointOfInterest& poi, iDisplayMgr* pDispMgr)
{
   CComPtr<iDisplayList> pDL;
   pDispMgr->FindDisplayList(CG_LIST, &pDL);
   ATLASSERT(pDL);
   pDL->Clear();

   const CSegmentKey& segmentKey(poi.GetSegmentKey());

   // nothing to draw if poi is in a closure joint
   // (poi object won't necessarily have the POI_CLOSURE attribute so just check to see
   // if it is beyond the end of the segment)
   GET_IFACE2(pBroker, IBridge, pBridge);
   Float64 segment_length = pBridge->GetSegmentLength(segmentKey);
   if (segment_length < poi.GetDistFromStart())
   {
      return;
   }

   GET_IFACE2(pBroker, IIntervals, pIntervals);
   EventIndexType eventIdx = m_pFrame->GetEvent();
   IntervalIndexType intervalIdx = pIntervals->GetInterval(eventIdx);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
   IntervalIndexType castDeckintervalIdx = pIntervals->GetCastDeckInterval();
   if (intervalIdx < releaseIntervalIdx)
   {
      intervalIdx = releaseIntervalIdx;
   }

   GET_IFACE2(pBroker, ISectionProperties, pSectProp);
   Float64 Xleft = pSectProp->GetXleft(intervalIdx, poi);
   Float64 Xright = pSectProp->GetXright(intervalIdx, poi);
   Float64 Yb = pSectProp->GetY(intervalIdx, poi, pgsTypes::BottomGirder);
   Float64 Hg = pSectProp->GetHg(intervalIdx, poi);

   pgsTypes::SupportedDeckType deckType = pBridge->GetDeckType();

   CComPtr<IPoint2d> point;
   point.CoCreateInstance(__uuidof(Point2d));
   point->Move(0.5*(Xleft - Xright), Yb - Hg);

   CComPtr<iPointDisplayObject> doPnt;
   ::CoCreateInstance(CLSID_PointDisplayObject, nullptr, CLSCTX_ALL, IID_iPointDisplayObject, (void**)&doPnt);
   doPnt->SetID(1);
   doPnt->SetPosition(point, FALSE, FALSE);

   CComPtr<iTargetDrawStrategy> strategy;
   ::CoCreateInstance(CLSID_TargetDrawStrategy, nullptr, CLSCTX_ALL, IID_iTargetDrawStrategy, (void**)&strategy);
   CRect rc;
   GetClientRect(&rc);
   strategy->SetRadius(rc.Width() / 80);

   strategy->SetFgColor(SECTION_CG_COLOR_1);
   strategy->SetBgColor(SECTION_CG_COLOR_2);

   doPnt->SetDrawingStrategy(strategy);

   // setup socket for dimension line
   CComQIPtr<iConnectable> connectable(doPnt);

   CComPtr<IPoint2d> socketCGPoint;
   socketCGPoint.CoCreateInstance(CLSID_Point2d);
   socketCGPoint->MoveEx(point);
   CComPtr<iSocket> socketCG;
   connectable->AddSocket(SOCKET_CG, socketCGPoint, &socketCG);

   CComPtr<IPoint2d> socketCGXPoint;
   socketCGXPoint.CoCreateInstance(CLSID_Point2d);
   CComPtr<IPoint2d> pnt;
   pnt.CoCreateInstance(CLSID_Point2d);
   pnt->Move(0.5*(Xleft - Xright), 0);
   CComPtr<iSocket> socketCGX;
   connectable->AddSocket(SOCKET_CGX, socketCGPoint, &socketCGX);

   // we don't know where the top/bottom of the dimensions are going to be
   // but we will create the sockets for them now. We will move these sockets
   // when we make the dimensions lines
   CComPtr<iSocket> socketCG_Top, socketCG_Bottom, socketCGX_Left, socketCGX_Right;
   connectable->AddSocket(SOCKET_CG_TOP, point, &socketCG_Top);
   connectable->AddSocket(SOCKET_CG_BOTTOM, point, &socketCG_Bottom);
   connectable->AddSocket(SOCKET_CGX_LEFT, point, &socketCGX_Left);
   connectable->AddSocket(SOCKET_CGX_RIGHT, point, &socketCGX_Right);


   pDL->AddDisplayObject(doPnt);
}

void CGirderModelSectionView::BuildDimensionDisplayObjects(CPGSDocBase* pDoc, IBroker* pBroker, const pgsPointOfInterest& poi, iDisplayMgr* pDispMgr)
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc);

   EventIndexType eventIdx = m_pFrame->GetEvent();

   if (eventIdx < pIBridgeDesc->GetSegmentErectionEventIndex(segmentKey))
   {
      return;
   }

   EventIndexType castDeckEventIdx = pIBridgeDesc->GetCastDeckEventIndex();
   pgsTypes::SupportedDeckType deckType = pIBridgeDesc->GetDeckDescription()->GetDeckType();

   EventIndexType castLongitudinalJointEvnetIdx = pIBridgeDesc->GetCastLongitudinalJointEventIndex();

   GET_IFACE2(pBroker, IGirder, pGirder);
   GET_IFACE2(pBroker, ISectionProperties, pSectProp);
   GET_IFACE2(pBroker, IIntervals, pIntervals);

   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
   IntervalIndexType intervalIdx = Max(pIntervals->GetInterval(eventIdx), releaseIntervalIdx);
   IntervalIndexType compositeIntervalIdx = pIntervals->GetLastCompositeInterval();

   // need to layout dimension line witness lines in twips
   const long twip_offset = 1440 / 2;

   CComPtr<iDisplayList> pDL;
   pDispMgr->FindDisplayList(DIMENSION_LIST, &pDL);
   ATLASSERT(pDL);
   pDL->Clear();

   CComPtr<iDimensionLine> doDimLineTopFlangeWidth;
   CComPtr<iDimensionLine> doDimLineBottomFlangeWidth;
   CComPtr<iDimensionLine> doDimLineHeight;
   CComPtr<iDimensionLine> doDimLineTopCGPS;
   CComPtr<iDimensionLine> doDimLineBottomCGPS;
   CComPtr<iDimensionLine> doDimLineTopCG;
   CComPtr<iDimensionLine> doDimLineBottomCG;
   CComPtr<iDimensionLine> doDimLineLeftCG;
   CComPtr<iDimensionLine> doDimLineRightCG;

   ::CoCreateInstance(CLSID_DimensionLineDisplayObject, nullptr, CLSCTX_ALL, IID_iDimensionLine, (void**)&doDimLineTopFlangeWidth);
   ::CoCreateInstance(CLSID_DimensionLineDisplayObject, nullptr, CLSCTX_ALL, IID_iDimensionLine, (void**)&doDimLineBottomFlangeWidth);
   ::CoCreateInstance(CLSID_DimensionLineDisplayObject, nullptr, CLSCTX_ALL, IID_iDimensionLine, (void**)&doDimLineHeight);
   ::CoCreateInstance(CLSID_DimensionLineDisplayObject, nullptr, CLSCTX_ALL, IID_iDimensionLine, (void**)&doDimLineTopCGPS);
   ::CoCreateInstance(CLSID_DimensionLineDisplayObject, nullptr, CLSCTX_ALL, IID_iDimensionLine, (void**)&doDimLineBottomCGPS);
   ::CoCreateInstance(CLSID_DimensionLineDisplayObject, nullptr, CLSCTX_ALL, IID_iDimensionLine, (void**)&doDimLineTopCG);
   ::CoCreateInstance(CLSID_DimensionLineDisplayObject, nullptr, CLSCTX_ALL, IID_iDimensionLine, (void**)&doDimLineBottomCG);
   ::CoCreateInstance(CLSID_DimensionLineDisplayObject, nullptr, CLSCTX_ALL, IID_iDimensionLine, (void**)&doDimLineLeftCG);
   ::CoCreateInstance(CLSID_DimensionLineDisplayObject, nullptr, CLSCTX_ALL, IID_iDimensionLine, (void**)&doDimLineRightCG);

   IDType id = 0;
   doDimLineTopFlangeWidth->SetID(id++);
   doDimLineBottomFlangeWidth->SetID(id++);
   doDimLineHeight->SetID(id++);
   doDimLineTopCGPS->SetID(id++);
   doDimLineBottomCGPS->SetID(id++);
   doDimLineTopCG->SetID(id++);
   doDimLineBottomCG->SetID(id++);
   doDimLineLeftCG->SetID(id++);
   doDimLineRightCG->SetID(id++);

   // Connect the dimension lines to the sockets in the section display object
   CComPtr<iDisplayList> pSectionDL;
   pDispMgr->FindDisplayList(SECTION_LIST, &pSectionDL);
   ATLASSERT(pSectionDL);

   CComPtr<iDisplayList> pJointDL;
   pDispMgr->FindDisplayList(JOINT_LIST, &pJointDL);
   ATLASSERT(pJointDL);

   // get the girder section display object
   CComPtr<iDisplayObject> doSection;
   pSectionDL->GetDisplayObject(0, &doSection);

   CComPtr<iDisplayObject> doJoint;
   pJointDL->GetDisplayObject(0, &doJoint);

   // get it's iConnectable interface
   CComQIPtr<iConnectable> connectableSection(doSection);
   CComQIPtr<iConnectable> connectableJoint(doJoint);

   // get the sockets
   CComPtr<iSocket> socketTL, socketTC, socketTR, socketBL, socketBC, socketBR;
   connectableSection->GetSocket(SOCKET_TL, atByID, &socketTL);
   connectableSection->GetSocket(SOCKET_TC, atByID, &socketTC);
   connectableSection->GetSocket(SOCKET_TR, atByID, &socketTR);
   connectableSection->GetSocket(SOCKET_BL, atByID, &socketBL);
   connectableSection->GetSocket(SOCKET_BC, atByID, &socketBC);
   connectableSection->GetSocket(SOCKET_BR, atByID, &socketBR);

   CComPtr<iSocket> socketTLJ, socketTRJ;
   if (connectableJoint)
   {
      connectableJoint->GetSocket(SOCKET_TLJ, atByID, &socketTLJ);
      connectableJoint->GetSocket(SOCKET_TRJ, atByID, &socketTRJ);
   }

   UINT settings = pDoc->GetGirderEditorSettings();

   CComPtr<iSocket> socketCG, socketCG_Top, socketCG_Bottom, socketCGX, socketCGX_Left, socketCGX_Right;
   if (settings & IDG_SV_GIRDER_CG)
   {
      CComPtr<iDisplayList> pCGList;
      pDispMgr->FindDisplayList(CG_LIST, &pCGList);

      CComPtr<iDisplayObject> doCG;
      pCGList->GetDisplayObject(0, &doCG);
      if (doCG)
      {
         CComQIPtr<iConnectable> connectableCG(doCG);
         connectableCG->GetSocket(SOCKET_CG, atByID, &socketCG);
         connectableCG->GetSocket(SOCKET_CG_TOP, atByID, &socketCG_Top);
         connectableCG->GetSocket(SOCKET_CG_BOTTOM, atByID, &socketCG_Bottom);
         connectableCG->GetSocket(SOCKET_CGX, atByID, &socketCGX);
         connectableCG->GetSocket(SOCKET_CGX_LEFT, atByID, &socketCGX_Left);
         connectableCG->GetSocket(SOCKET_CGX_RIGHT, atByID, &socketCGX_Right);
      }
   }

   CComPtr<iSocket> socketCGPS, socketCGPS_TC, socketCGPS_BC;
   if (settings & IDG_SV_SHOW_PS_CG)
   {
      CComPtr<iDisplayList> pStrandCGList;
      pDispMgr->FindDisplayList(STRAND_CG_LIST, &pStrandCGList);

      CComPtr<iDisplayObject> doCGPS;
      pStrandCGList->GetDisplayObject(0, &doCGPS);
      if (doCGPS)
      {
         CComQIPtr<iConnectable> connectableCGPS(doCGPS);
         connectableCGPS->GetSocket(SOCKET_CGPS, atByID, &socketCGPS);
         connectableCGPS->GetSocket(SOCKET_CGPS_TC, atByID, &socketCGPS_TC);
         connectableCGPS->GetSocket(SOCKET_CGPS_BC, atByID, &socketCGPS_BC);
      }
   }

   // get the connector interface from the dimension lines
   CComQIPtr<iConnector> connectorTopFlangeWidth(doDimLineTopFlangeWidth);
   CComQIPtr<iConnector> connectorBottomFlangeWidth(doDimLineBottomFlangeWidth);
   CComQIPtr<iConnector> connectorHeight(doDimLineHeight);
   CComQIPtr<iConnector> connectorTopCGPS(doDimLineTopCGPS);
   CComQIPtr<iConnector> connectorBottomCGPS(doDimLineBottomCGPS);
   CComQIPtr<iConnector> connectorTopCG(doDimLineTopCG);
   CComQIPtr<iConnector> connectorBottomCG(doDimLineBottomCG);
   CComQIPtr<iConnector> connectorLeftCG(doDimLineLeftCG);
   CComQIPtr<iConnector> connectorRightCG(doDimLineRightCG);

   // connect the top flange width dimension line (across the top)
   CComPtr<iPlug> startPlug, endPlug;
   connectorTopFlangeWidth->GetStartPlug(&startPlug);
   connectorTopFlangeWidth->GetEndPlug(&endPlug);

   DWORD dwCookie;
   if ((IsStructuralDeck(deckType) && castDeckEventIdx < eventIdx) || socketTLJ == nullptr)
   {
      socketTL->Connect(startPlug, &dwCookie);
   }
   else
   {
      socketTLJ->Connect(startPlug, &dwCookie);
   }

   if ((IsStructuralDeck(deckType) && castDeckEventIdx < eventIdx) || socketTRJ == nullptr)
   {
      socketTR->Connect(endPlug, &dwCookie);
   }
   else
   {
      socketTRJ->Connect(endPlug, &dwCookie);
   }

   // connect the bottom flange width dimension line (across the bottom)
   startPlug.Release();
   endPlug.Release();
   connectorBottomFlangeWidth->GetStartPlug(&startPlug);
   connectorBottomFlangeWidth->GetEndPlug(&endPlug);
   socketBR->Connect(startPlug, &dwCookie);
   socketBL->Connect(endPlug, &dwCookie);
   
   // connect the height dimension line (left side)
   startPlug.Release();
   endPlug.Release();
   connectorHeight->GetStartPlug(&startPlug);
   connectorHeight->GetEndPlug(&endPlug);

   socketBC->Connect(startPlug, &dwCookie);
   socketTC->Connect(endPlug, &dwCookie);

   // connect the CG dimension line (right side)
   if ((settings & IDG_SV_GIRDER_CG) && socketCG)
   {
      startPlug.Release();
      endPlug.Release();
      connectorTopCG->GetStartPlug(&startPlug);
      connectorTopCG->GetEndPlug(&endPlug);

      // need to determine the top center location for the upper dimension line
      // we want x of the CG and y for the top of the girder
      CComPtr<IPoint2d> cg;
      socketCG->GetPosition(&cg);
      Float64 x;
      cg->get_X(&x);

      CComPtr<IPoint2d> tc;
      socketTC->GetPosition(&tc);
      Float64 y;
      tc->get_Y(&y);

      CComPtr<IPoint2d> pntTC;
      pntTC.CoCreateInstance(CLSID_Point2d);
      pntTC->Move(x, y);
      socketCG_Top->SetPosition(pntTC);

      socketCG->Connect(startPlug, &dwCookie);
      socketCG_Top->Connect(endPlug, &dwCookie);

      startPlug.Release();
      endPlug.Release();
      connectorBottomCG->GetStartPlug(&startPlug);
      connectorBottomCG->GetEndPlug(&endPlug);

      // need y of the bottom center point
      CComPtr<IPoint2d> bc;
      socketBC->GetPosition(&bc);
      bc->get_Y(&y);

      CComPtr<IPoint2d> pntBC;
      pntBC.CoCreateInstance(CLSID_Point2d);
      pntBC->Move(x, y);
      socketCG_Bottom->SetPosition(pntBC);

      socketCG_Bottom->Connect(startPlug, &dwCookie);
      socketCG->Connect(endPlug, &dwCookie);

      // Xcg
      CComPtr<IPoint2d> pntTL;
      socketTL->GetPosition(&pntTL);
      Float64 tlX, tlY;
      pntTL->Location(&tlX, &tlY);

      CComPtr<IPoint2d> pntTR;
      socketTR->GetPosition(&pntTR);
      Float64 trX, trY;
      pntTR->Location(&trX, &trY);

      y = Max(tlY, trY);

      CComPtr<IPoint2d> pntCGX;
      socketCGX->GetPosition(&pntCGX);
      pntCGX->put_Y(y);
      socketCGX->SetPosition(pntCGX);

      CComPtr<IPoint2d> pntLeft;
      pntLeft.CoCreateInstance(CLSID_Point2d);
      pntLeft->Move(tlX,y);
      socketCGX_Left->SetPosition(pntLeft);

      CComPtr<IPoint2d> pntRight;
      pntRight.CoCreateInstance(CLSID_Point2d);
      pntRight->Move(trX, y);
      socketCGX_Right->SetPosition(pntRight);

      // Xleft dimension line
      startPlug.Release();
      endPlug.Release();
      connectorLeftCG->GetStartPlug(&startPlug);
      connectorLeftCG->GetEndPlug(&endPlug);

      socketCGX_Left->Connect(startPlug, &dwCookie);
      socketCGX->Connect(endPlug, &dwCookie);

      // Xright dimension line
      startPlug.Release();
      endPlug.Release();
      connectorRightCG->GetStartPlug(&startPlug);
      connectorRightCG->GetEndPlug(&endPlug);
      socketCGX->Connect(startPlug, &dwCookie);
      socketCGX_Right->Connect(endPlug, &dwCookie);
   }


   // connect cg ps dimension line (bottom center to cg symbol)
   if ((settings & IDG_SV_SHOW_PS_CG) && socketCGPS)
   {
      startPlug.Release();
      endPlug.Release();
      connectorTopCGPS->GetStartPlug(&startPlug);
      connectorTopCGPS->GetEndPlug(&endPlug);

      // need to determine the top center location for the upper dimension line
      // we want x of the CG and y for the top of the girder
      CComPtr<IPoint2d> cg;
      socketCGPS->GetPosition(&cg);
      Float64 x;
      cg->get_X(&x);

      CComPtr<IPoint2d> tc;
      socketTC->GetPosition(&tc);
      Float64 y;
      tc->get_Y(&y);

      CComPtr<IPoint2d> pntTC;
      pntTC.CoCreateInstance(CLSID_Point2d);
      pntTC->Move(x, y);
      socketCGPS_TC->SetPosition(pntTC);

      socketCGPS->Connect(startPlug, &dwCookie);
      socketCGPS_TC->Connect(endPlug, &dwCookie);

      startPlug.Release();
      endPlug.Release();
      connectorBottomCGPS->GetStartPlug(&startPlug);
      connectorBottomCGPS->GetEndPlug(&endPlug);

      // need y of the bottom center point
      CComPtr<IPoint2d> bc;
      socketBC->GetPosition(&bc);
      bc->get_Y(&y);

      CComPtr<IPoint2d> pntBC;
      pntBC.CoCreateInstance(CLSID_Point2d);
      pntBC->Move(x, y);
      socketCGPS_BC->SetPosition(pntBC);

      socketCGPS_BC->Connect(startPlug, &dwCookie);
      socketCGPS->Connect(endPlug, &dwCookie);
   }

   // set the text labels on the dimension lines
   Float64 twLeft, twRight;
   if (IsNonstructuralDeck(deckType) || (IsStructuralDeck(deckType) && eventIdx <= castDeckEventIdx))
   {
      pGirder->GetTopWidth(poi,&twLeft,&twRight);

      if (pGirder->HasStructuralLongitudinalJoints() && castLongitudinalJointEvnetIdx < eventIdx)
      {
         Float64 jwLeft, jwRight;
         pGirder->GetStructuralLongitudinalJointWidth(poi,&jwLeft,&jwRight);
         twLeft += jwLeft / 2; // half the joint width is attributed to this girder
         twRight += jwRight / 2;
      }
   }
   else
   {
      // top width includes longitudinal joints
      pSectProp->GetTributaryFlangeWidthEx(poi, &twLeft, &twRight);
   }

   Float64 bottom_width = pGirder->GetBottomWidth(poi);
   Float64 height = pSectProp->GetHg(intervalIdx, poi);
   Float64 Yt = pSectProp->GetY(intervalIdx, poi, intervalIdx < compositeIntervalIdx ? pgsTypes::TopGirder : pgsTypes::TopDeck);
   Float64 Yb = pSectProp->GetY(intervalIdx, poi, pgsTypes::BottomGirder);
   Float64 Xl = pSectProp->GetXleft(intervalIdx, poi);
   Float64 Xr = pSectProp->GetXright(intervalIdx, poi);

   CString strDim;
   CComPtr<iTextBlock> textBlock;

   GET_IFACE2(pBroker, IEAFDisplayUnits, pDisplayUnits);
   const unitmgtLengthData& length_unit = pDisplayUnits->GetComponentDimUnit();

   textBlock.CoCreateInstance(CLSID_TextBlock);
   textBlock->SetPointSize(FONT_POINT_SIZE);
   strDim = FormatDimension(twLeft + twRight, length_unit);
   textBlock->SetText(strDim);
   doDimLineTopFlangeWidth->SetTextBlock(textBlock);

   // move the top flange width line up
   doDimLineTopFlangeWidth->SetWitnessLength(3 * twip_offset / 2);

   textBlock.Release();
   textBlock.CoCreateInstance(CLSID_TextBlock);
   textBlock->SetPointSize(FONT_POINT_SIZE);
   strDim = FormatDimension(bottom_width, length_unit);
   textBlock->SetText(strDim);
   doDimLineBottomFlangeWidth->SetTextBlock(textBlock);

   textBlock.Release();
   textBlock.CoCreateInstance(CLSID_TextBlock);
   textBlock->SetPointSize(FONT_POINT_SIZE);
   strDim = FormatDimension(height, length_unit);
   textBlock->SetText(strDim);
   textBlock->SetBkMode(TRANSPARENT);
   doDimLineHeight->SetTextBlock(textBlock);

   // adjust the witness line
   long tx0, ty0;
   long tx1, ty1;
   m_pCoordinateMap->WPtoTP(0, 0, &tx0, &ty0);
   m_pCoordinateMap->WPtoTP(twLeft, 0, &tx1, &ty1);
   doDimLineHeight->SetWitnessLength(tx1 - tx0 + twip_offset);

   if ((settings & IDG_SV_GIRDER_CG) && socketCG)
   {
      textBlock.Release();
      textBlock.CoCreateInstance(CLSID_TextBlock);
      textBlock->SetPointSize(FONT_POINT_SIZE);
      strDim = FormatDimension(Yt, length_unit);
      textBlock->SetText(strDim);
      doDimLineTopCG->SetTextBlock(textBlock);

      textBlock.Release();
      textBlock.CoCreateInstance(CLSID_TextBlock);
      textBlock->SetPointSize(FONT_POINT_SIZE);
      strDim = FormatDimension(Yb, length_unit);
      textBlock->SetText(strDim);
      doDimLineBottomCG->SetTextBlock(textBlock);

      textBlock.Release();
      textBlock.CoCreateInstance(CLSID_TextBlock);
      textBlock->SetPointSize(FONT_POINT_SIZE);
      strDim = FormatDimension(Xl, length_unit);
      textBlock->SetText(strDim);
      doDimLineLeftCG->SetTextBlock(textBlock);

      textBlock.Release();
      textBlock.CoCreateInstance(CLSID_TextBlock);
      textBlock->SetPointSize(FONT_POINT_SIZE);
      strDim = FormatDimension(Xr, length_unit);
      textBlock->SetText(strDim);
      doDimLineRightCG->SetTextBlock(textBlock);

      CComPtr<IPoint2d> cg;
      socketCG->GetPosition(&cg);
      Float64 x1;
      cg->get_X(&x1);

      // adjust the witness lines on the vertical dimensions
      Float64 x2 = Max(twRight, bottom_width / 2);

      m_pCoordinateMap->WPtoTP(x1, 0, &tx0, &ty0);
      m_pCoordinateMap->WPtoTP(x2, 0, &tx1, &ty1);

      doDimLineTopCG->SetWitnessLength(-tx1 + tx0 - twip_offset);
      doDimLineBottomCG->SetWitnessLength(-tx1 + tx0 - twip_offset);
   }

   if ( (settings & IDG_SV_SHOW_PS_CG) && socketCGPS )
   {
      GET_IFACE2(pBroker,IStrandGeometry,pStrandGeometry);

      Float64 nEff;
      Float64 ecc = pStrandGeometry->GetEccentricity(releaseIntervalIdx, poi,true,&nEff);
      Float64 yps = pSectProp->GetY(releaseIntervalIdx,poi,pgsTypes::BottomGirder) - ecc;

      textBlock.Release();
      textBlock.CoCreateInstance(CLSID_TextBlock);
      textBlock->SetPointSize(FONT_POINT_SIZE);
      strDim = FormatDimension(yps,length_unit);
      textBlock->SetText(strDim);
      doDimLineBottomCGPS->SetTextBlock(textBlock);

      yps = height - yps;

      textBlock.Release();
      textBlock.CoCreateInstance(CLSID_TextBlock);
      textBlock->SetPointSize(FONT_POINT_SIZE);
      strDim = FormatDimension(yps, length_unit);
      textBlock->SetText(strDim);
      doDimLineTopCGPS->SetTextBlock(textBlock);

      CComPtr<IPoint2d> cg;
      socketCGPS->GetPosition(&cg);
      Float64 x1;
      cg->get_X(&x1);

      // adjust the witness lines on the vertical dimensions
      Float64 x2 = Max(twLeft, bottom_width / 2);

      m_pCoordinateMap->WPtoTP(x1, 0, &tx0, &ty0);
      m_pCoordinateMap->WPtoTP(x2, 0, &tx1, &ty1);
      doDimLineTopCGPS->SetWitnessLength(tx1 - tx0 + twip_offset);
      doDimLineBottomCGPS->SetWitnessLength(tx1 - tx0 + twip_offset);

      doDimLineHeight->SetWitnessLength(tx1-tx0+2*twip_offset);
   }

   // add the dimension line display objects to the display list
   pDL->AddDisplayObject(doDimLineTopFlangeWidth);
   pDL->AddDisplayObject(doDimLineBottomFlangeWidth);
   pDL->AddDisplayObject(doDimLineHeight);

   if ((settings & IDG_SV_GIRDER_CG) && socketCG)
   {
      pDL->AddDisplayObject(doDimLineTopCG);
      pDL->AddDisplayObject(doDimLineBottomCG);
      if (intervalIdx < compositeIntervalIdx)
      {
         pDL->AddDisplayObject(doDimLineLeftCG);
         pDL->AddDisplayObject(doDimLineRightCG);
      }
   }

   if ( (settings & IDG_SV_SHOW_PS_CG) && socketCGPS )
   {
      pDL->AddDisplayObject(doDimLineTopCGPS);
      pDL->AddDisplayObject(doDimLineBottomCGPS);
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
        lHint == HINT_SPECCHANGED   ||
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
      UpdateDrawingScale();
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
   ASSERT( m_pFrame != nullptr );
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

   UpdateDrawingScale();

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
      CFont* pOldFont = nullptr;
      if ( font.CreatePointFont(FONT_POINT_SIZE,_T("Arial"),pDC) )
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
      CFont* pOldFont = nullptr;
      if ( font.CreatePointFont(FONT_POINT_SIZE,_T("Arial"),pDC) )
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

void CGirderModelSectionView::UpdateDrawingScale()
{
   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   CComPtr<iDisplayList> display_list;
   dispMgr->FindDisplayList(PROPERTIES_LIST, &display_list);

   CDManipClientDC dc(this);

   // before scaling the drawing to fit, hide the title display objects
   // if they aren't hidden, they factor into the bounding box and they
   // mess up the scaling of the drawing.
   //
   // this is the best solution I've been able to come up with.
   if (display_list)
   {
      display_list->HideDisplayObjects(true);
   }

   ScaleToFit(false); // don't force a redraw

   if (display_list)
   {
      display_list->HideDisplayObjects(false);
   }
}
