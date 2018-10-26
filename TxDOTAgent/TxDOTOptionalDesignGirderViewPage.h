#pragma once

#include "TxDOTOptionalDesignData.h"
#include "TxDOTOptionalDesignBrokerRetreiver.h"

// CTxDOTOptionalDesignGirderViewPage dialog

class CTxDOTOptionalDesignGirderViewPage : public CPropertyPage, public ITxDataObserver
{
	DECLARE_DYNAMIC(CTxDOTOptionalDesignGirderViewPage)

public:
	CTxDOTOptionalDesignGirderViewPage();
	virtual ~CTxDOTOptionalDesignGirderViewPage();

// Dialog Data
	enum { IDD = IDD_GIRDER_VIEW_PAGE };

   // here we store a pointer to our data source and all change events until we need to display
   CTxDOTOptionalDesignData* m_pData;
   int m_ChangeStatus;

   // here we can get our broker
   ITxDOTBrokerRetriever* m_pBrokerRetriever;

// listen to data change events
   virtual void OnTxDotDataChanged(int change);


protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
   virtual BOOL OnSetActive();
   virtual BOOL OnInitDialog();
   afx_msg BOOL OnEraseBkgnd(CDC* pDC);
   afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
   afx_msg void OnFilePrint();
   virtual void AssertValid() const;
};
