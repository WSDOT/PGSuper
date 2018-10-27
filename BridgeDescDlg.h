///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2018  Washington State Department of Transportation
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
#include <PgsExt\BridgeDescription2.h>
#include "BridgeDescGeneralPage.h"
#include "BridgeDescFramingPage.h"
#include "BridgeDescRailingSystemPage.h"
#include "BridgeDescDeckDetailsPage.h"
#include "BridgeDescDeckReinforcementPage.h"
#include "BridgeDescEnvironmental.h"
#include "PGSuperAppPlugin\BridgeDescrBearings.h"

#include <IFace\ExtendUI.h>

/////////////////////////////////////////////////////////////////////////////
// CBridgeDescDlg

class CBridgeDescDlg : public CPropertySheet, public IEditBridgeData
{
	DECLARE_DYNAMIC(CBridgeDescDlg)

// Construction
public:
	CBridgeDescDlg(const CBridgeDescription2& bridgeDesc,CWnd* pParentWnd = nullptr, UINT iSelectPage = 0);

// IEditBridgeData
public:
   virtual void EBDummy() {};

// Attributes
public:
   void SetBridgeDescription(const CBridgeDescription2& bridgeDesc);
   const CBridgeDescription2& GetBridgeDescription();

   CBridgeDescGeneralPage           m_GeneralPage;
   CBridgeDescFramingPage           m_FramingPage;
   CBridgeDescRailingSystemPage     m_RailingSystemPage;
   CBridgeDescDeckDetailsPage       m_DeckDetailsPage;
   CBridgeDescDeckReinforcementPage m_DeckRebarPage;
   CBridgeDescEnvironmental         m_EnvironmentalPage;
   CBridgeDescrBearings             m_BridgeDescrBearings;

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CBridgeDescDlg)
	virtual BOOL OnInitDialog();
   afx_msg BOOL OnOK();
   //}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CBridgeDescDlg();

   virtual INT_PTR DoModal();

   // Returns a macro transaction object that contains editing transactions
   // for all the extension pages. The caller is responsble for deleting this object
   txnTransaction* GetExtensionPageTransaction();

   const std::vector<EditBridgeExtension>& GetExtensionPages() const;
   std::vector<EditBridgeExtension>& GetExtensionPages();

	// Generated message map functions
protected:
   void Init();
   void CreateExtensionPages();
   void DestroyExtensionPages();

   CBridgeDescription2 m_BridgeDesc; // this is the bridge we are operating on

   friend CBridgeDescGeneralPage;
   friend CBridgeDescFramingPage;
   friend CBridgeDescRailingSystemPage;
   friend CBridgeDescDeckDetailsPage;
   friend CBridgeDescDeckReinforcementPage;
   friend CBridgeDescEnvironmental;
   friend CBridgeDescDeckRebarGrid;
   friend CBridgeDescFramingGrid;
   friend CBridgeDescrBearings;

   txnMacroTxn m_Macro;
   std::vector<EditBridgeExtension> m_ExtensionPages;
   void NotifyExtensionPages();

	//{{AFX_MSG(CBridgeDescDlg)
		// NOTE - the ClassWizard will add and remove member functions here.
   virtual void DoDataExchange(CDataExchange* pDX);
	//}}AFX_MSG
	afx_msg LRESULT OnKickIdle(WPARAM, LPARAM);
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_BRIDGEDESCDLG_H__4C050873_311C_11D2_8EB6_006097DF3C68__INCLUDED_)
