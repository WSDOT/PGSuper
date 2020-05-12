#pragma once


// CSpecGirderFatiguePropertyPage

class CSpecGirderFatiguePropertyPage : public CMFCPropertyPage
{
	DECLARE_DYNAMIC(CSpecGirderFatiguePropertyPage)

public:
   CSpecGirderFatiguePropertyPage();
	virtual ~CSpecGirderFatiguePropertyPage();

protected:
   virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   virtual BOOL OnInitDialog();
   virtual BOOL OnSetActive();

   afx_msg void OnHelp();
   DECLARE_MESSAGE_MAP()
};

