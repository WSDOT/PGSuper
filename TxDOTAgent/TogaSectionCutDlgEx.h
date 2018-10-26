///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2017  Washington State Department of Transportation
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

#if !defined(AFX_TogaSectionCutDlgEx_H__03282213_735B_11D2_9D8B_00609710E6CE__INCLUDED_)
#define AFX_TogaSectionCutDlgEx_H__03282213_735B_11D2_9D8B_00609710E6CE__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// SectionCutDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CTogaSectionCutDlgEx dialog

class CTogaSectionCutDlgEx : public CDialog
{
// Construction
public:
   CTogaSectionCutDlgEx(CWnd* pParent=nullptr);   // standard constructor

	CTogaSectionCutDlgEx(IndexType nHarpPoints,Float64 value, Float64 lowerBound, Float64 upperBound, 
		CTxDOTOptionalDesignGirderViewPage::CutLocation, CWnd* pParent = nullptr);

   void SetValue(Float64 value);
   Float64 GetValue()const;
   void SetCutLocation(CTxDOTOptionalDesignGirderViewPage::CutLocation location);
   CTxDOTOptionalDesignGirderViewPage::CutLocation GetCutLocation() const;
   void SetBounds(Float64 lowerBound, Float64 upperBound);
   void GetBounds(Float64* plowerBound, Float64* pupperBound);
// Dialog Data
	//{{AFX_DATA(CTogaSectionCutDlgEx)
	enum { IDD = IDD_SECTION_CUT_DIALOG_EX };
	int		m_CutIndex;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTogaSectionCutDlgEx)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX) override;    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CTogaSectionCutDlgEx)
	virtual BOOL OnInitDialog() override;
	afx_msg void OnGirderMiddle();
	afx_msg void OnLeftEnd();
	afx_msg void OnLeftHarp();
	afx_msg void OnRightEnd();
	afx_msg void OnRightHarp();
	afx_msg void OnUserCut();
	afx_msg void OnHelp();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()

private:
   IndexType m_nHarpPoints;
   Float64	 m_Value;
   Float64   m_LowerBound;
   Float64   m_UpperBound;
   CTxDOTOptionalDesignGirderViewPage::CutLocation m_CutLocation;

   void Update();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TogaSectionCutDlgEx_H__03282213_735B_11D2_9D8B_00609710E6CE__INCLUDED_)
