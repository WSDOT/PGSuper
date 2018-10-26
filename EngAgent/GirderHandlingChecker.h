///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2010  Washington State Department of Transportation
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
#include <PgsExt\HaulingCheckArtifact.h>
#include <PgsExt\LiftingCheckArtifact.h>
#include <PgsExt\PoiMap.h>

#include <IFace\GirderHandlingPointOfInterest.h>

// LOCAL INCLUDES
//

// FORWARD DECLARATIONS
//

// MISCELLANEOUS
//

/*****************************************************************************
CLASS 
   pgsGirderHandlingChecker

   Design Checker for girder lifting and hauling


DESCRIPTION
   Design Checker for girder lifting and hauling


COPYRIGHT
   Copyright © 1997-1998
   Washington State Department Of Transportation
   All Rights Reserved

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
   void CheckHauling(SpanIndexType span,GirderIndexType gdr,pgsHaulingCheckArtifact* pArtifact);
   void CheckLifting(SpanIndexType span,GirderIndexType gdr,pgsLiftingCheckArtifact* pArtifact);

   void AnalyzeHauling(SpanIndexType span,GirderIndexType gdr,pgsHaulingAnalysisArtifact* pArtifact);
   void AnalyzeHauling(SpanIndexType span,GirderIndexType gdr,const HANDLINGCONFIG& config,IGirderHaulingDesignPointsOfInterest* pPOId,pgsHaulingAnalysisArtifact* pArtifact);
   bool DesignShipping(SpanIndexType span,GirderIndexType gdr,const GDRCONFIG& config,bool bDesignForEqualOverhangs,bool bIgnoreConfigurationLimits,IGirderHaulingDesignPointsOfInterest* pPOId,pgsHaulingAnalysisArtifact* pArtifact,SHARED_LOGFILE LOGFILE);

   void AnalyzeLifting(SpanIndexType span,GirderIndexType gdr,pgsLiftingAnalysisArtifact* pArtifact);
   void AnalyzeLifting(SpanIndexType span,GirderIndexType gdr,const HANDLINGCONFIG& config,IGirderLiftingDesignPointsOfInterest* pPOId, pgsLiftingAnalysisArtifact* pArtifact);
   pgsDesignCodes::OutcomeType DesignLifting(SpanIndexType span,GirderIndexType gdr,const GDRCONFIG& config,IGirderLiftingDesignPointsOfInterest* pPOId,pgsLiftingAnalysisArtifact* pArtifact,SHARED_LOGFILE LOGFILE);

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
   StatusCallbackIDType m_scidLiftingSupportLocation;
   StatusCallbackIDType m_scidTruckStiffness;

   CComPtr<IFem2dModel> m_Model;
   pgsPoiMap m_PoiMap;

   // GROUP: LIFECYCLE
   // can't construct without a broker
   pgsGirderHandlingChecker();

   // Prevent accidental copying and assignment
   pgsGirderHandlingChecker(const pgsGirderHandlingChecker&);
   pgsGirderHandlingChecker& operator=(const pgsGirderHandlingChecker&);

   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   void AnalyzeLifting(SpanIndexType span,GirderIndexType gdr,bool bUseConfig,const HANDLINGCONFIG& liftConfig,IGirderLiftingDesignPointsOfInterest* pPoiD,pgsLiftingAnalysisArtifact* pArtifact);
   void PrepareLiftingCheckArtifact(SpanIndexType span,GirderIndexType gdr,pgsLiftingCheckArtifact* pArtifact);
   void PrepareLiftingAnalysisArtifact(SpanIndexType span,GirderIndexType gdr,Float64 Loh,Float64 Roh,Float64 Fci,Float64 Eci,pgsLiftingAnalysisArtifact* pArtifact);
   void ComputeLiftingMoments(SpanIndexType span,GirderIndexType gdr,
                              const pgsLiftingAnalysisArtifact& rArtifact, 
                              const std::vector<pgsPointOfInterest>& rpoiVec,
                              std::vector<Float64>* pmomVec, 
                              Float64* pMidSpanDeflection, Float64* pOverhangDeflection);
   void ComputeLiftingStresses(SpanIndexType span,GirderIndexType gdr,bool bUseConfig,
                               const HANDLINGCONFIG& liftConfig,
                               const std::vector<pgsPointOfInterest>& rpoiVec,
                               const std::vector<Float64>& momVec,
                               pgsLiftingAnalysisArtifact* pArtifact);
   void ComputeLiftingFsAgainstFailure(SpanIndexType span,GirderIndexType gdr,pgsLiftingAnalysisArtifact* pArtifact);
   bool ComputeLiftingFsAgainstCracking(SpanIndexType span,GirderIndexType gdr,bool bUseConfig,
                                        const HANDLINGCONFIG& liftConfig,
                                        const std::vector<pgsPointOfInterest>& rpoiVec,
                                        const std::vector<Float64>& momVec,
                                        Float64 midSpanDeflection,
                                        Float64 overhangDeflection,
                                        pgsLiftingAnalysisArtifact* pArtifact);


   void AnalyzeHauling(SpanIndexType span,GirderIndexType gdr,bool bUseConfig,const HANDLINGCONFIG& config,IGirderHaulingDesignPointsOfInterest* pPOId,pgsHaulingAnalysisArtifact* pArtifact);
   void PrepareHaulingAnalysisArtifact(SpanIndexType span,GirderIndexType gdr,Float64 Loh,Float64 Roh,Float64 Fc,Float64 Ec,pgsHaulingAnalysisArtifact* pArtifact);

   void ComputeHaulingMoments(SpanIndexType span,GirderIndexType gdr,
                              const pgsHaulingAnalysisArtifact& rArtifact, 
                              const std::vector<pgsPointOfInterest>& rpoiVec,
                              std::vector<Float64>* pmomVec, Float64* pMidSpanDeflection,Float64* pOverhangDeflection);


   void ComputeHaulingRollAngle(SpanIndexType span,GirderIndexType gdr,
                                pgsHaulingAnalysisArtifact* pArtifact, 
                                const std::vector<pgsPointOfInterest> rpoiVec,
                                std::vector<Float64>* pmomVec, Float64* pMidSpanDeflection,Float64* pOverhangDeflection);
   void ComputeHaulingStresses(SpanIndexType span,GirderIndexType gdr,bool bUseConfig,
                               const HANDLINGCONFIG& haulConfig,
                               const std::vector<pgsPointOfInterest>& rpoiVec,
                               const std::vector<Float64>& momVec,
                               pgsHaulingAnalysisArtifact* pArtifact);
   void PrepareHaulingCheckArtifact(SpanIndexType span,GirderIndexType gdr,pgsHaulingCheckArtifact* pArtifact);
   void ComputeHaulingFsForCracking(SpanIndexType span,GirderIndexType gdr,
                                    const std::vector<pgsPointOfInterest>& rpoiVec,
                                    const std::vector<Float64>& momVec,
                                    Float64 midSpanDeflection,
                                    Float64 overhangDeflection,
                                    pgsHaulingAnalysisArtifact* pArtifact);
   void ComputeHaulingFsForRollover(SpanIndexType span,GirderIndexType gdr,pgsHaulingAnalysisArtifact* pArtifact);


   void ComputeMoments(SpanIndexType span,GirderIndexType gdr,
                       Float64 leftOH,Float64 glen,Float64 rightOH,
                       Float64 E, 
                       const std::vector<pgsPointOfInterest>& rpoiVec,
                       std::vector<Float64>* pmomVec, Float64* pMidSpanDeflection, Float64* pOverhangDeflection);

   void GetRequirementsForAlternativeTensileStress(const pgsPointOfInterest& poi,Float64 ftu,Float64 ftd,Float64 fbu,Float64 fbd,Float64* pY,Float64* pA,Float64* pT,Float64* pAs);

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

#endif // INCLUDED_PGSEXT_GIRDERHANDLINGCHECKER_H_
