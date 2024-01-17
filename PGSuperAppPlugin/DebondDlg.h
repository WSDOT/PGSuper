///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2024  Washington State Department of Transportation
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

#if !defined(AFX_DEBONDDLG_H__D11812B2_15B3_4AFE_A543_73AD0B3778A2__INCLUDED_)
#define AFX_DEBONDDLG_H__D11812B2_15B3_4AFE_A543_73AD0B3778A2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DebondDlg.h : header file
//

#include "resource.h"
#include "DebondGrid.h"

#include <WBFLTools.h>
#include <Graphing/PointMapper.h>

class CGirderDescDlg;

/////////////////////////////////////////////////////////////////////////////
// CGirderDescDebondPage dialog

class CGirderDescDebondPage : public CPropertyPage, public IDebondGridParent
{
   friend CGirderDescDlg;
// Construction
public:
	CGirderDescDebondPage();

// Dialog Data
	//{{AFX_DATA(CGirderDescDebondPage)
	enum { IDD = IDD_GIRDERDESC_DEBOND };
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGirderDescDebondPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

public:
   virtual LPCTSTR GetGirderName();
   virtual void OnChange();
   virtual const CSegmentKey& GetSegmentKey();
   virtual ConfigStrandFillVector ComputeStrandFillVector(pgsTypes::StrandType type);

   const std::vector<CDebondData>& GetDebondInfo() const;

// Implementation
protected:
   CComPtr<IIndexArray> m_Debondables;
   Float64         m_Radius;

   UINT m_nHelpID;

   CGirderDescDebondGrid m_Grid;

   StrandIndexType GetStrandCount();
   StrandIndexType GetNumPermanentStrands();
   void DrawShape(CDC* pDC,IShape* shape, WBFL::Graphing::PointMapper& mapper);
   void DrawStrands(CDC* pDC, WBFL::Graphing::PointMapper& mapper,Float64 Xadjustment);


	// Generated message map functions
	//{{AFX_MSG(CGirderDescDebondPage)
	virtual BOOL OnInitDialog();
	virtual BOOL OnSetActive();
	virtual BOOL OnKillActive();
	afx_msg void OnPaint();
	afx_msg void OnSymmetricDebond();
	afx_msg void OnHelp();
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
   //}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DEBONDDLG_H__D11812B2_15B3_4AFE_A543_73AD0B3778A2__INCLUDED_)
