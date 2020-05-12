#pragma once


// CSpecPrestressingPretensionLimitsPropertyPage

class CSpecPrestressingPretensionLimitsPropertyPage : public CMFCPropertyPage
{
	DECLARE_DYNAMIC(CSpecPrestressingPretensionLimitsPropertyPage)

public:
   CSpecPrestressingPretensionLimitsPropertyPage();
	virtual ~CSpecPrestressingPretensionLimitsPropertyPage();

protected:
   virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   virtual BOOL OnInitDialog();
   virtual BOOL OnSetActive();

   void EnableControls(BOOL bEnable, UINT nSR, UINT nLR);

   afx_msg void OnPsChecked();
   afx_msg void OnCheckPsAfterTransfer();
   afx_msg void OnHelp();
   DECLARE_MESSAGE_MAP()
};

