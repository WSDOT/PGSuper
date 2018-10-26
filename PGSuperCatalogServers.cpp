///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2015  Washington State Department of Transportation
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
#include "PGSuperAppPlugin\PGSuperApp.h"
#include "PGSuperCatalogServers.h"
#include "intsafe.h" // for SHORT_MAX

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CPGSuperCatalogServers::CPGSuperCatalogServers()
{
}

CPGSuperCatalogServers::~CPGSuperCatalogServers()
{
   m_Servers.clear();
}

void CPGSuperCatalogServers::SetTemplateFileExtenstion(LPCTSTR strExt)
{
   m_strExt = strExt;
}

void CPGSuperCatalogServers::AddServer(CPGSuperCatalogServer* server)
{
	m_Servers.insert( ServerPtr(server) );
}

CollectionIndexType CPGSuperCatalogServers::GetServerCount() const
{
   return m_Servers.size();
}

const CPGSuperCatalogServer* CPGSuperCatalogServers::GetServer(CollectionIndexType index) const
{
   Servers::const_iterator iter = m_Servers.begin();
   for ( CollectionIndexType i = 0; i < index; i++ )
      iter++;

   return iter->get();
}

const CPGSuperCatalogServer* CPGSuperCatalogServers::GetServer(LPCTSTR strName) const
{
   ServerPtr target(new CFtpPGSuperCatalogServer(strName,CString("bogus"),m_strExt));
   Servers::const_iterator found = m_Servers.find( target );

   if(found == m_Servers.end())
   {
      ATLASSERT(false);
      return NULL;
   }
   else
   {
      return ( found == m_Servers.end() ? m_Servers.begin()->get() : found->get() );
   }
}

void CPGSuperCatalogServers::RemoveServer(CollectionIndexType index)
{
   Servers::iterator iter = m_Servers.begin();
   for ( CollectionIndexType i = 0; i < index; i++ )
      iter++;

   m_Servers.erase(iter);
}

void CPGSuperCatalogServers::RemoveServer(LPCTSTR strName)
{
   ServerPtr target(new CFtpPGSuperCatalogServer(strName,CString("bogus"),m_strExt));
   Servers::iterator found = m_Servers.find(target);
   if (found != m_Servers.end())
   {
      m_Servers.erase(found);
   }
}

bool CPGSuperCatalogServers::IsServerDefined(LPCTSTR strName) const
{
   ServerPtr target(new CFtpPGSuperCatalogServer(strName,CString("bogus"),m_strExt));
   Servers::const_iterator found = m_Servers.find(target);
   return ( found == m_Servers.end() ? false : true );
}

void CPGSuperCatalogServers::LoadFromRegistry(CWinApp* theApp)
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

            CPGSuperCatalogServer* pserver = CreateCatalogServer(strValue,m_strExt);

            if (pserver!=NULL) // this is not good, but an assert should fire in CreateCatalogServer to help debugging
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
         while ( ::RegEnumValue(hSecKey,dwIndex++,&serverName[0],&serverNameSize,NULL,&type,(LPBYTE)&serverString[0],&serverStringSize) != ERROR_NO_MORE_ITEMS )
         {
            CPGSuperCatalogServer* pServer = CreateCatalogServer(serverName,serverString,m_strExt );
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
   if (!IsServerDefined(_T("WSDOT")))
      m_Servers.insert( ServerPtr(new CFtpPGSuperCatalogServer(m_strExt)) ); // wsdot

   if (!IsServerDefined(_T("TxDOT")))
      m_Servers.insert( ServerPtr( new CFtpPGSuperCatalogServer(CString("TxDOT"),CString("ftp://ftp.dot.state.tx.us/pub/txdot-info/brg/pgsuper/"),m_strExt) ) );
}

void CPGSuperCatalogServers::SaveToRegistry(CWinApp* theApp) const
{
   Servers::const_iterator iter;
   for ( iter = m_Servers.begin(); iter != m_Servers.end(); iter++ )
   {
      const CPGSuperCatalogServer* pServer = iter->get();
      CString server_creation_string = GetCreationString(pServer);

      CString key(pServer->GetServerName());
      theApp->WriteProfileString(_T("CatalogServers"), key, server_creation_string);
   }
}
