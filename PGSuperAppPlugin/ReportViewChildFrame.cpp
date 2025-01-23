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

// AnalysisResultsChildFrame.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "pgsuperDoc.h"
#include "ReportViewChildFrame.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CReportViewChildFrame

IMPLEMENT_DYNCREATE(CReportViewChildFrame, CEAFOutputChildFrame)

CReportViewChildFrame::CReportViewChildFrame()
{
}

CReportViewChildFrame::~CReportViewChildFrame()
{
}

BOOL CReportViewChildFrame::Create(LPCTSTR lpszClassName,
				LPCTSTR lpszWindowName,
				DWORD dwStyle,
				const RECT& rect,
				CMDIFrameWnd* pParentWnd,
				CCreateContext* pContext)
{
   BOOL bResult = CEAFOutputChildFrame::Create(lpszClassName,lpszWindowName,dwStyle,rect,pParentWnd,pContext);
   if ( bResult )
   {
      AFX_MANAGE_STATE(AfxGetStaticModuleState());
      HICON hIcon = AfxGetApp()->LoadIcon(IDR_REPORT);
      SetIcon(hIcon,TRUE);
   }

   return bResult;
}

BEGIN_MESSAGE_MAP(CReportViewChildFrame, CEAFOutputChildFrame)
	//{{AFX_MSG_MAP(CReportViewChildFrame)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CReportViewChildFrame message handlers

#ifdef _DEBUG
void CReportViewChildFrame::AssertValid() const
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
	CEAFOutputChildFrame::AssertValid();
}

void CReportViewChildFrame::Dump(CDumpContext& dc) const
{
	CEAFOutputChildFrame::Dump(dc);
}
#endif //_DEBUG
