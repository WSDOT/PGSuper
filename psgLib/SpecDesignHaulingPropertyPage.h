#pragma once


// CSpecDesignHaulingPropertyPage

class CSpecDesignHaulingPropertyPage : public CMFCPropertyPage
{
	DECLARE_DYNAMIC(CSpecDesignHaulingPropertyPage)

public:
   CSpecDesignHaulingPropertyPage();
	virtual ~CSpecDesignHaulingPropertyPage();

protected:
   virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   virtual BOOL OnInitDialog();

   afx_msg void OnCheckHauling();
   afx_msg void OnBnClickedIsSupportLessThan();
   afx_msg void OnHelp();
   DECLARE_MESSAGE_MAP()
};

