///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2016  Washington State Department of Transportation
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

// BridgeEditorProfileSettingsPage.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\PGSuperApp.h"
#include "BridgeEditorProfileSettingsPage.h"
#include <EAF\EAFDocument.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CBridgeEditorProfileSettingsPage dialog


CBridgeEditorProfileSettingsPage::CBridgeEditorProfileSettingsPage(CWnd* pParent /*=NULL*/)
	: CPropertyPage(CBridgeEditorProfileSettingsPage::IDD)
{
	//{{AFX_DATA_INIT(CBridgeEditorProfileSettingsPage)
   m_DrawBridge = FALSE;
   m_ShowSchematic = FALSE;
	//}}AFX_DATA_INIT
}


void CBridgeEditorProfileSettingsPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CBridgeEditorProfileSettingsPage)
   DDX_Check(pDX, IDC_DRAW_BRIDGE, m_DrawBridge);
   DDX_Check(pDX, IDC_SHOW_SCHEMATIC, m_ShowSchematic);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CBridgeEditorProfileSettingsPage, CPropertyPage)
	//{{AFX_MSG_MAP(CBridgeEditorProfileSettingsPage)
	ON_COMMAND(ID_HELP, OnHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CBridgeEditorProfileSettingsPage message handlers

void CBridgeEditorProfileSettingsPage::OnHelp() 
{
   EAFHelp( EAFGetDocument()->GetDocumentationSetName(), IDH_BRIDGEVIEW_PROFILE );
}
