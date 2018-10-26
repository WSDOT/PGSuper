#include "PGSuperAppPlugin\stdafx.h"
#include "PGSProjectImporterAppPluginBase.h"
#include "PGSuperAppPlugin\resource.h"
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
// PGS application plug-ins. The only differece is this plug-in provides a 
// framework for creating new PGS projects from external data sources. When this
// plug-in is loaded and unloaded it reads and writes from the registery. It reads
// and writes to the same location as PGS app plug-in. This works fine for 
// everything except the custom reporting options. Whichever plug-in saves last
// wins. Since we don't have any real project importers yet, it is just easier
// to by-pass saving custom report information. This will have to be fixed later
void CPGSProjectImporterAppPluginBase::SaveReportOptions()
{
   // do nothing
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
   std::vector<CEAFPluginState> vPluginStates = EAFManageApplicationPlugins(str,_T("Select the Project Importers that you want to be available."),catid,EAFGetMainFrame());
   if ( vPluginStates.size() == 0 )
   {
      return;
   }

   // Find our document template
   CEAFApp* pApp = EAFGetApp();

   POSITION template_position;
   CPGSImportPluginDocTemplateBase* pMyTemplate = NULL;
   POSITION pos = pApp->m_pDocManager->GetFirstDocTemplatePosition();
   while ( pos != NULL )
   {
      template_position = pos;
      CEAFDocTemplate* pTemplate = (CEAFDocTemplate*)(pApp->m_pDocManager->GetNextDocTemplate(pos));
      if ( pTemplate->IsKindOf(RUNTIME_CLASS(CPGSImportPluginDocTemplateBase)) )
      {
         pMyTemplate = (CPGSImportPluginDocTemplateBase*)pTemplate;
      }
   }

   // our doc template was not found...
   if ( pMyTemplate == NULL )
   {
      // when the doc template is created, it loads all the enabled plug-in objects
      // write the plugin states into the registry and then create the doc template
      BOOST_FOREACH(CEAFPluginState& state,vPluginStates)
      {
         pApp->WriteProfileString(_T("Plugins"),state.GetCLSIDString(),state.IsEnabled() ? _T("Enabled") : _T("Disabled") );
      }

      std::vector<CEAFDocTemplate*> vDocTemplates = CreateDocTemplates();
      BOOST_FOREACH(CEAFDocTemplate* pTemplate,vDocTemplates)
      {
         pApp->m_pDocManager->AddDocTemplate(pTemplate);
      }
      return;
   }

   // Set the state of the importer plugins, create and destroy them as needed
   CPGSProjectImporterMgrBase* pImporterMgr = pMyTemplate->GetProjectImporterManager();
   BOOST_FOREACH(CEAFPluginState& state,vPluginStates)
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

CPGSBaseCommandLineInfo* CPGSProjectImporterAppPluginBase::CreateCommandLineInfo() const
{
   return NULL;//return new CPGSuperProjectImporterCommandLineInfo; (if we want to handle command line options for an importer, we need a new class);
}

BOOL CPGSProjectImporterAppPluginBase::Init(CEAFApp* pParent)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   DefaultInit();

   // See MSKB Article ID: Q118435, "Sharing Menus Between MDI Child Windows"
   UINT nResource = GetMenuResourceID();
   m_hMenuShared = ::LoadMenu( AfxGetApp()->m_hInstance, MAKEINTRESOURCE(nResource) );

   return (m_hMenuShared != NULL);
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

   CPGSImportPluginDocTemplateBase* pDocTemplate = CreateDocTemplate();

   // If there aren't any importers, we don't want the "PGSuper/PGSplice Project Importer" option to
   // show up in the File | New dialog
   // Returning a NULL doc template will do the trick
   CPGSProjectImporterMgrBase* pImporterMgr = pDocTemplate->GetProjectImporterManager();
   if ( pImporterMgr->GetImporterCount() == 0 )
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

HMENU CPGSProjectImporterAppPluginBase::GetSharedMenuHandle()
{
   return m_hMenuShared;
}

CString CPGSProjectImporterAppPluginBase::GetName()
{
   CString str;
   str.Format(_T("%s Project Importer"),GetAppName());
   return str;
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
