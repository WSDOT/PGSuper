///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright � 1999-2025  Washington State Department of Transportation
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
#include "PGSuperAppPlugin_i.h"
#include "resource.h"
#include "PGSProjectImporterAppPluginBase.h"


class ATL_NO_VTABLE CPGSuperProjectImporterAppPlugin : 
   public CPGSProjectImporterAppPluginBase,
   public CComObjectRootEx<CComSingleThreadModel>,
   public CComCoClass<CPGSuperProjectImporterAppPlugin, &CLSID_PGSuperProjectImporterAppPlugin>
{
public:
   CPGSuperProjectImporterAppPlugin()
   {
   }

   HRESULT FinalConstruct();
   void FinalRelease();

   virtual CString GetAppName() const override { return CString("PGSuper"); }
   virtual CString GetDefaultCatalogServerName() const override  { return CString("WSDOT"); }
   virtual CString GetDefaultCatalogName() const override  { return CString("WSDOT"); }
   virtual CString GetTemplateFileExtension() override;
   virtual const CRuntimeClass* GetDocTemplateRuntimeClass() override;

   virtual LPCTSTR GetCatalogServerKey() const override;
   virtual LPCTSTR GetPublisherKey() const override;
   virtual LPCTSTR GetMasterLibraryCacheKey() const override;
   virtual LPCTSTR GetMasterLibraryURLKey() const override;
   virtual LPCTSTR GetWorkgroupTemplatesCacheKey() const override;
   virtual CString GetCacheFolder() const override;

   virtual CATID GetProjectImporterCATID() override;
   virtual UINT GetMenuResourceID() override;
   virtual CPGSImportPluginDocTemplateBase* CreateDocTemplate() override;

   virtual CEAFCommandLineInfo* CreateCommandLineInfo() const override;
   virtual CString GetUsageMessage() override;


BEGIN_COM_MAP(CPGSuperProjectImporterAppPlugin)
   COM_INTERFACE_ENTRY(IEAFAppPlugin)
   COM_INTERFACE_ENTRY(IEAFCommandCallback)
   COM_INTERFACE_ENTRY(IEAFAppCommandLine)
END_COM_MAP()

BEGIN_CONNECTION_POINT_MAP(CPGSuperProjectImporterAppPlugin)
END_CONNECTION_POINT_MAP()

DECLARE_REGISTRY_RESOURCEID(IDR_PGSUPERPROJECTIMPORTERAPPPLUGIN)
};

OBJECT_ENTRY_AUTO(__uuidof(PGSuperProjectImporterAppPlugin), CPGSuperProjectImporterAppPlugin)
