#pragma once


// CSpecLimitsConcretePropertyPage

class CSpecLimitsConcretePropertyPage : public CMFCPropertyPage
{
	DECLARE_DYNAMIC(CSpecLimitsConcretePropertyPage)

public:
   CSpecLimitsConcretePropertyPage();
	virtual ~CSpecLimitsConcretePropertyPage();

protected:
   virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   virtual BOOL OnInitDialog();
   virtual BOOL OnSetActive();

   afx_msg void OnHelp();
   DECLARE_MESSAGE_MAP()
};

