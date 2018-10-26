#pragma once

#include "StrandRowGrid.h"
#include "TxDOTOptionalDesignGirderData.h"
#include "afxwin.h"

// CTxDOTOptionalDesignNonStandardFillDlg dialog

class CTxDOTOptionalDesignNonStandardFillDlg : public CDialog, public StrandRowGridEventHandler
{
	DECLARE_DYNAMIC(CTxDOTOptionalDesignNonStandardFillDlg)

public:
	CTxDOTOptionalDesignNonStandardFillDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CTxDOTOptionalDesignNonStandardFillDlg();

   void Init(CTxDOTOptionalDesignGirderData* pGirderData, ITxDOTBrokerRetriever* pBrokerRetriever);

// Dialog Data
	enum { IDD = IDD_NONSTANDARD_FILL_DLG };

   CTxDOTOptionalDesignGirderData* m_pGirderData;
   ITxDOTBrokerRetriever* m_pBrokerRetriever;
   bool m_UseDepressed;

private:
   CStrandRowGrid m_GridAtCL;
   CStrandRowGrid m_GridAtEnds;

   bool m_bFirstActive;
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

public:
   virtual BOOL OnInitDialog();

   // We aren't a property page, but we need to act like one
   virtual BOOL OnFillSetActive();
   virtual BOOL OnFillKillActive();

   void DoUseDepressed(bool useDepr);


   // listen to our grids
   void OnGridDataChanged();

private:
   void UpdateNoStrandsCtrls();
   void DisplayEndCtrls();

   CStatic m_CLNoStrandsCtrl;
   CStatic m_EndsNoStrandsCtrl;
public:
   afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
   afx_msg BOOL OnEraseBkgnd(CDC* pDC);
};
