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

// BridgeEditorSettingsSheet.cpp : implementation file
//

#include "stdafx.h"
#include "PGSuperAppPlugin\PGSuperApp.h"
#include "BridgeEditorSettingsSheet.h"
#include "IFace\DrawBridgeSettings.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CBridgeEditorSettingsSheet

IMPLEMENT_DYNAMIC(CBridgeEditorSettingsSheet, CPropertySheet)

CBridgeEditorSettingsSheet::CBridgeEditorSettingsSheet(UINT nIDCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(nIDCaption, pParentWnd, iSelectPage)
{
   Init();
}

CBridgeEditorSettingsSheet::CBridgeEditorSettingsSheet(LPCTSTR pszCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(pszCaption, pParentWnd, iSelectPage)
{
   Init();
}

CBridgeEditorSettingsSheet::~CBridgeEditorSettingsSheet()
{
}


BEGIN_MESSAGE_MAP(CBridgeEditorSettingsSheet, CPropertySheet)
	//{{AFX_MSG_MAP(CBridgeEditorSettingsSheet)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CBridgeEditorSettingsSheet message handlers

void CBridgeEditorSettingsSheet::DoDataExchange(CDataExchange* pDX) 
{
	// TODO: Add your specialized code here and/or call the base class
	
	CPropertySheet::DoDataExchange(pDX);
}

BOOL CBridgeEditorSettingsSheet::OnInitDialog() 
{
	BOOL bResult = CPropertySheet::OnInitDialog();
	
	// TODO: Add your specialized code here
	
	return bResult;
}

void CBridgeEditorSettingsSheet::Init()
{
   m_Settings=0;

   // Turn on help for the property sheet
   m_psh.dwFlags |= PSH_HASHELP | PSH_NOAPPLYNOW;
   m_BridgeEditorPlanSettingsPage.m_psp.dwFlags |= PSP_HASHELP;
   m_BridgeEditorSectionPage.m_psp.dwFlags |= PSP_HASHELP;

   AddPage(&m_BridgeEditorPlanSettingsPage);
   AddPage(&m_BridgeEditorSectionPage);
}

void CBridgeEditorSettingsSheet::SetSettings(UINT set)
{
   // plan view
   m_BridgeEditorPlanSettingsPage.m_LabelPiers     = (set&IDB_PV_LABEL_PIERS)!=0;
   m_BridgeEditorPlanSettingsPage.m_LabelGirders   = (set&IDB_PV_LABEL_GIRDERS)!=0;
   m_BridgeEditorPlanSettingsPage.m_LabelBearings  = (set&IDB_PV_LABEL_BEARINGS)!=0;
   m_BridgeEditorPlanSettingsPage.m_NorthUp        = (set&IDB_PV_NORTH_UP)!=0;

   // section view
   m_BridgeEditorSectionPage.m_LabelGirders   = (set&IDB_CS_LABEL_GIRDERS)!=0;
   m_BridgeEditorSectionPage.m_ShowDimensions = (set&IDB_CS_SHOW_DIMENSIONS)!=0;
}

UINT CBridgeEditorSettingsSheet::GetSettings()const 
{
   UINT set=0;

   // draw both views isotropic and not to scale by default
   set |= IDB_PV_DRAW_ISOTROPIC;
   set |= IDB_CS_DRAW_ISOTROPIC;

   if (m_BridgeEditorPlanSettingsPage.m_LabelPiers)
      set |= IDB_PV_LABEL_PIERS;

   // turn off alignment labeling
   set ^= IDB_PV_LABEL_ALIGNMENT;

   if(m_BridgeEditorPlanSettingsPage.m_LabelGirders)
      set |= IDB_PV_LABEL_GIRDERS;

   if(m_BridgeEditorPlanSettingsPage.m_LabelBearings)
      set |= IDB_PV_LABEL_BEARINGS;

   if(m_BridgeEditorPlanSettingsPage.m_NorthUp)
      set |= IDB_PV_NORTH_UP;

   // section view
   if(m_BridgeEditorSectionPage.m_LabelGirders)
      set |= IDB_CS_LABEL_GIRDERS;

   if(m_BridgeEditorSectionPage.m_ShowDimensions)
      set |= IDB_CS_SHOW_DIMENSIONS;

   return set;
}
