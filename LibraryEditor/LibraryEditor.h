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

// LibraryEditor.h : main header file for the LIBRARYEDITOR application
//

#if !defined(AFX_LIBRARYEDITOR_H__340EC2F6_20E1_11D2_9D35_00609710E6CE__INCLUDED_)
#define AFX_LIBRARYEDITOR_H__340EC2F6_20E1_11D2_9D35_00609710E6CE__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols

#include <WBFLTools.h>

/////////////////////////////////////////////////////////////////////////////
// CLibraryEditorApp:
// See LibraryEditor.cpp for the implementation of this class
//

class CLibraryEditorApp : public CWinApp
{
public:
	CLibraryEditorApp();
   AcceptanceType ShowLegalNoticeAtStartup(void);
   AcceptanceType ShowLegalNotice(VARIANT_BOOL bGiveChoice = VARIANT_FALSE);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CLibraryEditorApp)
	public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	//}}AFX_VIRTUAL

// Implementation
   
   VARIANT_BOOL m_bShowLegalNotice;

	//{{AFX_MSG(CLibraryEditorApp)
	afx_msg void OnAppAbout();
	afx_msg void OnAppLegal();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LIBRARYEDITOR_H__340EC2F6_20E1_11D2_9D35_00609710E6CE__INCLUDED_)
