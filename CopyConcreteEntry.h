///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2013  Washington State Department of Transportation
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

#if !defined(AFX_COPYCONCRETEENTRY_H__3D57C105_BFE5_443E_8B92_3604850FEC35__INCLUDED_)
#define AFX_COPYCONCRETEENTRY_H__3D57C105_BFE5_443E_8B92_3604850FEC35__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// CopyConcreteEntry.h : header file
//

#include "PGSuperAppPlugin\resource.h"
#include <vector>

/////////////////////////////////////////////////////////////////////////////
// CCopyConcreteEntry dialog

class CCopyConcreteEntry : public CDialog
{
// Construction
public:
	CCopyConcreteEntry(bool isPrestressed, CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CCopyConcreteEntry)
	enum { IDD = IDD_COPY_CONC_ENTRY };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA
	CString	m_Concrete;
   bool m_IsPrestressed;
   const ConcreteLibraryEntry* m_ConcreteEntry;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCopyConcreteEntry)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CCopyConcreteEntry)
	virtual BOOL OnInitDialog();
	afx_msg void OnHelp();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_COPYCONCRETEENTRY_H__3D57C105_BFE5_443E_8B92_3604850FEC35__INCLUDED_)
