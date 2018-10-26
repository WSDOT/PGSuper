#pragma once

#include "TxDOTOptionalDesignData.h"
#include "TxDOTOptionalDesignBrokerRetreiver.h"
#include "TxDOTOptionalDesignStandardFillDlg.h"
#include "TxDOTOptionalDesignNonStandardFillDlg.h"
#include "TogaDirectFillDlg.h"

// CTxDOTOptionalDesignGirderInputPage dialog

class CTxDOTOptionalDesignGirderInputPage : public CPropertyPage, public ITxDataObserver
{
	DECLARE_DYNAMIC(CTxDOTOptionalDesignGirderInputPage)

public:
	CTxDOTOptionalDesignGirderInputPage();
	virtual ~CTxDOTOptionalDesignGirderInputPage();

// Dialog Data
	enum { IDD = IDD_GIRDER_INPUT_PAGE };

   // Store a pointer to our data source
   CTxDOTOptionalDesignData* m_pData;

   // here we can get our broker
   ITxDOTBrokerRetriever* m_pBrokerRetriever;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

public:
   BOOL UpdateData(BOOL bSaveAndValidate);

// ITxDataObserver
public:
   virtual void OnTxDotDataChanged(int change);
   bool m_GirderTypeChanged;

private:
   void LoadDialogData();
   void SaveDialogData();

   void OnStrandTypeChanged(long SizeCtrlID, long TypeCtrlID);
   void UpdateStrandSizeList(long StrandSizeListCtrlID, matPsStrand::Grade grade,matPsStrand::Type type, matPsStrand::Size size);
   void InitStrandTypeCtrl(long TypeCtrlID);
   void InitStrandSizeTypeCtrls();
   void InitOptStrandSizeTypeCtrls();
   void InitOrigStrandSizeTypeCtrls();
   void InitFillTypeCtrls();

   CString m_MostRecentLibraryEntry;

   // precaster optional design
   CTxDOTOptionalDesignStandardFillDlg    m_OptStandardDlg;
   CTxDOTOptionalDesignNonStandardFillDlg m_OptNonStandardDlg;
   CTogaDirectFillDlg                     m_OptDirectFillDlg;

   CTxDOTOptionalDesignGirderData::StrandFillType m_OptStrandFillType;
   CString m_strOptNoStrands;
   Float64 m_OptFc;
   Float64 m_OptFci;

   // Original design
   CTxDOTOptionalDesignStandardFillDlg    m_OrigStandardDlg;
   CTxDOTOptionalDesignNonStandardFillDlg m_OrigNonStandardDlg;
   CTogaDirectFillDlg                     m_OrigDirectFillDlg;

   CTxDOTOptionalDesignGirderData::StrandFillType m_OrigStrandFillType;
   CString m_strOrigNoStrands;
   Float64 m_OrigFc;
   Float64 m_OrigFci;


public:
   virtual BOOL OnInitDialog();
   afx_msg void OnCbnSelchangeOptStrandType();
   afx_msg void OnCbnSelchangeOptFilltypeCombo();

   afx_msg void OnCbnSelchangeOrigStrandType();
   afx_msg void OnCbnSelchangeOrigNumStrands();
   afx_msg void OnCbnSelchangeOrgFilltypeCombo();

   virtual BOOL OnSetActive();
   virtual BOOL OnKillActive();
   afx_msg BOOL OnEraseBkgnd(CDC* pDC);
   afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
   afx_msg void OnHelpFinder();
};
