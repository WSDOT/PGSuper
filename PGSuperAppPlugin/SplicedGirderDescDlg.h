///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2022  Washington State Department of Transportation
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

#pragma once

// SplicedGirderDescDlg.h : header file
//

#include "SplicedGirderGeneralPage.h"
#include <IFace\ExtendUI.h>


/////////////////////////////////////////////////////////////////////////////
// CSplicedGirderDescDlg

class CSplicedGirderDescDlg : public CPropertySheet, public IEditSplicedGirderData
{
	DECLARE_DYNAMIC(CSplicedGirderDescDlg)

// Construction
public:
	CSplicedGirderDescDlg(const CGirderKey& girderKey,CWnd* pParentWnd = nullptr, UINT iSelectPage = 0);

   // IEditSplicedGirderData
   const CGirderKey& GetGirderKey() { return m_GirderKey; }


   const std::vector<EditSplicedGirderExtension>& GetExtensionPages() const;
   std::vector<EditSplicedGirderExtension>& GetExtensionPages();

// Attributes
public:
   mutable CBridgeDescription2 m_BridgeDescription;
   mutable CSplicedGirderData* m_pGirder;
   CGirderKey m_GirderKey;
   GirderIDType m_GirderID;

   CSplicedGirderGeneralPage    m_General;

   bool m_bApplyToAll;

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSplicedGirderDescDlg)
	public:
	virtual BOOL OnInitDialog() override;
   virtual void DoDataExchange(CDataExchange* pDX) override;
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CSplicedGirderDescDlg();
	virtual INT_PTR DoModal() override;

   // Returns a macro transaction object that contains editing transactions
   // for all the extension pages. The caller is responsble for deleting this object
   txnTransaction* GetExtensionPageTransaction();

protected:
   void Init();
   void CreateExtensionPages();
   void DestroyExtensionPages();


   txnMacroTxn m_Macro;
   std::vector<EditSplicedGirderExtension> m_ExtensionPages;
   void NotifyExtensionPages();


   friend CSplicedGirderGeneralPage;

	// Generated message map functions
	//{{AFX_MSG(CSplicedGirderDescDlg)
		// NOTE - the ClassWizard will add and remove member functions here.
   afx_msg BOOL OnOK();
	//}}AFX_MSG
	afx_msg LRESULT OnKickIdle(WPARAM, LPARAM);
	DECLARE_MESSAGE_MAP()

   CButton m_CheckBox;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.
