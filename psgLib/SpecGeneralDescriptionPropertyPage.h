#pragma once


// CSpecGeneralDescriptionPropertyPage

class CSpecGeneralDescriptionPropertyPage : public CMFCPropertyPage
{
	DECLARE_DYNAMIC(CSpecGeneralDescriptionPropertyPage)

public:
   CSpecGeneralDescriptionPropertyPage();
	virtual ~CSpecGeneralDescriptionPropertyPage();

protected:
   virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   afx_msg void OnHelp();
   DECLARE_MESSAGE_MAP()
};

