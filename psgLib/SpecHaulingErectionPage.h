///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2021  Washington State Department of Transportation
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

#if !defined(AFX_SPECHAULINGERECTIONPAGE_H__22F9FD45_4F00_11D2_9D5F_00609710E6CE__INCLUDED_)
#define AFX_SPECHAULINGERECTIONPAGE_H__22F9FD45_4F00_11D2_9D5F_00609710E6CE__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// SpecHaulingErectionPage.h : header file
//
#include "resource.h"
#include "WsdotHaulingDlg.h"
#include "KdotHaulingDlg.h"

/////////////////////////////////////////////////////////////////////////////
// CSpecHaulingErectionPage dialog

class CSpecHaulingErectionPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CSpecHaulingErectionPage)

// Construction
public:
	CSpecHaulingErectionPage();
	~CSpecHaulingErectionPage();

// Dialog Data
	//{{AFX_DATA(CSpecHaulingErectionPage)
	enum { IDD = IDD_SPEC_HAULING_ERECTIOND };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CSpecHaulingErectionPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
public:
   pgsTypes::HaulingAnalysisMethod m_HaulingAnalysisMethod;

public:
   // Embedded dialogs
   CWsdotHaulingDlg m_WsdotHaulingDlg;
   CKdotHaulingDlg  m_KdotHaulingDlg;

   bool m_BeforeInit; // true 'til after oninitdialog

protected:
	// Generated message map functions
	//{{AFX_MSG(CSpecHaulingErectionPage)
	//}}AFX_MSG
   afx_msg void OnHelp();
	DECLARE_MESSAGE_MAP()

   virtual BOOL OnInitDialog();

public:
   afx_msg void OnCbnSelchangeHaulingMethod();

   void SwapDialogs();
   virtual BOOL OnSetActive();

   void EnableControls(BOOL bEnable);
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SPECHAULINGERECTIONPAGE_H__22F9FD45_4F00_11D2_9D5F_00609710E6CE__INCLUDED_)
