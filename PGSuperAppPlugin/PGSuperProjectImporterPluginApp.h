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

#pragma once

#include "PGSProjectImporterPluginAppBase.h"

class CPGSuperProjectImporterPluginApp : 
   public CPGSProjectImporterPluginAppBase
{
public:
   CPGSuperProjectImporterPluginApp()
   {
      m_MyCmdTarget.m_pMyAppPlugin = this;
   }

   CString GetAppName() const override { return CString("PGSuper"); }
   CString GetDefaultCatalogServerName() const override  { return CString("WSDOT"); }
   CString GetDefaultCatalogName() const override  { return CString("WSDOT"); }
   CString GetTemplateFileExtension() override;
   const CRuntimeClass* GetDocTemplateRuntimeClass() override;

   LPCTSTR GetCatalogServerKey() const override;
   LPCTSTR GetPublisherKey() const override;
   LPCTSTR GetMasterLibraryCacheKey() const override;
   LPCTSTR GetMasterLibraryURLKey() const override;
   LPCTSTR GetWorkgroupTemplatesCacheKey() const override;
   CString GetCacheFolder() const override;

   CATID GetProjectImporterCATID() override;
   UINT GetMenuResourceID() override;
   CPGSImportPluginDocTemplateBase* CreateDocTemplate() override;

   CEAFCommandLineInfo* CreateCommandLineInfo() const override;
   CString GetUsageMessage() override;
};
