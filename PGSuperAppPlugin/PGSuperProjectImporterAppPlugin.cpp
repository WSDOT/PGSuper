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
#include "PGSuperProjectImporterAppPlugin.h"
#include "BridgeModelViewChildFrame.h"
#include "BridgePlanView.h"
#include "PGSuperImportPluginDocTemplate.h"
#include <MFCTools\AutoRegistry.h>
#include "PGSuperCommandLineInfo.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



HRESULT CPGSuperProjectImporterAppPlugin::FinalConstruct()
{
   m_MyCmdTarget.m_pMyAppPlugin = this;
   return OnFinalConstruct(); // CPGSAppPluginBase
}

void CPGSuperProjectImporterAppPlugin::FinalRelease()
{
   OnFinalRelease(); // CPGSAppPluginBase
}
const CRuntimeClass* CPGSuperProjectImporterAppPlugin::GetDocTemplateRuntimeClass()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   return RUNTIME_CLASS(CPGSuperImportPluginDocTemplate);
}

LPCTSTR CPGSuperProjectImporterAppPlugin::GetCatalogServerKey() const
{
   return _T("CatalogServer2");
}

LPCTSTR CPGSuperProjectImporterAppPlugin::GetPublisherKey() const
{
   return _T("Publisher2");
}

LPCTSTR CPGSuperProjectImporterAppPlugin::GetMasterLibraryCacheKey() const
{
   return _T("MasterLibraryCache2");
}

LPCTSTR CPGSuperProjectImporterAppPlugin::GetMasterLibraryURLKey() const
{
   return _T("MasterLibraryURL2");
}

LPCTSTR CPGSuperProjectImporterAppPlugin::GetWorkgroupTemplatesCacheKey() const
{
   return _T("WorkgroupTemplatesCache2");
}

CString CPGSuperProjectImporterAppPlugin::GetCacheFolder() const
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

CString CPGSuperProjectImporterAppPlugin::GetTemplateFileExtension()
{ 
   CString strTemplateSuffix;
   VERIFY(strTemplateSuffix.LoadString(IDS_PGSUPER_TEMPLATE_FILE_SUFFIX));
   ASSERT(!strTemplateSuffix.IsEmpty());
   return strTemplateSuffix;
}

CATID CPGSuperProjectImporterAppPlugin::GetProjectImporterCATID()
{
   return CATID_PGSuperProjectImporter;
}

UINT CPGSuperProjectImporterAppPlugin::GetMenuResourceID()
{
   return IDR_PGSUPER;
}

CPGSImportPluginDocTemplateBase* CPGSuperProjectImporterAppPlugin::CreateDocTemplate()
{
   CPGSuperImportPluginDocTemplate* pDocTemplate = new CPGSuperImportPluginDocTemplate(
		IDR_PGSUPERPROJECTIMPORTER,
      this,
		RUNTIME_CLASS(CPGSuperDoc),
		RUNTIME_CLASS(CBridgeModelViewChildFrame),
		RUNTIME_CLASS(CBridgePlanView),
      m_hMenuShared,1);

   return pDocTemplate;
}

CEAFCommandLineInfo* CPGSuperProjectImporterAppPlugin::CreateCommandLineInfo() const
{
   return new CPGSuperProjectImporterCommandLineInfo();
}

CString CPGSuperProjectImporterAppPlugin::GetUsageMessage()
{
   CPGSuperProjectImporterCommandLineInfo pgsCmdInfo;
   return pgsCmdInfo.GetUsageMessage();
}
