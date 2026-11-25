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

// Catalog.cpp : implementation file
//
#include <PgsExt\PgsExtLib.h>
#include <PgsExt\Catalog.h>
#include "..\ConfigPackager\ConfigurationManifest.h"

// Our Catalog class

CCatalog::CCatalog():
m_bIsJson(false)
{
}

bool CCatalog::Init(LPCTSTR strLocalFile, LPCTSTR strAppName, const CString& strAppVersion, bool bIsJson, CString& strError)
{
   m_Publishers.clear();
   m_bIsJson = bIsJson;
   strError.Empty();

   ConfigManifest::Application application;
   if (bIsJson)
   {
      ConfigManifest::Manifest manifest;
      std::wstring error;
      if (!ConfigManifest::ReadManifest(strLocalFile, manifest, error))
      {
         strError = error.c_str();
         return false;
      }

      const ConfigManifest::Application* pApplication = manifest.FindApplication(strAppName);
      if (pApplication == nullptr)
      {
         // This server doesn't publish anything for this program. That isn't an error.
         return true;
      }
      application = *pApplication;
   }
   else
   {
      application = ConfigManifest::ReadLegacyIni(strLocalFile, strAppName);
   }

   // Select the package for this version of the program. Configurations that have nothing
   // for this version are left out of the list.
   for (const auto& configuration : application.Configurations)
   {
      const ConfigManifest::Package* pPackage = ConfigManifest::FindCompatiblePackage(configuration, (LPCTSTR)strAppVersion);
      if (pPackage == nullptr)
      {
         continue;
      }

      Publisher publisher;
      publisher.Name       = configuration.Name.c_str();
      publisher.WebLink    = configuration.WebLink.c_str();
      publisher.PackageURL = pPackage->Url.c_str();
      publisher.Md5        = pPackage->Md5.c_str();
      m_Publishers.push_back(publisher);
   }

   return true;
}

bool CCatalog::IsJson() const
{
   return m_bIsJson;
}

std::vector<CString> CCatalog::GetPublishers() const
{
   std::vector<CString> pubs;
   for (const auto& publisher : m_Publishers)
   {
      pubs.push_back(publisher.Name);
   }

   return pubs;
}

bool CCatalog::DoesPublisherExist(const CString& publisher) const
{
   return GetPublisher(publisher) != nullptr;
}

CString CCatalog::GetWebLink(const CString& publisher) const
{
   const Publisher* pPub = GetPublisher(publisher);
   return pPub ? pPub->WebLink : CString();
}

CString CCatalog::GetPackageURL(const CString& publisher) const
{
   const Publisher* pPub = GetPublisher(publisher);
   return pPub ? pPub->PackageURL : CString();
}

CString CCatalog::GetPackageMd5(const CString& publisher) const
{
   const Publisher* pPub = GetPublisher(publisher);
   return pPub ? pPub->Md5 : CString();
}

const CCatalog::Publisher* CCatalog::GetPublisher(LPCTSTR publisher) const
{
   for (const auto& pub : m_Publishers)
   {
      if (pub.Name.Compare(publisher) == 0)
      {
         return &pub;
      }
   }

   return nullptr;
}

CString CCatalog::GetAppVersion()
{
   CWinApp* pApp = AfxGetApp();
   CString strExe( pApp->m_pszExeName );
   strExe += _T(".dll");

   CVersionInfo verInfo;
   verInfo.Load(strExe);

   CString strVersion = verInfo.GetProductVersionAsString(false);

   return strVersion;
}
