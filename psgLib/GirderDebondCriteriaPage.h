///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2020  Washington State Department of Transportation
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
   afx_msg void OnHelp();
	DECLARE_MESSAGE_MAP()

   BOOL OnInitDialog();

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
