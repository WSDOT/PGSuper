///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2013  Washington State Department of Transportation
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
#include <PgsExt\LiftingAnalysisArtifact.h>
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
   void CheckHauling(const CSegmentKey& segmentKey,pgsHaulingAnalysisArtifact* pArtifact);
   void CheckLifting(const CSegmentKey& segmentKey,pgsLiftingAnalysisArtifact* pArtifact);

   void AnalyzeHauling(const CSegmentKey& segmentKey,pgsHaulingAnalysisArtifact* pArtifact);
   void AnalyzeHauling(const CSegmentKey& segmentKey,const HANDLINGCONFIG& config,IGirderHaulingDesignPointsOfInterest* pPOId,pgsHaulingAnalysisArtifact* pArtifact);
   bool DesignShipping(const CSegmentKey& segmentKey,const GDRCONFIG& config,bool bDesignForEqualOverhangs,bool bIgnoreConfigurationLimits,IGirderHaulingDesignPointsOfInterest* pPOId,pgsHaulingAnalysisArtifact* pArtifact,SHARED_LOGFILE LOGFILE);

   void AnalyzeLifting(const CSegmentKey& segmentKey,const HANDLINGCONFIG& config,IGirderLiftingDesignPointsOfInterest* pPOId, pgsLiftingAnalysisArtifact* pArtifact);
   pgsDesignCodes::OutcomeType DesignLifting(const CSegmentKey& segmentKey,const GDRCONFIG& config,IGirderLiftingDesignPointsOfInterest* pPOId,pgsLiftingAnalysisArtifact* pArtifact,SHARED_LOGFILE LOGFILE);

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
   StatusCallbackIDType m_scidBunkPointLocation;
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
   void AnalyzeLifting(const CSegmentKey& segmentKey,bool bUseConfig,const HANDLINGCONFIG& liftConfig,IGirderLiftingDesignPointsOfInterest* pPoiD,pgsLiftingAnalysisArtifact* pArtifact);
   void PrepareLiftingAnalysisArtifact(const CSegmentKey& segmentKey,Float64 Loh,Float64 Roh,Float64 Fci,Float64 Eci,pgsTypes::ConcreteType concType,pgsLiftingAnalysisArtifact* pArtifact);
   void ComputeLiftingMoments(const CSegmentKey& segmentKey,
                              const pgsLiftingAnalysisArtifact& rArtifact, 
                              const std::vector<pgsPointOfInterest>& rpoiVec,
                              std::vector<Float64>* pmomVec, 
                              Float64* pMidSpanDeflection);
   void ComputeLiftingStresses(const CSegmentKey& segmentKey,bool bUseConfig,
                               const HANDLINGCONFIG& liftConfig,
                               const std::vector<pgsPointOfInterest>& rpoiVec,
                               const std::vector<Float64>& momVec,
                               pgsLiftingAnalysisArtifact* pArtifact);
   void ComputeLiftingFsAgainstFailure(const CSegmentKey& segmentKey,pgsLiftingAnalysisArtifact* pArtifact);
   bool ComputeLiftingFsAgainstCracking(const CSegmentKey& segmentKey,bool bUseConfig,
                                        const HANDLINGCONFIG& liftConfig,
                                        const std::vector<pgsPointOfInterest>& rpoiVec,
                                        const std::vector<Float64>& momVec,
                                        Float64 midSpanDeflection,
                                        pgsLiftingAnalysisArtifact* pArtifact);


   void AnalyzeHauling(const CSegmentKey& segmentKey,bool bUseConfig,const HANDLINGCONFIG& config,IGirderHaulingDesignPointsOfInterest* pPOId,pgsHaulingAnalysisArtifact* pArtifact);
   void PrepareHaulingAnalysisArtifact(const CSegmentKey& segmentKey,Float64 Loh,Float64 Roh,Float64 Fc,Float64 Ec,pgsTypes::ConcreteType concType,pgsHaulingAnalysisArtifact* pArtifact);

   void ComputeHaulingMoments(const CSegmentKey& segmentKey,
                              const pgsHaulingAnalysisArtifact& rArtifact, 
                              const std::vector<pgsPointOfInterest>& rpoiVec,
                              std::vector<Float64>* pmomVec, Float64* pMidSpanDeflection);


   void ComputeHaulingRollAngle(const CSegmentKey& segmentKey,
                                pgsHaulingAnalysisArtifact* pArtifact, 
                                const std::vector<pgsPointOfInterest> rpoiVec,
                                std::vector<Float64>* pmomVec, Float64* pMidSpanDeflection);
   void ComputeHaulingStresses(const CSegmentKey& segmentKey,bool bUseConfig,
                               const HANDLINGCONFIG& haulConfig,
                               const std::vector<pgsPointOfInterest>& rpoiVec,
                               const std::vector<Float64>& momVec,
                               pgsHaulingAnalysisArtifact* pArtifact);
   void ComputeHaulingFsForCracking(const CSegmentKey& segmentKey,
                                    const std::vector<pgsPointOfInterest>& rpoiVec,
                                    const std::vector<Float64>& momVec,
                                    pgsHaulingAnalysisArtifact* pArtifact);
   void ComputeHaulingFsForRollover(const CSegmentKey& segmentKey,pgsHaulingAnalysisArtifact* pArtifact);


   void ComputeMoments(const CSegmentKey& segmentKey,
                       IntervalIndexType intervalIdx,
                       Float64 leftOH,Float64 segmentLength,Float64 rightOH,
                       Float64 E, 
                       PoiAttributeType poiReference,
                       const std::vector<pgsPointOfInterest>& rpoiVec,
                       std::vector<Float64>* pmomVec, Float64* pMidSpanDeflection);

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

/*****************************************************************************
CLASS 
   pgsAlternativeTensileStressCalculator


DESCRIPTION
   Utility class for dealing with alternative tensile stress in casting yard and at lifting


COPYRIGHT
   Copyright © 1997-1998
   Washington State Department Of Transportation
   All Rights Reserved

LOG
   rdp : 03.25.2013 : Created file
*****************************************************************************/

class pgsAlternativeTensileStressCalculator
{
public:
   // GROUP: LIFECYCLE
   //------------------------------------------------------------------------
   // Constructor
   pgsAlternativeTensileStressCalculator(const CSegmentKey& segmentKey, IntervalIndexType intervalIdx,IGirder* pGirder,
                                         IShapes* pShapes,ISectionProperties* pSectProps, ILongRebarGeometry* pRebarGeom,
                                         IMaterials* pMaterials,
                                         bool bSIUnits);

   //------------------------------------------------------------------------
   // Destructor
   virtual ~pgsAlternativeTensileStressCalculator()
   {;}

   Float64 ComputeAlternativeStressRequirements(const pgsPointOfInterest& poi, const GDRCONFIG* pConfig,
                                                Float64 fTop, Float64 fBot, 
                                                Float64 lowAllowTens, Float64 highAllowTens,
                                                Float64 *pYna, Float64 *pAreaTens, Float64 *pT, 
                                                Float64 *pAsProvd, Float64 *pAsReqd, bool* pIsAdequateRebar);

private:
   pgsAlternativeTensileStressCalculator(); // no default constructor

   // GROUP: DATA MEMBERS

   // these are weak references
   IGirder* m_pGirder;
   IShapes* m_pShapes;
   ISectionProperties* m_pSectProps;
   ILongRebarGeometry* m_pRebarGeom;
   IMaterials* m_pMaterials;
   Float64 m_AllowableFs;
   IntervalIndexType m_IntervalIdx;
};


#endif // INCLUDED_PGSEXT_GIRDERHANDLINGCHECKER_H_
