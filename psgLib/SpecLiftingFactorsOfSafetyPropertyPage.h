#pragma once


// CSpecLiftingFactorsOfSafetyPropertyPage

class CSpecLiftingFactorsOfSafetyPropertyPage : public CMFCPropertyPage
{
	DECLARE_DYNAMIC(CSpecLiftingFactorsOfSafetyPropertyPage)

public:
   CSpecLiftingFactorsOfSafetyPropertyPage();
	virtual ~CSpecLiftingFactorsOfSafetyPropertyPage();

protected:
   virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   virtual BOOL OnInitDialog();

   afx_msg void OnHelp();
   DECLARE_MESSAGE_MAP()
};

