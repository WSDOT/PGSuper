#pragma once


// CSpecCreepExcessCamberPropertyPage

class CSpecCreepExcessCamberPropertyPage : public CMFCPropertyPage
{
	DECLARE_DYNAMIC(CSpecCreepExcessCamberPropertyPage)

public:
   CSpecCreepExcessCamberPropertyPage();
	virtual ~CSpecCreepExcessCamberPropertyPage();

protected:
   virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   virtual BOOL OnInitDialog();

   afx_msg void OnChangeHaunch();
   afx_msg void OnHelp();
   DECLARE_MESSAGE_MAP()
};

