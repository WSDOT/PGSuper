#if !defined(AFX_SPECBRIDGESITE1PAGE_H__EC05479C_1AE6_11D4_AEA0_00105A9AF985__INCLUDED_)
#define AFX_SPECBRIDGESITE1PAGE_H__EC05479C_1AE6_11D4_AEA0_00105A9AF985__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SpecBridgeSite1Page.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSpecBridgeSite1Page dialog

class CSpecBridgeSite1Page : public CPropertyPage
{
	DECLARE_DYNCREATE(CSpecBridgeSite1Page)

// Construction
public:
	CSpecBridgeSite1Page();
	~CSpecBridgeSite1Page();

// Dialog Data
	//{{AFX_DATA(CSpecBridgeSite1Page)
	enum { IDD = IDD_SPEC_BRIDGE_SITE1 };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA

   afx_msg LRESULT OnCommandHelp(WPARAM, LPARAM lParam);
	void DoCheckMaxMax();
	void DoCheckMaxMax3();

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CSpecBridgeSite1Page)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CSpecBridgeSite1Page)
	virtual BOOL OnInitDialog();
	afx_msg void OnCheckNormalMaxMax2();
	afx_msg void OnCheckNormalMaxMax3();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
   afx_msg void OnBnClickedEvaluateTemporaryStresses();
   afx_msg void OnCheckBottomFlangeClearance();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SPECBRIDGESITE1PAGE_H__EC05479C_1AE6_11D4_AEA0_00105A9AF985__INCLUDED_)
