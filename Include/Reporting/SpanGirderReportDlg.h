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

#pragma once

#include <ReportManager\ReportManager.h>
#include "resource.h"

// CSpanGirderReportDlg dialog

enum RptDialogMode
{
   ChaptersOnly = 0,
   SpanAndChapters = 1,
   GirderAndChapters = 2,
   SpanGirderAndChapters = 3
};

class CSpanGirderReportDlg : public CDialog
{
	DECLARE_DYNAMIC(CSpanGirderReportDlg)

public:
	CSpanGirderReportDlg(IBroker* pBroker,const CReportDescription& rptDesc,RptDialogMode mode,std::shared_ptr<CReportSpecification>& pRptSpec,UINT nIDTemplate = IDD_SPANGIRDERREPORT,CWnd* pParent = nullptr);   // standard constructor
	virtual ~CSpanGirderReportDlg();

// Dialog Data
	enum { IDD = IDD_SPANGIRDERREPORT };

   GroupIndexType m_Group;
   GirderIndexType m_Girder;
   std::vector<std::_tstring> m_ChapterList;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

   virtual void UpdateChapterList();
   virtual void UpdateGirderComboBox(GroupIndexType grpIdx);

   virtual void ClearChapterCheckMarks();
   virtual void InitChapterListFromSpec();
   virtual void InitFromRptSpec();

   CCheckListBox	m_ChList;

   const CReportDescription& m_RptDesc;
   IBroker* m_pBroker;
   RptDialogMode m_Mode;

   std::shared_ptr<CReportSpecification> m_pInitRptSpec; // report spec for initializing the dialog

public:
	// Generated message map functions
	//{{AFX_MSG(CReportDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnHelp();
	afx_msg void OnGroupChanged();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};
