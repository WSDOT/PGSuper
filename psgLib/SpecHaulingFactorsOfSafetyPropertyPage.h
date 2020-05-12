#pragma once


// CSpecHaulingFactorsOfSafetyPropertyPage

class CSpecHaulingFactorsOfSafetyPropertyPage : public CMFCPropertyPage
{
	DECLARE_DYNAMIC(CSpecHaulingFactorsOfSafetyPropertyPage)

public:
   CSpecHaulingFactorsOfSafetyPropertyPage();
	virtual ~CSpecHaulingFactorsOfSafetyPropertyPage();

protected:
   virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   virtual BOOL OnInitDialog();

   afx_msg void OnHelp();
   DECLARE_MESSAGE_MAP()
};

