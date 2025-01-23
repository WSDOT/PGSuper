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

// GirderModelSectionView.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "PGSuperApp.h"
#include "PGSuperDocBase.h"
#include "PGSuperDoc.h"
#include "PGSpliceDoc.h"
#include "PGSuperUnits.h"
#include "PGSuperColors.h"
#include "GirderModelSectionView.h"
#include "GirderModelChildFrame.h"
#include "GMDisplayMgrEventsImpl.h"
#include "GirderDisplayObjectEvents.h"
#include "DisplayObjectFactory.h"
#include <IFace\Bridge.h>
#include <IFace\DrawBridgeSettings.h>
#include <IFace\Intervals.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\EditByUI.h>

#include <WBFLGenericBridgeTools.h>
#include <WBFLGeometry/GeomHelpers.h>

#include <PgsExt\BridgeDescription2.h>
#include <PgsExt\DeckDescription2.h>

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
#define SOCKET_CGX_TOP     14
#define SOCKET_CGX_TOP_LEFT 15
#define SOCKET_CGX_TOP_RIGHT 16
#define SOCKET_CGX_BOTTOM    17
#define SOCKET_CGX_BOTTOM_LEFT 18
#define SOCKET_CGX_BOTTOM_RIGHT 19

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
m_GirderKey(ALL_GROUPS, 0),
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
   CDManipClientDC dc2(this);

   OnBeginPrinting(pDC, pInfo);
   OnPrepareDC(pDC);

   // the leader lines for the vertical dimension lines are set in twips units which depend on the display
   // the screen and printer have different resolution so the dpi to twips conversion is going to be different
   // to get the dimension lines to scale properly, we have to rebuild all of the display objects.
   // at this point, the mapping objects have been updated for printing so all we need to do is rebuild the display objects and print
   UpdateDisplayObjects(); //rebuild display objects with printer mapping
   UpdateDrawingScale();
   OnDraw(pDC);
   OnEndPrinting(pDC, pInfo);

   // printing is done and the mapping is returned to the screen. rebuild the display objects for the screen
   UpdateDisplayObjects(); // rebuild display objects with screen mapping
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

   m_pDispMgr->EnableLBtnSelectRect(true);
   m_pDispMgr->EnableLBtnMultiSelect(true,MK_SHIFT);
   m_pDispMgr->SetSelectionLineColor(SELECTED_OBJECT_LINE_COLOR);
   m_pDispMgr->SetSelectionFillColor(SELECTED_OBJECT_FILL_COLOR);

   auto factory = std::make_shared<CDisplayObjectFactory>(pDoc);
   m_pDispMgr->AddDisplayObjectFactory(factory);

   // set up default event handler for canvas
   auto events = CGMDisplayMgrEventsImpl::Create(pDoc, m_pFrame, this, false);
   m_pDispMgr->RegisterEventSink(events);

   CSelection selection = pDoc->GetSelection();
   if ( selection.Type != CSelection::Girder )
   {
      pDoc->SelectGirder(m_GirderKey);
   }

   m_bOnIntialUpdateComplete = true;
}

void CGirderModelSectionView::CreateDisplayLists()
{
   m_pDispMgr->EnableLBtnSelect(TRUE);
   m_pDispMgr->EnableRBtnSelect(TRUE);
   m_pDispMgr->SetSelectionLineColor(SELECTED_OBJECT_LINE_COLOR);
   m_pDispMgr->SetSelectionFillColor(SELECTED_OBJECT_FILL_COLOR);


   // Create display lists
   m_pDispMgr->CreateDisplayList(STRAIGHT_STRAND_LIST);
   m_pDispMgr->CreateDisplayList(HARPED_STRAND_LIST);
   m_pDispMgr->CreateDisplayList(TEMP_STRAND_LIST);
   m_pDispMgr->CreateDisplayList(DUCT_LIST);
   m_pDispMgr->CreateDisplayList(LONG_REINF_LIST);
   m_pDispMgr->CreateDisplayList(STRAND_CG_LIST);
   m_pDispMgr->CreateDisplayList(CG_LIST);
   m_pDispMgr->CreateDisplayList(DIMENSION_LIST);
   m_pDispMgr->CreateDisplayList(SECTION_LIST);
   m_pDispMgr->CreateDisplayList(JOINT_LIST);
   m_pDispMgr->CreateDisplayList(PROPERTIES_LIST);
}

void CGirderModelSectionView::UpdateDisplayObjects()
{
   // clean out all the display objects
   m_pDispMgr->ClearDisplayObjects();

   pgsPointOfInterest poi(m_pFrame->GetCutLocation());

   if ( poi.GetSegmentKey().girderIndex == INVALID_INDEX )
   {
      return;
   }

   // Grab hold of the broker so we can pass it as a parameter
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker, IPointOfInterest, pPoi);
   if (!pPoi->IsOnSegment(poi))
   {
      GET_IFACE2(pBroker, IBridge, pBridge);
      const CSegmentKey& segmentKey = poi.GetSegmentKey();
      Float64 Ls = pBridge->GetSegmentLength(segmentKey);
      Float64 Xs = poi.GetDistFromStart();
      Xs = ::ForceIntoRange(0.0, Xs, Ls);
      poi = pPoi->GetPointOfInterest(segmentKey, Xs);
      ATLASSERT(pPoi->IsOnSegment(poi));
   }


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
      BuildPropertiesDisplayObjects(pDoc, pBroker, poi);
   }

   BuildSectionDisplayObjects(pDoc, pBroker, poi);
   BuildLongitudinalJointDisplayObject(pDoc, pBroker, poi);

   if (settings & IDG_SV_GIRDER_CG)
   {
      BuildCGDisplayObjects(pDoc, pBroker, poi);
   }

   if ( settings & IDG_SV_SHOW_STRANDS )
   {
      BuildStrandDisplayObjects(pDoc, pBroker, poi);
      BuildDuctDisplayObjects(pDoc, pBroker, poi);
   }

   if ( settings & IDG_SV_SHOW_LONG_REINF )
   {
      BuildLongReinfDisplayObjects(pDoc, pBroker, poi);
   }

   if (settings & IDG_SV_SHOW_PS_CG)
   {
      BuildStrandCGDisplayObjects(pDoc, pBroker, poi);
   }

   if ( settings & IDG_SV_SHOW_DIMENSIONS )
   {
      BuildDimensionDisplayObjects(pDoc, pBroker, poi);
   }

   SetMappingMode(WBFL::DManip::MapMode::Isotropic);
}

void CGirderModelSectionView::BuildPropertiesDisplayObjects(CPGSDocBase* pDoc, IBroker* pBroker, const pgsPointOfInterest& poi)
{
   auto pDL = m_pDispMgr->FindDisplayList(PROPERTIES_LIST);
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
         auto ecc = pStrandGeom->GetEccentricity(intervalIdx, poi, true /*include temp strands*/);

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
               FormatDimension(ecc.X(), pDisplayUnits->GetComponentDimUnit()),
               FormatDimension(ecc.Y(), pDisplayUnits->GetComponentDimUnit())
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
               FormatDimension(ecc.X(), pDisplayUnits->GetComponentDimUnit()),
               FormatDimension(ecc.Y(), pDisplayUnits->GetComponentDimUnit())
            );
         }
      }


      GET_IFACE2(pBroker, IPointOfInterest, pPoi);
      IndexType deckCastingRegionIdx = pPoi->GetDeckCastingRegion(poi);
      if (deckCastingRegionIdx != INVALID_INDEX)
      {
         IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval(deckCastingRegionIdx);
         if (compositeDeckIntervalIdx <= intervalIdx)
         {
            Float64 Wtrib = pSectProps->GetTributaryFlangeWidth(poi);
            Float64 Weff = pSectProps->GetEffectiveFlangeWidth(poi);
            CString strFlange;
            strFlange.Format(_T("\nTributary Flange Width = %s\nEffective Flange Width = %s"), FormatDimension(Wtrib, pDisplayUnits->GetComponentDimUnit()), FormatDimension(Weff, pDisplayUnits->GetComponentDimUnit()));
            strProps += strFlange;
         }
      }

      auto textBlock = WBFL::DManip::AnchoredTextBlock::Create();
      textBlock->SetLocation(CPoint(5,5)); // location of text block relative to top left corner
      textBlock->SetText(strProps);
      pDL->AddDisplayObject(textBlock);
   }
}

void CGirderModelSectionView::BuildSectionDisplayObjects(CPGSDocBase* pDoc,IBroker* pBroker,const pgsPointOfInterest& poi)
{
   auto pDL = m_pDispMgr->FindDisplayList(SECTION_LIST);
   ATLASSERT(pDL);
   pDL->Clear();

   const CSegmentKey& segmentKey(poi.GetSegmentKey());

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   GET_IFACE2(pBroker,IShapes,pShapes);
   GET_IFACE2(pBroker,IIntervals,pIntervals);
   GET_IFACE2(pBroker,ISectionProperties,pSectProps);

   EventIndexType eventIdx = m_pFrame->GetEvent();
   EventIndexType castDeckEventIdx = pIBridgeDesc->GetCastDeckEventIndex();
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
   IntervalIndexType intervalIdx = pIntervals->GetInterval(eventIdx);

   pgsTypes::SupportedDeckType deckType = pIBridgeDesc->GetDeckDescription()->GetDeckType();

   auto doPnt = WBFL::DManip::PointDisplayObject::Create(1);
   auto strategy = WBFL::DManip::ShapeDrawStrategy::Create();
   doPnt->SetDrawingStrategy(strategy);

   // Get the shape in Girder Section Coordinates so that it is in the same coordinate system
   // as the items internal to the section (strand, rebar, etc.) (0,0 is at top center of girder)
   IndexType girderShapeIdx, slabShapeIdx;
   CComPtr<IShape> shape;
   pShapes->GetSegmentShape(intervalIdx, poi,false/*don't orient... shape is always plumb*/,pgsTypes::scGirder,&shape, &girderShapeIdx, &slabShapeIdx);
   strategy->SetShape(geomUtil::ConvertShape(shape));
   strategy->SetSolidLineColor(SEGMENT_BORDER_COLOR);
   strategy->SetSolidFillColor(segmentKey.girderIndex == m_pFrame->GetSelection().girderIndex ? SEGMENT_FILL_COLOR : SEGMENT_FILL_GHOST_COLOR);
   strategy->SetVoidLineColor(VOID_BORDER_COLOR);
   strategy->SetVoidFillColor(GetSysColor(COLOR_WINDOW));
   strategy->Fill(true);

   // Set up sockets so dimension lines can plug into the girder shape
   CComQIPtr<ICompositeShape> composite(shape);
   CComPtr<ICompositeShapeItem> girderShapeItem;
   composite->get_Item(girderShapeIdx, &girderShapeItem);
   CComPtr<IShape> girderShape;
   girderShapeItem->get_Shape(&girderShape);

   // Girder is drawn in girder coordinates. Origin is at the top CL of this bounding box:
   CComPtr<IRect2d> boxGirder;
   girderShape->get_BoundingBox(&boxGirder);

   GET_IFACE2(pBroker, IGirder, pGirder);
   // Get top and bottom girder flange widths. These are in nominal girder coordinates, they will need to be converted to girder coord's
   Float64 twLeft, twRight, bwLeft, bwRight;
   Float64 top_width    = pGirder->GetTopWidth(poi, &twLeft, &twRight);
   Float64 bottom_width = pGirder->GetBottomWidth(poi, &bwLeft, &bwRight);

   // BIG ASSUMPTION: The nominal centerline is located at the maximum of the top/bottom left width of the girder from the left side of the bounding box.
   // This assumes that the widest part of the girder is at the top or bottom of the section.
   CComPtr<IPoint2d> pntBL;
   boxGirder->get_BottomLeft(&pntBL);
   Float64 xBL;
   pntBL->get_X(&xBL);
   Float64 xNCL = xBL + Max(twLeft, bwLeft);


   // X locations of top and bottom flange tips in girder coordinates
   Float64 bxLeft  = xNCL - bwLeft;
   Float64 bxRight = xNCL + bwRight;

   Float64 txLeft, txRight;
   Float64 tyCL;
   if (eventIdx <= castDeckEventIdx || IsNonstructuralDeck(deckType))
   {
      txLeft  = xNCL - twLeft;
      txRight = xNCL + twRight;
      tyCL = 0.0;

      CComQIPtr<IFlangePoints> flange_points(girderShape);
      if (flange_points)
      {
         CComPtr<IPoint2d> tl, bl, tcl, tc, tr, br;
         flange_points->GetTopFlangePoints(&tl, &bl, &tcl, &tc, &tr, &br);
         tcl->get_Y(&tyCL);
      }
   }
   else
   {
      if (slabShapeIdx == INVALID_INDEX)
      {
         ATLASSERT(0); // should never happen. New slab type or layout?
      }
      else
      {
         CComPtr<ICompositeShapeItem> slabShapeItem;
         composite->get_Item(slabShapeIdx, &slabShapeItem);
         CComPtr<IShape> slabShape;
         slabShapeItem->get_Shape(&slabShape);
         CComPtr<IRect2d> boxSlab;
         slabShape->get_BoundingBox(&boxSlab);

         boxSlab->get_Left(&txLeft);
         boxSlab->get_Right(&txRight);

         boxSlab->get_Top(&tyCL);
      }
   }


   CComPtr<IPoint2d> pntNCL; // nominal centerline top
   //boxGirder->get_TopCenter(&pntNCL);
   //pntNCL->put_X(xNCL); // now at the nominal centerline girder
   //pntNCL->put_Y(tyCL);

   // sockets for top flange dimension lines
   auto connectable = std::dynamic_pointer_cast<WBFL::DManip::iConnectable>(doPnt);

   auto socketTC = connectable->AddSocket(SOCKET_TC, {xNCL,tyCL});

   //CComPtr<IPoint2d> pntTL;
   //pntTL.CoCreateInstance(CLSID_Point2d);
   //pntTL->Move(txLeft, tyCL);
   auto socketTL = connectable->AddSocket(SOCKET_TL,{txLeft,tyCL});

   //CComPtr<IPoint2d> pntTR;
   //pntTR.CoCreateInstance(CLSID_Point2d);
   //pntTR->Move(txRight, tyCL);
   auto socketTR = connectable->AddSocket(SOCKET_TR,{txRight,tyCL});

   // sockets for bottom flange dimension line
   pntNCL.Release();
   boxGirder->get_BottomCenter(&pntNCL);
   pntNCL->put_X(xNCL);
   Float64 byCL;
   pntNCL->get_Y(&byCL);
   auto socketBC = connectable->AddSocket(SOCKET_BC, {xNCL,byCL});

   CComQIPtr<IGirderSection> section(girderShape);
   FlangeIndexType nBottomFlanges, nWebs;
   section->get_BottomFlangeCount(&nBottomFlanges);
   section->get_WebCount(&nWebs);

   Float64 Hg = pSectProps->GetHg(releaseIntervalIdx, poi); // CL height of girder

   std::shared_ptr<WBFL::DManip::iSocket> socketBL, socketBR;
   if (0 < nBottomFlanges)
   {
      Float64 x;
      section->get_BottomFlangeLocation(0, &x);
      Float64 wx;
      section->get_BottomFlangeWidth(0, &wx);
      //CComPtr<IPoint2d> pntBL;
      //pntBL.CoCreateInstance(CLSID_Point2d);
      //pntBL->Move(x - 0.5*(bwLeft - bwRight + wx), -Hg);
      socketBL = connectable->AddSocket(SOCKET_BL, { x - 0.5 * (bwLeft - bwRight + wx), -Hg });

      section->get_BottomFlangeLocation(nBottomFlanges - 1, &x);
      section->get_BottomFlangeWidth(nBottomFlanges - 1, &wx);
      //CComPtr<IPoint2d> pntBR;
      //pntBR.CoCreateInstance(CLSID_Point2d);
      //pntBR->Move(x - 0.5*(bwLeft - bwRight - wx), -Hg);
      socketBR = connectable->AddSocket(SOCKET_BR, { x - 0.5 * (bwLeft - bwRight - wx), -Hg });
   }
   else if (0 < nWebs)
   {
      Float64 x;
      section->get_WebLocation(0, &x);
      Float64 wx;
      section->get_WebThickness(0, &wx);
      //CComPtr<IPoint2d> pntBL;
      //pntBL.CoCreateInstance(CLSID_Point2d);
      //pntBL->Move(x - 0.5*(bwLeft - bwRight + wx), -Hg);
      socketBL = connectable->AddSocket(SOCKET_BL, { x - 0.5 * (bwLeft - bwRight + wx), -Hg });

      section->get_WebLocation(nWebs-1, &x);
      section->get_WebThickness(nWebs - 1, &wx);
      //CComPtr<IPoint2d> pntBR;
      //pntBR.CoCreateInstance(CLSID_Point2d);
      //pntBR->Move(x - 0.5*(bwLeft - bwRight - wx), -Hg);
      socketBR = connectable->AddSocket(SOCKET_BR, { x - 0.5 * (bwLeft - bwRight - wx), -Hg });
   }
   else
   {
      CComPtr<IPoint2d> pntBC;
      boxGirder->get_BottomCenter(&pntBC);
      pntBC->Offset(-bottom_width / 2, 0.0);
      Float64 x, y;
      pntBC->Location(&x, &y);
      socketBL = connectable->AddSocket(SOCKET_BL, {x,y});
      pntBC->Offset(bottom_width, 0.0);
      pntBC->Location(&x, &y);
      socketBR = connectable->AddSocket(SOCKET_BR, {x,y});
   }

   // Register an event sink with the segment display object so that we can handle double clicks
   // on the segment differently then a general double click
   auto events = std::make_shared<CGirderSectionViewSegmentDisplayObjectEvents>(poi, GetFrame());
   doPnt->RegisterEventSink(events);


   pDL->AddDisplayObject(doPnt);


   // Draw the vertical nominal centerline
   Float64 y_offset = 0.01*Hg;

   // create a point at the top of the section
   WBFL::Geometry::Point2d pntTop(xNCL, y_offset);

   // create a point at the bottom of the section
   WBFL::Geometry::Point2d pntBottom(xNCL, -(Hg + y_offset));

   CreateLineDisplayObject(pDL, pntTop, pntBottom, BLACK, WBFL::DManip::LineStyleType::Centerline);
}

void CGirderModelSectionView::BuildLongitudinalJointDisplayObject(CPGSDocBase* pDoc, IBroker* pBroker, const pgsPointOfInterest& poi)
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

   auto pDL = m_pDispMgr->FindDisplayList(JOINT_LIST);
   ATLASSERT(pDL);
   pDL->Clear();

   const CSegmentKey& segmentKey(poi.GetSegmentKey());

   GET_IFACE2(pBroker, IShapes, pShapes);
   //GET_IFACE2(pBroker, IGirder, pGirder);

   auto doPnt = WBFL::DManip::PointDisplayObject::Create(1);
   auto strategy = WBFL::DManip::ShapeDrawStrategy::Create();
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

   strategy->SetShape(geomUtil::ConvertShape(shape));
   strategy->SetSolidLineColor(JOINT_BORDER_COLOR);
   strategy->SetSolidFillColor(segmentKey.girderIndex == m_pFrame->GetSelection().girderIndex ? JOINT_FILL_COLOR : SEGMENT_FILL_GHOST_COLOR);
   strategy->Fill(true);

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

   auto connectable = std::dynamic_pointer_cast<WBFL::DManip::iConnectable>(doPnt);

   // sockets for top flange dimension lines
   CComPtr<IPoint2d> pntTL, pntTR;
   if (leftJointShape)
   {
      boxLeftJoint->get_TopLeft(&pntTL);
      Float64 x, y;
      pntTL->Location(&x, &y);
      pntTL->Move(x, 0);
      connectable->AddSocket(SOCKET_TLJ, geomUtil::GetPoint(pntTL));
   }

   if (rightJointShape)
   {
      boxRightJoint->get_TopRight(&pntTR);
      Float64 x, y;
      pntTR->Location(&x, &y);
      pntTR->Move(x, 0);
      connectable->AddSocket(SOCKET_TRJ, geomUtil::GetPoint(pntTR));
   }

   pDL->AddDisplayObject(doPnt);
}

void CGirderModelSectionView::BuildStrandDisplayObjects(CPGSDocBase* pDoc,IBroker* pBroker,const pgsPointOfInterest& poi)
{
   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeom);

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   // Strands are measured in Girder Section Coordinates
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

      auto doPnt = WBFL::DManip::PointDisplayObject::Create();
      doPnt->SetPosition(geomUtil::GetPoint(p),false,false);
      doPnt->SetID(strandPointIdx);

      if ( pStrandGeom->IsStrandDebonded(poi,strandPointIdx,pgsTypes::Straight) )
      {
         doPnt->SetDrawingStrategy(debond_strategy);
      }
      else
      {
         doPnt->SetDrawingStrategy(strategy);
      }

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

      auto doPnt = WBFL::DManip::PointDisplayObject::Create();
      doPnt->SetPosition(geomUtil::GetPoint(p),false,false);
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
      auto pTempDL = m_pDispMgr->FindDisplayList(TEMP_STRAND_LIST);
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

         auto doPnt = WBFL::DManip::PointDisplayObject::Create();
         doPnt->SetPosition(geomUtil::GetPoint(p),false,false);
         doPnt->SetID(strandPointIdx);
         doPnt->SetDrawingStrategy(strategy);

         pTempDL->AddDisplayObject(doPnt);
      }
   }
}

std::shared_ptr<WBFL::DManip::iDisplayObject> CGirderModelSectionView::GetDuctDisplayObject(IntervalIndexType intervalIdx,IntervalIndexType ptIntervalIdx, const WBFL::Geometry::Point2d& pntDuct,Float64 ductDiameter,StrandIndexType nStrands,COLORREF fillColor,COLORREF borderColor)
{
   auto compDO = WBFL::DManip::CompositeDisplayObject::Create();

   if (ptIntervalIdx < intervalIdx)
   {
      auto pntDO = WBFL::DManip::PointDisplayObject::Create();

      auto shape_draw_strategy = WBFL::DManip::ShapeDrawStrategy::Create();
      pntDO->SetDrawingStrategy(shape_draw_strategy);
      shape_draw_strategy->SetSolidFillColor(fillColor);
      shape_draw_strategy->SetSolidLineColor(borderColor);
      auto circle = std::make_shared<WBFL::Geometry::Circle>(pntDuct, ductDiameter / 2);
      shape_draw_strategy->SetShape(circle);
      compDO->AddDisplayObject(pntDO);
   }

   CString strStrands;
   strStrands.Format(_T("%d"), nStrands);

   auto textBlock = WBFL::DManip::TextBlock::Create();

   auto pntBottomCircle(pntDuct);
   pntBottomCircle.Offset(0, -ductDiameter / 2);
   textBlock->SetText(strStrands);
   textBlock->SetPosition(pntBottomCircle);
   textBlock->SetTextAlign(TA_BASELINE | TA_CENTER);
   textBlock->SetTextColor(BLACK);
   textBlock->SetBkMode(TRANSPARENT);
   textBlock->SetPointSize(FONT_POINT_SIZE);
   compDO->AddDisplayObject(textBlock);

   return compDO;
}

void CGirderModelSectionView::BuildDuctDisplayObjects(CPGSDocBase* pDoc,IBroker* pBroker,const pgsPointOfInterest& poi)
{
   // The outlines of the ducts are drawn as part of the girder cross section
   // This method adds the text labels to the ducts
   auto pDisplayList = m_pDispMgr->FindDisplayList(DUCT_LIST);
   ATLASSERT(pDisplayList);
   pDisplayList->Clear();

   const CSegmentKey& segmentKey(poi.GetSegmentKey());
   const CGirderKey& girderKey(segmentKey);

   GET_IFACE2(pBroker,IPointOfInterest,pPoi);
   Float64 Xg = pPoi->ConvertPoiToGirderCoordinate(poi);

   GET_IFACE2(pBroker, IIntervals, pIntervals);
   EventIndexType eventIdx = m_pFrame->GetEvent();
   IntervalIndexType intervalIdx = pIntervals->GetInterval(eventIdx);

   GET_IFACE2(pBroker, ISegmentTendonGeometry, pSegmentTendonGeometry);
   DuctIndexType nDucts = pSegmentTendonGeometry->GetDuctCount(segmentKey);
   for (DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++)
   {
      IntervalIndexType ptIntervalIdx = pIntervals->GetStressSegmentTendonInterval(segmentKey);
      CComPtr<IPoint2d> pntDuct;
      pSegmentTendonGeometry->GetSegmentDuctPoint(poi, ductIdx, &pntDuct);
      Float64 diameter = pSegmentTendonGeometry->GetOutsideDiameter(segmentKey, ductIdx);
      StrandIndexType nStrands = pSegmentTendonGeometry->GetTendonStrandCount(segmentKey, ductIdx);

      auto doDuct = GetDuctDisplayObject(intervalIdx, ptIntervalIdx, geomUtil::GetPoint(pntDuct), diameter, nStrands, SEGMENT_TENDON_FILL_COLOR, SEGMENT_TENDON_BORDER_COLOR);

      pDisplayList->AddDisplayObject(doDuct);
   }

   GET_IFACE2(pBroker,IGirderTendonGeometry,pTendonGeom);
   nDucts = pTendonGeom->GetDuctCount(girderKey);
   for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
   {
      if (!pTendonGeom->IsOnDuct(poi,ductIdx))
      {
         continue; // point is not within the extend of the tendon
      }

      IntervalIndexType ptIntervalIdx = pIntervals->GetStressGirderTendonInterval(girderKey, ductIdx);
      CComPtr<IPoint2d> pntDuct;
      pTendonGeom->GetGirderDuctPoint(girderKey, Xg, ductIdx, &pntDuct);

      Float64 diameter = pTendonGeom->GetOutsideDiameter(girderKey, ductIdx);

      StrandIndexType nStrands = pTendonGeom->GetTendonStrandCount(girderKey, ductIdx);

      auto doDuct = GetDuctDisplayObject(intervalIdx, ptIntervalIdx, geomUtil::GetPoint(pntDuct), diameter, nStrands, GIRDER_TENDON_FILL_COLOR, GIRDER_TENDON_BORDER_COLOR);

      pDisplayList->AddDisplayObject(doDuct);
   }
}

void CGirderModelSectionView::BuildLongReinfDisplayObjects(CPGSDocBase* pDoc,IBroker* pBroker,const pgsPointOfInterest& poi)
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   auto pDL = m_pDispMgr->FindDisplayList(LONG_REINF_LIST);
   ATLASSERT(pDL);
   pDL->Clear();

   auto strategy = WBFL::DManip::SimpleDrawPointStrategy::Create();
   strategy->SetColor(REBAR_COLOR);
   strategy->SetPointType(WBFL::DManip::PointType::Circle);

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

      auto doPnt = WBFL::DManip::PointDisplayObject::Create();
      doPnt->SetPosition(geomUtil::GetPoint(p),false,false);
      doPnt->SetID(id++);
      doPnt->SetDrawingStrategy(strategy);

      pDL->AddDisplayObject(doPnt);

      item.Release();
   }
}

void CGirderModelSectionView::BuildStrandCGDisplayObjects(CPGSDocBase* pDoc,IBroker* pBroker,const pgsPointOfInterest& poi)
{
   auto pDL = m_pDispMgr->FindDisplayList(STRAND_CG_LIST);
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
   auto cg = pStrandGeom->GetStrandCG(intervalIdx, poi, true);

   auto doPnt = WBFL::DManip::PointDisplayObject::Create(1);
   doPnt->SetPosition(cg,false,false);

   auto strategy = WBFL::DManip::TargetDrawStrategy::Create();
   CRect rc;
   GetClientRect(&rc);
   strategy->SetRadius(rc.Width()/80);
   strategy->SetFgColor(STRAND_CG_COLOR_1);
   strategy->SetBgColor(STRAND_CG_COLOR_2);

   doPnt->SetDrawingStrategy(strategy);

   // setup socket for dimension line
   auto connectable = std::dynamic_pointer_cast<WBFL::DManip::iConnectable>(doPnt);
   auto socketCGPS = connectable->AddSocket(SOCKET_CGPS,cg);

   // we don't know where the top/bottom of the dimensions are going to be
   // but we will create the sockets for them now. We will move these sockets
   // when we make the dimensions lines
   connectable->AddSocket(SOCKET_CGPS_TC, cg);
   connectable->AddSocket(SOCKET_CGPS_BC, cg);

   pDL->AddDisplayObject(doPnt);
}

void CGirderModelSectionView::BuildCGDisplayObjects(CPGSDocBase* pDoc, IBroker* pBroker, const pgsPointOfInterest& poi)
{
   auto pDL = m_pDispMgr->FindDisplayList(CG_LIST);
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
   if (intervalIdx < releaseIntervalIdx)
   {
      intervalIdx = releaseIntervalIdx;
   }

   GET_IFACE2(pBroker, ISectionProperties, pSectProp);
   Float64 Yb = pSectProp->GetY(intervalIdx, poi, pgsTypes::BottomGirder);
   Float64 Hg = pSectProp->GetHg(releaseIntervalIdx, poi); 
   // NOTE: release interval is correct for Hg. We are using the girder section coordinates
   // with Y=0 at the top of the non-composite girder. To get the elevation of the CG
   // go down Hg and then up Yb, or with the sign convention Yb-Hg

   CComPtr<IPoint2d> point;
   pSectProp->GetCentroid(intervalIdx, poi, &point);

   auto doPnt = WBFL::DManip::PointDisplayObject::Create(1);
   doPnt->SetPosition(geomUtil::GetPoint(point), false, false);

   auto strategy = WBFL::DManip::TargetDrawStrategy::Create();
   CRect rc;
   GetClientRect(&rc);
   strategy->SetRadius(rc.Width() / 80);
   strategy->SetFgColor(SECTION_CG_COLOR_1);
   strategy->SetBgColor(SECTION_CG_COLOR_2);

   doPnt->SetDrawingStrategy(strategy);

   // setup socket for dimension line
   auto connectable = std::dynamic_pointer_cast<WBFL::DManip::iConnectable>(doPnt);
   connectable->AddSocket(SOCKET_CG, geomUtil::GetPoint(point));

   Float64 cgx, cgy;
   point->Location(&cgx, &cgy);

   connectable->AddSocket(SOCKET_CGX_TOP, { cgx,0 });
   connectable->AddSocket(SOCKET_CGX_BOTTOM, { cgx, Yb - Hg });

   // we don't know where the top/bottom of the dimensions are going to be
   // but we will create the sockets for them now. We will move these sockets
   // when we make the dimensions lines
   connectable->AddSocket(SOCKET_CG_TOP, geomUtil::GetPoint(point));
   connectable->AddSocket(SOCKET_CG_BOTTOM, geomUtil::GetPoint(point));
   connectable->AddSocket(SOCKET_CGX_TOP_LEFT, geomUtil::GetPoint(point));
   connectable->AddSocket(SOCKET_CGX_TOP_RIGHT, geomUtil::GetPoint(point));
   connectable->AddSocket(SOCKET_CGX_BOTTOM_LEFT, geomUtil::GetPoint(point));
   connectable->AddSocket(SOCKET_CGX_BOTTOM_RIGHT, geomUtil::GetPoint(point));

   pDL->AddDisplayObject(doPnt);
}

void CGirderModelSectionView::BuildDimensionDisplayObjects(CPGSDocBase* pDoc, IBroker* pBroker, const pgsPointOfInterest& poi)
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

   auto pDL = m_pDispMgr->FindDisplayList(DIMENSION_LIST);
   ATLASSERT(pDL);
   pDL->Clear();

   IDType id = 0;
   auto doDimLineTopFlangeWidth = WBFL::DManip::DimensionLine::Create(id++);
   auto doDimLineBottomFlangeWidth = WBFL::DManip::DimensionLine::Create(id++);
   auto doDimLineHeight = WBFL::DManip::DimensionLine::Create(id++);
   auto doDimLineTopCGPS = WBFL::DManip::DimensionLine::Create(id++);
   auto doDimLineBottomCGPS = WBFL::DManip::DimensionLine::Create(id++);
   auto doDimLineTopCG = WBFL::DManip::DimensionLine::Create(id++);
   auto doDimLineBottomCG = WBFL::DManip::DimensionLine::Create(id++);
   auto doDimLineTopLeftCG = WBFL::DManip::DimensionLine::Create(id++);
   auto doDimLineTopRightCG = WBFL::DManip::DimensionLine::Create(id++);
   auto doDimLineBottomLeftCG = WBFL::DManip::DimensionLine::Create(id++);
   auto doDimLineBottomRightCG = WBFL::DManip::DimensionLine::Create(id++);

   // Connect the dimension lines to the sockets in the section display object
   auto pSectionDL = m_pDispMgr->FindDisplayList(SECTION_LIST);
   ATLASSERT(pSectionDL);

   auto pJointDL = m_pDispMgr->FindDisplayList(JOINT_LIST);
   ATLASSERT(pJointDL);

   // get the girder section display object
   auto doSection = pSectionDL->GetDisplayObject(0);

   auto doJoint = pJointDL->GetDisplayObject(0);

   // get it's iConnectable interface
   auto connectableSection = std::dynamic_pointer_cast<WBFL::DManip::iConnectable>(doSection);
   auto connectableJoint = std::dynamic_pointer_cast<WBFL::DManip::iConnectable>(doJoint);

   // get the sockets
   auto socketTL = connectableSection->GetSocket(SOCKET_TL, WBFL::DManip::AccessType::ByID);
   auto socketTC = connectableSection->GetSocket(SOCKET_TC, WBFL::DManip::AccessType::ByID);
   auto socketTR = connectableSection->GetSocket(SOCKET_TR, WBFL::DManip::AccessType::ByID);
   auto socketBL = connectableSection->GetSocket(SOCKET_BL, WBFL::DManip::AccessType::ByID);
   auto socketBC = connectableSection->GetSocket(SOCKET_BC, WBFL::DManip::AccessType::ByID);
   auto socketBR = connectableSection->GetSocket(SOCKET_BR, WBFL::DManip::AccessType::ByID);

   std::shared_ptr<WBFL::DManip::iSocket> socketTLJ, socketTRJ;
   if (connectableJoint)
   {
      socketTLJ = connectableJoint->GetSocket(SOCKET_TLJ, WBFL::DManip::AccessType::ByID);
      socketTRJ = connectableJoint->GetSocket(SOCKET_TRJ, WBFL::DManip::AccessType::ByID);
   }

   UINT settings = pDoc->GetGirderEditorSettings();

   std::shared_ptr<WBFL::DManip::iSocket> socketCG, socketCG_Top, socketCG_Bottom, socketCGX_Top, socketCGX_Bottom, socketCGX_TopLeft, socketCGX_TopRight, socketCGX_BottomLeft, socketCGX_BottomRight;
   if (settings & IDG_SV_GIRDER_CG)
   {
      auto pCGList = m_pDispMgr->FindDisplayList(CG_LIST);

      auto doCG = pCGList->GetDisplayObject(0);
      if (doCG)
      {
         auto connectableCG = std::dynamic_pointer_cast<WBFL::DManip::iConnectable>(doCG);
         socketCG = connectableCG->GetSocket(SOCKET_CG, WBFL::DManip::AccessType::ByID);
         socketCG_Top = connectableCG->GetSocket(SOCKET_CG_TOP, WBFL::DManip::AccessType::ByID);
         socketCG_Bottom = connectableCG->GetSocket(SOCKET_CG_BOTTOM, WBFL::DManip::AccessType::ByID);
         socketCGX_Top = connectableCG->GetSocket(SOCKET_CGX_TOP, WBFL::DManip::AccessType::ByID);
         socketCGX_Bottom = connectableCG->GetSocket(SOCKET_CGX_BOTTOM, WBFL::DManip::AccessType::ByID);
         socketCGX_TopLeft = connectableCG->GetSocket(SOCKET_CGX_TOP_LEFT, WBFL::DManip::AccessType::ByID);
         socketCGX_TopRight = connectableCG->GetSocket(SOCKET_CGX_TOP_RIGHT, WBFL::DManip::AccessType::ByID);
         socketCGX_BottomLeft = connectableCG->GetSocket(SOCKET_CGX_BOTTOM_LEFT, WBFL::DManip::AccessType::ByID);
         socketCGX_BottomRight = connectableCG->GetSocket(SOCKET_CGX_BOTTOM_RIGHT, WBFL::DManip::AccessType::ByID);
      }
   }

   std::shared_ptr<WBFL::DManip::iSocket> socketCGPS, socketCGPS_TC, socketCGPS_BC;
   if (settings & IDG_SV_SHOW_PS_CG)
   {
      auto pStrandCGList = m_pDispMgr->FindDisplayList(STRAND_CG_LIST);

      auto doCGPS = pStrandCGList->GetDisplayObject(0);
      if (doCGPS)
      {
         auto connectableCGPS = std::dynamic_pointer_cast<WBFL::DManip::iConnectable>(doCGPS);
         socketCGPS = connectableCGPS->GetSocket(SOCKET_CGPS, WBFL::DManip::AccessType::ByID);
         socketCGPS_TC = connectableCGPS->GetSocket(SOCKET_CGPS_TC, WBFL::DManip::AccessType::ByID);
         socketCGPS_BC = connectableCGPS->GetSocket(SOCKET_CGPS_BC, WBFL::DManip::AccessType::ByID);
      }
   }

   // get the connector interface from the dimension lines
   auto connectorTopFlangeWidth = std::dynamic_pointer_cast<WBFL::DManip::iConnector>(doDimLineTopFlangeWidth);
   auto connectorBottomFlangeWidth = std::dynamic_pointer_cast<WBFL::DManip::iConnector>(doDimLineBottomFlangeWidth);
   auto connectorHeight = std::dynamic_pointer_cast<WBFL::DManip::iConnector>(doDimLineHeight);
   auto connectorTopCGPS = std::dynamic_pointer_cast<WBFL::DManip::iConnector>(doDimLineTopCGPS);
   auto connectorBottomCGPS = std::dynamic_pointer_cast<WBFL::DManip::iConnector>(doDimLineBottomCGPS);
   auto connectorTopCG = std::dynamic_pointer_cast<WBFL::DManip::iConnector>(doDimLineTopCG);
   auto connectorBottomCG = std::dynamic_pointer_cast<WBFL::DManip::iConnector>(doDimLineBottomCG);
   auto connectorTopLeftCG = std::dynamic_pointer_cast<WBFL::DManip::iConnector>(doDimLineTopLeftCG);
   auto connectorTopRightCG = std::dynamic_pointer_cast<WBFL::DManip::iConnector>(doDimLineTopRightCG);
   auto connectorBottomLeftCG = std::dynamic_pointer_cast<WBFL::DManip::iConnector>(doDimLineBottomLeftCG);
   auto connectorBottomRightCG = std::dynamic_pointer_cast<WBFL::DManip::iConnector>(doDimLineBottomRightCG);

   // connect the top flange width dimension line (across the top)
   auto startPlug = connectorTopFlangeWidth->GetStartPlug();
   auto endPlug = connectorTopFlangeWidth->GetEndPlug();

   if ((IsStructuralDeck(deckType) && castDeckEventIdx < eventIdx) || socketTLJ == nullptr)
   {
      socketTL->Connect(startPlug);
   }
   else
   {
      socketTLJ->Connect(startPlug);
   }

   if ((IsStructuralDeck(deckType) && castDeckEventIdx < eventIdx) || socketTRJ == nullptr)
   {
      socketTR->Connect(endPlug);
   }
   else
   {
      socketTRJ->Connect(endPlug);
   }

   // connect the bottom flange width dimension line (across the bottom)
   startPlug = connectorBottomFlangeWidth->GetStartPlug();
   endPlug = connectorBottomFlangeWidth->GetEndPlug();
   socketBR->Connect(startPlug);
   socketBL->Connect(endPlug);
   
   // connect the height dimension line (left side)
   startPlug = connectorHeight->GetStartPlug();
   endPlug = connectorHeight->GetEndPlug();

   socketBC->Connect(startPlug);
   socketTC->Connect(endPlug);

   // connect the CG dimension line (right side)
   if ((settings & IDG_SV_GIRDER_CG) && socketCG)
   {
      startPlug = connectorTopCG->GetStartPlug();
      endPlug = connectorTopCG->GetEndPlug();

      // need to determine the top center location for the upper dimension line
      // we want x of the CG and y for the top of the girder
      auto cg = socketCG->GetPosition();
      Float64 x = cg.X();

      auto tc = socketTC->GetPosition();
      Float64 y = tc.Y();

      WBFL::Geometry::Point2d pntTC(x, y);
      socketCG_Top->SetPosition(pntTC);

      socketCG->Connect(startPlug);
      socketCG_Top->Connect(endPlug);

      startPlug = connectorBottomCG->GetStartPlug();
      endPlug = connectorBottomCG->GetEndPlug();

      // need y of the bottom center point
      auto bc = socketBC->GetPosition();
      y = bc.Y();

      WBFL::Geometry::Point2d pntBC(x, y);
      socketCG_Bottom->SetPosition(pntBC);

      socketCG_Bottom->Connect(startPlug);
      socketCG->Connect(endPlug);

      // Xcg
      auto pntTL = socketTL->GetPosition();
      auto [tlX, tlY] = pntTL.GetLocation();

      auto pntTR = socketTR->GetPosition();
      auto [trX, trY] = pntTR.GetLocation();

      y = Max(tlY, trY);

      auto pntCGX_Top = socketCGX_Top->GetPosition();
      pntCGX_Top.Y() = y;
      socketCGX_Top->SetPosition(pntCGX_Top);

      WBFL::Geometry::Point2d pntTopLeft(tlX,y);
      socketCGX_TopLeft->SetPosition(pntTopLeft);

      WBFL::Geometry::Point2d pntTopRight(trX, y);
      socketCGX_TopRight->SetPosition(pntTopRight);


      auto pntBL = socketBL->GetPosition();
      auto [blX, blY] = pntBL.GetLocation();

      auto pntBR = socketBR->GetPosition();
      auto [brX, brY] = pntBR.GetLocation();

      y = Max(blY, brY);

      auto pntCGX_Bottom = socketCGX_Bottom->GetPosition();
      pntCGX_Bottom.Y() = y;
      socketCGX_Bottom->SetPosition(pntCGX_Bottom);

      WBFL::Geometry::Point2d pntBottomLeft(blX, y);
      socketCGX_BottomLeft->SetPosition(pntBottomLeft);

      WBFL::Geometry::Point2d pntBottomRight(brX, y);
      socketCGX_BottomRight->SetPosition(pntBottomRight);

      // Top-Left corner to CG dimension line
      startPlug = connectorTopLeftCG->GetStartPlug();
      endPlug = connectorTopLeftCG->GetEndPlug();

      socketCGX_TopLeft->Connect(startPlug);
      socketCGX_Top->Connect(endPlug);

      // Top-Right corner to CG dimension line
      startPlug = connectorTopRightCG->GetStartPlug();
      endPlug = connectorTopRightCG->GetEndPlug();
      socketCGX_Top->Connect(startPlug);
      socketCGX_TopRight->Connect(endPlug);

      // Bottom left corner to CG dimension line
      startPlug = connectorBottomLeftCG->GetStartPlug();
      endPlug = connectorBottomLeftCG->GetEndPlug();

      socketCGX_Bottom->Connect(startPlug);
      socketCGX_BottomLeft->Connect(endPlug);

      // Bottom right corner to CG dimension line
      startPlug = connectorBottomRightCG->GetStartPlug();
      endPlug = connectorBottomRightCG->GetEndPlug();
      socketCGX_BottomRight->Connect(startPlug);
      socketCGX_Bottom->Connect(endPlug);

      // Draw a vertical line that connects the horizontal CG dimension leader lines
      CreateLineDisplayObject(pDL, pntCGX_Top, pntCGX_Bottom, BLACK, WBFL::DManip::LineStyleType::Solid);
   }


   // connect cg ps dimension line (bottom center to cg symbol)
   if ((settings & IDG_SV_SHOW_PS_CG) && socketCGPS)
   {
      startPlug = connectorTopCGPS->GetStartPlug();
      endPlug = connectorTopCGPS->GetEndPlug();

      // need to determine the top center location for the upper dimension line
      // we want x of the CG and y for the top of the girder
      auto cg = socketCGPS->GetPosition();
      auto x = cg.X();

      auto tc = socketTC->GetPosition();
      auto y = tc.Y();

      WBFL::Geometry::Point2d pntTC(x, y);
      socketCGPS_TC->SetPosition(pntTC);

      socketCGPS->Connect(startPlug);
      socketCGPS_TC->Connect(endPlug);

      startPlug = connectorBottomCGPS->GetStartPlug();
      endPlug = connectorBottomCGPS->GetEndPlug();

      // need y of the bottom center point
      auto bc = socketBC->GetPosition();
      y = bc.Y();

      WBFL::Geometry::Point2d pntBC(x, y);
      socketCGPS_BC->SetPosition(pntBC);

      socketCGPS_BC->Connect(startPlug);
      socketCGPS->Connect(endPlug);
   }

   // set the text labels on the dimension lines
   Float64 twLeft, twRight;
   Float64 top_width;
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

      top_width = twLeft + twRight;
   }
   else
   {
      // this is an analysis section model so we are showing the effective top flange width, centered on the girder
      // this is NOT the tributary width
      top_width = pSectProp->GetEffectiveFlangeWidth(poi);
      twLeft = top_width / 2;
      twRight = twLeft;
   }

   Float64 bwLeft, bwRight;
   Float64 bottom_width = pGirder->GetBottomWidth(poi, &bwLeft, &bwRight);
   Float64 height = pSectProp->GetHg(intervalIdx, poi);
   Float64 Yt = pSectProp->GetY(intervalIdx, poi, intervalIdx < compositeIntervalIdx ? pgsTypes::TopGirder : pgsTypes::TopDeck);
   Float64 Yb = pSectProp->GetY(intervalIdx, poi, pgsTypes::BottomGirder);
   Float64 Xl = pSectProp->GetXleft(intervalIdx, poi);
   Float64 Xr = pSectProp->GetXright(intervalIdx, poi);

   Float64 top_left_dim, top_right_dim, bottom_left_dim, bottom_right_dim;
   if ((twLeft + twRight) < bottom_width)
   {
      // bottom is wider than top

      // Xl and Xr are the left/right dimension for the bounding box measured from the CG of the girder section
      // bwLeft and bwRight are the left and right bottom width measured from the nominal centerline of the girder section
      // We want to dimension the left and right bottom width measured from the CG of the girder section
      if (Xl < Xr)
      {
         bottom_left_dim = Xl;
         bottom_right_dim = (bwLeft + bwRight) - Xl;
      }
      else
      {
         bottom_right_dim = Xr;
         bottom_left_dim = (bwLeft + bwRight) - Xr;
      }
      top_left_dim = twLeft + 0.5*(Xl - Xr);
      top_right_dim = twRight - 0.5*(Xl - Xr);
   }
   else
   {
      // top is wider than bottom (or top and bottom are same width)

      // Xl and Xr are the left/right dimension for the bounding box measured from the CG of the girder section
      // twLeft and twRight are the left and right top width measured from the nominal centerline of the girder section
      // We want to dimension the left and right top width measured from the CG of the girder section
      if (Xl < Xr)
      {
         top_left_dim = Xl;
         top_right_dim = (twLeft + twRight) - Xl;
      }
      else
      {
         top_right_dim = Xr;
         top_left_dim = (twLeft + twRight) - Xr;
      }
      bottom_left_dim = bwLeft + 0.5*(Xl - Xr);
      bottom_right_dim = bwRight - 0.5*(Xl - Xr);
   }


   GET_IFACE2(pBroker, IEAFDisplayUnits, pDisplayUnits);
   const WBFL::Units::LengthData& length_unit = pDisplayUnits->GetComponentDimUnit();

   auto textBlock = WBFL::DManip::TextBlock::Create();
   textBlock->SetPointSize(FONT_POINT_SIZE);
   auto strDim = FormatDimension(top_width, length_unit);
   textBlock->SetText(strDim);
   doDimLineTopFlangeWidth->SetTextBlock(textBlock);

   // move the top flange width line up
   doDimLineTopFlangeWidth->SetWitnessLength(3 * twip_offset / 2);

   textBlock = WBFL::DManip::TextBlock::Create();
   textBlock->SetPointSize(FONT_POINT_SIZE);
   strDim = FormatDimension(bottom_width, length_unit);
   textBlock->SetText(strDim);
   doDimLineBottomFlangeWidth->SetTextBlock(textBlock);
   doDimLineBottomFlangeWidth->SetWitnessLength(3 * twip_offset / 2);

   textBlock = WBFL::DManip::TextBlock::Create();
   textBlock->SetPointSize(FONT_POINT_SIZE);
   strDim = FormatDimension(height, length_unit);
   textBlock->SetText(strDim);
   textBlock->SetBkMode(TRANSPARENT);
   doDimLineHeight->SetTextBlock(textBlock);

   // adjust the witness line
   auto pntTC = socketTC->GetPosition();
   auto X = pntTC.X();
   long tx0, ty0;
   long tx1, ty1;
   m_pCoordinateMap->WPtoTP(X, 0, &tx0, &ty0);
   m_pCoordinateMap->WPtoTP(twLeft - X, 0, &tx1, &ty1);
   doDimLineHeight->SetWitnessLength(tx1 - tx0 + twip_offset);

   if ((settings & IDG_SV_GIRDER_CG) && socketCG)
   {
      textBlock = WBFL::DManip::TextBlock::Create();
      textBlock->SetPointSize(FONT_POINT_SIZE);
      strDim = FormatDimension(Yt, length_unit);
      textBlock->SetText(strDim);
      doDimLineTopCG->SetTextBlock(textBlock);

      textBlock = WBFL::DManip::TextBlock::Create();
      textBlock->SetPointSize(FONT_POINT_SIZE);
      strDim = FormatDimension(Yb, length_unit);
      textBlock->SetText(strDim);
      doDimLineBottomCG->SetTextBlock(textBlock);

      textBlock = WBFL::DManip::TextBlock::Create();
      textBlock->SetPointSize(FONT_POINT_SIZE);
      strDim = FormatDimension(top_left_dim, length_unit);
      textBlock->SetText(strDim);
      doDimLineTopLeftCG->SetTextBlock(textBlock);

      textBlock = WBFL::DManip::TextBlock::Create();
      textBlock->SetPointSize(FONT_POINT_SIZE);
      strDim = FormatDimension(top_right_dim, length_unit);
      textBlock->SetText(strDim);
      doDimLineTopRightCG->SetTextBlock(textBlock);

      textBlock = WBFL::DManip::TextBlock::Create();
      textBlock->SetPointSize(FONT_POINT_SIZE);
      strDim = FormatDimension(bottom_left_dim, length_unit);
      textBlock->SetText(strDim);
      doDimLineBottomLeftCG->SetTextBlock(textBlock);

      textBlock = WBFL::DManip::TextBlock::Create();
      textBlock->SetPointSize(FONT_POINT_SIZE);
      strDim = FormatDimension(bottom_right_dim, length_unit);
      textBlock->SetText(strDim);
      doDimLineBottomRightCG->SetTextBlock(textBlock);

      auto cg = socketCG->GetPosition();
      auto x1 = cg.X();

      // adjust the witness lines on the vertical dimensions
      Float64 x2 = Max(twRight, bottom_width / 2);

      m_pCoordinateMap->WPtoTP(x1, 0, &tx0, &ty0);
      m_pCoordinateMap->WPtoTP(x2 - x1, 0, &tx1, &ty1);

      doDimLineTopCG->SetWitnessLength(-tx1 + tx0 - twip_offset);
      doDimLineBottomCG->SetWitnessLength(-tx1 + tx0 - twip_offset);
   }

   if ( (settings & IDG_SV_SHOW_PS_CG) && socketCGPS )
   {
      GET_IFACE2(pBroker,IStrandGeometry,pStrandGeometry);

      Float64 ecc = pStrandGeometry->GetEccentricity(intervalIdx, poi, true).Y();
      Float64 yps = pSectProp->GetY(intervalIdx,poi,pgsTypes::BottomGirder) - ecc;

      textBlock = WBFL::DManip::TextBlock::Create();
      textBlock->SetPointSize(FONT_POINT_SIZE);
      strDim = FormatDimension(yps,length_unit);
      textBlock->SetText(strDim);
      doDimLineBottomCGPS->SetTextBlock(textBlock);

      yps = height - yps;

      textBlock = WBFL::DManip::TextBlock::Create();
      textBlock->SetPointSize(FONT_POINT_SIZE);
      strDim = FormatDimension(yps, length_unit);
      textBlock->SetText(strDim);
      doDimLineTopCGPS->SetTextBlock(textBlock);

      auto cg = socketCGPS->GetPosition();
      auto x1 = cg.X();

      // adjust the witness lines on the vertical dimensions
      Float64 x2 = Max(twLeft, bottom_width / 2);

      m_pCoordinateMap->WPtoTP(x1, 0, &tx0, &ty0);
      m_pCoordinateMap->WPtoTP(x2-x1, 0, &tx1, &ty1);
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
         pDL->AddDisplayObject(doDimLineTopLeftCG);
         pDL->AddDisplayObject(doDimLineTopRightCG);

         pDL->AddDisplayObject(doDimLineBottomLeftCG);
         pDL->AddDisplayObject(doDimLineBottomRightCG);
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
         BuildDimensionDisplayObjects(pDoc, pBroker, poi);
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
   auto display_list = m_pDispMgr->FindDisplayList(PROPERTIES_LIST);

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

void CGirderModelSectionView::CreateLineDisplayObject(std::shared_ptr<WBFL::DManip::iDisplayList> pDL, const WBFL::Geometry::Point2d& start, const WBFL::Geometry::Point2d& end,COLORREF color,WBFL::DManip::LineStyleType lineStyle)
{
   // create the line display object
   auto doLine = WBFL::DManip::LineDisplayObject::Create();

   // create the line drawing strategy
   auto line_draw_strategy = WBFL::DManip::SimpleDrawLineStrategy::Create();
   line_draw_strategy->SetColor(color);
   line_draw_strategy->SetLineStyle(lineStyle);

   // associate the drawing strategy with the display object
   doLine->SetDrawLineStrategy(line_draw_strategy);

   // get the plugs at the end of the line
   auto centerline_connector = std::dynamic_pointer_cast<WBFL::DManip::iConnector>(doLine);
   auto startPlug = centerline_connector->GetStartPlug();
   auto endPlug = centerline_connector->GetEndPlug();

   // create a point display object at the start of the line
   auto doStart = WBFL::DManip::PointDisplayObject::Create();
   doStart->SetPosition(start, false, false);

   // update the draw point strategy so that the point isn't displayed
   auto dps = doStart->GetDrawingStrategy();
   auto draw_point_strategy = std::dynamic_pointer_cast<WBFL::DManip::SimpleDrawPointStrategy>(dps);
   if (draw_point_strategy) draw_point_strategy->SetPointType(WBFL::DManip::PointType::None);

   // add a socket at the start for the line's plug
   auto start_connectable = std::dynamic_pointer_cast<WBFL::DManip::iConnectable>(doStart);
   auto start_socket = start_connectable->AddSocket(0, start);

   // create a point display object at the end of the line
   auto doEnd = WBFL::DManip::PointDisplayObject::Create();
   doEnd->SetPosition(end, false, false);

   // update the draw point strategy so that the point isn't displayed
   dps = doEnd->GetDrawingStrategy();
   draw_point_strategy = std::dynamic_pointer_cast<WBFL::DManip::SimpleDrawPointStrategy>(dps);
   if (draw_point_strategy) draw_point_strategy->SetPointType(WBFL::DManip::PointType::None);

   // add a socket at the end for the line's plug
   auto end_connectable = std::dynamic_pointer_cast<WBFL::DManip::iConnectable>(doEnd);
   auto end_socket = end_connectable->AddSocket(0, end);

   // connect the line's plugs into the sockets
   DWORD dwCookie;
   dwCookie = start_connectable->Connect(0, WBFL::DManip::AccessType::ByID, startPlug);
   dwCookie = end_connectable->Connect(0, WBFL::DManip::AccessType::ByID, endPlug);

   // add the display objects to the display list
   pDL->AddDisplayObject(doStart);
   pDL->AddDisplayObject(doEnd);
   pDL->AddDisplayObject(doLine);
}