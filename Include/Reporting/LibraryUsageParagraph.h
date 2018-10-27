///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2018  Washington State Department of Transportation
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

#ifndef INCLUDED_LIBRARYUSAGEPARAGRAPH_H_
#define INCLUDED_LIBRARYUSAGEPARAGRAPH_H_

#include <Reporting\ReportingExp.h>
#include <Reporter\Reporter.h>
#include <WBFLCore.h>

/*****************************************************************************
CLASS 
   CLibraryUsageParagraph

   Encapsulates the construction of the library usage paragraph.


DESCRIPTION
   Creates a paragraph that reports library usage.

LOG
   rdp : 11.20.2007 : Created file
*****************************************************************************/

class REPORTINGCLASS CLibraryUsageParagraph
{
public:
   CLibraryUsageParagraph();
   virtual ~CLibraryUsageParagraph();

   virtual rptParagraph* Build(IBroker* pBroker, bool doPrintTable=true) const;

private:
   CLibraryUsageParagraph(const CLibraryUsageParagraph& rOther);
};

#endif // INCLUDED_LIBRARYUSAGEPARAGRAPH_H_
