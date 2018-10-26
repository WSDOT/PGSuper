///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2016  Washington State Department of Transportation
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

// ChildFrm.cpp : implementation of the CEditLoadsChildFrame class
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\PGSuperApp.h"
#include "PGSuperAppPlugin\resource.h"

#include "EditLoadsChildFrm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CEditLoadsChildFrame

IMPLEMENT_DYNCREATE(CEditLoadsChildFrame, CEAFChildFrame)

BEGIN_MESSAGE_MAP(CEditLoadsChildFrame, CEAFChildFrame)
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

	if( !CEAFChildFrame::PreCreateWindow(cs) )
		return FALSE;

	return TRUE;
}


BOOL CEditLoadsChildFrame::Create(LPCTSTR lpszClassName,
				LPCTSTR lpszWindowName,
				DWORD dwStyle,
				const RECT& rect,
				CMDIFrameWnd* pParentWnd,
				CCreateContext* pContext)
{
#if defined _EAF_USING_MFC_FEATURE_PACK
   // If MFC Feature pack is used, we are using tabbed MDI windows so we don't want
   // the system menu or the minimize and maximize boxes
   dwStyle &= ~(WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);
#endif

   BOOL bResult = CEAFChildFrame::Create(lpszClassName,lpszWindowName,dwStyle,rect,pParentWnd,pContext);
   if ( bResult )
   {
      AFX_MANAGE_STATE(AfxGetStaticModuleState());
      HICON hIcon = AfxGetApp()->LoadIcon(IDR_EDITLOADS);
      SetIcon(hIcon,TRUE);
   }

   return bResult;
}


/////////////////////////////////////////////////////////////////////////////
// CEditLoadsChildFrame diagnostics

#ifdef _DEBUG
void CEditLoadsChildFrame::AssertValid() const
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
	CEAFChildFrame::AssertValid();
}

void CEditLoadsChildFrame::Dump(CDumpContext& dc) const
{
	CEAFChildFrame::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CEditLoadsChildFrame message handlers


void CEditLoadsChildFrame::OnUpdateFrameTitle(BOOL bAddToTitle)
{
	if (bAddToTitle)
   {
      CString msg("Loads");

      // set our title
		AfxSetWindowText(m_hWnd, msg);
   }
}
