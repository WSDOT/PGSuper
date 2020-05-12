#pragma once


// CSpecGirderPrincipalStressPropertyPage

class CSpecGirderPrincipalStressPropertyPage : public CMFCPropertyPage
{
	DECLARE_DYNAMIC(CSpecGirderPrincipalStressPropertyPage)

public:
   CSpecGirderPrincipalStressPropertyPage();
	virtual ~CSpecGirderPrincipalStressPropertyPage();

protected:
   virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   virtual BOOL OnInitDialog();
   virtual BOOL OnSetActive();

   afx_msg void OnHelp();
   DECLARE_MESSAGE_MAP()
};

