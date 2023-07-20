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

#ifndef INCLUDED_HAULINGCHECK_H_
#define INCLUDED_HAULINGCHECK_H_

#include <Reporting\ReportingExp.h>

interface IEAFDisplayUnits;
class pgsGirderArtifact;

/*****************************************************************************
CLASS 
   CHaulingCheck

   Encapsulates the construction of the girder hauling check report content.


DESCRIPTION
   Encapsulates the construction of the girder hauling check report content.

LOG
   rab : 11.27.1998 : Created file
*****************************************************************************/

class REPORTINGCLASS CHaulingCheck
{
public:
   // GROUP: LIFECYCLE

   //------------------------------------------------------------------------
   // Default constructor
   CHaulingCheck();

   //------------------------------------------------------------------------
   // Copy constructor
   CHaulingCheck(const CHaulingCheck& rOther);

   //------------------------------------------------------------------------
   // Destructor
   virtual ~CHaulingCheck();

   // GROUP: OPERATORS
   //------------------------------------------------------------------------
   // Assignment operator
   CHaulingCheck& operator = (const CHaulingCheck& rOther);

   // GROUP: OPERATIONS

   //------------------------------------------------------------------------
   void Build(rptChapter* pChapter,
              IBroker* pBroker,const CGirderKey& girderKey,
              IEAFDisplayUnits* pDisplayUnits) const;

   void Build(rptChapter* pChapter,
      IBroker* pBroker, const CSegmentKey& segmentKey,
      IEAFDisplayUnits* pDisplayUnits) const;

   // GROUP: ACCESS
   // GROUP: INQUIRY

protected:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   //------------------------------------------------------------------------
   void MakeCopy(const CHaulingCheck& rOther);

   //------------------------------------------------------------------------
   void MakeAssignment(const CHaulingCheck& rOther);

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

#endif // INCLUDED_HAULINGCHECK_H_
