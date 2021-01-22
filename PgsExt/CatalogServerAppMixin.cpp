///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2020  Washington State Department of Transportation
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
#include "PgsExt\PgsExtLib.h"

#include "PgsExt\CatalogServerAppMixin.h"
#include "PgsExt\CatalogServers.h"

#include "ConfigurePGSuperDlg.h"
#include <MFCTools\AutoRegistry.h>
#include <EAF\EAFApp.h>
#include <EAF\EAFDocManager.h>

#include <BridgeLink.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CCatalogServerAppMixin::CCatalogServerAppMixin(void)
{
   m_CacheUpdateFrequency = Daily;
   m_SharedResourceType = srtInternetFtp;
}

CCatalogServerAppMixin::~CCatalogServerAppMixin(void)
{
}

HRESULT CCatalogServerAppMixin::OnFinalConstruct()
{
   m_CatalogServers.SetAppName(GetAppName());

   return S_OK;
}

void CCatalogServerAppMixin::OnFinalRelease()
{
}

void CCatalogServerAppMixin::DefaultInit(IEAFAppPlugin* pAppPlugin)
{
   m_CatalogServers.SetTemplateFileExtenstion(GetTemplateFileExtension());

   if ( UseConfigurationCallback() )
   {
      CBridgeLinkApp* pApp = (CBridgeLinkApp*)EAFGetApp();
      IBridgeLink* pBL = (IBridgeLink*)pApp;
      m_CallbackID = pBL->Register(this);
   }
}

void CCatalogServerAppMixin::DefaultTerminate()
{
   if ( UseConfigurationCallback() )
   {
      CBridgeLinkApp* pApp = (CBridgeLinkApp*)EAFGetApp();
      IBridgeLink* pBL = (IBridgeLink*)pApp;
      pBL->UnregisterCallback(m_CallbackID);
   }
}

void CCatalogServerAppMixin::LoadRegistryValues()
{
   // Do any necessary conversions from previous versions of PGSuper
   LoadRegistrySettings();
   LoadRegistryOptions();
   LoadRegistryCatalogServers();
}

void CCatalogServerAppMixin::LoadRegistrySettings()
{
   CEAFApp* pApp = EAFGetApp();

   CAutoRegistry autoReg(GetAppName(),pApp);

   // The default values are read from HKEY_LOCAL_MACHINE\Software\Washington State Deparment of Transportation\PGSuper
   // If the default values are missing, the hard coded defaults found herein are used.
   // Install writers can create MSI transforms to alter the "defaults" by changing the registry values

   int iDefaultSharedResourceType     = pApp->GetLocalMachineInt(   _T("Settings"),_T("SharedResourceType"),    1);
   int iDefaultCacheUpdateFrequency   = pApp->GetLocalMachineInt(   _T("Settings"),_T("CacheUpdateFrequency"),  2);

   /////////////////////////////////
   // Shared Resource Settings
   /////////////////////////////////

   m_SharedResourceType   = (SharedResourceType)  pApp->GetProfileInt(_T("Settings"),_T("SharedResourceType"),  iDefaultSharedResourceType);
   m_CacheUpdateFrequency = (CacheUpdateFrequency)pApp->GetProfileInt(_T("Settings"),_T("CacheUpdateFrequency"),iDefaultCacheUpdateFrequency);
}

void CCatalogServerAppMixin::LoadRegistryOptions()
{
   CEAFApp* pApp = EAFGetApp();

   CString appName( GetAppName() );
   CAutoRegistry autoReg(appName,pApp);

   CString strDefaultUserTemplateFolder     = pApp->GetLocalMachineString(_T("Options"),_T("UserTemplateLocation"), _T("C:\\"));
   CString strDefaultCatalogServer          = pApp->GetLocalMachineString(_T("Options"),GetCatalogServerKey(),GetDefaultCatalogServerName());
   CString strDefaultPublisher              = pApp->GetLocalMachineString(_T("Options"),GetPublisherKey(),GetDefaultCatalogName());
   CString strDefaultLocalMasterLibraryFile = pApp->GetLocalMachineString(_T("Options"),_T("MasterLibraryLocal"),     GetDefaultMasterLibraryFile());
   CString strLocalWorkgroupTemplateFolder  = pApp->GetLocalMachineString(_T("Options"),_T("WorkgroupTemplatesLocal"),GetDefaultWorkgroupTemplateFolder());

   // Internet resources
   m_CurrentCatalogServer = pApp->GetProfileString(_T("Options"),GetCatalogServerKey(),strDefaultCatalogServer);

   m_Publisher = pApp->GetProfileString(_T("Options"),GetPublisherKey(),strDefaultPublisher);

   // defaults
   CString strVersion = CCatalog::GetAppVersion();

   CString strFTPServer(_T("ftp://ftp.wsdot.wa.gov/public/Bridge/Software"));
   //CString strFTPServer(_T("http://www.wsdot.wa.gov/eesc/bridge/software"));
   CString strDefaultMasterLibraryURL;
   strDefaultMasterLibraryURL.Format(_T("%s/Version_%s/WSDOT.lbr"),strFTPServer,strVersion);
   CString strDefaultWorkgroupTemplateFolderURL;
   strDefaultWorkgroupTemplateFolderURL.Format(_T("%s/Version_%s/WSDOT_Templates/"),strFTPServer,strVersion);

   m_MasterLibraryFileURL = pApp->GetProfileString(_T("Options"),GetMasterLibraryURLKey(),strDefaultMasterLibraryURL);

   // Cache file/folder for Internet or Local Network resources
   CString strMasterLibFile;
   strMasterLibFile.Format(_T("%s%s"),GetCacheFolder(),GetMasterLibraryFileName());
   m_MasterLibraryFileCache       = pApp->GetProfileString(_T("Options"),GetMasterLibraryCacheKey(),     strMasterLibFile);

   CString strWorkgroupTemplates;
   strWorkgroupTemplates.Format(_T("%s%s\\"),GetCacheFolder(),GetTemplateSubFolderName());
   m_WorkgroupTemplateFolderCache = pApp->GetProfileString(_T("Options"),GetWorkgroupTemplatesCacheKey(),strWorkgroupTemplates);
}

void CCatalogServerAppMixin::LoadRegistryCatalogServers()
{
   CEAFApp* pApp = EAFGetApp();
   CString appName( GetAppName() );
   CAutoRegistry autoReg(appName,pApp);
   m_CatalogServers.LoadFromRegistry(pApp);
}

void CCatalogServerAppMixin::SaveRegistryValues()
{
   SaveRegistrySettings();
   SaveRegistryOptions();
   SaveRegistryCatalogServers();
}

void CCatalogServerAppMixin::SaveRegistrySettings()
{
   CEAFApp* pApp = EAFGetApp();

   CAutoRegistry autoReg(GetAppName(),pApp);

   pApp->WriteProfileInt(_T("Settings"),_T("SharedResourceType"),m_SharedResourceType);
   pApp->WriteProfileInt(_T("Settings"),_T("CacheUpdateFrequency"),m_CacheUpdateFrequency);
}

void CCatalogServerAppMixin::SaveRegistryOptions()
{
   // Options settings
   CEAFApp* pApp = EAFGetApp();

   // Internet resources
   {
      CString appName(GetAppName());
      CAutoRegistry autoReg(appName, pApp);

      pApp->WriteProfileString(_T("Options"),GetCatalogServerKey(),m_CurrentCatalogServer);
      pApp->WriteProfileString(_T("Options"),GetPublisherKey(),m_Publisher);
      pApp->WriteProfileString(_T("Options"),GetMasterLibraryURLKey(),m_MasterLibraryFileURL);

      // Cache file/folder for Internet or Local Network resources
      pApp->WriteProfileString(_T("Options"),GetMasterLibraryCacheKey(),     m_MasterLibraryFileCache);
      pApp->WriteProfileString(_T("Options"),GetWorkgroupTemplatesCacheKey(),m_WorkgroupTemplateFolderCache);
   }
}

void CCatalogServerAppMixin::SaveRegistryCatalogServers()
{
   // Options settings
   CEAFApp* pApp = EAFGetApp();

   {
      CString appName(GetAppName());
      CAutoRegistry autoReg(appName, pApp);
      m_CatalogServers.SaveToRegistry(pApp);
   }
}

const CString& CCatalogServerAppMixin::GetCachedMasterLibraryFile() const
{
   return m_MasterLibraryFileCache;
}

CString CCatalogServerAppMixin::GetMasterLibraryFile() const
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

void CCatalogServerAppMixin::GetTemplateFolders(CString& strWorkgroupFolder) const
{
   strWorkgroupFolder = m_WorkgroupTemplateFolderCache;
}

void CCatalogServerAppMixin::SetCacheUpdateFrequency(CacheUpdateFrequency frequency)
{
   m_CacheUpdateFrequency = frequency;
}

CacheUpdateFrequency CCatalogServerAppMixin::GetCacheUpdateFrequency() const
{
   return m_CacheUpdateFrequency;
}

void CCatalogServerAppMixin::SetSharedResourceType(SharedResourceType resType)
{
   m_SharedResourceType = resType;
}

SharedResourceType CCatalogServerAppMixin::GetSharedResourceType() const
{
   return m_SharedResourceType;
}

CString CCatalogServerAppMixin::GetMasterLibraryPublisher() const
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

   case srtLocalIni:
      strPublisher = _T("Ini File Published on Local Network");
      break;

   default:
      ATLASSERT(false);
      break;
   }

   return strPublisher;
}

const CString& CCatalogServerAppMixin::GetConfigurationName() const
{
   return m_CurrentCatalogServer;
}

class CConfigureDlg : public CPropertySheet
{
public:
   CConfigureDlg();
   virtual BOOL OnInitDialog();
};

CConfigureDlg::CConfigureDlg() : CPropertySheet(_T("Configure"))
{
   SetWizardMode();
}

BOOL CConfigureDlg::OnInitDialog()
{
   SetWizardButtons(PSWIZB_FINISH);
   return CPropertySheet::OnInitDialog();
}

bool CCatalogServerAppMixin::UpdateProgramSettings()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   CConfigureDlg dlg;
   std::unique_ptr<CPropertyPage> pPage(CreatePropertyPage());
   dlg.AddPage(pPage.get());
   if (dlg.DoModal() == ID_WIZFINISH)
   {
      OnOK(pPage.get());
      return true; // configuration updated
   }
   return false; // configuration not updated
}

CPropertyPage* CCatalogServerAppMixin::CreatePropertyPage()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   CConfigurePGSuperDlg* pPage = new CConfigurePGSuperDlg(GetAppName(),GetTemplateFileExtension(),false);

   pPage->m_SharedResourceType   = m_SharedResourceType;
   pPage->m_CacheUpdateFrequency = m_CacheUpdateFrequency;
   pPage->m_CurrentServer        = m_CurrentCatalogServer;
   pPage->m_Publisher            = m_Publisher;
   pPage->m_Servers              = m_CatalogServers;
   return pPage;
}

void CCatalogServerAppMixin::OnOK(CPropertyPage* pPage)
{
   CConfigurePGSuperDlg* pDlg = (CConfigurePGSuperDlg*)pPage;
   m_SharedResourceType   = pDlg->m_SharedResourceType;
   m_CacheUpdateFrequency = pDlg->m_CacheUpdateFrequency;

   m_CatalogServers       = pDlg->m_Servers; 
   m_CurrentCatalogServer = pDlg->m_CurrentServer;
   m_Publisher            = pDlg->m_Publisher;

   if ( m_SharedResourceType == srtDefault )
   {
      // Using hard coded defaults
      RestoreLibraryAndTemplatesToDefault();
   }

   SharedResourceType   original_type = m_SharedResourceType;
   CacheUpdateFrequency original_freq = m_CacheUpdateFrequency;
   CString              original_server = m_CurrentCatalogServer;
   CString              original_publisher = m_Publisher;
   CCatalogServers      original_servers = m_CatalogServers;

   if (!DoCacheUpdate())
   {  
      // DoCacheUpdate will restore the cache, we need also to restore local data
      m_SharedResourceType   = original_type;
      m_CacheUpdateFrequency = original_freq;
      m_CurrentCatalogServer = original_server;
      m_Publisher            = original_publisher;
      m_CatalogServers       = original_servers;
   }
   SaveRegistryValues(); // Saves all the current settings to the registery
                         // There is no sense waiting until PGSuper closes to do this
}

void CCatalogServerAppMixin::UpdateCache()
{
   CEAFApp* pApp = EAFGetApp();

   bool was_error=false;
   CString error_msg;

   try
   {
      if ( IsTimeToUpdateCache() )
      {
         if ( AreUpdatesPending() )
         {
//            LOG(_T("Time to update cache and Updates are pending"));
            // this is not the first time, it is time to check for updates,
            // and sure enough there are updates pending.... do the update
            CString strMsg;
            strMsg.Format(_T("There are updates pending updates to your %s configuration.\n\nWould you like to update now?"),GetAppName());
            int result = ::MessageBox(EAFGetMainFrame()->GetSafeHwnd(),strMsg,_T("Pending Updates"),MB_YESNO | MB_ICONINFORMATION);

            if ( result == IDYES )
            {
               was_error = !DoCacheUpdate();
            }
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
      msg.Format(_T("The following error occurred while accessing the configuration server:\n %s \nThese settings have been restored to defaults."),error_msg);
      ::AfxMessageBox(msg,MB_ICONINFORMATION);
   }
}

void CCatalogServerAppMixin::RestoreLibraryAndTemplatesToDefault()
{
   m_SharedResourceType = srtDefault;

   m_MasterLibraryFileCache       = GetDefaultMasterLibraryFile();
   m_WorkgroupTemplateFolderCache = GetDefaultWorkgroupTemplateFolder();

   m_MasterLibraryFileURL = m_MasterLibraryFileCache;

   DoCacheUpdate();
}

void CCatalogServerAppMixin::DeleteCache(LPCTSTR pstrCache)
{
   RecursiveDelete(pstrCache);
   ::RemoveDirectory(pstrCache);
}

void CCatalogServerAppMixin::RecursiveDelete(LPCTSTR pstr)
{
   CFileFind finder;

   CString strWildcard(pstr);
   strWildcard += _T("\\*.*");

   BOOL bWorking = finder.FindFile(strWildcard);
   while ( bWorking )
   {
      bWorking = finder.FindNextFile();

      if ( finder.IsDots() )
      {
         continue;
      }

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

bool CCatalogServerAppMixin::IsTimeToUpdateCache() const
{
//   LOG(_T("IsTimeToUpdateCache()"));
   if ( m_SharedResourceType == srtDefault )
   {
//      LOG(_T("Using default resource type, no updating"));
      return false;
   }

   // resources are local network or Internet
   bool bTimeToUpdate = false;
   if ( m_CacheUpdateFrequency == Never )
   {
//      LOG(_T("Never update"));
      bTimeToUpdate = false;
   }
   else if ( m_CacheUpdateFrequency == Always )
   {
//      LOG(_T("Always update"));
      bTimeToUpdate = true;
   }
   else 
   {
      sysDate now;
      sysDate last_update = GetLastCacheUpdateDate();

      Int16 update_frequency = 0; // in days
      if ( m_CacheUpdateFrequency == Daily )
      {
//         LOG(_T("Update daily"));
         update_frequency = 1;
      }
      else if ( m_CacheUpdateFrequency == Weekly )
      {
//         LOG(_T("Update weekly"));
         update_frequency = 7;
      }
      else if ( m_CacheUpdateFrequency == Monthly )
      {
//         LOG(_T("Update monthly"));
         update_frequency = 30;
      }

      sysDate next_update = last_update;
      next_update += update_frequency;

      if ( next_update <= now )
      {
         bTimeToUpdate = true;
      }
   }

//   LOG(_T("Time to update = ") << (bTimeToUpdate ? _T("Yes") : _T("No")));
   return bTimeToUpdate;
}

bool CCatalogServerAppMixin::AreUpdatesPending() const
{
   // get the MD5 files from the Internet or local network, compute the MD5 of the cache
   // if different, then there is an update pending

//   LOG(_T("AreUpdatesPending()"));

   bool bUpdatesPending = false;
   if ( m_SharedResourceType == srtDefault )
   {
      bUpdatesPending = false;
   }
   else if ( m_SharedResourceType == srtInternetFtp  ||
             m_SharedResourceType == srtInternetHttp ||
             m_SharedResourceType == srtLocal        ||
             m_SharedResourceType == srtLocalIni)
   {

      // create a progress window
      CComPtr<IProgressMonitorWindow> wndProgress;
      wndProgress.CoCreateInstance(CLSID_ProgressMonitorWindow);
      wndProgress->put_HasGauge(VARIANT_FALSE);
      wndProgress->put_HasCancel(VARIANT_FALSE);
      CEAFMainFrame* pWnd = EAFGetMainFrame();
      wndProgress->Show(CComBSTR(_T("Checking for configuration updates")),pWnd->GetSafeHwnd());

      try
      {
         // Catalog server does the work here
         const CCatalogServer* pserver = m_CatalogServers.GetServer(m_CurrentCatalogServer);
         if (pserver!=nullptr)
         {
            bUpdatesPending = pserver->CheckForUpdates(m_Publisher, nullptr, GetCacheFolder());
         }
         else
         {
            CString msg;
            msg.Format(_T("Error: the configuration server, %s, could not be found."),m_CurrentCatalogServer);
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
      ATLASSERT(false);
   }

//   LOG(_T("Pending updates = ") << (bUpdatesPending ? _T("Yes") : _T("No")));
   return bUpdatesPending;
}


bool CCatalogServerAppMixin::DoCacheUpdate()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   CEAFApp* pApp = EAFGetApp();

//   LOG(_T("DoUpdateCache()"));

   // create a progress window
   CComPtr<IProgressMonitorWindow> wndProgress;
   wndProgress.CoCreateInstance(CLSID_ProgressMonitorWindow);
   wndProgress->put_HasGauge(VARIANT_FALSE);
   wndProgress->put_HasCancel(VARIANT_FALSE);

   CComQIPtr<IProgressMonitor> progress(wndProgress);
   CEAFMainFrame* pWnd = EAFGetMainFrame();
   wndProgress->Show(CComBSTR("Updating Configuration"),pWnd->GetSafeHwnd());

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
      bSuccessful = true;
   }
   else if ( m_SharedResourceType == srtInternetFtp ||
             m_SharedResourceType == srtInternetHttp ||
             m_SharedResourceType == srtLocal ||
             m_SharedResourceType == srtLocalIni)
   {
      // set cache folder
      m_MasterLibraryFileCache = GetCacheFolder() + GetMasterLibraryFileName();
      m_WorkgroupTemplateFolderCache = GetCacheFolder() + GetTemplateSubFolderName() + CString(_T("\\"));

      // Catalog server takes care of business
      try
      {
         const CCatalogServer* pserver = m_CatalogServers.GetServer(m_CurrentCatalogServer);
         bSuccessful = pserver->PopulateCatalog(m_Publisher,progress,GetCacheFolder());

         m_MasterLibraryFileURL = pserver->GetMasterLibraryURL(m_Publisher);

         if ( m_MasterLibraryFileURL == _T("") )
         {
            m_MasterLibraryFileCache = _T("");
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
         ATLASSERT(false);
         bSuccessful = false;
      }
   }
   else
   {
      ATLASSERT(false);
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
   {
      progress->put_Message(0,CComBSTR("The configuration have been updated"));
   }
   else
   {
      progress->put_Message(0,CComBSTR("Update failed. Previous settings restored."));
   }

   if ( bSuccessful )
   {
      // Need to update the main document template so that the File | New dialog is updated
      UpdateDocTemplates();
   }

   return bSuccessful;
}

sysDate CCatalogServerAppMixin::GetLastCacheUpdateDate() const
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   CEAFApp* pApp = EAFGetApp();
   CAutoRegistry autoReg(GetAppName(),pApp);

   JulTy last_update = pApp->GetProfileInt(_T("Settings"),_T("LastCacheUpdate"),0);
   return sysDate(last_update);
}

void CCatalogServerAppMixin::SetLastCacheUpdateDate(const sysDate& date)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   CEAFApp* pApp = EAFGetApp();
   CAutoRegistry autoReg(GetAppName(),pApp);

   pApp->WriteProfileInt(_T("Settings"),_T("LastCacheUpdate"),date.Hash());
}

CString CCatalogServerAppMixin::GetDefaultMasterLibraryFile() const
{
   CEAFApp* pApp = EAFGetApp();

   CString strAppPath = pApp->GetAppLocation();
   strAppPath.MakeUpper();

#if defined _DEBUG
#if defined _WIN64
   strAppPath.Replace(_T("REGFREECOM\\X64\\DEBUG\\"),_T(""));
#else
   strAppPath.Replace(_T("REGFREECOM\\WIN32\\DEBUG\\"),_T(""));
#endif
#else
   // in a real release, the path doesn't contain RegFreeCOM\\Release, but that's
   // ok... the replace will fail and the string wont be altered.
#if defined _WIN64
   strAppPath.Replace(_T("REGFREECOM\\X64\\RELEASE\\"),_T(""));
#else
   strAppPath.Replace(_T("REGFREECOM\\WIN32\\RELEASE\\"),_T(""));
#endif
#endif

   return strAppPath + CString(_T("Configurations\\WSDOT.lbr"));
}

CString CCatalogServerAppMixin::GetDefaultWorkgroupTemplateFolder() const
{
   CEAFApp* pApp = EAFGetApp();

   CString strAppPath = pApp->GetAppLocation();
   strAppPath.MakeUpper();

#if defined _DEBUG
#if defined _WIN64
   strAppPath.Replace(_T("REGFREECOM\\X64\\DEBUG\\"),_T(""));
#else
   strAppPath.Replace(_T("REGFREECOM\\WIN32\\DEBUG\\"),_T(""));
#endif
#else
   // in a real release, the path doesn't contain RegFreeCOM\\Release, but that's
   // ok... the replace will fail and the string wont be altered.
#if defined _WIN64
   strAppPath.Replace(_T("REGFREECOM\\X64\\RELEASE\\"),_T(""));
#else
   strAppPath.Replace(_T("REGFREECOM\\WIN32\\RELEASE\\"),_T(""));
#endif
#endif

   return strAppPath + CString(_T("Configurations\\")) + GetAppName();
}

CString CCatalogServerAppMixin::GetCacheFolder() const
{
   CEAFApp* pParentApp = EAFGetApp();

   LPWSTR path;
   HRESULT hr = ::SHGetKnownFolderPath(FOLDERID_RoamingAppData, KF_FLAG_DEFAULT, NULL, &path);

   if ( SUCCEEDED(hr) )
   {
      return CString(path) + CString(_T("\\")) + GetAppName() + CString(_T("\\"));
   }
   else
   {
      return pParentApp->GetAppLocation() + CString(_T("Cache\\"));
   }
}

CString CCatalogServerAppMixin::GetSaveCacheFolder() const
{
   CEAFApp* pApp = EAFGetApp();

   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   CWinApp* pMyApp     = AfxGetApp();

   LPWSTR path;
   HRESULT hr = ::SHGetKnownFolderPath(FOLDERID_RoamingAppData, KF_FLAG_DEFAULT, NULL, &path);

   if ( SUCCEEDED(hr) )
   {
      return CString(path) + GetAppName() + CString(_T("_Save\\"));
   }
   else
   {
      return pApp->GetAppLocation() + CString(_T("SaveCache\\"));
   }
}

const CCatalogServers* CCatalogServerAppMixin::GetCatalogServers() const
{
   return &m_CatalogServers;
}

void CCatalogServerAppMixin::ProcessLibrarySetUp(const CPGSBaseCommandLineInfo& rCmdInfo)
{
   ASSERT(rCmdInfo.m_bSetUpdateLibrary);

   // Set library to parsed names and attempt an update
   SharedResourceType        original_type = m_SharedResourceType;
   CacheUpdateFrequency      original_freq = m_CacheUpdateFrequency;
   CString                 original_server = m_CurrentCatalogServer;
   CString              original_publisher = m_Publisher;

   // find our server
   const CCatalogServer* pserver = GetCatalogServers()->GetServer(rCmdInfo.m_CatalogServerName);
   if (pserver != nullptr)
   {
      m_SharedResourceType   = pserver->GetServerType();

      m_CurrentCatalogServer = rCmdInfo.m_CatalogServerName;
      m_Publisher            = rCmdInfo.m_PublisherName ;

      if (!this->DoCacheUpdate())
      {
         // DoCacheUpdate will restore the cache, we need also to restore local data
         m_SharedResourceType   = original_type;
         m_CurrentCatalogServer = original_server;
         m_Publisher            = original_publisher;
      }
   }
   else
   {
      CString msg;
      msg.Format(_T("Error - The configuration server \"%s\" was not found. Could not update configuration."), rCmdInfo.m_CatalogServerName);
      AfxMessageBox(msg);
   }
}

LPCTSTR CCatalogServerAppMixin::GetCatalogServerKey() const
{
   return _T("CatalogServer");
}

LPCTSTR CCatalogServerAppMixin::GetPublisherKey() const
{
   return _T("Publisher");
}

LPCTSTR CCatalogServerAppMixin::GetMasterLibraryCacheKey() const
{
   return _T("MasterLibraryCache");
}

LPCTSTR CCatalogServerAppMixin::GetMasterLibraryURLKey() const
{
   return _T("MasterLibraryURL");
}

LPCTSTR CCatalogServerAppMixin::GetWorkgroupTemplatesCacheKey() const
{
   return _T("WorkgroupTemplatesCache");
}
