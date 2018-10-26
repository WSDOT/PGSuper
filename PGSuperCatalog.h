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

#if !defined INCLUDED_PGSUPERCATALOG_H_
#define INCLUDED_PGSUPERCATALOG_H_


#include <vector>

/////////////////////////////////////////////////////////////////////////////
// CPGSuperCatalog dialog

class CPGSuperCatalog
{
public:
   CPGSuperCatalog(const char* strName,const char* strServer,const char* file);

   void SetCatalogServer(const char* strName,const char* strServer);
   void SetCatalogFile(const char* file);
   CString GetCatalogServer() const;
   CString GetCatalogName() const;
   CString GetCatalogFile() const;

   bool Fetch();

   std::vector<CString> GetCatalogItems();

   bool GetSettings(const char* catalogItem,CString& strMasterLibrary,CString& strWorkgroupTemplates,bool bShowMessageOnError=true);

   void FakeNetworkError(bool bFake); // used for testing... causes IsNetworkError to always return true
   bool IsNetworkError();

private:
   bool m_bFetch;
   bool m_bError;
   bool m_bFakeError;
   CString m_strName; // this is a common name for the catalog server
   CString m_strServer; // this is the server that holds the catalog
   CString m_strCatalog; // this is the catalog file on the server
   CString m_strLocalCatalog; // this is the copy that is downloaded to the local computer
};

#endif // INCLUDED_PGSUPERCATALOG_H_