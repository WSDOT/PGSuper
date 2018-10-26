///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright (C) 1999  Washington State Department of Transportation
//                     Bridge and Structures Office
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
// 4500 3rd AVE SE - P.O. Box  47340, Olympia, WA 98503, USA or e-mail 
// Bridge_Support@wsdot.wa.gov
///////////////////////////////////////////////////////////////////////

#if !defined(AFX_TRAFFICBARRIERDLG_H__0A07B103_3B5D_11D2_9D4A_00609710E6CE__INCLUDED_)
#define AFX_TRAFFICBARRIERDLG_H__0A07B103_3B5D_11D2_9D4A_00609710E6CE__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#if !defined INCLUDED_MFCTOOLS_METAFILESTATIC_H_
#include <MfcTools\MetaFileStatic.h>
#endif

#include "TrafficBarrierGrid.h"

// TrafficBarrierDlg.h : header file
//
/////////////////////////////////////////////////////////////////////////////
// CTrafficBarrierDlg dialog

class CTrafficBarrierDlg : public CDialog
{
// Construction
public:
	CTrafficBarrierDlg(bool allowEditing,
      CWnd* pParent = NULL);   // standard constructor

	DECLARE_DYNAMIC(CTrafficBarrierDlg)

// Dialog Data
	//{{AFX_DATA(CTrafficBarrierDlg)
	enum { IDD = IDD_TRAFFIC_BARRIER };
	CString	m_Name;
	//}}AFX_DATA

   TrafficBarrierEntry::WeightMethod m_WeightMethod;
   Float64 m_Weight;
   Float64 m_Ec;

   CComPtr<IPoint2dCollection> m_BarrierPoints;

   bool m_bStructurallyContinuous;
   Float64 m_CurbOffset;

   bool m_AllowEditing;

   CTrafficBarrierGrid m_PointsGrid;


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTrafficBarrierDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

public:
   void EnableMoveUp(BOOL bEnable);
   void EnableMoveDown(BOOL bEnable);

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CTrafficBarrierDlg)
   afx_msg LRESULT OnCommandHelp(WPARAM, LPARAM lParam);
	virtual BOOL OnInitDialog();
	afx_msg void OnWeightMethodChanged();
   afx_msg void OnAdd();
   afx_msg void OnDelete();
   afx_msg void OnView();
   afx_msg void OnMoveUp();
   afx_msg void OnMoveDown();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
   CMetaFileStatic m_Picture;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TRAFFICBARRIERDLG_H__0A07B103_3B5D_11D2_9D4A_00609710E6CE__INCLUDED_)
