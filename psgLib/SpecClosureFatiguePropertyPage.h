#pragma once


// CSpecClosureFatiguePropertyPage

class CSpecClosureFatiguePropertyPage : public CMFCPropertyPage
{
	DECLARE_DYNAMIC(CSpecClosureFatiguePropertyPage)

public:
   CSpecClosureFatiguePropertyPage();
	virtual ~CSpecClosureFatiguePropertyPage();

protected:
   virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   virtual BOOL OnInitDialog();
   virtual BOOL OnSetActive();

   afx_msg void OnHelp();
   DECLARE_MESSAGE_MAP()
};

