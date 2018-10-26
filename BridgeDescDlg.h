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

#if !defined(AFX_BRIDGEDESCDLG_H__4C050873_311C_11D2_8EB6_006097DF3C68__INCLUDED_)
#define AFX_BRIDGEDESCDLG_H__4C050873_311C_11D2_8EB6_006097DF3C68__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// BridgeDescDlg.h : header file
//
#include <PgsExt\BridgeDescription.h>
#include "BridgeDescGeneralPage.h"
#include "BridgeDescFramingPage.h"
#include "BridgeDescRailingSystemPage.h"
#include "BridgeDescDeckDetailsPage.h"
#include "BridgeDescDeckReinforcementPage.h"
#include "BridgeDescEnvironmental.h"

/////////////////////////////////////////////////////////////////////////////
// CBridgeDescDlg

class CBridgeDescDlg : public CPropertySheet
{
	DECLARE_DYNAMIC(CBridgeDescDlg)

// Construction
public:
	CBridgeDescDlg(CWnd* pParentWnd = NULL, UINT iSelectPage = 0);

// Attributes
public:
   void SetBridgeDescription(const CBridgeDescription& bridgeDesc);
   const CBridgeDescription& GetBridgeDescription();

   CBridgeDescGeneralPage           m_GeneralPage;
   CBridgeDescFramingPage           m_FramingPage;
   CBridgeDescRailingSystemPage     m_RailingSystemPage;
   CBridgeDescDeckDetailsPage       m_DeckDetailsPage;
   CBridgeDescDeckReinforcementPage m_DeckRebarPage;
   CBridgeDescEnvironmental         m_EnvironmentalPage;

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CBridgeDescDlg)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CBridgeDescDlg();

	// Generated message map functions
protected:
   void Init();
   CBridgeDescription m_BridgeDesc;

   friend CBridgeDescGeneralPage;
   friend CBridgeDescFramingPage;
   friend CBridgeDescRailingSystemPage;
   friend CBridgeDescDeckDetailsPage;
   friend CBridgeDescDeckReinforcementPage;
   friend CBridgeDescEnvironmental;
   friend CBridgeDescDeckRebarGrid;
   friend CBridgeDescFramingGrid;

	//{{AFX_MSG(CBridgeDescDlg)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_BRIDGEDESCDLG_H__4C050873_311C_11D2_8EB6_006097DF3C68__INCLUDED_)
