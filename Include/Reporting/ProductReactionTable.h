///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2025  Washington State Department of Transportation
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
#include <IFace\AnalysisResults.h>
#include <Reporting\ReactionInterfaceAdapters.h>

class IEAFDisplayUnits;

/*****************************************************************************
CLASS 
   CProductReactionTable

   Encapsulates the construction of the product reaction table.


DESCRIPTION
   Encapsulates the construction of the product reaction table.

LOG
   rab : 11.05.1998 : Created file
*****************************************************************************/

class REPORTINGCLASS CProductReactionTable
{
public:
   CProductReactionTable();

   // Builds the strand eccentricity table.
   virtual rptRcTable* Build(std::shared_ptr<WBFL::EAF::Broker> pBroker,const CGirderKey& girderKey,pgsTypes::AnalysisType analysisType,
                             ReactionTableType type, bool bIncludedImpact, bool bDesign,bool bRating,bool bIndicateControllingLoad,
                             std::shared_ptr<IEAFDisplayUnits> pDisplayUnits) const;
};
