///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2011  Washington State Department of Transportation
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
// PluginManagerDlg.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\PGSuperApp.h"
#include "PluginManagerDlg.h"


// CPluginManagerDlg

IMPLEMENT_DYNAMIC(CPluginManagerDlg, CPropertySheet)

CPluginManagerDlg::CPluginManagerDlg(UINT nIDCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(nIDCaption, pParentWnd, iSelectPage)
{
   Init();
}

CPluginManagerDlg::CPluginManagerDlg(LPCTSTR pszCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(pszCaption, pParentWnd, iSelectPage)
{
   Init();
}

CPluginManagerDlg::~CPluginManagerDlg()
{
}


void CPluginManagerDlg::Init()
{
   m_psh.dwFlags |= PSH_HASHELP | PSH_NOAPPLYNOW;

   m_DataImporterPage.m_psp.dwFlags |= PSP_USETITLE | PSP_HASHELP;
   m_DataExporterPage.m_psp.dwFlags |= PSP_USETITLE | PSP_HASHELP;
   m_ExtensionAgentPage.m_psp.dwFlags |= PSP_USETITLE | PSP_HASHELP;

   m_DataImporterPage.m_psp.pszTitle = _T("Data Importers");
   m_DataExporterPage.m_psp.pszTitle = _T("Data Exporters");
   m_ExtensionAgentPage.m_psp.pszTitle = _T("Extensions");

   m_DataImporterPage.Init(DATA_IMPORTER_PAGE);
   m_DataExporterPage.Init(DATA_EXPORTER_PAGE);
   m_ExtensionAgentPage.Init(EXTENSION_AGENT_PAGE);

   AddPage(&m_DataImporterPage);
   AddPage(&m_DataExporterPage);
   AddPage(&m_ExtensionAgentPage);
}

BEGIN_MESSAGE_MAP(CPluginManagerDlg, CPropertySheet)
END_MESSAGE_MAP()


// CPluginManagerDlg message handlers
