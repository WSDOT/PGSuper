///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2016  Washington State Department of Transportation
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
#include "PGSuperAppPlugin\PGSuperAppPlugin_i.h"
#include "PGSuperAppPlugin\resource.h"

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

   virtual CString GetAppName() const { return CString("PGSuper"); }
   virtual CString GetDefaultCatalogServerName() const  { return CString("WSDOT"); }
   virtual CString GetDefaultCatalogName()  const  { return CString("WSDOT"); }
   virtual CString GetTemplateFileExtension();
   virtual const CRuntimeClass* GetDocTemplateRuntimeClass();

   //virtual BOOL UpdateProgramSettings(BOOL bFirstRun);


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
   virtual CPGSBaseCommandLineInfo* CreateCommandLineInfo() const;

#if defined _BETA_VERSION
   virtual LPCTSTR GetCatalogServerKey();
   virtual LPCTSTR GetPublisherKey();
   virtual LPCTSTR GetMasterLibraryCacheKey();
   virtual LPCTSTR GetMasterLibraryURLKey();
   virtual LPCTSTR GetWorkgroupTemplatesCacheKey();
   virtual CString GetCacheFolder();
#endif //_BETA_VERSION

// IEAFAppPlugin
public:
   virtual BOOL Init(CEAFApp* pParent);
   virtual void Terminate();
   virtual void IntegrateWithUI(BOOL bIntegrate);
   virtual std::vector<CEAFDocTemplate*> CreateDocTemplates();
   virtual HMENU GetSharedMenuHandle();
   virtual CString GetName();
   virtual CString GetDocumentationSetName();
   virtual CString GetDocumentationURL();
   virtual CString GetDocumentationMapFile();
   virtual void LoadDocumentationMap();
   virtual eafTypes::HelpResult GetDocumentLocation(LPCTSTR lpszDocSetName,UINT nID,CString& strURL);

// IEAFAppCommandLine
public:
   virtual CString GetUsageMessage();
   virtual BOOL ProcessCommandLineOptions(CEAFCommandLineInfo& cmdInfo);

// IEAFCommandCallback
public:
   virtual BOOL OnCommandMessage(UINT nID,int nCode,void* pExtra,AFX_CMDHANDLERINFO* pHandlerInfo);
   virtual BOOL GetStatusBarMessageString(UINT nID, CString& rMessage) const;
   virtual BOOL GetToolTipMessageString(UINT nID, CString& rMessage) const;

};

OBJECT_ENTRY_AUTO(__uuidof(PGSuperAppPlugin), CPGSuperAppPlugin)
