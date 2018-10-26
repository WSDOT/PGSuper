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
// PluginPage.cpp : implementation file
//

#include "stdafx.h"
#include "PGSuperAppPlugin\PGSuperApp.h"
#include "PluginPage.h"
#include "PGSuperPluginMgr.h"

#include "PGSuperCatCom.h"
#include "HtmlHelp\HelpTopics.hh"

#include <EAF\EAFApp.h>

// CPluginPage dialog

IMPLEMENT_DYNAMIC(CPluginPage, CPropertyPage)

CPluginPage::CPluginPage()
	: CPropertyPage(CPluginPage::IDD)
{

}

CPluginPage::~CPluginPage()
{
}

void CPluginPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_PLUGIN_LIST, m_ctlPluginList);
}

void CPluginPage::Init(int pageType)
{
   m_PageType = pageType;
}

BEGIN_MESSAGE_MAP(CPluginPage, CPropertyPage)
	ON_COMMAND(ID_HELP, OnHelp)
END_MESSAGE_MAP()


// CPluginPage message handlers

BOOL CPluginPage::OnInitDialog()
{
   CPropertyPage::OnInitDialog();

   bool bResult;
   if ( m_PageType == PROJECT_IMPORTER_PAGE )
      bResult = InitList(CATID_PGSuperProjectImporter);
   else if ( m_PageType == DATA_IMPORTER_PAGE )
      bResult = InitList(CATID_PGSuperDataImporter);
   else if ( m_PageType == DATA_EXPORTER_PAGE )
      bResult = InitList(CATID_PGSuperDataExporter);
   else
      bResult = InitList(CATID_PGSuperExtensionAgent);

   if ( !bResult )
      return FALSE;

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}

bool CPluginPage::InitList(const CATID& catid)
{
   USES_CONVERSION;

   CWaitCursor cursor;

   CComPtr<ICatRegister> pICatReg;
   HRESULT hr = pICatReg.CoCreateInstance(CLSID_StdComponentCategoriesMgr);
   if ( FAILED(hr) )
   {
      AfxMessageBox("Failed to create the component category manager");
      return false;
   }

   CComQIPtr<ICatInformation> pICatInfo(pICatReg);
   CComPtr<IEnumCLSID> pIEnumCLSID;

   const int nID = 1;
   CATID ID[nID];

   ID[0] = catid;
   pICatInfo->EnumClassesOfCategories(nID,ID,0,NULL,&pIEnumCLSID);

   const int nPlugins = 5;
   CLSID clsid[nPlugins]; 
   ULONG nFetched = 0;

   CString strSection( m_PageType == EXTENSION_AGENT_PAGE ? "Extensions" : "Plugins" );

   // Load Importers
   CEAFApp* pApp = EAFGetApp();

   while ( SUCCEEDED(pIEnumCLSID->Next(nPlugins,clsid,&nFetched)) && 0 < nFetched)
   {
      for ( ULONG i = 0; i < nFetched; i++ )
      {
         LPOLESTR pszUserType;
         OleRegGetUserType(clsid[i],USERCLASSTYPE_SHORT,&pszUserType);
         int idx = m_ctlPluginList.AddString(OLE2A(pszUserType));

         LPOLESTR pszCLSID;
         ::StringFromCLSID(clsid[i],&pszCLSID);
         
         CString strState = pApp->GetProfileString(strSection,OLE2A(pszCLSID),_T("Enabled"));
         m_CLSIDs.push_back(CString(pszCLSID));

         ::CoTaskMemFree((void*)pszCLSID);

         if ( strState.CompareNoCase("Enabled") == 0 )
            m_ctlPluginList.SetCheck(idx,TRUE);
         else
            m_ctlPluginList.SetCheck(idx,FALSE);
      }
   }

   return true;
}

void CPluginPage::OnOK()
{
   CEAFApp* pApp = EAFGetApp();

   CString strSection( m_PageType == EXTENSION_AGENT_PAGE ? "Extensions" : "Plugins" );

   int nItems = m_ctlPluginList.GetCount();
   for (int idx = 0; idx < nItems; idx++ )
   {
      CString strCLSID = m_CLSIDs[idx];
      BOOL bEnabled = m_ctlPluginList.GetCheck(idx);

      pApp->WriteProfileString(strSection,strCLSID,(bEnabled ? _T("Enabled") : _T("Disabled")));
   }
   CPropertyPage::OnOK();
}

void CPluginPage::OnHelp()
{
   ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_PLUGINS );
}