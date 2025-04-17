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
#include "PGSuperProjectImporterPluginApp.h"
#include "BridgeModelViewChildFrame.h"
#include "BridgePlanView.h"
#include "PGSuperImportPluginDocTemplate.h"
#include <MFCTools\AutoRegistry.h>
#include "PGSuperCommandLineInfo.h"

const CRuntimeClass* CPGSuperProjectImporterPluginApp::GetDocTemplateRuntimeClass()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   return RUNTIME_CLASS(CPGSuperImportPluginDocTemplate);
}

LPCTSTR CPGSuperProjectImporterPluginApp::GetCatalogServerKey() const
{
   return _T("CatalogServer2");
}

LPCTSTR CPGSuperProjectImporterPluginApp::GetPublisherKey() const
{
   return _T("Publisher2");
}

LPCTSTR CPGSuperProjectImporterPluginApp::GetMasterLibraryCacheKey() const
{
   return _T("MasterLibraryCache2");
}

LPCTSTR CPGSuperProjectImporterPluginApp::GetMasterLibraryURLKey() const
{
   return _T("MasterLibraryURL2");
}

LPCTSTR CPGSuperProjectImporterPluginApp::GetWorkgroupTemplatesCacheKey() const
{
   return _T("WorkgroupTemplatesCache2");
}

CString CPGSuperProjectImporterPluginApp::GetCacheFolder() const
{
   CAutoRegistry autoReg(GetAppName());

   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   CWinApp* pMyApp     = AfxGetApp();
   CEAFApp* pParentApp = EAFGetApp();

   LPWSTR path;
   HRESULT hr = ::SHGetKnownFolderPath(FOLDERID_RoamingAppData, KF_FLAG_DEFAULT, NULL, &path);

   if ( SUCCEEDED(hr) )
   {
      return CString(path) + CString(_T("\\PGSuperV3\\"));
   }
   else
   {
      return pParentApp->GetAppLocation() + CString(_T("PGSuperV3\\"));
   }
}

CString CPGSuperProjectImporterPluginApp::GetTemplateFileExtension()
{ 
   CString strTemplateSuffix;
   VERIFY(strTemplateSuffix.LoadString(IDS_PGSUPER_TEMPLATE_FILE_SUFFIX));
   ASSERT(!strTemplateSuffix.IsEmpty());
   return strTemplateSuffix;
}

CATID CPGSuperProjectImporterPluginApp::GetProjectImporterCATID()
{
   return CATID_PGSuperProjectImporter;
}

UINT CPGSuperProjectImporterPluginApp::GetMenuResourceID()
{
   return IDR_PGSUPER;
}

CPGSImportPluginDocTemplateBase* CPGSuperProjectImporterPluginApp::CreateDocTemplate()
{
   auto callback = std::dynamic_pointer_cast<WBFL::EAF::ICommandCallback>(shared_from_this());

   CPGSuperImportPluginDocTemplate* pDocTemplate = new CPGSuperImportPluginDocTemplate(
		IDR_PGSUPERPROJECTIMPORTER,
        callback,
		RUNTIME_CLASS(CPGSuperDoc),
		RUNTIME_CLASS(CBridgeModelViewChildFrame),
		RUNTIME_CLASS(CBridgePlanView),
      m_hMenuShared,1);

   return pDocTemplate;
}

CEAFCommandLineInfo* CPGSuperProjectImporterPluginApp::CreateCommandLineInfo() const
{
   return new CPGSuperProjectImporterCommandLineInfo();
}

CString CPGSuperProjectImporterPluginApp::GetUsageMessage()
{
   CPGSuperProjectImporterCommandLineInfo pgsCmdInfo;
   return pgsCmdInfo.GetUsageMessage();
}
