#pragma once


// CSpecCreepGeneralPropertyPage

class CSpecCreepGeneralPropertyPage : public CMFCPropertyPage
{
	DECLARE_DYNAMIC(CSpecCreepGeneralPropertyPage)

public:
   CSpecCreepGeneralPropertyPage();
	virtual ~CSpecCreepGeneralPropertyPage();

protected:
   virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   virtual BOOL OnInitDialog();

   afx_msg void OnHelp();
   DECLARE_MESSAGE_MAP()
};

