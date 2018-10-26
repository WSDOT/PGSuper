///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2011  Washington State Department of Transportation
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

#if !defined(AFX_BRIDGEDESCLIFTINGPAGE_H__4F85C8F1_DD47_11D2_AD34_00105A9AF985__INCLUDED_)
#define AFX_BRIDGEDESCLIFTINGPAGE_H__4F85C8F1_DD47_11D2_AD34_00105A9AF985__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// BridgeDescLiftingPage.h : header file
//

#include "PGSuperAppPlugin\resource.h"
#include <MfcTools\MetaFileStatic.h>

class CGirderDescDlg;

/////////////////////////////////////////////////////////////////////////////
// CGirderDescLiftingPage dialog

class CGirderDescLiftingPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CGirderDescLiftingPage)

// Construction
public:
	CGirderDescLiftingPage();
	~CGirderDescLiftingPage();

   // need pointer and can't call GetParent
   void ConstructEx( UINT nIDTemplate, CGirderDescDlg* pParent, UINT nIDCaption = 0 );

// Dialog Data
	//{{AFX_DATA(CGirderDescLiftingPage)
	enum { IDD = IDD_GIRDERDESC_LIFTING_PAGE };
	//}}AFX_DATA

   CGirderDescDlg* m_pParent;

   Float64 m_LiftingLocation;
   Float64 m_LeadingOverhang;
   Float64 m_TrailingOverhang;

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CGirderDescLiftingPage)
	public:
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CGirderDescLiftingPage)
		// NOTE: the ClassWizard will add member functions here
   afx_msg void OnHelp();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
   CMetaFileStatic m_Picture;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_BRIDGEDESCLIFTINGPAGE_H__4F85C8F1_DD47_11D2_AD34_00105A9AF985__INCLUDED_)
