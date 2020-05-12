#pragma once


// CSpecLiftingModulusOfRupturePropertyPage

class CSpecLiftingModulusOfRupturePropertyPage : public CMFCPropertyPage
{
	DECLARE_DYNAMIC(CSpecLiftingModulusOfRupturePropertyPage)

public:
   CSpecLiftingModulusOfRupturePropertyPage();
	virtual ~CSpecLiftingModulusOfRupturePropertyPage();

protected:
   virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   virtual BOOL OnInitDialog();
   virtual BOOL OnSetActive();

   afx_msg void OnHelp();
   DECLARE_MESSAGE_MAP()
};

