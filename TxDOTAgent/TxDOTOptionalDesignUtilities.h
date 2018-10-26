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
// 
#ifndef INCLUDED_PGSEXT_TXDOTOPTIONALDESIGNUTILILITIES_H_
#define INCLUDED_PGSEXT_TXDOTOPTIONALDESIGNUTILILITIES_H_

// SYSTEM INCLUDES
//


// PROJECT INCLUDES
//
#include <Material\PsStrand.h>
#include <System\Tokenizer.h>
#include <System\FileStream.h>

// LOCAL INCLUDES
//

// FORWARD DECLARATIONS
//

// MISCELLANEOUS
//
// Span and girder for our template model
#define TOGA_SPAN 0
#define TOGA_NUM_GDRS 8
#define TOGA_ORIG_GDR 3
#define TOGA_FABR_GDR 4

/*****************************************************************************
CLASS 

DESCRIPTION
   Utilitity functions and classes

COPYRIGHT
   Copyright © 1997-2010
   Washington State Department Of Transportation
   All Rights Reserved

LOG
   rdp : 02.19.2010 : Created file
*****************************************************************************/

   // Location of template folders
inline CString GetTOGAFolder()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CString strHelpFolderName =  AfxGetApp()->m_pszHelpFilePath;
   int loc = strHelpFolderName.ReverseFind(_T('\\'));
   CString strWorkgroupFolderName = strHelpFolderName.Left(loc+1) + _T("TogaTemplates");

   return strWorkgroupFolderName;
}


inline CString get_strand_size( matPsStrand::Size size )
{
   CString sz;
   switch( size )
   {
   case matPsStrand::D635:
      sz = _T("1/4\"");
      break;

   case matPsStrand::D794:
      sz = _T("5/16\"");
      break;

   case matPsStrand::D953:
      sz = _T("3/8\"");
      break;

   case matPsStrand::D1111:
      sz = _T("7/16\"");
      break;

   case matPsStrand::D1270:
      sz = _T("1/2\"");
      break;

   case matPsStrand::D1320:
      sz = _T("1/2\" Special (0.52\")");
      break;

   case matPsStrand::D1524:
      sz = _T("0.6\"");
      break;

   case matPsStrand::D1575:
      sz = _T("0.62\"");
      break;

   case matPsStrand::D1778:
      sz = _T("0.7\"");
      break;

   default:
      ATLASSERT(false); // should never get here (unless there is a new strand type)
   }

   return sz;
}

inline BOOL ParseTemplateFile(const LPCTSTR lpszPathName, CString& girderEntry, CString& leftConnEntry, CString& rightConnEntry)
{
   // Read girder type, connection types, and pgsuper template file name
   std::_tifstream ifile(lpszPathName);
   if ( !ifile )
   {
      CString msg;
      msg.Format(_T("Error opening template file: %s - File not found?"),lpszPathName);
      AfxMessageBox(msg );
      ASSERT( 0 ); // this should never happen
      return FALSE;
   }

   TCHAR line[1024];
   ifile.getline(line,1024);

   // comma delimited file in format of:
   // GirderEntryName, EndConnection, StartConnection, TemplateFile
   sysTokenizer tokenizer(_T(","));
   tokenizer.push_back(line);

   int nitems = tokenizer.size();
   if (nitems!=4 && nitems!=3)
   {
      CString msg;
      msg.Format(_T("Error reading template file: %s - Invalid Format"),lpszPathName);
      AfxMessageBox(msg );
      return FALSE;
   }

   // set our data values
   girderEntry = tokenizer[0].c_str();
   leftConnEntry = tokenizer[1].c_str();
   rightConnEntry = tokenizer[2].c_str();

   return TRUE;
}

#endif // INCLUDED_PGSEXT_TXDOTOPTIONALDESIGNUTILILITIES_H_

