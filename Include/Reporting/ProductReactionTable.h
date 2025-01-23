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

#ifndef INCLUDED_PRODUCTREACTIONTABLE_H_
#define INCLUDED_PRODUCTREACTIONTABLE_H_

#include <Reporting\ReportingExp.h>
#include <IFace\AnalysisResults.h>
#include <Reporting\ReactionInterfaceAdapters.h>

interface IEAFDisplayUnits;

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
   // This class serves dual duty. It can report pier reactions or girder bearing reactions.
   // The two are identical except for the title and the interfaces they use to get responses

   // GROUP: LIFECYCLE

   //------------------------------------------------------------------------
   // Default constructor
   CProductReactionTable();

   //------------------------------------------------------------------------
   // Copy constructor
   CProductReactionTable(const CProductReactionTable& rOther);

   //------------------------------------------------------------------------
   // Destructor
   virtual ~CProductReactionTable();

   // GROUP: OPERATORS
   //------------------------------------------------------------------------
   // Assignment operator
   CProductReactionTable& operator = (const CProductReactionTable& rOther);

   // GROUP: OPERATIONS

   //------------------------------------------------------------------------
   // Builds the strand eccentricity table.
   virtual rptRcTable* Build(IBroker* pBroker,const CGirderKey& girderKey,pgsTypes::AnalysisType analysisType,
                             ReactionTableType type, bool bIncludedImpact, bool bDesign,bool bRating,bool bIndicateControllingLoad,
                             IEAFDisplayUnits* pDisplayUnits) const;
   // GROUP: ACCESS
   // GROUP: INQUIRY

protected:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   //------------------------------------------------------------------------
   void MakeCopy(const CProductReactionTable& rOther);

   //------------------------------------------------------------------------
   void MakeAssignment(const CProductReactionTable& rOther);

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

#endif // INCLUDED_PRODUCTREACTIONTABLE_H_
