#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperProjectImporterAppPlugin.h"
#include "PGSuperImportPluginDocTemplate.h"
#include "resource.h"
#include "BridgeModelViewChildFrame.h"
#include "BridgePlanView.h"
#include "PGSuperProjectImporterMgr.h"
#include "PGSuperCommandLineInfo.h"

#include <EAF\EAFDocManager.h>


BEGIN_MESSAGE_MAP(CProjectImportersCmdTarget,CCmdTarget)
   ON_COMMAND(ID_MANAGE_PLUGINS,OnConfigureProjectImporters)
END_MESSAGE_MAP()

void CProjectImportersCmdTarget::OnConfigureProjectImporters()
{
   m_pMyAppPlugin->ConfigureProjectImporters();
}

CString CPGSuperProjectImporterAppPlugin::GetTemplateFileExtension()
{ 
   CString strTemplateSuffix;
   VERIFY(strTemplateSuffix.LoadString(IDS_PGSUPER_TEMPLATE_FILE_SUFFIX));
   ASSERT(!strTemplateSuffix.IsEmpty());
   return strTemplateSuffix;
}

// NOTE
// PGSuper Project Importer application plug in is basically the same thing as the
// PGSuper application plug in. The only differece is this plug-in provides a 
// framework for creating new PGSuper project from extern data sources. When this
// plug-in is loaded and unloaded it reads and writes from the registery. It reads
// and writes to the same location as PGSuper app plugin. This works fine for 
// everything except the custom reporting options. Which ever plug-in saves last
// wins. Since we don't have any real project importers yet, it is just easier
// to by-pass saving custom report information. This will have to be fixed later
void CPGSuperProjectImporterAppPlugin::SaveReportOptions()
{
   // do nothing
}

HRESULT CPGSuperProjectImporterAppPlugin::FinalConstruct()
{
   m_MyCmdTarget.m_pMyAppPlugin = this;
   return OnFinalConstruct(); // CPGSuperBaseAppPlugin
}

void CPGSuperProjectImporterAppPlugin::FinalRelease()
{
   OnFinalRelease(); // CPGSuperBaseAppPlugin
}

void CPGSuperProjectImporterAppPlugin::ConfigureProjectImporters()
{
   std::vector<CEAFPluginState> pluginStates = EAFManageApplicationPlugins(_T("Manage PGSuper Project Importers"),CATID_PGSuperProjectImporter,EAFGetMainFrame());
   if ( pluginStates.size() == 0 )
   {
      return;
   }

   // Find our document template
   CEAFApp* pApp = EAFGetApp();

   POSITION template_position;
   CPGSuperImportPluginDocTemplate* pMyTemplate = NULL;
   POSITION pos = pApp->m_pDocManager->GetFirstDocTemplatePosition();
   while ( pos != NULL )
   {
      template_position = pos;
      CEAFDocTemplate* pTemplate = (CEAFDocTemplate*)(pApp->m_pDocManager->GetNextDocTemplate(pos));
      if ( pTemplate->IsKindOf(RUNTIME_CLASS(CPGSuperImportPluginDocTemplate)) )
      {
         pMyTemplate = (CPGSuperImportPluginDocTemplate*)pTemplate;
      }
   }

   // our doc template was not found...
   if ( pMyTemplate == NULL )
   {
      // when the doc template is created, it loads all the enabled plug-in objects
      // write the plugin states into the registry and then create the doc template
      std::vector<CEAFPluginState>::iterator stateIter(pluginStates.begin());
      std::vector<CEAFPluginState>::iterator stateIterEnd(pluginStates.end());
      for ( ; stateIter != stateIterEnd; stateIter++ )
      {
         CEAFPluginState& state = *stateIter;
         pApp->WriteProfileString(_T("Plugins"),state.GetCLSIDString(),state.IsEnabled() ? _T("Enabled") : _T("Disabled") );
      }

      std::vector<CEAFDocTemplate*> vDocTemplates = CreateDocTemplates();
      std::vector<CEAFDocTemplate*>::iterator templateIter(vDocTemplates.begin());
      std::vector<CEAFDocTemplate*>::iterator templateIterEnd(vDocTemplates.end());
      for ( ; templateIter != templateIterEnd; templateIter++ )
      {
         pMyTemplate = (CPGSuperImportPluginDocTemplate*)*templateIter;
         pApp->m_pDocManager->AddDocTemplate(pMyTemplate);
      }
      return;
   }

   // Set the state of the importer plugins, create and destroy them as needed
   CPGSuperProjectImporterMgr& importerMgr = pMyTemplate->GetProjectImporterManager();
   std::vector<CEAFPluginState>::iterator iter;
   for ( iter = pluginStates.begin(); iter != pluginStates.end(); iter++ )
   {
      CEAFPluginState& state = *iter;
      if ( state.StateChanged() )
      {
         if ( state.InitiallyEnabled() )
         {
            // importer was initially enabled, but now it is not
            importerMgr.RemoveImporter( state.GetCLSID() );
            pApp->WriteProfileString(_T("Plugins"),state.GetCLSIDString(),_T("Disabled"));
         }
         else
         {
            // importer was not initially enabled, but now it is
            CComPtr<IPGSuperProjectImporter> importer;
            importer.CoCreateInstance(state.GetCLSID());
            importerMgr.AddImporter(state.GetCLSID(),importer);
            pApp->WriteProfileString(_T("Plugins"),state.GetCLSIDString(),_T("Enabled"));
         }
      }
   }

   if ( importerMgr.GetImporterCount() == 0 )
   {
      // there aren't any importers enabled, so we can remove the template from
      // the document manager
      ((CEAFDocManager*)pApp->m_pDocManager)->RemoveDocTemplate(template_position);
   }
}

CPGSuperBaseCommandLineInfo* CPGSuperProjectImporterAppPlugin::CreateCommandLineInfo() const
{
   return new CPGSuperCommandLineInfo;
}

BOOL CPGSuperProjectImporterAppPlugin::Init(CEAFApp* pParent)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   DefaultInit();

   // See MSKB Article ID: Q118435, "Sharing Menus Between MDI Child Windows"
   m_hMenuShared = ::LoadMenu( AfxGetApp()->m_hInstance, MAKEINTRESOURCE(IDR_PGSUPER) );

   return (m_hMenuShared != NULL);
}

void CPGSuperProjectImporterAppPlugin::Terminate()
{
   DefaultTerminate();

   // release the shared menu
   ::DestroyMenu( m_hMenuShared );
}

void CPGSuperProjectImporterAppPlugin::IntegrateWithUI(BOOL bIntegrate)
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
      pManageMenu->AppendMenu(ID_MANAGE_PLUGINS,_T("PGSuper Project Importers..."),this);
   }
   else
   {
      pManageMenu->RemoveMenu(ID_MANAGE_PLUGINS,  MF_BYCOMMAND, this);
   }
}

std::vector<CEAFDocTemplate*> CPGSuperProjectImporterAppPlugin::CreateDocTemplates()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   std::vector<CEAFDocTemplate*> vDocTemplates;

   CPGSuperImportPluginDocTemplate* pDocTemplate = new CPGSuperImportPluginDocTemplate(
		IDR_PROJECTIMPORTER,
      this,
		RUNTIME_CLASS(CPGSuperDoc),
		RUNTIME_CLASS(CBridgeModelViewChildFrame),
		RUNTIME_CLASS(CBridgePlanView),
      m_hMenuShared,1);

   // If there aren't any importers, we don't want the "PGSuper Project Importer" option to
   // show up in the File | New dialog
   // Returning a NULL doc template will do the trick
   const CPGSuperProjectImporterMgr& importerMgr = pDocTemplate->GetProjectImporterManager();
   if ( importerMgr.GetImporterCount() == 0 )
   {
      delete pDocTemplate;
      pDocTemplate = NULL;
   }

   if ( pDocTemplate != NULL )
   {
      vDocTemplates.push_back(pDocTemplate);
   }

   return vDocTemplates;
}

HMENU CPGSuperProjectImporterAppPlugin::GetSharedMenuHandle()
{
   return m_hMenuShared;
}

CString CPGSuperProjectImporterAppPlugin::GetName()
{
   return CString(_T("PGSuper Project Importer"));
}

//////////////////////////
// IEAFCommandCallback
BOOL CPGSuperProjectImporterAppPlugin::OnCommandMessage(UINT nID,int nCode,void* pExtra,AFX_CMDHANDLERINFO* pHandlerInfo)
{
   return m_MyCmdTarget.OnCmdMsg(nID,nCode,pExtra,pHandlerInfo);
}

BOOL CPGSuperProjectImporterAppPlugin::GetStatusBarMessageString(UINT nID, CString& rMessage) const
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
		TRACE1("Warning (CPGSuperProjectImporterAppPlugin): no message line prompt for ID %d.\n", nID);
	}

   return TRUE;
}

BOOL CPGSuperProjectImporterAppPlugin::GetToolTipMessageString(UINT nID, CString& rMessage) const
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
		TRACE1("Warning (CPGSuperProjectImporterAppPlugin): no tool tip for ID %d.\n", nID);
	}

   return TRUE;
}
