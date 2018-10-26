#include "StdAfx.h"

#include "TxDOTOptionalDesignView.h"
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

IMPLEMENT_DYNCREATE(CTxDOTOptionalDesignView, CFormView)


BEGIN_MESSAGE_MAP(CTxDOTOptionalDesignView, CFormView)
	//{{AFX_MSG_MAP(CTxDOTOptionalDesignView)
	//}}AFX_MSG_MAP
   ON_WM_SIZE()
   ON_WM_ERASEBKGND()
END_MESSAGE_MAP()


CTxDOTOptionalDesignView::CTxDOTOptionalDesignView(void): CFormView((LPCTSTR)NULL)
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
	CFormView::OnInitialUpdate();

   CWnd* pParentWnd = this->GetParent();

   // get size of parent - we want to maximize
	CRect rectSize;
	pParentWnd->GetWindowRect(rectSize);
	pParentWnd->CalcWindowRect(rectSize);

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

   // Resize mainframe to fit entire property sheet initially
	CRect rects;
	m_pPropSheet->GetWindowRect(rects);
	m_pPropSheet->CalcWindowRect(rects);

   GetParentFrame()->RecalcLayout();

   // Expand mainframe to fit entire dialogs
   CEAFMainFrame* pFrame = EAFGetMainFrame();

   CRect rframe;
   pFrame->GetWindowRect(rframe);

   CRect rthis;
   this->GetWindowRect(rthis); 

   // height of menus,toolbars, other stuff
   int rjunk = rthis.top - rframe.top; // (rframe.Height() - rthis.Height());

   pFrame->SetWindowPos(NULL,0,0,rects.Width(),rects.Height()+rjunk,SWP_NOMOVE | SWP_NOZORDER);

   // no scrolling
   this->SetScaleToFitSize(CSize(1,1));
   
   // plug in our hook so dialog messages are redirected to the property sheet
   PageHwnd = m_pPropSheet->GetSafeHwnd();
   hHook = ::SetWindowsHookEx(WH_GETMESSAGE, GetMsgProc, NULL, GetCurrentThreadId());
}

BOOL CTxDOTOptionalDesignView::DestroyWindow()
{
   // unhook hook
   UnhookWindowsHookEx(hHook);

   return CFormView::DestroyWindow();
}


void CTxDOTOptionalDesignView::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CTxDOTOptionalDesignView)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}

BOOL CTxDOTOptionalDesignView::Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName,
	DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext)
{
   ENSURE(pParentWnd != NULL);
   ASSERT_KINDOF(CFrameWnd, pParentWnd);

	CRect rectSize;
	pParentWnd->GetWindowRect(rectSize);
	pParentWnd->CalcWindowRect(rectSize);

   DWORD dwMyStyle = WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN;

   m_pCreateContext = pContext;
   if (!CWnd::Create(lpszClassName, lpszWindowName,  dwMyStyle,
	   rectSize, pParentWnd, nID, pContext))
   {
	   return FALSE;
   }
   m_pCreateContext = NULL;

   return TRUE;
}

void CTxDOTOptionalDesignView::OnSize(UINT nType, int cx, int cy)
{
   CFormView::OnSize(nType, cx, cy);

   if (m_pPropSheet!=NULL && ::IsWindow(m_pPropSheet->m_hWnd))
   {
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
       && m_pPropSheet!=NULL && ::IsWindow(m_pPropSheet->m_hWnd))
   {
      AFX_MANAGE_STATE(AfxGetStaticModuleState());

      CPropertyPage* pPage;
      pPage = m_pPropSheet->GetActivePage();

      if(pPage->OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
        return TRUE;
   }

   return CFormView::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
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


   return CFormView::PreTranslateMessage(pMsg);
}

void CTxDOTOptionalDesignView::AssertValid() const
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());

   CFormView::AssertValid();
}

