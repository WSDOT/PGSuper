#pragma once


// CSpecMomentResistanceFactorsPropertyPage

class CSpecMomentResistanceFactorsPropertyPage : public CMFCPropertyPage
{
	DECLARE_DYNAMIC(CSpecMomentResistanceFactorsPropertyPage)

public:
   CSpecMomentResistanceFactorsPropertyPage();
	virtual ~CSpecMomentResistanceFactorsPropertyPage();

protected:
   virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   virtual BOOL OnInitDialog();
   virtual BOOL OnSetActive();

   afx_msg void OnHelp();
   DECLARE_MESSAGE_MAP()
};

