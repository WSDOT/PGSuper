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

#ifndef INCLUDED_COMBINEDREACTIONTABLE_H_
#define INCLUDED_COMBINEDREACTIONTABLE_H_

#include <Reporting\ReportingExp.h>
#include <Reporting\ReactionInterfaceAdapters.h>

interface IEAFDisplayUnits;

/*****************************************************************************
CLASS 
   CCombinedReactionTable

   Encapsulates the construction of the combined reaction table.


DESCRIPTION
   Encapsulates the construction of the combined reaction table.

LOG
   rab : 11.08.1998 : Created file
*****************************************************************************/

class REPORTINGCLASS CCombinedReactionTable
{
public:
   // This class serves dual duty. It can report pier reactions or girder bearing reactions.
   // The two are identical except for the title and the interfaces they use to get responses

   // GROUP: LIFECYCLE

   //------------------------------------------------------------------------
   // Default constructor
   CCombinedReactionTable();

   //------------------------------------------------------------------------
   // Copy constructor
   CCombinedReactionTable(const CCombinedReactionTable& rOther);

   //------------------------------------------------------------------------
   // Destructor
   virtual ~CCombinedReactionTable();

   // GROUP: OPERATORS
   //------------------------------------------------------------------------
   // Assignment operator
   CCombinedReactionTable& operator = (const CCombinedReactionTable& rOther);

   // GROUP: OPERATIONS

   //------------------------------------------------------------------------
   // Builds the combined results table
   // bDesign and bRating are only considered for intervalIdx = live load interval index
   void Build(IBroker* pBroker, rptChapter* pChapter,
              const CGirderKey& girderKey,
              IEAFDisplayUnits* pDisplayUnits,
              IntervalIndexType intervalIdx,pgsTypes::AnalysisType analysisType, ReactionTableType tableType,
              bool bDesign,bool bRating) const;

   //------------------------------------------------------------------------
   // Builds the live load results table
   // bDesign and bRating are only considered from stage = pgsTypes::BridgeSite3
   void BuildLiveLoad(IBroker* pBroker, rptChapter* pChapter,
              const CGirderKey& girderKey,
              IEAFDisplayUnits* pDisplayUnits,
              pgsTypes::AnalysisType analysisType, 
              bool bIncludeImpact, bool bDesign,bool bRating) const;


   //------------------------------------------------------------------------
   // Builds tables for bearing design
   void BuildForBearingDesign(IBroker* pBroker, rptChapter* pChapter,
              const CGirderKey& girderKey,
              IEAFDisplayUnits* pDisplayUnits,
              IntervalIndexType intervalIdx,pgsTypes::AnalysisType analysisType,bool bIncludeImpact) const;

   // GROUP: ACCESS
   // GROUP: INQUIRY

protected:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   void BuildCombinedDeadTable(IBroker* pBroker, rptChapter* pChapter,
              const CGirderKey& girderKey,
              IEAFDisplayUnits* pDisplayUnits,
              IntervalIndexType intervalIdx,pgsTypes::AnalysisType analysisType, ReactionTableType tableType,
              bool bDesign=true,bool bRating=true) const;

   void BuildBearingLimitStateTable(IBroker* pBroker, rptChapter* pChapter,
              const CGirderKey& girderKey,bool bIncludeImpact,
              IEAFDisplayUnits* pDisplayUnits,IntervalIndexType intervalIdx,
              pgsTypes::AnalysisType analysisType,
              bool bDesign=true,bool bRating=true) const;

   //------------------------------------------------------------------------
   void MakeCopy(const CCombinedReactionTable& rOther);

   //------------------------------------------------------------------------
   void MakeAssignment(const CCombinedReactionTable& rOther);

   // GROUP: ACCESS
   // GROUP: INQUIRY

private:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   // GROUP: ACCESS
   // GROUP: INQUIRY
};

// INLINE METHODS
//

// EXTERNAL REFERENCES
//

#endif // INCLUDED_COMBINEDREACTIONTABLE_H_
