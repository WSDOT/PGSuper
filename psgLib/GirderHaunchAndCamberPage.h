#pragma once


// CGirderHaunchAndCamberPage dialog

class CGirderHaunchAndCamberPage : public CPropertyPage
{
	DECLARE_DYNAMIC(CGirderHaunchAndCamberPage)

public:
	CGirderHaunchAndCamberPage();
	virtual ~CGirderHaunchAndCamberPage();

// Dialog Data
	enum { IDD = IDD_GIRDER_HAUNCH_CAMBER };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
   afx_msg void OnBnClickedCheckCl();
   afx_msg void OnHelp();
   virtual BOOL OnInitDialog();
};
