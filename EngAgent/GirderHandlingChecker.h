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

#ifndef INCLUDED_PGSEXT_GIRDERHANDLINGCHECKER_H_
#define INCLUDED_PGSEXT_GIRDERHANDLINGCHECKER_H_

// SYSTEM INCLUDES
//

// PROJECT INCLUDES
//
#include <PgsExt\PgsExtExp.h>
#include <PgsExt\HaulingAnalysisArtifact.h>
#include <PgsExt\PoiMap.h>
#include <PgsExt\GirderModelFactory.h>

#include <IFace\PointOfInterest.h>

// LOCAL INCLUDES
//

// FORWARD DECLARATIONS
//

// Virtual members of polymorphic hauling checker
class pgsGirderHaulingChecker
{
public:
   virtual pgsHaulingAnalysisArtifact* CheckHauling(const CSegmentKey& segmentKey, SHARED_LOGFILE LOGFILE) = 0;
   virtual pgsHaulingAnalysisArtifact* AnalyzeHauling(const CSegmentKey& segmentKey) = 0;
   virtual pgsHaulingAnalysisArtifact* AnalyzeHauling(const CSegmentKey& segmentKey,Float64 leftOverhang,Float64 rightOverhang) = 0;
   virtual pgsHaulingAnalysisArtifact* AnalyzeHauling(const CSegmentKey& segmentKey,const HANDLINGCONFIG& config,ISegmentHaulingDesignPointsOfInterest* pPOId) = 0;
   virtual pgsHaulingAnalysisArtifact* DesignHauling(const CSegmentKey& segmentKey,HANDLINGCONFIG& config,bool bIgnoreConfigurationLimits,ISegmentHaulingDesignPointsOfInterest* pPOId, bool* bSuccess, SHARED_LOGFILE LOGFILE) = 0;
};


// MISCELLANEOUS
//

/*****************************************************************************
CLASS 
   pgsGirderHandlingChecker

   Design Checker Factory for girder lifting and hauling


DESCRIPTION
   Design Checker Factory for girder lifting and hauling

LOG
   rdp : 03.25.1999 : Created file
*****************************************************************************/

class pgsGirderHandlingChecker
{
public:
   // GROUP: LIFECYCLE

   //------------------------------------------------------------------------
   // Constructor
   pgsGirderHandlingChecker(IBroker* pBroker,StatusGroupIDType statusGroupID);

   //------------------------------------------------------------------------
   // Destructor
   virtual ~pgsGirderHandlingChecker();

   // GROUP: OPERATORS
   // GROUP: OPERATIONS

   // Factory Method to create the appropriate hauling checker
   pgsGirderHaulingChecker* CreateGirderHaulingChecker();

   // Utility functions for the checking classes
   static void ComputeMoments(IBroker* pBroker, pgsGirderModelFactory* pGirderModelFactory, const CSegmentKey& segmentKey,
                       IntervalIndexType intervalIdx,
                       Float64 leftOH,Float64 glen,Float64 rightOH,
                       Float64 E, 
                       PoiAttributeType poiReference,
                       const PoiList& rpoiVec,
                       std::vector<Float64>* pmomVec, Float64* pMidSpanDeflection);

   // GROUP: ACCESS
   // GROUP: INQUIRY

protected:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   // GROUP: ACCESS
   // GROUP: INQUIRY

private:
   // GROUP: DATA MEMBERS
   IBroker* m_pBroker;
   StatusGroupIDType m_StatusGroupID;

   // GROUP: LIFECYCLE
   // can't construct without a broker
   pgsGirderHandlingChecker() = delete;

   // Prevent accidental copying and assignment
   pgsGirderHandlingChecker(const pgsGirderHandlingChecker&) = delete;
   pgsGirderHandlingChecker& operator=(const pgsGirderHandlingChecker&) = delete;

   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   // GROUP: ACCESS
   // GROUP: INQUIRY
};

// INLINE METHODS
//

// EXTERNAL REFERENCES
//

#endif // INCLUDED_PGSEXT_GIRDERHANDLINGCHECKER_H_
