///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2014  Washington State Department of Transportation
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

#include <EAF\EAFDocManager.h>
#include <EAF\EAFBrokerDocument.h>

#include <IFace\Test1250.h>

// class that sets the application profile name for this plug-in and then
// rolls it back to the original value when the object goes out of scope
class CAutoRegistry
{
public:
   CAutoRegistry(LPCTSTR lpszProfile)
   {
      AFX_MANAGE_STATE(AfxGetStaticModuleState());
      CWinApp* pMyApp = AfxGetApp();

      m_strAppProfileName = pMyApp->m_pszProfileName;
      free((void*)pMyApp->m_pszProfileName);
      pMyApp->m_pszProfileName = _tcsdup(lpszProfile);
   }

   ~CAutoRegistry()
   {
      AFX_MANAGE_STATE(AfxGetStaticModuleState());
      CWinApp* pMyApp = AfxGetApp();
      free((void*)pMyApp->m_pszProfileName);
      pMyApp->m_pszProfileName = _tcsdup(m_strAppProfileName);
   }

private:
   CString m_strAppProfileName;
};

CPGSuperBaseAppPlugin::CPGSuperBaseAppPlugin()
{
   m_CacheUpdateFrequency = Daily;
   m_SharedResourceType = srtInternetFtp;
}

CPGSuperBaseAppPlugin::~CPGSuperBaseAppPlugin()
{
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
   m_CatalogServers.SetTemplateFileExtenstion(GetTemplateFileExtension());
   LoadRegistryValues();
}

void CPGSuperBaseAppPlugin::DefaultTerminate()
{
   SaveRegistryValues();
}

void CPGSuperBaseAppPlugin::GetAppUnitSystem(IAppUnitSystem** ppAppUnitSystem)
{
   m_AppUnitSystem.CopyTo(ppAppUnitSystem);
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
   CAutoRegistry autoReg(GetAppName());

   // The default values are read from HKEY_LOCAL_MACHINE\Software\Washington State Deparment of Transportation\PGSuper
   // If the default values are missing, the hard coded defaults found herein are used.
   // Install writers can create MSI transforms to alter the "defaults" by changing the registry values
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   CPGSuperAppPluginApp* pApp = (CPGSuperAppPluginApp*)AfxGetApp();

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
   CString strFTPServer(_T("ftp://ftp.wsdot.wa.gov/public/bridge/software"));
   CString strDefaultMasterLibraryURL;
   strDefaultMasterLibraryURL.Format(_T("%s/Version_%s/WSDOT.lbr"),strFTPServer,strVersion);
   CString strDefaultWorkgroupTemplateFolderURL;
   strDefaultWorkgroupTemplateFolderURL.Format(_T("%s/Version_%s/WSDOT_Templates/"),strFTPServer,strVersion);

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
   CAutoRegistry autoReg(GetAppName());

   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   CWinApp* pApp = AfxGetApp();

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
   CAutoRegistry autoReg(GetAppName());

   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   CWinApp* pApp = AfxGetApp();

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
   if (masterlib!=_T("Bogus") && type==srtLocal)
   {
      // our publisher and server is "Local Files"
      pApp->WriteProfileString(_T("Options"),_T("CatalogServer"),_T("Local Files"));
      pApp->WriteProfileString(_T("Options"),_T("Publisher"),    _T("Local Files"));

      // create that server
      CFileSystemPGSuperCatalogServer server(_T("Local Files"),masterlib,tempfolder,GetTemplateFileExtension());
      CString create_string = GetCreationString(&server);

      int count = pApp->GetProfileInt(_T("Servers"),_T("Count"),-1);
      count==-1 ? count=1 : count++;
      pApp->WriteProfileInt(_T("Servers"),_T("Count"),count);

      CString key(TCHAR(count-1+_T('A')));
      pApp->WriteProfileString(_T("Servers"), key, create_string);
   }

   // Delete profile strings if they are not empty
   if (masterlib!=_T("Bogus"))
   {
      pApp->WriteProfileString(_T("Options"),_T("MasterLibraryLocal"),NULL);
   }

   if (tempfolder!=_T("Bogus"))
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
      strPublisher.Format(_T("Default libraries installed with %s"),GetAppName());
      break;

   case srtInternetFtp:
   case srtInternetHttp:
      strPublisher = m_Publisher;
      break;

   case srtLocal:
      strPublisher = _T("Published on Local Network");
      break;

   default:
      ATLASSERT(false);
      break;
   }

   return strPublisher;
}

BOOL CPGSuperBaseAppPlugin::UpdateProgramSettings(BOOL bFirstRun) 
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CEAFApp* pApp = EAFGetApp();

   if (!bFirstRun && pApp->IsDocLoaded())
   {
      AfxMessageBox(_T("Program settings cannot be changed while a project is open. Close this project and try again."),MB_OK|MB_ICONINFORMATION);
   }
   else
   {
      CAutoRegistry autoReg(GetAppName());
      CConfigurePGSuperDlg dlg(GetAppName(),GetTemplateFileExtension(),bFirstRun,EAFGetMainFrame());

      dlg.m_Company  = m_CompanyName;
      dlg.m_Engineer = m_EngineerName;

      dlg.m_SharedResourceType   = m_SharedResourceType;
      dlg.m_CacheUpdateFrequency = m_CacheUpdateFrequency;
      dlg.m_CurrentServer        = m_CurrentCatalogServer;
      dlg.m_Publisher            = m_Publisher;
      dlg.m_Servers              = m_CatalogServers;

      // Save a copy of all server information in case our update fails
      SharedResourceType        original_type = m_SharedResourceType;
      CacheUpdateFrequency      original_freq = m_CacheUpdateFrequency;
      CString                 original_server = m_CurrentCatalogServer;
      CString              original_publisher = m_Publisher;
      CPGSuperCatalogServers original_servers = m_CatalogServers;

      INT_PTR result = dlg.DoModal();

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

      return (result == IDOK ? TRUE : FALSE);
   }

   return FALSE;
}

void CPGSuperBaseAppPlugin::UpdateCache()
{
   CAutoRegistry autoReg(GetAppName());
   CEAFApp* pApp = EAFGetApp();

   bool was_error=false;
   CString error_msg;

   try
   {
      BOOL bFirstRun = pApp->IsFirstRun();
      if (bFirstRun)
      {
         LOG(_T("Update Cache -> First Run"));

         // if this is the first time PGSuper is run after installation
         // go right to the program settings. OnProgramSettings will
         // force an update
         UpdateProgramSettings(TRUE); 
      }
      else if ( IsTimeToUpdateCache() )
      {
         if ( AreUpdatesPending() )
         {
            LOG(_T("Time to update cache and Updates are pending"));
            // this is not the first time, it is time to check for updates,
            // and sure enough there are updates pending.... do the update
            CString strMsg;
            strMsg.Format(_T("There are updates to Master Library and Workgroup Templates pending.\n\nWould you like to update %s now?"),AfxGetApp()->m_pszProfileName);
            int result = ::MessageBox(EAFGetMainFrame()->GetSafeHwnd(),strMsg,_T("Pending Updates"),MB_YESNO | MB_ICONINFORMATION);

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
      error_msg = _T("Error cause was unknown");
      was_error = true;
   }

   if (was_error)
   {
      // Things are totally screwed if we end up here. Reset to default library and templates from install
      RestoreLibraryAndTemplatesToDefault();
      CString msg;
      msg.Format(_T("The following error occurred while loading Library and Template server information:\n %s \nThese settings have been restored to factory defaults."),error_msg);
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

void CPGSuperBaseAppPlugin::DeleteCache(LPCTSTR pstrCache)
{
   RecursiveDelete(pstrCache);
   ::RemoveDirectory(pstrCache);
}

void CPGSuperBaseAppPlugin::RecursiveDelete(LPCTSTR pstr)
{
   CFileFind finder;

   CString strWildcard(pstr);
   strWildcard += _T("\\*.*");

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
   LOG(_T("IsTimeToUpdateCache()"));
   if ( m_SharedResourceType == srtDefault )
   {
      LOG(_T("Using default resource type, no updating"));
      return false;
   }

   // resources are local network or Internet
   bool bTimeToUpdate = false;
   if ( m_CacheUpdateFrequency == Never )
   {
      LOG(_T("Never update"));
      bTimeToUpdate = false;
   }
   else if ( m_CacheUpdateFrequency == Always )
   {
      LOG(_T("Always update"));
      bTimeToUpdate = true;
   }
   else 
   {
      sysDate now;
      sysDate last_update = GetLastCacheUpdateDate();

      Int16 update_frequency = 0; // in days
      if ( m_CacheUpdateFrequency == Daily )
      {
         LOG(_T("Update daily"));
         update_frequency = 1;
      }
      else if ( m_CacheUpdateFrequency == Weekly )
      {
         LOG(_T("Update weekly"));
         update_frequency = 7;
      }
      else if ( m_CacheUpdateFrequency == Monthly )
      {
         LOG(_T("Update monthly"));
         update_frequency = 30;
      }

      sysDate next_update = last_update;
      next_update += update_frequency;

      if ( next_update <= now )
      {
         bTimeToUpdate = true;
      }
   }

   LOG(_T("Time to update = ") << (bTimeToUpdate ? _T("Yes") : _T("No")));
   return bTimeToUpdate;
}

bool CPGSuperBaseAppPlugin::AreUpdatesPending()
{
   // get the MD5 files from the Internet or local network, compute the MD5 of the cache
   // if different, then there is an update pending

   LOG(_T("AreUpdatesPending()"));

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
      wndProgress->Show(CComBSTR(_T("Checking the Internet for Library updates")),pWnd->GetSafeHwnd());

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
            msg.Format(_T("Error - currently selected catalog server not found. Name was: %s"),m_CurrentCatalogServer);
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

   LOG(_T("Pending updates = ") << (bUpdatesPending ? _T("Yes") : _T("No")));
   return bUpdatesPending;
}


bool CPGSuperBaseAppPlugin::DoCacheUpdate()
{
   CEAFApp* pApp = EAFGetApp();

   LOG(_T("DoUpdateCache()"));

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
   int retval = _trename(strCache,strSaveCache); // rename the existing cache to "SaveCache" in case there is an update error
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
      m_WorkgroupTemplateFolderCache = GetCacheFolder() + GetTemplateSubFolderName() + CString(_T("\\"));

      // Catalog server takes care of business
      try
      {
         const CPGSuperCatalogServer* pserver = m_CatalogServers.GetServer(m_CurrentCatalogServer);
         bSuccessful = pserver->PopulateCatalog(m_Publisher,progress,GetCacheFolder());

         m_MasterLibraryFileURL = pserver->GetMasterLibraryURL(m_Publisher);

         if ( m_MasterLibraryFileURL == _T("") )
            m_MasterLibraryFileCache = _T("");
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
      _trename(strSaveCache,strCache);
   }

   if (bSuccessful)
      progress->put_Message(0,CComBSTR("The Master Library and Templates have been updated"));
   else
      progress->put_Message(0,CComBSTR("Update failed. Previous settings restored."));

   if ( bSuccessful )
   {
      // Need to update the main document template so that the File | New dialog is updated
      // Search for the CPGSuperDocTemplate object
      CEAFDocManager* pDocMgr = (CEAFDocManager*)(pApp->m_pDocManager);
      POSITION pos = pDocMgr->GetFirstDocTemplatePosition();
      while ( pos != NULL )
      {
         POSITION templatePos = pos;
         CDocTemplate* pDocTemplate = pDocMgr->GetNextDocTemplate(pos);
         if ( pDocTemplate->IsKindOf(RUNTIME_CLASS(CPGSuperDocTemplate)) )
         {
            pDocMgr->RemoveDocTemplate(templatePos);

            CPGSuperDocTemplate* pPGSuperDocTemplate = dynamic_cast<CPGSuperDocTemplate*>(pDocTemplate);
            pPGSuperDocTemplate->LoadTemplateInformation();

            pDocMgr->AddDocTemplate(pDocTemplate);
            break;
         }
      }
   }

   return bSuccessful;
}

sysDate CPGSuperBaseAppPlugin::GetLastCacheUpdateDate()
{
   CAutoRegistry autoReg(GetAppName());

   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   CWinApp* pApp = AfxGetApp();

   JulTy last_update = pApp->GetProfileInt(_T("Settings"),_T("LastCacheUpdate"),0);
   return sysDate(last_update);
}

void CPGSuperBaseAppPlugin::SetLastCacheUpdateDate(const sysDate& date)
{
   CAutoRegistry autoReg(GetAppName());

   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   CWinApp* pApp = AfxGetApp();

   pApp->WriteProfileInt(_T("Settings"),_T("LastCacheUpdate"),date.Hash());
}

CString CPGSuperBaseAppPlugin::GetDefaultMasterLibraryFile()
{
   CEAFApp* pApp = EAFGetApp();

   CString strAppPath = pApp->GetAppLocation();
   return strAppPath + CString(_T("WSDOT.lbr"));
}

CString CPGSuperBaseAppPlugin::GetDefaultWorkgroupTemplateFolder()
{
   CEAFApp* pApp = EAFGetApp();

   CString strAppPath = pApp->GetAppLocation();
   return strAppPath + CString(_T("Templates"));
}

CString CPGSuperBaseAppPlugin::GetCacheFolder()
{
   CAutoRegistry autoReg(GetAppName());

   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   CWinApp* pMyApp     = AfxGetApp();
   CEAFApp* pParentApp = EAFGetApp();

   TCHAR buffer[MAX_PATH];
   BOOL bResult = ::SHGetSpecialFolderPath(NULL,buffer,CSIDL_APPDATA,FALSE);

   if ( !bResult )
      return pParentApp->GetAppLocation() + CString(_T("Cache\\"));
   else
      return CString(buffer) + CString(_T("\\")) + CString(pMyApp->m_pszProfileName) + CString(_T("\\"));
}

CString CPGSuperBaseAppPlugin::GetSaveCacheFolder()
{
   CEAFApp* pApp = EAFGetApp();

   TCHAR buffer[MAX_PATH];
   BOOL bResult = ::SHGetSpecialFolderPath(NULL,buffer,CSIDL_APPDATA,FALSE);

   if ( !bResult )
      return pApp->GetAppLocation() + CString(_T("SaveCache\\"));
   else
      return CString(buffer) + CString(_T("\\PGSuper_Save\\"));
}

const CPGSuperCatalogServers* CPGSuperBaseAppPlugin::GetCatalogServers() const
{
   return &m_CatalogServers;
}

BOOL CPGSuperBaseAppPlugin::DoProcessCommandLineOptions(CEAFCommandLineInfo& cmdInfo)
{
   // cmdInfo is the command line information from the application. The application
   // doesn't know about this plug-in at the time the command line parameters are parsed
   //
   // Re-parse the parameters with our own command line information object
   CPGSuperCommandLineInfo pgsCmdInfo;
   EAFGetApp()->ParseCommandLine(pgsCmdInfo);
   cmdInfo = pgsCmdInfo;

   if (pgsCmdInfo.m_bDo1250Test)
   {
      Process1250Testing(pgsCmdInfo);
      return TRUE; // command line parameters handled
   }

   BOOL bHandled = FALSE;
   CEAFMainFrame* pFrame = EAFGetMainFrame();
   CEAFDocument* pDoc = pFrame->GetDocument();
   if ( pDoc )
   {
      bHandled = pDoc->ProcessCommandLineOptions(cmdInfo);
   }

   // If we get this far and there is one parameter and it isn't a file name and it isn't handled -OR-
   // if there is more than one parameter and it isn't handled there is something wrong
   if ( ((1 == pgsCmdInfo.m_Count && pgsCmdInfo.m_nShellCommand != CCommandLineInfo::FileOpen) || (1 <  pgsCmdInfo.m_Count)) && !bHandled )
   {
      cmdInfo.m_bError = TRUE;
      bHandled = TRUE;
   }

   return bHandled;
}

void CPGSuperBaseAppPlugin::Process1250Testing(const CPGSuperCommandLineInfo& rCmdInfo)
{
   USES_CONVERSION;
   ASSERT(rCmdInfo.m_bDo1250Test);

   // The document is opened when CEAFApp::InitInstance calls ProcessShellCommand
   // Get the document
   CEAFMainFrame* pFrame = EAFGetMainFrame();
   CEAFBrokerDocument* pDoc = (CEAFBrokerDocument*)pFrame->GetDocument();

   CComPtr<IBroker> pBroker;
   pDoc->GetBroker(&pBroker);
   GET_IFACE2( pBroker, ITest1250, ptst );

   CString resultsfile, poifile, errfile;
   if (create_test_file_names(rCmdInfo.m_strFileName,&resultsfile,&poifile,&errfile))
   {
      try
      {
         if (!ptst->RunTest(rCmdInfo.m_SubdomainId, std::_tstring(resultsfile), std::_tstring(poifile)))
         {
            CString msg = CString(_T("Error - Running test on file"))+rCmdInfo.m_strFileName;
            ::AfxMessageBox(msg);
         }

// Not sure why, but someone put this code in to save regression files.
// Sort of defeats the purpose of testing old files...
//
//         if ( pPgsDoc->IsModified() )
//            pPgsDoc->DoFileSave();
      }
      catch(const sysXBase& e)
      {
         std::_tstring msg;
         e.GetErrorMessage(&msg);
         std::_tofstream os;
         os.open(errfile);
         os <<_T("Error running test for input file: ")<<rCmdInfo.m_strFileName<<std::endl<< msg;
      }
      catch(CException* pex)
      {
         TCHAR   szCause[255];
         CString strFormatted;
         pex->GetErrorMessage(szCause, 255);
         std::_tofstream os;
         os.open(errfile);
         os <<_T("Error running test for input file: ")<<rCmdInfo.m_strFileName<<std::endl<< szCause;
         delete pex;
      }
      catch(CException& ex)
      {
         TCHAR   szCause[255];
         CString strFormatted;
         ex.GetErrorMessage(szCause, 255);
         std::_tofstream os;
         os.open(errfile);
         os <<_T("Error running test for input file: ")<<rCmdInfo.m_strFileName<<std::endl<< szCause;
      }
      catch(const std::exception* pex)
      {
         std::_tstring strMsg(CA2T(pex->what()));
         std::_tofstream os;
         os.open(errfile);
         os <<_T("Error running test for input file: ")<<rCmdInfo.m_strFileName<<std::endl<<strMsg<< std::endl;
         delete pex;
      }
      catch(const std::exception& ex)
      {
         std::_tstring strMsg(CA2T(ex.what()));
         std::_tofstream os;
         os.open(errfile);
         os <<_T("Error running test for input file: ")<<rCmdInfo.m_strFileName<<std::endl<<strMsg<< std::endl;
      }
      catch(...)
      {
         std::_tofstream os;
         os.open(errfile);
         os <<_T("Unknown Error running test for input file: ")<<rCmdInfo.m_strFileName;
      }
   }
}
