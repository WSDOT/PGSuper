///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2020  Washington State Department of Transportation
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

#if !defined(AFX_DEALWITHLOADDLG_H__C423F6B1_0211_4298_8555_B26D8D399313__INCLUDED_)
#define AFX_DEALWITHLOADDLG_H__C423F6B1_0211_4298_8555_B26D8D399313__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DealWithLoadDlg.h : header file
//

#include "resource.h"

/////////////////////////////////////////////////////////////////////////////
// CDealWithLoadDlg dialog

class CDealWithLoadDlg : public CDialog
{
// Construction
public:
   // define some constants to return from Domodal
   enum {IDEDITLOAD=3000, IDDELETELOAD=3001};

	CDealWithLoadDlg(CWnd* pParent = nullptr);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CDealWithLoadDlg)
	enum { IDD = IDD_DEAL_WITH_LOADS_DLG };
	CString	m_Message;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDealWithLoadDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX) override;    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDealWithLoadDlg)
	afx_msg void OnDeleteLoad();
	afx_msg void OnEditLoad();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DEALWITHLOADDLG_H__C423F6B1_0211_4298_8555_B26D8D399313__INCLUDED_)
