#pragma once


// CSpecLiveLoadHL93PropertyPage

class CSpecLiveLoadHL93PropertyPage : public CMFCPropertyPage
{
	DECLARE_DYNAMIC(CSpecLiveLoadHL93PropertyPage)

public:
   CSpecLiveLoadHL93PropertyPage();
	virtual ~CSpecLiveLoadHL93PropertyPage();

protected:
   virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   virtual BOOL OnInitDialog();

   afx_msg void OnHelp();
   DECLARE_MESSAGE_MAP()
};

