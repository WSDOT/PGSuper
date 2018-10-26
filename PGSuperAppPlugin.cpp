///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2012  Washington State Department of Transportation
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

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin.h"
#include "PGSuperCatCom.h"
#include "resource.h"

#include "PGSuperDoc.h"
#include "PGSuperDocTemplate.h"
#include "BridgeModelViewChildFrame.h"
#include "BridgePlanView.h"
#include <PsgLib\BeamFamilyManager.h>

#include "PluginManagerDlg.h"

#include <EAF\EAFMainFrame.h>

#include <IFace\Test1250.h>


BEGIN_MESSAGE_MAP(CMyCmdTarget,CCmdTarget)
   ON_COMMAND(ID_MANAGE_PLUGINS,OnConfigurePlugins)
   ON_COMMAND(ID_UPDATE_TEMPLATE,OnUpdateTemplates) // need to map this into an accelerator table
	ON_COMMAND(ID_CONFIGURE_PGSUPER, OnProgramSettings)
END_MESSAGE_MAP()

void CMyCmdTarget::OnConfigurePlugins()
{
   m_pMyAppPlugin->ConfigurePlugins();
}

void CMyCmdTarget::OnUpdateTemplates()
{
   m_pMyAppPlugin->UpdateTemplates();
}

void CMyCmdTarget::OnProgramSettings() 
{
   m_pMyAppPlugin->OnProgramSettings();
}

HRESULT CPGSuperAppPlugin::FinalConstruct()
{
   return OnFinalConstruct(); // CPGSuperBaseAppPlugin
}

void CPGSuperAppPlugin::FinalRelease()
{
   OnFinalRelease(); // CPGSuperBaseAppPlugin
}

void CPGSuperAppPlugin::ConfigurePlugins()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CPluginManagerDlg dlg(_T("Manage PGSuper Plugins and Extensions"));
   dlg.DoModal(); // this DoModal is correct... the dialog takes care of its own data
}

BOOL CPGSuperAppPlugin::Init(CEAFApp* pParent)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   DefaultInit();

   // See MSKB Article ID: Q118435, "Sharing Menus Between MDI Child Windows"
   CWinApp* pMyApp = AfxGetApp();
   m_hMenuShared = ::LoadMenu( pMyApp->m_hInstance, MAKEINTRESOURCE(IDR_PGSUPERTYPE) );

   if ( m_hMenuShared == NULL )
      return FALSE;

   // Application start up can be improved if this call
   // is executed in its own thread... Need to add some
   // code that indicates if the call fails.. then throw
   // a shut down exception
   if ( FAILED(CBeamFamilyManager::Init()) )
      return FALSE;

   if ( !EAFGetApp()->GetCommandLineInfo().m_bCommandLineMode )
      UpdateCache(); // we don't want to do this if we are running in batch/command line mode

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

   // add our commands to the menu
   CEAFMenu* pMainMenu = pFrame->GetMainMenu();

   UINT filePos = pMainMenu->FindMenuItem(_T("&File"));
   CEAFMenu* pFileMenu = pMainMenu->GetSubMenu(filePos);
   if ( pFileMenu == NULL )
   {
      ATLASSERT(false);
      return;
   }

   UINT managePos = pFileMenu->FindMenuItem(_T("Manage"));
   CEAFMenu* pManageMenu = pFileMenu->GetSubMenu(managePos);
   if ( pManageMenu == NULL )
   {
      ATLASSERT(false);
      return;
   }

   if ( bIntegrate )
   {
      // put "Configure PGSuper" the file menu just below "Manage"
      pFileMenu->InsertMenu(managePos+1,ID_CONFIGURE_PGSUPER,_T("Configure PGSuper..."), this);

      // Append to the end of the Manage menu
      pManageMenu->AppendMenu(ID_MANAGE_PLUGINS,_T("PGSuper Plugins and Extensions..."),this);

      // Alt+Ctrl+U
      pFrame->GetAcceleratorTable()->AddAccelKey(FALT | FCONTROL | FVIRTKEY, VK_U, ID_UPDATE_TEMPLATE,this);
   }
   else
   {
      pFileMenu->RemoveMenu(ID_CONFIGURE_PGSUPER, MF_BYCOMMAND, this);
      pManageMenu->RemoveMenu(ID_MANAGE_PLUGINS,  MF_BYCOMMAND, this);

      pFrame->GetAcceleratorTable()->RemoveAccelKey(ID_UPDATE_TEMPLATE,this);
   }
}

CEAFDocTemplate* CPGSuperAppPlugin::CreateDocTemplate()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CPGSuperDocTemplate* pTemplate = new CPGSuperDocTemplate(
		IDR_BRIDGEMODELEDITOR,
      NULL,
		RUNTIME_CLASS(CPGSuperDoc),
		RUNTIME_CLASS(CBridgeModelViewChildFrame),
		RUNTIME_CLASS(CBridgePlanView),
      m_hMenuShared,1);

   return pTemplate;
}

HMENU CPGSuperAppPlugin::GetSharedMenuHandle()
{
   return m_hMenuShared;
}

UINT CPGSuperAppPlugin::GetDocumentResourceID()
{
   return IDR_PGSUPERTYPE;
}

CString CPGSuperAppPlugin::GetName()
{
   return CString(_T("PGSuper"));
}

CString CPGSuperAppPlugin::GetUsageMessage()
{
   CPGSuperCommandLineInfo pgsCmdInfo;
   return pgsCmdInfo.GetUsageMessage();
}

BOOL CPGSuperAppPlugin::ProcessCommandLineOptions(CEAFCommandLineInfo& cmdInfo)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   // cmdInfo is the command line information from the application. The application
   // doesn't know about this plug-in at the time the command line parameters are parsed
   //
   // Re-parse the parameters with our own command line information object
   CPGSuperCommandLineInfo pgsCmdInfo;
   EAFGetApp()->ParseCommandLine(pgsCmdInfo);
   cmdInfo = pgsCmdInfo;

   if (pgsCmdInfo.m_bError)
   {
      return FALSE;
   }
   else if (pgsCmdInfo.m_bDo1250Test)
   {
      Process1250Testing(pgsCmdInfo);
      return TRUE; // command line parameters handled
   }
   else if (pgsCmdInfo.m_bSetUpdateLibrary)
   {
      ProcessLibrarySetUp(pgsCmdInfo);
      return TRUE;
   }

   BOOL bHandled = FALSE;
   CEAFMainFrame* pFrame = EAFGetMainFrame();
   CEAFDocument* pDoc = pFrame->GetDocument();
   if ( pDoc )
   {
      bHandled = pDoc->ProcessCommandLineOptions(cmdInfo);
   }

   // If we get this far and there is one parameter and it isn't a file name and it isn't handled,
   // then something is wrong
   if ( 1 == pgsCmdInfo.m_Count && pgsCmdInfo.m_nShellCommand != CCommandLineInfo::FileOpen )
   {
      cmdInfo.m_bError = TRUE;
      bHandled = TRUE;
   }

   return bHandled;
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

   int result = AfxMessageBox(_T("All of the template library entries will be updated to match the Master Library.\n\nDo you want to proceed?"),MB_YESNO);
   if ( result == IDNO )
      return;

   m_bUpdatingTemplate = true;
   CEAFApp* pApp = EAFGetApp();

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
   pICatInfo->EnumClassesOfCategories(nID,ID,0,NULL,&pIEnumCLSID);

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
         extension_states.push_back(std::make_pair(OLE2T(pszCLSID),strState));
         ::CoTaskMemFree((void*)pszCLSID);

         // Disable the extension
         pApp->WriteProfileString(strSection,OLE2T(pszCLSID),_T("Disabled"));
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

void CPGSuperAppPlugin::OnProgramSettings()
{
   UpdateProgramSettings(FALSE);
}

void CPGSuperAppPlugin::ProcessLibrarySetUp(const CPGSuperCommandLineInfo& rCmdInfo)
{
   ASSERT(rCmdInfo.m_bSetUpdateLibrary);

   // Set library to parsed names and attempt an update
   SharedResourceType        original_type = m_SharedResourceType;
   CacheUpdateFrequency      original_freq = m_CacheUpdateFrequency;
   CString                 original_server = m_CurrentCatalogServer;
   CString              original_publisher = m_Publisher;

   // find our server
   const CPGSuperCatalogServer* pserver = GetCatalogServers()->GetServer(rCmdInfo.m_CatalogServerName);
   if (pserver != NULL)
   {
      m_SharedResourceType   = pserver->GetServerType();

      m_CurrentCatalogServer = rCmdInfo.m_CatalogServerName;
      m_Publisher            = rCmdInfo.m_PublisherName ;

      if (!this->DoCacheUpdate())
      {
         // DoCacheUpdate will restore the cache, we need also to restore local data
         m_SharedResourceType   = original_type;
         m_CurrentCatalogServer = original_server;
         m_Publisher            = original_publisher;
      }
   }
   else
   {
      CString msg;
      msg.Format(_T("Error - The catalog server \"%s\" was not found. Could not update catalog"), rCmdInfo.m_CatalogServerName);
      AfxMessageBox(msg);
   }
}

void CPGSuperAppPlugin::Process1250Testing(const CPGSuperCommandLineInfo& rCmdInfo)
{
   USES_CONVERSION;

   ASSERT(rCmdInfo.m_bDo1250Test);

   // The document is opened when CEAFApp::InitInstance calls ProcessShellCommand
   // Get the document
   CEAFMainFrame* pFrame = EAFGetMainFrame();
   CPGSuperDoc* pPgsDoc = (CPGSuperDoc*)pFrame->GetDocument();

   CComPtr<IBroker> pBroker;
   pPgsDoc->GetBroker(&pBroker);
   GET_IFACE2( pBroker, ITest1250, ptst );

   CString resultsfile, poifile, errfile;
   if (create_test_file_names(rCmdInfo.m_strFileName,&resultsfile,&poifile,&errfile))
   {
      try
      {
         if (!ptst->RunTest(rCmdInfo.m_SubdomainId, std::_tstring(resultsfile), std::_tstring(poifile)))
         {
            CString msg = CString(_T("Error - Running test on file"))+rCmdInfo.m_strFileName;
            ::AfxMessageBox(msg);
         }

// Not sure why, but someone put this code in to save regression files.
// Sort of defeats the purpose of testing old files...
//
//         if ( pPgsDoc->IsModified() )
//            pPgsDoc->DoFileSave();
      }
      catch(const sysXBase& e)
      {
         std::_tstring msg;
         e.GetErrorMessage(&msg);
         std::_tofstream os;
         os.open(errfile);
         os <<_T("Error running test for input file: ")<<rCmdInfo.m_strFileName<<std::endl<< msg;
      }
      catch(CException* pex)
      {
         TCHAR   szCause[255];
         CString strFormatted;
         pex->GetErrorMessage(szCause, 255);
         std::_tofstream os;
         os.open(errfile);
         os <<_T("Error running test for input file: ")<<rCmdInfo.m_strFileName<<std::endl<< szCause;
         delete pex;
      }
      catch(CException& ex)
      {
         TCHAR   szCause[255];
         CString strFormatted;
         ex.GetErrorMessage(szCause, 255);
         std::_tofstream os;
         os.open(errfile);
         os <<_T("Error running test for input file: ")<<rCmdInfo.m_strFileName<<std::endl<< szCause;
      }
      catch(const std::exception* pex)
      {
         std::_tstring strMsg(CA2T(pex->what()));
         std::_tofstream os;
         os.open(errfile);
         os <<_T("Error running test for input file: ")<<rCmdInfo.m_strFileName<<std::endl<<strMsg<< std::endl;
         delete pex;
      }
      catch(const std::exception& ex)
      {
         std::_tstring strMsg(CA2T(ex.what()));
         std::_tofstream os;
         os.open(errfile);
         os <<_T("Error running test for input file: ")<<rCmdInfo.m_strFileName<<std::endl<<strMsg<< std::endl;
      }
      catch(...)
      {
         std::_tofstream os;
         os.open(errfile);
         os <<_T("Unknown Error running test for input file: ")<<rCmdInfo.m_strFileName;
      }
   }
}
