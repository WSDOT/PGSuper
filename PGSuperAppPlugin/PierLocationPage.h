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

#if !defined(AFX_PierLocationPage_H__B69BAC4A_7AB6_484E_87DA_4007107E740B__INCLUDED_)
#define AFX_PierLocationPage_H__B69BAC4A_7AB6_484E_87DA_4007107E740B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// PierLocationPage.h : header file
//
#include "resource.h"
#include <MFCTools\CacheEdit.h>

class CPierData2;

/////////////////////////////////////////////////////////////////////////////
// CPierLocationPage dialog

class CPierLocationPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CPierLocationPage)

// Construction
public:
	CPierLocationPage();
	~CPierLocationPage();

// Dialog Data
	//{{AFX_DATA(CPierLocationPage)
	enum { IDD = IDD_PIER_LOCATION };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA

   Float64 m_Station;
   pgsTypes::MovePierOption m_MovePierOption;
   std::_tstring m_strOrientation;

   void Init(const CPierData2* pPier);

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CPierLocationPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX) override;    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CPierLocationPage)
	virtual BOOL OnInitDialog() override;
	afx_msg void OnChangeStation();
	afx_msg void OnKillfocusStation();
	afx_msg void OnSetfocusMovePier();
   afx_msg void OnHelp();
   afx_msg void OnErectionStageChanged();
   afx_msg void OnErectionStageChanging();
   afx_msg HBRUSH OnCtlColor(CDC* pDC,CWnd* pWnd,UINT nCtlColor);
   afx_msg void OnBnClickedEditHaunchButton();
	//}}AFX_MSG
   DECLARE_MESSAGE_MAP()

   CComPtr<IStation> m_objStation;
   Float64 m_FromStation;
   Float64 m_NextPierStation;
   Float64 m_PrevPierStation;
   PierIDType      m_PierID;
   PierIndexType   m_PierIdx;
   SpanIndexType   m_nSpans;

   virtual BOOL OnSetActive();
   void UpdateMoveOptionList();

   void FillEventList();
   EventIndexType CreateEvent();
   BOOL IsValidStation(Float64* pStation);

   int m_PrevEventIdx;

   int m_PierFaceCount;

   CEdit m_ctrlBackSlabOffset;
   CEdit m_ctrlAheadSlabOffset;

   void UpdateHaunchAndCamberControls();
   void UpdateHaunchAndCamberData(CDataExchange* pDX);
   void DisableHaunchAndCamberControls();

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PierLocationPage_H__B69BAC4A_7AB6_484E_87DA_4007107E740B__INCLUDED_)
