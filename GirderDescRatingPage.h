#pragma once


// CGirderDescRatingPage dialog

class CGirderDescRatingPage : public CPropertyPage
{
	DECLARE_DYNAMIC(CGirderDescRatingPage)

public:
	CGirderDescRatingPage();
	virtual ~CGirderDescRatingPage();

   pgsTypes::ConditionFactorType m_ConditionFactorType;
   Float64 m_ConditionFactor;

// Dialog Data
	enum { IDD = IDD_GIRDERDESC_RATING };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
   afx_msg void OnConditionFactorTypeChanged();
   afx_msg void OnHelp();
   virtual BOOL OnInitDialog();
};
