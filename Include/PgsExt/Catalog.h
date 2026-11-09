///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2026  Washington State Department of Transportation
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

/////////////////////////////////////////////////////////////////////////////
// CCatalog
//
// The list of configurations a configuration server publishes for one program.
// It is read from ConfigurationPackages.json, or from a legacy <App>Packages.ini
// when the server has no JSON file. For each configuration, the package for the
// newest version that isn't newer than the running program is selected.

class CCatalog
{
public:
   CCatalog();

   // Reads a downloaded catalog file. bIsJson tells whether it is ConfigurationPackages.json
   // or a legacy INI file. Returns false and an explanation if the file can't be used.
   bool Init(LPCTSTR strLocalFile, LPCTSTR strAppName, const CString& strAppVersion, bool bIsJson, CString& strError);

   // True if the catalog was read from ConfigurationPackages.json
   bool IsJson() const;

   std::vector<CString> GetPublishers() const;

   bool DoesPublisherExist(const CString& publisher) const;

   CString GetWebLink(const CString& publisher) const;

   // Package location as published. It may be relative to the catalog file.
   CString GetPackageURL(const CString& publisher) const;

   // MD5 of the package as published in the catalog. Empty if the catalog doesn't have it.
   CString GetPackageMd5(const CString& publisher) const;

   // App version for catalogs
   static CString GetAppVersion();

private:
   struct Publisher
   {
      CString Name;
      CString PackageURL;
      CString Md5;
      CString WebLink;
   };

   std::vector<Publisher> m_Publishers;
   bool m_bIsJson;

   const Publisher* GetPublisher(LPCTSTR publisher) const;
};

#endif // INCLUDED_CATALOG_H_
