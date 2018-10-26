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
// PluginPage.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\resource.h"
#include "PGSuperAppPlugin\PGSuperApp.h"
#include "PluginPage.h"
#include "PGSuperPluginMgr.h"

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

void CPluginPage::Init(LPCTSTR strSection,const CATID& catid,LPCTSTR lpszAppName)
{
   m_Section = strSection;
   m_CATID   = catid;
   m_AppName = lpszAppName;
}

BEGIN_MESSAGE_MAP(CPluginPage, CPropertyPage)
	ON_COMMAND(ID_HELP, OnHelp)
END_MESSAGE_MAP()


// CPluginPage message handlers

BOOL CPluginPage::OnInitDialog()
{
   CPropertyPage::OnInitDialog();

   bool bResult = InitList();

   if ( !bResult )
   {
      return FALSE;
   }

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}

bool CPluginPage::InitList()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   CWinApp* pApp = AfxGetApp();

   USES_CONVERSION;

   CWaitCursor cursor;

   CComPtr<ICatRegister> pICatReg;
   HRESULT hr = pICatReg.CoCreateInstance(CLSID_StdComponentCategoriesMgr);
   if ( FAILED(hr) )
   {
      AfxMessageBox(_T("Failed to create the component category manager"));
      return false;
   }

   CComQIPtr<ICatInformation> pICatInfo(pICatReg);
   CComPtr<IEnumCLSID> pIEnumCLSID;

   const int nID = 1;
   CATID ID[nID];

   ID[0] = m_CATID;
   pICatInfo->EnumClassesOfCategories(nID,ID,0,NULL,&pIEnumCLSID);

   const int nPlugins = 5;
   CLSID clsid[nPlugins]; 
   ULONG nFetched = 0;

   // Load Importers
   while ( SUCCEEDED(pIEnumCLSID->Next(nPlugins,clsid,&nFetched)) && 0 < nFetched)
   {
      for ( ULONG i = 0; i < nFetched; i++ )
      {
         LPOLESTR pszUserType;
         OleRegGetUserType(clsid[i],USERCLASSTYPE_SHORT,&pszUserType);
         int idx = m_ctlPluginList.AddString(OLE2T(pszUserType));

         LPOLESTR pszCLSID;
         ::StringFromCLSID(clsid[i],&pszCLSID);
         
         // The checkbox list is sorted so the position of the various entries can shift around
         // We need to know the index into the m_CLSIDs vector for each entry. Get the index
         // here and save it in the entries item data
         CString strState = pApp->GetProfileString(m_Section,OLE2T(pszCLSID),_T("Enabled"));
         m_CLSIDs.push_back(CString(pszCLSID));
         ATLASSERT(0 < m_CLSIDs.size());
         int clsidIdx = (int)(m_CLSIDs.size()-1);
         m_ctlPluginList.SetItemData(idx,(DWORD_PTR)clsidIdx);

         ::CoTaskMemFree((void*)pszCLSID);

         if ( strState.CompareNoCase(_T("Enabled")) == 0 )
         {
            m_ctlPluginList.SetCheck(idx,TRUE);
         }
         else
         {
            m_ctlPluginList.SetCheck(idx,FALSE);
         }
      }
   }

   return true;
}

void CPluginPage::OnOK()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   CWinApp* pApp = AfxGetApp();

   int nItems = m_ctlPluginList.GetCount();
   for (int idx = 0; idx < nItems; idx++ )
   {
      BOOL bEnabled = m_ctlPluginList.GetCheck(idx);
      int clsidIdx = (int)m_ctlPluginList.GetItemData(idx);
      CString strCLSID = m_CLSIDs[clsidIdx];

      pApp->WriteProfileString(m_Section,strCLSID,(bEnabled ? _T("Enabled") : _T("Disabled")));
   }

   CPropertyPage::OnOK();
}

void CPluginPage::OnHelp()
{
   EAFHelp( m_AppName, IDH_PLUGINS);
}