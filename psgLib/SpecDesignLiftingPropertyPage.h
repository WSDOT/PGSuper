#pragma once


// CSpecDesignLiftingPropertyPage

class CSpecDesignLiftingPropertyPage : public CMFCPropertyPage
{
	DECLARE_DYNAMIC(CSpecDesignLiftingPropertyPage)

public:
   CSpecDesignLiftingPropertyPage();
	virtual ~CSpecDesignLiftingPropertyPage();

protected:
   virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   virtual BOOL OnInitDialog();

   afx_msg void OnCheckLifting();
   afx_msg void OnCheckHandlingWeight();
   afx_msg void OnHelp();
   DECLARE_MESSAGE_MAP()
};

