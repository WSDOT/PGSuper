///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2018  Washington State Department of Transportation
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

// ChildFrm.cpp : implementation of the CChildFrame class
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\PGSuperApp.h"

#include "ChildFrm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CChildFrame

IMPLEMENT_DYNCREATE(CChildFrame, CEAFChildFrame)

BEGIN_MESSAGE_MAP(CChildFrame, CEAFChildFrame)
	//{{AFX_MSG_MAP(CChildFrame)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CChildFrame construction/destruction

CChildFrame::CChildFrame()
{
}

CChildFrame::~CChildFrame()
{
}

BOOL CChildFrame::PreCreateWindow(CREATESTRUCT& cs)
{
   // :TRICKY: rab 11.23.96 : Modifying default behavior
	// By turning off the default MFC-defined FWS_ADDTOTITLE style,
	// the framework will use first string in the document template
	// STRINGTABLE resource instead of the document name.

	cs.style &= ~(LONG)FWS_ADDTOTITLE;
	return CEAFChildFrame::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CChildFrame diagnostics

#ifdef _DEBUG
void CChildFrame::AssertValid() const
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
	CEAFChildFrame::AssertValid();
}

void CChildFrame::Dump(CDumpContext& dc) const
{
	CEAFChildFrame::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CChildFrame message handlers

void CChildFrame::OnUpdateFrameTitle(BOOL bAddToTitle)
{
	if (bAddToTitle && (GetStyle() & FWS_ADDTOTITLE) == 0)
   {
      // By turning off the default MFC-defined FWS_ADDTOTITLE style,
      // the framework will use first string in the document template
      // STRINGTABLE resource instead of the document name. We want
      // to append a view count to the end of the window title.  
		TCHAR szText[256+_MAX_PATH];
      CString window_text;
      CString window_title;
      GetWindowText(window_text);

      // Look for the last :
      // The text to the left of the last : is the window title
      int idx = window_text.ReverseFind(_T(':'));
      if (idx != -1) // -1 meams : was not found
         window_title = window_text.Left(idx);
      else
         window_title = window_text;

		lstrcpy(szText,window_title);
//		if (m_nWindow > 0)
//			wsprintf(szText + lstrlen(szText), _T(":%d"), m_nWindow);

		// set title if changed, but don't remove completely
		AfxSetWindowText(m_hWnd, szText);
   }
   else
   {
      CEAFChildFrame::OnUpdateFrameTitle(bAddToTitle);
   }
}
