///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2020  Washington State Department of Transportation
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

// BridgeEditorAlignmentSettingsPage.cpp : implementation file
//

#include "stdafx.h"
#include "PGSuperApp.h"
#include "BridgeEditorAlignmentSettingsPage.h"
#include <EAF\EAFDocument.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CBridgeEditorAlignmentSettingsPage dialog


CBridgeEditorAlignmentSettingsPage::CBridgeEditorAlignmentSettingsPage(CWnd* pParent /*=nullptr*/)
	: CPropertyPage(CBridgeEditorAlignmentSettingsPage::IDD)
{
	//{{AFX_DATA_INIT(CBridgeEditorAlignmentSettingsPage)
   m_DrawBridge = FALSE;
   m_NorthUp = FALSE;
	//}}AFX_DATA_INIT
}


void CBridgeEditorAlignmentSettingsPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CBridgeEditorAlignmentSettingsPage)
   DDX_Check(pDX, IDC_DRAW_BRIDGE, m_DrawBridge);
	DDX_Check(pDX, IDC_NORTH_UP, m_NorthUp);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CBridgeEditorAlignmentSettingsPage, CPropertyPage)
	//{{AFX_MSG_MAP(CBridgeEditorAlignmentSettingsPage)
	ON_COMMAND(ID_HELP, OnHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CBridgeEditorAlignmentSettingsPage message handlers

void CBridgeEditorAlignmentSettingsPage::OnHelp() 
{
   EAFHelp( EAFGetDocument()->GetDocumentationSetName(), IDH_BRIDGEVIEW_ALIGNMENT );
}
