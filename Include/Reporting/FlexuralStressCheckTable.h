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

#ifndef INCLUDED_FLEXURALSTRESSCHECKTABLE_H_
#define INCLUDED_FLEXURALSTRESSCHECKTABLE_H_

#include <Reporting\ReportingExp.h>

interface IEAFDisplayUnits;
class pgsGirderArtifact;

/*****************************************************************************
CLASS 
   CFlexuralStressCheckTable

   Encapsulates the construction of the flexural stress check table.


DESCRIPTION
   Encapsulates the construction of the flexural stress check table.


COPYRIGHT
   Copyright © 1997-1998
   Washington State Department Of Transportation
   All Rights Reserved

LOG
   rab : 11.13.1998 : Created file
*****************************************************************************/

class REPORTINGCLASS CFlexuralStressCheckTable
{
public:
   // GROUP: LIFECYCLE

   //------------------------------------------------------------------------
   // Default constructor
   CFlexuralStressCheckTable();

   //------------------------------------------------------------------------
   // Copy constructor
   CFlexuralStressCheckTable(const CFlexuralStressCheckTable& rOther);

   //------------------------------------------------------------------------
   // Destructor
   virtual ~CFlexuralStressCheckTable();

   // GROUP: OPERATORS
   //------------------------------------------------------------------------
   // Assignment operator
   CFlexuralStressCheckTable& operator = (const CFlexuralStressCheckTable& rOther);

   // GROUP: OPERATIONS

   //------------------------------------------------------------------------
   // Builds the stress table and notes
   // NOTE: stress type is ignored for stages other than BSS3
   virtual void Build(rptChapter* pChapter,
                      IBroker* pBroker,SpanIndexType span,GirderIndexType girder,
                      IEAFDisplayUnits* pDisplayUnits,
                      pgsTypes::Stage stage,
                      pgsTypes::LimitState ls,
                      pgsTypes::StressType stress=pgsTypes::Tension) const;

   //------------------------------------------------------------------------
   // Builds the notes above stress table
   void BuildNotes(rptChapter* pChapter, const pgsGirderArtifact* gdrArtifact,
                   IBroker* pBroker,SpanIndexType span,GirderIndexType girder,
                   IEAFDisplayUnits* pDisplayUnits,
                   pgsTypes::Stage stage,
                   pgsTypes::LimitState ls,
                   pgsTypes::StressType stress=pgsTypes::Tension) const;

   //------------------------------------------------------------------------
   // Builds the table only
   void BuildTable(rptChapter* pChapter, const pgsGirderArtifact* gdrArtifact,
                   IBroker* pBroker,SpanIndexType span,GirderIndexType girder,
                   IEAFDisplayUnits* pDisplayUnits,
                   pgsTypes::Stage stage,
                   pgsTypes::LimitState ls,
                   pgsTypes::StressType stress=pgsTypes::Tension) const;


   // GROUP: ACCESS
   // GROUP: INQUIRY

protected:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   //------------------------------------------------------------------------
   void MakeCopy(const CFlexuralStressCheckTable& rOther);

   //------------------------------------------------------------------------
   void MakeAssignment(const CFlexuralStressCheckTable& rOther);

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

#endif // INCLUDED_FLEXURALSTRESSCHECKTABLE_H_
