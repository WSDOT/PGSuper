///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2025  Washington State Department of Transportation
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
#include <PgsExt\CatalogServers.h>
#include "intsafe.h" // for SHORT_MAX

CCatalogServers::CCatalogServers()
{
}

CCatalogServers::~CCatalogServers()
{
   m_Servers.clear();
}

void CCatalogServers::SetAppName(LPCTSTR strAppName)
{
   m_AppName = strAppName;
}

LPCTSTR CCatalogServers::GetAppName() const
{
   ATLASSERT(!m_AppName.IsEmpty()); // did you forget to call SetAppName
   return m_AppName;
}

void CCatalogServers::SetTemplateFileExtenstion(LPCTSTR strExt)
{
   m_strExt = strExt;
}

void CCatalogServers::AddServer(CCatalogServer* server)
{
	m_Servers.insert( ServerPtr(server) );
}

IndexType CCatalogServers::GetServerCount() const
{
   return m_Servers.size();
}

const CCatalogServer* CCatalogServers::GetServer(IndexType index) const
{
   Servers::const_iterator iter = m_Servers.begin();
   for ( IndexType i = 0; i < index; i++ )
      iter++;

   return iter->get();
}

const CCatalogServer* CCatalogServers::GetServer(LPCTSTR strName) const
{
   ServerPtr target(new CFtpCatalogServer(m_AppName,strName,CString("bogus"),m_strExt));
   Servers::const_iterator found = m_Servers.find( target );

   if(found == m_Servers.end())
   {
      ATLASSERT(false);
      return nullptr;
   }
   else
   {
      return ( found == m_Servers.end() ? m_Servers.begin()->get() : found->get() );
   }
}

void CCatalogServers::RemoveServer(IndexType index)
{
   Servers::iterator iter = m_Servers.begin();
   for ( IndexType i = 0; i < index; i++ )
      iter++;

   m_Servers.erase(iter);
}

void CCatalogServers::RemoveServer(LPCTSTR strName)
{
   ServerPtr target(new CFtpCatalogServer(m_AppName,strName,CString("bogus"),m_strExt));
   Servers::iterator found = m_Servers.find(target);
   if (found != m_Servers.end())
   {
      m_Servers.erase(found);
   }
}

bool CCatalogServers::IsServerDefined(LPCTSTR strName) const
{
   ServerPtr target(new CFtpCatalogServer(m_AppName,strName,CString("bogus"),m_strExt));
   Servers::const_iterator found = m_Servers.find(target);
   return ( found == m_Servers.end() ? false : true );
}

void CCatalogServers::LoadFromRegistry(CWinApp* theApp)
{
   m_Servers.clear();

   HKEY hAppKey = theApp->GetAppRegistryKey();
   HKEY hSecKey;
   if ( ::RegOpenKey(hAppKey,_T("Servers"),&hSecKey) == ERROR_SUCCESS )
   {
      // Load in old format
      int count = theApp->GetProfileInt(_T("Servers"),_T("Count"),-1);
      if ( count > 0 )
      {
         for ( int i = 0; i < count; i++ )
         {
            CString key(TCHAR(i+_T('A')));
            CString strValue = theApp->GetProfileString(_T("Servers"),key);

            CCatalogServer* pserver = CreateCatalogServer(m_AppName,strValue,m_strExt);

            if (pserver!=nullptr) // this is not good, but an assert should fire in CreateCatalogServer to help debugging
            {
               m_Servers.insert( ServerPtr(pserver) );
            }
         }
      }

      // Delete the "Servers" section from the registry (no longer using it)
      ::RegCloseKey(hSecKey);

      SaveToRegistry(theApp); // save in new format

      // delete the old key
      ::RegDeleteKey(hAppKey,_T("Servers"));
   }
   else
   {
      hSecKey = theApp->GetSectionKey(_T("CatalogServers"));
      if ( hSecKey )
      {
         DWORD dwIndex = 0;
         TCHAR serverName[SHORT_MAX];
         TCHAR serverString[SHORT_MAX];
         DWORD serverNameSize = sizeof(serverName);
         DWORD serverStringSize = sizeof(serverString);
         DWORD type;
         while ( ::RegEnumValue(hSecKey,dwIndex++,&serverName[0],&serverNameSize,nullptr,&type,(LPBYTE)&serverString[0],&serverStringSize) != ERROR_NO_MORE_ITEMS )
         {
            CCatalogServer* pServer = CreateCatalogServer(m_AppName,serverName,serverString,m_strExt );
            if ( pServer )
            {
               m_Servers.insert( ServerPtr(pServer) );
            }

            serverNameSize = sizeof(serverName);
            serverStringSize = sizeof(serverString);
         };

         ::RegCloseKey(hSecKey);
      }
   }

   ::RegCloseKey(hAppKey);

   // Always have a WSDOT and TxDOT server

   // This doesn't work... HTTP server wont allow configuration files.
   //if (m_AppName == _T("PGSuper") || m_AppName == _T("PGSplice"))
   //{
   //   const CCatalogServer* pServer = GetServer(_T("WSDOT"));
   //   if (pServer && pServer->GetServerType() == srtInternetFtp)
   //   {
   //      // the official WSDOT server has moved from FTP to HTTP...
   //      // still referencing the old FTP server
   //      // remove the server then it will be redefined below
   //      RemoveServer(_T("WSDOT"));
   //      pServer = nullptr;
   //   }
   //}

   if (!IsServerDefined(_T("WSDOT")) && m_AppName != _T("TOGA")) // don't need a wsdot toga server
   {
      m_Servers.insert( ServerPtr(new CFtpCatalogServer(m_AppName,m_strExt)) ); // wsdot
      //m_Servers.insert( ServerPtr(new CHttpCatalogServer(m_AppName,m_strExt)) ); // wsdot
   }

   if (!IsServerDefined(_T("TxDOT")) && m_AppName != _T("PGSplice")) // TxDOT does not have a PGSplice server
   {
      CString url = CString("https://ftp.dot.state.tx.us/pub/txdot-info/brg/") + CString(m_AppName).MakeLower() + CString("/");
      m_Servers.insert( ServerPtr( new CHttpCatalogServer(m_AppName,CString("TxDOT"),url,m_strExt) ) );
   }
}

void CCatalogServers::SaveToRegistry(CWinApp* theApp) const
{
   // first delete any keys
   HKEY hAppKey = theApp->GetAppRegistryKey();
   theApp->DelRegTree(hAppKey,_T("CatalogServers"));

   // Save current servers
   theApp->WriteProfileString(_T("CatalogServers"), nullptr, nullptr);
   Servers::const_iterator iter;
   for ( iter = m_Servers.begin(); iter != m_Servers.end(); iter++ )
   {
      const CCatalogServer* pServer = iter->get();
      CString server_creation_string = GetCreationString(pServer);

      CString key(pServer->GetServerName());
      theApp->WriteProfileString(_T("CatalogServers"), key, server_creation_string);
   }
}
