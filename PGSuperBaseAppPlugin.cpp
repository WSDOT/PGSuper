///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2010  Washington State Department of Transportation
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
#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin.h"
#include "PGSuperAppPlugin\PGSuperApp.h"
#include <PGSuperUnits.h>
#include "PGSuperDocTemplate.h"
#include "ConfigurePGSuperDlg.h"

CPGSuperBaseAppPlugin::CPGSuperBaseAppPlugin()
{
   m_CacheUpdateFrequency = Daily;
   m_SharedResourceType = srtInternetFtp;
}

HRESULT CPGSuperBaseAppPlugin::OnFinalConstruct()
{
   if ( !CreateAppUnitSystem(&m_AppUnitSystem) )
      return E_FAIL;

   return S_OK;
}

void CPGSuperBaseAppPlugin::OnFinalRelease()
{
   m_AppUnitSystem.Release();
}

void CPGSuperBaseAppPlugin::DefaultInit()
{
   LoadRegistryValues();
}

void CPGSuperBaseAppPlugin::DefaultTerminate()
{
   SaveRegistryValues();
}

void CPGSuperBaseAppPlugin::GetAppUnitSystem(IAppUnitSystem** ppAppUnitSystem)
{
   (*ppAppUnitSystem) = m_AppUnitSystem;
   (*ppAppUnitSystem)->AddRef();
}


CString CPGSuperBaseAppPlugin::GetEngineerName()
{
   return m_EngineerName;
}

CString CPGSuperBaseAppPlugin::GetEngineerCompany()
{
   return m_CompanyName;
}

void CPGSuperBaseAppPlugin::LoadRegistryValues()
{
   // The default values are read from HKEY_LOCAL_MACHINE\Software\Washington State Deparment of Transportation\PGSuper
   // If the default values are missing, the hard coded defaults found herein are used.
   // Install writers can create MSI transforms to alter the "defaults" by changing the registry values
   CEAFApp* pApp = EAFGetApp();

   int iDefaultSharedResourceType     = pApp->GetLocalMachineInt(   _T("Settings"),_T("SharedResourceType"),    1);
   int iDefaultCacheUpdateFrequency   = pApp->GetLocalMachineInt(   _T("Settings"),_T("CacheUpdateFrequency"),  2);

   CString strDefaultUserTemplateFolder     = pApp->GetLocalMachineString(_T("Options"),_T("UserTemplateLocation"), _T("C:\\"));
   CString strDefaultCatalogServer          = pApp->GetLocalMachineString(_T("Options"),_T("CatalogServer"),_T("WSDOT"));
   CString strDefaultPublisher              = pApp->GetLocalMachineString(_T("Options"),_T("Publisher"),_T("WSDOT"));
   CString strDefaultLocalMasterLibraryFile = pApp->GetLocalMachineString(_T("Options"),_T("MasterLibraryLocal"),     GetDefaultMasterLibraryFile());
   CString strLocalWorkgroupTemplateFolder  = pApp->GetLocalMachineString(_T("Options"),_T("WorkgroupTemplatesLocal"),GetDefaultWorkgroupTemplateFolder());

   CString strDefaultCompany                = pApp->GetLocalMachineString(_T("Options"),_T("CompanyName"), _T("Your Company"));
   CString strDefaultEngineer               = pApp->GetLocalMachineString(_T("Options"),_T("EngineerName"),_T("Your Name"));

   // NOTE: Settings is an MFC created Registry section

   // Do any necessary conversions from previous versions of PGSuper
   RegistryConvert();

   /////////////////////////////////
   // User Information Settings
   /////////////////////////////////

   // company name
   m_CompanyName = pApp->GetProfileString(_T("Options"),_T("CompanyName"), strDefaultCompany);

   // engineer name
   m_EngineerName = pApp->GetProfileString(_T("Options"),_T("EngineerName"), strDefaultEngineer);

   /////////////////////////////////
   // Shared Resource Settings
   /////////////////////////////////

   // defaults
   CString strVersion = theApp.GetVersion(true);
   CString strFTPServer("ftp://ftp.wsdot.wa.gov/public/bridge/Software/PGSuper");
   CString strDefaultMasterLibraryURL;
   strDefaultMasterLibraryURL.Format("%s/Version_%s/WSDOT.lbr",strFTPServer,strVersion);
   CString strDefaultWorkgroupTemplateFolderURL;
   strDefaultWorkgroupTemplateFolderURL.Format("%s/Version_%s/WSDOT_Templates/",strFTPServer,strVersion);

   m_SharedResourceType   = (SharedResourceType)  pApp->GetProfileInt(_T("Settings"),_T("SharedResourceType"),  iDefaultSharedResourceType);
   m_CacheUpdateFrequency = (CacheUpdateFrequency)pApp->GetProfileInt(_T("Settings"),_T("CacheUpdateFrequency"),iDefaultCacheUpdateFrequency);

   // Internet resources
   m_CatalogServers.LoadFromRegistry(pApp);
   m_CurrentCatalogServer = pApp->GetProfileString(_T("Options"),_T("CatalogServer"),strDefaultCatalogServer);

   m_Publisher = pApp->GetProfileString(_T("Options"),_T("Publisher"),strDefaultPublisher);

   m_MasterLibraryFileURL = pApp->GetProfileString(_T("Options"),_T("MasterLibraryURL"),strDefaultMasterLibraryURL);

   // Cache file/folder for Internet or Local Network resources
   m_MasterLibraryFileCache       = pApp->GetProfileString(_T("Options"),_T("MasterLibraryCache"),     GetCacheFolder()+GetMasterLibraryFileName());
   m_WorkgroupTemplateFolderCache = pApp->GetProfileString(_T("Options"),_T("WorkgroupTemplatesCache"),GetCacheFolder()+GetTemplateSubFolderName()+"\\");
}

void CPGSuperBaseAppPlugin::SaveRegistryValues()
{
   CEAFApp* pApp = EAFGetApp();

   // Options settings
   VERIFY(pApp->WriteProfileString(_T("Options"), _T("CompanyName"), m_CompanyName));

   // engineer name
   VERIFY(pApp->WriteProfileString(_T("Options"), _T("EngineerName"), m_EngineerName));

   pApp->WriteProfileInt(_T("Settings"),_T("SharedResourceType"),m_SharedResourceType);
   pApp->WriteProfileInt(_T("Settings"),_T("CacheUpdateFrequency"),m_CacheUpdateFrequency);

   // Internet resources
   pApp->WriteProfileString(_T("Options"),_T("CatalogServer"),m_CurrentCatalogServer);
   pApp->WriteProfileString(_T("Options"),_T("Publisher"),m_Publisher);
   pApp->WriteProfileString(_T("Options"),_T("MasterLibraryURL"),m_MasterLibraryFileURL);

   // Cache file/folder for Internet or Local Network resources
   pApp->WriteProfileString(_T("Options"),_T("MasterLibraryCache"),     m_MasterLibraryFileCache);
   pApp->WriteProfileString(_T("Options"),_T("WorkgroupTemplatesCache"),m_WorkgroupTemplateFolderCache);

   m_CatalogServers.SaveToRegistry(pApp);
}

void CPGSuperBaseAppPlugin::RegistryConvert()
{
   CEAFApp* pApp = EAFGetApp();

   // Prior to version 2.2.3 we allowed only a single local file system catalog server and the
   // app class did much of the server heavy lifting. The code below removes those registry keys
   // and converts the data to the new format

   // This code was written in Jan, 2010. If you are reading this in, say 2012, you 
   // are probably safe to blast it as nobody should be using 2 year old versions of PGSuper

   SharedResourceType type   = (SharedResourceType)pApp->GetProfileInt(_T("Settings"),_T("SharedResourceType"),  -1);
   CString masterlib  = pApp->GetProfileString(_T("Options"),_T("MasterLibraryLocal"), _T("Bogus"));
   CString tempfolder = pApp->GetProfileString(_T("Options"),_T("WorkgroupTemplatesLocal"), _T("Bogus"));

   // If we were set to a local library stored in old format, create a new catalog server 
   // and add its settings to the registry
   if (masterlib!="Bogus" && type==srtLocal)
   {
      // our publisher and server is "Local Files"
      pApp->WriteProfileString(_T("Options"),_T("CatalogServer"),"Local Files");
      pApp->WriteProfileString(_T("Options"),_T("Publisher"),    "Local Files");

      // create that server
      CFileSystemPGSuperCatalogServer server("Local Files",masterlib,tempfolder);
      CString create_string = GetCreationString(&server);

      int count = pApp->GetProfileInt(_T("Servers"),_T("Count"),-1);
      count==-1 ? count=1 : count++;
      pApp->WriteProfileInt(_T("Servers"),_T("Count"),count);

      CString key(char(count-1+'A'));
      pApp->WriteProfileString(_T("Servers"), key, create_string);
   }

   // Delete profile strings if they are not empty
   if (masterlib!="Bogus")
   {
      pApp->WriteProfileString(_T("Options"),_T("MasterLibraryLocal"),NULL);
   }

   if (tempfolder!="Bogus")
   {
      pApp->WriteProfileString(_T("Options"),_T("WorkgroupTemplatesLocal"), NULL);
   }

   //  m_WorkgroupTemplateFolderURL is no longer used
   pApp->WriteProfileString(_T("Options"),_T("WorkgroupTemplatesURL"),NULL);

   // user template location no longer used
   pApp->WriteProfileString(_T("Options"), _T("UserTemplateLocation"), NULL);

}

CString CPGSuperBaseAppPlugin::GetCachedMasterLibraryFile()
{
   return m_MasterLibraryFileCache;
}

CString CPGSuperBaseAppPlugin::GetMasterLibraryFile()
{
   CString strMasterLibFile;
   switch( m_SharedResourceType )
   {
   case srtDefault:
      strMasterLibFile = GetDefaultMasterLibraryFile();
      break;

   case srtInternetFtp:
   case srtInternetHttp:
   case srtLocal:
      {
         // use our cached value
         strMasterLibFile = m_MasterLibraryFileURL;
      }
      break;

   default:
      ATLASSERT(false);
      strMasterLibFile = GetDefaultMasterLibraryFile();
      break;
   }

   return strMasterLibFile;
}

void CPGSuperBaseAppPlugin::GetTemplateFolders(CString& strWorkgroupFolder)
{
   strWorkgroupFolder = m_WorkgroupTemplateFolderCache;
}

void CPGSuperBaseAppPlugin::SetCacheUpdateFrequency(CacheUpdateFrequency frequency)
{
   m_CacheUpdateFrequency = frequency;
}

CacheUpdateFrequency CPGSuperBaseAppPlugin::GetCacheUpdateFrequency()
{
   return m_CacheUpdateFrequency;
}

void CPGSuperBaseAppPlugin::SetSharedResourceType(SharedResourceType resType)
{
   m_SharedResourceType = resType;
}

SharedResourceType CPGSuperBaseAppPlugin::GetSharedResourceType()
{
   return m_SharedResourceType;
}

CString CPGSuperBaseAppPlugin::GetMasterLibraryPublisher() const
{
   CString strPublisher;
   switch( m_SharedResourceType )
   {
   case srtDefault:
      strPublisher = "Default libraries installed with PGSuper";
      break;

   case srtInternetFtp:
   case srtInternetHttp:
      strPublisher = m_Publisher;
      break;

   case srtLocal:
      strPublisher = "Published on Local Network";
      break;

   default:
      ATLASSERT(false);
      break;
   }

   return strPublisher;
}

void CPGSuperBaseAppPlugin::UpdateProgramSettings(BOOL bFirstRun) 
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CEAFApp* pApp = EAFGetApp();

   if (!bFirstRun && pApp->IsDocLoaded())
   {
      AfxMessageBox("Program settings cannot be changed while a project is open. Close this project and try again.",MB_OK|MB_ICONINFORMATION);
   }
   else
   {
      CConfigurePGSuperDlg dlg(bFirstRun,EAFGetMainFrame());

      dlg.m_Company  = m_CompanyName;
      dlg.m_Engineer = m_EngineerName;

      dlg.m_SharedResourceType               = m_SharedResourceType;
      dlg.m_CacheUpdateFrequency = m_CacheUpdateFrequency;
      dlg.m_CurrentServer  = m_CurrentCatalogServer;
      dlg.m_Publisher      = m_Publisher;
      dlg.m_Servers = m_CatalogServers;

      // Save a copy of all server information in case our update fails
      SharedResourceType        original_type = m_SharedResourceType;
      CacheUpdateFrequency      original_freq = m_CacheUpdateFrequency;
      CString                 original_server = m_CurrentCatalogServer;
      CString              original_publisher = m_Publisher;
      CPGSuperCatalogServers original_servers = m_CatalogServers;

      int result = dlg.DoModal();

      if ( result == IDOK )
      {
         m_EngineerName         = dlg.m_Engineer;
         m_CompanyName          = dlg.m_Company;

         m_SharedResourceType   = dlg.m_SharedResourceType;
         m_CacheUpdateFrequency = dlg.m_CacheUpdateFrequency;

         m_CatalogServers       = dlg.m_Servers;
         m_CurrentCatalogServer = dlg.m_CurrentServer;
         m_Publisher            = dlg.m_Publisher;

         if ( m_SharedResourceType == srtDefault )
         {
            // Using hard coded defaults
            RestoreLibraryAndTemplatesToDefault();
         }
      }

      if ( dlg.m_bUpdateCache )
      {
         if (!DoCacheUpdate())
         {  
            // DoCacheUpdate will restore the cache, we need also to restore local data
            m_SharedResourceType   = original_type;
            m_CacheUpdateFrequency = original_freq;
            m_CurrentCatalogServer = original_server;
            m_Publisher            = original_publisher;
            m_CatalogServers       = original_servers;
         }
      }

      if ( result == IDOK )
      {
         SaveRegistryValues(); // Saves all the current settings to the registery
                               // There is no sense waiting until PGSuper closes to do this
       }
   }
}

void CPGSuperBaseAppPlugin::UpdateCache()
{
   CEAFApp* pApp = EAFGetApp();

   bool was_error=false;
   CString error_msg;

   try
   {
      BOOL bFirstRun = pApp->IsFirstRun();
      if (bFirstRun)
      {
         LOG("Update Cache -> First Run");

         // if this is the first time PGSuper is run after installation
         // go right to the program settings. OnProgramSettings will
         // force an update
         UpdateProgramSettings(TRUE); 
      }
      else if ( IsTimeToUpdateCache() )
      {
         if ( AreUpdatesPending() )
         {
            LOG("Time to update cache and Updates are pending");
            // this is not the first time, it is time to check for updates,
            // and sure enough there are updates pending.... do the update
            int result = ::MessageBox(EAFGetMainFrame()->GetSafeHwnd(),"There are updates to Master Library and Workgroup Templates pending.\n\nWould you like to update PGSuper now?","Pending Updates",MB_YESNO | MB_ICONINFORMATION);

            if ( result == IDYES )
               was_error = !DoCacheUpdate();
         }
         else
         {
            // There aren't any update pending... set the last update date to now, otherwise
            // pgsuper will check for updates every day until there are some
            sysDate now;
            SetLastCacheUpdateDate(now);
         }
      }
   }
   catch(CCatalogServerException exp)
   {
      // catch and report error message
      error_msg = exp.GetErrorMessage();
      was_error = true;
   }
   catch(...)
   {
      error_msg = "Error cause was unknown";
      was_error = true;
   }

   if (was_error)
   {
      // Things are totally screwed if we end up here. Reset to default library and templates from install
      RestoreLibraryAndTemplatesToDefault();
      CString msg;
      msg.Format("The following error occurred while loading Library and Template server information:\n %s \nThese settings have been restored to factory defaults.",error_msg);
      ::AfxMessageBox(msg,MB_ICONINFORMATION);
   }
}

void CPGSuperBaseAppPlugin::RestoreLibraryAndTemplatesToDefault()
{
   m_SharedResourceType = srtDefault;

   m_MasterLibraryFileCache       = GetDefaultMasterLibraryFile();
   m_WorkgroupTemplateFolderCache = GetDefaultWorkgroupTemplateFolder();

   m_MasterLibraryFileURL = m_MasterLibraryFileCache;
}

void CPGSuperBaseAppPlugin::DeleteCache(LPCSTR pstrCache)
{
   RecursiveDelete(pstrCache);
   ::RemoveDirectory(pstrCache);
}

void CPGSuperBaseAppPlugin::RecursiveDelete(LPCSTR pstr)
{
   CFileFind finder;

   CString strWildcard(pstr);
   strWildcard += "\\*.*";

   BOOL bWorking = finder.FindFile(strWildcard);
   while ( bWorking )
   {
      bWorking = finder.FindNextFile();

      if ( finder.IsDots() )
         continue;

      CString str = finder.GetFilePath();
      if ( finder.IsDirectory() )
      {
         RecursiveDelete(str);

         ::RemoveDirectory(str);
      }
      else
      {
         ::DeleteFile(str);
      }
   }

   finder.Close();
}

bool CPGSuperBaseAppPlugin::IsTimeToUpdateCache()
{
   LOG("IsTimeToUpdateCache()");
   if ( m_SharedResourceType == srtDefault )
   {
      LOG("Using default resource type, no updating");
      return false;
   }

   // resources are local network or Internet
   bool bTimeToUpdate = false;
   if ( m_CacheUpdateFrequency == Never )
   {
      LOG("Never update");
      bTimeToUpdate = false;
   }
   else if ( m_CacheUpdateFrequency == Always )
   {
      LOG("Always update");
      bTimeToUpdate = true;
   }
   else 
   {
      sysDate now;
      sysDate last_update = GetLastCacheUpdateDate();

      Int16 update_frequency = 0; // in days
      if ( m_CacheUpdateFrequency == Daily )
      {
         LOG("Update daily");
         update_frequency = 1;
      }
      else if ( m_CacheUpdateFrequency == Weekly )
      {
         LOG("Update weekly");
         update_frequency = 7;
      }
      else if ( m_CacheUpdateFrequency == Monthly )
      {
         LOG("Update monthly");
         update_frequency = 30;
      }

      sysDate next_update = last_update;
      next_update += update_frequency;

      if ( next_update <= now )
      {
         bTimeToUpdate = true;
      }
   }

   LOG("Time to update = " << (bTimeToUpdate ? "Yes" : "No"));
   return bTimeToUpdate;
}

bool CPGSuperBaseAppPlugin::AreUpdatesPending()
{
   // get the MD5 files from the Internet or local network, compute the MD5 of the cache
   // if different, then there is an update pending

   LOG("AreUpdatesPending()");

   bool bUpdatesPending = false;
   if ( m_SharedResourceType == srtDefault )
   {
      bUpdatesPending = false;
   }
   else if ( m_SharedResourceType == srtInternetFtp ||
             m_SharedResourceType == srtInternetHttp ||
             m_SharedResourceType == srtLocal)
   {

      // create a progress window
      CComPtr<IProgressMonitorWindow> wndProgress;
      wndProgress.CoCreateInstance(CLSID_ProgressMonitorWindow);
      wndProgress->put_HasGauge(VARIANT_FALSE);
      wndProgress->put_HasCancel(VARIANT_FALSE);
      CEAFMainFrame* pWnd = EAFGetMainFrame();
      wndProgress->Show(CComBSTR("Checking the Internet for Library updates"),pWnd->GetSafeHwnd());

      try
      {
         // Catalog server does the work here
         const CPGSuperCatalogServer* pserver = m_CatalogServers.GetServer(m_CurrentCatalogServer);
         if (pserver!=NULL)
         {
            bUpdatesPending = pserver->CheckForUpdates(m_Publisher, NULL, GetCacheFolder());
         }
         else
         {
            CString msg;
            msg.Format("Error - currently selected catalog server not found. Name was: %s",m_CurrentCatalogServer);
            CCatalogServerException exc(CCatalogServerException::ceServerNotFound, msg);
            throw exc;
         }
      }
      catch(...)
      {
         throw;
      }
   }
   else
   {
      ATLASSERT(0);
   }

   LOG("Pending updates = " << (bUpdatesPending ? "Yes" : "No"));
   return bUpdatesPending;
}


bool CPGSuperBaseAppPlugin::DoCacheUpdate()
{
   CEAFApp* pApp = EAFGetApp();

   LOG("DoUpdateCache()");

   // create a progress window
   CComPtr<IProgressMonitorWindow> wndProgress;
   wndProgress.CoCreateInstance(CLSID_ProgressMonitorWindow);
   wndProgress->put_HasGauge(VARIANT_FALSE);
   wndProgress->put_HasCancel(VARIANT_FALSE);

   CComQIPtr<IProgressMonitor> progress(wndProgress);
   CEAFMainFrame* pWnd = EAFGetMainFrame();
   wndProgress->Show(CComBSTR("Update Libraries and Templates"),pWnd->GetSafeHwnd());

   // setup cache folders
   CString strAppPath = pApp->GetAppLocation();

   CString strSaveCache = GetSaveCacheFolder();
   CString strCache     = GetCacheFolder();

   SetCurrentDirectory(strAppPath);

   // Save the current cache in case of failure
   DeleteCache(strSaveCache); // be safe and delete the previous "SaveCache" if one exists
   int retval = rename(strCache,strSaveCache); // rename the existing cache to "SaveCache" in case there is an update error
   if ( retval != 0 )
   {
      // there was an error renaming the cache
      // just delete its contents
      DeleteCache(strCache);
   }

   ::CreateDirectory(strCache,0); // create a new cache folder to fill up
   ::SetFileAttributes(strCache,FILE_ATTRIBUTE_HIDDEN); // make the cache a hidden folder

   // fill up the cache
   bool bSuccessful(false);
   if ( m_SharedResourceType == srtDefault )
   {
      m_MasterLibraryFileCache = GetDefaultMasterLibraryFile();
      m_WorkgroupTemplateFolderCache = GetDefaultWorkgroupTemplateFolder();
      return true;
   }
   else if ( m_SharedResourceType == srtInternetFtp ||
             m_SharedResourceType == srtInternetHttp ||
             m_SharedResourceType == srtLocal)
   {
      // set cache folder
      m_MasterLibraryFileCache = GetCacheFolder() + GetMasterLibraryFileName();
      m_WorkgroupTemplateFolderCache = GetCacheFolder() + GetTemplateSubFolderName() + CString("\\");

      // Catalog server takes care of business
      try
      {
         const CPGSuperCatalogServer* pserver = m_CatalogServers.GetServer(m_CurrentCatalogServer);

         if (pserver!=NULL)
         {
            bSuccessful = pserver->PopulateCatalog(m_Publisher,progress,GetCacheFolder());

            m_MasterLibraryFileURL = pserver->GetMasterLibraryURL(m_Publisher);

            if ( m_MasterLibraryFileURL == "" )
               m_MasterLibraryFileCache = "";
         }
         else
         {
            CString msg;
            msg.Format("Error: Cannot perform update - Currently selected catalog server is not in server list. Name is %s",m_CurrentCatalogServer);
            AfxMessageBox(msg,MB_ICONEXCLAMATION);
            bSuccessful = false;
         }
      }
      catch(CCatalogServerException exp)
      {
         // catch and report error message
         CString msg = exp.GetErrorMessage();
         AfxMessageBox(msg,MB_ICONEXCLAMATION);
         bSuccessful = false;
      }
      catch(...)
      {
         ATLASSERT(0);
         bSuccessful = false;
      }
   }
   else
   {
      ATLASSERT(0);
   }

   // if the cache was successfully updated, delete the saved copy and update the time stamp
   if ( bSuccessful )
   {
      DeleteCache(strSaveCache);
      sysDate now;
      SetLastCacheUpdateDate(now);
   }
   else
   {
      // otherwise, delete the messed up cache and put it back the way it was
      DeleteCache(strCache);
      rename(strSaveCache,strCache);
   }

   if (bSuccessful)
      progress->put_Message(0,CComBSTR("The Master Library and Templates have been updated"));
   else
      progress->put_Message(0,CComBSTR("Update failed. Previous settings restored."));

   if ( bSuccessful )
   {
      // Need to update the main document template so that the File | New dialog is updated
      // Search for the CPGSuperDocTemplate object
      POSITION pos = pApp->m_pDocManager->GetFirstDocTemplatePosition();
      while ( pos != NULL )
      {
         CDocTemplate* pDocTemplate = pApp->m_pDocManager->GetNextDocTemplate(pos);
         if ( pDocTemplate->IsKindOf(RUNTIME_CLASS(CPGSuperDocTemplate)) )
         {
            CPGSuperDocTemplate* pPGSuperDocTemplate = dynamic_cast<CPGSuperDocTemplate*>(pDocTemplate);
            pPGSuperDocTemplate->LoadTemplateInformation();
            break;
         }
      }
   }

   return bSuccessful;
}

sysDate CPGSuperBaseAppPlugin::GetLastCacheUpdateDate()
{
   CEAFApp* pApp = EAFGetApp();

   JulTy last_update = pApp->GetProfileInt(_T("Settings"),_T("LastCacheUpdate"),0);
   return sysDate(last_update);
}

void CPGSuperBaseAppPlugin::SetLastCacheUpdateDate(const sysDate& date)
{
   CEAFApp* pApp = EAFGetApp();

   pApp->WriteProfileInt(_T("Settings"),_T("LastCacheUpdate"),date.Hash());
}

CString CPGSuperBaseAppPlugin::GetDefaultMasterLibraryFile()
{
   CEAFApp* pApp = EAFGetApp();

   CString strAppPath = pApp->GetAppLocation();
   return strAppPath + CString("WSDOT.lbr");
}

CString CPGSuperBaseAppPlugin::GetDefaultWorkgroupTemplateFolder()
{
   CEAFApp* pApp = EAFGetApp();

   CString strAppPath = pApp->GetAppLocation();
   return strAppPath + CString("Templates");
}

CString CPGSuperBaseAppPlugin::GetCacheFolder()
{
   CEAFApp* pApp = EAFGetApp();

   TCHAR buffer[MAX_PATH];
   BOOL bResult = ::SHGetSpecialFolderPath(NULL,buffer,CSIDL_APPDATA,FALSE);

   if ( !bResult )
      return pApp->GetAppLocation() + CString("Cache\\");
   else
      return CString(buffer) + CString("\\PGSuper\\");
}

CString CPGSuperBaseAppPlugin::GetSaveCacheFolder()
{
   CEAFApp* pApp = EAFGetApp();

   TCHAR buffer[MAX_PATH];
   BOOL bResult = ::SHGetSpecialFolderPath(NULL,buffer,CSIDL_APPDATA,FALSE);

   if ( !bResult )
      return pApp->GetAppLocation() + CString("SaveCache\\");
   else
      return CString(buffer) + CString("\\PGSuper_Save\\");
}

const CPGSuperCatalogServers* CPGSuperBaseAppPlugin::GetCatalogServers() const
{
   return &m_CatalogServers;
}
