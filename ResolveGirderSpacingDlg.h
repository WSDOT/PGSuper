///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2010  Washington State Department of Transportation
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

#if !defined(AFX_RESOLVEGIRDERSPACINGDLG_H__EAB70988_BE21_48AF_80B2_7E4CDEF8BBD0__INCLUDED_)
#define AFX_RESOLVEGIRDERSPACINGDLG_H__EAB70988_BE21_48AF_80B2_7E4CDEF8BBD0__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ResolveGirderSpacingDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CResolveGirderSpacingDlg dialog

class CResolveGirderSpacingDlg : public CDialog
{
// Construction
public:
	CResolveGirderSpacingDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CResolveGirderSpacingDlg)
	enum { IDD = IDD_RESOLVE_GIRDER_SPACING };
	CComboBox	m_cbDatum;
	CComboBox	m_cbList;
	//}}AFX_DATA

   int m_ItemIdx;

   std::string m_strSpacings; // \n delimited list of spacings
   DWORD m_MeasurementDatum;
   bool m_RestrictSpacing; // connection type restricts spacing locations

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CResolveGirderSpacingDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CResolveGirderSpacingDlg)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RESOLVEGIRDERSPACINGDLG_H__EAB70988_BE21_48AF_80B2_7E4CDEF8BBD0__INCLUDED_)
