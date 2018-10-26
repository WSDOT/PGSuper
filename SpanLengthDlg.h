///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2015  Washington State Department of Transportation
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

#if !defined(AFX_SPANLENGTHDLG_H__5D06E864_330C_492A_B5A0_ECD2C93FB973__INCLUDED_)
#define AFX_SPANLENGTHDLG_H__5D06E864_330C_492A_B5A0_ECD2C93FB973__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SpanLengthDlg.h : header file
//
#include "PGSuperAppPlugin\resource.h"
#include "SpanLengthGrid.h"

/////////////////////////////////////////////////////////////////////////////
// CSpanLengthDlg dialog

class CSpanLengthDlg : public CDialog
{
// Construction
public:
	CSpanLengthDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CSpanLengthDlg)
	enum { IDD = IDD_SPAN_LENGTHS };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSpanLengthDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
   public:
   std::vector<Float64> m_SpanLengths;
   int m_PierIdx;

protected:
   CSpanLengthGrid m_Grid;

	// Generated message map functions
	//{{AFX_MSG(CSpanLengthDlg)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SPANLENGTHDLG_H__5D06E864_330C_492A_B5A0_ECD2C93FB973__INCLUDED_)
