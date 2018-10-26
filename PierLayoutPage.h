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

#if !defined(AFX_PIERLAYOUTPAGE_H__B69BAC4A_7AB6_484E_87DA_4007107E740B__INCLUDED_)
#define AFX_PIERLAYOUTPAGE_H__B69BAC4A_7AB6_484E_87DA_4007107E740B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// PierLayoutPage.h : header file
//
#include "PGSuperAppPlugin\resource.h"

class CPierData;

/////////////////////////////////////////////////////////////////////////////
// CPierLayoutPage dialog

class CPierLayoutPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CPierLayoutPage)

// Construction
public:
	CPierLayoutPage();
	~CPierLayoutPage();

// Dialog Data
	//{{AFX_DATA(CPierLayoutPage)
	enum { IDD = IDD_PIERLAYOUT };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA

   Float64 m_Station;
   pgsTypes::MovePierOption m_MovePierOption;
   std::_tstring m_strOrientation;

   void Init(const CPierData* pPier);

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CPierLayoutPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CPierLayoutPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnChangeStation();
	afx_msg void OnKillfocusStation();
	afx_msg void OnSetfocusMovePier();
   afx_msg void OnHelp();
   afx_msg HBRUSH OnCtlColor(CDC* pDC,CWnd* pWnd,UINT nCtlColor);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

   CComPtr<IStation> m_objStation;
   Float64 m_FromStation;
   Float64 m_NextPierStation;
   Float64 m_PrevPierStation;
   PierIndexType   m_PierIdx;
   SpanIndexType   m_nSpans;

   void UpdateMoveOptionList();

   void DisableAll();
   void UpdateChildWindowState();

   BOOL IsValidStation(Float64* pStation);
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PIERLAYOUTPAGE_H__B69BAC4A_7AB6_484E_87DA_4007107E740B__INCLUDED_)
