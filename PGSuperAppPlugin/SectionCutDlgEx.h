///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright � 1999-2025  Washington State Department of Transportation
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

#if !defined(AFX_SECTIONCUTDLGEX_H__03282213_735B_11D2_9D8B_00609710E6CE__INCLUDED_)
#define AFX_SECTIONCUTDLGEX_H__03282213_735B_11D2_9D8B_00609710E6CE__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// SectionCutDlg.h : header file
//

#include "resource.h"

/////////////////////////////////////////////////////////////////////////////
// CSectionCutDlgEx dialog

class CSectionCutDlgEx : public CDialog
{
// Construction
public:
   CSectionCutDlgEx(IBroker* pBroker,const CGirderKey& girderKey,const pgsPointOfInterest& initialPoi,CWnd* pParent=nullptr);

   pgsPointOfInterest GetPOI();

// Dialog Data
	//{{AFX_DATA(CSectionCutDlgEx)
	enum { IDD = IDD_SECTION_CUT_DIALOG_EX };
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSectionCutDlgEx)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX) override;    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CSectionCutDlgEx)
	virtual BOOL OnInitDialog() override;
	afx_msg void OnHelp();
   afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()

private:
   IBroker* m_pBroker;
   pgsPointOfInterest m_InitialPOI;
   CGirderKey m_GirderKey;

   PoiList m_vPOI;

   CSliderCtrl m_Slider;
   CStatic m_Label;
   int m_SliderPos;

   void UpdateSliderLabel();
   void UpdatePOI();

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SECTIONCUTDLGEX_H__03282213_735B_11D2_9D8B_00609710E6CE__INCLUDED_)
