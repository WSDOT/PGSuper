///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2015  Washington State Department of Transportation
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

#ifndef INCLUDED_LIVELOADREACTIONTABLE_H_
#define INCLUDED_LIVELOADREACTIONTABLE_H_

#include <Reporting\ReportingExp.h>
#include <IFace\AnalysisResults.h>
#include <Reporting\ReactionInterfaceAdapters.h>

interface IEAFDisplayUnits;

// MISCELLANEOUS
//

/*****************************************************************************
CLASS 
   CLiveLoadReactionTable

   Encapsulates the construction of the combined reaction table.


DESCRIPTION
   Encapsulates the construction of the combined reaction table.


COPYRIGHT
   Copyright © 1997-1998
   Washington State Department Of Transportation
   All Rights Reserved

LOG
   rab : 04.01.2008 : Created file
*****************************************************************************/

class REPORTINGCLASS CLiveLoadReactionTable
{
public:
   // This class serves Float64 duty. It can report pier reactions or girder bearing reactions.
   // The two are identical except for the title and the interfaces they use to get responses

   // GROUP: LIFECYCLE

   //------------------------------------------------------------------------
   // Default constructor
   CLiveLoadReactionTable();

   //------------------------------------------------------------------------
   // Copy constructor
   CLiveLoadReactionTable(const CLiveLoadReactionTable& rOther);

   //------------------------------------------------------------------------
   // Destructor
   virtual ~CLiveLoadReactionTable();

   // GROUP: OPERATORS
   //------------------------------------------------------------------------
   // Assignment operator
   CLiveLoadReactionTable& operator = (const CLiveLoadReactionTable& rOther);

   // GROUP: OPERATIONS

   //------------------------------------------------------------------------
   // Builds the strand eccentricity table.
   virtual void Build(IBroker* pBroker, rptChapter* pChapter,
                      SpanIndexType span,GirderIndexType girder,
                      IEAFDisplayUnits* pDisplayUnits, ReactionTableType tableType,
                      pgsTypes::Stage stage, pgsTypes::AnalysisType analysisType) const;
   // GROUP: ACCESS
   // GROUP: INQUIRY

protected:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   //------------------------------------------------------------------------
   void MakeCopy(const CLiveLoadReactionTable& rOther);

   //------------------------------------------------------------------------
   void MakeAssignment(const CLiveLoadReactionTable& rOther);
};

// INLINE METHODS
//

// EXTERNAL REFERENCES
//

#endif // INCLUDED_LIVELOADREACTIONTABLE_H_
