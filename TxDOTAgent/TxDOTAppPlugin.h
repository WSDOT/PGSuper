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

#include <EAF\EAFAppPlugin.h>
#include <EAF\EAFUIIntegration.h>
#include <EAF\EAFAppPluginDocumentationImpl.h>
#include <PgsExt\CatalogServerAppMixin.h>
#include "CLSID.h"
#include "resource.h"

class CTxDOTAppPlugin;

class CMyCmdTarget : public CCmdTarget
{
public:
   CMyCmdTarget() {};

   afx_msg void OnManagePlugins();
   afx_msg void OnUpdateTemplates();

   CTxDOTAppPlugin* m_pMyAppPlugin;

   DECLARE_MESSAGE_MAP()
};

class ATL_NO_VTABLE CTxDOTAppPlugin : 
   public CCatalogServerAppMixin,
   public CComObjectRootEx<CComSingleThreadModel>,
   public CComCoClass<CTxDOTAppPlugin, &CLSID_TxDOTAppPlugin>,
   public IEAFAppPlugin,
   public IEAFAppCommandLine,
   public IEAFCommandCallback
{
public:
   CTxDOTAppPlugin()
   {
      m_MyCmdTarget.m_pMyAppPlugin = this;
   }

DECLARE_REGISTRY_RESOURCEID(IDR_TXDOTAPPPLUGIN)

BEGIN_COM_MAP(CTxDOTAppPlugin)
   COM_INTERFACE_ENTRY(IEAFAppPlugin)
   COM_INTERFACE_ENTRY(IEAFCommandCallback)
   COM_INTERFACE_ENTRY(IEAFAppCommandLine)
END_COM_MAP()

BEGIN_CONNECTION_POINT_MAP(CTxDOTAppPlugin)
END_CONNECTION_POINT_MAP()

   HRESULT FinalConstruct();
   void FinalRelease();


   CMyCmdTarget m_MyCmdTarget;

   virtual CString GetAppName() const  override { return CString("TOGA"); }
   virtual CString GetDefaultCatalogServerName() const  override { return CString("TxDOT"); }
   virtual CString GetDefaultCatalogName()  const  override { return CString("TxDOT"); }

   virtual CPGSBaseCommandLineInfo* CreateCommandLineInfo() const;

   virtual CString GetTemplateFileExtension() override;
   virtual const CRuntimeClass* GetDocTemplateRuntimeClass() override;
   virtual void CTxDOTAppPlugin::UpdateDocTemplates() override;

   virtual CString GetDefaultMasterLibraryFile() const override;
   virtual CString GetDefaultWorkgroupTemplateFolder() const override;

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

public:
   void UpdateTemplates();

private:
   CEAFAppPluginDocumentationImpl m_DocumentationImpl;
};


OBJECT_ENTRY_AUTO(__uuidof(TxDOTAppPlugin), CTxDOTAppPlugin)
