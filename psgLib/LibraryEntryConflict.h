///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2012  Washington State Department of Transportation
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

#if !defined(AFX_LIBRARYENTRYCONFLICT_H__7E7E6C58_213D_11D3_AD79_00105A9AF985__INCLUDED_)
#define AFX_LIBRARYENTRYCONFLICT_H__7E7E6C58_213D_11D3_AD79_00105A9AF985__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// LibraryEntryConflict.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CLibraryEntryConflict dialog

class CLibraryEntryConflict : public CDialog
{
// Construction
public:
   CLibraryEntryConflict(const std::_tstring& entryName, const std::_tstring& libName, const std::vector<std::_tstring>& keylists, bool isImported, CWnd* pParent = NULL);

// Dialog Data
	//{{AFX_DATA(CLibraryEntryConflict)
	enum { IDD = IDD_LIBRARY_ENTRY_CONFLICT };
	CButton	m_Overwrite;
	CStatic	m_ConflictBottom;
	CStatic	m_ConflictTop;
	CStatic	m_EntryText;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CLibraryEntryConflict)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CLibraryEntryConflict)
	afx_msg void OnRenameEntry();
	afx_msg void OnOverwrite();
	virtual BOOL OnInitDialog();
   afx_msg void OnHelp();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
   enum OutCome {Rename, OverWrite};
   OutCome m_OutCome;
   CString m_EntryName;
   CString m_LibName;
   CString m_NewName;
   const std::vector<std::_tstring>& m_KeyList;
   bool m_IsImported; // are libraries coming from import or project?
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LIBRARYENTRYCONFLICT_H__7E7E6C58_213D_11D3_AD79_00105A9AF985__INCLUDED_)
