#pragma once


// CSpecDesignConcretePropertyPage

class CSpecDesignConcretePropertyPage : public CMFCPropertyPage
{
	DECLARE_DYNAMIC(CSpecDesignConcretePropertyPage)

public:
   CSpecDesignConcretePropertyPage();
	virtual ~CSpecDesignConcretePropertyPage();

protected:
   virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   virtual BOOL OnInitDialog();

   afx_msg void OnFcTypeChanged();
   afx_msg void OnBnClicked90DayStrength();
   afx_msg void OnHelp();
   DECLARE_MESSAGE_MAP()
};

