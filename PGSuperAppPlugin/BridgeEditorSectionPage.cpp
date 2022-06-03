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

// BridgeEditorSectionPage.cpp : implementation file
//

#include "stdafx.h"
#include "PGSuperApp.h"
#include "BridgeEditorSectionPage.h"
#include <EAF\EAFDocument.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CBridgeEditorSectionPage property page

IMPLEMENT_DYNCREATE(CBridgeEditorSectionPage, CPropertyPage)

CBridgeEditorSectionPage::CBridgeEditorSectionPage() : CPropertyPage(CBridgeEditorSectionPage::IDD)
{
	//{{AFX_DATA_INIT(CBridgeEditorSectionPage)
	m_LabelGirders = FALSE;
	m_ShowDimensions = FALSE;
   m_ShowRwCrossSection = FALSE;
	//}}AFX_DATA_INIT
}

CBridgeEditorSectionPage::~CBridgeEditorSectionPage()
{
}

void CBridgeEditorSectionPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CBridgeEditorSectionPage)
	DDX_Check(pDX, IDC_LABEL_GIRDERS, m_LabelGirders);
	DDX_Check(pDX, IDC_SHOW_DIMENSIONS, m_ShowDimensions);
	DDX_Check(pDX, IDC_SHOW_RW_CROSSSECTION, m_ShowRwCrossSection);
   
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CBridgeEditorSectionPage, CPropertyPage)
	//{{AFX_MSG_MAP(CBridgeEditorSectionPage)
	ON_COMMAND(ID_HELP, OnHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CBridgeEditorSectionPage message handlers

void CBridgeEditorSectionPage::OnHelp() 
{
   EAFHelp( EAFGetDocument()->GetDocumentationSetName(), IDH_BRIDGEVIEW_SECTION );
}
