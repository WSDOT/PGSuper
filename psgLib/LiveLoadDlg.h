#if !defined(AFX_LIVELOADDLG_H__F9C9AC38_E2D9_4ABE_872F_2984E9B1C8BF__INCLUDED_)
#define AFX_LIVELOADDLG_H__F9C9AC38_E2D9_4ABE_872F_2984E9B1C8BF__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// LiveLoadDlg.h : header file
//
#include <Units\sysUnits.h>
#include "LiveLoadAxleGrid.h"
#include <psgLib\LiveLoadLibraryEntry.h>

/////////////////////////////////////////////////////////////////////////////
// CLiveLoadDlg dialog

class CLiveLoadDlg : public CDialog
{
   friend CLiveLoadAxleGrid;
// Construction
public:
	CLiveLoadDlg(bool allowEditing, CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CLiveLoadDlg)
	enum { IDD = IDD_LIVE_LOAD_ENTRY };
	CString	m_EntryName;
	Float64	m_LaneLoad;
   Float64 m_LaneLoadSpanLength;
	BOOL	m_IsNotional;
	//}}AFX_DATA
	LiveLoadLibraryEntry::LiveLoadConfigurationType m_ConfigType;
   LiveLoadLibraryEntry::LiveLoadApplicabilityType m_UsageType;

   Float64 m_MaxVariableAxleSpacing;
   AxleIndexType m_VariableAxleIndex;
   LiveLoadLibraryEntry::AxleContainer m_Axles;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CLiveLoadDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
public:
   void InitData(LiveLoadLibraryEntry* entry);
   void OnEnableDelete(bool canDelete);
protected:

	// Generated message map functions
	//{{AFX_MSG(CLiveLoadDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnAdd();
	afx_msg void OnDelete();
	afx_msg void OnSelchangeConfigType();
   afx_msg LRESULT OnCommandHelp(WPARAM, LPARAM lParam);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
   CLiveLoadAxleGrid  m_Grid;

   bool m_AllowEditing;

   void UpdateConfig();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LIVELOADDLG_H__F9C9AC38_E2D9_4ABE_872F_2984E9B1C8BF__INCLUDED_)
