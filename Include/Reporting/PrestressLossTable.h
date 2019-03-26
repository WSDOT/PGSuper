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

#ifndef INCLUDED_PRESTRESSLOSSTABLE_H_
#define INCLUDED_PRESTRESSLOSSTABLE_H_

#include <Reporting\ReportingExp.h>

interface IEAFDisplayUnits;

/*****************************************************************************
CLASS 
   CPrestressLossTable

   Encapsulates the construction of the prestress loss table.


DESCRIPTION
   Encapsulates the construction of the prestress loss table.

LOG
   rab : 08.16.2007 : Created file
*****************************************************************************/

class REPORTINGCLASS CPrestressLossTable
{
public:
   // GROUP: LIFECYCLE

   //------------------------------------------------------------------------
   // Default constructor
   CPrestressLossTable(bool bIsSplicedGirder=false);

   //------------------------------------------------------------------------
   // Copy constructor
   CPrestressLossTable(const CPrestressLossTable& rOther);

   //------------------------------------------------------------------------
   // Destructor
   virtual ~CPrestressLossTable();

   // GROUP: OPERATORS
   //------------------------------------------------------------------------
   // Assignment operator
   CPrestressLossTable& operator = (const CPrestressLossTable& rOther);

   // GROUP: OPERATIONS

   //------------------------------------------------------------------------
   // Builds the strand eccentricity table.
   virtual rptRcTable* Build(IBroker* pBroker,const CSegmentKey& segmentKey, bool bRating,
                             IEAFDisplayUnits* pDisplayUnits) const;
   // GROUP: ACCESS
   // GROUP: INQUIRY

protected:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   //------------------------------------------------------------------------
   void MakeCopy(const CPrestressLossTable& rOther);

   //------------------------------------------------------------------------
   void MakeAssignment(const CPrestressLossTable& rOther);

   // GROUP: ACCESS
   // GROUP: INQUIRY

private:
   // GROUP: DATA MEMBERS
   bool m_bSplicedGirder;
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

#endif // INCLUDED_PRESTRESSLOSSTABLE_H_
