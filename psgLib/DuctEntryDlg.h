///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2019  Washington State Department of Transportation
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
#include <psgLib\DuctLibraryEntry.h>
#include <MfcTools\MetaFileStatic.h>

/////////////////////////////////////////////////////////////////////////////
// CDuctEntryDlg dialog

class CDuctEntryDlg : public CDialog
{
// Construction
public:
	CDuctEntryDlg(bool allowEditing, CWnd* pParent = nullptr);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CDuctEntryDlg)
	enum { IDD = IDD_DUCT_ENTRY };
	CString	m_Name;
	//}}AFX_DATA
   Float64 m_OD;
   Float64 m_ID;
   Float64 m_Z;

   bool m_bAllowEditing;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDuctEntryDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
   CMetaFileStatic m_DuctPicture;
   CString m_strTip;

	// Generated message map functions
	//{{AFX_MSG(CDuctEntryDlg)
   afx_msg void OnHelp();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
   BOOL OnToolTipNotify(UINT id,NMHDR* pNMHDR, LRESULT* pResult);
};
