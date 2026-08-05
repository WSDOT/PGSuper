///////////////////////////////////////////////////////////////////////
// ExtensionAgentExample - Extension Agent Example Project for PGSuper
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

#pragma once

#include "resource.h"
class IEditPierData;

// CEditPierPage dialog
//
// This is the FULLER example property page - it's the only one of
// CExampleExtensionAgent's callback pages that actually talks to its parent
// dialog's data. Three things worth studying here:
//  - The constructor takes the same IEditPierData* that
//    IEditPierCallback::CreatePropertyPage(IEditPierData*) was handed, and holds
//    onto it in m_pEditPierData, exactly the way a real extension would receive
//    read access to the Pier dialog's data.
//  - OnSetActive() reads live data through that interface
//    (pEditPierData->GetGirderCount(...)) and displays it - showing how a page can
//    react to becoming the active tab, not just to being constructed.
//  - OnBnClickedCheck() responds to a normal control notification the same way any
//    other MFC dialog would.
//  CExampleExtensionAgent::OnOK(CPropertyPage*, IEditPierData*) is the other half of
//  this pattern - it casts pPage back to CEditPierPage* and reads m_bCheck to hand
//  the result back to the agent.
//
// Every other callback on CExampleExtensionAgent instead returns a plain
// CExtensionPage (see ExtensionPage.h) - a deliberately simpler page that does NOT
// take a data interface pointer, because that pattern only needs to be demonstrated
// once. A real extension for e.g. IEditBridgeCallback would normally follow this
// class's shape instead, taking an IEditBridgeData* in its own constructor.

/// @brief Property page returned by CExampleExtensionAgent::CreatePropertyPage(IEditPierData*) - the
/// "fuller" tab-extension example that reads live parent-dialog data. See
/// \ref creating_an_extension_agent "Creating an Extension Agent".
class CEditPierPage : public CPropertyPage
{
	DECLARE_DYNAMIC(CEditPierPage)

public:
	CEditPierPage(IEditPierData* pEditPierData);
	virtual ~CEditPierPage();

   bool m_bCheck;

// Dialog Data
	enum { IDD = IDD_EDIT_PIER_PAGE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX) override;    // DDX/DDV support

   IEditPierData* m_pEditPierData;

	DECLARE_MESSAGE_MAP()
public:
   afx_msg void OnBnClickedCheck();
   virtual BOOL OnSetActive() override;
};
