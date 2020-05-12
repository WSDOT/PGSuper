#pragma once


// CSpecGeneralSectionPropertiesPropertyPage

class CSpecGeneralSectionPropertiesPropertyPage : public CMFCPropertyPage
{
	DECLARE_DYNAMIC(CSpecGeneralSectionPropertiesPropertyPage)

public:
   CSpecGeneralSectionPropertiesPropertyPage();
	virtual ~CSpecGeneralSectionPropertiesPropertyPage();

protected:
   virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   virtual BOOL OnInitDialog();

   afx_msg void OnHelp();
   DECLARE_MESSAGE_MAP()
public:
   virtual BOOL OnSetActive();
};

