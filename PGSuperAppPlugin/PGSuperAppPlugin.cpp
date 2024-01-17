///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2024  Washington State Department of Transportation
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

#include "stdafx.h"
#include "PGSuperAppPlugin.h"
#include "PGSuperCatCom.h"
#include "resource.h"

#include "PGSuperDoc.h"
#include "PGSuperDocTemplate.h"
#include "BridgeModelViewChildFrame.h"
#include "BridgePlanView.h"

#include "PGSuperCommandLineInfo.h"

#include "PluginManagerDlg.h"

#include <EAF\EAFMainFrame.h>
#include <EAF\EAFUtilities.h>

#include <MFCTools\AutoRegistry.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


BEGIN_MESSAGE_MAP(CMyCmdTarget,CCmdTarget)
   ON_COMMAND(ID_MANAGE_PLUGINS,OnConfigurePlugins)
   ON_COMMAND(ID_UPDATE_TEMPLATE,OnUpdateTemplates) // need to map this into an accelerator table
END_MESSAGE_MAP()

void CMyCmdTarget::OnConfigurePlugins()
{
   m_pMyAppPlugin->ConfigurePlugins();
}

void CMyCmdTarget::OnUpdateTemplates()
{
   m_pMyAppPlugin->UpdateTemplates();
}

CString CPGSuperAppPlugin::GetTemplateFileExtension()
{ 
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CString strTemplateSuffix;
   VERIFY(strTemplateSuffix.LoadString(IDS_PGSUPER_TEMPLATE_FILE_SUFFIX));
   ASSERT(!strTemplateSuffix.IsEmpty());
   return strTemplateSuffix;
}

const CRuntimeClass* CPGSuperAppPlugin::GetDocTemplateRuntimeClass()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   return RUNTIME_CLASS(CPGSuperDocTemplate);
}

HRESULT CPGSuperAppPlugin::FinalConstruct()
{
   return OnFinalConstruct(); // CPGSAppPluginBase
}

void CPGSuperAppPlugin::FinalRelease()
{
   OnFinalRelease(); // CPGSAppPluginBase
}

void CPGSuperAppPlugin::ConfigurePlugins()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CAutoRegistry autoReg(GetAppName());

   CPluginManagerDlg dlg(_T("Manage PGSuper Plugins and Extensions"),EAFGetMainFrame(),0,CATID_PGSuperDataImporter,CATID_PGSuperDataExporter,CATID_PGSuperExtensionAgent,GetAppName());
   dlg.DoModal(); // this DoModal is correct... the dialog takes care of its own data
}

BOOL CPGSuperAppPlugin::Init(CEAFApp* pParent)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   CWinApp* pMyApp = AfxGetApp();

   DefaultInit(this);

   // See MSKB Article ID: Q118435, "Sharing Menus Between MDI Child Windows"
   m_hMenuShared = ::LoadMenu( pMyApp->m_hInstance, MAKEINTRESOURCE(IDR_PGSUPER) );

   if (m_hMenuShared == nullptr)
   {
      return FALSE;
   }

   if (!EAFGetApp()->GetCommandLineInfo().m_bCommandLineMode && !EAFGetApp()->IsFirstRun())
   {
      // Don't update cache if there are other instances of bridgelink running. This avoids race condition on libraries and templates
      if (!EAFAreOtherProgramInstancesRunning())
      {
         AFX_MANAGE_STATE(AfxGetStaticModuleState()); // need state of this dll
         UpdateCache(); // we don't want to do this if we are running in batch/command line mode or if this is the first run situation (because configuration will happen later)
      }
   }

   return TRUE;
}

void CPGSuperAppPlugin::Terminate()
{
   DefaultTerminate();
   // release the shared menu
   ::DestroyMenu( m_hMenuShared );
}

void CPGSuperAppPlugin::IntegrateWithUI(BOOL bIntegrate)
{
   CEAFMainFrame* pFrame = EAFGetMainFrame();
   CEAFMenu* pMainMenu = pFrame->GetMainMenu();

   UINT filePos = pMainMenu->FindMenuItem(_T("&File"));
   CEAFMenu* pFileMenu = pMainMenu->GetSubMenu(filePos);

   UINT managePos = pFileMenu->FindMenuItem(_T("Manage"));
   CEAFMenu* pManageMenu = pFileMenu->GetSubMenu(managePos);

   if ( bIntegrate )
   {
      // Append to the end of the Manage menu
      pManageMenu->AppendMenu(ID_MANAGE_PLUGINS,_T("PGSuper Plugins and Extensions..."),this);

      // Alt+Ctrl+U
      pFrame->GetAcceleratorTable()->AddAccelKey(FALT | FCONTROL | FVIRTKEY, VK_U, ID_UPDATE_TEMPLATE,this);
   }
   else
   {
      pManageMenu->RemoveMenu(ID_MANAGE_PLUGINS,  MF_BYCOMMAND, this);

      pFrame->GetAcceleratorTable()->RemoveAccelKey(ID_UPDATE_TEMPLATE,this);
   }
}

std::vector<CEAFDocTemplate*> CPGSuperAppPlugin::CreateDocTemplates()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   std::vector<CEAFDocTemplate*> vDocTemplates;

   std::unique_ptr<CPGSuperDocTemplate> pTemplate = std::make_unique<CPGSuperDocTemplate>(
		IDR_PGSUPER,
      nullptr,
		RUNTIME_CLASS(CPGSuperDoc),
		RUNTIME_CLASS(CBridgeModelViewChildFrame),
		RUNTIME_CLASS(CBridgePlanView),
      m_hMenuShared,1);

   vDocTemplates.push_back(pTemplate.release());
   return vDocTemplates;
}

HMENU CPGSuperAppPlugin::GetSharedMenuHandle()
{
   return m_hMenuShared;
}

CString CPGSuperAppPlugin::GetName()
{
   return CString(_T("PGSuper"));
}

CString CPGSuperAppPlugin::GetDocumentationSetName()
{
   return GetName();
}

CString CPGSuperAppPlugin::GetDocumentationURL()
{
   return CPGSAppPluginBase::GetDocumentationURL();
}

CString CPGSuperAppPlugin::GetDocumentationMapFile()
{
   return CPGSAppPluginBase::GetDocumentationMapFile();
}

void CPGSuperAppPlugin::LoadDocumentationMap()
{
   return CPGSAppPluginBase::LoadDocumentationMap();
}

eafTypes::HelpResult CPGSuperAppPlugin::GetDocumentLocation(LPCTSTR lpszDocSetName,UINT nID,CString& strURL)
{
   return CPGSAppPluginBase::GetDocumentLocation(lpszDocSetName,nID,strURL);
}

CString CPGSuperAppPlugin::GetCommandLineAppName() const
{
   return GetAppName();
}

CString CPGSuperAppPlugin::GetUsageMessage()
{
   CPGSuperCommandLineInfo pgsCmdInfo;
   return pgsCmdInfo.GetUsageMessage();
}

BOOL CPGSuperAppPlugin::ProcessCommandLineOptions(CEAFCommandLineInfo& cmdInfo)
{
   return DoProcessCommandLineOptions(cmdInfo);
}

//////////////////////////
// IEAFCommandCallback
BOOL CPGSuperAppPlugin::OnCommandMessage(UINT nID,int nCode,void* pExtra,AFX_CMDHANDLERINFO* pHandlerInfo)
{
   return m_MyCmdTarget.OnCmdMsg(nID,nCode,pExtra,pHandlerInfo);
}

BOOL CPGSuperAppPlugin::GetStatusBarMessageString(UINT nID, CString& rMessage) const
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   // load appropriate string
	if ( rMessage.LoadString(nID) )
	{
		// first newline terminates actual string
      rMessage.Replace(_T('\n'),_T('\0'));
	}
	else
	{
		// not found
		TRACE1("Warning (CPGSuperAppPlugin): no message line prompt for ID %d.\n", nID);
	}

   return TRUE;
}

BOOL CPGSuperAppPlugin::GetToolTipMessageString(UINT nID, CString& rMessage) const
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   CString string;
   // load appropriate string
	if ( string.LoadString(nID) )
	{
		// tip is after first newline 
      int pos = string.Find('\n');
      if ( 0 < pos )
         rMessage = string.Mid(pos+1);
	}
	else
	{
		// not found
		TRACE1("Warning (CPGSuperAppPlugin): no tool tip for ID %d.\n", nID);
	}

   return TRUE;
}

void CPGSuperAppPlugin::UpdateTemplates()
{
   USES_CONVERSION;

   CAutoRegistry autoReg(GetAppName());

   CEAFApp* pApp = EAFGetApp();

   int result = AfxMessageBox(_T("All of the template library entries will be updated to match the Master Library.\n\nDo you want to proceed?"),MB_YESNO);
   if ( result == IDNO )
      return;

   m_bUpdatingTemplate = true;

   // Get the application into a "just started" state
   if ( pApp->SaveAllModified() )
   {
      pApp->CloseAllDocuments(FALSE);
   }
   else
   {
      AfxMessageBox(_T("Unable to save and close the open document. Template Update cancelled"));
      return;
   }

   // take note of the state of all extension agents
   // disable all extension agents
   // NOTE: This only changes the values in the registery... the actual extensions are not
   // unloaded and disabled... this is a bug
   std::vector<std::pair<CString,CString>> extension_states;
   CComPtr<ICatRegister> pICatReg;
   HRESULT hr = pICatReg.CoCreateInstance(CLSID_StdComponentCategoriesMgr);
   if ( FAILED(hr) )
   {
      AfxMessageBox(_T("Failed to create the component category manager"));
      return;
   }

   CComQIPtr<ICatInformation> pICatInfo(pICatReg);
   CComPtr<IEnumCLSID> pIEnumCLSID;

   const int nID = 1;
   CATID ID[nID];

   ID[0] = CATID_PGSuperExtensionAgent;
   pICatInfo->EnumClassesOfCategories(nID,ID,0,nullptr,&pIEnumCLSID);

   const int nPlugins = 5;
   CLSID clsid[nPlugins]; 
   ULONG nFetched = 0;

   CString strSection(_T("Extensions"));

   while ( SUCCEEDED(pIEnumCLSID->Next(nPlugins,clsid,&nFetched)) && 0 < nFetched)
   {
      for ( ULONG i = 0; i < nFetched; i++ )
      {
         LPOLESTR pszCLSID;
         ::StringFromCLSID(clsid[i],&pszCLSID);
         
         CString strState = pApp->GetProfileString(strSection,OLE2T(pszCLSID),_T("Enabled"));
         extension_states.emplace_back(OLE2T(pszCLSID),strState);

         // Disable the extension
         pApp->WriteProfileString(strSection,OLE2T(pszCLSID),_T("Disabled"));

         ::CoTaskMemFree((void*)pszCLSID);
      }
   }

   // Update the templates
   POSITION pos = pApp->GetFirstDocTemplatePosition();
   CDocTemplate* pTemplate = pApp->GetNextDocTemplate(pos);
   while ( pTemplate )
   {
      if ( pTemplate->IsKindOf(RUNTIME_CLASS(CPGSuperDocTemplate)) )
      {
         CPGSuperDoc* pPGSuperDoc = (CPGSuperDoc*)pTemplate->CreateNewDocument();
         pPGSuperDoc->m_bAutoDelete = false;

         pPGSuperDoc->UpdateTemplates();

         delete pPGSuperDoc;
         break;
      }

      pTemplate = pApp->GetNextDocTemplate(pos);
   }
   AfxMessageBox(_T("Update complete"),MB_OK);
   m_bUpdatingTemplate = false;

   // re-set extension agents state
   std::vector<std::pair<CString,CString>>::iterator iter;
   for ( iter = extension_states.begin(); iter != extension_states.end(); iter++ )
   {
      pApp->WriteProfileString(strSection,iter->first,iter->second);
   }
}

bool CPGSuperAppPlugin::UpdatingTemplates()
{
   return m_bUpdatingTemplate;
}

CEAFCommandLineInfo* CPGSuperAppPlugin::CreateCommandLineInfo() const
{
   return new CPGSuperCommandLineInfo();
}

LPCTSTR CPGSuperAppPlugin::GetCatalogServerKey() const
{
   return _T("CatalogServer2");
}

LPCTSTR CPGSuperAppPlugin::GetPublisherKey() const
{
   return _T("Publisher2");
}

LPCTSTR CPGSuperAppPlugin::GetMasterLibraryCacheKey() const
{
   return _T("MasterLibraryCache2");
}

LPCTSTR CPGSuperAppPlugin::GetMasterLibraryURLKey() const
{
   return _T("MasterLibraryURL2");
}

LPCTSTR CPGSuperAppPlugin::GetWorkgroupTemplatesCacheKey() const
{
   return _T("WorkgroupTemplatesCache2");
}

CString CPGSuperAppPlugin::GetCacheFolder() const
{
   CAutoRegistry autoReg(GetAppName());

   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   CWinApp* pMyApp     = AfxGetApp();
   CEAFApp* pParentApp = EAFGetApp();

   LPWSTR path;
   HRESULT hr = ::SHGetKnownFolderPath(FOLDERID_RoamingAppData, KF_FLAG_DEFAULT, NULL, &path);

   if ( SUCCEEDED(hr) )
   {
      return CString(path) + CString(_T("\\PGSuperV3\\"));
   }
   else
   {
      return pParentApp->GetAppLocation() + CString(_T("PGSuperV3\\"));
   }
}
