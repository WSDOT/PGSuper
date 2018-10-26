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

#if !defined(AFX_GIRDERMODELCHILDFRAME_H__19F76E39_6848_11D2_9D7B_00609710E6CE__INCLUDED_)
#define AFX_GIRDERMODELCHILDFRAME_H__19F76E39_6848_11D2_9D7B_00609710E6CE__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// cfSplitChildFrame.h : header file
// 
#include "SplitChildFrm.h"
#include "SectionCutDrawStrategy.h"
#include "pgsExt\PointLoadData.h"
#include <DManip\ToolPalette.h>

class CGirderModelElevationView;
class CGirderModelSectionView;
class CGirderViewPrintJob;


/////////////////////////////////////////////////////////////////////////////
// CGirderModelChildFrame frame

class CGirderModelChildFrame : public CSplitChildFrame, public iCutLocation
{
	DECLARE_DYNCREATE(CGirderModelChildFrame)

   friend CGirderViewPrintJob;
protected:
	CGirderModelChildFrame();           // protected constructor used by dynamic creation

// Attributes
public:
   enum CutLocation {LeftEnd,LeftHarp,Center,RightHarp,RightEnd,UserInput};

// Operations
   // Update from doc/view - called by section view
   void Update();
   // status of the current views
   UserLoads::Stage  GetLoadingStage() const {return m_LoadingStage;}
   Float64 GetCurrentCutLocation() {return m_CurrentCutLocation;}
   void CutAt(Float64 cut);
   void ShowCutDlg();
   void CutAtLocation();
   void CutAtLeftEnd();
   void CutAtLeftHp();
   void CutAtCenter();
   void CutAtRightHp(); 
   void CutAtRightEnd(); 

   void CutAtNext();
   void CutAtPrev();

   void SelectSpanAndGirder(SpanIndexType spanIdx,GirderIndexType gdrIdx);
   void GetSpanAndGirderSelection(SpanIndexType* pSpanIdx,GirderIndexType* pGdrIdx);

   bool SyncWithBridgeModelView();

   void RefreshGirderLabeling();

public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGirderModelChildFrame)
	public:
	virtual BOOL Create(LPCTSTR lpszClassName,
				LPCTSTR lpszWindowName,
				DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_OVERLAPPEDWINDOW,
				const RECT& rect = rectDefault,
				CMDIFrameWnd* pParentWnd = NULL,
				CCreateContext* pContext = NULL);
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CGirderModelChildFrame();

	// Generated message map functions
	//{{AFX_MSG(CGirderModelChildFrame)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnFilePrint();
	afx_msg void OnFilePrintDirect();
	afx_msg void OnSelectLoadingStage();
	afx_msg void OnAddPointload();
	afx_msg void OnAddDistributedLoad();
	afx_msg void OnAddMoment();
   afx_msg void OnSync();
	//}}AFX_MSG
   afx_msg LRESULT OnCommandHelp(WPARAM, LPARAM lParam);
	DECLARE_MESSAGE_MAP()

   // update views - refreshes frame and views
   void UpdateViews();
   // update bar - set dialog bar content and update views
   void UpdateBar();

   virtual CRuntimeClass* GetLowerPaneClass() const;
   virtual double GetTopFrameFraction() const;
   void UpdateCutLocation(CutLocation cutLoc,Float64 cut = 0.0);
   void OnUpdateFrameTitle(BOOL bAddToTitle);

   CGirderModelElevationView* GetGirderModelElevationView() const;
   CGirderModelSectionView*   GetGirderModelSectionView() const;

private:
   void OnGirderChanged();
   void OnSpanChanged();
   void OnSectionCut();
   void DoFilePrint(bool direct);

   CToolPalette m_SettingsBar;

   // view variables
   Float64 m_CurrentCutLocation;
   CutLocation m_CutLocation;
   Float64 m_MaxCutLocation;
   UserLoads::Stage m_LoadingStage;

   SpanIndexType m_CurrentSpanIdx;
   GirderIndexType m_CurrentGirderIdx;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GIRDERMODELCHILDFRAME_H__19F76E39_6848_11D2_9D7B_00609710E6CE__INCLUDED_)
