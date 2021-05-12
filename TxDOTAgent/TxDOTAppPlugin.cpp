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
#include "TxDOTAppPlugin.h"
#include "TxDOTOptionalDesignDocTemplate.h"
#include "TxDOTOptionalDesignDoc.h"
#include "TxDOTOptionalDesignView.h"
#include "TxDOTOptionalDesignChildFrame.h"
#include "resource.h"
#include "TxDOTCommandLineInfo.h"

#include <EAF\EAFDocManager.h>
#include <EAF\EAFBrokerDocument.h>
#include <MFCTools\AutoRegistry.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


BEGIN_MESSAGE_MAP(CMyCmdTarget,CCmdTarget)
   ON_COMMAND(ID_UPDATE_TOGA_TEMPLATE,OnUpdateTemplates)
END_MESSAGE_MAP()

void CMyCmdTarget::OnUpdateTemplates()
{
   m_pMyAppPlugin->UpdateTemplates();
}

BOOL CTxDOTAppPlugin::Init(CEAFApp* pParent)
{
   DefaultInit(this);

   {
      AFX_MANAGE_STATE(AfxGetStaticModuleState());

      LoadRegistryValues();
   }

   // use manage state because we need exe's state below
   {
      AFX_MANAGE_STATE(AfxGetAppModuleState());

      // TRICKY: Must lock temporary ole control maps in app module or the report browser window
      //         will vanish after about 10 seconds. See http://support.microsoft.com/kb/161874
      //         for a sketchy discription
      AfxLockTempMaps();
   }

   {
      AFX_MANAGE_STATE(AfxGetStaticModuleState());

      if ( !EAFGetApp()->GetCommandLineInfo().m_bCommandLineMode )
         UpdateCache(); // we don't want to do this if we are running in batch/command line mode


      m_DocumentationImpl.Init(this);
   }

   return TRUE;
}

void CTxDOTAppPlugin::Terminate()
{
   SaveRegistryValues();

   DefaultTerminate();

   {
      AFX_MANAGE_STATE(AfxGetAppModuleState());
      // see tricky in Init
      AfxUnlockTempMaps();
   }
}

HRESULT CTxDOTAppPlugin::FinalConstruct()
{
   return OnFinalConstruct(); // CCatalogServerAppMixin
}

void CTxDOTAppPlugin::FinalRelease()
{
   OnFinalRelease(); // CCatalogServerAppMixin
}


void CTxDOTAppPlugin::IntegrateWithUI(BOOL bIntegrate)
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
//      pManageMenu->AppendMenu(ID_MANAGE_PLUGINS,_T("TOGA Plugins and Extensions..."),this);

      // Alt+Ctrl+T
      pFrame->GetAcceleratorTable()->AddAccelKey(FALT | FCONTROL | FVIRTKEY, VK_T, ID_UPDATE_TOGA_TEMPLATE,this);
   }
   else
   {
//      pManageMenu->RemoveMenu(ID_MANAGE_PLUGINS,  MF_BYCOMMAND, this);

      pFrame->GetAcceleratorTable()->RemoveAccelKey(ID_UPDATE_TOGA_TEMPLATE,this);
   }
}

std::vector<CEAFDocTemplate*> CTxDOTAppPlugin::CreateDocTemplates()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   std::vector<CEAFDocTemplate*> vDocTemplates;

	CTxDOTOptionalDesignDocTemplate* pDocTemplate = new CTxDOTOptionalDesignDocTemplate(
		IDR_TXDOTOPTIONALDESIGN,
      this,
		RUNTIME_CLASS(CTxDOTOptionalDesignDoc),
		RUNTIME_CLASS(CTxDOTOptionalDesignChildFrame), // substitute your own child frame if needed
		RUNTIME_CLASS(CTxDOTOptionalDesignView));

   pDocTemplate->SetPlugin(this);

   vDocTemplates.push_back(pDocTemplate);
   return vDocTemplates;
}

HMENU CTxDOTAppPlugin::GetSharedMenuHandle()
{
   return nullptr;
}

CString CTxDOTAppPlugin::GetName()
{
   return CString("TOGA");
}

CString CTxDOTAppPlugin::GetDocumentationSetName()
{
   return _T("TOGA");
}

CString CTxDOTAppPlugin::GetDocumentationURL()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   return m_DocumentationImpl.GetDocumentationURL();
}

CString CTxDOTAppPlugin::GetDocumentationMapFile()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   return m_DocumentationImpl.GetDocumentationMapFile();
}

void CTxDOTAppPlugin::LoadDocumentationMap()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   return m_DocumentationImpl.LoadDocumentationMap();
}

eafTypes::HelpResult CTxDOTAppPlugin::GetDocumentLocation(LPCTSTR lpszDocSetName,UINT nID,CString& strURL)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   return m_DocumentationImpl.GetDocumentLocation(lpszDocSetName,nID,strURL);
}

CString CTxDOTAppPlugin::GetUsageMessage()
{
   CTxDOTCommandLineInfo txCmdInfo;
   return txCmdInfo.GetUsageMessage();
}

BOOL CTxDOTAppPlugin::ProcessCommandLineOptions(CEAFCommandLineInfo& cmdInfo)
{
   // cmdInfo is the command line information from the application. The application
   // doesn't know about this plug-in at the time the command line parameters are parsed
   //
   // Re-parse the parameters with our own command line information object
   CTxDOTCommandLineInfo txCmdInfo;
   EAFGetApp()->ParseCommandLine(txCmdInfo);
   cmdInfo = txCmdInfo;

   if (txCmdInfo.m_bSetUpdateLibrary)
   {
      ProcessLibrarySetUp(txCmdInfo);
      return TRUE;
   }

   BOOL bHandled = FALSE;
   CEAFMainFrame* pFrame = EAFGetMainFrame();
   CEAFDocument* pDoc = pFrame->GetDocument();
   if ( pDoc )
   {
      bHandled = pDoc->ProcessCommandLineOptions(cmdInfo);
   }

   // If we get this far and there is one parameter and it isn't a file name and it isn't handled -OR-
   // if there is more than one parameter and it isn't handled there is something wrong
   if ( (1 == txCmdInfo.m_Count && txCmdInfo.m_nShellCommand != CCommandLineInfo::FileOpen) || 
        (1 <  txCmdInfo.m_Count && !bHandled ) )
   {
      cmdInfo.m_bError = TRUE;
      bHandled = TRUE;
   }

   return bHandled;
}


//////////////////////////
// IEAFCommandCallback
BOOL CTxDOTAppPlugin::OnCommandMessage(UINT nID,int nCode,void* pExtra,AFX_CMDHANDLERINFO* pHandlerInfo)
{
   return m_MyCmdTarget.OnCmdMsg(nID,nCode,pExtra,pHandlerInfo);
}

BOOL CTxDOTAppPlugin::GetStatusBarMessageString(UINT nID, CString& rMessage) const
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

BOOL CTxDOTAppPlugin::GetToolTipMessageString(UINT nID, CString& rMessage) const
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

CString CTxDOTAppPlugin::GetTemplateFileExtension()
{ 
   return CString(_T("togt"));
}

const CRuntimeClass* CTxDOTAppPlugin::GetDocTemplateRuntimeClass()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   return RUNTIME_CLASS(CTxDOTOptionalDesignDocTemplate);
}

CPGSBaseCommandLineInfo* CTxDOTAppPlugin::CreateCommandLineInfo() const
{
   return  new CTxDOTCommandLineInfo();
}

void CTxDOTAppPlugin::UpdateTemplates()
{
   ATLASSERT(0);
   AfxMessageBox(_T("Update Templates feature not supported (or applicable) to TOGA"));
}

void CTxDOTAppPlugin::UpdateDocTemplates()
{
   CEAFApp* pApp = EAFGetApp();

   // Need to update the main document template so that the File | New dialog is updated
   // Search for the CPGSuperDocTemplate object
   CEAFDocManager* pDocMgr = (CEAFDocManager*)(pApp->m_pDocManager);
   POSITION pos = pDocMgr->GetFirstDocTemplatePosition();
   while ( pos != nullptr )
   {
      POSITION templatePos = pos;
      CDocTemplate* pDocTemplate = pDocMgr->GetNextDocTemplate(pos);
      if ( pDocTemplate->IsKindOf(GetDocTemplateRuntimeClass()) )
      {
         pDocMgr->RemoveDocTemplate(templatePos);

         CTxDOTOptionalDesignDocTemplate* pTemplate = dynamic_cast<CTxDOTOptionalDesignDocTemplate*>(pDocTemplate);
         pTemplate->LoadTemplateInformation();

         pDocMgr->AddDocTemplate(pDocTemplate);

         break;
      }
   }
}

CString CTxDOTAppPlugin::GetDefaultMasterLibraryFile() const
{
   CString path = GetDefaultWorkgroupTemplateFolder();
   return path + (_T("\\TXDOT.LBR"));
}

CString CTxDOTAppPlugin::GetDefaultWorkgroupTemplateFolder() const
{
   CEAFApp* pApp = EAFGetApp();

   CString strAppPath = pApp->GetAppLocation();
   strAppPath.MakeUpper();

#if defined _DEBUG
#if defined _WIN64
   strAppPath.Replace(_T("BRIDGELINK\\REGFREECOM\\X64\\DEBUG\\"),_T(""));
#else
   strAppPath.Replace(_T("BRIDGELINK\\REGFREECOM\\WIN32\\DEBUG\\"),_T(""));
#endif

   strAppPath += _T("PGSUPER\\TXDOTAGENT");

#else
   // in a real release, the path doesn't contain RegFreeCOM\\Release, but that's
   // ok... the replace will fail and the string wont be altered.
#if defined _WIN64
   strAppPath.Replace(_T("BRIDGELINK\\REGFREECOM\\X64\\RELEASE\\"),_T("PGSUPER\\TXDOTAGENT"));
#else
   strAppPath.Replace(_T("BRIDGELINK\\REGFREECOM\\WIN32\\RELEASE\\"),_T("PGSUPER\\TXDOTAGENT"));
#endif
#endif

   return strAppPath + CString(_T("\\TOGATEMPLATES"));
}
