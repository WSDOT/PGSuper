///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2010  Washington State Department of Transportation
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

// BridgeEditorPlanSettingsPage.cpp : implementation file
//

#include "stdafx.h"
#include "PGSuperAppPlugin\PGSuperApp.h"
#include "BridgeEditorPlanSettingsPage.h"
#include "HtmlHelp\HelpTopics.hh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CBridgeEditorPlanSettingsPage dialog


CBridgeEditorPlanSettingsPage::CBridgeEditorPlanSettingsPage(CWnd* pParent /*=NULL*/)
	: CPropertyPage(CBridgeEditorPlanSettingsPage::IDD)
{
	//{{AFX_DATA_INIT(CBridgeEditorPlanSettingsPage)
	m_LabelAlignment = FALSE;
	m_LabelGirders = FALSE;
	m_LabelPiers = FALSE;
   m_LabelBearings = FALSE;
   m_NorthUp = FALSE;
	//}}AFX_DATA_INIT
}


void CBridgeEditorPlanSettingsPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CBridgeEditorPlanSettingsPage)
	DDX_Check(pDX, IDC_LABEL_GIRDERS, m_LabelGirders);
	DDX_Check(pDX, IDC_LABEL_BEARINGS, m_LabelBearings);
	DDX_Check(pDX, IDC_LABEL_PIERS, m_LabelPiers);
	DDX_Check(pDX, IDC_NORTH_UP, m_NorthUp);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CBridgeEditorPlanSettingsPage, CPropertyPage)
	//{{AFX_MSG_MAP(CBridgeEditorPlanSettingsPage)
	ON_COMMAND(ID_HELP, OnHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CBridgeEditorPlanSettingsPage message handlers

void CBridgeEditorPlanSettingsPage::OnHelp() 
{
   ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_BRIDGEVIEW_PLAN );
}
