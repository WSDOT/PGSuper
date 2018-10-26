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

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperCatalogServer.h"
#include "PGSuperAppPlugin\PGSuperApp.h"
#include "PGSuperTemplateManager.h"
#include "MakePgz\UnzipPgz.h" 
#include <EAF\EAFApp.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// Functions to save and create our servers to/from a string
CPGSuperCatalogServer* CreateCatalogServer(const CString& createString,const CString& strExt)
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
            CFtpPGSuperCatalogServer* psvr = new CFtpPGSuperCatalogServer(strName, strAddress, strExt);
            return psvr;
         }
         else if (strType==_T("HTTP"))
         {
            CHttpPGSuperCatalogServer* psvr = new CHttpPGSuperCatalogServer(strName, strAddress, strExt);
            return psvr;
         }
         else
         {
            ATLASSERT(strType==_T("FILSYS"));
            p = strAddress.Find(_T('|'));
            if ( p != -1 )
            {
               CString strLibraryFileName = strAddress.Left(p);
               CString strTemplateFolder = strAddress.Mid(p+1);
               CFileSystemPGSuperCatalogServer* psvr = new CFileSystemPGSuperCatalogServer(strName, strLibraryFileName, strTemplateFolder, strExt);
               return psvr;
            }
            else
            {
               ATLASSERT(0);
               return NULL;
            }
         }
      }
      else
      {
         ATLASSERT(0);
         return NULL;
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
         CFtpPGSuperCatalogServer* psvr = new CFtpPGSuperCatalogServer(strName, strAddress, strExt);
         return psvr;
      }
      else
      {
         ATLASSERT(0);
         return NULL;
      }
   }
}

CPGSuperCatalogServer* CreateCatalogServer(const CString& strServerName,const CString& createString,const CString& strExt)
{
   int p = createString.Find(_T('|'));
   if ( p != -1 )
   {
      CString strType(createString.Left(p));
      CString strAddress(createString.Mid(p+1));
      if (strType==_T("FTP"))
      {
         CFtpPGSuperCatalogServer* psvr = new CFtpPGSuperCatalogServer(strServerName, strAddress, strExt);
         return psvr;
      }
      else if (strType==_T("HTTP"))
      {
         CHttpPGSuperCatalogServer* psvr = new CHttpPGSuperCatalogServer(strServerName, strAddress, strExt);
         return psvr;
      }
      else
      {
         ATLASSERT(strType==_T("FILSYS"));
         p = strAddress.Find(_T('|'));
         if ( p != -1 )
         {
            CString strLibraryFileName = strAddress.Left(p);
            CString strTemplateFolder = strAddress.Mid(p+1);
            CFileSystemPGSuperCatalogServer* psvr = new CFileSystemPGSuperCatalogServer(strServerName, strLibraryFileName, strTemplateFolder, strExt);
            return psvr;
         }
         else
         {
            ATLASSERT(0);
            return NULL;
         }
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
         CFtpPGSuperCatalogServer* psvr = new CFtpPGSuperCatalogServer(strServerName, strAddress, strExt);
         return psvr;
      }
      else
      {
         ATLASSERT(0);
         return NULL;
      }
   }
}

CString GetCreationString(const CPGSuperCatalogServer* pServer)
{
   CString creastr;
   SharedResourceType type = pServer->GetServerType();
   if (type==srtInternetFtp)
   {
      const CFtpPGSuperCatalogServer* psvr = dynamic_cast<const CFtpPGSuperCatalogServer*>(pServer);
      ATLASSERT(psvr);
      creastr.Format(_T("FTP|%s"),psvr->GetAddress());
   }
   else if (type==srtInternetHttp)
   {
      const CHttpPGSuperCatalogServer* psvr = dynamic_cast<const CHttpPGSuperCatalogServer*>(pServer);
      ATLASSERT(psvr);
      creastr.Format(_T("HTTP|%s"),psvr->GetAddress());
   }
   else if (type==srtLocal)
   {
      const CFileSystemPGSuperCatalogServer* psvr = dynamic_cast<const CFileSystemPGSuperCatalogServer*>(pServer);
      ATLASSERT(psvr);
      creastr.Format(_T("FILSYS|%s|%s"), psvr->GetLibraryFileName(), psvr->GetTemplateFolderPath());
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

static CString GetPgzFilename()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   CWinApp* pApp = AfxGetApp();
   CString strPgzFilename;
   strPgzFilename.Format(_T("%s.pgz"),pApp->m_pszProfileName);
   return strPgzFilename;
}

static CString GetPgzMd5Filename()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   CWinApp* pApp = AfxGetApp();
   CString strPgzMd5;
   strPgzMd5.Format(_T("%s.pgz.md5"),pApp->m_pszProfileName);
   return strPgzMd5;
}

static CString GetTempPgzMD5Filename()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   CWinApp* pApp = AfxGetApp();
   CString strCatalogFile;

   TCHAR buffer[256];
   ::GetTempPath(256,buffer);

   CString cbuf;
   cbuf.Format(_T("%s%s.pgz.md5"),buffer,pApp->m_pszProfileName);
   return cbuf;
}

static CString GetCatalogFileName()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   CWinApp* pApp = AfxGetApp();
   CString strCatalogFile;
   strCatalogFile.Format(_T("%sPackages.ini"),pApp->m_pszProfileName);
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
      strMessage.Format(_T("The program %s is missing. This program is required for PGSuper to access the Internet for Master Library and Workgroup Templates. Please repair PGSuper."),strMD5Deep);

      throw CCatalogServerException(CCatalogServerException::ceMissingMd5Deep, strMessage);
   }

   CString strArg1 = CString(_T("-x ")) + CString(_T("\"")) + md5File + CString(_T("\""));
   CString strArg2 = CString(_T("\"")) + testFile + CString(_T("\""));
   LOG(strMD5Deep << _T(" ") << strArg1 << _T(" ") << strArg2);
#if defined _UNICODE
   intptr_t pgz_result = _wspawnl(_P_WAIT,strMD5Deep,strMD5Deep,strArg1,strArg2,NULL);
#else
   intptr_t pgz_result = _spawnl(_P_WAIT,strMD5Deep,strMD5Deep,strArg1,strArg2,NULL);
#endif
   LOG(_T("pgz_result = ") << pgz_result);

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
      url.Replace(_T("\\"),_T("/"));

   // if it doesn't start with ftp:// add it
   left = url.Left(4);
   left.MakeLower();
   if (left == _T("ftp."))
      url.Insert(0,_T("ftp://"));

   if (!isFile && url.Right(1) != _T("/") )
      url += _T("/");

   return url;
}

static CString CleanHTTPURL(const CString& strURL, bool isFile)
{
   // if this url starts with http, make sure the slashes go the right direction
   CString url = strURL;
   CString left = url.Left(4);
   left.MakeLower();
   if ( left == _T("http") )
      url.Replace(_T("\\"),_T("/"));

   // if it doesn't start with http:// add it
   left = url.Left(5);
   left.MakeLower();
   if (left == _T("http."))
      url.Insert(0,_T("http://"));

   if (!isFile && url.Right(1) != _T("/") )
      url += _T("/");

   return url;
}


BOOL PeekAndPump()
{
	static MSG msg;

	while (::PeekMessage(&msg,NULL,0,0,PM_NOREMOVE)) {
		if (!EAFGetApp()->PumpMessage()) {
			::PostQuitMessage(0);
			return FALSE;
		}	
	}

	return TRUE;
}

/////    CPGSuperCatalogServer  ///////////

CPGSuperCatalogServer::CPGSuperCatalogServer(const CString& name,SharedResourceType type,const CString& strExt):
m_Name(name),
m_ServerType(type),
m_TemplateFileExt(strExt)
{
}

CPGSuperCatalogServer::~CPGSuperCatalogServer()
{
}

CString CPGSuperCatalogServer::GetServerName() const
{
   return m_Name;
}

SharedResourceType CPGSuperCatalogServer::GetServerType() const
{
   return m_ServerType;
}

CString CPGSuperCatalogServer::GetTemplateFileExtension() const
{
   return m_TemplateFileExt;
}

bool CPGSuperCatalogServer::CheckForUpdatesUsingMD5(const CString& strLocalMasterLibMD5,const CString& strLocalWorkgroupTemplateMD5,
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
      strMessage.Format(_T("The program %s is missing. This program is required for PGSuper to access the Internet for Master Library and Workgroup Templates. Please repair PGSuper."),strMD5Deep);

      throw CCatalogServerException(CCatalogServerException::ceMissingMd5Deep, strMessage);
   }

   // Master library
   CString strArg1 = CString(_T("-x ")) + CString(_T("\"")) + strLocalMasterLibMD5 + CString(_T("\""));
   CString strArg2 = CString(_T("\"")) + cachedMasterLibFile + CString(_T("\""));
   LOG(strMD5Deep << _T(" ") << strArg1 << _T(" ") << strArg2);
#if defined _UNICODE
   intptr_t master_lib_result = _wspawnl(_P_WAIT,strMD5Deep,strMD5Deep,strArg1,strArg2,NULL);
#else
   intptr_t master_lib_result = _spawnl(_P_WAIT,strMD5Deep,strMD5Deep,strArg1,strArg2,NULL);
#endif
   LOG(_T("master_lib_result = ") << master_lib_result);

   // Workgroup templates
   CString strWorkgroupTemplateFolderCache = cachedTemplateFolder;

   // remove the last \\ because md5deep doesn't like it
   int loc = strWorkgroupTemplateFolderCache.GetLength()-1;
   if ( strWorkgroupTemplateFolderCache.GetAt(loc) == _T('\\') )
      strWorkgroupTemplateFolderCache.SetAt(loc,_T('\0'));

   strArg1 = CString(_T("-x ")) + CString(_T("\"")) + strLocalWorkgroupTemplateMD5 + CString(_T("\""));
   strArg2 = CString(_T("-r ")) + CString(_T("\"")) + strWorkgroupTemplateFolderCache + CString(_T("\""));
   LOG(strMD5Deep << _T(" ") << strArg1 << _T(" ") << strArg2);
#if defined _UNICODE
   intptr_t workgroup_template_result = _wspawnl(_P_WAIT,strMD5Deep,strMD5Deep,strArg1,strArg2,NULL);
#else
   intptr_t workgroup_template_result = _spawnl(_P_WAIT,strMD5Deep,strMD5Deep,strArg1,strArg2,NULL);
#endif
   LOG(_T("workgroup_template_result = ") << workgroup_template_result);

   if ( master_lib_result != 0 || workgroup_template_result != 0 )
      bUpdatePending = true;

   return bUpdatePending;
}


/// CFtpPGSuperCatalogServer ///
// default ftp to wsdot
CFtpPGSuperCatalogServer::CFtpPGSuperCatalogServer(const CString& strExt):
CPGSuperCatalogServer(_T("WSDOT"),srtInternetFtp,strExt)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   CWinApp* pApp = AfxGetApp();
   m_ServerAddress.Format(_T("%s/%s/"),_T("ftp://ftp.wsdot.wa.gov/public/bridge/software"),pApp->m_pszProfileName);
   Init();
}

CFtpPGSuperCatalogServer::CFtpPGSuperCatalogServer(const CString& name, const CString& address,const CString& strExt):
CPGSuperCatalogServer(name,srtInternetFtp,strExt)
{
   SetAddress(address);
   Init();
}

void CFtpPGSuperCatalogServer::Init()
{
   m_bDoFetchCatalog = true;
   m_bFakeError = false;
}

CString CFtpPGSuperCatalogServer::GetAddress() const
{
   return m_ServerAddress;
}

void CFtpPGSuperCatalogServer::SetAddress(const CString& address)
{
   // make sure our ftp address is good
   m_ServerAddress = CleanFTPURL(address,false);
}

void CFtpPGSuperCatalogServer::FetchCatalog(IProgressMonitor* pProgress, bool toTempfolder) const
{
   if (m_bDoFetchCatalog)
   {
      if (pProgress!=NULL)
         pProgress->put_Message(0, CComBSTR("Reading Library Publishers from the Internet"));

      TCHAR buffer[256];
      ::GetTempPath(256,buffer);

      m_strLocalCatalog = buffer;
      m_strLocalCatalog += GetCatalogFileName();

      if (toTempfolder)
      {
         // not our 'real' catalog file - give it a bogus name
         m_strLocalCatalog += _T(".tmp");
      }

      // pull the catalog down from the one and only catalog server
      // (we will support general catalog servers later)
      CString catURL = m_ServerAddress+GetCatalogFileName();
      try
      {
         DWORD dwServiceType;
         CString strServer, strObject;
         INTERNET_PORT nPort;

         BOOL bSuccess = AfxParseURL(catURL,dwServiceType,strServer,strObject,nPort);
         ATLASSERT( bSuccess );

         // the URL is bogus or it isn't for an FTP service
         if ( !bSuccess || dwServiceType != AFX_INET_SERVICE_FTP )
         {
            CString msg;
            msg.Format(_T("Error parsing catalog URL: %s"), catURL);
            throw CCatalogServerException(CCatalogServerException::ceGettingCatalogFile, msg);
         }
         else
         {
            // download the catalog file
            CInternetSession inetSession;
            CFtpConnection* pFTP = inetSession.GetFtpConnection(strServer);

            CFtpFileFind ftpFind(pFTP);
            if ( !ftpFind.FindFile(strObject) || !pFTP->GetFile(strObject,m_strLocalCatalog,FALSE,FILE_ATTRIBUTE_NORMAL,FTP_TRANSFER_TYPE_ASCII | INTERNET_FLAG_RELOAD) )
            {
               // could not find or get the file (it may not be on the server)
               CString msg;
               msg.Format(_T("Could not find catalog file on server at: %s"), catURL);
               throw CCatalogServerException(CCatalogServerException::ceGettingCatalogFile, msg);
            }
         }
      }
      catch (CInternetException* pException)
      {
         pException->Delete();

         CString msg;
         msg.Format(_T("An unknown ftp error occurred while trying to download the catalog file at: %s"),catURL);
         throw CCatalogServerException(CCatalogServerException::ceGettingCatalogFile,msg);
      }

      // We have the catalog, initialize the catalog parser
      CString strVersion = theApp.GetVersion(true);

      if (! m_Catalog.Init(m_strLocalCatalog,strVersion) )
      {
         CString msg;
         msg.Format(_T("Error occurred while reading the catalog file at: %s"),catURL);
         throw CCatalogServerException(CCatalogServerException::ceGettingCatalogFile,msg);
      }

      m_bDoFetchCatalog = false;
   }
}

std::vector<CString> CFtpPGSuperCatalogServer::GetPublishers() const
{
   std::vector<CString> items;

   FetchCatalog(NULL);

   items = m_Catalog.GetPublishers();

   return items;
}

bool CFtpPGSuperCatalogServer::DoesPublisherExist(const CString& publisher) const
{
   return m_Catalog.DoesPublisherExist(publisher);
}


CString CFtpPGSuperCatalogServer::GetMasterLibraryURL(const CString& publisher) const
{
   FetchCatalog(NULL);

   // catalog parser does the work
   CPGSuperCatalog::Format type = m_Catalog.GetFormat(publisher);

   CString strMasterLibrary;
   if (type==CPGSuperCatalog::ctOriginal)
   {
      CString strWorkgroupTemplates;
      m_Catalog.GetCatalogSettings(publisher, strMasterLibrary, strWorkgroupTemplates);
   }
   else
   {
      ATLASSERT(type==CPGSuperCatalog::ctPgz);
      CString strWorkgroupTemplates;
      m_Catalog.GetCatalogSettings(publisher, strMasterLibrary);
   }

   return strMasterLibrary;
}

CString CFtpPGSuperCatalogServer::GetWebLink(const CString& publisher) const
{
   FetchCatalog(NULL);

   CString url = m_Catalog.GetWebLink(publisher);

   return url;
}


bool CFtpPGSuperCatalogServer::CheckForUpdates(const CString& publisher, IProgressMonitor* pProgress, 
                                               const CString& cacheFolder) const
{
   FetchCatalog(pProgress);

   if (!m_Catalog.DoesPublisherExist(publisher))
   {
      // we are screwed if the publisher doesn't match. This probably will only
      // happen if registry values get trashed in the App class
      ATLASSERT(0);
      CString msg;
      msg.Format(_T("The publisher %s was not found in the current catalog. This appears to be a programing bug."), publisher);
      throw CCatalogServerException(CCatalogServerException::ceGettingCatalogFile, msg);
   }

   // all depends on type of catalog
   CPGSuperCatalog::Format type = m_Catalog.GetFormat(publisher);

   if (type==CPGSuperCatalog::ctOriginal)
   {
      return CheckForUpdatesOriginal(publisher, pProgress, cacheFolder);
   }
   else
   {
      return CheckForUpdatesPgz(publisher, pProgress, cacheFolder);
   }
}

bool CFtpPGSuperCatalogServer::CheckForUpdatesOriginal(const CString& publisher, IProgressMonitor* pProgress, 
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
   CFtpConnection* pFTP = NULL;

   CString strLocalFile;
   CString strLocalFolder;
   try
   {
      // Master library
      pFTP = inetSession.GetFtpConnection(strMasterLibraryServer);
      CFtpFileFind master_library_file_find(pFTP);

      CString strMasterLibMD5 = strMasterLibraryObject + CString(_T(".md5"));
      strLocalFile = GetLocalMasterLibraryMD5Filename();
      
      LOG(_T("Getting ") << strMasterLibMD5 << _T(" from ") << strMasterLibraryServer);

      if ( !master_library_file_find.FindFile(strMasterLibMD5) || !pFTP->GetFile(strMasterLibMD5,strLocalFile,FALSE,FILE_ATTRIBUTE_NORMAL,FTP_TRANSFER_TYPE_ASCII | INTERNET_FLAG_RELOAD) )
      {
         pFTP->Close();
         delete pFTP;

         CString msg;
         msg.Format(_T("Error getting %s from %s."), strMasterLibMD5, strMasterLibraryServer);
         LOG(msg);
         throw CCatalogServerException(CCatalogServerException::ceFindingFile, msg);
      }

      pFTP->Close();
      delete pFTP;

      // Workgroup Template
      pFTP = inetSession.GetFtpConnection(strWorkgroupTemplatesServer);
      CFtpFileFind workgroup_template_file_find(pFTP);

      CString strWorkgroupTemplateMD5 = strWorkgroupTemplatesObject + CString(_T("WorkgroupTemplates.md5"));
      strLocalFolder = GetLocalWorkgroupTemplateFolderMD5Filename();

      LOG(_T("Getting ") << strWorkgroupTemplateMD5 << _T(" from ") << strWorkgroupTemplatesServer);

      if ( !workgroup_template_file_find.FindFile(strWorkgroupTemplateMD5) || !pFTP->GetFile(strWorkgroupTemplateMD5,strLocalFolder,FALSE,FILE_ATTRIBUTE_NORMAL,FTP_TRANSFER_TYPE_ASCII | INTERNET_FLAG_RELOAD) )
      {
         pFTP->Close();
         delete pFTP;

         CString msg;
         msg.Format(_T("Error getting %s from %s."), strWorkgroupTemplateMD5, strWorkgroupTemplatesServer);
         LOG(msg);
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
      LOG(msg);
      throw CCatalogServerException(CCatalogServerException::ceFindingFile, msg);
   }

   bool bUpdatePending = CheckForUpdatesUsingMD5(strLocalFile,strLocalFolder,cachedMasterLibFile,cachedTemplateFolder);

   return bUpdatePending;
}

bool CFtpPGSuperCatalogServer::CheckForUpdatesPgz(const CString& publisher, IProgressMonitor* pProgress, 
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

   if(pProgress!=NULL)
      pProgress->put_Message(0,CComBSTR("Reading md5 hash from server"));

   BOOL bSuccess = AfxParseURL(strRawPgzUrl,dwServiceType,strPgzServer,strPgzObject,nPort);
   if ( !bSuccess || dwServiceType != AFX_INET_SERVICE_FTP)
   {
      CString strMessage;
      strMessage.Format(_T("The published md5 file location, %s, for publisher %s has invalid format. Library and Template settings will be restored to original defaults."),strRawPgzUrl,publisher);
      throw CCatalogServerException(CCatalogServerException::ceParsingURL, strMessage);
   }

   // name of md5p copied to temp folder
   CString strTempMd5File = GetTempPgzMD5Filename();

   // connect to the internet and get the md5 file
   CInternetSession inetSession;
   CFtpConnection* pFTP = NULL;

   try
   {
      pFTP = inetSession.GetFtpConnection(strPgzServer);
      CFtpFileFind pgz_file_find(pFTP);
      
      LOG(_T("Getting ") << strPgzObject << _T(" from ") << strPgzServer);

      if ( !pgz_file_find.FindFile(strPgzObject) || !pFTP->GetFile(strPgzObject,strTempMd5File,FALSE,FILE_ATTRIBUTE_NORMAL,FTP_TRANSFER_TYPE_ASCII | INTERNET_FLAG_RELOAD) )
      {
         pFTP->Close();
         delete pFTP;

         LOG(_T("Error getting ") << strPgzObject << _T(" from ") << strPgzServer);
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
      LOG(msg);
      throw CCatalogServerException(CCatalogServerException::ceFindingFile, msg);
   }

   // We have the md5, compare it to file in cache created at download
   CString cachedPgzFile = cacheFolder + GetPgzFilename();

   // we need to update if it doesn't compare
   result = !CheckFileAgainstMd5(cachedPgzFile, strTempMd5File);

   return result;
}


bool CFtpPGSuperCatalogServer::PopulateCatalog(const CString& publisher, IProgressMonitor* pProgress,
                                               const CString& cacheFolder) const
{
   FetchCatalog(pProgress);

   // all depends on type of catalog
   CPGSuperCatalog::Format type = m_Catalog.GetFormat(publisher);

   if (type==CPGSuperCatalog::ctOriginal)
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
      ATLASSERT(type==CPGSuperCatalog::ctPgz);

      bool st = PopulatePgz(publisher, pProgress, cacheFolder);
      if (!st)
      {
         return false;
      }
   }

   return true;
}

bool CFtpPGSuperCatalogServer::PopulateLibraryFile(IProgressMonitor* pProgress,const CString& masterLibraryURL, const CString& cachedMasterLibFile) const
{
   CString strMasterLibraryFile = CleanFTPURL(masterLibraryURL,true);

   DWORD dwServiceType;
   CString strServer, strObject;
   INTERNET_PORT nPort;
   BOOL bSuccess = AfxParseURL(strMasterLibraryFile,dwServiceType,strServer,strObject,nPort);
   ATLASSERT( bSuccess && dwServiceType == AFX_INET_SERVICE_FTP);

   CInternetSession inetSession;
   CFtpConnection* pFTP = 0;

   try
   {
      pFTP = inetSession.GetFtpConnection(strServer);
      CFtpFileFind file_find(pFTP);

      if(pProgress!=NULL)
         pProgress->put_Message(0,CComBSTR("Downloading the Master Library"));

      if ( file_find.FindFile(strObject) && pFTP->GetFile(strObject,cachedMasterLibFile,FALSE,FILE_ATTRIBUTE_NORMAL,FTP_TRANSFER_TYPE_ASCII | INTERNET_FLAG_RELOAD) )
      {
         if(pProgress!=NULL)
            pProgress->put_Message(0,CComBSTR("Master Library downloaded successfully"));
      }
      else
      {
         pFTP->Close();
         delete pFTP;

         CString msg;
         msg.Format(_T("An error occured while downloading master library file %s."),strMasterLibraryFile);

         if(pProgress!=NULL)
            pProgress->put_Message(0,CComBSTR(msg));

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
      LOG(msg);
      throw CCatalogServerException(CCatalogServerException::ceFindingFile, msg);
   }

   pFTP->Close();
   delete pFTP;

   return true;
}

bool CFtpPGSuperCatalogServer::PopulateTemplateFolder(IProgressMonitor* pProgress,const CString& templateFolderURL, const CString& cachedTemplateFolder) const
{
   // download templates from the FTP server
   CString strWorkgroupTemplateFolder = CleanFTPURL(templateFolderURL,false);

   DWORD dwServiceType;
   CString strServer, strObject;
   CString strLocalFile;
   INTERNET_PORT nPort;

   BOOL bSuccess = AfxParseURL(strWorkgroupTemplateFolder,dwServiceType,strServer,strObject,nPort);
   ATLASSERT( bSuccess && dwServiceType == AFX_INET_SERVICE_FTP); // this should be tested elsewhere

   CInternetSession inetSession;

   ::CreateDirectory(cachedTemplateFolder,NULL);

   CFtpConnection* pFTP = 0;

   try
   {
      pFTP = inetSession.GetFtpConnection(strServer);

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
      LOG(msg);
      throw CCatalogServerException(CCatalogServerException::ceFindingFile, msg);
   }

   pFTP->Close();
   delete pFTP;

   return true;
}

bool CFtpPGSuperCatalogServer::PopulatePgz(const CString& publisher, IProgressMonitor* pProgress, const CString& cacheFolder) const
{
   CString pgzFileURL;
   m_Catalog.GetCatalogSettings(publisher, pgzFileURL);

   // First step is to copy the pgzfile and its md5 to our cache folder
   CString pgzFolder(cacheFolder);
   CString pgzCachedFile = cacheFolder + GetPgzFilename();
   CString strPgzFile = CleanFTPURL(pgzFileURL,true);

   DWORD dwServiceType;
   CString strServer, strObject;
   INTERNET_PORT nPort;
   BOOL bSuccess = AfxParseURL(strPgzFile,dwServiceType,strServer,strObject,nPort);
   ATLASSERT( bSuccess && dwServiceType == AFX_INET_SERVICE_FTP);

   CInternetSession inetSession;
   CFtpConnection* pFTP = 0;

   try
   {
      pFTP = inetSession.GetFtpConnection(strServer);
      CFtpFileFind file_find(pFTP);

      if(pProgress!=NULL)
         pProgress->put_Message(0,CComBSTR("Downloading Compressed Library/Template file"));

      if ( file_find.FindFile(strObject) && pFTP->GetFile(strObject,pgzCachedFile,FALSE,FILE_ATTRIBUTE_NORMAL,FTP_TRANSFER_TYPE_BINARY | INTERNET_FLAG_RELOAD) )
      {
         if(pProgress!=NULL)
            pProgress->put_Message(0,CComBSTR("Compressed Library/Template file downloaded successfully"));
      }
      else
      {
         if(pProgress!=NULL)
            pProgress->put_Message(0,CComBSTR("Error downloading the Compressed Library/Template File"));

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
      strMessage.Format(_T("Error opening the Compressed Library/Template file from %s."),strPgzFile);

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

   if(pProgress!=NULL)
      pProgress->put_Message(0,CComBSTR("Uncompressing Library and Templates to local cache."));

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

bool CFtpPGSuperCatalogServer::IsNetworkError() const
{
   if (m_bFakeError)
      return true;

   try
   {
      FetchCatalog(NULL,true);
   }
   catch(CCatalogServerException exc)
   {
      LOG(exc.GetErrorMessage());
      return true;
   }
   catch(...)
   {
      ATLASSERT(0);
      LOG(_T("Unknown network error in IsNetworkError"));
      return true;
   }

   return false; // we fetched successfully
}

void CFtpPGSuperCatalogServer::FakeNetworkError(bool bFake) const
{
   m_bFakeError = bFake;
}

bool CFtpPGSuperCatalogServer::TestServer(CString& errorMessage) const
{
   DWORD dwServiceType;
   CString strServer, strObject;
   INTERNET_PORT nPort;

   CString catalog_file = m_ServerAddress+GetCatalogFileName();

   BOOL bSuccess = AfxParseURL(catalog_file,dwServiceType,strServer,strObject,nPort);
   if ( !bSuccess || dwServiceType != AFX_INET_SERVICE_FTP )
   {
      errorMessage.Format(_T("Error parsing URL - Please check the ftp string for errors"));
      return false;
   }

   // See if we find the catalog file
   CInternetSession inetSession;
   CFtpConnection* pFTP = inetSession.GetFtpConnection(strServer);

   CFtpFileFind ftpFind(pFTP);
   if ( !ftpFind.FindFile(strObject))
   {
      errorMessage.Format(_T("Error - Unable to find catalog file: %s on the server."),strObject);
      return false;
   }

   return true;
}

/// CHttpPGSuperCatalogServer ///

CHttpPGSuperCatalogServer::CHttpPGSuperCatalogServer(const CString& name, const CString& address,const CString& strExt):
CPGSuperCatalogServer(name,srtInternetHttp,strExt)
{
   SetAddress(address);
   Init();
}

void CHttpPGSuperCatalogServer::Init()
{
   m_bFakeError = false;
   m_bDoFetchCatalog = true;
}

CString CHttpPGSuperCatalogServer::GetAddress() const
{
   return m_ServerAddress;
}

void CHttpPGSuperCatalogServer::SetAddress(const CString& address)
{
   // make sure our address is good
   m_ServerAddress = CleanHTTPURL(address,false);
}

void CHttpPGSuperCatalogServer::FetchCatalog(IProgressMonitor* pProgress) const
{
   if (m_bDoFetchCatalog)
   {
      // create a progress window
      if (pProgress!=NULL)
         pProgress->put_Message(0,CComBSTR("Reading Library Publishers from the Internet"));

      TCHAR buffer[256];
      ::GetTempPath(256,buffer);

      m_strLocalCatalog = buffer;
      m_strLocalCatalog += GetCatalogFileName();

      CString catalogURL = m_ServerAddress+GetCatalogFileName();

      gwResult gwres = GetWebFile(catalogURL, m_strLocalCatalog);
      if ( gwres==gwOk )
      {
         // we have the catalog, initialize the catalog parser
         CString strVersion = theApp.GetVersion(true);

         if (! m_Catalog.Init(m_strLocalCatalog,strVersion) )
         {
            CString msg;
            msg.Format(_T("Error occurred while reading the catalog file at: %s"),catalogURL);
            throw CCatalogServerException(CCatalogServerException::ceGettingCatalogFile,msg);
         }
      }
      else
      {
         CString msg;
         msg.Format(_T("Error parsing catalog URL: %s"), catalogURL);
         throw CCatalogServerException(CCatalogServerException::ceGettingCatalogFile, msg);
      }

      m_bDoFetchCatalog = false;
   }
};

std::vector<CString> CHttpPGSuperCatalogServer::GetPublishers() const
{
   std::vector<CString> items;

   FetchCatalog(NULL);

   items = m_Catalog.GetPublishers();

   return items;
}

bool CHttpPGSuperCatalogServer::DoesPublisherExist(const CString& publisher) const
{
   return m_Catalog.DoesPublisherExist(publisher);
}

CString CHttpPGSuperCatalogServer::GetMasterLibraryURL(const CString& publisher) const
{
   FetchCatalog(NULL);

   // catalog parser does the work
   ATLASSERT(m_Catalog.GetFormat(publisher)==CPGSuperCatalog::ctPgz);
   CString pgzUrl;
   m_Catalog.GetCatalogSettings(publisher, pgzUrl);

   return pgzUrl;
}

CString CHttpPGSuperCatalogServer::GetWebLink(const CString& publisher) const
{
   FetchCatalog(NULL);

   CString url = m_Catalog.GetWebLink(publisher);

   return url;
}

bool CHttpPGSuperCatalogServer::CheckForUpdates(const CString& publisher, IProgressMonitor* pProgress, 
                                               const CString& cacheFolder) const
{
   FetchCatalog(pProgress);

   if (!m_Catalog.DoesPublisherExist(publisher))
   {
      // we are screwed if the publisher doesn't match. This probably will only
      // happen if registry values get trashed in the App class
      ATLASSERT(0);
      CString msg;
      msg.Format(_T("The publisher %s was not found in the current catalog. This appears to be a programing bug."), publisher);
      throw CCatalogServerException(CCatalogServerException::ceGettingCatalogFile, msg);
   }

   // name of md5 on server
   CString strRawPgzUrl;
   m_Catalog.GetCatalogSettings(publisher, strRawPgzUrl);
   strRawPgzUrl += CString(_T(".md5"));

   // name of md5p to be copied to temp folder
   CString strTempMd5File = GetTempPgzMD5Filename();

   if(pProgress!=NULL)
      pProgress->put_Message(0,CComBSTR("Reading md5 hash from server"));

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
      CString cachedPgzFile = cacheFolder + GetPgzFilename();

      result = !CheckFileAgainstMd5(cachedPgzFile, strTempMd5File);
   }

   return result;
}


bool CHttpPGSuperCatalogServer::PopulateCatalog(const CString& publisher, IProgressMonitor* pProgress,
                                               const CString& cacheFolder) const
{
   FetchCatalog(pProgress);

   if (m_Catalog.GetFormat(publisher)!=CPGSuperCatalog::ctPgz)
   {
      CString strMessage(_T("Error - Bad catalog type. Only pgz compressed catalogs are supported on http servers. Please contact the server owner"));
      throw CCatalogServerException(CCatalogServerException::ceFindingFile, strMessage);
   }

   CString pgzFileURL;
   m_Catalog.GetCatalogSettings(publisher, pgzFileURL);

   // First step is to copy the pgzfile and its md5 to our cache folder
   CString pgzFolder(cacheFolder);
   CString pgzCachedFile = cacheFolder + GetPgzFilename();
   CString strPgzFile = CleanHTTPURL(pgzFileURL,true);

   if(pProgress!=NULL)
      pProgress->put_Message(0,CComBSTR("Downloading Compressed Library/Template file"));

   bool st = gwOk == this->GetWebFile(strPgzFile, pgzCachedFile);
   if ( st )
   {
      if(pProgress!=NULL)
         pProgress->put_Message(0,CComBSTR("Compressed Library/Template file downloaded successfully"));
   }
   else
   {
      CString msg;
      msg.Format(_T("Error downloading the Compressed Library/Template File %s"),strPgzFile);
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

   if(pProgress!=NULL)
      pProgress->put_Message(0,CComBSTR("Uncompressing Library and Templates to local cache."));

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

bool CHttpPGSuperCatalogServer::IsNetworkError() const
{
   if (m_bFakeError)
      return true;
   else
   {
      CString msg;
      return !TestServer(msg);
   }
}

void CHttpPGSuperCatalogServer::FakeNetworkError(bool bFake) const
{
   m_bFakeError = bFake;
}

bool CHttpPGSuperCatalogServer::TestServer(CString& errorMessage) const
{
   // if we can get the catalog file - good enough
   // copy to a harmless location
   TCHAR buffer[256];
   ::GetTempPath(256,buffer);
   CString cbuf;
   cbuf.Format(_T("%sPGSuper.tmp"),buffer);

   CString catalog_fileurl = m_ServerAddress+GetCatalogFileName();

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

CHttpPGSuperCatalogServer::gwResult CHttpPGSuperCatalogServer::GetWebFile(const CString& strFileURL, const CString& strLocalTargetFile)const
{
	DWORD dwAccessType = PRE_CONFIG_INTERNET_ACCESS;
	DWORD dwHttpRequestFlags = INTERNET_FLAG_EXISTING_CONNECT | INTERNET_FLAG_DONT_CACHE;

	/*string containing the application name that is used to refer
	  client making the request. If this NULL the frame work will
	  call  the global function AfxGetAppName which returns the application
	  name.*/
	LPCTSTR pstrAgent = _T("Pgsuper");

	//the verb we will be using for this connection
	//if NULL then GET is assumed
	LPCTSTR pstrVerb = _T("GET");
	
	//the address of the url in the request was obtained from
	LPCTSTR pstrReferer = NULL;

	//Http version we are using; NULL = HTTP/1.0
	LPCTSTR pstrVersion = NULL;

	//For the Accept request headers if we need them later on
	LPCTSTR pstrAcceptTypes = NULL;
	CString szHeaders = _T("Accept: audio/x-aiff, audio/basic, audio/midi, audio/mpeg, audio/wav, image/jpeg, image/gif, image/jpg, image/png, image/mng, image/bmp, text/plain, text/html, text/htm\r\n");

	//Username we will use if a secure site comes into play
	LPCTSTR pstrUserName = NULL; 
	//The password we will use
	LPCTSTR pstrPassword = NULL;

	//CInternetSession flags if we need them
	//DWORD dwFlags = INTERNET_FLAG_ASYNC;
	DWORD dwFlags = NULL;

	//Proxy setting if we need them
	LPCTSTR pstrProxyName = NULL;
	LPCTSTR pstrProxyBypass = NULL;

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

	CHttpConnection*	pServer = NULL;   
	CHttpFile* pFile = NULL;
	DWORD dwRet;
	try 
   {		
		pServer = session.GetHttpConnection(strServer, nPort, 
			pstrUserName, pstrPassword);
		pFile = pServer->OpenRequest(pstrVerb, strObject, pstrReferer, 
			1, &pstrAcceptTypes, pstrVersion, dwHttpRequestFlags);

		pFile->AddRequestHeaders(szHeaders);
		pFile->AddRequestHeaders(_T("User-Agent: PGSuper/3.3\r\n"), HTTP_ADDREQ_FLAG_ADD_IF_NEW);
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
			if (nPos > 0)
				strNewAddress = strNewAddress.Left(nPos);

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
			_tcscpy_s(szErr,1024,_T("Some crazy unknown error"));
		LOG(_T("File transfer failed!!"));      
		pEx->Delete();
		if(pFile)
			delete pFile;
		if(pServer)
			delete pServer;
		session.Close(); 

      retVal = gwNotFound;
	}

	return retVal;
}


/// CFileSystemPGSuperCatalogServer ///
CFileSystemPGSuperCatalogServer::CFileSystemPGSuperCatalogServer(const CString& name, const CString& libraryFileName, const CString& templateFilePath,const CString& strExt):
CPGSuperCatalogServer(name,srtLocal,strExt),
m_LibraryFileName(libraryFileName),
m_TemplateFilePath(templateFilePath),
m_bFakeError(false)
{
}

CString CFileSystemPGSuperCatalogServer::GetLibraryFileName() const
{
   return m_LibraryFileName;
}

CString CFileSystemPGSuperCatalogServer::GetTemplateFolderPath() const
{
   return m_TemplateFilePath;
}

std::vector<CString> CFileSystemPGSuperCatalogServer::GetPublishers() const
{
   // file systems only have one publisher - just return name
   std::vector<CString> vec;
   vec.push_back(this->GetServerName());
   return vec;
}

bool CFileSystemPGSuperCatalogServer::DoesPublisherExist(const CString& publisher) const
{
   CString name = GetServerName();
   return publisher == name;
}

CString CFileSystemPGSuperCatalogServer::GetMasterLibraryURL(const CString& publisher) const
{
   return GetLibraryFileName();
}

CString CFileSystemPGSuperCatalogServer::GetWebLink(const CString& publisher) const
{
   // file system has now web links
   return CString();
}

bool CFileSystemPGSuperCatalogServer::CheckForUpdates(const CString& publisher, IProgressMonitor* pProgress, 
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

   LOG(_T("Getting ") << strMasterLibMD5);
   if ( !bMasterLibrarySuccess )
   {
      CString msg;
      msg.Format(_T("Error getting master library file md5 checksum %s"),strMasterLibMD5);
      LOG(msg);
   }

   // Workgroup Template Folder
   CString strWorkgroupTemplateMD5 = GetTemplateFolderPath() + CString(_T("WorkgroupTemplates.md5"));
   CString strLocalTemplateMD5 = GetLocalWorkgroupTemplateFolderMD5Filename();
   BOOL bWorkgroupTemplateSuccess = ::CopyFile(strWorkgroupTemplateMD5,strLocalTemplateMD5,FALSE);

   LOG(_T("Getting ") << strWorkgroupTemplateMD5);
   if ( !bWorkgroupTemplateSuccess )
   {
      LOG(_T("Error getting ") << strWorkgroupTemplateMD5);
   }

   if ( bMasterLibrarySuccess && bWorkgroupTemplateSuccess )
   {
      bUpdatePending = CheckForUpdatesUsingMD5(strLocalMasterLibMD5,strLocalTemplateMD5,cachedMasterLibFile,cachedTemplateFolder);
   }

   return bUpdatePending;
}

bool CFileSystemPGSuperCatalogServer::PopulateCatalog(const CString& publisher, IProgressMonitor* pProgress,
                                                      const CString& cacheFolder) const
{

   CString cachedMasterLibFile  = cacheFolder + CString(_T("\\")) + GetMasterLibraryFileName();
   CString cachedTemplateFolder = cacheFolder + CString(_T("\\")) + GetTemplateSubFolderName()+ CString(_T("\\"));

   // copy the network file to the cache
   CString localMasterLibraryFile = GetLibraryFileName();

   if ( localMasterLibraryFile != _T("") )
   {
      if(pProgress != NULL)
         pProgress->put_Message(0,CComBSTR("Copying the Master Library"));

      BOOL bSuccess = ::CopyFile(localMasterLibraryFile,cachedMasterLibFile,FALSE);
      if ( !bSuccess )
      {
         if(pProgress!=NULL)
            pProgress->put_Message(0,CComBSTR("Error copying the Master Library"));

         CString strMessage;
         strMessage.Format(_T("Error opening Master library file from %s"),localMasterLibraryFile);

         AfxMessageBox(strMessage,MB_ICONEXCLAMATION | MB_OK);

         return false;
      }
      else
      {
         if(pProgress!=NULL)
            pProgress->put_Message(0,CComBSTR("Master Library copied successfully"));
      }
   }

   // Next deal with templates
   BOOL bst = ::CreateDirectory(cachedTemplateFolder,NULL);
   if ( bst==0 )
   {
      CString strMessage;
      strMessage.Format(_T("Error creating Template Folder at %s"),cachedTemplateFolder);

      if(pProgress!=NULL)
         pProgress->put_Message(0,CComBSTR(strMessage));

      AfxMessageBox(strMessage,MB_ICONEXCLAMATION | MB_OK);
      return false;
   }

   CString localTemplateFolder = GetTemplateFolderPath();
   CFileTemplateManager templateMgr(GetTemplateFileExtension());
   templateMgr.GetTemplates(localTemplateFolder,cachedTemplateFolder,pProgress);

   return true;
}

bool CFileSystemPGSuperCatalogServer::IsNetworkError() const
{
   CString msg;
   return !TestServer(msg);
}

void CFileSystemPGSuperCatalogServer::FakeNetworkError(bool bFake) const
{
   m_bFakeError = bFake;
}

bool CFileSystemPGSuperCatalogServer::TestServer(CString& errorMessage) const
{
   if (m_bFakeError)
      return false;

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
