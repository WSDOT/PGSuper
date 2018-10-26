///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2016  Washington State Department of Transportation
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

/////////////////////////////////////////////////////////////////////////////
// CCopyGirderDlg dialog

class CCopyGirderDlg : public CDialog
{
// Construction
public:
	CCopyGirderDlg(IBroker* pBroker, std::map<IDType,ICopyGirderPropertiesCallback*>& rCopyGirderPropertiesCallbacks, CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CCopyGirderDlg)
	enum { IDD = IDD_COPY_GIRDER_PROPERTIES };
   
   CComboBox m_FromGroup;
   CComboBox m_FromGirder;
   CComboBox m_ToGroup;
   CComboBox m_ToGirder;

   CCheckListBox m_PropertiesList;
   std::vector<IDType> GetCallbackIDs();

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
   std::vector<IDType> m_CallbackIDs;

   CSelection m_FromSelection;

	// Generated message map functions
	//{{AFX_MSG(CCopyGirderDlg)
	virtual BOOL OnInitDialog();
   afx_msg void OnFromGroupChanged();
   afx_msg void OnToGroupChanged();
   afx_msg void OnToGirderChanged();
   afx_msg void OnFromGirderChanged();
   afx_msg void OnHelp();
   afx_msg void OnBnClickedRadio();
   afx_msg void OnBnClickedSelectGirders();
   afx_msg void OnCopyItemStateChanged();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

   void CopyToSelectionChanged();
   IBroker* m_pBroker;
   std::map<IDType,ICopyGirderPropertiesCallback*>& m_rCopyGirderPropertiesCallbacks;

   void FillComboBoxes(CComboBox& cbGroup,CComboBox& cbGirder, bool bIncludeAllGroups, bool bIncludeAllGirders);
   void FillGirderComboBox(CComboBox& cbGirder,GroupIndexType grpIdx,bool bIncludeAllGirders);

   std::map<int,CGirderKey> m_FromListIndicies;

private:
   // map from multi-select dialog
   std::vector<CGirderKey> m_MultiDialogSelections;
protected:
   virtual void OnOK();
   void EnableCopyNow(BOOL bEnable);
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_COPYGIRDERDLG_H__F07E0F0D_33D6_11D3_AD91_00105A9AF985__INCLUDED_)
