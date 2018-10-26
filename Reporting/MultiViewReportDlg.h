///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2015  Washington State Department of Transportation
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

// CMultiViewReportDlg dialog
class CMultiViewReportDlg : public CDialog
{
	DECLARE_DYNAMIC(CMultiViewReportDlg)

public:
	CMultiViewReportDlg(IBroker* pBroker,const CReportDescription& rptDesc,boost::shared_ptr<CReportSpecification>& pRptSpec,
                       SpanIndexType span, GirderIndexType girder,
                       UINT nIDTemplate = IDD_MULTIVIEWREPORT,CWnd* pParent = NULL);
	virtual ~CMultiViewReportDlg();

// Dialog Data
	enum { IDD = IDD_MULTIVIEWREPORT };

   std::vector<SpanGirderHashType> GetGirderList() const;

   std::vector<std::_tstring> m_ChapterList;

protected:
   SpanIndexType m_Span;
   GirderIndexType m_Girder;
   std::vector<SpanGirderHashType> m_GirderList;


   const CReportDescription& m_RptDesc;
   boost::shared_ptr<CReportSpecification> m_pInitRptSpec; // report spec for initializing the dialog

   IBroker* m_pBroker;

   CCheckListBox	m_ChList;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

   virtual void UpdateGirderComboBox(SpanIndexType spanIdx);

   virtual void UpdateChapterList();
   virtual void ClearChapterCheckMarks();
   virtual void InitChapterListFromSpec();
   virtual void InitFromRptSpec();

	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()
public:
   afx_msg void OnCbnSelchangeSpan();
   afx_msg void OnHelp();
   afx_msg void OnBnClickedRadio();
   afx_msg void OnBnClickedSelectMultipleButton();
};
