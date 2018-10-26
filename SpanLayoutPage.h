///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2011  Washington State Department of Transportation
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

#if !defined(AFX_SpanLayoutPage_H__D16283C9_71E7_4544_B96C_1B1D9EC5E7F5__INCLUDED_)
#define AFX_SpanLayoutPage_H__D16283C9_71E7_4544_B96C_1B1D9EC5E7F5__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SpanLayoutPage.h : header file
//
#include "PGSuperAppPlugin\resource.h"

class CSpanData;

/////////////////////////////////////////////////////////////////////////////
// CSpanLayoutPage dialog

class CSpanLayoutPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CSpanLayoutPage)

// Construction
public:
	CSpanLayoutPage();
	~CSpanLayoutPage();

// Dialog Data
	//{{AFX_DATA(CSpanLayoutPage)
	enum { IDD = IDD_SPANLAYOUT };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA

   double GetSpanLength();

   void Init(const CSpanData* pSpan);

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CSpanLayoutPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   afx_msg void OnHelp();
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CSpanLayoutPage)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

   double m_SpanLength;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SpanLayoutPage_H__D16283C9_71E7_4544_B96C_1B1D9EC5E7F5__INCLUDED_)
