///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2010  Washington State Department of Transportation
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
}
void CPGSuperCatalogServers::AddServer(CPGSuperCatalogServer* server)
{
	m_Servers.insert( ServerPtr(server) );
}

long CPGSuperCatalogServers::GetServerCount() const
{
   return m_Servers.size();
}

const CPGSuperCatalogServer* CPGSuperCatalogServers::GetServer(long index) const
{
   Servers::const_iterator iter = m_Servers.begin();
   for ( long i = 0; i < index; i++ )
      iter++;

   return iter->get();
}

const CPGSuperCatalogServer* CPGSuperCatalogServers::GetServer(const CString& strName) const
{
   ServerPtr target(new CFtpPGSuperCatalogServer(strName,CString("bogus")));
   Servers::const_iterator found = m_Servers.find( target );
   ATLASSERT(found != m_Servers.end());
   return ( found == m_Servers.end() ? m_Servers.begin()->get() : found->get() );
}

void CPGSuperCatalogServers::RemoveServer(long index)
{
   Servers::iterator iter = m_Servers.begin();
   for ( long i = 0; i < index; i++ )
      iter++;

   m_Servers.erase(iter);
}

void CPGSuperCatalogServers::RemoveServer(const CString& strName)
{
   ServerPtr target(new CFtpPGSuperCatalogServer(strName,CString("bogus")));
   Servers::iterator found = m_Servers.find(target);
   if (found != m_Servers.end())
   {
      m_Servers.erase(found);
   }
}

bool CPGSuperCatalogServers::IsServerDefined(const CString& strName) const
{
   ServerPtr target(new CFtpPGSuperCatalogServer(strName,CString("bogus")));
   Servers::const_iterator found = m_Servers.find(target);
   return ( found == m_Servers.end() ? false : true );
}

void CPGSuperCatalogServers::LoadFromRegistry(CWinApp* theApp)
{
   m_Servers.clear();
   int count = theApp->GetProfileInt(_T("Servers"),_T("Count"),-1);
   if ( count > 0 )
   {
      for ( int i = 0; i < count; i++ )
      {
         CString key(char(i+'A'));
         CString strValue = theApp->GetProfileString(_T("Servers"),key);

         CPGSuperCatalogServer* pserver = CreateCatalogServer(strValue);

         if (pserver!=NULL) // this is not good, but an assert should fire in CreateCatalogServer to help debugging
         {
            m_Servers.insert( ServerPtr(pserver) );
         }
      }
   }

   // Always have a WSDOT and TxDOT server
   if (!IsServerDefined("WSDOT"))
      m_Servers.insert( ServerPtr(new CFtpPGSuperCatalogServer()) ); // wsdot

   if (!IsServerDefined("TxDOT"))
      m_Servers.insert( ServerPtr( new CFtpPGSuperCatalogServer(CString("TxDOT"),CString("ftp://ftp.dot.state.tx.us/pub/txdot-info/brg/pgsuper/")) ) );
}

void CPGSuperCatalogServers::SaveToRegistry(CWinApp* theApp) const
{
   theApp->WriteProfileInt(_T("Servers"),_T("Count"),m_Servers.size());
   Servers::const_iterator iter;
   long i = 0;
   for ( iter = m_Servers.begin(); iter != m_Servers.end(); iter++, i++ )
   {
      CString server_creation_string = GetCreationString(iter->get());

      CString key(char(i+'A'));
      theApp->WriteProfileString(_T("Servers"), key, server_creation_string);
   }
}
