#pragma once


// CSpecHaulingKDOTPropertyPage

class CSpecHaulingKDOTPropertyPage : public CMFCPropertyPage
{
	DECLARE_DYNAMIC(CSpecHaulingKDOTPropertyPage)

public:
   CSpecHaulingKDOTPropertyPage();
	virtual ~CSpecHaulingKDOTPropertyPage();

protected:
   virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   virtual BOOL OnInitDialog();

   afx_msg void OnBnClickedCheckHaulingTensMax();
   afx_msg void OnHelp();
   DECLARE_MESSAGE_MAP()
};

