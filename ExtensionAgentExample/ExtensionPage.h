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

// CExtensionPage dialog
//
// This is the SIMPLER example property page. CExampleExtensionAgent hands one of
// these back from every CreatePropertyPage() override except
// IEditPierCallback::CreatePropertyPage(IEditPierData*) - see EditPierPage.h for
// that fuller example. This class intentionally does NOT take a data interface
// pointer (IEditBridgeData*, IEditSpanData*, etc.) in its constructor and does NOT
// read or write anything on its parent dialog - it just owns one CString, bound to
// an edit control via DDX_Text, to prove a page can be created, inserted, and have
// its data survive OnOK() without needing anything more elaborate.
//
// A real extension normally wants CEditPierPage's fuller pattern instead: take the
// corresponding IEditXxxData* in the page's own constructor (the same way
// CEditPierPage::CEditPierPage(IEditPierData* pEditPierData) does), read whatever it
// needs from that interface - typically in OnSetActive(), the way
// CEditPierPage::OnSetActive() queries pier girder counts - and hand results back to
// the agent's OnOK(CPropertyPage*, IEditXxxData*) override. This class stays
// deliberately minimal because CExampleExtensionAgent's non-Pier callbacks don't
// need to demonstrate that pattern more than once.

/// @brief Property page returned by every CExampleExtensionAgent::CreatePropertyPage() override
/// except the Pier one - the "simpler" self-contained tab-extension example. See
/// \ref creating_an_extension_agent "Creating an Extension Agent".
class CExtensionPage : public CPropertyPage
{
	DECLARE_DYNAMIC(CExtensionPage)

public:
	CExtensionPage();
	virtual ~CExtensionPage();

   CString m_SampleData; // sample data bound to IDC_EXTENSION_DATA via DDX_Text

// Dialog Data
	enum { IDD = IDD_EXTENSION_PAGE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX) override;    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
};
