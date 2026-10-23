///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2026  Washington State Department of Transportation
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

#if !defined(AFX_PIERLAYOUTPAGE_H__AA2956CC_2682_44A6_B7FF_6362E40C44DF__INCLUDED_)
#define AFX_PIERLAYOUTPAGE_H__AA2956CC_2682_44A6_B7FF_6362E40C44DF__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// PierLayoutPage.h : header file
//

#include "resource.h"
#include <PsgLib\PierData2.h>
#include "CommonPierLayoutDlg.h"
#include "ScallopedPierLayoutDlg.h"
#include "UserDefinedPierLayoutDlg.h"
#include <WBFLGenericBridge.h>

/////////////////////////////////////////////////////////////////////////////
// CPierLayoutPage dialog
class CPierLayoutPage : public CPropertyPage
{
	friend class CCommonPierLayoutDlg;
	friend class CScallopedPierLayoutDlg;
	friend class CUserDefinedPierLayoutDlg;
	friend class CPierDetailsDlg;
	friend class CPierConnectionsPage;

	DECLARE_DYNCREATE(CPierLayoutPage)

// Construction
public:
	CPierLayoutPage();
	~CPierLayoutPage();

// Dialog Data
	//{{AFX_DATA(CPierLayoutPage)
	enum { IDD = IDD_PIER_LAYOUT };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA

   void Init(CPierData2* pPier);
// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CPierLayoutPage)
	public:
	protected:
	virtual void DoDataExchange(CDataExchange* pDX) override;    // DDX/DDV support
	//}}AFX_VIRTUAL
	CEdit	m_ctrlEc;
	CButton m_ctrlEcCheck;
	CEdit	m_ctrlFc;
   CString m_strUserEc;

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CPierLayoutPage)
	virtual BOOL OnInitDialog() override;
   afx_msg void OnHelp();
   afx_msg void OnChangeFc();
   afx_msg void OnUserEc();
   afx_msg void OnMoreProperties();
   afx_msg void OnPierModelTypeChanged();
   afx_msg void OnPierLayoutTypeChanged();
   afx_msg void OnShowLiveClicked();
   afx_msg LRESULT OnPierLayoutChanged(WPARAM wParam, LPARAM lParam);

   void RefreshPierLayoutPopout();

   void SwapDialogs();

	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

   void FillPierModelTypeComboBox();
   void FillPierLayoutTypeComboBox();

   CDrawPierLayoutControl* m_pPierLayoutPopout = nullptr;

   void ShowPierLayoutPopout();

   CPierData2* m_pPier;
   PierIndexType m_PierIdx;

   pgsTypes::PierModelType m_PierModelType;
   pgsTypes::PierLayoutType m_PierLayoutType;

   // Embedded dialogs
   CCommonPierLayoutDlg m_CommonPierLayoutDlg;
   CScallopedPierLayoutDlg m_ScallopedPierLayoutDlg;
   CUserDefinedPierLayoutDlg m_UserDefinedPierLayoutDlg;

   bool CommitCommonPierLayout();
   bool CommitScallopedPierLayout();
   bool CommitUserDefinedPierLayout();

   BOOL OnKillActive();

   BOOL OnApply();

   void UpdateConcreteTypeLabel();
   void UpdateEc();

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PIERLAYOUTPAGE_H__AA2956CC_2682_44A6_B7FF_6362E40C44DF__INCLUDED_)
