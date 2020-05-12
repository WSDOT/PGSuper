#pragma once


// CSpecDesignRoadwayPropertyPage

class CSpecDesignRoadwayPropertyPage : public CMFCPropertyPage
{
	DECLARE_DYNAMIC(CSpecDesignRoadwayPropertyPage)

public:
   CSpecDesignRoadwayPropertyPage();
	virtual ~CSpecDesignRoadwayPropertyPage();

protected:
   virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   virtual BOOL OnInitDialog();

   afx_msg void OnHelp();
   DECLARE_MESSAGE_MAP()
};

