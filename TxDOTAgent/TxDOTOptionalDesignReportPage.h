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

#pragma once

#include "TxDOTOptionalDesignData.h"
#include "TxDOTOptionalDesignBrokerRetreiver.h"

#include <Reporting\SpanGirderReportSpecification.h>
#include <memory>
#include "afxwin.h"

interface IReportManager;
// CTxDOTOptionalDesignReportPage dialog

class CTxDOTOptionalDesignReportPage : public CPropertyPage, public ITxDataObserver
{
	DECLARE_DYNAMIC(CTxDOTOptionalDesignReportPage)

public:
	CTxDOTOptionalDesignReportPage();
	virtual ~CTxDOTOptionalDesignReportPage() override;

// Dialog Data
	enum { IDD = IDD_REPORT_PAGE };

   // here we store a pointer to our data source and all change events until we need to display
   CTxDOTOptionalDesignData* m_pData;
   int m_ChangeStatus;

   // here we can get our broker
   ITxDOTBrokerRetriever* m_pBrokerRetriever;

   // Our reporting guts
   std::shared_ptr<CReportSpecification> m_pRptSpec;
   std::shared_ptr<CReportBrowser> m_pBrowser; // this is the actual browser window that displays the report

   // listen to data change events
   virtual void OnTxDotDataChanged(int change) override;

   // create spec for currently selected report
   std::shared_ptr<CReportSpecification> CreateSelectedReportSpec(IReportManager* pReportMgr);
   void CreateNewBrowser(IBroker* pBroker);
   void DisplayErrorMode(TxDOTBrokerRetrieverException& exc);


protected:
	virtual void DoDataExchange(CDataExchange* pDX) override;    // DDX/DDV support

   void EditReport();

	DECLARE_MESSAGE_MAP()
public:
   virtual BOOL OnInitDialog() override;
   virtual BOOL OnSetActive() override;
   afx_msg void OnSize(UINT nType, int cx, int cy);
   afx_msg BOOL OnEraseBkgnd(CDC* pDC);
   CStatic m_BrowserPlaceholder;
   CComboBox m_ReportCombo;
   CStatic m_ErrorStatic;
   afx_msg void OnFilePrint();
   afx_msg void OnCmenuSelected(UINT id);

   afx_msg void OnEdit();

   afx_msg void OnCbnSelchangeReportCombo();
   afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
   virtual void AssertValid() const override;
   afx_msg void OnHelpFinder();
};
