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
#include "PGSplicePluginApp.h"
#include "PGSpliceCatCom.h"
#include "resource.h"

#include "PGSpliceDoc.h"
#include "PGSpliceDocTemplate.h"
#include "BridgeModelViewChildFrame.h"
#include "BridgePlanView.h"
#include <PsgLib\BeamFamilyManager.h>

#include "PGSpliceCommandLineInfo.h"

#include "PluginManagerDlg.h"

#include <EAF\EAFMainFrame.h>

#include <MFCTools\AutoRegistry.h>




BEGIN_MESSAGE_MAP(CPGSplicePluginApp,CCmdTarget)
   ON_COMMAND(ID_MANAGE_PLUGINS,OnConfigurePlugins)
   ON_COMMAND(ID_UPDATE_TEMPLATE,OnUpdateTemplates)
END_MESSAGE_MAP()

void CPGSplicePluginApp::OnConfigurePlugins()
{
   ConfigurePlugins();
}

void CPGSplicePluginApp::OnUpdateTemplates()
{
   UpdateTemplates();
}


//////////////////////////////////////////////////////////

CString CPGSplicePluginApp::GetTemplateFileExtension()
{ 
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CString strTemplateSuffix;
   VERIFY(strTemplateSuffix.LoadString(IDS_PGSPLICE_TEMPLATE_FILE_SUFFIX));
   ASSERT(!strTemplateSuffix.IsEmpty());
   return strTemplateSuffix;
}

const CRuntimeClass* CPGSplicePluginApp::GetDocTemplateRuntimeClass()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   return RUNTIME_CLASS(CPGSpliceDocTemplate);
}

void CPGSplicePluginApp::ConfigurePlugins()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CAutoRegistry autoReg(GetAppName());

   CPluginManagerDlg dlg(_T("Manage PGSplice Plugins and Extensions"),EAFGetMainFrame(),0,CATID_PGSpliceDataImporter,CATID_PGSpliceDataExporter,CATID_PGSpliceExtensionAgent,GetAppName());
   dlg.DoModal(); // this DoModal is correct... the dialog takes care of its own data
}

BOOL CPGSplicePluginApp::Init(CEAFApp* pParent)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   CWinApp* pMyApp = AfxGetApp();

   InitCatalogServer();

   // See MSKB Article ID: Q118435, _T("Sharing Menus Between MDI Child Windows")
   m_hMenuShared = ::LoadMenu( pMyApp->m_hInstance, MAKEINTRESOURCE(IDR_PGSPLICE) );

   if ( m_hMenuShared == nullptr )
      return FALSE;

   if (!EAFGetApp()->GetCommandLineInfo().m_bCommandLineMode && !EAFGetApp()->IsFirstRun())
   {
      // Don't update cache if there are other instances of bridgelink running. This avoids race condition on libraries and templates
      if (!EAFAreOtherProgramInstancesRunning())
      {
         UpdateCache(); // we don't want to do this if we are running in batch/command line mode or if this is the first run situation (because configuration will happen later)
      }
   }

   return TRUE;
}

void CPGSplicePluginApp::Terminate()
{
   TerminateCatalogServer();

   // release the shared menu
   ::DestroyMenu( m_hMenuShared );
}

void CPGSplicePluginApp::IntegrateWithUI(BOOL bIntegrate)
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
      pManageMenu->AppendMenu(ID_MANAGE_PLUGINS,_T("PGSplice Plugins and Extensions..."),callback);

      // Alt+Ctrl+I
      pFrame->GetAcceleratorTable()->AddAccelKey(FALT | FCONTROL | FVIRTKEY, VK_I, ID_UPDATE_TEMPLATE,callback);
   }
   else
   {
      pManageMenu->RemoveMenu(ID_MANAGE_PLUGINS,  MF_BYCOMMAND, callback);

      pFrame->GetAcceleratorTable()->RemoveAccelKey(ID_UPDATE_TEMPLATE,callback);
   }
}

std::vector<CEAFDocTemplate*> CPGSplicePluginApp::CreateDocTemplates()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CPGSpliceDocTemplate* pTemplate = new CPGSpliceDocTemplate(
		IDR_PGSPLICE,
      nullptr,
		RUNTIME_CLASS(CPGSpliceDoc),
		RUNTIME_CLASS(CBridgeModelViewChildFrame),
		RUNTIME_CLASS(CBridgePlanView),
      m_hMenuShared,1);

   std::vector<CEAFDocTemplate*> vDocTemplates;
   vDocTemplates.push_back(pTemplate);
   return vDocTemplates;
}

HMENU CPGSplicePluginApp::GetSharedMenuHandle()
{
   return m_hMenuShared;
}

CString CPGSplicePluginApp::GetName()
{
   return CString(_T("PGSplice"));
}

CString CPGSplicePluginApp::GetDocumentationSetName()
{
   return GetName();
}

CString CPGSplicePluginApp::GetDocumentationURL()
{
   return CPGSPluginAppBase::GetDocumentationURL();
}

CString CPGSplicePluginApp::GetDocumentationMapFile()
{
   return CPGSPluginAppBase::GetDocumentationMapFile();
}

void CPGSplicePluginApp::LoadDocumentationMap()
{
   return CPGSPluginAppBase::LoadDocumentationMap();
}

std::pair<WBFL::EAF::HelpResult,CString> CPGSplicePluginApp::GetDocumentLocation(LPCTSTR lpszDocSetName,UINT nID)
{
   return CPGSPluginAppBase::GetDocumentLocation(lpszDocSetName,nID);
}

CString CPGSplicePluginApp::GetCommandLineAppName() const
{
   return GetAppName();
}

CString CPGSplicePluginApp::GetUsageMessage()
{
   CPGSpliceCommandLineInfo pgsCmdInfo;
   return pgsCmdInfo.GetUsageMessage();
}

BOOL CPGSplicePluginApp::ProcessCommandLineOptions(CEAFCommandLineInfo& cmdInfo)
{
   return DoProcessCommandLineOptions(cmdInfo);
}

//////////////////////////
// IEAFCommandCallback
BOOL CPGSplicePluginApp::OnCommandMessage(UINT nID,int nCode,void* pExtra,AFX_CMDHANDLERINFO* pHandlerInfo)
{
   return OnCmdMsg(nID,nCode,pExtra,pHandlerInfo);
}

BOOL CPGSplicePluginApp::GetStatusBarMessageString(UINT nID, CString& rMessage) const
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   // load appropriate string
	if ( rMessage.LoadString(nID) )
	{
		// first newline terminates actual string
      rMessage.Replace('\n','\0');
	}
	else
	{
		// not found
		TRACE1("Warning (CPGSplicePluginApp): no message line prompt for ID %d.\n", nID);
	}

   return TRUE;
}

BOOL CPGSplicePluginApp::GetToolTipMessageString(UINT nID, CString& rMessage) const
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
		TRACE1("Warning (CPGSplicePluginApp): no tool tip for ID %d.\n", nID);
	}

   return TRUE;
}

void CPGSplicePluginApp::UpdateTemplates()
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
   // NOTE: This only changes the values in the registry... the actual extensions are not
   // unloaded and disabled... this is a bug

   std::vector<std::pair<CString,CString>> extension_states;
   CString strSection(_T("Extensions"));

   auto components = WBFL::EAF::ComponentManager::GetInstance().GetComponents(CATID_PGSpliceExtensionAgent);
   for (auto& component : components)
   {
      LPOLESTR pszCLSID;
      ::StringFromCLSID(component.clsid, &pszCLSID);

      CString strState = pApp->GetProfileString(strSection, OLE2T(pszCLSID), _T("Enabled"));
      extension_states.emplace_back(OLE2T(pszCLSID), strState);

      // Disable the extension
      pApp->WriteProfileString(strSection, OLE2T(pszCLSID), _T("Disabled"));
      ::CoTaskMemFree((void*)pszCLSID);
   }

   // Update the templates
   POSITION pos = pApp->GetFirstDocTemplatePosition();
   CDocTemplate* pTemplate = pApp->GetNextDocTemplate(pos);
   while ( pTemplate )
   {
      if ( pTemplate->IsKindOf(RUNTIME_CLASS(CPGSpliceDocTemplate)) )
      {
         CPGSpliceDoc* pPGSpliceDoc = (CPGSpliceDoc*)pTemplate->CreateNewDocument();
         pPGSpliceDoc->m_bAutoDelete = false;

         pPGSpliceDoc->UpdateTemplates();

         delete pPGSpliceDoc;
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

bool CPGSplicePluginApp::UpdatingTemplates()
{
   return m_bUpdatingTemplate;
}

CEAFCommandLineInfo* CPGSplicePluginApp::CreateCommandLineInfo() const
{
   return new CPGSpliceCommandLineInfo();
}
