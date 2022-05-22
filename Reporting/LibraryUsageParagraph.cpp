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
#include <Reporting\LibraryUsageParagraph.h>
#include <Reporting\LibraryUsageTable.h>

#include <IFace\Project.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CLibraryUsageParagraph
****************************************************************************/

CLibraryUsageParagraph::CLibraryUsageParagraph()
{
}

CLibraryUsageParagraph::~CLibraryUsageParagraph()
{
}

rptParagraph* CLibraryUsageParagraph::Build(IBroker* pBroker, bool doPrintTable) const
{
   rptParagraph* pParagraph = new rptParagraph;

   GET_IFACE2(pBroker,ILibrary,pLibrary);
   WBFL::System::Time time;
   bool bPrintDate = WBFL::System::Time::PrintDate(true);
   
   std::_tstring strServer;
   std::_tstring strConfiguration;
   std::_tstring strMasterLibFile;
   pLibrary->GetMasterLibraryInfo(strServer,strConfiguration,strMasterLibFile,time);

   *pParagraph << _T("Configuration Server: ") << strServer << rptNewLine;
   *pParagraph << _T("Configuration Name: ") << strConfiguration << rptNewLine;
   *pParagraph << _T("Configuration Source: ") << strMasterLibFile << rptNewLine;
   *pParagraph << _T("Configuration Date Stamp: ") << time.AsString() << rptNewLine;

   if (doPrintTable)
   {
      rptRcTable* table = CLibraryUsageTable().Build(pBroker);
      *pParagraph << table << rptNewLine;
   }

   WBFL::System::Time::PrintDate(bPrintDate);

   return pParagraph;
}
