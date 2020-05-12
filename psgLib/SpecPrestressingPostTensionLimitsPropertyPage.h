#pragma once


// CSpecPrestressingPostTensionLimitsPropertyPage

class CSpecPrestressingPostTensionLimitsPropertyPage : public CMFCPropertyPage
{
	DECLARE_DYNAMIC(CSpecPrestressingPostTensionLimitsPropertyPage)

public:
   CSpecPrestressingPostTensionLimitsPropertyPage();
	virtual ~CSpecPrestressingPostTensionLimitsPropertyPage();

protected:
   virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   virtual BOOL OnInitDialog();
   virtual BOOL OnSetActive();

   void EnableControls(BOOL bEnable, UINT nSR, UINT nLR);

   afx_msg void OnPtChecked();
   afx_msg void OnHelp();
   DECLARE_MESSAGE_MAP()
};

