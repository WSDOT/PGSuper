///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright (C) 2006  Washington State Department of Transportation
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

// AlignmentDescriptionDlg.cpp : implementation file
//

#include "stdafx.h"
#include "pgsuper.h"
#include "AlignmentDescriptionDlg.h"
#include "htmlhelp\HelpTopics.hh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAlignmentDescriptionDlg

IMPLEMENT_DYNAMIC(CAlignmentDescriptionDlg, CPropertySheet)

CAlignmentDescriptionDlg::CAlignmentDescriptionDlg(UINT nIDCaption,IBroker* pBroker, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(nIDCaption, pParentWnd, iSelectPage)
{
   m_pBroker = pBroker;
   Init();
}

CAlignmentDescriptionDlg::CAlignmentDescriptionDlg(LPCTSTR pszCaption, IBroker* pBroker,CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(pszCaption, pParentWnd, iSelectPage)
{
   m_pBroker = pBroker;
   Init();
}

CAlignmentDescriptionDlg::~CAlignmentDescriptionDlg()
{
}


BEGIN_MESSAGE_MAP(CAlignmentDescriptionDlg, CPropertySheet)
	//{{AFX_MSG_MAP(CAlignmentDescriptionDlg)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAlignmentDescriptionDlg message handlers

void CAlignmentDescriptionDlg::Init()
{
   m_psh.dwFlags |= PSH_HASHELP | PSH_NOAPPLYNOW;
   m_AlignmentPage.m_psp.dwFlags  |= PSP_HASHELP;
   m_ProfilePage.m_psp.dwFlags    |= PSP_HASHELP;
   m_CrownSlopePage.m_psp.dwFlags |= PSP_HASHELP;

   AddPage(&m_AlignmentPage);
   AddPage(&m_ProfilePage);
   AddPage(&m_CrownSlopePage);
}
