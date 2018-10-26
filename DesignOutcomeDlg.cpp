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

// DesignOutcomeDlg.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\PGSuperApp.h"
#include "DesignOutcomeDlg.h"
#include "HtmlHelp\HelpTopics.hh"

#include <EAF\EAFAutoProgress.h>

#include <IFace\Artifact.h>
#include <IReportManager.h>

#include <EAF\EAFApp.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDesignOutcomeDlg dialog
CDesignOutcomeDlg::CDesignOutcomeDlg(boost::shared_ptr<CMultiGirderReportSpecification>& pRptSpec,CWnd* pParent /*=NULL*/)
	: CDialog(CDesignOutcomeDlg::IDD, pParent), m_pRptSpec(pRptSpec)
{
	//{{AFX_DATA_INIT(CDesignOutcomeDlg)
	//}}AFX_DATA_INIT
}

void CDesignOutcomeDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDesignOutcomeDlg)
	DDX_Control(pDX, IDC_STATIC_TEXT, m_Static);
	DDX_Control(pDX, IDOK, m_Ok);
	DDX_Control(pDX, IDCANCEL, m_Cancel);
	DDX_Control(pDX, IDC_PRINT, m_Print);
	DDX_Control(pDX, ID_HELP, m_Help);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDesignOutcomeDlg, CDialog)
	//{{AFX_MSG_MAP(CDesignOutcomeDlg)
	ON_WM_SIZE()
	ON_BN_CLICKED(IDC_PRINT, OnPrint)
	ON_BN_CLICKED(ID_HELP, OnHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDesignOutcomeDlg message handlers

void CDesignOutcomeDlg::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	
   ATLASSERT(m_Ok.m_hWnd!=0);

   // Convert a 7du x 7du rect into pixels
   CRect sizeRect(0,0,7,7);
   MapDialogRect(&sizeRect);

   CRect btnSizeRect(0,0,50,14);
   MapDialogRect( &btnSizeRect );

   CRect btnResultSizeRect(0,0,95,14);
   MapDialogRect( &btnResultSizeRect );

   CRect clientRect;
   GetClientRect( &clientRect );

   // Figure out the print button's new position
   CRect printRect;
   printRect.bottom = clientRect.bottom - sizeRect.Height();
   printRect.right  = clientRect.right  - sizeRect.Width();
   printRect.left   = printRect.right   - btnSizeRect.Width();
   printRect.top    = printRect.bottom  - btnSizeRect.Height();

   // Figure out the help button's new position
   CRect helpRect;
   helpRect.bottom = printRect.bottom;
   helpRect.right  = printRect.left   - sizeRect.Width();
   helpRect.left   = helpRect.right  - btnSizeRect.Width();
   helpRect.top    = printRect.top;

   // Figure out the ok button's new position
   CRect okRect;
   okRect.bottom = printRect.bottom;
   okRect.top    = printRect.top;
   okRect.left   = clientRect.left + sizeRect.Width();
   okRect.right  = okRect.left    + btnResultSizeRect.Width();

   CRect cancelRect;
   cancelRect.bottom = printRect.bottom;
   cancelRect.top    = printRect.top;
   cancelRect.left   = okRect.right  + sizeRect.Width();
   cancelRect.right  = okRect.right + btnResultSizeRect.Width();

   // Figure out the static box and its controls' new positions
   CRect staticRect;
   staticRect.bottom = printRect.top - sizeRect.Height()/10;
   staticRect.right  = printRect.right;
   staticRect.left   = clientRect.left  + sizeRect.Width();
   staticRect.top    = staticRect.bottom - btnSizeRect.Height();

   // Figure out the browser's new position
   CRect browserRect;
   browserRect.left   = clientRect.left + sizeRect.Width();
   browserRect.top    = clientRect.top  + sizeRect.Height();
   browserRect.right  = printRect.right;
   browserRect.bottom = staticRect.top - sizeRect.Height();

   m_Ok.MoveWindow( okRect, FALSE );
   m_Cancel.MoveWindow( cancelRect, FALSE );
   m_Help.MoveWindow( helpRect, FALSE );
   m_Print.MoveWindow( printRect, FALSE );
   m_Static.MoveWindow( staticRect, FALSE );


   m_pBrowser->Move(browserRect.TopLeft());
   m_pBrowser->Size( browserRect.Size() );

   Invalidate();
}

void CDesignOutcomeDlg::OnPrint() 
{
   m_pBrowser->Print(true);
}

void CDesignOutcomeDlg::OnCancel() 
{
   CleanUp();
	CDialog::OnCancel();
}

void CDesignOutcomeDlg::OnOK() 
{
   CleanUp();
	CDialog::OnOK();
}

void CDesignOutcomeDlg::OnHelp() 
{
   ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_DIALOG_DESIGNCOMPLETE );
}

BOOL CDesignOutcomeDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

   CComPtr<IBroker> pBroker;
   m_pRptSpec->GetBroker(&pBroker);

/*
   SpanIndexType span = m_pRptSpec->GetSpan();
   GirderIndexType gdr = m_pRptSpec->GetGirder();

   GET_IFACE2(pBroker,IArtifact,pArtifact);
   const pgsSegmentDesignArtifact* pDesignArtifact = pArtifact->GetDesignArtifact(span,gdr);
   ATLASSERT( pDesignArtifact != NULL ); // the design should be done if we are displaying the outcome

   // If design failed, default for Update Bridge Description is No, otherwise, Yes
   if ( pDesignArtifact->GetOutcome() == pgsSegmentDesignArtifact::Success )
      SetDefID(IDOK);
   else
      SetDefID(IDCANCEL);
*/

   GET_IFACE2(pBroker,IReportManager,pRptMgr);
   boost::shared_ptr<CReportSpecification> pRptSpec = boost::dynamic_pointer_cast<CReportSpecification,CMultiGirderReportSpecification>(m_pRptSpec);
   boost::shared_ptr<CReportSpecificationBuilder> nullSpecBuilder;
   m_pBrowser = pRptMgr->CreateReportBrowser(GetSafeHwnd(),pRptSpec,nullSpecBuilder);

   // restore the size of the window
   {
      CEAFApp* pApp = EAFGetApp();
      WINDOWPLACEMENT wp;
      if (pApp->ReadWindowPlacement("Settings","DesignOutcome",&wp))
      {
         CRect rect(wp.rcNormalPosition);
         SetWindowPos(NULL,0,0,rect.Size().cx,rect.Size().cy,SWP_NOMOVE);
      }
   }

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CDesignOutcomeDlg::CleanUp()
{
   if ( m_pBrowser )
   {
      m_pBrowser = boost::shared_ptr<CReportBrowser>();
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
         pApp->WriteWindowPlacement("Settings","DesignOutcome",&wp);
      }
   }
}
