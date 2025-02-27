///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright � 1999-2025  Washington State Department of Transportation
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

#if !defined(AFX_SELECTGIRDERDLG_H__90D479C0_EC98_4BC6_9A41_6DC8A9D56FE0__INCLUDED_)
#define AFX_SELECTGIRDERDLG_H__90D479C0_EC98_4BC6_9A41_6DC8A9D56FE0__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SelectGirderDlg.h : header file
//
#include "resource.h"

/////////////////////////////////////////////////////////////////////////////
// CSelectGirderDlg dialog

class CSelectGirderDlg : public CDialog
{
// Construction
public:
	CSelectGirderDlg(	IBroker* pBroker, CWnd* pParent = nullptr);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CSelectGirderDlg)
	enum { IDD = IDD_SELECT_GIRDER };
		// NOTE: the ClassWizard will add data members here
	GroupIndexType	m_Group;
	GirderIndexType m_Girder;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSelectGirderDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX) override;    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CSelectGirderDlg)
	virtual BOOL OnInitDialog() override;
   void OnGroupChanged();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	IBroker* m_pBroker;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SELECTGIRDERDLG_H__90D479C0_EC98_4BC6_9A41_6DC8A9D56FE0__INCLUDED_)
