///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2014  Washington State Department of Transportation
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

#if !defined(AFX_CONCRETEENTRYDLG_H__B76E1313_26EE_11D2_9D39_00609710E6CE__INCLUDED_)
#define AFX_CONCRETEENTRYDLG_H__B76E1313_26EE_11D2_9D39_00609710E6CE__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// ConcreteEntryDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CConcreteEntryDlg dialog

class CConcreteEntryDlg : public CDialog
{
// Construction
public:
	CConcreteEntryDlg(bool allowEditing, CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CConcreteEntryDlg)
	enum { IDD = IDD_CONCRETE_ENTRY };
	CString	m_EntryName;
	//}}AFX_DATA
   Float64 m_Fc;
   bool m_bUserEc;
   Float64 m_Ec;
   Float64 m_Ds;
   Float64 m_Dw;
   Float64 m_AggSize;
   Float64 m_EccK1;
   Float64 m_EccK2;
   Float64 m_CreepK1;
   Float64 m_CreepK2;
   Float64 m_ShrinkageK1;
   Float64 m_ShrinkageK2;
   pgsTypes::ConcreteType m_Type;
   bool m_bHasFct;
   Float64 m_Fct;

   bool m_AllowEditing;

   CString m_InitialEc;
   CString m_strFct;

   Float64 m_MinNWCDensity;
   Float64 m_MaxLWCDensity;
   bool m_bErrorInDDX;

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
   bool IsDensityInRange(Float64 density,pgsTypes::ConcreteType type);
   pgsTypes::ConcreteType GetConreteType();

	// Generated message map functions
	//{{AFX_MSG(CConcreteEntryDlg)
   afx_msg LRESULT OnCommandHelp(WPARAM, LPARAM lParam);
	virtual BOOL OnInitDialog();
   afx_msg void OnModE();
   afx_msg void OnAggSplittingStrengthClicked();
   afx_msg void OnConcreteType();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
   afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
protected:
   virtual void OnOK();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CONCRETEENTRYDLG_H__B76E1313_26EE_11D2_9D39_00609710E6CE__INCLUDED_)
