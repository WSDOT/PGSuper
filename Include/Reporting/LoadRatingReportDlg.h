///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2022  Washington State Department of Transportation
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

#include "SharedCTrls\MultiGirderSelectGrid.h" 

// CLoadRatingReportDlg dialog

class CLoadRatingReportDlg : public CDialog
{
	DECLARE_DYNAMIC(CLoadRatingReportDlg)

public:
	CLoadRatingReportDlg(IBroker* pBroker,const CReportDescription& rptDesc,std::shared_ptr<CReportSpecification>& pRptSpec,UINT nIDTemplate = IDD_LOADRATINGREPORT,CWnd* pParent = nullptr);   // standard constructor
	virtual ~CLoadRatingReportDlg();

// Dialog Data
	enum { IDD = IDD_LOADRATINGREPORT };

   bool m_bReportAtAllPoi;
   std::vector<std::_tstring> m_ChapterList;

   void SetGirderKey(const CGirderKey& girderKey);
   CGirderKey GetGirderKey() const;
   bool IsSingleGirderLineSelected() const;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

   virtual void UpdateChapterList();
   virtual void UpdateGirderComboBox();
   virtual void UpdateGirderLineComboBox();
   virtual void UpdateSpanComboBox();

   virtual void ClearChapterCheckMarks();
   virtual void InitChapterListFromSpec();
   virtual void InitFromRptSpec();

   CCheckListBox	m_ChList;

   const CReportDescription& m_RptDesc;
   IBroker* m_pBroker;

   std::shared_ptr<CReportSpecification> m_pInitRptSpec; // report spec for initializing the dialog

private:
   int m_RadioNum;
   GirderIndexType m_Girder;
   GirderIndexType m_GirderLine;
   SpanIndexType m_Span;

public:
	// Generated message map functions
	//{{AFX_MSG(CReportDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnHelp();
   afx_msg void OnBnClickedRadio();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
   afx_msg void OnCbnSelchangeSpan();
   afx_msg void OnDestroy(); 

private:
   void LoadSettings();
   void SaveSettings();
};
