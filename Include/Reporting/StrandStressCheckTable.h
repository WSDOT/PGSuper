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

#ifndef INCLUDED_STRANDSTRESSCHECKTABLE_H_
#define INCLUDED_STRANDSTRESSCHECKTABLE_H_

#include <Reporting\ReportingExp.h>

interface IEAFDisplayUnits;
class pgsGirderArtifact;

/*****************************************************************************
CLASS 
   CStrandStressCheckTable

   Encapsulates the construction of the prestressing strand stress check table.


DESCRIPTION
   Encapsulates the construction of the prestressing strand stress check table.

LOG
   rab : 10.28.1998 : Created file
*****************************************************************************/

class REPORTINGCLASS CStrandStressCheckTable
{
public:
   // GROUP: LIFECYCLE

   //------------------------------------------------------------------------
   // Default constructor
   CStrandStressCheckTable();

   //------------------------------------------------------------------------
   // Copy constructor
   CStrandStressCheckTable(const CStrandStressCheckTable& rOther);

   //------------------------------------------------------------------------
   // Destructor
   virtual ~CStrandStressCheckTable();

   // GROUP: OPERATORS
   //------------------------------------------------------------------------
   // Assignment operator
   CStrandStressCheckTable& operator = (const CStrandStressCheckTable& rOther);

   // GROUP: OPERATIONS

   //------------------------------------------------------------------------
   // Builds the strand eccentricity table.
   void Build(rptChapter* pChapter,IBroker* pBroker,const pgsGirderArtifact* pGirderArtifact,IEAFDisplayUnits* pDisplayUnits) const;
   // GROUP: ACCESS
   // GROUP: INQUIRY

protected:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   //------------------------------------------------------------------------
   void MakeCopy(const CStrandStressCheckTable& rOther);

   //------------------------------------------------------------------------
   void MakeAssignment(const CStrandStressCheckTable& rOther);

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

#endif // INCLUDED_STRANDSTRESSCHECKTABLE_H_
