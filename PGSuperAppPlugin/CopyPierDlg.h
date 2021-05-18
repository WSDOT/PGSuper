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

#if !defined(AFX_CopyPierDLG_H__F07E0F0D_33D6_11D3_AD91_00105A9AF985__INCLUDED_)
#define AFX_CopyPierDLG_H__F07E0F0D_33D6_11D3_AD91_00105A9AF985__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// CopyPierDlg.h : header file
//

#include "resource.h"
#include <vector>
#include <Reporting\CopyPierPropertiesReportSpecification.h>
#include <MfcTools\CheckListBoxEx.h>

/////////////////////////////////////////////////////////////////////////////
// CCopyPierDlg dialog

class CCopyPierDlg : public CDialog
{
// Construction
public:
	CCopyPierDlg(IBroker* pBroker, const std::map<IDType,ICopyPierPropertiesCallback*>&  pCopyPierPropertiesCallbacks, IDType selectedID, CWnd* pParent = nullptr);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CCopyPierDlg)
	enum { IDD = IDD_COPY_PIER_PROPERTIES };
   
   CComboBox m_FromPier;
   CComboBox m_ToPier;
   CCheckListBoxEx	m_SelectedPropertyTypesCL;
	//}}AFX_DATA

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCopyPierDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL


   // return selected Pier to copy properties from
   PierIndexType GetFromPier();

   // return a list of Piers to be copied to
   std::vector<PierIndexType> GetToPiers();

   PierIndexType m_FromPierIdx;
   std::vector<PierIndexType> m_ToPiers;

// Implementation
protected:
   CSelection m_FromSelection;

	// Generated message map functions
	//{{AFX_MSG(CCopyPierDlg)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	virtual BOOL OnInitDialog();
   afx_msg void OnFromPierChanged();
   afx_msg void OnToPierChanged();
   afx_msg void OnHelp();
   afx_msg void OnCopyItemStateChanged();
   afx_msg void OnEdit();
	afx_msg void OnPrint();
   afx_msg void OnCmenuSelected(UINT id);
   afx_msg void OnDestroy();
   afx_msg void OnLbnSelchangePropertyList();
   afx_msg void OnLbnChkchangePropertyList();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

   BOOL OnToolTipNotify(UINT id,NMHDR* pNMHDR, LRESULT* pResult);

   void OnFromPierChangedNoUpdate();

   IBroker* m_pBroker;
   const std::map<IDType, ICopyPierPropertiesCallback*> m_CopyPierPropertiesCallbacks;
   std::set<IDType> m_SelectedIDs; 

   void FillComboBoxes(CComboBox& cbPier, bool bIncludeAllPiers, PierIndexType fromIdx=INVALID_INDEX);

   void UpdateReportData();
   void UpdateReport();

   void UpdateSelectedPropertyList();
   void InitSelectedPropertyList();

   std::vector<ICopyPierPropertiesCallback*> GetSelectedCopyPierPropertiesCallbacks();

private:
   std::shared_ptr<CCopyPierPropertiesReportSpecification> m_pRptSpec;
   std::shared_ptr<CReportBrowser> m_pBrowser; // this is the actual browser window that displays the report

   int m_cxMin;
   int m_cyMin;

   CString m_strTip;

protected:
   virtual void OnOK();
   void EnableCopyNow();
   void CleanUp();
   int IsAllSelectedInList(); // return checklist index where All Properties is found

   virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CopyPierDLG_H__F07E0F0D_33D6_11D3_AD91_00105A9AF985__INCLUDED_)
