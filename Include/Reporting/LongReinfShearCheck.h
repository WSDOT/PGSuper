///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2019  Washington State Department of Transportation
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

#ifndef INCLUDED_LONGREINFSHEARCHECK_H_
#define INCLUDED_LONGREINFSHEARCHECK_H_

#include <Reporting\ReportingExp.h>

interface IEAFDisplayUnits;
class pgsGirderArtifact;

/*****************************************************************************
CLASS 
   CLongReinfShearCheck

   Encapsulates the construction of the longitudinal reinf for shear check report content.


DESCRIPTION
   Encapsulates the construction of the  longitudinal reinf for shear check report content.

LOG
   rdp : 06.01.1999 : Created file
*****************************************************************************/

class REPORTINGCLASS CLongReinfShearCheck
{
public:
   // GROUP: LIFECYCLE

   //------------------------------------------------------------------------
   // Default constructor
   CLongReinfShearCheck();

   //------------------------------------------------------------------------
   // Copy constructor
   CLongReinfShearCheck(const CLongReinfShearCheck& rOther);

   //------------------------------------------------------------------------
   // Destructor
   virtual ~CLongReinfShearCheck();

   // GROUP: OPERATORS
   //------------------------------------------------------------------------
   // Assignment operator
   CLongReinfShearCheck& operator = (const CLongReinfShearCheck& rOther);

   // GROUP: OPERATIONS

   //------------------------------------------------------------------------
   // Build (design limit states)
   virtual void Build(rptChapter* pChapter,
                      IBroker* pBroker,const pgsGirderArtifact* pGirderArtifact,
                      IntervalIndexType intervalIdx,pgsTypes::LimitState ls,
                      IEAFDisplayUnits* pDisplayUnits) const;


   //------------------------------------------------------------------------
   // Build (rating limit states)
   virtual void Build(rptChapter* pChapter,
                      IBroker* pBroker,const CGirderKey& girderKey,
                      pgsTypes::LimitState ls,
                      IEAFDisplayUnits* pDisplayUnits) const;

protected:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   //------------------------------------------------------------------------
   void MakeCopy(const CLongReinfShearCheck& rOther);

   //------------------------------------------------------------------------
   void MakeAssignment(const CLongReinfShearCheck& rOther);

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

#endif // INCLUDED_LONGREINFSHEARCHECK_H_
