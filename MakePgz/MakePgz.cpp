///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2011  Washington State Department of Transportation
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

#include <string>

#pragma warning(disable : 4996)

void PrintCommandLine();

int _tmain(int argc, _TCHAR* argv[])
{
   if (argc!=3 && argc!=4)
   {
     PrintCommandLine();
     return -1;
   }

   if (argc==4)
   {

      // Zipping master library file and templates
      int st = ZipPGZ(argv[2], // master library file
                      argv[3], // root of workgroup templates
                      argv[1]);// pgz file
      if (st==0)
      {
         _tprintf(_T("Last step is to create MD5 checksum:\n"));

         // get MD5 command (assume MD5Deep.exe is in the same folder as us
         std::basic_string<wchar_t> strMD5Deep(argv[0]);

         // make all lower case
         std::basic_string<wchar_t>::iterator begin = strMD5Deep.begin();
         std::basic_string<wchar_t>::iterator end   = strMD5Deep.end();
         while ( begin != end )
         {
            *begin = tolower(*begin);
            begin++;
         }

         std::basic_string<wchar_t>::size_type loc = strMD5Deep.find(_T("makepgz.exe"));
         if (loc == std::basic_string<wchar_t>::npos)
         {
            // alternate command is to drop the .exe
            loc = strMD5Deep.find(_T("makepgz"));

            if (loc == std::basic_string<wchar_t>::npos)
            {
               _tprintf(_T("FATAL ERROR - Could not parse command line to make MD5"));
            }
         }

         strMD5Deep.replace(strMD5Deep.begin()+loc,strMD5Deep.end(),_T("md5deep.exe"));

         wchar_t arg1[MAX_PATH], arg2[MAX_PATH];
         _stprintf(arg1,_T("%s"),argv[1]);
         _stprintf(arg2,_T("%s.md5"),argv[1]);
         int result = _wspawnl(_P_WAIT,strMD5Deep.c_str(),_T("-q"),arg1,_T(">"),arg2,NULL);

         wchar_t cmd[MAX_PATH];
         _stprintf(cmd, _T("\"%s\" -q %s > %s.md5"),strMD5Deep.c_str(),argv[1],argv[1]); 

         _tprintf(_T("  Executing command: \"%s\"\n"), cmd);
         result = _wsystem(cmd);

         _tprintf(_T("    Result = %d\n"), result);
      }
   }
   else if ( argc == 3 )
   {
      // unzipping or listing
      if (argv[2][1]==_T('L')) // listing
      {
         return ListPGZ(argv[1]);
      }
      else if (argv[2][1]==_T('U')) // unzipping
      {
         return UnZipPGZ(argv[1],_T(".\\"),true);
      }
   }
   else
   {
      // command line error
     PrintCommandLine();
     return -1;
   }
}


void PrintCommandLine()
{
     _tprintf(_T("Error - Invalid Command line!\n"));
     _tprintf(_T("To uncompress a PGZ file:\n"));
     _tprintf(_T("    makepgz FileName.pgz /U\n"));
     _tprintf(_T("\n"));
     _tprintf(_T("To list the contents of a PGZ file:\n"));
     _tprintf(_T("    makepgz FileName.pgz /L\n"));
     _tprintf(_T("\n"));
     _tprintf(_T("To create a PGZ file:\n"));
     _tprintf(_T("    makepgz FileName.pgz Library.lbr RootTemplateFolder"));
     _tprintf(_T("\n"));
     _tprintf(_T("FileName.pgz = the name of the PGZ file to list, uncompress, or create\n"));
     _tprintf(_T("Library.lbr  = the name of the PGSuper Master Library file to add\n"));
     _tprintf(_T("RootTemplateFolder = the name of the root template folder that contains a tree of PGSuper Project Templates to add\n"));
}
