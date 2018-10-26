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

// MainFrm.h : interface of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_MAINFRM_H__59D503EE_265C_11D2_8EB0_006097DF3C68__INCLUDED_)
#define AFX_MAINFRM_H__59D503EE_265C_11D2_8EB0_006097DF3C68__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "CountedMultiDocTemplate.h"
#include "PGSuperStatusBar.h"
#include "PGSuperTypes.h"

#include <boost\shared_ptr.hpp>

class CPGSuperDoc;

class CMainFrame : public CMDIFrameWnd
{
	DECLARE_DYNAMIC(CMainFrame)
public:
	CMainFrame();

// Attributes
public:

// Operations
public:
   CPGSuperDoc* GetPGSuperDocument();

   void AutoCalcEnabled( bool bEnable );

   // Call this method if you don't want the view failed error message displayed.
   void DisableFailCreateMessage();

   // called by the document class when the modified flag changes
   // if bEnable is true, the modified status bar indicator is turned on
   // otherwise it is turned off
   void EnableModifiedFlag(BOOL bEnable);

   void SetAnalysisTypeStatusIndicator(pgsTypes::AnalysisType analysisType);

   // Creating a window was canceled by a user action.
   // Call this method before creation is complete and
   // the window will be destroyed (as if it was never created).
   void CreateCanceled();

   // need to have public access to frame updating function from document
   void UpdateFrameTitle(LPCTSTR lpszDocName);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMainFrame)
	public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	//}}AFX_VIRTUAL

   void UpdateToolbarStatusItems(pgsTypes::StatusSeverityType severity);
   void UpdateStatusBar();
   int GetToolBarIndex(UINT nID);

// Implementation
public:
	virtual ~CMainFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

   void CreateGirderEditorView(SpanIndexType spanIdx,GirderIndexType gdrIdx);
   void CreateReport(CollectionIndexType rptIdx,bool bPromptForSpec=true);
   void CreateAnalysisResultsView();

protected:  // control bar embedded members
	CPGSuperStatusBar  m_wndStatusBar;
   std::vector<boost::shared_ptr<CToolBar> > m_ToolBars;

// Generated message map functions
protected:
	//{{AFX_MSG(CMainFrame)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnViewBridgeModelEditor();
	afx_msg void OnViewGirderEditor();
	afx_msg void OnViewAnalysisResults();
//	afx_msg void OnViewReports();
   void OnViewReports(NMHDR* pnmtb,LRESULT* plr);
   afx_msg void OnViewStability();
	afx_msg void OnClose();
	afx_msg void OnViewToolbar();
	afx_msg void OnUpdateViewToolbar(CCmdUI* pCmdUI);
	afx_msg void OnUpdateEditors(CCmdUI* pCmdUI);
	afx_msg void OnDropFiles(HDROP hDropInfo);
   afx_msg void OnHelpFinder();
	afx_msg void OnEditUserloads();
public:
	afx_msg void OnViewLibraryEditor();
	//}}AFX_MSG
   afx_msg void OnHelp();
	DECLARE_MESSAGE_MAP()



private:
   CView* CreateOrActivateFrame(CCountedMultiDocTemplate* pTemplate,CRuntimeClass* pViewClass);
   void OnUpdateFrameTitle(BOOL bAddToTitle);
   void UpdateFrameTitleForDocument(LPCTSTR lpszDocName);
   void OnToolbarMenuSelected(UINT id);

   void DockControlBarLeftOf(CToolBar* Bar,CToolBar* LeftOf);
   std::vector<UINT> GetToolBarResourceIDs();
   std::vector<BOOL> GetToolBarStates();
   void SetToolBarStates(const std::vector<BOOL>& vStates);
   void ToggleToolBarState(Uint16 idx);

   bool m_bDisableFailCreateMsg;
   bool m_bCreateCanceled;
   BOOL m_bShowToolTips;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAINFRM_H__59D503EE_265C_11D2_8EB0_006097DF3C68__INCLUDED_)
