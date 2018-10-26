#pragma once
#include "afxwin.h"

class CTxDOTOptionalDesignView :
   public CFormView
{
public:
	DECLARE_DYNCREATE(CTxDOTOptionalDesignView)
   CTxDOTOptionalDesignView(void);
   ~CTxDOTOptionalDesignView(void);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSnapView)
	public:
	virtual BOOL Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext = NULL);
	virtual void OnInitialUpdate();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

public:
   // update input data on currently viewed page
   BOOL UpdateCurrentPageData();


// Generated message map functions
protected:
	//{{AFX_MSG(CSnapView)
//	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
protected:

	// property sheet is wired to MDI child frame and is not displayed
	CPropertySheet* m_pPropSheet;

	// one page for each menu so we can initialize controls
	// using OnInitDialog
	CPropertyPage* m_pBridgeInputPage;
	CPropertyPage* m_pGirderInputPage;
	CPropertyPage* m_pGirderViewPage;
	CPropertyPage* m_pReportPage;

public:
   afx_msg void OnSize(UINT nType, int cx, int cy);
   virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
   virtual BOOL DestroyWindow();
   virtual BOOL PreTranslateMessage(MSG* pMsg);
   virtual void AssertValid() const;
};
