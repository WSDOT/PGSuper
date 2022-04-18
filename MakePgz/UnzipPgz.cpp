///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2022  Washington State Department of Transportation
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
#include "stdafx.h"
#include "UnzipPgz.h" 

uzErrorType UnZipPGZ(_TCHAR* fileName,_TCHAR* folderName,bool emitMessages)
{
  HZIP hz;
  hz = OpenZip(fileName,0);
  if (hz==nullptr)
  {
     if (emitMessages)
         _tprintf(_T("Error opening compressed file: %s \n"),fileName);

     return uzErrorOpeningFile;
  }

  if (emitMessages)
     _tprintf(_T("Uncompressing: %s ...\n"),fileName);

  ZIPENTRY ze;
  ZRESULT zr = GetZipItem(hz,-1,&ze); 
  if (zr!=ZR_OK)
  {
     if (emitMessages)
        _tprintf(_T("Unknown error getting file list\n"));

     return uzErrorGettingFileList;
  }

  int numitems=ze.index;

  if (numitems<1)
  {
     if (emitMessages)
        _tprintf(_T("File is empty? No files found.\n"));

     return uzNoFilesFound;
  }

  SetUnzipBaseDir(hz,folderName);
  for (int zi=0; zi<numitems; zi++)
  {
     GetZipItem(hz,zi,&ze);

     if (emitMessages)
        _tprintf(_T("  Uncompressing:  %s\n"),ze.name);

     zr = UnzipItem(hz,zi,ze.name);
     if (zr!=ZR_OK)
     {
        if (emitMessages)
           _tprintf(_T("Unknown error extracting file\n"));

        return uzExtractingFile;
     }
  }

  if (emitMessages)
     _tprintf(_T("\nListing Complete...\n"));

  CloseZip(hz);

   return uzOk;
}

