///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2026  Washington State Department of Transportation
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
#include <PgsExt\PgsExtLib.h>
#include <PgsExt\CatalogServer.h>
#include "TemplateManager.h"
#include "..\ConfigPackager\UnzipPgz.h"
#include "..\ConfigPackager\ConfigurationManifest.h"
#include <EAF\EAFApp.h>

CString GetWSDOTConfigurationServerAddress()
{
   return CString(_T("https://wsdot.wa.gov/eesc/bridge/software/configurations/"));
}

bool IsLegacyWSDOTAddress(const CString& address)
{
   CString strAddress(address);
   strAddress.MakeLower();
   strAddress.Replace(_T('\\'), _T('/'));

   // the FTP site, and the per-program folders on the WSDOT web site that were never used
   return strAddress.Find(_T("ftp.wsdot.wa.gov")) != -1 ||
          (strAddress.Find(_T("wsdot.wa.gov/eesc/bridge/software/")) != -1 && strAddress.Find(_T("/software/configurations")) == -1);
}

static CCatalogServer* CreateServer(LPCTSTR strAppName, const CString& strType, const CString& strName, const CString& strAddress, const CString& strExt)
{
   if (strType == _T("FTP") || strType == _T("HTTP"))
   {
      // FTP servers are no longer supported. The HTTP server tries the same host and path over HTTPS
      // and saves the new address if that works.
      return new CHttpCatalogServer(strAppName, strName, strAddress, strExt);
   }
   else if (strType == _T("FILSYS"))
   {
      int p = strAddress.Find(_T('|'));
      if (p != -1)
      {
         CString strLibraryFileName = strAddress.Left(p);
         CString strTemplateFolder = strAddress.Mid(p + 1);
         return new CFileSystemCatalogServer(strAppName, strName, strLibraryFileName, strTemplateFolder, strExt);
      }
      else
      {
         ATLASSERT(false);
         return nullptr;
      }
   }
   else if (strType == _T("FILSYSINI"))
   {
      return new CFileSystemIniCatalogServer(strAppName, strName, strAddress, strExt);
   }
   else
   {
      ATLASSERT(false);
      return nullptr;
   }
}

// Functions to save and create our servers to/from a string
CCatalogServer* CreateCatalogServer(LPCTSTR strAppName,const CString& createString,const CString& strExt)
{
   int p = createString.Find(_T('|'));
   if ( p != -1 )
   {
      CString strType = createString.Left(p);
      CString strNext = createString.Mid(p+1);
      p = strNext.Find(_T('|'));
      if ( p != -1 )
      {
         CString strName = strNext.Left(p);
         CString strAddress = strNext.Mid(p+1);
         return CreateServer(strAppName, strType, strName, strAddress, strExt);
      }
      else
      {
         ATLASSERT(false);
         return nullptr;
      }
   }
   else
   {
      // old style string - equal or previous to pgsuper 2.2.3
      // only supported ftp at that time
      p = createString.Find(_T('!'));
      if ( p != -1 )
      {
         CString strName = createString.Left(p);
         CString strAddress = createString.Mid(p+1);
         return CreateServer(strAppName, _T("FTP"), strName, strAddress, strExt);
      }
      else
      {
         ATLASSERT(false);
         return nullptr;
      }
   }
}

CCatalogServer* CreateCatalogServer(LPCTSTR strAppName,const CString& strServerName,const CString& createString,const CString& strExt)
{
   int p = createString.Find(_T('|'));
   if ( p != -1 )
   {
      CString strType(createString.Left(p));
      CString strAddress(createString.Mid(p+1));
      return CreateServer(strAppName, strType, strServerName, strAddress, strExt);
   }
   else
   {
      // old style string - equal or previous to pgsuper 2.2.3
      // only supported ftp at that time
      p = createString.Find(_T('!'));
      if ( p != -1 )
      {
         CString strAddress = createString.Left(p);
         return CreateServer(strAppName, _T("FTP"), strServerName, strAddress, strExt);
      }
      else
      {
         ATLASSERT(false);
         return nullptr;
      }
   }
}

CString GetCreationString(const CCatalogServer* pServer)
{
   CString creastr;
   SharedResourceType type = pServer->GetServerType();
   if (type==srtInternetHttp || type==srtInternetFtp)
   {
      const CHttpCatalogServer* psvr = dynamic_cast<const CHttpCatalogServer*>(pServer);
      ATLASSERT(psvr);
      // An address that is still ftp:// keeps the FTP tag so it is clear in the registry
      CString strAddress(psvr->GetAddress());
      creastr.Format(_T("%s|%s"), ConfigManifest::StartsWithNoCase((LPCTSTR)strAddress, L"ftp://") ? _T("FTP") : _T("HTTP"), strAddress);
   }
   else if (type==srtLocal)
   {
      const CFileSystemCatalogServer* psvr = dynamic_cast<const CFileSystemCatalogServer*>(pServer);
      ATLASSERT(psvr);
      creastr.Format(_T("FILSYS|%s|%s"), psvr->GetLibraryFileName(), psvr->GetTemplateFolderPath());
   }
   else if (type==srtLocalIni)
   {
      const CFileSystemIniCatalogServer* psvr = dynamic_cast<const CFileSystemIniCatalogServer*>(pServer);
      ATLASSERT(psvr);
      creastr.Format(_T("FILSYSINI|%s"), psvr->GetAddress());
   }
   else
   {
      ATLASSERT(0);
   }

   return creastr;
}

static CString GetTempFolder()
{
   TCHAR buffer[MAX_PATH + 1];
   ::GetTempPath(MAX_PATH + 1, buffer);
   return CString(buffer);
}

static CString GetPgzFilename(LPCTSTR strAppName)
{
   // The package is always saved in the cache under this name, whatever its published extension (.zip or .pgz)
   CString strPgzFilename;
   strPgzFilename.Format(_T("%s.pgz"),strAppName);
   return strPgzFilename;
}

static bool FileExists(const CString& strFile)
{
   DWORD attributes = ::GetFileAttributes(strFile);
   return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

static CString CleanHTTPURL(const CString& strURL, bool isFile)
{
   CString url(strURL);
   url.Trim();

   // web addresses use forward slashes
   url.Replace(_T("\\"),_T("/"));

   // if no scheme was typed, use https://
   if (url.Find(_T("://")) == -1 && !url.IsEmpty())
   {
      url.Insert(0,_T("https://"));
   }

   // folders end with / so file names can be added. This also matters because some servers
   // redirect a folder address without the / from https:// down to http://
   if (!isFile && !url.IsEmpty() && url.Right(1) != _T("/") )
   {
      url += _T("/");
   }

   return url;
}

static bool IsJsonAddress(const CString& address)
{
   return ConfigManifest::EndsWithNoCase((LPCTSTR)address, L".json");
}

BOOL PeekAndPump()
{
	static MSG msg;

	while (::PeekMessage(&msg,nullptr,0,0,PM_NOREMOVE))
   {
		if (!EAFGetApp()->PumpMessage())
      {
			::PostQuitMessage(0);
			return FALSE;
		}
	}

	return TRUE;
}

static void ReportUnzipError(uzErrorType st, const CString& pgzCachedFile)
{
   CString msg;
   if (st==uzErrorOpeningFile)
   {
      msg.Format(_T("Error opening the file %s."),pgzCachedFile);
   }
   else if (st==uzErrorGettingFileList || st==uzExtractingFile)
   {
      msg.Format(_T("Error extracting files from %s."),pgzCachedFile);
   }
   else if (st==uzNoFilesFound)
   {
      msg.Format(_T("Error - no files found in %s."),pgzCachedFile);
   }

   ::AfxMessageBox(msg,MB_ICONEXCLAMATION | MB_OK);
}

/////    CCatalogServer  ///////////

CCatalogServer::CCatalogServer(LPCTSTR strAppName,LPCTSTR name,SharedResourceType type,const CString& strExt):
m_AppName(strAppName),
m_Name(name),
m_ServerType(type),
m_TemplateFileExt(strExt)
{
}

CCatalogServer::~CCatalogServer()
{
}

CString CCatalogServer::GetServerName() const
{
   return m_Name;
}

SharedResourceType CCatalogServer::GetServerType() const
{
   return m_ServerType;
}

CString CCatalogServer::GetTemplateFileExtension() const
{
   return m_TemplateFileExt;
}

/// CManifestCatalogServer ///
CManifestCatalogServer::CManifestCatalogServer(LPCTSTR strAppName,LPCTSTR name,SharedResourceType type, const CString& address,const CString& strExt):
CCatalogServer(strAppName,name,type,strExt)
{
   SetAddress(address);
   Init();
}

void CManifestCatalogServer::Init()
{
   m_bFakeError = false;
   m_bDoFetchCatalog = true;
}

CString CManifestCatalogServer::GetAddress() const
{
   return m_ServerAddress;
}

void CManifestCatalogServer::SetAddress(const CString& address)
{
   m_ServerAddress = address;
}

std::vector<CString> CManifestCatalogServer::GetDownloadCandidates(const CString& strURL) const
{
   return std::vector<CString>{strURL};
}

void CManifestCatalogServer::OnCatalogFound(const CString& strAddress) const
{
}

bool CManifestCatalogServer::IsLegacyFtp() const
{
   return false;
}

bool CManifestCatalogServer::LocateCatalog(const CString& strLocalFile, CString& strFoundAddress, CString& strCatalogURL, bool& bIsJson, CString& strError, bool& bConnectionError) const
{
   strError.Empty();
   bConnectionError = false;

   CString strLastError;
   std::vector<CString> addresses = GetCandidateAddresses();
   for (const auto& address : addresses)
   {
      // ConfigurationPackages.json first, then the legacy INI file
      std::vector<CString> files;
      if (IsJsonAddress(address))
      {
         files.push_back(address);
      }
      else
      {
         files.push_back(address + ConfigManifest::GetManifestFileName());
         files.push_back(address + ConfigManifest::GetLegacyIniFileName((LPCTSTR)m_AppName).c_str());
      }

      bool bServerAnswered = false;
      for (const auto& file : files)
      {
         CString strFileError;
         gwResult result = GetWebFile(file, strLocalFile, strFileError);
         if (result == gwOk)
         {
            strFoundAddress = address;
            strCatalogURL = file;
            bIsJson = IsJsonAddress(file);
            return true;
         }
         else if (result == gwInvalidUrl)
         {
            strError.Format(_T("The configuration server address is not valid: %s"), file);
            return false;
         }
         else if (result == gwNotFound)
         {
            bServerAnswered = true;
         }
         else
         {
            ATLASSERT(result == gwConnectionError);
            strLastError = strFileError;
            break; // the server can't be reached at this address, try the next one
         }
      }

      if (bServerAnswered)
      {
         // The server was reached but doesn't publish configurations at this address.
         // Don't fall back to another address (such as http://) when the server answered.
         strError.Format(_T("The configuration server %s doesn't have a list of configurations (%s) at %s."), GetServerName(), ConfigManifest::GetManifestFileName(), address);
         return false;
      }
   }

   bConnectionError = true;
   strError.Format(_T("The configuration server %s could not be reached at %s."), GetServerName(), m_ServerAddress);
   if (!strLastError.IsEmpty())
   {
      strError += _T("\n\n") + strLastError;
   }
   return false;
}

void CManifestCatalogServer::FetchCatalog(IProgressMonitor* pProgress) const
{
   if (m_bDoFetchCatalog)
   {
      m_strLocalCatalog = GetTempFolder() + m_AppName + _T("_ConfigurationPackages.tmp");

      if (pProgress!=nullptr)
      {
         CComBSTR str(CString(_T("Reading the list of configurations from ")) + m_ServerAddress);
         pProgress->put_Message(0,str);
      }

      CString strFoundAddress, strCatalogURL, strError;
      bool bIsJson, bConnectionError;
      if (!LocateCatalog(m_strLocalCatalog, strFoundAddress, strCatalogURL, bIsJson, strError, bConnectionError))
      {
         if (IsLegacyFtp())
         {
            CString msg;
            msg.Format(_T("The configuration server \"%s\" uses FTP, which %s no longer supports.\n\n%s will keep using the configuration it downloaded earlier. Ask the publisher of this configuration for an HTTPS address, then update the server with Configure %s > Manage."),
               GetServerName(), m_AppName, m_AppName, m_AppName);
            throw CCatalogServerException(CCatalogServerException::ceFtpNotSupported, msg);
         }

         throw CCatalogServerException(CCatalogServerException::ceGettingCatalogFile, strError);
      }

      m_CatalogURL = strCatalogURL;
      OnCatalogFound(strFoundAddress);

      CString strVersion = CCatalog::GetAppVersion();
      if (!m_Catalog.Init(m_strLocalCatalog, m_AppName, strVersion, bIsJson, strError))
      {
         CString msg;
         msg.Format(_T("The list of configurations at %s could not be read.\n\n%s\n\nPlease contact the server owner."), strCatalogURL, strError);
         throw CCatalogServerException(CCatalogServerException::ceGettingCatalogFile, msg);
      }

      m_bDoFetchCatalog = false;
   }
}

CManifestCatalogServer::gwResult CManifestCatalogServer::DownloadFile(const CString& strURL, const CString& strLocalFile, CString& strError) const
{
   gwResult result = gwInvalidUrl;
   for (const auto& url : GetDownloadCandidates(strURL))
   {
      result = GetWebFile(url, strLocalFile, strError);
      if (result == gwOk)
      {
         break;
      }
   }
   return result;
}

std::vector<CString> CManifestCatalogServer::GetPublishers() const
{
   FetchCatalog(nullptr);
   return m_Catalog.GetPublishers();
}

bool CManifestCatalogServer::DoesPublisherExist(const CString& publisher) const
{
   return m_Catalog.DoesPublisherExist(publisher);
}

CString CManifestCatalogServer::GetMasterLibraryURL(const CString& publisher) const
{
   FetchCatalog(nullptr);
   return ResolvePackageURL(m_Catalog.GetPackageURL(publisher));
}

CString CManifestCatalogServer::GetWebLink(const CString& publisher) const
{
   FetchCatalog(nullptr);
   return m_Catalog.GetWebLink(publisher);
}

bool CManifestCatalogServer::IsPackageDifferent(const CString& publisher, const CString& strCachedPackage) const
{
   if (!FileExists(strCachedPackage))
   {
      return true;
   }

   CString strMd5 = m_Catalog.GetPackageMd5(publisher);
   if (!strMd5.IsEmpty())
   {
      // the checksum is published in the catalog
      std::wstring cachedMd5 = ConfigManifest::ComputeFileMd5((LPCTSTR)strCachedPackage);
      return strMd5.CompareNoCase(cachedMd5.c_str()) != 0;
   }

   // Older servers publish the checksum in a file next to the package
   CString strMd5URL = ResolvePackageURL(m_Catalog.GetPackageURL(publisher)) + _T(".md5");
   CString strTempMd5File = GetTempFolder() + m_AppName + _T(".pgz.md5");

   CString strError;
   gwResult result = DownloadFile(strMd5URL, strTempMd5File, strError);
   if (result == gwInvalidUrl)
   {
      CString strMessage;
      strMessage.Format(_T("The checksum file location, %s, for the %s configuration is not valid. Please contact the server owner."),strMd5URL,publisher);
      throw CCatalogServerException(CCatalogServerException::ceParsingURL, strMessage);
   }
   else if (result != gwOk)
   {
      CString msg;
      msg.Format(_T("The checksum for the %s configuration was not found on the server (%s). Please contact the server owner."),publisher,strMd5URL);
      throw CCatalogServerException(CCatalogServerException::ceFindingFile, msg);
   }

   std::vector<std::wstring> hashes;
   ConfigManifest::ReadMd5List((LPCTSTR)strTempMd5File, hashes);
   return !ConfigManifest::IsInMd5List((LPCTSTR)strCachedPackage, hashes);
}

bool CManifestCatalogServer::CheckForUpdates(const CString& publisher, IProgressMonitor* pProgress,
                                               const CString& cacheFolder) const
{
   FetchCatalog(pProgress);

   if (!m_Catalog.DoesPublisherExist(publisher))
   {
      CString msg;
      msg.Format(_T("The configuration %s is no longer published on the configuration server %s, or is not available for this version of %s."), publisher, GetServerName(), m_AppName);
      throw CCatalogServerException(CCatalogServerException::ceGettingCatalogFile, msg);
   }

   if(pProgress!=nullptr)
   {
      pProgress->put_Message(0,CComBSTR("Checking for configuration updates"));
   }

   CString cachedPgzFile = cacheFolder + GetPgzFilename(m_AppName);
   return IsPackageDifferent(publisher, cachedPgzFile);
}

bool CManifestCatalogServer::PopulateCatalog(const CString& publisher, IProgressMonitor* pProgress,
                                               const CString& cacheFolder) const
{
   FetchCatalog(pProgress);

   if (!m_Catalog.DoesPublisherExist(publisher))
   {
      CString msg;
      msg.Format(_T("The configuration %s is not published on the configuration server %s for this version of %s."), publisher, GetServerName(), m_AppName);
      throw CCatalogServerException(CCatalogServerException::ceGettingCatalogFile, msg);
   }

   CString strPgzFile = ResolvePackageURL(m_Catalog.GetPackageURL(publisher));

   CString pgzFolder(cacheFolder);
   CString pgzCachedFile = cacheFolder + GetPgzFilename(m_AppName);

   if(pProgress!=nullptr)
   {
      pProgress->put_Message(0,CComBSTR("Downloading Configuration"));
   }

   CString strError;
   if ( DownloadFile(strPgzFile, pgzCachedFile, strError) == gwOk )
   {
      if(pProgress!=nullptr)
      {
         pProgress->put_Message(0,CComBSTR("Configuration downloaded successfully"));
      }
   }
   else
   {
      CString msg;
      msg.Format(_T("Error downloading the Configuration %s"),strPgzFile);
      if (!strError.IsEmpty())
      {
         msg += _T("\n\n") + strError;
      }
      throw CCatalogServerException(CCatalogServerException::ceFindingFile, msg);
   }

   // We now should have our file in the cache.
   // Use md5 to check that download was successful
   if( IsPackageDifferent(publisher, pgzCachedFile) )
   {
      CString strMessage;
      strMessage.Format(_T("It appears that there was an error downloading the file \"%s\"  The md5 file hash does not match that on the server. Try updating again, and contact the server owner if this error persists"),strPgzFile);

      ::MessageBox(EAFGetMainFrame()->GetSafeHwnd(),strMessage,_T("Error"), MB_OK );
      return false;
   }

   // Now that we have a validated file - decompress it
   if(pProgress!=nullptr)
   {
      pProgress->put_Message(0,CComBSTR("Uncompressing Configuration to local cache."));
   }

   uzErrorType uzst = UnZipPGZ( pgzCachedFile.GetBuffer(), pgzFolder.GetBuffer() );
   pgzFolder.ReleaseBuffer();
   pgzCachedFile.ReleaseBuffer();

   if(uzst!=uzOk)
   {
      ReportUnzipError(uzst, pgzCachedFile);
      return false;
   }

   return true;
}

bool CManifestCatalogServer::IsNetworkError() const
{
   if (m_bFakeError)
   {
      return true;
   }
   else
   {
      CString msg;
      return !TestServer(msg);
   }
}

void CManifestCatalogServer::FakeNetworkError(bool bFake) const
{
   m_bFakeError = bFake;
}

bool CManifestCatalogServer::TestServer(CString& errorMessage) const
{
   // if we can get and read the catalog - good enough
   // copy to a harmless location
   CString strLocalFile = GetTempFolder() + m_AppName + _T("_ConfigurationPackagesTest.tmp");

   CString strFoundAddress, strCatalogURL;
   bool bIsJson, bConnectionError;
   if (!LocateCatalog(strLocalFile, strFoundAddress, strCatalogURL, bIsJson, errorMessage, bConnectionError))
   {
      return false;
   }

   CCatalog catalog;
   CString strError;
   if (!catalog.Init(strLocalFile, m_AppName, CCatalog::GetAppVersion(), bIsJson, strError))
   {
      errorMessage.Format(_T("The list of configurations at %s could not be read.\n\n%s"), strCatalogURL, strError);
      return false;
   }

   return true;
}

/// CHttpCatalogServer ///
CHttpCatalogServer::CHttpCatalogServer(LPCTSTR strAppName, const CString& strExt) :
   CManifestCatalogServer(strAppName, _T("WSDOT"), srtInternetHttp, GetWSDOTConfigurationServerAddress(), strExt)
{
}

CHttpCatalogServer::CHttpCatalogServer(LPCTSTR strAppName,LPCTSTR strServerName, const CString& address,const CString& strExt):
   CManifestCatalogServer(strAppName, strServerName, srtInternetHttp, address, strExt)
{
}

bool CHttpCatalogServer::IsLegacyFtp() const
{
   return ConfigManifest::StartsWithNoCase((LPCTSTR)m_ServerAddress, L"ftp://");
}

std::vector<CString> CHttpCatalogServer::GetCandidateAddresses() const
{
   CString strAddress = CleanUpURL(m_ServerAddress, IsJsonAddress(m_ServerAddress));

   std::vector<CString> addresses;
   if (ConfigManifest::StartsWithNoCase((LPCTSTR)strAddress, L"ftp://"))
   {
      // FTP is no longer supported. Try the same host and path over HTTPS.
      addresses.push_back(_T("https://") + strAddress.Mid(6));
   }
   else if (ConfigManifest::StartsWithNoCase((LPCTSTR)strAddress, L"http://"))
   {
      // Try HTTPS first. Use HTTP only if the server can't be reached over HTTPS.
      addresses.push_back(_T("https://") + strAddress.Mid(7));
      addresses.push_back(strAddress);
   }
   else
   {
      addresses.push_back(strAddress);
   }

   return addresses;
}

std::vector<CString> CHttpCatalogServer::GetDownloadCandidates(const CString& strURL) const
{
   std::vector<CString> urls;
   if (ConfigManifest::StartsWithNoCase((LPCTSTR)strURL, L"http://"))
   {
      // only legacy INI files can have http:// package addresses. Try HTTPS first.
      urls.push_back(_T("https://") + strURL.Mid(7));
   }
   urls.push_back(strURL);
   return urls;
}

CString CHttpCatalogServer::ResolvePackageURL(const CString& strURL) const
{
   if (ConfigManifest::IsAbsoluteLocation((LPCTSTR)strURL))
   {
      return strURL;
   }

   CString strRelative(strURL);
   strRelative.Replace(_T('\\'), _T('/'));

   if (strRelative.Left(1) == _T("/"))
   {
      // relative to the host
      int start = m_CatalogURL.Find(_T("://"));
      int end = m_CatalogURL.Find(_T('/'), start + 3);
      return (end == -1 ? m_CatalogURL : m_CatalogURL.Left(end)) + strRelative;
   }

   // relative to the folder that holds the catalog
   int end = m_CatalogURL.ReverseFind(_T('/'));
   return m_CatalogURL.Left(end + 1) + strRelative;
}

void CHttpCatalogServer::OnCatalogFound(const CString& strAddress) const
{
   // Save the upgraded address (http:// or ftp:// to https://) so it is used from now on
   if (ConfigManifest::StartsWithNoCase((LPCTSTR)strAddress, L"https://") && !ConfigManifest::StartsWithNoCase((LPCTSTR)m_ServerAddress, L"https://"))
   {
      m_ServerAddress = strAddress;
   }
}

// Custom deleter for files opened with CInternetSession::OpenURL
struct CInternetFileDeleter {
   void operator()(CStdioFile* pFile) const {
      if (pFile != nullptr) {
         pFile->Close();
         delete pFile;
      }
   }
};

CManifestCatalogServer::gwResult CHttpCatalogServer::GetWebFile(const CString& strFileURL, const CString& strLocalTargetFile, CString& strError) const
{
   strError.Empty();

   // see if url parses before going further
   DWORD dwServiceType;
   CString strServer, strObject;
   INTERNET_PORT nPort;

   BOOL bSuccess = AfxParseURL(strFileURL,dwServiceType,strServer,strObject,nPort);
   if ( !bSuccess || (dwServiceType != AFX_INET_SERVICE_HTTP && dwServiceType != AFX_INET_SERVICE_HTTPS))
   {
      return gwInvalidUrl;
   }

   CInternetSession session(m_AppName);

   try
   {
      // Always get a fresh copy. Allow redirects from http:// to https://.
      DWORD dwFlags = INTERNET_FLAG_TRANSFER_BINARY | INTERNET_FLAG_RELOAD | INTERNET_FLAG_DONT_CACHE | INTERNET_FLAG_PRAGMA_NOCACHE | INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTPS;
      std::unique_ptr<CStdioFile, CInternetFileDeleter> pFile(session.OpenURL(strFileURL, 1, dwFlags));
      CHttpFile* pHttpFile = dynamic_cast<CHttpFile*>(pFile.get());
      if (pHttpFile == nullptr)
      {
         strError.Format(_T("Could not open %s"), strFileURL);
         return gwConnectionError;
      }

      DWORD dwStatus = 0;
      pHttpFile->QueryInfoStatusCode(dwStatus);
      if (dwStatus != HTTP_STATUS_OK)
      {
         // The server answered, but it doesn't have the file (or won't send it)
         strError.Format(_T("The server answered %d for %s"), dwStatus, strFileURL);
         return gwNotFound;
      }

      // Never accept a secure request that was redirected to an insecure address
      if (dwServiceType == AFX_INET_SERVICE_HTTPS)
      {
         TCHAR finalURL[INTERNET_MAX_URL_LENGTH + 1];
         DWORD dwLength = sizeof(finalURL);
         if (pHttpFile->QueryOption(INTERNET_OPTION_URL, finalURL, &dwLength) && ConfigManifest::StartsWithNoCase(finalURL, L"http://"))
         {
            strError.Format(_T("The server redirected %s to an insecure address (%s)"), strFileURL, finalURL);
            return gwConnectionError;
         }
      }

      // Copy file contents to local file
      CFile localFile(strLocalTargetFile, CFile::modeCreate | CFile::modeWrite | CFile::typeBinary);
      BYTE buffer[16384];
      UINT nRead;
      while ((nRead = pHttpFile->Read(buffer, sizeof(buffer))) > 0)
      {
         localFile.Write(buffer, nRead);
         PeekAndPump();
      }
      localFile.Close();
   }
   catch (CInternetException* pEx)
   {
      TCHAR szErr[512];
      pEx->GetErrorMessage(szErr, 512);
      strError.Format(_T("%s (error %d)"), szErr, pEx->m_dwError);
      strError.Trim();
      pEx->Delete();
      return gwConnectionError;
   }
   catch (CFileException* pEx)
   {
      TCHAR szErr[512];
      pEx->GetErrorMessage(szErr, 512);
      strError = szErr;
      pEx->Delete();
      return gwConnectionError;
   }

   return gwOk;
}

CString CHttpCatalogServer::CleanUpURL(const CString& strURL, bool isFile) const
{
   if (ConfigManifest::StartsWithNoCase((LPCTSTR)strURL, L"ftp://"))
   {
      CString url(strURL);
      url.Replace(_T("\\"), _T("/"));
      if (!isFile && url.Right(1) != _T("/"))
      {
         url += _T("/");
      }
      return url;
   }

   return CleanHTTPURL(strURL, isFile);
}

/// CFileSystemIniCatalogServer ///
CString CFileSystemIniCatalogServer::CleanUpURL(const CString& strURL, bool isFile) const
{
   CString newstr(strURL);
   newstr.Trim();

   if (!isFile && !newstr.IsEmpty())
   {
      // add trailing \ if needed
      TCHAR last = newstr.GetAt(newstr.GetLength() - 1);
      if ( last != _T('\\') && last != _T('/') )
      {
         newstr += _T("\\");
      }
   }

   return newstr;
}

std::vector<CString> CFileSystemIniCatalogServer::GetCandidateAddresses() const
{
   return std::vector<CString>{CleanUpURL(m_ServerAddress, IsJsonAddress(m_ServerAddress))};
}

CString CFileSystemIniCatalogServer::ResolvePackageURL(const CString& strURL) const
{
   if (ConfigManifest::IsAbsoluteLocation((LPCTSTR)strURL))
   {
      return strURL;
   }

   CString strRelative(strURL);
   strRelative.Replace(_T('/'), _T('\\'));
   strRelative.TrimLeft(_T('\\'));

   int end = max(m_CatalogURL.ReverseFind(_T('\\')), m_CatalogURL.ReverseFind(_T('/')));
   return m_CatalogURL.Left(end + 1) + strRelative;
}

CManifestCatalogServer::gwResult CFileSystemIniCatalogServer::GetWebFile(const CString& strFileURL, const CString& strLocalTargetFile, CString& strError)const
{
   strError.Empty();

   if ( strFileURL.IsEmpty() )
   {
      return gwInvalidUrl;
   }

   if (!FileExists(strFileURL))
   {
      // The folder exists but the file doesn't: the "server" answered. Otherwise the folder can't be reached.
      CString strFolder(strFileURL);
      int end = max(strFolder.ReverseFind(_T('\\')), strFolder.ReverseFind(_T('/')));
      strFolder = strFolder.Left(end);
      DWORD attributes = ::GetFileAttributes(strFolder);
      if (attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY))
      {
         return gwNotFound;
      }

      strError.Format(_T("The folder %s could not be found."), strFolder);
      return gwConnectionError;
   }

   if (!::CopyFile(strFileURL,strLocalTargetFile,FALSE))
   {
      strError.Format(_T("Could not copy %s."), strFileURL);
      return gwConnectionError;
   }

   return gwOk;
}

/// CFileSystemCatalogServer ///
CFileSystemCatalogServer::CFileSystemCatalogServer(LPCTSTR strAppName,LPCTSTR name, const CString& libraryFileName, const CString& templateFilePath,const CString& strExt):
CCatalogServer(strAppName,name,srtLocal,strExt),
m_LibraryFileName(libraryFileName),
m_TemplateFilePath(templateFilePath),
m_bFakeError(false)
{
}

CString CFileSystemCatalogServer::GetLibraryFileName() const
{
   return m_LibraryFileName;
}

CString CFileSystemCatalogServer::GetTemplateFolderPath() const
{
   return m_TemplateFilePath;
}

std::vector<CString> CFileSystemCatalogServer::GetPublishers() const
{
   // file systems only have one publisher - just return name
   std::vector<CString> vec;
   vec.push_back(this->GetServerName());
   return vec;
}

bool CFileSystemCatalogServer::DoesPublisherExist(const CString& publisher) const
{
   CString name = GetServerName();
   return publisher == name;
}

CString CFileSystemCatalogServer::GetMasterLibraryURL(const CString& publisher) const
{
   return GetLibraryFileName();
}

CString CFileSystemCatalogServer::GetWebLink(const CString& publisher) const
{
   // file system has now web links
   return CString();
}

bool CFileSystemCatalogServer::CheckForUpdates(const CString& publisher, IProgressMonitor* pProgress, 
                                               const CString& cacheFolder) const
{
   // The publisher can put checksum lists next to the library file (MasterLibrary.lbr.md5 style, named <library>.md5)
   // and in the template folder (WorkgroupTemplates.md5). Each has one "hash  path" line per file.
   // If the lists are missing, assume that an update is pending.
   CString cachedMasterLibFile  = cacheFolder + CString(_T("\\")) + GetMasterLibraryFileName();
   CString cachedTemplateFolder = cacheFolder + CString(_T("\\")) + GetTemplateSubFolderName()+ CString(_T("\\"));

   CString strTemplateFolder = GetTemplateFolderPath();
   if (!strTemplateFolder.IsEmpty() && strTemplateFolder.Right(1) != _T("\\") && strTemplateFolder.Right(1) != _T("/"))
   {
      strTemplateFolder += _T("\\");
   }

   std::vector<std::wstring> libraryHashes, templateHashes;
   bool bHaveLibraryList = GetLibraryFileName().IsEmpty() || ConfigManifest::ReadMd5List((LPCTSTR)(GetLibraryFileName() + _T(".md5")), libraryHashes);
   bool bHaveTemplateList = ConfigManifest::ReadMd5List((LPCTSTR)(strTemplateFolder + _T("WorkgroupTemplates.md5")), templateHashes);

   if ( !bHaveLibraryList || !bHaveTemplateList )
   {
      return true;
   }

   bool bLibraryCurrent = GetLibraryFileName().IsEmpty() || ConfigManifest::IsInMd5List((LPCTSTR)cachedMasterLibFile, libraryHashes);
   bool bTemplatesCurrent = ConfigManifest::IsInMd5List((LPCTSTR)cachedTemplateFolder, templateHashes);

   return !(bLibraryCurrent && bTemplatesCurrent);
}

bool CFileSystemCatalogServer::PopulateCatalog(const CString& publisher, IProgressMonitor* pProgress,
                                                      const CString& cacheFolder) const
{

   CString cachedMasterLibFile  = cacheFolder + CString(_T("\\")) + GetMasterLibraryFileName();
   CString cachedTemplateFolder = cacheFolder + CString(_T("\\")) + GetTemplateSubFolderName()+ CString(_T("\\"));

   // copy the network file to the cache
   CString localMasterLibraryFile = GetLibraryFileName();

   if ( localMasterLibraryFile != _T("") )
   {
      if(pProgress != nullptr)
      {
         pProgress->put_Message(0,CComBSTR("Copying the Master Library"));
      }

      BOOL bSuccess = ::CopyFile(localMasterLibraryFile,cachedMasterLibFile,FALSE);
      if ( !bSuccess )
      {
         if(pProgress!=nullptr)
         {
            pProgress->put_Message(0,CComBSTR("Error copying the Master Library"));
         }

         CString strMessage;
         strMessage.Format(_T("Error opening Master library file from %s"),localMasterLibraryFile);

         AfxMessageBox(strMessage,MB_ICONEXCLAMATION | MB_OK);

         return false;
      }
      else
      {
         if(pProgress!=nullptr)
         {
            pProgress->put_Message(0,CComBSTR("Master Library copied successfully"));
         }
      }
   }

   // Next deal with templates
   BOOL bst = ::CreateDirectory(cachedTemplateFolder,nullptr);
   if ( bst==0 )
   {
      CString strMessage;
      strMessage.Format(_T("Error creating Template Folder at %s"),cachedTemplateFolder);

      if(pProgress!=nullptr)
      {
         pProgress->put_Message(0,CComBSTR(strMessage));
      }

      AfxMessageBox(strMessage,MB_ICONEXCLAMATION | MB_OK);
      return false;
   }

   CString localTemplateFolder = GetTemplateFolderPath();
   CFileTemplateManager templateMgr(GetTemplateFileExtension());
   templateMgr.GetTemplates(localTemplateFolder,cachedTemplateFolder,pProgress);

   return true;
}

bool CFileSystemCatalogServer::IsNetworkError() const
{
   CString msg;
   return !TestServer(msg);
}

void CFileSystemCatalogServer::FakeNetworkError(bool bFake) const
{
   m_bFakeError = bFake;
}

bool CFileSystemCatalogServer::TestServer(CString& errorMessage) const
{
   if (m_bFakeError)
   {
      return false;
   }

   CFileFind libfinder;
   CString strLibraryFileName = GetLibraryFileName();
   if ( strLibraryFileName != _T("") )
   {
      BOOL bFound = libfinder.FindFile(GetLibraryFileName());
      if (!bFound)
      {
         errorMessage.Format(_T("Error finding master library file at: %s"),GetLibraryFileName());
         return false;
      }
   }

   // strip trailing back slash if present
   CString templpath = GetTemplateFolderPath();
   templpath.TrimRight(_T("\\"));

   CFileFind tempfinder;
   BOOL bFound = tempfinder.FindFile(templpath);
   if (!bFound)
   {
      errorMessage.Format(_T("Error finding template folder at: %s"),GetTemplateFolderPath());
      return false;
   }

   return true;
}

CString CFileSystemCatalogServer::CleanUpURL(const CString& strURL, bool isFile) const
{
   // file dialog should be doing the job for us
   return strURL;
}
