///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2016  Washington State Department of Transportation
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
enum SharedResourceType
{
   srtDefault,     // use the default files provided by PGSuper
   srtInternetFtp, // use library file and workgroup templates published on an internet ftp server
   srtLocal,       // use library file and workgroup templates published on a local network
   srtInternetHttp,// use library file and workgroup templates published on an internet http server
   srtLocalIni     // library file and workgroup templates published on a local network with an .ini file
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
      ceMissingMd5Deep,
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
   bool CheckForUpdatesUsingMD5(const CString& strLocalMasterLibMD5,const CString& strLocalWorkgroupTemplateMD5,
                                const CString& cachedMasterLibFile, const CString& cachedTemplateFolder) const;

   CString m_AppName;

private:
   CCatalogServer(); 

   CString m_Name;
   SharedResourceType m_ServerType;
   CString m_TemplateFileExt;
};

//////////////////////  CFtpCatalogServer    ////////////////////////////////

class CFtpCatalogServer : public CCatalogServer
{
public:
   // Default constructor tries to hit wsdot server
   CFtpCatalogServer(LPCTSTR strAppName,const CString& strExt);
   // constructor for anonymous server
   CFtpCatalogServer(LPCTSTR strAppName,LPCTSTR strServerName, const CString& address,const CString& strExt);
   CString GetAddress() const;

   // virtual's
   virtual std::vector<CString> GetPublishers() const;
   virtual bool DoesPublisherExist(const CString& publisher) const;
   virtual CString GetMasterLibraryURL(const CString& publisher) const;
   virtual CString GetWebLink(const CString& publisher) const;
   virtual bool CheckForUpdates(const CString& publisher, IProgressMonitor* pProgress, 
                                const CString& cacheFolder) const;
   virtual bool PopulateCatalog(const CString& publisher, IProgressMonitor* pProgress,
                                               const CString& cacheFolder) const;
   virtual bool TestServer(CString& errorMessage) const;
   virtual bool IsNetworkError() const;
   virtual void FakeNetworkError(bool bFake) const; // used for testing... causes IsNetworkError to always return true
   virtual CString CleanUpURL(const CString& strURL, bool isFile) const;

private:
   void Init();
   void SetAddress(const CString& address);
   void FetchCatalog(IProgressMonitor* pProgress, bool toTempfolder=false) const;
   bool PopulateLibraryFile(IProgressMonitor* pProgress,const CString& masterLibraryURL, const CString& cachedMasterLibFile) const;
   bool PopulateTemplateFolder(IProgressMonitor* pProgress,const CString& templateFolderURL, const CString& cachedTemplateFolder) const;
   bool PopulatePgz(const CString& publisher, IProgressMonitor* pProgress, const CString& cacheFolder) const;

   bool CheckForUpdatesOriginal(const CString& publisher, IProgressMonitor* pProgress, const CString& cacheFolder) const;
   bool CheckForUpdatesPgz(const CString& publisher, IProgressMonitor* pProgress, const CString& cacheFolder) const;

   CString m_ServerAddress;

   
   // local state
   mutable CCatalog m_Catalog; // catalog parser
   mutable CString m_strLocalCatalog; // full path to local catalog file in temp folder
   mutable bool m_bDoFetchCatalog;
   mutable bool m_bFakeError;
};

//////////////////////  CIniCatalogServer    ////////////////////////////////
// abstract server that works on .ini files

class CIniCatalogServer : public CCatalogServer
{
public:
   // constructor for anonymous server
   CIniCatalogServer(LPCTSTR strAppName,LPCTSTR strServerName,SharedResourceType type, const CString& address,const CString& strExt);

   CString GetAddress() const;

   // virtual's
   virtual std::vector<CString> GetPublishers() const;
   virtual bool DoesPublisherExist(const CString& publisher) const;
   virtual CString GetMasterLibraryURL(const CString& publisher) const;
   virtual CString GetWebLink(const CString& publisher) const;
   virtual bool CheckForUpdates(const CString& publisher, IProgressMonitor* pProgress, 
                                const CString& cacheFolder) const;
   virtual bool PopulateCatalog(const CString& publisher, IProgressMonitor* pProgress,
                                const CString& cacheFolder) const;
   virtual bool TestServer(CString& errorMessage) const;
   virtual bool IsNetworkError() const;
   virtual void FakeNetworkError(bool bFake) const; // used for testing... causes IsNetworkError to always return true

   enum gwResult
   {
      gwOk,
      gwInvalidUrl,
      gwConnectionError,
      gwNotFound
   };

protected:
   // pure virtual to get file from wherever and bring it down to local file system cache
   virtual gwResult GetWebFile(const CString& strFileURL, const CString& strLocalTargetFile) const = 0;

protected:
   void Init();
   void SetAddress(const CString& address);
   void FetchCatalog(IProgressMonitor* pProgress) const;


   CString m_ServerAddress;

   mutable CCatalog m_Catalog; // catalog parser
   mutable CString m_strLocalCatalog; // full path to local catalog file in temp folder
   mutable bool m_bFakeError;
   mutable bool m_bDoFetchCatalog;
};


//////////////////////  CHttpCatalogServer    ////////////////////////////////
// Use CIniCatalogServer for most of the heavy lifting
// 
class CHttpCatalogServer : public CIniCatalogServer
{
public:
   // constructor for anonymous server
   CHttpCatalogServer(LPCTSTR strAppName,LPCTSTR strServerName, const CString& address,const CString& strExt):
      CIniCatalogServer(strAppName, strServerName, srtInternetHttp, address, strExt)
   {
      ;
   }

protected:
   virtual gwResult GetWebFile(const CString& strFileURL, const CString& strLocalTargetFile) const;
   virtual CString CleanUpURL(const CString& strURL, bool isFile) const;
};

//////////////////////  CFileSystemIniCatalogServer    ////////////////////////////////
// Use CIniCatalogServer for most of the heavy lifting
// 
class CFileSystemIniCatalogServer : public CIniCatalogServer
{
public:
   // constructor for anonymous server
   CFileSystemIniCatalogServer(LPCTSTR strAppName,LPCTSTR strServerName, const CString& address,const CString& strExt):
   CIniCatalogServer(strAppName, strServerName, srtLocalIni, address, strExt)
   {
      ;
   }

protected:
   virtual gwResult GetWebFile(const CString& strFileURL, const CString& strLocalTargetFile) const;
   virtual CString CleanUpURL(const CString& strURL, bool isFile) const;
};


//////////////////////  CFileSystemCatalogServer - Old version layout    ////////////////////////////////

class CFileSystemCatalogServer : public CCatalogServer
{
public:
   CFileSystemCatalogServer(LPCTSTR strAppName,LPCTSTR strServerName, const CString& libraryFileName, const CString& templateFilePath,const CString& strExt);

   CString GetLibraryFileName() const;

   CString GetTemplateFolderPath() const;

   // virtual's
   virtual std::vector<CString> GetPublishers() const;
   virtual bool DoesPublisherExist(const CString& publisher) const;
   virtual CString GetMasterLibraryURL(const CString& publisher) const;
   virtual CString GetWebLink(const CString& publisher) const;
   virtual bool CheckForUpdates(const CString& publisher, IProgressMonitor* pProgress, 
                                const CString& cacheFolder) const;
   virtual bool PopulateCatalog(const CString& publisher, IProgressMonitor* pProgress,
                                const CString& cacheFolder) const;
   virtual bool TestServer(CString& errorMessage) const;
   virtual bool IsNetworkError() const;
   virtual void FakeNetworkError(bool bFake) const; // used for testing... causes IsNetworkError to always return true
   virtual CString CleanUpURL(const CString& strURL, bool isFile) const;

private:
   CString m_LibraryFileName;
   CString m_TemplateFilePath;

   mutable bool m_bFakeError;

};

#endif // INCLUDED_CatalogServer_H_