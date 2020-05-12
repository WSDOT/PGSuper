#pragma once


// CSpecClosureFinalPropertyPage

class CSpecClosureFinalPropertyPage : public CMFCPropertyPage
{
	DECLARE_DYNAMIC(CSpecClosureFinalPropertyPage)

public:
   CSpecClosureFinalPropertyPage();
	virtual ~CSpecClosureFinalPropertyPage();

protected:
   virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   virtual BOOL OnInitDialog();
   virtual BOOL OnSetActive();

   afx_msg void OnHelp();
   DECLARE_MESSAGE_MAP()
};

