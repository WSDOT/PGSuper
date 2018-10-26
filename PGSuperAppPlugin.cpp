///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2010  Washington State Department of Transportation
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
#include <PsgLib\BeamFamilyManager.h>

#include "PluginManagerDlg.h"

#include <EAF\EAFMainFrame.h>

#include "PGSuper.h"


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
   CPluginManagerDlg dlg("Manage Plugins and Extensions");
   dlg.DoModal(); // this DoModal is correct... the dialog takes care of its own data
}

BOOL CPGSuperAppPlugin::Init(CEAFApp* pParent)
{
   // See MSKB Article ID: Q118435, "Sharing Menus Between MDI Child Windows"
   CWinApp* pApp = AfxGetApp();
   m_hMenuShared = ::LoadMenu( pApp->m_hInstance, MAKEINTRESOURCE(IDR_PGSUPERTYPE) );

   if ( m_hMenuShared == NULL )
      return FALSE;

   // Application start up can be improved if this call
   // is executed in its own thread... Need to add some
   // code that indicates if the call fails.. then throw
   // a shut down exception
   if ( FAILED(CBeamFamilyManager::Init()) )
      return FALSE;

   sysComCatMgr::CreateCategory(L"PGSuper Components",CATID_PGSuperComponents);

   return TRUE;
}

void CPGSuperAppPlugin::Terminate()
{
   // release the shared menu
   ::DestroyMenu( m_hMenuShared );
}

void CPGSuperAppPlugin::IntegrateWithUI(BOOL bIntegrate)
{
   CEAFMainFrame* pFrame = (CEAFMainFrame*)AfxGetMainWnd();
   CEAFMenu* pMainMenu = pFrame->GetMainMenu();

   UINT filePos = pMainMenu->FindMenuItem("&File");
   CEAFMenu* pFileMenu = pMainMenu->GetSubMenu(filePos);

   if ( bIntegrate )
   {
      pFileMenu->InsertSeparator(4,MF_BYPOSITION);
      pFileMenu->InsertMenu(5,ID_CONFIGURE_PGSUPER,"Configure PGSuper...",                    this);
      pFileMenu->InsertMenu(6,ID_MANAGE_PLUGINS,   "Manage PGSuper Plugins and Extensions...",this);

      // Alt+Ctrl+U
      pFrame->GetAcceleratorTable()->AddAccelKey(FALT | FCONTROL | FVIRTKEY, VK_U, ID_UPDATE_TEMPLATE,this);
   }
   else
   {
      pFileMenu->RemoveMenu(4,                    MF_BYPOSITION,this); // separator
      pFileMenu->RemoveMenu(ID_CONFIGURE_PGSUPER, MF_BYCOMMAND, this);
      pFileMenu->RemoveMenu(ID_MANAGE_PLUGINS,    MF_BYCOMMAND, this);

      pFrame->GetAcceleratorTable()->RemoveAccelKey(ID_UPDATE_TEMPLATE,this);
   }
}

CEAFDocTemplate* CPGSuperAppPlugin::CreateDocTemplate()
{
   CPGSuperDocTemplate* pTemplate = new CPGSuperDocTemplate(
		IDR_BRIDGEMODELEDITOR,
		RUNTIME_CLASS(CPGSuperDoc),
		RUNTIME_CLASS(CBridgeModelViewChildFrame),
		RUNTIME_CLASS(CBridgePlanView),
      m_hMenuShared,1);

   pTemplate->SetPlugin(this);

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
   return CString("PGSuper");
}

//////////////////////////
// IEAFCommandCallback
BOOL CPGSuperAppPlugin::OnCommandMessage(UINT nID,int nCode,void* pExtra,AFX_CMDHANDLERINFO* pHandlerInfo)
{
   return m_MyCmdTarget.OnCmdMsg(nID,nCode,pExtra,pHandlerInfo);
}

void CPGSuperAppPlugin::GetStatusBarMessageString(UINT nID, CString& rMessage) const
{
   //AFX_MANAGE_STATE(AfxGetStaticModuleState());

   // load appropriate string
	if ( rMessage.LoadString(nID) )
	{
		// first newline terminates actual string
      rMessage.Replace('\n','\0');
	}
	else
	{
		// not found
		TRACE1("Warning: no message line prompt for ID %d.\n", nID);
	}
}

void CPGSuperAppPlugin::GetToolTipMessageString(UINT nID, CString& rMessage) const
{
   //AFX_MANAGE_STATE(AfxGetStaticModuleState());
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
}

void CPGSuperAppPlugin::UpdateTemplates()
{
   USES_CONVERSION;

   int result = AfxMessageBox("All of the template library entries will be updated to match the Master Library.\n\nDo you want to proceed?",MB_YESNO);
   if ( result == IDNO )
      return;

   m_bUpdatingTemplate = true;
   CWinApp* pApp = AfxGetApp();

   // Get the application into a "just started" state
   if ( pApp->SaveAllModified() )
   {
      pApp->CloseAllDocuments(FALSE);
   }
   else
   {
      AfxMessageBox("Unable to save and close the open document. Template Update cancelled");
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
      AfxMessageBox("Failed to create the component category manager");
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

   CString strSection("Extensions");

   while ( SUCCEEDED(pIEnumCLSID->Next(nPlugins,clsid,&nFetched)) && 0 < nFetched)
   {
      for ( ULONG i = 0; i < nFetched; i++ )
      {
         LPOLESTR pszCLSID;
         ::StringFromCLSID(clsid[i],&pszCLSID);
         
         CString strState = pApp->GetProfileString(strSection,OLE2A(pszCLSID),_T("Enabled"));
         extension_states.push_back(std::make_pair(OLE2A(pszCLSID),strState));
         ::CoTaskMemFree((void*)pszCLSID);

         // Disable the extension
         pApp->WriteProfileString(strSection,OLE2A(pszCLSID),_T("Disabled"));
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
   AfxMessageBox("Update complete",MB_OK);
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
   // this is a temporary implementation... the whole program settings
   // things needs to be moved into this object
   CPGSuperApp* pApp = (CPGSuperApp*)AfxGetApp();
   pApp->OnProgramSettings(FALSE);
}
