///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright � 1999-2022  Washington State Department of Transportation
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

#if !defined INCLUDED_CATALOG_H_
#define INCLUDED_CATALOG_H_

#include <vector>

// Exception class for catalog parsing errors
class CCatalogParsingException
{
public:
   enum ErrorType
   {
      cteVersionStringNotValid,
      cteUnknownError
   };

   CCatalogParsingException(ErrorType error, const CString& lineBeingParsed = CString()):
   m_ErrorType(error),
   m_LineBeingParsed(lineBeingParsed)
   {
   }

   ErrorType GetErrorType()
   {
      return m_ErrorType;
   }

   CString GetLineBeingParsed()
   {
      return m_LineBeingParsed;
   }

   void SetLineBeingParsed(const CString& error)
   {
      m_LineBeingParsed = error;
   }

private:
   ErrorType m_ErrorType;
   CString m_LineBeingParsed;

   CCatalogParsingException();
};
/////////////////////////////////////////////////////////////////////////////
// CCatalog 

class CCatalog
{
public:
   enum Format
   {
      ctOriginal, // original version contining separate library file and template folders
      ctPgz       // publisher in pgz compressed files
   };

   CCatalog();

   // this function can throw a CCatalogParsingException
   bool Init(LPCTSTR strIniFileName, const CString& strPGSuperVersion);

   std::vector<CString> GetPublishers();

   bool DoesPublisherExist(const CString& publisher);

   Format GetFormat(const CString& publisher);
   CString GetWebLink(const CString& publisher);

   // for ctOriginal catalogs
   void GetCatalogSettings(LPCTSTR publisher,CString& strMasterLibrary,CString& strWorkgroupTemplates);

   // for ctOriginal catalogs
   void GetCatalogSettings(LPCTSTR publisher,CString& strPgzFile);

   // App version for catalogs
   static CString GetAppVersion();


private:
   struct Publisher
   {
      CString Name;
      Format  Format;
      CString MasterLibrary; // location of pgz if ctPgz format
      CString TemplateFolder;
      CString WebLink;
   };

   typedef std::vector<Publisher> PublisherList;
   typedef PublisherList::iterator PublisherIterator;
   PublisherList m_Publishers;


   bool DoParse();
   const Publisher* GetPublisher(LPCTSTR publisher);

   CString m_strLocalCatalog; // location of ini file on local computer
   CString m_PGSuperVersion;

   bool m_DidParse;


};

#endif // INCLUDED_PGSUPERCATALOG_H_