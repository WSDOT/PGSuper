#pragma once


// CSpecShearResistanceFactorsPropertyPage

class CSpecShearResistanceFactorsPropertyPage : public CMFCPropertyPage
{
	DECLARE_DYNAMIC(CSpecShearResistanceFactorsPropertyPage)

public:
   CSpecShearResistanceFactorsPropertyPage();
	virtual ~CSpecShearResistanceFactorsPropertyPage();

protected:
   virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   virtual BOOL OnInitDialog();
   virtual BOOL OnSetActive();

   afx_msg void OnHelp();
   DECLARE_MESSAGE_MAP()
};

