#pragma once


// CSpecHaulingMethodPropertyPage

class CSpecHaulingMethodPropertyPage : public CMFCPropertyPage
{
	DECLARE_DYNAMIC(CSpecHaulingMethodPropertyPage)

public:
   CSpecHaulingMethodPropertyPage();
	virtual ~CSpecHaulingMethodPropertyPage();

protected:
   virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   virtual BOOL OnInitDialog();

   afx_msg void OnHelp();
   DECLARE_MESSAGE_MAP()
};

