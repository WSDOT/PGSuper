#if !defined(AFX_SPECDEFLECTIONSPAGE_H__34909A2B_1E2E_496E_9C60_B1021A9AC123__INCLUDED_)
#define AFX_SPECDEFLECTIONSPAGE_H__34909A2B_1E2E_496E_9C60_B1021A9AC123__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SpecDeflectionsPage.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSpecDeflectionsPage dialog

class CSpecDeflectionsPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CSpecDeflectionsPage)

// Construction
public:
	CSpecDeflectionsPage();
	~CSpecDeflectionsPage();

// Dialog Data
	//{{AFX_DATA(CSpecDeflectionsPage)
	enum { IDD = IDD_SPEC_DEFLECTIONS };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CSpecDeflectionsPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
   void DoCheckLlDeflection();
	// Generated message map functions
	//{{AFX_MSG(CSpecDeflectionsPage)
	afx_msg void OnCheckLlDeflection();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
   afx_msg LRESULT OnCommandHelp(WPARAM, LPARAM lParam);
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SPECDEFLECTIONSPAGE_H__34909A2B_1E2E_496E_9C60_B1021A9AC123__INCLUDED_)
