///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright (C) 1999  Washington State Department of Transportation
//                     Bridge and Structures Office
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

// PGSuperCatalog.cpp : implementation file
//

#include "stdafx.h"
#include "pgsuper.h"
#include "PGSuperCatalog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/*
class CCallbackInternetSession : public CInternetSession
{
public:
   CCallbackInternetSession(WBFLTools::IProgressMonitorPtr wndProgress);
   ~CCallbackInternetSession();

   virtual void OnStatusCallback(DWORD dwContext,DWORD dwInternetStatus,LPVOID lpvStatusInformation,DWORD dwStatusInformationLength);

private:
   WBFLTools::IProgressMonitorPtr m_wndProgress;
};

CCallbackInternetSession::CCallbackInternetSession(WBFLTools::IProgressMonitorPtr wndProgress)
{
   m_wndProgress = wndProgress;
   EnableStatusCallback();
}

CCallbackInternetSession::~CCallbackInternetSession()
{
   EnableStatusCallback(FALSE);
}

void CCallbackInternetSession::OnStatusCallback(DWORD dwContext,DWORD dwInternetStatus,LPVOID lpvStatusInformation,DWORD dwStatusInformationLength)
{
   AFX_MANAGE_STATE( AfxGetAppModuleState() );
   CInternetSession::OnStatusCallback(dwContext,dwInternetStatus,lpvStatusInformation,dwStatusInformationLength);

   CString strMsg;
   switch(dwInternetStatus)
   {
   case INTERNET_STATUS_RESOLVING_NAME:
      strMsg.FormatMessage("Resolving %s",(const char*)lpvStatusInformation);
      break;

   case INTERNET_STATUS_NAME_RESOLVED:
      strMsg.FormatMessage("%s resolved",(const char*)lpvStatusInformation);
      break;

   case INTERNET_STATUS_CONNECTING_TO_SERVER:
      strMsg.FormatMessage("Connecting to %s",(const char*)lpvStatusInformation);
      break;

   case INTERNET_STATUS_CONNECTED_TO_SERVER:
      strMsg.FormatMessage("Connected to %s",(const char*)lpvStatusInformation);
      break;

   case INTERNET_STATUS_SENDING_REQUEST:
      strMsg = "Sending request";
      break;

   case INTERNET_STATUS_REQUEST_SENT:
      strMsg = "Request sent";
      break;

   case INTERNET_STATUS_RECEIVING_RESPONSE:
      strMsg = "Receiving response";
      break;

   case INTERNET_STATUS_RESPONSE_RECEIVED:
      strMsg = "Response received";
      break;

   case INTERNET_STATUS_CLOSING_CONNECTION:
      strMsg = "Closing connection";
      break;

   case INTERNET_STATUS_CONNECTION_CLOSED:
      strMsg = "Connection closed";
      break;

   case INTERNET_STATUS_HANDLE_CREATED:
      break;

   case INTERNET_STATUS_HANDLE_CLOSING:
      break;

   case INTERNET_STATUS_REQUEST_COMPLETE:
      break;
   }

   if ( strMsg != "" )
   {
      bstr_t bstrMsg(strMsg.AllocSysString());
      m_wndProgress->put_Message(0,bstrMsg);
   }
}
*/
////////////////////////////////////////////////////
CPGSuperCatalog::CPGSuperCatalog(const char* strName,const char* strServer,const char* file)
{
   SetCatalogServer(strName,strServer);
   SetCatalogFile(file);

   m_bFetch = true;
   m_bFakeError = false;
   m_bError = false;
}

void CPGSuperCatalog::SetCatalogServer(const char* strName,const char* strServer)
{
   m_strName = strName;
   m_strServer = strServer;

   CString left = m_strServer.Left(3);
   left.MakeLower();
   if ( left == "ftp" )
      m_strServer.Replace("\\","/");

   left = m_strServer.Left(4);
   left.MakeLower();
   if (left == "ftp.")
      m_strServer.Insert(0,"ftp://");

   if ( m_strServer.Right(1) != "/" )
      m_strServer += "/";

   m_bFetch = true;
}

void CPGSuperCatalog::SetCatalogFile(const char* file)
{
   m_strCatalog = file;
   m_bFetch = true;
}

CString CPGSuperCatalog::GetCatalogName() const
{
   return m_strName;
}

CString CPGSuperCatalog::GetCatalogServer() const
{
   return m_strServer;
}

CString CPGSuperCatalog::GetCatalogFile() const
{
   return m_strCatalog;
}

bool CPGSuperCatalog::Fetch()
{
   // create a progress window
   CComPtr<IProgressMonitorWindow> wndProgress;
   wndProgress.CoCreateInstance(CLSID_ProgressMonitorWindow);
   wndProgress->put_HasGauge(VARIANT_FALSE);
   wndProgress->put_HasCancel(VARIANT_FALSE);
   wndProgress->Show(CComBSTR("Reading Library Publishers from the Internet"));

   char buffer[256];
   ::GetTempPath(256,buffer);

   m_strLocalCatalog = buffer;
   m_strLocalCatalog += m_strCatalog;

   bool bRetVal = true;

   // pull the catalog down from the one and only catalog server
   // (we will support general catalog servers later)
   try
   {
      DWORD dwServiceType;
      CString strServer, strObject;
      INTERNET_PORT nPort;

      BOOL bSuccess = AfxParseURL(m_strServer+m_strCatalog,dwServiceType,strServer,strObject,nPort);
      ATLASSERT( bSuccess );

      // the URL is bogus or it isn't for an FTP service
      if ( !bSuccess || dwServiceType != AFX_INET_SERVICE_FTP )
      {
         m_bFetch = true;
         m_bError = true;
         bRetVal = false;
      }
      else
      {
         // download the catalog file
//         WBFLTools::IProgressMonitorPtr progress;
//         wndProgress->QueryInterface(&progress);
//         CCallbackInternetSession inetSession(progress);
         CInternetSession inetSession;
         CFtpConnection* pFTP = inetSession.GetFtpConnection(strServer);

         CFtpFileFind ftpFind(pFTP);
         if ( !ftpFind.FindFile(strObject) || !pFTP->GetFile(strObject,m_strLocalCatalog,FALSE,FILE_ATTRIBUTE_NORMAL,FTP_TRANSFER_TYPE_ASCII | INTERNET_FLAG_RELOAD) )
         {
            // could not find or get the file (it may not be on the server)
            m_bFetch = true;
            m_bError = true;
            bRetVal = false;
         }
      }
   }
   catch (CInternetException* pException)
   {
      pException->Delete();

      m_bError = true;

      bRetVal = false;
   }

   if ( bRetVal != false )
   {
      m_bFetch = false;
      m_bError = false;
      bRetVal = true;
   }

   wndProgress->Hide();
   wndProgress.Release();
   return bRetVal;
};

std::vector<CString> CPGSuperCatalog::GetCatalogItems()
{
   std::vector<CString> items;

   if ( m_bFetch ) 
      Fetch();

   if ( m_bError )
      return items;

   char buffer[256];

   DWORD dwResult = GetPrivateProfileSectionNames(buffer,sizeof(buffer),m_strLocalCatalog);
   // buffer contains a null terminated seperated list of section names

   char* token = buffer;
   while ( *token != 0x00 )
   {
      CString strItem(token);
      CString strMasterLibrary;
      CString strWorkgroupTemplates;

      bool bHasSettings = GetSettings(strItem,strMasterLibrary,strWorkgroupTemplates,false);

      if ( bHasSettings )
         items.push_back(strItem);

      token = token + strItem.GetLength()+1;
   }
   return items;
}

bool CPGSuperCatalog::GetSettings(const char* catalogItem,CString& strMasterLibrary,CString& strWorkgroupTemplates,bool bShowMessageOnError)
{
   if ( m_bFetch ) 
      Fetch();

   if ( m_bError )
      return false;

   CPGSuperApp* pApp = (CPGSuperApp*)AfxGetApp();
   std::string strVersion = pApp->GetVersion(true);

   // get all the key value pairs for this catalog item
   CString section(catalogItem);
   char sections[32767];
   ASSERT( sizeof(sections) <= 32767 );

   memset((void*)sections,0,sizeof(sections));
   DWORD dwResult = GetPrivateProfileSection(section,sections,sizeof(sections),m_strLocalCatalog);
   ASSERT( dwResult != sizeof(sections)-2 );

   for ( int i = 0; i < sizeof(sections)/sizeof(char); i++ )
   {
      if ( sections[i] == '\0' )
         sections[i] = '\n';
   }


   // sort the key values pairs into keys for MasterLibrary and WorkgroupTemplates
   // remove the value part
   std::set<std::string> MasterLibraryEntries, WorkgroupTemplateEntries;
   char sep[] = "\n";
   char* next_token;
   char* token = strtok_s(sections,sep,&next_token);
   while (token != NULL )
   {
      std::string strToken(token);

      std::string::size_type pos = strToken.find("MasterLibrary");
      if ( pos != std::string::npos )
      {
         std::string::size_type eqpos = strToken.find("_MasterLibrary");
         MasterLibraryEntries.insert(strToken.substr(0,eqpos));
      }

      pos = strToken.find("WorkgroupTemplates");
      if ( pos != std::string::npos )
      {
         std::string::size_type eqpos = strToken.find("_WorkgroupTemplates");
         WorkgroupTemplateEntries.insert(strToken.substr(0,eqpos));
      }

      token = strtok_s(NULL,sep,&next_token);
   }


   // find the master library key closest to the one for the current version (but not after)
   std::string master_library_key("Version_");
   master_library_key += strVersion;

   std::set<std::string>::const_iterator found = MasterLibraryEntries.find(master_library_key);
   if ( found == MasterLibraryEntries.end() )
   {
      // not in the set... add it and then go back one
      std::pair<std::set<std::string>::iterator,bool> result = MasterLibraryEntries.insert(master_library_key);
      ASSERT( result.second == true );
      std::set<std::string>::iterator insert_loc = result.first;
      insert_loc--;
      master_library_key = *insert_loc;
   }
   master_library_key += std::string("_MasterLibrary");


   std::string workgroup_template_key("Version_");
   workgroup_template_key += strVersion;

   found = WorkgroupTemplateEntries.find(workgroup_template_key);
   if ( found == WorkgroupTemplateEntries.end() )
   {
      // not in the set... add it and then go back one
      std::pair<std::set<std::string>::iterator,bool> result = WorkgroupTemplateEntries.insert(workgroup_template_key);
      ASSERT( result.second == true );
      std::set<std::string>::iterator insert_loc = result.first;
      insert_loc--;
      workgroup_template_key = *insert_loc;
   }
   workgroup_template_key += CString("_WorkgroupTemplates");


   char buffer1[256];
   DWORD dwResult1 = GetPrivateProfileString(section,master_library_key.c_str(),"",buffer1,sizeof(buffer1),m_strLocalCatalog);
   CString msg1;
   if ( dwResult1 == 0 )
      msg1.Format("Could not find Master Library Key: %s",master_library_key.c_str());



   char buffer2[256];
   DWORD dwResult2 = GetPrivateProfileString(section,workgroup_template_key.c_str(),"",buffer2,sizeof(buffer2),m_strLocalCatalog);
   CString msg2;
   if ( dwResult2 == 0 )
      msg2.Format("Could not find Workgroup Template Key: %s",workgroup_template_key.c_str());


   if ( (dwResult1 == 0 || dwResult2 == 0) )
   {
      if ( bShowMessageOnError )
      {
         CString msg;
         msg.Format("The Master Library and Workgroup Templates could not be updated because PGSuper:\n\n%s\n%s\n\nPlease contact the owner of %s.",msg1,msg2,m_strServer);
         AfxMessageBox(msg,MB_ICONEXCLAMATION | MB_OK);
      }

      return false;
   }

   strMasterLibrary = buffer1;
   strWorkgroupTemplates = buffer2;
   return true;
}

void CPGSuperCatalog::FakeNetworkError(bool bFake)
{
   m_bFakeError = bFake;
}

bool CPGSuperCatalog::IsNetworkError()
{
   if ( m_bFetch )
      Fetch();

   return (m_bFakeError ? true : m_bError);
}

