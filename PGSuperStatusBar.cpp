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

// PGSuperStatusBar.cpp : implementation file
//

#include "stdafx.h"
#include "pgsuper.h"
#include "PGSuperStatusBar.h"
#include "PGSuperDoc.h"
#include "PGSuperColors.h"
#include <IFace\StatusCenter.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPGSuperStatusBar

CPGSuperStatusBar::CPGSuperStatusBar()
{
}

CPGSuperStatusBar::~CPGSuperStatusBar()
{
}


BEGIN_MESSAGE_MAP(CPGSuperStatusBar, CStatusBar)
	//{{AFX_MSG_MAP(CPGSuperStatusBar)
	ON_WM_LBUTTONDBLCLK()
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPGSuperStatusBar message handlers
void CPGSuperStatusBar::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{
   CDC* pDC = CDC::FromHandle(lpDrawItemStruct->hDC);

   COLORREF color;

   long status_count = 0;

   CWnd* pMainWnd = AfxGetMainWnd();
   if ( pMainWnd->IsKindOf(RUNTIME_CLASS(CFrameWnd)) )
   {
      CFrameWnd* pMainFrame = (CFrameWnd*)pMainWnd;
      CMDIChildWnd* pChild = (CMDIChildWnd*)pMainFrame->GetActiveFrame();
      CView* pView = pChild->GetActiveView();
      CDocument* pDoc = NULL;
      if ( pView )
         pDoc = pView->GetDocument();

      if ( pDoc && pDoc->IsKindOf(RUNTIME_CLASS(CPGSuperDoc)) )
      {
         CPGSuperDoc* pPGSuperDoc = (CPGSuperDoc*)pDoc;

         pgsStatusCenter& status_center = pPGSuperDoc->GetStatusCenter();
         pgsTypes::StatusSeverityType severity = status_center.GetSeverity();
         color = (severity == pgsTypes::statusOK      ? STATUS_OK_COLOR :
                  severity == pgsTypes::statusWarning ? STATUS_WARN_COLOR : STATUS_ERROR_COLOR);

         status_count = status_center.Count();

         CBrush brush(color);
         CPen pen(PS_SOLID,1,color);

         CBrush* pOldBrush = pDC->SelectObject(&brush);
         CPen* pOldPen = pDC->SelectObject(&pen);

         pDC->Rectangle( &(lpDrawItemStruct->rcItem) );

         CString strStatus;
         strStatus.Format("%d",status_count);
         int bkMode = pDC->SetBkMode(TRANSPARENT);
         pDC->DrawText(strStatus,&(lpDrawItemStruct->rcItem),DT_CENTER | DT_VCENTER);
         pDC->SetBkMode(bkMode);

         pDC->SelectObject(pOldBrush);
         pDC->SelectObject(pOldPen);
      }
   }
}

void CPGSuperStatusBar::OnLButtonDblClk(UINT nFlags, CPoint point) 
{
   CRect rect;

   // Analysis Type
   GetStatusBarCtrl().GetRect(1,&rect);
   if (rect.PtInRect(point))
   {
      PostMessage(WM_COMMAND,ID_PROJECT_ANALYSIS,0);
   }

   // Status Center
   GetStatusBarCtrl().GetRect(2,&rect);
   if (rect.PtInRect(point))
   {
      PostMessage(WM_COMMAND,ID_VIEW_STATUSCENTER,0);
   }

   // AutoCalc Mode
   GetStatusBarCtrl().GetRect(4,&rect);
   if (rect.PtInRect(point))
   {
      PostMessage(WM_COMMAND,ID_PROJECT_AUTOCALC,0);
   }

   CStatusBar::OnLButtonDblClk(nFlags, point);
}
