#pragma once


// CSpecMomentCapacityPropertyPage

class CSpecMomentCapacityPropertyPage : public CMFCPropertyPage
{
	DECLARE_DYNAMIC(CSpecMomentCapacityPropertyPage)

public:
   CSpecMomentCapacityPropertyPage();
	virtual ~CSpecMomentCapacityPropertyPage();

protected:
   virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   virtual BOOL OnInitDialog();
   virtual BOOL OnSetActive();

   afx_msg void OnHelp();
   DECLARE_MESSAGE_MAP()
};

