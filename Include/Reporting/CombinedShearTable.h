///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2013  Washington State Department of Transportation
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

#ifndef INCLUDED_COMBINEDSHEARTABLE_H_
#define INCLUDED_COMBINEDSHEARTABLE_H_

#include <Reporting\ReportingExp.h>

interface IEAFDisplayUnits;

/*****************************************************************************
CLASS 
   CCombinedShearTable

   Encapsulates the construction of the combined shear table.


DESCRIPTION
   Encapsulates the construction of the combined shear table.


COPYRIGHT
   Copyright © 1997-1998
   Washington State Department Of Transportation
   All Rights Reserved

LOG
   rab : 11.08.1998 : Created file
*****************************************************************************/

class REPORTINGCLASS CCombinedShearTable
{
public:
   // GROUP: LIFECYCLE

   //------------------------------------------------------------------------
   // Default constructor
   CCombinedShearTable();

   //------------------------------------------------------------------------
   // Copy constructor
   CCombinedShearTable(const CCombinedShearTable& rOther);

   //------------------------------------------------------------------------
   // Destructor
   virtual ~CCombinedShearTable();

   // GROUP: OPERATORS
   //------------------------------------------------------------------------
   // Assignment operator
   CCombinedShearTable& operator = (const CCombinedShearTable& rOther);

   // GROUP: OPERATIONS

   //------------------------------------------------------------------------
   // Builds the combined results table
   // bDesign and bRating are only considered from stage = pgsTypes::BridgeSite3
   virtual void Build(IBroker* pBroker, rptChapter* pChapter,
                      const CGirderKey& girderKey,
                      IEAFDisplayUnits* pDisplayUnits,
                      IntervalIndexType intervalIdx,pgsTypes::AnalysisType analysisType,
                      bool bDesign=true,bool bRating=true) const;

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
                      IntervalIndexType intervalIdx,pgsTypes::AnalysisType analysisType,
                      bool bDesign=true,bool bRating=true) const;

   void BuildCombinedLiveTable(IBroker* pBroker, rptChapter* pChapter,
                      const CGirderKey& girderKey,
                      IEAFDisplayUnits* pDisplayUnits,
                      pgsTypes::AnalysisType analysisType,
                      bool bDesign=true,bool bRating=true) const;

   void BuildLimitStateTable(IBroker* pBroker, rptChapter* pChapter,
                      const CGirderKey& girderKey,
                      IEAFDisplayUnits* pDisplayUnits,
                      IntervalIndexType intervalIdx,pgsTypes::AnalysisType analysisType,
                      bool bDesign=true,bool bRating=true) const;
   //------------------------------------------------------------------------
   void MakeCopy(const CCombinedShearTable& rOther);

   //------------------------------------------------------------------------
   void MakeAssignment(const CCombinedShearTable& rOther);

   // GROUP: ACCESS
   // GROUP: INQUIRY

private:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   // GROUP: ACCESS
   // GROUP: INQUIRY

public:
   // GROUP: DEBUG
   #if defined _DEBUG
   //------------------------------------------------------------------------
   // Returns true if the object is in a valid state, otherwise returns false.
   virtual bool AssertValid() const;

   //------------------------------------------------------------------------
   // Dumps the contents of the object to the given dump context.
   virtual void Dump(dbgDumpContext& os) const;
   #endif // _DEBUG

   #if defined _UNITTEST
   //------------------------------------------------------------------------
   // Runs a self-diagnostic test.  Returns true if the test passed,
   // otherwise false.
   static bool TestMe(dbgLog& rlog);
   #endif // _UNITTEST
};

// INLINE METHODS
//

// EXTERNAL REFERENCES
//

#endif // INCLUDED_COMBINEDSHEARTABLE_H_
