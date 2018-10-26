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

// PGSuperCatalog.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\PGSuperApp.h"
#include "PGSuperCatalog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CPGSuperCatalog::CPGSuperCatalog():
m_DidParse(false)
{
}

bool CPGSuperCatalog::Init(LPCTSTR strIniFileName, const CString& strPGSuperVersion)
{
   m_strLocalCatalog = strIniFileName;
   m_PGSuperVersion = strPGSuperVersion;
   m_DidParse = false;
   m_Publishers.clear();

   return DoParse();
}

std::vector<CString> CPGSuperCatalog::GetPublishers()
{
   ATLASSERT(m_DidParse);
   std::vector<CString> pubs;
   for (PublisherIterator it=m_Publishers.begin(); it!=m_Publishers.end(); it++)
   {
      pubs.push_back(it->Name);
   }

   return pubs;
}

bool CPGSuperCatalog::DoesPublisherExist(const CString& publisher)
{
   const Publisher* pPub = GetPublisher(publisher);

   return pPub!=NULL;
}

CPGSuperCatalog::Format CPGSuperCatalog::GetFormat(const CString& publisher)
{
   const Publisher* pPub = GetPublisher(publisher);
   return pPub->Format;
}

CString CPGSuperCatalog::GetWebLink(const CString& publisher)
{
   const Publisher* pPub = GetPublisher(publisher);
   return pPub->WebLink;
}

void CPGSuperCatalog::GetCatalogSettings(LPCTSTR publisher,CString& strMasterLibrary,CString& strWorkgroupTemplates)
{
   const Publisher* pPub = GetPublisher(publisher);
   ATLASSERT(pPub->Format==ctOriginal);

   strMasterLibrary      = pPub->MasterLibrary;
   strWorkgroupTemplates = pPub->TemplateFolder;
}

void CPGSuperCatalog::GetCatalogSettings(LPCTSTR publisher,CString& strPgzFile)
{
   const Publisher* pPub = GetPublisher(publisher);
   ATLASSERT(pPub->Format==ctPgz);

   strPgzFile      = pPub->MasterLibrary;
}

const CPGSuperCatalog::Publisher* CPGSuperCatalog::GetPublisher(LPCTSTR publisher)
{
   for (PublisherIterator it=m_Publishers.begin(); it!=m_Publishers.end(); it++)
   {
      if(it->Name.Compare(publisher)==0)
      {
         return &(*it);
      }
   }

   ATLASSERT(0);
   return NULL;
}


bool CPGSuperCatalog::DoParse()
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
         std::set<std::_tstring> MasterLibraryEntries, WorkgroupTemplateEntries;
         TCHAR sep[] = _T("\n");
         LPTSTR next_token;
         LPTSTR token = _tcstok_s(sections,sep,&next_token);
         while (token != NULL )
         {
            std::_tstring strToken(token);

            std::_tstring::size_type pos = strToken.find(_T("MasterLibrary"));
            if ( pos != std::_tstring::npos )
            {
               std::_tstring::size_type eqpos = strToken.find(_T("_MasterLibrary"));
               MasterLibraryEntries.insert(strToken.substr(0,eqpos));
            }

            pos = strToken.find(_T("WorkgroupTemplates"));
            if ( pos != std::_tstring::npos )
            {
               std::_tstring::size_type eqpos = strToken.find(_T("_WorkgroupTemplates"));
               WorkgroupTemplateEntries.insert(strToken.substr(0,eqpos));
            }

            token = _tcstok_s(NULL,sep,&next_token);
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

         std::set<std::_tstring>::const_iterator found = MasterLibraryEntries.find(master_library_key);
         if ( found == MasterLibraryEntries.end() )
         {
            // not in the set... add it and then go back one
            std::pair<std::set<std::_tstring>::iterator,bool> result = MasterLibraryEntries.insert(master_library_key);
            ASSERT( result.second == true );
            std::set<std::_tstring>::iterator insert_loc = result.first;
            insert_loc--;
            master_library_key = *insert_loc;
         }
         master_library_key += std::_tstring(_T("_MasterLibrary"));

         std::_tstring workgroup_template_key(_T("Version_"));
         workgroup_template_key += m_PGSuperVersion;

         found = WorkgroupTemplateEntries.find(workgroup_template_key);
         if ( found == WorkgroupTemplateEntries.end() )
         {
            // not in the set... add it and then go back one
            std::pair<std::set<std::_tstring>::iterator,bool> result = WorkgroupTemplateEntries.insert(workgroup_template_key);
            ASSERT( result.second == true );
            std::set<std::_tstring>::iterator insert_loc = result.first;
            insert_loc--;
            workgroup_template_key = *insert_loc;
         }
         workgroup_template_key += CString(_T("_WorkgroupTemplates"));


         TCHAR buffer1[256];
         memset(buffer1,0,sizeof(buffer1));
         DWORD dwResult1 = GetPrivateProfileString(publisherName,master_library_key.c_str(),_T(""),buffer1,sizeof(buffer1)/sizeof(TCHAR),m_strLocalCatalog);
         CString msg1;
         if ( dwResult1 == 0 )
            msg1.Format(_T("Could not find Master Library Key: %s"),master_library_key.c_str());



         TCHAR buffer2[256];
         memset(buffer2,0,sizeof(buffer2));
         DWORD dwResult2 = GetPrivateProfileString(publisherName,workgroup_template_key.c_str(),_T(""),buffer2,sizeof(buffer2)/sizeof(TCHAR),m_strLocalCatalog);
         CString msg2;
         if ( dwResult2 == 0 )
            msg2.Format(_T("Could not find Workgroup Template Key: %s"),workgroup_template_key.c_str());


         if ( (dwResult1 == 0 || dwResult2 == 0) )
         {
            CString msg;
            msg.Format(_T("The Master Library and Workgroup Templates could not be updated because PGSuper:\n\n%s\n%s\n\nPlease contact the server owner."),msg1,msg2);
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
         std::set<std::_tstring> PgzEntries;
         TCHAR sep[] = _T("\n");
         LPTSTR next_token;
         LPTSTR token = _tcstok_s(sections,sep,&next_token);
         while (token != NULL )
         {
            std::_tstring strToken(token);

            std::_tstring::size_type pos = strToken.find(_T("PgzFiles"));
            if ( pos != std::_tstring::npos )
            {
               std::_tstring::size_type eqpos = strToken.find(_T("_PgzFiles"));
               std::_tstring strPgzEntry(strToken.substr(0,eqpos));
               PgzEntries.insert(strPgzEntry);
            }

            token = _tcstok_s(NULL,sep,&next_token);
         }

        if (PgzEntries.empty())
         {
            break;
         }

         // find the key closest to the one for the current version (but not after)
         std::_tstring pgz_key(_T("Version_"));
         pgz_key += m_PGSuperVersion;

         std::set<std::_tstring>::const_iterator found( PgzEntries.find(pgz_key) );
         if ( found == PgzEntries.end() )
         {
            // not in the set... add it and then go back one
            std::pair<std::set<std::_tstring>::iterator,bool> result( PgzEntries.insert(pgz_key) );
            ASSERT( result.second == true );
            std::set<std::_tstring>::iterator insert_loc( result.first );
            insert_loc--;
            pgz_key = *insert_loc;
         }
         pgz_key += std::_tstring(_T("_PgzFiles"));


         TCHAR buffer1[256];
         memset(buffer1,0,sizeof(buffer1));
         DWORD dwResult1 = GetPrivateProfileString(publisherName,pgz_key.c_str(),_T(""),buffer1,sizeof(buffer1)/sizeof(TCHAR),m_strLocalCatalog);

         if ( dwResult1 == 0 )
         {
            CString msg;
            msg.Format(_T("The Master Library and Workgroup Templates could not be updated because PGSuper could not find the Pgz File Key:\n%s\nin the server's ini file.\n\n\nPlease contact the server owner."),pgz_key);
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
