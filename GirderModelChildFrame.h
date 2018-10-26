///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2016  Washington State Department of Transportation
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
// Operations
   // Let our views tell us about updates
	void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);

   // status of the current views
   EventIndexType GetEvent() const {return m_EventIndex;}

   // iCutLocation
   virtual Float64 GetCurrentCutLocation();
   virtual void CutAt(Float64 Xg);
   virtual void CutAtNext();
   virtual void CutAtPrev();
   virtual void ShowCutDlg();
   virtual Float64 GetMinCutLocation();
   virtual Float64 GetMaxCutLocation();

   pgsPointOfInterest GetCutLocation();



   void SelectGirder(const CGirderKey& girderKey,bool bDoUpdate);
   const CGirderKey& GetSelection() const;

protected:
   bool DoSyncWithBridgeModelView();

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
	afx_msg void OnSelectEvent();
	afx_msg void OnAddPointload();
	afx_msg void OnAddDistributedLoad();
	afx_msg void OnAddMoment();
   afx_msg void OnSync();
   afx_msg void OnSetFocus(CWnd* pOldWnd);
	//}}AFX_MSG
   afx_msg LRESULT OnCommandHelp(WPARAM, LPARAM lParam);
	DECLARE_MESSAGE_MAP()

   // update views - refreshes frame and views
   void UpdateViews();
   // update bar - set dialog bar content and update views
   void UpdateBar();

   void UpdateMaxCutLocation();

   virtual CRuntimeClass* GetLowerPaneClass() const;
   virtual Float64 GetTopFrameFraction() const;
   void UpdateCutLocation(const pgsPointOfInterest& poi);
   void OnUpdateFrameTitle(BOOL bAddToTitle);
   
   virtual BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext);

   CGirderModelElevationView* GetGirderModelElevationView() const;
   CGirderModelSectionView*   GetGirderModelSectionView() const;

private:
   void OnGirderChanged();
   void OnGroupChanged();
   void OnSectionCut();
   void DoFilePrint(bool direct);
   void FillEventComboBox();

   CToolPalette m_SettingsBar;

   // view variables
   Float64 m_CurrentCutLocation;
   Float64 m_MaxCutLocation;
   
   EventIndexType m_EventIndex; 

   CGirderKey m_GirderKey;
   bool m_bIsAfterFirstUpdate;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GIRDERMODELCHILDFRAME_H__19F76E39_6848_11D2_9D7B_00609710E6CE__INCLUDED_)
