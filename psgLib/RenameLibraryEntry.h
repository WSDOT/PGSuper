///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2010  Washington State Department of Transportation
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

#if !defined(AFX_RENAMELIBRARYENTRY_H__7E7E6C59_213D_11D3_AD79_00105A9AF985__INCLUDED_)
#define AFX_RENAMELIBRARYENTRY_H__7E7E6C59_213D_11D3_AD79_00105A9AF985__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// RenameLibraryEntry.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CRenameLibraryEntry dialog

class CRenameLibraryEntry : public CDialog
{
// Construction
public:
	CRenameLibraryEntry(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CRenameLibraryEntry)
	enum { IDD = IDD_RENAME_LIBRARY_ENTRY };
	CString	m_EntryName;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CRenameLibraryEntry)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CRenameLibraryEntry)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RENAMELIBRARYENTRY_H__7E7E6C59_213D_11D3_AD79_00105A9AF985__INCLUDED_)
