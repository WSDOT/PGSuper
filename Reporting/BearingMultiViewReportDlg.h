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
#include "MultiViewReportDlg.h"
#include <IFace/AnalysisResults.h>



// CMultiViewReportDlg dialog
class CBearingMultiViewReportDlg : public CMultiViewReportDlg
{
	DECLARE_DYNAMIC(CBearingMultiViewReportDlg)

public:
	CBearingMultiViewReportDlg(std::shared_ptr<WBFL::EAF::Broker> pBroker,
		const WBFL::Reporting::ReportDescription& rptDesc,
		std::shared_ptr<WBFL::Reporting::ReportSpecification> pRptSpec,
		const CGirderKey& girderKey, const ReactionLocation& reactionLocation);
	virtual ~CBearingMultiViewReportDlg();

	virtual BOOL OnInitDialog();

	virtual void InitFromRptSpec();

	std::vector<ReactionLocation> GetReactionLocations() const;

	void OnBnClickedRadio() override;

	void UpdateBearingComboBox();
	
	DECLARE_MESSAGE_MAP()



	afx_msg void UpdateGirderComboBox();
	afx_msg void UpdateSpanComboBox();
	//afx_msg void OnHelp();
	//afx_msg void OnBnClickedRadio();
	afx_msg void OnBnClickedSelectMultipleButton();
	//afx_msg void OnSelectAll();
	//afx_msg void OnDeselectAll();

// Dialog Data
	enum { IDD = IDD_MULTIVIEWREPORT };


protected:

	std::vector<ReactionLocation> m_RLmenuItems;
	ReactionLocation m_ReactionLocation;
	std::vector<ReactionLocation> m_ReactionLocations;

//   void UpdateButtonText();
//
	 virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
//
//   virtual void UpdateGirderComboBox(SpanIndexType spanIdx);
//
//   virtual void UpdateChapterList();
//   virtual void ClearChapterCheckMarks(BOOL bClear = TRUE);
//   virtual void InitChapterListFromSpec();
//   virtual void InitFromRptSpec();
//

};
