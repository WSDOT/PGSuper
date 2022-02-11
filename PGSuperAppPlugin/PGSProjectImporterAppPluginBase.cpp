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

#include "stdafx.h"
#include "PGSProjectImporterAppPluginBase.h"
#include "resource.h"
#include <EAF\EAFDocManager.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



BEGIN_MESSAGE_MAP(CProjectImportersCmdTarget,CCmdTarget)
   ON_COMMAND(ID_MANAGE_PLUGINS,OnConfigureProjectImporters)
END_MESSAGE_MAP()

void CProjectImportersCmdTarget::OnConfigureProjectImporters()
{
   m_pMyAppPlugin->ConfigureProjectImporters();
}

// NOTE
// PGS Project Importer application plug-ins are basically the same thing as the
// PGS application plug-ins. The only differece is this plug-in provides a 
// framework for creating new PGS projects from external data sources. When this
// plug-in is loaded and unloaded it reads and writes from the registery. It reads
// and writes to the same location as a PGS App Plug-in. This works fine for 
// everything except the custom reporting and catalog server information. Whichever plug-in saves last
// wins. Since we don't have any real project importers yet, it is just easier
// to by-pass saving custom report information. This will have to be fixed later.
void CPGSProjectImporterAppPluginBase::SaveReportOptions()
{
   // do nothing
}

void CPGSProjectImporterAppPluginBase::SaveRegistryValues()
{
   // do nothing (See note above)
}

HRESULT CPGSProjectImporterAppPluginBase::FinalConstruct()
{
   m_MyCmdTarget.m_pMyAppPlugin = this;
   return OnFinalConstruct(); // CPGSAppPluginBase
}

void CPGSProjectImporterAppPluginBase::FinalRelease()
{
   OnFinalRelease(); // CPGSAppPluginBase
}

void CPGSProjectImporterAppPluginBase::ConfigureProjectImporters()
{
   CString str;
   str.Format(_T("Manage %s Project Importers"),GetAppName());
   CATID catid = GetProjectImporterCATID();
   std::vector<CEAFPluginState> vPluginStates = EAFManageApplicationPlugins(str,_T("Select the Project Importers that you want to be available."),catid,EAFGetMainFrame(),IDH_PLUGINS,GetAppName());
   if ( vPluginStates.size() == 0 )
   {
      return;
   }

   // Find our document template
   CEAFApp* pApp = EAFGetApp();

   POSITION template_position;
   CPGSImportPluginDocTemplateBase* pMyTemplate = nullptr;
   POSITION pos = pApp->m_pDocManager->GetFirstDocTemplatePosition();
   while ( pos != nullptr )
   {
      template_position = pos;
      CEAFDocTemplate* pTemplate = (CEAFDocTemplate*)(pApp->m_pDocManager->GetNextDocTemplate(pos));
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
            CComPtr<IPGSProjectImporter> importer;
            importer.CoCreateInstance(state.GetCLSID());
            pImporterMgr->AddImporter(state.GetCLSID(),importer);
            pApp->WriteProfileString(_T("Plugins"),state.GetCLSIDString(),_T("Enabled"));
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

BOOL CPGSProjectImporterAppPluginBase::DoProcessCommandLineOptions(CEAFCommandLineInfo& cmdInfo)
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

void CPGSProjectImporterAppPluginBase::CreateNewProject(CPGSProjectImporterBaseCommandLineInfo& cmdInfo)
{
   ATLASSERT(cmdInfo.m_bNewProject);

   CEAFApp* pApp = EAFGetApp();

   CEAFBrokerDocument* pDoc = nullptr;
   POSITION pos = pApp->m_pDocManager->GetFirstDocTemplatePosition();
   while (pos != nullptr)
   {
      CPGSImportPluginDocTemplateBase* pTemplate = (CPGSImportPluginDocTemplateBase*)pApp->m_pDocManager->GetNextDocTemplate(pos);
      CComPtr<IEAFAppPlugin> pAppPlugin;
      pTemplate->GetPlugin(&pAppPlugin);
      CComPtr<IEAFAppPlugin> me(this);
      if (pAppPlugin.IsEqualObject(me))
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

BOOL CPGSProjectImporterAppPluginBase::Init(CEAFApp* pParent)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   DefaultInit(this);

   // See MSKB Article ID: Q118435, "Sharing Menus Between MDI Child Windows"
   UINT nResource = GetMenuResourceID();
   m_hMenuShared = ::LoadMenu( AfxGetApp()->m_hInstance, MAKEINTRESOURCE(nResource) );

   return (m_hMenuShared != nullptr);
}

void CPGSProjectImporterAppPluginBase::Terminate()
{
   DefaultTerminate();

   // release the shared menu
   ::DestroyMenu( m_hMenuShared );
}

void CPGSProjectImporterAppPluginBase::IntegrateWithUI(BOOL bIntegrate)
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
      CString str;
      str.Format(_T("%s Project Importers..."),GetAppName());
      pManageMenu->AppendMenu(ID_MANAGE_PLUGINS,str,this);
   }
   else
   {
      pManageMenu->RemoveMenu(ID_MANAGE_PLUGINS,  MF_BYCOMMAND, this);
   }
}

std::vector<CEAFDocTemplate*> CPGSProjectImporterAppPluginBase::CreateDocTemplates()
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

HMENU CPGSProjectImporterAppPluginBase::GetSharedMenuHandle()
{
   return m_hMenuShared;
}

CString CPGSProjectImporterAppPluginBase::GetName()
{
   CString str;
   str.Format(_T("%s"),GetAppName());
   return str;
}

CString CPGSProjectImporterAppPluginBase::GetDocumentationSetName()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   return AfxGetAppName();
}

CString CPGSProjectImporterAppPluginBase::GetDocumentationURL()
{
   return CPGSAppPluginBase::GetDocumentationURL();
}

CString CPGSProjectImporterAppPluginBase::GetDocumentationMapFile()
{
   return CPGSAppPluginBase::GetDocumentationMapFile();
}

void CPGSProjectImporterAppPluginBase::LoadDocumentationMap()
{
   return CPGSAppPluginBase::LoadDocumentationMap();
}

eafTypes::HelpResult CPGSProjectImporterAppPluginBase::GetDocumentLocation(LPCTSTR lpszDocSetName,UINT nID,CString& strURL)
{
   return CPGSAppPluginBase::GetDocumentLocation(lpszDocSetName,nID,strURL);
}

//////////////////////////
// IEAFCommandCallback
BOOL CPGSProjectImporterAppPluginBase::OnCommandMessage(UINT nID,int nCode,void* pExtra,AFX_CMDHANDLERINFO* pHandlerInfo)
{
   return m_MyCmdTarget.OnCmdMsg(nID,nCode,pExtra,pHandlerInfo);
}

BOOL CPGSProjectImporterAppPluginBase::GetStatusBarMessageString(UINT nID, CString& rMessage) const
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

BOOL CPGSProjectImporterAppPluginBase::GetToolTipMessageString(UINT nID, CString& rMessage) const
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

CString CPGSProjectImporterAppPluginBase::GetCommandLineAppName() const
{
   return GetAppName() + CString(_T("ProjectImporter"));
}

BOOL CPGSProjectImporterAppPluginBase::ProcessCommandLineOptions(CEAFCommandLineInfo& cmdInfo)
{
   return DoProcessCommandLineOptions(cmdInfo);
}