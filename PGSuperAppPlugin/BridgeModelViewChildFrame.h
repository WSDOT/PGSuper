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

#include <DManip\ToolPalette.h>

#include <EAF\EAFViewControllerFactory.h>

class CBridgeViewPane;
class CBridgePlanView;
class CBridgeSectionView;
class CAlignmentPlanView;
class CAlignmentProfileView;

/////////////////////////////////////////////////////////////////////////////
// CBridgeModelViewChildFrame frame

class CBridgeModelViewChildFrame : public CSplitChildFrame, public iCutLocation, public CEAFViewControllerFactory
{
	DECLARE_DYNCREATE(CBridgeModelViewChildFrame)
protected:
	CBridgeModelViewChildFrame();           // protected constructor used by dynamic creation

// Attributes
public:
   enum ViewMode {Bridge,Alignment};

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
				CMDIFrameWnd* pParentWnd = nullptr,
				CCreateContext* pContext = nullptr);
	//}}AFX_VIRTUAL
   void SelectPier(PierIndexType pierIdx);
   void SelectSpan(SpanIndexType spanIdx);
   void SelectGirder(const CGirderKey& girderKey);
   void SelectSegment(const CSegmentKey& segmentKey);
   void SelectClosureJoint(const CSegmentKey& closureKey);
   void SelectTemporarySupport(SupportIDType tsID);
   void SelectDeck();
   void SelectAlignment();
   void SelectTrafficBarrier(pgsTypes::TrafficBarrierOrientation orientation);
   void ClearSelection();

   void InvalidateCutLocation() {m_bCutLocationInitialized = false;}

   // iCutLocation
   virtual Float64 GetCurrentCutLocation() override;
   virtual void CutAt(Float64 X) override;
   virtual void CutAtNext() override;
   virtual void CutAtPrev() override;
   virtual void ShowCutDlg() override;
   virtual void GetCutRange(Float64* pMin, Float64* pMax) override;

   // CEAFViewControllerFactory
protected:
   virtual void CreateViewController(IEAFViewController** ppController) override;


   // my stuff
public:
   Float64 GetNextCutStation(Float64 direction);

   CBridgePlanView* GetBridgePlanView();
   CBridgeSectionView* GetBridgeSectionView();
   CAlignmentPlanView* GetAlignmentPlanView();
   CAlignmentProfileView* GetAlignmentProfileView();
   CBridgeViewPane* GetUpperView();
   CBridgeViewPane* GetLowerView();

   void InitSpanRange(); // call this method to initialize the span range controls

   void SetViewMode(ViewMode viewMode);
   ViewMode GetViewMode() const;
   void NorthUp(bool bNorthUp);
   bool NorthUp() const;
   void ShowLabels(bool bShowLabels);
   bool ShowLabels() const;
   void ShowDimensions(bool bShowDimensions);
   bool ShowDimensions() const;
   void ShowBridge(bool bShowBridge);
   bool ShowBridge() const;
   void Schematic(bool bSchematic);
   bool Schematic() const;
   void ShowRwCrossSection(bool bShow);
   bool ShowRwCrossSection() const;


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
	afx_msg void OnViewGirder();
	afx_msg void OnDeletePier();
	afx_msg void OnUpdateDeletePier(CCmdUI* pCmdUI);
	afx_msg void OnDeleteSpan();
	afx_msg void OnUpdateDeleteSpan(CCmdUI* pCmdUI);
	afx_msg void OnInsertSpan();
	afx_msg void OnInsertPier();
   afx_msg void OnViewModeChanged(UINT nIDC);
   afx_msg void OnStartSpanChanged(NMHDR *pNMHDR, LRESULT *pResult);
   afx_msg void OnEndSpanChanged(NMHDR *pNMHDR, LRESULT *pResult);
   afx_msg void OnNorth();
   afx_msg void OnUpdateNorth(CCmdUI* pCmdUI);
   afx_msg void OnShowLabels();
   afx_msg void OnUpdateShowLabels(CCmdUI* pCmdUI);
   afx_msg void OnDimensions();
   afx_msg void OnUpdateDimensions(CCmdUI* pCmdUI);
   afx_msg void OnBridge();
   afx_msg void OnUpdateBridge(CCmdUI* pCmdUI);
   afx_msg void OnRwCrossSection();
   afx_msg void OnUpdateRwCrossSection(CCmdUI* pCmdUI);
   afx_msg void OnSchematic();
   afx_msg void OnUpdateSchematic(CCmdUI* pCmdUI);
   afx_msg void OnBoundaryCondition(UINT nIDC);
   afx_msg void OnUpdateBoundaryCondition(CCmdUI* pCmdUI);
   afx_msg LRESULT OnCommandHelp(WPARAM, LPARAM lParam);
   //}}AFX_MSG
	DECLARE_MESSAGE_MAP()

   bool m_bCutLocationInitialized;

   bool m_bSelecting; // set to true while a display object is being selected (prevents recurive selection)

   virtual CRuntimeClass* GetLowerPaneClass() const;
   virtual Float64 GetTopFrameFraction() const;
   void DoFilePrint(bool direct);

   Float64 m_CurrentCutLocation;
   void UpdateCutLocation(Float64 cut);

   CToolPalette m_SettingsBar;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_BRIDGEMODELVIEWILDFRAME_H__19F76E39_6848_11D2_9D7B_00609710E6CE__INCLUDED_)
