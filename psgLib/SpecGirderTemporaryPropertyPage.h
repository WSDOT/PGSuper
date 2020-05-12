#pragma once


// CSpecGirderTemporaryPropertyPage

class CSpecGirderTemporaryPropertyPage : public CMFCPropertyPage
{
	DECLARE_DYNAMIC(CSpecGirderTemporaryPropertyPage)

public:
   CSpecGirderTemporaryPropertyPage();
	virtual ~CSpecGirderTemporaryPropertyPage();

protected:
   virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   virtual BOOL OnInitDialog();
   virtual BOOL OnSetActive();

   afx_msg void OnCheckReleaseTensionMax();
   afx_msg void OnCheckTemporaryStresses();
   afx_msg void OnCheckTSRemovalTensionMax();
   afx_msg void OnCheckAfterDeckTensionMax();
   afx_msg void OnHelp();
   DECLARE_MESSAGE_MAP()
};

