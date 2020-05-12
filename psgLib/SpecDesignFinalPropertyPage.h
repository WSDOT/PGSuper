#pragma once


// CSpecDesignFinalPropertyPage

class CSpecDesignFinalPropertyPage : public CMFCPropertyPage
{
	DECLARE_DYNAMIC(CSpecDesignFinalPropertyPage)

public:
   CSpecDesignFinalPropertyPage();
	virtual ~CSpecDesignFinalPropertyPage();

protected:
   virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   virtual BOOL OnInitDialog();

   afx_msg void OnCheckA();
   afx_msg void OnDesignA();
   afx_msg void OnBnClickedCheckBottomFlangeClearance();
   afx_msg void OnBnClickedCheckInclindedGirder();
   afx_msg void OnBnClickedLlDeflection();
   afx_msg void OnHelp();
   DECLARE_MESSAGE_MAP()
};

