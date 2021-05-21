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

#if !defined(AFX_CopyTempSupportDLG_H__F07E0F0D_33D6_11D3_AD91_00105A9AF985__INCLUDED_)
#define AFX_CopyTempSupportDLG_H__F07E0F0D_33D6_11D3_AD91_00105A9AF985__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// CopyTempSupportDlg.h : header file
//

#include "resource.h"
#include <vector>
#include "MultiGirderSelectDlg.h"
#include <Reporting\CopyTempSupportPropertiesReportSpecification.h>
#include <MfcTools\CheckListBoxEx.h>

/////////////////////////////////////////////////////////////////////////////
// CCopyTempSupportDlg dialog

class CCopyTempSupportDlg : public CDialog
{
// Construction
public:
	CCopyTempSupportDlg(IBroker* pBroker, const std::map<IDType,ICopyTemporarySupportPropertiesCallback*>&  pCopyTempSupportPropertiesCallbacks, IDType selectedID, CWnd* pParent = nullptr);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CCopyTempSupportDlg)
	enum { IDD = IDD_COPY_PIER_PROPERTIES };
   
   CComboBox m_FromTempSupport;
   CComboBox m_ToTempSupport;
   CCheckListBoxEx	m_SelectedPropertyTypesCL;
	//}}AFX_DATA

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCopyTempSupportDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL


   // return selected TempSupport to copy properties from
   PierIndexType GetFromTempSupport();

   // return a list of TempSupports to be copied to
   std::vector<PierIndexType> GetToTempSupports();

   PierIndexType m_FromTempSupportIdx;
   std::vector<PierIndexType> m_ToTempSupports;

// Implementation
protected:
   CSelection m_FromSelection;

	// Generated message map functions
	//{{AFX_MSG(CCopyTempSupportDlg)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	virtual BOOL OnInitDialog();
   afx_msg void OnFromTempSupportChanged();
   afx_msg void OnToTempSupportChanged();
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

   void OnFromTempSupportChangedNoUpdate();

   IBroker* m_pBroker;
   const std::map<IDType, ICopyTemporarySupportPropertiesCallback*> m_CopyTempSupportPropertiesCallbacks;
   std::set<IDType> m_SelectedIDs; 

   void FillComboBoxes(CComboBox& cbTempSupport, bool bIncludeAllTempSupports, PierIndexType fromIdx=INVALID_INDEX);

   void UpdateReportData();
   void UpdateReport();

   void UpdateSelectedPropertyList();
   void InitSelectedPropertyList();

   std::vector<ICopyTemporarySupportPropertiesCallback*> GetSelectedCopyTempSupportPropertiesCallbacks();

private:
   std::shared_ptr<CCopyTempSupportPropertiesReportSpecification> m_pRptSpec;
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

#endif // !defined(AFX_CopyTempSupportDLG_H__F07E0F0D_33D6_11D3_AD91_00105A9AF985__INCLUDED_)
