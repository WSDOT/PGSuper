#pragma once


// CBridgeDescRatingPage dialog

class CBridgeDescRatingPage : public CPropertyPage
{
	DECLARE_DYNAMIC(CBridgeDescRatingPage)

public:
	CBridgeDescRatingPage();
	virtual ~CBridgeDescRatingPage();

// Dialog Data
	enum { IDD = IDD_BRIDGEDESC_RATING };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
   virtual BOOL OnInitDialog();
   afx_msg void OnConditionFactorTypeChanged();
   afx_msg void OnHelp();
};
