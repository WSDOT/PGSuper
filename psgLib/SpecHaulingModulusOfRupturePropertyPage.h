#pragma once


// CSpecHaulingModulusOfRupturePropertyPage

class CSpecHaulingModulusOfRupturePropertyPage : public CMFCPropertyPage
{
	DECLARE_DYNAMIC(CSpecHaulingModulusOfRupturePropertyPage)

public:
   CSpecHaulingModulusOfRupturePropertyPage();
	virtual ~CSpecHaulingModulusOfRupturePropertyPage();

protected:
   virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   virtual BOOL OnInitDialog();
   virtual BOOL OnSetActive();

   afx_msg void OnHelp();
   DECLARE_MESSAGE_MAP()
};

