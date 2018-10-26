///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2017  Washington State Department of Transportation
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

// BridgeViewPane.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\resource.h"
#include "PGSuperAppPlugin\PGSuperApp.h"
#include "BridgeViewPane.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CBridgeViewPane

IMPLEMENT_DYNAMIC(CBridgeViewPane, CDisplayView)

CBridgeViewPane::CBridgeViewPane()
{
}

CBridgeViewPane::~CBridgeViewPane()
{
}


BEGIN_MESSAGE_MAP(CBridgeViewPane, CDisplayView)
	//{{AFX_MSG_MAP(CAlignmentPlanView)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CBridgeViewPane drawing
void CBridgeViewPane::DoPrint(CDC* pDC, CPrintInfo* pInfo,CRect rcDraw)
{
   OnBeginPrinting(pDC, pInfo, rcDraw);
   OnPrepareDC(pDC);
   UpdateDrawingScale();
   OnDraw(pDC);
   OnEndPrinting(pDC, pInfo);
}

/////////////////////////////////////////////////////////////////////////////
// CAlignmentPlanView diagnostics

#ifdef _DEBUG
void CBridgeViewPane::AssertValid() const
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
	CDisplayView::AssertValid();
}

void CBridgeViewPane::Dump(CDumpContext& dc) const
{
	CDisplayView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CBridgePlanView message handlers
void CBridgeViewPane::OnInitialUpdate()
{
   BuildDisplayLists();
   CDisplayView::OnInitialUpdate();
}

void CBridgeViewPane::OnSize(UINT nType, int cx, int cy) 
{
	CDisplayView::OnSize(nType, cx, cy);

   CRect rect;
   GetClientRect(&rect);
   rect.DeflateRect(15,15,15,15);

   CSize size = rect.Size();
   size.cx = Max(0L,size.cx);
   size.cy = Max(0L,size.cy);

   SetLogicalViewRect(MM_TEXT,rect);

   SetScrollSizes(MM_TEXT,size,CScrollView::sizeDefault,CScrollView::sizeDefault);

   UpdateDrawingScale();
}

void CBridgeViewPane::OnDraw(CDC* pDC)
{
   CDisplayView::OnDraw(pDC);

   if ( CWnd::GetFocus() == this && !pDC->IsPrinting() )
   {
      DrawFocusRect();
   }
}

void CBridgeViewPane::HandleLButtonDown(UINT nFlags, CPoint logPoint)
{
   CBridgeModelViewChildFrame* pFrame = GetFrame();
   pFrame->ClearSelection();
}

void CBridgeViewPane::OnSetFocus(CWnd* pOldWnd) 
{
	CDisplayView::OnSetFocus(pOldWnd);
   DrawFocusRect();
}

void CBridgeViewPane::OnKillFocus(CWnd* pNewWnd) 
{
	CDisplayView::OnKillFocus(pNewWnd);
   DrawFocusRect();
}

int CBridgeViewPane::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CDisplayView::OnCreate(lpCreateStruct) == -1)
   {
		return -1;
   }
	
   m_pFrame = (CBridgeModelViewChildFrame*)GetParent()->GetParent();
   ASSERT( m_pFrame != 0 );
   ASSERT( m_pFrame->IsKindOf( RUNTIME_CLASS( CBridgeModelViewChildFrame ) ) );

	return 0;
}

CBridgeModelViewChildFrame* CBridgeViewPane::GetFrame()
{
   return m_pFrame;
}

void CBridgeViewPane::DrawFocusRect()
{
   CClientDC dc(this);
   CRect rClient;
   GetClientRect(&rClient);
   dc.DrawFocusRect(rClient);
}
