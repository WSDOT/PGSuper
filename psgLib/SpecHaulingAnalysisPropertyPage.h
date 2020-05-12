#pragma once


// CSpecHaulingAnalysisPropertyPage

class CSpecHaulingAnalysisPropertyPage : public CMFCPropertyPage
{
	DECLARE_DYNAMIC(CSpecHaulingAnalysisPropertyPage)

public:
   CSpecHaulingAnalysisPropertyPage();
	virtual ~CSpecHaulingAnalysisPropertyPage();

protected:
   virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   virtual BOOL OnInitDialog();

   afx_msg void OnCbnSelchangeWindType();
   afx_msg void OnHelp();
   DECLARE_MESSAGE_MAP()
};

