#pragma once


// CSpecClosureTemporaryPropertyPage

class CSpecClosureTemporaryPropertyPage : public CMFCPropertyPage
{
	DECLARE_DYNAMIC(CSpecClosureTemporaryPropertyPage)

public:
   CSpecClosureTemporaryPropertyPage();
	virtual ~CSpecClosureTemporaryPropertyPage();

protected:
   virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   virtual BOOL OnInitDialog();
   virtual BOOL OnSetActive();

   afx_msg void OnHelp();
   DECLARE_MESSAGE_MAP()
};

