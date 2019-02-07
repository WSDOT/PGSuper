///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2019  Washington State Department of Transportation
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

#include <BridgeLinkConfiguration.h>

#include <PgsExt\CatalogServers.h>
#include <PgsExt\BaseCommandLineInfo.h>
#include <PgsExt\PgsExtExp.h>



class PGSEXTCLASS CCatalogServerAppMixin : public IBridgeLinkConfigurationCallback
{
public:
   virtual CString GetAppName() const = 0;
   virtual CString GetTemplateFileExtension() = 0;

   virtual CString GetDefaultCatalogServerName() const = 0;
   virtual CString GetDefaultCatalogName() const = 0;

   virtual const CRuntimeClass* GetDocTemplateRuntimeClass() = 0;
   virtual void UpdateDocTemplates() = 0;

   virtual BOOL UseConfigurationCallback() { return TRUE; }

   virtual bool UpdateProgramSettings();

   virtual CPropertyPage* CreatePropertyPage() override;
   virtual void OnOK(CPropertyPage* pPage) override;

public:
   CCatalogServerAppMixin(void);
   ~CCatalogServerAppMixin(void);

   virtual HRESULT OnFinalConstruct();
   virtual void OnFinalRelease();

   CString GetMasterLibraryFile();
   CString GetCachedMasterLibraryFile();
   void GetTemplateFolders(CString& strWorkgroupFolder);

   void SetCacheUpdateFrequency(CacheUpdateFrequency frequence);
   CacheUpdateFrequency GetCacheUpdateFrequency();

   void SetSharedResourceType(SharedResourceType resType);
   SharedResourceType GetSharedResourceType();

   CString GetMasterLibraryPublisher() const;

protected:
   IDType m_CallbackID; // BridgeLink configuration interface callback ID

   SharedResourceType   m_SharedResourceType;     // method for using shared resources (Master lib and Workgroup templates)
   CacheUpdateFrequency m_CacheUpdateFrequency;

   CString m_CurrentCatalogServer; // name of current catalog server
   CString m_Publisher;     // Name of publisher in m_CurrentServer
   CString m_MasterLibraryFileURL; // URL of a published Master library file

   // Cache file/folder for Internet or Local Network resources
   CString m_MasterLibraryFileCache; 
   CString m_WorkgroupTemplateFolderCache;

   CString m_UserTemplateFolder;

   virtual LPCTSTR GetCatalogServerKey();
   virtual LPCTSTR GetPublisherKey();
   virtual LPCTSTR GetMasterLibraryCacheKey();
   virtual LPCTSTR GetMasterLibraryURLKey();
   virtual LPCTSTR GetWorkgroupTemplatesCacheKey();

   virtual CString GetDefaultMasterLibraryFile();
   virtual CString GetDefaultWorkgroupTemplateFolder();

   // call these from Init and Terminate
   virtual void DefaultInit(IEAFAppPlugin* pAppPlugin);
   virtual void DefaultTerminate();

   virtual void LoadRegistryValues();
   virtual void SaveRegistryValues();

   virtual void LoadRegistrySettings();
   virtual void LoadRegistryOptions();
   virtual void LoadRegistryCatalogServers();

   virtual void SaveRegistrySettings();
   virtual void SaveRegistryOptions();
   virtual void SaveRegistryCatalogServers();

   bool IsTimeToUpdateCache();
   bool AreUpdatesPending();


   void UpdateCache(); // only updates if needed
   bool DoCacheUpdate(); // always does the update
   sysDate GetLastCacheUpdateDate();
   void SetLastCacheUpdateDate(const sysDate& date);
   void RestoreLibraryAndTemplatesToDefault();
   void DeleteCache(LPCTSTR pstrCache);
   void RecursiveDelete(LPCTSTR pstr);
   
   virtual CString GetCacheFolder();
   virtual CString GetSaveCacheFolder();

   const CCatalogServers* GetCatalogServers() const;

   virtual void ProcessLibrarySetUp(const CPGSBaseCommandLineInfo& rCmdInfo);

private:
   CCatalogServers m_CatalogServers;

};
