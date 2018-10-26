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
// P.O. Box  47340, Olympia, WA 98503, USA or e-mail 
// Bridge_Support@wsdot.wa.gov
///////////////////////////////////////////////////////////////////////

#if !defined(AFX_GIRDERDESCDLG_H__4C050873_311C_11D2_8EB6_006097DF3C68__INCLUDED_)
#define AFX_GIRDERDESCDLG_H__4C050873_311C_11D2_8EB6_006097DF3C68__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// GirderDescDlg.h : header file
//
#include "BridgeDescPrestressPage.h"
#include "BridgeDescShearPage.h"
#include "BridgeDescLongitudinalRebar.h"
#include "BridgeDescLiftingPage.h"
#include "DebondDlg.h"
#include "BridgeDescGirderMaterialsPage.h"

/////////////////////////////////////////////////////////////////////////////
// CGirderDescDlg

class CGirderDescDlg : public CPropertySheet
{
	DECLARE_DYNAMIC(CGirderDescDlg)

// Construction
public:
	CGirderDescDlg(SpanIndexType spanIdx,GirderIndexType gdrIdx,CWnd* pParentWnd = NULL, UINT iSelectPage = 0);

// Attributes
public:
   SpanIndexType   m_CurrentSpanIdx;
   GirderIndexType m_CurrentGirderIdx;

   CGirderDescGeneralPage       m_General;
   CGirderDescPrestressPage     m_Prestress;
   CGirderDescShearPage         m_Shear;
   CGirderDescLongitudinalRebar m_LongRebar;
   CGirderDescLiftingPage       m_Lifting;
   CGirderDescDebondPage        m_Debond;

   std::string m_strGirderName;
   CGirderData m_GirderData;

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGirderDescDlg)
	public:
	virtual BOOL OnInitDialog();
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CGirderDescDlg();
   void DoUpdate();

	// Generated message map functions
protected:
   void Init();
   StrandIndexType GetStraightStrandCount();

   friend CGirderDescGeneralPage;
   friend CGirderDescLiftingPage;
   friend CGirderDescPrestressPage;
   friend CGirderDescShearPage;
   friend CGirderDescDebondPage;
   friend CGirderDescLongitudinalRebar;

	//{{AFX_MSG(CGirderDescDlg)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GIRDERDESCDLG_H__4C050873_311C_11D2_8EB6_006097DF3C68__INCLUDED_)
