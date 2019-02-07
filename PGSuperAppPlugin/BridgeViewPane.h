///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2019  Washington State Department of Transportation
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

#pragma once

// BridgeViewPane.h : header file
//
//
#include <DManip\DManip.h>
#include "BridgeModelViewChildFrame.h"

/////////////////////////////////////////////////////////////////////////////
// CBridgeViewPane view

class CBridgeViewPane : public CDisplayView
{
   friend CBridgeModelViewChildFrame;
protected:
	CBridgeViewPane();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CBridgeViewPane)

// Attributes
public:

// Operations
public:
   virtual void DoPrint(CDC* pDC, CPrintInfo* pInfo,CRect rcDraw);


   CBridgeModelViewChildFrame* GetFrame();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CBridgeViewPane)
public:
	virtual void OnInitialUpdate();
protected:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CBridgeViewPane();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
protected:
   virtual void HandleLButtonDown(UINT nFlags, CPoint logPoint);

   //{{AFX_MSG(CAlignmentPlanView)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

   CBridgeModelViewChildFrame* m_pFrame;

   void DrawFocusRect();

   virtual void BuildDisplayLists() = 0;
   virtual void UpdateDrawingScale() = 0;
   virtual void UpdateDisplayObjects() = 0;

   virtual void UpdateDrawingArea();
};
