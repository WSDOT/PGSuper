#pragma once


// CSpecDeadLoadRailingPropertyPage

class CSpecDeadLoadRailingPropertyPage : public CMFCPropertyPage
{
	DECLARE_DYNAMIC(CSpecDeadLoadRailingPropertyPage)

public:
   CSpecDeadLoadRailingPropertyPage();
	virtual ~CSpecDeadLoadRailingPropertyPage();

protected:
   virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   virtual BOOL OnInitDialog();

   CSpinButtonCtrl	m_TrafficSpin;

   afx_msg void OnHelp();
   DECLARE_MESSAGE_MAP()
};

