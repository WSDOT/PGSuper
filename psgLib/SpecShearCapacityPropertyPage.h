#pragma once


// CSpecShearCapacityPropertyPage

class CSpecShearCapacityPropertyPage : public CMFCPropertyPage
{
	DECLARE_DYNAMIC(CSpecShearCapacityPropertyPage)

public:
   CSpecShearCapacityPropertyPage();
	virtual ~CSpecShearCapacityPropertyPage();

protected:
   virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   virtual BOOL OnInitDialog();
   virtual BOOL OnSetActive();

   void FillShearMethodList();

   afx_msg void OnHelp();
   DECLARE_MESSAGE_MAP()
};

