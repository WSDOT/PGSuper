#pragma once


// CSpecDesignReleasePropertyPage

class CSpecDesignReleasePropertyPage : public CMFCPropertyPage
{
	DECLARE_DYNAMIC(CSpecDesignReleasePropertyPage)

public:
   CSpecDesignReleasePropertyPage();
	virtual ~CSpecDesignReleasePropertyPage();

protected:
   virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   virtual BOOL OnInitDialog();
   virtual BOOL OnSetActive();

   afx_msg void OnCheckHoldDownForce();
   afx_msg void OnCheckSlope();
   afx_msg void OnCheckSplitting();
   afx_msg void OnCheckConfinement();
   afx_msg void OnHelp();
   DECLARE_MESSAGE_MAP()
};

