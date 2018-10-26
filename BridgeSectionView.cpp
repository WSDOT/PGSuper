///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2010  Washington State Department of Transportation
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
#include "resource.h"
#include "PGSuperAppPlugin\PGSuperApp.h"
#include "PGSuperDoc.h"
#include <IFace\DrawBridgeSettings.h>
#include "BridgeSectionView.h"
#include "SlabDisplayObjectEvents.h"
#include "BridgePlanView.h"

#include "PGSuperUnits.h"
#include "PGSuperColors.h"

#include <PgsExt\PierData.h>
#include <PgsExt\BridgeDescription.h>

#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <IFace\Alignment.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\EditByUI.h>

#include <GraphicsLib\GraphTool.h>

#include "GirderDisplayObjectEvents.h"

#include <MfcTools\Text.h>
#include <WBFLDManip.h>

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


/////////////////////////////////////////////////////////////////////////////
// CBridgeSectionView

IMPLEMENT_DYNCREATE(CBridgeSectionView, CDisplayView)

CBridgeSectionView::CBridgeSectionView()
{
}

CBridgeSectionView::~CBridgeSectionView()
{
}


BEGIN_MESSAGE_MAP(CBridgeSectionView, CDisplayView)
	//{{AFX_MSG_MAP(CBridgeSectionView)
	ON_COMMAND(ID_EDIT_BRIDGE, OnEditBridge)
	ON_COMMAND(ID_EDIT_DECK, OnEditDeck)
	ON_COMMAND(ID_VIEWSETTINGS, OnViewSettings)
	ON_WM_SIZE()
	ON_WM_CREATE()
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
	ON_WM_KEYDOWN()
	ON_WM_MOUSEWHEEL()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CBridgeSectionView::DoPrint(CDC* pDC, CPrintInfo* pInfo,CRect rcDraw)
{
   OnBeginPrinting(pDC, pInfo, rcDraw);
   OnPrepareDC(pDC);
   UpdateDrawingScale();
   OnDraw(pDC);
   OnEndPrinting(pDC, pInfo);
}

bool CBridgeSectionView::GetSelectedGirder(SpanIndexType* pSpanIdx,GirderIndexType* pGirderIdx)
{
   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   DisplayObjectContainer displayObjects;
   dispMgr->GetSelectedObjects(&displayObjects);

   ATLASSERT(displayObjects.size() == 0 || displayObjects.size() == 1 );

   if ( displayObjects.size() == 0 )
      return false;

   CComPtr<iDisplayObject> pDO = displayObjects.front().m_T;

   // girder IDs are positive values
   long ID = pDO->GetID();
   if ( ID < 0 )
      return false;

   // do a reverse search in the map (look for values to get the key)
   std::map<SpanGirderHashType,long>::iterator iter;
   for ( iter = m_GirderIDs.begin(); iter != m_GirderIDs.end(); iter++ )
   {
      std::pair<SpanGirderHashType,long> map = *iter;
      if ( map.second == ID )
      {
         // ID found,  get the key, unhash it
         UnhashSpanGirder(map.first,pSpanIdx,pGirderIdx);
         return true;
      }
   }

   return false;
}

bool CBridgeSectionView::IsDeckSelected()
{
   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   DisplayObjectContainer displayObjects;
   dispMgr->GetSelectedObjects(&displayObjects);

   ATLASSERT(displayObjects.size() == 0 || displayObjects.size() == 1 );

   if ( displayObjects.size() == 0 )
      return false;

   CComPtr<iDisplayObject> pDO = displayObjects.front().m_T;

   long ID = pDO->GetID();
   if ( ID == DECK_ID )
      return true;

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

void CBridgeSectionView::SelectGirder(SpanIndexType spanIdx,GirderIndexType gdrIdx,bool bSelect)
{
   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   SpanGirderHashType hash = HashSpanGirder(spanIdx,gdrIdx);
   std::map<SpanGirderHashType,long>::iterator found = m_GirderIDs.find(hash);
   if ( found == m_GirderIDs.end() )
   {
      dispMgr->ClearSelectedObjects();
      return;
   }

   long ID = (*found).second;

   CComPtr<iDisplayObject> pDO;
   dispMgr->FindDisplayObject(ID,GIRDER_DISPLAY_LIST,atByID,&pDO);

   if ( pDO )
      dispMgr->SelectObject(pDO,bSelect);
   else
      dispMgr->ClearSelectedObjects();
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
   // when a span is selected in another view
   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);
   dispMgr->ClearSelectedObjects();
}

/////////////////////////////////////////////////////////////////////////////
// CBridgeSectionView drawing
void CBridgeSectionView::OnInitialUpdate()
{
   EnableToolTips();

   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   dispMgr->EnableLBtnSelect(TRUE);
   dispMgr->EnableRBtnSelect(TRUE);
   dispMgr->SetSelectionLineColor(SELECTED_OBJECT_LINE_COLOR);
   dispMgr->SetSelectionFillColor(SELECTED_OBJECT_FILL_COLOR);

   CDisplayView::SetMappingMode(DManip::Isotropic);

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

   CDisplayView::OnInitialUpdate();

   UpdateDisplayObjects();
   UpdateDrawingScale();
}

/////////////////////////////////////////////////////////////////////////////
// CBridgeSectionView diagnostics

#ifdef _DEBUG
void CBridgeSectionView::AssertValid() const
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
	CDisplayView::AssertValid();
}

void CBridgeSectionView::Dump(CDumpContext& dc) const
{
	CDisplayView::Dump(dc);
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

      double x,y;
      point->get_X(&x);
      point->get_Y(&y);

      CString str;
      str.Format("%f, %f\r\n",::ConvertFromSysUnits(x,unitMeasure::Feet),::ConvertFromSysUnits(y,unitMeasure::Feet));
      strDump += str;
   }

   return strDump;
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CBridgeSectionView message handlers

void CBridgeSectionView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
   if ( lHint == HINT_BRIDGECHANGED )
   {
      CComPtr<IBroker> pBroker;
      EAFGetBroker(&pBroker);
      GET_IFACE2(pBroker,IBridge,pBridge);
      double first_station = pBridge->GetPierStation(0);
      double last_station  = pBridge->GetPierStation(pBridge->GetPierCount()-1);
      double cut_station = m_pFrame->GetCurrentCutLocation();

      if ( !InRange(first_station,cut_station,last_station) )
         m_pFrame->InvalidateCutLocation();
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
         this->SelectGirder( pSelection->SpanIdx,pSelection->GirderIdx,true);
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

      default:
         ATLASSERT(FALSE); // is there a new type of object to be selected?
         this->ClearSelection();
         break;
      }
   }
}

void CBridgeSectionView::HandleLButtonDblClk(UINT nFlags, CPoint point) 
{
   ((CPGSuperDoc*)GetDocument())->EditBridgeDescription(EBD_GENERAL);
}

void CBridgeSectionView::HandleLButtonDown(UINT nFlags, CPoint logPoint)
{
   CBridgeModelViewChildFrame* pFrame = GetFrame();
   pFrame->ClearSelection();
   Invalidate();
}

void CBridgeSectionView::HandleContextMenu(CWnd* pWnd,CPoint logPoint)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CPGSuperDoc* pDoc = (CPGSuperDoc*)GetDocument();
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

   std::map<Uint32,IBridgeSectionViewEventCallback*> callbacks = pDoc->GetBridgeSectionViewCallbacks();
   std::map<Uint32,IBridgeSectionViewEventCallback*>::iterator iter;
   for ( iter = callbacks.begin(); iter != callbacks.end(); iter++ )
   {
      IBridgeSectionViewEventCallback* callback = iter->second;
      callback->OnBackgroundContextMenu(pMenu);
   }


   pMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, logPoint.x, logPoint.y, this);
   delete pMenu;
}

void CBridgeSectionView::OnEditBridge() 
{
   ((CPGSuperDoc*)GetDocument())->EditBridgeDescription(EBD_GENERAL);
}

void CBridgeSectionView::OnEditDeck() 
{
   ((CPGSuperDoc*)GetDocument())->EditBridgeDescription(EBD_DECK);
}

void CBridgeSectionView::OnViewSettings() 
{
   ((CPGSuperDoc*)GetDocument())->EditBridgeViewSettings(VS_BRIDGE_SECTION);
}

void CBridgeSectionView::OnSize(UINT nType, int cx, int cy) 
{
	CDisplayView::OnSize(nType, cx, cy);

   CRect rect;
   GetClientRect(&rect);
   rect.DeflateRect(15,15,15,15);

   CSize size = rect.Size();
   size.cx = max(0,size.cx);
   size.cy = max(0,size.cy);

   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   SetLogicalViewRect(MM_TEXT,rect);

   SetScrollSizes(MM_TEXT,size,CScrollView::sizeDefault,CScrollView::sizeDefault);

   UpdateDrawingScale();
}

void CBridgeSectionView::UpdateGirderTooltips()
{
   CPGSuperDoc* pDoc = (CPGSuperDoc*)GetDocument();
   CComPtr<IBroker> pBroker;
   pDoc->GetBroker(&pBroker);

   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeom);
   GET_IFACE2(pBroker,IBridgeMaterialEx,pBridgeMaterial);

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   CComPtr<iDisplayList> girder_list;
   dispMgr->FindDisplayList(GIRDER_DISPLAY_LIST,&girder_list);
   SpanIndexType spanIdx = GetSpanIndex();

   GirderIndexType nGirders = pBridgeDesc->GetSpan(spanIdx)->GetGirderCount();
   for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
   {
      SpanGirderHashType hash = HashSpanGirder(spanIdx,gdrIdx);
      std::map<SpanGirderHashType,long>::iterator found = m_GirderIDs.find(hash);
      if ( found == m_GirderIDs.end() )
         continue;

      long ID = (*found).second;

      CComPtr<iDisplayObject> pDO;
      girder_list->FindDisplayObject(ID,&pDO);

      if ( !pDO )
         continue;

      CString strMsg1;
      strMsg1.Format("Double click to edit Span %d Girder %s.\r\nRight click for more options.",LABEL_SPAN(spanIdx),LABEL_GIRDER(gdrIdx));

      double fc, fci;
      fc = pBridgeMaterial->GetFcGdr(spanIdx,gdrIdx);
      fci = pBridgeMaterial->GetFciGdr(spanIdx,gdrIdx);

      CString strMsg2;
      strMsg2.Format("\r\n\r\nGirder: %s\r\n%s\r\nf'ci: %s\r\nf'c: %s",
                     pBridgeDesc->GetSpan(spanIdx)->GetGirderTypes()->GetGirderName(gdrIdx),
                     matConcrete::GetTypeName((matConcrete::Type)pBridgeMaterial->GetGdrConcreteType(spanIdx,gdrIdx),true).c_str(),
                     FormatDimension(fci,pDisplayUnits->GetStressUnit()),
                     FormatDimension(fc, pDisplayUnits->GetStressUnit())
                    );

      const matPsStrand* pStrand = pBridgeMaterial->GetStrand(spanIdx,gdrIdx,pgsTypes::Permanent);
      const matPsStrand* pTempStrand = pBridgeMaterial->GetStrand(spanIdx,gdrIdx,pgsTypes::Temporary);

      StrandIndexType Ns, Nh, Nt, Nsd;
      Ns = pStrandGeom->GetNumStrands(spanIdx,gdrIdx,pgsTypes::Straight);
      Nh = pStrandGeom->GetNumStrands(spanIdx,gdrIdx,pgsTypes::Harped);
      Nt = pStrandGeom->GetNumStrands(spanIdx,gdrIdx,pgsTypes::Temporary);
      Nsd= pStrandGeom->GetNumDebondedStrands(spanIdx,gdrIdx,pgsTypes::Straight);

      CString strMsg3;
      if ( pStrandGeom->GetMaxStrands(spanIdx,gdrIdx,pgsTypes::Temporary) != 0 )
      {
         if ( Nsd == 0 )
         {
            strMsg3.Format("\r\n\r\nStrand: %s\r\n# Straight: %2d\r\n# Harped: %2d\r\n\r\n%s\r\n# Temporary: %2d",
                            pStrand->GetName().c_str(),Ns,Nh,pTempStrand->GetName().c_str(),Nt);
         }
         else
         {
            strMsg3.Format("\r\n\r\nStrand: %s\r\n# Straight: %2d (%2d Debonded)\r\n# Harped: %2d\r\n\r\n%s\r\n# Temporary: %2d",
                            pStrand->GetName().c_str(),Ns,Nsd,Nh,pTempStrand->GetName().c_str(),Nt);
         }
      }
      else
      {
         if ( Nsd == 0 )
         {
            strMsg3.Format("\r\n\r\nStrand: %s\r\n# Straight: %2d\r\n# Harped: %2d",
                            pStrand->GetName().c_str(),Ns,Nh);
         }
         else
         {
            strMsg3.Format("\r\n\r\nStrand: %s\r\n# Straight: %2d (%2d Debonded)\r\n# Harped: %2d",
                            pStrand->GetName().c_str(),Ns,Nsd,Nh);
         }
      }

      // Slab Offset
      Float64 startOffset, endOffset;
      startOffset = pBridge->GetSlabOffset(spanIdx,gdrIdx,pgsTypes::metStart);
      endOffset   = pBridge->GetSlabOffset(spanIdx,gdrIdx,pgsTypes::metEnd);
      CString strMsg4;
      strMsg4.Format("\r\n\r\nSlab Offset\r\nStart: %s\r\nEnd: %s",
         FormatDimension(startOffset,pDisplayUnits->GetComponentDimUnit()),
         FormatDimension(endOffset,pDisplayUnits->GetComponentDimUnit()));

      CString strMsg = strMsg1 + strMsg2 + strMsg3 + strMsg4;

      pDO->SetMaxTipWidth(TOOLTIP_WIDTH);
      pDO->SetTipDisplayTime(TOOLTIP_DURATION);
      pDO->SetToolTipText(strMsg);
   }
}

void CBridgeSectionView::UpdateDisplayObjects()
{
   CWaitCursor wait;

   SpanIndexType spanIdx;
   GirderIndexType gdrIdx;
   bool bSelectedGirder = GetSelectedGirder(&spanIdx,&gdrIdx);
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
      SelectGirder(spanIdx,gdrIdx,TRUE);
   else if ( bDeckSelected )
      SelectDeck(true);
}

void CBridgeSectionView::BuildTitleDisplayObjects()
{
   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   CComPtr<iDisplayList> title_list;
   dispMgr->FindDisplayList(TITLE_DISPLAY_LIST,&title_list);

   CComPtr<iViewTitle> title;
   title.CoCreateInstance(CLSID_ViewTitle);

   CPGSuperDoc* pDoc = (CPGSuperDoc*)GetDocument();
   CComPtr<IBroker> pBroker;
   pDoc->GetBroker(&pBroker);

   GET_IFACE2(pBroker,IEAFDisplayUnits,pdisp_units);
   const unitStationFormat& station_format = pdisp_units->GetStationFormat();
   CString strTitle;
   CString strStation = FormatStation(station_format,m_pFrame->GetCurrentCutLocation());

   strTitle.Format("Section at Station %s - Normal to Alignment",strStation);
   title->SetText(strTitle);
   title_list->AddDisplayObject(title);
}

void CBridgeSectionView::BuildGirderDisplayObjects()
{
   CPGSuperDoc* pDoc = (CPGSuperDoc*)GetDocument();
   CComPtr<IBroker> pBroker;
   pDoc->GetBroker(&pBroker);

   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,ISectProp2,pSectProp);

   UINT settings = pDoc->GetBridgeEditorSettings();;

   SpanIndexType spanIdx = GetSpanIndex();

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   double firstPierStation = pBridgeDesc->GetPier(0)->GetStation();

   SpanIndexType nSpans = pBridgeDesc->GetSpanCount();

   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   CComPtr<iDisplayList> girder_list;
   dispMgr->FindDisplayList(GIRDER_DISPLAY_LIST,&girder_list);
   girder_list->Clear();

   CComPtr<iDisplayList> girder_label_list;
   dispMgr->FindDisplayList(GIRDER_LABEL_DISPLAY_LIST,&girder_label_list);
   girder_label_list->Clear();

   double cut_station = m_pFrame->GetCurrentCutLocation();

   double distFromStartOfBridge = cut_station - firstPierStation;

   m_GirderIDs.clear();
   m_NextGirderID = 0;

   GirderIndexType nGirders = pBridgeDesc->GetSpan(spanIdx)->GetGirderCount();
   for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
   {
      double offset = pBridge->GetGirderOffset(spanIdx,gdrIdx,cut_station);

      SpanIndexType spanIndex;
      double distFromStartOfSpan;
      pBridge->GetDistFromStartOfSpan(gdrIdx,distFromStartOfBridge,&spanIndex,&distFromStartOfSpan);

      if ( spanIndex == INVALID_INDEX )
         continue; // girder is off bridge at this cut location

      GirderIndexType nGirdersThisSpan = pBridgeDesc->GetSpan(spanIndex)->GetGirderCount();
      GirderIndexType gdrIndex = (nGirdersThisSpan <= gdrIdx ? nGirdersThisSpan-1 : gdrIdx);

      double start_connection_length = pBridge->GetGirderStartConnectionLength(spanIndex,gdrIndex);
      double start_bearing_offset    = pBridge->GetGirderStartBearingOffset(spanIndex,gdrIndex);
      double end_offset = start_bearing_offset - start_connection_length;
      double girder_length = pBridge->GetGirderLength(spanIndex,gdrIndex);

      double distFromStartOfGirder = distFromStartOfSpan - end_offset;
      distFromStartOfGirder = ::ForceIntoRange(0.,distFromStartOfGirder,girder_length);

      pgsPointOfInterest poi;
      COLORREF girder_fill_color;
      COLORREF girder_border_color;
      if (spanIndex == spanIdx )
      {
         poi.SetSpan(spanIdx);
         poi.SetGirder(gdrIdx);
         poi.SetDistFromStart(distFromStartOfGirder);

         girder_fill_color = GIRDER_FILL_COLOR;
         girder_border_color = GIRDER_BORDER_COLOR;
      }
      else
      {
         // girder cut is on bridge, but not in the same span
         poi.SetSpan(spanIndex);
         poi.SetGirder(gdrIndex);
         poi.SetDistFromStart(distFromStartOfGirder);

         girder_fill_color = GIRDER_FILL_COLOR_ADJACENT;
         girder_border_color = GIRDER_BORDER_COLOR_ADJACENT;
      }

      // Display object for the girder cross section
      CComPtr<iPointDisplayObject> dispObj;
      dispObj.CoCreateInstance(CLSID_PointDisplayObject);

      CComPtr<IShape> shape;
      pSectProp->GetGirderShape(poi,true,&shape);

      CComQIPtr<IXYPosition> position(shape);
      CComPtr<IPoint2d> topCenter;
      position->get_LocatorPoint(lpTopCenter,&topCenter);
      dispObj->SetPosition(topCenter,FALSE,FALSE);

      CComPtr<iShapeDrawStrategy> strategy;
      strategy.CoCreateInstance(CLSID_ShapeDrawStrategy);
      strategy->SetShape(shape);
      strategy->SetSolidLineColor(girder_border_color);
      strategy->SetSolidFillColor(girder_fill_color);
      strategy->SetVoidLineColor(VOID_BORDER_COLOR);
      strategy->SetVoidFillColor(GetSysColor(COLOR_WINDOW));
      strategy->DoFill(true);

      dispObj->SetDrawingStrategy(strategy);

      dispObj->SetSelectionType(stAll);

      SpanGirderHashType hash = HashSpanGirder(spanIndex,gdrIndex);
      long ID = m_NextGirderID++;
      m_GirderIDs.insert( std::make_pair(hash,ID) );

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
         strLabel.Format("%s",LABEL_GIRDER(gdrIdx));
         doText->SetText(strLabel);
         doText->SetBkMode(TRANSPARENT);
         doText->SetTextAlign(TA_CENTER | TA_TOP);
         girder_label_list->AddDisplayObject(doText);

// Text block for debugging
// labels the girder offset and top of girder elevation
//         CComPtr<iTextBlock> doText2;
//         doText2.CoCreateInstance(CLSID_TextBlock);
//         doText2->SetPosition(topCenter);
//
//         GET_IFACE2(pBroker,IEAFDisplayUnits,pdisp_units);
//         double x,y;
//         topCenter->get_X(&x);
//         topCenter->get_Y(&y);
//         CString strCoordinates;
//         strCoordinates.Format("Offset %s\nElev %s",FormatDimension(x,pdisp_units->GetXSectionDimUnit()),
//                                                    FormatDimension(y,pdisp_units->GetXSectionDimUnit()) );
//
//         doText2->SetText(strCoordinates);
//         doText2->SetTextAlign(TA_LEFT | TA_TOP);
//         doText2->SetBkMode(TRANSPARENT);
//         girder_label_list->AddDisplayObject(doText2);
      }
   

      // Register an event sink with the girder display object so that we can handle double clicks
      // on the girder differently then a general double click
      CBridgeSectionViewGirderDisplayObjectEvents* pEvents = new CBridgeSectionViewGirderDisplayObjectEvents(spanIndex,gdrIndex,nSpans,nGirdersThisSpan,m_pFrame); // ref count = 1
      IUnknown* unk = pEvents->GetInterface(&IID_iDisplayObjectEvents); // ref count = 1
      CComQIPtr<iDisplayObjectEvents,&IID_iDisplayObjectEvents> events(unk); // ref count = 2
      dispObj->RegisterEventSink(events); // ref count = 3
      unk->Release(); // removes the AddRef from new above // ref count = 2
      events.Release(); // ref count = 1 ... i.e dispObj holds the only reference
   }
}

void CBridgeSectionView::BuildDeckDisplayObjects()
{
   CPGSuperDoc* pDoc = (CPGSuperDoc*)GetDocument();
   CComPtr<IBroker> pBroker;
   pDoc->GetBroker(&pBroker);

   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,ISectProp2,pSectProp);
   GET_IFACE2(pBroker,IBridgeMaterial,pBridgeMaterial);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CDeckDescription* pDeck = pBridgeDesc->GetDeckDescription();

   SpanIndexType spanIdx = GetSpanIndex();

   pgsTypes::SupportedDeckType deckType = pDeck->DeckType;
   if ( deckType == pgsTypes::sdtNone )
      return; // if there is no deck, don't create a display object

   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   CComPtr<iDisplayList> display_list;
   dispMgr->FindDisplayList(SLAB_DISPLAY_LIST,&display_list);

   CComPtr<iPointDisplayObject> dispObj;
   dispObj.CoCreateInstance(CLSID_PointDisplayObject);

   CComPtr<IShape> shape;
   pSectProp->GetSlabShape(m_pFrame->GetCurrentCutLocation(),&shape);

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

   
   CString strMsg1("Double click to edit slab.\r\nRight click for more options.");

   CString strMsg2;
   if ( deckType != pgsTypes::sdtNone )
   {
      pgsTypes::SlabOffsetType slabOffsetType = pBridgeDesc->GetSlabOffsetType();
      if ( slabOffsetType == pgsTypes::sotBridge )
      {
         strMsg2.Format("\r\n\r\nDeck: %s\r\nSlab Thickness: %s\r\nSlab Offset: %s\r\n%s\r\nf'c: %s",
                        m_pFrame->GetDeckTypeName(deckType),
                        FormatDimension(pDeck->GrossDepth,pDisplayUnits->GetComponentDimUnit()),
                        FormatDimension(pBridgeDesc->GetSlabOffset(),pDisplayUnits->GetComponentDimUnit()),
                        matConcrete::GetTypeName((matConcrete::Type)pDeck->SlabConcreteType,true).c_str(),
                        FormatDimension(pBridgeMaterial->GetFcSlab(),pDisplayUnits->GetStressUnit())
                        );
      }
      else
      {
         strMsg2.Format("\r\n\r\nDeck: %s\r\nSlab Thickness: %s\r\nSlab Offset: per girder\r\n%s\r\nf'c: %s",
                        m_pFrame->GetDeckTypeName(deckType),
                        FormatDimension(pDeck->GrossDepth,pDisplayUnits->GetComponentDimUnit()),
                        matConcrete::GetTypeName((matConcrete::Type)pDeck->SlabConcreteType,true).c_str(),
                        FormatDimension(pBridgeMaterial->GetFcSlab(),pDisplayUnits->GetStressUnit())
                        );
      }
   }

   CString strMsg3;
   double overlay_weight = pBridge->GetOverlayWeight();
   if ( pBridge->HasOverlay() )
   {
      strMsg3.Format("\r\n\r\n%s: %s",
         pBridge->IsFutureOverlay() ? "Future Overlay" : "Overlay",
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
   CPGSuperDoc* pDoc = (CPGSuperDoc*)GetDocument();
   CComPtr<IBroker> pBroker;
   pDoc->GetBroker(&pBroker);

   GET_IFACE2(pBroker,IBridge,pBridge);

   // no overlay = nothing to draw
   if ( !pBridge->HasOverlay() )
      return;

   double overlay_weight = pBridge->GetOverlayWeight();
   double depth = pBridge->GetOverlayDepth();

   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   CComPtr<iDisplayList> display_list;
   dispMgr->FindDisplayList(OVERLAY_DISPLAY_LIST,&display_list);

   // point a simple point display object in the corner between the top of deck and the traffic barrier
   // at the left hand side of the section. then use a ShapeDrawStrategy to draw the overlay shape

   CComPtr<iPointDisplayObject> dispObj;
   dispObj.CoCreateInstance(CLSID_PointDisplayObject);

   double station = m_pFrame->GetCurrentCutLocation();

   double dist_from_start_of_bridge = pBridge->GetDistanceFromStartOfBridge(station);
   double left_offset, right_offset;
   left_offset  = pBridge->GetLeftCurbOffset(dist_from_start_of_bridge);
   right_offset = pBridge->GetRightCurbOffset(dist_from_start_of_bridge);

   GET_IFACE2(pBroker,IRoadway,pRoadway);
   double left_elev  = pRoadway->GetElevation(station,left_offset);
   double right_elev = pRoadway->GetElevation(station,right_offset);

   GET_IFACE2(pBroker,IRoadwayData,pRoadwayData);
   double crown_point_offset = pRoadway->GetCrownPointOffset(station);
   double crown_point_elev = pRoadway->GetElevation(station,crown_point_offset);

   CComPtr<IPoint2d> pos;
   pos.CoCreateInstance(CLSID_Point2d);
   pos->Move(left_offset,left_elev);
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
   CComPtr<IPoint2d> p1,p2,p3,p4,p5,p6;
   p1.CoCreateInstance(CLSID_Point2d);
   p1->Move(left_offset,left_elev);

   p2.CoCreateInstance(CLSID_Point2d);
   p2->Move(left_offset,left_elev+depth);

   p3.CoCreateInstance(CLSID_Point2d);
   p3->Move(crown_point_offset,crown_point_elev+depth);

   p4.CoCreateInstance(CLSID_Point2d);
   p4->Move(right_offset,right_elev+depth);

   p5.CoCreateInstance(CLSID_Point2d);
   p5->Move(right_offset,right_elev);

   p6.CoCreateInstance(CLSID_Point2d);
   p6->Move(crown_point_offset,crown_point_elev);

   poly_shape->AddPointEx(p1);
   poly_shape->AddPointEx(p2);

   if ( InRange(left_offset,crown_point_offset,right_offset) )
      poly_shape->AddPointEx(p3);

   poly_shape->AddPointEx(p4);
   poly_shape->AddPointEx(p5);

   if ( InRange(left_offset,crown_point_offset,right_offset) )
      poly_shape->AddPointEx(p6);

   display_list->AddDisplayObject(dispObj);
}

void CBridgeSectionView::BuildTrafficBarrierDisplayObjects()
{
   CPGSuperDoc* pDoc = (CPGSuperDoc*)GetDocument();
   CComPtr<IBroker> pBroker;
   pDoc->GetBroker(&pBroker);

   GET_IFACE2(pBroker,IRoadway,pAlignment);
   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,ISectProp2,pSectProp);

   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   CComPtr<iDisplayList> display_list;
   dispMgr->FindDisplayList(TRAFFIC_BARRIER_DISPLAY_LIST,&display_list);

   // left hand barrier
   CComPtr<iPointDisplayObject> left_dispObj;
   left_dispObj.CoCreateInstance(CLSID_PointDisplayObject);

   double cut_station = m_pFrame->GetCurrentCutLocation();
   double pier_1_station = pBridge->GetPierStation(0);
   double left_curb_offset  = pBridge->GetLeftCurbOffset(cut_station - pier_1_station);
   double right_curb_offset = pBridge->GetRightCurbOffset(cut_station - pier_1_station);

   double cpo = pAlignment->GetCrownPointOffset(cut_station);

   CComPtr<IShape> left_shape;
   pSectProp->GetLeftTrafficBarrierShape(cut_station,&left_shape);

   CComPtr<iShapeDrawStrategy> strategy;
   if ( left_shape )
   {
      // rotate the shape to match the crown slope
      double slope = ::BinarySign(cpo-left_curb_offset)*pAlignment->GetCrownSlope(cut_station,left_curb_offset);
      double angle = -atan(slope);
      CComQIPtr<IXYPosition> position(left_shape);
      CComPtr<IPoint2d> hook_point;
      position->get_LocatorPoint(lpHookPoint,&hook_point);
      position->RotateEx(hook_point,angle);

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
   pSectProp->GetRightTrafficBarrierShape(cut_station,&right_shape);

   if ( right_shape )
   {
      // rotate the shape to match the crown slope
      double slope = ::BinarySign(right_curb_offset-cpo)*pAlignment->GetCrownSlope(cut_station,right_curb_offset);
      double angle = atan(slope);
      CComQIPtr<IXYPosition> position(right_shape);
      CComPtr<IPoint2d> hook_point;
      position->get_LocatorPoint(lpHookPoint,&hook_point);
      position->RotateEx(hook_point,angle);

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
   double dist_from_start_of_bridge = pBridge->GetDistanceFromStartOfBridge(m_pFrame->GetCurrentCutLocation());
   double left_offset, right_offset;
   left_offset  = pBridge->GetLeftCurbOffset(dist_from_start_of_bridge);
   right_offset = pBridge->GetRightCurbOffset(dist_from_start_of_bridge);

   GET_IFACE2(pBroker,IRoadway,pRoadway);
   double left_elev  = pRoadway->GetElevation(m_pFrame->GetCurrentCutLocation(),left_offset);
   double right_elev = pRoadway->GetElevation(m_pFrame->GetCurrentCutLocation(),right_offset);

   double elev = _cpp_max(left_elev,right_elev);

   CComPtr<IPoint2d> p1, p2;
   p1.CoCreateInstance(CLSID_Point2d);
   p2.CoCreateInstance(CLSID_Point2d);
   p1->Move(left_offset, elev);
   p2->Move(right_offset,elev);

   CComPtr<iSocket> socket1, socket2;
   left_connectable->AddSocket(LEFT_CURB_SOCKET, p1,&socket1);
   right_connectable->AddSocket(RIGHT_CURB_SOCKET,p2,&socket2);

   // Put sockets at the hook point
   socket1.Release();
   socket2.Release();
   CComPtr<IPoint2d> pl,pr;

   if ( left_shape )
   {  
      pl.CoCreateInstance(CLSID_Point2d);
      left_offset = pBridge->GetLeftSlabEdgeOffset(dist_from_start_of_bridge);
      pl->put_X(left_offset);
      pl->put_Y(elev);
      left_connectable->AddSocket(LEFT_SLAB_EDGE_SOCKET, pl,&socket1);
   }

   if ( right_shape )
   {
      pr.CoCreateInstance(CLSID_Point2d);
      right_offset = pBridge->GetRightSlabEdgeOffset(dist_from_start_of_bridge);
      pr->put_X(right_offset);
      pr->put_Y(elev);
      right_connectable->AddSocket(RIGHT_SLAB_EDGE_SOCKET,pr,&socket2);
   }
}

void CBridgeSectionView::BuildDimensionLineDisplayObjects()
{
   CPGSuperDoc* pDoc = (CPGSuperDoc*)GetDocument();

   UINT settings = pDoc->GetBridgeEditorSettings();

   if ( !(settings & IDB_CS_SHOW_DIMENSIONS ) )
      return;


   CComPtr<IBroker> pBroker;
   pDoc->GetBroker(&pBroker);

   SpanIndexType spanIdx = GetSpanIndex();

   GET_IFACE2(pBroker,IBridge,pBridge);
   GirderIndexType nGirders = pBridge->GetGirderCount(spanIdx);

   // Get display lists
   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   CComPtr<iDisplayList> display_list;
   dispMgr->FindDisplayList(DIMENSION_DISPLAY_LIST,&display_list);

   CComPtr<iDisplayList> girder_list;
   dispMgr->FindDisplayList(GIRDER_DISPLAY_LIST,&girder_list);

   double cut_location = m_pFrame->GetCurrentCutLocation();
   double distFromStartOfBridge = pBridge->GetDistanceFromStartOfBridge(cut_location);
   double distFromStartOfSpan   = cut_location - pBridge->GetPierStation(spanIdx);

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
   double yLowest = DBL_MAX;
   for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
   {
      CComPtr<iDisplayObject> doGirder;

      SpanGirderHashType hash = HashSpanGirder(spanIdx,gdrIdx);
      std::map<SpanGirderHashType,long>::iterator found = m_GirderIDs.find(hash);

      if ( found == m_GirderIDs.end() )
         continue;

      long ID = (*found).second;

      girder_list->FindDisplayObject(ID,&doGirder);

      if ( !doGirder )
         continue;

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

      double y;
      p->get_Y(&y);
      yLowest = _cpp_min(y,yLowest);
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
      std::vector<SpaceBetweenGirder> vSpacing = pBridge->GetGirderSpacing(spanIdx, distFromStartOfSpan);
      std::vector<SpaceBetweenGirder>::iterator iter;
      for ( iter = vSpacing.begin(); iter != vSpacing.end(); iter++ )
      {
         SpaceBetweenGirder spacingData = *iter;

         // get the girder spacing for this group
         double spacing = spacingData.spacing;
         GirderIndexType firstGdrIdx = spacingData.firstGdrIdx;
         GirderIndexType lastGdrIdx  = spacingData.lastGdrIdx;
      
         long nSpacesInGroup = lastGdrIdx - firstGdrIdx;

         double total = spacing*nSpacesInGroup;

         // Create dimension line display object for this spacing group
         CComPtr<iDimensionLine> doDimLine;
         doDimLine.CoCreateInstance(CLSID_DimensionLineDisplayObject);
         CComQIPtr<iConnector> connector(doDimLine);

         // Going to attach dimension line to girder display objects, so get them now
         CComPtr<iDisplayObject> do1, do2;
         SpanGirderHashType firstHash = HashSpanGirder(spanIdx,firstGdrIdx);
         SpanGirderHashType lastHash  = HashSpanGirder(spanIdx,lastGdrIdx);
   
         std::map<SpanGirderHashType,long>::iterator found = m_GirderIDs.find(firstHash);
         if ( found == m_GirderIDs.end() )
            continue;

         long firstID = (*found).second;

         found = m_GirderIDs.find(lastHash);
         if ( found == m_GirderIDs.end() )
            continue;

         long lastID  = (*found).second;

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
               firstSocket = s1;

            if ( lastGdrIdx == nGirders-1 )
               lastSocket = s2;

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
               std::ostringstream os;
               if ( nSpacesInGroup == 1 )
               {
                  os << (LPCTSTR)strSpacing;
               }
               else
               {
                  os << nSpacesInGroup << " spaces @ " << (LPCTSTR)strSpacing << " = " << (LPCTSTR)strTotal;
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
      double leftOverhang = pBridge->GetLeftSlabOverhang(distFromStartOfBridge);
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
      double rightOverhang = pBridge->GetRightSlabOverhang(distFromStartOfBridge);
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

      double ccWidth = pBridge->GetCurbToCurbWidth( distFromStartOfBridge );
      CString strCurb = FormatDimension(ccWidth,rlen);
      ccText->SetText(strCurb);

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

            display_list->AddDisplayObject(tbDimLine);
         }
      }
   }
}

int CBridgeSectionView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CDisplayView::OnCreate(lpCreateStruct) == -1)
		return -1;
	
   m_pFrame = (CBridgeModelViewChildFrame*)GetParent()->GetParent();
   ASSERT( m_pFrame != 0 );
   ASSERT( m_pFrame->IsKindOf( RUNTIME_CLASS( CBridgeModelViewChildFrame ) ) );
	
	return 0;
}

void CBridgeSectionView::UpdateDrawingScale()
{
   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   CComPtr<iDisplayList> display_list;
   dispMgr->FindDisplayList(TITLE_DISPLAY_LIST,&display_list);

   if ( display_list == NULL )
      return;

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

SpanIndexType CBridgeSectionView::GetSpanIndex()
{
   CPGSuperDoc* pDoc = (CPGSuperDoc*)GetDocument();
   CComPtr<IBroker> pBroker;
   pDoc->GetBroker(&pBroker);

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   
   double cut_station = m_pFrame->GetCurrentCutLocation();

   const CSpanData* pSpan = pBridgeDesc->GetSpan(0);
   while ( pSpan )
   {
      double prev_pier_station = pSpan->GetPrevPier()->GetStation();
      double next_pier_station = pSpan->GetNextPier()->GetStation();

      if ( prev_pier_station <= cut_station && cut_station <= next_pier_station )
         return pSpan->GetSpanIndex();

      pSpan = pSpan->GetNextPier()->GetNextSpan();
   }

   return 0;
}


CBridgeModelViewChildFrame* CBridgeSectionView::GetFrame()
{
   CWnd* pWnd = GetParent()->GetParent();
   ATLASSERT( pWnd->IsKindOf( RUNTIME_CLASS(CBridgeModelViewChildFrame) ) );
   CBridgeModelViewChildFrame* pFrame = (CBridgeModelViewChildFrame*)pWnd;
   return pFrame;
}

void CBridgeSectionView::ClearSelection()
{
   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   dispMgr->ClearSelectedObjects();
}

void CBridgeSectionView::OnDraw(CDC* pDC)
{
   CDisplayView::OnDraw(pDC);

   if ( CWnd::GetFocus() == this && !pDC->IsPrinting() )
   {
   	DrawFocusRect();
   }
}

void CBridgeSectionView::OnSetFocus(CWnd* pOldWnd) 
{
	CDisplayView::OnSetFocus(pOldWnd);
	DrawFocusRect();
}

void CBridgeSectionView::OnKillFocus(CWnd* pNewWnd) 
{
	CDisplayView::OnKillFocus(pNewWnd);
	DrawFocusRect();
}

void CBridgeSectionView::DrawFocusRect()
{
   CClientDC dc(this);
   CRect rClient;
   GetClientRect(&rClient);
   dc.DrawFocusRect(rClient);
}

void CBridgeSectionView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
   if ( nChar == VK_UP && ::GetKeyState(VK_CONTROL) < 0 )
   {
      m_pFrame->GetBridgePlanView()->SetFocus();
      return;
   }
   else if ( nChar == VK_LEFT || nChar == VK_RIGHT )
   {
      CComPtr<iDisplayMgr> dispMgr;
      GetDisplayMgr(&dispMgr);
      DisplayObjectContainer selObjs;
      dispMgr->GetSelectedObjects(&selObjs);

      if ( selObjs.size() == 0 )
      {
         SpanIndexType spanIdx = GetSpanIndex();

         m_pFrame->SelectGirder(spanIdx,0);
         return;
      }
   }

	CDisplayView::OnKeyDown(nChar, nRepCnt, nFlags);
}

BOOL CBridgeSectionView::OnMouseWheel(UINT nFlags,short zDelta,CPoint pt)
{
   OnKeyDown(zDelta < 0 ? VK_RIGHT : VK_LEFT, 1, nFlags);
   return TRUE;
}
