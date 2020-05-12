#pragma once


// CSpecPrestressingDuctsPropertyPage

class CSpecPrestressingDuctsPropertyPage : public CMFCPropertyPage
{
	DECLARE_DYNAMIC(CSpecPrestressingDuctsPropertyPage)

public:
   CSpecPrestressingDuctsPropertyPage();
	virtual ~CSpecPrestressingDuctsPropertyPage();

protected:
   virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   virtual BOOL OnInitDialog();

   afx_msg void OnHelp();
   DECLARE_MESSAGE_MAP()
};

