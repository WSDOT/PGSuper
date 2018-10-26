// TxDOTOptionalDesignReportPage.cpp : implementation file
//

#include "stdafx.h"
#include "HtmlHelp\TogaHelp.hh"
#include "TxDOTOptionalDesignReportPage.h"
#include "TxDOTOptionalDesignUtilities.h"

#include <IReportManager.h>
#include <EAF\EAFAutoProgress.h>
#include <EAF\EAFCustSiteVars.h>


// CTxDOTOptionalDesignReportPage dialog

IMPLEMENT_DYNAMIC(CTxDOTOptionalDesignReportPage, CPropertyPage)

CTxDOTOptionalDesignReportPage::CTxDOTOptionalDesignReportPage()
	: CPropertyPage(CTxDOTOptionalDesignReportPage::IDD),
   m_pData(NULL),
   m_pBrokerRetriever(NULL),
   m_ChangeStatus(0)
{

}

CTxDOTOptionalDesignReportPage::~CTxDOTOptionalDesignReportPage()
{
   if ( m_pBrowser )
   {
      m_pBrowser = boost::shared_ptr<CReportBrowser>();
   }
}

void CTxDOTOptionalDesignReportPage::DoDataExchange(CDataExchange* pDX)
{
   CPropertyPage::DoDataExchange(pDX);
   DDX_Control(pDX, IDC_BROWSER, m_BrowserPlaceholder);
   DDX_Control(pDX, IDC_REPORT_COMBO, m_ReportCombo);
   DDX_Control(pDX, IDC_ERROR_STATIC, m_ErrorStatic);
}


BEGIN_MESSAGE_MAP(CTxDOTOptionalDesignReportPage, CPropertyPage)
   ON_WM_SIZE()
   ON_WM_ERASEBKGND()
   ON_COMMAND(ID_FILE_PRINT, &CTxDOTOptionalDesignReportPage::OnFilePrint)
   ON_COMMAND_RANGE(CCS_CMENU_BASE, CCS_CMENU_MAX, OnCmenuSelected)
   ON_CBN_SELCHANGE(IDC_REPORT_COMBO, &CTxDOTOptionalDesignReportPage::OnCbnSelchangeReportCombo)
   ON_WM_CTLCOLOR()
   ON_COMMAND(ID_HELP, &CTxDOTOptionalDesignReportPage::OnHelpFinder)
   ON_COMMAND(ID_HELP_FINDER, &CTxDOTOptionalDesignReportPage::OnHelpFinder)
END_MESSAGE_MAP()


// TxDOTOptionalDesignReportPage message handlers

BOOL CTxDOTOptionalDesignReportPage::OnInitDialog()
{
   __super::OnInitDialog();

   // Always start with short report
   m_ReportCombo.SetCurSel(0);

   // At this point our document is alive. 
   // We aren't a view, so we subvert doc view and listen directly to data source
   ASSERT(m_pData);
   m_pData->Attach(this);

   // This is our first update - we know changes have happened
   m_ChangeStatus = ITxDataObserver::ctPGSuper;

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CTxDOTOptionalDesignReportPage::OnSetActive()
{
   // Only need to update report if data has changed
   if (m_ChangeStatus!=0)
   {
      try
      {
//         AFX_MANAGE_STATE(AfxGetAppModuleState()); // autoprogress complains if not in app state

         // Make sure our bridge data has been updated
         CComPtr<IBroker> pBroker = m_pBrokerRetriever->GetUpdatedBroker();

         GET_IFACE2(pBroker,IProgress,pProgress);
         CEAFAutoProgress ap(pProgress);
         pProgress->UpdateMessage(_T("Building Report"));

         if (m_pBrowser==NULL)
         {
            // Create a new browser
            CreateNewBrowser(pBroker);
         }
         else
         {
            GET_IFACE2(pBroker, IReportManager,pReportMgr);
            // Create spec for currently selected report
            m_pRptSpec = CreateSelectedReportSpec(pReportMgr);

            // Already have a browser, just need to re-marry report
            boost::shared_ptr<CReportBuilder> pBuilder = pReportMgr->GetReportBuilder( m_pRptSpec->GetReportName() );
            boost::shared_ptr<rptReport> pReport = pBuilder->CreateReport( m_pRptSpec );
            m_pBrowser->UpdateReport( pReport, true );
         }

         // our data is updated
         m_ChangeStatus = 0;

      }
      catch(TxDOTBrokerRetrieverException exc)
      {
         // An error occurred in analysis - go to error mode
         DisplayErrorMode(exc);
      }
      catch(...)
      {
         ASSERT(0);
         TxDOTBrokerRetrieverException exc;
         exc.Message = _T("An Unknown Error has Occurred");
         DisplayErrorMode(exc);
      }
   }

   return __super::OnSetActive();
}

void CTxDOTOptionalDesignReportPage::DisplayErrorMode(TxDOTBrokerRetrieverException& exc)
{
   // go into error mode - delete browser and displaymessage

   m_BrowserPlaceholder.ShowWindow(SW_HIDE);
   m_pBrowser = boost::shared_ptr<CReportBrowser>();

   CString msg;
   msg.Format(_T("Error - Analysis run Failed because: \n %s \n More Information May be in Status Center"),exc.Message);
   m_ErrorStatic.SetWindowText(msg);
   m_ErrorStatic.ShowWindow(SW_SHOW);
}

void CTxDOTOptionalDesignReportPage::CreateNewBrowser(IBroker* pBroker)
{
   // Make sure we're out of error mode
   m_BrowserPlaceholder.ShowWindow(SW_SHOW);
   m_ErrorStatic.ShowWindow(SW_HIDE);

   GET_IFACE2(pBroker, IReportManager,pReportMgr);

   // Create spec for currently selected report
   m_pRptSpec = CreateSelectedReportSpec(pReportMgr);

   // Create our browser
   CRect rect;
   m_BrowserPlaceholder.GetClientRect(&rect);

   GET_IFACE2(pBroker,IProgress,pProgress);
   CEAFAutoProgress ap(pProgress);

   m_pBrowser = pReportMgr->CreateReportBrowser(m_BrowserPlaceholder.GetSafeHwnd(),m_pRptSpec);
   m_pBrowser->Size(rect.Size());

   // resize browser window
   this->SendMessage(WM_SIZE);
}

boost::shared_ptr<CReportSpecification> CTxDOTOptionalDesignReportPage::CreateSelectedReportSpec(IReportManager* pReportMgr)
{
   int curidx = m_ReportCombo.GetCurSel();
   ASSERT(curidx==0 || curidx==1);

   // Two choices here
   LPCTSTR spec_name = curidx==0 ? _T("TxDOT Optional Girder Analysis (TOGA) - Short Report") : _T("TxDOT Optional Girder Analysis (TOGA) - Long Report");

   // Get our report description
   CReportDescription rptDesc = pReportMgr->GetReportDescription(spec_name);
   boost::shared_ptr<CReportSpecificationBuilder> pRptSpecBuilder = pReportMgr->GetReportSpecificationBuilder(rptDesc);
   boost::shared_ptr<CReportSpecification> pDefRptSpec = pRptSpecBuilder->CreateDefaultReportSpec(rptDesc);
   boost::shared_ptr<CSpanGirderReportSpecification> pSGRptSpec = boost::dynamic_pointer_cast<CSpanGirderReportSpecification,CReportSpecification>(pDefRptSpec);
   pSGRptSpec->SetSpan(TOGA_SPAN);
   pSGRptSpec->SetGirder(TOGA_FABR_GDR);

   return pDefRptSpec;
}

void CTxDOTOptionalDesignReportPage::OnTxDotDataChanged(int change)
{
   // save change information
   m_ChangeStatus |= change;
}

void CTxDOTOptionalDesignReportPage::OnSize(UINT nType, int cx, int cy)
{
   CPropertyPage::OnSize(nType, cx, cy);

   //AFX_MANAGE_STATE(AfxGetStaticModuleState());

   if (m_pBrowser)
   {
      // Convert a 7du x 7du rect into pixels
      CRect sizeRect(0,0,7,7);
      MapDialogRect(&sizeRect);

      CRect clientRect;
      GetClientRect( &clientRect );

      // Figure out the browser's new position
      if (::IsWindow(m_BrowserPlaceholder.m_hWnd))
      {
         CRect browserRect;
         browserRect.left = clientRect.left + sizeRect.Width();
         browserRect.top  = clientRect.top + 3 * sizeRect.Width();
         browserRect.right  = clientRect.right  - sizeRect.Width();
         browserRect.bottom = clientRect.bottom - 3 * sizeRect.Height();

         m_BrowserPlaceholder.MoveWindow( browserRect, FALSE );

         CSize size = browserRect.Size();
         size -= CSize(4,4);

         m_pBrowser->Size( size );
      }
   }

   Invalidate();
}

BOOL CTxDOTOptionalDesignReportPage::OnEraseBkgnd(CDC* pDC)
{
   // Set brush to dialog background color
   CBrush backBrush;
   backBrush.CreateSolidBrush(TXDOT_BACK_COLOR);

   // Save old brush
   CBrush* pOldBrush = pDC->SelectObject(&backBrush);

   CRect rect;
   pDC->GetClipBox(&rect);     // Erase the area needed

   pDC->PatBlt(rect.left, rect.top, rect.Width(), rect.Height(),
       PATCOPY);
   pDC->SelectObject(pOldBrush);

   return true;
}


void CTxDOTOptionalDesignReportPage::OnFilePrint()
{
   m_pBrowser->Print(true);
}

void CTxDOTOptionalDesignReportPage::OnEdit()
{
   EditReport();
}

void CTxDOTOptionalDesignReportPage::EditReport()
{
   m_pBrowser->Edit();
   m_pRptSpec = m_pBrowser->GetReportSpecification();
}

void CTxDOTOptionalDesignReportPage::OnCmenuSelected(UINT id)
{
  UINT cmd = id-CCS_CMENU_BASE ;

  switch(cmd)
  {
  case CCS_RB_EDIT:
     OnEdit();
     break;

  case CCS_RB_FIND:
     m_pBrowser->Find();
     break;

  case CCS_RB_SELECT_ALL:
     m_pBrowser->SelectAll();
     break;

  case CCS_RB_PRINT:
     m_pBrowser->Print(true);
     break;

  case CCS_RB_REFRESH:
     m_pBrowser->Refresh();
     break;

  case CCS_RB_VIEW_SOURCE:
     m_pBrowser->ViewSource();
     break;

  case CCS_RB_VIEW_BACK:
     m_pBrowser->Back();
     break;

  case CCS_RB_VIEW_FORWARD:
     m_pBrowser->Forward();
     break;

  default:
     // must be a toc anchor
     CHECK(cmd>=CCS_RB_TOC);
     m_pBrowser->NavigateAnchor(cmd-CCS_RB_TOC);
  }
}

void CTxDOTOptionalDesignReportPage::OnCbnSelchangeReportCombo()
{
   try
   {
      // Report type changed - need to create a new browser
      CComPtr<IBroker> pBroker = m_pBrokerRetriever->GetUpdatedBroker();

      // Create a new browser
      CreateNewBrowser(pBroker);
   }
   catch(TxDOTBrokerRetrieverException exc)
   {
      // An error occurred in analysis - go to error mode
      DisplayErrorMode(exc);
   }
   catch(...)
   {
      ASSERT(0);
      TxDOTBrokerRetrieverException exc;
      exc.Message = _T("An Unknown Error has Occurred");
      DisplayErrorMode(exc);
   }
}

HBRUSH CTxDOTOptionalDesignReportPage::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
   pDC->SetBkColor(TXDOT_BACK_COLOR);

   CBrush backBrush;
   backBrush.CreateSolidBrush(TXDOT_BACK_COLOR);

   return (HBRUSH)backBrush;

}

void CTxDOTOptionalDesignReportPage::AssertValid() const
{
   // Asserts will fire if not in static module state
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   __super::AssertValid();
}


void CTxDOTOptionalDesignReportPage::OnHelpFinder()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   CWinApp* pApp = AfxGetApp();
   ::HtmlHelp( *this, pApp->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_REPORT_TAB );
}
