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
//
#include "stdafx.h"
#include "ZipPgz.h"

#pragma warning(disable : 4996)

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


int ZipPGZ(_TCHAR* fileName)
{
  HZIP hz;
  hz = CreateZip(fileName,0);
  if (hz==NULL)
  {
     _tprintf(_T("Error Creating compressed file: %s \n"),fileName);
     return -1;
  }

  // Add library
  ZRESULT zr = ZipAdd(hz,_T("MasterLibrary.lbr"), _T("MasterLibrary.lbr"));
  if (zr!=ZR_OK)
  {
     _tprintf(_T("Error adding MasterLibrary.lbr - file doesn't exist?\n"));
     return -1;
  }
  else
  {
     _tprintf(_T("Added MasterLibrary.lbr...\n"));
  }

// Traverse templates folders
  WIN32_FIND_DATA FindFileData;
  HANDLE hFind;
  hFind = FindFirstFile(_T(".\\WorkgroupTemplates"), &FindFileData);
  if (hFind == INVALID_HANDLE_VALUE || !(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ) 
  {
     _tprintf(_T("Couldn't find WorkgroupTemplates folder - doesn't exist?\n"));
     return -1;
  }

  FindClose(hFind);

  hFind = FindFirstFile(_T(".\\WorkgroupTemplates\\*.*"), &FindFileData);
  if (hFind == INVALID_HANDLE_VALUE) 
  {
     _tprintf(_T("Did not find any subfolders in WorkgroupTemplates folder. No Templates added.\n"));
     return -1;
  }
  else
  {
      // Traverse templates folders
     bool first = true; // first folder is ".." and we don't want
      while (FindNextFile(hFind, &FindFileData) != 0) 
      {
         if (!first && (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
         {
            _tprintf(_T("Searching folder: %s\n"), FindFileData.cFileName);

            wchar_t DirSpec[MAX_PATH];
            _stprintf(DirSpec,_T(".\\WorkgroupTemplates\\%s\\*.pgt"),FindFileData.cFileName);

           HANDLE hsubFind;
           WIN32_FIND_DATA subFindFileData;
           hsubFind = FindFirstFile(DirSpec, &subFindFileData);
           if (hsubFind != INVALID_HANDLE_VALUE)
           {
               while (FindNextFile(hsubFind, &subFindFileData) != 0) 
               {
                  wchar_t FileSpec[MAX_PATH];
                  _stprintf(FileSpec,_T(".\\WorkgroupTemplates\\%s\\%s"),FindFileData.cFileName,subFindFileData.cFileName);

                 zr = ZipAdd(hz,FileSpec, FileSpec);
                 if (zr!=ZR_OK)
                 {
                    _tprintf (TEXT("   Uknown Error Adding file: %s\n"), FileSpec);
                    return -1;
                 }
                 else
                 {
                     _tprintf (TEXT("   Adding file: %s\n"), FileSpec);
                 }
               }

               FindClose(hsubFind);
           }
         }

         first = false;
      }

      FindClose(hFind);
  }

  CloseZip(hz);
  _tprintf(_T("Created %s \n"),fileName);

	return 0;
}
