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

//// MakePgz.cpp : Defines the entry point for the console application.
//
#include "stdafx.h"
#include "ZipPgz.h"
#include "UnzipPgz.h"

#include "process.h"

#pragma warning(disable : 4996)

void PrintCommandLine();

int _tmain(int argc, _TCHAR* argv[])
{
   if (argc!=2 && argc!=3)
   {
     PrintCommandLine();
     return -1;
   }

   if (argc==2)
   {
      int st = ZipPGZ(argv[1]);
      if (st==0)
      {
         _tprintf(_T("Last step is to create MD5 checksum:\n"));

         wchar_t arg1[MAX_PATH], arg2[MAX_PATH];
         _stprintf(arg1,_T(".\\%s"),argv[1]);
         _stprintf(arg2,_T(".\\%s.md5"),argv[1]);
         int result = _wspawnl(_P_WAIT,_T("MD5Deep.exe"),_T("-q"),arg1,_T(">"),arg2,NULL);

         wchar_t cmd[MAX_PATH];
         _stprintf(cmd, _T("MD5Deep.exe -q .\\%s > .\\%s.md5"),argv[1],argv[1]);

         _tprintf(_T("  Executing command: \"%s\"\n"), cmd);
         result = _wsystem(cmd);

         _tprintf(_T("    Result = %d\n"), result);
      }
   }
   else if (argv[2][1]=='L')
   {
      return ListPGZ(argv[1]);
   }
   else if (argv[2][1]=='U')
   {
      return UnZipPGZ(argv[1],_T(".\\"),true);
   }
   else
   {
     PrintCommandLine();
     return -1;
   }
}


void PrintCommandLine()
{
     _tprintf(_T("Error - Invalid Command line! Valid format is:\n"));
     _tprintf(_T("    LTCompressor <FileName.pgz> [/L or /U]\n"));
     _tprintf(_T("\n"));
     _tprintf(_T("L - List files in pgz\n"));
     _tprintf(_T("U - Uncompress to current folder\n"));
}
