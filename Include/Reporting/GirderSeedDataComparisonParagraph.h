///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2014  Washington State Department of Transportation
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

#ifndef INCLUDED_GIRDERSEEDDATACOMPARISONPARAGRAPH_H_
#define INCLUDED_GIRDERSEEDDATACOMPARISONPARAGRAPH_H_

#include <Reporting\ReportingExp.h>
#include <Reporter\Reporter.h>
#include <WBFLCore.h>

/*****************************************************************************
CLASS 
   CGirderSeedDataComparisonParagraph

   Encapsulates the construction of the paragraph comparing girder seed data.


DESCRIPTION
   Creates a paragraph that reports girder seed data usage.


COPYRIGHT
   Copyright © 1997-2007
   Washington State Department Of Transportation
   All Rights Reserved

LOG
   rdp : 12.16.2009 : Created file
*****************************************************************************/

class REPORTINGCLASS CGirderSeedDataComparisonParagraph
{
public:
   CGirderSeedDataComparisonParagraph();
   virtual ~CGirderSeedDataComparisonParagraph();

   virtual rptParagraph* Build(IBroker* pBroker, const CGirderKey& girderKey) const;

private:
   CGirderSeedDataComparisonParagraph(const CGirderSeedDataComparisonParagraph& rOther);
};

#endif // INCLUDED_GIRDERSEEDDATACOMPARISONPARAGRAPH_H_
