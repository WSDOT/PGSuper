///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2015  Washington State Department of Transportation
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

#include "PGSuperCatalogServers.h"
#include <EAF\EAFApp.h>
#include <EAF\EAFCustomReport.h>

#include "PGSuperBaseCommandLineInfo.h"

// Base class for all PGSuper Document-type application plugins
// Performs common initialization expected by the CPGSuperDocBase class
class CPGSuperBaseAppPlugin
{
public:
   CPGSuperBaseAppPlugin();
   virtual ~CPGSuperBaseAppPlugin();

   virtual CString GetAppName() const = 0;
   virtual CString GetTemplateFileExtension() = 0;

   virtual HRESULT OnFinalConstruct();
   virtual void OnFinalRelease();

   // call these from Init and Terminate
   virtual void DefaultInit();
   virtual void DefaultTerminate();


   void GetAppUnitSystem(IAppUnitSystem** ppAppUnitSystem);

   CString GetEngineerName();
   CString GetEngineerCompany();

   CString GetMasterLibraryFile();
   CString GetCachedMasterLibraryFile();
   void GetTemplateFolders(CString& strWorkgroupFolder);

   void SetCacheUpdateFrequency(CacheUpdateFrequency frequence);
   CacheUpdateFrequency GetCacheUpdateFrequency();

   void SetSharedResourceType(SharedResourceType resType);
   SharedResourceType GetSharedResourceType();

   CString GetMasterLibraryPublisher() const;

   virtual BOOL UpdateProgramSettings(BOOL bFirstRun);

   // Determine whether to display favorite reports or all reports in menu dropdowns
   bool GetDoDisplayFavoriteReports() const;
   void SetDoDisplayFavoriteReports(bool doDisplay);

   // Current list of favorite reports
   const std::vector<std::_tstring>& GetFavoriteReports() const;
   void SetFavoriteReports(const std::vector<std::_tstring>& reports);

   // Custom, user-defined reports
   const CEAFCustomReports& GetCustomReports() const;
   void SetCustomReports(const CEAFCustomReports& reports);

protected:
   CString m_strAppProfileName; // this is the original app profile name before we mess with it
   // need to hang on to it so we can put it back the way it was

   SharedResourceType   m_SharedResourceType;     // method for using shared resources (Master lib and Workgroup templates)
   CacheUpdateFrequency m_CacheUpdateFrequency;

   CString m_CurrentCatalogServer; // name of current catalog server
   CString m_Publisher;     // Name of publisher in m_CurrentServer
   CString m_MasterLibraryFileURL; // URL of a published Master library file

   // Cache file/folder for Internet or Local Network resources
   CString m_MasterLibraryFileCache; 
   CString m_WorkgroupTemplateFolderCache;

   CString m_UserTemplateFolder;

   CString m_EngineerName;
   CString m_CompanyName;

   BOOL m_DisplayFavoriteReports;
   std::vector<std::_tstring> m_FavoriteReports;

   CEAFCustomReports m_CustomReports;

   virtual LPCTSTR GetCatalogServerKey();
   virtual LPCTSTR GetPublisherKey();
   virtual LPCTSTR GetMasterLibraryCacheKey();
   virtual LPCTSTR GetMasterLibraryURLKey();
   virtual LPCTSTR GetWorkgroupTemplatesCacheKey();

   virtual void RegistryConvert(); // Convert any old registry settings for current program (move into app plugin class)
   virtual void LoadRegistryValues();
   virtual void LoadSettings();
   virtual void LoadOptions();
   virtual void LoadReportOptions();
   virtual void SaveRegistryValues();
   virtual void SaveSettings();
   virtual void SaveOptions();
   virtual void SaveReportOptions();

   bool IsTimeToUpdateCache();
   bool AreUpdatesPending();

   void UpdateCache(); // only updates if needed
   bool DoCacheUpdate(); // always does the update
   sysDate GetLastCacheUpdateDate();
   void SetLastCacheUpdateDate(const sysDate& date);
   bool UpdateCatalogCache(IProgressMonitor* pProgress);
   void RestoreLibraryAndTemplatesToDefault();
   void DeleteCache(LPCTSTR pstrCache);
   void RecursiveDelete(LPCTSTR pstr);
   
   CString GetDefaultMasterLibraryFile();
   CString GetDefaultWorkgroupTemplateFolder();
   virtual CString GetCacheFolder();
   virtual CString GetSaveCacheFolder();

   const CPGSuperCatalogServers* GetCatalogServers() const;

   virtual CPGSuperBaseCommandLineInfo* CreateCommandLineInfo() const = 0;
   virtual BOOL DoProcessCommandLineOptions(CEAFCommandLineInfo& cmdInfo);
   virtual void Process1250Testing(const CPGSuperBaseCommandLineInfo& rCmdInfo);
   virtual void ProcessLibrarySetUp(const CPGSuperBaseCommandLineInfo& rCmdInfo);


private:
   CPGSuperCatalogServers m_CatalogServers;
   CComPtr<IAppUnitSystem> m_AppUnitSystem;
};
