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

#if !defined INCLUDED_PGSUPERCATALOGSERVERS_H_
#define INCLUDED_PGSUPERCATALOGSERVERS_H_

#include <set>
#include "PGSuperCatalogServer.h"
#include <boost\shared_ptr.hpp>

class CPGSuperCatalogServers
{
public:
   CPGSuperCatalogServers();
   ~CPGSuperCatalogServers();
   void SetAppName(LPCTSTR strAppName);
   LPCTSTR GetAppName() const;
   void SetTemplateFileExtenstion(LPCTSTR strExt);
   void AddServer(CPGSuperCatalogServer* pserver);
   CollectionIndexType GetServerCount() const;
   const CPGSuperCatalogServer* GetServer(CollectionIndexType index) const;
   const CPGSuperCatalogServer* GetServer(LPCTSTR strName) const;
   void RemoveServer(CollectionIndexType index);
   void RemoveServer(LPCTSTR strName);
   bool IsServerDefined(LPCTSTR strName) const;

   void LoadFromRegistry(CWinApp* theApp);
   void SaveToRegistry(CWinApp* theApp) const;

private:
   // predicate class for comparing servers only by name
   class CatalogServerCompareByName
   {
   public:
      bool operator () (const boost::shared_ptr<CPGSuperCatalogServer>& pserver1, const boost::shared_ptr<CPGSuperCatalogServer>& pserver2) const
      {
         return pserver1->GetServerName() < pserver2->GetServerName();
      }
   };
   
   typedef boost::shared_ptr<CPGSuperCatalogServer> ServerPtr;
   typedef std::set<ServerPtr ,CatalogServerCompareByName> Servers;
   Servers m_Servers;
   CString m_strExt;
   CString m_AppName;
};

#endif // INCLUDED_PGSUPERCATALOGSERVERS_H_