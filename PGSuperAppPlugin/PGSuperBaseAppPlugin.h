///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2023  Washington State Department of Transportation
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

#include <EAF\EAFApp.h>
#include <EAF\EAFCustomReport.h>
#include <EAF\EAFAppPluginDocumentationImpl.h>
#include "PgsExt\CatalogServers.h"
#include "PgsExt\CatalogServerAppMixin.h"
#include "pgsExt\BaseCommandLineInfo.h"

// Base class for all PGS Document-type application plugins
// Performs common initialization expected by the CPGSDocBase class
class CPGSAppPluginBase : 
   public CEAFCustomReportMixin,
   public CCatalogServerAppMixin
{
public:
   CPGSAppPluginBase();
   virtual ~CPGSAppPluginBase();

   virtual HRESULT OnFinalConstruct() override;
   virtual void OnFinalRelease() override;

   // call these from Init and Terminate
   virtual void DefaultInit(IEAFAppPlugin* pAppPlugin) override;
   virtual void DefaultTerminate() override;

   void GetAppUnitSystem(IAppUnitSystem** ppAppUnitSystem);

   virtual void UpdateDocTemplates() override;

   // CEAFCustomReportMixin
   virtual void LoadCustomReportInformation() override;
   virtual void SaveCustomReportInformation() override;

   virtual CString GetDocumentationURL();
   virtual CString GetDocumentationMapFile();
   virtual void LoadDocumentationMap();
   virtual eafTypes::HelpResult GetDocumentLocation(LPCTSTR lpszDocSetName,UINT nID,CString& strURL);

protected:
   CString m_strAppProfileName; // this is the original app profile name before we mess with it
   // need to hang on to it so we can put it back the way it was

   virtual void LoadRegistryValues() override;
   virtual void SaveRegistryValues() override;

   virtual CEAFCommandLineInfo* CreateCommandLineInfo() const = 0;
   virtual BOOL DoProcessCommandLineOptions(CEAFCommandLineInfo& cmdInfo);
   virtual void Process1250Testing(const CPGSBaseCommandLineInfo& rCmdInfo);

private:
   CComPtr<IAppUnitSystem> m_AppUnitSystem;
   CEAFAppPluginDocumentationImpl m_DocumentationImpl;
};
