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

#pragma once

#include "GeneralRatingOptionsPage.h"
#include "DesignRatingPage.h"
#include "LegalRatingPage.h"
#include "PermitRatingPage.h"

#include <IFace\ExtendUI.h>

// CRatingOptionsDlg

class CRatingOptionsDlg : public CPropertySheet, public IEditLoadRatingOptions
{
	DECLARE_DYNAMIC(CRatingOptionsDlg)

public:
	CRatingOptionsDlg(CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
	virtual ~CRatingOptionsDlg();

   // IEditLoadRatingOptions
public:
   virtual void LRDummy() {}

   CGeneralRatingOptionsPage m_GeneralPage;
   CDesignRatingPage m_DesignPage;
   CLegalRatingPage m_LegalPage;
   CPermitRatingPage m_PermitPage;

   BOOL OnInitDialog();

   void GetLoadFactorToolTip(CString& strTip,pgsTypes::LimitState ls);
   void GetLoadFactorToolTip(CString& strTip,pgsTypes::LimitState ls,pgsTypes::SpecialPermitType specialPermitType);

// Implementation
public:
	virtual INT_PTR DoModal();


   // Returns a macro transaction object that contains editing transactions
   // for all the extension pages. The caller is responsble for deleting this object
   txnTransaction* GetExtensionPageTransaction();

protected:
	DECLARE_MESSAGE_MAP()

	afx_msg LRESULT OnKickIdle(WPARAM, LPARAM);

   void Init();
   void CreateExtensionPages();
   void DestroyExtensionPages();

   txnMacroTxn m_Macro;
   std::set<EditLoadRatingOptionsExtension> m_ExtensionPages;
   void NotifyExtensionPages();
};


