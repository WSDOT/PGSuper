///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2012  Washington State Department of Transportation
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

#if !defined(AFX_BRIDGEMODELVIEWILDFRAME_H__19F76E39_6848_11D2_9D7B_00609710E6CE__INCLUDED_)
#define AFX_BRIDGEMODELVIEWILDFRAME_H__19F76E39_6848_11D2_9D7B_00609710E6CE__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// cfSplitChildFrame.h : header file
//
#include "SplitChildFrm.h"
#include "SectionCutDrawStrategy.h"
#include "PGSuperTypes.h"

class CBridgePlanView;
class CBridgeSectionView;

#if defined _EAF_USING_MFC_FEATURE_PACK
#include <EAF\EAFPaneDialog.h>
#else
#define CEAFPaneDialog CDialogBar
#endif

/////////////////////////////////////////////////////////////////////////////
// CBridgeModelViewChildFrame frame

class CBridgeModelViewChildFrame : public CSplitChildFrame, public iCutLocation
{
	DECLARE_DYNCREATE(CBridgeModelViewChildFrame)
protected:
	CBridgeModelViewChildFrame();           // protected constructor used by dynamic creation

// Attributes
public:

// Operations
public:
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CBridgeModelViewChildFrame)
	public:
	virtual BOOL Create(LPCTSTR lpszClassName,
				LPCTSTR lpszWindowName,
				DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_OVERLAPPEDWINDOW,
				const RECT& rect = rectDefault,
				CMDIFrameWnd* pParentWnd = NULL,
				CCreateContext* pContext = NULL);
	//}}AFX_VIRTUAL
   void SelectPier(PierIndexType pierIdx);
   void SelectSpan(SpanIndexType spanIdx);
   void SelectGirder(SpanIndexType spanIdx,GirderIndexType gdrIdx);
   void SelectDeck();
   void SelectAlignment();
   void ClearSelection();

   // iCutLocation
   void InvalidateCutLocation() {m_bCutLocationInitialized = false;}
   Float64 GetCurrentCutLocation();
   void CutAt(Float64 cut);
   void ShowCutDlg();

   LPCTSTR GetDeckTypeName(pgsTypes::SupportedDeckType deckType) const;

   CBridgePlanView* GetBridgePlanView();
   CBridgeSectionView* GetBridgeSectionView();

   void InitSpanRange(); // call this method to initialize the span range controls

#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

// Implementation
protected:
	virtual ~CBridgeModelViewChildFrame();
   virtual BOOL PreCreateWindow(CREATESTRUCT& cs);

	// Generated message map functions
	//{{AFX_MSG(CBridgeModelViewChildFrame)
	afx_msg void OnFilePrint();
	afx_msg void OnFilePrintDirect();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
   afx_msg void OnEditSpan();
	afx_msg void OnEditPier();
	afx_msg void OnEditGirder();
	afx_msg void OnViewGirder();
	afx_msg void OnDeletePier();
	afx_msg void OnUpdateDeletePier(CCmdUI* pCmdUI);
	afx_msg void OnDeleteSpan();
	afx_msg void OnUpdateDeleteSpan(CCmdUI* pCmdUI);
	afx_msg void OnInsertSpan();
	afx_msg void OnInsertPier();
	//}}AFX_MSG
   afx_msg void OnBoundaryCondition(UINT nIDC);
   afx_msg void OnUpdateBoundaryCondition(CCmdUI* pCmdUI);
   afx_msg LRESULT OnCommandHelp(WPARAM, LPARAM lParam);
	DECLARE_MESSAGE_MAP()

   bool m_bCutLocationInitialized;

   virtual CRuntimeClass* GetLowerPaneClass() const;
   virtual double GetTopFrameFraction() const;
   void DoFilePrint(bool direct);

   Float64 m_CurrentCutLocation;
   void UpdateCutLocation(Float64 cut);

   CEAFPaneDialog m_SettingsBar;
public:
   afx_msg void OnStartSpanChanged(NMHDR *pNMHDR, LRESULT *pResult);
   afx_msg void OnEndSpanChanged(NMHDR *pNMHDR, LRESULT *pResult);
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_BRIDGEMODELVIEWILDFRAME_H__19F76E39_6848_11D2_9D7B_00609710E6CE__INCLUDED_)
