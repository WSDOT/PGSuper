///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2023  Washington State Department of Transportation
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

// TxDOTOptionalDesignReportPage.cpp : implementation file
//

#include "stdafx.h"
#include "TxDOTOptionalDesignReportPage.h"
#include "TxDOTOptionalDesignUtilities.h"

#include <IReportManager.h>
#include <EAF\EAFAutoProgress.h>
#include <EAF\EAFCustSiteVars.h>
#include <EAF\EAFDocument.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



// CTxDOTOptionalDesignReportPage dialog

IMPLEMENT_DYNAMIC(CTxDOTOptionalDesignReportPage, CPropertyPage)

CTxDOTOptionalDesignReportPage::CTxDOTOptionalDesignReportPage()
	: CPropertyPage(CTxDOTOptionalDesignReportPage::IDD),
   m_pData(nullptr),
   m_pBrokerRetriever(nullptr),
   m_ChangeStatus(0)
{

}

CTxDOTOptionalDesignReportPage::~CTxDOTOptionalDesignReportPage()
{
   if ( m_pBrowser )
   {
      m_pBrowser = nullptr;
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

         if (m_pBrowser==nullptr)
         {
            // Create a new browser
            CreateNewBrowser(pBroker);
         }
         else
         {
            GET_IFACE2(pBroker, IReportManager,pReportMgr);
            // Create spec for currently selected report
            m_pRptSpec = CreateSelectedReportSpec(pReportMgr);

            // Already have a browser, just need to update headers and footers and re-marry report
            std::shared_ptr<WBFL::Reporting::ReportSpecification> brRptSpec = m_pBrowser->GetReportSpecification();
            brRptSpec->SetLeftHeader(m_pRptSpec->GetLeftHeader().c_str());
            brRptSpec->SetCenterHeader(m_pRptSpec->GetCenterHeader().c_str());
            brRptSpec->SetLeftFooter(m_pRptSpec->GetLeftFooter().c_str());
            brRptSpec->SetCenterFooter(m_pRptSpec->GetCenterFooter().c_str());

            std::shared_ptr<WBFL::Reporting::ReportBuilder> pBuilder = pReportMgr->GetReportBuilder( m_pRptSpec->GetReportName() );
            std::shared_ptr<rptReport> pReport = pBuilder->CreateReport( m_pRptSpec );
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
   m_pBrowser = nullptr;

   CString msg;
   msg.Format(_T("Error - Analysis run Failed because: \n %s \n More Information May be in Status Center"),exc.Message);
   m_ErrorStatic.SetWindowText(msg);
   m_ErrorStatic.ShowWindow(SW_SHOW);
}

void CTxDOTOptionalDesignReportPage::CreateNewBrowser(IBroker* pBroker)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState())

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

   m_pBrowser = pReportMgr->CreateReportBrowser(m_BrowserPlaceholder.GetSafeHwnd(),m_pRptSpec,std::shared_ptr<WBFL::Reporting::ReportSpecificationBuilder>());
   m_pBrowser->Size(rect.Size());

   // resize browser window
   this->SendMessage(WM_SIZE);
}

std::shared_ptr<WBFL::Reporting::ReportSpecification> CTxDOTOptionalDesignReportPage::CreateSelectedReportSpec(IReportManager* pReportMgr)
{
   int curidx = m_ReportCombo.GetCurSel();
   ASSERT(curidx==0 || curidx==1);

   // Two choices here
   LPCTSTR spec_name = curidx==0 ? _T("TxDOT Optional Girder Analysis (TOGA) - Short Report") : _T("TxDOT Optional Girder Analysis (TOGA) - Long Report");

   // Get our report description
   WBFL::Reporting::ReportDescription rptDesc = pReportMgr->GetReportDescription(spec_name);
   std::shared_ptr<WBFL::Reporting::ReportSpecificationBuilder> pRptSpecBuilder = pReportMgr->GetReportSpecificationBuilder(rptDesc);
   std::shared_ptr<WBFL::Reporting::ReportSpecification> pDefRptSpec = pRptSpecBuilder->CreateDefaultReportSpec(rptDesc);
   std::shared_ptr<CGirderReportSpecification> pGirderRptSpec = std::dynamic_pointer_cast<CGirderReportSpecification,WBFL::Reporting::ReportSpecification>(pDefRptSpec);
   pGirderRptSpec->SetGirderKey(CGirderKey(TOGA_SPAN,TOGA_FABR_GDR));

   // Set report header and footer for printing
   CComPtr<IBroker> pBroker = m_pBrokerRetriever->GetUpdatedBroker();
   GET_IFACE2(pBroker,IGetTogaData,pGetTogaData);
   const CTxDOTOptionalDesignData* pProjectData = pGetTogaData->GetTogaData();

   CString bridgeName = CString(_T("Bridge: ")) + pProjectData->GetBridge();
   CString bridgeID   = CString(_T("BID: ")) + pProjectData->GetBridgeID();
   CString bridgeCmnt = CString(_T("Cmts: ")) +pProjectData->GetComments();

   pDefRptSpec->SetLeftHeader(bridgeName.GetBuffer());
   pDefRptSpec->SetCenterHeader(bridgeID.GetBuffer());
   pDefRptSpec->SetLeftFooter(bridgeCmnt.GetBuffer());

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
     ATLASSERT(cmd>=CCS_RB_TOC);
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
   EAFHelp( EAFGetDocument()->GetDocumentationSetName(), IDH_REPORT_TAB );
}
