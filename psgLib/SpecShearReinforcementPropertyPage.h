#pragma once


// CSpecShearReinforcementPropertyPage

class CSpecShearReinforcementPropertyPage : public CMFCPropertyPage
{
	DECLARE_DYNAMIC(CSpecShearReinforcementPropertyPage)

public:
   CSpecShearReinforcementPropertyPage();
	virtual ~CSpecShearReinforcementPropertyPage();

protected:
   virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   virtual BOOL OnInitDialog();
   virtual BOOL OnSetActive();

   afx_msg void OnHelp();
   DECLARE_MESSAGE_MAP()
};

