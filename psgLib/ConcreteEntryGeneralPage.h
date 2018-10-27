///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2018  Washington State Department of Transportation
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
#pragma once

// ConcreteEntryGeneralPage.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CConcreteEntryGeneralPage dialog

class CConcreteEntryGeneralPage : public CPropertyPage
{
// Construction
public:
	CConcreteEntryGeneralPage();

// Dialog Data
	//{{AFX_DATA(CConcreteEntryGeneralPage)
	//}}AFX_DATA
	CString	m_EntryName;
   Float64 m_Fc;
   bool m_bUserEc;
   Float64 m_Ec;
   Float64 m_Ds;
   Float64 m_Dw;
   Float64 m_AggSize;
   pgsTypes::ConcreteType m_Type;

   CString m_InitialEc;
   CString m_strFct;

   Float64 m_MinNWCDensity;
   Float64 m_MaxLWCDensity;
   bool m_bErrorInDDX;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CConcreteEntryGeneralPage)
	public:
//	virtual void WinHelp(DWORD dwData, UINT nCmd = HELP_CONTEXT);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
   bool IsDensityInRange(Float64 density,pgsTypes::ConcreteType type);
   pgsTypes::ConcreteType GetConreteType();

	// Generated message map functions
	//{{AFX_MSG(CConcreteEntryDlg)
	virtual BOOL OnInitDialog();
   afx_msg void OnModE();
   afx_msg void OnConcreteType();
   afx_msg void OnHelp();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
   afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
protected:
   virtual void OnOK();

   bool DoCopyFromLibrary();
#ifdef _DEBUG
   void AssertValid() const;
#endif
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.
