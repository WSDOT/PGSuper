///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2025  Washington State Department of Transportation
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
#include "resource.h"
#include "PGSProjectImporterPluginAppBase.h"
#include <EAF\EAFDocManager.h>


BEGIN_MESSAGE_MAP(CProjectImportersCmdTarget,CCmdTarget)
   ON_COMMAND(ID_MANAGE_PLUGINS,OnConfigureProjectImporters)
END_MESSAGE_MAP()

void CProjectImportersCmdTarget::OnConfigureProjectImporters()
{
   m_pMyAppPlugin->ConfigureProjectImporters();
}

// NOTE
// PGS Project Importer application plug-ins are basically the same thing as the
// PGS application plug-ins. The only difference is this plug-in provides a 
// framework for creating new PGS projects from external data sources. When this
// plug-in is loaded and unloaded it reads and writes from the registry. It reads
// and writes to the same location as a PGS App Plug-in. This works fine for 
// everything except the custom reporting and catalog server information. Whichever plug-in saves last
// wins. Since we don't have any real project importers yet, it is just easier
// to by-pass saving custom report information. This will have to be fixed later.
void CPGSProjectImporterPluginAppBase::SaveReportOptions()
{
   // do nothing
}

void CPGSProjectImporterPluginAppBase::SaveRegistryValues()
{
   // do nothing (See note above)
}

void CPGSProjectImporterPluginAppBase::ConfigureProjectImporters()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   CEAFApp* pApp = (CEAFApp*)AfxGetApp(); // DLLs CEAFApp

   CString str;
   str.Format(_T("Manage %s Project Importers"),GetAppName());
   CATID catid = GetProjectImporterCATID();
   std::vector<CEAFPluginState> vPluginStates = EAFManageApplicationPlugins(str,_T("Select the Project Importers that you want to be available."),catid,EAFGetMainFrame(), pApp,IDH_PLUGINS,GetAppName());
   if ( vPluginStates.size() == 0 )
   {
      return;
   }

   // Find our document template
   CEAFApp* pEAFApp = EAFGetApp(); // EXEs CEAFApp

   POSITION template_position;
   CPGSImportPluginDocTemplateBase* pMyTemplate = nullptr;
   POSITION pos = pEAFApp->m_pDocManager->GetFirstDocTemplatePosition();
   while ( pos != nullptr )
   {
      template_position = pos;
      CEAFDocTemplate* pTemplate = (CEAFDocTemplate*)(pEAFApp->m_pDocManager->GetNextDocTemplate(pos));
      if ( pTemplate->IsKindOf(RUNTIME_CLASS(CPGSImportPluginDocTemplateBase)) )
      {
         pMyTemplate = (CPGSImportPluginDocTemplateBase*)pTemplate;
      }
   }

   // our doc template was not found...
   if ( pMyTemplate == nullptr )
   {
      // when the doc template is created, it loads all the enabled plug-in objects
      // write the plugin states into the registry and then create the doc template
      for (const auto& state : vPluginStates)
      {
         pApp->WriteProfileString(_T("Plugins"),state.GetCLSIDString(),state.IsEnabled() ? _T("Enabled") : _T("Disabled") );
      }

      std::vector<CEAFDocTemplate*> vDocTemplates = CreateDocTemplates();
      for (const auto& pTemplate : vDocTemplates)
      {
         pApp->m_pDocManager->AddDocTemplate(pTemplate);
      }
      return;
   }

   // Set the state of the importer plugins, create and destroy them as needed
   CPGSProjectImporterMgrBase* pImporterMgr = pMyTemplate->GetProjectImporterManager();
   for (const auto& state : vPluginStates)
   {
      if ( state.StateChanged() )
      {
         if ( state.InitiallyEnabled() )
         {
            // importer was initially enabled, but now it is not
            pImporterMgr->RemoveImporter( state.GetCLSID() );
            pApp->WriteProfileString(_T("Plugins"),state.GetCLSIDString(),_T("Disabled"));
         }
         else
         {
            // importer was not initially enabled, but now it is
            auto importer = WBFL::EAF::ComponentManager::GetInstance().CreateComponent<PGS::IProjectImporter>(state.GetCLSID());
            if (importer)
            {
               pImporterMgr->AddImporter(state.GetCLSID(), importer);
               pApp->WriteProfileString(_T("Plugins"), state.GetCLSIDString(), _T("Enabled"));
            }
            else
            {
               CString strMsg;
               strMsg.Format(_T("Failed to load %s Project Importer plug in."), state.GetName());
               AfxMessageBox(strMsg, MB_OK | MB_ICONEXCLAMATION);
               pApp->WriteProfileString(_T("Plugins"), state.GetCLSIDString(), _T("Disabled"));
            }
         }
      }
   }

   if ( pImporterMgr->GetImporterCount() == 0 )
   {
      // there aren't any importers enabled, so we can remove the template from
      // the document manager
      ((CEAFDocManager*)pApp->m_pDocManager)->RemoveDocTemplate(template_position);
   }
}

BOOL CPGSProjectImporterPluginAppBase::DoProcessCommandLineOptions(CEAFCommandLineInfo& cmdInfo)
{
//   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   // cmdInfo is the command line information from the application. The application
   // doesn't know about this plug-in at the time the command line parameters are parsed
   //
   // Re-parse the parameters with our own command line information object
   std::unique_ptr<CPGSProjectImporterBaseCommandLineInfo> pgsCmdInfo((CPGSProjectImporterBaseCommandLineInfo*)CreateCommandLineInfo());
   if (pgsCmdInfo.get() != nullptr)
   {
      EAFGetApp()->ParseCommandLine(*pgsCmdInfo);
      cmdInfo = *pgsCmdInfo;

      if (pgsCmdInfo->m_bNewProject)
      {
         CreateNewProject(*pgsCmdInfo);
         return TRUE; // command line parameters handled
      }

      if (pgsCmdInfo->m_bError)
      {
         return FALSE;
      }
   }

   return FALSE;
}

void CPGSProjectImporterPluginAppBase::CreateNewProject(CPGSProjectImporterBaseCommandLineInfo& cmdInfo)
{
   ATLASSERT(cmdInfo.m_bNewProject);

   CEAFApp* pApp = EAFGetApp();

   CEAFBrokerDocument* pDoc = nullptr;
   POSITION pos = pApp->m_pDocManager->GetFirstDocTemplatePosition();
   while (pos != nullptr)
   {
      CPGSImportPluginDocTemplateBase* pTemplate = (CPGSImportPluginDocTemplateBase*)pApp->m_pDocManager->GetNextDocTemplate(pos);
      auto pluginApp = pTemplate->GetPluginApp();
      auto me = std::dynamic_pointer_cast<WBFL::EAF::IPluginApp>(shared_from_this());
      if (pluginApp == me)
      {
         CLSID clsid;
         HRESULT hr = ::CLSIDFromString(cmdInfo.m_strCLSID, &clsid);
         if (hr != NOERROR)
         {
            cmdInfo.m_bError = TRUE;
            return;
         }

         CEAFTemplateItem* pItem = pTemplate->GetTemplateItem(clsid);
         pTemplate->SetTemplateItem(pItem);
         pDoc = (CEAFBrokerDocument*)pTemplate->OpenDocumentFile(nullptr, FALSE, TRUE);
         break;
      }
   }

   if (pDoc == nullptr)
   {
      cmdInfo.m_bError = TRUE;
   }
}

BOOL CPGSProjectImporterPluginAppBase::Init(CEAFApp* pParent)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   InitCatalogServer();

   // See MSKB Article ID: Q118435, "Sharing Menus Between MDI Child Windows"
   UINT nResource = GetMenuResourceID();
   m_hMenuShared = ::LoadMenu( AfxGetApp()->m_hInstance, MAKEINTRESOURCE(nResource) );

   return (m_hMenuShared != nullptr);
}

void CPGSProjectImporterPluginAppBase::Terminate()
{
   TerminateCatalogServer();

   // release the shared menu
   ::DestroyMenu( m_hMenuShared );
}

void CPGSProjectImporterPluginAppBase::IntegrateWithUI(BOOL bIntegrate)
{
   CEAFMainFrame* pFrame = EAFGetMainFrame();
   auto pMainMenu = pFrame->GetMainMenu();

   UINT filePos = pMainMenu->FindMenuItem(_T("&File"));
   auto pFileMenu = pMainMenu->GetSubMenu(filePos);

   UINT managePos = pFileMenu->FindMenuItem(_T("Manage"));
   auto pManageMenu = pFileMenu->GetSubMenu(managePos);

   auto callback = std::dynamic_pointer_cast<WBFL::EAF::ICommandCallback>(shared_from_this());

   if ( bIntegrate )
   {
      // Append to the end of the Manage menu
      CString str;
      str.Format(_T("%s Project Importers..."),GetAppName());
      pManageMenu->AppendMenu(ID_MANAGE_PLUGINS,str,callback);
   }
   else
   {
      pManageMenu->RemoveMenu(ID_MANAGE_PLUGINS,  MF_BYCOMMAND, callback);
   }
}

std::vector<CEAFDocTemplate*> CPGSProjectImporterPluginAppBase::CreateDocTemplates()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   std::vector<CEAFDocTemplate*> vDocTemplates;

   // Create the one and only document template for all project importers associated with this Plug-in Application
   CPGSImportPluginDocTemplateBase* pDocTemplate = CreateDocTemplate();

   // If there aren't any importers, we don't want the "PGSuper/PGSplice Project Importer" option to
   // show up in the File | New dialog
   // Returning a nullptr doc template will do the trick
   CPGSProjectImporterMgrBase* pImporterMgr = pDocTemplate->GetProjectImporterManager();
   if ( pImporterMgr->GetImporterCount() == 0 )
   {
      delete pDocTemplate;
      pDocTemplate = nullptr;
   }

   if ( pDocTemplate != nullptr )
   {
      vDocTemplates.push_back(pDocTemplate);
   }

   return vDocTemplates;
}

HMENU CPGSProjectImporterPluginAppBase::GetSharedMenuHandle()
{
   return m_hMenuShared;
}

CString CPGSProjectImporterPluginAppBase::GetName()
{
   CString str;
   str.Format(_T("%s"),GetAppName());
   return str;
}

CString CPGSProjectImporterPluginAppBase::GetDocumentationSetName()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   return AfxGetAppName();
}

CString CPGSProjectImporterPluginAppBase::GetDocumentationURL()
{
   return CPGSPluginAppBase::GetDocumentationURL();
}

CString CPGSProjectImporterPluginAppBase::GetDocumentationMapFile()
{
   return CPGSPluginAppBase::GetDocumentationMapFile();
}

void CPGSProjectImporterPluginAppBase::LoadDocumentationMap()
{
   return CPGSPluginAppBase::LoadDocumentationMap();
}

std::pair<WBFL::EAF::HelpResult,CString> CPGSProjectImporterPluginAppBase::GetDocumentLocation(LPCTSTR lpszDocSetName,UINT nID)
{
   return CPGSPluginAppBase::GetDocumentLocation(lpszDocSetName,nID);
}

//////////////////////////
// IEAFCommandCallback
BOOL CPGSProjectImporterPluginAppBase::OnCommandMessage(UINT nID,int nCode,void* pExtra,AFX_CMDHANDLERINFO* pHandlerInfo)
{
   return m_MyCmdTarget.OnCmdMsg(nID,nCode,pExtra,pHandlerInfo);
}

BOOL CPGSProjectImporterPluginAppBase::GetStatusBarMessageString(UINT nID, CString& rMessage) const
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
		TRACE1("Warning (CPGSProjectImporterAppPluginBase): no message line prompt for ID %d.\n", nID);
	}

   return TRUE;
}

BOOL CPGSProjectImporterPluginAppBase::GetToolTipMessageString(UINT nID, CString& rMessage) const
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   CString string;
   // load appropriate string
	if ( string.LoadString(nID) )
	{
		// tip is after first newline 
      int pos = string.Find('\n');
      if ( 0 < pos )
      {
         rMessage = string.Mid(pos+1);
      }
	}
	else
	{
		// not found
		TRACE1("Warning (CPGSProjectImporterAppPluginBase): no tool tip for ID %d.\n", nID);
	}

   return TRUE;
}

CString CPGSProjectImporterPluginAppBase::GetCommandLineAppName() const
{
   return GetAppName() + CString(_T("ProjectImporter"));
}

BOOL CPGSProjectImporterPluginAppBase::ProcessCommandLineOptions(CEAFCommandLineInfo& cmdInfo)
{
   return DoProcessCommandLineOptions(cmdInfo);
}