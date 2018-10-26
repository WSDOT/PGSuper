///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright (C) 1999  Washington State Department of Transportation
//                     Bridge and Structures Office
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

// ReportView.cpp : implementation file
//

#include "stdafx.h"
#include "PGSuper.h"

#include "PGSuperDoc.h"
#include <ATLBase.h>
#include "SelectReportDlg.h"

#include "ReportView.h"
#include "Hints.h"

#include <PgsExt\AutoProgress.h>

#include <Reporting\SpanGirderReportSpecification.h>

#include <MfcTools\XUnwind.h>
#include <MfcTools\Text.h>

#include "CustSiteVars.h"

#include "LicensePlateChildFrm.h"
#include "htmlhelp\HelpTopics.hh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CReportView


IMPLEMENT_DYNCREATE(CReportView, CAutoCalcView)
bool CReportView::ms_bIsUpdatingReport = false;

CReportView::CReportView()
{
   m_bNoBrowser = false;
   m_bUpdateError = false;
   m_bIsNewReport = true;

   // Timer settings
   m_Timer      = -1;
   m_TimerEvent = 100;
   m_Timeout    = 500; // 0.5 seconds
}

CReportView::~CReportView()
{
}

BEGIN_MESSAGE_MAP(CReportView, CAutoCalcView)
	//{{AFX_MSG_MAP(CReportView)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_COMMAND(ID_FILE_PRINT, OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, OnToolbarPrint)
	ON_UPDATE_COMMAND_UI(ID_FILE_PRINT, OnUpdateFilePrint)
	ON_WM_TIMER()
	ON_UPDATE_COMMAND_UI(ID_FILE_PRINT_DIRECT, OnUpdateFilePrint)
	//}}AFX_MSG_MAP
    ON_COMMAND_RANGE(CCS_CMENU_BASE, CCS_CMENU_MAX, OnCmenuSelected)
    ON_BN_CLICKED(IDC_EDIT,OnEdit)
    ON_WM_ERASEBKGND()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CReportView drawing

void CReportView::OnDraw(CDC* pDC)
{
   if ( !m_pReportBrowser )
   {
      // don't draw license plate frame if nothing to view
      EnableLpFrame( false );

      CString msg;
      if ( m_bNoBrowser )
         msg.LoadString(IDS_E_NOBROWSER);
      else if ( m_bUpdateError )
         AfxFormatString1(msg,IDS_E_UPDATE,m_ErrorMsg.c_str());
      else
      {
         if ( m_Timer != -1 )
            msg.LoadString( IDS_WORKING );
         else
            msg.LoadString(IDS_RESULTS_NOT_AVAILABLE);
      }

      COLORREF oldColor = pDC->SetBkColor( GetSysColor(COLOR_BTNFACE) );
      MultiLineTextOut(pDC,0,0,msg);
      pDC->SetBkColor( oldColor );
   }
   else
   {
   }
}

/////////////////////////////////////////////////////////////////////////////
// CReportView diagnostics

#ifdef _DEBUG
void CReportView::AssertValid() const
{
	CAutoCalcView::AssertValid();
}

void CReportView::Dump(CDumpContext& dc) const
{
	CAutoCalcView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CReportView message handlers

int CReportView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CAutoCalcView::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	CPGSuperDoc* pDoc = (CPGSuperDoc*)GetDocument();

   pgsStatusCenter& status_center = pDoc->GetStatusCenter();
   if ( status_center.GetSeverity() == STATUS_ERROR )
      return STATUS_ERROR;

   CRect rect(0,0,50,21);
   m_btnEdit.Create("Edit",WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON | BS_TEXT, rect, this, IDC_EDIT);
   m_btnFont.Attach( GetStockObject(DEFAULT_GUI_FONT) );
   m_btnEdit.SetFont(&m_btnFont);
   return 0;
}

bool CReportView::CreateReport(CollectionIndexType rptIdx,bool bPromptForSpec)
{
   CreateReportSpecification(rptIdx,bPromptForSpec);
   if ( !m_pReportSpec )
      return false;

	CPGSuperDoc* pDoc = (CPGSuperDoc*)GetDocument();
   if ( pDoc->IsAutoCalcEnabled() && SUCCEEDED(UpdateReportBrowser() ) )
      return true;

   return false;
}

void CReportView::CreateReportSpecification(CollectionIndexType rptIdx,bool bPromptForSpec)
{
   CComPtr<IBroker> pBroker;
   AfxGetBroker(&pBroker);

   GET_IFACE2(pBroker,IReportManager,pRptMgr);

   std::vector<std::string> rptNames = pRptMgr->GetReportNames();
   std::string rptName;
   if ( rptIdx == INVALID_INDEX )
   {
      // creating report with invalid index, this means we have to prompt for the
      // report
      if ( rptNames.size() == 1 )
      {
         rptName = rptNames[0];
      }
      else
      {
         CSelectReportDlg dlg(rptNames);
         if ( dlg.DoModal() == IDOK )
         {
            rptName = dlg.m_ReportName;
         }
         else
         {
            // the user cancelled the report creation because he failed to
            // select a report (ie. the Cancel buttow was pressed)

            // The view creation must fail and this is intentional
            // Turn off the error message so the user doesn't see it
            CMainFrame* pFrame = (CMainFrame*)AfxGetMainWnd();
            pFrame->DisableFailCreateMessage();
            m_pReportSpec = boost::shared_ptr<CReportSpecification>();
            return;
         }
      }
   }
   else
   {
      ATLASSERT( 0 <= rptIdx && rptIdx < (CollectionIndexType)rptNames.size() );
      rptName = rptNames[rptIdx];
   }

   boost::shared_ptr<CReportBuilder> pRptBuilder = pRptMgr->GetReportBuilder(rptName);
   CReportDescription rptDesc = pRptBuilder->GetReportDescription();

   boost::shared_ptr<CReportSpecificationBuilder> pRptSpecBuilder = pRptBuilder->GetReportSpecificationBuilder();
   if ( bPromptForSpec )
   {
      m_pReportSpec = pRptSpecBuilder->CreateReportSpec(rptDesc,m_pReportSpec);
   }
   else
   {
      m_pReportSpec = pRptSpecBuilder->CreateDefaultReportSpec(rptDesc);
   }

   if ( m_pReportSpec == NULL )
   {
      // the user probably cancelled the report creation because he pressed the Cancel button
      // in the report specification dialog

      // The view creation must fail and this is intentional
      // Turn off the error message so the user doesn't see it
      CMainFrame* pFrame = (CMainFrame*)AfxGetMainWnd();
      pFrame->DisableFailCreateMessage();
      pFrame->CreateCanceled();
   }
}

HRESULT CReportView::UpdateReportBrowser()
{
   Invalidate();

   if ( m_pReportSpec == NULL )
      return S_OK;

   HRESULT hr = m_pReportSpec->Validate();
   if ( FAILED(hr) )
      return hr;

   CComPtr<IBroker> pBroker;
   AfxGetBroker(&pBroker);

   // get the IReportManager interface
   GET_IFACE2(pBroker,IReportManager,pRptMgr);
   ASSERT( pRptMgr != NULL ); // The ReportManagerAgent isn't in your project... and it must be!

   GET_IFACE2(pBroker,IProgress,pProgress);
   pgsAutoProgress ap(pProgress);
   pProgress->UpdateMessage("Building Report");

   try
   {
      m_bUpdateError = false;
      m_bNoBrowser = false;

      // All the chapter builders get called from here... this is where all the
      // work related to generating the content of the report happens

      // if we already have a report browser, just refresh the report
      if ( m_pReportBrowser )
      {
         boost::shared_ptr<CReportBuilder> pBuilder = pRptMgr->GetReportBuilder( m_pReportSpec->GetReportName() );
         boost::shared_ptr<rptReport> pReport = pBuilder->CreateReport( m_pReportSpec );
         m_pReportBrowser->UpdateReport( pReport, true );
      }
      else
      {
         // create the report and browser
         m_pReportBrowser = pRptMgr->CreateReportBrowser(GetSafeHwnd(),m_pReportSpec);
      }
   }
   catch(...)
   {
      if ( m_pReportBrowser )
      {
         // delete the report browser because what ever it is displaying is totally invalid
         // also need to elimintate it so that we can draw the error message on the view window itself
         m_pReportBrowser = boost::shared_ptr<CReportBrowser>();
      }

      throw; // keep the exception moving
   }

   if ( m_pReportBrowser )
   {
      Invalidate();
      m_btnEdit.ShowWindow(SW_SHOW);

      // size the browser window to fill the view
      CRect rect;
      GetClientRect(&rect);
      OnSize(0,rect.Width(),rect.Height());
   }
   else
   {
      m_btnEdit.ShowWindow(SW_HIDE);
      m_bNoBrowser = true;
   }


   return S_OK;
}

void CReportView::OnEdit()
{
   CComPtr<IBroker> pBroker;
   AfxGetBroker(&pBroker);
   GET_IFACE2(pBroker,IProgress,pProgress);
   pgsAutoProgress ap(pProgress);

   m_pReportBrowser->Edit();
   m_pReportSpec = m_pReportBrowser->GetReportSpecification();
   UpdateViewTitle();
}

void CReportView::OnSize(UINT nType, int cx, int cy) 
{
	CAutoCalcView::OnSize(nType, cx, cy);

   if ( m_pReportBrowser )
   {
   //   // make browser window a little shorter so
   //   // we have room for the Edit button
      CRect btnRect;
      m_btnEdit.GetClientRect(&btnRect);
   //   cy -= btnRect.Height();

   //   m_pReportBrowser->Move( CPoint(btnRect.left,btnRect.bottom) );
      m_pReportBrowser->Size( CSize(cx,cy) );
   }

   m_btnEdit.SetWindowPos(&CWnd::wndTop,0,0,0,0,SWP_NOMOVE | SWP_NOSIZE);
}

void CReportView::OnFilePrint() 
{
   ASSERT(m_pReportBrowser);
   m_pReportBrowser->Print(TRUE);
}

void CReportView::OnToolbarPrint() 
{
   ASSERT(m_pReportBrowser);
   m_pReportBrowser->Print(FALSE);
}

void CReportView::OnUpdateFilePrint(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable( m_pReportBrowser == NULL ? FALSE : TRUE );
}

void CReportView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
   if ( m_bIsNewReport )
      return; // this the OnUpdate that comes from OnInitialUpdate() ... nothing to do here

   if ( lHint == HINT_GIRDERLABELFORMATCHANGED )
   {
      UpdateViewTitle();
   }

   if ( 0 < lHint && lHint <= MAX_DISPLAY_HINT )
      return; // some display feature changed... not data... nothing to do here

   // The current implementation of this method is to re-generate a report
   // every time anything changes.  We might want to consider making the
   // report objects themselves be connection points for the things
   // they depend on.  That way,  a report only re-generates if it needs to.
   if ( lHint == HINT_UPDATEERROR )
   {
      CString* pmsg = (CString*)pHint;
      m_ErrorMsg = *pmsg;
      m_bUpdateError = true;

      Invalidate();
      return;
   }

   CAutoCalcView::OnUpdate( pSender, lHint, pHint );

   // Something has changed to invalidate the report
   m_bInvalidReport = true;

   // Only update if autocalc is on 
   // Otherwise just keep the view as it is.
	CPGSuperDoc* pDoc = (CPGSuperDoc*)GetDocument();
   if ( pDoc->IsAutoCalcEnabled() )
   {
//      // AutoCalc mode is on.  We don't want to start updating immedately.
//      // If other update events are being fired we will have to re-generated
//      // the report for each one.  Lets setup a timer and when the timer
//      // expires then update.
//
//      // Is the timer already running?
//      if ( 0 <= m_Timer )
//      {
//         // Timer is running... Kill it and restart
//         KillTimer( m_Timer );
//      }
//
//      m_Timer = SetTimer( m_TimerEvent, m_Timeout, NULL );
      UpdateNow();
   }
}

void CReportView::UpdateNow()
{
   if ( CReportView::ms_bIsUpdatingReport )
      return;

   CReportView::ms_bIsUpdatingReport = true;

   if ( m_bInvalidReport )
   {
      try
      {
         CLicensePlateChildFrame* pParent = (CLicensePlateChildFrame*)GetParent();

         HRESULT hr = UpdateReportBrowser();
         if ( FAILED(hr) )
         {
            if ( m_bNoBrowser )
            {
                AfxMessageBox( IDS_E_NOBROWSER );
            }
            else if ( hr == RPT_E_INVALIDGIRDER || hr == RPT_E_INVALIDSPAN )
            {
               pParent->SetLicensePlateMode(CLicensePlateChildFrame::On);
               pParent->SetLicensePlateText(m_ErrorMsg.c_str());
            }
         }
         else
         {
   	      CPGSuperDoc* pDoc = (CPGSuperDoc*)GetDocument();
            if ( pDoc->IsAutoCalcEnabled() && pParent->GetLicensePlateMode() == CLicensePlateChildFrame::On )
            {
               // if auto calc is enable and there was no error updating the view,
               // make sure the license plate frame is off
               pParent->SetLicensePlateMode(CLicensePlateChildFrame::Off);
            }
         }

         m_bInvalidReport = false;
      }
      catch(...)
      {
         CReportView::ms_bIsUpdatingReport = false;
         throw;
      }
   }

   CReportView::ms_bIsUpdatingReport = false;
}

bool CReportView::DoResultsExist() const
{
   return (m_pReportBrowser != 0 ? true : false);
}

void CReportView::OnInitialUpdate() 
{
   m_bIsNewReport = true;

   try
   {
      CReportViewDocTemplate* pDocTemplate = theApp.GetReportViewTemplate();
      CollectionIndexType rptIdx = pDocTemplate->GetReportIndex();
      bool bPromptForSpec = pDocTemplate->PromptForReportSpecification();
      CreateReport(rptIdx,bPromptForSpec);
      CAutoCalcView::OnInitialUpdate();
   }
   catch(...)
   {
      m_bIsNewReport = false; // no longer creating a new report
      throw; // keep the exception moving
   }

   UpdateViewTitle();
   m_bIsNewReport = false;
}

void CReportView::UpdateViewTitle()
{
   if ( m_pReportSpec == NULL )
   {
      SetWindowText("Report View");
      return;
   }

   CString strTitle( m_pReportSpec->GetReportTitle().c_str() );
   SetWindowText(strTitle);
   CPGSuperDoc* pDoc = (CPGSuperDoc*)GetDocument();
   pDoc->UpdateFrameCounts();
}

void CReportView::OnTimer(UINT nIDEvent) 
{
	CAutoCalcView::OnTimer(nIDEvent);

   if ( m_TimerEvent == nIDEvent && !CReportView::ms_bIsUpdatingReport)
   {
      // The timer has gone off.  It is time to update the report

      // Kill the timer so it stops going off
      KillTimer( m_TimerEvent );
      m_Timer = -1;

      UpdateNow();
   }
}

void CReportView::OnCmenuSelected(UINT id)
{
  UINT cmd = id-CCS_CMENU_BASE ;

  switch(cmd)
  {
  case CCS_RB_EDIT:
     OnEdit();
     break;

  case CCS_RB_FIND:
     m_pReportBrowser->Find();
     break;

  case CCS_RB_SELECT_ALL:
     m_pReportBrowser->SelectAll();
     break;
  case CCS_RB_PRINT:
     m_pReportBrowser->Print(true);
     break;

  case CCS_RB_REFRESH:
     m_pReportBrowser->Refresh();
     break;

  case CCS_RB_VIEW_SOURCE:
     m_pReportBrowser->ViewSource();
     break;

  case CCS_RB_VIEW_BACK:
     m_pReportBrowser->Back();
     break;

  case CCS_RB_VIEW_FORWARD:
     m_pReportBrowser->Forward();
     break;

  default:
     // must be a toc anchor
     CHECK(cmd>=CCS_RB_TOC);
     m_pReportBrowser->NavigateAnchor(cmd-CCS_RB_TOC);
  }
}


BOOL CReportView::PreTranslateMessage(MSG* pMsg) 
{
   // had to go here to process help and Find messages before they get to IE
   if (pMsg->message == WM_KEYDOWN) 
   {
      if (pMsg->wParam == VK_F1)
      {
         ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_REPORT_VIEW );
         return TRUE;
      }
      else if (pMsg->wParam =='f' || pMsg->wParam =='F')
      {
         // ctrl - F
         if (::GetKeyState(VK_CONTROL))
         {
            m_pReportBrowser->Find();
            return TRUE;
         }
      }
   }

	return CAutoCalcView::PreTranslateMessage(pMsg);
}

BOOL CReportView::OnEraseBkgnd(CDC* pDC)
{
   CRect rect;
   GetClientRect(&rect);

   CBrush brush;
   brush.Attach( GetSysColorBrush(COLOR_BTNFACE) ); // dialog background color
   brush.UnrealizeObject();
   CBrush* pOldBrush = pDC->SelectObject(&brush);

   CPen pen(PS_SOLID,1, GetSysColor(COLOR_BTNFACE) );
   CPen* pOldPen = pDC->SelectObject(&pen);

   pDC->Rectangle(rect);
   
   pDC->SelectObject(pOldBrush);
   pDC->SelectObject(pOldPen);

   return TRUE;
}
