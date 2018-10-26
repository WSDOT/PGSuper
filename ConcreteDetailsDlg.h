///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2010  Washington State Department of Transportation
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

#if !defined(AFX_CONCRETEDETAILSDLG_H__83CAAB88_8DEA_428A_8098_5E8ECCA92AB8__INCLUDED_)
#define AFX_CONCRETEDETAILSDLG_H__83CAAB88_8DEA_428A_8098_5E8ECCA92AB8__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ConcreteDetailsDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CConcreteDetailsDlg dialog

class CConcreteDetailsDlg : public CDialog
{
// Construction
public:
	CConcreteDetailsDlg(CWnd* pParent = NULL);   // standard constructor

   // text strings to in in display units... Ec comes out in display units
   static CString UpdateEc(const CString& strFc,const CString& strDensity,const CString& strK1);

// Dialog Data
	//{{AFX_DATA(CConcreteDetailsDlg)
	enum { IDD = IDD_CONCRETE_DETAILS };
	CStatic	m_ctrlStrengthDensityUnit;
	CStatic	m_ctrlStrengthDensityTitle;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CConcreteDetailsDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL
	CEdit	m_ctrlK1;
	CEdit	m_ctrlEc;
	CButton m_ctrlEcCheck;
	CEdit	m_ctrlFc;
	CEdit	m_ctrlStrengthDensity;

   CComPtr<IBroker> m_pBroker;
   Float64 m_MinNWCDensity;
   bool m_bIsStrengthNWC;
   bool m_bIsDensityNWC;
   bool m_bErrorInDDX;

public:
   Float64 m_Fc;
   bool m_bUserEc;
   Float64 m_Ec;
   Float64 m_Ds;
   Float64 m_Dw;
   Float64 m_AggSize;
   Float64 m_K1;

   CString m_strUserEc;

   // Implementation
protected:
   void UpdateEc();
   void ShowStrengthDensity(bool enable);

	// Generated message map functions
	//{{AFX_MSG(CConcreteDetailsDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnHelp();
   afx_msg void OnUserEc();
	afx_msg void OnChangeFc();
	afx_msg void OnChangeDs();
	afx_msg void OnChangeDw();
	afx_msg void OnChangeK1();
	afx_msg void OnCopyMaterial();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
   afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
protected:
   virtual void OnOK();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CONCRETEDETAILSDLG_H__83CAAB88_8DEA_428A_8098_5E8ECCA92AB8__INCLUDED_)
