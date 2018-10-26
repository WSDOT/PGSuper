///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2013  Washington State Department of Transportation
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

#if !defined INCLUDED_PGSUPERCATALOGSERVER_H_
#define INCLUDED_PGSUPERCATALOGSERVER_H_

#include "PGSuperCatalog.h"
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
   srtInternetHttp // use library file and workgroup templates published on an internet http server
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

class CPGSuperCatalogServer;
class CPGSuperApp;

// Factory functions for creating and saving servers
CPGSuperCatalogServer* CreateCatalogServer(const CString& createString,const CString& strExt);
CPGSuperCatalogServer* CreateCatalogServer(const CString& strServerName,const CString& createString,const CString& strExt);
CString GetCreationString(const CPGSuperCatalogServer* pServer);

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
      ceDownloadingFile
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
class CPGSuperCatalogServer
{
public:
   CPGSuperCatalogServer(const CString& name,SharedResourceType type,const CString& strExt); 

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


protected:
   bool CheckForUpdatesUsingMD5(const CString& strLocalMasterLibMD5,const CString& strLocalWorkgroupTemplateMD5,
                                const CString& cachedMasterLibFile, const CString& cachedTemplateFolder) const;

private:
   CPGSuperCatalogServer(); 

   CString m_Name;
   SharedResourceType m_ServerType;
   CString m_TemplateFileExt;
};

//////////////////////  CFtpPGSuperCatalogServer    ////////////////////////////////

class CFtpPGSuperCatalogServer : public CPGSuperCatalogServer
{
public:
   // Default constructor tries to hit wsdot server
   CFtpPGSuperCatalogServer(const CString& strExt);
   // constructor for anonymous server
   CFtpPGSuperCatalogServer(const CString& name, const CString& address,const CString& strExt);
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
   mutable CPGSuperCatalog m_Catalog; // catalog parser
   mutable CString m_strLocalCatalog; // full path to local catalog file in temp folder
   mutable bool m_bDoFetchCatalog;
   mutable bool m_bFakeError;
};

//////////////////////  CHttpPGSuperCatalogServer    ////////////////////////////////

class CHttpPGSuperCatalogServer : public CPGSuperCatalogServer
{
public:
   // constructor for anonymous server
   CHttpPGSuperCatalogServer(const CString& name, const CString& address,const CString& strExt);

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

private:
   void Init();
   void SetAddress(const CString& address);
   void FetchCatalog(IProgressMonitor* pProgress) const;

   enum gwResult
   {
      gwOk,
      gwInvalidUrl,
      gwConnectionError,
      gwNotFound
   };

   gwResult GetWebFile(const CString& strFileURL, const CString& strLocalTargetFile) const;

   CString m_ServerAddress;

   mutable CPGSuperCatalog m_Catalog; // catalog parser
   mutable CString m_strLocalCatalog; // full path to local catalog file in temp folder
   mutable bool m_bFakeError;
   mutable bool m_bDoFetchCatalog;

};


//////////////////////  CFileSystemPGSuperCatalogServer    ////////////////////////////////

class CFileSystemPGSuperCatalogServer : public CPGSuperCatalogServer
{
public:
   CFileSystemPGSuperCatalogServer(const CString& name, const CString& libraryFileName, const CString& templateFilePath,const CString& strExt);

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

private:
   CString m_LibraryFileName;
   CString m_TemplateFilePath;

   mutable bool m_bFakeError;

};

#endif // INCLUDED_PGSUPERCATALOGSERVER_H_