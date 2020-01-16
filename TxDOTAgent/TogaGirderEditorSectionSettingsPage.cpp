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

// TogaGirderEditorSectionSettingsPage.cpp : implementation file
//

#include "stdafx.h"
#include "TogaGirderEditorSectionSettingsPage.h"
#include <EAF\EAFDocument.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTogaGirderEditorSectionSettingsPage property page

IMPLEMENT_DYNCREATE(CTogaGirderEditorSectionSettingsPage, CPropertyPage)

CTogaGirderEditorSectionSettingsPage::CTogaGirderEditorSectionSettingsPage() : CPropertyPage(CTogaGirderEditorSectionSettingsPage::IDD)
{
	//{{AFX_DATA_INIT(CTogaGirderEditorSectionSettingsPage)
	m_ShowDimensions = FALSE;
	m_ShowPsCg = FALSE;
	m_ShowStrands = FALSE;
	m_ShowLongReinf = FALSE;
	//}}AFX_DATA_INIT
}

CTogaGirderEditorSectionSettingsPage::~CTogaGirderEditorSectionSettingsPage()
{
}

void CTogaGirderEditorSectionSettingsPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CTogaGirderEditorSectionSettingsPage)
	DDX_Check(pDX, IDC_SHOW_DIMENSIONS, m_ShowDimensions);
	DDX_Check(pDX, IDC_SHOW_PS_CG, m_ShowPsCg);
	DDX_Check(pDX, IDC_SHOW_STRANDS, m_ShowStrands);
	DDX_Check(pDX, IDC_SHOW_SHOW_LONG_REINF, m_ShowLongReinf);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CTogaGirderEditorSectionSettingsPage, CPropertyPage)
	//{{AFX_MSG_MAP(CTogaGirderEditorSectionSettingsPage)
	ON_COMMAND(ID_HELP, OnHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTogaGirderEditorSectionSettingsPage message handlers

void CTogaGirderEditorSectionSettingsPage::OnHelp() 
{
   EAFHelp( EAFGetDocument()->GetDocumentationSetName(), IDH_VIEW_SETTINGS );
}
