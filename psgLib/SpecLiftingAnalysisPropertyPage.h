#pragma once


// CSpecLiftingAnalysisPropertyPage

class CSpecLiftingAnalysisPropertyPage : public CMFCPropertyPage
{
	DECLARE_DYNAMIC(CSpecLiftingAnalysisPropertyPage)

public:
   CSpecLiftingAnalysisPropertyPage();
	virtual ~CSpecLiftingAnalysisPropertyPage();

protected:
   virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   virtual BOOL OnInitDialog();

   afx_msg void OnCbnSelchangeWindType();
   afx_msg void OnHelp();
   DECLARE_MESSAGE_MAP()
};

