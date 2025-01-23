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
#include "PGSuperAppPlugin_i.h"
#include <EAF\EAFAppPlugin.h>
#include "PGSuperBaseAppPlugin.h"
#include "PGSImportPluginDocTemplateBase.h"

class CPGSProjectImporterAppPluginBase;

class CProjectImportersCmdTarget : public CCmdTarget
{
public:
   CProjectImportersCmdTarget() {};

   afx_msg void OnConfigureProjectImporters();

   CPGSProjectImporterAppPluginBase* m_pMyAppPlugin;

   DECLARE_MESSAGE_MAP()
};

class ATL_NO_VTABLE CPGSProjectImporterAppPluginBase : 
   public CPGSAppPluginBase,
   public IEAFAppPlugin,
   public IEAFCommandCallback,
   public IEAFAppCommandLine 
{
public:
   CPGSProjectImporterAppPluginBase()
   {
   }

   HRESULT FinalConstruct();
   void FinalRelease();

   virtual CATID GetProjectImporterCATID() = 0;
   virtual UINT GetMenuResourceID() = 0;
   virtual CPGSImportPluginDocTemplateBase* CreateDocTemplate() = 0;

   virtual BOOL UseConfigurationCallback() { return FALSE; }

   virtual void SaveReportOptions();
   virtual void SaveRegistryValues();

   HMENU m_hMenuShared;
   CProjectImportersCmdTarget m_MyCmdTarget;

   void ConfigureProjectImporters();

   virtual BOOL DoProcessCommandLineOptions(CEAFCommandLineInfo& cmdInfo) override;
   void CreateNewProject(CPGSProjectImporterBaseCommandLineInfo& cmdInfo);


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

// IEAFCommandCallback
public:
   virtual BOOL OnCommandMessage(UINT nID,int nCode,void* pExtra,AFX_CMDHANDLERINFO* pHandlerInfo);
   virtual BOOL GetStatusBarMessageString(UINT nID, CString& rMessage) const;
   virtual BOOL GetToolTipMessageString(UINT nID, CString& rMessage) const;

   // IEAFAppCommandLine
public:
   virtual CString GetCommandLineAppName() const override;
   //virtual CString GetUsageMessage() override; // implement in subclass
   virtual BOOL ProcessCommandLineOptions(CEAFCommandLineInfo& cmdInfo) override;
};
