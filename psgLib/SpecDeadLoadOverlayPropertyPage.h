#pragma once


// CSpecDeadLoadOverlayPropertyPage

class CSpecDeadLoadOverlayPropertyPage : public CMFCPropertyPage
{
	DECLARE_DYNAMIC(CSpecDeadLoadOverlayPropertyPage)

public:
   CSpecDeadLoadOverlayPropertyPage();
	virtual ~CSpecDeadLoadOverlayPropertyPage();

protected:
   virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   virtual BOOL OnInitDialog();

   afx_msg void OnHelp();
   DECLARE_MESSAGE_MAP()
};

