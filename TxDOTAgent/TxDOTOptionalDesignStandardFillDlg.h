#pragma once

#include "TxDOTOptionalDesignGirderData.h"
#include "TxDOTOptionalDesignBrokerRetreiver.h"

class CTxDOTOptionalDesignGirderInputPage;
// CTxDOTOptionalDesignStandardFillDlg dialog

class CTxDOTOptionalDesignStandardFillDlg : public CDialog
{
public:
   friend CTxDOTOptionalDesignGirderInputPage;

	DECLARE_DYNAMIC(CTxDOTOptionalDesignStandardFillDlg)

public:
	CTxDOTOptionalDesignStandardFillDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CTxDOTOptionalDesignStandardFillDlg();

// Dialog Data
	enum { IDD = IDD_STANDARD_FILL_DLG };

   CTxDOTOptionalDesignGirderData* m_pGirderData;
   CString m_GirderEntryName;
   ITxDOTBrokerRetriever* m_pBrokerRetriever;

   CString m_strNumStrands;
   StrandIndexType m_NumStrands;
   Float64 m_To;

public:
   void LoadDialogData();
   void SaveDialogData();
   void Init(CTxDOTOptionalDesignGirderData* pGirderData, ITxDOTBrokerRetriever* m_pBrokerRetriever, LPCTSTR entryName);
   void UpdateLibraryData();
   void InitStrandNoCtrl();
   void UpdateControls();

   // common girder dialog calls
   virtual void SetGirderEntryName(LPCTSTR entryName);
   virtual BOOL OnFillSetActive();
   virtual BOOL OnFillKillActive();

   bool m_bFirstActive;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
   afx_msg void OnCbnSelchangeOptNumStrands();
   virtual BOOL OnInitDialog();
   afx_msg void OnBnClickedOptCompute();
   afx_msg BOOL OnEraseBkgnd(CDC* pDC);
   afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
};
