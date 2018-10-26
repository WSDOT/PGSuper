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


#pragma once
#include <Reporting\ReportingExp.h>
#include <WBFLCore.h>
#include <Reporter\Reporter.h>

/*****************************************************************************
CLASS 
   CLibraryUsageTable

   Encapsulates the construction of the library usage table table.


DESCRIPTION
   This table reports the library entries that are in use, and where they
   come from (Master library or project library)


COPYRIGHT
   Copyright © 1997-2007
   Washington State Department Of Transportation
   All Rights Reserved

LOG
   rdp : 11.20.2007 : Created file
*****************************************************************************/

class REPORTINGCLASS CLibraryUsageTable
{
public:
   CLibraryUsageTable();
   virtual ~CLibraryUsageTable();

   //------------------------------------------------------------------------
   // Builds the table.
   virtual rptRcTable* Build(IBroker* pBroker) const;


private:
   CLibraryUsageTable(const CLibraryUsageTable& rOther);
};
