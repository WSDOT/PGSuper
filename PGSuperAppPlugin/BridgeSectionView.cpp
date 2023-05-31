///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2023  Washington State Department of Transportation
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

// BridgeSectionView.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "PGSuperApp.h"
#include "PGSuperDocBase.h"
#include "BridgeSectionView.h"
#include "SlabDisplayObjectEvents.h"
#include "AlignmentDisplayObjectEvents.h"
#include "BridgePlanView.h"

#include "PGSuperUnits.h"
#include "PGSuperColors.h"

#include <PgsExt\BridgeDescription2.h>
#include <PgsExt\Helpers.h>

#include <IFace\DrawBridgeSettings.h>
#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <IFace\Alignment.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\EditByUI.h>
#include <IFace\Intervals.h>

#include "GirderDisplayObjectEvents.h"
#include "TrafficBarrierDisplayObjectEvents.h"

#include <DManip\DManip.h>
#include <DManipTools\DManipTools.h>

#include <Materials/Materials.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// Display List Identifiers
#define TITLE_DISPLAY_LIST             0
#define GIRDER_DISPLAY_LIST            1
#define JOINT_DISPLAY_LIST             2
#define SLAB_DISPLAY_LIST              3
#define OVERLAY_DISPLAY_LIST           4
#define DIMENSION_DISPLAY_LIST         5
#define GIRDER_LABEL_DISPLAY_LIST      6
#define TRAFFIC_BARRIER_DISPLAY_LIST   7
#define ALIGNMENT_DISPLAY_LIST         8
#define RW_CROSS_SECTION_DISPLAY_LIST  9

#define LEFT_CURB_SOCKET         100
#define RIGHT_CURB_SOCKET        200
#define LEFT_SLAB_EDGE_SOCKET    300
#define RIGHT_SLAB_EDGE_SOCKET   400
#define LEFT_OVERHANG_SOCKET     500
#define RIGHT_OVERHANG_SOCKET    600
#define LEFT_EXT_SW_SOCKET       700
#define LEFT_INT_SW_SOCKET       701
#define RIGHT_EXT_SW_SOCKET      702
#define RIGHT_INT_SW_SOCKET      703
#define LEFT_INT_OVERLAY_SOCKET     710
#define RIGHT_INT_OVERLAY_SOCKET    711

#define ALIGNMENT_ID -200

inline void GetGirderSection(IShape* pShape, IGirderSection** ppSection)
{
   CComQIPtr<IGirderSection> section(pShape);
   if (!section)
   {
      CComQIPtr<ICompositeShape> compShape(pShape);
      if (compShape)
      {
         CComPtr<ICompositeShapeItem> shapeItem;
         compShape->get_Item(0, &shapeItem);
         CComPtr<IShape> gdrShape;
         shapeItem->get_Shape(&gdrShape);
         gdrShape.QueryInterface(ppSection);
      }
   }
   else
   {
      section.CopyTo(ppSection);
   }
}

/////////////////////////////////////////////////////////////////////////////
// CBridgeSectionView

IMPLEMENT_DYNCREATE(CBridgeSectionView, CBridgeViewPane)

CBridgeSectionView::CBridgeSectionView()
{
}

CBridgeSectionView::~CBridgeSectionView()
{
}


BEGIN_MESSAGE_MAP(CBridgeSectionView, CBridgeViewPane)
	//{{AFX_MSG_MAP(CBridgeSectionView)
	ON_COMMAND(ID_EDIT_DECK, OnEditDeck)
	ON_COMMAND(ID_VIEWSETTINGS, OnViewSettings)
	ON_WM_KEYDOWN()
	ON_WM_MOUSEWHEEL()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

bool CBridgeSectionView::IsDeckSelected()
{
   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   DisplayObjectContainer displayObjects;
   dispMgr->GetSelectedObjects(&displayObjects);

   ATLASSERT(displayObjects.size() == 0 || displayObjects.size() == 1 );

   if ( displayObjects.size() == 0 )
   {
      return false;
   }

   CComPtr<iDisplayObject> pDO = displayObjects.front().m_T;

   IDType ID = pDO->GetID();
   if ( ID == DECK_ID )
   {
      return true;
   }

   return false;
}

void CBridgeSectionView::SelectPier(PierIndexType pierIdx,bool bSelect)
{
   // sort of a dummy function to clear the selection in this view
   // when a pier is selected in another view
   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);
   dispMgr->ClearSelectedObjects();
}

void CBridgeSectionView::SelectSpan(SpanIndexType spanIdx,bool bSelect)
{
   // sort of a dummy function to clear the selection in this view
   // when a span is selected in another view
   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);
   dispMgr->ClearSelectedObjects();
}

bool CBridgeSectionView::GetSelectedGirder(CGirderKey* pGirderKey)
{
   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   DisplayObjectContainer displayObjects;
   dispMgr->GetSelectedObjects(&displayObjects);

   ATLASSERT(displayObjects.size() == 0 || displayObjects.size() == 1 );

   if ( displayObjects.size() == 0 )
   {
      return false;
   }

   CComPtr<iDisplayObject> pDO = displayObjects.front().m_T;

   // girder IDs are positive values
   IDType ID = pDO->GetID();
   if ( ID == INVALID_ID )
   {
      return false;
   }

   // do a reverse search in the map (look for values to get the key)
   GirderIDCollection::iterator iter;
   for ( iter = m_GirderIDs.begin(); iter != m_GirderIDs.end(); iter++ )
   {
      GirderIDCollection::value_type map = *iter;
      if ( map.second == ID )
      {
         // ID found
         *pGirderKey = map.first;
         return true;
      }
   }

   return false;
}

void CBridgeSectionView::SelectGirder(const CGirderKey& girderKey,bool bSelect)
{
   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   GirderIDCollection::iterator found = m_GirderIDs.find(girderKey);
   if ( found == m_GirderIDs.end() )
   {
      dispMgr->ClearSelectedObjects();
      return;
   }

   IDType ID = (*found).second;

   CComPtr<iDisplayObject> pDO;
   dispMgr->FindDisplayObject(ID,GIRDER_DISPLAY_LIST,atByID,&pDO);

   if ( pDO )
   {
      dispMgr->SelectObject(pDO,bSelect);
   }
   else
   {
      dispMgr->ClearSelectedObjects();
   }
}

void CBridgeSectionView::SelectDeck(bool bSelect)
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

void CBridgeSectionView::SelectTrafficBarrier(pgsTypes::TrafficBarrierOrientation orientation, bool bSelect)
{
   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   CComPtr<iDisplayObject> pDO;
   dispMgr->FindDisplayObject(orientation == pgsTypes::tboLeft ? LEFT_TRAFFIC_BARRIER_ID : RIGHT_TRAFFIC_BARRIER_ID, TRAFFIC_BARRIER_DISPLAY_LIST, atByID, &pDO);

   if (pDO)
   {
      dispMgr->SelectObject(pDO, bSelect);
   }
   else
   {
      dispMgr->ClearSelectedObjects();
   }
}

void CBridgeSectionView::SelectAlignment(bool bSelect)
{
   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   CComPtr<iDisplayList> displayList;
   dispMgr->FindDisplayList(RW_CROSS_SECTION_DISPLAY_LIST, &displayList);

   CComPtr<iDisplayObject> pDO;
   dispMgr->FindDisplayObject(ALIGNMENT_ID, RW_CROSS_SECTION_DISPLAY_LIST, atByID, &pDO);

   if (pDO)
   {
      dispMgr->SelectObject(pDO, bSelect);
   }
   else
   {
      dispMgr->ClearSelectedObjects();
   }
}

void CBridgeSectionView::SelectTemporarySupport(bool bSelect)
{
   // sort of a dummy function to clear the selection in this view
   // when a temporary support is selected in another view
   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);
   dispMgr->ClearSelectedObjects();
}

/////////////////////////////////////////////////////////////////////////////
// CBridgeSectionView drawing
void CBridgeSectionView::OnInitialUpdate()
{
   EnableToolTips();
   CBridgeViewPane::OnInitialUpdate();
}

void CBridgeSectionView::BuildDisplayLists()
{
   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   dispMgr->EnableLBtnSelect(TRUE);
   dispMgr->EnableRBtnSelect(TRUE);
   dispMgr->SetSelectionLineColor(SELECTED_OBJECT_LINE_COLOR);
   dispMgr->SetSelectionFillColor(SELECTED_OBJECT_FILL_COLOR);

   CBridgeViewPane::SetMappingMode(DManip::Isotropic);

   // Setup display lists

   CComPtr<iDisplayList> alignment_list;
   ::CoCreateInstance(CLSID_DisplayList,nullptr,CLSCTX_ALL,IID_iDisplayList,(void**)&alignment_list);
   alignment_list->SetID(ALIGNMENT_DISPLAY_LIST);
   dispMgr->AddDisplayList(alignment_list);

   CComPtr<iDisplayList> girder_label_list;
   ::CoCreateInstance(CLSID_DisplayList, nullptr, CLSCTX_ALL, IID_iDisplayList, (void**)&girder_label_list);
   girder_label_list->SetID(GIRDER_LABEL_DISPLAY_LIST);
   dispMgr->AddDisplayList(girder_label_list);

   CComPtr<iDisplayList> dim_line_list;
   ::CoCreateInstance(CLSID_DisplayList,nullptr,CLSCTX_ALL,IID_iDisplayList,(void**)&dim_line_list);
   dim_line_list->SetID(DIMENSION_DISPLAY_LIST);
   dispMgr->AddDisplayList(dim_line_list);

   CComPtr<iDisplayList> rwxs_line_list;
   ::CoCreateInstance(CLSID_DisplayList,nullptr,CLSCTX_ALL,IID_iDisplayList,(void**)&rwxs_line_list);
   rwxs_line_list->SetID(RW_CROSS_SECTION_DISPLAY_LIST);
   dispMgr->AddDisplayList(rwxs_line_list);

   CComPtr<iDisplayList> title_list;
   ::CoCreateInstance(CLSID_DisplayList,nullptr,CLSCTX_ALL,IID_iDisplayList,(void**)&title_list);
   title_list->SetID(TITLE_DISPLAY_LIST);
   dispMgr->AddDisplayList(title_list);

   CComPtr<iDisplayList> girder_list;
   ::CoCreateInstance(CLSID_DisplayList, nullptr, CLSCTX_ALL, IID_iDisplayList, (void**)&girder_list);
   girder_list->SetID(GIRDER_DISPLAY_LIST);
   dispMgr->AddDisplayList(girder_list);

   CComPtr<iDisplayList> traffic_barrier_list;
   ::CoCreateInstance(CLSID_DisplayList,nullptr,CLSCTX_ALL,IID_iDisplayList,(void**)&traffic_barrier_list);
   traffic_barrier_list->SetID(TRAFFIC_BARRIER_DISPLAY_LIST);
   dispMgr->AddDisplayList(traffic_barrier_list);

   CComPtr<iDisplayList> slab_list;
   ::CoCreateInstance(CLSID_DisplayList,nullptr,CLSCTX_ALL,IID_iDisplayList,(void**)&slab_list);
   slab_list->SetID(SLAB_DISPLAY_LIST);
   dispMgr->AddDisplayList(slab_list);

   CComPtr<iDisplayList> overlay_list;
   ::CoCreateInstance(CLSID_DisplayList,nullptr,CLSCTX_ALL,IID_iDisplayList,(void**)&overlay_list);
   overlay_list->SetID(OVERLAY_DISPLAY_LIST);
   dispMgr->AddDisplayList(overlay_list);

   CComPtr<iDisplayList> joint_list;
   ::CoCreateInstance(CLSID_DisplayList, nullptr, CLSCTX_ALL, IID_iDisplayList, (void**)&joint_list);
   joint_list->SetID(JOINT_DISPLAY_LIST);
   dispMgr->AddDisplayList(joint_list);

   // OnInitialUpdate eventually calls OnUpdate... OnUpdate calls the
   // following two methods so there isn't any need to call them here
   //UpdateDisplayObjects();
   //UpdateDrawingScale();
}

/////////////////////////////////////////////////////////////////////////////
// CBridgeSectionView diagnostics

#ifdef _DEBUG
void CBridgeSectionView::AssertValid() const
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
	CBridgeViewPane::AssertValid();
}

void CBridgeSectionView::Dump(CDumpContext& dc) const
{
	CBridgeViewPane::Dump(dc);
}

CString DumpPoints(IShape* pShape)
{
   CComPtr<IPoint2dCollection> points;
   pShape->get_PolyPoints(&points);

   CollectionIndexType nPoints;
   points->get_Count(&nPoints);

   CString strDump;
   for ( CollectionIndexType i = 0; i < nPoints; i++ )
   {
      CComPtr<IPoint2d> point;
      points->get_Item(i,&point);

      Float64 x,y;
      point->get_X(&x);
      point->get_Y(&y);

      CString str;
      str.Format(_T("%f, %f\r\n"),WBFL::Units::ConvertFromSysUnits(x,WBFL::Units::Measure::Feet),WBFL::Units::ConvertFromSysUnits(y,WBFL::Units::Measure::Feet));
      strDump += str;
   }

   return strDump;
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CBridgeSectionView message handlers

void CBridgeSectionView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
   // override default CBridgePlanView::OnUpdate()
   CDisplayView::OnUpdate(pSender,lHint,pHint);

   if ( lHint == HINT_BRIDGECHANGED )
   {
      CComPtr<IBroker> pBroker;
      EAFGetBroker(&pBroker);
      GET_IFACE2(pBroker,IBridge,pBridge);
      Float64 first_station = pBridge->GetPierStation(0);
      Float64 last_station  = pBridge->GetPierStation(pBridge->GetPierCount()-1);
      Float64 cut_station = m_pFrame->GetCurrentCutLocation();

      if ( !InRange(first_station,cut_station,last_station) )
      {
         m_pFrame->InvalidateCutLocation();
      }
   }

	if ( (lHint == 0)                              || 
        (lHint == HINT_BRIDGECHANGED)             || 
        (lHint == HINT_GIRDERFAMILYCHANGED)       ||
        (lHint == HINT_UNITSCHANGED)              ||
        (lHint == HINT_BRIDGEVIEWSETTINGSCHANGED) || 
        (lHint == HINT_BRIDGEVIEWSECTIONCUTCHANGED) ||
        (lHint == HINT_SPECCHANGED) ||
        (lHint == HINT_GIRDERLABELFORMATCHANGED)
        )
   {
      UpdateDisplayObjects();
      UpdateDrawingScale();
   }
   else if ( lHint == HINT_GIRDERCHANGED )
   {
      UpdateDisplayObjects();
      UpdateDrawingScale();
      UpdateGirderTooltips();
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
      case CSelection::Segment:
      case CSelection::ClosureJoint:
         this->SelectGirder( CSegmentKey(pSelection->GroupIdx,pSelection->GirderIdx,INVALID_INDEX),true);
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

      case CSelection::LeftRailingSystem:
         this->SelectTrafficBarrier(pgsTypes::tboLeft, true);
         break;

      case CSelection::RightRailingSystem:
         this->SelectTrafficBarrier(pgsTypes::tboRight, true);
         break;

      case CSelection::TemporarySupport:
         this->SelectTemporarySupport(true);
         break;

      default:
         ATLASSERT(FALSE); // is there a new type of object to be selected?
         this->ClearSelection();
         break;
      }
   }
}

void CBridgeSectionView::HandleLButtonDown(UINT nFlags, CPoint logPoint)
{
   CBridgeModelViewChildFrame* pFrame = GetFrame();
   pFrame->ClearSelection();
}

void CBridgeSectionView::HandleLButtonDblClk(UINT nFlags, CPoint point) 
{
   GetFrame()->PostMessage(WM_COMMAND,ID_PROJECT_BRIDGEDESC,0);
}

void CBridgeSectionView::HandleContextMenu(CWnd* pWnd,CPoint logPoint)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CPGSDocBase* pDoc = (CPGSDocBase*)GetDocument();
   CEAFMenu* pMenu = CEAFMenu::CreateContextMenu(pDoc->GetPluginCommandManager());
   pMenu->LoadMenu(IDR_BRIDGE_XSECTION_CTX,nullptr);

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

   const std::map<IDType,IBridgeSectionViewEventCallback*>& callbacks = pDoc->GetBridgeSectionViewCallbacks();
   std::map<IDType,IBridgeSectionViewEventCallback*>::const_iterator callbackIter(callbacks.begin());
   std::map<IDType,IBridgeSectionViewEventCallback*>::const_iterator callbackIterEnd(callbacks.end());
   for ( ; callbackIter != callbackIterEnd; callbackIter++ )
   {
      IBridgeSectionViewEventCallback* pCallback = callbackIter->second;
      pCallback->OnBackgroundContextMenu(pMenu);
   }


   pMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, logPoint.x, logPoint.y, this);
   delete pMenu;
}

void CBridgeSectionView::OnEditDeck() 
{
   ((CPGSDocBase*)GetDocument())->EditBridgeDescription(EBD_DECK);
}

void CBridgeSectionView::OnViewSettings() 
{
   ((CPGSDocBase*)GetDocument())->EditBridgeViewSettings(VS_BRIDGE_SECTION);
}

void CBridgeSectionView::UpdateGirderTooltips()
{
   CPGSDocBase* pDoc = (CPGSDocBase*)GetDocument();
   CComPtr<IBroker> pBroker;
   pDoc->GetBroker(&pBroker);

   GET_IFACE2_NOCHECK(pBroker, IEAFDisplayUnits, pDisplayUnits);
   GET_IFACE2_NOCHECK(pBroker, IStrandGeometry, pStrandGeom);
   GET_IFACE2_NOCHECK(pBroker, IMaterials, pMaterial);
   GET_IFACE2_NOCHECK(pBroker, IBridge, pBridge);
   GET_IFACE2_NOCHECK(pBroker, IPointOfInterest, pPoi);
   GET_IFACE2_NOCHECK(pBroker, IGirder, pGirder);

   GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   CComPtr<iDisplayList> girder_list;
   dispMgr->FindDisplayList(GIRDER_DISPLAY_LIST, &girder_list);
   GroupIndexType grpIdx = GetGroupIndex();

   std::vector<pgsPointOfInterest> vPoi = GetPointsOfInterest();

   for (const auto& poi : vPoi)
   {
      const CSegmentKey& segmentKey(poi.GetSegmentKey());

      GirderIDCollection::iterator found = m_GirderIDs.find(segmentKey);
      if ( found == m_GirderIDs.end() )
      {
         continue;
      }

      IDType ID = (*found).second;

      CComPtr<iDisplayObject> pDO;
      girder_list->FindDisplayObject(ID,&pDO);

      if ( !pDO )
      {
         continue;
      }

      const CPrecastSegmentData* pSegment = pBridgeDesc->GetGirderGroup(segmentKey.groupIndex)->GetGirder(segmentKey.girderIndex)->GetSegment(segmentKey.segmentIndex);

      CString strMsg1(_T("Double click to edit.\nRight click for more options."));

      Float64 fc  = pSegment->Material.Concrete.Fc;
      Float64 fci = pSegment->Material.Concrete.Fci;

      CString strMsg2;
      strMsg2.Format(_T("\n\nGirder: %s\n%s\nf'ci: %s\nf'c: %s"),
                     pBridgeDesc->GetGirderGroup(segmentKey.groupIndex)->GetGirder(segmentKey.girderIndex)->GetGirderName(),
                     lrfdConcreteUtil::GetTypeName((WBFL::Materials::ConcreteType)pMaterial->GetSegmentConcreteType(segmentKey),true).c_str(),
                     FormatDimension(fci,pDisplayUnits->GetStressUnit()),
                     FormatDimension(fc, pDisplayUnits->GetStressUnit())
                    );

      const auto* pStraightStrand = pMaterial->GetStrandMaterial(segmentKey, pgsTypes::Straight);
      const auto* pHarpedStrand = pMaterial->GetStrandMaterial(segmentKey, pgsTypes::Harped);
      const auto* pTempStrand = pMaterial->GetStrandMaterial(segmentKey,pgsTypes::Temporary);

      StrandIndexType Ns = pStrandGeom->GetStrandCount(segmentKey, pgsTypes::Straight);
      StrandIndexType Nh = pStrandGeom->GetStrandCount(segmentKey, pgsTypes::Harped);
      StrandIndexType Nt = pStrandGeom->GetStrandCount(segmentKey, pgsTypes::Temporary);
      StrandIndexType Nsd = pStrandGeom->GetNumDebondedStrands(segmentKey, pgsTypes::Straight, pgsTypes::dbetEither);

      std::_tstring harp_type(LABEL_HARP_TYPE(pStrandGeom->GetAreHarpedStrandsForcedStraight(segmentKey)));

      CString strMsg3;
      if (Nsd == 0)
      {
         strMsg3.Format(_T("\n\nStraight Strands\n%s\n# Straight: %2d"), pStraightStrand->GetName().c_str(), Ns);
      }
      else
      {
         strMsg3.Format(_T("\n\nStraight Strands\n%s\n# Straight: %2d (%2d Debonded)"), pStraightStrand->GetName().c_str(), Ns, Nsd);
      }
      CString strHarped;
      strHarped.Format(_T("\n\n%s Strands\n%s\n# %s: %2d"), harp_type.c_str(), pHarpedStrand->GetName().c_str(), harp_type.c_str(), Nh);
      strMsg3 += strHarped;

      if (pStrandGeom->GetMaxStrands(segmentKey, pgsTypes::Temporary) != 0)
      {
         CString strTemp;
         strTemp.Format(_T("\n\nTemporary Strands\n%s\n# Temporary: %2d"), pTempStrand->GetName().c_str(), Nt);
         strMsg3 += strTemp;
      }

      CString strMsg4;
      if (pBridge->GetDeckType() != pgsTypes::sdtNone && pBridge->GetHaunchInputDepthType() == pgsTypes::hidACamber)
      {
         // Slab Offset
         PierIndexType startPierIdx, endPierIdx;
         pBridge->GetGirderGroupPiers(segmentKey.groupIndex, &startPierIdx, &endPierIdx);

         Float64 startOffset = pBridge->GetSlabOffset(segmentKey,pgsTypes::metStart);
         Float64 endOffset   = pBridge->GetSlabOffset(segmentKey,pgsTypes::metEnd);

         CSpanKey spanKey;
         Float64 Xspan;
         pPoi->ConvertPoiToSpanPoint(poi, &spanKey, &Xspan);

         Float64 fillet = pBridge->GetFillet();

         strMsg4.Format(_T("\n\nSlab Offset\nStart: %s\nEnd: %s\nFillet: %s"),
            FormatDimension(startOffset, pDisplayUnits->GetComponentDimUnit()),
            FormatDimension(endOffset, pDisplayUnits->GetComponentDimUnit()),
            FormatDimension(fillet, pDisplayUnits->GetComponentDimUnit())
         );
      }

      CString strMsg = strMsg1 + strMsg2 + strMsg3 + strMsg4;

      // Precamber
      Float64 precamber = pGirder->GetPrecamber(segmentKey);
      if (pGirder->CanPrecamber(segmentKey) && !IsZero(precamber))
      {
         CString strPrecamber;
         strPrecamber.Format(_T("\n\nPrecamber: %s"), FormatDimension(precamber, pDisplayUnits->GetComponentDimUnit()));
         strMsg += strPrecamber;
      }

      // Top Flange Thickening
      pgsTypes::TopFlangeThickeningType tftType = pGirder->GetTopFlangeThickeningType(segmentKey);
      if (pGirder->CanTopFlangeBeLongitudinallyThickened(segmentKey) && tftType != pgsTypes::tftNone)
      {
         Float64 tft = pGirder->GetTopFlangeThickening(segmentKey);
         CString strTFT;
         strTFT.Format(_T("\n\nTop Flange Thickening: %s at %s"), FormatDimension(tft, pDisplayUnits->GetComponentDimUnit()), (tftType == pgsTypes::tftEnds ? _T("Ends") : _T("Middle")));
         strMsg += strTFT;
      }

      // Top Width
      if (IsTopWidthSpacing(pIBridgeDesc->GetGirderSpacingType()))
      {
         const CSplicedGirderData* pSplicedGirder = pIBridgeDesc->GetGirder(segmentKey);
         Float64 wLeft, wRight;
         Float64 topWidthStart = pSplicedGirder->GetTopWidth(pgsTypes::metStart, &wLeft, &wRight);
         Float64 topWidthEnd   = pSplicedGirder->GetTopWidth(pgsTypes::metEnd, &wLeft, &wRight);

         CString strTopWidth;
         if (IsEqual(topWidthStart, topWidthEnd))
         {
            strTopWidth.Format(_T("\n\nTop Width: %s"), FormatDimension(topWidthStart, pDisplayUnits->GetXSectionDimUnit()));
         }
         else
         {
            strTopWidth.Format(_T("\n\nTop Width: Start %s, End %s"), FormatDimension(topWidthStart, pDisplayUnits->GetXSectionDimUnit()), FormatDimension(topWidthEnd, pDisplayUnits->GetXSectionDimUnit()));
         }
         strMsg += strTopWidth;
      }

#if defined _DEBUG
            CString strSegID;
            strSegID.Format(_T("\n\nGroup %d Girder %s Segment %d"),
               LABEL_GROUP(segmentKey.groupIndex),
               LABEL_GIRDER(segmentKey.girderIndex),
               LABEL_SEGMENT(segmentKey.segmentIndex));

            strMsg += strSegID;

            CString strGirderID;
            strGirderID.Format(_T("\n\nGirder ID: %d"),pSegment->GetGirder()->GetID());

            strMsg += strGirderID;
#endif // _DEBUG

      pDO->SetMaxTipWidth(TOOLTIP_WIDTH);
      pDO->SetTipDisplayTime(TOOLTIP_DURATION);
      pDO->SetToolTipText(strMsg);
   }
}

CString CBridgeSectionView::GetBarrierToolTip(IBroker* pBroker,const CRailingSystem* pRailingSystem)
{
   CString strTip;
   if (pRailingSystem->bUseInteriorRailing)
   {
      if (pRailingSystem->bUseSidewalk)
      {
         GET_IFACE2(pBroker, IEAFDisplayUnits, pDisplayUnits);
         strTip.Format(_T("Exterior Barrier: %s\nInterior Barrier: %s\nSidewalk Width: %s\nLeft Depth: %s\nRight Depth: %s"),
            pRailingSystem->strExteriorRailing.c_str(),
            pRailingSystem->strInteriorRailing.c_str(),
            FormatDimension(pRailingSystem->Width, pDisplayUnits->GetXSectionDimUnit()),
            FormatDimension(pRailingSystem->LeftDepth, pDisplayUnits->GetComponentDimUnit()),
            FormatDimension(pRailingSystem->RightDepth, pDisplayUnits->GetComponentDimUnit())
         );
      }
      else
      {
         strTip.Format(_T("Exterior Barrier: %s\nInterior Barrier: %s"), pRailingSystem->strExteriorRailing.c_str(), pRailingSystem->strInteriorRailing.c_str());
      }
   }
   else
   {
      if (pRailingSystem->bUseSidewalk)
      {
         GET_IFACE2(pBroker, IEAFDisplayUnits, pDisplayUnits);
         strTip.Format(_T("Barrier: %s\nSidewalk Width: %s\nLeft Depth: %s\nRight Depth: %s"),
            pRailingSystem->strExteriorRailing.c_str(),
            FormatDimension(pRailingSystem->Width, pDisplayUnits->GetXSectionDimUnit()),
            FormatDimension(pRailingSystem->LeftDepth, pDisplayUnits->GetComponentDimUnit()),
            FormatDimension(pRailingSystem->RightDepth, pDisplayUnits->GetComponentDimUnit())
         );
      }
      else
      {
         strTip.Format(_T("Barrier: %s"), pRailingSystem->strExteriorRailing.c_str());
      }
   }

   CString strMsg(_T("Double click to edit\nRight click for more options\n\n"));
   strMsg += strTip;

   return strMsg;
}

void CBridgeSectionView::UpdateDisplayObjects()
{
   CWaitCursor wait;

   CGirderKey girderKey;
   bool bSelectedGirder = GetSelectedGirder(&girderKey);
   bool bDeckSelected = IsDeckSelected();

   SetMappingMode(DManip::Isotropic);

   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   dispMgr->ClearDisplayObjects();

   CDManipClientDC dc(this);

   BuildTitleDisplayObjects();
   BuildGirderDisplayObjects();
   BuildLongitudinalJointDisplayObject();
   BuildDeckDisplayObjects();
   BuildOverlayDisplayObjects();
   BuildTrafficBarrierDisplayObjects();
   BuildDimensionLineDisplayObjects();
   BuildAlignmentDisplayObjects();
   BuildRoadwayCrossSectionDisplayObjects();

   UpdateGirderTooltips();

   if ( bSelectedGirder )
   {
      SelectGirder(girderKey,TRUE);
   }
   else if ( bDeckSelected )
   {
      SelectDeck(true);
   }
}

void CBridgeSectionView::BuildTitleDisplayObjects()
{
   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   CComPtr<iDisplayList> title_list;
   dispMgr->FindDisplayList(TITLE_DISPLAY_LIST,&title_list);

   CComPtr<iViewTitle> title;
   title.CoCreateInstance(CLSID_ViewTitle);

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IEAFDisplayUnits,pdisp_units);
   const WBFL::Units::StationFormat& station_format = pdisp_units->GetStationFormat();
   CString strTitle;
   CString strStation = FormatStation(station_format,m_pFrame->GetCurrentCutLocation());

   strTitle.Format(_T("Section at Station %s - Normal to Alignment"),strStation);
   title->SetText(strTitle);
   title_list->AddDisplayObject(title);
}

void CBridgeSectionView::BuildGirderDisplayObjects()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   GET_IFACE2(pBroker,IShapes,pShapes);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   
   CPGSDocBase* pDoc = (CPGSDocBase*)GetDocument();
   UINT settings = pDoc->GetBridgeEditorSettings();

   GroupIndexType grpIdx = GetGroupIndex();

   GroupIndexType nGroups = pBridgeDesc->GetGirderGroupCount();
   GirderIndexType nGirders = pBridgeDesc->GetGirderGroup(grpIdx)->GetGirderCount();

   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   CComPtr<iDisplayList> girder_list;
   dispMgr->FindDisplayList(GIRDER_DISPLAY_LIST,&girder_list);
   girder_list->Clear();

   CComPtr<iDisplayList> girder_label_list;
   dispMgr->FindDisplayList(GIRDER_LABEL_DISPLAY_LIST,&girder_label_list);
   girder_label_list->Clear();

   Float64 cut_station = m_pFrame->GetCurrentCutLocation();
   m_GirderIDs.clear();
   m_NextGirderID = 0;

   GET_IFACE2(pBroker, IRoadway, pAlignment);
   GET_IFACE2(pBroker, IBridge, pBridge);
   GET_IFACE2_NOCHECK(pBroker, IGirder, pGirder);
   GET_IFACE2(pBroker, ISegmentTendonGeometry, pTendonGeom);

   std::vector<pgsPointOfInterest> vPoi = GetPointsOfInterest();

   CComPtr<IDirection> objDirAlignmentAtCut;
   pAlignment->GetBearing(cut_station, &objDirAlignmentAtCut);

   Float64 dirAlignmentAtCut;
   objDirAlignmentAtCut->get_Value(&dirAlignmentAtCut);

   for (const auto& poi : vPoi)
   {
      const CSegmentKey& thisSegmentKey(poi.GetSegmentKey());
      COLORREF segment_fill_color;
      COLORREF segment_border_color;
      if ( thisSegmentKey.groupIndex == grpIdx )
      {
         // girder cut is on bridge, and in the same group
         segment_fill_color   = SEGMENT_FILL_COLOR;
         segment_border_color = SEGMENT_BORDER_COLOR;
      }
      else
      {
         // girder cut is on bridge, but not in the same group
         segment_fill_color   = SEGMENT_FILL_COLOR_ADJACENT;
         segment_border_color = SEGMENT_BORDER_COLOR_ADJACENT;
      }

      CComPtr<iCompoundDrawPointStrategy> compound_strategy;
      compound_strategy.CoCreateInstance(CLSID_CompoundDrawPointStrategy);

      // Display object for the girder cross section
      CComPtr<iPointDisplayObject> dispObj;
      dispObj.CoCreateInstance(CLSID_PointDisplayObject);

      // get the girder shape before it is made composite (we don't want the deck with the shape)
      IntervalIndexType intervalIdx = pIntervals->GetErectSegmentInterval(thisSegmentKey);

      CComPtr<IShape> shape;
      pShapes->GetSegmentShape(intervalIdx,poi,true,pgsTypes::scBridge,&shape);

      CComPtr<IPoint2d> point;
      pBridge->GetPoint(poi, pgsTypes::pcGlobal, &point);
      Float64 station, offset;
      pAlignment->GetStationAndOffset(pgsTypes::pcGlobal, point, &station, &offset);
      if (!IsEqual(station, cut_station))
      {
         CComPtr<IDirection> segDirection;
         pGirder->GetSegmentDirection(thisSegmentKey, &segDirection);

         Float64 dirSegment;
         segDirection->get_Value(&dirSegment);

         Float64 skew = dirSegment - dirAlignmentAtCut;

         Float64 correctedOffset = offset / cos(skew); // this is where the section is located in the coordinate system for our cut
         Float64 shapeOffset = correctedOffset - offset; // this is the amount we need to change offset so that the shape is located at corrected offset

         if (!IsZero(shapeOffset))
         {
            CComQIPtr<IXYPosition> position(shape);
            position->Offset(shapeOffset, 0);
         }

         Float64 shear = 0;
         CComPtr<IShape> skewedShape;
         SkewGirderShape(-skew, shear, shape, &skewedShape);

         shape = skewedShape;
      }

      CComPtr<IPoint2d> pntWorkPoint; // note that the section work point is always the top CL girder and not the pgsuper-level WP
      CComPtr<IGirderSection> section;
      GetGirderSection(shape, &section);

      CComQIPtr<IXYPosition> position(shape);
      CComPtr<IPoint2d> pntTopCenter;
      position->get_LocatorPoint(lpTopCenter,&pntTopCenter);
      if (section)
      {
         section->get_WorkPoint(&pntWorkPoint);
      }
      else
      {
         pntWorkPoint = pntTopCenter;
      }
      dispObj->SetPosition(pntWorkPoint,FALSE,FALSE);

      // Drawing objct for girder shape
      CComPtr<iShapeDrawStrategy> shape_draw_strategy;
      shape_draw_strategy.CoCreateInstance(CLSID_ShapeDrawStrategy);
      shape_draw_strategy->SetShape(shape);
      shape_draw_strategy->SetSolidLineColor(segment_border_color);
      shape_draw_strategy->SetSolidFillColor(segment_fill_color);
      shape_draw_strategy->SetVoidLineColor(VOID_BORDER_COLOR);
      shape_draw_strategy->SetVoidFillColor(GetSysColor(COLOR_WINDOW));
      shape_draw_strategy->DoFill(true);

      compound_strategy->AddStrategy(shape_draw_strategy);

      // Drawing object for plant installed ducts.
      // the bridge section cut is for the erection interval. at that interval, the plant installed tendons
      // are grouted so the circle shape is no longer part of the girder shape object (unlike the field
      // installed tendons which are part because they ducts aren't grouted yet). we want to draw the
      // circles for the plant installed tendons so we will create circle shape objects here
      DuctIndexType nDucts = pTendonGeom->GetDuctCount(thisSegmentKey);
      for (DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++)
      {
         CComPtr<iShapeDrawStrategy> duct_draw_strategy;
         duct_draw_strategy.CoCreateInstance(CLSID_ShapeDrawStrategy);

         CComPtr<IPoint2d> pntDuct;
         pTendonGeom->GetSegmentDuctPoint(poi, ductIdx, &pntDuct);

         // pntDuct is in girder section coordinates (measured from top center of girder)
         // pntTopCenter is in bridge section coordinates
         // we want pntDuct in bridge section coordinates... duct x,y are relative offsets from
         // top center.... move the duct point
         Float64 dx, dy;
         pntDuct->Location(&dx, &dy);
         
         Float64 tx, ty;
         pntTopCenter->Location(&tx, &ty);

         pntDuct->Move(tx + dx, ty + dy);

         Float64 diameter = pTendonGeom->GetOutsideDiameter(thisSegmentKey, ductIdx);

         duct_draw_strategy->DoFill(true);
         duct_draw_strategy->SetSolidFillColor(GetSysColor(COLOR_WINDOW)); // draw as a void
         duct_draw_strategy->SetSolidLineColor(SEGMENT_TENDON_BORDER_COLOR);

         CComPtr<ICircle> circle;
         circle.CoCreateInstance(CLSID_Circle);
         circle->putref_Center(pntDuct);
         circle->put_Radius(diameter / 2);
         CComQIPtr<IShape> shape(circle);
         duct_draw_strategy->SetShape(shape);

         compound_strategy->AddStrategy(duct_draw_strategy);
      }

      // Drawing object for the work point
      CComPtr<iShapeDrawStrategy> draw_work_point_strategy;
      draw_work_point_strategy.CoCreateInstance(CLSID_ShapeDrawStrategy);

      draw_work_point_strategy->DoFill(true);
      draw_work_point_strategy->SetSolidFillColor(RED);
      draw_work_point_strategy->SetSolidLineColor(RED);

      pgsTypes::WorkPointLocation wploc = pBridgeDesc->GetWorkPointLocation();
      CComPtr<IPoint2d> pntWP;
      pntWorkPoint->Clone(&pntWP);

      if (wploc == pgsTypes::wplBottomGirder)
      {
         // assume small angles to locate work point. This will reconstitute assumptions made in 
         // bridge geometry model builder so we can get dimensions correct
         Float64 gdr_depth = pGirder->GetHeight(poi);
         Float64 wp_offset = pGirder->GetWorkPointShiftOffset(thisSegmentKey);

         pntWP->Offset(wp_offset, -gdr_depth);
      }
      else
      {
         ATLASSERT(wploc == pgsTypes::wplTopGirder); // new option??
      }

      CComPtr<ICircle> wpcircle;
      wpcircle.CoCreateInstance(CLSID_Circle);
      wpcircle->putref_Center(pntWP);
      wpcircle->put_Radius(WBFL::Units::ConvertToSysUnits(0.75,WBFL::Units::Measure::Inch)); // 1.5" diameter point
      CComQIPtr<IShape> wpshape(wpcircle);
      draw_work_point_strategy->SetShape(wpshape);
      compound_strategy->AddStrategy(draw_work_point_strategy); // draw second so it goes over the girder shape

      // If the work point is not at the top of the girder draw a cross hair at the top, since we will continue dimensioning here
      if (wploc != pgsTypes::wplTopGirder)
      {
         CComPtr<iSimpleDrawPointStrategy> draw_top_cl_strategy;
         draw_top_cl_strategy.CoCreateInstance(CLSID_SimpleDrawPointStrategy);
         draw_top_cl_strategy->SetColor(BLACK);
         draw_top_cl_strategy->SetLogicalPointSize(5);
         draw_top_cl_strategy->SetPointType(ptCrossHair);
         compound_strategy->AddStrategy(draw_top_cl_strategy);
      }

      dispObj->SetDrawingStrategy(compound_strategy);

      dispObj->SetSelectionType(stAll);

      IDType ID = m_NextGirderID++;
      m_GirderIDs.insert( std::make_pair(thisSegmentKey,ID) );

      dispObj->SetID(ID);

      girder_list->AddDisplayObject(dispObj);

      // Display object for the girder label
      if ( settings & IDB_CS_LABEL_GIRDERS )
      {
         CComPtr<iTextBlock> doText;
         doText.CoCreateInstance(CLSID_TextBlock);
         CComPtr<IPoint2d> botCenter;
         position->get_LocatorPoint(lpBottomCenter,&botCenter);

         CComQIPtr<IAsymmetricSection> asymmetric(shape);
         if (asymmetric)
         {
            Float64 wLeft, wRight;
            asymmetric->GetTopWidth(&wLeft, &wRight);
            botCenter->Offset(0.5*(wLeft - wRight), 0);
         }
         doText->SetPosition(botCenter);
         CString strLabel;
         strLabel.Format(_T("%s"),LABEL_GIRDER(thisSegmentKey.girderIndex));
         doText->SetText(strLabel);
         doText->SetBkMode(TRANSPARENT);
         doText->SetTextAlign(TA_CENTER | TA_TOP);
         girder_label_list->AddDisplayObject(doText);

         //// Text block for debugging
         //// labels the girder offset and top of girder elevation
         //CComPtr<iTextBlock> doText2;
         //doText2.CoCreateInstance(CLSID_TextBlock);
         //doText2->SetPosition(topCenter);

         //GET_IFACE2(pBroker,IEAFDisplayUnits,pdisp_units);
         //Float64 x,y;
         //topCenter->get_X(&x);
         //topCenter->get_Y(&y);
         //CString strCoordinates;
         //strCoordinates.Format(_T("Offset %s\nElev %s"),FormatDimension(x,pdisp_units->GetXSectionDimUnit()),
         //                                           FormatDimension(y,pdisp_units->GetXSectionDimUnit()) );

         //doText2->SetText(strCoordinates);
         //doText2->SetTextAlign(TA_LEFT | TA_TOP);
         //doText2->SetBkMode(TRANSPARENT);
         //girder_label_list->AddDisplayObject(doText2);
      } // end of if
   

      // Register an event sink with the girder display object so that we can handle dbl-clicks
      // on the girder differently then a general dbl-click in the field of the window
      CBridgeSectionViewGirderDisplayObjectEvents* pEvents = new CBridgeSectionViewGirderDisplayObjectEvents(thisSegmentKey,nGroups,nGirders,m_pFrame); // ref count = 1
      IUnknown* unk = pEvents->GetInterface(&IID_iDisplayObjectEvents); // ref count = 1
      CComQIPtr<iDisplayObjectEvents,&IID_iDisplayObjectEvents> events(unk); // ref count = 2
      dispObj->RegisterEventSink(events); // ref count = 3
      unk->Release(); // removes the AddRef from new above // ref count = 2
      events.Release(); // ref count = 1 ... i.e dispObj holds the only reference
   } // next segment key
}

void CBridgeSectionView::BuildLongitudinalJointDisplayObject()
{
   CPGSDocBase* pDoc = (CPGSDocBase*)GetDocument();
   CComPtr<IBroker> pBroker;
   pDoc->GetBroker(&pBroker);

   GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   if (!pBridgeDesc->HasLongitudinalJoints())
   {
      return;
   }

   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   CComPtr<iDisplayList> joint_list;
   dispMgr->FindDisplayList(JOINT_DISPLAY_LIST, &joint_list);
   joint_list->Clear();

   IDType jointID = 0;

   GET_IFACE2(pBroker, IIntervals, pIntervals);
   IntervalIndexType intervalIdx = pIntervals->GetCompositeLongitudinalJointInterval();
   if (intervalIdx == INVALID_INDEX)
   {
      return; // the longitudinal joints aren't structural (they are just a gap)
   }

   GET_IFACE2(pBroker, IShapes, pShapes);

   GroupIndexType grpIdx = GetGroupIndex();

   std::vector<pgsPointOfInterest> vPoi = GetPointsOfInterest();
   for (const auto& poi : vPoi)
   {
      const CSegmentKey& thisSegmentKey(poi.GetSegmentKey());
      COLORREF joint_fill_color;
      COLORREF joint_border_color;
      if (thisSegmentKey.groupIndex == grpIdx)
      {
         // girder cut is on bridge, and in the same group
         joint_fill_color = pBridgeDesc->HasStructuralLongitudinalJoints() ? JOINT_FILL_COLOR : JOINT_FILL_GHOST_COLOR;
         joint_border_color = pBridgeDesc->HasStructuralLongitudinalJoints() ? JOINT_BORDER_COLOR : JOINT_BORDER_GHOST_COLOR;;
      }
      else
      {
         // girder cut is on bridge, but not in the same group
         joint_fill_color = pBridgeDesc->HasStructuralLongitudinalJoints() ? JOINT_FILL_COLOR_ADJACENT : JOINT_FILL_GHOST_COLOR;
         joint_border_color = pBridgeDesc->HasStructuralLongitudinalJoints() ? JOINT_BORDER_COLOR_ADJACENT : JOINT_BORDER_GHOST_COLOR;
      }

      // Display object for the girder cross section
      CComPtr<iPointDisplayObject> dispObj;
      dispObj.CoCreateInstance(CLSID_PointDisplayObject);

      CComPtr<IShape> leftJointShape, rightJointShape;
      pShapes->GetJointShapes(intervalIdx, poi, true, pgsTypes::scBridge, &leftJointShape, &rightJointShape);

      if (leftJointShape == nullptr && rightJointShape == nullptr)
      {
         return; // there aren't any longitudinal joint
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

      CComQIPtr<IXYPosition> position(shape);
      CComPtr<IPoint2d> topCenter;
      position->get_LocatorPoint(lpTopCenter, &topCenter);
      dispObj->SetPosition(topCenter, FALSE, FALSE);

      CComPtr<iShapeDrawStrategy> strategy;
      strategy.CoCreateInstance(CLSID_ShapeDrawStrategy);
      strategy->SetShape(shape);
      strategy->SetSolidLineColor(joint_border_color);
      strategy->SetSolidFillColor(joint_fill_color);
      strategy->SetVoidLineColor(VOID_BORDER_COLOR);
      strategy->SetVoidFillColor(GetSysColor(COLOR_WINDOW));
      strategy->DoFill(true);

      dispObj->SetDrawingStrategy(strategy);
      dispObj->SetSelectionType(stNone);

      dispObj->SetID(jointID++);

      joint_list->AddDisplayObject(dispObj);
   } // next segment key
}

void CBridgeSectionView::BuildDeckDisplayObjects()
{
   CPGSDocBase* pDoc = (CPGSDocBase*)GetDocument();
   CComPtr<IBroker> pBroker;
   pDoc->GetBroker(&pBroker);

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CDeckDescription2* pDeck = pBridgeDesc->GetDeckDescription();

   pgsTypes::SupportedDeckType deckType = pDeck->GetDeckType();
   if ( deckType == pgsTypes::sdtNone )
   {
      return; // if there is no deck, don't create a display object
   }

   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IShapes,pShapes);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   CComPtr<iDisplayList> display_list;
   dispMgr->FindDisplayList(SLAB_DISPLAY_LIST,&display_list);

   CComPtr<iPointDisplayObject> dispObj;
   dispObj.CoCreateInstance(CLSID_PointDisplayObject);

   CComPtr<IShape> shape;
   pShapes->GetSlabShape(m_pFrame->GetCurrentCutLocation(), nullptr, true/*include haunch*/, &shape);

   CComPtr<iShapeDrawStrategy> strategy;
   strategy.CoCreateInstance(CLSID_ShapeDrawStrategy);

   strategy->SetShape(shape);
   strategy->SetSolidLineColor(IsStructuralDeck(deckType) ? DECK_BORDER_COLOR : NONSTRUCTURAL_DECK_BORDER_COLOR);
   strategy->SetSolidFillColor(IsStructuralDeck(deckType) ? DECK_FILL_COLOR : NONSTRUCTURAL_DECK_FILL_COLOR);
   strategy->SetVoidLineColor(VOID_BORDER_COLOR);
   strategy->SetVoidFillColor(GetSysColor(COLOR_WINDOW));
   strategy->DoFill(true);

   dispObj->SetDrawingStrategy(strategy);

   CComPtr<iShapeGravityWellStrategy> gravity_well;
   gravity_well.CoCreateInstance(CLSID_ShapeGravityWellStrategy);
   gravity_well->SetShape(shape);

   dispObj->SetGravityWellStrategy(gravity_well);

   CBridgeSectionViewSlabDisplayObjectEvents* pEvents = new CBridgeSectionViewSlabDisplayObjectEvents(pDoc,pBroker,m_pFrame,true);
   IUnknown* unk = pEvents->GetInterface(&IID_iDisplayObjectEvents);
   CComQIPtr<iDisplayObjectEvents,&IID_iDisplayObjectEvents> events(unk);

   dispObj->RegisterEventSink(events);

   unk->Release();
   events.Release();

   
   CString strMsg1(_T("Double click to edit deck.\r\nRight click for more options."));

   CString strMsg2;
   if (deckType != pgsTypes::sdtNone)
   {
      if (pBridge->GetHaunchInputDepthType() == pgsTypes::hidACamber)
   {
      pgsTypes::SlabOffsetType slabOffsetType = pBridgeDesc->GetSlabOffsetType();
         if (slabOffsetType == pgsTypes::sotBridge)
      {
         strMsg2.Format(_T("\r\n\nDeck: %s\r\nSlab Thickness: %s\r\nSlab Offset: %s\r\n%s\r\nf'c: %s"),
                        GetDeckTypeName(deckType),
                        FormatDimension(pDeck->GrossDepth,pDisplayUnits->GetComponentDimUnit()),
                        FormatDimension(pBridgeDesc->GetSlabOffset(),pDisplayUnits->GetComponentDimUnit()),
                        lrfdConcreteUtil::GetTypeName((WBFL::Materials::ConcreteType)pDeck->Concrete.Type,true).c_str(),
                        FormatDimension(pDeck->Concrete.Fc,pDisplayUnits->GetStressUnit())
                        );
      }
      else
      {
         strMsg2.Format(_T("\r\n\nDeck: %s\r\nSlab Thickness: %s\r\nSlab Offset: per girder\r\n%s\r\nf'c: %s"),
                        GetDeckTypeName(deckType),
                        FormatDimension(pDeck->GrossDepth,pDisplayUnits->GetComponentDimUnit()),
                        lrfdConcreteUtil::GetTypeName((WBFL::Materials::ConcreteType)pDeck->Concrete.Type,true).c_str(),
                        FormatDimension(pDeck->Concrete.Fc,pDisplayUnits->GetStressUnit())
                        );
      }
   }
      else
      {
         strMsg2.Format(_T("\r\n\nDeck: %s\r\nSlab Thickness: %s\r\nHaunch Depths Defined by Direct Input\r\n%s\r\nf'c: %s"),
            GetDeckTypeName(deckType),
            FormatDimension(pDeck->GrossDepth,pDisplayUnits->GetComponentDimUnit()),
            lrfdConcreteUtil::GetTypeName((WBFL::Materials::ConcreteType)pDeck->Concrete.Type,true).c_str(),
            FormatDimension(pDeck->Concrete.Fc,pDisplayUnits->GetStressUnit())
         );
      }
   }

   CString strMsg3;
   Float64 overlay_weight = pBridge->GetOverlayWeight();
   if ( pBridge->HasOverlay() )
   {
      strMsg3.Format(_T("\r\n\n%s: %s"),
         pBridge->IsFutureOverlay() ? _T("Future Overlay") : _T("Overlay"),
         FormatDimension(overlay_weight,pDisplayUnits->GetOverlayWeightUnit()));
   }

   CString strMsg = strMsg1 + strMsg2 + strMsg3;
   dispObj->SetToolTipText(strMsg);
   dispObj->SetTipDisplayTime(TOOLTIP_DURATION);
   dispObj->SetMaxTipWidth(TOOLTIP_WIDTH);

   dispObj->SetSelectionType(stAll);

   dispObj->SetID(DECK_ID);

   display_list->AddDisplayObject(dispObj);
}

void CBridgeSectionView::BuildOverlayDisplayObjects()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IBridge,pBridge);

   // no overlay = nothing to draw
   if ( !pBridge->HasOverlay() )
   {
      return;
   }

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType geomCtrlInterval = pIntervals->GetGeometryControlInterval();

   Float64 overlay_weight = pBridge->GetOverlayWeight();
   Float64 depth = pBridge->GetOverlayDepth(geomCtrlInterval);

   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   CComPtr<iDisplayList> display_list;
   dispMgr->FindDisplayList(OVERLAY_DISPLAY_LIST,&display_list);

   // point a simple point display object in the corner between the top of deck and the traffic barrier
   // at the left hand side of the section. then use a ShapeDrawStrategy to draw the overlay shape

   CComPtr<iPointDisplayObject> dispObj;
   dispObj.CoCreateInstance(CLSID_PointDisplayObject);

   Float64 station = m_pFrame->GetCurrentCutLocation();

   GET_IFACE2(pBroker,IPointOfInterest,pPoi);
   Float64 Xb = pPoi->ConvertRouteToBridgeLineCoordinate(station);
   Float64 left_offset, right_offset;
   left_offset  = pBridge->GetLeftOverlayToeOffset(Xb);
   right_offset = pBridge->GetRightOverlayToeOffset(Xb);

   GET_IFACE2(pBroker,IRoadway,pRoadway);
   CComPtr<IPoint2dCollection> surfacePoints;
   pRoadway->GetRoadwaySurface(station,nullptr,&surfacePoints);

   TrimSurface(surfacePoints,left_offset,right_offset);

   CComPtr<IPoint2d> pos;
   surfacePoints->get_Item(0,&pos);
   dispObj->SetPosition(pos,FALSE,FALSE);

   // create a poly shape to represent the overlay
   CComPtr<IPolyShape> poly_shape;
   poly_shape.CoCreateInstance(CLSID_PolyShape);


   // Create a drawing strategy
   CComPtr<iShapeDrawStrategy> strategy;
   strategy.CoCreateInstance(CLSID_ShapeDrawStrategy);
   // associate the shape with the strategy
   CComQIPtr<IShape> shape(poly_shape);
   strategy->SetShape(shape);

   bool bIsFutureOverlay = pBridge->IsFutureOverlay();
   if ( bIsFutureOverlay )
   {
      strategy->SetSolidLineColor(FUTURE_OVERLAY_COLOR);
      strategy->SetSolidFillColor(FUTURE_OVERLAY_COLOR);
   }
   else
   {
      strategy->SetSolidLineColor(OVERLAY_COLOR);
      strategy->SetSolidFillColor(OVERLAY_COLOR);
   }
   strategy->DoFill(true);


   // associate the strategy with the display object
   dispObj->SetDrawingStrategy(strategy);

   // Fill up the polygon
   IndexType nPoints;
   surfacePoints->get_Count(&nPoints);

   // points along the finished profile grade surface
   for (IndexType pntIdx = 0; pntIdx < nPoints; pntIdx++ )
   {
      CComPtr<IPoint2d> pnt;
      surfacePoints->get_Item(pntIdx,&pnt);
      poly_shape->AddPointEx(pnt);
   }

   // if it is a future overaly, the overlay is above the finished surface
   // otherwise it is the finished surface
   depth *= (bIsFutureOverlay ? 1 : -1);

   // now work backwards, offset the points by the depth of the overlay
   for ( IndexType pntIdx = nPoints-1; pntIdx != INVALID_INDEX; pntIdx-- )
   {
      CComPtr<IPoint2d> pnt;
      surfacePoints->get_Item(pntIdx,&pnt);
      CComPtr<IPoint2d> topPoint;
      pnt->Clone(&topPoint);
      topPoint->Offset(0,depth);
      poly_shape->AddPointEx(topPoint);
   }

   display_list->AddDisplayObject(dispObj);
}

void CBridgeSectionView::BuildTrafficBarrierDisplayObjects()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IRoadway,pAlignment);
   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IShapes,pShapes);
   GET_IFACE2(pBroker,IBarriers,pBarriers);
   GET_IFACE2(pBroker,IPointOfInterest,pPoi);
   GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc);

   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   CComPtr<iDisplayList> display_list;
   dispMgr->FindDisplayList(TRAFFIC_BARRIER_DISPLAY_LIST,&display_list);

   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   // left hand barrier
   CComPtr<iPointDisplayObject> left_dispObj;
   left_dispObj.CoCreateInstance(CLSID_PointDisplayObject);
   left_dispObj->SetSelectionType(stAll);
   left_dispObj->SetID(LEFT_TRAFFIC_BARRIER_ID);

   const CRailingSystem* pLeftRailingSystem = pBridgeDesc->GetLeftRailingSystem();
   CString strTip = GetBarrierToolTip(pBroker,pLeftRailingSystem);
   left_dispObj->SetMaxTipWidth(TOOLTIP_WIDTH);
   left_dispObj->SetTipDisplayTime(TOOLTIP_DURATION);
   left_dispObj->SetToolTipText(strTip);

   Float64 cut_station = m_pFrame->GetCurrentCutLocation();
   Float64 pier_1_station = pBridge->GetPierStation(0);
   Float64 cut_dist_from_start = cut_station - pier_1_station;
   Float64 left_curb_offset  = pBridge->GetLeftCurbOffset(cut_dist_from_start);
   Float64 right_curb_offset = pBridge->GetRightCurbOffset(cut_dist_from_start);

   CComPtr<IDirection> normal;
   pAlignment->GetBearingNormal(cut_station,&normal);

   CComPtr<IShape> left_shape;
   pShapes->GetLeftTrafficBarrierShape(cut_station,nullptr,&left_shape);

   CComPtr<iShapeDrawStrategy> strategy;
   if ( left_shape )
   {
      strategy.CoCreateInstance(CLSID_ShapeDrawStrategy);

      strategy->SetShape(left_shape);
      strategy->SetSolidLineColor(BARRIER_BORDER_COLOR);
      strategy->SetSolidFillColor(BARRIER_FILL_COLOR);
      strategy->SetVoidLineColor(VOID_BORDER_COLOR);
      strategy->SetVoidFillColor(GetSysColor(COLOR_WINDOW));
      strategy->DoFill(true);
      strategy->HasBoundingShape(false);

      left_dispObj->SetDrawingStrategy(strategy);
   }


   // right hand barrier
   CComPtr<iPointDisplayObject> right_dispObj;
   right_dispObj.CoCreateInstance(CLSID_PointDisplayObject);
   right_dispObj->SetSelectionType(stAll);
   right_dispObj->SetID(RIGHT_TRAFFIC_BARRIER_ID);

   const CRailingSystem* pRightRailingSystem = pBridgeDesc->GetRightRailingSystem();
   strTip = GetBarrierToolTip(pBroker,pRightRailingSystem);

   right_dispObj->SetMaxTipWidth(TOOLTIP_WIDTH);
   right_dispObj->SetTipDisplayTime(TOOLTIP_DURATION);
   right_dispObj->SetToolTipText(strTip);

   CComPtr<IShape> right_shape;
   pShapes->GetRightTrafficBarrierShape(cut_station,nullptr,&right_shape);

   if ( right_shape )
   {
      strategy.Release();
      strategy.CoCreateInstance(CLSID_ShapeDrawStrategy);

      strategy->SetShape(right_shape);
      strategy->SetSolidLineColor(BARRIER_BORDER_COLOR);
      strategy->SetSolidFillColor(BARRIER_FILL_COLOR);
      strategy->SetVoidLineColor(VOID_BORDER_COLOR);
      strategy->SetVoidFillColor(GetSysColor(COLOR_WINDOW));
      strategy->DoFill(true);
      strategy->HasBoundingShape(false);

      right_dispObj->SetDrawingStrategy(strategy);
   }

   // place sockets at curb line so we can do a curb-to-curb dimension line
   CComQIPtr<iConnectable> left_connectable(left_dispObj);
   CComQIPtr<iConnectable> right_connectable(right_dispObj);
   Float64 Xb = pPoi->ConvertRouteToBridgeLineCoordinate(m_pFrame->GetCurrentCutLocation());
   Float64 left_offset, right_offset;
   left_offset  = pBridge->GetLeftCurbOffset(Xb);
   right_offset = pBridge->GetRightCurbOffset(Xb);

   GET_IFACE2(pBroker,IRoadway,pRoadway);
   Float64 left_elev  = pRoadway->GetElevation(m_pFrame->GetCurrentCutLocation(),left_offset);
   Float64 right_elev = pRoadway->GetElevation(m_pFrame->GetCurrentCutLocation(),right_offset);

   Float64 elev = Max(left_elev,right_elev);

   CComPtr<IPoint2d> p1, p2;
   p1.CoCreateInstance(CLSID_Point2d);
   p2.CoCreateInstance(CLSID_Point2d);
   p1->Move(left_offset, elev);
   p2->Move(right_offset,elev);

   CComPtr<iSocket> socket1, socket2;
   left_connectable->AddSocket(LEFT_CURB_SOCKET, p1,&socket1);
   right_connectable->AddSocket(RIGHT_CURB_SOCKET,p2,&socket2);

   // Put sockets at slab edges
   socket1.Release();
   socket2.Release();
   CComPtr<IPoint2d> pl,pr;
   pl.CoCreateInstance(CLSID_Point2d);
   pr.CoCreateInstance(CLSID_Point2d);

   left_offset = pBridge->GetLeftSlabEdgeOffset(Xb);
   right_offset = pBridge->GetRightSlabEdgeOffset(Xb);

   if ( left_shape )
   {  
      pl->put_X(left_offset);
      pl->put_Y(elev);
      left_connectable->AddSocket(LEFT_SLAB_EDGE_SOCKET, pl,&socket1);
   }

   if ( right_shape )
   {
      pr->put_X(right_offset);
      pr->put_Y(elev);
      right_connectable->AddSocket(RIGHT_SLAB_EDGE_SOCKET,pr,&socket2);
   }

   // Put sockets at edges of sidewalks
   Float64 ext_edge, int_edge;
   if (pBarriers->HasSidewalk(pgsTypes::tboLeft))
   {
      socket1.Release();
      socket2.Release();
      pBarriers->GetSidewalkPedLoadEdges(pgsTypes::tboLeft,&int_edge,&ext_edge);

      pl->put_X(left_offset+ext_edge);
      pl->put_Y(elev);
      left_connectable->AddSocket(LEFT_EXT_SW_SOCKET, pl,&socket1);

      pl->put_X(left_offset+int_edge);
      left_connectable->AddSocket(LEFT_INT_SW_SOCKET, pl,&socket2);
   }

   if (pBarriers->HasSidewalk(pgsTypes::tboRight))
   {
      socket1.Release();
      socket2.Release();
      pBarriers->GetSidewalkPedLoadEdges(pgsTypes::tboRight,&int_edge,&ext_edge);

      pl->put_X(right_offset-ext_edge);
      pl->put_Y(elev);
      right_connectable->AddSocket(RIGHT_EXT_SW_SOCKET, pl,&socket1);

      pl->put_X(right_offset-int_edge);
      right_connectable->AddSocket(RIGHT_INT_SW_SOCKET, pl,&socket2);
   }

   // interior overlay sockets
   if (pBridge->HasOverlay())
   {
      socket1.Release();
      socket2.Release();

      Float64 left_icb_offset, right_icb_offset;
      left_icb_offset  = pBridge->GetLeftOverlayToeOffset(Xb);
      right_icb_offset = pBridge->GetRightOverlayToeOffset(Xb);

      pl->put_X(left_icb_offset);
      pl->put_Y(elev);
      left_connectable->AddSocket(LEFT_INT_OVERLAY_SOCKET, pl,&socket1);

      pl->put_X(right_icb_offset);
      left_connectable->AddSocket(RIGHT_INT_OVERLAY_SOCKET, pl,&socket2);
   }

   // on the piers differently then a general dbl-click
   CTrafficBarrierDisplayObjectEvents* pLeftEvents = new CTrafficBarrierDisplayObjectEvents(pBroker, m_pFrame, pgsTypes::tboLeft);
   CComPtr<iDisplayObjectEvents> left_events;
   left_events.Attach((iDisplayObjectEvents*)pLeftEvents->GetInterface(&IID_iDisplayObjectEvents));

   CTrafficBarrierDisplayObjectEvents* pRightEvents = new CTrafficBarrierDisplayObjectEvents(pBroker, m_pFrame, pgsTypes::tboRight);
   CComPtr<iDisplayObjectEvents> right_events;
   right_events.Attach((iDisplayObjectEvents*)pRightEvents->GetInterface(&IID_iDisplayObjectEvents));

   left_dispObj->RegisterEventSink(left_events);
   right_dispObj->RegisterEventSink(right_events);

   display_list->AddDisplayObject(left_dispObj);
   display_list->AddDisplayObject(right_dispObj);
}

void CBridgeSectionView::BuildDimensionLineDisplayObjects()
{
   CPGSDocBase* pDoc = (CPGSDocBase*)GetDocument();

   UINT settings = pDoc->GetBridgeEditorSettings();

   if ( !(settings & IDB_CS_SHOW_DIMENSIONS ) )
   {
      return;
   }


   CComPtr<IBroker> pBroker;
   pDoc->GetBroker(&pBroker);

   GroupIndexType grpIdx = GetGroupIndex();

   GET_IFACE2(pBroker,IPointOfInterest,pPoi);
   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(grpIdx);
   GirderIndexType nGirders = pGroup->GetGirderCount();

   // Get display lists
   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   CComPtr<iDisplayList> display_list;
   dispMgr->FindDisplayList(DIMENSION_DISPLAY_LIST,&display_list);

   CComPtr<iDisplayList> girder_list;
   dispMgr->FindDisplayList(GIRDER_DISPLAY_LIST,&girder_list);

   Float64 cut_station = m_pFrame->GetCurrentCutLocation();
   Float64 Xb = pPoi->ConvertRouteToBridgeLineCoordinate(cut_station);

   // get length unit so section can be labelled
   GET_IFACE2(pBroker,IEAFDisplayUnits,pdisp_units);
   const WBFL::Units::LengthData& rlen = pdisp_units->GetXSectionDimUnit();

   //
   // Create Girder Spacing Dimension Line
   //
   CComPtr<iSocket> firstSocket, lastSocket;
   long witness_length;

   // find the top of the "highest" girder so all the dimension lines can be at
   // the same elevation
   Float64 yHighest = -DBL_MAX;
   Float64 yLowest  =  DBL_MAX;
   for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
   {
      CComPtr<iDisplayObject> doGirder;

      CGirderKey girderKey(grpIdx,gdrIdx);
      GirderIDCollection::iterator found = m_GirderIDs.find(girderKey);

      if ( found == m_GirderIDs.end() )
      {
         continue;
      }

      IDType ID = (*found).second;

      girder_list->FindDisplayObject(ID,&doGirder);

      if ( !doGirder )
      {
         continue;
      }

      CComQIPtr<iPointDisplayObject> pdoGirder(doGirder);

      // We know that these display objects use CompoundDrawPointStrategy and the second strategy, index 0, is ShapeDrawStrategy objects, and they hold the girder shape
      // Get the strategies and then the shapes
      CComPtr<iDrawPointStrategy> dsGirder;
      pdoGirder->GetDrawingStrategy(&dsGirder);

      CComQIPtr<iCompoundDrawPointStrategy> compound_strategy(dsGirder);

      CComQIPtr<iDrawPointStrategy> dps;
      compound_strategy->GetStrategy(0, &dps);
      CComQIPtr<iShapeDrawStrategy> strategy(dps);

      CComPtr<IShape> shape;
      strategy->GetShape(&shape);

      // Get bottom center coordinates of teh exterior girders
      CComQIPtr<IXYPosition> position(shape);

      CComPtr<IPoint2d> p;
      position->get_LocatorPoint(lpTopCenter, &p);

      Float64 y;
      p->get_Y(&y);
      yHighest = Max(y, yHighest);

      CComPtr<IPoint2d> pl;
      position->get_LocatorPoint(lpBottomCenter, &pl);
      pl->get_Y(&y);
      yLowest  = Min(y, yLowest);
   }

   // make a dimension line for each spacing group
   if (nGirders == 1)
   {
      CComPtr<iDisplayObject> dispObj;
      girder_list->GetDisplayObject(0, &dispObj);

      CComQIPtr<iPointDisplayObject> pntDO(dispObj);

      // We know that these display objects use CompoundDrawPointStrategy and the second strategy, index 0, is ShapeDrawStrategy objects, and they hold the girder shape
      // Get the strategies and then the shapes
      CComPtr<iDrawPointStrategy> ds;
      pntDO->GetDrawingStrategy(&ds);

      CComQIPtr<iCompoundDrawPointStrategy> compound_strategy(ds);

      CComQIPtr<iDrawPointStrategy> dps;
      compound_strategy->GetStrategy(0, &dps);
      CComQIPtr<iShapeDrawStrategy> strategy(dps);

      CComPtr<IShape> shape;
      strategy->GetShape(&shape);

      // Get bottom center coordinates of the exterior girders
      CComPtr<IPoint2d> p1;
      CComPtr<IGirderSection> section;
      GetGirderSection(shape, &section);

      if (section)
      {
         section->get_WorkPoint(&p1);
      }
      else
      {
         CComQIPtr<IXYPosition> position(shape);
         position->get_LocatorPoint(lpTopCenter, &p1);
      }

      CComQIPtr<iConnectable> c1(dispObj);
      c1->AddSocket(0,p1,&firstSocket);
      lastSocket = firstSocket;
   }
   else
   {
      std::vector<SpaceBetweenGirder> vSpacing( pBridge->GetGirderSpacing(cut_station) );
      std::vector<SpaceBetweenGirder>::iterator iter(vSpacing.begin());
      std::vector<SpaceBetweenGirder>::iterator iterEnd(vSpacing.end());
      for ( ; iter != iterEnd; iter++ )
      {
         SpaceBetweenGirder& spacingData = *iter;

         // get the girder spacing for this group
         Float64 spacing = spacingData.spacing;
         GirderIndexType firstGdrIdx = spacingData.firstGdrIdx;
         GirderIndexType lastGdrIdx  = spacingData.lastGdrIdx;
      
         SpacingIndexType nSpacesInGroup = lastGdrIdx - firstGdrIdx;

         Float64 total = spacing*nSpacesInGroup;

         // Create dimension line display object for this spacing group
         CComPtr<iDimensionLine> doDimLine;
         doDimLine.CoCreateInstance(CLSID_DimensionLineDisplayObject);
         CComQIPtr<iConnector> connector(doDimLine);

         // Going to attach dimension line to girder display objects, so get them now
         CComPtr<iDisplayObject> do1, do2;
         CSegmentKey firstGirderKey(grpIdx,firstGdrIdx,INVALID_INDEX);
         CSegmentKey lastGirderKey(grpIdx,lastGdrIdx,INVALID_INDEX);
   
         GirderIDCollection::iterator found( m_GirderIDs.find(firstGirderKey) );
         if ( found == m_GirderIDs.end() )
         {
            continue;
         }

         IDType firstID = (*found).second;

         found = m_GirderIDs.find(lastGirderKey);
         if ( found == m_GirderIDs.end() )
         {
            continue;
         }

         IDType lastID  = (*found).second;

         girder_list->FindDisplayObject(firstID,&do1);
         girder_list->FindDisplayObject(lastID, &do2);
 
         if ( do1 && do2 )
         {
            CComQIPtr<iPointDisplayObject> pdo1(do1);
            CComQIPtr<iPointDisplayObject> pdo2(do2);

            // We know that these display objects use ShapeDrawStrategy objects, and they hold the girder shape
            // Get the strategies and then the shapes
            CComPtr<iDrawPointStrategy> ds1, ds2;
            pdo1->GetDrawingStrategy(&ds1);
            pdo2->GetDrawingStrategy(&ds2);

            CComQIPtr<iCompoundDrawPointStrategy> compound_strategy1(ds1);
            CComQIPtr<iCompoundDrawPointStrategy> compound_strategy2(ds2);

            CComPtr<iDrawPointStrategy> dps1;
            CComPtr<iDrawPointStrategy> dps2;
            compound_strategy1->GetStrategy(0, &dps1);
            compound_strategy2->GetStrategy(0, &dps2);

            CComQIPtr<iShapeDrawStrategy> strategy1(dps1);
            CComQIPtr<iShapeDrawStrategy> strategy2(dps2);

            CComPtr<IShape> shape1, shape2;
            strategy1->GetShape(&shape1);
            strategy2->GetShape(&shape2);

            CComPtr<IPoint2d> p1;
            CComPtr<IGirderSection> section1;
            GetGirderSection(shape1, &section1);
            if (section1)
            {
               section1->get_WorkPoint(&p1);
            }
            else
            {
               CComQIPtr<IXYPosition> position1(shape1);
               position1->get_LocatorPoint(lpTopCenter, &p1);
            }

            CComPtr<IPoint2d> p2;
            CComPtr<IGirderSection> section2;
            GetGirderSection(shape2, &section2);
            if (section2)
            {
               section2->get_WorkPoint(&p2);
            }
            else
            {
               CComQIPtr<IXYPosition> position2(shape2);
               position2->get_LocatorPoint(lpTopCenter, &p2);
            }

            // adjust points so both are at the same, and highest, elevation
            p1->put_Y(yHighest);
            p2->put_Y(yHighest);

            // Add sockets to the display objects at these points
            CComQIPtr<iConnectable> c1(do1);
            CComQIPtr<iConnectable> c2(do2);
            CComPtr<iSocket> s1, s2;
            c1->AddSocket(0,p1,&s1);
            c2->AddSocket(0,p2,&s2);

            // save the first and last sockets for use with creating
            // the slab overhang dimensions
            if ( firstGdrIdx == 0 )
            {
               firstSocket = s1;
            }

            if ( lastGdrIdx == nGirders-1 )
            {
               lastSocket = s2;
            }

            // get the plugs
            CComPtr<iPlug> startPlug;
            CComPtr<iPlug> endPlug;
            connector->GetStartPlug(&startPlug);
            connector->GetEndPlug(&endPlug);

            // connect the dimension line
            DWORD dwCookie;
            s1->Connect(startPlug,&dwCookie);
            s2->Connect(endPlug,&dwCookie);

            // Orient dimension line
            witness_length = doDimLine->GetWitnessLength();
            witness_length *= 2;
            doDimLine->SetWitnessLength(witness_length);

            //
            // Develop the text for the dimension line
            //
            CComPtr<iTextBlock> text;
            text.CoCreateInstance(CLSID_TextBlock);
            text->SetBkMode(TRANSPARENT);

            CString strSpacing = FormatDimension(spacing,rlen);
            CString strTotal   = FormatDimension(total,rlen);

            if ( 0 < nSpacesInGroup )
            {
               std::_tostringstream os;
               if ( nSpacesInGroup == 1 )
               {
                  os << (LPCTSTR)strSpacing;
               }
               else
               {
                  os << nSpacesInGroup << _T(" spaces @ ") << (LPCTSTR)strSpacing << _T(" = ") << (LPCTSTR)strTotal;
               }

               text->SetText(os.str().c_str());

               doDimLine->SetTextBlock(text);

               display_list->AddDisplayObject(doDimLine);
            }
         } // end if do1 && do2
      } // end group loop
   } // end if

   //
   // Slab Overhang Dimensions
   //
   if ( pBridge->GetDeckType() != pgsTypes::sdtNone )
   {
      CComPtr<iDisplayList> slab_list;
      dispMgr->FindDisplayList(SLAB_DISPLAY_LIST,&slab_list);

      // get the slab display object
      CComPtr<iDisplayObject> doSlab;
      slab_list->GetDisplayObject(0,&doSlab);

      CComQIPtr<iPointDisplayObject> pdoSlab(doSlab);

      // We know that these display objects use ShapeDrawStrategy objects
      CComPtr<iDrawPointStrategy> dsSlab;
      pdoSlab->GetDrawingStrategy(&dsSlab);

      CComQIPtr<iShapeDrawStrategy> slabStrategy(dsSlab);

      CComPtr<IShape> slabShape;
      slabStrategy->GetShape(&slabShape);

      CComQIPtr<IXYPosition> slabPosition(slabShape);

      //
      // Create Left Slab Overhang Dimension Line
      //
      Float64 leftOverhang = pBridge->GetLeftSlabOverhang(Xb);
      if ( 0 <= leftOverhang )
      {
         CComPtr<iDimensionLine> leftOverhangDimLine;
         leftOverhangDimLine.CoCreateInstance(CLSID_DimensionLineDisplayObject);
         CComPtr<iConnector> connector;
         leftOverhangDimLine.QueryInterface(&connector);

         // get bottom left of slab
         CComPtr<IPoint2d> left_overhang_point;
         slabPosition->get_LocatorPoint(lpBottomLeft,&left_overhang_point);
         left_overhang_point->put_Y(yHighest);

         // Add sockets to the display objects at these points
         CComQIPtr<iConnectable> leftOverhangConnectable(doSlab);
         CComPtr<iSocket> leftOverhangSocket;
         leftOverhangConnectable->AddSocket(LEFT_OVERHANG_SOCKET,  left_overhang_point,  &leftOverhangSocket);

         // get the plugs
         CComPtr<iPlug> startPlug, endPlug;
         connector->GetStartPlug(&startPlug);
         connector->GetEndPlug(&endPlug);

         // connect the dimension line
         DWORD dwCookie;
         if ( firstSocket )
         {
            leftOverhangSocket->Connect(startPlug,&dwCookie);
            firstSocket->Connect(endPlug,&dwCookie);

            witness_length = leftOverhangDimLine->GetWitnessLength();
            witness_length *= 2;
            leftOverhangDimLine->SetWitnessLength(witness_length);

            CComPtr<iTextBlock> leftOverhangText;
            leftOverhangText.CoCreateInstance(CLSID_TextBlock);
      
            leftOverhangText->SetBkMode(TRANSPARENT);

            CString strLeftOH = FormatDimension(leftOverhang,rlen);

            leftOverhangText->SetText(strLeftOH);

            leftOverhangDimLine->SetTextBlock(leftOverhangText);

            display_list->AddDisplayObject(leftOverhangDimLine);
         }
      }


      //
      // Create Right Overhang Dimension Line
      //
      Float64 rightOverhang = pBridge->GetRightSlabOverhang(Xb);
      if ( 0 <= rightOverhang )
      {
         CComPtr<iDimensionLine> rightOverhangDimLine;
         rightOverhangDimLine.CoCreateInstance(CLSID_DimensionLineDisplayObject);
         CComPtr<iConnector> connector;
         rightOverhangDimLine.QueryInterface(&connector);

         // get bottom right of slab
         CComPtr<IPoint2d> right_overhang_point;
         slabPosition->get_LocatorPoint(lpBottomRight,&right_overhang_point);
         right_overhang_point->put_Y(yHighest);

         // Add sockets to the display objects at these points
         CComQIPtr<iConnectable> rightOverhangConnectable(doSlab);
         CComPtr<iSocket> rightOverhangSocket;
         rightOverhangConnectable->AddSocket(RIGHT_OVERHANG_SOCKET, right_overhang_point, &rightOverhangSocket);

         // get the plugs
         CComPtr<iPlug> startPlug, endPlug;
         connector->GetStartPlug(&startPlug);
         connector->GetEndPlug(&endPlug);

         // connect the dimension line
         DWORD dwCookie;
         if ( lastSocket )
         {
            lastSocket->Connect(startPlug,&dwCookie);
            rightOverhangSocket->Connect(endPlug,&dwCookie);

            witness_length = rightOverhangDimLine->GetWitnessLength();
            witness_length *= 2;
            rightOverhangDimLine->SetWitnessLength(witness_length);

            CComPtr<iTextBlock> rightOverhangText;
            rightOverhangText.CoCreateInstance(CLSID_TextBlock);
      
            rightOverhangText->SetBkMode(TRANSPARENT);

            CString strRightOH = FormatDimension(rightOverhang,rlen);
            rightOverhangText->SetText(strRightOH);

            rightOverhangDimLine->SetTextBlock(rightOverhangText);

            display_list->AddDisplayObject(rightOverhangDimLine);
         }
      }
   }

   // If work point is at bottom, add dimension lines along bottom of bridge cross section
   // make a dimension line for each spacing group
   pgsTypes::WorkPointLocation wploc = pBridgeDesc->GetWorkPointLocation();
   if (nGirders != 1 && wploc == pgsTypes::wplBottomGirder)
   {
      GET_IFACE2(pBroker, IGirder, pGirder);

      std::vector<SpaceBetweenGirder> vSpacing(pBridge->GetGirderSpacingAtBottomClGirder(cut_station));
      for (auto& spacingData : vSpacing)
      {
         // get the girder spacing for this group
         Float64 spacing = spacingData.spacing;
         GirderIndexType firstGdrIdx = spacingData.firstGdrIdx;
         GirderIndexType lastGdrIdx = spacingData.lastGdrIdx;

         SpacingIndexType nSpacesInGroup = lastGdrIdx - firstGdrIdx;

         Float64 total = spacing*nSpacesInGroup;

         // Create dimension line display object for this spacing group
         CComPtr<iDimensionLine> doDimLine;
         doDimLine.CoCreateInstance(CLSID_DimensionLineDisplayObject);
         CComQIPtr<iConnector> connector(doDimLine);

         // Going to attach dimension line to girder display objects, so get them now
         CComPtr<iDisplayObject> do1, do2;
         CSegmentKey firstGirderKey(grpIdx, firstGdrIdx, INVALID_INDEX);
         CSegmentKey lastGirderKey(grpIdx, lastGdrIdx, INVALID_INDEX);

         GirderIDCollection::iterator found(m_GirderIDs.find(firstGirderKey));
         if (found == m_GirderIDs.end())
         {
            continue;
         }

         IDType firstID = (*found).second;

         found = m_GirderIDs.find(lastGirderKey);
         if (found == m_GirderIDs.end())
         {
            continue;
         }

         IDType lastID = (*found).second;

         girder_list->FindDisplayObject(firstID, &do1);
         girder_list->FindDisplayObject(lastID, &do2);

         if (do1 && do2)
         {
            CComQIPtr<iPointDisplayObject> pdo1(do1);
            CComQIPtr<iPointDisplayObject> pdo2(do2);

            // We know that these display objects use ShapeDrawStrategy objects, and they hold the girder shape
            // Get the strategies and then the shapes
            CComPtr<iDrawPointStrategy> ds1, ds2;
            pdo1->GetDrawingStrategy(&ds1);
            pdo2->GetDrawingStrategy(&ds2);

            CComQIPtr<iCompoundDrawPointStrategy> compound_strategy1(ds1);
            CComQIPtr<iCompoundDrawPointStrategy> compound_strategy2(ds2);

            CComPtr<iDrawPointStrategy> dps1;
            CComPtr<iDrawPointStrategy> dps2;
            compound_strategy1->GetStrategy(0, &dps1);
            compound_strategy2->GetStrategy(0, &dps2);

            CComQIPtr<iShapeDrawStrategy> strategy1(dps1);
            CComQIPtr<iShapeDrawStrategy> strategy2(dps2);

            CComPtr<IShape> shape1, shape2;
            strategy1->GetShape(&shape1);
            strategy2->GetShape(&shape2);

            CComPtr<IPoint2d> p1;
            CComPtr<IGirderSection> section1;
            GetGirderSection(shape1, &section1);
            if (section1)
            {
               section1->get_WorkPoint(&p1);
            }
            else
            {
               CComQIPtr<IXYPosition> position1(shape1);
               position1->get_LocatorPoint(lpTopCenter, &p1);
            }

            CComPtr<IPoint2d> p2;
            CComPtr<IGirderSection> section2;
            GetGirderSection(shape2, &section2);
            if (section2)
            {
               section2->get_WorkPoint(&p2);
            }
            else
            {
               CComQIPtr<IXYPosition> position2(shape2);
               position2->get_LocatorPoint(lpTopCenter, &p2);
            }

            // adjust points so both are at the same, and lowest, elevation
            p1->put_Y(yLowest);
            p2->put_Y(yLowest);

            // now adjust for orientation shift
            CSegmentKey gk1(grpIdx, firstGdrIdx, 0);
            CSegmentKey gk2(grpIdx, lastGdrIdx, 0);

            Float64 wp_offset1 = pGirder->GetWorkPointShiftOffset(gk1);
            Float64 wp_offset2 = pGirder->GetWorkPointShiftOffset(gk2);

            p1->Offset(wp_offset1, 0.0);
            p2->Offset(wp_offset2, 0.0);

            // Add sockets to the display objects at these points
            CComQIPtr<iConnectable> c1(do1);
            CComQIPtr<iConnectable> c2(do2);
            CComPtr<iSocket> s1, s2;
            c1->AddSocket(0, p1, &s1);
            c2->AddSocket(0, p2, &s2);

            // save the first and last sockets for use with creating
            // the slab overhang dimensions
            if (firstGdrIdx == 0)
            {
               firstSocket = s1;
            }

            if (lastGdrIdx == nGirders - 1)
            {
               lastSocket = s2;
            }

            // get the plugs
            CComPtr<iPlug> startPlug;
            CComPtr<iPlug> endPlug;
            connector->GetStartPlug(&startPlug);
            connector->GetEndPlug(&endPlug);

            // connect the dimension line
            DWORD dwCookie;
            s1->Connect(startPlug, &dwCookie);
            s2->Connect(endPlug, &dwCookie);

            // Orient dimension line
            witness_length = doDimLine->GetWitnessLength();
            witness_length *= -1;
            doDimLine->SetWitnessLength(witness_length);

            //
            // Develop the text for the dimension line
            //
            CComPtr<iTextBlock> text;
            text.CoCreateInstance(CLSID_TextBlock);
            text->SetBkMode(TRANSPARENT);

            CString strSpacing = FormatDimension(spacing, rlen);
            CString strTotal = FormatDimension(total, rlen);

            if (0 < nSpacesInGroup)
            {
               std::_tostringstream os;
               if (nSpacesInGroup == 1)
               {
                  os << (LPCTSTR)strSpacing;
               }
               else
               {
                  os << nSpacesInGroup << _T(" spaces @ ") << (LPCTSTR)strSpacing << _T(" = ") << (LPCTSTR)strTotal;
               }

               text->SetText(os.str().c_str());

               doDimLine->SetTextBlock(text);

               display_list->AddDisplayObject(doDimLine);
            }
         } // end if do1 && do2
      } // end group loop
   }

   //
   // Curb-to-Curb dimension line
   //
   CComPtr<iDisplayList> tb_list;
   dispMgr->FindDisplayList(TRAFFIC_BARRIER_DISPLAY_LIST,&tb_list);

   // get the slab display object
   CComPtr<iDisplayObject> doLeftTB, doRightTB;
   tb_list->GetDisplayObject(0, &doLeftTB);
   tb_list->GetDisplayObject(1, &doRightTB);

   if ( doLeftTB && doRightTB )
   {
      // need both traffic barriers to add dimension line
      CComPtr<iDimensionLine> curbDimLine;
      curbDimLine.CoCreateInstance(CLSID_DimensionLineDisplayObject);
      CComPtr<iConnector> connector;
      curbDimLine.QueryInterface(&connector);

      CComQIPtr<iConnectable> left_connectable(doLeftTB);
      CComQIPtr<iConnectable> right_connectable(doRightTB);
      CComPtr<iSocket> left_socket, right_socket;
      left_connectable->GetSocket(LEFT_CURB_SOCKET, atByID,&left_socket);
      right_connectable->GetSocket(RIGHT_CURB_SOCKET,atByID,&right_socket);

      CComQIPtr<iConnector> curbConnector(curbDimLine);
      CComPtr<iPlug> startPlug, endPlug;
      curbConnector->GetStartPlug(&startPlug);
      curbConnector->GetEndPlug(&endPlug);

      DWORD dwCookie;
      left_socket->Connect(startPlug,&dwCookie);
      right_socket->Connect(endPlug,&dwCookie);

      CComPtr<iTextBlock> ccText;
      ccText.CoCreateInstance(CLSID_TextBlock);

      ccText->SetBkMode(TRANSPARENT);

      Float64 ccWidth = pBridge->GetCurbToCurbWidth( Xb );
      CString strCurb = FormatDimension(ccWidth,rlen);
      ccText->SetText(strCurb);

      // increase witness line length
      long witness_length = curbDimLine->GetWitnessLength();
      witness_length *= 3;
      curbDimLine->SetWitnessLength(witness_length);

      curbDimLine->SetTextBlock(ccText);

      display_list->AddDisplayObject(curbDimLine);
   }

   //
   // Barrier Width dimension line
   //
   GET_IFACE2(pBroker,IBarriers,pBarriers);
   if ( doLeftTB )
   {
      Float64 width = pBarriers->GetInterfaceWidth(pgsTypes::tboLeft);
      if ( 0 < width )
      {
         CComPtr<iDimensionLine> tbDimLine;
         tbDimLine.CoCreateInstance(CLSID_DimensionLineDisplayObject);
         CComPtr<iConnector> connector;
         tbDimLine.QueryInterface(&connector);

         CComQIPtr<iConnectable> tbConnectable(doLeftTB);
         CComPtr<iSocket> left_socket, right_socket;
         tbConnectable->GetSocket(LEFT_SLAB_EDGE_SOCKET, atByID,&left_socket);
         tbConnectable->GetSocket(LEFT_CURB_SOCKET,      atByID,&right_socket);

         if ( left_socket && right_socket )
         {
            CComQIPtr<iConnector> tbConnector(tbDimLine);
            CComPtr<iPlug> startPlug, endPlug;
            tbConnector->GetStartPlug(&startPlug);
            tbConnector->GetEndPlug(&endPlug);

            DWORD dwCookie;
            left_socket->Connect(startPlug,&dwCookie);
            right_socket->Connect(endPlug,&dwCookie);

            CComPtr<iTextBlock> ccText;
            ccText.CoCreateInstance(CLSID_TextBlock);
            ccText->SetBkMode(TRANSPARENT);
            CString strCurb = FormatDimension(width,rlen);
            ccText->SetText(strCurb);

            tbDimLine->SetTextBlock(ccText);

            // increase witness line length
            long witness_length = tbDimLine->GetWitnessLength();
            witness_length *= 3;
            tbDimLine->SetWitnessLength(witness_length);

            display_list->AddDisplayObject(tbDimLine);
         }
      }
   }

   if ( doRightTB )
   {
      Float64 width = pBarriers->GetInterfaceWidth(pgsTypes::tboRight);
      if ( 0 < width )
      {
         CComPtr<iDimensionLine> tbDimLine;
         tbDimLine.CoCreateInstance(CLSID_DimensionLineDisplayObject);
         CComPtr<iConnector> connector;
         tbDimLine.QueryInterface(&connector);

         CComQIPtr<iConnectable> tbConnectable(doRightTB);
         CComPtr<iSocket> left_socket, right_socket;
         tbConnectable->GetSocket(RIGHT_CURB_SOCKET,      atByID,&left_socket);
         tbConnectable->GetSocket(RIGHT_SLAB_EDGE_SOCKET, atByID,&right_socket);

         if ( left_socket && right_socket )
         {
            CComQIPtr<iConnector> tbConnector(tbDimLine);
            CComPtr<iPlug> startPlug, endPlug;
            tbConnector->GetStartPlug(&startPlug);
            tbConnector->GetEndPlug(&endPlug);

            DWORD dwCookie;
            left_socket->Connect(startPlug,&dwCookie);
            right_socket->Connect(endPlug,&dwCookie);

            CComPtr<iTextBlock> ccText;
            ccText.CoCreateInstance(CLSID_TextBlock);
            ccText->SetBkMode(TRANSPARENT);
            CString strCurb = FormatDimension(width,rlen);
            ccText->SetText(strCurb);

            tbDimLine->SetTextBlock(ccText);

            // increase witness line length
            long witness_length = tbDimLine->GetWitnessLength();
            witness_length *= 3;
            tbDimLine->SetWitnessLength(witness_length);

            display_list->AddDisplayObject(tbDimLine);
         }
      }
   }

   // sidewalk dimension lines
   if (doLeftTB && pBarriers->HasSidewalk(pgsTypes::tboLeft))
   {
      Float64 ext_edge, int_edge;
      pBarriers->GetSidewalkPedLoadEdges(pgsTypes::tboLeft,&int_edge,&ext_edge);

      Float64 width = int_edge-ext_edge;
      if ( 0 < width )
      {
         CComPtr<iDimensionLine> swDimLine;
         swDimLine.CoCreateInstance(CLSID_DimensionLineDisplayObject);
         CComPtr<iConnector> connector;
         swDimLine.QueryInterface(&connector);

         CComQIPtr<iConnectable> swConnectable(doLeftTB);
         CComPtr<iSocket> left_socket, right_socket;
         swConnectable->GetSocket(LEFT_EXT_SW_SOCKET, atByID,&left_socket);
         swConnectable->GetSocket(LEFT_INT_SW_SOCKET, atByID,&right_socket);

         if ( left_socket && right_socket )
         {
            CComQIPtr<iConnector> swConnector(swDimLine);
            CComPtr<iPlug> startPlug, endPlug;
            swConnector->GetStartPlug(&startPlug);
            swConnector->GetEndPlug(&endPlug);

            DWORD dwCookie;
            left_socket->Connect(startPlug,&dwCookie);
            right_socket->Connect(endPlug,&dwCookie);

            CComPtr<iTextBlock> ccText;
            ccText.CoCreateInstance(CLSID_TextBlock);
            ccText->SetBkMode(TRANSPARENT);
            CString strCurb = FormatDimension(width,rlen);
            ccText->SetText(strCurb);

            witness_length = swDimLine->GetWitnessLength();
            witness_length *= 4;
            swDimLine->SetWitnessLength(witness_length);

            swDimLine->SetTextBlock(ccText);

            display_list->AddDisplayObject(swDimLine);
         }
      }
   }

   if (doRightTB && pBarriers->HasSidewalk(pgsTypes::tboRight))
   {
      Float64 ext_edge, int_edge;
      pBarriers->GetSidewalkPedLoadEdges(pgsTypes::tboRight,&int_edge,&ext_edge);

      Float64 width = int_edge-ext_edge;
      if ( 0 < width )
      {
         CComPtr<iDimensionLine> swDimLine;
         swDimLine.CoCreateInstance(CLSID_DimensionLineDisplayObject);
         CComPtr<iConnector> connector;
         swDimLine.QueryInterface(&connector);

         CComQIPtr<iConnectable> swConnectable(doRightTB);
         CComPtr<iSocket> Right_socket, right_socket;
         swConnectable->GetSocket(RIGHT_INT_SW_SOCKET, atByID,&Right_socket);
         swConnectable->GetSocket(RIGHT_EXT_SW_SOCKET, atByID,&right_socket);

         if ( Right_socket && right_socket )
         {
            CComQIPtr<iConnector> swConnector(swDimLine);
            CComPtr<iPlug> startPlug, endPlug;
            swConnector->GetStartPlug(&startPlug);
            swConnector->GetEndPlug(&endPlug);

            DWORD dwCookie;
            Right_socket->Connect(startPlug,&dwCookie);
            right_socket->Connect(endPlug,&dwCookie);

            CComPtr<iTextBlock> ccText;
            ccText.CoCreateInstance(CLSID_TextBlock);
            ccText->SetBkMode(TRANSPARENT);
            CString strCurb = FormatDimension(width,rlen);
            ccText->SetText(strCurb);

            witness_length = swDimLine->GetWitnessLength();
            witness_length *= 4;
            swDimLine->SetWitnessLength(witness_length);

            swDimLine->SetTextBlock(ccText);

            display_list->AddDisplayObject(swDimLine);
         }
      }
   }

   // Interior overlay width
   if (doLeftTB && pBridge->HasOverlay())
   {
      Float64 left_icb_offset, right_icb_offset;
      left_icb_offset  = pBridge->GetLeftOverlayToeOffset(Xb);
      right_icb_offset = pBridge->GetRightOverlayToeOffset(Xb);

      Float64 width = right_icb_offset-left_icb_offset;
      if ( 0 < width )
      {
         CComPtr<iDimensionLine> icbDimLine;
         icbDimLine.CoCreateInstance(CLSID_DimensionLineDisplayObject);
         CComPtr<iConnector> connector;
         icbDimLine.QueryInterface(&connector);

         CComQIPtr<iConnectable> icbConnectable(doLeftTB);
         CComPtr<iSocket> left_socket, right_socket;
         icbConnectable->GetSocket(LEFT_INT_OVERLAY_SOCKET, atByID,&left_socket);
         icbConnectable->GetSocket(RIGHT_INT_OVERLAY_SOCKET, atByID,&right_socket);

         if ( left_socket && right_socket )
         {
            CComQIPtr<iConnector> icbConnector(icbDimLine);
            CComPtr<iPlug> startPlug, endPlug;
            icbConnector->GetStartPlug(&startPlug);
            icbConnector->GetEndPlug(&endPlug);

            DWORD dwCookie;
            left_socket->Connect(startPlug,&dwCookie);
            right_socket->Connect(endPlug,&dwCookie);

            CComPtr<iTextBlock> ccText;
            ccText.CoCreateInstance(CLSID_TextBlock);
            ccText->SetBkMode(TRANSPARENT);
            CString strCurb = FormatDimension(width,rlen);
            ccText->SetText(strCurb);

            icbDimLine->SetTextBlock(ccText);

            witness_length = icbDimLine->GetWitnessLength();
            witness_length *= 4;
            icbDimLine->SetWitnessLength(witness_length);

            display_list->AddDisplayObject(icbDimLine);
         }
      }
   }
}

void CBridgeSectionView::BuildAlignmentDisplayObjects()
{
   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   CDManipClientDC dc(this);

   CComPtr<iDisplayList> displayList;
   dispMgr->FindDisplayList(ALIGNMENT_DISPLAY_LIST,&displayList);

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker, IRoadway, pAlignment);
   GET_IFACE2(pBroker, IRoadwayData, pRoadwayData);

   Float64 cut_station = m_pFrame->GetCurrentCutLocation();

   // model vertical line for the alignment
   // The alignment is at X=0 in Bridge Section Coordinates
   Float64 Xcl = 0;
   Float64 Ydeck = pAlignment->GetElevation(cut_station,0); // deck elevation at alignment
   Float64 Yt = Ydeck + WBFL::Units::ConvertToSysUnits(1.0,WBFL::Units::Measure::Feet); // add a little so it projects over the roadway surface

   CComPtr<IPoint2d> pnt1;
   pnt1.CoCreateInstance(CLSID_Point2d);
   pnt1->Move(Xcl,Yt);

   CComPtr<IPoint2d> pnt2;
   pnt2.CoCreateInstance(CLSID_Point2d);
   pnt2->Move(Xcl,Yt - WBFL::Units::ConvertToSysUnits(3.0,WBFL::Units::Measure::Feet));

   CComPtr<iLineDisplayObject> doAlignment;
   CreateLineDisplayObject(pnt1,pnt2,&doAlignment);
   CComPtr<iDrawLineStrategy> drawStrategy;
   doAlignment->GetDrawLineStrategy(&drawStrategy);
   CComQIPtr<iSimpleDrawLineStrategy> drawAlignmentStrategy(drawStrategy);
   drawAlignmentStrategy->SetWidth(ALIGNMENT_LINE_WEIGHT);
   drawAlignmentStrategy->SetColor(ALIGNMENT_COLOR);
   drawAlignmentStrategy->SetLineStyle(lsCenterline);

   displayList->AddDisplayObject(doAlignment);

   CComPtr<iTextBlock> doText;
   doText.CoCreateInstance(CLSID_TextBlock);
   doText->SetPosition(pnt1);
   doText->SetText(pRoadwayData->GetAlignmentData2().Name.c_str());
   doText->SetTextAlign(TA_BASELINE | TA_CENTER);
   doText->SetBkMode(TRANSPARENT);
   displayList->AddDisplayObject(doText);

   // draw bridge line if different then the alignment
   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 BLO = pBridge->GetAlignmentOffset();
   if ( !IsZero(BLO) )
   {
      Ydeck = pAlignment->GetElevation(cut_station,BLO);
      Yt = Ydeck + WBFL::Units::ConvertToSysUnits(1.0,WBFL::Units::Measure::Feet);
      pnt1.Release();
      pnt2.Release();

      pnt1.CoCreateInstance(CLSID_Point2d);
      pnt1->Move(Xcl+BLO,Yt);

      pnt2.CoCreateInstance(CLSID_Point2d);
      pnt2->Move(Xcl+BLO,Yt - WBFL::Units::ConvertToSysUnits(3.0,WBFL::Units::Measure::Feet));

      CComPtr<iLineDisplayObject> doBridgeLine;
      CreateLineDisplayObject(pnt1,pnt2,&doBridgeLine);
      CComPtr<iDrawLineStrategy> drawStrategy;
      doBridgeLine->GetDrawLineStrategy(&drawStrategy);
      CComQIPtr<iSimpleDrawLineStrategy> drawBridgeLineStrategy(drawStrategy);
      drawBridgeLineStrategy->SetWidth(BRIDGELINE_LINE_WEIGHT);
      drawBridgeLineStrategy->SetColor(BRIDGE_COLOR);
      drawBridgeLineStrategy->SetLineStyle(lsCenterline);

      displayList->AddDisplayObject(doBridgeLine);

      // Need to add a dimension line between the alignment and the BLO
   }

   if (pRoadwayData->GetRoadwaySectionData().AlignmentPointIdx != pRoadwayData->GetRoadwaySectionData().ProfileGradePointIdx)
   {
      IndexType alignmentIdx = pAlignment->GetAlignmentPointIndex(cut_station); // get index of crown point corresponding to the alignment
      Float64 offset = pAlignment->GetProfileGradeLineOffset(alignmentIdx, cut_station); // get the offset from the alignment point to the PGL

      ATLASSERT(!IsZero(offset)); // only drawing PGL if it is offset from alignment so this better not be zero
      Float64 pgl_offset = -offset;

      Ydeck = pAlignment->GetElevation(cut_station, pgl_offset);
      Yt = Ydeck + WBFL::Units::ConvertToSysUnits(1.0, WBFL::Units::Measure::Feet);
      pnt1.Release();
      pnt2.Release();

      pnt1.CoCreateInstance(CLSID_Point2d);
      pnt1->Move(Xcl + pgl_offset, Yt);

      pnt2.CoCreateInstance(CLSID_Point2d);
      pnt2->Move(Xcl + pgl_offset, Yt - WBFL::Units::ConvertToSysUnits(3.0, WBFL::Units::Measure::Feet));

      CComPtr<iLineDisplayObject> doPGL;
      CreateLineDisplayObject(pnt1, pnt2, &doPGL);
      CComPtr<iDrawLineStrategy> drawStrategy;
      doPGL->GetDrawLineStrategy(&drawStrategy);
      CComQIPtr<iSimpleDrawLineStrategy> drawPGLStrategy(drawStrategy);
      drawPGLStrategy->SetWidth(PROFILE_LINE_WEIGHT);
      drawPGLStrategy->SetColor(PROFILE_COLOR);
      drawPGLStrategy->SetLineStyle(lsCenterline);

      displayList->AddDisplayObject(doPGL);

      CComPtr<iTextBlock> doText;
      doText.CoCreateInstance(CLSID_TextBlock);
      doText->SetPosition(pnt1);
      doText->SetText(_T("PGL"));
      doText->SetTextAlign(TA_BASELINE | TA_CENTER);
      doText->SetBkMode(TRANSPARENT);
      displayList->AddDisplayObject(doText);
   }
}

void CBridgeSectionView::BuildRoadwayCrossSectionDisplayObjects()
{
   CPGSDocBase* pDoc = (CPGSDocBase*)GetDocument();

   UINT settings = pDoc->GetBridgeEditorSettings();

   if (!(settings & IDB_CS_DRAW_RW_CS))
   {
      return;
   }

   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   CDManipClientDC dc(this);

   CComPtr<iDisplayList> displayList;
   dispMgr->FindDisplayList(RW_CROSS_SECTION_DISPLAY_LIST, &displayList);

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker, IRoadway, pRoadway);
   GET_IFACE2(pBroker, IRoadwayData, pRoadwayData);
   GET_IFACE2(pBroker, IBridge, pBridge);
   GET_IFACE2(pBroker, IPointOfInterest, pPoi);
   GET_IFACE2(pBroker, IEAFDisplayUnits, pDisplayUnits);

   CString strDimTag = pDisplayUnits->GetAlignmentLengthUnit().UnitOfMeasure.UnitTag().c_str();
   CString strSlopeTag = strDimTag + _T("/") + strDimTag;

   Float64 cut_station = m_pFrame->GetCurrentCutLocation();
   Float64 Xb = pPoi->ConvertRouteToBridgeLineCoordinate(cut_station);

   Float64 feets = WBFL::Units::ConvertToSysUnits(1.0, WBFL::Units::Measure::Feet); //  make sure alignment is also in there
   Float64 left_offset = pBridge->GetLeftSlabEdgeOffset(Xb);
   left_offset = min(left_offset, 0.0);
   Float64 right_offset = pBridge->GetRightSlabEdgeOffset(Xb);
   right_offset = max(right_offset, 0.0);

   left_offset -= feets; // display line a bit wider than bridge.
   right_offset += feets;

   Float64 left_elev = pRoadway->GetElevation(cut_station, left_offset);

   // The alignment is represented on the screen by a poly line object
   CComPtr<iPolyLineDisplayObject> doAlignment;
   doAlignment.CoCreateInstance(CLSID_PolyLineDisplayObject);

   // Register an event sink with the alignment object so that we can handle double clicks
   // on the alignment differently then a general dbl-click
   CAlignmentDisplayObjectEvents* pEvents = new CAlignmentDisplayObjectEvents(pBroker, m_pFrame, CAlignmentDisplayObjectEvents::BridgeSection);
   CComPtr<iDisplayObjectEvents> events;
   events.Attach((iDisplayObjectEvents*)pEvents->GetInterface(&IID_iDisplayObjectEvents));

   CComPtr<iDisplayObject> dispObj;
   doAlignment->QueryInterface(IID_iDisplayObject, (void**)&dispObj);
   dispObj->RegisterEventSink(events);
   dispObj->SetToolTipText(_T("Double click to edit alignment"));
   dispObj->SetMaxTipWidth(TOOLTIP_WIDTH);
   dispObj->SetTipDisplayTime(TOOLTIP_DURATION);

   CComPtr<IPoint2d> pnt1, pnt2;
   pnt1.CoCreateInstance(CLSID_Point2d);
   pnt1->Move(left_offset, left_elev);
   doAlignment->AddPoint(pnt1);

   bool bfinished = false;
   IndexType contrl_crown_point = pRoadway->GetAlignmentPointIndex(cut_station);
   IndexType num_crown_points = pRoadway->GetCrownPointIndexCount(cut_station);
   for (IndexType icp = 0; icp < num_crown_points; icp++)
   {
      Float64 cp_offset = pRoadway->GetAlignmentOffset(icp, cut_station);
      Float64 cp_elev = pRoadway->GetElevation(cut_station, cp_offset);

      if (left_offset < cp_offset && cp_offset < right_offset)
      {
         pnt2.CoCreateInstance(CLSID_Point2d);
         pnt2->Move(cp_offset, cp_elev);
      }
      else if (right_offset < cp_offset)
      {
         bfinished = true;
         cp_offset = right_offset;
         cp_elev = pRoadway->GetElevation(cut_station, right_offset);

         pnt2.CoCreateInstance(CLSID_Point2d);
         pnt2->Move(cp_offset, cp_elev);
      }

      if (pnt2)
      {
         doAlignment->AddPoint(pnt2);

         if (!bfinished)
         {
            // ridge points
            CComPtr<iPointDisplayObject> dispPnt;
            dispPnt.CoCreateInstance(CLSID_PointDisplayObject);
            dispPnt->SetPosition(pnt2,FALSE,FALSE);

            CComPtr<iDrawPointStrategy> draw_point_strategy;
            dispPnt->GetDrawingStrategy(&draw_point_strategy);
            CComQIPtr<iSimpleDrawPointStrategy> drawRwXsPointStrategy(draw_point_strategy);
            drawRwXsPointStrategy->SetColor(ALIGNMENT_COLOR);
            drawRwXsPointStrategy->SetLogicalPointSize((icp==contrl_crown_point) ? 12 : 8); // make controlling point bigger
            drawRwXsPointStrategy->SetPointType(ptCircle);

            displayList->AddDisplayObject(dispPnt);
         }

         // Slope text
         CComPtr<IPoint2d> txtLoc;
         txtLoc.CoCreateInstance(CLSID_Point2d);
         Float64 x1, x2, y1, y2;
         pnt1->get_X(&x1);
         pnt1->get_Y(&y1);
         pnt2->get_X(&x2);
         pnt2->get_Y(&y2);
         txtLoc->Move((x1+x2)/2.0, (y1+y2)/2.0);

         Float64 slope(0.0);
         if (!IsEqual(x1, x2))
         {
            slope = (y2-y1)/(x2-x1);
         }

         CComPtr<iTextBlock> doText;
         doText.CoCreateInstance(CLSID_TextBlock);
         doText->SetPosition(txtLoc);
         Float64 angle = atan(slope) * 1800.0 / M_PI;
         doText->SetAngle((LONG)angle); 

         // change the slope from our analytical model to
         // the sign convention of the input
         // slopes down and away from the controlling crown point are < 0
         if (pRoadwayData->GetRoadwaySectionData().slopeMeasure == RoadwaySectionData::RelativeToAlignmentPoint && icp <= contrl_crown_point)
         {
            slope *= -1;
         }

         CString strLabel;
         strLabel.Format(_T("%.3f %s"), slope, strSlopeTag);
         doText->SetText(strLabel);
         doText->SetBkMode(TRANSPARENT);
         doText->SetTextColor(ALIGNMENT_COLOR);
         doText->SetTextAlign(TA_CENTER | TA_BOTTOM);
         displayList->AddDisplayObject(doText);

         // clean up for next point
         pnt1.Release();
         pnt2->Clone(&pnt1);
         pnt2.Release();
      }

      if (bfinished)
      {
         break;
      }
   }

   doAlignment->put_Width(ALIGNMENT_LINE_WEIGHT);
   doAlignment->put_Color(ALIGNMENT_COLOR);
   doAlignment->put_PointType(plpNone);
   doAlignment->Commit();

   doAlignment->SetSelectionType(stAll);
   doAlignment->SetID(ALIGNMENT_ID);

   displayList->AddDisplayObject(doAlignment);

   // links the display object to the event handler
   // the display object isn't really a drop site, however the drop
   // site interface has the method we need. contrast this with the
   // bridge plan view where the alignment display object is really a drop site
   CComQIPtr<iDropSite> drop_site(events);
   drop_site->SetDisplayObject(dispObj);
}

void CBridgeSectionView::CreateLineDisplayObject(IPoint2d* pntStart,IPoint2d* pntEnd,iLineDisplayObject** ppLineDO)
{
   CComPtr<iPointDisplayObject> doPntStart;
   doPntStart.CoCreateInstance(CLSID_PointDisplayObject);
   doPntStart->Visible(FALSE);
   doPntStart->SetPosition(pntStart,FALSE,FALSE);
   CComQIPtr<iConnectable> connectable1(doPntStart);
   CComPtr<iSocket> socket1;
   connectable1->AddSocket(0,pntStart,&socket1);

   CComPtr<iPointDisplayObject> doPntEnd;
   doPntEnd.CoCreateInstance(CLSID_PointDisplayObject);
   doPntEnd->Visible(FALSE);
   doPntEnd->SetPosition(pntEnd,FALSE,FALSE);
   CComQIPtr<iConnectable> connectable2(doPntEnd);
   CComPtr<iSocket> socket2;
   connectable2->AddSocket(0,pntEnd,&socket2);

   CComPtr<iLineDisplayObject> doLine;
   doLine.CoCreateInstance(CLSID_LineDisplayObject);

   CComQIPtr<iConnector> connector(doLine);
   CComPtr<iPlug> startPlug, endPlug;
   connector->GetStartPlug(&startPlug);
   connector->GetEndPlug(&endPlug);
   DWORD dwCookie;
   connectable1->Connect(0,atByID,startPlug,&dwCookie);
   connectable2->Connect(0,atByID,endPlug,  &dwCookie);

   doLine.CopyTo(ppLineDO);
}

void CBridgeSectionView::UpdateDrawingScale()
{
   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   CComPtr<iDisplayList> display_list;
   dispMgr->FindDisplayList(TITLE_DISPLAY_LIST,&display_list);

   if ( display_list == nullptr )
   {
      return;
   }

   CDManipClientDC dc(this);

   // before scaling the drawing to fit, hide the title display objects
   // if they aren't hidden, they factor into the bounding box and they
   // mess up the scaling of the drawing.
   //
   // this is the best solution I've been able to come up with.
   display_list->HideDisplayObjects(true);

   ScaleToFit();

   display_list->HideDisplayObjects(false);
}

GroupIndexType CBridgeSectionView::GetGroupIndex()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   
   Float64 cut_station = m_pFrame->GetCurrentCutLocation();

   GroupIndexType nGroups = pBridgeDesc->GetGirderGroupCount();
   for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
   {
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(grpIdx);
      const CPierData2* pStartPier = pGroup->GetPier(pgsTypes::metStart);
      const CPierData2* pEndPier   = pGroup->GetPier(pgsTypes::metEnd);

      Float64 prev_pier_station = pStartPier->GetStation();
      Float64 next_pier_station = pEndPier->GetStation();

      if ( grpIdx == nGroups-1 )
      {
         // Include end station for last group
         if ( ::IsLE(prev_pier_station,cut_station) && ::IsLE(cut_station,next_pier_station) )
         {
            return pGroup->GetIndex();
         }
      }
      else
      {
         // Exclue end station for all other groups
         // Section cuts look ahead on station so if we are cutting at a group
         // boundary, we want the next group
         if ( ::IsLE(prev_pier_station,cut_station) && ::IsLT(cut_station,next_pier_station) )
         {
            return pGroup->GetIndex();
         }
      }
   }

   // if a group isn't found, return 0 so that there is a chance of drawing a cross section
   return 0;
}

void CBridgeSectionView::ClearSelection()
{
   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   dispMgr->ClearSelectedObjects();
}

void CBridgeSectionView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
   if ( nChar == VK_UP && ::GetKeyState(VK_CONTROL) < 0 )
   {
      m_pFrame->GetBridgePlanView()->SetFocus();
      return;
   }
   else if ( nChar == VK_RIGHT )
   {
      CComPtr<iDisplayMgr> dispMgr;
      GetDisplayMgr(&dispMgr);
      DisplayObjectContainer selObjs;
      dispMgr->GetSelectedObjects(&selObjs);

      if ( selObjs.size() == 0 )
      {
         GroupIndexType grpIdx = GetGroupIndex();
         m_pFrame->SelectGirder(CSegmentKey(grpIdx,0,INVALID_INDEX));
         return;
      }
   }
   else if ( nChar == VK_LEFT )
   {
      CComPtr<iDisplayMgr> dispMgr;
      GetDisplayMgr(&dispMgr);
      DisplayObjectContainer selObjs;
      dispMgr->GetSelectedObjects(&selObjs);

      if ( selObjs.size() == 0 )
      {
         GroupIndexType grpIdx = GetGroupIndex();
         CComPtr<IBroker> pBroker;
         EAFGetBroker(&pBroker);
         GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
         const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
         const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(grpIdx);
         m_pFrame->SelectGirder(CSegmentKey(grpIdx,pGroup->GetGirderCount()-1,INVALID_INDEX));
         return;
      }
   }


	CBridgeViewPane::OnKeyDown(nChar, nRepCnt, nFlags);
}

BOOL CBridgeSectionView::OnMouseWheel(UINT nFlags,short zDelta,CPoint pt)
{
   OnKeyDown(zDelta < 0 ? VK_RIGHT : VK_LEFT, 1, nFlags);
   return TRUE;
}

void CBridgeSectionView::TrimSurface(IPoint2dCollection* pPoints,Float64 Xleft,Float64 Xright)
{
   IndexType nPoints;
   pPoints->get_Count(&nPoints);
   for ( IndexType idx = 0; idx < nPoints-1; idx++ )
   {
      CComPtr<IPoint2d> pnt1, pnt2;
      pPoints->get_Item(idx,  &pnt1);
      pPoints->get_Item(idx+1,&pnt2);

      Float64 x1,x2;
      pnt1->get_X(&x1);
      pnt2->get_X(&x2);

      if ( x1 < Xleft && x2 < Xleft )
      {
         // pnt1 and pnt2 are to the left of the left trimming line
         // remove pnt1 and try again
         ATLASSERT(idx == 0);
         pPoints->Remove(idx);
         TrimSurface(pPoints,Xleft,Xright);
         return;
      }

      if ( x1 < Xleft && Xleft < x2 )
      {
         // found an intersecting line segment
         // find the intersection point at make this the left point
         ATLASSERT(idx == 0);
         Float64 y1,y2;
         pnt1->get_Y(&y1);
         pnt2->get_Y(&y2);

         Float64 y = ::LinInterp(Xleft-x1,y1,y2,x2-x1);
         pnt1->Move(Xleft,y);
      }

      if ( x1 < Xright && Xright < x2 )
      {
         // found an interscting line segment
         // find the intersection point and make this the right point
         Float64 y1,y2;
         pnt1->get_Y(&y1);
         pnt2->get_Y(&y2);

         Float64 y = ::LinInterp(Xright-x1,y1,y2,x2-x1);
         pnt2->Move(Xright,y);

         // remove all remaining points
         for ( IndexType i = idx+2; i < nPoints-1; i++ )
         {
            pPoints->Remove(idx+2);
         }
         return; // leave now, the point container has been changed
      }
   }
}

std::vector<pgsPointOfInterest> CBridgeSectionView::GetPointsOfInterest()
{
   // returns the POI we need for the girder cross sections
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker, IRoadway, pAlignment);
   GET_IFACE2(pBroker, IPointOfInterest, pPoi);

   GroupIndexType grpIdx = GetGroupIndex();

   GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   GroupIndexType nGroups = pBridgeDesc->GetGirderGroupCount();
   GirderIndexType nGirders = pBridgeDesc->GetGirderGroup(grpIdx)->GetGirderCount();

   Float64 cut_station = m_pFrame->GetCurrentCutLocation();

   CComPtr<IDirection> dirAlignmentNormal;
   pAlignment->GetBearingNormal(cut_station, &dirAlignmentNormal); // section cut is normal to the alignment

   std::vector<pgsPointOfInterest> vPoi;
   pPoi->GetPointsOfInterest(cut_station, dirAlignmentNormal, &vPoi);
   if (vPoi.size() < nGirders)
   {
      // the cut line doesn't intersect some of the girders. this can happen when the section cut is at or near
      // a pier, especially at the ends of the bridge.
      // find the missing girders and use the POI at the start face so there is something to draw

      // all possible girders
      std::vector<GirderIndexType> vAllGirders;
      vAllGirders.resize(nGirders);
      std::generate(vAllGirders.begin(), vAllGirders.end(), IncrementValue<GirderIndexType>(0)); // fill container with sequential values begining with 0

                                                                                                 // all the girders we found
      std::vector<GirderIndexType> vFoundGirders;
      for (const auto& poi : vPoi)
      {
         vFoundGirders.push_back(poi.GetSegmentKey().girderIndex);
      }
      std::sort(vFoundGirders.begin(), vFoundGirders.end());

      // missing girders = all girders - found girders
      std::vector<GirderIndexType> vMissingGirders(vAllGirders.size());
      std::vector<GirderIndexType>::iterator end = std::set_difference(vAllGirders.begin(), vAllGirders.end(), vFoundGirders.begin(), vFoundGirders.end(), vMissingGirders.begin());
      vMissingGirders.resize(end - vMissingGirders.begin());

      // for each missing girder, get a POI to use
      for (const auto& gdrIdx : vMissingGirders)
      {
         CGirderKey girderKey(grpIdx, gdrIdx);
         pgsPointOfInterest poi;
         VERIFY(pPoi->GetPointOfInterest(girderKey, cut_station, dirAlignmentNormal, true, &poi));
         vPoi.push_back(poi);
      }
   }

   return vPoi;
}

void CBridgeSectionView::SkewGirderShape(Float64 skew, Float64 shear, IShape* pShape, IShape** ppSkewedShape)
{
   if (IsZero(skew))
   {
      // Not skewed... nothing to do
      (*ppSkewedShape) = pShape;
      (*ppSkewedShape)->AddRef();
      return;
   }

   CComPtr<IRect2d> bbox;
   pShape->get_BoundingBox(&bbox);
   CComPtr<IPoint2d> pntTC;
   bbox->get_TopCenter(&pntTC);
   Float64 xcl,ycl;
   pntTC->Location(&xcl, &ycl);

   CComPtr<IPoint2dCollection> points;
   pShape->get_PolyPoints(&points);
   CComPtr<IPoint2d> pnt;
   CComPtr<IEnumPoint2d> enumPoints;
   points->get__Enum(&enumPoints);
   while (enumPoints->Next(1, &pnt, nullptr) != S_FALSE)
   {
      Float64 x, y;
      pnt->Location(&x, &y);

      y -= ycl; // translate
      y += shear*(x - xcl); // shear
      y += ycl; // translate

      x -= xcl; // translate
      x /= cos(skew); // skew
      x += xcl; // translate

      pnt->Move(x, y);

      pnt.Release();
   }

   CComPtr<IPolyShape> polyShape;
   polyShape.CoCreateInstance(CLSID_PolyShape);
   polyShape->AddPoints(points);

   CComQIPtr<IShape> s(polyShape);
   s.CopyTo(ppSkewedShape);
}
