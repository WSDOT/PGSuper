#pragma once


// CSpecLiftingLimitsPropertyPage

class CSpecLiftingLimitsPropertyPage : public CMFCPropertyPage
{
	DECLARE_DYNAMIC(CSpecLiftingLimitsPropertyPage)

public:
   CSpecLiftingLimitsPropertyPage();
	virtual ~CSpecLiftingLimitsPropertyPage();

protected:
   virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   virtual BOOL OnInitDialog();

   afx_msg void OnCheckLiftingNormalMax();
   afx_msg void OnHelp();
   DECLARE_MESSAGE_MAP()
};

