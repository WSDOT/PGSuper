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

#pragma once

#include <Reporting\ReportingExp.h>
#include <IFace\AnalysisResults.h>
#include <Reporting\ReactionInterfaceAdapters.h>

interface IEAFDisplayUnits;

/*****************************************************************************
CLASS 
   CLiveLoadReactionTable

   Encapsulates the construction of the combined reaction table.


DESCRIPTION
   Encapsulates the construction of the combined reaction table.

LOG
   rab : 04.01.2008 : Created file
*****************************************************************************/

class REPORTINGCLASS CLiveLoadReactionTable
{
public:
   // This class serves dual duty. It can report pier reactions or girder bearing reactions.
   // The two are identical except for the title and the interfaces they use to get responses


   //------------------------------------------------------------------------
   // Default constructor
   CLiveLoadReactionTable();

   //------------------------------------------------------------------------
   // Copy constructor
   CLiveLoadReactionTable(const CLiveLoadReactionTable& rOther);

   //------------------------------------------------------------------------
   // Destructor
   virtual ~CLiveLoadReactionTable();

   //------------------------------------------------------------------------
   // Assignment operator
   CLiveLoadReactionTable& operator = (const CLiveLoadReactionTable& rOther);

   //------------------------------------------------------------------------
   // Builds the strand eccentricity table.
   virtual void Build(IBroker* pBroker, rptChapter* pChapter,
                      const CGirderKey& girderKey,
                      IEAFDisplayUnits* pDisplayUnits, ReactionTableType tableType,
                      IntervalIndexType intervalIdx, pgsTypes::AnalysisType analysisType) const;

protected:
   //------------------------------------------------------------------------
   void MakeCopy(const CLiveLoadReactionTable& rOther);

   //------------------------------------------------------------------------
   void MakeAssignment(const CLiveLoadReactionTable& rOther);
};
