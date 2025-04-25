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

#ifndef INCLUDED_CASTINGYARDMOMENTSTABLE_H_
#define INCLUDED_CASTINGYARDMOMENTSTABLE_H_

#include <Reporting\ReportingExp.h>

class IEAFDisplayUnits;

/*****************************************************************************
CLASS 
   CCastingYardMomentsTable

   Encapsulates the construction of the casting yard moments table.


DESCRIPTION
   Encapsulates the construction of the casting yard moments table.

LOG
   rab : 01.08.1999 : Created file
*****************************************************************************/

class REPORTINGCLASS CCastingYardMomentsTable
{
public:
   // GROUP: LIFECYCLE

   //------------------------------------------------------------------------
   // Default constructor
   CCastingYardMomentsTable();

   //------------------------------------------------------------------------
   // Copy constructor
   CCastingYardMomentsTable(const CCastingYardMomentsTable& rOther);

   //------------------------------------------------------------------------
   // Destructor
   virtual ~CCastingYardMomentsTable();

   // GROUP: OPERATORS
   //------------------------------------------------------------------------
   // Assignment operator
   CCastingYardMomentsTable& operator = (const CCastingYardMomentsTable& rOther);

   // GROUP: OPERATIONS

   //------------------------------------------------------------------------
   // Builds the strand eccentricity table.
   virtual rptRcTable* Build(std::shared_ptr<WBFL::EAF::Broker> pBroker,const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,PoiAttributeType poiRefAttribute,LPCTSTR strTableTitle,
                             std::shared_ptr<IEAFDisplayUnits> pDisplayUnits) const;

   // GROUP: ACCESS
   // GROUP: INQUIRY

protected:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   //------------------------------------------------------------------------
   void MakeCopy(const CCastingYardMomentsTable& rOther);

   //------------------------------------------------------------------------
   void MakeAssignment(const CCastingYardMomentsTable& rOther);

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

#endif // INCLUDED_CASTINGYARDMOMENTSTABLE_H_
