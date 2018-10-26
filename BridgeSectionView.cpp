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

// BridgeSectionView.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\resource.h"
#include "PGSuperAppPlugin\PGSuperApp.h"
#include "PGSuperDocBase.h"
#include "BridgeSectionView.h"
#include "SlabDisplayObjectEvents.h"
#include "BridgePlanView.h"

#include "PGSuperUnits.h"
#include "PGSuperColors.h"

#include <PgsExt\PierData.h>
#include <PgsExt\BridgeDescription2.h>

#include <IFace\DrawBridgeSettings.h>
#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <IFace\Alignment.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\EditByUI.h>
#include <IFace\Intervals.h>

#include <GraphicsLib\GraphTool.h>

#include "GirderDisplayObjectEvents.h"

#include <WBFLDManip.h>
#include <DManipTools\DManipTools.h>

#include <Material\Material.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// Display List Identifiers
#define TITLE_DISPLAY_LIST             0
#define GIRDER_DISPLAY_LIST            1
#define SLAB_DISPLAY_LIST              2
#define OVERLAY_DISPLAY_LIST           3
#define DIMENSION_DISPLAY_LIST         4
#define GIRDER_LABEL_DISPLAY_LIST      5
#define TRAFFIC_BARRIER_DISPLAY_LIST   6

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

void CBridgeSectionView::SelectAlignment(bool bSelect)
{
   // sort of a dummy function to clear the selection in this view
   // when the alignment is selected in another view
   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);
   dispMgr->ClearSelectedObjects();
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
   CComPtr<iDisplayList> girder_label_list;
   ::CoCreateInstance(CLSID_DisplayList,NULL,CLSCTX_ALL,IID_iDisplayList,(void**)&girder_label_list);
   girder_label_list->SetID(GIRDER_LABEL_DISPLAY_LIST);
   dispMgr->AddDisplayList(girder_label_list);

   CComPtr<iDisplayList> dim_line_list;
   ::CoCreateInstance(CLSID_DisplayList,NULL,CLSCTX_ALL,IID_iDisplayList,(void**)&dim_line_list);
   dim_line_list->SetID(DIMENSION_DISPLAY_LIST);
   dispMgr->AddDisplayList(dim_line_list);

   CComPtr<iDisplayList> title_list;
   ::CoCreateInstance(CLSID_DisplayList,NULL,CLSCTX_ALL,IID_iDisplayList,(void**)&title_list);
   title_list->SetID(TITLE_DISPLAY_LIST);
   dispMgr->AddDisplayList(title_list);

   CComPtr<iDisplayList> traffic_barrier_list;
   ::CoCreateInstance(CLSID_DisplayList,NULL,CLSCTX_ALL,IID_iDisplayList,(void**)&traffic_barrier_list);
   traffic_barrier_list->SetID(TRAFFIC_BARRIER_DISPLAY_LIST);
   dispMgr->AddDisplayList(traffic_barrier_list);

   CComPtr<iDisplayList> slab_list;
   ::CoCreateInstance(CLSID_DisplayList,NULL,CLSCTX_ALL,IID_iDisplayList,(void**)&slab_list);
   slab_list->SetID(SLAB_DISPLAY_LIST);
   dispMgr->AddDisplayList(slab_list);

   CComPtr<iDisplayList> overlay_list;
   ::CoCreateInstance(CLSID_DisplayList,NULL,CLSCTX_ALL,IID_iDisplayList,(void**)&overlay_list);
   overlay_list->SetID(OVERLAY_DISPLAY_LIST);
   dispMgr->AddDisplayList(overlay_list);

   CComPtr<iDisplayList> girder_list;
   ::CoCreateInstance(CLSID_DisplayList,NULL,CLSCTX_ALL,IID_iDisplayList,(void**)&girder_list);
   girder_list->SetID(GIRDER_DISPLAY_LIST);
   dispMgr->AddDisplayList(girder_list);

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
      str.Format(_T("%f, %f\r\n"),::ConvertFromSysUnits(x,unitMeasure::Feet),::ConvertFromSysUnits(y,unitMeasure::Feet));
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
        (lHint == HINT_GIRDERLABELFORMATCHANGED)
        )
   {
      UpdateDisplayObjects();
      UpdateDrawingScale();
   }
   else if ( lHint == HINT_GIRDERCHANGED )
   {
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

void CBridgeSectionView::HandleLButtonDblClk(UINT nFlags, CPoint point) 
{
   GetFrame()->SendMessage(WM_COMMAND,ID_PROJECT_BRIDGEDESC,0);
}

void CBridgeSectionView::HandleContextMenu(CWnd* pWnd,CPoint logPoint)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CPGSDocBase* pDoc = (CPGSDocBase*)GetDocument();
   CEAFMenu* pMenu = CEAFMenu::CreateContextMenu(pDoc->GetPluginCommandManager());
   pMenu->LoadMenu(IDR_BRIDGE_XSECTION_CTX,NULL);

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

   GET_IFACE2_NOCHECK(pBroker,IEAFDisplayUnits,pDisplayUnits);
   GET_IFACE2_NOCHECK(pBroker,IStrandGeometry,pStrandGeom);
   GET_IFACE2_NOCHECK(pBroker,IMaterials,pMaterial);

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   CComPtr<iDisplayList> girder_list;
   dispMgr->FindDisplayList(GIRDER_DISPLAY_LIST,&girder_list);
   GroupIndexType grpIdx = GetGroupIndex();

   GirderIndexType nGirders = pBridgeDesc->GetGirderGroup(grpIdx)->GetGirderCount();
   for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
   {
      CGirderKey girderKey(grpIdx,gdrIdx);
      GirderIDCollection::iterator found = m_GirderIDs.find(girderKey);
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

#pragma Reminder("UPDATE: assuming precast girder bridge")
      // need to get segment where section cut is
      CSegmentKey segmentKey(girderKey,0);

      const CPrecastSegmentData* pSegment = pBridgeDesc->GetGirderGroup(segmentKey.groupIndex)->GetGirder(segmentKey.girderIndex)->GetSegment(segmentKey.segmentIndex);

      CString strMsg1(_T("Double click to edit.\r\nRight click for more options."));

      Float64 fc  = pSegment->Material.Concrete.Fc;
      Float64 fci = pSegment->Material.Concrete.Fci;

      CString strMsg2;
      strMsg2.Format(_T("\r\n\r\nGirder: %s\r\n%s\r\nf'ci: %s\r\nf'c: %s"),
                     pBridgeDesc->GetGirderGroup(grpIdx)->GetGirder(gdrIdx)->GetGirderName(),
                     lrfdConcreteUtil::GetTypeName((matConcrete::Type)pMaterial->GetSegmentConcreteType(segmentKey),true).c_str(),
                     FormatDimension(fci,pDisplayUnits->GetStressUnit()),
                     FormatDimension(fc, pDisplayUnits->GetStressUnit())
                    );

      const matPsStrand* pStrand     = pMaterial->GetStrandMaterial(segmentKey,pgsTypes::Permanent);
      const matPsStrand* pTempStrand = pMaterial->GetStrandMaterial(segmentKey,pgsTypes::Temporary);

      StrandIndexType Ns, Nh, Nt, Nsd;
      Ns = pStrandGeom->GetStrandCount(segmentKey,pgsTypes::Straight);
      Nh = pStrandGeom->GetStrandCount(segmentKey,pgsTypes::Harped);
      Nt = pStrandGeom->GetStrandCount(segmentKey,pgsTypes::Temporary);
      Nsd= pStrandGeom->GetNumDebondedStrands(segmentKey,pgsTypes::Straight);

      std::_tstring harp_type(LABEL_HARP_TYPE(pStrandGeom->GetAreHarpedStrandsForcedStraight(segmentKey)));

      CString strMsg3;
      if ( pStrandGeom->GetMaxStrands(segmentKey,pgsTypes::Temporary) != 0 )
      {
         if ( Nsd == 0 )
         {
            strMsg3.Format(_T("\r\n\r\nStrand: %s\r\n# Straight: %2d\r\n# %s: %2d\r\n\r\n%s\r\n# Temporary: %2d"),
                            pStrand->GetName().c_str(),Ns,harp_type.c_str(),Nh,pTempStrand->GetName().c_str(),Nt);
         }
         else
         {
            strMsg3.Format(_T("\r\n\r\nStrand: %s\r\n# Straight: %2d (%2d Debonded)\r\n# %s: %2d\r\n\r\n%s\r\n# Temporary: %2d"),
                            pStrand->GetName().c_str(),Ns,Nsd,harp_type.c_str(),Nh,pTempStrand->GetName().c_str(),Nt);
         }
      }
      else
      {
         if ( Nsd == 0 )
         {
            strMsg3.Format(_T("\r\n\r\nStrand: %s\r\n# Straight: %2d\r\n# %s: %2d"),
                            pStrand->GetName().c_str(),Ns,harp_type.c_str(),Nh);
         }
         else
         {
            strMsg3.Format(_T("\r\n\r\nStrand: %s\r\n# Straight: %2d (%2d Debonded)\r\n# %s: %2d"),
                            pStrand->GetName().c_str(),Ns,Nsd,harp_type.c_str(),Nh);
         }
      }

#pragma Reminder("UPDATE: slab offset tooltip")
      //// Slab Offset
      //Float64 startOffset, endOffset;
      //startOffset = pBridge->GetSlabOffset(segmentKey,pgsTypes::metStart);
      //endOffset   = pBridge->GetSlabOffset(segmentKey,pgsTypes::metEnd);
      //CString strMsg4;
      //strMsg4.Format(_T("\r\n\r\nSlab Offset\r\nStart: %s\r\nEnd: %s"),
      //   FormatDimension(startOffset,pDisplayUnits->GetComponentDimUnit()),
      //   FormatDimension(endOffset,pDisplayUnits->GetComponentDimUnit()));

      CString strMsg = strMsg1 + strMsg2 + strMsg3;// + strMsg4;

#if defined _DEBUG
            CString strSegID;
            strSegID.Format(_T("\r\n\r\nGroup %d Girder %s Segment %d"),
               LABEL_GROUP(segmentKey.groupIndex),
               LABEL_GIRDER(segmentKey.girderIndex),
               LABEL_SEGMENT(segmentKey.segmentIndex));

            strMsg += strSegID;

            CString strGirderID;
            strGirderID.Format(_T("\r\n\r\nGirder ID: %d"),pSegment->GetGirder()->GetID());

            strMsg += strGirderID;
#endif // _DEBUG

      pDO->SetMaxTipWidth(TOOLTIP_WIDTH);
      pDO->SetTipDisplayTime(TOOLTIP_DURATION);
      pDO->SetToolTipText(strMsg);
   }
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
   BuildDeckDisplayObjects();
   BuildOverlayDisplayObjects();
   BuildTrafficBarrierDisplayObjects();
   BuildDimensionLineDisplayObjects();

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
   const unitStationFormat& station_format = pdisp_units->GetStationFormat();
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

   GET_IFACE2(pBroker,IRoadway,pAlignment);
   GET_IFACE2(pBroker,IIntervals,pIntervals);
   GET_IFACE2(pBroker,IShapes,pShapes);
   GET_IFACE2(pBroker,IPointOfInterest,pPoi);
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

   CComPtr<IDirection> dirAlignmentNormal;
   pAlignment->GetBearingNormal(cut_station,&dirAlignmentNormal); // section cut is normal to the alignment

   m_GirderIDs.clear();
   m_NextGirderID = 0;

   std::vector<pgsPointOfInterest> vPoi = pPoi->GetPointOfInterests(cut_station,dirAlignmentNormal);
   if ( vPoi.size() < nGirders )
   {
      // the cut line doesn't intersect some of the girders. this can happen when the section cut is at or near
      // a pier, especially at the ends of the bridge.
      // find the missing girders and use the POI at the start face so there is something to draw

      // all possible girders
      std::vector<GirderIndexType> vAllGirders;
      vAllGirders.resize(nGirders);
      std::generate(vAllGirders.begin(),vAllGirders.end(),IncrementValue<GirderIndexType>(0)); // fill container with sequential values begining with 0

      // all the girders we found
      std::vector<GirderIndexType> vFoundGirders;
      BOOST_FOREACH(pgsPointOfInterest& poi,vPoi)
      {
         vFoundGirders.push_back(poi.GetSegmentKey().girderIndex);
      }
      std::sort(vFoundGirders.begin(),vFoundGirders.end());

      // missing girders = all girders - found girders
      std::vector<GirderIndexType> vMissingGirders(vAllGirders.size());
      std::vector<GirderIndexType>::iterator end = std::set_difference(vAllGirders.begin(),vAllGirders.end(),vFoundGirders.begin(),vFoundGirders.end(),vMissingGirders.begin());
      vMissingGirders.resize(end-vMissingGirders.begin());

      // for each missing girder, get a POI to use
      BOOST_FOREACH(GirderIndexType gdrIdx,vMissingGirders)
      {
         CGirderKey girderKey(grpIdx,gdrIdx);
         pgsPointOfInterest poi;
         VERIFY( pPoi->GetPointOfInterest(girderKey,cut_station,dirAlignmentNormal,true,&poi) );
         vPoi.push_back(poi);
      }
   }

   BOOST_FOREACH(pgsPointOfInterest& poi,vPoi)
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

      // Display object for the girder cross section
      CComPtr<iPointDisplayObject> dispObj;
      dispObj.CoCreateInstance(CLSID_PointDisplayObject);

      // get the girder shape before it is made composite (we don't want the deck with the shape)
      IntervalIndexType intervalIdx = pIntervals->GetErectSegmentInterval(thisSegmentKey);

      CComPtr<IShape> shape;
      pShapes->GetSegmentShape(intervalIdx,poi,true,pgsTypes::scBridge,&shape);

      CComQIPtr<IXYPosition> position(shape);
      CComPtr<IPoint2d> topCenter;
      position->get_LocatorPoint(lpTopCenter,&topCenter);
      dispObj->SetPosition(topCenter,FALSE,FALSE);

      CComPtr<iShapeDrawStrategy> strategy;
      strategy.CoCreateInstance(CLSID_ShapeDrawStrategy);
      strategy->SetShape(shape);
      strategy->SetSolidLineColor(segment_border_color);
      strategy->SetSolidFillColor(segment_fill_color);
      strategy->SetVoidLineColor(VOID_BORDER_COLOR);
      strategy->SetVoidFillColor(GetSysColor(COLOR_WINDOW));
      strategy->DoFill(true);

      dispObj->SetDrawingStrategy(strategy);

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

void CBridgeSectionView::BuildDeckDisplayObjects()
{
   CPGSDocBase* pDoc = (CPGSDocBase*)GetDocument();
   CComPtr<IBroker> pBroker;
   pDoc->GetBroker(&pBroker);

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CDeckDescription2* pDeck = pBridgeDesc->GetDeckDescription();

   pgsTypes::SupportedDeckType deckType = pDeck->DeckType;
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
   pShapes->GetSlabShape(m_pFrame->GetCurrentCutLocation(),NULL,&shape);

   CComPtr<iShapeDrawStrategy> strategy;
   strategy.CoCreateInstance(CLSID_ShapeDrawStrategy);

   strategy->SetShape(shape);
   strategy->SetSolidLineColor(DECK_BORDER_COLOR);
   strategy->SetSolidFillColor(DECK_FILL_COLOR);
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
   if ( deckType != pgsTypes::sdtNone )
   {
      pgsTypes::SlabOffsetType slabOffsetType = pBridgeDesc->GetSlabOffsetType();
      if ( slabOffsetType == pgsTypes::sotBridge )
      {
         strMsg2.Format(_T("\r\n\r\nDeck: %s\r\nSlab Thickness: %s\r\nSlab Offset: %s\r\n%s\r\nf'c: %s"),
                        m_pFrame->GetDeckTypeName(deckType),
                        FormatDimension(pDeck->GrossDepth,pDisplayUnits->GetComponentDimUnit()),
                        FormatDimension(pBridgeDesc->GetSlabOffset(),pDisplayUnits->GetComponentDimUnit()),
                        lrfdConcreteUtil::GetTypeName((matConcrete::Type)pDeck->Concrete.Type,true).c_str(),
                        FormatDimension(pDeck->Concrete.Fc,pDisplayUnits->GetStressUnit())
                        );
      }
      else
      {
         strMsg2.Format(_T("\r\n\r\nDeck: %s\r\nSlab Thickness: %s\r\nSlab Offset: per girder\r\n%s\r\nf'c: %s"),
                        m_pFrame->GetDeckTypeName(deckType),
                        FormatDimension(pDeck->GrossDepth,pDisplayUnits->GetComponentDimUnit()),
                        lrfdConcreteUtil::GetTypeName((matConcrete::Type)pDeck->Concrete.Type,true).c_str(),
                        FormatDimension(pDeck->Concrete.Fc,pDisplayUnits->GetStressUnit())
                        );
      }
   }

   CString strMsg3;
   Float64 overlay_weight = pBridge->GetOverlayWeight();
   if ( pBridge->HasOverlay() )
   {
      strMsg3.Format(_T("\r\n\r\n%s: %s"),
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

   Float64 overlay_weight = pBridge->GetOverlayWeight();
   Float64 depth = pBridge->GetOverlayDepth();

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
   pRoadway->GetRoadwaySurface(station,NULL,&surfacePoints);

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

   if ( pBridge->IsFutureOverlay() )
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

   // points along the bottom of the overlay (along the top of deck)
   for (IndexType pntIdx = 0; pntIdx < nPoints; pntIdx++ )
   {
      CComPtr<IPoint2d> pnt;
      surfacePoints->get_Item(pntIdx,&pnt);
      poly_shape->AddPointEx(pnt);
   }

   // now work backwards, offset the points upwards by the depth of the overlay
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

   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   CComPtr<iDisplayList> display_list;
   dispMgr->FindDisplayList(TRAFFIC_BARRIER_DISPLAY_LIST,&display_list);

   // left hand barrier
   CComPtr<iPointDisplayObject> left_dispObj;
   left_dispObj.CoCreateInstance(CLSID_PointDisplayObject);

   Float64 cut_station = m_pFrame->GetCurrentCutLocation();
   Float64 pier_1_station = pBridge->GetPierStation(0);
   Float64 cut_dist_from_start = cut_station - pier_1_station;
   Float64 left_curb_offset  = pBridge->GetLeftCurbOffset(cut_dist_from_start);
   Float64 right_curb_offset = pBridge->GetRightCurbOffset(cut_dist_from_start);

   CComPtr<IDirection> normal;
   pAlignment->GetBearingNormal(cut_station,&normal);

   CComPtr<IShape> left_shape;
   pShapes->GetLeftTrafficBarrierShape(cut_station,NULL,&left_shape);

   CComPtr<iShapeDrawStrategy> strategy;
   if ( left_shape )
   {
      //// rotate the shape to match the crown slope
      //Float64 slope = pAlignment->GetSlope(cut_station,left_curb_offset);
      //Float64 angle = atan(slope);

      //// Rotate shape around edge of deck - this is where barrier origin is placed
      //Float64 left_offset = pBridge->GetLeftSlabEdgeOffset(cut_dist_from_start);
      //Float64 left_elev   = pAlignment->GetElevation(cut_station,left_offset);

      //CComPtr<IPoint2d> de_point;
      //de_point.CoCreateInstance(CLSID_Point2d);
      //de_point->Move(left_offset,left_elev);

      //CComQIPtr<IXYPosition> position(left_shape);
      //position->RotateEx(de_point,angle);

      strategy.CoCreateInstance(CLSID_ShapeDrawStrategy);

      strategy->SetShape(left_shape);
      strategy->SetSolidLineColor(BARRIER_BORDER_COLOR);
      strategy->SetSolidFillColor(BARRIER_FILL_COLOR);
      strategy->SetVoidLineColor(VOID_BORDER_COLOR);
      strategy->SetVoidFillColor(GetSysColor(COLOR_WINDOW));
      strategy->DoFill(true);
      strategy->HasBoundingShape(false);

      left_dispObj->SetDrawingStrategy(strategy);

      display_list->AddDisplayObject(left_dispObj);
   }


   // right hand barrier
   CComPtr<iPointDisplayObject> right_dispObj;
   right_dispObj.CoCreateInstance(CLSID_PointDisplayObject);

   CComPtr<IShape> right_shape;
   pShapes->GetRightTrafficBarrierShape(cut_station,NULL,&right_shape);

   if ( right_shape )
   {
      //// rotate the shape to match the crown slope
      //Float64 slope = pAlignment->GetSlope(cut_station,right_curb_offset);
      //Float64 angle = atan(slope);
      //CComQIPtr<IXYPosition> position(right_shape);
      //CComPtr<IPoint2d> hook_point;
      //position->get_LocatorPoint(lpHookPoint,&hook_point);
      //position->RotateEx(hook_point,angle);

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

      display_list->AddDisplayObject(right_dispObj);
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
   const unitmgtLengthData& rlen = pdisp_units->GetXSectionDimUnit();

   //
   // Create Girder Spacing Dimension Line
   //
   CComPtr<iSocket> firstSocket, lastSocket;
   long witness_length;

   // find the bottom of the "lowest" girder so all the dimension lines can be at
   // the same elevation
   Float64 yLowest = DBL_MAX;
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

      // We know that these display objects use ShapeDrawStrategy objects, and they hold the girder shape
      // Get the strategies and then the shapes
      CComPtr<iDrawPointStrategy> dsGirder;
      pdoGirder->GetDrawingStrategy(&dsGirder);

      CComQIPtr<iShapeDrawStrategy> strategy(dsGirder);

      CComPtr<IShape> shape;
      strategy->GetShape(&shape);

      // Get bottom center coordinates of teh exterior girders
      CComQIPtr<IXYPosition> position(shape);

      CComPtr<IPoint2d> p;
      position->get_LocatorPoint(lpBottomCenter,&p);

      Float64 y;
      p->get_Y(&y);
      yLowest = Min(y,yLowest);
   }

   // make a dimension line for each spacing group
   if ( nGirders == 1 )
   {
      CComPtr<iDisplayObject> dispObj;
      girder_list->GetDisplayObject(0,&dispObj);

      CComQIPtr<iPointDisplayObject> pntDO(dispObj);

      // We know that these display objects use ShapeDrawStrategy objects, and they hold the girder shape
      // Get the strategies and then the shapes
      CComPtr<iDrawPointStrategy> ds;
      pntDO->GetDrawingStrategy(&ds);

      CComQIPtr<iShapeDrawStrategy> strategy(ds);

      CComPtr<IShape> shape;
      strategy->GetShape(&shape);

      // Get bottom center coordinates of teh exterior girders
      CComQIPtr<IXYPosition> position(shape);

      CComPtr<IPoint2d> p1;
      position->get_LocatorPoint(lpBottomCenter,&p1);

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

            CComQIPtr<iShapeDrawStrategy> strategy1(ds1);
            CComQIPtr<iShapeDrawStrategy> strategy2(ds2);

            CComPtr<IShape> shape1, shape2;
            strategy1->GetShape(&shape1);
            strategy2->GetShape(&shape2);

            // Get bottom center coordinates of teh exterior girders
            CComQIPtr<IXYPosition> position1(shape1);
            CComQIPtr<IXYPosition> position2(shape2);

            CComPtr<IPoint2d> p1, p2;
            position1->get_LocatorPoint(lpBottomCenter,&p1);
            position2->get_LocatorPoint(lpBottomCenter,&p2);

            // adjust points so both are at the same, and lowest, elevation
            p1->put_Y(yLowest);
            p2->put_Y(yLowest);

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
            doDimLine->SetWitnessLength(-witness_length);

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
         left_overhang_point->put_Y(yLowest);

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
            leftOverhangDimLine->SetWitnessLength(-witness_length);

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
         right_overhang_point->put_Y(yLowest);

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
            rightOverhangDimLine->SetWitnessLength(-witness_length);

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

   //
   // Curb-to-Curb dimension line
   //
   CComPtr<iDisplayList> tb_list;
   dispMgr->FindDisplayList(TRAFFIC_BARRIER_DISPLAY_LIST,&tb_list);

   // get the slab display object
   CComPtr<iDisplayObject> doLeftTB, doRightTB;
   tb_list->GetDisplayObject(0,&doLeftTB);
   tb_list->GetDisplayObject(1,&doRightTB);

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
      curbDimLine->SetWitnessLength((long)(1.75*witness_length));

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
            tbDimLine->SetWitnessLength((long)(1.75*witness_length));

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
            tbDimLine->SetWitnessLength((long)(1.75*witness_length));

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

            display_list->AddDisplayObject(icbDimLine);
         }
      }
   }
}

void CBridgeSectionView::UpdateDrawingScale()
{
   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   CComPtr<iDisplayList> display_list;
   dispMgr->FindDisplayList(TITLE_DISPLAY_LIST,&display_list);

   if ( display_list == NULL )
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
         if ( prev_pier_station <= cut_station && cut_station <= next_pier_station )
         {
            return pGroup->GetIndex();
         }
      }
      else
      {
         // Exclue end station for all other groups
         // Section cuts look ahead on station so if we are cutting at a group
         // boundary, we want the next group
         if ( prev_pier_station <= cut_station && cut_station < next_pier_station )
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
