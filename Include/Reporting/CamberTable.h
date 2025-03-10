///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright � 1999-2025  Washington State Department of Transportation
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

#ifndef INCLUDED_CAMBERTABLE_H_
#define INCLUDED_CAMBERTABLE_H_

#include <Reporting\ReportingExp.h>

interface IEAFDisplayUnits;
struct CamberMultipliers;

/*****************************************************************************
CLASS 
   CCamberTable

   Encapsulates the construction of the camber table.


DESCRIPTION
   Encapsulates the construction of the camber table.

LOG
   rab : 11.29.1998 : Created file
*****************************************************************************/

class REPORTINGCLASS CCamberTable
{
public:
   // GROUP: LIFECYCLE

   //------------------------------------------------------------------------
   // Default constructor
   CCamberTable();

   //------------------------------------------------------------------------
   // Copy constructor
   CCamberTable(const CCamberTable& rOther);

   //------------------------------------------------------------------------
   // Destructor
   virtual ~CCamberTable();

   // GROUP: OPERATORS
   //------------------------------------------------------------------------
   // Assignment operator
   CCamberTable& operator = (const CCamberTable& rOther);

   // GROUP: OPERATIONS

   //------------------------------------------------------------------------
   // Build the strand eccentricity tables
   void Build_Deck(IBroker* pBroker,const CSegmentKey& segmentKey, 
                  bool bTempStrands, bool bSidewalk, bool bShearKey, bool bLongitudinalJoint, bool bConstruction, bool bOverlay, bool bDeckPanels,
                  IEAFDisplayUnits* pDisplayUnits, pgsTypes::CreepTime time, const CamberMultipliers& cm,
                  rptRcTable** pTable1,rptRcTable** pTable2,rptRcTable** pTable3) const;

   void Build_NoDeck(IBroker* pBroker,const CSegmentKey& segmentKey,
                     bool bTempStrands, bool bSidewalk, bool bShearKey, bool bLongitudinalJoint, bool bConstruction, bool bOverlay,
                     IEAFDisplayUnits* pDisplayUnits, pgsTypes::CreepTime time, const CamberMultipliers& cm,
                     rptRcTable** pTable1,rptRcTable** pTable2,rptRcTable** pTable3) const;

   // GROUP: ACCESS
   // GROUP: INQUIRY

protected:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   //------------------------------------------------------------------------
   void MakeCopy(const CCamberTable& rOther);

   //------------------------------------------------------------------------
   void MakeAssignment(const CCamberTable& rOther);

   void GetPointsOfInterest(IBroker* pBroker,const CSegmentKey& segmentKey,PoiList* pvPoiRelease,PoiList* pvPoiStorage,PoiList* pvPoiErected) const;
   // GROUP: ACCESS
   // GROUP: INQUIRY
   void Build_Deck_Y(IBroker* pBroker, const CSegmentKey& segmentKey,
      bool bTempStrands, bool bSidewalk, bool bShearKey, bool bLongitudinalJoint, bool bConstruction, bool bOverlay, bool bDeckPanels,
      IEAFDisplayUnits* pDisplayUnits, pgsTypes::CreepTime time, const CamberMultipliers& cm,
      rptRcTable** pTable1, rptRcTable** pTable2, rptRcTable** pTable3) const;

   void Build_Deck_XY(IBroker* pBroker, const CSegmentKey& segmentKey,
      bool bTempStrands, bool bSidewalk, bool bShearKey, bool bLongitudinalJoint, bool bConstruction, bool bOverlay, bool bDeckPanels,
      IEAFDisplayUnits* pDisplayUnits, pgsTypes::CreepTime time, const CamberMultipliers& cm,
      rptRcTable** pTable1, rptRcTable** pTable2, rptRcTable** pTable3) const;

   void Build_NoDeck_Y(IBroker* pBroker, const CSegmentKey& segmentKey,
      bool bTempStrands, bool bSidewalk, bool bShearKey, bool bLongitudinalJoint, bool bConstruction, bool bOverlay,
      IEAFDisplayUnits* pDisplayUnits, pgsTypes::CreepTime time, const CamberMultipliers& cm,
      rptRcTable** pTable1, rptRcTable** pTable2, rptRcTable** pTable3) const;

   void Build_NoDeck_XY(IBroker* pBroker, const CSegmentKey& segmentKey,
      bool bTempStrands, bool bSidewalk, bool bShearKey, bool bLongitudinalJoint, bool bConstruction, bool bOverlay,
      IEAFDisplayUnits* pDisplayUnits, pgsTypes::CreepTime time, const CamberMultipliers& cm,
      rptRcTable** pTable1, rptRcTable** pTable2, rptRcTable** pTable3) const;

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

#endif // INCLUDED_CAMBERTABLE_H_
