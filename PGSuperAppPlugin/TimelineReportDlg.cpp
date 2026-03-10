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

// TimelinereportDlg.cpp : implementation file
//

#include "stdafx.h"
#include "TimelineReportDlg.h"
#include <EAF/EAFReportManager.h>
#include <EAF\EAFDocument.h>
#include <EAF\EAFCustSiteVars.h>
#include <IFace/Tools.h>


/////////////////////////////////////////////////////////////////////////////
// CTimelineReportDlg dialog
CTimelineReportDlg::CTimelineReportDlg(std::shared_ptr<CTimelineManagerReportSpecification>& pRptSpec, CWnd* pParent /*=nullptr*/)
	: CDialog(CTimelineReportDlg::IDD, pParent), m_pRptSpec(pRptSpec)
{
	//{{AFX_DATA_INIT(CTimelineReportDlg)
	//}}AFX_DATA_INIT
}

void CTimelineReportDlg::DoDataExchange(CDataExchange* pDX)
{
   CDialog::DoDataExchange(pDX);
   //{{AFX_DATA_MAP(CTimelineReportDlg)
   //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CTimelineReportDlg, CDialog)
	//{{AFX_MSG_MAP(CTimelineReportDlg)
	ON_WM_SIZE()
	ON_BN_CLICKED(IDC_PRINT, OnPrint)
   ON_COMMAND_RANGE(CCS_CMENU_BASE, CCS_CMENU_MAX, OnCmenuSelected)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTimelineReportDlg message handlers

void CTimelineReportDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);
   m_pBrowser->FitToParent();
}

void CTimelineReportDlg::OnPrint()
{
   m_pBrowser->Print(true);
}

void CTimelineReportDlg::OnOK()
{
   CleanUp();
	CDialog::OnOK();
}

BOOL CTimelineReportDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

   
   auto pBroker = m_pRptSpec->GetBroker();

   std::shared_ptr<WBFL::Reporting::ReportSpecification> pRptSpec = std::dynamic_pointer_cast<CBrokerReportSpecification, CTimelineManagerReportSpecification>(m_pRptSpec);

   GET_IFACE2(pBroker,IEAFReportManager,pRptMgr);
   std::shared_ptr<WBFL::Reporting::ReportSpecificationBuilder> nullSpecBuilder;
   CWnd* pWnd = GetDlgItem(IDC_BROWSER);
   m_pBrowser = pRptMgr->CreateReportBrowser(pWnd->GetSafeHwnd(),0,pRptSpec,nullSpecBuilder);

   // restore the size of the window
   {
      CEAFApp* pApp = EAFGetApp();
      WINDOWPLACEMENT wp;
      if (pApp->ReadWindowPlacement(CString("Window Positions"),CString("TimelineManager"),&wp))
      {
         HMONITOR hMonitor = MonitorFromRect(&wp.rcNormalPosition, MONITOR_DEFAULTTONULL); // get the monitor that has maximum overlap with the dialog rectangle (returns null if none)
         if (hMonitor != NULL)
         {
            // if dialog is within a monitor, set its position... otherwise the default position will be sued
            SetWindowPos(NULL, wp.rcNormalPosition.left, wp.rcNormalPosition.top, wp.rcNormalPosition.right - wp.rcNormalPosition.left, wp.rcNormalPosition.bottom - wp.rcNormalPosition.top, 0);
         }
      }
   }


	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CTimelineReportDlg::CleanUp()
{
   if ( m_pBrowser )
   {
      m_pBrowser = std::shared_ptr<WBFL::Reporting::ReportBrowser>();
   }

   // save the size of the window
   WINDOWPLACEMENT wp;
   wp.length = sizeof wp;
   {
      CEAFApp* pApp = EAFGetApp();
      if (GetWindowPlacement(&wp))
      {
         wp.flags = 0;
         wp.showCmd = SW_SHOWNORMAL;
         pApp->WriteWindowPlacement(CString("Window Positions"),CString("TimelineManager"),&wp);
      }
   }
}

void CTimelineReportDlg::OnCmenuSelected(UINT id)
{
  UINT cmd = id-CCS_CMENU_BASE ;

  switch(cmd)
  {
  case CCS_RB_EDIT:
//     OnEdit();
     break;

  case CCS_RB_FIND:
     m_pBrowser->Find();
     break;

  case CCS_RB_SELECT_ALL:
     m_pBrowser->SelectAll();
     break;

  case CCS_RB_COPY:
     m_pBrowser->Copy();
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
