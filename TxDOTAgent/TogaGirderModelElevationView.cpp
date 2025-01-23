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

// TogaGirderModelElevationView.cpp : implementation file
//
#include "stdafx.h"
#include "TogaGirderModelElevationView.h"
#include "TxDOTOptionalDesignDoc.h"

#include "PGSuperColors.h"

#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <IFace\DrawBridgeSettings.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\EditByUI.h>
#include <IFace\Intervals.h>

#include <PgsExt\BridgeDescription2.h>

#include "TogaDisplayObjectFactory.h"
#include "TogaSupportDrawStrategyImpl.h"
#include "TogaSectionCutDrawStrategy.h"
#include "TogaSectionCutDisplayImpl.h"
#include "TogaGMDisplayMgrEventsImpl.h"

#include <MfcTools\Text.h>
#include <sstream>

#include <WBFLGenericBridgeTools.h>
#include <WBFLGeometry/GeomHelpers.h>

// display list constants
#define GDR_LIST          1
#define STRAND_LIST       2
#define DEBOND_LIST       3
#define STRAND_CG_LIST    4
#define SUPPORT_LIST      5
#define DIMLINE_LIST      6
#define SECT_CUT_LIST     7
#define REBAR_LIST        8
#define STIRRUP_LIST     10

// display object ID
#define SECTION_CUT_ID   100



// simple, exception-safe class for blocking events
class SimpleMutex
{
public:
   SimpleMutex(bool& flag):
   m_Flag(flag)
   {
      m_Flag = true;
   }

   ~SimpleMutex()
   {
      m_Flag = false;
   }
private:
   bool& m_Flag;
};


/////////////////////////////////////////////////////////////////////////////
// CTogaGirderModelElevationView

IMPLEMENT_DYNCREATE(CTogaGirderModelElevationView, CDisplayView)

CTogaGirderModelElevationView::CTogaGirderModelElevationView():
m_First(true),
m_CurrID(0),
m_DoBlockUpdate(false)
{
   m_bUpdateError = false;
}

CTogaGirderModelElevationView::~CTogaGirderModelElevationView()
{
}


BEGIN_MESSAGE_MAP(CTogaGirderModelElevationView, CDisplayView)
	//{{AFX_MSG_MAP(CTogaGirderModelElevationView)
	ON_WM_CREATE()
	ON_WM_LBUTTONDOWN()

	ON_COMMAND(ID_LEFTEND, OnLeftEnd)
	ON_COMMAND(ID_LEFT_HP, OnLeftHp)
	ON_COMMAND(ID_CENTER, OnCenter)
	ON_COMMAND(ID_RIGHT_HP, OnRightHp)
	ON_COMMAND(ID_RIGHTEND, OnRightEnd)
	ON_COMMAND(ID_USER_CUT, OnUserCut)
	ON_WM_SIZE()
	ON_WM_LBUTTONUP()

	ON_COMMAND(ID_VIEWSETTINGS, OnViewSettings)

	ON_WM_CONTEXTMENU()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_TIMER()
	ON_WM_DESTROY()
   ON_WM_MOUSEWHEEL()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CTogaGirderModelElevationView drawing
void CTogaGirderModelElevationView::OnInitialUpdate() 
{
   HRESULT hr = S_OK;

	CDisplayView::OnInitialUpdate();
   EnableToolTips();

   // Setup the local display object factory
   m_pDispMgr->EnableLBtnSelect(TRUE);
   m_pDispMgr->EnableRBtnSelect(TRUE);
   m_pDispMgr->SetSelectionLineColor(SELECTED_OBJECT_LINE_COLOR);
   m_pDispMgr->SetSelectionFillColor(SELECTED_OBJECT_FILL_COLOR);

   CTxDOTOptionalDesignDoc* pDoc = (CTxDOTOptionalDesignDoc*)GetDocument();
   auto factory = std::make_shared<CTogaDisplayObjectFactory>(pDoc);
   m_pDispMgr->AddDisplayObjectFactory(factory);

   // set up default event handler for canvas
   auto events = std::make_shared<CTogaGMDisplayMgrEventsImpl>(pDoc, m_pFrame, this);
   m_pDispMgr->RegisterEventSink(events);

   // Create display lists
   // section cut - add first so it's always on top
   m_pDispMgr->CreateDisplayList(SECT_CUT_LIST);
   m_pDispMgr->CreateDisplayList(DIMLINE_LIST);
   m_pDispMgr->CreateDisplayList(SUPPORT_LIST);
   m_pDispMgr->CreateDisplayList(DEBOND_LIST);
   m_pDispMgr->CreateDisplayList(STRAND_LIST);
   m_pDispMgr->CreateDisplayList(STRAND_CG_LIST);
   m_pDispMgr->CreateDisplayList(REBAR_LIST);
   m_pDispMgr->CreateDisplayList(STIRRUP_LIST);
   m_pDispMgr->CreateDisplayList(GDR_LIST);
}

void CTogaGirderModelElevationView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
   // block updates so this function is not reentrant
   if (m_DoBlockUpdate)
      return;

   SimpleMutex mutex(m_DoBlockUpdate);

   m_bUpdateError = false;

   CDisplayView::OnUpdate(pSender,lHint,pHint);

   // do update
   m_CurrID = 0;

   if (m_First)
   {
      // We don't want to build display objects until we are all the way through OnInitialUpdate
      m_First = false;
      return;
   }
   else if ( lHint == 0 ||
        lHint == HINT_GIRDERVIEWSETTINGSCHANGED ||
        lHint == HINT_GIRDERCHANGED )
   {
      // set up a valid dc first
      CDManipClientDC dc(this);

      UpdateDisplayObjects();
      ScaleToFit(false);
   }
   else if (lHint==HINT_GIRDERVIEWSECTIONCUTCHANGED)
   {
      // set up a valid dc first
      CDManipClientDC dc(this);

      // only need to update section cut location
      auto pDL = m_pDispMgr->FindDisplayList(SECT_CUT_LIST);
      ATLASSERT(pDL);

      auto dispObj = pDL->FindDisplayObject(SECTION_CUT_ID);
      ATLASSERT(dispObj);

      auto sink = dispObj->GetEventSink();

      sink->OnChanged(dispObj);
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

	Invalidate(TRUE);
}

void CTogaGirderModelElevationView::UpdateDisplayObjects()
{
   // clean out all the display objects
   m_pDispMgr->ClearDisplayObjects();

   CTxDOTOptionalDesignDoc* pDoc = (CTxDOTOptionalDesignDoc*)GetDocument();

   SpanIndexType span;
   GirderIndexType girder;

   m_pFrame->GetSpanAndGirderSelection(&span,&girder);

   CSegmentKey segmentKey(span,girder,0);

   // Grab hold of the broker so we can pass it as a parameter
   CComPtr<IBroker> pBroker = pDoc->GetUpdatedBroker();

   UINT settings = pDoc->GetGirderEditorSettings();

   BuildGirderDisplayObjects(pDoc, pBroker, segmentKey);
   BuildSupportDisplayObjects(pDoc, pBroker, segmentKey);

   if (settings & IDG_EV_SHOW_STRANDS)
      BuildStrandDisplayObjects(pDoc, pBroker, segmentKey);

   if (settings & IDG_EV_SHOW_PS_CG)
      BuildStrandCGDisplayObjects(pDoc, pBroker, segmentKey);

   if (settings & IDG_EV_SHOW_LONG_REINF)
      BuildRebarDisplayObjects(pDoc, pBroker, segmentKey);

   if (settings & IDG_EV_SHOW_STIRRUPS)
      BuildStirrupDisplayObjects(pDoc, pBroker, segmentKey);

   if (settings & IDG_EV_SHOW_DIMENSIONS)
      BuildDimensionDisplayObjects(pDoc, pBroker, segmentKey);

   BuildSectionCutDisplayObjects(pDoc, pBroker, segmentKey);

   auto mode = (settings & IDG_EV_DRAW_ISOTROPIC) ? WBFL::DManip::MapMode::Isotropic : WBFL::DManip::MapMode::Anisotropic;
   CDisplayView::SetMappingMode(mode);
}

void CTogaGirderModelElevationView::DoPrint(CDC* pDC, CPrintInfo* pInfo)
{
   OnBeginPrinting(pDC, pInfo);
   OnPrepareDC(pDC);
   ScaleToFit();
   OnDraw(pDC);
   OnEndPrinting(pDC, pInfo);
}

/////////////////////////////////////////////////////////////////////////////
// CTogaGirderModelElevationView diagnostics

#ifdef _DEBUG
void CTogaGirderModelElevationView::AssertValid() const
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
//   AFX_MANAGE_STATE(AfxGetAppModuleState());
	CDisplayView::AssertValid();
}

void CTogaGirderModelElevationView::Dump(CDumpContext& dc) const
{
	CDisplayView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CTogaGirderModelElevationView message handlers

int CTogaGirderModelElevationView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CDisplayView::OnCreate(lpCreateStruct) == -1)
		return -1;
	
   m_pFrame = (CTxDOTOptionalDesignGirderViewPage*)GetParent();
   ASSERT( m_pFrame != 0 );

	return 0;
} 

DROPEFFECT CTogaGirderModelElevationView::CanDrop(COleDataObject* pDataObject,DWORD dwKeyState, const WBFL::Geometry::Point2d& point)
{
   // This override has to determine if the thing being dragged over it can
   // be dropped. In order to do that, it must unpackage the OleDataObject.
   //
   // The stuff in the data object is just from the display object. The display
   // objects need to be updated so that the client can attach an object to it
   // that knows how to package up domain specific information. At the same
   // time, this view needs to be able to get some domain specific hint 
   // as to the type of data that is going to be dropped.

   if ( pDataObject->IsDataAvailable(CTogaSectionCutDisplayImpl::ms_Format) )
   {
      // need to peek at our object first and make sure it's coming from the local process
      // this is ugly because it breaks encapsulation of CSectionCutDisplayImpl
      auto source = WBFL::DManip::DragDataSource::Create();
      source->SetDataObject(pDataObject);
      source->PrepareFormat(CTogaSectionCutDisplayImpl::ms_Format);

      CWinThread* thread = ::AfxGetThread( );
      DWORD threadid = thread->m_nThreadID;

      DWORD threadl;
      // know (by voodoo) that the first member of this data source is the thread id
      source->Read(CTogaSectionCutDisplayImpl::ms_Format,&threadl,sizeof(DWORD));

      if (threadl == threadid)
        return DROPEFFECT_MOVE;
   }
   else
   {
      if (m_Legend)
      {
         auto drag = std::dynamic_pointer_cast<WBFL::DManip::iDraggable>(m_Legend);
         UINT format = drag->Format();
         if ( pDataObject->IsDataAvailable(format) )
            return DROPEFFECT_MOVE;
      }
   }

   return DROPEFFECT_NONE;
}

void CTogaGirderModelElevationView::OnLeftEnd() 
{
   m_pFrame->CutAtLeftEnd();
}

void CTogaGirderModelElevationView::OnLeftHp() 
{
   m_pFrame->CutAtLeftHp();
}

void CTogaGirderModelElevationView::OnCenter() 
{
   m_pFrame->CutAtCenter();
}

void CTogaGirderModelElevationView::OnRightHp() 
{
   m_pFrame->CutAtRightHp();
}

void CTogaGirderModelElevationView::OnRightEnd() 
{
   m_pFrame->CutAtRightEnd();
}

void CTogaGirderModelElevationView::OnUserCut() 
{
	m_pFrame->CutAtLocation();
}

void CTogaGirderModelElevationView::OnSize(UINT nType, int cx, int cy) 
{
	CDisplayView::OnSize(nType, cx, cy);

   if (!m_First)
   {

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
}

void CTogaGirderModelElevationView::OnViewSettings() 
{
	((CTxDOTOptionalDesignDoc*)GetDocument())->EditGirderViewSettings(VS_GIRDER_ELEVATION);
}

void CTogaGirderModelElevationView::BuildGirderDisplayObjects(CTxDOTOptionalDesignDoc* pDoc,IBroker* pBroker,const CSegmentKey& segmentKey)
{
   // get the display list and clear out any old display objects
   auto pDL =  m_pDispMgr->FindDisplayList(GDR_LIST);
   ATLASSERT(pDL);
   pDL->Clear();

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();

   // the origin of the coordinate system is at the left end of the girder
   // at the CG
   GET_IFACE2(pBroker,ISectionProperties,pSectProp);
   pgsPointOfInterest poi(segmentKey,0.00); // start of girder
   Float64 Yb = pSectProp->GetY(releaseIntervalIdx,poi,pgsTypes::BottomGirder);

   // get the shape of the girder in profile
   GET_IFACE2(pBroker,IGirder,pGirder);
   CComPtr<IShape> shape;
   pGirder->GetSegmentProfile(segmentKey,false,&shape);

   // create the display object
   auto doPnt = WBFL::DManip::PointDisplayObject::Create(1);

   // create the drawing strategy
   auto strategy = WBFL::DManip::ShapeDrawStrategy::Create();
   doPnt->SetDrawingStrategy(strategy);

   // configure the strategy
   strategy->SetShape(geomUtil::ConvertShape(shape));
   strategy->SetSolidLineColor(SEGMENT_BORDER_COLOR);
   strategy->SetSolidFillColor(SEGMENT_FILL_COLOR);
   strategy->SetVoidLineColor(VOID_BORDER_COLOR);
   strategy->SetVoidFillColor(GetSysColor(COLOR_WINDOW));
   strategy->Fill(true);

   // set the tool tip text
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 gdr_length, span_length;
   gdr_length  = pBridge->GetSegmentLength(segmentKey);
   span_length = pBridge->GetSegmentSpanLength(segmentKey);
   CString strMsg1;
   strMsg1.Format(_T("Girder: %s\r\nGirder Length: %s\r\nSpan Length: %s"),
                  pBridgeDesc->GetGirderGroup(segmentKey.groupIndex)->GetGirder(segmentKey.girderIndex)->GetGirderName(),
                  FormatDimension(gdr_length,pDisplayUnits->GetSpanLengthUnit()),
                  FormatDimension(span_length,pDisplayUnits->GetSpanLengthUnit())
                  );

   GET_IFACE2(pBroker,IMaterials,pMaterials);
   Float64 fci = pMaterials->GetSegmentFc(segmentKey,releaseIntervalIdx);
   Float64 fc  = pMaterials->GetSegmentFc(segmentKey,liveLoadIntervalIdx);

   CString strMsg2;
   strMsg2.Format(_T("\r\n\r\nf'ci: %s\r\nf'c: %s"),
                  FormatDimension(fci,pDisplayUnits->GetStressUnit()),
                  FormatDimension(fc, pDisplayUnits->GetStressUnit())
                  );

   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeom);
   const auto* pStrand     = pMaterials->GetStrandMaterial(segmentKey,pgsTypes::Straight);
   const auto* pTempStrand = pMaterials->GetStrandMaterial(segmentKey,pgsTypes::Temporary);

   StrandIndexType Ns, Nh, Nt, Nsd;
   Ns = pStrandGeom->GetStrandCount(segmentKey,pgsTypes::Straight);
   Nh = pStrandGeom->GetStrandCount(segmentKey,pgsTypes::Harped);
   Nt = pStrandGeom->GetStrandCount(segmentKey,pgsTypes::Temporary);
   Nsd= pStrandGeom->GetNumDebondedStrands(segmentKey,pgsTypes::Straight,pgsTypes::dbetEither);

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

   CString strMsg = strMsg1 + strMsg2 + strMsg3;

   doPnt->SetMaxTipWidth(TOOLTIP_WIDTH);
   doPnt->SetTipDisplayTime(TOOLTIP_DURATION);
   doPnt->SetToolTipText(strMsg);

   // put the display object in its display list
   pDL->AddDisplayObject(doPnt);

   ///////////////////////////////////////////////////////////////////////////////////////////

   // May need this code to set up plugs/sockets for dimension lines
   Float64 start_brg_offset = pBridge->GetSegmentStartBearingOffset(segmentKey);
   Float64 start_end_distance = pBridge->GetSegmentStartEndDistance(segmentKey);
   Float64 start_offset = start_brg_offset - start_end_distance;

   // make starting point
   WBFL::Geometry::Point2d point(start_offset, 0.0);

   auto startpt_rep = WBFL::DManip::PointDisplayObject::Create(100);
   startpt_rep->SetPosition(point,false,false);
   auto start_connectable = std::dynamic_pointer_cast<WBFL::DManip::iConnectable>(startpt_rep);
   auto start_socket = start_connectable->AddSocket(0,point);
   pDL->AddDisplayObject(startpt_rep);

   WBFL::Geometry::Point2d point2(start_offset + gdr_length, 0.0);
   
   // end point display object
   auto endpt_rep = WBFL::DManip::PointDisplayObject::Create(200);
   endpt_rep->SetPosition(point2,false,false);
   auto end_connectable = std::dynamic_pointer_cast<WBFL::DManip::iConnectable>(endpt_rep);
   auto end_socket = end_connectable->AddSocket(0,point2);

   pDL->AddDisplayObject(endpt_rep);

   // line to represent girder
   auto ssm_rep = WBFL::DManip::LineDisplayObject::Create(101);

   // plug in
   auto connector = std::dynamic_pointer_cast<WBFL::DManip::iConnector>(ssm_rep);

   auto startPlug = connector->GetStartPlug();
   auto endPlug = connector->GetEndPlug();

   auto startConnectable = std::dynamic_pointer_cast<WBFL::DManip::iConnectable>(startpt_rep);
   auto socket = startConnectable->GetSocket(0,WBFL::DManip::AccessType::ByIndex);
   socket->Connect(startPlug);

   auto endConnectable = std::dynamic_pointer_cast<WBFL::DManip::iConnectable>(endpt_rep);
   socket = endConnectable->GetSocket(0,WBFL::DManip::AccessType::ByIndex);
   socket->Connect(endPlug);

   pDL->AddDisplayObject(ssm_rep);
   ///////////////////////////////////////////////////////////////////////////////////////////
}

void CTogaGirderModelElevationView::BuildSupportDisplayObjects(CTxDOTOptionalDesignDoc* pDoc, IBroker* pBroker,const CSegmentKey& segmentKey)
{
   auto pDL = m_pDispMgr->FindDisplayList(SUPPORT_LIST);
   ATLASSERT(pDL);
   pDL->Clear();

   // Get location to display the connection symbols
   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 gdr_length = pBridge->GetSegmentLength(segmentKey);
   Float64 start_lgth = pBridge->GetSegmentStartEndDistance(segmentKey);
   Float64 end_lgth   = pBridge->GetSegmentEndEndDistance(segmentKey);

   // Get POI for these locations
   pgsPointOfInterest poiStartBrg(segmentKey,start_lgth);
   pgsPointOfInterest poiEndBrg(segmentKey,gdr_length - end_lgth);

   // Display at bottom of girder.
   GET_IFACE2(pBroker,IGirder,pGirder);
   Float64 Hg_start = pGirder->GetHeight(poiStartBrg);
   Float64 Hg_end   = pGirder->GetHeight(poiEndBrg);

   //
   // left support
   //

   // create point
   WBFL::Geometry::Point2d point(start_lgth, -Hg_start);

   // create display object
   auto ptDispObj = WBFL::DManip::PointDisplayObject::Create(0);
   ptDispObj->SetPosition(point, false, false);

   // create drawing strategy
   auto pDrawStrategy = std::make_shared<CTogaSupportDrawStrategyImpl>(pDoc);
   ptDispObj->SetDrawingStrategy(pDrawStrategy);

   pDL->AddDisplayObject(ptDispObj);

   // right support
   WBFL::Geometry::Point2d point2(gdr_length-end_lgth, -Hg_end);

   ptDispObj = WBFL::DManip::PointDisplayObject::Create(1);
   ptDispObj->SetPosition(point2, false, false);

   pDrawStrategy = std::make_shared<CTogaSupportDrawStrategyImpl>(pDoc);
   ptDispObj->SetDrawingStrategy(pDrawStrategy);

   pDL->AddDisplayObject(ptDispObj);
}

void CTogaGirderModelElevationView::BuildStrandDisplayObjects(CTxDOTOptionalDesignDoc* pDoc, IBroker* pBroker,const CSegmentKey& segmentKey)
{
   auto pDL = m_pDispMgr->FindDisplayList(STRAND_LIST);
   ATLASSERT(pDL);
   pDL->Clear();

   auto pDebondDL = m_pDispMgr->FindDisplayList(DEBOND_LIST);
   ATLASSERT(pDebondDL);
   pDebondDL->Clear();

   GET_IFACE2(pBroker, IBridge, pBridge);
   GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc);
   GET_IFACE2(pBroker, IStrandGeometry, pStrandGeometry);

   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(segmentKey.groupIndex);
   const CSplicedGirderData* pGirder = pGroup->GetGirder(segmentKey.girderIndex);
   const CPrecastSegmentData* pSegment = pGirder->GetSegment(segmentKey.segmentIndex);

   Float64 segment_length = pBridge->GetSegmentLength(segmentKey);
   Float64 start_brg_offset = pBridge->GetSegmentStartBearingOffset(segmentKey);
   Float64 start_end_distance = pBridge->GetSegmentStartEndDistance(segmentKey);
   Float64 start_offset = start_brg_offset - start_end_distance;

   for (int i = 0; i < 3; i++)
   {
      pgsTypes::StrandType strandType = (pgsTypes::StrandType)i;
      StrandIndexType nStrands = pStrandGeometry->GetStrandCount(segmentKey, strandType);
      for (StrandIndexType strandIdx = 0; strandIdx < nStrands; strandIdx++)
      {
         CComPtr<IPoint2dCollection> profilePoints;
         pStrandGeometry->GetStrandProfile(segmentKey, strandType, strandIdx, &profilePoints);

         IndexType nPoints;
         profilePoints->get_Count(&nPoints);
         ATLASSERT(2 <= nPoints);

         CComPtr<IPoint2d> from_point, to_point;
         profilePoints->get_Item(0, &from_point);
         for (IndexType pntIdx = 1; pntIdx < nPoints; pntIdx++)
         {
            to_point.Release();
            profilePoints->get_Item(pntIdx, &to_point);
            BuildLine(pDL, 0.0, geomUtil::GetPoint(from_point), geomUtil::GetPoint(to_point), STRAND_FILL_COLOR);
            from_point = to_point;
         } // next pntIdx
      } // next strandIdx
   } // next i

   // draw debonded strands... this must happen after other other strands
   // so the debonded lines are drawn on top
   // Also... the strand profile is the actual profile of the bonded strands we want to draw the unbonded
   // portion
   StrandIndexType nStrands = pStrandGeometry->GetStrandCount(segmentKey, pgsTypes::Straight);
   for (StrandIndexType strandIdx = 0; strandIdx < nStrands; strandIdx++)
   {
      Float64 start, end;
      if (pStrandGeometry->IsStrandDebonded(segmentKey, strandIdx, pgsTypes::Straight, nullptr, &start, &end))
      {
         CComPtr<IPoint2dCollection> profilePoints;
         pStrandGeometry->GetStrandProfile(segmentKey, pgsTypes::Straight, strandIdx, &profilePoints);

         IndexType nPoints;
         profilePoints->get_Count(&nPoints);

         if (!IsZero(start))
         {
            // Left debond point
            CComPtr<IPoint2d> left_debond;
            profilePoints->get_Item(0, &left_debond);

            CComPtr<IPoint2d> from_point;
            from_point.CoCreateInstance(CLSID_Point2d);
            from_point->MoveEx(left_debond);
            from_point->put_X(start_offset);

            BuildLine(pDebondDL, 0.0, geomUtil::GetPoint(from_point), geomUtil::GetPoint(left_debond), DEBOND_FILL_COLOR);
            BuildDebondTick(pDebondDL, 0.0, geomUtil::GetPoint(left_debond), DEBOND_FILL_COLOR);
         }

         if (!IsEqual(end, segment_length))
         {
            CComPtr<IPoint2d> right_debond;
            profilePoints->get_Item(nPoints - 1, &right_debond);

            CComPtr<IPoint2d> to_point;
            to_point.CoCreateInstance(CLSID_Point2d);
            to_point->MoveEx(right_debond);
            to_point->put_X(start_offset + segment_length);

            BuildLine(pDebondDL, 0.0, geomUtil::GetPoint(right_debond), geomUtil::GetPoint(to_point), DEBOND_FILL_COLOR);
            BuildDebondTick(pDebondDL, 0.0, geomUtil::GetPoint(right_debond), DEBOND_FILL_COLOR);
         }
      }
   }
}

void CTogaGirderModelElevationView::BuildStrandCGDisplayObjects(CTxDOTOptionalDesignDoc* pDoc, IBroker* pBroker,const CSegmentKey& segmentKey)
{
   auto pDL = m_pDispMgr->FindDisplayList(STRAND_CG_LIST);
   ATLASSERT(pDL);
   pDL->Clear();

   GET_IFACE2(pBroker, IBridge, pBridge);
   Float64 start_brg_offset = pBridge->GetSegmentStartBearingOffset(segmentKey);
   Float64 start_end_distance = pBridge->GetSegmentStartEndDistance(segmentKey);
   Float64 start_offset = start_brg_offset - start_end_distance;

   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeometry);
   StrandIndexType ns = pStrandGeometry->GetStrandCount(segmentKey, pgsTypes::Straight);
   ns += pStrandGeometry->GetStrandCount(segmentKey, pgsTypes::Harped);
   ns += pStrandGeometry->GetStrandCount(segmentKey, pgsTypes::Temporary);
   if (0 < ns)
   {
      GET_IFACE2(pBroker,ISectionProperties,pSectProp);
      GET_IFACE2(pBroker,IIntervals,pIntervals);

      CComPtr<IPoint2d> from_point, to_point;
      from_point.CoCreateInstance(__uuidof(Point2d));
      to_point.CoCreateInstance(__uuidof(Point2d));

      bool red = false;

      GET_IFACE2(pBroker,IPointOfInterest,pPOI);
      PoiList vPOI;
      pPOI->GetPointsOfInterest(segmentKey, &vPOI);

      Float64 from_y;
      Float64 to_y;
      auto iter( vPOI.begin() );
      pgsPointOfInterest prev_poi = *iter;

      IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(prev_poi.GetSegmentKey());

      Float64 hg  = pSectProp->GetHg(releaseIntervalIdx,prev_poi);
      Float64 ybg = pSectProp->GetY(releaseIntervalIdx,prev_poi,pgsTypes::BottomGirder);
      Float64 ey = pStrandGeometry->GetEccentricity(releaseIntervalIdx, prev_poi, pgsTypes::Permanent).Y();
      from_y = ybg - (hg + ey);

      for ( ; iter!= vPOI.end(); iter++ )
      {
         const pgsPointOfInterest& poi = *iter;
      
         releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(poi.GetSegmentKey());

         hg  = pSectProp->GetHg(releaseIntervalIdx,poi);
         ybg = pSectProp->GetY(releaseIntervalIdx,poi,pgsTypes::BottomGirder);
         ey = pStrandGeometry->GetEccentricity(releaseIntervalIdx, poi, pgsTypes::Permanent).Y();
         to_y = ybg - (hg + ey);

         from_point->put_X(prev_poi.GetDistFromStart());
         from_point->put_Y(from_y);
         to_point->put_X(poi.GetDistFromStart());
         to_point->put_Y(to_y);

         COLORREF col = red ? RED : WHITE;

         BuildLine(pDL, start_offset, geomUtil::GetPoint(from_point), geomUtil::GetPoint(to_point), col);

         red = !red;

         prev_poi = poi;
         from_y = to_y;
      }
   }
}

void CTogaGirderModelElevationView::BuildRebarDisplayObjects(CTxDOTOptionalDesignDoc* pDoc, IBroker* pBroker, const CSegmentKey& segmentKey)
{
   auto pDL = m_pDispMgr->FindDisplayList(REBAR_LIST);
   ATLASSERT(pDL);
   pDL->Clear();

   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,ILongRebarGeometry,pLongRebarGeometry);
   GET_IFACE2(pBroker,IPointOfInterest,pPOI);
   GET_IFACE2(pBroker,IGirder,pGirder);

   Float64 gdr_length = pBridge->GetSegmentLength(segmentKey);

   Float64 start_brg_offset = pBridge->GetSegmentStartBearingOffset(segmentKey);
   Float64 start_end_distance = pBridge->GetSegmentStartEndDistance(segmentKey);
   Float64 start_offset = start_brg_offset - start_end_distance;

   pgsPointOfInterest poiStart( pPOI->GetPointOfInterest(segmentKey,0.0) );
   pgsPointOfInterest poiEnd(   pPOI->GetPointOfInterest(segmentKey,gdr_length) );

   Float64 HgStart = pGirder->GetHeight(poiStart);
   Float64 HgEnd   = pGirder->GetHeight(poiEnd);

   if ( poiStart.GetID() < 0 )
      poiStart.SetDistFromStart(0.0);

   if ( poiEnd.GetID() < 0 )
      poiEnd.SetDistFromStart(gdr_length);

   CComPtr<IRebarSection> rebar_section_start, rebar_section_end;
   pLongRebarGeometry->GetRebars(poiStart,&rebar_section_start);
   pLongRebarGeometry->GetRebars(poiEnd,  &rebar_section_end);

   CComPtr<IEnumRebarSectionItem> enum_start, enum_end;
   rebar_section_start->get__EnumRebarSectionItem(&enum_start);
   rebar_section_end->get__EnumRebarSectionItem(&enum_end);

   CComPtr<IRebarSectionItem> startItem, endItem;
   while ( enum_start->Next(1,&startItem,nullptr) != S_FALSE && enum_end->Next(1,&endItem,nullptr) != S_FALSE )
   {
      CComPtr<IPoint2d> startLocation, endLocation;
      startItem->get_Location(&startLocation);
      endItem->get_Location(&endLocation);

      Float64 yStart;
      startLocation->get_Y(&yStart);

      Float64 yEnd;
      endLocation->get_Y(&yEnd);

      startItem.Release();
      endItem.Release();

      WBFL::Geometry::Point2d from_point(0.0, yStart);
      WBFL::Geometry::Point2d to_point(gdr_length, yEnd);
      
      BuildLine(pDL, start_offset, from_point, to_point, REBAR_COLOR);
   }
}

void CTogaGirderModelElevationView::BuildDimensionDisplayObjects(CTxDOTOptionalDesignDoc* pDoc, IBroker* pBroker,const CSegmentKey& segmentKey)
{
   auto pDL = m_pDispMgr->FindDisplayList(DIMLINE_LIST);
   ATLASSERT(pDL);
   pDL->Clear();

   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IGirder,pGirder);
   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeometry);

   Float64 gdr_length  = pBridge->GetSegmentLength(segmentKey);
   Float64 start_lgth  = pBridge->GetSegmentStartEndDistance(segmentKey);
   Float64 end_lgth    = pBridge->GetSegmentEndEndDistance(segmentKey);
   Float64 span_length = pBridge->GetSegmentSpanLength(segmentKey);

   StrandIndexType Nh = pStrandGeometry->GetStrandCount(segmentKey,pgsTypes::Harped);

   Float64 Hg_start = pGirder->GetHeight(pgsPointOfInterest(segmentKey,0.00) );
   Float64 Hg_end   = pGirder->GetHeight(pgsPointOfInterest(segmentKey,gdr_length) );

   Float64 lft_harp, rgt_harp;
   pStrandGeometry->GetHarpingPointLocations(segmentKey, &lft_harp, &rgt_harp);

   WBFL::Geometry::Point2d from_point, to_point;

   // need to layout dimension line witness lines in twips
   const long twip_offset = 1440/2;
/*
   // girder length (top dimension line)
   from_point->put_X(0.0);
   from_point->put_Y(0.0);

   to_point->put_X(gdr_length);
   to_point->put_Y(0.0);

   dimLine = BuildDimensionLine(pDL, from_point, to_point, gdr_length);
   dimLine->SetWitnessLength(twip_offset);

   // harp locations (along top)
   if ( 0 < Nh )
   {
      to_point->put_X(lft_harp);
      dimLine = BuildDimensionLine(pDL, from_point, to_point,lft_harp);
      dimLine->SetWitnessLength(twip_offset/2);

      from_point->put_X(rgt_harp);
      to_point->put_X(gdr_length);
      dimLine = BuildDimensionLine(pDL, from_point, to_point, gdr_length-rgt_harp);
      dimLine->SetWitnessLength(twip_offset/2);
   }
*/
   // support distances (along bottom)
   from_point.Move(start_lgth, -Max(Hg_start,Hg_end));
   to_point.Move(gdr_length - end_lgth, -Max(Hg_start,Hg_end));
   auto dimLine = BuildDimensionLine(pDL, from_point, to_point, span_length);
   dimLine->SetWitnessLength(-twip_offset);

   // harp locations
   if ( 0 < Nh )
   {
      to_point.X() = lft_harp;
      dimLine = BuildDimensionLine(pDL, from_point, to_point, lft_harp-start_lgth);
      dimLine->SetWitnessLength(-twip_offset/2);

      from_point.X() = rgt_harp;
      to_point.X() = gdr_length - end_lgth;
      dimLine = BuildDimensionLine(pDL, from_point, to_point, gdr_length-end_lgth-rgt_harp);
      dimLine->SetWitnessLength(-twip_offset/2);
   }
}

void CTogaGirderModelElevationView::BuildSectionCutDisplayObjects(CTxDOTOptionalDesignDoc* pDoc, IBroker* pBroker,const CSegmentKey& segmentKey)
{
   auto factory = m_pDispMgr->GetDisplayObjectFactory(0);

   auto disp_obj = factory->Create(CTogaSectionCutDisplayImpl::ms_Format,nullptr);

   auto sink = disp_obj->GetEventSink();

   auto point_disp = std::dynamic_pointer_cast<WBFL::DManip::iPointDisplayObject>(disp_obj);
   point_disp->SetToolTipText(_T("Click on me and drag to move section cut"));

   auto sc_strat = std::dynamic_pointer_cast<iTogaSectionCutDrawStrategy>(sink);
   sc_strat->Init(point_disp, pBroker, segmentKey, m_pFrame);
   sc_strat->SetColor(CUT_COLOR);

   point_disp->SetID(SECTION_CUT_ID);

   auto pDL = m_pDispMgr->FindDisplayList(SECT_CUT_LIST);
   ATLASSERT(pDL);
   pDL->Clear();
   pDL->AddDisplayObject(disp_obj);
}

void CTogaGirderModelElevationView::BuildStirrupDisplayObjects(CTxDOTOptionalDesignDoc* pDoc, IBroker* pBroker,const CSegmentKey& segmentKey)
{
   auto pDL = m_pDispMgr->FindDisplayList(STIRRUP_LIST);
   ATLASSERT(pDL);
   pDL->Clear();

   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IStirrupGeometry,pStirrupGeom);

   Float64 start_brg_offset = pBridge->GetSegmentStartBearingOffset(segmentKey);
   Float64 start_end_distance = pBridge->GetSegmentStartEndDistance(segmentKey);
   Float64 start_offset = start_brg_offset - start_end_distance;

   // assume a typical cover
   Float64 top_cover = WBFL::Units::ConvertToSysUnits(2.0,WBFL::Units::Measure::Inch);
   Float64 bot_cover = WBFL::Units::ConvertToSysUnits(2.0,WBFL::Units::Measure::Inch);

   PierIndexType startPierIdx = pBridge->GetGirderGroupStartPier(segmentKey.groupIndex);
   Float64 slab_offset = pBridge->GetSlabOffset(segmentKey,pgsTypes::metStart); // use for dummy top of stirrup if they are extended into deck

   pgsTypes::SupportedDeckType deckType = pBridge->GetDeckType();
   bool bDoStirrupsEngageDeck = pStirrupGeom->DoStirrupsEngageDeck(segmentKey);

   ZoneIndexType nStirrupZones = pStirrupGeom->GetPrimaryZoneCount(segmentKey);
   for ( ZoneIndexType zoneIdx = 0; zoneIdx < nStirrupZones; zoneIdx++ )
   {
      Float64 start, end;
      pStirrupGeom->GetPrimaryZoneBounds(segmentKey, zoneIdx, &start, &end);

      WBFL::Materials::Rebar::Size barSize;
      Float64 spacing;
      Float64 nStirrups;
      pStirrupGeom->GetPrimaryVertStirrupBarInfo(segmentKey,zoneIdx,&barSize,&nStirrups,&spacing);

      if ( barSize != WBFL::Materials::Rebar::Size::bsNone && nStirrups != 0 )
      {
         GET_IFACE2(pBroker,IGirder,pGirder);

         ZoneIndexType nStirrupsInZone = ZoneIndexType(floor((end - start)/spacing));
         spacing = (end-start)/nStirrupsInZone;
         for ( ZoneIndexType i = 0; i <= nStirrupsInZone; i++ )
         {
            Float64 x = start_offset + start + i*spacing;

            pgsPointOfInterest poi(segmentKey,x);

            Float64 Hg = pGirder->GetHeight(poi);

            Float64 bottom = bot_cover - Hg;

            Float64 top;
            if (deckType == pgsTypes::sdtNone || !bDoStirrupsEngageDeck)
            {
               top = -top_cover;
            }
            else
            {
               top = slab_offset;
            }

            WBFL::Geometry::Point2d p1(x,bottom);
            WBFL::Geometry::Point2d p2(x,top);

            auto doPnt1 = WBFL::DManip::PointDisplayObject::Create();
            auto doPnt2 = WBFL::DManip::PointDisplayObject::Create();
            doPnt1->SetPosition(p1,false,false);
            doPnt2->SetPosition(p2,false,false);

            auto c1 = std::dynamic_pointer_cast<WBFL::DManip::iConnectable>(doPnt1);
            auto c2 = std::dynamic_pointer_cast<WBFL::DManip::iConnectable>(doPnt2);

            auto s1 = c1->AddSocket(0, p1);
            auto s2 = c2->AddSocket(0,p2);

            auto line = WBFL::DManip::LineDisplayObject::Create();

            // color
            auto pStrategy = line->GetDrawLineStrategy();

            // dangerous cast here
            auto pSimple = std::dynamic_pointer_cast<WBFL::DManip::SimpleDrawLineStrategy>(pStrategy);
            if (pSimple)
            {
               pSimple->SetColor(STIRRUP_COLOR);
               pSimple->SetBeginType(WBFL::DManip::PointType::None);
               pSimple->SetEndType(WBFL::DManip::PointType::None);
            }
            else
               ATLASSERT(false);

            // Attach connector to the sockets 
            auto connector = std::dynamic_pointer_cast<WBFL::DManip::iConnector>(line);
            auto startPlug = connector->GetStartPlug();
            auto endPlug = connector->GetEndPlug();

            // connect the line to the sockets
            s1->Connect(startPlug);
            s2->Connect(endPlug);

            pDL->AddDisplayObject(line);
         }
      }
   }
}

void CTogaGirderModelElevationView::BuildLine(std::shared_ptr<WBFL::DManip::iDisplayList> pDL, Float64 offset, WBFL::Geometry::Point2d fromPoint, WBFL::Geometry::Point2d toPoint, COLORREF color)
{
   fromPoint.Offset(offset, 0);
   toPoint.Offset(offset, 0);

   // put points at locations and make them sockets
   auto from_rep = WBFL::DManip::PointDisplayObject::Create(m_CurrID++);
   from_rep->SetPosition(fromPoint,false,false);
   auto from_connectable = std::dynamic_pointer_cast<WBFL::DManip::iConnectable>(from_rep);
   auto from_socket = from_connectable->AddSocket(0,fromPoint);
   from_rep->Visible(false);
   pDL->AddDisplayObject(from_rep);

   auto to_rep = WBFL::DManip::PointDisplayObject::Create(m_CurrID++);
   to_rep->SetPosition(toPoint,false,false);
   auto to_connectable = std::dynamic_pointer_cast<WBFL::DManip::iConnectable>(to_rep);
   auto to_socket = to_connectable->AddSocket(0,toPoint);
   to_rep->Visible(false);
   pDL->AddDisplayObject(to_rep);

   // Create the dimension line object
   auto line = WBFL::DManip::LineDisplayObject::Create(m_CurrID++);

   // color
   auto pStrategy = line->GetDrawLineStrategy();

   // dangerous cast here
   auto pSimple = std::dynamic_pointer_cast<WBFL::DManip::SimpleDrawLineStrategy>(pStrategy);
   if (pSimple)
   {
      pSimple->SetColor(color);
   }
   else
      ATLASSERT(false);

   // Attach connector to the sockets 
   auto connector = std::dynamic_pointer_cast<WBFL::DManip::iConnector>(line);
   auto startPlug = connector->GetStartPlug();
   auto endPlug = connector->GetEndPlug();

   // connect the line to the sockets
   from_socket->Connect(startPlug);
   to_socket->Connect(endPlug);

   pDL->AddDisplayObject(line);
}

void CTogaGirderModelElevationView::BuildDebondTick(std::shared_ptr<WBFL::DManip::iDisplayList> pDL, Float64 offset, WBFL::Geometry::Point2d tickPoint,COLORREF color)
{
   tickPoint.Offset(offset, 0);

   // put points at locations and make them sockets
   auto doPnt = WBFL::DManip::PointDisplayObject::Create(m_CurrID++);
   doPnt->SetPosition(tickPoint,false,false);

   auto strategy = WBFL::DManip::SimpleDrawPointStrategy::Create();
   strategy->SetColor(color);
   strategy->SetPointType(WBFL::DManip::PointType::Circle);

   doPnt->SetDrawingStrategy(strategy);

   pDL->AddDisplayObject(doPnt);
}

std::shared_ptr<WBFL::DManip::DimensionLine> CTogaGirderModelElevationView::BuildDimensionLine(std::shared_ptr<WBFL::DManip::iDisplayList> pDL, const WBFL::Geometry::Point2d& fromPoint, const WBFL::Geometry::Point2d& toPoint,Float64 dimension)
{
   // put points at locations and make them sockets
   auto from_rep = WBFL::DManip::PointDisplayObject::Create(m_CurrID++);
   from_rep->SetPosition(fromPoint,false,false);
   auto from_connectable = std::dynamic_pointer_cast<WBFL::DManip::iConnectable>(from_rep);
   auto from_socket = from_connectable->AddSocket(0,fromPoint);
   from_rep->Visible(false);
   pDL->AddDisplayObject(from_rep);

   auto to_rep = WBFL::DManip::PointDisplayObject::Create(m_CurrID++);
   to_rep->SetPosition(toPoint,false,false);
   auto to_connectable = std::dynamic_pointer_cast<WBFL::DManip::iConnectable>(to_rep);
   auto to_socket = to_connectable->AddSocket(0,toPoint);
   to_rep->Visible(false);
   pDL->AddDisplayObject(to_rep);

   // Create the dimension line object
   auto dimLine = WBFL::DManip::DimensionLine::Create(m_CurrID++);

   dimLine->SetArrowHeadStyle(WBFL::DManip::ArrowHeadStyleType::Filled);

   // Attach connector (the dimension line) to the sockets 
   auto connector = std::dynamic_pointer_cast<WBFL::DManip::iConnector>(dimLine);
   auto startPlug = connector->GetStartPlug();
   auto endPlug = connector->GetEndPlug();

   from_socket->Connect(startPlug);
   to_socket->Connect(endPlug);

   // Create the text block and attach it to the dimension line
   auto textBlock = WBFL::DManip::TextBlock::Create();

   // Format the dimension text
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   CString strDimension = FormatDimension(dimension,pDisplayUnits->GetSpanLengthUnit());

   textBlock->SetText(strDimension);
   textBlock->SetBkMode(TRANSPARENT);

   dimLine->SetTextBlock(textBlock);

   pDL->AddDisplayObject(dimLine);

   return dimLine;
}

void CTogaGirderModelElevationView::OnDestroy() 
{
   // free up our connectable objects so they don't leak
   IndexType dlcnt = m_pDispMgr->GetDisplayListCount();
   for (IndexType idl=0; idl<dlcnt; idl++)
   {
      auto dlist = m_pDispMgr->GetDisplayList(idl);

      IndexType docnt = dlist->GetDisplayObjectCount();
      for (IndexType ido=0; ido<docnt; ido++)
      {
         auto pdo = dlist->GetDisplayObject(ido);

         auto connectable = std::dynamic_pointer_cast<WBFL::DManip::iConnectable>(pdo);

         if (connectable)
         {
            connectable->RemoveAllSockets();
         }
      }
   }


	CDisplayView::OnDestroy();
	
}

void CTogaGirderModelElevationView::OnDraw(CDC* pDC)
{
   if ( m_bUpdateError )
   {
      CString msg;
      msg.Format(_T("The following error occurred while updating the views.\n\n%s."),m_ErrorMsg.c_str());
      MultiLineTextOut(pDC,0,0,msg);
      return;
   }

   SpanIndexType spanIdx, gdrIdx;
   m_pFrame->GetSpanAndGirderSelection(&spanIdx,&gdrIdx);

   if (  spanIdx != ALL_SPANS && gdrIdx != ALL_GIRDERS )
   {
      CDisplayView::OnDraw(pDC);
   }
   else
   {
      CString msg(_T("Select a girder to display"));
      MultiLineTextOut(pDC,0,0,msg);
   }
}

BOOL CTogaGirderModelElevationView::OnMouseWheel(UINT nFlags,short zDelta,CPoint pt)
{
   auto selObjs = m_pDispMgr->GetSelectedObjects();

   if ( selObjs.size() == 0 || selObjs.front()->GetID() != SECTION_CUT_ID )
      return FALSE;

   if ( 0 < zDelta )
      m_pFrame->CutAtPrev();
   else
      m_pFrame->CutAtNext();

   return TRUE;
}
