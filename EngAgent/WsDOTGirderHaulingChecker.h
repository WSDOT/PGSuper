///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2014  Washington State Department of Transportation
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
#include <PgsExt\HaulingAnalysisArtifact.h>
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
   pgsWsdotGirderHaulingChecker

   Design Checker for girder hauling

DESCRIPTION
   Design Checker for girder lifting and hauling


COPYRIGHT
   Copyright © 1997-1998
   Washington State Department Of Transportation
   All Rights Reserved

LOG
   rdp : 06.25.2013 : Created file
*****************************************************************************/

class pgsWsdotGirderHaulingChecker: public pgsGirderHaulingChecker
{
public:
   // GROUP: LIFECYCLE

   //------------------------------------------------------------------------
   // Constructor
   pgsWsdotGirderHaulingChecker(IBroker* pBroker,StatusGroupIDType statusGroupID);

   //------------------------------------------------------------------------
   // Destructor
   virtual ~pgsWsdotGirderHaulingChecker();

   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   virtual pgsHaulingAnalysisArtifact* CheckHauling(const CSegmentKey& segmentKey, SHARED_LOGFILE LOGFILE);
   virtual pgsHaulingAnalysisArtifact* AnalyzeHauling(const CSegmentKey& segmentKey);
   virtual pgsHaulingAnalysisArtifact* AnalyzeHauling(const CSegmentKey& segmentKey,Float64 leftOverhang,Float64 rightOverhang);
   virtual pgsHaulingAnalysisArtifact* AnalyzeHauling(const CSegmentKey& segmentKey,const HANDLINGCONFIG& config,IGirderHaulingDesignPointsOfInterest* pPOId);
   virtual pgsHaulingAnalysisArtifact* DesignHauling(const CSegmentKey& segmentKey,const GDRCONFIG& config,bool bDesignForEqualOverhangs,bool bIgnoreConfigurationLimits,IGirderHaulingDesignPointsOfInterest* pPOId,bool* bSuccess, SHARED_LOGFILE LOGFILE);

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
   StatusCallbackIDType m_scidBunkPointLocation;
   StatusCallbackIDType m_scidTruckStiffness;

   // GROUP: LIFECYCLE
   // can't construct without a broker
   pgsWsdotGirderHaulingChecker();

   // Prevent accidental copying and assignment
   pgsWsdotGirderHaulingChecker(const pgsWsdotGirderHaulingChecker&);
   pgsWsdotGirderHaulingChecker& operator=(const pgsWsdotGirderHaulingChecker&);

   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   void AnalyzeHauling(const CSegmentKey& segmentKey,bool bUseConfig,const HANDLINGCONFIG& config,IGirderHaulingDesignPointsOfInterest* pPOId,pgsWsdotHaulingAnalysisArtifact* pArtifact);
   void PrepareHaulingAnalysisArtifact(const CSegmentKey& segmentKey,Float64 Loh,Float64 Roh,Float64 Fc,Float64 Ec,pgsTypes::ConcreteType concType,pgsWsdotHaulingAnalysisArtifact* pArtifact);

   void ComputeHaulingMoments(const CSegmentKey& segmentKey,
                              const pgsWsdotHaulingAnalysisArtifact& rArtifact, 
                              const std::vector<pgsPointOfInterest>& rpoiVec,
                              std::vector<Float64>* pmomVec, Float64* pMidSpanDeflection);

   void ComputeHaulingRollAngle(const CSegmentKey& segmentKey,
                                pgsWsdotHaulingAnalysisArtifact* pArtifact, 
                                const std::vector<pgsPointOfInterest> rpoiVec,
                                std::vector<Float64>* pmomVec, Float64* pMidSpanDeflection);
   void ComputeHaulingStresses(const CSegmentKey& segmentKey,bool bUseConfig,
                               const HANDLINGCONFIG& haulConfig,
                               const std::vector<pgsPointOfInterest>& rpoiVec,
                               const std::vector<Float64>& momVec,
                               pgsWsdotHaulingAnalysisArtifact* pArtifact);
   void ComputeHaulingFsForCracking(const CSegmentKey& segmentKey,
                                    const std::vector<pgsPointOfInterest>& rpoiVec,
                                    const std::vector<Float64>& momVec,
                                    pgsWsdotHaulingAnalysisArtifact* pArtifact);
   void ComputeHaulingFsForRollover(const CSegmentKey& segmentKey,pgsWsdotHaulingAnalysisArtifact* pArtifact);

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

