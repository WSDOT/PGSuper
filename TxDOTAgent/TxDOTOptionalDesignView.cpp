///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2015  Washington State Department of Transportation
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

#include "StdAfx.h"

#include "TxDOTOptionalDesignView.h"
#include "TxDOTOptionalDesignChildFrame.h"
#include "TxDOTOptionalDesignPropertySheet.h"
#include "TxDOTOptionalDesignBridgeInputPage.h"
#include "TxDOTOptionalDesignGirderInputPage.h"
#include "TxDOTOptionalDesignGirderViewPage.h"
#include "TxDOTOptionalDesignReportPage.h"

#include "TxDOTOptionalDesignDoc.h"


// TRICKY:
// This was a nasty bug that caused the program to hang while tabbing through controls
// on property pages. It worked partially, but would hang if the tab key was pressed 
// on the last control in the z-order on the page. 
// The bug seems to be due to running a dialog on a dll. A description of the fix can be found at:
// http://www.sumbera.com/ustation/articles/MFCDlg/MFCDlg.pdf
//
// Hook function capturing dialog messages
static HWND PageHwnd = 0;
static HHOOK hHook  = 0;

LRESULT FAR PASCAL GetMsgProc(int nCode, WPARAM wParam,LPARAM lParam)
{
   LPMSG lpMsg = (LPMSG) lParam;
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   if( ( nCode >= 0 && PM_REMOVE == wParam ) &&
        (lpMsg->message >= WM_KEYFIRST &&
         lpMsg->message <= WM_KEYLAST) &&
         lpMsg->wParam == VK_TAB &&
         (IsDialogMessage(PageHwnd, lpMsg))) 
   {
      lpMsg->message = WM_NULL;
      lpMsg->lParam = 0;
      lpMsg->wParam = 0;
   }

   return CallNextHookEx(hHook, nCode, wParam, lParam);
}

IMPLEMENT_DYNCREATE(CTxDOTOptionalDesignView, CView)


BEGIN_MESSAGE_MAP(CTxDOTOptionalDesignView, CView)
	//{{AFX_MSG_MAP(CTxDOTOptionalDesignView)
	//}}AFX_MSG_MAP
   ON_WM_SIZE()
   ON_WM_ERASEBKGND()
END_MESSAGE_MAP()


CTxDOTOptionalDesignView::CTxDOTOptionalDesignView(void): CView()
{
	m_pPropSheet = NULL;
	m_pBridgeInputPage = NULL;
	m_pGirderInputPage = NULL;
	m_pGirderViewPage = NULL;
	m_pReportPage = NULL;
}

CTxDOTOptionalDesignView::~CTxDOTOptionalDesignView(void)
{
	delete m_pPropSheet;
	delete m_pBridgeInputPage;
	delete m_pGirderInputPage;
	delete m_pGirderViewPage;
	delete m_pReportPage;
}

void CTxDOTOptionalDesignView::OnInitialUpdate()
{
	CView::OnInitialUpdate();

   // manage state only within block below, otherwise sizing will crash
   {
      AFX_MANAGE_STATE(AfxGetStaticModuleState());

	   // Create property pages
	   m_pBridgeInputPage = new CTxDOTOptionalDesignBridgeInputPage;
	   m_pGirderInputPage = new CTxDOTOptionalDesignGirderInputPage;
	   m_pGirderViewPage  = new CTxDOTOptionalDesignGirderViewPage;
      m_pReportPage      = new CTxDOTOptionalDesignReportPage;

      // Link them to document's data
      CTxDOTOptionalDesignDoc* pDoc = (CTxDOTOptionalDesignDoc*)this->GetDocument();
      CTxDOTOptionalDesignData* pData = &(pDoc->m_ProjectData);

      ((CTxDOTOptionalDesignBridgeInputPage*)m_pBridgeInputPage)->m_pData = pData;
      ((CTxDOTOptionalDesignGirderInputPage*)m_pGirderInputPage)->m_pData = pData;
      ((CTxDOTOptionalDesignGirderViewPage*)m_pGirderViewPage)->m_pData   = pData;
      ((CTxDOTOptionalDesignReportPage*)m_pReportPage)->m_pData           = pData;

      // Output pages need access to a broker
      ((CTxDOTOptionalDesignBridgeInputPage*)m_pBridgeInputPage)->m_pBrokerRetriever = pDoc;
      ((CTxDOTOptionalDesignGirderInputPage*)m_pGirderInputPage)->m_pBrokerRetriever = pDoc;
      ((CTxDOTOptionalDesignGirderViewPage*)m_pGirderViewPage)->m_pBrokerRetriever   = pDoc;
      ((CTxDOTOptionalDesignReportPage*)m_pReportPage)->m_pBrokerRetriever           = pDoc;

      // Views need their document
      ((CTxDOTOptionalDesignGirderViewPage*)m_pGirderViewPage)->m_pDocument  = pDoc;

	   // Add property pages to sheet
	   m_pPropSheet = new CTxDOTOptionalDesignPropertySheet(IDS_SHEETNAME,this);
	   m_pPropSheet->AddPage(m_pBridgeInputPage);
	   m_pPropSheet->AddPage(m_pGirderInputPage);
	   m_pPropSheet->AddPage(m_pGirderViewPage);
	   m_pPropSheet->AddPage(m_pReportPage);

	   // Create a modeless property page
      DWORD sheetStyle = WS_CHILD | WS_VISIBLE;

	   if (!m_pPropSheet->Create(this,sheetStyle))
	   {
		   DestroyWindow();
	   }

      // Set Z order
	   m_pPropSheet->SetWindowPos(this, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);

      m_pPropSheet->SetParent(this);
   }

   // plug in our hook so dialog messages are redirected to the property sheet
   PageHwnd = m_pPropSheet->GetSafeHwnd();
   hHook = ::SetWindowsHookEx(WH_GETMESSAGE, GetMsgProc, NULL, GetCurrentThreadId());

   // Resize the application main frame to fits around the property sheet.
	CRect rcPropSheet;
	m_pPropSheet->GetWindowRect(rcPropSheet);
	m_pPropSheet->CalcWindowRect(rcPropSheet);

   // Minimum size so we fit
   int cxMin = rcPropSheet.Width();
   int cyMin = rcPropSheet.Height();

   // figure out how much height the control bars take (menus, toolbars, etc)
   CRect rcView;
   GetWindowRect(rcView);

   CRect rcMainFrame;
   CEAFMainFrame* pMainFrame = EAFGetMainFrame();
   pMainFrame->GetWindowRect(rcMainFrame);

   int cyControlBars = rcView.top - rcMainFrame.top; // (rcMainFrame.Height() - rcView.Height());
   cyMin += cyControlBars;

   CTxDOTOptionalDesignChildFrame* pChildFrame = (CTxDOTOptionalDesignChildFrame*)GetParentFrame();
   pChildFrame->SetFrameSize(cxMin,cyMin);
   SetWindowPos(pChildFrame,0,0,cxMin,cyMin,SWP_NOMOVE | SWP_NOZORDER);
   m_pPropSheet->SetWindowPos(this,0,0,cxMin,cyMin,SWP_NOZORDER);

   GetClientRect(rcView);

   m_szMin.cx = rcView.Width();
   m_szMin.cy = rcView.Height();

   // Don't shrink window, only grow
   cxMin = Max(cxMin, rcMainFrame.Width());
   cyMin = Max(cyMin, rcMainFrame.Height());

   pMainFrame->SetWindowPos(NULL,0,0,cxMin,cyMin,SWP_NOMOVE | SWP_NOZORDER);
}

BOOL CTxDOTOptionalDesignView::DestroyWindow()
{
   // unhook hook
   UnhookWindowsHookEx(hHook);

   return CView::DestroyWindow();
}


void CTxDOTOptionalDesignView::DoDataExchange(CDataExchange* pDX)
{
	CView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CTxDOTOptionalDesignView)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}

void CTxDOTOptionalDesignView::OnSize(UINT nType, int cx, int cy)
{
   CView::OnSize(nType, cx, cy);

   if (m_pPropSheet!=NULL && ::IsWindow(m_pPropSheet->m_hWnd))
   {
      if ( GetParentFrame()->IsZoomed() )
      {
         // window is maximized so the only way it can resize is if
         // the main frame is resizing... limit the size of the
         // property sheet to its minimum size
         cx = Max(cx,(int)m_szMin.cx);
         cy = Max(cy,(int)m_szMin.cy);
      }

	   m_pPropSheet->SetWindowPos(this, 0, 0, cx, cy, SWP_NOZORDER);
   }
}

BOOL CTxDOTOptionalDesignView::UpdateCurrentPageData()
{
   // If active page is an input page, update it's data
   if (m_pPropSheet!=NULL && ::IsWindow(m_pPropSheet->m_hWnd))
   {
      AFX_MANAGE_STATE(AfxGetStaticModuleState());

      CPropertyPage* pPage = m_pPropSheet->GetActivePage();

      CTxDOTOptionalDesignBridgeInputPage* pBridgeInput = dynamic_cast<CTxDOTOptionalDesignBridgeInputPage*>(pPage);
      if (pBridgeInput!=NULL)
      {
         return pBridgeInput->UpdateData(TRUE);
      }

      CTxDOTOptionalDesignGirderInputPage* pGirderInput = dynamic_cast<CTxDOTOptionalDesignGirderInputPage*>(pPage);
      if (pGirderInput!=NULL)
      {
         return pGirderInput->UpdateData(TRUE);
      }
   }

   return TRUE; // active page is not an input page
}

BOOL CTxDOTOptionalDesignView::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo)
{

   // Redirect commands to active property page
   // If this is not here, the context menu in the report view will not work
   if (nCode==CN_COMMAND  && !(nID>=ID_FILE_MRU_FIRST && nID<=ID_FILE_MRU_LAST) // mru commands must go to main app
       && (nID != ID_APP_EXIT)
       && m_pPropSheet!=NULL && ::IsWindow(m_pPropSheet->m_hWnd))
   {
      AFX_MANAGE_STATE(AfxGetStaticModuleState());

      CPropertyPage* pPage;
      pPage = m_pPropSheet->GetActivePage();

      if(pPage->OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
        return TRUE;
   }

   return CView::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}


BOOL CTxDOTOptionalDesignView::PreTranslateMessage(MSG* pMsg)
{

   if (pMsg->message < WM_KEYLAST)
      return FALSE;


   {
      AFX_MANAGE_STATE(AfxGetStaticModuleState());

      if ( ::IsDialogMessage(m_hWnd,pMsg) )
         return TRUE;
   }


   return CView::PreTranslateMessage(pMsg);
}

void CTxDOTOptionalDesignView::OnDraw(CDC* pDC)
{
   // do nothing
}

#if defined _DEBUG
void CTxDOTOptionalDesignView::AssertValid() const
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());

   CView::AssertValid();
}
#endif