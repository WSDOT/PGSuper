#if !defined(AFX_STRANDGRIDLOCATION_H__0C0A1B3D_5065_4A3D_9299_6FD780CC30E3__INCLUDED_)
#define AFX_STRANDGRIDLOCATION_H__0C0A1B3D_5065_4A3D_9299_6FD780CC30E3__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// StrandGridLocation.h : header file
//
#include "GirderGlobalStrandGrid.h"

/////////////////////////////////////////////////////////////////////////////
// CStrandGridLocation dialog

class CStrandGridLocation : public CDialog
{
// Construction
public:
	CStrandGridLocation(CWnd* pParent = NULL);   // standard constructor

   void SetEntry(const CGirderGlobalStrandGrid::GlobalStrandGridEntry& Entry, bool UseHarpedGrid);
   CGirderGlobalStrandGrid::GlobalStrandGridEntry GetEntry();

// Dialog Data
	//{{AFX_DATA(CStrandGridLocation)
	enum { IDD = IDD_STRAND_DEFINITION };
	int		m_StrandType;
	CString	m_UnitString;
	double	m_HpX;
	double	m_HpY;
	double	m_EndX;
	double	m_EndY;
	BOOL	m_AllowDebonding;
	//}}AFX_DATA
   afx_msg LRESULT OnCommandHelp(WPARAM, LPARAM lParam);

   ROWCOL m_Row;
   bool m_UseHarpedGrid;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CStrandGridLocation)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CStrandGridLocation)
	virtual BOOL OnInitDialog();
	afx_msg void OnSelchangeStrandType();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
   void EnableEndBox(BOOL enable);
   void ShowDebondCtrl(BOOL show);

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STRANDGRIDLOCATION_H__0C0A1B3D_5065_4A3D_9299_6FD780CC30E3__INCLUDED_)
