#pragma once


// CSpecDesignStrandsPropertyPage

class CSpecDesignStrandsPropertyPage : public CMFCPropertyPage
{
	DECLARE_DYNAMIC(CSpecDesignStrandsPropertyPage)

public:
   CSpecDesignStrandsPropertyPage();
	virtual ~CSpecDesignStrandsPropertyPage();

protected:
   virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   virtual BOOL OnInitDialog();

   afx_msg void OnHelp();
   DECLARE_MESSAGE_MAP()
};

