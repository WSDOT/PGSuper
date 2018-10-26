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

// TxDOTOptionalDesignChildFrm.cpp : implementation of the CTxDOTOptionalDesignChildFrame class
//

#include "stdafx.h"
#include "resource.h"
#include "HtmlHelp\TogaHelp.hh"

#include "TxDOTOptionalDesignChildFrame.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTxDOTOptionalDesignChildFrame

IMPLEMENT_DYNCREATE(CTxDOTOptionalDesignChildFrame, CEAFChildFrame)

BEGIN_MESSAGE_MAP(CTxDOTOptionalDesignChildFrame, CEAFChildFrame)
	//{{AFX_MSG_MAP(CTxDOTOptionalDesignChildFrame)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG_MAP
   ON_WM_CREATE()
   ON_COMMAND(ID_LICENSE_AGREEMENT, &CTxDOTOptionalDesignChildFrame::OnLicenseAgreement)
   ON_WM_HELPINFO()
   ON_COMMAND(ID_HELP_FINDER, &CTxDOTOptionalDesignChildFrame::OnHelpFinder)
   ON_COMMAND(ID_HELP, &CTxDOTOptionalDesignChildFrame::OnHelpFinder)
   ON_WM_GETMINMAXINFO()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTxDOTOptionalDesignChildFrame construction/destruction

CTxDOTOptionalDesignChildFrame::CTxDOTOptionalDesignChildFrame()
{
}

CTxDOTOptionalDesignChildFrame::~CTxDOTOptionalDesignChildFrame()
{
}

BOOL CTxDOTOptionalDesignChildFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if( !CEAFChildFrame::PreCreateWindow(cs) )
		return FALSE;

   // Make the frame start in a maximized state
   cs.style = WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_MAXIMIZE; // start the window in a maximized state

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CTxDOTOptionalDesignChildFrame diagnostics

#ifdef _DEBUG
void CTxDOTOptionalDesignChildFrame::AssertValid() const
{
	CEAFChildFrame::AssertValid();
}

void CTxDOTOptionalDesignChildFrame::Dump(CDumpContext& dc) const
{
	CEAFChildFrame::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CTxDOTOptionalDesignChildFrame message handlers

void CTxDOTOptionalDesignChildFrame::SetFrameSize(int cx,int cy)
{
   m_szFrame.cx = cx;
   m_szFrame.cy = cy;
}

void CTxDOTOptionalDesignChildFrame::OnUpdateFrameTitle(BOOL bAddToTitle)
{
	if (bAddToTitle)
   {
      CString msg(_T("Toga Plugin"));

      // set our title
		AfxSetWindowText(m_hWnd, msg);
   }
}

void CTxDOTOptionalDesignChildFrame::OnLicenseAgreement()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CWinApp* papp = AfxGetApp();
   ::HtmlHelp( *this, papp->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_LICENSE );
}

BOOL CTxDOTOptionalDesignChildFrame::OnHelpInfo(HELPINFO* pHelpInfo)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   CWinApp* pApp = AfxGetApp();
   ::HtmlHelp( *this, pApp->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_GIRDER_INPUT );

   return TRUE;
}

void CTxDOTOptionalDesignChildFrame::OnHelpFinder()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   CWinApp* pApp = AfxGetApp();
   ::HtmlHelp( *this, pApp->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_WELCOME );
}

void CTxDOTOptionalDesignChildFrame::OnGetMinMaxInfo(MINMAXINFO* lpMMI)
{
   CEAFChildFrame::OnGetMinMaxInfo(lpMMI); // get the default values

   if ( !IsZoomed() && !IsIconic() )
   {
   // Want to prevent resizing so we are going to
   // set the min and max tracking size ot the same values
   lpMMI->ptMinTrackSize.x = m_szFrame.cx;
   lpMMI->ptMinTrackSize.y = m_szFrame.cy;
   lpMMI->ptMaxTrackSize.x = m_szFrame.cx;
   lpMMI->ptMaxTrackSize.y = m_szFrame.cy;
   }
}
