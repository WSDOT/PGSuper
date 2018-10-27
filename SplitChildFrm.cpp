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

// ChildFrm.cpp : implementation of the CSplitChildFrame class
//

#include "PGSuperAppPlugin\stdafx.h"
#include "SplitChildFrm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSplitChildFrame


BEGIN_MESSAGE_MAP(CSplitChildFrame, CChildFrame)
	//{{AFX_MSG_MAP(CSplitChildFrame)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSplitChildFrame construction/destruction

CSplitChildFrame::CSplitChildFrame()
{
   m_bPanesCreated = FALSE;
}

CSplitChildFrame::~CSplitChildFrame()
{
}

/////////////////////////////////////////////////////////////////////////////
// CSplitChildFrame diagnostics

#ifdef _DEBUG
void CSplitChildFrame::AssertValid() const
{
	CChildFrame::AssertValid();
}

void CSplitChildFrame::Dump(CDumpContext& dc) const
{
	CChildFrame::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CSplitChildFrame message handlers
IMPLEMENT_DYNAMIC(CSplitChildFrame, CChildFrame)

BOOL CSplitChildFrame::OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext) 
{
   m_SplitterWnd.SetFirstPaneFraction(GetTopFrameFraction());


   // Create a splitter window with 2 rows and 1 column
   if ( !m_SplitterWnd.CreateStatic( this, 2, 1 ) )
   {
      TRACE0("Failed to create static splitter");
      return FALSE;
   }

   // Add the first pane
   int top_siz = (int)(((Float64)lpcs->cy)*GetTopFrameFraction());
   int bot_siz = lpcs->cy - top_siz;
   if ( !m_SplitterWnd.CreateView(0,0,pContext->m_pNewViewClass, CSize(lpcs->cx,top_siz), pContext) )
   {
      TRACE0("Failed to create first pane");
      return FALSE;
   }

   // call virtual function to get run time class of lower pane
   CRuntimeClass* prclass = GetLowerPaneClass();

   // Add the second pane
   if ( !m_SplitterWnd.CreateView(1,0, prclass, CSize(lpcs->cx,bot_siz), pContext ) )
   {
      TRACE0("Failed to create second pane");
      return FALSE;
   }

   // Activate the first pane
   SetActiveView( (CView*)m_SplitterWnd.GetPane(0,0));
	
   // I don't know why you don't call the parent but it makes the difference between
   // the splitter working and not.  See the ViewEx example. They don't call the
   // parent method either.
	//return CChildFrame::OnCreateClient(lpcs, pContext);
   m_bPanesCreated = TRUE;
   return TRUE;
}
