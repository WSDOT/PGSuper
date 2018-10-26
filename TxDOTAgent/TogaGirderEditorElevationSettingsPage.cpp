///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2012  Washington State Department of Transportation
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

// TogaGirderEditorElevationSettingsPage.cpp : implementation file
//

#include "stdafx.h"
#include "TogaGirderEditorElevationSettingsPage.h"
#include "HtmlHelp\TogaHelp.hh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTogaGirderEditorElevationSettingsPage property page

IMPLEMENT_DYNCREATE(CTogaGirderEditorElevationSettingsPage, CPropertyPage)

CTogaGirderEditorElevationSettingsPage::CTogaGirderEditorElevationSettingsPage() : CPropertyPage(CTogaGirderEditorElevationSettingsPage::IDD)
{
	//{{AFX_DATA_INIT(CTogaGirderEditorElevationSettingsPage)
	m_ShowDimensions = FALSE;
	m_ShowPsCg = FALSE;
	m_ShowStirrups = FALSE;
	m_ShowStrands = FALSE;
	m_ShowSchematic = FALSE;
	m_ShowLongReinf = FALSE;
	//}}AFX_DATA_INIT
}

CTogaGirderEditorElevationSettingsPage::~CTogaGirderEditorElevationSettingsPage()
{
}

void CTogaGirderEditorElevationSettingsPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CTogaGirderEditorElevationSettingsPage)
	DDX_Check(pDX, IDC_SHOW_DIMENSIONS, m_ShowDimensions);
	DDX_Check(pDX, IDC_SHOW_PS_CG, m_ShowPsCg);
	DDX_Check(pDX, IDC_SHOW_TRANSV_REINF, m_ShowStirrups);
	DDX_Check(pDX, IDC_SHOW_STRANDS, m_ShowStrands);
	DDX_Check(pDX, IDC_SHOW_SCHEMATIC, m_ShowSchematic);
	DDX_Check(pDX, IDC_SHOW_LONG_REINF, m_ShowLongReinf);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CTogaGirderEditorElevationSettingsPage, CPropertyPage)
	//{{AFX_MSG_MAP(CTogaGirderEditorElevationSettingsPage)
	ON_COMMAND(ID_HELP, OnHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTogaGirderEditorElevationSettingsPage message handlers

void CTogaGirderEditorElevationSettingsPage::OnHelp() 
{
   ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_VIEW_SETTINGS );
}
