#pragma once


#include "BridgeDescLongRebarGrid.h"

/////////////////////////////////////////////////////////////////////////////
// CGirderSegmentLongitudinalRebarPage dialog

class CGirderSegmentLongitudinalRebarPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CGirderSegmentLongitudinalRebarPage)

// Construction
public:
	CGirderSegmentLongitudinalRebarPage();
	~CGirderSegmentLongitudinalRebarPage();

// Dialog Data
	//{{AFX_DATA(CGirderSegmentLongitudinalRebarPage)
	enum { IDD = IDD_SEGMENT_LONGITUDINAL_REBAR };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CGirderSegmentLongitudinalRebarPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

public:
   void OnEnableDelete(bool canDelete);

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CGirderSegmentLongitudinalRebarPage)
	afx_msg void OnRestoreDefaults();
	virtual BOOL OnInitDialog();
	afx_msg void OnInsertrow();
	afx_msg void OnAppendRow();
	afx_msg void OnRemoveRows();
	afx_msg void OnHelp();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

   void RestoreToLibraryDefaults();

   CRebarMaterialComboBox m_cbRebar;

public:
   CGirderDescLongRebarGrid m_Grid;
};
