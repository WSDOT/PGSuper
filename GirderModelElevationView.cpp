///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2014  Washington State Department of Transportation
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

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\resource.h"
#include "PGSuperAppPlugin\PGSuperApp.h"
#include "PGSuperDocBase.h"
#include "PGSuperDoc.h"
#include "PGSpliceDoc.h"
#include "PGSuperUnits.h"
#include "PGSuperColors.h"
#include "GirderModelElevationView.h"
#include "GirderModelChildFrame.h"
#include "DisplayObjectFactory.h"
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <IFace\DrawBridgeSettings.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\EditByUI.h>
#include <IFace\Intervals.h>
#include <IFace\DocumentType.h>

#include <PgsExt\BridgeDescription2.h>
#include <PgsExt\ClosureJointData.h>

#include "SupportDrawStrategyImpl.h"
#include "PGSuperAppPlugin\TemporarySupportDrawStrategyImpl.h"
#include "SectionCutDrawStrategy.h"
#include "PointLoadDrawStrategyImpl.h"
#include "DistributedLoadDrawStrategyImpl.h"
#include "MomentLoadDrawStrategyImpl.h"
#include <DManip\SimpleDrawLineStrategy.h>
#include "SectionCutDisplayImpl.h"
#include "GMDisplayMgrEventsImpl.h"
#include "GevEditLoad.h"
#include "GirderDropSite.h"

#include "PGSuperColors.h"

#include <sstream>

#include <WBFLDManip.h>
#include <WBFLDManipTools.h>
#include <WBFLGenericBridgeTools.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// display list constants
#define GDR_LIST          1
#define TENDON_LIST       2
#define CP_LIST           3
#define DEBOND_LIST       4
#define STRAND_LIST       5 
#define STRAND_CG_LIST    6
#define SUPPORT_LIST      7
#define DIMLINE_LIST      8
#define SECT_CUT_LIST     9
#define REBAR_LIST       10
#define LOAD_LIST        11
#define STIRRUP_LIST     12
#define DROP_TARGET_LIST 13

// display object ID
#define SECTION_CUT_ID   100


static std::_tstring GetLoadGroupNameForUserLoad(UserLoads::LoadCase lc)
{
   switch(lc)
   {
      case UserLoads::DC:
         return std::_tstring(_T("DC"));
      break;

      case UserLoads::DW:
         return std::_tstring(_T("DW"));
      break;

      case UserLoads::LL_IM:
         return std::_tstring(_T("LL_IM"));
      break;

         default:
         ATLASSERT(false); // SHOULD NEVER GET HERE
   }

   return std::_tstring(_T("Error"));
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

static void CreateLegendEntry(UserLoads::LoadCase lc, iLegendDisplayObject* legend)
{
   COLORREF color = GetLoadGroupColor(lc);
   std::_tstring name =  GetLoadGroupNameForUserLoad(lc);

   CComPtr<iSymbolLegendEntry> legend_entry;
   legend_entry.CoCreateInstance(CLSID_LegendEntry);

   // add entry to legend
   legend_entry->put_Name(CComBSTR(name.c_str()));
   legend_entry->put_Color(color);
   legend_entry->put_DoDrawLine(FALSE);
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
m_GirderKey(0,0)
{
   m_bUpdateError = false;
}

CGirderModelElevationView::~CGirderModelElevationView()
{
}


BEGIN_MESSAGE_MAP(CGirderModelElevationView, CDisplayView)
	//{{AFX_MSG_MAP(CGirderModelElevationView)
	ON_WM_CREATE()
	ON_COMMAND(ID_LEFTEND, OnLeftEnd)
	ON_COMMAND(ID_LEFT_HP, OnLeftHp)
	ON_COMMAND(ID_CENTER, OnCenter)
	ON_COMMAND(ID_RIGHT_HP, OnRightHp)
	ON_COMMAND(ID_RIGHTEND, OnRightEnd)
	ON_COMMAND(ID_USER_CUT, OnUserCut)
	ON_WM_SIZE()
	ON_COMMAND(ID_EDIT_GIRDER, OnEditGirder)
	ON_COMMAND(ID_EDIT_PRESTRESSING, OnEditPrestressing)
	ON_COMMAND(ID_VIEWSETTINGS, OnViewSettings)
	ON_COMMAND(ID_EDIT_STIRRUPS, OnEditStirrups)
	ON_COMMAND(ID_EDIT_LOAD, OnGevCtxEditLoad)
	ON_COMMAND(ID_DELETE_LOAD, OnGevCtxDeleteLoad)
//	ON_WM_CONTEXTMENU()
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
   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   dispMgr->SetSelectionLineColor(SELECTED_OBJECT_LINE_COLOR);
   dispMgr->SetSelectionFillColor(SELECTED_OBJECT_FILL_COLOR);

   CPGSuperDocBase* pDoc = (CPGSuperDocBase*)GetDocument();
   CDisplayObjectFactory* factory = new CDisplayObjectFactory(pDoc);
   CComPtr<iDisplayObjectFactory> doFactory;
   doFactory.Attach((iDisplayObjectFactory*)factory->GetInterface(&IID_iDisplayObjectFactory));
   dispMgr->AddDisplayObjectFactory(doFactory);

   // add factory from in dmaniptools
   CComPtr<iDisplayObjectFactory> pfac2;
   hr = pfac2.CoCreateInstance(CLSID_DManipToolsDisplayObjectFactory);
   ATLASSERT(SUCCEEDED(hr));
   dispMgr->AddDisplayObjectFactory(pfac2);

   // set up default event handler for canvas
   CGMDisplayMgrEventsImpl* pEvents = new CGMDisplayMgrEventsImpl(pDoc, m_pFrame, this, true);
   CComPtr<iDisplayMgrEvents> events;
   events.Attach((iDisplayMgrEvents*)pEvents->GetInterface(&IID_iDisplayMgrEvents));
   dispMgr->RegisterEventSink(events);

   // Create display lists
   // section cut - add first so it's always on top
   CComPtr<iDisplayList> sc_list;
   ::CoCreateInstance(CLSID_DisplayList,NULL,CLSCTX_ALL,IID_iDisplayList,(void**)&sc_list);
   sc_list->SetID(SECT_CUT_LIST);
   dispMgr->AddDisplayList(sc_list);

   // dimension lines
   CComPtr<iDisplayList> dlList;
   ::CoCreateInstance(CLSID_DisplayList,NULL,CLSCTX_ALL,IID_iDisplayList,(void**)&dlList);
   dlList->SetID(DIMLINE_LIST);
   dispMgr->AddDisplayList(dlList);

   // loads
   CComPtr<iDisplayList> load_list;
   ::CoCreateInstance(CLSID_DisplayList,NULL,CLSCTX_ALL,IID_iDisplayList,(void**)&load_list);
   load_list->SetID(LOAD_LIST);
   dispMgr->AddDisplayList(load_list);

   // supports
   CComPtr<iDisplayList> sup_list;
   ::CoCreateInstance(CLSID_DisplayList,NULL,CLSCTX_ALL,IID_iDisplayList,(void**)&sup_list);
   sup_list->SetID(SUPPORT_LIST);
   dispMgr->AddDisplayList(sup_list);

   // drop target
   CComPtr<iDisplayList> dt_list;
   ::CoCreateInstance(CLSID_DisplayList,NULL,CLSCTX_ALL,IID_iDisplayList,(void**)&dt_list);
   dt_list->SetID(DROP_TARGET_LIST);
   dispMgr->AddDisplayList(dt_list);

   // strand cg
   CComPtr<iDisplayList> cg_list;
   ::CoCreateInstance(CLSID_DisplayList,NULL,CLSCTX_ALL,IID_iDisplayList,(void**)&cg_list);
   cg_list->SetID(STRAND_CG_LIST);
   dispMgr->AddDisplayList(cg_list);

   // debond strands
   CComPtr<iDisplayList> db_list;
   ::CoCreateInstance(CLSID_DisplayList,NULL,CLSCTX_ALL,IID_iDisplayList,(void**)&db_list);
   db_list->SetID(DEBOND_LIST);
   dispMgr->AddDisplayList(db_list);

   // strands
   CComPtr<iDisplayList> strand_list;
   ::CoCreateInstance(CLSID_DisplayList,NULL,CLSCTX_ALL,IID_iDisplayList,(void**)&strand_list);
   strand_list->SetID(STRAND_LIST);
   dispMgr->AddDisplayList(strand_list);

   // rebar
   CComPtr<iDisplayList> rb_list;
   ::CoCreateInstance(CLSID_DisplayList,NULL,CLSCTX_ALL,IID_iDisplayList,(void**)&rb_list);
   rb_list->SetID(REBAR_LIST);
   dispMgr->AddDisplayList(rb_list);

   // tendons
   CComPtr<iDisplayList> ts_list;
   ::CoCreateInstance(CLSID_DisplayList,NULL,CLSCTX_ALL,IID_iDisplayList,(void**)&ts_list);
   ts_list->SetID(TENDON_LIST);
   dispMgr->AddDisplayList(ts_list);

   // strirrups
   CComPtr<iDisplayList> stirrups_list;
   ::CoCreateInstance(CLSID_DisplayList,NULL,CLSCTX_ALL,IID_iDisplayList,(void**)&stirrups_list);
   stirrups_list->SetID(STIRRUP_LIST);
   dispMgr->AddDisplayList(stirrups_list);

   // closure joint
   CComPtr<iDisplayList> cp_list;
   ::CoCreateInstance(CLSID_DisplayList,NULL,CLSCTX_ALL,IID_iDisplayList,(void**)&cp_list);
   cp_list->SetID(CP_LIST);
   dispMgr->AddDisplayList(cp_list);

   // girder
   CComPtr<iDisplayList> gdr_list;
   ::CoCreateInstance(CLSID_DisplayList,NULL,CLSCTX_ALL,IID_iDisplayList,(void**)&gdr_list);
   gdr_list->SetID(GDR_LIST);
   dispMgr->AddDisplayList(gdr_list);

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
      return;

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
      CComPtr<iDisplayMgr> dispMgr;
      GetDisplayMgr(&dispMgr);

      CComPtr<iDisplayList> pDL;
      dispMgr->FindDisplayList(SECT_CUT_LIST,&pDL);
      ATLASSERT(pDL);

      CComPtr<iDisplayObject> dispObj;
      pDL->FindDisplayObject(SECTION_CUT_ID,&dispObj);
      ATLASSERT(dispObj);

      CComPtr<iDisplayObjectEvents> sink;
      dispObj->GetEventSink(&sink);

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
   // get the display manager
   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   // clean out all the display objects
   dispMgr->ClearDisplayObjects();

   CPGSuperDocBase* pDoc = (CPGSuperDocBase*)GetDocument();

   EventIndexType eventIdx = m_pFrame->GetEvent();

   // Grab hold of the broker so we can pass it as a parameter
   CComPtr<IBroker> pBroker;
   pDoc->GetBroker(&pBroker);

   UINT settings = pDoc->GetGirderEditorSettings();
   
   CGirderKey girderKey(GetGirderKey());

   BuildSupportDisplayObjects(     pDoc, pBroker, girderKey, eventIdx, dispMgr);
   BuildDropTargetDisplayObjects(  pDoc, pBroker, girderKey, eventIdx, dispMgr);
   BuildSegmentDisplayObjects(     pDoc, pBroker, girderKey, eventIdx, dispMgr);
   BuildClosureJointDisplayObjects( pDoc, pBroker, girderKey, eventIdx, dispMgr);

   if (settings & IDG_EV_SHOW_STRANDS)
   {
      BuildStrandDisplayObjects(pDoc, pBroker, girderKey, eventIdx, dispMgr);
      BuildTendonDisplayObjects(pDoc, pBroker, girderKey, eventIdx, dispMgr);
   }

   if (settings & IDG_EV_SHOW_PS_CG)
   {
      BuildStrandCGDisplayObjects(pDoc, pBroker, girderKey, eventIdx, dispMgr);
   }

   if (settings & IDG_EV_SHOW_LONG_REINF)
   {
      BuildRebarDisplayObjects(pDoc, pBroker, girderKey, eventIdx, dispMgr);
   }

   if (settings & IDG_EV_SHOW_STIRRUPS)
   {
      BuildStirrupDisplayObjects(pDoc, pBroker, girderKey, eventIdx, dispMgr);
   }

   bool cases_exist[3] = {false,false,false};
   if (settings & IDG_EV_SHOW_LOADS)
   {
      BuildPointLoadDisplayObjects(      pDoc, pBroker, girderKey, eventIdx, dispMgr, cases_exist);
      BuildDistributedLoadDisplayObjects(pDoc, pBroker, girderKey, eventIdx, dispMgr, cases_exist);
#pragma Reminder("UPDATE: display object for moment loads not working")
      //BuildMomentLoadDisplayObjects(pDoc, pBroker, girderKey, dispMgr, cases_exist);
   }

   if (settings & IDG_EV_SHOW_DIMENSIONS)
   {
      BuildDimensionDisplayObjects(pDoc, pBroker, girderKey, eventIdx, dispMgr);
   }

   BuildSectionCutDisplayObjects(pDoc, pBroker, girderKey, eventIdx, dispMgr);

   // Legend must be displayed last so we can place it relative to bounding box
   if (settings & IDG_EV_SHOW_LEGEND && settings & IDG_EV_SHOW_LOADS)
   {
      BuildLegendDisplayObjects(pDoc, pBroker, girderKey, eventIdx, dispMgr, cases_exist);
   }

   DManip::MapMode mode = (settings & IDG_EV_DRAW_ISOTROPIC) ? DManip::Isotropic : DManip::Anisotropic;
   CDisplayView::SetMappingMode(mode);
}

void CGirderModelElevationView::DoPrint(CDC* pDC, CPrintInfo* pInfo)
{
   this->OnBeginPrinting(pDC, pInfo);
   this->OnPrepareDC(pDC);
   this->ScaleToFit();
   this->OnDraw(pDC);
   this->OnEndPrinting(pDC, pInfo);
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
		return -1;
	
   m_pFrame = (CGirderModelChildFrame*)GetParent()->GetParent();
   ASSERT( m_pFrame != 0 );
   ASSERT( m_pFrame->IsKindOf( RUNTIME_CLASS( CGirderModelChildFrame ) ) );

	return 0;
} 

DROPEFFECT CGirderModelElevationView::CanDrop(COleDataObject* pDataObject,DWORD dwKeyState,IPoint2d* point)
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
      CComPtr<iDragDataSource> source;               
      ::CoCreateInstance(CLSID_DragDataSource,NULL,CLSCTX_ALL,IID_iDragDataSource,(void**)&source);
      source->SetDataObject(pDataObject);
      source->PrepareFormat(CSectionCutDisplayImpl::ms_Format);

      CWinThread* thread = ::AfxGetThread( );
      DWORD threadid = thread->m_nThreadID;

      DWORD threadl;
      // know (by voodoo) that the first member of this data source is the thread id
      source->Read(CSectionCutDisplayImpl::ms_Format,&threadl,sizeof(DWORD));

      if (threadl == threadid)
        return DROPEFFECT_MOVE;
   }
   else
   {
      if (m_Legend)
      {
         CComQIPtr<iDraggable> drag(m_Legend);
         UINT format = drag->Format();
         if ( pDataObject->IsDataAvailable(format) )
            return DROPEFFECT_MOVE;
      }
   }

   return DROPEFFECT_NONE;
}

void CGirderModelElevationView::OnDropped(COleDataObject* pDataObject,DROPEFFECT dropEffect,IPoint2d* point)
{
}

pgsPointOfInterest CGirderModelElevationView::GetCutLocation()
{
   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   CComPtr<iDisplayList> pDL;
   dispMgr->FindDisplayList(SECT_CUT_LIST,&pDL);
   ATLASSERT(pDL);

   CComPtr<iDisplayObject> dispObj;
   pDL->FindDisplayObject(SECTION_CUT_ID,&dispObj);

   if ( dispObj == NULL )
      return pgsPointOfInterest();

   CComPtr<iDisplayObjectEvents> sink;
   dispObj->GetEventSink(&sink);

   CComQIPtr<iPointDisplayObject,&IID_iPointDisplayObject> point_disp(dispObj);
   CComQIPtr<iSectionCutDrawStrategy,&IID_iSectionCutDrawStrategy> sc_strat(sink);

   return sc_strat->GetCutPOI(m_pFrame->GetCurrentCutLocation());
}

void CGirderModelElevationView::OnLeftEnd() 
{
   m_pFrame->CutAtLeftEnd();
}

void CGirderModelElevationView::OnLeftHp() 
{
   m_pFrame->CutAtLeftHp();
}

void CGirderModelElevationView::OnCenter() 
{
   m_pFrame->CutAtCenter();
}

void CGirderModelElevationView::OnRightHp() 
{
   m_pFrame->CutAtRightHp();
}

void CGirderModelElevationView::OnRightEnd() 
{
   m_pFrame->CutAtRightEnd();
}

void CGirderModelElevationView::OnUserCut() 
{
	m_pFrame->CutAtLocation();
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

      CComPtr<iDisplayMgr> dispMgr;
      GetDisplayMgr(&dispMgr);

      SetLogicalViewRect(MM_TEXT,rect);

      SetScrollSizes(MM_TEXT,size,CScrollView::sizeDefault,CScrollView::sizeDefault);

      ScaleToFit();
   }
}

void CGirderModelElevationView::OnEditGirder() 
{
   if ( GetDocument()->IsKindOf(RUNTIME_CLASS(CPGSuperDoc)) )
   {
      pgsPointOfInterest poi(m_pFrame->GetCutLocation());
      ((CPGSuperDoc*)GetDocument())->EditGirderSegmentDescription(poi.GetSegmentKey(),EGD_GENERAL);
   }
   else
   {
      pgsPointOfInterest poi(m_pFrame->GetCutLocation());
      ((CPGSpliceDoc*)GetDocument())->EditGirderDescription(poi.GetSegmentKey(),EGS_GENERAL);
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
   ((CPGSuperDocBase*)GetDocument())->EditGirderSegmentDescription(poi.GetSegmentKey(),page);
}

void CGirderModelElevationView::OnEditStirrups() 
{
   int page = EGS_STIRRUPS;
   if ( GetDocument()->IsKindOf(RUNTIME_CLASS(CPGSuperDoc)) )
   {
      page = EGD_STIRRUPS;
   }

   pgsPointOfInterest poi = GetCutLocation();
   ((CPGSuperDocBase*)GetDocument())->EditGirderSegmentDescription(poi.GetSegmentKey(),page);
}

void CGirderModelElevationView::OnViewSettings() 
{
	((CPGSuperDocBase*)GetDocument())->EditGirderViewSettings(VS_GIRDER_ELEVATION);
}

void CGirderModelElevationView::CreateSegmentEndSupportDisplayObject(Float64 groupOffset,const CPrecastSegmentData* pSegment,pgsTypes::MemberEndType endType,EventIndexType eventIdx,const CTimelineManager* pTimelineMgr,iDisplayList* pDL)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,ISectionProperties,pSectProp);
   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IPointOfInterest,pIPoi);

   const CSegmentKey& segmentKey(pSegment->GetSegmentKey());

   const CClosureJointData* pClosure = (endType == pgsTypes::metStart ? pSegment->GetLeftClosure() : pSegment->GetRightClosure());
   const CPierData2* pPier = NULL;
   const CTemporarySupportData* pTS = NULL;

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

   bool* pbIsPier = NULL;
   Float64 pierLocation;
   Float64 sectionHeight;
   IDType ID;
   if ( pPier )
   {
      PierIndexType pierIdx = pPier->GetIndex();
      EventIndexType erectionEventIdx = pTimelineMgr->GetPierErectionEventIndex(pierIdx);
      if ( eventIdx < erectionEventIdx )
         return; // pier is not erected in this event

      pbIsPier = new bool;
      *pbIsPier = true;
      pierLocation = pBridge->GetPierLocation(pierIdx,segmentKey.girderIndex);
      ID = (IDType)pierIdx;

      sectionHeight = pSectProp->GetSegmentHeightAtPier(segmentKey,pierIdx);
   }
   else
   {
      ATLASSERT(pTS != NULL);

      EventIndexType erectionEventIdx, removalEventIdx;
      pTimelineMgr->GetTempSupportEvents(pTS->GetID(),&erectionEventIdx,&removalEventIdx);
      if ( eventIdx < erectionEventIdx || removalEventIdx <= eventIdx )
         return;

      pbIsPier = new bool;
      *pbIsPier = false;

      pierLocation = pBridge->GetTemporarySupportLocation(pTS->GetIndex(),segmentKey.girderIndex);

      ID = pTS->GetID();

      GET_IFACE2(pBroker,IPointOfInterest,pIPoi);
      GET_IFACE2(pBroker,IIntervals,pIntervals);
      IntervalIndexType intervalIdx = pIntervals->GetInterval(eventIdx);
      PoiAttributeType poiReference = (pIntervals->GetErectSegmentInterval(segmentKey) <= intervalIdx ? POI_ERECTED_SEGMENT : POI_RELEASED_SEGMENT);
      PoiAttributeType attribute = (endType == pgsTypes::metStart ? POI_0L : POI_10L);
      std::vector<pgsPointOfInterest> vPoi(pIPoi->GetPointsOfInterest(segmentKey,poiReference | attribute,POIFIND_AND));
      ATLASSERT(vPoi.size() == 1);
      pgsPointOfInterest poiCLBrg(vPoi.front());

      sectionHeight = pSectProp->GetHg(pIntervals->GetPrestressReleaseInterval(segmentKey),poiCLBrg);
   }

   pierLocation = pIPoi->ConvertGirderPathCoordinateToGirderCoordinate(segmentKey,pierLocation);

   CComPtr<IPoint2d> point;
   point.CoCreateInstance(CLSID_Point2d);
   point->Move(pierLocation-groupOffset,-sectionHeight);

   GET_IFACE2(pBroker,IGirder,pIGirder);
   const CSplicedGirderData* pGirder = pSegment->GetGirder();
   const CGirderGroupData* pGroup = pGirder->GetGirderGroup();
   Float64 brgOffsetStart, brgOffsetEnd;
   pIGirder->GetSegmentBearingOffset(segmentKey,&brgOffsetStart,&brgOffsetEnd);
   if ( segmentKey.groupIndex == 0 && segmentKey.segmentIndex == 0 && endType == pgsTypes::metStart )
   {
      point->Offset(brgOffsetStart,0);
   }

   if ( segmentKey.groupIndex == pBridge->GetGirderGroupCount()-1 && segmentKey.segmentIndex == pGirder->GetSegmentCount()-1 && endType == pgsTypes::metEnd )
   {
      point->Offset(-brgOffsetEnd,0);
   }

   // create display object
   CComPtr<iPointDisplayObject> ptDispObj;
   ::CoCreateInstance(CLSID_PointDisplayObject,NULL,CLSCTX_ALL,IID_iPointDisplayObject,(void**)&ptDispObj);

   ptDispObj->SetItemData((void*)pbIsPier,true);

   // create drawing strategy
   IUnknown* unk;
   if ( pPier )
   {
      CSupportDrawStrategyImpl* pDrawStrategy = new CSupportDrawStrategyImpl();
      unk = pDrawStrategy->GetInterface(&IID_iDrawPointStrategy);
   }
   else
   {
      Float64 temp, leftBrgOffset, rightBrgOffset;
      pIGirder->GetSegmentBearingOffset(segmentKey,&temp,&leftBrgOffset);
      pIGirder->GetSegmentBearingOffset(CSegmentKey(segmentKey.groupIndex,segmentKey.girderIndex,segmentKey.segmentIndex+1),&rightBrgOffset,&temp);
      CTemporarySupportDrawStrategyImpl* pDrawStrategy = new CTemporarySupportDrawStrategyImpl(pTS->GetSupportType(),leftBrgOffset,rightBrgOffset);
      unk = pDrawStrategy->GetInterface(&IID_iDrawPointStrategy);
   }

   ptDispObj->SetDrawingStrategy((iDrawPointStrategy*)unk);
   unk->Release();

   CComQIPtr<iPointDisplayObject,&IID_iPointDisplayObject> supportRep(ptDispObj);
   supportRep->SetPosition(point,FALSE,FALSE);
   supportRep->SetID( ID );

   pDL->AddDisplayObject(supportRep);
}

void CGirderModelElevationView::CreateIntermediatePierDisplayObject(Float64 groupOffset,const CPrecastSegmentData* pSegment,EventIndexType eventIdx,const CTimelineManager* pTimelineMgr,iDisplayList* pDL)
{
   const CSpanData2* pStartSpan = pSegment->GetSpan(pgsTypes::metStart);
   const CSpanData2* pEndSpan   = pSegment->GetSpan(pgsTypes::metEnd);

   if ( pStartSpan == pEndSpan )
      return; // no intermediate pier

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,ISectionProperties,pSectProp);
   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IGirder,pGirder);
   GET_IFACE2(pBroker,IPointOfInterest,pIPoi);

   const CSpanData2* pSpan = pStartSpan;
   bool bDone = false;
   while ( !bDone )
   {
      const CPierData2* pPier = pSpan->GetNextPier();

      PierIndexType pierIdx = pPier->GetIndex();

      EventIndexType erectionEventIdx = pTimelineMgr->GetPierErectionEventIndex(pierIdx);
      if ( erectionEventIdx <= eventIdx )
      {
         CSegmentKey segmentKey(pSpan->GetBridgeDescription()->GetGirderGroup(pSpan)->GetIndex(),
                                pSegment->GetGirder()->GetIndex(),
                                pSegment->GetIndex());

         Float64 sectionHeight = pSectProp->GetSegmentHeightAtPier(segmentKey,pierIdx);
         Float64 pierLocation = pBridge->GetPierLocation(pierIdx,segmentKey.girderIndex);
         pierLocation = pIPoi->ConvertGirderPathCoordinateToGirderCoordinate(segmentKey,pierLocation);

         CComPtr<IPoint2d> point;
         point.CoCreateInstance(CLSID_Point2d);
         point->Move(pierLocation - groupOffset,-sectionHeight);

         // create display object
         CComPtr<iPointDisplayObject> ptDispObj;
         ::CoCreateInstance(CLSID_PointDisplayObject,NULL,CLSCTX_ALL,IID_iPointDisplayObject,(void**)&ptDispObj);

         bool* pbIsPier = new bool;
         *pbIsPier = true;
         ptDispObj->SetItemData((void*)pbIsPier,true);

         // create drawing strategy
         CSupportDrawStrategyImpl* pDrawStrategy = new CSupportDrawStrategyImpl();
         IUnknown* unk = pDrawStrategy->GetInterface(&IID_iDrawPointStrategy);

         ptDispObj->SetDrawingStrategy((iDrawPointStrategy*)unk);
         unk->Release();

         CComQIPtr<iPointDisplayObject,&IID_iPointDisplayObject> supportRep(ptDispObj);
         supportRep->SetPosition(point,FALSE,FALSE);
         supportRep->SetID( pierIdx );

         pDL->AddDisplayObject(supportRep);
      }

      pSpan = pSpan->GetNextPier()->GetNextSpan();
      if ( pSpan == pEndSpan )
      {
         bDone = true;
      }
   }
}

void CGirderModelElevationView::CreateIntermediateTemporarySupportDisplayObject(Float64 groupOffset,const CPrecastSegmentData* pSegment,EventIndexType eventIdx,const CTimelineManager* pTimelineMgr,iDisplayList* pDL)
{
   const CSpanData2* pStartSpan = pSegment->GetSpan(pgsTypes::metStart);
   const CSpanData2* pEndSpan   = pSegment->GetSpan(pgsTypes::metEnd);

   std::vector<const CTemporarySupportData*> tempSupports(pStartSpan->GetTemporarySupports());
   std::vector<const CTemporarySupportData*> endTempSupports(pEndSpan->GetTemporarySupports());
   tempSupports.insert(tempSupports.begin(),endTempSupports.begin(),endTempSupports.end());

   if ( tempSupports.size() == 0 )
      return; // no temporary supports

   Float64 segment_start_station, segment_end_station;
   pSegment->GetStations(&segment_start_station,&segment_end_station);
   std::vector<const CTemporarySupportData*>::iterator iter(tempSupports.begin());
   std::vector<const CTemporarySupportData*>::iterator iterEnd(tempSupports.end());
   for ( ; iter != iterEnd; iter++ )
   {
      const CTemporarySupportData* pTS = *iter;
      Float64 ts_station = pTS->GetStation();
      if ( ::IsEqual(segment_start_station,ts_station) || ::IsEqual(segment_end_station,ts_station) )
         continue; // temporary support display objects already created when creating DO's at ends of segment

      if ( ::InRange(segment_start_station,ts_station,segment_end_station) )
      {
         EventIndexType erectionEventIdx, removalEventIdx;
         pTimelineMgr->GetTempSupportEvents(pTS->GetID(),&erectionEventIdx,&removalEventIdx);
         if ( eventIdx < erectionEventIdx || removalEventIdx <= eventIdx )
            return; // temp support does not exist in this event

         CComPtr<IBroker> pBroker;
         EAFGetBroker(&pBroker);
         GET_IFACE2(pBroker,ISectionProperties,pSectProp);
         GET_IFACE2(pBroker,IBridge,pBridge);
         GET_IFACE2(pBroker,IPointOfInterest,pIPoi);

         CSegmentKey segmentKey(pStartSpan->GetBridgeDescription()->GetGirderGroup(pStartSpan)->GetIndex(),
                                pSegment->GetGirder()->GetIndex(),
                                pSegment->GetIndex());

         Float64 sectionHeight = pSectProp->GetSegmentHeightAtTemporarySupport(segmentKey,pTS->GetIndex());
         Float64 pierLocation = pBridge->GetTemporarySupportLocation(pTS->GetIndex(),segmentKey.girderIndex);
         pierLocation = pIPoi->ConvertGirderPathCoordinateToGirderCoordinate(segmentKey,pierLocation);

         CComPtr<IPoint2d> point;
         point.CoCreateInstance(CLSID_Point2d);
         point->Move(pierLocation-groupOffset,-sectionHeight);

         // create display object
         CComPtr<iPointDisplayObject> ptDispObj;
         ::CoCreateInstance(CLSID_PointDisplayObject,NULL,CLSCTX_ALL,IID_iPointDisplayObject,(void**)&ptDispObj);

         // create drawing strategy
         CTemporarySupportDrawStrategyImpl* pDrawStrategy = new CTemporarySupportDrawStrategyImpl(pTS->GetSupportType(),sectionHeight/4,sectionHeight/4);
         IUnknown* unk = pDrawStrategy->GetInterface(&IID_iDrawPointStrategy);

         ptDispObj->SetDrawingStrategy((iDrawPointStrategy*)unk);
         unk->Release();

         CComQIPtr<iPointDisplayObject,&IID_iPointDisplayObject> supportRep(ptDispObj);
         supportRep->SetPosition(point,FALSE,FALSE);
         supportRep->SetID( pTS->GetID() );

         pDL->AddDisplayObject(supportRep);
      }
   }
}

void CGirderModelElevationView::BuildSupportDisplayObjects(CPGSuperDocBase* pDoc, IBroker* pBroker,const CGirderKey& girderKey,EventIndexType eventIdx,iDisplayMgr* dispMgr)
{
   CComPtr<iDisplayList> pDL;
   dispMgr->FindDisplayList(SUPPORT_LIST,&pDL);
   ATLASSERT(pDL);
   pDL->Clear();

   GET_IFACE2(pBroker,IBridge,pBridge);

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CTimelineManager* pTimelineMgr = pBridgeDesc->GetTimelineManager();

   GroupIndexType startGroupIdx = (girderKey.groupIndex == ALL_GROUPS ? 0 : girderKey.groupIndex);
   GroupIndexType endGroupIdx   = (girderKey.groupIndex == ALL_GROUPS ? pBridgeDesc->GetGirderGroupCount()-1 : startGroupIdx);

   PierIndexType startPierIdx = pBridge->GetGirderGroupStartPier(startGroupIdx);
   Float64 groupOffset = pBridge->GetPierLocation(startPierIdx,girderKey.girderIndex);

   for ( GroupIndexType grpIdx = startGroupIdx; grpIdx <= endGroupIdx; grpIdx++ )
   {
      CGirderKey gdrKey(girderKey);
      gdrKey.groupIndex = grpIdx;

      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(grpIdx);
      const CSplicedGirderData* pGirder = pGroup->GetGirder(girderKey.girderIndex);
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
   } // group loop
}

void CGirderModelElevationView::BuildDropTargetDisplayObjects(CPGSuperDocBase* pDoc, IBroker* pBroker, const CGirderKey& girderKey, EventIndexType eventIdx, iDisplayMgr* dispMgr)
{
   CComPtr<iDisplayList> pSupportDisplayList;
   dispMgr->FindDisplayList(SUPPORT_LIST,&pSupportDisplayList);
   ATLASSERT(pSupportDisplayList);

   CComPtr<iDisplayList> pDL;
   dispMgr->FindDisplayList(DROP_TARGET_LIST,&pDL);
   ATLASSERT(pDL);
   pDL->Clear();

   COLORREF color = HOTPINK1;

   SpanIndexType spanIdx;
   CComPtr<IPoint2d> p1, p2;
   CollectionIndexType nItems = pSupportDisplayList->GetDisplayObjectCount();
   for ( CollectionIndexType idx = 0; idx < nItems; idx++ )
   {
      CComPtr<iDisplayObject> pDO;
      pSupportDisplayList->GetDisplayObject(idx,&pDO);

      CComQIPtr<iPointDisplayObject> doPoint(pDO);
      ATLASSERT(doPoint != NULL);

      bool* pbIsPier;
      pDO->GetItemData((void**)&pbIsPier);

      if ( pbIsPier && *pbIsPier )
      {
         if ( p1 == NULL )
         {
            CComPtr<IPoint2d> p;
            doPoint->GetPosition(&p);
            p->Clone(&p1);
            p1->put_Y(0.0);

            spanIdx = pDO->GetID();
         }
         else
         {
            CComPtr<IPoint2d> p;
            doPoint->GetPosition(&p);
            p->Clone(&p2);
            p2->put_Y(0.0);
         }

         if ( p1 != NULL && p2 != NULL )
         {
            CComPtr<iDisplayObject> line;
            BuildLine(pDL,p1,p2,color,&line);

            line->Visible(FALSE);

            // create a drop site for drag and drop loads
            CGirderDropSite* pDropSite = new CGirderDropSite(pDoc, CSpanGirderKey(spanIdx,girderKey.girderIndex), m_pFrame);
            CComPtr<iDropSite> dropSite;
            dropSite.Attach((iDropSite*)pDropSite->GetInterface(&IID_iDropSite));
            line->RegisterDropSite(dropSite);

            p1.Release();
            p1 = p2;
            p2.Release();
            spanIdx++;
         }
      }
   }
}

void CGirderModelElevationView::BuildSegmentDisplayObjects(CPGSuperDocBase* pDoc,IBroker* pBroker, const CGirderKey& girderKey,EventIndexType eventIdx,iDisplayMgr* dispMgr)
{
   // get the display list and clear out any old display objects
   CComPtr<iDisplayList> pDL;
   dispMgr->FindDisplayList(GDR_LIST,&pDL);
   ATLASSERT(pDL);
   pDL->Clear();

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CTimelineManager* pTimelineMgr = pBridgeDesc->GetTimelineManager();

   GroupIndexType startGroupIdx = (girderKey.groupIndex == ALL_GROUPS ? 0 : girderKey.groupIndex);
   GroupIndexType endGroupIdx   = (girderKey.groupIndex == ALL_GROUPS ? pBridgeDesc->GetGirderGroupCount()-1 : startGroupIdx);

   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IGirder,pIGirder);
   Float64 group_offset = 0;
  
   for ( GroupIndexType grpIdx = startGroupIdx; grpIdx <= endGroupIdx; grpIdx++ )
   {
      CGirderKey gdrKey(girderKey);
      gdrKey.groupIndex = grpIdx;

      Float64 running_segment_length = 0;

      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(grpIdx);
      const CSplicedGirderData* pGirder = pGroup->GetGirder(gdrKey.girderIndex);
      SegmentIndexType nSegments = pGirder->GetSegmentCount();
      for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
      {
         const CPrecastSegmentData* pSegment = pGirder->GetSegment(segIdx);
         SegmentIDType segID = pSegment->GetID();

         CSegmentKey segmentKey(gdrKey.groupIndex,gdrKey.girderIndex,segIdx);

         Float64 segment_layout_length = pBridge->GetSegmentLayoutLength(segmentKey);
         running_segment_length += segment_layout_length;

         // create display objects for each segment, but only if it is erected in this event
         // or has been erected in a previous event
         EventIndexType erectSegmentEventIdx = pTimelineMgr->GetSegmentErectionEventIndex(segID);
         if ( erectSegmentEventIdx <= eventIdx )
         {
            CComPtr<IShape> shape;
            pIGirder->GetSegmentProfile(segmentKey,false,&shape);

            // create the display object
            CComPtr<iPointDisplayObject> doPnt;
            ::CoCreateInstance(CLSID_PointDisplayObject,NULL,CLSCTX_ALL,IID_iPointDisplayObject,(void**)&doPnt);

            CComQIPtr<IXYPosition> position(shape);
            position->Offset(group_offset,0);
            CComPtr<IPoint2d> pnt;
            position->get_LocatorPoint(lpTopLeft,&pnt);
            doPnt->SetPosition(pnt,FALSE,FALSE);

            // create the drawing strategy
            CComPtr<iShapeDrawStrategy> strategy;
            ::CoCreateInstance(CLSID_ShapeDrawStrategy,NULL,CLSCTX_ALL,IID_iShapeDrawStrategy,(void**)&strategy);
            doPnt->SetDrawingStrategy(strategy);

            // configure the strategy
            strategy->SetShape(shape);
            strategy->SetSolidLineColor(SEGMENT_BORDER_COLOR);
            strategy->SetSolidFillColor(SEGMENT_FILL_COLOR);
            strategy->SetVoidLineColor(VOID_BORDER_COLOR);
            strategy->SetVoidFillColor(GetSysColor(COLOR_WINDOW));
            strategy->DoFill(true);

            CString strMsg(GetSegmentTooltip(pBroker,segmentKey));
            doPnt->SetMaxTipWidth(TOOLTIP_WIDTH);
            doPnt->SetTipDisplayTime(TOOLTIP_DURATION);
            doPnt->SetToolTipText(strMsg);

            // put the display object in its display list
            pDL->AddDisplayObject(doPnt);
         }
      }

      group_offset += running_segment_length;
   } // group loop
}

void CGirderModelElevationView::BuildClosureJointDisplayObjects(CPGSuperDocBase* pDoc, IBroker* pBroker, const CGirderKey& girderKey,EventIndexType eventIdx,iDisplayMgr* dispMgr)
{
   // get the display list and clear out any old display objects
   CComPtr<iDisplayList> pDL;
   dispMgr->FindDisplayList(CP_LIST,&pDL);
   ATLASSERT(pDL);
   pDL->Clear();

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CTimelineManager* pTimelineMgr = pBridgeDesc->GetTimelineManager();

   GroupIndexType startGroupIdx = (girderKey.groupIndex == ALL_GROUPS ? 0 : girderKey.groupIndex);
   GroupIndexType endGroupIdx   = (girderKey.groupIndex == ALL_GROUPS ? pBridgeDesc->GetGirderGroupCount()-1 : startGroupIdx);

   GET_IFACE2(pBroker,IBridge,pBridge);

   Float64 group_offset = 0;

   for ( GroupIndexType grpIdx = startGroupIdx; grpIdx <= endGroupIdx; grpIdx++ )
   {
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(grpIdx);
      const CSplicedGirderData* pGirder = pGroup->GetGirder(girderKey.girderIndex);

      Float64 running_segment_length = 0;

      CollectionIndexType nClosures = pGirder->GetClosureJointCount();
      for ( CollectionIndexType closureIdx = 0; closureIdx < nClosures; closureIdx++ )
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
            pBridge->GetClosureJointProfile(closureKey,&shape);

            // offset the shape by the length of all the girder groups that
            // came before the current group
            CComQIPtr<IXYPosition> position(shape);
            position->Offset(group_offset,0);

            // create the display object
            CComPtr<iPointDisplayObject> doPnt;
            ::CoCreateInstance(CLSID_PointDisplayObject,NULL,CLSCTX_ALL,IID_iPointDisplayObject,(void**)&doPnt);
            doPnt->SetID(segID);

            // create the drawing strategy
            CComPtr<iShapeDrawStrategy> strategy;
            ::CoCreateInstance(CLSID_ShapeDrawStrategy,NULL,CLSCTX_ALL,IID_iShapeDrawStrategy,(void**)&strategy);
            doPnt->SetDrawingStrategy(strategy);

            // configure the strategy
            strategy->SetShape(shape);
            strategy->SetSolidLineColor(CLOSURE_BORDER_COLOR);
            strategy->SetSolidFillColor(CLOSURE_FILL_COLOR);
            strategy->SetVoidLineColor(VOID_BORDER_COLOR);
            strategy->SetVoidFillColor(GetSysColor(COLOR_WINDOW));
            strategy->DoFill(true);

            CString strMsg(GetClosureTooltip(pBroker,closureKey));
            doPnt->SetMaxTipWidth(TOOLTIP_WIDTH);
            doPnt->SetTipDisplayTime(TOOLTIP_DURATION);
            doPnt->SetToolTipText(strMsg);

            // put the display object in its display list
            pDL->AddDisplayObject(doPnt);
         }
      } // next closure

      Float64 last_segment_layout_length = pBridge->GetSegmentLayoutLength(CSegmentKey(grpIdx,girderKey.girderIndex,nClosures));
      running_segment_length += last_segment_layout_length;
      group_offset += running_segment_length;
   } // group loop
}

void CGirderModelElevationView::BuildStrandDisplayObjects(CPGSuperDocBase* pDoc, IBroker* pBroker, const CGirderKey& girderKey,EventIndexType eventIdx,iDisplayMgr* dispMgr)
{
   CComPtr<iDisplayList> pDL;
   dispMgr->FindDisplayList(STRAND_LIST,&pDL);
   ATLASSERT(pDL);
   pDL->Clear();

   CComPtr<iDisplayList> pDebondDL;
   dispMgr->FindDisplayList(DEBOND_LIST,&pDebondDL);
   ATLASSERT(pDebondDL);
   pDebondDL->Clear();

   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeometry);
   GET_IFACE2(pBroker,IGirder,pIGirder);
   GET_IFACE2(pBroker,IPointOfInterest,pPOI);

   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   GroupIndexType startGroupIdx = (girderKey.groupIndex == ALL_GROUPS ? 0 : girderKey.groupIndex);
   GroupIndexType endGroupIdx   = (girderKey.groupIndex == ALL_GROUPS ? pBridgeDesc->GetGirderGroupCount()-1 : startGroupIdx);

   Float64 group_offset = 0;

   for (GroupIndexType grpIdx = startGroupIdx; grpIdx <= endGroupIdx; grpIdx++ )
   {
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(grpIdx);
      const CSplicedGirderData* pGirder = pGroup->GetGirder(girderKey.girderIndex);
      Float64 running_segment_length = 0; // sum of the segment lengths from segIdx = 0 to current segment
      SegmentIndexType nSegments = pGirder->GetSegmentCount();
      for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
      {
         CSegmentKey segmentKey(grpIdx,girderKey.girderIndex,segIdx);

         Float64 segment_length        = pBridge->GetSegmentLength(segmentKey);
         Float64 start_brg_offset      = pBridge->GetSegmentStartBearingOffset(segmentKey);
         Float64 start_end_distance    = pBridge->GetSegmentStartEndDistance(segmentKey);
         Float64 start_offset          = start_brg_offset - start_end_distance;
         Float64 segment_layout_length = pBridge->GetSegmentLayoutLength(segmentKey);

         if ( !(grpIdx == 0 && segIdx == 0) )
         {
            // running_segment_length goes to the CL of the closure... adjust the distance
            // so that it goes to the left face of the current segment
            running_segment_length += start_offset;
         }

         Float64 lft_harp, rgt_harp;
         pStrandGeometry->GetHarpingPointLocations(segmentKey, &lft_harp, &rgt_harp);

         CComPtr<IPoint2d> from_point, to_point;
         from_point.CoCreateInstance(__uuidof(Point2d));
         to_point.CoCreateInstance(__uuidof(Point2d));
         from_point->put_X(group_offset + running_segment_length);
         to_point->put_X(group_offset + running_segment_length + segment_length);


         // Look up the POI so it is faster to get section properties
         pgsPointOfInterest start_poi( pPOI->GetPointOfInterest(segmentKey,0.0) );
         pgsPointOfInterest end_poi(   pPOI->GetPointOfInterest(segmentKey,segment_length) );

         if ( start_poi.GetID() == INVALID_ID )
            start_poi.SetDistFromStart(0.0);

         if ( end_poi.GetID() == INVALID_ID )
            end_poi.SetDistFromStart(segment_length);

         std::vector<pgsPointOfInterest> vPOI( pPOI->GetPointsOfInterest(segmentKey,POI_HARPINGPOINT) );
         ATLASSERT( 0 <= vPOI.size() && vPOI.size() < 3 );
         pgsPointOfInterest hp1_poi;
         pgsPointOfInterest hp2_poi;

         if ( 0 < vPOI.size() )
         {
            hp1_poi = vPOI.front();
            hp2_poi = vPOI.back();
         }

         // straight strands
         CComPtr<IPoint2dCollection> spoints, epoints;
         pStrandGeometry->GetStrandPositions(start_poi, pgsTypes::Straight,&spoints);
         pStrandGeometry->GetStrandPositions(end_poi,   pgsTypes::Straight,&epoints);
         CollectionIndexType nStrandPoints;
         spoints->get_Count(&nStrandPoints);
         CollectionIndexType strandPointIdx;
         for (strandPointIdx = 0; strandPointIdx < nStrandPoints; strandPointIdx++)
         {
            // strand points measured from bottom of girder
            CComPtr<IPoint2d> pntStart, pntEnd;
            spoints->get_Item(strandPointIdx,&pntStart);
            epoints->get_Item(strandPointIdx,&pntEnd);

            Float64 yStart, yEnd;
            pntStart->get_Y(&yStart);
            pntEnd->get_Y(&yEnd);

            from_point->put_Y(yStart);
            to_point->put_Y(yEnd);
            
            BuildLine(pDL, from_point, to_point, STRAND_FILL_COLOR);

            Float64 start,end;
            if ( pStrandGeometry->IsStrandDebonded(segmentKey,strandPointIdx,pgsTypes::Straight,&start,&end) )
            {
               // Left debond point
               CComPtr<IPoint2d> left_debond;
               left_debond.CoCreateInstance(CLSID_Point2d);
               left_debond->Move(start_offset + running_segment_length + start,yStart);

               BuildLine(pDebondDL, from_point, left_debond, DEBOND_FILL_COLOR );
               BuildDebondTick(pDebondDL, left_debond, DEBOND_FILL_COLOR );

               CComPtr<IPoint2d> right_debond;
               right_debond.CoCreateInstance(CLSID_Point2d);
               right_debond->Move(start_offset + running_segment_length + end,yEnd);

               BuildLine(pDebondDL, right_debond, to_point, DEBOND_FILL_COLOR);
               BuildDebondTick(pDebondDL, right_debond, DEBOND_FILL_COLOR );
            }
         }

         // harped strands
         if ( 0 < vPOI.size() )
         {
            CComPtr<IPoint2dCollection> spoints, hp1points, hp2points, epoints;
            pStrandGeometry->GetStrandPositions(start_poi, pgsTypes::Harped,&spoints);
            pStrandGeometry->GetStrandPositions(hp1_poi,   pgsTypes::Harped,&hp1points);
            pStrandGeometry->GetStrandPositions(hp2_poi,   pgsTypes::Harped,&hp2points);
            pStrandGeometry->GetStrandPositions(end_poi,   pgsTypes::Harped,&epoints);
            spoints->get_Count(&nStrandPoints);
            for (strandPointIdx = 0; strandPointIdx < nStrandPoints; strandPointIdx++)
            {
               CComPtr<IPoint2d> start_pos, hp1_pos, hp2_pos, end_pos;
               spoints->get_Item(strandPointIdx,&start_pos);
               hp1points->get_Item(strandPointIdx,&hp1_pos);
               hp2points->get_Item(strandPointIdx,&hp2_pos);
               epoints->get_Item(strandPointIdx,&end_pos);

               Float64 start_x, start_y;
               start_pos->Location(&start_x,&start_y);

               Float64 hp1_x, hp1_y;
               hp1_pos->Location(&hp1_x,&hp1_y);

               Float64 hp2_x, hp2_y;
               hp2_pos->Location(&hp2_x,&hp2_y);

               Float64 end_x, end_y;
               end_pos->Location(&end_x,&end_y);

               from_point->put_X(group_offset + running_segment_length);
               from_point->put_Y(start_y);
               to_point->put_X(group_offset + running_segment_length + lft_harp);
               to_point->put_Y(hp1_y);
               
               BuildLine(pDL, from_point, to_point, STRAND_FILL_COLOR);

               from_point->put_X(group_offset + running_segment_length + lft_harp);
               from_point->put_Y(hp1_y);
               to_point->put_X(group_offset + running_segment_length + rgt_harp);
               to_point->put_Y(hp2_y);
               
               BuildLine(pDL, from_point, to_point, STRAND_FILL_COLOR);

               from_point->put_X(group_offset + running_segment_length + rgt_harp);
               from_point->put_Y(hp2_y);
               to_point->put_X(group_offset + running_segment_length + segment_length);
               to_point->put_Y(end_y);

               BuildLine(pDL, from_point, to_point, STRAND_FILL_COLOR);
            }
         }

         // Temporary strands
         spoints.Release();
         epoints.Release();
         pStrandGeometry->GetStrandPositions(start_poi, pgsTypes::Temporary,&spoints);
         pStrandGeometry->GetStrandPositions(end_poi,   pgsTypes::Temporary,&epoints);
         spoints->get_Count(&nStrandPoints);
         for (strandPointIdx = 0; strandPointIdx < nStrandPoints; strandPointIdx++)
         {
            CComPtr<IPoint2d> pntStart, pntEnd;
            spoints->get_Item(strandPointIdx, &pntStart);
            epoints->get_Item(strandPointIdx, &pntEnd);

            Float64 y;
            from_point->put_X(group_offset + running_segment_length);
            pntStart->get_Y(&y);
            from_point->put_Y(y);

            to_point->put_X(group_offset + running_segment_length + segment_length);
            pntEnd->get_Y(&y);
            to_point->put_Y(y);
            
            BuildLine(pDL, from_point, to_point, STRAND_FILL_COLOR);
         }

         running_segment_length += segment_layout_length - start_offset;
      } // end of segment loop

      group_offset += running_segment_length;
   } // end of group loop
}

void CGirderModelElevationView::BuildStrandCGDisplayObjects(CPGSuperDocBase* pDoc, IBroker* pBroker, const CGirderKey& girderKey,EventIndexType /*eventIdx*/,iDisplayMgr* dispMgr)
{
#pragma Reminder("UPDATE: this shows the geometric centroid of the strand, need to show location of resultant PS force")

   CComPtr<iDisplayList> pDL;
   dispMgr->FindDisplayList(STRAND_CG_LIST,&pDL);
   ATLASSERT(pDL);
   pDL->Clear();

   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeometry);
   GET_IFACE2(pBroker,ISectionProperties,pSectProp);
   GET_IFACE2(pBroker,IIntervals,pIntervals);

   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   GroupIndexType startGroupIdx = (girderKey.groupIndex == ALL_GROUPS ? 0 : girderKey.groupIndex);
   GroupIndexType endGroupIdx   = (girderKey.groupIndex == ALL_GROUPS ? pBridgeDesc->GetGirderGroupCount()-1 : startGroupIdx);

   Float64 group_offset = 0;

   for (GroupIndexType grpIdx = startGroupIdx; grpIdx <= endGroupIdx; grpIdx++ )
   {
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(grpIdx);
      const CSplicedGirderData* pGirder = pGroup->GetGirder(girderKey.girderIndex);
      SegmentIndexType nSegments = pGirder->GetSegmentCount();
      Float64 running_segment_length = 0;
      for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
      {
         CSegmentKey segmentKey(grpIdx,girderKey.girderIndex,segIdx);

         IntervalIndexType intervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);

         Float64 segment_length        = pBridge->GetSegmentLength(segmentKey);
         Float64 start_brg_offset      = pBridge->GetSegmentStartBearingOffset(segmentKey);
         Float64 start_end_distance    = pBridge->GetSegmentStartEndDistance(segmentKey);
         Float64 start_offset          = start_brg_offset - start_end_distance;
         Float64 segment_layout_length = pBridge->GetSegmentLayoutLength(segmentKey);

         if ( !(grpIdx == 0 && segIdx == 0) )
         {
            // running_segment_length goes to the CL of the closure... adjust the distance
            // so that it goes to the left face of the current segment
            running_segment_length += start_offset;
         }

         StrandIndexType nStrands = pStrandGeometry->GetNumStrands(segmentKey, pgsTypes::Straight);
         nStrands += pStrandGeometry->GetNumStrands(segmentKey, pgsTypes::Harped);
         nStrands += pStrandGeometry->GetNumStrands(segmentKey, pgsTypes::Temporary);
         if (0 < nStrands)
         {
            CComPtr<IPoint2d> from_point, to_point;
            from_point.CoCreateInstance(__uuidof(Point2d));
            to_point.CoCreateInstance(__uuidof(Point2d));

            Float64 segment_length = pBridge->GetSegmentLength(segmentKey);

            GET_IFACE2(pBroker,IPointOfInterest,pPOI);
            std::vector<pgsPointOfInterest> vPOI( pPOI->GetPointsOfInterest(segmentKey,POI_HARPINGPOINT) );
            ATLASSERT( 0 <= vPOI.size() && vPOI.size() < 3 );

            // Look up the POI so it is faster to get section properties
            pgsPointOfInterest start_poi( pPOI->GetPointOfInterest(segmentKey,0.0) );
            pgsPointOfInterest end_poi(   pPOI->GetPointOfInterest(segmentKey,segment_length) );

            if ( start_poi.GetID() == INVALID_ID )
               start_poi.SetDistFromStart(0.0);

            if ( end_poi.GetID() == INVALID_ID )
               end_poi.SetDistFromStart(segment_length);

            vPOI.push_back(start_poi);
            vPOI.push_back(end_poi);
            std::sort(vPOI.begin(),vPOI.end());

            Float64 from_y;
            Float64 to_y;
            std::vector<pgsPointOfInterest>::iterator iter( vPOI.begin() );
            pgsPointOfInterest prev_poi = *iter;
            iter++;

            Float64 Ybg = pSectProp->GetY(intervalIdx,prev_poi,pgsTypes::BottomGirder);
            Float64 Hg  = pSectProp->GetHg(intervalIdx,prev_poi);
            Float64 nEff;
            Float64 ecc = pStrandGeometry->GetEccentricity(intervalIdx, prev_poi, false, &nEff);
            from_y = Ybg - (Hg+ecc);

            std::vector<pgsPointOfInterest>::iterator end( vPOI.end() );
            for ( ; iter != end; iter++ )
            {
               pgsPointOfInterest& poi = *iter;
            
               Ybg = pSectProp->GetY(intervalIdx,poi,pgsTypes::BottomGirder);
               Hg  = pSectProp->GetHg(intervalIdx,poi);
               ecc = pStrandGeometry->GetEccentricity(intervalIdx, poi, false, &nEff);
               to_y = Ybg - (Hg+ecc);

               from_point->put_X(group_offset + running_segment_length + prev_poi.GetDistFromStart());
               from_point->put_Y(from_y);
               to_point->put_X(group_offset + running_segment_length + poi.GetDistFromStart());
               to_point->put_Y(to_y);

               BuildLine(pDL, from_point, to_point, RED, WHITE);

               prev_poi = poi;
               from_y = to_y;
            }
         }

         running_segment_length += segment_layout_length - start_offset;
      }

      group_offset += running_segment_length;
   }
}

void CGirderModelElevationView::BuildTendonDisplayObjects(CPGSuperDocBase* pDoc, IBroker* pBroker, const CGirderKey& girderKey,EventIndexType eventIdx,iDisplayMgr* dispMgr)
{
   CComPtr<iDisplayList> pDL;
   dispMgr->FindDisplayList(TENDON_LIST,&pDL);
   ATLASSERT(pDL);
   pDL->Clear();

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   GET_IFACE2(pBroker,IBridge,pBridge);

   const CTimelineManager* pTimelineMgr = pBridgeDesc->GetTimelineManager();

   GroupIndexType startGroupIdx = (girderKey.groupIndex == ALL_GROUPS ? 0 : girderKey.groupIndex);
   GroupIndexType endGroupIdx   = (girderKey.groupIndex == ALL_GROUPS ? pBridgeDesc->GetGirderGroupCount()-1 : startGroupIdx);


   Float64 group_offset = 0;
   GET_IFACE2(pBroker,ITendonGeometry,pTendonGeometry);
   GET_IFACE2(pBroker,IGirder,pGirder);
   for ( GroupIndexType grpIdx = startGroupIdx; grpIdx <= endGroupIdx; grpIdx++ )
   {
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(grpIdx);

      CGirderKey thisGirderKey(girderKey);
      thisGirderKey.groupIndex = grpIdx;

#pragma Reminder("UPDATE: draw portion of strand in a segment")
      // this next block of code is a cop-out. it would be better to 
      // draw the duct in the segment if it is erected.
      if ( !pTimelineMgr->AreAllSegmentsErected(thisGirderKey,eventIdx) )
         continue; // if all segments are not erected, there is nothing to draw

      WebIndexType nWebs = pGirder->GetWebCount(thisGirderKey);

      DuctIndexType nDucts = pTendonGeometry->GetDuctCount(thisGirderKey);
      for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
      {
         bool bIsTendonInstalled = true;
         IndexType index = ductIdx/nWebs;
         EventIndexType ptEventIdx = pTimelineMgr->GetStressTendonEventIndex(thisGirderKey,index);
         if ( eventIdx < ptEventIdx || ptEventIdx == INVALID_INDEX )
            bIsTendonInstalled = false;
         
         CComPtr<IPoint2dCollection> ductPoints;
         pTendonGeometry->GetDuctCenterline(thisGirderKey,ductIdx,&ductPoints);

         CollectionIndexType nPoints;
         ductPoints->get_Count(&nPoints);
         CComPtr<IPoint2d> from_point;
         ductPoints->get_Item(0,&from_point);
         from_point->Offset(group_offset,0);

         for( CollectionIndexType pntIdx = 1; pntIdx < nPoints; pntIdx++ )
         {
            CComPtr<IPoint2d> to_point;
            ductPoints->get_Item(pntIdx,&to_point);

            to_point->Offset(group_offset,0);

            if ( bIsTendonInstalled )
               BuildLine(pDL, from_point, to_point, TENDON_LINE_COLOR);
            else
               BuildLine(pDL, from_point, to_point, DUCT_LINE_COLOR1, DUCT_LINE_COLOR2);

            from_point = to_point;
         }
      }

      Float64 girder_length = pBridge->GetGirderLayoutLength(thisGirderKey);
      group_offset += girder_length;
   }
}

void CGirderModelElevationView::BuildRebarDisplayObjects(CPGSuperDocBase* pDoc, IBroker* pBroker, const CGirderKey& girderKey,EventIndexType eventIdx,iDisplayMgr* dispMgr)
{
#pragma Reminder("Remove obsolete code")
   // the commented out code below is what the code was before merging RDP's code
   // for partial length rebar. When the new code works, remove this code
/*
   CComPtr<iDisplayList> pDL;
   dispMgr->FindDisplayList(REBAR_LIST,&pDL);
   ATLASSERT(pDL);
   pDL->Clear();

   // if segment isn't constructed, don't display it
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   if ( m_pFrame->GetEvent() < pIBridgeDesc->GetSegmentConstructionEventIndex() )
      return;

   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,ILongRebarGeometry,pLongRebarGeometry);
   GET_IFACE2(pBroker,IPointOfInterest,pPOI);
   GET_IFACE2(pBroker,IGirder,pIGirder);

   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   GroupIndexType startGroupIdx = (girderKey.groupIndex == ALL_GROUPS ? 0 : girderKey.groupIndex);
   GroupIndexType endGroupIdx   = (girderKey.groupIndex == ALL_GROUPS ? pBridgeDesc->GetGirderGroupCount()-1 : startGroupIdx);

   Float64 group_offset = 0;

   for ( GroupIndexType grpIdx = startGroupIdx; grpIdx <= endGroupIdx; grpIdx++ )
   {
      Float64 running_segment_length = 0;

      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(grpIdx);
      const CSplicedGirderData* pGirder = pGroup->GetGirder(girderKey.girderIndex);
      SegmentIndexType nSegments = pGirder->GetSegmentCount();
      for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
      {
         CSegmentKey segmentKey(grpIdx,girderKey.girderIndex,segIdx);

         Float64 start_brg_offset   = pBridge->GetSegmentStartBearingOffset(segmentKey);
         Float64 start_end_distance = pBridge->GetSegmentStartEndDistance(segmentKey);
         Float64 start_offset       = start_brg_offset - start_end_distance;

         Float64 segment_length        = pBridge->GetSegmentLength(segmentKey);
         Float64 segment_layout_length = pBridge->GetSegmentLayoutLength(segmentKey);

         // Look up the POI so it is faster to get section properties
         pgsPointOfInterest poiStart( pPOI->GetPointOfInterest(segmentKey,0.0) );
         pgsPointOfInterest poiEnd(   pPOI->GetPointOfInterest(segmentKey,segment_length) );

         if ( poiStart.GetID() == INVALID_ID )
            poiStart.SetDistFromStart(0.0);

         if ( poiEnd.GetID() == INVALID_ID )
            poiEnd.SetDistFromStart(segment_length);


         Float64 HgStart = pIGirder->GetHeight(poiStart);
         Float64 HgEnd   = pIGirder->GetHeight(poiEnd);

         CComPtr<IRebarSection> rebar_section_start, rebar_section_end;
         pLongRebarGeometry->GetRebars(poiStart,&rebar_section_start);
         pLongRebarGeometry->GetRebars(poiEnd,  &rebar_section_end);

         CComPtr<IEnumRebarSectionItem> enum_start, enum_end;
         rebar_section_start->get__EnumRebarSectionItem(&enum_start);
         rebar_section_end->get__EnumRebarSectionItem(&enum_end);

         CComPtr<IRebarSectionItem> startItem, endItem;
         while ( enum_start->Next(1,&startItem,NULL) != S_FALSE && enum_end->Next(1,&endItem,NULL) != S_FALSE )
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

            CComPtr<IPoint2d> from_point, to_point;
            from_point.CoCreateInstance(__uuidof(Point2d));
            to_point.CoCreateInstance(__uuidof(Point2d));

            from_point->put_X(running_segment_length + start_offset);
            from_point->put_Y(yStart-HgStart);

            to_point->put_X(running_segment_length + start_offset + segment_length);
            to_point->put_Y(yEnd-HgEnd);
            
            BuildLine(pDL, from_point, to_point, REBAR_COLOR);
         }

         running_segment_length += segment_layout_length;
      }

      group_offset += running_segment_length;
   }
*/
   CComPtr<iDisplayList> pDL;
   dispMgr->FindDisplayList(REBAR_LIST,&pDL);
   ATLASSERT(pDL);
   pDL->Clear();

   // if segment isn't constructed, don't display it
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   if ( m_pFrame->GetEvent() < pIBridgeDesc->GetSegmentConstructionEventIndex() )
      return;

   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,ILongRebarGeometry,pLongRebarGeometry);
   GET_IFACE2(pBroker,IPointOfInterest,pPOI);
   GET_IFACE2(pBroker,IGirder,pIGirder);

   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   GroupIndexType startGroupIdx = (girderKey.groupIndex == ALL_GROUPS ? 0 : girderKey.groupIndex);
   GroupIndexType endGroupIdx   = (girderKey.groupIndex == ALL_GROUPS ? pBridgeDesc->GetGirderGroupCount()-1 : startGroupIdx);

   Float64 group_offset = 0;

   for ( GroupIndexType grpIdx = startGroupIdx; grpIdx <= endGroupIdx; grpIdx++ )
   {
      Float64 running_segment_length = 0;

      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(grpIdx);
      const CSplicedGirderData* pGirder = pGroup->GetGirder(girderKey.girderIndex);
      SegmentIndexType nSegments = pGirder->GetSegmentCount();
      for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
      {
         CSegmentKey segmentKey(grpIdx,girderKey.girderIndex,segIdx);

         Float64 start_brg_offset      = pBridge->GetSegmentStartBearingOffset(segmentKey);
         Float64 start_end_distance    = pBridge->GetSegmentStartEndDistance(segmentKey);
         Float64 start_offset          = start_brg_offset - start_end_distance;
         Float64 segment_layout_length = pBridge->GetSegmentLayoutLength(segmentKey);
         Float64 segment_length        = pBridge->GetSegmentLength(segmentKey);

         if ( !(grpIdx == 0 && segIdx == 0) )
         {
            // running_segment_length goes to the CL of the closure... adjust the distance
            // so that it goes to the left face of the current segment
            running_segment_length += start_offset;
         }

         CComPtr<IRebarLayout> rebarLayout;
         pLongRebarGeometry->GetRebarLayout(segmentKey, &rebarLayout);

         CComPtr<IEnumRebarLayoutItems> enumItems;
         rebarLayout->get__EnumRebarLayoutItems(&enumItems);
   
         CComPtr<IRebarLayoutItem> rebarLayoutItem;
         while ( enumItems->Next(1,&rebarLayoutItem,NULL) != S_FALSE )
         {
            Float64 startLoc, layoutLength, endLoc;
            rebarLayoutItem->get_Start(&startLoc);
            rebarLayoutItem->get_Length(&layoutLength);
            endLoc = startLoc + layoutLength;

            if ( segment_length <= startLoc )
            {
               // rebar is beyond the end of the segment which means it is in the closure joint
               // only draw rebar in the closure joint if it has been cast
               if ( m_pFrame->GetEvent() < pIBridgeDesc->GetCastClosureJointEventIndex(segmentKey.groupIndex,segmentKey.segmentIndex) )
               {
                  rebarLayoutItem.Release();
                  continue;
               }
            }

            CComPtr<IEnumRebarPatterns> enumPatterns;
            rebarLayoutItem->get__EnumRebarPatterns(&enumPatterns);

            CComPtr<IRebarPattern> rebarPattern;
            while ( enumPatterns->Next(1,&rebarPattern,NULL) != S_FALSE )
            {
               // Currently, we only enter rebars in rows in PGSuper. If this is not the case,
               // all bars must be drawn. But since we are drawing an elevation, we only need one bar
               CComQIPtr<IRebarRowPattern> rowPat(rebarPattern);
               ATLASSERT(rowPat); // Rethink single-bar logic below

               CollectionIndexType nbars;
               rebarPattern->get_Count(&nbars);
               if (0 < nbars)
               {
                  CollectionIndexType ibar = 0; // only need to draw one bar
                  CComPtr<IPoint2d> startBarLocation, endBarLocation;
                  rebarPattern->get_Location(0.0, ibar, &startBarLocation);
                  rebarPattern->get_Location(layoutLength, ibar, &endBarLocation);

                  // Rebar locations are in Girder Section Coordinates (0,0 is at the top center of the shape)
                  // Since this is an elevation view, we don't use the X-value

                  Float64 yStart, yEnd;
                  startBarLocation->get_Y(&yStart);
                  endBarLocation->get_Y(&yEnd);

                  // Move points along girder
                  startBarLocation->put_X(group_offset + running_segment_length + startLoc);
                  startBarLocation->put_Y(yStart);
                  endBarLocation->put_X(group_offset + running_segment_length + endLoc);
                  endBarLocation->put_Y(yEnd);
                  
                  BuildLine(pDL, startBarLocation, endBarLocation, REBAR_COLOR);
               } // if 0 < nbars

               rebarPattern.Release();
            } // next rebar pattern

            rebarLayoutItem.Release();
        
         } // next rebar layout
   
         running_segment_length += segment_layout_length - start_offset;
      } // next segment
         
      group_offset += running_segment_length;
   } // next group
}

template <class T> bool IsLoadApplicable(IBroker* pBroker,const T& load,EventIndexType eventIdx,const CGirderKey& girderKey)
{
   if ( load.m_EventIndex != eventIdx )
      return false;

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

   bool bMatchSpan = ((startSpanIdx <= load.m_SpanGirderKey.spanIndex && load.m_SpanGirderKey.spanIndex <= endSpanIdx) || load.m_SpanGirderKey.spanIndex == ALL_SPANS) ? true : false;
   bool bMatchGirder = (load.m_SpanGirderKey.girderIndex == girderKey.girderIndex || load.m_SpanGirderKey.girderIndex == ALL_GIRDERS) ? true : false;

   return bMatchSpan && bMatchGirder;
}

void CGirderModelElevationView::BuildPointLoadDisplayObjects(CPGSuperDocBase* pDoc, IBroker* pBroker, const CGirderKey& girderKey,EventIndexType eventIdx,iDisplayMgr* dispMgr, bool* casesExist)
{
   CComPtr<iDisplayList> pDL;
   dispMgr->FindDisplayList(LOAD_LIST,&pDL);
   ATLASSERT(pDL);

   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IUserDefinedLoadData,pUserDefinedLoadData);

   CollectionIndexType nLoads =  pUserDefinedLoadData->GetPointLoadCount();
   SpanIndexType nSpans = pBridge->GetSpanCount();

   // filter loads and determine magnitude of max load
   Float64 max = 0.0;
   CollectionIndexType loadIdx;
   for (loadIdx = 0; loadIdx < nLoads; loadIdx++)
   {
      const CPointLoadData& load = pUserDefinedLoadData->GetPointLoad(loadIdx);

      if (IsLoadApplicable(pBroker,load,eventIdx,girderKey))
      {
         max = Max(fabs(load.m_Magnitude), max);
      }
   }

   CComPtr<iDisplayObjectFactory> factory;
   dispMgr->GetDisplayObjectFactory(0, &factory);

   // create load display objects from filtered list
   for (loadIdx = 0; loadIdx < nLoads; loadIdx++)
   {
      const CPointLoadData& load = pUserDefinedLoadData->GetPointLoad(loadIdx);
      if (IsLoadApplicable(pBroker,load,eventIdx,girderKey))
      {
         casesExist[load.m_LoadCase] = true;

         COLORREF color = GetLoadGroupColor(load.m_LoadCase);

         SpanIndexType startSpanIdx = (load.m_SpanGirderKey.spanIndex == ALL_SPANS ? 0 : load.m_SpanGirderKey.spanIndex);
         SpanIndexType endSpanIdx   = (load.m_SpanGirderKey.spanIndex == ALL_SPANS ? nSpans-1 : startSpanIdx);
         for ( SpanIndexType spanIdx = startSpanIdx; spanIdx <= endSpanIdx; spanIdx++ )
         {
            CSpanGirderKey spanGirderKey(spanIdx,girderKey.girderIndex);

            Float64 span_length = pBridge->GetSpanLength(spanGirderKey.spanIndex,spanGirderKey.girderIndex);

            Float64 location_from_left_end = load.m_Location;
            if (load.m_Fractional)
               location_from_left_end *= span_length;

            CComPtr<iDisplayObject> disp_obj;
            factory->Create(CPointLoadDrawStrategyImpl::ms_Format,NULL,&disp_obj);

            CComQIPtr<iPointDisplayObject,&IID_iPointDisplayObject> point_disp(disp_obj);

            CComPtr<iDrawPointStrategy> pStrategy;
            point_disp->GetDrawingStrategy(&pStrategy);

            CComQIPtr<iPointLoadDrawStrategy,&IID_iPointLoadDrawStrategy> pls(pStrategy);
            pls->Init(point_disp, pBroker, load, loadIdx, span_length, max, color);

            Float64 x_position = GetSpanStartLocation(spanGirderKey);
            x_position += location_from_left_end;

            CComPtr<IPoint2d> point;
            point.CoCreateInstance(__uuidof(Point2d));
            point->put_X(x_position);
            point->put_Y(0.0);

            point_disp->SetPosition(point, FALSE, FALSE);

            // tool tip
            CComPtr<IBroker> pBroker;
            EAFGetBroker(&pBroker);
            GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
            CString strMagnitude = FormatDimension(load.m_Magnitude,pDisplayUnits->GetGeneralForceUnit(),true);
            CString strLocation  = FormatDimension(location_from_left_end,pDisplayUnits->GetSpanLengthUnit(),true);

            CString strToolTip;
            strToolTip.Format(_T("Point Load\r\nP = %s  L = %s from left end of span\r\n%s Load Case"),
                               strMagnitude,strLocation,GetLoadGroupNameForUserLoad(load.m_LoadCase).c_str());

            if ( load.m_Description != _T("") )
            {
               strToolTip += _T("\r\n") + CString(load.m_Description.c_str());
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

void CGirderModelElevationView::BuildDistributedLoadDisplayObjects(CPGSuperDocBase* pDoc, IBroker* pBroker, const CGirderKey& girderKey,EventIndexType eventIdx,iDisplayMgr* dispMgr, bool* casesExist)
{
   CComPtr<iDisplayList> pDL;
   dispMgr->FindDisplayList(LOAD_LIST,&pDL);
   ATLASSERT(pDL);

   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,ISectionProperties,pSectProp);
   GET_IFACE2(pBroker,IUserDefinedLoadData,pUserDefinedLoadData);

   CollectionIndexType nLoads =  pUserDefinedLoadData->GetDistributedLoadCount();
   SpanIndexType nSpans = pBridge->GetSpanCount();

   // filter loads and determine magnitude of max load
   Float64 max = 0.0;
   CollectionIndexType loadIdx;
   for (loadIdx = 0; loadIdx < nLoads; loadIdx++ )
   {
      const CDistributedLoadData& load = pUserDefinedLoadData->GetDistributedLoad(loadIdx);

      if (IsLoadApplicable(pBroker,load,eventIdx,girderKey))
      {
         max = Max(fabs(load.m_WStart), max);
         max = Max(fabs(load.m_WEnd), max);
      }
   }

   CComPtr<iDisplayObjectFactory> factory;
   dispMgr->GetDisplayObjectFactory(0, &factory);

   // create load display objects from filtered list
   for (loadIdx = 0; loadIdx < nLoads; loadIdx++ )
   {
      const CDistributedLoadData& load = pUserDefinedLoadData->GetDistributedLoad(loadIdx);

      if (IsLoadApplicable(pBroker,load,eventIdx,girderKey))
      {
         casesExist[load.m_LoadCase] = true;

         COLORREF color = GetLoadGroupColor(load.m_LoadCase);

         SpanIndexType startSpanIdx = (load.m_SpanGirderKey.spanIndex == ALL_SPANS ? 0 : load.m_SpanGirderKey.spanIndex);
         SpanIndexType endSpanIdx   = (load.m_SpanGirderKey.spanIndex == ALL_SPANS ? nSpans-1 : startSpanIdx);
         for ( SpanIndexType spanIdx = startSpanIdx; spanIdx <= endSpanIdx; spanIdx++ )
         {
            CSpanGirderKey spanGirderKey(spanIdx,girderKey.girderIndex);

            Float64 span_length = pBridge->GetSpanLength(spanGirderKey.spanIndex,spanGirderKey.girderIndex);

            Float64 wstart_loc, wend_loc;
            if (load.m_Type == UserLoads::Uniform)
            {
               wstart_loc = 0.0;
               wend_loc   = span_length;
            }
            else
            {
               wstart_loc = load.m_StartLocation;
               wend_loc   = load.m_EndLocation;
               if (load.m_Fractional)
               {
                  wstart_loc *= span_length;
                  wend_loc   *= span_length;;
               }
            }

            Float64 load_length = wend_loc - wstart_loc;
            if(load_length <= 0.0)
            {
               ATLASSERT(0); // interface should be blocking this
               break;
            }

            Float64 x_position = GetSpanStartLocation(spanGirderKey);

            CComPtr<iDisplayObject> disp_obj;
            factory->Create(CDistributedLoadDrawStrategyImpl::ms_Format,NULL,&disp_obj);

            CComQIPtr<iPointDisplayObject,&IID_iPointDisplayObject> point_disp(disp_obj);

            CComPtr<iDrawPointStrategy> pStrategy;
            point_disp->GetDrawingStrategy(&pStrategy);

            CComQIPtr<iDistributedLoadDrawStrategy,&IID_iDistributedLoadDrawStrategy> pls(pStrategy);
            pls->Init(point_disp, pBroker, load, loadIdx, load_length, span_length, max, color);

            CComPtr<IPoint2d> point;
            point.CoCreateInstance(__uuidof(Point2d));
            point->put_X(x_position + wstart_loc);
            point->put_Y(0.0);

            point_disp->SetPosition(point, FALSE, FALSE);

            // tool tip
            CComPtr<IBroker> pBroker;
            EAFGetBroker(&pBroker);
            GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
            CString strStartMagnitude = FormatDimension(load.m_WStart,pDisplayUnits->GetForcePerLengthUnit(),true);
            CString strEndMagnitude   = FormatDimension(load.m_WEnd,pDisplayUnits->GetForcePerLengthUnit(),true);
            CString strStartLocation  = FormatDimension(wstart_loc,pDisplayUnits->GetSpanLengthUnit(),true);
            CString strEndLocation    = FormatDimension(wend_loc,pDisplayUnits->GetSpanLengthUnit(),true);


            CString strToolTip;
            strToolTip.Format(_T("Distributed Load\r\nWstart = %s  Wend = %s\r\nLstart = %s  Lend = %s from left end of span\r\n%s Load Case"),
                               strStartMagnitude,strEndMagnitude,strStartLocation,strEndLocation,GetLoadGroupNameForUserLoad(load.m_LoadCase).c_str());

            if ( load.m_Description != _T("") )
            {
               strToolTip += _T("\r\n") + CString(load.m_Description.c_str());
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


void CGirderModelElevationView::BuildMomentLoadDisplayObjects(CPGSuperDocBase* pDoc, IBroker* pBroker,const CGirderKey& girderKey, EventIndexType eventIdx,iDisplayMgr* dispMgr, bool* casesExist)
{
   CComPtr<iDisplayList> pDL;
   dispMgr->FindDisplayList(LOAD_LIST,&pDL);
   ATLASSERT(pDL);

   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IGirder,pGirder);
   GET_IFACE2(pBroker,IUserDefinedLoadData,pUserDefinedLoadData);

   // Moment loads are only used in PGSuper documents
   ATLASSERT(EAFGetDocument()->IsKindOf(RUNTIME_CLASS(CPGSuperDoc)));

   CSegmentKey segmentKey(girderKey,0);

   Float64 gdr_length = pBridge->GetSegmentLength(segmentKey);
   Float64 start_lgth = pBridge->GetSegmentStartEndDistance(segmentKey);
   Float64 end_lgth   = pBridge->GetSegmentEndEndDistance(segmentKey);
   Float64 span_lgth   = gdr_length - start_lgth - end_lgth;

   CollectionIndexType num_loads =  pUserDefinedLoadData->GetMomentLoadCount();

   // filter loads and determine magnitude of max load
   Float64 max = 0.0;
   CollectionIndexType ild;
   for (ild=0; ild<num_loads; ild++)
   {
      const CMomentLoadData& load = pUserDefinedLoadData->GetMomentLoad(ild);

      if (IsLoadApplicable(pBroker,load,eventIdx,girderKey))
      {
         max = Max(fabs(load.m_Magnitude), max);
      }
   }

   CComPtr<iDisplayObjectFactory> factory;
   dispMgr->GetDisplayObjectFactory(0, &factory);

   // create load display objects from filtered list
   for (ild=0; ild<num_loads; ild++)
   {
      const CMomentLoadData& load = pUserDefinedLoadData->GetMomentLoad(ild);

      if (IsLoadApplicable(pBroker,load,eventIdx,girderKey))
      {
         casesExist[load.m_LoadCase] = true;

         CComPtr<iDisplayObject> disp_obj;
         factory->Create(CMomentLoadDrawStrategyImpl::ms_Format,NULL,&disp_obj);

         CComQIPtr<iPointDisplayObject,&IID_iPointDisplayObject> point_disp(disp_obj);

         CComPtr<iDrawPointStrategy> pStrategy;
         point_disp->GetDrawingStrategy(&pStrategy);

         COLORREF color = GetLoadGroupColor(load.m_LoadCase);

         Float64 location;
         if (load.m_Fractional)
            location = start_lgth + span_lgth * load.m_Location;
         else
            location = start_lgth + load.m_Location;

         Float64 gdr_hgt = pGirder->GetHeight( pgsPointOfInterest(segmentKey,location) );

         CComQIPtr<iMomentLoadDrawStrategy,&IID_iMomentLoadDrawStrategy> pls(pStrategy);
         pls->Init(point_disp, pBroker, load, ild, gdr_hgt, span_lgth+start_lgth, max, color);

         CComPtr<IPoint2d> point;
         point.CoCreateInstance(__uuidof(Point2d));
         point->put_X(location);
         point->put_Y(0.0);

         point_disp->SetPosition(point, FALSE, FALSE);

         // tool tip
         CComPtr<IBroker> pBroker;
         EAFGetBroker(&pBroker);
         GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
         CString strMagnitude = FormatDimension(load.m_Magnitude,pDisplayUnits->GetMomentUnit(),true);
         CString strLocation  = FormatDimension(location,pDisplayUnits->GetSpanLengthUnit(),true);

         std::_tostringstream os;
         os << _T("Moment Load\r\n");
         os << _T("M = ") << strMagnitude;
         os << _T("   ");
         os << _T("L = ") << strLocation << _T(" from left end of girder");
         os << _T("\r\n");
         os << GetLoadGroupNameForUserLoad(load.m_LoadCase) << _T(" Load Case");

         if ( load.m_Description != _T("") )
         {
            os << _T("\r\n");
            os << load.m_Description;
         }

         point_disp->SetMaxTipWidth(TOOLTIP_WIDTH);
         point_disp->SetTipDisplayTime(TOOLTIP_DURATION);
         point_disp->SetToolTipText(os.str().c_str());
         point_disp->SetID(ild);

         pDL->AddDisplayObject(disp_obj);
      }
   }
}

void CGirderModelElevationView::BuildLegendDisplayObjects(CPGSuperDocBase* pDoc, IBroker* pBroker,const CGirderKey& girderKey, EventIndexType eventIdx,iDisplayMgr* dispMgr, bool* casesExist)
{
   if (casesExist[UserLoads::DC] || casesExist[UserLoads::DW] || casesExist[UserLoads::LL_IM])
   {
      CollectionIndexType prevNumEntries(INVALID_INDEX);
      if (!m_Legend)
      {
         m_Legend.CoCreateInstance(CLSID_LegendDisplayObject);
         m_Legend->put_Title(CComBSTR(_T("Legend")));
         m_Legend->put_DoDrawBorder(TRUE);
         m_Legend->put_IsDraggable(TRUE);

         // locate the legend at the top left corner the first time through only
         this->ScaleToFit(false);
         CComPtr<IRect2d> rect;
         m_pDispMgr->GetBoundingBox(m_pCoordinateMap, false, &rect);

         CComPtr<IPoint2d> point;
         rect->get_TopRight(&point);

         m_Legend->put_Position(point,FALSE,FALSE);
      }
      else
      {
         m_Legend->get_NumEntries(&prevNumEntries);
         m_Legend->ClearEntries();
      }

      CollectionIndexType nEntries = 0;
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
      CComPtr<iDisplayList> pDL;
      dispMgr->FindDisplayList(LOAD_LIST,&pDL);
      ATLASSERT(pDL);

      pDL->AddDisplayObject(m_Legend);

      // now can change size and shape of legend
      if (nEntries != prevNumEntries)
         m_Legend->put_NumRows(nEntries);

      CSize size;
      m_Legend->GetMinCellSize(&size);
//      size.cy += 60; // expand a bit (twips) to give border buffer
      m_Legend->put_CellSize(size);
   }
}

void CGirderModelElevationView::BuildDimensionDisplayObjects(CPGSuperDocBase* pDoc, IBroker* pBroker, const CGirderKey& girderKey,EventIndexType eventIdx,iDisplayMgr* dispMgr)
{
   CComPtr<iDisplayList> pDL;
   dispMgr->FindDisplayList(DIMLINE_LIST,&pDL);
   ATLASSERT(pDL);
   pDL->Clear();

   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   GET_IFACE2(pBroker,IGirder,pIGirder);
   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeometry);
   GET_IFACE2(pBroker,IPointOfInterest,pIPoi);

   // need to layout dimension line witness lines in twips
   const long twip_offset = 1440/2;

   CComPtr<IPoint2d> from_point, to_point;
   from_point.CoCreateInstance(__uuidof(Point2d));
   to_point.CoCreateInstance(__uuidof(Point2d));

   CComPtr<iDimensionLine> dimLine;

   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CTimelineManager* pTimelineMgr = pBridgeDesc->GetTimelineManager();

   GroupIndexType nGroups = pBridgeDesc->GetGirderGroupCount();
   GroupIndexType startGroupIdx = (girderKey.groupIndex == ALL_GROUPS ? 0 : girderKey.groupIndex);
   GroupIndexType endGroupIdx   = (girderKey.groupIndex == ALL_GROUPS ? nGroups-1 : startGroupIdx);

   // Get maximum height of the girder. This will be used for locating the dimensions vertically in the window
   Float64 Hg;
   for ( GroupIndexType grpIdx = startGroupIdx; grpIdx <= endGroupIdx; grpIdx++ )
   {
      CGirderKey gdrKey(girderKey);
      gdrKey.groupIndex = grpIdx;

      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(grpIdx);
      const CSplicedGirderData* pGirder = pGroup->GetGirder(gdrKey.girderIndex);
      SegmentIndexType nSegments = pGirder->GetSegmentCount();

      for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
      {
         CSegmentKey segmentKey(gdrKey,segIdx);
         Float64 segment_length  = pBridge->GetSegmentLength(segmentKey); // end to end length of segment

         // Use the real POI rather than ones created on the fly. It is faster to get section information
         // with real POI
         pgsPointOfInterest poiStart = pIPoi->GetPointOfInterest(segmentKey,0.00);
         pgsPointOfInterest poiEnd   = pIPoi->GetPointOfInterest(segmentKey,segment_length);
         Float64 Hg_start = pIGirder->GetHeight(poiStart);
         Float64 Hg_end   = pIGirder->GetHeight(poiEnd);
         Hg = ::Max(Hg_start,Hg_end,Hg);
      }
   }

   pgsPointOfInterest poiStart;
   pgsPointOfInterest poiEnd;
   Float64 Xgs, Xge;
   Float64 x1,x2;

   Float64 group_offset = 0;
   for ( GroupIndexType grpIdx = startGroupIdx; grpIdx <= endGroupIdx; grpIdx++ )
   {
      CGirderKey gdrKey(girderKey);
      gdrKey.groupIndex = grpIdx;

      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(grpIdx);
      const CSplicedGirderData* pGirder = pGroup->GetGirder(gdrKey.girderIndex);
      SegmentIndexType nSegments = pGirder->GetSegmentCount();

      for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
      {
         CSegmentKey segmentKey(gdrKey,segIdx);

         Float64 segment_length = pBridge->GetSegmentLength(segmentKey);       // end to end length of segment

         poiStart = pIPoi->GetPointOfInterest(segmentKey,0.00);
         poiEnd   = pIPoi->GetPointOfInterest(segmentKey,segment_length);

         //
         // Top Dimension Lines
         //

         // segment length measure to end of segment
         Xgs = pIPoi->ConvertPoiToGirderCoordinate(poiStart);
         Xge = pIPoi->ConvertPoiToGirderCoordinate(poiEnd);

         from_point->put_X(group_offset + Xgs);
         from_point->put_Y(0.0);

         to_point->put_X(group_offset + Xge);
         to_point->put_Y(0.0);

         from_point->get_X(&x1);
         to_point->get_X(&x2);

         dimLine.Release();
         BuildDimensionLine(pDL, from_point, to_point, x2-x1,&dimLine);
         dimLine->SetWitnessLength(twip_offset);

         if ( 1 < nSegments )
         {
            // segment length measured to CL of closure joint
            if ( segIdx == 0 )
            {
               poiStart = pIPoi->GetPointOfInterest(segmentKey,0.00);
            }
            else
            {
               CSegmentKey prevSegmentKey(segmentKey.groupIndex,segmentKey.girderIndex,segmentKey.segmentIndex-1);
               std::vector<pgsPointOfInterest> vPoi(pIPoi->GetPointsOfInterest(prevSegmentKey,POI_CLOSURE));
               ATLASSERT(vPoi.size() == 1);
               poiStart = vPoi.front();
            }

            if ( segIdx < nSegments-1 )
            {
               std::vector<pgsPointOfInterest> vPoi(pIPoi->GetPointsOfInterest(segmentKey,POI_CLOSURE));
               ATLASSERT(vPoi.size() == 1);
               poiEnd = vPoi.front();
            }
            else
            {
               poiEnd = pIPoi->GetPointOfInterest(segmentKey,segment_length);
            }

            Xgs = pIPoi->ConvertPoiToGirderCoordinate(poiStart);
            Xge = pIPoi->ConvertPoiToGirderCoordinate(poiEnd);

            from_point->put_X(group_offset + Xgs);
            from_point->put_Y(0.0);

            to_point->put_X(group_offset + Xge);
            to_point->put_Y(0.0);

            from_point->get_X(&x1);
            to_point->get_X(&x2);

            dimLine.Release();
            BuildDimensionLine(pDL, from_point, to_point, x2-x1, &dimLine);
            dimLine->SetWitnessLength(3*twip_offset/2);
         }

         //
         // Bottom Dimension Lines
         //

         // CL Brg to CL Brg
         std::vector<pgsPointOfInterest> vPoi(pIPoi->GetPointsOfInterest(segmentKey,POI_ERECTED_SEGMENT | POI_0L,POIFIND_AND));
         ATLASSERT(vPoi.size() == 1);
         poiStart = vPoi.front();

         vPoi = pIPoi->GetPointsOfInterest(segmentKey,POI_ERECTED_SEGMENT | POI_10L,POIFIND_AND);
         ATLASSERT(vPoi.size() == 1);
         poiEnd = vPoi.front();

         Xgs = pIPoi->ConvertPoiToGirderCoordinate(poiStart);
         Xge = pIPoi->ConvertPoiToGirderCoordinate(poiEnd);

         from_point->put_X(group_offset + Xgs);
         from_point->put_Y(-Hg);

         to_point->put_X(group_offset + Xge);
         to_point->put_Y(-Hg);

         from_point->get_X(&x1);
         to_point->get_X(&x2);

         dimLine.Release();
         BuildDimensionLine(pDL, from_point, to_point, x2-x1, &dimLine);
         dimLine->SetWitnessLength(-twip_offset);

         //StrandIndexType Nh = pStrandGeometry->GetNumStrands(segmentKey,pgsTypes::Harped);

         //Float64 lft_harp, rgt_harp;
         //pStrandGeometry->GetHarpingPointLocations(segmentKey, &lft_harp, &rgt_harp);

#pragma Reminder("UPDATE: fix harped strand dimension lines")
         //// harp locations from end of segment (along top)
         //if ( 0 < Nh )
         //{
         //   from_point->put_X(group_offset + running_segment_length + start_brg_offset - start_lgth);
         //   to_point->put_X(group_offset + lft_harp);
         //   dimLine.Release();
         //   BuildDimensionLine(pDL, from_point, to_point,lft_harp,&dimLine);
         //   dimLine->SetWitnessLength(twip_offset/2);

         //   from_point->put_X(group_offset + rgt_harp + start_brg_offset - start_lgth);
         //   to_point->put_X(group_offset + running_segment_length + segment_layout_length - end_brg_offset + end_lgth);

         //   Float64 x1,x2;
         //   from_point->get_X(&x1);
         //   to_point->get_X(&x2);

         //   dimLine.Release();
         //   BuildDimensionLine(pDL, from_point, to_point, x2-x1, &dimLine);
         //   dimLine->SetWitnessLength(twip_offset/2);
         //}

#pragma Reminder("UPDATE: fix harped strand dimension lines")
         //// harp locations (from bearings) (along bottom)
         //if ( 0 < Nh )
         //{
         //   from_point->put_X(group_offset + running_segment_length + start_brg_offset);
         //   to_point->put_X(group_offset + running_segment_length + start_brg_offset - start_lgth + lft_harp);
         //   dimLine.Release();
         //   BuildDimensionLine(pDL, from_point, to_point, lft_harp-start_lgth, &dimLine);
         //   dimLine->SetWitnessLength(-twip_offset/2);

         //   from_point->put_X(group_offset + running_segment_length + start_brg_offset - start_lgth + rgt_harp);
         //   to_point->put_X(group_offset + running_segment_length + segment_layout_length - end_brg_offset);

         //   Float64 x1,x2;
         //   from_point->get_X(&x1);
         //   to_point->get_X(&x2);

         //   dimLine.Release();
         //   BuildDimensionLine(pDL, from_point, to_point, x2-x1, &dimLine);
         //   dimLine->SetWitnessLength(-twip_offset/2);
         //}

      } // next segment

      if ( 1 < nSegments )
      {
         // Overall length along the top
         CSegmentKey segmentKey(gdrKey,0);
         poiStart = pIPoi->GetPointOfInterest(segmentKey,0);

         segmentKey.segmentIndex = nSegments-1;
         Float64 segment_length = pBridge->GetSegmentLength(segmentKey);
         poiEnd = pIPoi->GetPointOfInterest(segmentKey,segment_length);

         Xgs = pIPoi->ConvertPoiToGirderCoordinate(poiStart);
         Xge = pIPoi->ConvertPoiToGirderCoordinate(poiEnd);

         from_point->put_X(group_offset + Xgs);
         from_point->put_Y(0.0);

         to_point->put_X(group_offset + Xge);
         to_point->put_Y(0.0);

         from_point->get_X(&x1);
         to_point->get_X(&x2);

         dimLine.Release();
         BuildDimensionLine(pDL, from_point, to_point, x2-x1, &dimLine);
         dimLine->SetWitnessLength(2*twip_offset);

         // Overall length along the bottom
         segmentKey.segmentIndex = 0;
         std::vector<pgsPointOfInterest> vPoi(pIPoi->GetPointsOfInterest(segmentKey,POI_ERECTED_SEGMENT | POI_0L,POIFIND_AND));
         ATLASSERT(vPoi.size() == 1);
         poiStart = vPoi.front();

         segmentKey.segmentIndex = nSegments-1;
         vPoi = pIPoi->GetPointsOfInterest(segmentKey,POI_ERECTED_SEGMENT | POI_10L,POIFIND_AND);
         ATLASSERT(vPoi.size() == 1);
         poiEnd = vPoi.front();

         Xgs = pIPoi->ConvertPoiToGirderCoordinate(poiStart);
         Xge = pIPoi->ConvertPoiToGirderCoordinate(poiEnd);

         from_point->put_X(group_offset + Xgs);
         from_point->put_Y(-Hg);

         to_point->put_X(group_offset + Xge);
         to_point->put_Y(-Hg);

         from_point->get_X(&x1);
         to_point->get_X(&x2);

         dimLine.Release();
         BuildDimensionLine(pDL, from_point, to_point, x2-x1, &dimLine);
         dimLine->SetWitnessLength(-2*twip_offset);
      }

      // Update the offset for starting the dimensions lines in the next group
      Float64 group_length = pBridge->GetGirderLayoutLength(gdrKey);
      if ( grpIdx == 0 )
      {
         // if this is group 0, adjust the layout length so that it starts from the
         // left face of the girder.
         Float64 brg_offset = pBridge->GetSegmentStartBearingOffset(CSegmentKey(gdrKey,0));
         Float64 end_dist   = pBridge->GetSegmentStartEndDistance(  CSegmentKey(gdrKey,0));
         group_length -= (brg_offset-end_dist);
      }

      // dimensions are layed out in girder coordinates. group_offset needs to be adjusted
      // so that it starts at the left face of the next group
      if ( grpIdx < endGroupIdx )
      {
         Float64 brg_offset = pBridge->GetSegmentStartBearingOffset(CSegmentKey(grpIdx+1,gdrKey.girderIndex,0));
         Float64 end_dist   = pBridge->GetSegmentStartEndDistance(  CSegmentKey(grpIdx+1,gdrKey.girderIndex,0));
         group_length += (brg_offset-end_dist);
      }

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
         Float64 group_length = pBridge->GetGirderLayoutLength(gdrKey);
         if ( grpIdx == 0 )
         {
            // if this is group 0, adjust the layout length so that it starts from the
            // left face of the girder.
            Float64 brg_offset = pBridge->GetSegmentStartBearingOffset(CSegmentKey(gdrKey,0));
            Float64 end_dist   = pBridge->GetSegmentStartEndDistance(  CSegmentKey(gdrKey,0));
            group_length -= (brg_offset-end_dist);
         }

         // dimensions are layed out in girder coordinates. group_offset needs to be adjusted
         // so that it starts at the left face of the next group
         if ( grpIdx < girderKey.groupIndex-1 )
         {
            Float64 brg_offset = pBridge->GetSegmentStartBearingOffset(CSegmentKey(grpIdx+1,gdrKey.girderIndex,0));
            Float64 end_dist   = pBridge->GetSegmentStartEndDistance(  CSegmentKey(grpIdx+1,gdrKey.girderIndex,0));
            group_length += (brg_offset-end_dist);
         }

         girderlineOffset -= group_length;
      }
   }
   std::vector<pgsPointOfInterest> vPoi(pIPoi->GetPointsOfInterest(CSegmentKey(girderKey,ALL_SEGMENTS),POI_ABUTMENT | POI_BOUNDARY_PIER | POI_INTERMEDIATE_PIER,POIFIND_OR));
   std::vector<pgsPointOfInterest>::iterator iter(vPoi.begin());
   std::vector<pgsPointOfInterest>::iterator end(vPoi.end());
   poiStart = *iter;
   iter++;
   for ( ; iter != end; iter++ )
   {
      poiEnd = *iter;

      Xgs = pIPoi->ConvertPoiToGirderlineCoordinate(poiStart);
      Xge = pIPoi->ConvertPoiToGirderlineCoordinate(poiEnd);

      from_point->put_X(girderlineOffset + Xgs);
      from_point->put_Y(-Hg);

      to_point->put_X(girderlineOffset + Xge);
      to_point->put_Y(-Hg);

      from_point->get_X(&x1);
      to_point->get_X(&x2);

      dimLine.Release();
      BuildDimensionLine(pDL, from_point, to_point, x2-x1, &dimLine);
      dimLine->SetWitnessLength(-3*twip_offset/2);
      dimLine->SetHiddenWitnessLength(twip_offset);

      poiStart = poiEnd;
   }
}

void CGirderModelElevationView::BuildSectionCutDisplayObjects(CPGSuperDocBase* pDoc, IBroker* pBroker, const CGirderKey& girderKey,EventIndexType eventIdx,iDisplayMgr* dispMgr)
{
   CComPtr<iDisplayObjectFactory> factory;
   dispMgr->GetDisplayObjectFactory(0, &factory);

   CComPtr<iDisplayObject> disp_obj;
   factory->Create(CSectionCutDisplayImpl::ms_Format,NULL,&disp_obj);

   CComPtr<iDisplayObjectEvents> sink;
   disp_obj->GetEventSink(&sink);

   CComQIPtr<iPointDisplayObject,&IID_iPointDisplayObject> point_disp(disp_obj);
   point_disp->SetToolTipText(_T("Click on me and drag to move section cut"));

   CComQIPtr<iSectionCutDrawStrategy,&IID_iSectionCutDrawStrategy> sc_strat(sink);
   sc_strat->Init(point_disp, pBroker, girderKey, m_pFrame);
   sc_strat->SetColor(CUT_COLOR);

   point_disp->SetID(SECTION_CUT_ID);

   CComPtr<iDisplayList> pDL;
   dispMgr->FindDisplayList(SECT_CUT_LIST,&pDL);
   ATLASSERT(pDL);
   pDL->Clear();
   pDL->AddDisplayObject(disp_obj);
}

void CGirderModelElevationView::BuildStirrupDisplayObjects(CPGSuperDocBase* pDoc, IBroker* pBroker,const CGirderKey& girderKey,EventIndexType eventIdx,iDisplayMgr* dispMgr)
{
   CComPtr<iDisplayList> pDL;
   dispMgr->FindDisplayList(STIRRUP_LIST,&pDL);
   ATLASSERT(pDL);
   pDL->Clear();

   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   GET_IFACE2(pBroker,IStirrupGeometry,pStirrupGeom);
   GET_IFACE2(pBroker,IPointOfInterest,pPOI);
   GET_IFACE2(pBroker,IGirder,pIGirder);

   // assume a typical cover
   Float64 top_cover = ::ConvertToSysUnits(2.0,unitMeasure::Inch);
   Float64 bot_cover = ::ConvertToSysUnits(2.0,unitMeasure::Inch);

   pgsTypes::SupportedDeckType deckType = pBridge->GetDeckType();
      
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CTimelineManager* pTimelineMgr = pBridgeDesc->GetTimelineManager();

   GroupIndexType startGroupIdx = (girderKey.groupIndex == ALL_GROUPS ? 0 : girderKey.groupIndex);
   GroupIndexType endGroupIdx   = (girderKey.groupIndex == ALL_GROUPS ? pBridgeDesc->GetGirderGroupCount()-1 : startGroupIdx);
   Float64 group_offset = 0;
   for ( GroupIndexType grpIdx = startGroupIdx; grpIdx <= endGroupIdx; grpIdx++ )
   {
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(grpIdx);
      const CSplicedGirderData* pGirder = pGroup->GetGirder(girderKey.girderIndex);

      Float64 running_segment_length = 0; // sum of the segment lengths from segIdx = 0 to current segment
      SegmentIndexType nSegments = pGirder->GetSegmentCount();
      for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
      {
         const CPrecastSegmentData* pSegment = pGirder->GetSegment(segIdx);
         SegmentIDType segID = pSegment->GetID();

         if ( eventIdx < pTimelineMgr->GetSegmentConstructionEventIndex() )
            continue; // segment not constructed in this event, go to next segment

         CSegmentKey segmentKey(grpIdx,girderKey.girderIndex,segIdx);

         Float64 segment_length        = pBridge->GetSegmentLength(segmentKey);
         Float64 segment_layout_length = pBridge->GetSegmentLayoutLength(segmentKey);
         Float64 start_brg_offset = pBridge->GetSegmentStartBearingOffset(segmentKey);
         Float64 start_end_distance = pBridge->GetSegmentStartEndDistance(segmentKey);
         Float64 start_offset = start_brg_offset - start_end_distance;

         if ( !(grpIdx == 0 && segIdx == 0) )
         {
            // running_segment_length goes to the CL of the closure... adjust the distance
            // so that it goes to the left face of the current segment
            running_segment_length += start_offset;
         }

         Float64 slab_offset = pBridge->GetSlabOffset(segmentKey,pgsTypes::metStart); // use for dummy top of stirrup if they are extended into deck

         bool bDoStirrupsEngageDeck = pStirrupGeom->DoStirrupsEngageDeck(segmentKey);


         ZoneIndexType nStirrupZones = pStirrupGeom->GetNumPrimaryZones(segmentKey);
         for ( ZoneIndexType zoneIdx = 0; zoneIdx < nStirrupZones; zoneIdx++ )
         {
            Float64 start,end;
            pStirrupGeom->GetPrimaryZoneBounds(segmentKey,zoneIdx,&start,&end);

            Float64 zone_length = end - start;

            Float64 nHorzInterfaceShearBars = pStirrupGeom->GetPrimaryHorizInterfaceBarCount(segmentKey,zoneIdx);
            bool bDoStirrupsEngageDeck = (0.0 < nHorzInterfaceShearBars ? true : false);

            matRebar::Size barSize;
            Float64 nStirrups;
            Float64 spacing;
            pStirrupGeom->GetPrimaryVertStirrupBarInfo(segmentKey,zoneIdx,&barSize,&nStirrups,&spacing);

            if ( barSize != matRebar::bsNone && nStirrups != 0 )
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
                     ATLASSERT(slack>-1.0e-6); 
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

                  Float64 Hg = pIGirder->GetHeight(poi);

                  Float64 bottom = bot_cover - Hg;

                  Float64 top;
                  if ( deckType == pgsTypes::sdtNone || !bDoStirrupsEngageDeck )
                     top = -top_cover;
                  else
                     top = slab_offset;

                  CComPtr<IPoint2d> p1, p2;
                  p1.CoCreateInstance(CLSID_Point2d);
                  p2.CoCreateInstance(CLSID_Point2d);

                  p1->Move(x + running_segment_length,bottom);
                  p2->Move(x + running_segment_length,top);

                  CComPtr<iPointDisplayObject> doPnt1, doPnt2;
                  ::CoCreateInstance(CLSID_PointDisplayObject,NULL,CLSCTX_ALL,IID_iPointDisplayObject,(void**)&doPnt1);
                  ::CoCreateInstance(CLSID_PointDisplayObject,NULL,CLSCTX_ALL,IID_iPointDisplayObject,(void**)&doPnt2);
                  doPnt1->SetPosition(p1,FALSE,FALSE);
                  doPnt2->SetPosition(p2,FALSE,FALSE);

                  CComQIPtr<iConnectable,&IID_iConnectable> c1(doPnt1);
                  CComQIPtr<iConnectable,&IID_iConnectable> c2(doPnt2);

                  CComPtr<iSocket> s1, s2;
                  c1->AddSocket(0,p1,&s1);
                  c2->AddSocket(0,p2,&s2);

                  CComPtr<iLineDisplayObject> line;
                  ::CoCreateInstance(CLSID_LineDisplayObject,NULL,CLSCTX_ALL,IID_iLineDisplayObject,(void**)&line);

                  // color
                  CComPtr<iDrawLineStrategy> pStrategy;
                  line->GetDrawLineStrategy(&pStrategy);

                  // dangerous cast here
                  iSimpleDrawLineStrategy* pSimple = dynamic_cast<iSimpleDrawLineStrategy*>(pStrategy.p);
                  if (pSimple)
                  {
                     pSimple->SetColor(STIRRUP_COLOR);
                     pSimple->SetBeginType(leNone);
                     pSimple->SetEndType(leNone);
                  }
                  else
                  {
                     ATLASSERT(0);
                  }

                  // Attach connector to the sockets 
                  CComPtr<iConnector> connector;
                  line->QueryInterface(IID_iConnector,(void**)&connector);
                  CComPtr<iPlug> startPlug;
                  CComPtr<iPlug> endPlug;
                  connector->GetStartPlug(&startPlug);
                  connector->GetEndPlug(&endPlug);

                  // connect the line to the sockets
                  DWORD dwCookie;
                  s1->Connect(startPlug,&dwCookie);
                  s2->Connect(endPlug,&dwCookie);

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

void CGirderModelElevationView::BuildLine(iDisplayList* pDL, IPoint2d* fromPoint,IPoint2d* toPoint, COLORREF color,iDisplayObject** ppDO)
{
   // put points at locations and make them sockets
   CComPtr<iPointDisplayObject> from_rep;
   ::CoCreateInstance(CLSID_PointDisplayObject,NULL,CLSCTX_ALL,IID_iPointDisplayObject,(void**)&from_rep);
   from_rep->SetPosition(fromPoint,FALSE,FALSE);
   from_rep->SetID(m_DisplayObjectID++);
   CComQIPtr<iConnectable,&IID_iConnectable> from_connectable(from_rep);
   CComPtr<iSocket> from_socket;
   from_connectable->AddSocket(0,fromPoint,&from_socket);
   from_rep->Visible(FALSE);
   pDL->AddDisplayObject(from_rep);

   CComPtr<iPointDisplayObject> to_rep;
   ::CoCreateInstance(CLSID_PointDisplayObject,NULL,CLSCTX_ALL,IID_iPointDisplayObject,(void**)&to_rep);
   to_rep->SetPosition(toPoint,FALSE,FALSE);
   to_rep->SetID(m_DisplayObjectID++);
   CComQIPtr<iConnectable,&IID_iConnectable> to_connectable(to_rep);
   CComPtr<iSocket> to_socket;
   to_connectable->AddSocket(0,toPoint,&to_socket);
   to_rep->Visible(FALSE);
   pDL->AddDisplayObject(to_rep);

   // Create the dimension line object
   CComPtr<iLineDisplayObject> line;
   ::CoCreateInstance(CLSID_LineDisplayObject,NULL,CLSCTX_ALL,IID_iLineDisplayObject,(void**)&line);

   // color
   CComPtr<iDrawLineStrategy> pStrategy;
   line->GetDrawLineStrategy(&pStrategy);

   if ( ppDO )
   {
      (*ppDO) = line;
      (*ppDO)->AddRef();
   }

   // dangerous cast here
   iSimpleDrawLineStrategy* pSimple = dynamic_cast<iSimpleDrawLineStrategy*>(pStrategy.p);
   if (pSimple)
   {
      pSimple->SetColor(color);
   }
   else
      ATLASSERT(0);

   // Attach connector to the sockets 
   CComPtr<iConnector> connector;
   line->QueryInterface(IID_iConnector,(void**)&connector);
   CComPtr<iPlug> startPlug;
   CComPtr<iPlug> endPlug;
   connector->GetStartPlug(&startPlug);
   connector->GetEndPlug(&endPlug);

   // connect the line to the sockets
   DWORD dwCookie;
   from_socket->Connect(startPlug,&dwCookie);
   to_socket->Connect(endPlug,&dwCookie);

   line->SetID(m_DisplayObjectID++);

   pDL->AddDisplayObject(line);
}

void CGirderModelElevationView::BuildLine(iDisplayList* pDL, IPoint2d* fromPoint,IPoint2d* toPoint, COLORREF color1, COLORREF color2,iDisplayObject** ppDO)
{
   // put points at locations and make them sockets
   CComPtr<iPointDisplayObject> from_rep;
   ::CoCreateInstance(CLSID_PointDisplayObject,NULL,CLSCTX_ALL,IID_iPointDisplayObject,(void**)&from_rep);
   from_rep->SetPosition(fromPoint,FALSE,FALSE);
   from_rep->SetID(m_DisplayObjectID++);
   CComQIPtr<iConnectable,&IID_iConnectable> from_connectable(from_rep);
   CComPtr<iSocket> from_socket;
   from_connectable->AddSocket(0,fromPoint,&from_socket);
   from_rep->Visible(FALSE);
   pDL->AddDisplayObject(from_rep);

   CComPtr<iPointDisplayObject> to_rep;
   ::CoCreateInstance(CLSID_PointDisplayObject,NULL,CLSCTX_ALL,IID_iPointDisplayObject,(void**)&to_rep);
   to_rep->SetPosition(toPoint,FALSE,FALSE);
   to_rep->SetID(m_DisplayObjectID++);
   CComQIPtr<iConnectable,&IID_iConnectable> to_connectable(to_rep);
   CComPtr<iSocket> to_socket;
   to_connectable->AddSocket(0,toPoint,&to_socket);
   to_rep->Visible(FALSE);
   pDL->AddDisplayObject(to_rep);

   // Create the dimension line object
   CComPtr<iLineDisplayObject> line;
   ::CoCreateInstance(CLSID_LineDisplayObject,NULL,CLSCTX_ALL,IID_iLineDisplayObject,(void**)&line);

   if ( ppDO )
   {
      (*ppDO) = line;
      (*ppDO)->AddRef();
   }

   // color
   CComPtr<iSimpleDrawDashedLineStrategy> strategy;
   ::CoCreateInstance(CLSID_SimpleDrawDashedLineStrategy,NULL,CLSCTX_ALL,IID_iSimpleDrawDashedLineStrategy,(void**)&strategy);
   line->SetDrawLineStrategy(strategy);

   strategy->SetColor1(color1);
   strategy->SetColor2(color2);
   strategy->SetDashLength(10);

   // Attach connector to the sockets 
   CComPtr<iConnector> connector;
   line->QueryInterface(IID_iConnector,(void**)&connector);
   CComPtr<iPlug> startPlug;
   CComPtr<iPlug> endPlug;
   connector->GetStartPlug(&startPlug);
   connector->GetEndPlug(&endPlug);

   // connect the line to the sockets
   DWORD dwCookie;
   from_socket->Connect(startPlug,&dwCookie);
   to_socket->Connect(endPlug,&dwCookie);

   line->SetID(m_DisplayObjectID++);

   pDL->AddDisplayObject(line);
}

void CGirderModelElevationView::BuildDebondTick(iDisplayList* pDL, IPoint2d* tickPoint,COLORREF color)
{
   // put points at locations and make them sockets
   CComPtr<iPointDisplayObject> doPnt;
   ::CoCreateInstance(CLSID_PointDisplayObject,NULL,CLSCTX_ALL,IID_iPointDisplayObject,(void**)&doPnt);
   doPnt->SetPosition(tickPoint,FALSE,FALSE);
   doPnt->SetID(m_DisplayObjectID++);

   CComPtr<iSimpleDrawPointStrategy> strategy;
   ::CoCreateInstance(CLSID_SimpleDrawPointStrategy,NULL,CLSCTX_ALL,IID_iSimpleDrawPointStrategy,(void**)&strategy);
   strategy->SetColor(color);
   strategy->SetPointType(ptCircle);

   doPnt->SetDrawingStrategy(strategy);

   pDL->AddDisplayObject(doPnt);
}

void CGirderModelElevationView::BuildDimensionLine(iDisplayList* pDL, IPoint2d* fromPoint,IPoint2d* toPoint,Float64 dimension,iDimensionLine** ppDimLine)
{
   // put points at locations and make them sockets
   CComPtr<iPointDisplayObject> from_rep;
   ::CoCreateInstance(CLSID_PointDisplayObject,NULL,CLSCTX_ALL,IID_iPointDisplayObject,(void**)&from_rep);
   from_rep->SetPosition(fromPoint,FALSE,FALSE);
   from_rep->SetID(m_DisplayObjectID++);
   CComQIPtr<iConnectable,&IID_iConnectable> from_connectable(from_rep);
   CComPtr<iSocket> from_socket;
   from_connectable->AddSocket(0,fromPoint,&from_socket);
   from_rep->Visible(FALSE);
   pDL->AddDisplayObject(from_rep);

   CComPtr<iPointDisplayObject> to_rep;
   ::CoCreateInstance(CLSID_PointDisplayObject,NULL,CLSCTX_ALL,IID_iPointDisplayObject,(void**)&to_rep);
   to_rep->SetPosition(toPoint,FALSE,FALSE);
   to_rep->SetID(m_DisplayObjectID++);
   CComQIPtr<iConnectable,&IID_iConnectable> to_connectable(to_rep);
   CComPtr<iSocket> to_socket;
   to_connectable->AddSocket(0,toPoint,&to_socket);
   to_rep->Visible(FALSE);
   pDL->AddDisplayObject(to_rep);

   // Create the dimension line object
   CComPtr<iDimensionLine> dimLine;
   ::CoCreateInstance(CLSID_DimensionLineDisplayObject,NULL,CLSCTX_ALL,IID_iDimensionLine,(void**)&dimLine);

   dimLine->SetArrowHeadStyle(DManip::ahsFilled);

   // Attach connector (the dimension line) to the sockets 
   CComPtr<iConnector> connector;
   dimLine->QueryInterface(IID_iConnector,(void**)&connector);
   CComPtr<iPlug> startPlug;
   CComPtr<iPlug> endPlug;
   connector->GetStartPlug(&startPlug);
   connector->GetEndPlug(&endPlug);

   DWORD dwCookie;
   from_socket->Connect(startPlug,&dwCookie);
   to_socket->Connect(endPlug,&dwCookie);

   // Create the text block and attach it to the dimension line
   CComPtr<iTextBlock> textBlock;
   ::CoCreateInstance(CLSID_TextBlock,NULL,CLSCTX_ALL,IID_iTextBlock,(void**)&textBlock);

   // Format the dimension text
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   CString strDimension = FormatDimension(dimension,pDisplayUnits->GetSpanLengthUnit());

   textBlock->SetText(strDimension);

   dimLine->SetTextBlock(textBlock);

   // Assign the span id to the dimension line (so they are the same)
   dimLine->SetID(m_DisplayObjectID++);

   pDL->AddDisplayObject(dimLine);

   if ( ppDimLine )
   {
      (*ppDimLine) = dimLine;
      (*ppDimLine)->AddRef();
   }
}

void CGirderModelElevationView::OnGevCtxEditLoad() 
{
   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   DisplayObjectContainer selObjs;
   dispMgr->GetSelectedObjects(&selObjs);

   if (selObjs.size()==1)
   {
      DisplayObjectItem pid = *(selObjs.begin());

      CComQIPtr<iPointDisplayObject, &IID_iPointDisplayObject> pdo(pid.m_T);
      if (pdo!=NULL)
      {
         CComPtr<iDrawPointStrategy> strategy;
         pdo->GetDrawingStrategy(&strategy);

         CComQIPtr<iGevEditLoad, &IID_iGevEditLoad> pel(strategy);
         if (pel!=NULL)
         {
            pel->EditLoad();
            return;
         }
      }
   }

   CHECK(0);
}

void CGirderModelElevationView::OnGevCtxDeleteLoad() 
{
   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   DisplayObjectContainer selObjs;
   dispMgr->GetSelectedObjects(&selObjs);

   if (selObjs.size()==1)
   {
      DisplayObjectItem pid = *(selObjs.begin());

      CComQIPtr<iPointDisplayObject, &IID_iPointDisplayObject> pdo(pid.m_T);
      if (pdo!=NULL)
      {
         CComPtr<iDrawPointStrategy> strategy;
         pdo->GetDrawingStrategy(&strategy);

         CComQIPtr<iGevEditLoad, &IID_iGevEditLoad> pel(strategy);
         if (pel!=NULL)
         {
            pel->DeleteLoad();
            return;
         }
      }
   }

   CHECK(0);
}

void CGirderModelElevationView::OnDestroy() 
{
   // free up our connectable objects so they don't leak
   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   CollectionIndexType dlcnt = dispMgr->GetDisplayListCount();
   for (CollectionIndexType idl=0; idl<dlcnt; idl++)
   {
      CComPtr<iDisplayList> dlist;
      dispMgr->GetDisplayList(idl, &dlist);

      CollectionIndexType docnt = dlist->GetDisplayObjectCount();
      for (CollectionIndexType ido=0; ido<docnt; ido++)
      {
         CComPtr<iDisplayObject> pdo;
         dlist->GetDisplayObject(ido,&pdo);

         CComQIPtr<iConnectable,&IID_iConnectable> connectable(pdo);

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
      CFont* pOldFont = NULL;
      if ( font.CreatePointFont(100,_T("Arial"),pDC) )
         pOldFont = pDC->SelectObject(&font);

      MultiLineTextOut(pDC,0,0,msg);

      if ( pOldFont )
         pDC->SelectObject(pOldFont);
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
      CFont* pOldFont = NULL;
      if ( font.CreatePointFont(100,_T("Arial"),pDC) )
         pOldFont = pDC->SelectObject(&font);

      MultiLineTextOut(pDC,0,0,msg);

      if ( pOldFont )
         pDC->SelectObject(pOldFont);
   }
}

BOOL CGirderModelElevationView::OnMouseWheel(UINT nFlags,short zDelta,CPoint pt)
{
   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   DisplayObjectContainer selObjs;
   dispMgr->GetSelectedObjects(&selObjs);

   if ( selObjs.size() == 0 || selObjs.front().m_T->GetID() != SECTION_CUT_ID )
      return FALSE;

   if ( 0 < zDelta )
      m_pFrame->CutAtPrev();
   else
      m_pFrame->CutAtNext();

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
   CGirderKey girderKey = m_pFrame->GetSelection();
   //if ( girderKey.groupIndex == INVALID_INDEX )
   //{
   //   pgsPointOfInterest poi = GetCutLocation();
   //   girderKey = poi.GetSegmentKey();
   //}

   //ATLASSERT(girderKey.groupIndex != INVALID_INDEX && girderKey.girderIndex != INVALID_INDEX);
   return girderKey;
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

Float64 CGirderModelElevationView::GetSpanStartLocation(const CSpanGirderKey& spanGirderKey)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   GET_IFACE2(pBroker,IBridge,           pBridge);

   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   SpanIndexType startSpanIdx = 0;
   SpanIndexType endSpanIdx   = spanGirderKey.spanIndex;

   Float64 span_offset = 0;

   for (SpanIndexType spanIdx = startSpanIdx; spanIdx < endSpanIdx; spanIdx++ )
   {
      Float64 span_length = pBridge->GetSpanLength(spanIdx,spanGirderKey.girderIndex);
      span_offset += span_length;
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
   const matPsStrand* pStrand     = pMaterials->GetStrandMaterial(segmentKey,pgsTypes::Permanent);
   const matPsStrand* pTempStrand = pMaterials->GetStrandMaterial(segmentKey,pgsTypes::Temporary);

   StrandIndexType Ns, Nh, Nt, Nsd;
   Ns = pStrandGeom->GetNumStrands(segmentKey,pgsTypes::Straight);
   Nh = pStrandGeom->GetNumStrands(segmentKey,pgsTypes::Harped);
   Nt = pStrandGeom->GetNumStrands(segmentKey,pgsTypes::Temporary);
   Nsd= pStrandGeom->GetNumDebondedStrands(segmentKey,pgsTypes::Straight);

   std::_tstring harp_type(LABEL_HARP_TYPE(pStrandGeom->GetAreHarpedStrandsForcedStraight(segmentKey)));

   CString strMsg3;
   if ( pStrandGeom->GetMaxStrands(segmentKey,pgsTypes::Temporary) != 0 )
   {
      if ( Nsd == 0 )
      {
         strMsg3.Format(_T("\r\n\r\nStrand: %s\r\n# Straight: %2d\r\n# %s: %2d\r\n\r\nStrand: %s\r\n# Temporary: %2d"),
                         pStrand->GetName().c_str(),Ns,harp_type.c_str(),Nh,pTempStrand->GetName().c_str(),Nt);
      }
      else
      {
         strMsg3.Format(_T("\r\n\r\nStrand: %s\r\n# Straight: %2d (%2d Debonded)\r\n# %s: %2d\r\n\r\nStrand: %s\r\n# Temporary: %2d"),
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
