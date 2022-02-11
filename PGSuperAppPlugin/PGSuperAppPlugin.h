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


#pragma once
#include "PGSuperAppPlugin_i.h"
#include "resource.h"

#include <EAF\EAFAppPlugin.h>
#include <EAF\EAFUIIntegration.h>
#include "PGSuperBaseAppPlugin.h"

class CPGSuperAppPlugin;

class CMyCmdTarget : public CCmdTarget
{
public:
   CMyCmdTarget() {};

   afx_msg void OnConfigurePlugins();
   afx_msg void OnUpdateTemplates();

   CPGSuperAppPlugin* m_pMyAppPlugin;

   DECLARE_MESSAGE_MAP()
};

class ATL_NO_VTABLE CPGSuperAppPlugin : 
   public CPGSAppPluginBase,
   public CComObjectRootEx<CComSingleThreadModel>,
   //public CComRefCountTracer<CPGSuperAppPlugin,CComObjectRootEx<CComSingleThreadModel>>,
   public CComCoClass<CPGSuperAppPlugin, &CLSID_PGSuperAppPlugin>,
   public IEAFAppPlugin,
   public IEAFAppCommandLine,
   public IEAFCommandCallback
{
public:
   CPGSuperAppPlugin()
   {
      m_MyCmdTarget.m_pMyAppPlugin = this;

      m_bUpdatingTemplate = false;
   }

BEGIN_COM_MAP(CPGSuperAppPlugin)
   COM_INTERFACE_ENTRY(IEAFAppPlugin)
   COM_INTERFACE_ENTRY(IEAFAppCommandLine)
   COM_INTERFACE_ENTRY(IEAFCommandCallback)
END_COM_MAP()

BEGIN_CONNECTION_POINT_MAP(CPGSuperAppPlugin)
END_CONNECTION_POINT_MAP()

DECLARE_REGISTRY_RESOURCEID(IDR_PGSUPERAPPPLUGINIMPL)

   virtual CString GetAppName() const override { return CString("PGSuper"); }
   virtual CString GetDefaultCatalogServerName() const override { return CString("WSDOT"); }
   virtual CString GetDefaultCatalogName() const override { return CString("WSDOT"); }
   virtual CString GetTemplateFileExtension() override;
   virtual const CRuntimeClass* GetDocTemplateRuntimeClass() override;

   //virtual BOOL UpdateProgramSettings(BOOL bFirstRun) override;


   HRESULT FinalConstruct();
   void FinalRelease();

private:
   HMENU m_hMenuShared;
   CMyCmdTarget m_MyCmdTarget;
   bool m_bUpdatingTemplate;

public:
   void ConfigurePlugins();
   void UpdateTemplates();
   bool UpdatingTemplates();

protected:
   virtual CEAFCommandLineInfo* CreateCommandLineInfo() const override;

   virtual LPCTSTR GetCatalogServerKey() const override;
   virtual LPCTSTR GetPublisherKey() const override;
   virtual LPCTSTR GetMasterLibraryCacheKey() const override;
   virtual LPCTSTR GetMasterLibraryURLKey() const override;
   virtual LPCTSTR GetWorkgroupTemplatesCacheKey() const override;
   virtual CString GetCacheFolder() const override;

// IEAFAppPlugin
public:
   virtual BOOL Init(CEAFApp* pParent) override;
   virtual void Terminate() override;
   virtual void IntegrateWithUI(BOOL bIntegrate) override;
   virtual std::vector<CEAFDocTemplate*> CreateDocTemplates() override;
   virtual HMENU GetSharedMenuHandle() override;
   virtual CString GetName() override;
   virtual CString GetDocumentationSetName() override;
   virtual CString GetDocumentationURL() override;
   virtual CString GetDocumentationMapFile() override;
   virtual void LoadDocumentationMap() override;
   virtual eafTypes::HelpResult GetDocumentLocation(LPCTSTR lpszDocSetName,UINT nID,CString& strURL) override;

// IEAFAppCommandLine
public:
   virtual CString GetCommandLineAppName() const override;
   virtual CString GetUsageMessage() override;
   virtual BOOL ProcessCommandLineOptions(CEAFCommandLineInfo& cmdInfo) override;

// IEAFCommandCallback
public:
   virtual BOOL OnCommandMessage(UINT nID,int nCode,void* pExtra,AFX_CMDHANDLERINFO* pHandlerInfo) override;
   virtual BOOL GetStatusBarMessageString(UINT nID, CString& rMessage) const override;
   virtual BOOL GetToolTipMessageString(UINT nID, CString& rMessage) const override;

};

OBJECT_ENTRY_AUTO(__uuidof(PGSuperAppPlugin), CPGSuperAppPlugin)
