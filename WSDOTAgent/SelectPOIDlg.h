#pragma once
#include "resource.h"
#include <PgsExt\GirderPointOfInterest.h>

// CSelectPOIDlg dialog

class CSelectPOIDlg : public CDialog
{
	DECLARE_DYNAMIC(CSelectPOIDlg)

public:
	CSelectPOIDlg(IBroker* pBroker,CWnd* pParent = NULL);   // standard constructor
	virtual ~CSelectPOIDlg();

// Dialog Data
	enum { IDD = IDD_SELECT_POI };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
   virtual BOOL OnInitDialog();

   pgsPointOfInterest m_InitialPOI;
   IntervalIndexType m_IntervalIdx;
   CGirderKey m_GirderKey;

   pgsPointOfInterest GetPOI();

private:
   IBroker* m_pBroker;

   std::vector<pgsPointOfInterest> m_vPOI;

   CComboBox m_cbGroup;
   CComboBox m_cbGirder;
   CSliderCtrl m_Slider;
   CStatic m_Label;
   int m_SliderPos;

   void UpdateGirderComboBox(GroupIndexType groupIdx);
   void UpdateSliderLabel();
   void UpdatePOI();
   void FillIntervalCtrl();

public:
   afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
   afx_msg void OnGroupChanged();
   afx_msg void OnGirderChanged();
   afx_msg void OnHelp();
};
