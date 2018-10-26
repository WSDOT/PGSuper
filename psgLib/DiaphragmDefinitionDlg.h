#if !defined(AFX_DIAPHRAGMDEFINITIONDLG_H__DBEED739_8347_40B7_9623_23C5E1C0B4C8__INCLUDED_)
#define AFX_DIAPHRAGMDEFINITIONDLG_H__DBEED739_8347_40B7_9623_23C5E1C0B4C8__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DiaphragmDefinitionDlg.h : header file
//

#include "resource.h"
#include <psgLib\GirderLibraryEntry.h>

/////////////////////////////////////////////////////////////////////////////
// CDiaphragmDefinitionDlg dialog

class CDiaphragmDefinitionDlg : public CDialog
{
// Construction
public:
	CDiaphragmDefinitionDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CDiaphragmDefinitionDlg)
	enum { IDD = IDD_INTERMEDATE_DIAPHRAGM };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

   GirderLibraryEntry::DiaphragmLayoutRule m_Rule;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDiaphragmDefinitionDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDiaphragmDefinitionDlg)
	afx_msg void OnDiaphragmTypeChanged();
   afx_msg void OnMeasurementTypeChanged();
   afx_msg void OnMethodChanged();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIAPHRAGMDEFINITIONDLG_H__DBEED739_8347_40B7_9623_23C5E1C0B4C8__INCLUDED_)
