#pragma once


// CSpecShearInterfaceShearPropertyPage

class CSpecShearInterfaceShearPropertyPage : public CMFCPropertyPage
{
	DECLARE_DYNAMIC(CSpecShearInterfaceShearPropertyPage)

public:
   CSpecShearInterfaceShearPropertyPage();
	virtual ~CSpecShearInterfaceShearPropertyPage();

protected:
   virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   virtual BOOL OnInitDialog();
   virtual BOOL OnSetActive();

   afx_msg void OnHelp();
   DECLARE_MESSAGE_MAP()
};

