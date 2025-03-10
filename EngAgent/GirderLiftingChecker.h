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

#pragma once

// SYSTEM INCLUDES
//

// PROJECT INCLUDES
//
#include <PgsExt\PgsExtExp.h>
#include <PgsExt\PoiMap.h>

#include <IFace\PointOfInterest.h>

// LOCAL INCLUDES
//

// FORWARD DECLARATIONS
//

// MISCELLANEOUS
//

/*****************************************************************************
CLASS 
   pgsGirderLiftingChecker

   Design Checker for girder lifting


DESCRIPTION
   Design Checker for girder lifting

LOG
   rdp : 06.25.2013 : Created file
*****************************************************************************/

class pgsGirderLiftingChecker
{
public:
   // GROUP: LIFECYCLE

   //------------------------------------------------------------------------
   // Constructor
   pgsGirderLiftingChecker(IBroker* pBroker,StatusGroupIDType statusGroupID);

   //------------------------------------------------------------------------
   // Destructor
   virtual ~pgsGirderLiftingChecker();

   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   void CheckLifting(const CSegmentKey& segmentKey,WBFL::Stability::LiftingCheckArtifact* pArtifact);
   void AnalyzeLifting(const CSegmentKey& segmentKey,Float64 supportLoc,WBFL::Stability::LiftingCheckArtifact* pArtifact);
   void AnalyzeLifting(const CSegmentKey& segmentKey,const HANDLINGCONFIG& config,ISegmentLiftingDesignPointsOfInterest* pPOId, WBFL::Stability::LiftingCheckArtifact* pArtifact, const WBFL::Stability::LiftingStabilityProblem** ppStabilityProblem = nullptr);
   pgsDesignCodes::OutcomeType DesignLifting(const CSegmentKey& segmentKey,HANDLINGCONFIG& config,ISegmentLiftingDesignPointsOfInterest* pPOId,WBFL::Stability::LiftingCheckArtifact* pArtifact,const WBFL::Stability::LiftingStabilityProblem** ppStabilityProblem,SHARED_LOGFILE LOGFILE);

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
   StatusCallbackIDType m_scidLiftingSupportLocationError;
   StatusCallbackIDType m_scidLiftingSupportLocationWarning;

   // GROUP: LIFECYCLE
   // can't construct without a broker
   pgsGirderLiftingChecker() = delete;

   // Prevent accidental copying and assignment
   pgsGirderLiftingChecker(const pgsGirderLiftingChecker&) = delete;
   pgsGirderLiftingChecker& operator=(const pgsGirderLiftingChecker&) = delete;

   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   void AnalyzeLifting(const CSegmentKey& segmentKey,bool bUseConfig,const HANDLINGCONFIG& liftConfig,ISegmentLiftingDesignPointsOfInterest* pPoiD,WBFL::Stability::LiftingCheckArtifact* pArtifact,const WBFL::Stability::LiftingStabilityProblem** ppStabilityProblem = nullptr);

   // GROUP: ACCESS
   // GROUP: INQUIRY
};

// INLINE METHODS
//

// EXTERNAL REFERENCES
//
