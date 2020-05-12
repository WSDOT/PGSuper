#pragma once


// CSpecLimitsWarningsPropertyPage

class CSpecLimitsWarningsPropertyPage : public CMFCPropertyPage
{
	DECLARE_DYNAMIC(CSpecLimitsWarningsPropertyPage)

public:
   CSpecLimitsWarningsPropertyPage();
	virtual ~CSpecLimitsWarningsPropertyPage();

protected:
   virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   virtual BOOL OnInitDialog();
   virtual BOOL OnSetActive();

   afx_msg void OnBnClickedCheckGirderSag();
   afx_msg void OnHelp();
   DECLARE_MESSAGE_MAP()
};

