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

#include "stdafx.h"
#include "pgsuper.h"
#include "PGSuperCatalogServers.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CPGSuperCatalogServers::CPGSuperCatalogServers()
{
}

void CPGSuperCatalogServers::AddServer(const CString& strName,const CString& strAddress)
{
	m_Servers.insert(std::make_pair(strName,strAddress));
}

long CPGSuperCatalogServers::GetServerCount() const
{
   return m_Servers.size();
}

void CPGSuperCatalogServers::GetServer(long index,CString& strName,CString& strAddress) const
{
   Servers::const_iterator iter = m_Servers.begin();
   for ( long i = 0; i < index; i++ )
      iter++;

   std::pair<CString,CString> data = *iter;
   strName    = data.first;
   strAddress = data.second;
}

CString CPGSuperCatalogServers::GetServerAddress(const CString& strName) const
{
   Servers::const_iterator found = m_Servers.find(strName);
   if ( found == m_Servers.end() )
   {
      return CString("No such server");
   }

   return (*found).second;
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
   m_Servers.erase(strName);
}

bool CPGSuperCatalogServers::IsServerDefined(const CString& strName) const
{
   Servers::const_iterator found = m_Servers.find(strName);
   return ( found == m_Servers.end() ? false : true );
}

void CPGSuperCatalogServers::LoadFromRegistry()
{
   CPGSuperApp* theApp = (CPGSuperApp*)AfxGetApp();

   CString strDefaultCatalogServerURL = theApp->GetLocalMachineString(_T("Servers"),_T(""),_T("WSDOT!ftp://ftp.wsdot.wa.gov/public/bridge/Software/PGSuper"));

   m_Servers.clear();
   int count = theApp->GetProfileInt(_T("Servers"),_T("Count"),-1);
   if ( count <= 0 )
   {
      // "count" not found or it is zero... make sure there is at least one catalog server
      int p = strDefaultCatalogServerURL.Find('!');
      if ( p != -1 )
      {
         CString strName = strDefaultCatalogServerURL.Left(p);
         CString strAddress = strDefaultCatalogServerURL.Mid(p+1);

         m_Servers.insert( std::make_pair(strName,strAddress) );
      }
      else
      {
         m_Servers.insert( std::make_pair(CString("WSDOT"),CString("ftp://ftp.wsdot.wa.gov/public/bridge/Software/PGSuper")) );
      }
   }
   else
   {
      for ( int i = 0; i < count; i++ )
      {
         CString key(char(i+'A'));
         CString strValue = theApp->GetProfileString(_T("Servers"),key,strDefaultCatalogServerURL);
         int p = strValue.Find('!');
         if ( p != -1 )
         {
            CString strName = strValue.Left(p);
            CString strAddress = strValue.Mid(p+1);

            m_Servers.insert( std::make_pair(strName,strAddress) );
         }
      }
   }

   // always have a WSDOT and TxDOT server!!!
   m_Servers.insert( std::make_pair(CString("WSDOT"),CString("ftp://ftp.wsdot.wa.gov/public/bridge/Software/PGSuper")) );
   m_Servers.insert( std::make_pair(CString("TxDOT"),CString("ftp://ftp.dot.state.tx.us/pub/txdot-info/brg/pgsuper/")) );
}

void CPGSuperCatalogServers::SaveToRegistry() const
{
   CWinApp* theApp = AfxGetApp();

   theApp->WriteProfileInt(_T("Servers"),_T("Count"),m_Servers.size());
   Servers::const_iterator iter;
   long i = 0;
   for ( iter = m_Servers.begin(); iter != m_Servers.end(); iter++, i++ )
   {
      std::pair<CString,CString> server = *iter;
      CString value;
      value.Format("%s!%s",server.first,server.second);
      theApp->WriteProfileString(_T("Servers"),CString(char(i+'A')),value);
   }
}
