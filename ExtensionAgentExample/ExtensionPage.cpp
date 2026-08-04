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

// ExtensionPage.cpp : implementation file
//

#include "stdafx.h"
#include "ExtensionAgent.h"
#include "resource.h"
#include "ExtensionPage.h"

// CExtensionPage dialog

IMPLEMENT_DYNAMIC(CExtensionPage, CPropertyPage)

CExtensionPage::CExtensionPage()
	: CPropertyPage(CExtensionPage::IDD)
{
   m_psp.dwFlags |= PSP_HASHELP;
}

CExtensionPage::~CExtensionPage()
{
}

void CExtensionPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);

   DDX_Text(pDX, IDC_EXTENSION_DATA, m_SampleData);
}

BEGIN_MESSAGE_MAP(CExtensionPage, CPropertyPage)
END_MESSAGE_MAP()
