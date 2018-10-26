///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2016  Washington State Department of Transportation
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

#ifndef INCLUDED_OptionalDeflectionCheck_H_
#define INCLUDED_OptionalDeflectionCheck_H_

#include <Reporting\ReportingExp.h>
#include <PgsExt\GirderArtifact.h>

interface IEAFDisplayUnits;

/*****************************************************************************
CLASS 
   COptionalDeflectionCheck

   Encapsulates the construction of the girder detailing check report content.


DESCRIPTION
   Encapsulates the construction of the girder detailing check report content.


COPYRIGHT
   Copyright © 1997-2004
   Washington State Department Of Transportation
   All Rights Reserved

LOG
   rdp : 04.14.2004 : Created file
*****************************************************************************/

class REPORTINGCLASS COptionalDeflectionCheck
{
public:
   // GROUP: LIFECYCLE

   //------------------------------------------------------------------------
   // Default constructor
   COptionalDeflectionCheck();

   //------------------------------------------------------------------------
   // Copy constructor
   COptionalDeflectionCheck(const COptionalDeflectionCheck& rOther);

   //------------------------------------------------------------------------
   // Destructor
   virtual ~COptionalDeflectionCheck();

   // GROUP: OPERATORS
   //------------------------------------------------------------------------
   // Assignment operator
   COptionalDeflectionCheck& operator = (const COptionalDeflectionCheck& rOther);

   // GROUP: OPERATIONS

   //------------------------------------------------------------------------
   // Builds the strand eccentricity table.
   virtual void Build(rptChapter* pChapter, const pgsGirderArtifact* pArtifact,
                      SpanIndexType span,GirderIndexType girder, IEAFDisplayUnits* pDisplayUnits) const;
   // GROUP: ACCESS
   // GROUP: INQUIRY

protected:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   //------------------------------------------------------------------------
   void MakeCopy(const COptionalDeflectionCheck& rOther);

   //------------------------------------------------------------------------
   void MakeAssignment(const COptionalDeflectionCheck& rOther);

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

#endif // INCLUDED_OptionalDeflectionCheck_H_
