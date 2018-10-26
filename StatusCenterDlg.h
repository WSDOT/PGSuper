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

#if !defined(AFX_STATUSCENTERDLG_H__A88CCCB1_9426_4004_8371_9798C98A5570__INCLUDED_)
#define AFX_STATUSCENTERDLG_H__A88CCCB1_9426_4004_8371_9798C98A5570__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// StatusCenterDlg.h : header file
//

#include "Resource.h"
#include "StatusCenterImp.h"

/////////////////////////////////////////////////////////////////////////////
// CStatusCenterDlg dialog

class CStatusCenterDlg : public CDialog, public iStatusCenterEventSink
{
// Construction
public:
	CStatusCenterDlg(pgsStatusCenter& statusCenter);
	~CStatusCenterDlg();

// Dialog Data
	//{{AFX_DATA(CStatusCenterDlg)
	enum { IDD = IDD_STATUSCENTER };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

   void OnStatusItemAdded(pgsStatusItem* pNewItem);
   void OnStatusItemRemoved(long id);


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CStatusCenterDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL
   afx_msg void OnDoubleClick(NMHDR* pNotifyStruct,LRESULT* pResult);
   afx_msg LRESULT OnCommandHelp(WPARAM, LPARAM lParam);

// Implementation
protected:
   pgsStatusCenter& m_StatusCenter;

	// Generated message map functions
	//{{AFX_MSG(CStatusCenterDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnClose();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STATUSCENTERDLG_H__A88CCCB1_9426_4004_8371_9798C98A5570__INCLUDED_)
