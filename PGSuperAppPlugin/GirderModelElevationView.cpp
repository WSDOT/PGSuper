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

// GirderModelElevationView.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "PGSuperApp.h"
#include "PGSuperDocBase.h"
#include "PGSuperDoc.h"
#include "PGSpliceDoc.h"
#include "PGSuperUnits.h"
#include "PGSuperColors.h"
#include "GirderModelElevationView.h"
#include "GirderModelChildFrame.h"
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <IFace\DrawBridgeSettings.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\EditByUI.h>
#include <IFace\Intervals.h>
#include <IFace\DocumentType.h>
#include <IFace\AnalysisResults.h>

#include <PgsExt\BridgeDescription2.h>
#include <PgsExt\ClosureJointData.h>

#include "SupportDrawStrategyImpl.h"
#include "TemporarySupportDrawStrategyImpl.h"
#include "SectionCutDrawStrategy.h"
#include "PointLoadDrawStrategyImpl.h"
#include "DistributedLoadDrawStrategyImpl.h"
#include "MomentLoadDrawStrategyImpl.h"
#include "SectionCutDisplayImpl.h"
#include "GMDisplayMgrEventsImpl.h"
#include "GevEditLoad.h"
#include "GirderDropSite.h"
#include "GirderDisplayObjectEvents.h"
#include "DisplayObjectFactory.h"

#include "PGSuperColors.h"

#include <sstream>

#include <WBFLGenericBridgeTools.h>
#include <WBFLGeometry/GeomHelpers.h>

// NOTE: See GirderModelChildFrame.cpp for information about the coordinate systems used in this view

// display list constants
#define GDR_LIST          1
#define TENDON_LIST       2
#define CP_LIST           3
#define DEBOND_LIST       4
#define STRAND_LIST       5 
#define CG_LIST           6
#define STRAND_CG_LIST    7
#define SUPPORT_LIST      8
#define DIMLINE_LIST      9
#define SECT_CUT_LIST    10
#define REBAR_LIST       11
#define LOAD_LIST        12
#define STIRRUP_LIST     13
#define DROP_TARGET_LIST 14

// display object ID
#define SECTION_CUT_ID   100


static CString GetLoadGroupNameForUserLoad(UserLoads::LoadCase lc)
{
   switch(lc)
   {
      case UserLoads::DC:
         return _T("DC");

      case UserLoads::DW:
         return _T("DW");

      case UserLoads::LL_IM:
         return _T("LL_IM");

      default:
         ATLASSERT(false); // SHOULD NEVER GET HERE
   }

   return _T("Error");
}

static COLORREF GetLoadGroupColor(UserLoads::LoadCase lc)
{
   switch(lc)
   {
      case UserLoads::DC:
         return DC_COLOR;
      break;

      case UserLoads::DW:
         return DW_COLOR;
      break;

      case UserLoads::LL_IM:
         return LLIM_COLOR;
      break;

         default:
         ATLASSERT(false); // SHOULD NEVER GET HERE
   }

   return  RED1;
}

static void CreateLegendEntry(UserLoads::LoadCase lc, std::shared_ptr<WBFL::DManip::iLegendDisplayObject> legend)
{
   COLORREF color = GetLoadGroupColor(lc);
   CString name = GetLoadGroupNameForUserLoad(lc);

   auto legend_entry = WBFL::DManip::LegendEntry::Create();

   // add entry to legend
   legend_entry->SetName(name);
   legend_entry->SetColor(color);
   legend_entry->DrawLine(false);
   legend->AddEntry(legend_entry);
}

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
// CGirderModelElevationView

IMPLEMENT_DYNCREATE(CGirderModelElevationView, CDisplayView)

CGirderModelElevationView::CGirderModelElevationView():
m_bOnIntialUpdateComplete(false),
m_DisplayObjectID(0),
m_DoBlockUpdate(false),
m_GirderKey(0,0),
m_pFrame(nullptr)
{
   m_bUpdateError = false;
}

CGirderModelElevationView::~CGirderModelElevationView()
{
}


BEGIN_MESSAGE_MAP(CGirderModelElevationView, CDisplayView)
	//{{AFX_MSG_MAP(CGirderModelElevationView)
	ON_WM_CREATE()
	ON_COMMAND(ID_USER_CUT, OnUserCut)
	ON_WM_SIZE()
	ON_COMMAND(ID_EDIT_PRESTRESSING, OnEditPrestressing)
	ON_COMMAND(ID_VIEWSETTINGS, OnViewSettings)
	ON_COMMAND(ID_EDIT_STIRRUPS, OnEditStirrups)
	ON_COMMAND(ID_EDIT_LOAD, OnGevCtxEditLoad)
	ON_COMMAND(ID_DELETE_LOAD, OnGevCtxDeleteLoad)
	ON_WM_MOUSEMOVE()
	ON_WM_TIMER()
	ON_WM_DESTROY()
   ON_WM_MOUSEWHEEL()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CGirderModelElevationView drawing
void CGirderModelElevationView::OnInitialUpdate() 
{
   ATLASSERT(m_bOnIntialUpdateComplete == false);

   HRESULT hr = S_OK;

	CDisplayView::OnInitialUpdate();
   EnableToolTips();

   // Setup the local display object factory
   m_pDispMgr->EnableLBtnSelect(true);
   m_pDispMgr->EnableRBtnSelect(true);
   m_pDispMgr->SetSelectionLineColor(SELECTED_OBJECT_LINE_COLOR);
   m_pDispMgr->SetSelectionFillColor(SELECTED_OBJECT_FILL_COLOR);

   CPGSDocBase* pDoc = (CPGSDocBase*)GetDocument();
   auto doFactory = std::make_shared<CDisplayObjectFactory>(pDoc);
   m_pDispMgr->AddDisplayObjectFactory(doFactory);

   // set up default event handler for canvas
   auto events = CGMDisplayMgrEventsImpl::Create(pDoc, m_pFrame, this, true);
   m_pDispMgr->RegisterEventSink(events);

   // Create display lists
   m_pDispMgr->CreateDisplayList(SECT_CUT_LIST);
   m_pDispMgr->CreateDisplayList(DIMLINE_LIST);
   m_pDispMgr->CreateDisplayList(LOAD_LIST);
   m_pDispMgr->CreateDisplayList(SUPPORT_LIST);
   m_pDispMgr->CreateDisplayList(DROP_TARGET_LIST);
   m_pDispMgr->CreateDisplayList(STRAND_CG_LIST);
   m_pDispMgr->CreateDisplayList(CG_LIST);
   m_pDispMgr->CreateDisplayList(DEBOND_LIST);
   m_pDispMgr->CreateDisplayList(STRAND_LIST);
   m_pDispMgr->CreateDisplayList(REBAR_LIST);
   m_pDispMgr->CreateDisplayList(TENDON_LIST);
   m_pDispMgr->CreateDisplayList(STIRRUP_LIST);
   m_pDispMgr->CreateDisplayList(CP_LIST);
   m_pDispMgr->CreateDisplayList(GDR_LIST);

   // build display objects
   // set up a valid dc first
   CDManipClientDC dc2(this);

   // set girder
   DidGirderSelectionChange();

   UpdateDisplayObjects();

   ScaleToFit();

   m_bOnIntialUpdateComplete = true;
}

void CGirderModelElevationView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
   // block updates so this function is not reentrant
   if (m_DoBlockUpdate)
   {
      return;
   }

   SimpleMutex mutex(m_DoBlockUpdate);

   m_bUpdateError = false;

   CDisplayView::OnUpdate(pSender,lHint,pHint);

   // Let our frame deal with updates as well
   m_pFrame->OnUpdate(this,lHint,pHint);

   // do update
   m_DisplayObjectID = 0;

   if ( !m_bOnIntialUpdateComplete )
   {
      // We don't want to build display objects until we are all the way through OnInitialUpdate
      return;
   }
   else if ( lHint == 0 ||
        lHint == HINT_GIRDERVIEWSETTINGSCHANGED ||
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

	Invalidate(TRUE);

   m_bOnIntialUpdateComplete = true;
}

void CGirderModelElevationView::UpdateDisplayObjects()
{
   // NOTE: Each girder in this view is in Girder Path Coordinates

   // clean out all the display objects
   m_pDispMgr->ClearDisplayObjects();

   CPGSDocBase* pDoc = (CPGSDocBase*)GetDocument();

   EventIndexType eventIdx = m_pFrame->GetEvent();

   // Grab hold of the broker so we can pass it as a parameter
   CComPtr<IBroker> pBroker;
   pDoc->GetBroker(&pBroker);

   UINT settings = pDoc->GetGirderEditorSettings();
   
   CGirderKey girderKey(GetGirderKey());

   BuildSupportDisplayObjects(     pDoc, pBroker, girderKey, eventIdx);
   BuildDropTargetDisplayObjects(  pDoc, pBroker, girderKey, eventIdx);
   BuildSegmentDisplayObjects(     pDoc, pBroker, girderKey, eventIdx);
   BuildClosureJointDisplayObjects( pDoc, pBroker, girderKey, eventIdx);

   if (settings & IDG_EV_GIRDER_CG)
   {
      BuildSegmentCGDisplayObjects(pDoc, pBroker, girderKey, eventIdx);
   }

   if (settings & IDG_EV_SHOW_STRANDS)
   {
      BuildStrandDisplayObjects(pDoc, pBroker, girderKey, eventIdx);
      BuildTendonDisplayObjects(pDoc, pBroker, girderKey, eventIdx);
   }

   if (settings & IDG_EV_SHOW_PS_CG)
   {
      BuildStrandCGDisplayObjects(pDoc, pBroker, girderKey, eventIdx);
   }

   if (settings & IDG_EV_SHOW_LONG_REINF)
   {
      BuildRebarDisplayObjects(pDoc, pBroker, girderKey, eventIdx);
   }

   if (settings & IDG_EV_SHOW_STIRRUPS)
   {
      BuildStirrupDisplayObjects(pDoc, pBroker, girderKey, eventIdx);
   }

   bool cases_exist[3] = {false,false,false};
   if (settings & IDG_EV_SHOW_LOADS)
   {
      BuildPointLoadDisplayObjects(      pDoc, pBroker, girderKey, eventIdx, cases_exist);
      BuildDistributedLoadDisplayObjects(pDoc, pBroker, girderKey, eventIdx, cases_exist);
      BuildMomentLoadDisplayObjects(pDoc, pBroker, girderKey, eventIdx, cases_exist);
   }

   if (settings & IDG_EV_SHOW_DIMENSIONS)
   {
      BuildDimensionDisplayObjects(pDoc, pBroker, girderKey, eventIdx);
   }

   BuildSectionCutDisplayObjects(pDoc, pBroker, girderKey, eventIdx);

   // Legend must be displayed last so we can place it relative to bounding box
   if (settings & IDG_EV_SHOW_LEGEND && settings & IDG_EV_SHOW_LOADS)
   {
      BuildLegendDisplayObjects(pDoc, pBroker, girderKey, eventIdx, cases_exist);
   }

   auto mode = (settings & IDG_EV_DRAW_ISOTROPIC) ? WBFL::DManip::MapMode::Isotropic : WBFL::DManip::MapMode::Anisotropic;
   CDisplayView::SetMappingMode(mode);
}

void CGirderModelElevationView::DoPrint(CDC* pDC, CPrintInfo* pInfo)
{
   OnBeginPrinting(pDC, pInfo);
   OnPrepareDC(pDC);
   ScaleToFit();
   OnDraw(pDC);
   OnEndPrinting(pDC, pInfo);
}

/////////////////////////////////////////////////////////////////////////////
// CGirderModelElevationView diagnostics

#ifdef _DEBUG
void CGirderModelElevationView::AssertValid() const
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
	CDisplayView::AssertValid();
}

void CGirderModelElevationView::Dump(CDumpContext& dc) const
{
	CDisplayView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CGirderModelElevationView message handlers

int CGirderModelElevationView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
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

DROPEFFECT CGirderModelElevationView::CanDrop(COleDataObject* pDataObject,DWORD dwKeyState, const WBFL::Geometry::Point2d& point)
{
   // This override has to determine if the thing being dragged over it can
   // be dropped. In order to do that, it must unpackage the OleDataObject.
   //
   // The stuff in the data object is just from the display object. The display
   // objects need to be updated so that the client can attach an object to it
   // that knows how to package up domain specific information. At the same
   // time, this view needs to be able to get some domain specific hint 
   // as to the type of data that is going to be dropped.

   if ( pDataObject->IsDataAvailable(CSectionCutDisplayImpl::ms_Format) )
   {
      // need to peek at our object first and make sure it's coming from the local process
      // this is ugly because it breaks encapsulation of CSectionCutDisplayImpl
      auto source = WBFL::DManip::DragDataSource::Create();
      source->SetDataObject(pDataObject);
      source->PrepareFormat(CSectionCutDisplayImpl::ms_Format);

      CWinThread* thread = ::AfxGetThread( );
      DWORD threadid = thread->m_nThreadID;

      DWORD threadl;
      // know (by voodoo) that the first member of this data source is the thread id
      source->Read(CSectionCutDisplayImpl::ms_Format,&threadl,sizeof(DWORD));

      if (threadl == threadid)
      {
        return DROPEFFECT_MOVE;
      }
   }
   else
   {
      if (m_Legend)
      {
         auto drag = std::dynamic_pointer_cast<WBFL::DManip::iDraggable>(m_Legend);
         UINT format = drag->Format();
         if ( pDataObject->IsDataAvailable(format) )
         {
            return DROPEFFECT_MOVE;
         }
      }
   }

   return DROPEFFECT_NONE;
}

pgsPointOfInterest CGirderModelElevationView::GetCutLocation()
{
   auto pDL = m_pDispMgr->FindDisplayList(SECT_CUT_LIST);
   ATLASSERT(pDL);

   auto dispObj = pDL->FindDisplayObject(SECTION_CUT_ID);

   if ( dispObj == nullptr )
   {
      return pgsPointOfInterest();
   }

   auto sink = dispObj->GetEventSink();

   auto point_disp = std::dynamic_pointer_cast<WBFL::DManip::iPointDisplayObject>(dispObj);
   auto sectionCutStrategy = std::dynamic_pointer_cast<iSectionCutDrawStrategy>(sink);
   return sectionCutStrategy->GetCutPOI(m_pFrame->GetCurrentCutLocation());
}

CGirderModelChildFrame* CGirderModelElevationView::GetFrame()
{
   return m_pFrame;
}

void CGirderModelElevationView::OnUserCut() 
{
	m_pFrame->ShowCutDlg();
}

void CGirderModelElevationView::OnSize(UINT nType, int cx, int cy) 
{
	CDisplayView::OnSize(nType, cx, cy);

   if (m_bOnIntialUpdateComplete)
   {
      CRect rect;
      this->GetClientRect(&rect);
      rect.DeflateRect(5,5,5,5);

      CSize size = rect.Size();
      size.cx = Max(0L,size.cx);
      size.cy = Max(0L,size.cy);

      SetLogicalViewRect(MM_TEXT,rect);

      SetScrollSizes(MM_TEXT,size,CScrollView::sizeDefault,CScrollView::sizeDefault);

      ScaleToFit();
   }
}

void CGirderModelElevationView::OnEditPrestressing() 
{
   int page = EGS_PRESTRESSING;
   if ( GetDocument()->IsKindOf(RUNTIME_CLASS(CPGSuperDoc)) )
   {
      page = EGD_PRESTRESSING;
   }

   pgsPointOfInterest poi = GetCutLocation();
   ((CPGSDocBase*)GetDocument())->EditGirderSegmentDescription(poi.GetSegmentKey(),page);
}

void CGirderModelElevationView::OnEditStirrups() 
{
   int page = EGS_STIRRUPS;
   if ( GetDocument()->IsKindOf(RUNTIME_CLASS(CPGSuperDoc)) )
   {
      page = EGD_STIRRUPS;
   }

   pgsPointOfInterest poi = GetCutLocation();
   ((CPGSDocBase*)GetDocument())->EditGirderSegmentDescription(poi.GetSegmentKey(),page);
}

void CGirderModelElevationView::OnViewSettings() 
{
	((CPGSDocBase*)GetDocument())->EditGirderViewSettings(VS_GIRDER_ELEVATION);
}

void CGirderModelElevationView::CreateSegmentEndSupportDisplayObject(Float64 groupOffset,const CPrecastSegmentData* pSegment,pgsTypes::MemberEndType endType,EventIndexType eventIdx,const CTimelineManager* pTimelineMgr,std::shared_ptr<WBFL::DManip::iDisplayList> pDL)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   const CSegmentKey& segmentKey(pSegment->GetSegmentKey());

   const CClosureJointData* pClosure = (endType == pgsTypes::metStart ? pSegment->GetClosureJoint(pgsTypes::metStart) : pSegment->GetClosureJoint(pgsTypes::metEnd));
   const CPierData2* pPier = nullptr;
   const CTemporarySupportData* pTS = nullptr;

   if ( pClosure )
   {
      pPier = pClosure->GetPier();
      pTS   = pClosure->GetTemporarySupport();
   }
   else
   {
      const CSpanData2* pSpan = pSegment->GetSpan(endType);
      pPier = (endType == pgsTypes::metStart ? pSpan->GetPrevPier() : pSpan->GetNextPier());
   }

   bool* pbIsPier = nullptr;
   Float64 pierLocation;
   Float64 sectionHeight;
   IDType ID;
   if ( pPier )
   {
      PierIndexType pierIdx = pPier->GetIndex();
      PierIDType pierID = pPier->GetID();
      EventIndexType erectionEventIdx = pTimelineMgr->GetPierErectionEventIndex(pierID);
      if ( eventIdx < erectionEventIdx )
      {
         return; // pier is not erected in this event
      }

      pbIsPier = new bool;
      *pbIsPier = true;

      GET_IFACE2(pBroker,IBridge,pBridge);
      VERIFY(pBridge->GetPierLocation(segmentKey,pierIdx,&pierLocation)); // pier location is in girder path coordinates
      ID = (IDType)pierIdx;

      // adjust location so that it is at the CL Bearing and not the pier reference line
      if ( pPier->IsAbutment() )
      {
         if ( pPier->GetPrevSpan() == 0 )
         {
            Float64 brgOffset = pBridge->GetSegmentStartBearingOffset(segmentKey);
            pierLocation += brgOffset;
         }
         else
         {
            Float64 brgOffset = pBridge->GetSegmentEndBearingOffset(segmentKey);
            pierLocation -= brgOffset;
         }
      }
      else if ( pPier->IsBoundaryPier() )
      {
         pgsTypes::BoundaryConditionType connectionType = pPier->GetBoundaryConditionType();
         if ( connectionType != pgsTypes::bctContinuousAfterDeck && 
              connectionType != pgsTypes::bctContinuousBeforeDeck &&
              connectionType != pgsTypes::bctIntegralAfterDeck &&
              connectionType != pgsTypes::bctIntegralBeforeDeck 
            )
         {
            if ( endType == pgsTypes::metStart )
            {
               if ( connectionType == pgsTypes::bctHinge ||
                    connectionType == pgsTypes::bctRoller ||
                    connectionType == pgsTypes::bctIntegralAfterDeckHingeAhead ||
                    connectionType == pgsTypes::bctIntegralBeforeDeckHingeAhead)
               {
                  Float64 brgOffset = pBridge->GetSegmentStartBearingOffset(segmentKey);
                  pierLocation += brgOffset;
               }
            }
            else
            {
               if (connectionType == pgsTypes::bctHinge ||
                   connectionType == pgsTypes::bctRoller ||
                   connectionType == pgsTypes::bctIntegralAfterDeckHingeBack ||
                   connectionType == pgsTypes::bctIntegralBeforeDeckHingeBack)
               {
                  Float64 brgOffset = pBridge->GetSegmentEndBearingOffset(segmentKey);
                  pierLocation -= brgOffset;
               }
            }
         }
      }

      GET_IFACE2(pBroker,ISectionProperties,pSectProp);
      sectionHeight = pSectProp->GetSegmentHeightAtPier(segmentKey,pierIdx);

      GET_IFACE2(pBroker, IIntervals, pIntervals);
      IntervalIndexType intervalIdx = pIntervals->GetInterval(eventIdx);

      GET_IFACE2(pBroker, IPointOfInterest, pPoi);
      PoiAttributeType attribute = (endType == pgsTypes::metStart ? POI_0L : POI_10L);
      PoiList vPoi;
      pPoi->GetPointsOfInterest(segmentKey, POI_ERECTED_SEGMENT | attribute, &vPoi);
      ATLASSERT(vPoi.size() == 1);
      const pgsPointOfInterest& poiCLBrg(vPoi.front());

      GET_IFACE2(pBroker, IGirder, pGirder);
      Float64 precamber = pGirder->GetPrecamber(poiCLBrg);
      sectionHeight -= precamber;
   }
   else
   {
      ATLASSERT(pTS != nullptr);

      EventIndexType erectionEventIdx, removalEventIdx;
      ID = pTS->GetID();
      SupportIndexType tsIdx = pTS->GetIndex();

      pTimelineMgr->GetTempSupportEvents(ID,&erectionEventIdx,&removalEventIdx);
      if ( eventIdx < erectionEventIdx || removalEventIdx <= eventIdx )
      {
         return;
      }

      pbIsPier = new bool;
      *pbIsPier = false;

      GET_IFACE2(pBroker,IBridge,pBridge);
      pierLocation = pBridge->GetTemporarySupportLocation(tsIdx,segmentKey.girderIndex);

      GET_IFACE2(pBroker,IPointOfInterest,pPoi);
      GET_IFACE2(pBroker,IIntervals,pIntervals);
      IntervalIndexType intervalIdx = pIntervals->GetInterval(eventIdx);
      PoiAttributeType poiReference = (pIntervals->GetErectSegmentInterval(segmentKey) <= intervalIdx ? POI_ERECTED_SEGMENT : POI_RELEASED_SEGMENT);
      PoiAttributeType attribute = (endType == pgsTypes::metStart ? POI_0L : POI_10L);
      PoiList vPoi;
      pPoi->GetPointsOfInterest(segmentKey, poiReference | attribute, &vPoi);
      ATLASSERT(vPoi.size() == 1);
      const pgsPointOfInterest& poiCLBrg(vPoi.front());

      GET_IFACE2(pBroker,ISectionProperties,pSectProp);
      sectionHeight = pSectProp->GetHg(pIntervals->GetPrestressReleaseInterval(segmentKey),poiCLBrg);

      GET_IFACE2(pBroker, IGirder, pGirder);
      Float64 precamber = pGirder->GetPrecamber(poiCLBrg);
      sectionHeight -= precamber;
   }

   WBFL::Geometry::Point2d point(pierLocation + groupOffset,-sectionHeight);

   // create display object
   auto ptDispObj = WBFL::DManip::PointDisplayObject::Create();
   ptDispObj->SetItemData((void*)pbIsPier,true);

   // create drawing strategy
   std::shared_ptr<WBFL::DManip::iDrawPointStrategy> strategy;
   if ( pPier )
   {
      strategy = std::make_shared<CSupportDrawStrategyImpl>(pPier);
   }
   else
   {
      GET_IFACE2(pBroker,IGirder,pIGirder);
      Float64 temp, leftBrgOffset, rightBrgOffset;
      pIGirder->GetSegmentBearingOffset(segmentKey,&temp,&leftBrgOffset);
      pIGirder->GetSegmentBearingOffset(CSegmentKey(segmentKey.groupIndex,segmentKey.girderIndex,segmentKey.segmentIndex+1),&rightBrgOffset,&temp);
      strategy = std::make_shared<CTemporarySupportDrawStrategyImpl>(pTS->GetSupportType(),leftBrgOffset,rightBrgOffset);
   }

   ptDispObj->SetDrawingStrategy(strategy);

   ptDispObj->SetPosition(point,false,false);
   ptDispObj->SetID( ID );

   pDL->AddDisplayObject(ptDispObj);
}

void CGirderModelElevationView::CreateIntermediatePierDisplayObject(Float64 groupOffset,const CPrecastSegmentData* pSegment,EventIndexType eventIdx,const CTimelineManager* pTimelineMgr,std::shared_ptr<WBFL::DManip::iDisplayList> pDL)
{
   const CSpanData2* pStartSpan = pSegment->GetSpan(pgsTypes::metStart);
   const CSpanData2* pEndSpan   = pSegment->GetSpan(pgsTypes::metEnd);

   if ( pStartSpan == pEndSpan )
   {
      return; // no intermediate pier
   }

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,ISectionProperties,pSectProp);
   GET_IFACE2(pBroker,IBridge,pBridge);

   const CSpanData2* pSpan = pStartSpan;
   bool bDone = false;
   while ( !bDone )
   {
      const CPierData2* pPier = pSpan->GetNextPier();

      PierIDType pierID = pPier->GetID();
      PierIndexType pierIdx = pPier->GetIndex();

      EventIndexType erectionEventIdx = pTimelineMgr->GetPierErectionEventIndex(pierID);
      if ( erectionEventIdx <= eventIdx )
      {
         CSegmentKey segmentKey(pSpan->GetBridgeDescription()->GetGirderGroup(pSpan)->GetIndex(),
                                pSegment->GetGirder()->GetIndex(),
                                pSegment->GetIndex());

         Float64 sectionHeight = pSectProp->GetSegmentHeightAtPier(segmentKey,pierIdx);
         Float64 pierLocation;
         VERIFY( pBridge->GetPierLocation(segmentKey,pierIdx,&pierLocation) );

         WBFL::Geometry::Point2d point(pierLocation + groupOffset,-sectionHeight);

         // create display object
         auto ptDispObj = WBFL::DManip::PointDisplayObject::Create();

         bool* pbIsPier = new bool;
         *pbIsPier = true;
         ptDispObj->SetItemData((void*)pbIsPier,true);

         // create drawing strategy
         auto pDrawStrategy = std::make_shared<CSupportDrawStrategyImpl>(pPier);
         ptDispObj->SetDrawingStrategy(pDrawStrategy);

         ptDispObj->SetPosition(point,false,false);
         ptDispObj->SetID( pierIdx );

         pDL->AddDisplayObject(ptDispObj);
      }

      pSpan = pSpan->GetNextPier()->GetNextSpan();
      if ( pSpan == pEndSpan )
      {
         bDone = true;
      }
   }
}

void CGirderModelElevationView::CreateIntermediateTemporarySupportDisplayObject(Float64 groupOffset,const CPrecastSegmentData* pSegment,EventIndexType eventIdx,const CTimelineManager* pTimelineMgr,std::shared_ptr<WBFL::DManip::iDisplayList> pDL)
{
   const CSpanData2* pStartSpan = pSegment->GetSpan(pgsTypes::metStart);
   const CSpanData2* pEndSpan   = pSegment->GetSpan(pgsTypes::metEnd);

   const CSpanData2* pSpan = pStartSpan;
   std::vector<const CTemporarySupportData*> tempSupports(pSpan->GetTemporarySupports());
   while ( pSpan != pEndSpan )
   {
      pSpan = pSpan->GetNextPier()->GetNextSpan();
      ATLASSERT(pSpan != nullptr);
      std::vector<const CTemporarySupportData*> endTempSupports(pSpan->GetTemporarySupports());
      tempSupports.insert(tempSupports.end(),endTempSupports.begin(),endTempSupports.end());
   }

   if ( tempSupports.size() == 0 )
   {
      return; // no temporary supports
   }

   Float64 segment_start_station, segment_end_station;
   pSegment->GetStations(&segment_start_station,&segment_end_station);
   std::vector<const CTemporarySupportData*>::iterator iter(tempSupports.begin());
   std::vector<const CTemporarySupportData*>::iterator iterEnd(tempSupports.end());
   for ( ; iter != iterEnd; iter++ )
   {
      const CTemporarySupportData* pTS = *iter;
      Float64 ts_station = pTS->GetStation();
      if ( ::IsEqual(segment_start_station,ts_station) || ::IsEqual(segment_end_station,ts_station) )
      {
         continue; // temporary support display objects already created when creating DO's at ends of segment
      }

      if ( ::InRange(segment_start_station,ts_station,segment_end_station) )
      {
         EventIndexType erectionEventIdx, removalEventIdx;
         pTimelineMgr->GetTempSupportEvents(pTS->GetID(),&erectionEventIdx,&removalEventIdx);
         if ( eventIdx < erectionEventIdx || removalEventIdx <= eventIdx )
         {
            continue; // temp support does not exist in this event
         }

         CComPtr<IBroker> pBroker;
         EAFGetBroker(&pBroker);
         GET_IFACE2(pBroker,ISectionProperties,pSectProp);
         GET_IFACE2(pBroker,IBridge,pBridge);

         CSegmentKey segmentKey(pStartSpan->GetBridgeDescription()->GetGirderGroup(pStartSpan)->GetIndex(),
                                pSegment->GetGirder()->GetIndex(),
                                pSegment->GetIndex());

         Float64 sectionHeight = pSectProp->GetSegmentHeightAtTemporarySupport(segmentKey,pTS->GetIndex());
         Float64 pierLocation = pBridge->GetTemporarySupportLocation(pTS->GetIndex(),segmentKey.girderIndex);

         WBFL::Geometry::Point2d point(pierLocation+groupOffset,-sectionHeight);

         // create display object
         auto ptDispObj = WBFL::DManip::PointDisplayObject::Create();

         // create drawing strategy
         auto pDrawStrategy = std::make_shared<CTemporarySupportDrawStrategyImpl>(pTS->GetSupportType(),sectionHeight/4,sectionHeight/4);
         ptDispObj->SetDrawingStrategy(pDrawStrategy);

         ptDispObj->SetPosition(point,false,false);
         ptDispObj->SetID( pTS->GetID() );

         pDL->AddDisplayObject(ptDispObj);
      }
   }
}

void CGirderModelElevationView::BuildSupportDisplayObjects(CPGSDocBase* pDoc, IBroker* pBroker,const CGirderKey& girderKey,EventIndexType eventIdx)
{
   auto pDL = m_pDispMgr->FindDisplayList(SUPPORT_LIST);
   ATLASSERT(pDL);
   pDL->Clear();

   GET_IFACE2(pBroker,IBridge,pBridge);


   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CTimelineManager* pTimelineMgr = pBridgeDesc->GetTimelineManager();

   GroupIndexType startGroupIdx = (girderKey.groupIndex == ALL_GROUPS ? 0 : girderKey.groupIndex);
   GroupIndexType endGroupIdx   = (girderKey.groupIndex == ALL_GROUPS ? pBridgeDesc->GetGirderGroupCount()-1 : startGroupIdx);

   PierIndexType startPierIdx = pBridge->GetGirderGroupStartPier(startGroupIdx);
   Float64 groupOffset = 0;
   GroupIndexType nGirdersThisGroup = pBridge->GetGirderCount(startGroupIdx);
   GirderIndexType thisGirderIdx = Min(girderKey.girderIndex,nGirdersThisGroup-1);
   VERIFY( pBridge->GetPierLocation(CGirderKey(startGroupIdx,thisGirderIdx),startPierIdx,&groupOffset) );

   for ( GroupIndexType grpIdx = startGroupIdx; grpIdx <= endGroupIdx; grpIdx++ )
   {
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(grpIdx);
      GirderIndexType nGirders = pGroup->GetGirderCount();
      GirderIndexType gdrIdx = Min(girderKey.girderIndex,nGirders-1);

      CGirderKey thisGirderKey(grpIdx,gdrIdx);

      const CSplicedGirderData* pGirder = pGroup->GetGirder(thisGirderKey.girderIndex);
      SegmentIndexType nSegments = pGirder->GetSegmentCount();
      for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
      {
         const CPrecastSegmentData* pSegment = pGirder->GetSegment(segIdx);

         if ( segIdx == 0 )
         {
            CreateSegmentEndSupportDisplayObject(groupOffset,pSegment,pgsTypes::metStart,eventIdx,pTimelineMgr,pDL);
         }

         CreateSegmentEndSupportDisplayObject(groupOffset,pSegment,pgsTypes::metEnd,eventIdx,pTimelineMgr,pDL);
         CreateIntermediatePierDisplayObject(groupOffset,pSegment,eventIdx,pTimelineMgr,pDL);
         CreateIntermediateTemporarySupportDisplayObject(groupOffset,pSegment,eventIdx,pTimelineMgr,pDL);
      } // next segment

      Float64 girderLength = pBridge->GetGirderLayoutLength(thisGirderKey);
      groupOffset += girderLength;
   } // group loop
}

void CGirderModelElevationView::BuildDropTargetDisplayObjects(CPGSDocBase* pDoc, IBroker* pBroker, const CGirderKey& girderKey, EventIndexType eventIdx)
{
   auto pSupportDisplayList = m_pDispMgr->FindDisplayList(SUPPORT_LIST);
   ATLASSERT(pSupportDisplayList);

   auto pDL = m_pDispMgr->FindDisplayList(DROP_TARGET_LIST);
   ATLASSERT(pDL);
   pDL->Clear();

   COLORREF color = HOTPINK1;

   PierIndexType startPierIdx, endPierIdx;
   std::optional<WBFL::Geometry::Point2d> p1, p2;
   IndexType nItems = pSupportDisplayList->GetDisplayObjectCount();
   for ( IndexType idx = 0; idx < nItems; idx++ )
   {
      auto pDO = pSupportDisplayList->GetDisplayObject(idx);

      auto doPoint = std::dynamic_pointer_cast<WBFL::DManip::iPointDisplayObject>(pDO);
      ATLASSERT(doPoint != nullptr);

      bool* pbIsPier;
      pDO->GetItemData((void**)&pbIsPier);

      if ( pbIsPier && *pbIsPier )
      {
         if ( p1.has_value() )
         {
            auto p = doPoint->GetPosition();
            p2 = WBFL::Geometry::Point2d(p.X(), 0.0);

            endPierIdx = pDO->GetID();
         }
         else
         {
            auto p = doPoint->GetPosition();
            p1 = WBFL::Geometry::Point2d(p.X(), 0.0);

            startPierIdx = pDO->GetID();
         }

         if ( p1.has_value() && p2.has_value() && (startPierIdx < endPierIdx) )
         {
            auto line = BuildLine(pDL,p1.value(), p2.value(), color, 1);

            line->Visible(false);

            SpanIndexType spanIdx = (SpanIndexType)startPierIdx;

            // create a drop site for drag and drop loads
            auto dropSite = std::make_shared<CGirderDropSite>(pDoc, CSpanKey(spanIdx,girderKey.girderIndex), m_pFrame);
            line->RegisterDropSite(dropSite);

            p1 = p2;
            p2.reset();

            startPierIdx = endPierIdx;
         }
      }
   }
}

void CGirderModelElevationView::BuildSegmentDisplayObjects(CPGSDocBase* pDoc,IBroker* pBroker, const CGirderKey& girderKey,EventIndexType eventIdx)
{
   // get the display list and clear out any old display objects
   auto pDL = m_pDispMgr->FindDisplayList(GDR_LIST);
   ATLASSERT(pDL);
   pDL->Clear();

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CTimelineManager* pTimelineMgr = pBridgeDesc->GetTimelineManager();

   GroupIndexType startGroupIdx = (girderKey.groupIndex == ALL_GROUPS ? 0 : girderKey.groupIndex);
   GroupIndexType endGroupIdx   = (girderKey.groupIndex == ALL_GROUPS ? pBridgeDesc->GetGirderGroupCount()-1 : startGroupIdx);

   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2_NOCHECK(pBroker,IGirder,pIGirder); // doesn't always get used
   Float64 group_offset = 0;
  
   for ( GroupIndexType grpIdx = startGroupIdx; grpIdx <= endGroupIdx; grpIdx++ )
   {
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(grpIdx);
      GirderIndexType nGirders = pGroup->GetGirderCount();
      GirderIndexType gdrIdx = Min(girderKey.girderIndex,nGirders-1);
      CGirderKey thisGirderKey(grpIdx,gdrIdx);

      COLORREF segmentFillColor(gdrIdx == girderKey.girderIndex ? SEGMENT_FILL_COLOR : SEGMENT_FILL_GHOST_COLOR);

      Float64 running_segment_length = 0;

      const CSplicedGirderData* pGirder = pGroup->GetGirder(thisGirderKey.girderIndex);
      SegmentIndexType nSegments = pGirder->GetSegmentCount();
      for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
      {
         const CPrecastSegmentData* pSegment = pGirder->GetSegment(segIdx);
         SegmentIDType segID = pSegment->GetID();

         CSegmentKey segmentKey(thisGirderKey,segIdx);

         Float64 segment_layout_length = pBridge->GetSegmentLayoutLength(segmentKey);
         running_segment_length += segment_layout_length;

         // create display objects for each segment, but only if it is erected in this event
         // or has been erected in a previous event
         EventIndexType erectSegmentEventIdx = pTimelineMgr->GetSegmentErectionEventIndex(segID);
         if ( erectSegmentEventIdx <= eventIdx )
         {
            CComPtr<IShape> shape;
            pIGirder->GetSegmentProfile(segmentKey,false,&shape); // X values are in girder path coordinates

            CComQIPtr<IXYPosition> position(shape);
            if (!IsZero(group_offset))
            {
               // the group offset moves the segment to the right, based on all the groups
               // that have been previously drawn
               position->Offset(group_offset, 0);
            }

            // create the display object
            auto doPnt = WBFL::DManip::PointDisplayObject::Create();

            CComPtr<IPoint2d> pnt;
            position->get_LocatorPoint(lpTopLeft,&pnt);
            doPnt->SetPosition(geomUtil::GetPoint(pnt),false,false);

            // create the drawing strategy
            auto strategy = WBFL::DManip::ShapeDrawStrategy::Create();
            doPnt->SetDrawingStrategy(strategy);

            // configure the strategy
            strategy->SetShape(geomUtil::ConvertShape(shape));
            strategy->SetSolidLineColor(SEGMENT_BORDER_COLOR);
            strategy->SetSolidFillColor(segmentFillColor);
            strategy->SetVoidLineColor(VOID_BORDER_COLOR);
            strategy->SetVoidFillColor(GetSysColor(COLOR_WINDOW));
            strategy->Fill(true);

            CString strMsg(GetSegmentTooltip(pBroker,segmentKey));
            doPnt->SetMaxTipWidth(TOOLTIP_WIDTH);
            doPnt->SetTipDisplayTime(TOOLTIP_DURATION);
            doPnt->SetToolTipText(strMsg);

            // Register an event sink with the segment display object so that we can handle double clicks
            // on the segment differently then a general double click
            auto events = std::make_shared<CGirderElevationViewSegmentDisplayObjectEvents>(segmentKey, GetFrame());
            doPnt->RegisterEventSink(events);

            // put the display object in its display list
            pDL->AddDisplayObject(doPnt);
         }
      }

      group_offset += running_segment_length;
   } // group loop
}

void CGirderModelElevationView::BuildClosureJointDisplayObjects(CPGSDocBase* pDoc, IBroker* pBroker, const CGirderKey& girderKey,EventIndexType eventIdx)
{
   // get the display list and clear out any old display objects
   auto pDL = m_pDispMgr->FindDisplayList(CP_LIST);
   ATLASSERT(pDL);
   pDL->Clear();

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CTimelineManager* pTimelineMgr = pBridgeDesc->GetTimelineManager();

   GroupIndexType startGroupIdx = (girderKey.groupIndex == ALL_GROUPS ? 0 : girderKey.groupIndex);
   GroupIndexType endGroupIdx   = (girderKey.groupIndex == ALL_GROUPS ? pBridgeDesc->GetGirderGroupCount()-1 : startGroupIdx);

   GET_IFACE2(pBroker, IBridge, pBridge);
   GET_IFACE2_NOCHECK(pBroker, IGirder, pIGirder);

   Float64 group_offset = 0;

   for ( GroupIndexType grpIdx = startGroupIdx; grpIdx <= endGroupIdx; grpIdx++ )
   {
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(grpIdx);
      GirderIndexType nGirders = pGroup->GetGirderCount();
      GirderIndexType gdrIdx = Min(girderKey.girderIndex,nGirders-1);
      CGirderKey thisGirderKey(grpIdx,gdrIdx);

      const CSplicedGirderData* pGirder = pGroup->GetGirder(thisGirderKey.girderIndex);

      Float64 running_segment_length = 0;

      IndexType nClosures = pGirder->GetClosureJointCount();
      for ( IndexType closureIdx = 0; closureIdx < nClosures; closureIdx++ )
      {
         // segments and closures share an index
         const CPrecastSegmentData* pSegment = pGirder->GetSegment(closureIdx);
         const CSegmentKey& segmentKey = pSegment->GetSegmentKey();
         SegmentIDType segID = pSegment->GetID();

         CSegmentKey closureKey(segmentKey);

         Float64 segment_layout_length = pBridge->GetSegmentLayoutLength(segmentKey);
         running_segment_length += segment_layout_length;

         EventIndexType castClosureEventIdx = pTimelineMgr->GetCastClosureJointEventIndex(segID);
         if ( castClosureEventIdx <= eventIdx && castClosureEventIdx != INVALID_INDEX )
         {
            // add a display object if the closure exists in this event

            // get the profile shape of the closure joint
            // shape is located with x = 0 at start of girder
            CComPtr<IShape> shape;
            pIGirder->GetClosureJointProfile(closureKey,&shape);

            // offset the shape by the length of all the girder groups that
            // came before the current group
            CComQIPtr<IXYPosition> position(shape);
            position->Offset(group_offset,0);

            // create the display object
            auto doPnt = WBFL::DManip::PointDisplayObject::Create(segID);

            // create the drawing strategy
            auto strategy = WBFL::DManip::ShapeDrawStrategy::Create();
            doPnt->SetDrawingStrategy(strategy);

            // configure the strategy
            strategy->SetShape(geomUtil::ConvertShape(shape));
            strategy->SetSolidLineColor(CLOSURE_BORDER_COLOR);
            strategy->SetSolidFillColor(CLOSURE_FILL_COLOR);
            strategy->SetVoidLineColor(VOID_BORDER_COLOR);
            strategy->SetVoidFillColor(GetSysColor(COLOR_WINDOW));
            strategy->Fill(true);

            CString strMsg(GetClosureTooltip(pBroker,closureKey));
            doPnt->SetMaxTipWidth(TOOLTIP_WIDTH);
            doPnt->SetTipDisplayTime(TOOLTIP_DURATION);
            doPnt->SetToolTipText(strMsg);

            // put the display object in its display list
            pDL->AddDisplayObject(doPnt);
         }
      } // next closure

      Float64 last_segment_layout_length = pBridge->GetSegmentLayoutLength(CSegmentKey(thisGirderKey,nClosures));
      running_segment_length += last_segment_layout_length;
      group_offset += running_segment_length;
   } // group loop
}

void CGirderModelElevationView::BuildStrandDisplayObjects(CPGSDocBase* pDoc, IBroker* pBroker, const CGirderKey& girderKey,EventIndexType eventIdx)
{
   auto pDL = m_pDispMgr->FindDisplayList(STRAND_LIST);
   ATLASSERT(pDL);
   pDL->Clear();

   auto pDebondDL = m_pDispMgr->FindDisplayList(DEBOND_LIST);
   ATLASSERT(pDebondDL);
   pDebondDL->Clear();

   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeometry);

   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   GroupIndexType startGroupIdx = (girderKey.groupIndex == ALL_GROUPS ? 0 : girderKey.groupIndex);
   GroupIndexType endGroupIdx   = (girderKey.groupIndex == ALL_GROUPS ? pBridgeDesc->GetGirderGroupCount()-1 : startGroupIdx);

   const CTimelineManager* pTimelineMgr = pBridgeDesc->GetTimelineManager();

   Float64 group_offset = 0;

   GET_IFACE2(pBroker, IIntervals, pIntervals);
   IntervalIndexType intervalIdx = pIntervals->GetInterval(eventIdx);

   for (GroupIndexType grpIdx = startGroupIdx; grpIdx <= endGroupIdx; grpIdx++ )
   {
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(grpIdx);
      GirderIndexType nGirders = pGroup->GetGirderCount();
      GirderIndexType gdrIdx = Min(girderKey.girderIndex,nGirders-1);
      CGirderKey thisGirderKey(grpIdx,gdrIdx);

      const CSplicedGirderData* pGirder = pGroup->GetGirder(thisGirderKey.girderIndex);
      Float64 running_segment_length = 0; // sum of the segment lengths from segIdx = 0 to current segment
      SegmentIndexType nSegments = pGirder->GetSegmentCount();
      for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
      {
         CSegmentKey segmentKey(thisGirderKey,segIdx);

         Float64 segment_length        = pBridge->GetSegmentLength(segmentKey);
         Float64 start_brg_offset      = pBridge->GetSegmentStartBearingOffset(segmentKey);
         Float64 start_end_distance    = pBridge->GetSegmentStartEndDistance(segmentKey);
         Float64 start_offset          = start_brg_offset - start_end_distance;
         Float64 segment_layout_length = pBridge->GetSegmentLayoutLength(segmentKey);

         // running_segment_length goes to the CL of the closure... adjust the distance
         // so that it goes to the left face of the current segment
         running_segment_length += start_offset;

         const CPrecastSegmentData* pSegment = pGirder->GetSegment(segIdx);
         SegmentIDType segID = pSegment->GetID();
         EventIndexType erectSegmentEventIdx = pTimelineMgr->GetSegmentErectionEventIndex(segID);
         if ( erectSegmentEventIdx <= eventIdx )
         {
            for (int i = 0; i < 3; i++)
            {
               pgsTypes::StrandType strandType = (pgsTypes::StrandType)i;
               if (strandType == pgsTypes::Temporary)
               {
                  IntervalIndexType tsInstallationIntervalIdx = pIntervals->GetTemporaryStrandInstallationInterval(segmentKey);
                  IntervalIndexType tsRemovalIntervalIdx = pIntervals->GetTemporaryStrandRemovalInterval(segmentKey);
                  if ((intervalIdx < tsInstallationIntervalIdx && tsInstallationIntervalIdx!=INVALID_INDEX) || (tsRemovalIntervalIdx <= intervalIdx && tsRemovalIntervalIdx != INVALID_INDEX))
                  {
                     // if the current interval is before temporary strand installation or if it is after temporary strand removal
                     // don't draw the temporary strands
                     continue;
                  }
               }
               StrandIndexType nStrands = pStrandGeometry->GetStrandCount(segmentKey, strandType);
               for (StrandIndexType strandIdx = 0; strandIdx < nStrands; strandIdx++)
               {
                  CComPtr<IPoint2dCollection> profilePoints;
                  pStrandGeometry->GetStrandProfile(segmentKey, strandType, strandIdx, &profilePoints);

                  profilePoints->Offset(group_offset + running_segment_length - start_offset, 0);

                  IndexType nPoints;
                  profilePoints->get_Count(&nPoints);
                  ATLASSERT(2 <= nPoints);

                  CComPtr<IPoint2d> from_point, to_point;
                  profilePoints->get_Item(0, &from_point);
                  for (IndexType pntIdx = 1; pntIdx < nPoints; pntIdx++)
                  {
                     to_point.Release();
                     profilePoints->get_Item(pntIdx, &to_point);
                     BuildLine(pDL, geomUtil::GetPoint(from_point), geomUtil::GetPoint(to_point), STRAND_FILL_COLOR);
                     from_point = to_point;
                  } // next pntIdx
               } // next strandIdx
            } // next i

            // draw debonded strands... this must happen after other strands
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

                  profilePoints->Offset(group_offset + running_segment_length - start_offset, 0);

                  if (!IsZero(start))
                  {
                     // Left debond point
                     CComPtr<IPoint2d> left_debond;
                     profilePoints->get_Item(0, &left_debond);

                     CComPtr<IPoint2d> from_point;
                     from_point.CoCreateInstance(CLSID_Point2d);
                     from_point->MoveEx(left_debond);
                     from_point->put_X(group_offset + running_segment_length);

                     BuildLine(pDebondDL, geomUtil::GetPoint(from_point), geomUtil::GetPoint(left_debond), DEBOND_FILL_COLOR);
                     BuildDebondTick(pDebondDL, geomUtil::GetPoint(left_debond), DEBOND_FILL_COLOR);
                  }

                  if (!IsEqual(end, segment_length))
                  {
                     CComPtr<IPoint2d> right_debond;
                     profilePoints->get_Item(nPoints - 1, &right_debond);

                     CComPtr<IPoint2d> to_point;
                     to_point.CoCreateInstance(CLSID_Point2d);
                     to_point->MoveEx(right_debond);
                     to_point->put_X(group_offset + running_segment_length + segment_length);

                     BuildLine(pDebondDL, geomUtil::GetPoint(right_debond), geomUtil::GetPoint(to_point), DEBOND_FILL_COLOR);
                     BuildDebondTick(pDebondDL, geomUtil::GetPoint(right_debond), DEBOND_FILL_COLOR);
                  }
               }
            }
         } // if after release event

         running_segment_length += segment_layout_length - start_offset;
      } // end of segment loop

      group_offset += running_segment_length;
   } // end of group loop
}

void CGirderModelElevationView::BuildStrandCGDisplayObjects(CPGSDocBase* pDoc, IBroker* pBroker, const CGirderKey& girderKey,EventIndexType eventIdx)
{
#pragma Reminder("UPDATE: this shows the geometric centroid of the strand, need to show location of resultant PS force")

   auto pDL = m_pDispMgr->FindDisplayList(STRAND_CG_LIST);
   ATLASSERT(pDL);
   pDL->Clear();

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);

   // this interfaces are need in the loop, and there are cases where they don't get used
   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeometry);
   GET_IFACE2(pBroker,IIntervals,pIntervals);

   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   GroupIndexType startGroupIdx = (girderKey.groupIndex == ALL_GROUPS ? 0 : girderKey.groupIndex);
   GroupIndexType endGroupIdx   = (girderKey.groupIndex == ALL_GROUPS ? pBridgeDesc->GetGirderGroupCount()-1 : startGroupIdx);

   const CTimelineManager* pTimelineMgr = pBridgeDesc->GetTimelineManager();

   Float64 group_offset = 0;

   IntervalIndexType intervalIdx = pIntervals->GetInterval(eventIdx);

   for (GroupIndexType grpIdx = startGroupIdx; grpIdx <= endGroupIdx; grpIdx++ )
   {
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(grpIdx);
      GirderIndexType nGirders = pGroup->GetGirderCount();
      GirderIndexType gdrIdx = Min(girderKey.girderIndex,nGirders-1);
      CGirderKey thisGirderKey(grpIdx,gdrIdx);

      const CSplicedGirderData* pGirder = pGroup->GetGirder(thisGirderKey.girderIndex);
      Float64 running_segment_length = 0; // sum of the segment lengths from segIdx = 0 to current segment
      SegmentIndexType nSegments = pGirder->GetSegmentCount();
      for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
      {
         CSegmentKey segmentKey(thisGirderKey,segIdx);

         Float64 segment_length        = pBridge->GetSegmentLength(segmentKey);
         Float64 start_brg_offset      = pBridge->GetSegmentStartBearingOffset(segmentKey);
         Float64 start_end_distance    = pBridge->GetSegmentStartEndDistance(segmentKey);
         Float64 start_offset          = start_brg_offset - start_end_distance;
         Float64 segment_layout_length = pBridge->GetSegmentLayoutLength(segmentKey);

         // running_segment_length goes to the CL of the closure... adjust the distance
         // so that it goes to the left face of the current segment
         running_segment_length += start_offset;

         const CPrecastSegmentData* pSegment = pGirder->GetSegment(segIdx);
         SegmentIDType segID = pSegment->GetID();

         EventIndexType erectSegmentEventIdx = pTimelineMgr->GetSegmentErectionEventIndex(segID);
         if ( erectSegmentEventIdx <= eventIdx )
         {
            bool bIncludeTempStrands = true;
            IntervalIndexType tsInstallationIntervalIdx = pIntervals->GetTemporaryStrandInstallationInterval(segmentKey);
            IntervalIndexType tsRemovalIntervalIdx = pIntervals->GetTemporaryStrandRemovalInterval(segmentKey);
            if (tsInstallationIntervalIdx == INVALID_INDEX || tsRemovalIntervalIdx == INVALID_INDEX || intervalIdx < tsInstallationIntervalIdx || tsRemovalIntervalIdx <= intervalIdx)
            {
               bIncludeTempStrands = false;
            }

            CComPtr<IPoint2dCollection> profilePoints;
            pStrandGeometry->GetStrandCGProfile(segmentKey, bIncludeTempStrands, &profilePoints);

            profilePoints->Offset(group_offset + running_segment_length - start_offset, 0);

            IndexType nPoints;
            profilePoints->get_Count(&nPoints);
            if (0 < nPoints)
            {
               ATLASSERT(2 <= nPoints);
               CComPtr<IPoint2d> from_point;
               profilePoints->get_Item(0, &from_point);
               for (IndexType pntIdx = 1; pntIdx < nPoints; pntIdx++)
               {
                  CComPtr<IPoint2d> to_point;
                  profilePoints->get_Item(pntIdx, &to_point);

                  BuildDashLine(pDL, geomUtil::GetPoint(from_point), geomUtil::GetPoint(to_point), STRAND_CG_COLOR_1, STRAND_CG_COLOR_2);

                  from_point = to_point;
               }
            }
         }

         running_segment_length += segment_layout_length - start_offset;
      }

      group_offset += running_segment_length;
   }
}

void CGirderModelElevationView::BuildSegmentCGDisplayObjects(CPGSDocBase* pDoc, IBroker* pBroker, const CGirderKey& girderKey, EventIndexType eventIdx)
{
   auto pDL = m_pDispMgr->FindDisplayList(CG_LIST);
   ATLASSERT(pDL);
   pDL->Clear();

   GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc);

   // this interfaces are need in the loop, and there are cases where they don't get used
   GET_IFACE2_NOCHECK(pBroker, IBridge, pBridge);
   GET_IFACE2_NOCHECK(pBroker, IIntervals, pIntervals);
   GET_IFACE2_NOCHECK(pBroker, ISectionProperties, pSectProp);
   GET_IFACE2_NOCHECK(pBroker, IPointOfInterest, pPoi);
   GET_IFACE2_NOCHECK(pBroker, ICamber, pCamber);
   GET_IFACE2_NOCHECK(pBroker, IGirder, pGirder);

   Float64 group_offset = 0;


   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   GroupIndexType startGroupIdx = (girderKey.groupIndex == ALL_GROUPS ? 0 : girderKey.groupIndex);
   GroupIndexType endGroupIdx = (girderKey.groupIndex == ALL_GROUPS ? pBridgeDesc->GetGirderGroupCount() - 1 : startGroupIdx);

   const CTimelineManager* pTimelineMgr = pBridgeDesc->GetTimelineManager();

   for (GroupIndexType grpIdx = startGroupIdx; grpIdx <= endGroupIdx; grpIdx++)
   {
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(grpIdx);
      GirderIndexType nGirders = pGroup->GetGirderCount();
      GirderIndexType gdrIdx = Min(girderKey.girderIndex, nGirders - 1);
      CGirderKey thisGirderKey(grpIdx, gdrIdx);

      Float64 running_segment_length = 0; // sum of the segment lengths from segIdx = 0 to current segment

      const CSplicedGirderData* pThisGirder = pGroup->GetGirder(thisGirderKey.girderIndex);
      SegmentIndexType nSegments = pThisGirder->GetSegmentCount();
      for (SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++)
      {
         CSegmentKey segmentKey(thisGirderKey, segIdx);

         Float64 start_brg_offset = pBridge->GetSegmentStartBearingOffset(segmentKey);
         Float64 start_end_distance = pBridge->GetSegmentStartEndDistance(segmentKey);
         Float64 start_offset = start_brg_offset - start_end_distance;
         Float64 segment_layout_length = pBridge->GetSegmentLayoutLength(segmentKey);
         // running_segment_length goes to the CL of the closure... adjust the distance
         // so that it goes to the left face of the current segment
         running_segment_length += start_offset;

         const CPrecastSegmentData* pSegment = pThisGirder->GetSegment(segIdx);
         SegmentIDType segID = pSegment->GetID();
         EventIndexType erectSegmentEventIdx = pTimelineMgr->GetSegmentErectionEventIndex(segID);
         if (erectSegmentEventIdx <= eventIdx)
         {
            IntervalIndexType intervalIdx = pIntervals->GetInterval(eventIdx);
            IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
            if (intervalIdx < releaseIntervalIdx)
            {
               intervalIdx = releaseIntervalIdx;
            }

            PoiList vPoi;
            pPoi->GetPointsOfInterest(segmentKey, POI_ERECTED_SEGMENT, &vPoi);
            pPoi->GetPointsOfInterest(segmentKey, POI_SECTCHANGE, &vPoi);
            pPoi->SortPoiList(&vPoi); // sorts and removes duplicates
            auto iter = vPoi.cbegin();
            auto end = vPoi.cend();


            const pgsPointOfInterest& poi = *iter;
            iter++;
            Float64 Xs = poi.GetDistFromStart();
            Float64 X = Xs + group_offset + running_segment_length;

            Float64 Yt = pSectProp->GetY(intervalIdx, poi, pgsTypes::TopGirder);
            Float64 precamber = pCamber->GetPrecamber(poi, pgsTypes::pddErected);
            Float64 tft = pGirder->GetTopFlangeThickening(poi);

            CComPtr<IPoint2d> prevPoint;
            prevPoint.CoCreateInstance(CLSID_Point2d);
            prevPoint->Move(X, -Yt + precamber + tft);

            for (; iter != end; iter++)
            {
               const pgsPointOfInterest& poi(*iter);

               Xs = poi.GetDistFromStart();
               X = Xs + group_offset + running_segment_length;

               Yt = pSectProp->GetY(intervalIdx, poi, pgsTypes::TopGirder);
               precamber = pCamber->GetPrecamber(poi, pgsTypes::pddErected);
               tft = pGirder->GetTopFlangeThickening(poi);

               CComPtr<IPoint2d> thisPoint;
               thisPoint.CoCreateInstance(CLSID_Point2d);
               thisPoint->Move(X, -Yt + precamber + tft);

               BuildDashLine(pDL, geomUtil::GetPoint(prevPoint), geomUtil::GetPoint(thisPoint), SECTION_CG_COLOR_1, SECTION_CG_COLOR_2);

               prevPoint = thisPoint;
            } // next poi
         } // if segment erected

         running_segment_length += segment_layout_length - start_offset;
      } // next segment

      group_offset += running_segment_length;
   } // next group
}

void CGirderModelElevationView::BuildTendonDisplayObjects(CPGSDocBase* pDoc, IBroker* pBroker, const CGirderKey& girderKey,EventIndexType eventIdx)
{
   auto pDL = m_pDispMgr->FindDisplayList(TENDON_LIST);
   ATLASSERT(pDL);
   pDL->Clear();

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   GET_IFACE2_NOCHECK(pBroker, IBridge, pBridge);
   GET_IFACE2_NOCHECK(pBroker, IPointOfInterest, pPoi);
   GET_IFACE2_NOCHECK(pBroker, IGirderTendonGeometry, pGirderTendonGeometry); // not always used (this is because of the continue statement below)
   GET_IFACE2_NOCHECK(pBroker, ISegmentTendonGeometry, pSegmentTendonGeometry); // not always used (this is because of the continue statement below)
   GET_IFACE2_NOCHECK(pBroker, IGirder, pGirder); // not always used (this is because of the continue statement below)

   const CTimelineManager* pTimelineMgr = pBridgeDesc->GetTimelineManager();

   GroupIndexType startGroupIdx = (girderKey.groupIndex == ALL_GROUPS ? 0 : girderKey.groupIndex);
   GroupIndexType endGroupIdx   = (girderKey.groupIndex == ALL_GROUPS ? pBridgeDesc->GetGirderGroupCount()-1 : startGroupIdx);


   Float64 group_offset = 0;
   for ( GroupIndexType grpIdx = startGroupIdx; grpIdx <= endGroupIdx; grpIdx++ )
   {
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(grpIdx);
      GirderIndexType nGirders = pGroup->GetGirderCount();
      GirderIndexType gdrIdx = Min(girderKey.girderIndex,nGirders-1);

      CGirderKey thisGirderKey(grpIdx,gdrIdx);

      const CSplicedGirderData* pThisGirder = pIBridgeDesc->GetGirder(thisGirderKey);
      GirderIDType thisGirderID = pThisGirder->GetID();

#pragma Reminder("UPDATE: draw portion of strand in a segment")
      // this next block of code is a cop-out. it would be better to 
      // draw the duct in the segment if it is erected.
      //
      // IF THIS GETS FIXED, RE-EVALUATE THE USE OF GET_IFACE2_NOCHECK above
      if ( !pTimelineMgr->AreAllSegmentsErected(thisGirderID,eventIdx) )
      {
         continue; // if all segments are not erected, there is nothing to draw
      }

      WebIndexType nWebs = pGirder->GetWebCount(thisGirderKey);

      DuctIndexType nDucts = pGirderTendonGeometry->GetDuctCount(thisGirderKey);
      for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
      {
         bool bIsTendonInstalled = true;
         IndexType index = ductIdx/nWebs;
         EventIndexType ptEventIdx = pTimelineMgr->GetStressTendonEventIndex(thisGirderID,index);
         if ( eventIdx < ptEventIdx || ptEventIdx == INVALID_INDEX )
         {
            bIsTendonInstalled = false;
         }
         
         CComPtr<IPoint2dCollection> ductPoints;
         pGirderTendonGeometry->GetDuctCenterline(thisGirderKey,ductIdx,&ductPoints); // this is in Girder Coordinates (measured from face of girder)
         
         // The view is working in Girder Path Coordinates. Need to convert the X values from Girder to Girder Path Cooordinates

         IndexType nPoints;
         ductPoints->get_Count(&nPoints);
         CComPtr<IPoint2d> from_point;
         ductPoints->get_Item(0,&from_point);
         Float64 X,Y;
         from_point->Location(&X,&Y);
         X = pPoi->ConvertGirderCoordinateToGirderPathCoordinate(thisGirderKey,X);
         X += group_offset;
         from_point->Move(X,Y);

         for( IndexType pntIdx = 1; pntIdx < nPoints; pntIdx++ )
         {
            CComPtr<IPoint2d> to_point;
            ductPoints->get_Item(pntIdx,&to_point);
            to_point->Location(&X,&Y);
            X = pPoi->ConvertGirderCoordinateToGirderPathCoordinate(thisGirderKey,X);
            X += group_offset;
            to_point->Move(X,Y);

            if ( bIsTendonInstalled )
            {
               BuildLine(pDL, geomUtil::GetPoint(from_point), geomUtil::GetPoint(to_point), GIRDER_TENDON_LINE_COLOR);
            }
            else
            {
               BuildDashLine(pDL, geomUtil::GetPoint(from_point), geomUtil::GetPoint(to_point), GIRDER_DUCT_LINE_COLOR1, GIRDER_DUCT_LINE_COLOR2);
            }

            from_point = to_point;
         } // next point
      } // next duct

      SegmentIndexType nSegments = pThisGirder->GetSegmentCount();
      for (SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++)
      {
         CSegmentKey segmentKey(thisGirderKey, segIdx);
         DuctIndexType nDucts = pSegmentTendonGeometry->GetDuctCount(segmentKey);
         for (DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++)
         {
            CComPtr<IPoint2dCollection> ductPoints;
            pSegmentTendonGeometry->GetDuctCenterline(segmentKey, ductIdx, &ductPoints); // this is in Segment Coordinates (measured from face of segment)

            // The view is working in Girder Path Coordinates. Need to convert the X values from Segment to Girder Path Cooordinates
            IndexType nPoints;
            ductPoints->get_Count(&nPoints);
            CComPtr<IPoint2d> from_point;
            ductPoints->get_Item(0, &from_point);
            Float64 X, Y;
            from_point->Location(&X, &Y);
            X = pPoi->ConvertSegmentCoordinateToGirderCoordinate(segmentKey, X);
            X = pPoi->ConvertGirderCoordinateToGirderPathCoordinate(thisGirderKey, X);
            X += group_offset;
            from_point->Move(X, Y);

            for (IndexType pntIdx = 1; pntIdx < nPoints; pntIdx++)
            {
               CComPtr<IPoint2d> to_point;
               ductPoints->get_Item(pntIdx, &to_point);
               to_point->Location(&X, &Y);
               X = pPoi->ConvertSegmentCoordinateToGirderCoordinate(segmentKey, X);
               X = pPoi->ConvertGirderCoordinateToGirderPathCoordinate(thisGirderKey, X);
               X += group_offset;
               to_point->Move(X, Y);

               BuildLine(pDL, geomUtil::GetPoint(from_point), geomUtil::GetPoint(to_point), SEGMENT_TENDON_LINE_COLOR);

               from_point = to_point;
            } // next point
         } // next duct
      } // next segment

      Float64 girder_length = pBridge->GetGirderLayoutLength(thisGirderKey);
      group_offset += girder_length;
   } // next group
}

void CGirderModelElevationView::BuildRebarDisplayObjects(CPGSDocBase* pDoc, IBroker* pBroker, const CGirderKey& girderKey,EventIndexType eventIdx)
{
   auto pDL = m_pDispMgr->FindDisplayList(REBAR_LIST);
   ATLASSERT(pDL);
   pDL->Clear();

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,ILongRebarGeometry,pLongRebarGeometry);
   GET_IFACE2_NOCHECK(pBroker,IPointOfInterest,pPoi); // not used if there aren't any rebar

   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CTimelineManager* pTimelineMgr = pBridgeDesc->GetTimelineManager();

   GroupIndexType startGroupIdx = (girderKey.groupIndex == ALL_GROUPS ? 0 : girderKey.groupIndex);
   GroupIndexType endGroupIdx   = (girderKey.groupIndex == ALL_GROUPS ? pBridgeDesc->GetGirderGroupCount()-1 : startGroupIdx);

   Float64 group_offset = 0;

   for ( GroupIndexType grpIdx = startGroupIdx; grpIdx <= endGroupIdx; grpIdx++ )
   {
      Float64 running_segment_length = 0;

      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(grpIdx);
      GirderIndexType nGirders = pGroup->GetGirderCount();
      GirderIndexType gdrIdx = Min(girderKey.girderIndex,nGirders-1);
      CGirderKey thisGirderKey(grpIdx,gdrIdx);

      const CSplicedGirderData* pGirder = pGroup->GetGirder(thisGirderKey.girderIndex);
      SegmentIndexType nSegments = pGirder->GetSegmentCount();
      for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
      {
         const CPrecastSegmentData* pSegment = pGirder->GetSegment(segIdx);
         SegmentIDType segID = pSegment->GetID();

         CSegmentKey segmentKey(thisGirderKey,segIdx);

#pragma Reminder("UPDATE: need to include closure joint rebar")
         Float64 segment_length = pBridge->GetSegmentLength(segmentKey);
         Float64 segment_layout_length = pBridge->GetSegmentLayoutLength(segmentKey);
         Float64 start_brg_offset = pBridge->GetSegmentStartBearingOffset(segmentKey);
         Float64 start_end_distance = pBridge->GetSegmentStartEndDistance(segmentKey);
         Float64 start_offset = start_brg_offset - start_end_distance;

         // running_segment_length goes to the CL of the closure... adjust the distance
         // so that it goes to the left face of the current segment
         running_segment_length += start_offset;


         // if segment isn't erected, don't display rebar
         if ( eventIdx < pTimelineMgr->GetSegmentErectionEventIndex(segID) )
         {
            running_segment_length += segment_layout_length - start_offset;
            continue;
         }

         CComPtr<IRebarLayout> rebarLayout;
         pLongRebarGeometry->GetRebarLayout(segmentKey, &rebarLayout);

         CComPtr<IEnumRebarLayoutItems> enumItems;
         rebarLayout->get__EnumRebarLayoutItems(&enumItems);
   
         CComPtr<IRebarLayoutItem> rebarLayoutItem;
         while ( enumItems->Next(1,&rebarLayoutItem,nullptr) != S_FALSE )
         {
            Float64 startLoc, layoutLength, endLoc;
            rebarLayoutItem->get_Start(&startLoc);
            rebarLayoutItem->get_Length(&layoutLength);

            startLoc = pPoi->ConvertSegmentCoordinateToSegmentPathCoordinate(segmentKey,startLoc); 
            startLoc = pPoi->ConvertSegmentPathCoordinateToGirderPathCoordinate(segmentKey,startLoc);

            endLoc = startLoc + layoutLength;

            if ( running_segment_length+segment_length <= startLoc )
            {
               // rebar is beyond the end of the segment which means it is in the closure joint
               // only draw rebar in the closure joint if it has been cast
               if ( eventIdx < pIBridgeDesc->GetCastClosureJointEventIndex(segmentKey.groupIndex,segmentKey.segmentIndex) )
               {
                  rebarLayoutItem.Release();
                  running_segment_length += segment_layout_length - start_offset;
                  continue;
               }
            }

            CComPtr<IEnumRebarPatterns> enumPatterns;
            rebarLayoutItem->get__EnumRebarPatterns(&enumPatterns);

            CComPtr<IRebarPattern> rebarPattern;
            while ( enumPatterns->Next(1,&rebarPattern,nullptr) != S_FALSE )
            {
               // Currently, we only enter rebars in horizontal rows. If this is not the case,
               // bars at each elevation must be drawn. Since the rebar rows are horizontal
               // we only need to draw one.
               IndexType nbars;
               rebarPattern->get_Count(&nbars);
               if (0 < nbars)
               {
                  CComPtr<IPoint2dCollection> profile;
                  rebarPattern->get_Profile(0,&profile);
                  profile->Offset(group_offset + running_segment_length,0);
                  BuildLine(pDL, geomUtil::CreatePointCollection(profile), REBAR_COLOR);
               }

               rebarPattern.Release();
            } // next rebar pattern

            rebarLayoutItem.Release();
        
         } // next rebar layout
   
         running_segment_length += segment_layout_length - start_offset;
      } // next segment
         
      group_offset += running_segment_length;
   } // next group
}

template <class T> bool IsLoadApplicable(IBroker* pBroker,const T* pLoad,EventIndexType eventIdx,const CGirderKey& girderKey)
{
   GET_IFACE2(pBroker,IBridgeDescription,pBridgeDesc);
   const CTimelineManager* pTimelineMgr = pBridgeDesc->GetTimelineManager();
   EventIndexType loadingEventIdx = pTimelineMgr->FindUserLoadEventIndex(pLoad->m_ID);
   if ( loadingEventIdx != eventIdx )
   {
      return false;
   }

   GET_IFACE2(pBroker,IBridge,pBridge);
   PierIndexType startPierIdx, endPierIdx;
   if ( girderKey.groupIndex == ALL_GROUPS )
   {
      startPierIdx = 0;
      endPierIdx = pBridge->GetPierCount()-1;
   }
   else
   {
      pBridge->GetGirderGroupPiers(girderKey.groupIndex,&startPierIdx,&endPierIdx);
   }

   SpanIndexType startSpanIdx = (SpanIndexType)startPierIdx;
   SpanIndexType endSpanIdx   = (SpanIndexType)(endPierIdx-1);

   bool bMatchSpan = ((startSpanIdx <= pLoad->m_SpanKey.spanIndex && pLoad->m_SpanKey.spanIndex <= endSpanIdx) || pLoad->m_SpanKey.spanIndex == ALL_SPANS) ? true : false;
   bool bMatchGirder = (pLoad->m_SpanKey.girderIndex == girderKey.girderIndex || pLoad->m_SpanKey.girderIndex == ALL_GIRDERS) ? true : false;

   return bMatchSpan && bMatchGirder;
}

void CGirderModelElevationView::BuildPointLoadDisplayObjects(CPGSDocBase* pDoc, IBroker* pBroker, const CGirderKey& girderKey,EventIndexType eventIdx, bool* casesExist)
{
   auto pDL = m_pDispMgr->FindDisplayList(LOAD_LIST);
   ATLASSERT(pDL);

   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IUserDefinedLoadData,pUserDefinedLoadData);
   GET_IFACE2_NOCHECK(pBroker,IPointOfInterest,pPoi);

   IndexType nLoads =  pUserDefinedLoadData->GetPointLoadCount();
   SpanIndexType nSpans = pBridge->GetSpanCount();

   // filter loads and determine magnitude of max load
   Float64 max = 0.0;
   IndexType loadIdx;
   for (loadIdx = 0; loadIdx < nLoads; loadIdx++)
   {
      const CPointLoadData* pLoad = pUserDefinedLoadData->GetPointLoad(loadIdx);

      if (IsLoadApplicable(pBroker,pLoad,eventIdx,girderKey))
      {
         max = Max(fabs(pLoad->m_Magnitude), max);
      }
   }

   auto factory = m_pDispMgr->GetDisplayObjectFactory(0);

   SpanIndexType displayStartSpanIdx, displayEndSpanIdx;
   GetSpanRange(pBroker,girderKey,&displayStartSpanIdx,&displayEndSpanIdx);
   Float64 span_offset = GetSpanStartLocation(CSpanKey(displayStartSpanIdx,girderKey.girderIndex));


   // create load display objects from filtered list
   for (loadIdx = 0; loadIdx < nLoads; loadIdx++)
   {
      const CPointLoadData* pLoad = pUserDefinedLoadData->GetPointLoad(loadIdx);
      if (IsLoadApplicable(pBroker,pLoad,eventIdx,girderKey))
      {
         casesExist[pLoad->m_LoadCase] = true;

         COLORREF color = GetLoadGroupColor(pLoad->m_LoadCase);

         SpanIndexType startSpanIdx = (pLoad->m_SpanKey.spanIndex == ALL_SPANS ? 0 : pLoad->m_SpanKey.spanIndex);
         SpanIndexType endSpanIdx   = (pLoad->m_SpanKey.spanIndex == ALL_SPANS ? nSpans-1 : startSpanIdx);
         for ( SpanIndexType spanIdx = startSpanIdx; spanIdx <= endSpanIdx; spanIdx++ )
         {
            if ( spanIdx < displayStartSpanIdx || displayEndSpanIdx < spanIdx )
            {
               // this span is not being displayed
               continue;
            }

            GroupIndexType grpIdx = pBridge->GetGirderGroupIndex(spanIdx);
            GirderIndexType nGirders = pBridge->GetGirderCount(grpIdx);
            GirderIndexType gdrIdx = Min(girderKey.girderIndex,nGirders-1);
            CSpanKey spanKey(spanIdx,gdrIdx);

            CSpanKey startOfGroupSpanKey(pBridge->GetGirderGroupStartSpan(grpIdx), gdrIdx);

            Float64 cantilever_length = pBridge->GetCantileverLength(spanKey.spanIndex,spanKey.girderIndex,(spanKey.spanIndex == 0 ? pgsTypes::metStart : pgsTypes::metEnd));
            Float64 span_length = pBridge->GetSpanLength(spanKey.spanIndex,spanKey.girderIndex);

            Float64 Xspan = pLoad->m_Location;
            if (pLoad->m_Fractional)
            {
               if ( pLoad->m_bLoadOnCantilever[pgsTypes::metStart] )
               {
                  Xspan *= cantilever_length;
               }
               else if ( pLoad->m_bLoadOnCantilever[pgsTypes::metEnd] )
               {
                  Xspan *= cantilever_length;
                  Xspan += span_length;
               }
               else
               {
                  Xspan *= span_length;
               }
            }

            auto disp_obj = factory->Create(CPointLoadDrawStrategyImpl::ms_Format,nullptr);

            auto point_disp = std::dynamic_pointer_cast<WBFL::DManip::iPointDisplayObject>(disp_obj);

            auto pStrategy = point_disp->GetDrawingStrategy();

            auto pls = std::dynamic_pointer_cast<iPointLoadDrawStrategy>(pStrategy);
            pls->Init(point_disp, pBroker, *pLoad, loadIdx, span_length, max, color);

            CSegmentKey segmentKey;
            Float64 Xsp;
            pPoi->ConvertSpanPointToSegmentPathCoordiante(spanKey, Xspan, &segmentKey, &Xsp); // get start of load location (we can get it in segment path coordinates)
            Float64 Xgp = pPoi->ConvertSegmentPathCoordinateToGirderPathCoordinate(segmentKey, Xsp); // convert to girder path coordinates

            Float64 groupOffset = GetSpanStartLocation(startOfGroupSpanKey); // get the offset to the start of the span

            Float64 x_position = Xgp + groupOffset - span_offset; // load position

            WBFL::Geometry::Point2d point(x_position, 0.0);
            point_disp->SetPosition(point, false, false);

            // tool tip
            CComPtr<IBroker> pBroker;
            EAFGetBroker(&pBroker);
            GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
            CString strMagnitude = FormatDimension(pLoad->m_Magnitude,pDisplayUnits->GetGeneralForceUnit(),true);
            CString strLocation  = FormatDimension(Xspan,pDisplayUnits->GetSpanLengthUnit(),true);

            CString strToolTip;
            strToolTip.Format(_T("Point Load\r\nP = %s  L = %s from left end of span\r\n%s Load Case"),
                               strMagnitude,strLocation,GetLoadGroupNameForUserLoad(pLoad->m_LoadCase));

            if ( pLoad->m_Description != _T("") )
            {
               strToolTip += _T("\r\n") + CString(pLoad->m_Description.c_str());
            }

            point_disp->SetMaxTipWidth(TOOLTIP_WIDTH);
            point_disp->SetTipDisplayTime(TOOLTIP_DURATION);
            point_disp->SetToolTipText(strToolTip);
            point_disp->SetID(loadIdx);

            pDL->AddDisplayObject(disp_obj);
         }
      }
   }
}

void CGirderModelElevationView::BuildDistributedLoadDisplayObjects(CPGSDocBase* pDoc, IBroker* pBroker, const CGirderKey& girderKey,EventIndexType eventIdx, bool* casesExist)
{
   auto pDL = m_pDispMgr->FindDisplayList(LOAD_LIST);
   ATLASSERT(pDL);

   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IUserDefinedLoadData,pUserDefinedLoadData);
   GET_IFACE2_NOCHECK(pBroker, IPointOfInterest, pPoi);

   IndexType nLoads =  pUserDefinedLoadData->GetDistributedLoadCount();
   SpanIndexType nSpans = pBridge->GetSpanCount();

   SpanIndexType displayStartSpanIdx, displayEndSpanIdx;
   GetSpanRange(pBroker,girderKey,&displayStartSpanIdx,&displayEndSpanIdx);
   Float64 span_offset = GetSpanStartLocation(CSpanKey(displayStartSpanIdx,girderKey.girderIndex));

   // filter loads and determine magnitude of max load
   Float64 max = 0.0;
   IndexType loadIdx;
   for (loadIdx = 0; loadIdx < nLoads; loadIdx++ )
   {
      const CDistributedLoadData* pLoad = pUserDefinedLoadData->GetDistributedLoad(loadIdx);

      if (IsLoadApplicable(pBroker,pLoad,eventIdx,girderKey))
      {
         max = Max(fabs(pLoad->m_WStart), fabs(pLoad->m_WEnd), max);
      }
   }

   auto factory = m_pDispMgr->GetDisplayObjectFactory(0);

   // create load display objects from filtered list
   for (loadIdx = 0; loadIdx < nLoads; loadIdx++ )
   {
      const CDistributedLoadData* pLoad = pUserDefinedLoadData->GetDistributedLoad(loadIdx);

      if (IsLoadApplicable(pBroker,pLoad,eventIdx,girderKey))
      {
         casesExist[pLoad->m_LoadCase] = true;

         COLORREF color = GetLoadGroupColor(pLoad->m_LoadCase);

         SpanIndexType startSpanIdx = (pLoad->m_SpanKey.spanIndex == ALL_SPANS ? 0 : pLoad->m_SpanKey.spanIndex);
         SpanIndexType endSpanIdx   = (pLoad->m_SpanKey.spanIndex == ALL_SPANS ? nSpans-1 : startSpanIdx);
         for ( SpanIndexType spanIdx = startSpanIdx; spanIdx <= endSpanIdx; spanIdx++ )
         {
            if ( spanIdx < displayStartSpanIdx || displayEndSpanIdx < spanIdx )
            {
               // this span is not being displayed
               continue;
            }

            GroupIndexType grpIdx = pBridge->GetGirderGroupIndex(spanIdx);
            GirderIndexType nGirders = pBridge->GetGirderCount(grpIdx);
            GirderIndexType gdrIdx = Min(girderKey.girderIndex,nGirders-1);
            CSpanKey spanKey(spanIdx,gdrIdx);

            CSpanKey startOfGroupSpanKey(pBridge->GetGirderGroupStartSpan(grpIdx), gdrIdx);

            Float64 span_length = pBridge->GetSpanLength(spanKey.spanIndex,spanKey.girderIndex);

            Float64 Xspan_start, Xspan_end;
            if (pLoad->m_Type == UserLoads::Uniform)
            {
               Xspan_start = 0.0;
               Xspan_end   = span_length;
            }
            else
            {
               Xspan_start = pLoad->m_StartLocation;
               Xspan_end   = pLoad->m_EndLocation;
               if (pLoad->m_Fractional)
               {
                  Xspan_start *= span_length;
                  Xspan_end   *= span_length;;
               }
            }

            Float64 load_length = Xspan_end - Xspan_start;
            if(load_length <= 0.0)
            {
               ATLASSERT(false); // interface should be blocking this
               break;
            }

            auto disp_obj = factory->Create(CDistributedLoadDrawStrategyImpl::ms_Format,nullptr);

            auto point_disp = std::dynamic_pointer_cast<WBFL::DManip::iPointDisplayObject>(disp_obj);

            auto pStrategy = point_disp->GetDrawingStrategy();

            auto pls = std::dynamic_pointer_cast<iDistributedLoadDrawStrategy>(pStrategy);
            pls->Init(point_disp, pBroker, *pLoad, loadIdx, load_length, span_length, max, color);

            // get the point for the load display object
            CSegmentKey segmentKey;
            Float64 Xsp;
            pPoi->ConvertSpanPointToSegmentPathCoordiante(spanKey, Xspan_start, &segmentKey, &Xsp); // get start of load location (we can get it in segment path coordinates)
            Float64 Xgp = pPoi->ConvertSegmentPathCoordinateToGirderPathCoordinate(segmentKey, Xsp); // convert to girder path coordinates

            Float64 groupOffset = GetSpanStartLocation(startOfGroupSpanKey); // get the offset to the start of the span

            Float64 x_position = Xgp + groupOffset - span_offset; // load position

            WBFL::Geometry::Point2d point(x_position, 0.0);
            point_disp->SetPosition(point, FALSE, FALSE);

            // tool tip
            CComPtr<IBroker> pBroker;
            EAFGetBroker(&pBroker);
            GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
            CString strStartMagnitude = FormatDimension(pLoad->m_WStart,pDisplayUnits->GetForcePerLengthUnit(),true);
            CString strEndMagnitude   = FormatDimension(pLoad->m_WEnd,pDisplayUnits->GetForcePerLengthUnit(),true);
            CString strStartLocation  = FormatDimension(Xspan_start,pDisplayUnits->GetSpanLengthUnit(),true);
            CString strEndLocation    = FormatDimension(Xspan_end,pDisplayUnits->GetSpanLengthUnit(),true);


            CString strToolTip;
            strToolTip.Format(_T("Distributed Load\r\nWstart = %s  Wend = %s\r\nLstart = %s  Lend = %s from left end of span\r\n%s Load Case"),
                               strStartMagnitude,strEndMagnitude,strStartLocation,strEndLocation,GetLoadGroupNameForUserLoad(pLoad->m_LoadCase));

            if ( pLoad->m_Description != _T("") )
            {
               strToolTip += _T("\r\n") + CString(pLoad->m_Description.c_str());
            }

            point_disp->SetMaxTipWidth(TOOLTIP_WIDTH);
            point_disp->SetTipDisplayTime(TOOLTIP_DURATION);
            point_disp->SetToolTipText(strToolTip);
            point_disp->SetID(loadIdx);

            pDL->AddDisplayObject(disp_obj);
         }
      }
   }
}

void CGirderModelElevationView::BuildMomentLoadDisplayObjects(CPGSDocBase* pDoc, IBroker* pBroker,const CGirderKey& girderKey, EventIndexType eventIdx, bool* casesExist)
{
   auto pDL = m_pDispMgr->FindDisplayList(LOAD_LIST);
   ATLASSERT(pDL);

   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IUserDefinedLoadData,pUserDefinedLoadData);

   IndexType nLoads =  pUserDefinedLoadData->GetMomentLoadCount();
   SpanIndexType nSpans = pBridge->GetSpanCount();

#if defined _DEBUG
   if ( EAFGetDocument()->IsKindOf(RUNTIME_CLASS(CPGSpliceDoc)) )
   {
      // there shouldn't be any moment loads for PGSplice
      ATLASSERT(nLoads == 0);
   }
#endif

   if ( nLoads == 0 )
   {
      return;
   }

   GET_IFACE2_NOCHECK(pBroker,IGirder,pGirder);
   GET_IFACE2_NOCHECK(pBroker,IPointOfInterest,pPoi);

   SpanIndexType displayStartSpanIdx, displayEndSpanIdx;
   GetSpanRange(pBroker,girderKey,&displayStartSpanIdx,&displayEndSpanIdx);
   Float64 span_offset = GetSpanStartLocation(CSpanKey(displayStartSpanIdx,girderKey.girderIndex));


   // filter loads and determine magnitude of max load
   Float64 max = 0.0;
   for (IndexType loadIdx = 0; loadIdx < nLoads; loadIdx++ )
   {
      const CMomentLoadData* pLoad = pUserDefinedLoadData->GetMomentLoad(loadIdx);

      if (IsLoadApplicable(pBroker,pLoad,eventIdx,girderKey))
      {
         max = Max(fabs(pLoad->m_Magnitude), max);
      }
   }

   auto factory = m_pDispMgr->GetDisplayObjectFactory(0);

   // create load display objects from filtered list
   for (IndexType loadIdx = 0; loadIdx < nLoads; loadIdx++)
   {
      const CMomentLoadData* pLoad = pUserDefinedLoadData->GetMomentLoad(loadIdx);

      if (IsLoadApplicable(pBroker,pLoad,eventIdx,girderKey))
      {
         casesExist[pLoad->m_LoadCase] = true;

         auto disp_obj = factory->Create(CMomentLoadDrawStrategyImpl::ms_Format,nullptr);

         auto point_disp = std::dynamic_pointer_cast<WBFL::DManip::iPointDisplayObject>(disp_obj);

         auto pStrategy = point_disp->GetDrawingStrategy();

         COLORREF color = GetLoadGroupColor(pLoad->m_LoadCase);

         SpanIndexType startSpanIdx = (pLoad->m_SpanKey.spanIndex == ALL_SPANS ? 0 : pLoad->m_SpanKey.spanIndex);
         SpanIndexType endSpanIdx   = (pLoad->m_SpanKey.spanIndex == ALL_SPANS ? nSpans-1 : startSpanIdx);
         for ( SpanIndexType spanIdx = startSpanIdx; spanIdx <= endSpanIdx; spanIdx++ )
         {
            if ( spanIdx < displayStartSpanIdx || displayEndSpanIdx < spanIdx )
            {
               // this span is not being displayed
               continue;
            }

            GroupIndexType grpIdx = pBridge->GetGirderGroupIndex(spanIdx);
            GirderIndexType nGirders = pBridge->GetGirderCount(grpIdx);
            GirderIndexType gdrIdx = Min(girderKey.girderIndex,nGirders-1);
            CSpanKey spanKey(spanIdx,gdrIdx);

            CSpanKey startOfGroupSpanKey(pBridge->GetGirderGroupStartSpan(grpIdx), gdrIdx);

            Float64 span_length = pBridge->GetSpanLength(spanKey.spanIndex,spanKey.girderIndex);

            Float64 Xspan = pLoad->m_Location;
            if (pLoad->m_Fractional)
            {
               Xspan *= span_length;
            }

            auto disp_obj = factory->Create(CMomentLoadDrawStrategyImpl::ms_Format,nullptr);

            auto point_disp = std::dynamic_pointer_cast<WBFL::DManip::iPointDisplayObject>(disp_obj);

            auto pStrategy = point_disp->GetDrawingStrategy();

            auto pls = std::dynamic_pointer_cast<iMomentLoadDrawStrategy>(pStrategy);
            pls->Init(point_disp, pBroker, *pLoad, loadIdx, span_length, max, color);


            CSegmentKey segmentKey;
            Float64 Xsp;
            pPoi->ConvertSpanPointToSegmentPathCoordiante(spanKey, Xspan, &segmentKey, &Xsp); // get start of load location (we can get it in segment path coordinates)
            Float64 Xgp = pPoi->ConvertSegmentPathCoordinateToGirderPathCoordinate(segmentKey, Xsp); // convert to girder path coordinates

            Float64 groupOffset = GetSpanStartLocation(startOfGroupSpanKey); // get the offset to the start of the span

            Float64 x_position = Xgp + groupOffset - span_offset; // load position

            WBFL::Geometry::Point2d point(x_position, 0.0);
            point_disp->SetPosition(point, FALSE, FALSE);

            // tool tip
            CComPtr<IBroker> pBroker;
            EAFGetBroker(&pBroker);
            GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
            CString strMagnitude = FormatDimension(pLoad->m_Magnitude,pDisplayUnits->GetMomentUnit(),true);
            CString strLocation  = FormatDimension(Xspan,pDisplayUnits->GetSpanLengthUnit(),true);

            CString strTooltip;
            strTooltip.Format(_T("Moment Load\r\nM = %s L = %s from left end of girder\r\n%s Load Case"),strMagnitude,strLocation,GetLoadGroupNameForUserLoad(pLoad->m_LoadCase));
            if ( pLoad->m_Description != _T("") )
            {
               strTooltip += _T("\r\n") + CString(pLoad->m_Description.c_str());
            }

            point_disp->SetMaxTipWidth(TOOLTIP_WIDTH);
            point_disp->SetTipDisplayTime(TOOLTIP_DURATION);
            point_disp->SetToolTipText(strTooltip);
            point_disp->SetID(loadIdx);

            pDL->AddDisplayObject(disp_obj);
         }
      }
   }
}

void CGirderModelElevationView::BuildLegendDisplayObjects(CPGSDocBase* pDoc, IBroker* pBroker,const CGirderKey& girderKey, EventIndexType eventIdx, bool* casesExist)
{
   if (casesExist[UserLoads::DC] || casesExist[UserLoads::DW] || casesExist[UserLoads::LL_IM])
   {
      IndexType prevNumEntries(INVALID_INDEX);
      m_Legend = WBFL::DManip::LegendDisplayObject::Create();
      m_Legend->SetTitle(_T("Legend"));
      m_Legend->DrawBorder(true);
      m_Legend->IsDraggable(true);

      // locate the legend at the top left corner the first time through only
      ScaleToFit(false);
      auto rect = m_pDispMgr->GetBoundingBox(false);
      auto point = rect.TopRight();

      m_Legend->SetPosition(point,false,false);

      IndexType nEntries = 0;
      if (casesExist[UserLoads::DC])
      {
         nEntries++;
         CreateLegendEntry(UserLoads::DC, m_Legend);
      }

      if (casesExist[UserLoads::DW])
      {
         nEntries++;
         CreateLegendEntry(UserLoads::DW, m_Legend);
      }

      if (casesExist[UserLoads::LL_IM])
      {
         nEntries++;
         CreateLegendEntry(UserLoads::LL_IM, m_Legend);
      }

      // add to display list
      auto pDL = m_pDispMgr->FindDisplayList(LOAD_LIST);
      ATLASSERT(pDL);

      pDL->AddDisplayObject(m_Legend);

      // now can change size and shape of legend
      if (nEntries != prevNumEntries)
      {
         m_Legend->SetNumRows(nEntries);
      }

      auto size = m_Legend->GetMinCellSize();
      m_Legend->SetCellSize(size);
   }
}

void CGirderModelElevationView::BuildDimensionDisplayObjects(CPGSDocBase* pDoc, IBroker* pBroker, const CGirderKey& girderKey,EventIndexType eventIdx)
{
   auto pDL = m_pDispMgr->FindDisplayList(DIMLINE_LIST);
   ATLASSERT(pDL);
   pDL->Clear();

   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   GET_IFACE2(pBroker,IGirder,pIGirder);
   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeometry);
   GET_IFACE2(pBroker,IPointOfInterest,pPoi);

   // need to layout dimension line witness lines in twips
   const long twip_offset = 1440/2;

   CComPtr<IPoint2d> from_point, to_point;
   from_point.CoCreateInstance(__uuidof(Point2d));
   to_point.CoCreateInstance(__uuidof(Point2d));

   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CTimelineManager* pTimelineMgr = pBridgeDesc->GetTimelineManager();

   GroupIndexType nGroups = pBridgeDesc->GetGirderGroupCount();
   GroupIndexType startGroupIdx = (girderKey.groupIndex == ALL_GROUPS ? 0 : girderKey.groupIndex);
   GroupIndexType endGroupIdx   = (girderKey.groupIndex == ALL_GROUPS ? nGroups-1 : startGroupIdx);

   // Get maximum height of the girder. This will be used for locating the dimensions vertically in the window
   Float64 Hg = 0;
   Float64 HgEnds = 0;
   Float64 topPrecamberAdjustment = 0;
   Float64 bottomPrecamberAdjustment = 0;
   Float64 topFlangeThickeningAdjustment = 0;
   for ( GroupIndexType grpIdx = startGroupIdx; grpIdx <= endGroupIdx; grpIdx++ )
   {
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(grpIdx);
      GirderIndexType nGirders = pGroup->GetGirderCount();
      GirderIndexType gdrIdx = Min(girderKey.girderIndex,nGirders-1);
      CGirderKey thisGirderKey(grpIdx,gdrIdx);

      const CSplicedGirderData* pGirder = pGroup->GetGirder(thisGirderKey.girderIndex);
      SegmentIndexType nSegments = pGirder->GetSegmentCount();

      for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
      {
         CSegmentKey segmentKey(thisGirderKey,segIdx);

         CComPtr<IShape> shape;
         pIGirder->GetSegmentProfile(segmentKey, false, &shape);

         CComPtr<IRect2d> box;
         shape->get_BoundingBox(&box);
         Float64 H;
         box->get_Height(&H);
         Hg = Max(H, Hg);

         Float64 segment_length  = pBridge->GetSegmentLength(segmentKey); // end to end length of segment

         // Use the real POI rather than ones created on the fly. It is faster to get section information
         // with real POI
         pgsPointOfInterest poiStart = pPoi->GetPointOfInterest(segmentKey,0.00);
         pgsPointOfInterest poiEnd   = pPoi->GetPointOfInterest(segmentKey,segment_length);
         Float64 Hg_start = pIGirder->GetHeight(poiStart);
         Float64 Hg_end   = pIGirder->GetHeight(poiEnd);
         HgEnds = ::Max(Hg_start,Hg_end, HgEnds);

         const CPrecastSegmentData* pSegment = pGirder->GetSegment(segIdx);
         if (!IsZero(pSegment->Precamber))
         {
            topPrecamberAdjustment = Max(topPrecamberAdjustment, pSegment->Precamber);
            bottomPrecamberAdjustment = Min(bottomPrecamberAdjustment, pSegment->Precamber);
         }

         if (pSegment->TopFlangeThickeningType == pgsTypes::tftMiddle)
         {
            topFlangeThickeningAdjustment = Max(topFlangeThickeningAdjustment, pSegment->TopFlangeThickening);
         }
      }
   }

   pgsPointOfInterest poiStart;
   pgsPointOfInterest poiEnd;
   Float64 Xgs, Xge;
   Float64 x1,x2;

   Float64 group_offset = 0;
   for ( GroupIndexType grpIdx = startGroupIdx; grpIdx <= endGroupIdx; grpIdx++ )
   {
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(grpIdx);
      GirderIndexType nGirders = pGroup->GetGirderCount();
      GirderIndexType gdrIdx = Min(girderKey.girderIndex,nGirders-1);

      CGirderKey thisGirderKey(grpIdx,gdrIdx);

      const CSplicedGirderData* pGirder = pGroup->GetGirder(thisGirderKey.girderIndex);
      SegmentIndexType nSegments = pGirder->GetSegmentCount();

      for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
      {
         CSegmentKey segmentKey(thisGirderKey,segIdx);

         const auto* pSegment = pIBridgeDesc->GetPrecastSegmentData(segmentKey);

         Float64 segment_length = pBridge->GetSegmentLength(segmentKey);       // end to end length of segment

         poiStart = pPoi->GetPointOfInterest(segmentKey,0.00);
         poiEnd   = pPoi->GetPointOfInterest(segmentKey,segment_length);

         SegmentIDType segID = pSegment->GetID();
         EventIndexType erectSegmentEventIdx = pTimelineMgr->GetSegmentErectionEventIndex(segID);
         bool bIsErected = (erectSegmentEventIdx <= eventIdx ? true : false);
         
         //
         // Top Dimension Lines
         //

         if (bIsErected)
         {
            // segment length measure to end of segment
            Xgs = pPoi->ConvertPoiToGirderPathCoordinate(poiStart);
            Xge = pPoi->ConvertPoiToGirderPathCoordinate(poiEnd);

            from_point->put_X(group_offset + Xgs);
            from_point->put_Y(topPrecamberAdjustment + topFlangeThickeningAdjustment);

            to_point->put_X(group_offset + Xge);
            to_point->put_Y(topPrecamberAdjustment + topFlangeThickeningAdjustment);

            from_point->get_X(&x1);
            to_point->get_X(&x2);

            auto dimLine = BuildDimensionLine(pDL, geomUtil::GetPoint(from_point), geomUtil::GetPoint(to_point), x2 - x1);
            dimLine->SetWitnessLength(twip_offset);

            if ( 1 < nSegments )
            {
               // segment length measured to CL of closure joint
               if ( segIdx == 0 )
               {
                  poiStart = pPoi->GetPointOfInterest(segmentKey,0.00);
               }
               else
               {
                  CSegmentKey prevSegmentKey(segmentKey.groupIndex,segmentKey.girderIndex,segmentKey.segmentIndex-1);
                  PoiList vPoi;
                  pPoi->GetPointsOfInterest(prevSegmentKey, POI_CLOSURE, &vPoi);
                  ATLASSERT(vPoi.size() == 1);
                  poiStart = vPoi.front();
               }

               if ( segIdx < nSegments-1 )
               {
                  PoiList vPoi;
                  pPoi->GetPointsOfInterest(segmentKey, POI_CLOSURE, &vPoi);
                  ATLASSERT(vPoi.size() == 1);
                  poiEnd = vPoi.front();
               }
               else
               {
                  poiEnd = pPoi->GetPointOfInterest(segmentKey,segment_length);
               }

               Xgs = pPoi->ConvertPoiToGirderPathCoordinate(poiStart);
               Xge = pPoi->ConvertPoiToGirderPathCoordinate(poiEnd);

               from_point->put_X(group_offset + Xgs);
               from_point->put_Y(topPrecamberAdjustment + topFlangeThickeningAdjustment);

               to_point->put_X(group_offset + Xge);
               to_point->put_Y(topPrecamberAdjustment + topFlangeThickeningAdjustment);

               from_point->get_X(&x1);
               to_point->get_X(&x2);

               auto dimLine = BuildDimensionLine(pDL, geomUtil::GetPoint(from_point), geomUtil::GetPoint(to_point), x2-x1);
               dimLine->SetWitnessLength(3*twip_offset/2);
            }

         //
         // Bottom Dimension Lines
         //

            // CL Brg to CL Brg
            PoiList vPoi;
            pPoi->GetPointsOfInterest(segmentKey, POI_0L | POI_10L | POI_ERECTED_SEGMENT, &vPoi);
            ATLASSERT(vPoi.size() == 2);
            poiStart = vPoi.front();
            poiEnd = vPoi.back();

            Xgs = pPoi->ConvertPoiToGirderPathCoordinate(poiStart);
            Xge = pPoi->ConvertPoiToGirderPathCoordinate(poiEnd);

            from_point->put_X(group_offset + Xgs);
            from_point->put_Y(-HgEnds - bottomPrecamberAdjustment);

            to_point->put_X(group_offset + Xge);
            to_point->put_Y(-HgEnds - bottomPrecamberAdjustment);

            from_point->get_X(&x1);
            to_point->get_X(&x2);

            dimLine = BuildDimensionLine(pDL, geomUtil::GetPoint(from_point), geomUtil::GetPoint(to_point), x2 - x1);
            dimLine->SetWitnessLength(-twip_offset);

            StrandIndexType Nh = pStrandGeometry->GetStrandCount(segmentKey, pgsTypes::Harped);

            Float64 lft_harp, rgt_harp;
            pStrandGeometry->GetHarpingPointLocations(segmentKey, &lft_harp, &rgt_harp);

            if (0 < Nh && pSegment->Strands.GetAdjustableStrandType() != pgsTypes::asStraight)
            {
               // harp locations from end of segment (along top)
               PoiList vPoi;
               pPoi->GetPointsOfInterest(segmentKey, POI_HARPINGPOINT, &vPoi);
               ATLASSERT(vPoi.size() <= 2);
               Float64 Xsp = pPoi->ConvertSegmentCoordinateToSegmentPathCoordinate(segmentKey, 0.0);
               Float64 Xgs = pPoi->ConvertSegmentPathCoordinateToGirderPathCoordinate(segmentKey, Xsp);
               Float64 Xge = pPoi->ConvertPoiToGirderPathCoordinate(vPoi.front());

               from_point->put_X(group_offset + Xgs);
               from_point->put_Y(topPrecamberAdjustment + topFlangeThickeningAdjustment);

               to_point->put_X(group_offset + Xge);
               to_point->put_Y(topPrecamberAdjustment + topFlangeThickeningAdjustment);

               from_point->get_X(&x1);
               to_point->get_X(&x2);

               dimLine = BuildDimensionLine(pDL, geomUtil::GetPoint(from_point), geomUtil::GetPoint(to_point), x2 - x1);
               dimLine->SetWitnessLength(twip_offset / 2);

               Xgs = pPoi->ConvertPoiToGirderPathCoordinate(vPoi.back());
               Xsp = pPoi->ConvertSegmentCoordinateToSegmentPathCoordinate(segmentKey, segment_length);
               Xge = pPoi->ConvertSegmentPathCoordinateToGirderPathCoordinate(segmentKey, Xsp);

               from_point->put_X(group_offset + Xgs);
               from_point->put_Y(topPrecamberAdjustment + topFlangeThickeningAdjustment);

               to_point->put_X(group_offset + Xge);
               to_point->put_Y(topPrecamberAdjustment + topFlangeThickeningAdjustment);

               from_point->get_X(&x1);
               to_point->get_X(&x2);

               dimLine = BuildDimensionLine(pDL, geomUtil::GetPoint(from_point), geomUtil::GetPoint(to_point), x2 - x1);
               dimLine->SetWitnessLength(twip_offset / 2);

               // harp locations from cl bearing (along bottom)
               Float64 end_dist = pBridge->GetSegmentStartEndDistance(segmentKey);
               Xsp = pPoi->ConvertSegmentCoordinateToSegmentPathCoordinate(segmentKey, end_dist);
               Xgs = pPoi->ConvertSegmentPathCoordinateToGirderPathCoordinate(segmentKey, Xsp);
               Xge = pPoi->ConvertPoiToGirderPathCoordinate(vPoi.front());

               from_point->put_X(group_offset + Xgs);
               from_point->put_Y(-HgEnds - bottomPrecamberAdjustment);

               to_point->put_X(group_offset + Xge);
               to_point->put_Y(-HgEnds - bottomPrecamberAdjustment);

               from_point->get_X(&x1);
               to_point->get_X(&x2);

               dimLine = BuildDimensionLine(pDL, geomUtil::GetPoint(from_point), geomUtil::GetPoint(to_point), x2 - x1);
               dimLine->SetWitnessLength(-twip_offset / 2);

               end_dist = pBridge->GetSegmentEndEndDistance(segmentKey);
               Xgs = pPoi->ConvertPoiToGirderPathCoordinate(vPoi.back());
               Xsp = pPoi->ConvertSegmentCoordinateToSegmentPathCoordinate(segmentKey, segment_length - end_dist);
               Xge = pPoi->ConvertSegmentPathCoordinateToGirderPathCoordinate(segmentKey, Xsp);

               from_point->put_X(group_offset + Xgs);
               from_point->put_Y(-HgEnds - bottomPrecamberAdjustment);

               to_point->put_X(group_offset + Xge);
               to_point->put_Y(-HgEnds - bottomPrecamberAdjustment);

               from_point->get_X(&x1);
               to_point->get_X(&x2);

               dimLine = BuildDimensionLine(pDL, geomUtil::GetPoint(from_point), geomUtil::GetPoint(to_point), x2 - x1);
               dimLine->SetWitnessLength(-twip_offset / 2);
            }

            // Cantilevers         
            bool bStartCantilever, bEndCantilever;
            pBridge->ModelCantilevers(segmentKey,&bStartCantilever,&bEndCantilever);
            if ( bStartCantilever )
            {
               vPoi.clear();
               pPoi->GetPointsOfInterest(segmentKey, POI_START_FACE, &vPoi);
               ATLASSERT(vPoi.size() == 1);
               poiStart = vPoi.front();

               vPoi.clear();
               pPoi->GetPointsOfInterest(segmentKey, POI_0L | POI_ERECTED_SEGMENT, &vPoi);
               ATLASSERT(vPoi.size() == 1);
               poiEnd = vPoi.front();

               Xgs = pPoi->ConvertPoiToGirderPathCoordinate(poiStart);
               Xge = pPoi->ConvertPoiToGirderPathCoordinate(poiEnd);

               from_point->put_X(group_offset + Xgs);
               from_point->put_Y(-HgEnds - bottomPrecamberAdjustment);

               to_point->put_X(group_offset + Xge);
               to_point->put_Y(-HgEnds - bottomPrecamberAdjustment);

               from_point->get_X(&x1);
               to_point->get_X(&x2);

               dimLine = BuildDimensionLine(pDL, geomUtil::GetPoint(from_point), geomUtil::GetPoint(to_point), x2-x1);
               dimLine->SetWitnessLength(-twip_offset);
            }

            if ( bEndCantilever )
            {
               vPoi.clear();
               pPoi->GetPointsOfInterest(segmentKey, POI_10L | POI_ERECTED_SEGMENT, &vPoi);
               ATLASSERT(vPoi.size() == 1);
               poiStart = vPoi.front();

               vPoi.clear();
               pPoi->GetPointsOfInterest(segmentKey, POI_END_FACE, &vPoi);
               ATLASSERT(vPoi.size() == 1);
               poiEnd = vPoi.front();

               Xgs = pPoi->ConvertPoiToGirderPathCoordinate(poiStart);
               Xge = pPoi->ConvertPoiToGirderPathCoordinate(poiEnd);

               from_point->put_X(group_offset + Xgs);
               from_point->put_Y(-HgEnds - bottomPrecamberAdjustment);

               to_point->put_X(group_offset + Xge);
               to_point->put_Y(-HgEnds - bottomPrecamberAdjustment);

               from_point->get_X(&x1);
               to_point->get_X(&x2);

               dimLine = BuildDimensionLine(pDL, geomUtil::GetPoint(from_point), geomUtil::GetPoint(to_point), x2-x1);
               dimLine->SetWitnessLength(-twip_offset);
            }

         } // next segment
      }

      if ( 1 < nSegments )
      {
         // Overall length along the top
         CSegmentKey segmentKey(thisGirderKey,0);
         poiStart = pPoi->GetPointOfInterest(segmentKey,0.0);

         segmentKey.segmentIndex = nSegments-1;
         Float64 segment_length = pBridge->GetSegmentLength(segmentKey);
         poiEnd = pPoi->GetPointOfInterest(segmentKey,segment_length);

         Xgs = pPoi->ConvertPoiToGirderPathCoordinate(poiStart);
         Xge = pPoi->ConvertPoiToGirderPathCoordinate(poiEnd);

         from_point->put_X(group_offset + Xgs);
         from_point->put_Y(topPrecamberAdjustment + topFlangeThickeningAdjustment);

         to_point->put_X(group_offset + Xge);
         to_point->put_Y(topPrecamberAdjustment + topFlangeThickeningAdjustment);

         from_point->get_X(&x1);
         to_point->get_X(&x2);

         auto dimLine = BuildDimensionLine(pDL, geomUtil::GetPoint(from_point), geomUtil::GetPoint(to_point), x2-x1);
         dimLine->SetWitnessLength(2*twip_offset);

         // Overall length along the bottom
         segmentKey.segmentIndex = 0;
         PoiList vPoi;
         pPoi->GetPointsOfInterest(segmentKey, POI_0L | POI_ERECTED_SEGMENT, &vPoi);
         ATLASSERT(vPoi.size() == 1);
         poiStart = vPoi.front();

         segmentKey.segmentIndex = nSegments-1;
         vPoi.clear();
         pPoi->GetPointsOfInterest(segmentKey, POI_10L | POI_ERECTED_SEGMENT, &vPoi);
         ATLASSERT(vPoi.size() == 1);
         poiEnd = vPoi.front();

         Xgs = pPoi->ConvertPoiToGirderPathCoordinate(poiStart);
         Xge = pPoi->ConvertPoiToGirderPathCoordinate(poiEnd);

         from_point->put_X(group_offset + Xgs);
         from_point->put_Y(-HgEnds - bottomPrecamberAdjustment);

         to_point->put_X(group_offset + Xge);
         to_point->put_Y(-HgEnds - bottomPrecamberAdjustment);

         from_point->get_X(&x1);
         to_point->get_X(&x2);

         dimLine = BuildDimensionLine(pDL, geomUtil::GetPoint(from_point), geomUtil::GetPoint(to_point), x2-x1);
         dimLine->SetWitnessLength(-2*twip_offset);
      }

      // Update the offset for starting the dimensions lines in the next group
      Float64 group_length = pBridge->GetGirderLayoutLength(thisGirderKey);
      group_offset += group_length;
   } // next group


   // Span lengths along the bottom
   Float64 girderlineOffset = 0;
   if ( girderKey.groupIndex != ALL_GROUPS )
   {
      for ( GroupIndexType grpIdx = 0; grpIdx < girderKey.groupIndex; grpIdx++ )
      {
         CGirderKey gdrKey(girderKey);
         gdrKey.groupIndex = grpIdx;
         gdrKey.girderIndex = Min(girderKey.girderIndex,pBridge->GetGirderCount(grpIdx)-1);
         Float64 group_length = pBridge->GetGirderLayoutLength(gdrKey);
         girderlineOffset -= group_length;
      }
   }

   CGirderKey gdrKey(girderKey);
   if ( girderKey.groupIndex == ALL_GROUPS )
   {
      gdrKey.groupIndex = 0;
      gdrKey.girderIndex = Min(girderKey.girderIndex,pBridge->GetGirderCount(0)-1);
   }

   Float64 brgOffset = pBridge->GetSegmentStartBearingOffset(CSegmentKey(gdrKey,0));
   Float64 endDist   = pBridge->GetSegmentStartEndDistance(CSegmentKey(gdrKey,0));
   Float64 offset = brgOffset - endDist;
   PoiList vPoi;
   pPoi->GetPointsOfInterest(CSegmentKey(girderKey, ALL_SEGMENTS), POI_ABUTMENT | POI_BOUNDARY_PIER | POI_INTERMEDIATE_PIER, &vPoi);
   ATLASSERT(vPoi.size() != 0);
   auto iter(vPoi.begin());
   auto end(vPoi.end());
   poiStart = *iter;
   Xgs = pPoi->ConvertPoiToGirderlineCoordinate(poiStart);
   iter++;
   for ( ; iter != end; iter++ )
   {
      poiEnd = *iter;

      Xge = pPoi->ConvertPoiToGirderlineCoordinate(poiEnd);

      from_point->put_X(girderlineOffset + Xgs + offset);
      from_point->put_Y(-HgEnds - bottomPrecamberAdjustment);

      to_point->put_X(girderlineOffset + Xge + offset);
      to_point->put_Y(-HgEnds - bottomPrecamberAdjustment);

      from_point->get_X(&x1);
      to_point->get_X(&x2);

      auto dimLine = BuildDimensionLine(pDL, geomUtil::GetPoint(from_point), geomUtil::GetPoint(to_point), x2-x1);
      dimLine->SetWitnessLength(-3*twip_offset/2);
      dimLine->SetHiddenWitnessLength(twip_offset);

      poiStart = poiEnd;
      Xgs      = Xge;
   }
}

void CGirderModelElevationView::BuildSectionCutDisplayObjects(CPGSDocBase* pDoc, IBroker* pBroker, const CGirderKey& girderKey,EventIndexType eventIdx)
{
   auto factory = m_pDispMgr->GetDisplayObjectFactory(0);

   auto disp_obj = factory->Create(CSectionCutDisplayImpl::ms_Format,nullptr);

   auto sink = disp_obj->GetEventSink();

   auto point_disp = std::dynamic_pointer_cast<WBFL::DManip::iPointDisplayObject>(disp_obj);
   point_disp->SetToolTipText(_T("Click drag to move section cut"));

   auto sc_strat = std::dynamic_pointer_cast<iSectionCutDrawStrategy>(sink);
   sc_strat->Init(point_disp, pBroker, girderKey, m_pFrame);
   sc_strat->SetColor(CUT_COLOR);

   point_disp->SetID(SECTION_CUT_ID);

   auto pDL = m_pDispMgr->FindDisplayList(SECT_CUT_LIST);
   ATLASSERT(pDL);
   pDL->Clear();
   pDL->AddDisplayObject(disp_obj);
}

void CGirderModelElevationView::BuildStirrupDisplayObjects(CPGSDocBase* pDoc, IBroker* pBroker,const CGirderKey& girderKey,EventIndexType eventIdx)
{
   auto pDL = m_pDispMgr->FindDisplayList(STIRRUP_LIST);
   ATLASSERT(pDL);
   pDL->Clear();

   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   GET_IFACE2(pBroker,IStirrupGeometry,pStirrupGeom);
   GET_IFACE2(pBroker, IPointOfInterest, pPoi);
   GET_IFACE2_NOCHECK(pBroker,IGirder,pIGirder);
   GET_IFACE2_NOCHECK(pBroker, ICamber, pCamber);
   GET_IFACE2_NOCHECK(pBroker, ISectionProperties,pSectProps);

   // assume a typical cover
   Float64 top_cover = WBFL::Units::ConvertToSysUnits(1.0,WBFL::Units::Measure::Inch);
   Float64 bot_cover = WBFL::Units::ConvertToSysUnits(1.0,WBFL::Units::Measure::Inch);

   pgsTypes::SupportedDeckType deckType = pBridge->GetDeckType();
      
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CTimelineManager* pTimelineMgr = pBridgeDesc->GetTimelineManager();

   GroupIndexType startGroupIdx = (girderKey.groupIndex == ALL_GROUPS ? 0 : girderKey.groupIndex);
   GroupIndexType endGroupIdx   = (girderKey.groupIndex == ALL_GROUPS ? pBridgeDesc->GetGirderGroupCount()-1 : startGroupIdx);
   Float64 group_offset = 0;
   for ( GroupIndexType grpIdx = startGroupIdx; grpIdx <= endGroupIdx; grpIdx++ )
   {
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(grpIdx);
      GirderIndexType nGirders = pGroup->GetGirderCount();
      GirderIndexType gdrIdx = Min(girderKey.girderIndex,nGirders-1);
      CGirderKey thisGirderKey(grpIdx,gdrIdx);

      const CSplicedGirderData* pGirder = pGroup->GetGirder(thisGirderKey.girderIndex);

      Float64 running_segment_length = 0; // sum of the segment lengths from segIdx = 0 to current segment
      SegmentIndexType nSegments = pGirder->GetSegmentCount();
      for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
      {
         const CPrecastSegmentData* pSegment = pGirder->GetSegment(segIdx);
         SegmentIDType segID = pSegment->GetID();

         CSegmentKey segmentKey(thisGirderKey,segIdx);

         PoiList vPoi;
         pPoi->GetPointsOfInterest(segmentKey, POI_START_FACE, &vPoi);
         const pgsPointOfInterest& start_poi(vPoi.front());
         Float64 top_flange_thickening_at_start = pIGirder->GetTopFlangeThickening(start_poi);

         Float64 segment_length        = pBridge->GetSegmentLength(segmentKey);
         Float64 segment_layout_length = pBridge->GetSegmentLayoutLength(segmentKey);
         Float64 start_brg_offset = pBridge->GetSegmentStartBearingOffset(segmentKey);
         Float64 start_end_distance = pBridge->GetSegmentStartEndDistance(segmentKey);
         Float64 start_offset = start_brg_offset - start_end_distance;

         // running_segment_length goes to the CL of the closure... adjust the distance
         // so that it goes to the left face of the current segment
         running_segment_length += start_offset;


         if ( eventIdx < pTimelineMgr->GetSegmentErectionEventIndex(segID) )
         {
            // update running segment length
            running_segment_length += segment_layout_length - start_offset;
            continue; // segment not constructed in this event, go to next segment
         }

         PierIndexType startPierIdx = pBridge->GetGirderGroupStartPier(segmentKey.groupIndex);
         Float64 slab_offset = pSectProps->GetStructuralHaunchDepth(start_poi,pgsTypes::hspDetailedDescription); // use for dummy top of stirrup if they are extended into deck

         bool bDoStirrupsEngageDeck = pStirrupGeom->DoStirrupsEngageDeck(segmentKey);


         ZoneIndexType nStirrupZones = pStirrupGeom->GetPrimaryZoneCount(segmentKey);
         for ( ZoneIndexType zoneIdx = 0; zoneIdx < nStirrupZones; zoneIdx++ )
         {
            Float64 start,end;
            pStirrupGeom->GetPrimaryZoneBounds(segmentKey,zoneIdx,&start,&end);

            Float64 zone_length = end - start;

            if (!bDoStirrupsEngageDeck)
            {
               Float64 nHorzInterfaceShearBars = pStirrupGeom->GetPrimaryHorizInterfaceBarCount(segmentKey, zoneIdx);
               bDoStirrupsEngageDeck = (0.0 < nHorzInterfaceShearBars ? true : false);
            }

            WBFL::Materials::Rebar::Size barSize;
            Float64 nStirrups;
            Float64 spacing;
            pStirrupGeom->GetPrimaryVertStirrupBarInfo(segmentKey,zoneIdx,&barSize,&nStirrups,&spacing);

            if ( barSize != WBFL::Materials::Rebar::Size::bsNone && nStirrups != 0 )
            {
               SpacingIndexType nSpacesInZone = SpacingIndexType(floor((zone_length+1.0e-07)/spacing));
               ZoneIndexType nStirrupsInZone = nSpacesInZone+1;

               // Place stirrups in zone as follows:
               Float64 left_offset;
               if ( zoneIdx == 0 )
               {
                  // Left-most zone - shift stirrups so right-most stirrup is at right end of zone
                  Float64 slack = zone_length - nSpacesInZone*spacing; // Amount of extra space outside of stirrups
                  if (slack < 0.0)
                  {
                     ATLASSERT(-1.0e-6<slack); 
                     slack = 0.0;
                  }

                  left_offset = slack;
               }
               else if (zoneIdx == nStirrupZones-1)
               {
                  // Right-most zone - left-most stirrup is at left end of zone
                  left_offset = 0.0;
               }
               else
               {
                  // Interior zones - modify spacing to fit exactly in zone
                  left_offset = 0.0;
                  spacing = zone_length/nSpacesInZone;
               }

               for ( ZoneIndexType i = 0; i < nStirrupsInZone; i++ )
               {
                  Float64 x = start + i*spacing;

                  pgsPointOfInterest poi(segmentKey,x);

                  Float64 precamber = pCamber->GetPrecamber(poi, pgsTypes::pddErected);

                  Float64 top_flange_thickening = pIGirder->GetTopFlangeThickening(poi);

                  Float64 delta_top_flange_thickening = top_flange_thickening - top_flange_thickening_at_start;

                  Float64 Hg = pIGirder->GetHeight(poi);

                  Float64 bottom = precamber + bot_cover - Hg + delta_top_flange_thickening;

                  Float64 top;
                  if ( deckType == pgsTypes::sdtNone || !bDoStirrupsEngageDeck )
                  {
                     top = precamber - top_cover + delta_top_flange_thickening;
                  }
                  else
                  {
                     top = precamber + slab_offset + delta_top_flange_thickening;
                  }

                  WBFL::Geometry::Point2d p1(x + running_segment_length + group_offset,bottom);
                  WBFL::Geometry::Point2d p2(x + running_segment_length + group_offset,top);

                  auto doPnt1 = WBFL::DManip::PointDisplayObject::Create();
                  auto doPnt2 = WBFL::DManip::PointDisplayObject::Create();
                  doPnt1->SetPosition(p1,false, false);
                  doPnt2->SetPosition(p2,false, false);

                  auto c1 = std::dynamic_pointer_cast<WBFL::DManip::iConnectable>(doPnt1);
                  auto c2 = std::dynamic_pointer_cast<WBFL::DManip::iConnectable>(doPnt2);

                  auto s1 = c1->AddSocket(0,p1);
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
                  {
                     ATLASSERT(false);
                  }

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

         // update running segment length
         running_segment_length += segment_layout_length - start_offset;
      } // segment loop

      group_offset += running_segment_length;
   }
}

std::shared_ptr<WBFL::DManip::iDisplayObject> CGirderModelElevationView::BuildLine(std::shared_ptr<WBFL::DManip::iDisplayList> pDL,const std::vector<WBFL::Geometry::Point2d>& points, COLORREF color,UINT nWidth)
{
   PRECONDITION(2 <= points.size());
   
   auto compDO = WBFL::DManip::CompositeDisplayObject::Create();

   auto iter = points.begin();
   auto end = points.end();
   iter++;
   for(;iter != end;iter++)
   {
      auto fromPoint = *(std::prev(iter));
      auto toPoint = *iter;
      auto dispObj = BuildLine(pDL,fromPoint,toPoint,color,nWidth);

      compDO->AddDisplayObject(dispObj);
   }

   pDL->AddDisplayObject(compDO);

   return compDO;
}

std::shared_ptr<WBFL::DManip::iDisplayObject> CGirderModelElevationView::BuildLine(std::shared_ptr<WBFL::DManip::iDisplayList> pDL, const WBFL::Geometry::Point2d& fromPoint, const WBFL::Geometry::Point2d& toPoint, COLORREF color,UINT nWidth)
{
   // put points at locations and make them sockets
   auto from_rep = WBFL::DManip::PointDisplayObject::Create();
   from_rep->SetPosition(fromPoint,false,false);
   from_rep->SetID(m_DisplayObjectID++);
   auto from_connectable = std::dynamic_pointer_cast<WBFL::DManip::iConnectable>(from_rep);
   auto from_socket = from_connectable->AddSocket(0,fromPoint);
   from_rep->Visible(false);
   pDL->AddDisplayObject(from_rep);

   auto to_rep = WBFL::DManip::PointDisplayObject::Create();
   to_rep->SetPosition(toPoint,false,false);
   to_rep->SetID(m_DisplayObjectID++);
   auto to_connectable = std::dynamic_pointer_cast<WBFL::DManip::iConnectable>(to_rep);
   auto to_socket = to_connectable->AddSocket(0,toPoint);
   to_rep->Visible(false);
   pDL->AddDisplayObject(to_rep);

   // Create the dimension line object
   auto line = WBFL::DManip::LineDisplayObject::Create();
   line->SetID(m_DisplayObjectID++);

   // color
   auto pStrategy = line->GetDrawLineStrategy();

   // dangerous cast here
   auto pSimple = std::dynamic_pointer_cast<WBFL::DManip::SimpleDrawLineStrategy>(pStrategy);
   if (pSimple)
   {
      pSimple->SetColor(color);
      pSimple->SetWidth(nWidth);
   }
   else
   {
      ATLASSERT(false);
   }

   // Attach connector to the sockets 
   auto connector = std::dynamic_pointer_cast<WBFL::DManip::iConnector>(line);
   auto startPlug = connector->GetStartPlug();
   auto endPlug = connector->GetEndPlug();

   // connect the line to the sockets
   DWORD dwCookie;
   dwCookie = from_socket->Connect(startPlug);
   dwCookie = to_socket->Connect(endPlug);

   pDL->AddDisplayObject(line);

   return line;
}

std::shared_ptr<WBFL::DManip::iDisplayObject> CGirderModelElevationView::BuildDashLine(std::shared_ptr<WBFL::DManip::iDisplayList> pDL, const WBFL::Geometry::Point2d& fromPoint, const WBFL::Geometry::Point2d& toPoint, COLORREF color1, COLORREF color2)
{
   // put points at locations and make them sockets
   auto from_rep = WBFL::DManip::PointDisplayObject::Create();
   from_rep->SetPosition(fromPoint, false, false);
   from_rep->SetID(m_DisplayObjectID++);
   auto from_connectable = std::dynamic_pointer_cast<WBFL::DManip::iConnectable>(from_rep);
   auto from_socket = from_connectable->AddSocket(0, fromPoint);
   from_rep->Visible(false);
   pDL->AddDisplayObject(from_rep);

   auto to_rep = WBFL::DManip::PointDisplayObject::Create();
   to_rep->SetPosition(toPoint, false, false);
   to_rep->SetID(m_DisplayObjectID++);
   auto to_connectable = std::dynamic_pointer_cast<WBFL::DManip::iConnectable>(to_rep);
   auto to_socket = to_connectable->AddSocket(0, toPoint);
   to_rep->Visible(false);
   pDL->AddDisplayObject(to_rep);

   // Create the dimension line object
   auto line = WBFL::DManip::LineDisplayObject::Create();
   line->SetID(m_DisplayObjectID++);

   // color
   auto strategy = WBFL::DManip::SimpleDrawDashedLineStrategy::Create();
   line->SetDrawLineStrategy(strategy);

   strategy->SetColor1(color1);
   strategy->SetColor2(color2);
   strategy->SetDashLength(10);

   // Attach connector to the sockets 
   auto connector = std::dynamic_pointer_cast<WBFL::DManip::iConnector>(line);
   auto startPlug = connector->GetStartPlug();
   auto endPlug = connector->GetEndPlug();

   // connect the line to the sockets
   DWORD dwCookie;
   dwCookie = from_socket->Connect(startPlug);
   dwCookie = to_socket->Connect(endPlug);

   pDL->AddDisplayObject(line);

   return line;
}

void CGirderModelElevationView::BuildDebondTick(std::shared_ptr<WBFL::DManip::iDisplayList> pDL, const WBFL::Geometry::Point2d& tickPoint,COLORREF color)
{
   // put points at locations and make them sockets
   auto doPnt = WBFL::DManip::PointDisplayObject::Create();
   doPnt->SetPosition(tickPoint,false,false);
   doPnt->SetID(m_DisplayObjectID++);

   auto strategy = WBFL::DManip::SimpleDrawPointStrategy::Create();
   strategy->SetColor(color);
   strategy->SetPointType(WBFL::DManip::PointType::Circle);

   doPnt->SetDrawingStrategy(strategy);

   pDL->AddDisplayObject(doPnt);
}

std::shared_ptr<WBFL::DManip::DimensionLine> CGirderModelElevationView::BuildDimensionLine(std::shared_ptr<WBFL::DManip::iDisplayList> pDL, const WBFL::Geometry::Point2d& fromPoint, const WBFL::Geometry::Point2d& toPoint,Float64 dimension)
{
   // put points at locations and make them sockets
   auto from_rep = WBFL::DManip::PointDisplayObject::Create();
   from_rep->SetPosition(fromPoint, false, false);
   from_rep->SetID(m_DisplayObjectID++);
   auto from_connectable = std::dynamic_pointer_cast<WBFL::DManip::iConnectable>(from_rep);
   auto from_socket = from_connectable->AddSocket(0, fromPoint);
   from_rep->Visible(false);
   pDL->AddDisplayObject(from_rep);

   auto to_rep = WBFL::DManip::PointDisplayObject::Create();
   to_rep->SetPosition(toPoint, false, false);
   to_rep->SetID(m_DisplayObjectID++);
   auto to_connectable = std::dynamic_pointer_cast<WBFL::DManip::iConnectable>(to_rep);
   auto to_socket = to_connectable->AddSocket(0, toPoint);
   to_rep->Visible(false);
   pDL->AddDisplayObject(to_rep);

   // Create the dimension line object
   auto dimLine = WBFL::DManip::DimensionLine::Create();
   dimLine->SetArrowHeadStyle(WBFL::DManip::ArrowHeadStyleType::Filled);

   // Attach connector (the dimension line) to the sockets 
   auto connector = std::dynamic_pointer_cast<WBFL::DManip::iConnector>(dimLine);
   auto startPlug = connector->GetStartPlug();
   auto endPlug = connector->GetEndPlug();

   // connect the line to the sockets
   DWORD dwCookie;
   dwCookie = from_socket->Connect(startPlug);
   dwCookie = to_socket->Connect(endPlug);

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

   // Assign the span id to the dimension line (so they are the same)
   dimLine->SetID(m_DisplayObjectID++);

   pDL->AddDisplayObject(dimLine);

   return dimLine;
}

void CGirderModelElevationView::OnGevCtxEditLoad() 
{
   auto selObjs = m_pDispMgr->GetSelectedObjects();

   if (selObjs.size()==1)
   {
      auto pid = *(selObjs.begin());

      auto pdo = std::dynamic_pointer_cast<WBFL::DManip::iPointDisplayObject>(pid);
      if (pdo!=nullptr)
      {
         auto strategy = pdo->GetDrawingStrategy();

         auto pel = std::dynamic_pointer_cast<iGevEditLoad>(strategy);
         if (pel!=nullptr)
         {
            pel->EditLoad();
            return;
         }
      }
   }

   ATLASSERT(false);
}

void CGirderModelElevationView::OnGevCtxDeleteLoad() 
{
   auto selObjs = m_pDispMgr->GetSelectedObjects();

   if (selObjs.size()==1)
   {
      auto pid = *(selObjs.begin());

      auto pdo = std::dynamic_pointer_cast<WBFL::DManip::iPointDisplayObject>(pid);
      if (pdo!=nullptr)
      {
         auto strategy = pdo->GetDrawingStrategy();
         auto pel = std::dynamic_pointer_cast<iGevEditLoad>(strategy);
         if (pel!=nullptr)
         {
            pel->DeleteLoad();
            return;
         }
      }
   }

   ATLASSERT(false);
}

void CGirderModelElevationView::OnDestroy() 
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

void CGirderModelElevationView::OnDraw(CDC* pDC)
{
   if ( m_bUpdateError )
   {
      AFX_MANAGE_STATE(AfxGetStaticModuleState());

      CString msg;
      AfxFormatString1(msg,IDS_E_UPDATE,m_ErrorMsg.c_str());
      CFont font;
      CFont* pOldFont = nullptr;
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

   if ( m_GirderKey.girderIndex != INVALID_INDEX )
   {
      CDisplayView::OnDraw(pDC);
   }
   else
   {
      ATLASSERT(false); // frame and onupdate should never let this happen
      CString msg(_T("Select a girder to display"));
      CFont font;
      CFont* pOldFont = nullptr;
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

BOOL CGirderModelElevationView::OnMouseWheel(UINT nFlags,short zDelta,CPoint pt)
{
   auto selObjs = m_pDispMgr->GetSelectedObjects();

   if ( selObjs.size() == 0 || selObjs.front()->GetID() != SECTION_CUT_ID )
   {
      return FALSE;
   }

   if ( 0 < zDelta )
   {
      m_pFrame->CutAtPrev();
   }
   else
   {
      m_pFrame->CutAtNext();
   }

   return TRUE;
}

bool CGirderModelElevationView::DidGirderSelectionChange()
{
   const CGirderKey& girderKey = m_pFrame->GetSelection();
   if ( girderKey != m_GirderKey )
   {
      m_GirderKey = girderKey;
      return true;
   }

   return false;
}

CGirderKey CGirderModelElevationView::GetGirderKey()
{
   return m_pFrame->GetSelection();
}

Float64 CGirderModelElevationView::GetSegmentStartLocation(const CSegmentKey& segmentKey)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   GET_IFACE2(pBroker,IBridge,           pBridge);

   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   GroupIndexType startGroupIdx = 0;
   GroupIndexType endGroupIdx   = segmentKey.groupIndex;

   Float64 group_offset = 0;

   for (GroupIndexType grpIdx = startGroupIdx; grpIdx <= endGroupIdx; grpIdx++ )
   {
      const CGirderGroupData* pGroup    = pBridgeDesc->GetGirderGroup(grpIdx);
      const CSplicedGirderData* pGirder = pGroup->GetGirder(segmentKey.girderIndex);
      Float64 running_segment_length = 0; // sum of the segment lengths from segIdx = 0 to current segment in this group
      SegmentIndexType nSegments = (grpIdx < segmentKey.groupIndex ? pGirder->GetSegmentCount() : segmentKey.segmentIndex);
      for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
      {
         CSegmentKey thisSegmentKey(grpIdx,segmentKey.girderIndex,segIdx);

         Float64 segment_layout_length = pBridge->GetSegmentLayoutLength(thisSegmentKey);
         running_segment_length += segment_layout_length;
      } // end of segment loop

      group_offset += running_segment_length;
   } // end of group loop

   return group_offset;
}

Float64 CGirderModelElevationView::GetSpanStartLocation(const CSpanKey& spanKey)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   GET_IFACE2_NOCHECK(pBroker,IBridge,           pBridge);

   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   SpanIndexType startSpanIdx = 0;
   SpanIndexType endSpanIdx   = spanKey.spanIndex;

   Float64 span_offset = 0;

   for (SpanIndexType spanIdx = startSpanIdx; spanIdx < endSpanIdx; spanIdx++ )
   {
      const CSpanData2* pSpan = pBridgeDesc->GetSpan(spanIdx);
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(pSpan);
      GirderIndexType nGirders = pGroup->GetGirderCount();
      GirderIndexType gdrIdx = Min(spanKey.girderIndex,nGirders-1);

      std::vector<std::pair<SegmentIndexType,Float64>> vSegments = pBridge->GetSegmentLengths(CSpanKey(spanIdx,gdrIdx));
      std::vector<std::pair<SegmentIndexType,Float64>>::iterator iter(vSegments.begin());
      std::vector<std::pair<SegmentIndexType,Float64>>::iterator end(vSegments.end());
      for (; iter != end; iter++)
      {
         span_offset += iter->second;
      }

      if (spanIdx == 0)
      {
         CSegmentKey segmentKey(pGroup->GetIndex(), gdrIdx, 0);
         Float64 brgOffset = pBridge->GetSegmentStartBearingOffset(segmentKey);
         span_offset += brgOffset;
      }
   }
   
   return span_offset;
}

CString CGirderModelElevationView::GetSegmentTooltip(IBroker* pBroker, const CSegmentKey& segmentKey)
{
   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   GET_IFACE2(pBroker,IDocumentType,pDocType);
   GET_IFACE2(pBroker,IIntervals,pIntervals);

   IntervalIndexType intervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);

   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   Float64 segment_length = pBridge->GetSegmentLength(segmentKey);
   Float64 span_length    = pBridge->GetSegmentSpanLength(segmentKey);

   CString strGirderName = pBridgeDesc->GetGirderGroup(segmentKey.groupIndex)->GetGirderName(segmentKey.girderIndex);

   CString strMsg1;
   if ( pDocType->IsPGSuperDocument() )
   {
      strMsg1.Format(_T("Girder: %s\r\nGirder Length: %s\r\nSpan Length: %s"),
                     strGirderName,
                     FormatDimension(segment_length,pDisplayUnits->GetSpanLengthUnit()),
                     FormatDimension(span_length,pDisplayUnits->GetSpanLengthUnit())
                     );
   }
   else
   {
      strMsg1.Format(_T("Girder: %s\r\nSegment Length: %s\r\nSpan Length: %s"),
                     strGirderName,
                     FormatDimension(segment_length,pDisplayUnits->GetSpanLengthUnit()),
                     FormatDimension(span_length,pDisplayUnits->GetSpanLengthUnit())
                     );
   }

   Float64 start_conn_length = pBridge->GetSegmentStartEndDistance(segmentKey);
   Float64 end_conn_length   = pBridge->GetSegmentEndEndDistance(segmentKey);
   CString strMsgConn;
   strMsgConn.Format(_T("\r\n\r\nLeft Overhang: %s\r\nRight Overhang: %s"),
                     FormatDimension(start_conn_length,pDisplayUnits->GetComponentDimUnit()),
                     FormatDimension(end_conn_length,pDisplayUnits->GetComponentDimUnit())
                     );

   GET_IFACE2(pBroker,IMaterials,pMaterials);
   Float64 fci = pMaterials->GetSegmentFc(segmentKey,intervalIdx);
   Float64 fc  = pMaterials->GetSegmentFc28(segmentKey);

   CString strMsg2;
   strMsg2.Format(_T("\r\n\r\nf'ci: %s\r\nf'c: %s"),
                  FormatDimension(fci,pDisplayUnits->GetStressUnit()),
                  FormatDimension(fc, pDisplayUnits->GetStressUnit())
                  );

   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeom);
   const auto* pStraightStrand = pMaterials->GetStrandMaterial(segmentKey, pgsTypes::Straight);
   const auto* pHarpedStrand = pMaterials->GetStrandMaterial(segmentKey, pgsTypes::Harped);
   const auto* pTempStrand = pMaterials->GetStrandMaterial(segmentKey,pgsTypes::Temporary);

   StrandIndexType Ns, Nh, Nt, Nsd;
   Ns = pStrandGeom->GetStrandCount(segmentKey,pgsTypes::Straight);
   Nh = pStrandGeom->GetStrandCount(segmentKey,pgsTypes::Harped);
   Nt = pStrandGeom->GetStrandCount(segmentKey,pgsTypes::Temporary);
   Nsd= pStrandGeom->GetNumDebondedStrands(segmentKey,pgsTypes::Straight,pgsTypes::dbetEither);

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

   CString strMsg = strMsg1 + strMsgConn + strMsg2 + strMsg3;

   return strMsg;
}

CString CGirderModelElevationView::GetClosureTooltip(IBroker* pBroker, const CClosureKey& closureKey)
{
   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   GET_IFACE2(pBroker,IIntervals,pIntervals);

   IntervalIndexType intervalIdx = pIntervals->GetCompositeClosureJointInterval(closureKey);

   Float64 length = pBridge->GetClosureJointLength(closureKey);
   CString strMsg1;
   strMsg1.Format(_T("Length: %s"),
                  FormatDimension(length,pDisplayUnits->GetSpanLengthUnit())
                  );

   GET_IFACE2(pBroker,IMaterials,pMaterials);
   Float64 fci = pMaterials->GetClosureJointFc(closureKey,intervalIdx);
   Float64 fc  = pMaterials->GetClosureJointFc28(closureKey);

   CString strMsg2;
   strMsg2.Format(_T("\r\n\r\nf'ci: %s\r\nf'c: %s"),
                  FormatDimension(fci,pDisplayUnits->GetStressUnit()),
                  FormatDimension(fc, pDisplayUnits->GetStressUnit())
                  );

   CString strMsg = strMsg1 + strMsg2;

   return strMsg;
}

void CGirderModelElevationView::GetSpanRange(IBroker* pBroker,const CGirderKey& girderKey,SpanIndexType* pStartSpanIdx,SpanIndexType* pEndSpanIdx)
{
   GET_IFACE2(pBroker,IBridge,pBridge);
   PierIndexType startPierIdx, endPierIdx;
   if ( girderKey.groupIndex == ALL_GROUPS )
   {
      startPierIdx = 0;
      endPierIdx = pBridge->GetPierCount()-1;
   }
   else
   {
      pBridge->GetGirderGroupPiers(girderKey.groupIndex,&startPierIdx,&endPierIdx);
   }

   *pStartSpanIdx = (SpanIndexType)startPierIdx;
   *pEndSpanIdx   = (SpanIndexType)(endPierIdx-1);
}
