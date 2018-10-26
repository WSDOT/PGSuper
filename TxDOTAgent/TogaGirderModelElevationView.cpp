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

// TogaGirderModelElevationView.cpp : implementation file
//
#include "stdafx.h"
#include "TogaGirderModelElevationView.h"
#include "TxDOTOptionalDesignDoc.h"

#include "PGSuperUnits.h"
#include "PGSuperColors.h"

#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <IFace\DrawBridgeSettings.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\EditByUI.h>

#include <PgsExt\BridgeDescription.h>

#include "TogaDisplayObjectFactory.h"
#include "TogaSupportDrawStrategyImpl.h"
#include "TogaSectionCutDrawStrategy.h"
#include "TogaSectionCutDisplayImpl.h"
#include "TogaGMDisplayMgrEventsImpl.h"

#include <MfcTools\Text.h>
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
   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   dispMgr->SetSelectionLineColor(SELECTED_OBJECT_LINE_COLOR);
   dispMgr->SetSelectionFillColor(SELECTED_OBJECT_FILL_COLOR);

   CTxDOTOptionalDesignDoc* pDoc = (CTxDOTOptionalDesignDoc*)GetDocument();
   CTogaDisplayObjectFactory* factory = new CTogaDisplayObjectFactory(pDoc);
   IUnknown* unk = factory->GetInterface(&IID_iDisplayObjectFactory);
   dispMgr->AddDisplayObjectFactory((iDisplayObjectFactory*)unk);

   // add factory from in dmaniptools
   CComPtr<iDisplayObjectFactory> pfac2;
   hr = pfac2.CoCreateInstance(CLSID_DManipToolsDisplayObjectFactory);
   ATLASSERT(SUCCEEDED(hr));
   dispMgr->AddDisplayObjectFactory(pfac2);

   // set up default event handler for canvas
   CTogaGMDisplayMgrEventsImpl* events = new CTogaGMDisplayMgrEventsImpl(pDoc, m_pFrame, this);
   unk = events->GetInterface(&IID_iDisplayMgrEvents);
   dispMgr->RegisterEventSink((iDisplayMgrEvents*)unk);

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

   // supports
   CComPtr<iDisplayList> sup_list;
   ::CoCreateInstance(CLSID_DisplayList,NULL,CLSCTX_ALL,IID_iDisplayList,(void**)&sup_list);
   sup_list->SetID(SUPPORT_LIST);
   dispMgr->AddDisplayList(sup_list);

   // debond strands
   CComPtr<iDisplayList> db_list;
   ::CoCreateInstance(CLSID_DisplayList,NULL,CLSCTX_ALL,IID_iDisplayList,(void**)&db_list);
   db_list->SetID(DEBOND_LIST);
   dispMgr->AddDisplayList(db_list);

   // strands
   CComPtr<iDisplayList> ts_list;
   ::CoCreateInstance(CLSID_DisplayList,NULL,CLSCTX_ALL,IID_iDisplayList,(void**)&ts_list);
   ts_list->SetID(STRAND_LIST);
   dispMgr->AddDisplayList(ts_list);

   // strand cg
   CComPtr<iDisplayList> cg_list;
   ::CoCreateInstance(CLSID_DisplayList,NULL,CLSCTX_ALL,IID_iDisplayList,(void**)&cg_list);
   cg_list->SetID(STRAND_CG_LIST);
   dispMgr->AddDisplayList(cg_list);

   // rebar
   CComPtr<iDisplayList> rb_list;
   ::CoCreateInstance(CLSID_DisplayList,NULL,CLSCTX_ALL,IID_iDisplayList,(void**)&rb_list);
   rb_list->SetID(REBAR_LIST);
   dispMgr->AddDisplayList(rb_list);

   // strirrups
   CComPtr<iDisplayList> stirrups_list;
   ::CoCreateInstance(CLSID_DisplayList,NULL,CLSCTX_ALL,IID_iDisplayList,(void**)&stirrups_list);
   stirrups_list->SetID(STIRRUP_LIST);
   dispMgr->AddDisplayList(stirrups_list);

   // girder
   CComPtr<iDisplayList> gdr_list;
   ::CoCreateInstance(CLSID_DisplayList,NULL,CLSCTX_ALL,IID_iDisplayList,(void**)&gdr_list);
   gdr_list->SetID(GDR_LIST);
   dispMgr->AddDisplayList(gdr_list);
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
   else
      ASSERT(0);

	Invalidate(TRUE);
}

void CTogaGirderModelElevationView::UpdateDisplayObjects()
{
   // get the display manager
   CComPtr<iDisplayMgr> dispMgr;
   GetDisplayMgr(&dispMgr);

   // clean out all the display objects
   dispMgr->ClearDisplayObjects();

   CTxDOTOptionalDesignDoc* pDoc = (CTxDOTOptionalDesignDoc*)GetDocument();

   SpanIndexType span;
   GirderIndexType girder;

   m_pFrame->GetSpanAndGirderSelection(&span,&girder);

   // Grab hold of the broker so we can pass it as a parameter
   CComPtr<IBroker> pBroker = pDoc->GetUpdatedBroker();

   UINT settings = pDoc->GetGirderEditorSettings();

   BuildGirderDisplayObjects(pDoc, pBroker, span, girder, dispMgr);
   BuildSupportDisplayObjects(pDoc, pBroker, span, girder, dispMgr);

   if (settings & IDG_EV_SHOW_STRANDS)
      BuildStrandDisplayObjects(pDoc, pBroker, span, girder, dispMgr);

   if (settings & IDG_EV_SHOW_PS_CG)
      BuildStrandCGDisplayObjects(pDoc, pBroker, span, girder, dispMgr);

   if (settings & IDG_EV_SHOW_LONG_REINF)
      BuildRebarDisplayObjects(pDoc, pBroker, span, girder, dispMgr);

   if (settings & IDG_EV_SHOW_STIRRUPS)
      BuildStirrupDisplayObjects(pDoc, pBroker, span, girder, dispMgr);

   if (settings & IDG_EV_SHOW_DIMENSIONS)
      BuildDimensionDisplayObjects(pDoc, pBroker, span, girder, dispMgr);

   BuildSectionCutDisplayObjects(pDoc, pBroker, span, girder, dispMgr);

   DManip::MapMode mode = (settings & IDG_EV_DRAW_ISOTROPIC) ? DManip::Isotropic : DManip::Anisotropic;
   CDisplayView::SetMappingMode(mode);
}

void CTogaGirderModelElevationView::DoPrint(CDC* pDC, CPrintInfo* pInfo)
{
   this->OnBeginPrinting(pDC, pInfo);
   this->OnPrepareDC(pDC);
   this->ScaleToFit();
   this->OnDraw(pDC);
   this->OnEndPrinting(pDC, pInfo);
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

DROPEFFECT CTogaGirderModelElevationView::CanDrop(COleDataObject* pDataObject,DWORD dwKeyState,IPoint2d* point)
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
      CComPtr<iDragDataSource> source;               
      ::CoCreateInstance(CLSID_DragDataSource,NULL,CLSCTX_ALL,IID_iDragDataSource,(void**)&source);
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
         CComQIPtr<iDraggable> drag(m_Legend);
         UINT format = drag->Format();
         if ( pDataObject->IsDataAvailable(format) )
            return DROPEFFECT_MOVE;
      }
   }

   return DROPEFFECT_NONE;
}

void CTogaGirderModelElevationView::OnDropped(COleDataObject* pDataObject,DROPEFFECT dropEffect,IPoint2d* point)
{
//   AfxMessageBox("OnDropped");
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
      this->GetClientRect(&rect);
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
}

void CTogaGirderModelElevationView::OnViewSettings() 
{
	((CTxDOTOptionalDesignDoc*)GetDocument())->EditGirderViewSettings(VS_GIRDER_ELEVATION);
}

void CTogaGirderModelElevationView::BuildGirderDisplayObjects(CTxDOTOptionalDesignDoc* pDoc,IBroker* pBroker, SpanIndexType span,GirderIndexType girder,iDisplayMgr* dispMgr)
{
   // get the display list and clear out any old display objects
   CComPtr<iDisplayList> pDL;
   dispMgr->FindDisplayList(GDR_LIST,&pDL);
   ATLASSERT(pDL);
   pDL->Clear();

   // the origin of the coordinate system is at the left end of the girder
   // at the CG
   GET_IFACE2(pBroker,ISectProp2,pSectProp2);
   pgsPointOfInterest poi(span,girder,0.00); // start of girder
   Float64 Yb = pSectProp2->GetYb(pgsTypes::CastingYard,poi);

   // get the shape of the girder in profile
   GET_IFACE2(pBroker,IGirder,pGirder);
   CComPtr<IShape> shape;
   pGirder->GetProfileShape(span,girder,&shape);

   // create the display object
   CComPtr<iPointDisplayObject> doPnt;
   ::CoCreateInstance(CLSID_PointDisplayObject,NULL,CLSCTX_ALL,IID_iPointDisplayObject,(void**)&doPnt);
   doPnt->SetID(1);

   // create the drawing strategy
   CComPtr<iShapeDrawStrategy> strategy;
   ::CoCreateInstance(CLSID_ShapeDrawStrategy,NULL,CLSCTX_ALL,IID_iShapeDrawStrategy,(void**)&strategy);
   doPnt->SetDrawingStrategy(strategy);

   // configure the strategy
   strategy->SetShape(shape);
   strategy->SetSolidLineColor(GIRDER_BORDER_COLOR);
   strategy->SetSolidFillColor(GIRDER_FILL_COLOR);
   strategy->SetVoidLineColor(VOID_BORDER_COLOR);
   strategy->SetVoidFillColor(GetSysColor(COLOR_WINDOW));
   strategy->DoFill(true);

   // set the tool tip text
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 gdr_length, span_length;
   gdr_length  = pBridge->GetGirderLength(span,girder);
   span_length = pBridge->GetSpanLength(span,girder);
   CString strMsg1;
   strMsg1.Format(_T("Girder: %s\r\nGirder Length: %s\r\nSpan Length: %s"),
                  pBridgeDesc->GetSpan(span)->GetGirderTypes()->GetGirderName(girder),
                  FormatDimension(gdr_length,pDisplayUnits->GetSpanLengthUnit()),
                  FormatDimension(span_length,pDisplayUnits->GetSpanLengthUnit())
                  );

   GET_IFACE2(pBroker,IBridgeMaterial,pBridgeMaterial);
   Float64 fc, fci;
   fc  = pBridgeMaterial->GetFcGdr(span,girder);
   fci = pBridgeMaterial->GetFciGdr(span,girder);

   CString strMsg2;
   strMsg2.Format(_T("\r\n\r\nf'ci: %s\r\nf'c: %s"),
                  FormatDimension(fci,pDisplayUnits->GetStressUnit()),
                  FormatDimension(fc, pDisplayUnits->GetStressUnit())
                  );

   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeom);
   const matPsStrand* pStrand = pBridgeMaterial->GetStrand(span,girder,pgsTypes::Permanent);
   const matPsStrand* pTempStrand = pBridgeMaterial->GetStrand(span,girder,pgsTypes::Temporary);

   StrandIndexType Ns, Nh, Nt, Nsd;
   Ns = pStrandGeom->GetNumStrands(span,girder,pgsTypes::Straight);
   Nh = pStrandGeom->GetNumStrands(span,girder,pgsTypes::Harped);
   Nt = pStrandGeom->GetNumStrands(span,girder,pgsTypes::Temporary);
   Nsd= pStrandGeom->GetNumDebondedStrands(span,girder,pgsTypes::Straight);

   std::_tstring harp_type(LABEL_HARP_TYPE(pStrandGeom->GetAreHarpedStrandsForcedStraight(span,girder)));

   CString strMsg3;
   if ( pStrandGeom->GetMaxStrands(span,girder,pgsTypes::Temporary) != 0 )
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

   // make starting point
   CComPtr<IPoint2d> point;
   point.CoCreateInstance(__uuidof(Point2d));
   point->put_X(0.0);
   point->put_Y(0.0);

   CComPtr<iPointDisplayObject> startpt_rep;
   ::CoCreateInstance(CLSID_PointDisplayObject,NULL,CLSCTX_ALL,IID_iPointDisplayObject,(void**)&startpt_rep);
   startpt_rep->SetPosition(point,FALSE,FALSE);
   startpt_rep->SetID(100);
   CComQIPtr<iConnectable,&IID_iConnectable> start_connectable(startpt_rep);
   CComPtr<iSocket> start_socket;
   start_connectable->AddSocket(0,point,&start_socket);
   pDL->AddDisplayObject(startpt_rep);

   CComPtr<IPoint2d> point2;
   point2.CoCreateInstance(__uuidof(Point2d));
   point2->put_X(gdr_length);
   point2->put_Y(0.00);

   // end point display object
   CComPtr<iPointDisplayObject> endpt_rep;
   ::CoCreateInstance(CLSID_PointDisplayObject,NULL,CLSCTX_ALL,IID_iPointDisplayObject,(void**)&endpt_rep);
   endpt_rep->SetPosition(point2,FALSE,FALSE);
   endpt_rep->SetID(200);
   CComQIPtr<iConnectable,&IID_iConnectable> end_connectable(endpt_rep);
   CComPtr<iSocket> end_socket;
   end_connectable->AddSocket(0,point2,&end_socket);

   pDL->AddDisplayObject(endpt_rep);

   // line to represent girder
   CComPtr<iLineDisplayObject> ssm_rep;
   ::CoCreateInstance(CLSID_LineDisplayObject,NULL,CLSCTX_ALL,IID_iLineDisplayObject,(void**)&ssm_rep);
   ssm_rep->SetID(101);

   // plug in
   CComQIPtr<iConnector,&IID_iConnector> connector(ssm_rep);

   CComPtr<iPlug> startPlug;
   connector->GetStartPlug(&startPlug);

   CComPtr<iPlug> endPlug;
   connector->GetEndPlug(&endPlug);

   CComQIPtr<iConnectable,&IID_iConnectable> startConnectable(startpt_rep);
   CComPtr<iSocket> socket;
   startConnectable->GetSocket(0,atByIndex,&socket);
   DWORD dwCookie;
   socket->Connect(startPlug,&dwCookie);

   CComQIPtr<iConnectable,&IID_iConnectable> endConnectable(endpt_rep);
   socket.Release();
   endConnectable->GetSocket(0,atByIndex,&socket);
   socket->Connect(endPlug,&dwCookie);

   ///////////////////////////////////////////////////////////////////////////////////////////
}

void CTogaGirderModelElevationView::BuildSupportDisplayObjects(CTxDOTOptionalDesignDoc* pDoc, IBroker* pBroker, SpanIndexType span,GirderIndexType girder,iDisplayMgr* dispMgr)
{
   CComPtr<iDisplayList> pDL;
   dispMgr->FindDisplayList(SUPPORT_LIST,&pDL);
   ATLASSERT(pDL);
   pDL->Clear();

   // Get location to display the connection symbols
   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 gdr_length = pBridge->GetGirderLength(span, girder);
   Float64 start_lgth = pBridge->GetGirderStartConnectionLength(span, girder);
   Float64 end_lgth   = pBridge->GetGirderEndConnectionLength(span, girder);

   // Get POI for these locations
   pgsPointOfInterest poiStartBrg(span,girder,start_lgth);
   pgsPointOfInterest poiEndBrg(span,girder,gdr_length - end_lgth);

   // Display at bottom of girder.
   GET_IFACE2(pBroker,IGirder,pGirder);
   Float64 Hg_start = pGirder->GetHeight(poiStartBrg);
   Float64 Hg_end   = pGirder->GetHeight(poiEndBrg);

   //
   // left support
   //

   // create point
   CComPtr<IPoint2d> point;
   point.CoCreateInstance(__uuidof(Point2d));
   point->put_X(start_lgth);
   point->put_Y(-Hg_start);

   // create display object
   CComPtr<iPointDisplayObject> dispObj;
   ::CoCreateInstance(CLSID_PointDisplayObject,NULL,CLSCTX_ALL,IID_iPointDisplayObject,(void**)&dispObj);

   // create drawing strategy
   CTogaSupportDrawStrategyImpl* pDrawStrategy = new CTogaSupportDrawStrategyImpl(pDoc);
   IUnknown* unk = pDrawStrategy->GetInterface(&IID_iDrawPointStrategy);
   dispObj->SetDrawingStrategy((iDrawPointStrategy*)unk);
   unk->Release();

   CComQIPtr<iPointDisplayObject,&IID_iPointDisplayObject> ptDispObj(dispObj);

   CComPtr<iDrawPointStrategy> ds;
   ptDispObj->GetDrawingStrategy(&ds);

   CComQIPtr<iPointDisplayObject,&IID_iPointDisplayObject> supportRep(dispObj);
   supportRep->SetPosition(point,FALSE,FALSE);
   supportRep->SetID(0);

   pDL->AddDisplayObject(supportRep);

   // right support
   CComPtr<IPoint2d> point2;
   point2.CoCreateInstance(__uuidof(Point2d));
   point2->put_X(gdr_length-end_lgth);
   point2->put_Y(-Hg_end);
   dispObj.Release();
   ::CoCreateInstance(CLSID_PointDisplayObject,NULL,CLSCTX_ALL,IID_iPointDisplayObject,(void**)&dispObj);

   pDrawStrategy = new CTogaSupportDrawStrategyImpl(pDoc);
   unk = pDrawStrategy->GetInterface(&IID_iDrawPointStrategy);
   dispObj->SetDrawingStrategy((iDrawPointStrategy*)unk);

   CComQIPtr<iPointDisplayObject,&IID_iPointDisplayObject> ptDispObj2(dispObj);

   ds.Release();
   ptDispObj2->GetDrawingStrategy(&ds);

   CComQIPtr<iPointDisplayObject,&IID_iPointDisplayObject> supportRep2(dispObj);
   supportRep2->SetPosition(point2,FALSE,FALSE);
   supportRep2->SetID(1);

   pDL->AddDisplayObject(supportRep2);

}

void CTogaGirderModelElevationView::BuildStrandDisplayObjects(CTxDOTOptionalDesignDoc* pDoc, IBroker* pBroker, SpanIndexType span,GirderIndexType girder,iDisplayMgr* dispMgr)
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
   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeometry);
   GET_IFACE2(pBroker,IGirder,pGirder);
   GET_IFACE2(pBroker,IPointOfInterest,pPOI);

   Float64 gdr_length = pBridge->GetGirderLength(span, girder);

   Float64 lft_harp, rgt_harp;
   pStrandGeometry->GetHarpingPointLocations(span, girder, &lft_harp, &rgt_harp);

   CComPtr<IPoint2d> from_point, to_point;
   from_point.CoCreateInstance(__uuidof(Point2d));
   to_point.CoCreateInstance(__uuidof(Point2d));
   from_point->put_X(0.0);
   to_point->put_X(gdr_length);

   std::vector<pgsPointOfInterest> vPOI = pPOI->GetPointsOfInterest(span,girder,pgsTypes::CastingYard,POI_HARPINGPOINT);
   ATLASSERT( 0 <= vPOI.size() && vPOI.size() < 3 );
   pgsPointOfInterest hp1_poi;
   pgsPointOfInterest hp2_poi;

   if ( 0 < vPOI.size() )
   {
      hp1_poi = vPOI.front();
      hp2_poi = vPOI.back();
   }

   pgsPointOfInterest start_poi(span, girder, 0.0);
   pgsPointOfInterest end_poi(span, girder, gdr_length);

   // Need to use POI and Hg at harp points
   Float64 HgStart = pGirder->GetHeight(start_poi);
   Float64 HgEnd   = pGirder->GetHeight(end_poi);
   Float64 HgHP1, HgHP2;
   if ( 0 < vPOI.size() )
   {
      HgHP1 = pGirder->GetHeight(hp1_poi);
      HgHP2 = pGirder->GetHeight(hp2_poi);
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

      from_point->put_Y(yStart - HgStart);
      to_point->put_Y(yEnd - HgEnd);
      
      BuildLine(pDL, from_point, to_point, STRAND_FILL_COLOR);

      Float64 start,end;
      if ( pStrandGeometry->IsStrandDebonded(span,girder,strandPointIdx,pgsTypes::Straight,&start,&end) )
      {
         // Left debond point
         CComPtr<IPoint2d> left_debond;
         left_debond.CoCreateInstance(CLSID_Point2d);
         left_debond->Move(start,yStart-HgStart);

         BuildLine(pDebondDL, from_point, left_debond, DEBOND_FILL_COLOR );
         BuildDebondTick(pDebondDL, left_debond, DEBOND_FILL_COLOR );

         CComPtr<IPoint2d> right_debond;
         right_debond.CoCreateInstance(CLSID_Point2d);
         right_debond->Move(end,yEnd-HgEnd);

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
         start_pos->get_X(&start_x);
         start_pos->get_Y(&start_y);

         Float64 hp1_x, hp1_y;
         hp1_pos->get_X(&hp1_x);
         hp1_pos->get_Y(&hp1_y);

         Float64 hp2_x, hp2_y;
         hp2_pos->get_X(&hp2_x);
         hp2_pos->get_Y(&hp2_y);

         Float64 end_x, end_y;
         end_pos->get_X(&end_x);
         end_pos->get_Y(&end_y);

         from_point->put_X(0.0);
         from_point->put_Y(start_y-HgStart);
         to_point->put_X(lft_harp);
         to_point->put_Y(hp1_y-HgHP1);
         
         BuildLine(pDL, from_point, to_point, STRAND_FILL_COLOR);

         from_point->put_X(lft_harp);
         from_point->put_Y(hp1_y-HgHP1);
         to_point->put_X(rgt_harp);
         to_point->put_Y(hp2_y-HgHP2);
         
         BuildLine(pDL, from_point, to_point, STRAND_FILL_COLOR);

         from_point->put_X(rgt_harp);
         from_point->put_Y(hp2_y-HgHP2);
         to_point->put_X(gdr_length);
         to_point->put_Y(end_y-HgEnd);

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
      from_point->put_X(0.0);
      pntStart->get_Y(&y);
      from_point->put_Y(y-HgStart);

      to_point->put_X(gdr_length);
      pntEnd->get_Y(&y);
      to_point->put_Y(y-HgEnd);
      
      BuildLine(pDL, from_point, to_point, STRAND_FILL_COLOR);
   }
}

void CTogaGirderModelElevationView::BuildStrandCGDisplayObjects(CTxDOTOptionalDesignDoc* pDoc, IBroker* pBroker, SpanIndexType span,GirderIndexType girder,iDisplayMgr* dispMgr)
{
   CComPtr<iDisplayList> pDL;
   dispMgr->FindDisplayList(STRAND_CG_LIST,&pDL);
   ATLASSERT(pDL);
   pDL->Clear();

   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeometry);
   GET_IFACE2(pBroker,ISectProp2,pSectProp2);

   pgsTypes::Stage stage = pgsTypes::BridgeSite1;

   StrandIndexType ns = pStrandGeometry->GetNumStrands(span, girder, pgsTypes::Straight);
   ns += pStrandGeometry->GetNumStrands(span, girder, pgsTypes::Harped);
   ns += pStrandGeometry->GetNumStrands(span, girder, pgsTypes::Temporary);
   if (0 < ns)
   {
      CComPtr<IPoint2d> from_point, to_point;
      from_point.CoCreateInstance(__uuidof(Point2d));
      to_point.CoCreateInstance(__uuidof(Point2d));

      bool red = false;

      GET_IFACE2(pBroker,IPointOfInterest,pPOI);
      std::vector<pgsPointOfInterest> vPOI = pPOI->GetPointsOfInterest(span,girder,stage,POI_ALLACTIONS | POI_ALLOUTPUT,POIFIND_OR);

      Float64 from_y;
      Float64 to_y;
      std::vector<pgsPointOfInterest>::iterator iter = vPOI.begin();
      pgsPointOfInterest prev_poi = *iter;

      Float64 hg  = pSectProp2->GetHg(pgsTypes::CastingYard,prev_poi);
      Float64 ybg = pSectProp2->GetYb(pgsTypes::CastingYard,prev_poi);
      Float64 nEff;
      Float64 ex = pStrandGeometry->GetEccentricity(prev_poi, false, &nEff);
      from_y = ybg - ex - hg;

      for ( ; iter!= vPOI.end(); iter++ )
      {
         pgsPointOfInterest& poi = *iter;
      
         hg  = pSectProp2->GetHg(pgsTypes::CastingYard,poi);
         ybg = pSectProp2->GetYb(pgsTypes::CastingYard,poi);
         ex = pStrandGeometry->GetEccentricity(poi, false, &nEff);
         to_y = ybg - ex - hg;

         from_point->put_X(prev_poi.GetDistFromStart());
         from_point->put_Y(from_y);
         to_point->put_X(poi.GetDistFromStart());
         to_point->put_Y(to_y);

         COLORREF col = red ? RED : WHITE;

         BuildLine(pDL, from_point, to_point, col);

         red = !red;

         prev_poi = poi;
         from_y = to_y;
      }
   }
}

void CTogaGirderModelElevationView::BuildRebarDisplayObjects(CTxDOTOptionalDesignDoc* pDoc, IBroker* pBroker, SpanIndexType span,GirderIndexType girder,iDisplayMgr* dispMgr)
{
   CComPtr<iDisplayList> pDL;
   dispMgr->FindDisplayList(REBAR_LIST,&pDL);
   ATLASSERT(pDL);
   pDL->Clear();

   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,ILongRebarGeometry,pLongRebarGeometry);
   GET_IFACE2(pBroker,IPointOfInterest,pPOI);
   GET_IFACE2(pBroker,IGirder,pGirder);

   Float64 gdr_length = pBridge->GetGirderLength(span, girder);

   pgsPointOfInterest poiStart = pPOI->GetPointOfInterest(pgsTypes::CastingYard,span,girder,0.0);
   pgsPointOfInterest poiEnd   = pPOI->GetPointOfInterest(pgsTypes::CastingYard,span,girder,gdr_length);

   Float64 HgStart = pGirder->GetHeight(poiStart);
   Float64 HgEnd   = pGirder->GetHeight(poiEnd);

   if ( poiStart.GetID() == INVALID_ID )
      poiStart.SetDistFromStart(0.0);

   if ( poiEnd.GetID() == INVALID_ID )
      poiEnd.SetDistFromStart(gdr_length);

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

      from_point->put_X(0.0);
      from_point->put_Y(yStart-HgStart);

      to_point->put_X(gdr_length);
      to_point->put_Y(yEnd-HgEnd);
      
      BuildLine(pDL, from_point, to_point, REBAR_COLOR);
   }
}

void CTogaGirderModelElevationView::BuildDimensionDisplayObjects(CTxDOTOptionalDesignDoc* pDoc, IBroker* pBroker, SpanIndexType span,GirderIndexType girder,iDisplayMgr* dispMgr)
{
   CComPtr<iDisplayList> pDL;
   dispMgr->FindDisplayList(DIMLINE_LIST,&pDL);
   ATLASSERT(pDL);
   pDL->Clear();

   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IGirder,pGirder);
   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeometry);

   Float64 gdr_length  = pBridge->GetGirderLength(span, girder);
   Float64 start_lgth  = pBridge->GetGirderStartConnectionLength(span, girder);
   Float64 end_lgth    = pBridge->GetGirderEndConnectionLength(span, girder);
   Float64 span_length = pBridge->GetSpanLength(span,girder);

   StrandIndexType Nh = pStrandGeometry->GetNumStrands(span,girder,pgsTypes::Harped);

   Float64 Hg_start = pGirder->GetHeight(pgsPointOfInterest(span,girder,0.00) );
   Float64 Hg_end   = pGirder->GetHeight(pgsPointOfInterest(span,girder,gdr_length) );

   Float64 lft_harp, rgt_harp;
   pStrandGeometry->GetHarpingPointLocations(span, girder, &lft_harp, &rgt_harp);

   CComPtr<IPoint2d> from_point, to_point;
   from_point.CoCreateInstance(__uuidof(Point2d));
   to_point.CoCreateInstance(__uuidof(Point2d));

   CComPtr<iDimensionLine> dimLine;

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
   from_point->put_X(start_lgth);
   from_point->put_Y(-max(Hg_start,Hg_end));
   to_point->put_X(gdr_length-end_lgth);
   to_point->put_Y(-max(Hg_start,Hg_end));
   dimLine = BuildDimensionLine(pDL, from_point, to_point, span_length);
   dimLine->SetWitnessLength(-twip_offset);

   // harp locations
   if ( 0 < Nh )
   {
      to_point->put_X(lft_harp);
      dimLine = BuildDimensionLine(pDL, from_point, to_point, lft_harp-start_lgth);
      dimLine->SetWitnessLength(-twip_offset/2);

      from_point->put_X(rgt_harp);
      to_point->put_X(gdr_length-end_lgth);
      dimLine = BuildDimensionLine(pDL, from_point, to_point, gdr_length-end_lgth-rgt_harp);
      dimLine->SetWitnessLength(-twip_offset/2);
   }
}

void CTogaGirderModelElevationView::BuildSectionCutDisplayObjects(CTxDOTOptionalDesignDoc* pDoc, IBroker* pBroker, SpanIndexType span,GirderIndexType girder,iDisplayMgr* dispMgr)
{
   CComPtr<iDisplayObjectFactory> factory;
   dispMgr->GetDisplayObjectFactory(0, &factory);

   CComPtr<iDisplayObject> disp_obj;
   factory->Create(CTogaSectionCutDisplayImpl::ms_Format,NULL,&disp_obj);

   CComPtr<iDisplayObjectEvents> sink;
   disp_obj->GetEventSink(&sink);

   CComQIPtr<iPointDisplayObject,&IID_iPointDisplayObject> point_disp(disp_obj);
   point_disp->SetToolTipText(_T("Click on me and drag to move section cut"));

   CComQIPtr<iTogaSectionCutDrawStrategy,&IID_iTogaSectionCutDrawStrategy> sc_strat(sink);
   sc_strat->Init(point_disp, pBroker, span, girder, m_pFrame);
   sc_strat->SetColor(CUT_COLOR);

   point_disp->SetID(SECTION_CUT_ID);

   CComPtr<iDisplayList> pDL;
   dispMgr->FindDisplayList(SECT_CUT_LIST,&pDL);
   ATLASSERT(pDL);
   pDL->Clear();
   pDL->AddDisplayObject(disp_obj);
}

void CTogaGirderModelElevationView::BuildStirrupDisplayObjects(CTxDOTOptionalDesignDoc* pDoc, IBroker* pBroker,SpanIndexType span,GirderIndexType girder,iDisplayMgr* dispMgr)
{
   CComPtr<iDisplayList> pDL;
   dispMgr->FindDisplayList(STIRRUP_LIST,&pDL);
   ATLASSERT(pDL);
   pDL->Clear();

   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IGirder,pGirder);
   GET_IFACE2(pBroker,IStirrupGeometry,pStirrupGeom);
   GET_IFACE2(pBroker,IPointOfInterest,pPOI);


   // assume a typical cover
   Float64 top_cover = ::ConvertToSysUnits(2.0,unitMeasure::Inch);
   Float64 bot_cover = ::ConvertToSysUnits(2.0,unitMeasure::Inch);

   Float64 slab_offset = pBridge->GetSlabOffset(span,girder,pgsTypes::metStart); // use for dummy top of stirrup if they are extended into deck

   pgsTypes::SupportedDeckType deckType = pBridge->GetDeckType();
   bool bDoStirrupsEngageDeck = pStirrupGeom->DoStirrupsEngageDeck(span,girder);

   ZoneIndexType nStirrupZones = pStirrupGeom->GetNumPrimaryZones(span,girder);
   for ( ZoneIndexType zoneIdx = 0; zoneIdx < nStirrupZones; zoneIdx++ )
   {
      Float64 start, end;
      pStirrupGeom->GetPrimaryZoneBounds(span , girder, zoneIdx, &start, &end);

      matRebar::Size barSize;
      Float64 spacing;
      Float64 nStirrups;
      pStirrupGeom->GetPrimaryVertStirrupBarInfo(span,girder,zoneIdx,&barSize,&nStirrups,&spacing);

      if ( barSize != matRebar::bsNone && nStirrups != 0 )
      {
         ZoneIndexType nStirrupsInZone = ZoneIndexType(floor((end - start)/spacing));
         spacing = (end-start)/nStirrupsInZone;
         for ( ZoneIndexType i = 0; i <= nStirrupsInZone; i++ )
         {
            Float64 x = start + i*spacing;

            pgsPointOfInterest poi(span,girder,x);

            Float64 Hg = pGirder->GetHeight(poi);

            Float64 bottom = bot_cover - Hg;

            Float64 top;
            if ( deckType == pgsTypes::sdtNone || !bDoStirrupsEngageDeck )
               top = -top_cover;
            else
               top = slab_offset;

            CComPtr<IPoint2d> p1, p2;
            p1.CoCreateInstance(CLSID_Point2d);
            p2.CoCreateInstance(CLSID_Point2d);

            p1->Move(x,bottom);
            p2->Move(x,top);

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
            s1->Connect(startPlug,&dwCookie);
            s2->Connect(endPlug,&dwCookie);

            pDL->AddDisplayObject(line);
         }
      }
   }
}

void CTogaGirderModelElevationView::BuildLine(iDisplayList* pDL, IPoint2d* fromPoint,IPoint2d* toPoint, COLORREF color)
{
   // put points at locations and make them sockets
   CComPtr<iPointDisplayObject> from_rep;
   ::CoCreateInstance(CLSID_PointDisplayObject,NULL,CLSCTX_ALL,IID_iPointDisplayObject,(void**)&from_rep);
   from_rep->SetPosition(fromPoint,FALSE,FALSE);
   from_rep->SetID(m_CurrID++);
   CComQIPtr<iConnectable,&IID_iConnectable> from_connectable(from_rep);
   CComPtr<iSocket> from_socket;
   from_connectable->AddSocket(0,fromPoint,&from_socket);
   from_rep->Visible(FALSE);
   pDL->AddDisplayObject(from_rep);

   CComPtr<iPointDisplayObject> to_rep;
   ::CoCreateInstance(CLSID_PointDisplayObject,NULL,CLSCTX_ALL,IID_iPointDisplayObject,(void**)&to_rep);
   to_rep->SetPosition(toPoint,FALSE,FALSE);
   to_rep->SetID(m_CurrID++);
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

   line->SetID(m_CurrID++);

   pDL->AddDisplayObject(line);
}

void CTogaGirderModelElevationView::BuildDebondTick(iDisplayList* pDL, IPoint2d* tickPoint,COLORREF color)
{
   // put points at locations and make them sockets
   CComPtr<iPointDisplayObject> doPnt;
   ::CoCreateInstance(CLSID_PointDisplayObject,NULL,CLSCTX_ALL,IID_iPointDisplayObject,(void**)&doPnt);
   doPnt->SetPosition(tickPoint,FALSE,FALSE);
   doPnt->SetID(m_CurrID++);

   CComPtr<iSimpleDrawPointStrategy> strategy;
   ::CoCreateInstance(CLSID_SimpleDrawPointStrategy,NULL,CLSCTX_ALL,IID_iSimpleDrawPointStrategy,(void**)&strategy);
   strategy->SetColor(color);
   strategy->SetPointType(ptCircle);

   doPnt->SetDrawingStrategy(strategy);

   pDL->AddDisplayObject(doPnt);
}

iDimensionLine* CTogaGirderModelElevationView::BuildDimensionLine(iDisplayList* pDL, IPoint2d* fromPoint,IPoint2d* toPoint,Float64 dimension)
{
   // put points at locations and make them sockets
   CComPtr<iPointDisplayObject> from_rep;
   ::CoCreateInstance(CLSID_PointDisplayObject,NULL,CLSCTX_ALL,IID_iPointDisplayObject,(void**)&from_rep);
   from_rep->SetPosition(fromPoint,FALSE,FALSE);
   from_rep->SetID(m_CurrID++);
   CComQIPtr<iConnectable,&IID_iConnectable> from_connectable(from_rep);
   CComPtr<iSocket> from_socket;
   from_connectable->AddSocket(0,fromPoint,&from_socket);
   from_rep->Visible(FALSE);
   pDL->AddDisplayObject(from_rep);

   CComPtr<iPointDisplayObject> to_rep;
   ::CoCreateInstance(CLSID_PointDisplayObject,NULL,CLSCTX_ALL,IID_iPointDisplayObject,(void**)&to_rep);
   to_rep->SetPosition(toPoint,FALSE,FALSE);
   to_rep->SetID(m_CurrID++);
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
   dimLine->SetID(m_CurrID++);

   pDL->AddDisplayObject(dimLine);

   return dimLine.Detach();
}

void CTogaGirderModelElevationView::OnDestroy() 
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

void CTogaGirderModelElevationView::OnDraw(CDC* pDC)
{
   if ( m_bUpdateError )
   {
      CString msg;
      msg.Format(_T("The following error occured while updating the views.\n\n%s."),m_ErrorMsg.c_str());
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
