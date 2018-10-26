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

#include "stdafx.h"
#include "pgsuper.h"
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

bool CPGSuperCatalog::Init(const char* strIniFileName, const CString& strPGSuperVersion)
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

void CPGSuperCatalog::GetCatalogSettings(const char* publisher,CString& strMasterLibrary,CString& strWorkgroupTemplates)
{
   const Publisher* pPub = GetPublisher(publisher);
   ATLASSERT(pPub->Format==ctOriginal);

   strMasterLibrary      = pPub->MasterLibrary;
   strWorkgroupTemplates = pPub->TemplateFolder;
}

void CPGSuperCatalog::GetCatalogSettings(const char* publisher,CString& strPgzFile)
{
   const Publisher* pPub = GetPublisher(publisher);
   ATLASSERT(pPub->Format==ctPgz);

   strPgzFile      = pPub->MasterLibrary;
}

const CPGSuperCatalog::Publisher* CPGSuperCatalog::GetPublisher(const char* publisher)
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

   char buffer[256];
   DWORD dwResult = GetPrivateProfileSectionNames(buffer,sizeof(buffer),m_strLocalCatalog);
   // buffer contains a null terminated separated list of section names

   char* token = buffer;
   while ( *token != 0x00 )
   {
      CString strItem(token);
      CString strMasterLibrary;
      CString strWorkgroupTemplates;

      publishers.push_back(strItem);

      token = token + strItem.GetLength()+1;
   }

   // cycle through publishers and build list
   for (std::vector<CString>::iterator pubit=publishers.begin(); pubit!=publishers.end(); pubit++)
   {
      const CString publisherName(*pubit);

      Publisher publisher;
      publisher.Name = publisherName;
      publisher.Format = ctOriginal;

      // Determine what format we have
      dwResult = GetPrivateProfileString(publisherName,"Format","",buffer,sizeof(buffer),m_strLocalCatalog);
      if ( dwResult != 0 )
      {
         CString format(buffer);
         if(format.CompareNoCase("pgz")==0)
         {
            publisher.Format = ctPgz;
         }
      }

      // get all the key value pairs for this publisher
      char sections[32767];
      ASSERT( sizeof(sections) <= 32767 );

      memset((void*)sections,0,sizeof(sections));
      DWORD dwResult = GetPrivateProfileSection(publisherName,sections,sizeof(sections),m_strLocalCatalog);
      ASSERT( dwResult != sizeof(sections)-2 );

      for ( int i = 0; i < sizeof(sections)/sizeof(char); i++ )
      {
         if ( sections[i] == '\0' )
            sections[i] = '\n';
      }

      if (publisher.Format == ctOriginal)
      {
         // Original .ini file format. This consists of key value pairs for both the 
         // library file location and template.

         // sort the key values pairs into keys for MasterLibrary and WorkgroupTemplates
         // remove the value part
         std::set<std::string> MasterLibraryEntries, WorkgroupTemplateEntries;
         char sep[] = "\n";
         char* next_token;
         char* token = strtok_s(sections,sep,&next_token);
         while (token != NULL )
         {
            std::string strToken(token);

            std::string::size_type pos = strToken.find("MasterLibrary");
            if ( pos != std::string::npos )
            {
               std::string::size_type eqpos = strToken.find("_MasterLibrary");
               MasterLibraryEntries.insert(strToken.substr(0,eqpos));
            }

            pos = strToken.find("WorkgroupTemplates");
            if ( pos != std::string::npos )
            {
               std::string::size_type eqpos = strToken.find("_WorkgroupTemplates");
               WorkgroupTemplateEntries.insert(strToken.substr(0,eqpos));
            }

            token = strtok_s(NULL,sep,&next_token);
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
         std::string master_library_key("Version_");
         master_library_key += m_PGSuperVersion;

         std::set<std::string>::const_iterator found = MasterLibraryEntries.find(master_library_key);
         if ( found == MasterLibraryEntries.end() )
         {
            // not in the set... add it and then go back one
            std::pair<std::set<std::string>::iterator,bool> result = MasterLibraryEntries.insert(master_library_key);
            ASSERT( result.second == true );
            std::set<std::string>::iterator insert_loc = result.first;
            insert_loc--;
            master_library_key = *insert_loc;
         }
         master_library_key += std::string("_MasterLibrary");

         std::string workgroup_template_key("Version_");
         workgroup_template_key += m_PGSuperVersion;

         found = WorkgroupTemplateEntries.find(workgroup_template_key);
         if ( found == WorkgroupTemplateEntries.end() )
         {
            // not in the set... add it and then go back one
            std::pair<std::set<std::string>::iterator,bool> result = WorkgroupTemplateEntries.insert(workgroup_template_key);
            ASSERT( result.second == true );
            std::set<std::string>::iterator insert_loc = result.first;
            insert_loc--;
            workgroup_template_key = *insert_loc;
         }
         workgroup_template_key += CString("_WorkgroupTemplates");


         char buffer1[256];
         DWORD dwResult1 = GetPrivateProfileString(publisherName,master_library_key.c_str(),"",buffer1,sizeof(buffer1),m_strLocalCatalog);
         CString msg1;
         if ( dwResult1 == 0 )
            msg1.Format("Could not find Master Library Key: %s",master_library_key.c_str());



         char buffer2[256];
         DWORD dwResult2 = GetPrivateProfileString(publisherName,workgroup_template_key.c_str(),"",buffer2,sizeof(buffer2),m_strLocalCatalog);
         CString msg2;
         if ( dwResult2 == 0 )
            msg2.Format("Could not find Workgroup Template Key: %s",workgroup_template_key.c_str());


         if ( (dwResult1 == 0 || dwResult2 == 0) )
         {
            CString msg;
            msg.Format("The Master Library and Workgroup Templates could not be updated because PGSuper:\n\n%s\n%s\n\nPlease contact the server owner.",msg1,msg2);
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
         std::set<std::string> PgzEntries;
         char sep[] = "\n";
         char* next_token;
         char* token = strtok_s(sections,sep,&next_token);
         while (token != NULL )
         {
            std::string strToken(token);

            std::string::size_type pos = strToken.find("PgzFiles");
            if ( pos != std::string::npos )
            {
               std::string::size_type eqpos = strToken.find("_PgzFiles");
               PgzEntries.insert(strToken.substr(0,eqpos));
            }

            token = strtok_s(NULL,sep,&next_token);
         }

        if (PgzEntries.empty())
         {
            break;
         }

         // find the key closest to the one for the current version (but not after)
         std::string pgz_key("Version_");
         pgz_key += m_PGSuperVersion;

         std::set<std::string>::const_iterator found = PgzEntries.find(pgz_key);
         if ( found == PgzEntries.end() )
         {
            // not in the set... add it and then go back one
            std::pair<std::set<std::string>::iterator,bool> result = PgzEntries.insert(pgz_key);
            ASSERT( result.second == true );
            std::set<std::string>::iterator insert_loc = result.first;
            insert_loc--;
            pgz_key = *insert_loc;
         }
         pgz_key += std::string("_PgzFiles");


         char buffer1[256];
         DWORD dwResult1 = GetPrivateProfileString(publisherName,pgz_key.c_str(),"",buffer1,sizeof(buffer1),m_strLocalCatalog);

         if ( dwResult1 == 0 )
         {
            CString msg;
            msg.Format("The Master Library and Workgroup Templates could not be updated because PGSuper could not find the Pgz File Key:\n%s\nin the server's ini file.\n\n\nPlease contact the server owner.",pgz_key);
            AfxMessageBox(msg,MB_ICONEXCLAMATION | MB_OK);

            return false;
         }

         publisher.MasterLibrary  = buffer1; // we store pgz file url here
      }

      // Lastly, see if we can snab a web link
      dwResult = GetPrivateProfileString(publisherName,"WebLink","",buffer,sizeof(buffer),m_strLocalCatalog);
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
