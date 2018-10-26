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

#if !defined(AFX_SELECTITEMDLG_H__A65830E3_36AB_4B60_9FA5_45A145B5C582__INCLUDED_)
#define AFX_SELECTITEMDLG_H__A65830E3_36AB_4B60_9FA5_45A145B5C582__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SelectItemDlg.h : header file
//

#include "PGSuperAppPlugin\resource.h"

/////////////////////////////////////////////////////////////////////////////
// CSelectItemDlg dialog

class CSelectItemDlg : public CDialog
{
// Construction
public:
	CSelectItemDlg(CWnd* pParent = NULL);   // standard constructor

private:
// Dialog Data
	//{{AFX_DATA(CSelectItemDlg)
	enum { IDD = IDD_SELECT_ITEM };
	CComboBox	m_cbList;
	CStatic	m_Label;
	//}}AFX_DATA

public:
	int		m_ItemIdx;
   CString m_strTitle; // text for the title bar
   CString m_strItems; // \n delimited list of items
   CString m_strLabel; // descriptive label

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSelectItemDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CSelectItemDlg)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SELECTITEMDLG_H__A65830E3_36AB_4B60_9FA5_45A145B5C582__INCLUDED_)
