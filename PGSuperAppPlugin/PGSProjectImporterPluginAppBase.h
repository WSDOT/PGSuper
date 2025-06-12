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
#include <EAF\PluginApp.h>
#include "PGSPluginAppBase.h"
#include "PGSImportPluginDocTemplateBase.h"

class CPGSProjectImporterPluginAppBase;

class CProjectImportersCmdTarget : public CCmdTarget
{
public:
   CProjectImportersCmdTarget() {};

   afx_msg void OnConfigureProjectImporters();

   CPGSProjectImporterPluginAppBase* m_pMyAppPlugin;

   DECLARE_MESSAGE_MAP()
};

class CPGSProjectImporterPluginAppBase : public CPGSPluginAppBase,
   public WBFL::EAF::IPluginApp,
   public WBFL::EAF::ICommandCallback,
   public WBFL::EAF::IAppCommandLine 
{
public:
   CPGSProjectImporterPluginAppBase()
   {
      m_MyCmdTarget.m_pMyAppPlugin = this;
   }

   virtual CATID GetProjectImporterCATID() = 0;
   virtual UINT GetMenuResourceID() = 0;
   virtual CPGSImportPluginDocTemplateBase* CreateDocTemplate() = 0;

   virtual BOOL UseConfigurationCallback() { return FALSE; }

   virtual void SaveReportOptions();
   virtual void SaveRegistryValues();

   HMENU m_hMenuShared;
   CProjectImportersCmdTarget m_MyCmdTarget;

   void ConfigureProjectImporters();

   BOOL DoProcessCommandLineOptions(CEAFCommandLineInfo& cmdInfo) override;
   void CreateNewProject(CPGSProjectImporterBaseCommandLineInfo& cmdInfo);


// IPluginApp
public:
   BOOL Init(CEAFApp* pParent) override;
   void Terminate() override;
   void IntegrateWithUI(BOOL bIntegrate) override;
   std::vector<CEAFDocTemplate*> CreateDocTemplates() override;
   HMENU GetSharedMenuHandle() override;
   CString GetName() override;
   CString GetDocumentationSetName() override;
   CString GetDocumentationURL() override;
   CString GetDocumentationMapFile() override;
   void LoadDocumentationMap() override;
   std::pair<WBFL::EAF::HelpResult,CString> GetDocumentLocation(LPCTSTR lpszDocSetName,UINT nID) override;

// ICommandCallback
public:
   BOOL OnCommandMessage(UINT nID,int nCode,void* pExtra,AFX_CMDHANDLERINFO* pHandlerInfo) override;
   BOOL GetStatusBarMessageString(UINT nID, CString& rMessage) const override;
   BOOL GetToolTipMessageString(UINT nID, CString& rMessage) const override;

   // IAppCommandLine
public:
   CString GetCommandLineAppName() const override;
   BOOL ProcessCommandLineOptions(CEAFCommandLineInfo& cmdInfo) override;
};
