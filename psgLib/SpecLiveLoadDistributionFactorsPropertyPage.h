#pragma once


// CSpecLiveLoadDistributionFactorsPropertyPage

class CSpecLiveLoadDistributionFactorsPropertyPage : public CMFCPropertyPage
{
	DECLARE_DYNAMIC(CSpecLiveLoadDistributionFactorsPropertyPage)

public:
   CSpecLiveLoadDistributionFactorsPropertyPage();
	virtual ~CSpecLiveLoadDistributionFactorsPropertyPage();

protected:
   virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   virtual BOOL OnInitDialog();
   virtual BOOL OnSetActive();

   afx_msg void OnCbnSelchangeLldf();
   afx_msg void OnHelp();
   DECLARE_MESSAGE_MAP()
};

