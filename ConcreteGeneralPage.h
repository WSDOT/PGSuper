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

#pragma once

// ConcreteGeneralPage.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CConcreteGeneralPage dialog

class CConcreteGeneralPage : public CPropertyPage
{
// Construction
public:
	CConcreteGeneralPage();

// Dialog Data
	//{{AFX_DATA(CConcreteGeneralPage)
	enum { IDD = IDD_CONCRETE_DETAILS };
	CStatic	m_ctrlStrengthDensityUnit;
	CStatic	m_ctrlStrengthDensityTitle;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CConcreteGeneralPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL
	CEdit	m_ctrlEc;
	CButton m_ctrlEcCheck;
	CEdit	m_ctrlFc;
	CEdit	m_ctrlStrengthDensity;

   CComPtr<IBroker> m_pBroker;
   Float64 m_MinNWCDensity;
   Float64 m_MaxLWCDensity;
   bool m_bErrorInDDX;

   CString m_strFct;

public:
   CString m_strUserEc;
   Float64 m_Ds;
   Float64 m_Dw;
   Float64 m_AggSize;
   pgsTypes::ConcreteType m_Type;

   // Implementation
protected:
   void UpdateEc();
   void ShowStrengthDensity(bool enable);

	// Generated message map functions
	//{{AFX_MSG(CConcreteGeneralPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnHelp();
   afx_msg void OnUserEc();
	afx_msg void OnChangeFc();
	afx_msg void OnChangeDs();
	afx_msg void OnCopyMaterial();
   afx_msg void OnConcreteType();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
   afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);

protected:
   virtual void OnOK();
   pgsTypes::ConcreteType GetConreteType();
   bool IsDensityInRange(Float64 density,pgsTypes::ConcreteType type);
};
