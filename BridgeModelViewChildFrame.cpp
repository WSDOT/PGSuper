///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2013  Washington State Department of Transportation
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

// cfSplitChildFrame.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\PGSuperApp.h"
#include "PGSuperAppPlugin\resource.h"
#include "pgsuperDoc.h"
#include "BridgeModelViewChildFrame.h"
#include "BridgeSectionView.h"
#include "BridgeViewPrintJob.h"
#include "StationCutDlg.h"
#include "SelectItemDlg.h"
#include "MainFrm.h"
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <IFace\EditByUI.h>
#include <EAF\EAFDisplayUnits.h>
#include "htmlhelp\HelpTopics.hh"
#include <PgsExt\BridgeDescription2.h>
#include "EditBoundaryConditions.h"

#include "PGSuperAppPlugin\InsertSpanDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CBridgeModelViewChildFrame

IMPLEMENT_DYNCREATE(CBridgeModelViewChildFrame, CSplitChildFrame)

CBridgeModelViewChildFrame::CBridgeModelViewChildFrame()
{
   m_bCutLocationInitialized = false;
   m_CurrentCutLocation = 0;
   m_bSelecting = false;
}

CBridgeModelViewChildFrame::~CBridgeModelViewChildFrame()
{
}

BOOL CBridgeModelViewChildFrame::PreCreateWindow(CREATESTRUCT& cs)
{
   // force this window to be maximized (not sure why WS_VISIBLE is required)
   cs.style |= WS_MAXIMIZE | WS_VISIBLE;

#if defined _EAF_USING_MFC_FEATURE_PACK
   // If MFC Feature pack is used, we are using tabbed MDI windows so we don't want
   // the system menu or the minimize and maximize boxes
   cs.style &= ~(WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);
#endif

   return __super::PreCreateWindow(cs);
}

BOOL CBridgeModelViewChildFrame::Create(LPCTSTR lpszClassName,
				LPCTSTR lpszWindowName,
				DWORD dwStyle,
				const RECT& rect,
				CMDIFrameWnd* pParentWnd,
				CCreateContext* pContext)
{
   BOOL bResult = CSplitChildFrame::Create(lpszClassName,lpszWindowName,dwStyle,rect,pParentWnd,pContext);
   if ( bResult )
   {
      AFX_MANAGE_STATE(AfxGetStaticModuleState());
      HICON hIcon = AfxGetApp()->LoadIcon(IDR_BRIDGEMODELEDITOR);
      SetIcon(hIcon,TRUE);
   }

   return bResult;
}

BEGIN_MESSAGE_MAP(CBridgeModelViewChildFrame, CSplitChildFrame)
	//{{AFX_MSG_MAP(CBridgeModelViewChildFrame)
	ON_COMMAND(ID_FILE_PRINT, OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, OnFilePrintDirect)
	ON_WM_CREATE()
	ON_COMMAND(ID_EDIT_SPAN,   OnEditSpan)
	ON_COMMAND(ID_EDIT_PIER,   OnEditPier)
	ON_COMMAND(ID_VIEW_GIRDER, OnViewGirder)
	ON_COMMAND(ID_DELETE_PIER, OnDeletePier)
	ON_UPDATE_COMMAND_UI(ID_DELETE_PIER, OnUpdateDeletePier)
	ON_COMMAND(ID_DELETE_SPAN, OnDeleteSpan)
	ON_UPDATE_COMMAND_UI(ID_DELETE_SPAN, OnUpdateDeleteSpan)
	ON_COMMAND(ID_INSERT_SPAN, OnInsertSpan)
	ON_COMMAND(ID_INSERT_PIER, OnInsertPier)
	//}}AFX_MSG_MAP
   ON_COMMAND_RANGE(IDM_HINGE,IDM_INTEGRAL_SEGMENT_AT_PIER,OnBoundaryCondition)
   ON_UPDATE_COMMAND_UI_RANGE(IDM_HINGE,IDM_INTEGRAL_SEGMENT_AT_PIER,OnUpdateBoundaryCondition)
	ON_MESSAGE(WM_HELP, OnCommandHelp)
   ON_NOTIFY(UDN_DELTAPOS, IDC_START_SPAN_SPIN, &CBridgeModelViewChildFrame::OnStartSpanChanged)
   ON_NOTIFY(UDN_DELTAPOS, IDC_END_SPAN_SPIN, &CBridgeModelViewChildFrame::OnEndSpanChanged)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CBridgeModelViewChildFrame message handlers
CRuntimeClass* CBridgeModelViewChildFrame::GetLowerPaneClass() const
{
   return RUNTIME_CLASS(CBridgeSectionView);
}

Float64 CBridgeModelViewChildFrame::GetTopFrameFraction() const
{
   return 0.5;
}

void CBridgeModelViewChildFrame::OnFilePrint() 
{
   DoFilePrint(false);
}

void CBridgeModelViewChildFrame::OnFilePrintDirect() 
{
   DoFilePrint(true);
}

void CBridgeModelViewChildFrame::DoFilePrint(bool direct)
{
   CBridgePlanView*    ppv = GetBridgePlanView();
   CBridgeSectionView* psv = GetBridgeSectionView();

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   
   // create a print job and do it
   CBridgeViewPrintJob pj(this,ppv, psv, pBroker);
   pj.OnFilePrint(direct);
}



CBridgePlanView* CBridgeModelViewChildFrame::GetBridgePlanView() 
{
   AFX_MANAGE_STATE(AfxGetAppModuleState()); // GetPane calls AssertValid, Must be in the application module state
   CWnd* pwnd = m_SplitterWnd.GetPane(0, 0);
   CBridgePlanView* pvw = dynamic_cast<CBridgePlanView*>(pwnd);
   ASSERT(pvw);
   return pvw;
}

CBridgeSectionView* CBridgeModelViewChildFrame::GetBridgeSectionView() 
{
   AFX_MANAGE_STATE(AfxGetAppModuleState()); // GetPane calls AssertValid, Must be in the application module state
   CWnd* pwnd = m_SplitterWnd.GetPane(1, 0);
   CBridgeSectionView* pvw = dynamic_cast<CBridgeSectionView*>(pwnd);
   ASSERT(pvw);
   return pvw;
}

void CBridgeModelViewChildFrame::InitSpanRange()
{
   // Can't get to the broker, and thus the bridge information in OnCreate, so we need a method
   // that can be called later to initalize the span range for viewing
   CSpinButtonCtrl* pStartSpinner = (CSpinButtonCtrl*)m_SettingsBar.GetDlgItem(IDC_START_SPAN_SPIN);
   CSpinButtonCtrl* pEndSpinner   = (CSpinButtonCtrl*)m_SettingsBar.GetDlgItem(IDC_END_SPAN_SPIN);

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IBridge,pBridge);
   SpanIndexType nSpans = pBridge->GetSpanCount();

   CBridgePlanView* pPlanView = GetBridgePlanView();

   SpanIndexType startSpanIdx, endSpanIdx;
   pPlanView->GetSpanRange(&startSpanIdx,&endSpanIdx);

   startSpanIdx = (startSpanIdx == ALL_SPANS ? 0        : startSpanIdx);
   endSpanIdx   = (endSpanIdx   == ALL_SPANS ? nSpans-1 : endSpanIdx  );

   pStartSpinner->SetRange32(1,(int)nSpans);
   pEndSpinner->SetRange32((int)startSpanIdx+1,(int)nSpans);

   pStartSpinner->SetPos32((int)startSpanIdx+1);
   pEndSpinner->SetPos32((int)endSpanIdx+1);

   CString str;
   str.Format(_T("of %d Spans"),nSpans);
   m_SettingsBar.GetDlgItem(IDC_SPAN_COUNT)->SetWindowText(str);
}

#if defined _DEBUG
void CBridgeModelViewChildFrame::AssertValid() const
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   CSplitChildFrame::AssertValid();
}

void CBridgeModelViewChildFrame::Dump(CDumpContext& dc) const
{
   CSplitChildFrame::Dump(dc);
}
#endif 

int CBridgeModelViewChildFrame::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CSplitChildFrame::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	this->SetWindowText(_T("Bridge Model View"));

   {
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
#if defined _EAF_USING_MFC_FEATURE_PACK
	if ( !m_SettingsBar.Create( _T("Title"), this, FALSE, IDD_BRIDGEVIEW_SETTINGS, CBRS_TOP, IDD_BRIDGEVIEW_SETTINGS) )
#else
	if ( !m_SettingsBar.Create( this, IDD_BRIDGEVIEW_SETTINGS, CBRS_TOP, IDD_BRIDGEVIEW_SETTINGS) )
#endif
	{
		TRACE0("Failed to create control bar\n");
		return -1;      // fail to create
	}
   }

#if defined _EAF_USING_MFC_FEATURE_PACK
   EnableDocking(CBRS_ALIGN_TOP);
   m_SettingsBar.EnableDocking(CBRS_ALIGN_TOP);
   m_SettingsBar.DockToFrameWindow(CBRS_ALIGN_TOP);
#endif

   return 0;
}

void CBridgeModelViewChildFrame::CutAt(Float64 cut)
{
   UpdateCutLocation(cut);
}

LPCTSTR CBridgeModelViewChildFrame::GetDeckTypeName(pgsTypes::SupportedDeckType deckType) const
{
   switch ( deckType )
   {
   case pgsTypes::sdtCompositeCIP:
      return _T("Composite cast-in-place deck");

   case pgsTypes::sdtCompositeSIP: 
      return _T("Composite stay-in-place deck panels");

   case pgsTypes::sdtCompositeOverlay:
      return _T("Composite structural overlay");

   case pgsTypes::sdtNone:
      return _T("None");
   }

   return _T("");
}

void CBridgeModelViewChildFrame::ShowCutDlg()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IBridge,pBridge);

   PierIndexType nPiers = pBridge->GetPierCount();
   Float64 start = pBridge->GetPierStation(0);
   Float64 end   = pBridge->GetPierStation(nPiers-1);

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   bool bUnitsSI = IS_SI_UNITS(pDisplayUnits);

   CStationCutDlg dlg(m_CurrentCutLocation,start,end,bUnitsSI);
   if ( dlg.DoModal() == IDOK )
   {
      UpdateCutLocation(dlg.GetValue());
   }
}

Float64 CBridgeModelViewChildFrame::GetMinCutLocation()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IBridge,pBridge);

   return pBridge->GetPierStation(0);
}

Float64 CBridgeModelViewChildFrame::GetMaxCutLocation()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IBridge,pBridge);

   PierIndexType nPiers = pBridge->GetPierCount();
   return pBridge->GetPierStation(nPiers-1);
}

void CBridgeModelViewChildFrame::UpdateCutLocation(Float64 cut)
{
   m_CurrentCutLocation = cut;

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IBridge,pBridge);

   SpanIndexType startSpanIdx, endSpanIdx;
   GetBridgePlanView()->GetSpanRange(&startSpanIdx,&endSpanIdx);

   PierIndexType startPierIdx = (PierIndexType)startSpanIdx;
   PierIndexType endPierIdx   = (PierIndexType)(endSpanIdx + 1);

   Float64 start = pBridge->GetPierStation(startPierIdx);
   Float64 end   = pBridge->GetPierStation(endPierIdx);

   m_CurrentCutLocation = ForceIntoRange(start,m_CurrentCutLocation,end);

//   UpdateBar();
   GetBridgePlanView()->OnUpdate(NULL, HINT_BRIDGEVIEWSECTIONCUTCHANGED, NULL);
   GetBridgeSectionView()->OnUpdate(NULL, HINT_BRIDGEVIEWSECTIONCUTCHANGED, NULL);
}
   
Float64 CBridgeModelViewChildFrame::GetCurrentCutLocation()
{
   if ( !m_bCutLocationInitialized )
   {
      CComPtr<IBroker> pBroker;
      EAFGetBroker(&pBroker);

      SpanIndexType startSpanIdx, endSpanIdx;
      CBridgePlanView* pPlanView = GetBridgePlanView();
      pPlanView->GetSpanRange(&startSpanIdx,&endSpanIdx);
      PierIndexType startPierIdx = (PierIndexType)startSpanIdx;
      PierIndexType endPierIdx   = (PierIndexType)(endSpanIdx+1);

      GET_IFACE2(pBroker,IBridge,pBridge);
      Float64 start = pBridge->GetPierStation(startPierIdx);
      Float64 end   = pBridge->GetPierStation(endPierIdx);
      m_CurrentCutLocation = 0.5*(end-start) + start;
      m_bCutLocationInitialized = true;
   }

   return m_CurrentCutLocation;
}

void CBridgeModelViewChildFrame::OnViewGirder() 
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   CSegmentKey segmentKey;

   if ( GetBridgePlanView()->GetSelectedGirder(&segmentKey) || GetBridgePlanView()->GetSelectedSegment(&segmentKey) )
   {
      CPGSuperDocBase* pDoc = (CPGSuperDocBase*)(GetBridgePlanView()->GetDocument());
      pDoc->OnViewGirderEditor();
   }
}

void CBridgeModelViewChildFrame::OnEditSpan() 
{
   SpanIndexType spanIdx;
   if ( GetBridgePlanView()->GetSelectedSpan(&spanIdx) )
   {
      CPGSuperDocBase* pDoc = (CPGSuperDocBase*)(GetBridgePlanView()->GetDocument());
      pDoc->EditSpanDescription(spanIdx,ESD_GENERAL);
   }
}

void CBridgeModelViewChildFrame::OnEditPier() 
{
   PierIndexType pierIdx;
   if ( GetBridgePlanView()->GetSelectedPier(&pierIdx) )
   {
      CPGSuperDocBase* pDoc = (CPGSuperDocBase*)(GetBridgePlanView()->GetDocument());
      pDoc->EditPierDescription(pierIdx,EPD_GENERAL);
   }
}

void CBridgeModelViewChildFrame::SelectSpan(SpanIndexType spanIdx)
{
   if ( m_bSelecting )
      return;

   m_bSelecting = true;
   CPGSuperDocBase* pDoc = (CPGSuperDocBase*)(GetBridgePlanView()->GetDocument());
   pDoc->SelectSpan(spanIdx);
   m_bSelecting = false;
}

void CBridgeModelViewChildFrame::SelectPier(PierIndexType pierIdx)
{
   if ( m_bSelecting )
      return;

   m_bSelecting = true;
   CPGSuperDocBase* pDoc = (CPGSuperDocBase*)(GetBridgePlanView()->GetDocument());
   pDoc->SelectPier(pierIdx);
   m_bSelecting = false;
}

void CBridgeModelViewChildFrame::SelectGirder(const CGirderKey& girderKey)
{
   if ( m_bSelecting )
      return;

   m_bSelecting = true;
   CPGSuperDocBase* pDoc = (CPGSuperDocBase*)(GetBridgePlanView()->GetDocument());
   pDoc->SelectGirder(girderKey);
   m_bSelecting = false;
}

void CBridgeModelViewChildFrame::SelectSegment(const CSegmentKey& segmentKey)
{
   if ( m_bSelecting )
      return;
   
   ATLASSERT(segmentKey.segmentIndex != INVALID_INDEX);

   m_bSelecting = true;
   CPGSuperDocBase* pDoc = (CPGSuperDocBase*)(GetBridgePlanView()->GetDocument());
   pDoc->SelectSegment(segmentKey);
   m_bSelecting = false;
}

void CBridgeModelViewChildFrame::SelectClosurePour(const CSegmentKey& closureKey)
{
   if ( m_bSelecting )
      return;
   
   ATLASSERT(closureKey.segmentIndex != INVALID_INDEX);

   m_bSelecting = true;
   CPGSuperDocBase* pDoc = (CPGSuperDocBase*)(GetBridgePlanView()->GetDocument());
   pDoc->SelectClosurePour(closureKey);
   m_bSelecting = false;
}

void CBridgeModelViewChildFrame::SelectTemporarySupport(SupportIDType tsID)
{
   if ( m_bSelecting )
      return;

   m_bSelecting = true;
   CPGSuperDocBase* pDoc = (CPGSuperDocBase*)(GetBridgePlanView()->GetDocument());
   pDoc->SelectTemporarySupport(tsID);
   m_bSelecting = false;
}

void CBridgeModelViewChildFrame::SelectDeck()
{
   if ( m_bSelecting )
      return;

   m_bSelecting = true;
   CPGSuperDocBase* pDoc = (CPGSuperDocBase*)(GetBridgePlanView()->GetDocument());
   pDoc->SelectDeck();
   m_bSelecting = false;
}

void CBridgeModelViewChildFrame::SelectAlignment()
{
   if ( m_bSelecting )
      return;

   m_bSelecting = true;
   CPGSuperDocBase* pDoc = (CPGSuperDocBase*)(GetBridgePlanView()->GetDocument());
   pDoc->SelectAlignment();
   m_bSelecting = false;
}

void CBridgeModelViewChildFrame::ClearSelection()
{
   CPGSuperDocBase* pDoc = (CPGSuperDocBase*)(GetBridgePlanView()->GetDocument());
   pDoc->ClearSelection();
}

LRESULT CBridgeModelViewChildFrame::OnCommandHelp(WPARAM, LPARAM lParam)
{
   ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_BRIDGE_VIEW );
   return TRUE;
}

void CBridgeModelViewChildFrame::OnDeletePier() 
{
   PierIndexType pierIdx;
   CBridgePlanView* pView = GetBridgePlanView();
	if ( pView->GetSelectedPier(&pierIdx) )
   {
      CPGSuperDocBase* pDoc = (CPGSuperDocBase*)pView->GetDocument();
      pDoc->DeletePier(pierIdx);
   }
   else
   {
      ATLASSERT(FALSE); // shouldn't get here
   }
}

void CBridgeModelViewChildFrame::OnUpdateDeletePier(CCmdUI* pCmdUI) 
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);

   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   PierIndexType pierIdx;
	if ( 1 < pBridgeDesc->GetSpanCount() && GetBridgePlanView()->GetSelectedPier(&pierIdx) )
   {
      pCmdUI->Enable(TRUE);
   }
   else
   {
      pCmdUI->Enable(FALSE);
   }
}

void CBridgeModelViewChildFrame::OnDeleteSpan() 
{
   SpanIndexType spanIdx;
   CBridgePlanView* pView = GetBridgePlanView();
	if ( pView->GetSelectedSpan(&spanIdx) )
   {
      CPGSuperDocBase* pDoc = (CPGSuperDocBase*)pView->GetDocument();
      pDoc->DeleteSpan(spanIdx);
   }
   else
   {
      ATLASSERT(FALSE); // shouldn't get here
   }
}

void CBridgeModelViewChildFrame::OnUpdateDeleteSpan(CCmdUI* pCmdUI) 
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);

   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   SpanIndexType spanIdx;
	if ( 1 < pBridgeDesc->GetSpanCount() && GetBridgePlanView()->GetSelectedSpan(&spanIdx) )
   {
      pCmdUI->Enable(TRUE);
   }
   else
   {
      pCmdUI->Enable(FALSE);
   }
}

void CBridgeModelViewChildFrame::OnInsertSpan() 
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   SpanIndexType spanIdx;
   CBridgePlanView* pView = GetBridgePlanView();
	if ( pView->GetSelectedSpan(&spanIdx) )
   {
      CComPtr<IBroker> broker;
      EAFGetBroker(&broker);
      GET_IFACE2(broker,IBridgeDescription,pIBridgeDesc);
      CInsertSpanDlg dlg(pIBridgeDesc->GetBridgeDescription());
      if ( dlg.DoModal() == IDOK )
      {
         Float64 span_length = dlg.m_SpanLength;
         PierIndexType refPierIdx = dlg.m_RefPierIdx;
         pgsTypes::PierFaceType face = dlg.m_PierFace;
         bool bCreateNewGroup = dlg.m_bCreateNewGroup;
         EventIndexType eventIdx = dlg.m_EventIdx;

         CPGSuperDocBase* pDoc = (CPGSuperDocBase*)pView->GetDocument();
         pDoc->InsertSpan(refPierIdx,face,span_length,bCreateNewGroup,eventIdx);
      }
   }
   else
   {
      ATLASSERT(FALSE); // shouldn't get here
   }
}

void CBridgeModelViewChildFrame::OnInsertPier() 
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   PierIndexType pierIdx;
   CBridgePlanView* pView = GetBridgePlanView();
	if ( pView->GetSelectedPier(&pierIdx) )
   {
      CComPtr<IBroker> broker;
      EAFGetBroker(&broker);
      GET_IFACE2(broker,IBridgeDescription,pIBridgeDesc);
      CInsertSpanDlg dlg(pIBridgeDesc->GetBridgeDescription());
      if ( dlg.DoModal() == IDOK )
      {
         Float64 span_length = dlg.m_SpanLength;
         PierIndexType refPierIdx = dlg.m_RefPierIdx;
         pgsTypes::PierFaceType pierFace = dlg.m_PierFace;
         bool bCreateNewGroup = dlg.m_bCreateNewGroup;
         EventIndexType eventIdx = dlg.m_EventIdx;

         CPGSuperDocBase* pDoc = (CPGSuperDocBase*)pView->GetDocument();
         pDoc->InsertSpan(refPierIdx,pierFace,span_length,bCreateNewGroup,eventIdx);

         if ( pierFace == pgsTypes::Back )
         {
            // move the pier selection ahead one pier so that it seems to
            // stay with the currently selected pier
            pView->SelectPier(pierIdx+1,true);
         }
      }
   }
   else
   {
      ATLASSERT(FALSE); // shouldn't get here
   }
}

void CBridgeModelViewChildFrame::OnBoundaryCondition(UINT nIDC)
{
#pragma Reminder("UPDATE: need to deal with InteriorPier... this is for BoundaryPier")
   PierIndexType pierIdx;
   CBridgePlanView* pView = GetBridgePlanView();
	if ( pView->GetSelectedPier(&pierIdx) )
   {
      pgsTypes::PierConnectionType newPierConnectionType;
      pgsTypes::PierSegmentConnectionType newSegmentConnectionType;
      switch( nIDC )
      {
         case IDM_HINGE:                          newPierConnectionType = pgsTypes::Hinge;                        break;
         case IDM_ROLLER:                         newPierConnectionType = pgsTypes::Roller;                       break;
         case IDM_CONTINUOUS_AFTERDECK:           newPierConnectionType = pgsTypes::ContinuousAfterDeck;          break;
         case IDM_CONTINUOUS_BEFOREDECK:          newPierConnectionType = pgsTypes::ContinuousBeforeDeck;         break;
         case IDM_INTEGRAL_AFTERDECK:             newPierConnectionType = pgsTypes::IntegralAfterDeck;            break;
         case IDM_INTEGRAL_BEFOREDECK:            newPierConnectionType = pgsTypes::IntegralBeforeDeck;           break;
         case IDM_INTEGRAL_AFTERDECK_HINGEBACK:   newPierConnectionType = pgsTypes::IntegralAfterDeckHingeBack;   break;
         case IDM_INTEGRAL_BEFOREDECK_HINGEBACK:  newPierConnectionType = pgsTypes::IntegralBeforeDeckHingeBack;  break;
         case IDM_INTEGRAL_AFTERDECK_HINGEAHEAD:  newPierConnectionType = pgsTypes::IntegralAfterDeckHingeAhead;  break;
         case IDM_INTEGRAL_BEFOREDECK_HINGEAHEAD: newPierConnectionType = pgsTypes::IntegralBeforeDeckHingeAhead; break;
         case IDM_CONTINUOUS_CLOSURE:             newSegmentConnectionType = pgsTypes::psctContinousClosurePour;  break;
         case IDM_INTEGRAL_CLOSURE:               newSegmentConnectionType = pgsTypes::psctIntegralClosurePour;   break;
         case IDM_CONTINUOUS_SEGMENT_AT_PIER:     newSegmentConnectionType = pgsTypes::psctContinuousSegment;     break;
         case IDM_INTEGRAL_SEGMENT_AT_PIER:       newSegmentConnectionType = pgsTypes::psctIntegralSegment;       break;

         default: ATLASSERT(0); // is there a new connection type?
      }

      CComPtr<IBroker> pBroker;
      EAFGetBroker(&pBroker);

      GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
      txnEditBoundaryConditions* pTxn;

      const CPierData2* pPier = pIBridgeDesc->GetPier(pierIdx);
      if ( pPier->IsBoundaryPier() )
      {
         pgsTypes::PierConnectionType oldConnectionType = pPier->GetPierConnectionType();
         pTxn = new txnEditBoundaryConditions(pierIdx,oldConnectionType,newPierConnectionType);
      }
      else
      {
         pgsTypes::PierSegmentConnectionType oldConnectionType = pPier->GetSegmentConnectionType();
         pTxn = new txnEditBoundaryConditions(pierIdx,oldConnectionType,newSegmentConnectionType);
      }
      txnTxnManager::GetInstance()->Execute(pTxn);
   }
}

void CBridgeModelViewChildFrame::OnUpdateBoundaryCondition(CCmdUI* pCmdUI)
{
   PierIndexType pierIdx;
   CBridgePlanView* pView = GetBridgePlanView();
	if ( pView->GetSelectedPier(&pierIdx) )
   {
      CComPtr<IBroker> pBroker;
      EAFGetBroker(&pBroker);
#pragma Reminder("UPDATE: need to deal with InteriorPier, this is for BoundaryPier")
      GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
      const CPierData2* pPier = pIBridgeDesc->GetPier(pierIdx);
      pgsTypes::PierConnectionType pierConnectionType = pPier->GetPierConnectionType();
      pgsTypes::PierSegmentConnectionType segmentConnectionType = pPier->GetSegmentConnectionType();
      switch( pCmdUI->m_nID )
      {
         case IDM_HINGE:                          pCmdUI->SetCheck(pierConnectionType == pgsTypes::Hinge);                        break;
         case IDM_ROLLER:                         pCmdUI->SetCheck(pierConnectionType == pgsTypes::Roller);                       break;
         case IDM_CONTINUOUS_AFTERDECK:           pCmdUI->SetCheck(pierConnectionType == pgsTypes::ContinuousAfterDeck);          break;
         case IDM_CONTINUOUS_BEFOREDECK:          pCmdUI->SetCheck(pierConnectionType == pgsTypes::ContinuousBeforeDeck);         break;
         case IDM_INTEGRAL_AFTERDECK:             pCmdUI->SetCheck(pierConnectionType == pgsTypes::IntegralAfterDeck);            break;
         case IDM_INTEGRAL_BEFOREDECK:            pCmdUI->SetCheck(pierConnectionType == pgsTypes::IntegralBeforeDeck);           break;
         case IDM_INTEGRAL_AFTERDECK_HINGEBACK:   pCmdUI->SetCheck(pierConnectionType == pgsTypes::IntegralAfterDeckHingeBack);   break;
         case IDM_INTEGRAL_BEFOREDECK_HINGEBACK:  pCmdUI->SetCheck(pierConnectionType == pgsTypes::IntegralBeforeDeckHingeBack);  break;
         case IDM_INTEGRAL_AFTERDECK_HINGEAHEAD:  pCmdUI->SetCheck(pierConnectionType == pgsTypes::IntegralAfterDeckHingeAhead);  break;
         case IDM_INTEGRAL_BEFOREDECK_HINGEAHEAD: pCmdUI->SetCheck(pierConnectionType == pgsTypes::IntegralBeforeDeckHingeAhead); break;
         case IDM_CONTINUOUS_CLOSURE:             pCmdUI->SetCheck(segmentConnectionType == pgsTypes::psctContinousClosurePour);  break;
         case IDM_INTEGRAL_CLOSURE:               pCmdUI->SetCheck(segmentConnectionType == pgsTypes::psctIntegralClosurePour);   break;
         case IDM_CONTINUOUS_SEGMENT_AT_PIER:     pCmdUI->SetCheck(segmentConnectionType == pgsTypes::psctContinuousSegment);     break;
         case IDM_INTEGRAL_SEGMENT_AT_PIER:       pCmdUI->SetCheck(segmentConnectionType == pgsTypes::psctIntegralSegment);       break;
         default: ATLASSERT(0); // is there a new connection type?
      }
   }
}
void CBridgeModelViewChildFrame::OnStartSpanChanged(NMHDR *pNMHDR, LRESULT *pResult)
{
   LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);
   // TODO: Add your control notification handler code here
   *pResult = 0;


   CSpinButtonCtrl* pStartSpinner = (CSpinButtonCtrl*)m_SettingsBar.GetDlgItem(IDC_START_SPAN_SPIN);
   int start, end;
   pStartSpinner->GetRange32(start,end);
   int newPos = pNMUpDown->iPos + pNMUpDown->iDelta;

   if ( newPos < start || end < newPos )
   {
      *pResult = 1;
      return;
   }

   SpanIndexType newStartSpanIdx = newPos - 1;

   CBridgePlanView* pPlanView = GetBridgePlanView();

   SpanIndexType startSpanIdx, endSpanIdx;
   pPlanView->GetSpanRange(&startSpanIdx,&endSpanIdx);

   CSpinButtonCtrl* pEndSpinner = (CSpinButtonCtrl*)m_SettingsBar.GetDlgItem(IDC_END_SPAN_SPIN);
   if ( endSpanIdx <= newStartSpanIdx )
   {
      // new start span is greater than end span
      // force position to be the same
      endSpanIdx = newStartSpanIdx;

      pEndSpinner->SetPos32((int)endSpanIdx+1);
   }

   pPlanView->SetSpanRange(newStartSpanIdx,endSpanIdx);
}

void CBridgeModelViewChildFrame::OnEndSpanChanged(NMHDR *pNMHDR, LRESULT *pResult)
{
   LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);
   // TODO: Add your control notification handler code here
   *pResult = 0;

   CSpinButtonCtrl* pEndSpinner = (CSpinButtonCtrl*)m_SettingsBar.GetDlgItem(IDC_END_SPAN_SPIN);
   int start, end;
   pEndSpinner->GetRange32(start,end);
   int newPos = pNMUpDown->iPos + pNMUpDown->iDelta;

   if ( newPos < start || end < newPos )
   {
      *pResult = 1;
      return;
   }

   SpanIndexType newEndSpanIdx = newPos - 1;

   CBridgePlanView* pPlanView = GetBridgePlanView();

   SpanIndexType startSpanIdx, endSpanIdx;
   pPlanView->GetSpanRange(&startSpanIdx,&endSpanIdx);
   
   CSpinButtonCtrl* pStartSpinner = (CSpinButtonCtrl*)m_SettingsBar.GetDlgItem(IDC_START_SPAN_SPIN);
   if ( newEndSpanIdx <= startSpanIdx )
   {
      // new end span is less than start span
      // force position to be the same
      startSpanIdx = newEndSpanIdx;

      pStartSpinner->SetPos32((int)startSpanIdx+1);
   }

   pPlanView->SetSpanRange(startSpanIdx,newEndSpanIdx);
}
