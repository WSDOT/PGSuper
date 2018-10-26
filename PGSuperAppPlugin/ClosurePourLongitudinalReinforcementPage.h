#pragma once

#include "BridgeDescLongRebarGrid.h"

// CClosurePourLongitudinalReinforcementPage dialog

class CClosurePourLongitudinalReinforcementPage : public CPropertyPage
{
	DECLARE_DYNAMIC(CClosurePourLongitudinalReinforcementPage)

public:
	CClosurePourLongitudinalReinforcementPage();
	virtual ~CClosurePourLongitudinalReinforcementPage();

// Dialog Data
	enum { IDD = IDD_CLOSURE_LONGITUDINAL };

   virtual BOOL OnInitDialog();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

public:
   void OnEnableDelete(bool canDelete);

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CGirderDescLongitudinalRebar)
	afx_msg void OnRestoreDefaults();
	afx_msg void OnInsertrow();
	afx_msg void OnAppendRow();
	afx_msg void OnRemoveRows();
	afx_msg void OnHelp();
	//}}AFX_MSG

public:
   CGirderDescLongRebarGrid m_Grid;
};
