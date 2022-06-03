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

// GirderEditorSettingsSheet.cpp : implementation file
//

#include "stdafx.h"
#include "PGSuperApp.h"
#include "GirderEditorSettingsSheet.h"
#include "IFace\DrawBridgeSettings.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CGirderEditorSettingsSheet

IMPLEMENT_DYNAMIC(CGirderEditorSettingsSheet, CPropertySheet)

CGirderEditorSettingsSheet::CGirderEditorSettingsSheet(UINT nIDCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(nIDCaption, pParentWnd, iSelectPage)
{
   Init();
}

CGirderEditorSettingsSheet::CGirderEditorSettingsSheet(LPCTSTR pszCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(pszCaption, pParentWnd, iSelectPage)
{
   Init();
}

CGirderEditorSettingsSheet::~CGirderEditorSettingsSheet()
{
}


BEGIN_MESSAGE_MAP(CGirderEditorSettingsSheet, CPropertySheet)
	//{{AFX_MSG_MAP(CGirderEditorSettingsSheet)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGirderEditorSettingsSheet message handlers

void CGirderEditorSettingsSheet::DoDataExchange(CDataExchange* pDX) 
{
	// TODO: Add your specialized code here and/or call the base class
	
	CPropertySheet::DoDataExchange(pDX);
}

BOOL CGirderEditorSettingsSheet::OnInitDialog() 
{
	BOOL bResult = CPropertySheet::OnInitDialog();
	
	// TODO: Add your specialized code here
	
	return bResult;
}

void CGirderEditorSettingsSheet::Init()
{
   m_Settings=0;

   // Turn on help for the property sheet
   m_psh.dwFlags |= PSH_HASHELP | PSH_NOAPPLYNOW;
   m_GirderEditorSectionSettingsPage.m_psp.dwFlags |= PSP_HASHELP;
   m_GirderEditorElevationSettingsPage.m_psp.dwFlags |= PSP_HASHELP;

   AddPage(&m_GirderEditorElevationSettingsPage);
   AddPage(&m_GirderEditorSectionSettingsPage);
}

void CGirderEditorSettingsSheet::SetSettings(UINT set)
{
   // elevation view
   m_GirderEditorElevationSettingsPage.m_ShowDimensions = (set&IDG_EV_SHOW_DIMENSIONS)!=0;
   m_GirderEditorElevationSettingsPage.m_ShowPsCg       = (set&IDG_EV_SHOW_PS_CG)!=0;
   m_GirderEditorElevationSettingsPage.m_ShowStirrups   = (set&IDG_EV_SHOW_STIRRUPS)!=0;
   m_GirderEditorElevationSettingsPage.m_ShowStrands    = (set&IDG_EV_SHOW_STRANDS)!=0;
   m_GirderEditorElevationSettingsPage.m_ShowSchematic  = (set&IDG_EV_DRAW_ISOTROPIC)==0;
   m_GirderEditorElevationSettingsPage.m_ShowLongReinf  = (set&IDG_EV_SHOW_LONG_REINF)!=0;
   m_GirderEditorElevationSettingsPage.m_ShowLoads      = (set&IDG_EV_SHOW_LOADS)!=0;
   m_GirderEditorElevationSettingsPage.m_ShowLegend      = (set&IDG_EV_SHOW_LEGEND)!=0;
   m_GirderEditorElevationSettingsPage.m_ShowSectionCG = (set&IDG_EV_GIRDER_CG) != 0;

   // section view
   m_GirderEditorSectionSettingsPage.m_ShowPsCg       = (set&IDG_SV_SHOW_PS_CG)!=0;
   m_GirderEditorSectionSettingsPage.m_ShowDimensions = (set&IDG_SV_SHOW_DIMENSIONS)!=0;
   m_GirderEditorSectionSettingsPage.m_ShowStrands    = (set&IDG_SV_SHOW_STRANDS)!=0;
   m_GirderEditorSectionSettingsPage.m_ShowLongReinf  = (set&IDG_SV_SHOW_LONG_REINF)!=0;
   m_GirderEditorSectionSettingsPage.m_ShowSectionCG = (set&IDG_SV_GIRDER_CG) != 0;
   m_GirderEditorSectionSettingsPage.m_ShowGirderProperties = (set&IDG_SV_PROPERTIES) != 0;
}

UINT CGirderEditorSettingsSheet::GetSettings()const 
{
   UINT set=0;

   // draw both views isotropic and not to scale by default
   set |= IDG_SV_DRAW_ISOTROPIC;

   // elevation view
   if (m_GirderEditorElevationSettingsPage.m_ShowDimensions)
      set |= IDG_EV_SHOW_DIMENSIONS;

   if (m_GirderEditorElevationSettingsPage.m_ShowPsCg)
      set |= IDG_EV_SHOW_PS_CG;

   if (m_GirderEditorElevationSettingsPage.m_ShowStirrups)
      set |= IDG_EV_SHOW_STIRRUPS;

   if (m_GirderEditorElevationSettingsPage.m_ShowStrands) 
      set |= IDG_EV_SHOW_STRANDS;

   if (m_GirderEditorElevationSettingsPage.m_ShowLongReinf) 
      set |= IDG_EV_SHOW_LONG_REINF;

   if (!m_GirderEditorElevationSettingsPage.m_ShowSchematic) 
      set |= IDG_EV_DRAW_ISOTROPIC;

   if (m_GirderEditorElevationSettingsPage.m_ShowLoads)
      set |= IDG_EV_SHOW_LOADS;

   if (m_GirderEditorElevationSettingsPage.m_ShowLegend)
      set |= IDG_EV_SHOW_LEGEND;

   if (m_GirderEditorElevationSettingsPage.m_ShowSectionCG)
      set |= IDG_EV_GIRDER_CG;

   // section view
   if (m_GirderEditorSectionSettingsPage.m_ShowPsCg)
      set |= IDG_SV_SHOW_PS_CG;

   if (m_GirderEditorSectionSettingsPage.m_ShowDimensions)
      set |= IDG_SV_SHOW_DIMENSIONS;

   if (m_GirderEditorSectionSettingsPage.m_ShowStrands)
      set |= IDG_SV_SHOW_STRANDS;

   if (m_GirderEditorSectionSettingsPage.m_ShowLongReinf)
      set |= IDG_SV_SHOW_LONG_REINF;

   if (m_GirderEditorSectionSettingsPage.m_ShowSectionCG)
      set |= IDG_SV_GIRDER_CG;

   if (m_GirderEditorSectionSettingsPage.m_ShowGirderProperties)
      set |= IDG_SV_PROPERTIES;

   return set;
}
