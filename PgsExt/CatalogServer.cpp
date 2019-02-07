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
#include <PgsExt\PgsExtLib.h>
#include <PgsExt\CatalogServer.h>
#include "TemplateManager.h"
#include "..\MakePgz\UnzipPgz.h" 
#include <EAF\EAFApp.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

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

         if (strType==_T("FTP"))
         {
            CFtpCatalogServer* psvr = new CFtpCatalogServer(strAppName, strName, strAddress, strExt);
            return psvr;
         }
         else if (strType==_T("HTTP"))
         {
            CHttpCatalogServer* psvr = new CHttpCatalogServer(strAppName, strName, strAddress, strExt);
            return psvr;
         }
         else if (strType==_T("FILSYS"))
         {
            p = strAddress.Find(_T('|'));
            if ( p != -1 )
            {
               CString strLibraryFileName = strAddress.Left(p);
               CString strTemplateFolder = strAddress.Mid(p+1);
               CFileSystemCatalogServer* psvr = new CFileSystemCatalogServer(strAppName, strName, strLibraryFileName, strTemplateFolder, strExt);
               return psvr;
            }
            else
            {
               ATLASSERT(false);
               return nullptr;
            }
         }
         else if (strType==_T("FILSYSINI"))
         {
            CFileSystemIniCatalogServer* psvr = new CFileSystemIniCatalogServer(strAppName, strName, strAddress, strExt);
            return psvr;
         }
         else
         {
            ATLASSERT(0);
            return nullptr;
         }
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
         CFtpCatalogServer* psvr = new CFtpCatalogServer(strAppName, strName, strAddress, strExt);
         return psvr;
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
      if (strType==_T("FTP"))
      {
         CFtpCatalogServer* psvr = new CFtpCatalogServer(strAppName, strServerName, strAddress, strExt);
         return psvr;
      }
      else if (strType==_T("HTTP"))
      {
         CHttpCatalogServer* psvr = new CHttpCatalogServer(strAppName, strServerName, strAddress, strExt);
         return psvr;
      }
      else if (strType==_T("FILSYS"))
      {
         p = strAddress.Find(_T('|'));
         if ( p != -1 )
         {
            CString strLibraryFileName = strAddress.Left(p);
            CString strTemplateFolder = strAddress.Mid(p+1);
            CFileSystemCatalogServer* psvr = new CFileSystemCatalogServer(strAppName,strServerName, strLibraryFileName, strTemplateFolder, strExt);
            return psvr;
         }
         else
         {
            ATLASSERT(false);
            return nullptr;
         }
      }
      else if (strType==_T("FILSYSINI"))
      {
         CFileSystemIniCatalogServer* psvr = new CFileSystemIniCatalogServer(strAppName,strServerName, strAddress, strExt);
         return psvr;
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
         CString strAddress = createString.Left(p);
         CFtpCatalogServer* psvr = new CFtpCatalogServer(strAppName,strServerName, strAddress, strExt);
         return psvr;
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
   if (type==srtInternetFtp)
   {
      const CFtpCatalogServer* psvr = dynamic_cast<const CFtpCatalogServer*>(pServer);
      ATLASSERT(psvr);
      creastr.Format(_T("FTP|%s"),psvr->GetAddress());
   }
   else if (type==srtInternetHttp)
   {
      const CHttpCatalogServer* psvr = dynamic_cast<const CHttpCatalogServer*>(pServer);
      ATLASSERT(psvr);
      creastr.Format(_T("HTTP|%s"),psvr->GetAddress());
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

// Temporary locations to download md5 files before comparisons
static CString GetLocalMasterLibraryMD5Filename()
{
   TCHAR buffer[256];
   ::GetTempPath(256,buffer);

   CString cbuf;
   cbuf.Format(_T("%sMasterLib.md5"),buffer);
   return cbuf;
}

static CString GetLocalWorkgroupTemplateFolderMD5Filename()
{
   TCHAR buffer[256];
   ::GetTempPath(256,buffer);
   CString cbuf;
   cbuf.Format(_T("%sWorkgroupTemplates.md5"),buffer);
   return cbuf;
}

static CString GetPgzFilename(LPCTSTR strAppName)
{
   CString strPgzFilename;
   strPgzFilename.Format(_T("%s.pgz"),strAppName);
   return strPgzFilename;
}

static CString GetPgzMd5Filename(LPCTSTR strAppName)
{
   CString strPgzMd5;
   strPgzMd5.Format(_T("%s.pgz.md5"),strAppName);
   return strPgzMd5;
}

static CString GetTempPgzMD5Filename(LPCTSTR strAppName)
{
   CString strCatalogFile;

   TCHAR buffer[256];
   ::GetTempPath(256,buffer);

   CString cbuf;
   cbuf.Format(_T("%s%s.pgz.md5"),buffer,strAppName);
   return cbuf;
}

CString GetCatalogFileName(LPCTSTR strAppName)
{
   CString strCatalogFile;
   strCatalogFile.Format(_T("%sPackages.ini"),strAppName);
   return strCatalogFile;
}

// return true if check passes
static bool CheckFileAgainstMd5(const CString& testFile, const CString& md5File)
{
   CEAFApp* pApp = EAFGetApp();
   CString strAppPath = pApp->GetAppLocation();

   CString strMD5Deep = strAppPath + CString(_T("md5deep.exe"));

   CFileFind finder;
   BOOL bFound = finder.FindFile(strMD5Deep);
   if ( !bFound )
   {
      CString strMessage;
      strMessage.Format(_T("The program %s is missing. This program is required for %s to access configuration file. Please repair %s."),strMD5Deep,AfxGetApp()->m_pszProfileName,AfxGetApp()->m_pszProfileName);

      throw CCatalogServerException(CCatalogServerException::ceMissingMd5Deep, strMessage);
   }

   CString strArg1 = CString(_T("-x ")) + CString(_T("\"")) + md5File + CString(_T("\""));
   CString strArg2 = CString(_T("\"")) + testFile + CString(_T("\""));
//   LOG(strMD5Deep << _T(" ") << strArg1 << _T(" ") << strArg2);
#if defined _UNICODE
   intptr_t pgz_result = _wspawnl(_P_WAIT,strMD5Deep,strMD5Deep,strArg1,strArg2,nullptr);
#else
   intptr_t pgz_result = _spawnl(_P_WAIT,strMD5Deep,strMD5Deep,strArg1,strArg2,nullptr);
#endif
//   LOG(_T("pgz_result = ") << pgz_result);

   return pgz_result==0;
}


// Function to clean sloppy ftp strings
static CString CleanFTPURL(const CString& strURL, bool isFile)
{
   // if this url starts with ftp, make sure the slashes go the right direction
   CString url = strURL;
   CString left = url.Left(3);
   left.MakeLower();
   if ( left == _T("ftp") )
   {
      url.Replace(_T("\\"),_T("/"));
   }

   // if it doesn't start with ftp:// add it
   left = url.Left(4);
   left.MakeLower();
   if (left == _T("ftp."))
   {
      url.Insert(0,_T("ftp://"));
   }

   if (!isFile && url.Right(1) != _T("/") )
   {
      url += _T("/");
   }

   return url;
}

static CString CleanHTTPURL(const CString& strURL, bool isFile)
{
   // if this url starts with http, make sure the slashes go the right direction
   CString url = strURL;
   CString left = url.Left(4);
   left.MakeLower();
   if ( left == _T("http") )
   {
      url.Replace(_T("\\"),_T("/"));
   }

   // if it doesn't start with http:// add it
   left = url.Left(5);
   left.MakeLower();
   if (left == _T("http."))
   {
      url.Insert(0,_T("http://"));
   }

   if (!isFile && url.Right(1) != _T("/") )
   {
      url += _T("/");
   }

   return url;
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



bool CCatalogServer::CheckForUpdatesUsingMD5(const CString& strLocalMasterLibMD5,const CString& strLocalWorkgroupTemplateMD5,
                                                    const CString& cachedMasterLibFile, const CString& cachedTemplateFolder) const
{
   bool bUpdatePending = false;

   CEAFApp* pApp = EAFGetApp();
   CString strAppPath = pApp->GetAppLocation();

   CString strMD5Deep = strAppPath + CString(_T("md5deep.exe"));

   CFileFind finder;
   BOOL bFound = finder.FindFile(strMD5Deep);
   if ( !bFound )
   {
      CString strMessage;
      strMessage.Format(_T("The program %s is missing. This program is required for %s to access the configuration file. Please repair %s."),strMD5Deep,AfxGetApp()->m_pszProfileName,AfxGetApp()->m_pszProfileName);

      throw CCatalogServerException(CCatalogServerException::ceMissingMd5Deep, strMessage);
   }

   // Master library
   CString strArg1 = CString(_T("-x ")) + CString(_T("\"")) + strLocalMasterLibMD5 + CString(_T("\""));
   CString strArg2 = CString(_T("\"")) + cachedMasterLibFile + CString(_T("\""));
//   LOG(strMD5Deep << _T(" ") << strArg1 << _T(" ") << strArg2);
#if defined _UNICODE
   intptr_t master_lib_result = _wspawnl(_P_WAIT,strMD5Deep,strMD5Deep,strArg1,strArg2,nullptr);
#else
   intptr_t master_lib_result = _spawnl(_P_WAIT,strMD5Deep,strMD5Deep,strArg1,strArg2,nullptr);
#endif
//   LOG(_T("master_lib_result = ") << master_lib_result);

   // Workgroup templates
   CString strWorkgroupTemplateFolderCache = cachedTemplateFolder;

   // remove the last \\ because md5deep doesn't like it
   int loc = strWorkgroupTemplateFolderCache.GetLength()-1;
   if ( strWorkgroupTemplateFolderCache.GetAt(loc) == _T('\\') )
   {
      strWorkgroupTemplateFolderCache.SetAt(loc,_T('\0'));
   }

   strArg1 = CString(_T("-x ")) + CString(_T("\"")) + strLocalWorkgroupTemplateMD5 + CString(_T("\""));
   strArg2 = CString(_T("-r ")) + CString(_T("\"")) + strWorkgroupTemplateFolderCache + CString(_T("\""));
//   LOG(strMD5Deep << _T(" ") << strArg1 << _T(" ") << strArg2);
#if defined _UNICODE
   intptr_t workgroup_template_result = _wspawnl(_P_WAIT,strMD5Deep,strMD5Deep,strArg1,strArg2,nullptr);
#else
   intptr_t workgroup_template_result = _spawnl(_P_WAIT,strMD5Deep,strMD5Deep,strArg1,strArg2,nullptr);
#endif
//   LOG(_T("workgroup_template_result = ") << workgroup_template_result);

   if ( master_lib_result != 0 || workgroup_template_result != 0 )
   {
      bUpdatePending = true;
   }

   return bUpdatePending;
}


/// CFtpCatalogServer ///
// default ftp to wsdot
CFtpCatalogServer::CFtpCatalogServer(LPCTSTR strAppName,const CString& strExt):
CCatalogServer(strAppName,_T("WSDOT"),srtInternetFtp,strExt)
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   CWinApp* pApp = AfxGetApp();
   m_ServerAddress.Format(_T("%s/%s/"),_T("ftp://ftp.wsdot.wa.gov/public/Bridge/Software"),pApp->m_pszProfileName);
   Init();
}

CFtpCatalogServer::CFtpCatalogServer(LPCTSTR strAppName,LPCTSTR name, const CString& address,const CString& strExt):
CCatalogServer(strAppName,name,srtInternetFtp,strExt)
{
   SetAddress(address);
   Init();
}

void CFtpCatalogServer::Init()
{
   m_bDoFetchCatalog = true;
   m_bFakeError = false;
}

CString CFtpCatalogServer::GetAddress() const
{
   return m_ServerAddress;
}

void CFtpCatalogServer::SetAddress(const CString& address)
{
   m_ServerAddress = address;
}

void CFtpCatalogServer::FetchCatalog(IProgressMonitor* pProgress, bool toTempfolder) const
{
   if (m_bDoFetchCatalog)
   {
      if (pProgress!=nullptr)
      {
         pProgress->put_Message(0, CComBSTR("Reading Configuration Publishers from the Internet"));
      }

      TCHAR buffer[256];
      ::GetTempPath(256,buffer);

      m_strLocalCatalog = buffer;
      m_strLocalCatalog += GetCatalogFileName(m_AppName);

      if (toTempfolder)
      {
         // not our 'real' catalog file - give it a bogus name
         m_strLocalCatalog += _T(".tmp");
      }

      // pull the catalog down from the one and only catalog server
      // (we will support general catalog servers later)
      CString catURL = CleanUpURL(m_ServerAddress,false);
      catURL += GetCatalogFileName(m_AppName);
      try
      {
         DWORD dwServiceType;
         CString strServer, strObject;
         INTERNET_PORT nPort;

         BOOL bSuccess = AfxParseURL(catURL,dwServiceType,strServer,strObject,nPort);
         ATLASSERT( bSuccess );

	      //Username we will use if a secure site comes into play
	      LPCTSTR pstrUserName = nullptr; 
	      //The password we will use
	      LPCTSTR pstrPassword = nullptr;

         // the URL is bogus or it isn't for an FTP service
         if ( !bSuccess || dwServiceType != AFX_INET_SERVICE_FTP )
         {
            CString msg;
            msg.Format(_T("Error parsing configuration URL: %s"), catURL);
            throw CCatalogServerException(CCatalogServerException::ceGettingCatalogFile, msg);
         }
         else
         {
            // download the catalog file
            CInternetSession inetSession;
            std::unique_ptr<CFtpConnection> pFTP( inetSession.GetFtpConnection(strServer,pstrUserName,pstrPassword,nPort,TRUE) );

            CFtpFileFind ftpFind(pFTP.get());
            BOOL bFound = ftpFind.FindFile(strObject,INTERNET_FLAG_RELOAD | INTERNET_FLAG_EXISTING_CONNECT);
            if ( !bFound || !pFTP->GetFile(strObject,m_strLocalCatalog,FALSE,FILE_ATTRIBUTE_NORMAL,FTP_TRANSFER_TYPE_ASCII | INTERNET_FLAG_RELOAD) )
            {
               // could not find or get the file (it may not be on the server)
               CString msg;
               msg.Format(_T("Could not find configuration: %s"), catURL);
               throw CCatalogServerException(CCatalogServerException::ceGettingCatalogFile, msg);
            }
         }
      }
      catch (CInternetException* pException)
      {
         // the call to inetSession.GetFtpConnection is one tha throws.
         // the result of GetLastError is stored in m_dwError
         TCHAR sz[1024];
         pException->GetErrorMessage(sz, 1024);

         CString msg;
         msg.Format(_T("An error occurred while trying to access the configuration server: (%d) %s\n%s"),pException->m_dwError,sz,catURL);

         if (pException->m_dwError == ERROR_INTERNET_EXTENDED_ERROR)
         {
            // there is extended error information
            // https://docs.microsoft.com/en-us/windows/desktop/api/wininet/nf-wininet-internetconnecta
            // https://docs.microsoft.com/en-us/windows/desktop/api/wininet/nf-wininet-internetgetlastresponseinfoa
            // https://docs.microsoft.com/en-us/windows/desktop/WinInet/wininet-errors - error codes are here

            DWORD dwExtError;
            TCHAR extError[1024];
            DWORD len = 1024;
            if (InternetGetLastResponseInfo(&dwExtError, extError, &len))
            {
               CString msg2;
               msg2.Format(_T("\nExtended error info: %d - %s"), dwExtError, extError);
               msg += msg2;
            }
         }

         pException->Delete();

         throw CCatalogServerException(CCatalogServerException::ceGettingCatalogFile,msg);
      }

      // We have the catalog, initialize the catalog parser
      CString strVersion = CCatalog::GetAppVersion();

      if (! m_Catalog.Init(m_strLocalCatalog,strVersion) )
      {
         CString msg;
         msg.Format(_T("Error occurred while reading the configuration: %s"),catURL);
         throw CCatalogServerException(CCatalogServerException::ceGettingCatalogFile,msg);
      }

      m_bDoFetchCatalog = false;
   }
}

std::vector<CString> CFtpCatalogServer::GetPublishers() const
{
   std::vector<CString> items;

   FetchCatalog(nullptr);

   items = m_Catalog.GetPublishers();

   return items;
}

bool CFtpCatalogServer::DoesPublisherExist(const CString& publisher) const
{
   return m_Catalog.DoesPublisherExist(publisher);
}


CString CFtpCatalogServer::GetMasterLibraryURL(const CString& publisher) const
{
   FetchCatalog(nullptr);

   // catalog parser does the work
   CCatalog::Format type = m_Catalog.GetFormat(publisher);

   CString strMasterLibrary;
   if (type==CCatalog::ctOriginal)
   {
      CString strWorkgroupTemplates;
      m_Catalog.GetCatalogSettings(publisher, strMasterLibrary, strWorkgroupTemplates);
   }
   else
   {
      ATLASSERT(type==CCatalog::ctPgz);
      CString strWorkgroupTemplates;
      m_Catalog.GetCatalogSettings(publisher, strMasterLibrary);
   }

   return strMasterLibrary;
}

CString CFtpCatalogServer::GetWebLink(const CString& publisher) const
{
   FetchCatalog(nullptr);

   CString url = m_Catalog.GetWebLink(publisher);

   return url;
}


bool CFtpCatalogServer::CheckForUpdates(const CString& publisher, IProgressMonitor* pProgress, 
                                               const CString& cacheFolder) const
{
   FetchCatalog(pProgress);

   if (!m_Catalog.DoesPublisherExist(publisher))
   {
      // we are screwed if the publisher doesn't match. This probably will only
      // happen if registry values get trashed in the App class
      ATLASSERT(false);
      CString msg;
      msg.Format(_T("The publisher %s was not found in the configuration information. This appears to be a programing bug."), publisher);
      throw CCatalogServerException(CCatalogServerException::ceGettingCatalogFile, msg);
   }

   // all depends on type of catalog
   CCatalog::Format type = m_Catalog.GetFormat(publisher);

   if (type==CCatalog::ctOriginal)
   {
      return CheckForUpdatesOriginal(publisher, pProgress, cacheFolder);
   }
   else
   {
      return CheckForUpdatesPgz(publisher, pProgress, cacheFolder);
   }
}

bool CFtpCatalogServer::CheckForUpdatesOriginal(const CString& publisher, IProgressMonitor* pProgress, 
                                                       const CString& cacheFolder) const
{
   CString cachedMasterLibFile  = cacheFolder + CString(_T("\\")) + GetMasterLibraryFileName();
   CString cachedTemplateFolder = cacheFolder + CString(_T("\\")) + GetTemplateSubFolderName()+ CString(_T("\\"));

   CString strRawMasterLibraryUrl, strRawWorkgroupTemplatesUrl;
   m_Catalog.GetCatalogSettings(publisher, strRawMasterLibraryUrl, strRawWorkgroupTemplatesUrl);

   // Check the URL and parse it
   DWORD dwServiceType;
   CString strMasterLibraryServer,      strMasterLibraryObject;
   CString strWorkgroupTemplatesServer, strWorkgroupTemplatesObject;
   INTERNET_PORT nPort;

   BOOL bSuccess = AfxParseURL(strRawMasterLibraryUrl,dwServiceType,strMasterLibraryServer,strMasterLibraryObject,nPort);
   if ( !bSuccess || dwServiceType != AFX_INET_SERVICE_FTP)
   {
      CString strMessage;
      strMessage.Format(_T("The published Master library file location, %s, for publisher %s is invalid. Library and Template settings will be restored to original defaults."),strRawMasterLibraryUrl,publisher);

      throw CCatalogServerException(CCatalogServerException::ceParsingURL, strMessage);
   }

   bSuccess = AfxParseURL(strRawWorkgroupTemplatesUrl,dwServiceType,strWorkgroupTemplatesServer,strWorkgroupTemplatesObject,nPort);

   if ( !bSuccess || dwServiceType != AFX_INET_SERVICE_FTP)
   {
      CString strMessage;
      strMessage.Format(_T("The Template folder location, %s, for publisher %s is invalid. Library and Template settings will be restored to original defaults."),strRawWorkgroupTemplatesUrl,publisher);

      throw CCatalogServerException(CCatalogServerException::ceParsingURL, strMessage);
   }

   // connect to the internet and get the md5 files
   CInternetSession inetSession;
   CFtpConnection* pFTP = nullptr;

   //Username we will use if a secure site comes into play
   LPCTSTR pstrUserName = nullptr; 
   //The password we will use
   LPCTSTR pstrPassword = nullptr;

   CString strLocalFile;
   CString strLocalFolder;
   try
   {
      // Master library
      pFTP = inetSession.GetFtpConnection(strMasterLibraryServer,pstrUserName,pstrPassword,nPort,TRUE);
      CFtpFileFind master_library_file_find(pFTP);

      CString strMasterLibMD5 = strMasterLibraryObject + CString(_T(".md5"));
      strLocalFile = GetLocalMasterLibraryMD5Filename();
      
//      LOG(_T("Getting ") << strMasterLibMD5 << _T(" from ") << strMasterLibraryServer);

      if ( !master_library_file_find.FindFile(strMasterLibMD5,INTERNET_FLAG_RELOAD | INTERNET_FLAG_EXISTING_CONNECT) || !pFTP->GetFile(strMasterLibMD5,strLocalFile,FALSE,FILE_ATTRIBUTE_NORMAL,FTP_TRANSFER_TYPE_ASCII | INTERNET_FLAG_RELOAD) )
      {
         pFTP->Close();
         delete pFTP;

         CString msg;
         msg.Format(_T("Error getting %s from %s."), strMasterLibMD5, strMasterLibraryServer);
//         LOG(msg);
         throw CCatalogServerException(CCatalogServerException::ceFindingFile, msg);
      }

      pFTP->Close();
      delete pFTP;

      // Workgroup Template
      pFTP = inetSession.GetFtpConnection(strWorkgroupTemplatesServer,pstrUserName,pstrPassword,nPort,TRUE);
      CFtpFileFind workgroup_template_file_find(pFTP);

      CString strWorkgroupTemplateMD5 = strWorkgroupTemplatesObject + CString(_T("WorkgroupTemplates.md5"));
      strLocalFolder = GetLocalWorkgroupTemplateFolderMD5Filename();

//      LOG(_T("Getting ") << strWorkgroupTemplateMD5 << _T(" from ") << strWorkgroupTemplatesServer);

      if ( !workgroup_template_file_find.FindFile(strWorkgroupTemplateMD5,INTERNET_FLAG_RELOAD | INTERNET_FLAG_EXISTING_CONNECT) || !pFTP->GetFile(strWorkgroupTemplateMD5,strLocalFolder,FALSE,FILE_ATTRIBUTE_NORMAL,FTP_TRANSFER_TYPE_ASCII | INTERNET_FLAG_RELOAD) )
      {
         pFTP->Close();
         delete pFTP;

         CString msg;
         msg.Format(_T("Error getting %s from %s."), strWorkgroupTemplateMD5, strWorkgroupTemplatesServer);
//         LOG(msg);
         throw CCatalogServerException(CCatalogServerException::ceFindingFile, msg);
      }

      pFTP->Close();
      delete pFTP;
   }
   catch(CInternetException* pException)
   {
      TCHAR lmsg[256];
      pException->GetErrorMessage(lmsg,256);
      pException->Delete();

      CString msg;
      msg.Format(_T("A network error occured while trying determine if an update was pending. Last message was %s"),lmsg);
//      LOG(msg);
      throw CCatalogServerException(CCatalogServerException::ceFindingFile, msg);
   }

   bool bUpdatePending = CheckForUpdatesUsingMD5(strLocalFile,strLocalFolder,cachedMasterLibFile,cachedTemplateFolder);

   return bUpdatePending;
}

bool CFtpCatalogServer::CheckForUpdatesPgz(const CString& publisher, IProgressMonitor* pProgress, 
                                                       const CString& cacheFolder) const
{
   bool result = false; // assume the best

   CString strRawPgzUrl;
   m_Catalog.GetCatalogSettings(publisher, strRawPgzUrl);

   // name of md5 on server
   strRawPgzUrl += CString(_T(".md5"));

   // Check the URL and parse it
   DWORD dwServiceType;
   CString strPgzServer,      strPgzObject;
   INTERNET_PORT nPort;

   if(pProgress!=nullptr)
   {
      pProgress->put_Message(0,CComBSTR("Reading md5 hash from server"));
   }

   BOOL bSuccess = AfxParseURL(strRawPgzUrl,dwServiceType,strPgzServer,strPgzObject,nPort);
   if ( !bSuccess || dwServiceType != AFX_INET_SERVICE_FTP)
   {
      CString strMessage;
      strMessage.Format(_T("The published md5 file location, %s, for publisher %s has invalid format. Library and Template settings will be restored to original defaults."),strRawPgzUrl,publisher);
      throw CCatalogServerException(CCatalogServerException::ceParsingURL, strMessage);
   }

   // name of md5p copied to temp folder
   CString strTempMd5File = GetTempPgzMD5Filename(m_AppName);

   //Username we will use if a secure site comes into play
   LPCTSTR pstrUserName = nullptr; 
   //The password we will use
   LPCTSTR pstrPassword = nullptr;

   // connect to the internet and get the md5 file
   CInternetSession inetSession;
   CFtpConnection* pFTP = nullptr;

   try
   {
      pFTP = inetSession.GetFtpConnection(strPgzServer,pstrUserName,pstrPassword,nPort,TRUE);
      CFtpFileFind pgz_file_find(pFTP);
      
//      LOG(_T("Getting ") << strPgzObject << _T(" from ") << strPgzServer);

      if ( !pgz_file_find.FindFile(strPgzObject,INTERNET_FLAG_RELOAD | INTERNET_FLAG_EXISTING_CONNECT) || !pFTP->GetFile(strPgzObject,strTempMd5File,FALSE,FILE_ATTRIBUTE_NORMAL,FTP_TRANSFER_TYPE_ASCII | INTERNET_FLAG_RELOAD) )
      {
         pFTP->Close();
         delete pFTP;

//         LOG(_T("Error getting ") << strPgzObject << _T(" from ") << strPgzServer);
         CString strMessage;
         strMessage.Format(_T("Error getting md5 file , %s, from publisher %s . Contact server owner."),strRawPgzUrl,publisher);
         throw CCatalogServerException(CCatalogServerException::ceParsingURL, strMessage);
      }

      pFTP->Close();
      delete pFTP;
   }
   catch(CInternetException* pException)
   {
      TCHAR lmsg[256];
      pException->GetErrorMessage(lmsg,256);
      pException->Delete();

      CString msg;
      msg.Format(_T("A network error occured while trying download pgz md5 file. Last message was %s"),lmsg);
//      LOG(msg);
      throw CCatalogServerException(CCatalogServerException::ceFindingFile, msg);
   }

   // We have the md5, compare it to file in cache created at download
   CString cachedPgzFile = cacheFolder + GetPgzFilename(m_AppName);

   // we need to update if it doesn't compare
   result = !CheckFileAgainstMd5(cachedPgzFile, strTempMd5File);

   return result;
}


bool CFtpCatalogServer::PopulateCatalog(const CString& publisher, IProgressMonitor* pProgress,
                                               const CString& cacheFolder) const
{
   FetchCatalog(pProgress);

   // all depends on type of catalog
   CCatalog::Format type = m_Catalog.GetFormat(publisher);

   if (type==CCatalog::ctOriginal)
   {
      CString cachedMasterLibFile  = cacheFolder + CString(_T("\\")) + GetMasterLibraryFileName();
      CString cachedTemplateFolder = cacheFolder + CString(_T("\\")) + GetTemplateSubFolderName()+ CString(_T("\\"));

      CString strMasterLibraryURL;
      CString strWorkgroupTemplatesURL;
      m_Catalog.GetCatalogSettings(publisher, strMasterLibraryURL, strWorkgroupTemplatesURL);

      // Download the Master library file first
      bool st = PopulateLibraryFile(pProgress, strMasterLibraryURL, cachedMasterLibFile);
      if (!st)
      {
         ATLASSERT(st);
         return false;
      }

      // Next the templates
      st = PopulateTemplateFolder(pProgress,strWorkgroupTemplatesURL, cachedTemplateFolder);
      if (!st)
      {
         ATLASSERT(st);
         return false;
      }
   }
   else
   {
      // Looking for a pgz compressed format file
      ATLASSERT(type==CCatalog::ctPgz);

      bool st = PopulatePgz(publisher, pProgress, cacheFolder);
      if (!st)
      {
         return false;
      }
   }

   return true;
}

bool CFtpCatalogServer::PopulateLibraryFile(IProgressMonitor* pProgress,const CString& masterLibraryURL, const CString& cachedMasterLibFile) const
{
   CString strMasterLibraryFile = CleanUpURL(masterLibraryURL,true);

   DWORD dwServiceType;
   CString strServer, strObject;
   INTERNET_PORT nPort;
   BOOL bSuccess = AfxParseURL(strMasterLibraryFile,dwServiceType,strServer,strObject,nPort);
   ATLASSERT( bSuccess && dwServiceType == AFX_INET_SERVICE_FTP);

   //Username we will use if a secure site comes into play
   LPCTSTR pstrUserName = nullptr; 
   //The password we will use
   LPCTSTR pstrPassword = nullptr;

   CInternetSession inetSession;
   CFtpConnection* pFTP = 0;

   try
   {
      pFTP = inetSession.GetFtpConnection(strServer,pstrUserName,pstrPassword,nPort,TRUE);
      CFtpFileFind file_find(pFTP);

      if(pProgress!=nullptr)
      {
         pProgress->put_Message(0,CComBSTR("Downloading the Master Library"));
      }

      if ( file_find.FindFile(strObject,INTERNET_FLAG_RELOAD | INTERNET_FLAG_EXISTING_CONNECT) && pFTP->GetFile(strObject,cachedMasterLibFile,FALSE,FILE_ATTRIBUTE_NORMAL,FTP_TRANSFER_TYPE_ASCII | INTERNET_FLAG_RELOAD) )
      {
         if(pProgress!=nullptr)
         {
            pProgress->put_Message(0,CComBSTR("Master Library downloaded successfully"));
         }
      }
      else
      {
         pFTP->Close();
         delete pFTP;

         CString msg;
         msg.Format(_T("An error occured while downloading master library file %s."),strMasterLibraryFile);

         if(pProgress!=nullptr)
         {
            pProgress->put_Message(0,CComBSTR(msg));
         }

         throw CCatalogServerException(CCatalogServerException::ceFindingFile, msg);
      }
   }
   catch ( CInternetException* pException)
   {
      if ( pFTP )
      {
         pFTP->Close();
         delete pFTP;
      }

      TCHAR lmsg[256];
      pException->GetErrorMessage(lmsg,256);
      pException->Delete();

      CString msg;
      msg.Format(_T("A network error occured while trying to download Master library file %s. Last message was %s"),strMasterLibraryFile,lmsg);
//      LOG(msg);
      throw CCatalogServerException(CCatalogServerException::ceFindingFile, msg);
   }

   pFTP->Close();
   delete pFTP;

   return true;
}

bool CFtpCatalogServer::PopulateTemplateFolder(IProgressMonitor* pProgress,const CString& templateFolderURL, const CString& cachedTemplateFolder) const
{
   // download templates from the FTP server
   CString strWorkgroupTemplateFolder = CleanUpURL(templateFolderURL,false);

   DWORD dwServiceType;
   CString strServer, strObject;
   CString strLocalFile;
   INTERNET_PORT nPort;

   BOOL bSuccess = AfxParseURL(strWorkgroupTemplateFolder,dwServiceType,strServer,strObject,nPort);
   ATLASSERT( bSuccess && dwServiceType == AFX_INET_SERVICE_FTP); // this should be tested elsewhere

   //Username we will use if a secure site comes into play
   LPCTSTR pstrUserName = nullptr; 
   //The password we will use
   LPCTSTR pstrPassword = nullptr;

   CInternetSession inetSession;

   ::CreateDirectory(cachedTemplateFolder,nullptr);

   CFtpConnection* pFTP = 0;

   try
   {
      pFTP = inetSession.GetFtpConnection(strServer,pstrUserName,pstrPassword,nPort,TRUE);

      CFTPTemplateManager templateMgr(GetTemplateFileExtension(),pFTP);
      templateMgr.GetTemplates(strObject,cachedTemplateFolder,pProgress);
   }
   catch ( CInternetException* pException)
   {
      // can't open the connection for what ever reason
      if ( pFTP )
      {
         pFTP->Close();
         delete pFTP;
      }

      TCHAR lmsg[256];
      pException->GetErrorMessage(lmsg,256);
      pException->Delete();

      CString msg;
      msg.Format(_T("A network error occured while trying to download Template library files at %s. Last message was %s"),strWorkgroupTemplateFolder,lmsg);
//      LOG(msg);
      throw CCatalogServerException(CCatalogServerException::ceFindingFile, msg);
   }

   pFTP->Close();
   delete pFTP;

   return true;
}

bool CFtpCatalogServer::PopulatePgz(const CString& publisher, IProgressMonitor* pProgress, const CString& cacheFolder) const
{
   CString pgzFileURL;
   m_Catalog.GetCatalogSettings(publisher, pgzFileURL);

   // First step is to copy the pgzfile and its md5 to our cache folder
   CString pgzFolder(cacheFolder);
   CString pgzCachedFile = cacheFolder + GetPgzFilename(m_AppName);
   CString strPgzFile = CleanUpURL(pgzFileURL,true);

   DWORD dwServiceType;
   CString strServer, strObject;
   INTERNET_PORT nPort;
   BOOL bSuccess = AfxParseURL(strPgzFile,dwServiceType,strServer,strObject,nPort);
   ATLASSERT( bSuccess && dwServiceType == AFX_INET_SERVICE_FTP);

   //Username we will use if a secure site comes into play
   LPCTSTR pstrUserName = nullptr; 
   //The password we will use
   LPCTSTR pstrPassword = nullptr;

   CInternetSession inetSession;
   CFtpConnection* pFTP = 0;

   try
   {
      pFTP = inetSession.GetFtpConnection(strServer,pstrUserName,pstrPassword,nPort,TRUE);
      CFtpFileFind file_find(pFTP);

      if(pProgress!=nullptr)
      {
         pProgress->put_Message(0,CComBSTR("Downloading Configuration"));
      }

      if ( file_find.FindFile(strObject,INTERNET_FLAG_RELOAD | INTERNET_FLAG_EXISTING_CONNECT) && pFTP->GetFile(strObject,pgzCachedFile,FALSE,FILE_ATTRIBUTE_NORMAL,FTP_TRANSFER_TYPE_BINARY | INTERNET_FLAG_RELOAD) )
      {
         if(pProgress!=nullptr)
         {
            pProgress->put_Message(0,CComBSTR("Configuration downloaded successfully"));
         }
      }
      else
      {
         if(pProgress!=nullptr)
         {
            pProgress->put_Message(0,CComBSTR("Error downloading the Configuration"));
         }

         AfxThrowInternetException(0);
      }
   }
   catch ( CInternetException* pException)
   {
      // can't open the connection for what ever reason
      pException->Delete();

      if ( pFTP )
      {
         pFTP->Close();
         delete pFTP;
      }

      CString strMessage;
      strMessage.Format(_T("Error opening the Configuration from %s."),strPgzFile);

      throw CCatalogServerException(CCatalogServerException::ceFindingFile, strMessage);
   }

   pFTP->Close();
   delete pFTP;

   // We now should have our file in the cache.
   // Use md5 to check that download was successful
   if( CheckForUpdatesPgz(publisher, pProgress, cacheFolder) )
   {
      CString strMessage;
      strMessage.Format(_T("It appears that there was an error downloading the file \"%s\"  The md5 file hash does not match that on the server. Try updating again, and contact the server owner if this error persists"),pgzCachedFile);

      ::MessageBox(EAFGetMainFrame()->GetSafeHwnd(),strMessage,_T("Error"), MB_OK );
      return false;
   }

   // Now that we have a validated file - decompress it
   TCHAR* sFolder = pgzFolder.GetBuffer(1);
   TCHAR* sFile   = pgzCachedFile.GetBuffer(1);

   if(pProgress!=nullptr)
   {
      pProgress->put_Message(0,CComBSTR("Uncompressing Configuration to local cache."));
   }

   uzErrorType st = UnZipPGZ( sFile, sFolder );

   pgzFolder.ReleaseBuffer();
   pgzCachedFile.ReleaseBuffer();

   if(st!=uzOk)
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

      return false;
   }

   return true;
}

bool CFtpCatalogServer::IsNetworkError() const
{
   if (m_bFakeError)
   {
      return true;
   }

   try
   {
      FetchCatalog(nullptr,true);
   }
   catch(CCatalogServerException& exc)
   {
      CString msg = exc.GetErrorMessage();
      ::AfxMessageBox(msg,MB_ICONEXCLAMATION | MB_OK);
      return true;
   }
   catch(...)
   {
      ATLASSERT(false);
//      LOG(_T("Unknown network error in IsNetworkError"));
      return true;
   }

   return false; // we fetched successfully
}

void CFtpCatalogServer::FakeNetworkError(bool bFake) const
{
   m_bFakeError = bFake;
}

CString CFtpCatalogServer::CleanUpURL(const CString& strURL, bool isFile) const
{
   return CleanFTPURL(strURL, isFile);
}

bool CFtpCatalogServer::TestServer(CString& errorMessage) const
{
   DWORD dwServiceType;
   CString strServer, strObject;
   INTERNET_PORT nPort;

   CString catalog_file = CleanUpURL(m_ServerAddress,false);
   catalog_file += GetCatalogFileName(m_AppName);

   BOOL bSuccess = AfxParseURL(catalog_file,dwServiceType,strServer,strObject,nPort);
   if ( !bSuccess || dwServiceType != AFX_INET_SERVICE_FTP )
   {
      errorMessage.Format(_T("Error parsing URL - Please check the ftp string for errors"));
      return false;
   }

   //Username we will use if a secure site comes into play
   LPCTSTR pstrUserName = nullptr; 
   //The password we will use
   LPCTSTR pstrPassword = nullptr;

   // See if we find the catalog file
   CInternetSession inetSession;
   std::unique_ptr<CFtpConnection> pFTP( inetSession.GetFtpConnection(strServer,pstrUserName,pstrPassword,nPort,TRUE) );

   CFtpFileFind ftpFind(pFTP.get());
   if ( !ftpFind.FindFile(strObject,INTERNET_FLAG_RELOAD | INTERNET_FLAG_EXISTING_CONNECT))
   {
      errorMessage.Format(_T("Error - Unable to find catalog file: %s on the server."),strObject);
      return false;
   }

   return true;
}

/// CIniCatalogServer ///
CIniCatalogServer::CIniCatalogServer(LPCTSTR strAppName,LPCTSTR name,SharedResourceType type, const CString& address,const CString& strExt):
CCatalogServer(strAppName,name,type,strExt)
{
   SetAddress(address);
   Init();
}

void CIniCatalogServer::Init()
{
   m_bFakeError = false;
   m_bDoFetchCatalog = true;
}

CString CIniCatalogServer::GetAddress() const
{
   return m_ServerAddress;
}

void CIniCatalogServer::SetAddress(const CString& address)
{
   m_ServerAddress = address;
}

void CIniCatalogServer::FetchCatalog(IProgressMonitor* pProgress) const
{
   if (m_bDoFetchCatalog)
   {
      TCHAR buffer[256];
      ::GetTempPath(256,buffer);

      m_strLocalCatalog = buffer;
      m_strLocalCatalog += GetCatalogFileName(m_AppName);

      CString catalogURL = CleanUpURL(m_ServerAddress,false);
      catalogURL += GetCatalogFileName(m_AppName);

      // create a progress window
      if (pProgress!=nullptr)
      {
         CComBSTR str = CString(_T("Reading Library Publishers from: ")) + catalogURL;
         pProgress->put_Message(0,str);
      }

      gwResult gwres = GetWebFile(catalogURL, m_strLocalCatalog);
      if ( gwres==gwOk )
      {
         // we have the catalog, initialize the catalog parser
         CString strVersion =  CCatalog::GetAppVersion();

         try
         {
            if (!m_Catalog.Init(m_strLocalCatalog, strVersion))
            {
               CString msg;
               msg.Format(_T("An Error occurred while reading the catalog file at: %s"), catalogURL);
               throw CCatalogServerException(CCatalogServerException::ceGettingCatalogFile, msg);
            }
         }
         catch (CCatalogParsingException& ex)
         {
            ATLASSERT(!ex.GetLineBeingParsed().IsEmpty()); // this should always be filled by caller
            CString erdsc;
            if (ex.GetErrorType() == CCatalogParsingException::cteVersionStringNotValid)
            {
               erdsc = _T("version string parsing");
            }
            else
            {
               erdsc = _T("undefined");
            }

            CString msg;
            msg.Format(_T("A %s Error occurred while reading the catalog file at:\n %s.\n\n Contents of the last line parsed was:\n%s"), erdsc, catalogURL, ex.GetLineBeingParsed());
            throw CCatalogServerException(CCatalogServerException::ceGettingCatalogFile, msg);
         }
         catch(...)
         {
            CString msg;
            msg.Format(_T("An Uknown Error occurred while reading the catalog file at:\n %s"), catalogURL);
            throw CCatalogServerException(CCatalogServerException::ceGettingCatalogFile, msg);
         }
      }
      else
      {
         CString msg;
         msg.Format(_T("Error parsing catalog URL:\n %s"), catalogURL);
         throw CCatalogServerException(CCatalogServerException::ceGettingCatalogFile, msg);
      }

      m_bDoFetchCatalog = false;
   }
};

std::vector<CString> CIniCatalogServer::GetPublishers() const
{
   std::vector<CString> items;

   FetchCatalog(nullptr);

   items = m_Catalog.GetPublishers();

   return items;
}

bool CIniCatalogServer::DoesPublisherExist(const CString& publisher) const
{
   return m_Catalog.DoesPublisherExist(publisher);
}

CString CIniCatalogServer::GetMasterLibraryURL(const CString& publisher) const
{
   FetchCatalog(nullptr);

   // catalog parser does the work
   ATLASSERT(m_Catalog.GetFormat(publisher)==CCatalog::ctPgz);
   CString pgzUrl;
   m_Catalog.GetCatalogSettings(publisher, pgzUrl);

   return pgzUrl;
}

CString CIniCatalogServer::GetWebLink(const CString& publisher) const
{
   FetchCatalog(nullptr);

   CString url = m_Catalog.GetWebLink(publisher);

   return url;
}

bool CIniCatalogServer::CheckForUpdates(const CString& publisher, IProgressMonitor* pProgress, 
                                               const CString& cacheFolder) const
{
   FetchCatalog(pProgress);

   if (!m_Catalog.DoesPublisherExist(publisher))
   {
      // we are screwed if the publisher doesn't match. This probably will only
      // happen if registry values get trashed in the App class
      ATLASSERT(false);
      CString msg;
      msg.Format(_T("The publisher %s was not found in the current catalog. This appears to be a programing bug."), publisher);
      throw CCatalogServerException(CCatalogServerException::ceGettingCatalogFile, msg);
   }

   // name of md5 on server
   CString strRawPgzUrl;
   m_Catalog.GetCatalogSettings(publisher, strRawPgzUrl);
   strRawPgzUrl += CString(_T(".md5"));

   // name of md5p to be copied to temp folder
   CString strTempMd5File = GetTempPgzMD5Filename(m_AppName);

   if(pProgress!=nullptr)
   {
      pProgress->put_Message(0,CComBSTR("Reading md5 hash from server"));
   }

   bool result = true;

   gwResult gwresult = GetWebFile(strRawPgzUrl, strTempMd5File);
   if (gwresult==gwInvalidUrl)
   {
      CString strMessage;
      strMessage.Format(_T("The published Pgz md5 file location, %s, for publisher %s has invalid format. Library and Template settings will be restored to original defaults."),strRawPgzUrl,publisher);
      throw CCatalogServerException(CCatalogServerException::ceParsingURL, strMessage);
   }
   else if (gwresult==gwNotFound)
   {
      CString msg;
      msg.Format(_T("Md5 file %s not found on server. Contact server owner."),strRawPgzUrl);
      throw CCatalogServerException(CCatalogServerException::ceFindingFile, msg);
   }
   else
   {
      // We have the md5, compare it to file in cache created at download
      CString cachedPgzFile = cacheFolder + GetPgzFilename(m_AppName);

      result = !CheckFileAgainstMd5(cachedPgzFile, strTempMd5File);
   }

   return result;
}


bool CIniCatalogServer::PopulateCatalog(const CString& publisher, IProgressMonitor* pProgress,
                                               const CString& cacheFolder) const
{
   FetchCatalog(pProgress);

   if (m_Catalog.GetFormat(publisher)!=CCatalog::ctPgz)
   {
      CString strMessage(_T("Error - Bad catalog type. Only pgz compressed catalogs are supported on http servers. Please contact the server owner"));
      throw CCatalogServerException(CCatalogServerException::ceFindingFile, strMessage);
   }

   CString pgzFileURL;
   m_Catalog.GetCatalogSettings(publisher, pgzFileURL);

   // First step is to copy the pgzfile and its md5 to our cache folder
   CString pgzFolder(cacheFolder);
   CString pgzCachedFile = cacheFolder + GetPgzFilename(m_AppName);
   CString strPgzFile = CleanUpURL(pgzFileURL,true);

   if(pProgress!=nullptr)
   {
      pProgress->put_Message(0,CComBSTR("Downloading Configuration"));
   }

   bool st = gwOk == this->GetWebFile(strPgzFile, pgzCachedFile);
   if ( st )
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
      throw CCatalogServerException(CCatalogServerException::ceFindingFile, msg);
   }

   // We now should have our file in the cache.
   // Use md5 to check that download was successful
   if( CheckForUpdates(publisher, pProgress, cacheFolder) )
   {
      CString strMessage;
      strMessage.Format(_T("It appears that there was an error downloading the file \"%s\"  The md5 file hash does not match that on the server. Try updating again, and contact the server owner if this error persists"),strPgzFile);

      ::MessageBox(EAFGetMainFrame()->GetSafeHwnd(),strMessage,_T("Error"), MB_OK );
      return false;
   }

   // Now that we have a validated file - decompress it
   TCHAR* sFolder = pgzFolder.GetBuffer(1);
   TCHAR* sFile   = pgzCachedFile.GetBuffer(1);

   if(pProgress!=nullptr)
   {
      pProgress->put_Message(0,CComBSTR("Uncompressing Configuration to local cache."));
   }

   uzErrorType uzst = UnZipPGZ( sFile, sFolder );

   pgzFolder.ReleaseBuffer();
   pgzCachedFile.ReleaseBuffer();

   if(uzst!=uzOk)
   {
      CString msg;
      if (uzst==uzErrorOpeningFile)
      {
         msg.Format(_T("Error opening the file %s."),pgzCachedFile);
      }
      else if (uzst==uzErrorGettingFileList || uzst==uzExtractingFile)
      {
         msg.Format(_T("Error extracting files from %s."),pgzCachedFile);
      }
      else if (uzst==uzNoFilesFound)
      {
         msg.Format(_T("Error - no files found in %s."),pgzCachedFile);
      }

      ::AfxMessageBox(msg,MB_ICONEXCLAMATION | MB_OK);

      return false;
   }

   return true;
}

bool CIniCatalogServer::IsNetworkError() const
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

void CIniCatalogServer::FakeNetworkError(bool bFake) const
{
   m_bFakeError = bFake;
}

bool CIniCatalogServer::TestServer(CString& errorMessage) const
{
   // if we can get the catalog file - good enough
   // copy to a harmless location
   TCHAR buffer[256];
   ::GetTempPath(256,buffer);
   CString cbuf;
   cbuf.Format(_T("%s%s.tmp"),buffer,m_AppName);

   CString catalog_fileurl = CleanUpURL(m_ServerAddress,false);
   catalog_fileurl += GetCatalogFileName(m_AppName);

   gwResult res = GetWebFile(catalog_fileurl, cbuf);
   if ( res==gwInvalidUrl )
   {
      errorMessage.Format(_T("Error parsing URL - Please check the http string for errors"));
      return false;
   }
   else if ( res==gwNotFound )
   {
      errorMessage.Format(_T("Error - Unable to find catalog file: %s on the server."),catalog_fileurl);
      return false;
   }
   else
   {
      ATLASSERT(res==gwOk);
   }

   return true;
}

/// CHttpCatalogServer ///
CHttpCatalogServer::CHttpCatalogServer(LPCTSTR strAppName, const CString& strExt) :
   CIniCatalogServer(strAppName, _T("WSDOT"), srtInternetHttp, _T("dummy"), strExt)
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   CWinApp* pApp = AfxGetApp();
   m_ServerAddress.Format(_T("%s/%s/"), _T("http://www.wsdot.wa.gov/eesc/bridge/software"), pApp->m_pszProfileName);
   Init();
}


CIniCatalogServer::gwResult CHttpCatalogServer::GetWebFile(const CString& strFileURL, const CString& strLocalTargetFile)const
{
	DWORD dwAccessType = PRE_CONFIG_INTERNET_ACCESS;
	DWORD dwHttpRequestFlags = INTERNET_FLAG_EXISTING_CONNECT | INTERNET_FLAG_DONT_CACHE;

	/*string containing the application name that is used to refer
	  client making the request. If this nullptr the frame work will
	  call  the global function AfxGetAppName which returns the application
	  name.*/
	//LPCTSTR pstrAgent = _T("Pgsuper");
	LPCTSTR pstrAgent = m_AppName;

	//the verb we will be using for this connection
	//if nullptr then GET is assumed
	LPCTSTR pstrVerb = _T("GET");
	
	//the address of the url in the request was obtained from
	LPCTSTR pstrReferer = nullptr;

	//Http version we are using; nullptr = HTTP/1.0
	LPCTSTR pstrVersion = nullptr;

	//For the Accept request headers if we need them later on
	LPCTSTR pstrAcceptTypes = nullptr;
	CString szHeaders = _T("Accept: audio/x-aiff, audio/basic, audio/midi, audio/mpeg, audio/wav, image/jpeg, image/gif, image/jpg, image/png, image/mng, image/bmp, text/plain, text/html, text/htm\r\n");

	//Username we will use if a secure site comes into play
	LPCTSTR pstrUserName = nullptr; 
	//The password we will use
	LPCTSTR pstrPassword = nullptr;

	//CInternetSession flags if we need them
	//DWORD dwFlags = INTERNET_FLAG_ASYNC;
	DWORD dwFlags = 0;

	//Proxy setting if we need them
	LPCTSTR pstrProxyName = nullptr;
	LPCTSTR pstrProxyBypass = nullptr;

   // see if url parses before going further
   DWORD dwServiceType;
   CString strServer, strObject;
   INTERNET_PORT nPort;

   BOOL bSuccess = AfxParseURL(strFileURL,dwServiceType,strServer,strObject,nPort);
   if ( !bSuccess || dwServiceType != AFX_INET_SERVICE_HTTP )
   {
      return gwInvalidUrl;
   }

	CInternetSession	session(pstrAgent, 1, dwAccessType, pstrProxyName, pstrProxyBypass, dwFlags);

	//Set any CInternetSession options we  may need
	int ntimeOut = 30;
	session.SetOption(INTERNET_OPTION_CONNECT_TIMEOUT,1000* ntimeOut);
	session.SetOption(INTERNET_OPTION_CONNECT_BACKOFF,1000);
	session.SetOption(INTERNET_OPTION_CONNECT_RETRIES,1);

	//Enable or disable status callbacks
	//session.EnableStatusCallback(FALSE);
   gwResult retVal = gwConnectionError;

	CHttpConnection*	pServer = nullptr;   
	CHttpFile* pFile = nullptr;
	DWORD dwRet;
	try 
   {		
		pServer = session.GetHttpConnection(strServer, nPort, 
			pstrUserName, pstrPassword);
		pFile = pServer->OpenRequest(pstrVerb, strObject, pstrReferer, 
			1, &pstrAcceptTypes, pstrVersion, dwHttpRequestFlags);

		pFile->AddRequestHeaders(szHeaders);
      CString strHeader;
      strHeader.Format(_T("User-Agent: %s/3.3/r/n"),m_AppName);
		pFile->AddRequestHeaders(strHeader, HTTP_ADDREQ_FLAG_ADD_IF_NEW);
		pFile->SendRequest();

		pFile->QueryInfoStatusCode(dwRet);//Check wininet.h for info
										  //about the status codes


		if (dwRet == HTTP_STATUS_DENIED)
		{
			return gwConnectionError;
		}

		if (dwRet == HTTP_STATUS_MOVED ||
			dwRet == HTTP_STATUS_REDIRECT ||
			dwRet == HTTP_STATUS_REDIRECT_METHOD)
		{
			CString strNewAddress;
			//again check wininet.h for info on the query info codes
			//there is alot one can do and re-act to based on these codes
			pFile->QueryInfo(HTTP_QUERY_RAW_HEADERS_CRLF, strNewAddress);
			
			int nPos = strNewAddress.Find(_T("Location: "));
			if (nPos == -1)
			{
				return gwNotFound;
			}

			strNewAddress = strNewAddress.Mid(nPos + 10);
			nPos = strNewAddress.Find('\n');
			if (0 < nPos)
         {
				strNewAddress = strNewAddress.Left(nPos);
         }

			pFile->Close();      
			delete pFile;
			pServer->Close();  
			delete pServer;

			CString strServerName;
			CString strObject;
			INTERNET_PORT nNewPort;
			DWORD dwServiceType;

			if (!AfxParseURL(strNewAddress, dwServiceType, strServerName, strObject, nNewPort))
			{
				return gwInvalidUrl;
			}

			pServer = session.GetHttpConnection(strServerName, nNewPort, 
				pstrUserName, pstrPassword);
			pFile = pServer->OpenRequest(pstrVerb, strObject, 
				pstrReferer, 1, &pstrAcceptTypes, pstrVersion, dwHttpRequestFlags);
			pFile->AddRequestHeaders(szHeaders);
			pFile->SendRequest();

			pFile->QueryInfoStatusCode(dwRet);
			if (dwRet != HTTP_STATUS_OK)
			{
				return gwNotFound;
			}
		}

		if(dwRet == HTTP_STATUS_OK)
      {
         // Copy file
			ULONGLONG len = pFile->GetLength();
			TCHAR buf[2000];
			int numread;
			CFile myfile(strLocalTargetFile, CFile::modeCreate|CFile::modeWrite|CFile::typeBinary);
			while ((numread = pFile->Read(buf,sizeof(buf)/sizeof(TCHAR)-1)) > 0)
			{
				buf[numread] = _T('\0');
				strObject += buf;
				myfile.Write(buf, numread);
				PeekAndPump();
			}
			myfile.Close();

        retVal = gwOk; // only good exit
		}
      else
      {
         retVal = gwNotFound;
      }

		pFile->Close();      
		delete pFile;
		pServer->Close();  
		delete pServer;
		session.Close();

	}
	catch (CInternetException* pEx) 
	{
      // catch any exceptions from WinINet      
		TCHAR szErr[1024];
		szErr[0] = _T('\0');
      if(!pEx->GetErrorMessage(szErr, 1024))
      {
			_tcscpy_s(szErr,1024,_T("Some crazy unknown error"));
      }
//		LOG(_T("File transfer failed!!"));      
		pEx->Delete();
		if(pFile)
      {
			delete pFile;
      }
		if(pServer)
      {
			delete pServer;
      }
		session.Close(); 

      retVal = gwNotFound;
	}

	return retVal;
}

CString CHttpCatalogServer::CleanUpURL(const CString& strURL, bool isFile) const
{
   return CleanHTTPURL(strURL, isFile);
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
   // if the md5 files are missing, assume that an update is pending
   // so start with a default of true
#if defined _DEBUG
   // In the development environment, the source of the workgroup templates folder
   // have CVS information. When PGSuper sync's with this source, the templates
   // are copied to the Application Data folder along with the md5 file that
   // was created considering the CVS information. When PGSuper computes the md5
   // of the cached data, it will always be different because the Application Data
   // folder does not contain the CVS information.
   // 
   // To prevent us from getting asked to update the templates every time the program
   // is run, we don't create any md5 files and assume there is not an updated if the
   // md5 files are missing.
   bool bUpdatePending = false;
#else
   bool bUpdatePending = true;
#endif
   CString cachedMasterLibFile  = cacheFolder + CString(_T("\\")) + GetMasterLibraryFileName();
   CString cachedTemplateFolder = cacheFolder + CString(_T("\\")) + GetTemplateSubFolderName()+ CString(_T("\\"));

   // Master Library File
   CString strMasterLibMD5 = GetLibraryFileName() + _T(".md5");
   CString strLocalMasterLibMD5 = GetLocalMasterLibraryMD5Filename();
   BOOL bMasterLibrarySuccess = ::CopyFile(strMasterLibMD5,strLocalMasterLibMD5,FALSE);

//   LOG(_T("Getting ") << strMasterLibMD5);
   if ( !bMasterLibrarySuccess )
   {
      CString msg;
      msg.Format(_T("Error getting master library file md5 checksum %s"),strMasterLibMD5);
//      LOG(msg);
   }

   // Workgroup Template Folder
   CString strWorkgroupTemplateMD5 = GetTemplateFolderPath() + CString(_T("WorkgroupTemplates.md5"));
   CString strLocalTemplateMD5 = GetLocalWorkgroupTemplateFolderMD5Filename();
   BOOL bWorkgroupTemplateSuccess = ::CopyFile(strWorkgroupTemplateMD5,strLocalTemplateMD5,FALSE);

//   LOG(_T("Getting ") << strWorkgroupTemplateMD5);
   if ( !bWorkgroupTemplateSuccess )
   {
//      LOG(_T("Error getting ") << strWorkgroupTemplateMD5);
   }

   if ( bMasterLibrarySuccess && bWorkgroupTemplateSuccess )
   {
      bUpdatePending = CheckForUpdatesUsingMD5(strLocalMasterLibMD5,strLocalTemplateMD5,cachedMasterLibFile,cachedTemplateFolder);
   }

   return bUpdatePending;
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

/// CFileSystemIniCatalogServer ///
CString CFileSystemIniCatalogServer::CleanUpURL(const CString& strURL, bool isFile) const
{
   CString newstr(strURL);

   if (!isFile)
   {
      // add trailing \ if needed
      int loc = newstr.ReverseFind(_T('\\'));
      if ( loc != newstr.GetLength()-1 )
      {
         newstr += _T("\\");
      }
   }

   return newstr;
}

CIniCatalogServer::gwResult CFileSystemIniCatalogServer::GetWebFile(const CString& strFileURL, const CString& strLocalTargetFile)const
{
   gwResult retVal = gwConnectionError;

   CFileFind libfinder;
   CString strFileName = strFileURL;
   if ( strFileName != _T("") )
   {
      BOOL bFound = libfinder.FindFile(strFileName);
      if (!bFound)
      {
         retVal = gwNotFound;
      }
      else
      {
         BOOL bSuccess = ::CopyFile(strFileURL,strLocalTargetFile,FALSE);
         if (bSuccess)
         {
            retVal = gwOk;
	      }
         else
         {
            retVal = gwNotFound;
         }
      }
   }
   else 
   {
      retVal = gwInvalidUrl;
   }

	return retVal;
}
