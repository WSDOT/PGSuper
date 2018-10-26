#if !defined(AFX_SPECDEBONDINGPAGE_H__9A9548B4_94A8_4F90_84B9_A3170F670D1C__INCLUDED_)
#define AFX_SPECDEBONDINGPAGE_H__9A9548B4_94A8_4F90_84B9_A3170F670D1C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SpecDebondingPage.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSpecDebondingPage dialog

class CSpecDebondingPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CSpecDebondingPage)

// Construction
public:
	CSpecDebondingPage();
	~CSpecDebondingPage();

// Dialog Data
	//{{AFX_DATA(CSpecDebondingPage)
	enum { IDD = IDD_SPEC_DEBONDING };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CSpecDebondingPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CSpecDebondingPage)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
   afx_msg LRESULT OnCommandHelp(WPARAM, LPARAM lParam);
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SPECDEBONDINGPAGE_H__9A9548B4_94A8_4F90_84B9_A3170F670D1C__INCLUDED_)
