#if !defined(AFX_GIRDERDEBONDCRITERIAPAGE_H__F7C3C78F_28B6_466B_9933_C766C1BCA9ED__INCLUDED_)
#define AFX_GIRDERDEBONDCRITERIAPAGE_H__F7C3C78F_28B6_466B_9933_C766C1BCA9ED__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// GirderDebondCriteriaPage.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CGirderDebondCriteriaPage dialog

class CGirderDebondCriteriaPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CGirderDebondCriteriaPage)

// Construction
public:
	CGirderDebondCriteriaPage();
	~CGirderDebondCriteriaPage();

// Dialog Data
	//{{AFX_DATA(CGirderDebondCriteriaPage)
	enum { IDD = IDD_GIRDER_DEBOND_CRITERIA };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CGirderDebondCriteriaPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CGirderDebondCriteriaPage)
	afx_msg void OnCheckMaxLengthFraction();
	afx_msg void OnCheckMaxLength();
	//}}AFX_MSG
   afx_msg LRESULT OnCommandHelp(WPARAM, LPARAM lParam);
	DECLARE_MESSAGE_MAP()

   void UpdateCheckBoxes();
   void UpdateDebondCheckBoxes();
   void UpdateDesignCheckBoxes();

   void EnableCtrls(int* ctrlIDs, BOOL enable);

public:
   afx_msg void OnBnClickedStraightDesignCheck();
   afx_msg void OnBnClickedDebondDesignCheck();
   afx_msg void OnBnClickedHarpedDesignCheck();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GIRDERDEBONDCRITERIAPAGE_H__F7C3C78F_28B6_466B_9933_C766C1BCA9ED__INCLUDED_)
