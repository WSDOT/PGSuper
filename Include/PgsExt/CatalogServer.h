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

#if !defined INCLUDED_CATALOGSERVER_H_
#define INCLUDED_CATALOGSERVER_H_

#include "Catalog.h"
#include <WBFLTools.h>

enum CacheUpdateFrequency
{
   Never,
   Always,
   Daily,
   Weekly,
   Monthly
};

// Enum to describe server connection, which describes server type
// These values are saved in the registry. Don't renumber them.
enum SharedResourceType
{
   srtDefault,     // use the default files provided by PGSuper
   srtInternetFtp, // FTP servers are no longer supported. This value is read from older settings and treated as srtInternetHttp.
   srtLocal,       // use library file and workgroup templates published on a local network
   srtInternetHttp,// configurations published on an HTTPS web server (ConfigurationPackages.json)
   srtLocalIni     // configurations published in a local or network folder (ConfigurationPackages.json)
};

// Functions to return names of master library and template subfolder in cache
inline CString GetMasterLibraryFileName()
{
   return CString("MasterLibrary.lbr");
}

inline CString GetTemplateSubFolderName()
{
   return CString("WorkgroupTemplates");
}

// Address of the WSDOT configuration server
CString GetWSDOTConfigurationServerAddress();

// True if the address is an old WSDOT configuration server location (FTP, or the old HTTP folders)
bool IsLegacyWSDOTAddress(const CString& address);

class CCatalogServer;
class CApp;

// Factory functions for creating and saving servers
CCatalogServer* CreateCatalogServer(LPCTSTR strAppName,const CString& createString,const CString& strExt);
CCatalogServer* CreateCatalogServer(LPCTSTR strAppName,const CString& strServerName,const CString& createString,const CString& strExt);
CString GetCreationString(const CCatalogServer* pServer);

// Exception class for catalog server errors
class CCatalogServerException
{
public:
   enum ErrorType
   {
      ceNetworkConnection,
      ceGettingCatalogFile,
      ceParsingURL,
      ceFtpNotSupported, // the server is an FTP server that couldn't be reached over HTTPS
      ceFindingFile,
      ceDownloadingFile,
      ceServerNotFound
   };

   CCatalogServerException(ErrorType error, const CString& msg = CString()):
   m_ErrorType(error),
   m_ErrorMessage(msg)
   {
   }

   ErrorType GetErrorType()
   {
      return m_ErrorType;
   }

   CString GetErrorMessage()
   {
      return m_ErrorMessage;
   }

   void SetErrorMessage(const CString& error)
   {
      m_ErrorMessage = error;
   }

private:
   ErrorType m_ErrorType;
   CString m_ErrorMessage;

   CCatalogServerException();
};

// Semi-abstract Base class for all catalog servers
class CCatalogServer
{
public:
   CCatalogServer(LPCTSTR strAppName,LPCTSTR strServerName,SharedResourceType type,const CString& strExt);
   virtual ~CCatalogServer();

   CString GetServerName() const;
   SharedResourceType GetServerType() const;

   virtual CString GetTemplateFileExtension() const; // returns the file extension for template files

   // ** Any below can throw a CCatalogServerException **
   //////////////////////////////////////////////////////
   // The catalogs associated with this server
   virtual std::vector<CString> GetPublishers() const=0;

   virtual bool DoesPublisherExist(const CString& publisher) const=0;

   // path to master library - for reporting
   virtual CString GetMasterLibraryURL(const CString& publisher) const=0;

   // URL of web site associated with publisher
   virtual CString GetWebLink(const CString& publisher) const=0;

   // Check to see if updates are needed from server.
   virtual bool CheckForUpdates(const CString& publisher, IProgressMonitor* pProgress,
                                const CString& cacheFolder) const=0;

   // Copy files from server to catalog cache
   virtual bool PopulateCatalog(const CString& publisher, IProgressMonitor* pProgress,
                                const CString& cacheFolder) const=0;

   // ** These do not throw **
   //////////////////////////////////////////////////////
   virtual bool TestServer(CString& errorMessage) const=0;
   virtual bool IsNetworkError() const=0;
   virtual void FakeNetworkError(bool bFake) const=0; // used for testing... causes IsNetworkError to always return true

   // Try to clean up mistyped urls
   virtual CString CleanUpURL(const CString& strURL, bool isFile) const = 0;

protected:
   CString m_AppName;

private:
   CCatalogServer();

   CString m_Name;
   SharedResourceType m_ServerType;
   CString m_TemplateFileExt;
};

//////////////////////  CManifestCatalogServer    ////////////////////////////////
// Abstract server that publishes ConfigurationPackages.json and configuration packages.
// When a server has no ConfigurationPackages.json, the legacy <App>Packages.ini is read instead.

class CManifestCatalogServer : public CCatalogServer
{
public:
   CManifestCatalogServer(LPCTSTR strAppName,LPCTSTR strServerName,SharedResourceType type, const CString& address,const CString& strExt);

   // The address as stored in the settings. It changes from http:// to https:// once HTTPS is known to work.
   CString GetAddress() const;

   // virtual's
   virtual std::vector<CString> GetPublishers() const override;
   virtual bool DoesPublisherExist(const CString& publisher) const override;
   virtual CString GetMasterLibraryURL(const CString& publisher) const override;
   virtual CString GetWebLink(const CString& publisher) const override;
   virtual bool CheckForUpdates(const CString& publisher, IProgressMonitor* pProgress,
                                const CString& cacheFolder) const override;
   virtual bool PopulateCatalog(const CString& publisher, IProgressMonitor* pProgress,
                                const CString& cacheFolder) const override;
   virtual bool TestServer(CString& errorMessage) const override;
   virtual bool IsNetworkError() const override;
   virtual void FakeNetworkError(bool bFake) const override; // used for testing... causes IsNetworkError to always return true

   enum gwResult
   {
      gwOk,
      gwInvalidUrl,      // the address can't be used
      gwConnectionError, // the server couldn't be reached
      gwNotFound         // the server was reached, but it doesn't have the file
   };

protected:
   // Copies a file from the server to the local file system
   virtual gwResult GetWebFile(const CString& strFileURL, const CString& strLocalTargetFile, CString& strError) const = 0;

   // Addresses to try, in order, when looking for the catalog. Each is a folder (ending with a separator) or a .json file.
   virtual std::vector<CString> GetCandidateAddresses() const = 0;

   // Addresses to try, in order, when downloading a file whose full address is strURL
   virtual std::vector<CString> GetDownloadCandidates(const CString& strURL) const;

   // Turns a package location from the catalog into a full address
   virtual CString ResolvePackageURL(const CString& strURL) const = 0;

   // Called when the catalog was found at one of the candidate addresses
   virtual void OnCatalogFound(const CString& strAddress) const;

   // True if the server was defined as an FTP server
   virtual bool IsLegacyFtp() const;

protected:
   void Init();
   void SetAddress(const CString& address);
   void FetchCatalog(IProgressMonitor* pProgress) const;

   // Finds and downloads the catalog. Returns false and an explanation if it can't be found.
   bool LocateCatalog(const CString& strLocalFile, CString& strFoundAddress, CString& strCatalogURL, bool& bIsJson, CString& strError, bool& bConnectionError) const;

   // Downloads a file, trying each of GetDownloadCandidates
   gwResult DownloadFile(const CString& strURL, const CString& strLocalFile, CString& strError) const;

   // True if the cached package differs from the published package
   bool IsPackageDifferent(const CString& publisher, const CString& strCachedPackage) const;

   mutable CString m_ServerAddress;
   mutable CString m_CatalogURL; // where the catalog was found

   mutable CCatalog m_Catalog; // catalog parser
   mutable CString m_strLocalCatalog; // full path to local catalog file in temp folder
   mutable bool m_bFakeError;
   mutable bool m_bDoFetchCatalog;
};


//////////////////////  CHttpCatalogServer    ////////////////////////////////
// Configurations published on a web server. New servers must use HTTPS.
// Servers stored with an http:// address are tried over HTTPS first. Servers stored with an
// ftp:// address (FTP is no longer supported) are tried over HTTPS on the same host and path.
class CHttpCatalogServer : public CManifestCatalogServer
{
public:
   // The WSDOT server
   CHttpCatalogServer(LPCTSTR strAppName, const CString& strExt);
   CHttpCatalogServer(LPCTSTR strAppName,LPCTSTR strServerName, const CString& address,const CString& strExt);

   virtual CString CleanUpURL(const CString& strURL, bool isFile) const override;

protected:
   virtual gwResult GetWebFile(const CString& strFileURL, const CString& strLocalTargetFile, CString& strError) const override;
   virtual std::vector<CString> GetCandidateAddresses() const override;
   virtual std::vector<CString> GetDownloadCandidates(const CString& strURL) const override;
   virtual CString ResolvePackageURL(const CString& strURL) const override;
   virtual void OnCatalogFound(const CString& strAddress) const override;
   virtual bool IsLegacyFtp() const override;
};

//////////////////////  CFileSystemIniCatalogServer    ////////////////////////////////
// Configurations published in a local or network folder that has ConfigurationPackages.json
// (or a legacy <App>Packages.ini). The class name is kept so stored settings still load.
class CFileSystemIniCatalogServer : public CManifestCatalogServer
{
public:
   CFileSystemIniCatalogServer(LPCTSTR strAppName,LPCTSTR strServerName, const CString& address,const CString& strExt):
   CManifestCatalogServer(strAppName, strServerName, srtLocalIni, address, strExt)
   {
      ;
   }

   virtual CString CleanUpURL(const CString& strURL, bool isFile) const override;

protected:
   virtual gwResult GetWebFile(const CString& strFileURL, const CString& strLocalTargetFile, CString& strError) const override;
   virtual std::vector<CString> GetCandidateAddresses() const override;
   virtual CString ResolvePackageURL(const CString& strURL) const override;
};


//////////////////////  CFileSystemCatalogServer - Old version layout    ////////////////////////////////

class CFileSystemCatalogServer : public CCatalogServer
{
public:
   CFileSystemCatalogServer(LPCTSTR strAppName,LPCTSTR strServerName, const CString& libraryFileName, const CString& templateFilePath,const CString& strExt);

   CString GetLibraryFileName() const;

   CString GetTemplateFolderPath() const;

   // virtual's
   virtual std::vector<CString> GetPublishers() const override;
   virtual bool DoesPublisherExist(const CString& publisher) const override;
   virtual CString GetMasterLibraryURL(const CString& publisher) const override;
   virtual CString GetWebLink(const CString& publisher) const override;
   virtual bool CheckForUpdates(const CString& publisher, IProgressMonitor* pProgress,
                                const CString& cacheFolder) const override;
   virtual bool PopulateCatalog(const CString& publisher, IProgressMonitor* pProgress,
                                const CString& cacheFolder) const override;
   virtual bool TestServer(CString& errorMessage) const override;
   virtual bool IsNetworkError() const override;
   virtual void FakeNetworkError(bool bFake) const override; // used for testing... causes IsNetworkError to always return true
   virtual CString CleanUpURL(const CString& strURL, bool isFile) const override;

private:
   CString m_LibraryFileName;
   CString m_TemplateFilePath;

   mutable bool m_bFakeError;

};

#endif // INCLUDED_CatalogServer_H_
