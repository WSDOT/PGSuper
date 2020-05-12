#pragma once


// CSpecPrestressingOptionsPropertyPage

class CSpecPrestressingOptionsPropertyPage : public CMFCPropertyPage
{
	DECLARE_DYNAMIC(CSpecPrestressingOptionsPropertyPage)

public:
   CSpecPrestressingOptionsPropertyPage();
	virtual ~CSpecPrestressingOptionsPropertyPage();

protected:
   virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   virtual BOOL OnInitDialog();

   afx_msg void OnHelp();
   DECLARE_MESSAGE_MAP()
};

