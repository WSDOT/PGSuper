///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright (C) 1999  Washington State Department of Transportation
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

// ChildFrm.cpp : implementation of the CEditLoadsChildFrame class
//

#include "stdafx.h"
#include "PgSuper.h"

#include "EditLoadsChildFrm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CEditLoadsChildFrame

IMPLEMENT_DYNCREATE(CEditLoadsChildFrame, CMDIChildWnd)

BEGIN_MESSAGE_MAP(CEditLoadsChildFrame, CMDIChildWnd)
	//{{AFX_MSG_MAP(CEditLoadsChildFrame)
	ON_WM_CREATE()
	ON_WM_SHOWWINDOW()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEditLoadsChildFrame construction/destruction

CEditLoadsChildFrame::CEditLoadsChildFrame()
{
	// TODO: add member initialization code here
	
}

CEditLoadsChildFrame::~CEditLoadsChildFrame()
{
}

BOOL CEditLoadsChildFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	if( !CMDIChildWnd::PreCreateWindow(cs) )
		return FALSE;

	return TRUE;
}



/////////////////////////////////////////////////////////////////////////////
// CEditLoadsChildFrame diagnostics

#ifdef _DEBUG
void CEditLoadsChildFrame::AssertValid() const
{
	CMDIChildWnd::AssertValid();
}

void CEditLoadsChildFrame::Dump(CDumpContext& dc) const
{
	CMDIChildWnd::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CEditLoadsChildFrame message handlers


void CEditLoadsChildFrame::OnUpdateFrameTitle(BOOL bAddToTitle)
{
	if (bAddToTitle)
   {
      CString msg("Edit User-Defined Loads");

      // set our title
		AfxSetWindowText(m_hWnd, msg);
   }
}
