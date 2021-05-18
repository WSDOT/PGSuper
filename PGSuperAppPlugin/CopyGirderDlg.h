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

#if !defined(AFX_COPYGIRDERDLG_H__F07E0F0D_33D6_11D3_AD91_00105A9AF985__INCLUDED_)
#define AFX_COPYGIRDERDLG_H__F07E0F0D_33D6_11D3_AD91_00105A9AF985__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// CopyGirderDlg.h : header file
//

#include "resource.h"
#include <vector>
#include "MultiGirderSelectDlg.h"
#include <Reporting\CopyGirderPropertiesReportSpecification.h>
#include <MfcTools\CheckListBoxEx.h>

/////////////////////////////////////////////////////////////////////////////
// CCopyGirderDlg dialog

class CCopyGirderDlg : public CDialog
{
// Construction
public:
	CCopyGirderDlg(IBroker* pBroker, const std::map<IDType,ICopyGirderPropertiesCallback*>&  rcopyGirderPropertiesCallbacks, IDType selectedID, CWnd* pParent = nullptr);

// Dialog Data
	//{{AFX_DATA(CCopyGirderDlg)
	enum { IDD = IDD_COPY_GIRDER_PROPERTIES };
   
   CComboBox m_FromGroup;
   CComboBox m_FromGirder;
   CComboBox m_ToGroup;
   CComboBox m_ToGirder;
	CCheckListBoxEx	m_SelectedPropertyTypesCL;
	//}}AFX_DATA

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCopyGirderDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

   // return selected girder to copy properties from
   CGirderKey GetFromGirder();

public:

   // return a list of girders to be copied to
   std::vector<CGirderKey> GetToGirders();

   CGirderKey m_FromGirderKey;
   std::vector<CGirderKey> m_ToGirderKeys;

// Implementation
protected:
   CSelection m_FromSelection;

	// Generated message map functions
	//{{AFX_MSG(CCopyGirderDlg)
	afx_msg void OnSize(UINT nType, int cx, int cy);
   virtual BOOL OnInitDialog();
   afx_msg void OnFromGroupChanged();
   afx_msg void OnToGroupChanged();
   afx_msg void OnToGirderChanged();
   afx_msg void OnFromGirderChanged();
   afx_msg void OnHelp();
   afx_msg void OnBnClickedRadio();
   afx_msg void OnBnClickedSelectGirders();
   afx_msg void OnEdit();
	afx_msg void OnPrint();
   afx_msg void OnCmenuSelected(UINT id);
   afx_msg void OnDestroy();
//   afx_msg void OnLbnSelchangePropertyList();
   afx_msg void OnLbnChkchangePropertyList();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

   BOOL OnToolTipNotify(UINT id,NMHDR* pNMHDR, LRESULT* pResult);

   void OnFromGroupChangedNoUpdate();
   void OnToGroupChangedNoUpdate();

   void FillComboBoxes(CComboBox& cbGroup,CComboBox& cbGirder, bool bIncludeAllGroups, bool bIncludeAllGirders);
   void FillGirderComboBox(CComboBox& cbGirder,GroupIndexType grpIdx,bool bIncludeAllGirders);

   void UpdateReportData();
   void UpdateReport();

   void UpdateSelectedPropertyList();
   void InitSelectedPropertyList();

   std::vector<ICopyGirderPropertiesCallback*> GetSelectedCopyGirderPropertiesCallbacks();

   std::map<int,CGirderKey> m_FromListIndicies;

private:
   IBroker* m_pBroker;
   const std::map<IDType, ICopyGirderPropertiesCallback*> m_CopyGirderPropertiesCallbacks;
   std::set<IDType> m_SelectedIDs; 

   std::shared_ptr<CCopyGirderPropertiesReportSpecification> m_pRptSpec;
   std::shared_ptr<CReportBrowser> m_pBrowser; // this is the actual browser window that displays the report

   // map from multi-select dialog
   std::vector<CGirderKey> m_MultiDialogSelections;

   int m_cxMin;
   int m_cyMin;

   bool m_bIsPGSplice;
   CString m_strTip;

protected:
   virtual void OnOK();
   void EnableCopyNow();
   void CleanUp();
   int IsAllSelectedInList(); // return checklist index where All Properties is found

   virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);

public:
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_COPYGIRDERDLG_H__F07E0F0D_33D6_11D3_AD91_00105A9AF985__INCLUDED_)
