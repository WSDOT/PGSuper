///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2024  Washington State Department of Transportation
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


class CSpanGirderReportDlg : public CDialog
{
	DECLARE_DYNAMIC(CSpanGirderReportDlg)

public:
   enum class Mode
   {
      ChaptersOnly = 0,
      GroupAndChapters = 1,
      GirderAndChapters = 2,
      GroupGirderAndChapters = 3,
      GroupGirderSegmentAndChapters = 4
   };
   
   CSpanGirderReportDlg(IBroker* pBroker,const WBFL::Reporting::ReportDescription& rptDesc,Mode mode,std::shared_ptr<WBFL::Reporting::ReportSpecification> pRptSpec,UINT nIDTemplate = IDD_SPANGIRDERREPORT,CWnd* pParent = nullptr);   // standard constructor
	virtual ~CSpanGirderReportDlg();

// Dialog Data
	enum { IDD = IDD_SPANGIRDERREPORT };

   CSegmentKey m_SegmentKey;
   std::vector<std::_tstring> m_ChapterList;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

   virtual void UpdateChapterList();
   virtual void UpdateGirderComboBox();
   virtual void UpdateSegmentComboBox();

   virtual void ClearChapterCheckMarks(BOOL bClear=TRUE);
   virtual void InitChapterListFromSpec();
   virtual void InitFromRptSpec();

   CCheckListBox	m_ChList;

   const WBFL::Reporting::ReportDescription& m_RptDesc;
   IBroker* m_pBroker;
   Mode m_Mode;

   std::shared_ptr<WBFL::Reporting::ReportSpecification> m_pInitRptSpec; // report spec for initializing the dialog

public:
	// Generated message map functions
	//{{AFX_MSG(CReportDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnHelp();
	afx_msg void OnGroupChanged();
   afx_msg void OnGirderChanged();
   afx_msg void OnSelectAll();
   afx_msg void OnDeselectAll();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};
