///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright (C) 1999  Washington State Department of Transportation
//                     Bridge and Structures Office
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

#if !defined(AFX_REPORTVIEW_H__E2B376C8_2D38_11D2_8EB4_006097DF3C68__INCLUDED_)
#define AFX_REPORTVIEW_H__E2B376C8_2D38_11D2_8EB4_006097DF3C68__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// ReportView.h : header file
//

#include "AutoCalcView.h"
#include <IReportManager.h>

/////////////////////////////////////////////////////////////////////////////
// CReportView view

class CReportView : public CAutoCalcView
{
protected:
	CReportView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CReportView)

// Attributes
public:

// Operations
public:
   bool DoResultsExist() const;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CReportView)
	public:
	virtual void OnInitialUpdate();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	//}}AFX_VIRTUAL

// Implementation
public:
   virtual void UpdateNow();

protected:
	virtual ~CReportView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

public:
   bool CreateReport(CollectionIndexType rptIdx,bool bPromptForSpec=true);

   // Generated message map functions
protected:
	//{{AFX_MSG(CReportView)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnFilePrint();
	afx_msg void OnToolbarPrint();
	afx_msg void OnUpdateFilePrint(CCmdUI* pCmdUI);
	afx_msg void OnTimer(UINT nIDEvent);
   afx_msg void OnEdit();
   afx_msg BOOL OnEraseBkgnd(CDC* pDC);

	//}}AFX_MSG
   afx_msg void OnCmenuSelected(UINT id);
	DECLARE_MESSAGE_MAP()

   virtual void CreateReportSpecification(CollectionIndexType rptIdx,bool bCreateDefaultReport);
   virtual HRESULT UpdateReportBrowser();

protected:
   boost::shared_ptr<CReportBrowser> m_pReportBrowser;
   boost::shared_ptr<CReportSpecification> m_pReportSpec;

   bool m_bInvalidReport; // true if an update event is received and the contents of the report are not invalid
   bool m_bNoBrowser;     // true if the browser window couldn't be created
   bool m_bUpdateError;   // true if an error occured while updating the report contents
   bool m_bIsNewReport;   // true while calls are coming from OnInitialUpdate

   std::string m_ErrorMsg;

   static bool ms_bIsUpdatingReport; // true while the report content is being updated

   CButton m_btnEdit;
   CFont   m_btnFont;

   UINT m_Timer;
   UINT m_TimerEvent;
   UINT m_Timeout;
 
   void UpdateViewTitle();
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_REPORTVIEW_H__E2B376C8_2D38_11D2_8EB4_006097DF3C68__INCLUDED_)
