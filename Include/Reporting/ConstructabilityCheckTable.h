///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2023  Washington State Department of Transportation
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

#ifndef INCLUDED_CONSTRUCTABILITYCHECKTABLE_H_
#define INCLUDED_CONSTRUCTABILITYCHECKTABLE_H_

#include <Reporting\ReportingExp.h>

interface IEAFDisplayUnits;
class pgsGirderArtifact;

/*****************************************************************************
CLASS 
   CConstructabilityCheckTable

   Encapsulates the construction of the constructability check report content.


DESCRIPTION
   Encapsulates the construction of the constructability check report content.

LOG
   rab : 12.01.1998 : Created file
*****************************************************************************/

class REPORTINGCLASS CConstructabilityCheckTable
{
public:
   // GROUP: LIFECYCLE

   //------------------------------------------------------------------------
   // Default constructor
   CConstructabilityCheckTable();

   //------------------------------------------------------------------------
   // Copy constructor
   CConstructabilityCheckTable(const CConstructabilityCheckTable& rOther);

   //------------------------------------------------------------------------
   // Destructor
   virtual ~CConstructabilityCheckTable();

   // GROUP: OPERATORS
   //------------------------------------------------------------------------
   // Assignment operator
   CConstructabilityCheckTable& operator = (const CConstructabilityCheckTable& rOther);

   // GROUP: OPERATIONS

   //------------------------------------------------------------------------
   // Builds the constructability check table.
   void BuildSlabOffsetTable(rptChapter* pChapter,IBroker* pBroker, const std::vector<CGirderKey>& girderList, IEAFDisplayUnits* pDisplayUnits) const;
   void BuildMinimumHaunchCLCheck(rptChapter* pChapter,IBroker* pBroker, const std::vector<CGirderKey>& girderList, IEAFDisplayUnits* pDisplayUnits) const;
   void BuildMinimumFilletCheck(rptChapter* pChapter,IBroker* pBroker, const std::vector<CGirderKey>& girderList, IEAFDisplayUnits* pDisplayUnits) const;
   void BuildHaunchGeometryComplianceCheck(rptChapter* pChapter,IBroker* pBroker, const std::vector<CGirderKey>& girderList, IEAFDisplayUnits* pDisplayUnits) const;
   void BuildCamberCheck(rptChapter* pChapter,IBroker* pBroker,const CGirderKey& girderKey, IEAFDisplayUnits* pDisplayUnits) const;
   void BuildGlobalGirderStabilityCheck(rptChapter* pChapter,IBroker* pBroker,const pgsGirderArtifact* pGirderArtifact, IEAFDisplayUnits* pDisplayUnits) const;
   void BuildPrecamberCheck(rptChapter* pChapter, IBroker* pBroker, const std::vector<CGirderKey>& girderList, IEAFDisplayUnits* pDisplayUnits) const;
   void BuildBottomFlangeClearanceCheck(rptChapter* pChapter,IBroker* pBroker, const std::vector<CGirderKey>& girderList, IEAFDisplayUnits* pDisplayUnits) const;
   void BuildFinishedElevationCheck(rptChapter* pChapter,IBroker* pBroker,const std::vector<CGirderKey>& girderList,IEAFDisplayUnits* pDisplayUnits) const;
   void BuildMinimumHaunchCheck(rptChapter* pChapter, IBroker* pBroker, const std::vector<CGirderKey>& girderList, IEAFDisplayUnits* pDisplayUnits) const;

   // GROUP: ACCESS
   // GROUP: INQUIRY

protected:
   void BuildMonoSlabOffsetTable(rptChapter* pChapter,IBroker* pBroker, const std::vector<CGirderKey>& girderList, IEAFDisplayUnits* pDisplayUnits) const;
   void BuildMultiSlabOffsetTable(rptChapter* pChapter,IBroker* pBroker, const std::vector<CGirderKey>& girderList, IEAFDisplayUnits* pDisplayUnits) const;
   void BuildRegularCamberCheck(rptChapter* pChapter,IBroker* pBroker,const CGirderKey& girderKey, IEAFDisplayUnits* pDisplayUnits) const;
   void BuildTimeStepCamberCheck(rptChapter* pChapter,IBroker* pBroker,const CGirderKey& girderKey, IEAFDisplayUnits* pDisplayUnits) const;
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   //------------------------------------------------------------------------
   void MakeCopy(const CConstructabilityCheckTable& rOther);

   //------------------------------------------------------------------------
   void MakeAssignment(const CConstructabilityCheckTable& rOther);

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
   virtual void Dump(WBFL::Debug::LogContext& os) const;
   #endif // _DEBUG

   #if defined _UNITTEST
   //------------------------------------------------------------------------
   // Runs a self-diagnostic test.  Returns true if the test passed,
   // otherwise false.
   static bool TestMe(WBFL::Debug::Log& rlog);
   #endif // _UNITTEST
};

// INLINE METHODS
//

// EXTERNAL REFERENCES
//

#endif // INCLUDED_CONSTRUCTABILITYCHECKTABLE_H_
