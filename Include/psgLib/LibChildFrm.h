///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2025  Washington State Department of Transportation
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

#if !defined(AFX_MYCHILDFRAME_H__A39C4813_27B6_11D2_9D3A_00609710E6CE__INCLUDED_)
#define AFX_MYCHILDFRAME_H__A39C4813_27B6_11D2_9D3A_00609710E6CE__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// MyChildFrame.h : header file
//
#include "PsgLibLib.h"

#include <PsgLib\LibraryEditorChildFrm.h>

/////////////////////////////////////////////////////////////////////////////
// CLibChildFrame frame

class PSGLIBCLASS CLibChildFrame : public CLibraryEditorChildFrame
{
	DECLARE_DYNCREATE(CLibChildFrame)
protected:
	CLibChildFrame();           // protected constructor used by dynamic creation

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CLibChildFrame)
	virtual BOOL Create(LPCTSTR lpszClassName,
				LPCTSTR lpszWindowName,
				DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_OVERLAPPEDWINDOW,
				const RECT& rect = rectDefault,
				CMDIFrameWnd* pParentWnd = nullptr,
				CCreateContext* pContext = nullptr);
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CLibChildFrame();

	// Generated message map functions
	//{{AFX_MSG(CLibChildFrame)
	afx_msg void OnAddEntry();
	afx_msg void OnUpdateAddEntry(CCmdUI* pCmdUI);
	afx_msg void OnDeleteEntry();
	afx_msg void OnUpdateDeleteEntry(CCmdUI* pCmdUI);
	afx_msg void OnEditEntry();
	afx_msg void OnUpdateEditEntry(CCmdUI* pCmdUI);
	afx_msg void OnDuplicateEntry();
	afx_msg void OnUpdateDuplicateEntry(CCmdUI* pCmdUI);
	afx_msg void OnSmallIcons();
	afx_msg void OnLargeIcons();
	afx_msg void OnListIcons();
	afx_msg void OnArrangeIcons();
	afx_msg void OnUpdateLargeIcons(CCmdUI* pCmdUI);
	afx_msg void OnUpdateListIcons(CCmdUI* pCmdUI);
	afx_msg void OnUpdateSmallIcons(CCmdUI* pCmdUI);
	afx_msg void OnRenameEntry();
	afx_msg void OnUpdateRenameEntry(CCmdUI* pCmdUI);
	//}}AFX_MSG
   afx_msg void OnHelp();
	DECLARE_MESSAGE_MAP()

protected:
   void OnUpdateFrameTitle(BOOL bAddToTitle);

};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MYCHILDFRAME_H__A39C4813_27B6_11D2_9D3A_00609710E6CE__INCLUDED_)
