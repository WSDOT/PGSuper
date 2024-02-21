///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2024  Washington State Department of Transportation
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

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// Class to compare version numbers as numbers and not alpha value. This was a bug prior and in version 4.0.11
class VersionComparator
{
	std::_tstring m_Prolog;
public:
   VersionComparator(const std::_tstring& proLog) :
      m_Prolog(proLog)
	{}

	bool operator() (const std::_tstring& ver1, const std::_tstring& ver2) const
	{
      Uint32 V1PrimeVersion, V1SecVersion, V1TersVersion;
      Uint32 V2PrimeVersion, V2SecVersion, V2TersVersion;
      if (!ParseVersionString(ver1, &V1PrimeVersion, &V1SecVersion, &V1TersVersion))
      {
         throw CCatalogParsingException(CCatalogParsingException::cteUnknownError);
      }

      // now deal with integer version numbering
      if (!ParseVersionString(ver2, &V2PrimeVersion, &V2SecVersion, &V2TersVersion))
      {
         throw CCatalogParsingException(CCatalogParsingException::cteUnknownError);
      }

      // correct comparators are tricky
      if (V1PrimeVersion < V2PrimeVersion)
      {
         return true;
      }
      else if (V1PrimeVersion == V2PrimeVersion)
      {
         if (V1SecVersion < V2SecVersion)
         {
            return true;
         }
         else if (V1SecVersion == V2SecVersion)
         {
            if (V1TersVersion < V2TersVersion)
            {
               return true;
            }
         }
      }

      return false;
	}

   bool ParseVersionString(const std::_tstring& verstr, Uint32* pPrimeVersion, Uint32* pSecVersion, Uint32* pTersVersion) const
   {
      *pPrimeVersion = 0;
      *pSecVersion = 0;
      *pTersVersion = 0;

      // strip prolog from version names
      std::_tstring newstr;
      std::_tstring::size_type midpos = verstr.find(m_Prolog);
      if (std::_tstring::npos == midpos)
      {
         ATLASSERT(0); // string format is likely invalid,
         throw CCatalogParsingException(CCatalogParsingException::cteUnknownError);
      }
      else
      {
         newstr = verstr.substr(midpos+m_Prolog.size(), verstr.size()-1);
      }

      // get version numbers
      WBFL::System::Tokenizer tokenizer(_T("."));
      tokenizer.push_back(newstr);
      if (tokenizer.size() != 3)// should always be the case
      {
         throw CCatalogParsingException(CCatalogParsingException::cteVersionStringNotValid);
      }

      Uint32 n = 0;
      WBFL::System::Tokenizer::iterator iter;
      for ( iter = tokenizer.begin(); iter != tokenizer.end(); iter++ )
      {
         long vn; 
         if (tokenizer.ParseLong(iter->c_str(), &vn))
         {
            if (0 == n)
            {
               *pPrimeVersion = vn;
            }
            if (1 == n)
            {
               *pSecVersion = vn;
            }
            if (2 == n)
            {
               *pTersVersion = vn;
            }
         }
         else
         {
            throw CCatalogParsingException(CCatalogParsingException::cteVersionStringNotValid);
         }

         n++;
      }

      return true;
   }
};

typedef std::set<std::_tstring, VersionComparator> VersionSet;

// function to find closest compatible version of catalog entry for current software version
// find the key closest to the one for the current version (but not after)
static bool FindCompatibleVersion(VersionSet& rEntries, std::_tstring& rVersion)
{
   bool did_find = true;
   VersionSet::const_iterator found( rEntries.find(rVersion) );
   if ( found == rEntries.end() )
   {
      // Not in the set... add it and then try to use most recent version to this one.
      std::pair<VersionSet::iterator,bool> result( rEntries.insert(rVersion) );
      ASSERT( result.second == true );
      VersionSet::iterator insert_loc( result.first );

      if (insert_loc != rEntries.begin())
      {
         // Found a version prior to the current program version. We can use this.
         insert_loc--;
         rVersion = *insert_loc;
      }
      else
      {
         // If the inserted entry is at the beginning it means all entries in the set are for a newer
         // version of PGSuper than this one. We can't use it.
         did_find = false;
      }
   }

   return did_find;
}


// Our Catalog class

CCatalog::CCatalog():
m_DidParse(false)
{
}

bool CCatalog::Init(LPCTSTR strIniFileName, const CString& strPGSuperVersion)
{
   m_strLocalCatalog = strIniFileName;
   m_PGSuperVersion = strPGSuperVersion;
   m_DidParse = false;
   m_Publishers.clear();

   return DoParse();
}

std::vector<CString> CCatalog::GetPublishers()
{
   ATLASSERT(m_DidParse);
   std::vector<CString> pubs;
   for (PublisherIterator it=m_Publishers.begin(); it!=m_Publishers.end(); it++)
   {
      pubs.push_back(it->Name);
   }

   return pubs;
}

bool CCatalog::DoesPublisherExist(const CString& publisher)
{
   const Publisher* pPub = GetPublisher(publisher);

   return pPub!=nullptr;
}

CCatalog::Format CCatalog::GetFormat(const CString& publisher)
{
   const Publisher* pPub = GetPublisher(publisher);
   return pPub->Format;
}

CString CCatalog::GetWebLink(const CString& publisher)
{
   const Publisher* pPub = GetPublisher(publisher);
   return pPub->WebLink;
}

void CCatalog::GetCatalogSettings(LPCTSTR publisher,CString& strMasterLibrary,CString& strWorkgroupTemplates)
{
   const Publisher* pPub = GetPublisher(publisher);
   ATLASSERT(pPub->Format==ctOriginal);

   strMasterLibrary      = pPub->MasterLibrary;
   strWorkgroupTemplates = pPub->TemplateFolder;
}

void CCatalog::GetCatalogSettings(LPCTSTR publisher,CString& strPgzFile)
{
   const Publisher* pPub = GetPublisher(publisher);
   ATLASSERT(pPub->Format==ctPgz);

   strPgzFile      = pPub->MasterLibrary;
}

const CCatalog::Publisher* CCatalog::GetPublisher(LPCTSTR publisher)
{
   for (PublisherIterator it=m_Publishers.begin(); it!=m_Publishers.end(); it++)
   {
      if(it->Name.Compare(publisher)==0)
      {
         return &(*it);
      }
   }

   ATLASSERT(false);
   return nullptr;
}


bool CCatalog::DoParse()
{
   // first get all publisher names
   std::vector<CString> publishers;

   TCHAR buffer[256];
   DWORD dwResult = GetPrivateProfileSectionNames(buffer,sizeof(buffer)/sizeof(TCHAR),m_strLocalCatalog);
   // buffer contains a null terminated separated list of section names

   LPTSTR token = buffer;
   while ( *token != _T('\0') )
   {
      CString strItem(token);
      CString strMasterLibrary;
      CString strWorkgroupTemplates;

      publishers.push_back(strItem);

      token = token + strItem.GetLength()+1;
   }

   // cycle through publishers and build list
   std::vector<CString>::iterator pubIter(publishers.begin());
   std::vector<CString>::iterator pubIterEnd(publishers.end());
   for ( ; pubIter != pubIterEnd; pubIter++)
   {
      const CString publisherName(*pubIter);

      Publisher publisher;
      publisher.Name   = publisherName;
      publisher.Format = ctOriginal;

      // Determine what format we have
      dwResult = GetPrivateProfileString(publisherName,_T("Format"),_T(""),buffer,sizeof(buffer)/sizeof(TCHAR),m_strLocalCatalog);
      if ( dwResult != 0 )
      {
         CString format(buffer);
         if(format.CompareNoCase(_T("pgz"))==0)
         {
            publisher.Format = ctPgz;
         }
      }

      // get all the key value pairs for this publisher
      TCHAR sections[32767];
      memset(sections,0,sizeof(sections));
      DWORD dwResult = GetPrivateProfileSection(publisherName,sections,sizeof(sections)/sizeof(TCHAR),m_strLocalCatalog);
      ASSERT( dwResult != sizeof(sections)/sizeof(TCHAR)-2 );

      for ( int i = 0; i < sizeof(sections)/sizeof(TCHAR); i++ )
      {
         if ( sections[i] == _T('\0') )
         {
            sections[i] = _T('\n');
         }
      }

      if (publisher.Format == ctOriginal)
      {
         // Original .ini file format. This consists of key value pairs for both the 
         // library file location and template.

         // sort the key values pairs into keys for MasterLibrary and WorkgroupTemplates
         // remove the value part
         VersionSet MasterLibraryEntries(VersionComparator(_T("Version_")));
         VersionSet WorkgroupTemplateEntries(VersionComparator(_T("Version_")));
         TCHAR sep[] = _T("\n");
         LPTSTR next_token;
         LPTSTR token = _tcstok_s(sections,sep,&next_token);
         while (token != nullptr )
         {
            std::_tstring strToken(token);

            std::_tstring::size_type pos = strToken.find(_T("MasterLibrary"));
            if ( pos != std::_tstring::npos )
            {
               std::_tstring::size_type eqpos = strToken.find(_T("_MasterLibrary"));

               try
               {
                  MasterLibraryEntries.insert(strToken.substr(0, eqpos));
               }
               catch (CCatalogParsingException& ex)
               {
                  // Comparator can throw - we are toast
                  ex.SetLineBeingParsed(strToken.c_str());
                  throw ex;
               }
            }

            pos = strToken.find(_T("WorkgroupTemplates"));
            if ( pos != std::_tstring::npos )
            {
               std::_tstring::size_type eqpos = strToken.find(_T("_WorkgroupTemplates"));

               try
               {
                  WorkgroupTemplateEntries.insert(strToken.substr(0, eqpos));
               }
               catch (CCatalogParsingException& ex)
               {
                  // Comparator can throw - we are toast
                  ex.SetLineBeingParsed(strToken.c_str());
                  throw ex;
               }
            }

            token = _tcstok_s(nullptr,sep,&next_token);
         }

         // make sure we have entries for this publisher
         if (MasterLibraryEntries.empty())
         {
            break;
         }

         if (WorkgroupTemplateEntries.empty())
         {
            break;
         }

         // find the master library key closest to the one for the current version (but not after)
         std::_tstring master_library_key(_T("Version_"));
         master_library_key += m_PGSuperVersion;

         bool did_find = FindCompatibleVersion(MasterLibraryEntries, master_library_key);
         if (!did_find)
         {
            break;
         }

         master_library_key += std::_tstring(_T("_MasterLibrary"));

         std::_tstring workgroup_template_key(_T("Version_"));
         workgroup_template_key += m_PGSuperVersion;

         did_find = FindCompatibleVersion(WorkgroupTemplateEntries, workgroup_template_key);
         if (!did_find)
         {
            break;
         }

         workgroup_template_key += CString(_T("_WorkgroupTemplates"));

         TCHAR buffer1[256];
         memset(buffer1,0,sizeof(buffer1));
         DWORD dwResult1 = GetPrivateProfileString(publisherName,master_library_key.c_str(),_T(""),buffer1,sizeof(buffer1)/sizeof(TCHAR),m_strLocalCatalog);
         CString msg1;
         if ( dwResult1 == 0 )
         {
            msg1.Format(_T("Could not find Master Library Key: %s"),master_library_key.c_str());
         }



         TCHAR buffer2[256];
         memset(buffer2,0,sizeof(buffer2));
         DWORD dwResult2 = GetPrivateProfileString(publisherName,workgroup_template_key.c_str(),_T(""),buffer2,sizeof(buffer2)/sizeof(TCHAR),m_strLocalCatalog);
         CString msg2;
         if ( dwResult2 == 0 )
         {
            msg2.Format(_T("Could not find Workgroup Template Key: %s"),workgroup_template_key.c_str());
         }


         if ( (dwResult1 == 0 || dwResult2 == 0) )
         {
            CString msg;
            msg.Format(_T("The %s configuration could not be updated because:\n\n%s\n%s\n\nPlease contact the server owner."),AfxGetApp()->m_pszProfileName,msg1,msg2);
            AfxMessageBox(msg,MB_ICONEXCLAMATION | MB_OK);
            break;
         }

         publisher.MasterLibrary      = buffer1;
         publisher.TemplateFolder = buffer2;
      }
      else
      {
         // Format is pgz. This means that each version has a key/value pair pointing to a pgz file
         // sort the key values pairs into keys for PgzFiles remove the value part
         VersionSet PgzEntries(VersionComparator(_T("Version_")));
         TCHAR sep[] = _T("\n");
         LPTSTR next_token;
         LPTSTR token = _tcstok_s(sections,sep,&next_token);
         while (token != nullptr )
         {
            std::_tstring strToken(token);

            std::_tstring::size_type pos = strToken.find(_T("PgzFiles"));
            if ( pos != std::_tstring::npos )
            {
               std::_tstring::size_type eqpos = strToken.find(_T("_PgzFiles"));
               std::_tstring strPgzEntry(strToken.substr(0,eqpos));

               try
               {
               PgzEntries.insert(strPgzEntry);
               }
               catch (CCatalogParsingException& ex)
               {
                  // Comparator can throw - we are toast
                  ex.SetLineBeingParsed(strToken.c_str());
                  throw ex;
               }
            }

            token = _tcstok_s(nullptr,sep,&next_token);
         }

        if (PgzEntries.empty())
         {
            break;
         }

         // find the key closest to the one for the current version (but not after)
         std::_tstring pgz_key(_T("Version_"));
         pgz_key += m_PGSuperVersion;

         bool did_find = FindCompatibleVersion(PgzEntries, pgz_key);
         if (!did_find)
         {
            break;
         }

         pgz_key += std::_tstring(_T("_PgzFiles"));


         TCHAR buffer1[256];
         memset(buffer1,0,sizeof(buffer1));
         DWORD dwResult1 = GetPrivateProfileString(publisherName,pgz_key.c_str(),_T(""),buffer1,sizeof(buffer1)/sizeof(TCHAR),m_strLocalCatalog);

         if ( dwResult1 == 0 )
         {
            CString msg;
            msg.Format(_T("The %s configuration could not be updated because the Pgz File Key (%s) was not be found in the server's ini file.\n\n\nPlease contact the server owner."),AfxGetApp()->m_pszProfileName,pgz_key.c_str());
            AfxMessageBox(msg,MB_ICONEXCLAMATION | MB_OK);

            return false;
         }

         publisher.MasterLibrary  = buffer1; // we store pgz file url here
      }

      // Lastly, see if we can snab a web link
      dwResult = GetPrivateProfileString(publisherName,_T("WebLink"),_T(""),buffer,sizeof(buffer)/sizeof(TCHAR),m_strLocalCatalog);
      if ( dwResult != 0 )
      {
         CString weblink(buffer);
         publisher.WebLink = weblink;
      }

      m_Publishers.push_back(publisher);
   }

   m_DidParse = true;
   return true;
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