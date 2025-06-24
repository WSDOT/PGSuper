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

#include "Reporting.h"
#include "MultiViewReportDlg.h"

#include "..\Documentation\PGSuper.hh"

#include <initguid.h>
#include <IFace\Tools.h>
#include <IFace\Bridge.h>
#include <IFace\DocumentType.h>

#include <PsgLib\GirderLabel.h>

#include "RMultiGirderSelectDlg.h"
#include <EAF\EAFDocument.h>

// CGirderMultiViewReportDlg dialog
class CGirderMultiViewReportDlg : public CMultiViewReportDlg
{
	DECLARE_DYNAMIC(CGirderMultiViewReportDlg)

public:
	CGirderMultiViewReportDlg(const CGirderKey& girderKey, std::shared_ptr<WBFL::EAF::Broker> pBroker, const WBFL::Reporting::ReportDescription& rptDesc,
		std::shared_ptr<WBFL::Reporting::ReportSpecification> pRptSpec);

	virtual ~CGirderMultiViewReportDlg();

	BOOL OnInitDialog();

	virtual void InitFromRptSpec();

	std::vector<CGirderKey> GetGirderKeys() const;

protected:

	std::vector<CGirderKey> m_GirderKeys;

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
   afx_msg void OnBnClickedRadio() override;
   afx_msg void OnBnClickedSelectMultipleButton() override;
};
