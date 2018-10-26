///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2010  Washington State Department of Transportation
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

// ChildFrm.cpp : implementation of the COutputChildFrame class
//

#include "stdafx.h"
#include "PGSuper.h"

#include "OutputChildFrame.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// COutputChildFrame

IMPLEMENT_DYNCREATE(COutputChildFrame, CLicensePlateChildFrame)

BEGIN_MESSAGE_MAP(COutputChildFrame, CLicensePlateChildFrame)
	//{{AFX_MSG_MAP(COutputChildFrame)
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// COutputChildFrame construction/destruction

COutputChildFrame::COutputChildFrame()
{
	// TODO: add member initialization code here
	
}

COutputChildFrame::~COutputChildFrame()
{
}

BOOL COutputChildFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

   // :TRICKY: rab 11.23.96 : Modifying default behavior
	// By turning off the default MFC-defined FWS_ADDTOTITLE style,
	// the framework will use first string in the document template
	// STRINGTABLE resource instead of the document name.

	cs.style &= ~(LONG)FWS_ADDTOTITLE;
	return CLicensePlateChildFrame::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// COutputChildFrame diagnostics

#ifdef _DEBUG
void COutputChildFrame::AssertValid() const
{
	CLicensePlateChildFrame::AssertValid();
}

void COutputChildFrame::Dump(CDumpContext& dc) const
{
	CLicensePlateChildFrame::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// COutputChildFrame message handlers

void COutputChildFrame::OnUpdateFrameTitle(BOOL bAddToTitle)
{
	if (bAddToTitle && (GetStyle() & FWS_ADDTOTITLE) == 0)
   {

      // TRICKY:
      // we expect our view to provide is with text
      CView* pv = this->GetActiveView();

      if ( pv )
      {
         ASSERT(pv!=0);
         CString name;
         pv->GetWindowText(name);

         // set our title
		   AfxSetWindowText(m_hWnd, name);
      }
   }
   else
   {
      CLicensePlateChildFrame::OnUpdateFrameTitle(bAddToTitle);
   }
}
