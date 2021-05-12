///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2021  Washington State Department of Transportation
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

#if !defined(AFX_CONNECTIONENTRYDLG_H__3C5D7EF3_3089_11D2_9D3D_00609710E6CE__INCLUDED_)
#define AFX_CONNECTIONENTRYDLG_H__3C5D7EF3_3089_11D2_9D3D_00609710E6CE__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// ConnectionEntryDlg.h : header file
//
#if !defined INCLUDED_PSGLIB_CONNECTIONLIBRARYENTRY_H_
#include <psgLib\ConnectionLibraryEntry.h>
#endif

#if !defined INCLUDED_MFCTOOLS_METAFILESTATIC_H_
#include <MfcTools\MetaFileStatic.h>
#endif

/////////////////////////////////////////////////////////////////////////////
// CConnectionEntryDlg dialog

class CConnectionEntryDlg : public CDialog
{
// Construction
public:
	CConnectionEntryDlg(bool allowEditing, CWnd* pParent = nullptr);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CConnectionEntryDlg)
	enum { IDD = IDD_CONNECTION_ENTRY };
	CString	m_Name;
	//}}AFX_DATA
   Float64 m_GirderEndDistance;
   Float64 m_GirderBearingOffset;
   Float64 m_DiaphragmHeight;
   Float64 m_DiaphragmWidth;
   ConnectionLibraryEntry::DiaphragmLoadType m_DiaphragmLoadType;
   ConnectionLibraryEntry::EndDistanceMeasurementType m_EndDistanceMeasurementType;
   ConnectionLibraryEntry::BearingOffsetMeasurementType m_BearingOffsetMeasurementType;
   Float64 m_DiaphragmLoadLocation;

   bool m_bAllowEditing;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CConnectionEntryDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CConnectionEntryDlg)
	virtual BOOL OnInitDialog();
   afx_msg void OnHelp();
	afx_msg void OnApplyDrToBearing();
	afx_msg void OnApplyDrToBeam();
	afx_msg void OnDontApply();
	afx_msg void OnSelchangeEndDistanceMeasure();
	afx_msg void OnSelchangeBearingOffsetMeasure();
   afx_msg HBRUSH OnCtlColor(CDC* pDC,CWnd* pWnd,UINT nCtlColor);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
   void UpdateButtons();
   void EnableDLEdit(bool flag);
   void UpdateConnectionPicture();

   void FillBearingOffsetComboBox();
   void FillEndDistanceComboBox();

   CString GetImageName(ConnectionLibraryEntry::BearingOffsetMeasurementType brgOffsetType,ConnectionLibraryEntry::EndDistanceMeasurementType endType);

   CMetaFileStatic m_ConnectionPicture;
   CBrush m_WhiteBrush;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CONNECTIONENTRYDLG_H__3C5D7EF3_3089_11D2_9D3D_00609710E6CE__INCLUDED_)
