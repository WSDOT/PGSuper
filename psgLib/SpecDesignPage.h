#if !defined(AFX_SpecDesignPage_H__6A54414E_08FE_4D7B_BB7A_846CEC129E71__INCLUDED_)
#define AFX_SpecDesignPage_H__6A54414E_08FE_4D7B_BB7A_846CEC129E71__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SpecDesignPage.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSpecDesignPage dialog

class CSpecDesignPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CSpecDesignPage)
// Construction
public:
	CSpecDesignPage(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CSpecDesignPage)
	enum { IDD = IDD_SPEC_DESIGN };
	BOOL	m_CheckA;
	BOOL	m_CheckHauling;
	BOOL	m_CheckHoldDown;
	BOOL	m_CheckLifting;
	BOOL	m_CheckSlope;
	BOOL	m_DesignA;
	BOOL	m_DesignHauling;
	BOOL	m_DesignHoldDown;
	BOOL	m_DesignLifting;
	BOOL	m_DesignSlope;
	BOOL	m_CheckSplitting;
	BOOL	m_DesignSplitting;
	BOOL	m_CheckConfinement;
	BOOL	m_DesignConfinement;
	//}}AFX_DATA

   int m_FillMethod;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSpecDesignPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CSpecDesignPage)
	afx_msg void OnCheckA();
	afx_msg void OnCheckHauling();
	afx_msg void OnCheckHd();
	afx_msg void OnCheckLifting();
	afx_msg void OnCheckSlope();
	afx_msg void OnCheckSplitting();
	afx_msg void OnCheckConfinement();
	//}}AFX_MSG
   afx_msg LRESULT OnCommandHelp(WPARAM, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SpecDesignPage_H__6A54414E_08FE_4D7B_BB7A_846CEC129E71__INCLUDED_)
