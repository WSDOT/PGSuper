///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2014  Washington State Department of Transportation
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

#if !defined(AFX_FACTOROFSAFETYCHILDFRAME_H__19F76E39_6848_11D2_9D7B_00609710E6CE__INCLUDED_)
#define AFX_FACTOROFSAFETYCHILDFRAME_H__19F76E39_6848_11D2_9D7B_00609710E6CE__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
//
#include <EAF\EAFOutputChildFrame.h>
#include "PGSuperTypes.h"
#include "IFace\AnalysisResults.h"

#if defined _EAF_USING_MFC_FEATURE_PACK
#include <EAF\EAFPaneDialog.h>
#else
#define CEAFPaneDialog CDialogBar
#endif

class CFactorOfSafetyView;

/////////////////////////////////////////////////////////////////////////////
// CFactorOfSafetyChildFrame frame

class CFactorOfSafetyChildFrame : public CEAFOutputChildFrame
{
	DECLARE_DYNCREATE(CFactorOfSafetyChildFrame)
protected:
	CFactorOfSafetyChildFrame();           // protected constructor used by dynamic creation

// Attributes
public:
// Operations
   enum Stage { Lifting, Hauling };
   // Update from doc/view - called by section view
   void Update();
   void SelectSpan(SpanIndexType spanIdx,GirderIndexType gdrIdx);
   // status of the current view settings
   GirderIndexType GetGirderIdx() const;
   SpanIndexType   GetSpanIdx() const;
   Stage  GetStage() const;
   bool GetGrid() const {return m_Grid;}
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CFactorOfSafetyChildFrame)
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
	virtual ~CFactorOfSafetyChildFrame();

	// Generated message map functions
	//{{AFX_MSG(CFactorOfSafetyChildFrame)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	//}}AFX_MSG
   afx_msg LRESULT OnCommandHelp(WPARAM, LPARAM lParam);
	DECLARE_MESSAGE_MAP()

   // update views - refreshes frame and views
   void UpdateViews();

   // update bar - set dialog bar content and update views
   void UpdateBar();

   CFactorOfSafetyView* GetFactorOfSafetyView() const;

public:
   void OnUpdateFrameTitle(BOOL bAddToTitle);

private:
   void OnGirderChanged();
   void OnSpanChanged();
   void OnStageChanged();
   void OnGridClicked();

   CEAFPaneDialog m_SettingsBar;

   // view variables
   GirderIndexType m_GirderIdx;
   SpanIndexType m_SpanIdx;
   Stage  m_Stage;
   bool   m_Grid;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_FACTOROFSAFETYCHILDFRAME_H__19F76E39_6848_11D2_9D7B_00609710E6CE__INCLUDED_)
