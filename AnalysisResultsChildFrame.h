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

#if !defined(AFX_ANALYSISRESULTSCHILDFRAME_H__19F76E39_6848_11D2_9D7B_00609710E6CE__INCLUDED_)
#define AFX_ANALYSISRESULTSCHILDFRAME_H__19F76E39_6848_11D2_9D7B_00609710E6CE__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
//
#include <EAF\EAFOutputChildFrame.h>
#include "AnalysisResultsGraphDefinition.h"
#include <vector>

#if defined _EAF_USING_MFC_FEATURE_PACK
#include <EAF\EAFPaneDialog.h>
#else
#define CEAFPaneDialog CDialogBar
#endif

class CAnalysisResultsView;

/////////////////////////////////////////////////////////////////////////////
// CAnalysisResultsChildFrame frame

class CAnalysisResultsChildFrame : public CEAFOutputChildFrame
{
	DECLARE_DYNCREATE(CAnalysisResultsChildFrame)
protected:
	CAnalysisResultsChildFrame();           // protected constructor used by dynamic creation

// Attributes
public:

// Operations
   // Update from doc/view - called by section view
   void Update(LPARAM lHint);

   // status of the current view settings
   void SelectSpan(SpanIndexType spanIdx,GirderIndexType gdrIdx);
   SpanIndexType GetSpanIdx() const;
   GirderIndexType GetGirderIdx() const;
   pgsTypes::Stage  GetStage() const;
   int GetGraphCount() const; // returns number of load cases, limit states, product loads, etc that are selected
   GraphType GetGraphType(int graphIdx) const;
   pgsTypes::AnalysisType GetAnalysisType() const;

   pgsTypes::LimitState  GetLimitState(int graphIdx) const;
   ProductForceType  GetProductLoadCase(int graphIdx) const;
   LoadingCombination GetCombinedLoadCase(int graphIdx) const;
   COLORREF GetGraphColor(int graphIdx) const;
   CString GetGraphDataLabel(int graphIdx) const;
   pgsTypes::LiveLoadType GetLiveLoadType(int graphIdx) const;
   VehicleIndexType GetVehicleIndex(int graphIdx) const;
   ActionType GetAction() const {return m_Action;}
   bool GetGrid() const {return m_Grid;}
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAnalysisResultsChildFrame)
	public:
	virtual BOOL Create(LPCTSTR lpszClassName,
				LPCTSTR lpszWindowName,
				DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_OVERLAPPEDWINDOW,
				const RECT& rect = rectDefault,
				CMDIFrameWnd* pParentWnd = NULL,
				CCreateContext* pContext = NULL);
	//}}AFX_VIRTUAL
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

// Implementation
protected:
	virtual ~CAnalysisResultsChildFrame();

	// Generated message map functions
	//{{AFX_MSG(CAnalysisResultsChildFrame)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	//}}AFX_MSG
   afx_msg LRESULT OnCommandHelp(WPARAM, LPARAM lParam);
	DECLARE_MESSAGE_MAP()

   // update views - refreshes frame and views
   void UpdateViews();
   void UpdateBar();

   CAnalysisResultsView* GetAnalysisResultsView() const;

public:
   void OnUpdateFrameTitle(BOOL bAddToTitle);

private:
   void OnGirderChanged();
   void OnSpanChanged();
   void OnStageChanged();
   void OnLoadCaseChanged();
   void OnActionChanged();
   void OnGridClicked();
   void OnAnalysisTypeClicked();
   void FillLoadCtrl();
   void FillSpanCtrl();
   void FillActionCtrl();
   void CreateGraphDefinitions();

   int SelectedGraphIndexToGraphID(int graphIdx) const;

   CEAFPaneDialog m_SettingsBar;

   // view variables
   SpanIndexType          m_SpanIdx;
   GirderIndexType        m_GirderIdx;
   pgsTypes::Stage        m_Stage;
   ActionType             m_Action;
   pgsTypes::AnalysisType m_AnalysisType;
   bool                   m_Grid;

   CAnalysisResultsGraphDefinitions m_GraphDefinitions;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ANALYSISRESULTSCHILDFRAME_H__19F76E39_6848_11D2_9D7B_00609710E6CE__INCLUDED_)
