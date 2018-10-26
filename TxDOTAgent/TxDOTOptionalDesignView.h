///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2017  Washington State Department of Transportation
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
#include "afxwin.h"

class CTxDOTOptionalDesignView :
   public CView
{
public:
	DECLARE_DYNCREATE(CTxDOTOptionalDesignView)
   CTxDOTOptionalDesignView(void);
   ~CTxDOTOptionalDesignView(void);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSnapView)
	public:
	//virtual BOOL Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext = nullptr) override;
	virtual void OnInitialUpdate() override;
	protected:
	virtual void DoDataExchange(CDataExchange* pDX) override;    // DDX/DDV support
	//}}AFX_VIRTUAL

public:
   // update input data on currently viewed page
   BOOL UpdateCurrentPageData();


// Generated message map functions
protected:
	//{{AFX_MSG(CSnapView)
   afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
protected:
	virtual void OnDraw(CDC* pDC) override;      // default does nothing
   virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo) override;
   virtual BOOL DestroyWindow() override;
   virtual BOOL PreTranslateMessage(MSG* pMsg) override;

	// property sheet is wired to MDI child frame and is not displayed
	CPropertySheet* m_pPropSheet;

	// one page for each menu so we can initialize controls
	// using OnInitDialog
	CPropertyPage* m_pBridgeInputPage;
	CPropertyPage* m_pGirderInputPage;
	CPropertyPage* m_pGirderViewPage;
	CPropertyPage* m_pReportPage;

   CSize m_szMin;

public:

#if defined _DEBUG
   virtual void AssertValid() const override;
#endif
};
