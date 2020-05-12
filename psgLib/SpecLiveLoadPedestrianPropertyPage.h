#pragma once


// CSpecLiveLoadPedestrianPropertyPage

class CSpecLiveLoadPedestrianPropertyPage : public CMFCPropertyPage
{
	DECLARE_DYNAMIC(CSpecLiveLoadPedestrianPropertyPage)

public:
   CSpecLiveLoadPedestrianPropertyPage();
	virtual ~CSpecLiveLoadPedestrianPropertyPage();

protected:
   virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   virtual BOOL OnInitDialog();

   afx_msg void OnHelp();
   DECLARE_MESSAGE_MAP()
};

