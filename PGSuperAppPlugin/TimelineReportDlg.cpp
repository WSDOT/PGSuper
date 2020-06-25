///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2020  Washington State Department of Transportation
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
#include <IReportManager.h>
#include <EAF\EAFDocument.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTimelineReportDlg dialog
CTimelineReportDlg::CTimelineReportDlg(std::shared_ptr<CBrokerReportSpecification>& pRptSpec, CWnd* pParent /*=nullptr*/)
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
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTimelineReportDlg message handlers

void CTimelineReportDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);

   CRect hiddenRect;
   GetDlgItem(IDC_BROWSER)->GetWindowRect(&hiddenRect);
   ScreenToClient(hiddenRect);
   m_pBrowser->Move(hiddenRect.TopLeft());
   m_pBrowser->Size( hiddenRect.Size() );

   Invalidate();
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

   CComPtr<IBroker> pBroker;
   m_pRptSpec->GetBroker(&pBroker);

   std::shared_ptr<CReportSpecification> pRptSpec = std::dynamic_pointer_cast<CReportSpecification, CBrokerReportSpecification>(m_pRptSpec);

   GET_IFACE2(pBroker,IReportManager,pRptMgr);
   std::shared_ptr<CReportSpecificationBuilder> nullSpecBuilder;
   m_pBrowser = pRptMgr->CreateReportBrowser(GetSafeHwnd(),pRptSpec,nullSpecBuilder);

   // restore the size of the window
   {
      CEAFApp* pApp = EAFGetApp();
      WINDOWPLACEMENT wp;
      if (pApp->ReadWindowPlacement(CString("Window Positions"),CString("TimelineManager"),&wp))
      {
         CWnd* pDesktop = GetDesktopWindow();
         //CRect rDesktop;
         //pDesktop->GetWindowRect(&rDesktop); // this is the size of one monitor.... use GetSystemMetrics to get the entire desktop
         CRect rDesktop(0, 0, GetSystemMetrics(SM_CXVIRTUALSCREEN), GetSystemMetrics(SM_CYVIRTUALSCREEN));
         CRect rDlg(wp.rcNormalPosition);
         if (rDesktop.PtInRect(rDlg.TopLeft()) && rDesktop.PtInRect(rDlg.BottomRight()))
         {
            // if dialog is within the desktop area, set its position... otherwise the default position will be sued
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
      m_pBrowser = std::shared_ptr<CReportBrowser>();
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
