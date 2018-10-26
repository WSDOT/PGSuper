///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
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

#if !defined(AFX_CONCRETEENTRYDLG_H__B76E1313_26EE_11D2_9D39_00609710E6CE__INCLUDED_)
#define AFX_CONCRETEENTRYDLG_H__B76E1313_26EE_11D2_9D39_00609710E6CE__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// ConcreteEntryDlg.h : header file
//

#if !defined INCLUDED_LIBRARYFW_UNITSMODE_H_
#include <LibraryFw\UnitsMode.h>
#endif

/////////////////////////////////////////////////////////////////////////////
// CConcreteEntryDlg dialog

class CConcreteEntryDlg : public CDialog
{
// Construction
public:
	CConcreteEntryDlg(libUnitsMode::Mode mode, bool allowEditing, CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CConcreteEntryDlg)
	enum { IDD = IDD_CONCRETE_ENTRY };
	CString	m_EntryName;
	//}}AFX_DATA
   double m_Fc;
   bool m_bUserEc;
   double m_Ec;
   double m_Ds;
   double m_Dw;
   double m_AggSize;
   double m_K1;
   libUnitsMode::Mode m_Mode;
   bool m_AllowEditing;

   CString m_InitialEc;


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CConcreteEntryDlg)
	public:
//	virtual void WinHelp(DWORD dwData, UINT nCmd = HELP_CONTEXT);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CConcreteEntryDlg)
   afx_msg LRESULT OnCommandHelp(WPARAM, LPARAM lParam);
	virtual BOOL OnInitDialog();
   afx_msg void OnModE();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CONCRETEENTRYDLG_H__B76E1313_26EE_11D2_9D39_00609710E6CE__INCLUDED_)
