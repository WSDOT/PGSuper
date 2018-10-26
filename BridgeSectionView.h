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

#if !defined(AFX_BRIDGESECTIONVIEW_H__5244F08E_6900_11D2_9D7C_00609710E6CE__INCLUDED_)
#define AFX_BRIDGESECTIONVIEW_H__5244F08E_6900_11D2_9D7C_00609710E6CE__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// BridgeSectionView.h : header file
//
#include <DManip\DManip.h>
#include <DManipTools\DManipTools.h>
#include "BridgeModelViewChildFrame.h"

/////////////////////////////////////////////////////////////////////////////
// CBridgeSectionView view

class CBridgeSectionView : public CDisplayView
{
   friend CBridgeModelViewChildFrame;
protected:
	CBridgeSectionView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CBridgeSectionView)

// Attributes
public:

// Operations
public:
   void DoPrint(CDC* pDC, CPrintInfo* pInfo,CRect rcDraw);
   bool IsDeckSelected();
   void SelectPier(PierIndexType pierIdx,bool bSelect);
   void SelectSpan(PierIndexType pierIdx,bool bSelect);
   bool GetSelectedGirder(CGirderKey* pGirderKey);
   void SelectGirder(const CGirderKey& girderKey,bool bSelect);
   void SelectDeck(bool bSelect);
   void SelectAlignment(bool bSelect);
   void SelectTemporarySupport(bool bSelect);
   void ClearSelection();

   // Returns the index of the girder group that the section cut is being taken from
   GroupIndexType GetGroupIndex();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CBridgeSectionView)
	public:
	virtual void OnInitialUpdate();
	protected:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CBridgeSectionView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
protected:
	//{{AFX_MSG(CBridgeSectionView)
	afx_msg void OnEditDeck();
	afx_msg void OnViewSettings();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
   afx_msg BOOL OnMouseWheel(UINT nFlags,short zDelta,CPoint pt);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

   virtual void HandleLButtonDown(UINT nFlags, CPoint logPoint);
	virtual void HandleLButtonDblClk(UINT nFlags, CPoint logPoint);
   virtual void HandleContextMenu(CWnd* pWnd,CPoint logPoint);

   CBridgeModelViewChildFrame* m_pFrame;

   void UpdateDisplayObjects();
   void BuildTitleDisplayObjects();
   void BuildGirderDisplayObjects();
   void BuildDeckDisplayObjects();
   void BuildOverlayDisplayObjects();
   void BuildTrafficBarrierDisplayObjects();
   void BuildDimensionLineDisplayObjects();

   void UpdateDrawingScale();

   void UpdateGirderTooltips();

   void DrawFocusRect();

   void TrimSurface(IPoint2dCollection* pPoints,Float64 Xleft,Float64 Xright);

   CBridgeModelViewChildFrame* GetFrame();

   typedef std::map<CGirderKey,IDType> GirderIDCollection;
   GirderIDCollection m_GirderIDs; // maps girder keys into DMANIP display object IDs
   IDType m_NextGirderID;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_BRIDGESECTIONVIEW_H__5244F08E_6900_11D2_9D7C_00609710E6CE__INCLUDED_)
