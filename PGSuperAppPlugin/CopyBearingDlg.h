///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2025  Washington State Department of Transportation
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


#include "resource.h"
#include <vector>
#include "MultiBearingSelectDlg.h"
#include <Reporting\CopyBearingPropertiesReportSpecification.h>
#include <PgsExt\Selection.h>

/////////////////////////////////////////////////////////////////////////////
// CCopyBearingDlg dialog

class CCopyBearingDlg : public CDialog
{
// Construction
public:
	CCopyBearingDlg(std::shared_ptr<WBFL::EAF::Broker> pBroker, CWnd* pParent = nullptr);


	enum { IDD = IDD_COPY_BEARING_PROPERTIES };
   
   CComboBox m_FromGroup;
   CComboBox m_FromGirder;
   CComboBox m_FromBearing;
   CComboBox m_ToGroup;
   CComboBox m_ToGirder;
   CComboBox m_ToBearing;


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCopyGirderDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

   // return selected reaction location to copy properties from
   ReactionLocation GetFromReactionLocation();

public:

   // return a list of girders to be copied to
   std::vector<ReactionLocation> GetToReactionLocations();

   ReactionLocation m_FromReactionLocation;
   std::vector<ReactionLocation> m_ToReactionLocations;

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
   afx_msg void OnToReactionLocationChanged();
   afx_msg void OnFromReactionLocationChanged();
   afx_msg void OnBnClickedRadio();
   afx_msg void OnBnClickedSelectReactionLocations();
   afx_msg void OnEdit();
	afx_msg void OnPrint();
   afx_msg void OnCmenuSelected(UINT id);
   afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

   BOOL OnToolTipNotify(UINT id,NMHDR* pNMHDR, LRESULT* pResult);

   void OnFromGroupChangedNoUpdate();
   void OnToGroupChangedNoUpdate();

   void FillComboBoxes(CComboBox& cbGroup,CComboBox& cbGirder, CComboBox& cbReactionLocation, 
	   bool bIncludeAllGroups, bool bIncludeAllGirders, bool bIncludeAllReactionLocations);
   void FillGirderComboBox(CComboBox& cbGirder, GroupIndexType grpIdx, bool bIncludeAllGirders);
   void FillReactionLocationComboBox(CComboBox& cbReactionLocation, GirderIndexType gdrIdx,
	   GroupIndexType grpIdx,bool bIncludeAllReactionLocations);

   void UpdateReportData();
   void UpdateReport();


   std::map<int,ReactionLocation> m_FromListIndicies;

private:
	std::shared_ptr<WBFL::EAF::Broker> m_pBroker;

   std::shared_ptr<CCopyBearingPropertiesReportSpecification> m_pRptSpec;
   std::shared_ptr<WBFL::Reporting::ReportBrowser> m_pBrowser; // this is the actual browser window that displays the report

   // map from multi-select dialog
   std::vector<ReactionLocation> m_MultiDialogSelections;

   std::vector<ReactionLocation> m_RLmenuItems;

   int m_cxMin;
   int m_cyMin;

   bool m_bIsPGSplice;
   CString m_strTip;

protected:
   virtual void OnOK();
   void EnableCopyNow();
   void CleanUp();

   virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);

public:
};

