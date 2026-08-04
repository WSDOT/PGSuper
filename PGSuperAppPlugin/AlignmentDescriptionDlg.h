///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2026  Washington State Department of Transportation
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

#if !defined(AFX_ALIGNMENTDESCRIPTIONDLG_H__CD668187_60ED_45E3_8387_F1A47F5AB264__INCLUDED_)
#define AFX_ALIGNMENTDESCRIPTIONDLG_H__CD668187_60ED_45E3_8387_F1A47F5AB264__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// AlignmentDescriptionDlg.h : header file
//

#include "HorizontalAlignmentPage.h"
#include "ProfilePage.h"
#include "CrownSlopePage.h"

#include <IFace\ExtendUI.h>
#include <EAF\MacroTxn.h>
#include "ExtensionPageManager.h"

/////////////////////////////////////////////////////////////////////////////
// CAlignmentDescriptionDlg

class CAlignmentDescriptionDlg : public CPropertySheet, public IEditAlignmentData
{
	DECLARE_DYNAMIC(CAlignmentDescriptionDlg)

// Construction
public:
	CAlignmentDescriptionDlg(UINT nIDCaption, std::shared_ptr<WBFL::EAF::Broker> pBroker,CWnd* pParentWnd = nullptr, UINT iSelectPage = 0);
	CAlignmentDescriptionDlg(LPCTSTR pszCaption, std::shared_ptr<WBFL::EAF::Broker> pBroker,CWnd* pParentWnd = nullptr, UINT iSelectPage = 0);

   // IEditAlignmentData
   virtual void EADummy() override {}

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAlignmentDescriptionDlg)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CAlignmentDescriptionDlg();
   virtual INT_PTR DoModal() override;

   // Returns a macro transaction object that contains editing transactions
   // for all the extension pages. The caller is responsible for deleting this object
   std::unique_ptr<WBFL::EAF::Transaction> GetExtensionPageTransaction();

   CPropertyPage* FindPage(LPCTSTR name) const { return m_PageManager.FindPage(name); }

   CHorizontalAlignmentPage m_AlignmentPage;
   CProfilePage m_ProfilePage;
   CCrownSlopePage m_CrownSlopePage;

	afx_msg LRESULT OnKickIdle(WPARAM, LPARAM);

	// Generated message map functions
protected:
   std::shared_ptr<WBFL::EAF::Broker> m_pBroker;
	void Init();
   void CreateExtensionPages();
   void DestroyExtensionPages();
	//{{AFX_MSG(CAlignmentDescriptionDlg)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

   WBFL::EAF::MacroTxn m_Macro;
   CExtensionPageManager m_PageManager;
   std::vector<std::pair<IEditAlignmentCallback*,CPropertyPage*>> m_ExtensionPages;
   void NotifyExtensionPages();

   friend CHorizontalAlignmentPage;
   friend CProfilePage;
   friend CCrownSlopePage;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ALIGNMENTDESCRIPTIONDLG_H__CD668187_60ED_45E3_8387_F1A47F5AB264__INCLUDED_)
