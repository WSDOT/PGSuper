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
#include "PGSuperAppPlugin.h"
#include "PGSuperApp.h"
#include <PGSuperUnits.h>
#include "PGSuperDocTemplate.h"

#include <EAF\EAFDocManager.h>
#include <EAF\EAFBrokerDocument.h>

#include <IFace\Test1250.h>

#include <MFCTools\AutoRegistry.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CPGSAppPluginBase::CPGSAppPluginBase()
{
}

CPGSAppPluginBase::~CPGSAppPluginBase()
{
}

HRESULT CPGSAppPluginBase::OnFinalConstruct()
{
   if (FAILED(CCatalogServerAppMixin::OnFinalConstruct()))
   {
      ATLASSERT(0);
      return E_FAIL;
   }

   if ( !CreateAppUnitSystem(&m_AppUnitSystem) )
   {
      return E_FAIL;
   }

   return S_OK;
}

void CPGSAppPluginBase::OnFinalRelease()
{
   if ( m_strAppProfileName != _T("") )
   {
      // this method is called from OnNewDocument... don't want to mess with the profile name
      // if it hasn't been changed
      AFX_MANAGE_STATE(AfxGetStaticModuleState());

      CWinApp* pMyApp = AfxGetApp();

      free((void*)pMyApp->m_pszProfileName);
      pMyApp->m_pszProfileName = _tcsdup(m_strAppProfileName);
   }

   m_AppUnitSystem.Release();
}

void CPGSAppPluginBase::DefaultInit(IEAFAppPlugin* pAppPlugin)
{
   CCatalogServerAppMixin::DefaultInit(pAppPlugin);

   LoadRegistryValues();

   m_DocumentationImpl.Init(pAppPlugin);
}

void CPGSAppPluginBase::DefaultTerminate()
{
   CCatalogServerAppMixin::DefaultTerminate();

   SaveRegistryValues();
}

void CPGSAppPluginBase::GetAppUnitSystem(IAppUnitSystem** ppAppUnitSystem)
{
   m_AppUnitSystem.CopyTo(ppAppUnitSystem);
}

void CPGSAppPluginBase::LoadRegistryValues()
{
   CCatalogServerAppMixin::LoadRegistryValues();

   LoadCustomReportInformation();
}

void CPGSAppPluginBase::SaveRegistryValues()
{
   CCatalogServerAppMixin::SaveRegistryValues();

   SaveCustomReportInformation();
}

void CPGSAppPluginBase::LoadCustomReportInformation()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   CAutoRegistry autoReg(GetAppName());
   CEAFCustomReportMixin::LoadCustomReportInformation();
}

void CPGSAppPluginBase::SaveCustomReportInformation()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   CAutoRegistry autoReg(GetAppName());
   CEAFCustomReportMixin::SaveCustomReportInformation();
}

CString CPGSAppPluginBase::GetDocumentationURL()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   return m_DocumentationImpl.GetDocumentationURL();
}

CString CPGSAppPluginBase::GetDocumentationMapFile()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   return m_DocumentationImpl.GetDocumentationMapFile();
}

void CPGSAppPluginBase::LoadDocumentationMap()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   return m_DocumentationImpl.LoadDocumentationMap();
}

eafTypes::HelpResult CPGSAppPluginBase::GetDocumentLocation(LPCTSTR lpszDocSetName,UINT nID,CString& strURL)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   return m_DocumentationImpl.GetDocumentLocation(lpszDocSetName,nID,strURL);
}

void CPGSAppPluginBase::UpdateDocTemplates()
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

         CPGSuperDocTemplateBase* pTemplate = dynamic_cast<CPGSuperDocTemplateBase*>(pDocTemplate);
         pTemplate->LoadTemplateInformation();

         pDocMgr->AddDocTemplate(pDocTemplate);

         break;
      }
   }
}

BOOL CPGSAppPluginBase::DoProcessCommandLineOptions(CEAFCommandLineInfo& cmdInfo)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   // cmdInfo is the command line information from the application. The application
   // doesn't know about this plug-in at the time the command line parameters are parsed
   //
   // Re-parse the parameters with our own command line information object
   std::unique_ptr<CPGSBaseCommandLineInfo> pgsCmdInfo((CPGSBaseCommandLineInfo*)CreateCommandLineInfo());
   if ( pgsCmdInfo.get() != nullptr )
   {
      EAFGetApp()->ParseCommandLine(*pgsCmdInfo);
      cmdInfo = *pgsCmdInfo;

      if (pgsCmdInfo->m_bError)
      {
         return FALSE;
      }
      else if (pgsCmdInfo->m_bDo1250Test)
      {
         Process1250Testing(*pgsCmdInfo);
         return TRUE; // command line parameters handled
      }
      else if (pgsCmdInfo->m_bSetUpdateLibrary)
      {
         ProcessLibrarySetUp(*pgsCmdInfo);
         return TRUE;
      }
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
   if ( ((pgsCmdInfo.get() != nullptr && 1 == pgsCmdInfo->m_Count && pgsCmdInfo->m_nShellCommand != CCommandLineInfo::FileOpen) || (1 <  pgsCmdInfo->m_Count)) && !bHandled )
   {
      cmdInfo.m_bError = TRUE;
      bHandled = TRUE;
   }

   return bHandled;
}

void CPGSAppPluginBase::Process1250Testing(const CPGSBaseCommandLineInfo& rCmdInfo)
{
   USES_CONVERSION;
   ASSERT(rCmdInfo.m_bDo1250Test);

   // The document is opened when CEAFApp::InitInstance calls ProcessShellCommand
   // Get the document
   CEAFMainFrame* pFrame = EAFGetMainFrame();
   CEAFBrokerDocument* pDoc = (CEAFBrokerDocument*)pFrame->GetDocument();

   CDocTemplate* pTemplate = pDoc->GetDocTemplate();
   CString strExt;
   pTemplate->GetDocString(strExt,CDocTemplate::filterExt);
   strExt = strExt.Left(strExt.Find(_T(";")));

   CComPtr<IBroker> pBroker;
   pDoc->GetBroker(&pBroker);
   GET_IFACE2( pBroker, ITest1250, ptst );

   CString resultsfile, poifile, errfile;
   if (create_test_file_names(strExt,rCmdInfo.m_strFileName,&resultsfile,&poifile,&errfile))
   {
      try
      {
         if (!ptst->RunTest(rCmdInfo.m_SubdomainId, std::_tstring(resultsfile), std::_tstring(poifile)))
         {
            CString msg = CString(_T("Error - Running test on file"))+rCmdInfo.m_strFileName;
            ::AfxMessageBox(msg);
         }
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

