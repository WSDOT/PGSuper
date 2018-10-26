///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2015  Washington State Department of Transportation
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

// AlignmentProfileView.h : header file
//
//
#include <map>
#include <DManip\DManip.h>
#include <DManipTools\DManipTools.h>
#include "BridgeModelViewChildFrame.h"

/////////////////////////////////////////////////////////////////////////////
// CAlignmentProfileView view

class CAlignmentProfileView : public CDisplayView
{
   friend CBridgeModelViewChildFrame;
protected:
	CAlignmentProfileView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CAlignmentProfileView)

// Attributes
public:

// Operations
public:
   void DoPrint(CDC* pDC, CPrintInfo* pInfo,CRect rcDraw);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAlignmentProfileView)
	public:
	virtual void OnInitialUpdate();
	protected:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CAlignmentProfileView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
protected:
   virtual void HandleLButtonDown(UINT nFlags, CPoint logPoint);
	virtual void HandleLButtonDblClk(UINT nFlags, CPoint logPoint);
   virtual void HandleContextMenu(CWnd* pWnd,CPoint logPoint);

	//{{AFX_MSG(CAlignmentPlanView)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnViewSettings();
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

   CBridgeModelViewChildFrame* m_pFrame;

   void UpdateDisplayObjects();
   void BuildTitleDisplayObjects();
   void BuildProfileDisplayObjects();
   void BuildBridgeDisplayObjects();
   void BuildLabelDisplayObjects();

   void UpdateDrawingScale();

   void DrawFocusRect();

   void CreateStationLabel(iDisplayList* pDisplayList,Float64 station,LPCTSTR strBaseLabel=NULL,UINT textAlign=TA_BASELINE | TA_RIGHT);
   void CreateStationLabel(iDisplayList* pDisplayList,Float64 station,Float64 elevation,LPCTSTR strBaseLabel=NULL,UINT textAlign=TA_BASELINE | TA_RIGHT);

   CBridgeModelViewChildFrame* GetFrame();
};
