#pragma once


// CSpecGirderFinalPropertyPage

class CSpecGirderFinalPropertyPage : public CMFCPropertyPage
{
	DECLARE_DYNAMIC(CSpecGirderFinalPropertyPage)

public:
   CSpecGirderFinalPropertyPage();
	virtual ~CSpecGirderFinalPropertyPage();

protected:
   virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   virtual BOOL OnInitDialog();
   virtual BOOL OnSetActive();

   afx_msg void OnCheckServiceITensileStress();
   afx_msg void OnCheckServiceITensionMax();
   afx_msg void OnCheckServiceIIITensionMax();
   afx_msg void OnCheckSevereServiceIIITensionMax();
   afx_msg void OnHelp();
   DECLARE_MESSAGE_MAP()
};

