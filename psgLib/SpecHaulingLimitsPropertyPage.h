#pragma once


// CSpecHaulingLimitsPropertyPage

class CSpecHaulingLimitsPropertyPage : public CMFCPropertyPage
{
	DECLARE_DYNAMIC(CSpecHaulingLimitsPropertyPage)

public:
   CSpecHaulingLimitsPropertyPage();
	virtual ~CSpecHaulingLimitsPropertyPage();

protected:
   virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   virtual BOOL OnInitDialog();

   afx_msg void OnCheckHaulingTensMaxCrown();
   afx_msg void OnCheckHaulingTensMaxSuper();
   afx_msg void OnHelp();
   DECLARE_MESSAGE_MAP()
};

