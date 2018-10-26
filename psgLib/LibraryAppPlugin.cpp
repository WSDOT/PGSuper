#include "stdafx.h"
#include "PGSuperLibrary_i.h"
#include "LibraryAppPlugin.h"

#include "resource.h"

#include <PsgLib\LibraryDocTemplate.h>
#include <PsgLib\LibraryEditorDoc.h>
#include <PsgLib\LibChildFrm.h>
#include <PsgLib\LibraryEditorView.h>

#include <EAF\EAFUtilities.h>
#include <EAF\EAFMainFrame.h>

#include "PGSuperLibraryMgrCATID.h"

BEGIN_MESSAGE_MAP(CMyCmdTarget,CCmdTarget)
   ON_COMMAND(ID_MANAGE_PLUGINS,OnManagePlugins)
END_MESSAGE_MAP()

void CMyCmdTarget::OnManagePlugins()
{
   m_pMyAppPlugin->ManagePlugins();
}

///////////////////////////////////////////////////////////////////
void CLibraryAppPlugin::ManagePlugins()
{
   std::vector<CEAFPluginState> pluginStates = EAFManagePlugins(_T("Manage Library Editor Plugins"),CATID_PGSuperLibraryManagerPlugin,EAFGetMainFrame());

   if ( pluginStates.size() == 0 )
      return;

   // Find our document template
   CEAFApp* pApp = EAFGetApp();

   // write the plugin states into the registry 
   std::vector<CEAFPluginState>::iterator iter;
   for ( iter = pluginStates.begin(); iter != pluginStates.end(); iter++ )
   {
      CEAFPluginState& state = *iter;
#if !defined _WBFL_VERSION
#error _WBFL_VERSION must be defined... add #include <WBFLAll.h>
#endif

#if _WBFL_VERSION < 330
      // Prior to WBFL version 3.3, there is a bug in the state.IsEnabled function
      // This code works around the bug
      bool bIsEnabled = false;
      if ( (state.InitiallyEnabled() && !state.StateChanged()) || (!state.InitiallyEnabled() && state.StateChanged()) )
         bIsEnabled = true;

      pApp->WriteProfileString(_T("Plugins"),state.GetCLSIDString(),bIsEnabled ? _T("Enabled") : _T("Disabled") );
#else
      pApp->WriteProfileString(_T("Plugins"),state.GetCLSIDString(),state.IsEnabled() ? _T("Enabled") : _T("Disabled") );
#endif
   }
}

BOOL CLibraryAppPlugin::Init(CEAFApp* pParent)
{
   return TRUE;
}

void CLibraryAppPlugin::Terminate()
{
}

void CLibraryAppPlugin::IntegrateWithUI(BOOL bIntegrate)
{
   CEAFMainFrame* pFrame = EAFGetMainFrame();
   CEAFMenu* pMainMenu = pFrame->GetMainMenu();

   UINT filePos = pMainMenu->FindMenuItem(_T("&File"));
   CEAFMenu* pFileMenu = pMainMenu->GetSubMenu(filePos);

   UINT managePos = pFileMenu->FindMenuItem(_T("Manage"));
   CEAFMenu* pManageMenu = pFileMenu->GetSubMenu(managePos);

   if ( bIntegrate )
   {
      pManageMenu->AppendMenu(ID_MANAGE_PLUGINS,_T("Manage Library Editor Plugins..."),this);
   }
   else
   {
      pManageMenu->RemoveMenu(ID_MANAGE_PLUGINS,  MF_BYCOMMAND, this);
   }
}

std::vector<CEAFDocTemplate*> CLibraryAppPlugin::CreateDocTemplates()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   std::vector<CEAFDocTemplate*> vDocTemplates;

   // doc template for Library editor
	CLibraryDocTemplate* pLibMgrDocTemplate = new CLibraryDocTemplate(
		IDR_LIBRARYTYPE,this,
		RUNTIME_CLASS(CLibraryEditorDoc),
		RUNTIME_CLASS(CLibChildFrame),
		RUNTIME_CLASS(CLibraryEditorView));

   pLibMgrDocTemplate->SetPlugin(this);

   HICON hIcon = AfxGetApp()->LoadIcon(IDI_LIBRARY_MANAGER);
   pLibMgrDocTemplate->CreateDefaultItem(hIcon);

   vDocTemplates.push_back(pLibMgrDocTemplate);
   return vDocTemplates;
}

HMENU CLibraryAppPlugin::GetSharedMenuHandle()
{
   return NULL;
}

CString CLibraryAppPlugin::GetName()
{
   return CString(_T("PGSuper Library Editor"));
}

//////////////////////////
// IEAFCommandCallback
BOOL CLibraryAppPlugin::OnCommandMessage(UINT nID,int nCode,void* pExtra,AFX_CMDHANDLERINFO* pHandlerInfo)
{
   return m_MyCmdTarget.OnCmdMsg(nID,nCode,pExtra,pHandlerInfo);
}

BOOL CLibraryAppPlugin::GetStatusBarMessageString(UINT nID, CString& rMessage) const
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
		TRACE1("Warning: no message line prompt for ID %d.\n", nID);
	}

   return TRUE;
}

BOOL CLibraryAppPlugin::GetToolTipMessageString(UINT nID, CString& rMessage) const
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
		TRACE1("Warning: no tool tip for ID %d.\n", nID);
	}

   return TRUE;
}

