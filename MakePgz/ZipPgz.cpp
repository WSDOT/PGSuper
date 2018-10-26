///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2016  Washington State Department of Transportation
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
//
#include "stdafx.h"
#include "ZipPgz.h"
#include <vector>

#pragma warning(disable : 4996)

int FindInFolder(HZIP hz,_TCHAR* lpszFolderName,_TCHAR* lpszZipFolderName,_TCHAR* pgzFileName,_TCHAR* lpszTemplateExtension);
int FindTemplateFiles(HZIP hz,_TCHAR* lpszFolderName,_TCHAR* lpszZipFolderName,_TCHAR* pgzFileName,_TCHAR* lpszTemplateExtension);
int FindIconFiles(HZIP hz,_TCHAR* lpszFolderName,_TCHAR* lpszZipFolderName,_TCHAR* pgzFileName);
int FindFiles(HZIP hz,_TCHAR* lpszExtension,_TCHAR* lpszFolderName,_TCHAR* lpszZipFolderName,_TCHAR* pgzFileName);

int ListPGZ(_TCHAR* fileName)
{
  // List files contained in pgz
  HZIP hz;
  hz = OpenZip(fileName,0);
  if (hz==NULL)
  {
     _tprintf(_T("Error opening compressed file: %s \n"),fileName);
     return -1;
  }

  _tprintf(_T("Listing of files in: %s ...\n"),fileName);

  ZIPENTRY ze;
  GetZipItem(hz,-1,&ze); 
  int numitems=ze.index;
  for (int zi=0; zi<numitems; zi++)
  { 
    GetZipItem(hz,zi,&ze);
    _tprintf(_T("    %s\n"),ze.name);
  }

  _tprintf(_T("\nListing Complete...\n"));

  CloseZip(hz);

  return 0;
}


int ZipPGZ(_TCHAR* masterLibraryFile,_TCHAR* templateRoot,_TCHAR* pgzFileName,_TCHAR* lpszTemplateExtension)
{
  HZIP hz;
  hz = CreateZip(pgzFileName,0);
  if (hz==NULL)
  {
     _tprintf(_T("Error Creating compressed file: %s \n"),pgzFileName);
     return -1;
  }

  // Add library
  ZRESULT zr = ZipAdd(hz,_T("MasterLibrary.lbr"), masterLibraryFile);
  if (zr!=ZR_OK)
  {
     _tprintf(_T("Error adding %s - file doesn't exist?\n"),masterLibraryFile);
     return -1;
  }
  else
  {
     _tprintf(_T("Adding %s...\n"),masterLibraryFile);
  }

  // Traverse templates folders
  int retval = 0;
  WIN32_FIND_DATA FindFileData;
  HANDLE hFind;
  hFind = FindFirstFile(templateRoot, &FindFileData);
  if (hFind == INVALID_HANDLE_VALUE || !(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ) 
  {
     _tprintf(_T("Couldn't find %s folder - does it exist?\n"),templateRoot);
     retval = -1;
  }
  else
  {
     retval = FindInFolder(hz,templateRoot,_T("WorkgroupTemplates"),pgzFileName,lpszTemplateExtension);
  }

  FindClose(hFind);

  CloseZip(hz);
  _tprintf(_T("Created %s \n"),pgzFileName);

	return retval;
}

int FindInFolder(HZIP hz,_TCHAR* lpszFolderName,_TCHAR* lpszZipFolderName,_TCHAR* pgzFileName,_TCHAR* lpszTemplateExtension)
{
   int result = FindTemplateFiles(hz,lpszFolderName,lpszZipFolderName,pgzFileName,lpszTemplateExtension); // look for templates in this folder
   if ( result != -1 )
      FindIconFiles(hz,lpszFolderName,lpszZipFolderName,pgzFileName); // if there are template files, look for icon files

   // look for sub-folders
   TCHAR buffer[MAX_PATH];
   _stprintf(buffer,_T("%s\\*.*"),lpszFolderName);

   WIN32_FIND_DATA FindFileData;
   HANDLE hFind = FindFirstFile(buffer, &FindFileData);
   do
   {
      if ( FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY && 
           _tcscmp(FindFileData.cFileName,_T("."))  != 0             && 
           _tcscmp(FindFileData.cFileName,_T("..")) != 0)
      {
         TCHAR full_path[MAX_PATH];
         _stprintf(full_path,_T("%s\\%s"),lpszFolderName,FindFileData.cFileName);
         TCHAR full_zip_path[MAX_PATH];
         _stprintf(full_zip_path,_T("%s\\%s"),lpszZipFolderName,FindFileData.cFileName);
         FindInFolder(hz,full_path,full_zip_path,pgzFileName,lpszTemplateExtension);
      }

   } while ( FindNextFile(hFind,&FindFileData) );

   FindClose(hFind);

   return 0;
}

int FindTemplateFiles(HZIP hz,_TCHAR* lpszFolderName,_TCHAR* lpszZipFolderName,_TCHAR* pgzFileName,_TCHAR* lpszTemplateExtension)
{
   return FindFiles(hz,lpszTemplateExtension,lpszFolderName,lpszZipFolderName,pgzFileName);
}

int FindIconFiles(HZIP hz,_TCHAR* lpszFolderName,_TCHAR* lpszZipFolderName,_TCHAR* pgzFileName)
{
   return FindFiles(hz,_T("ico"),lpszFolderName,lpszZipFolderName,pgzFileName);
}

int FindFiles(HZIP hz,_TCHAR* lpszExtension,_TCHAR* lpszFolderName,_TCHAR* lpszZipFolderName,_TCHAR* pgzFileName)
{
   TCHAR buffer[MAX_PATH];
   _stprintf(buffer,_T("%s\\*.%s"),lpszFolderName,lpszExtension);

   WIN32_FIND_DATA FindFileData;
   HANDLE hFind = FindFirstFile(buffer, &FindFileData);
   if ( hFind == INVALID_HANDLE_VALUE )
      return -1; // there aren't any pgt files in this folder

   do
   {
      TCHAR FileSpec1[MAX_PATH], FileSpec2[MAX_PATH];
      _stprintf(FileSpec1,_T("%s\\%s"),lpszZipFolderName,FindFileData.cFileName);
      _stprintf(FileSpec2,_T("%s\\%s"),lpszFolderName,FindFileData.cFileName);

      ZRESULT zr = ZipAdd(hz,FileSpec1,FileSpec2);
      if (zr != ZR_OK)
      {
         _tprintf (TEXT("   Unknown Error Adding file: %s\n"), FileSpec2);
         return -1;
      }
      else
      {
         _tprintf (TEXT("   Adding file: %s\n"), FileSpec2);
      }

   } while ( FindNextFile(hFind,&FindFileData) );

   FindClose(hFind);

   return 0;
}