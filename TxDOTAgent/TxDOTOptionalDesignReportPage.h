#pragma once

#include "TxDOTOptionalDesignData.h"
#include "TxDOTOptionalDesignBrokerRetreiver.h"

#include <Reporting\SpanGirderReportSpecification.h>
#include <boost\shared_ptr.hpp>
#include "afxwin.h"

interface IReportManager;
// CTxDOTOptionalDesignReportPage dialog

class CTxDOTOptionalDesignReportPage : public CPropertyPage, public ITxDataObserver
{
	DECLARE_DYNAMIC(CTxDOTOptionalDesignReportPage)

public:
	CTxDOTOptionalDesignReportPage();
	virtual ~CTxDOTOptionalDesignReportPage();

// Dialog Data
	enum { IDD = IDD_REPORT_PAGE };

   // here we store a pointer to our data source and all change events until we need to display
   CTxDOTOptionalDesignData* m_pData;
   int m_ChangeStatus;

   // here we can get our broker
   ITxDOTBrokerRetriever* m_pBrokerRetriever;

   // Oour reporting guts
   boost::shared_ptr<CReportSpecification> m_pRptSpec;
   boost::shared_ptr<CReportBrowser> m_pBrowser; // this is the actual browser window that displays the report

   // listen to data change events
   virtual void OnTxDotDataChanged(int change);

   // create spec for currently selected report
   boost::shared_ptr<CReportSpecification> CreateSelectedReportSpec(IReportManager* pReportMgr);
   void CreateNewBrowser(IBroker* pBroker);
   void DisplayErrorMode(TxDOTBrokerRetrieverException& exc);


protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

   virtual void EditReport();

	DECLARE_MESSAGE_MAP()
public:
   virtual BOOL OnInitDialog();
   virtual BOOL OnSetActive();
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
   virtual void AssertValid() const;
   afx_msg void OnHelpFinder();
};
