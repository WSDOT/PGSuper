///////////////////////////////////////////////////////////////////////
// Library Editor - Editor for WBFL Library Services
// Copyright (C) 1999  Washington State Department of Transportation
//                     Bridge and Structures Office
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

#pragma once

// MyChildFrame.h : header file
//
#include "LibraryEditorChildFrm.h"

/////////////////////////////////////////////////////////////////////////////
// CMyChildFrame frame

class CMyChildFrame : public CLibraryEditorChildFrame
{
	DECLARE_DYNCREATE(CMyChildFrame)
protected:
	CMyChildFrame();           // protected constructor used by dynamic creation

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMyChildFrame)
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CMyChildFrame();

	// Generated message map functions
	//{{AFX_MSG(CMyChildFrame)
	afx_msg void OnAddEntry();
	afx_msg void OnDeleteEntry();
	afx_msg void OnUpdateDeleteEntry(CCmdUI* pCmdUI);
	afx_msg void OnEditEntry();
	afx_msg void OnUpdateEditEntry(CCmdUI* pCmdUI);
	afx_msg void OnDuplicateEntry();
	afx_msg void OnUpdateDuplicateEntry(CCmdUI* pCmdUI);
	afx_msg void OnSi();
	afx_msg void OnUs();
	afx_msg void OnUpdateSi(CCmdUI* pCmdUI);
	afx_msg void OnUpdateUs(CCmdUI* pCmdUI);
	afx_msg void OnSmallIcons();
	afx_msg void OnLargeIcons();
	afx_msg void OnListIcons();
	afx_msg void OnArrangeIcons();
	afx_msg void OnUpdateLargeIcons(CCmdUI* pCmdUI);
	afx_msg void OnUpdateListIcons(CCmdUI* pCmdUI);
	afx_msg void OnUpdateSmallIcons(CCmdUI* pCmdUI);
	afx_msg void OnUpdateAddEntry(CCmdUI* pCmdUI);
	afx_msg void OnUpdateRenameEntry(CCmdUI* pCmdUI);
	afx_msg void OnRenameEntry();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.
