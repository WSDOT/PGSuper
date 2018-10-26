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

#if !defined INCLUDED_PGSUPERCATALOGSERVERS_H_
#define INCLUDED_PGSUPERCATALOGSERVERS_H_

#include <map>

class CPGSuperCatalogServers
{
public:
   CPGSuperCatalogServers();
   void AddServer(const CString& strName,const CString& strAddress);
   long GetServerCount() const;
   void GetServer(long index,CString& strName,CString& strAddress) const;
   CString GetServerAddress(const CString& strName) const;
   void RemoveServer(long index);
   void RemoveServer(const CString& strName);
   bool IsServerDefined(const CString& strName) const;

   void LoadFromRegistry();
   void SaveToRegistry() const;

private:
   typedef std::map<CString,CString> Servers;
   Servers m_Servers;
};

#endif // INCLUDED_PGSUPERCATALOGSERVERS_H_