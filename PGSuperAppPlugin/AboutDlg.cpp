///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2021  Washington State Department of Transportation
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

//#include "stdafx.h"
#include "stdafx.h"
#include "resource.h"
#include "resource.h"
#include "AboutDlg.h"

#include "PGSuperDoc.h"
#include "PGSpliceDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNAMIC(CAboutDlg, CDialog)

CAboutDlg::CAboutDlg(UINT nResourceID,UINT nIDTemplate,CWnd* pParent /*=nullptr*/)
: CDialog(nIDTemplate == 0 ? IDD_ABOUTBOX : nIDTemplate, pParent)
{
   m_ResourceID = nResourceID;
}

CAboutDlg::~CAboutDlg()
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
   DDX_Control(pDX,IDC_WSDOT,m_WSDOT);
   DDX_Control(pDX,IDC_TXDOT,m_TxDOT);
   DDX_Control(pDX,IDC_KDOT, m_KDOT);
   DDX_Control(pDX,IDC_BRIDGESIGHT,m_BridgeSight);
   DDX_Control(pDX,IDC_APPLIST,m_AppList);
   DDX_Control(pDX,IDC_DESCRIPTION,m_Description);
}


BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
   ON_LBN_SELCHANGE(IDC_APPLIST,OnAppListSelChanged)
	ON_BN_CLICKED(IDC_MOREINFO,OnMoreInfo)
   ON_WM_CTLCOLOR()
END_MESSAGE_MAP()


// CAboutDlg message handlers

BOOL CAboutDlg::OnInitDialog()
{
   CDialog::OnInitDialog();

   // put the icon in the dialog
   CStatic* pIcon = (CStatic*)GetDlgItem(IDC_APPICON);

   CWinApp* pApp = AfxGetApp();
   HICON hIcon = pApp->LoadIcon(m_ResourceID);
   if ( hIcon )
   {
      pIcon->SetIcon( hIcon );
   }
   else
   {
      CWnd* pWnd = EAFGetMainFrame();
      hIcon = pWnd->GetIcon(TRUE);
      if ( hIcon )
      {
         pIcon->SetIcon( hIcon );
      }
      else
      {
         pIcon->ShowWindow(SW_HIDE);
      }
   }

   // Get the version information and update the version # and copyright
   CString docString;
   docString.LoadString(m_ResourceID);
   CString strAppName;
   AfxExtractSubString(strAppName, docString, 1);

   CString strTitle;
   strTitle.Format(_T("About %s"),strAppName);
   SetWindowText( strTitle );

   CString strExe = AfxGetApp()->m_pszExeName;
   strExe += _T(".dll");

   CVersionInfo verInfo;
   verInfo.Load(strExe);
   CString strVersion = verInfo.GetFileVersionAsString();
   CString strCopyright = verInfo.GetLegalCopyright();

#if defined _WIN64
   CString strPlatform(_T("x64"));
#else
   CString strPlatform(_T("x86"));
#endif
   CString str;
   str.Format(_T("Version %s (%s)"),strVersion,strPlatform);
   GetDlgItem(IDC_VERSION)->SetWindowText(str);
   GetDlgItem(IDC_COPYRIGHT)->SetWindowText(strCopyright);


   CString strExtensions;
   strExtensions.Format(_T("%s Extensions"),strAppName);
   GetDlgItem(IDC_EXTENSIONS)->SetWindowText(strExtensions);
	

   m_WSDOT.SetURL(_T("http://www.wsdot.wa.gov"));
   m_WSDOT.SetTooltip(_T("http://www.wsdot.wa.gov"));
   m_WSDOT.SizeToContent();

   m_TxDOT.SetURL(_T("http://www.dot.state.tx.us"));
   m_TxDOT.SetTooltip(_T("http://www.dot.state.tx.us"));
   m_TxDOT.SizeToContent();

   m_KDOT.SetURL(_T("http://www.ksdot.org"));
   m_KDOT.SetTooltip(_T("http://www.ksdot.org"));
   m_KDOT.SizeToContent();

   m_BridgeSight.SetURL(_T("http://www.bridgesight.com"));
   m_BridgeSight.SetTooltip(_T("http://www.bridgesight.com"));
   m_BridgeSight.SizeToContent();

   // Fill the list control with plugin names
   CEAFDocument* pEAFDoc = EAFGetDocument();
   if ( pEAFDoc->IsKindOf(RUNTIME_CLASS(CPGSuperDoc)) )
   {
      CPGSuperDoc* pDoc = (CPGSuperDoc*)pEAFDoc;
      CPGSuperComponentInfoManager* pComponentInfoMgr = pDoc->GetComponentInfoManager();
      CollectionIndexType nPlugins = pComponentInfoMgr->GetPluginCount();
      
      // for each plugin
      for ( CollectionIndexType pluginIdx = 0; pluginIdx < nPlugins; pluginIdx++ )
      {
         CComPtr<IPGSuperComponentInfo> plugin;
         pComponentInfoMgr->GetPlugin(pluginIdx,&plugin);
         UINT idx = m_AppList.AddString(plugin->GetName());
         m_AppList.SetItemData(idx,pluginIdx);
      }
   }
   else if ( pEAFDoc->IsKindOf(RUNTIME_CLASS(CPGSpliceDoc)) )
   {
      CPGSpliceDoc* pDoc = (CPGSpliceDoc*)pEAFDoc;
      CPGSpliceComponentInfoManager* pComponentInfoMgr = pDoc->GetComponentInfoManager();
      CollectionIndexType nPlugins = pComponentInfoMgr->GetPluginCount();
      
      // for each plugin
      for ( CollectionIndexType pluginIdx = 0; pluginIdx < nPlugins; pluginIdx++ )
      {
         CComPtr<IPGSpliceComponentInfo> plugin;
         pComponentInfoMgr->GetPlugin(pluginIdx,&plugin);
         UINT idx = m_AppList.AddString(plugin->GetName());
         m_AppList.SetItemData(idx,pluginIdx);
      }
   }

   m_AppList.SetCurSel(0);
   OnAppListSelChanged();

   m_AppList.SetFocus();

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}

void CAboutDlg::OnAppListSelChanged()
{
   int idx = m_AppList.GetCurSel();
   if ( idx != LB_ERR )
   {
      CEAFDocument* pEAFDoc = EAFGetDocument();
      if ( pEAFDoc->IsKindOf(RUNTIME_CLASS(CPGSuperDoc)) )
      {
         CPGSuperDoc* pDoc = (CPGSuperDoc*)pEAFDoc;
         CPGSuperComponentInfoManager* pComponentInfoMgr = pDoc->GetComponentInfoManager();
         CComPtr<IPGSuperComponentInfo> component;
         DWORD_PTR pluginIdx = m_AppList.GetItemData(idx);
         pComponentInfoMgr->GetPlugin(pluginIdx,&component);
         m_Description.SetWindowText(component->GetDescription());
         if ( component->HasMoreInfo() )
         {
            GetDlgItem(IDC_MOREINFO)->EnableWindow(TRUE);
         }
         else
         {
            GetDlgItem(IDC_MOREINFO)->EnableWindow(FALSE);
         }
      }
      else
      {
         CPGSpliceDoc* pDoc = (CPGSpliceDoc*)pEAFDoc;
         CPGSpliceComponentInfoManager* pComponentInfoMgr = pDoc->GetComponentInfoManager();
         CComPtr<IPGSpliceComponentInfo> component;
         DWORD_PTR pluginIdx = m_AppList.GetItemData(idx);
         pComponentInfoMgr->GetPlugin(pluginIdx,&component);
         m_Description.SetWindowText(component->GetDescription());
         if ( component->HasMoreInfo() )
         {
            GetDlgItem(IDC_MOREINFO)->EnableWindow(TRUE);
         }
         else
         {
            GetDlgItem(IDC_MOREINFO)->EnableWindow(FALSE);
         }
      }
   }
   else
   {
      GetDlgItem(IDC_MOREINFO)->EnableWindow(FALSE);
      m_Description.SetWindowText(_T(""));
   }
}

void CAboutDlg::OnMoreInfo()
{
   int idx = m_AppList.GetCurSel();
   if ( idx != LB_ERR )
   {
      CEAFDocument* pEAFDoc = EAFGetDocument();
      if ( pEAFDoc->IsKindOf(RUNTIME_CLASS(CPGSuperDoc)) )
      {
         CPGSuperDoc* pDoc = (CPGSuperDoc*)pEAFDoc;
         CPGSuperComponentInfoManager* pComponentInfoMgr = pDoc->GetComponentInfoManager();

         DWORD_PTR pluginIdx = m_AppList.GetItemData(idx);
         CComPtr<IPGSuperComponentInfo> component;
         pComponentInfoMgr->GetPlugin(pluginIdx,&component);

         component->OnMoreInfo();
      }
      else
      {
         CPGSpliceDoc* pDoc = (CPGSpliceDoc*)pEAFDoc;
         CPGSpliceComponentInfoManager* pComponentInfoMgr = pDoc->GetComponentInfoManager();

         DWORD_PTR pluginIdx = m_AppList.GetItemData(idx);
         CComPtr<IPGSpliceComponentInfo> component;
         pComponentInfoMgr->GetPlugin(pluginIdx,&component);

         component->OnMoreInfo();
      }
      OnAppListSelChanged();
   }
}

HBRUSH CAboutDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	if (pWnd->GetDlgCtrlID() == IDC_DESCRIPTION)
   {
		return GetSysColorBrush(COLOR_WINDOW);
   }

	return CDialog::OnCtlColor(pDC, pWnd, nCtlColor);
}
