///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2023  Washington State Department of Transportation
//                        Bridge and Structures Office
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the Alternate Route Open Source License as 
// published by the Washington State Department of Transportation, 
// Bridge and Structures Office.
//
// This program is distributed in the hope that it will be useful, but 
// distribution is AS IS, WITHOUT ANY WARRANTY; without even the implied 
// warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See 
// the Alternate Route Open Source License for more details.
//
// You should have received a copy of the Alternate Route Open Source 
// License along with this program; if not, write to the Washington 
// State Department of Transportation, Bridge and Structures Office, 
// P.O. Box  47340, Olympia, WA 98503, USA or e-mail 
// Bridge_Support@wsdot.wa.gov
///////////////////////////////////////////////////////////////////////


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
	CLiveLoadDlg(bool allowEditing, CWnd* pParent = nullptr);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CLiveLoadDlg)
	enum { IDD = IDD_LIVE_LOAD_ENTRY };
	CString	m_EntryName;
	Float64	m_LaneLoad;
   Float64 m_LaneLoadSpanLength;
	BOOL	m_IsNotional;
	//}}AFX_DATA
	LiveLoadLibraryEntry::LiveLoadConfigurationType m_ConfigType;
   pgsTypes::LiveLoadApplicabilityType m_UsageType;

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
   void UpdateTruckDimensions();
protected:

	// Generated message map functions
	//{{AFX_MSG(CLiveLoadDlg)
	virtual BOOL OnInitDialog();
   afx_msg void OnHelp();
	afx_msg void OnAdd();
	afx_msg void OnDelete();
	afx_msg void OnSelchangeConfigType();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
   CLiveLoadAxleGrid  m_Grid;

   bool m_bAllowEditing;

   void UpdateConfig();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LIVELOADDLG_H__F9C9AC38_E2D9_4ABE_872F_2984E9B1C8BF__INCLUDED_)
