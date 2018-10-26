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

#ifndef INCLUDED_DESIGNER2_H_
#define INCLUDED_DESIGNER2_H_

// SYSTEM INCLUDES
//

// PROJECT INCLUDES
//
#if !defined INCLUDED_DETAILS_H_
#include <Details.h>
#endif

#if !defined INCLUDED_ROARK_ROARK_H_
#include <Roark\Roark.h>
#endif

#include <IFace\Artifact.h>
#include <IFace\AnalysisResults.h>

#include "StrandDesignTool.h"
#include "ShearDesignTool.h"
#include "DesignCodes.h"
#include "LoadRater.h" // friend so it can access some private functions

// LOCAL INCLUDES
//

// FORWARD DECLARATIONS
//
interface IBroker;
interface IAllowableConcreteStress;

// MISCELLANEOUS
//
// utilty struct for shear design
struct ShearDesignAvs
{
   Float64 VerticalAvS;
   Float64 HorizontalAvS;
   Float64 Vu;
   Float64 Pt1FcBvDv; // 0.1*F'c*bv*dv  for min spacing location
};

#define NUM_LEGS  2
#define MAX_ZONES 4

/*****************************************************************************
CLASS 
   pgsDesigner2

   Encapsulates the design strategy


DESCRIPTION
   Encapsulates the design strategy


COPYRIGHT
   Copyright © 1997-1998
   Washington State Department Of Transportation
   All Rights Reserved

LOG
   rab : 12.09.1998 : Created file 
*****************************************************************************/

class pgsDesigner2 : LongReinfShearChecker
{
public:
   // GROUP: LIFECYCLE

   //------------------------------------------------------------------------
   // Default constructor
   pgsDesigner2();

   //------------------------------------------------------------------------
   // Copy constructor
   pgsDesigner2(const pgsDesigner2& rOther);

   //------------------------------------------------------------------------
   // Destructor
   virtual ~pgsDesigner2();

   // GROUP: OPERATORS
   //------------------------------------------------------------------------
   // Assignment operator
   pgsDesigner2& operator = (const pgsDesigner2& rOther);

   // GROUP: OPERATIONS

   //------------------------------------------------------------------------
   void SetBroker(IBroker* pBroker);
   void SetStatusGroupID(StatusGroupIDType statusGroupID);

   pgsGirderArtifact Check(const CGirderKey& girderKey);
   pgsDesignArtifact Design(const CGirderKey& girderKey,arDesignOptions options);

   void GetHaunchDetails(const CSegmentKey& segmentKey,HAUNCHDETAILS* pHaunchDetails);
   void GetHaunchDetails(const CSegmentKey& segmentKey,const GDRCONFIG& config,HAUNCHDETAILS* pHaunchDetails);

   // GROUP: ACCESS
   // GROUP: INQUIRY

protected:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   //------------------------------------------------------------------------
   void MakeCopy(const pgsDesigner2& rOther);

   //------------------------------------------------------------------------
   virtual void MakeAssignment(const pgsDesigner2& rOther);

   // GROUP: ACCESS
   // GROUP: INQUIRY

private:
   struct StressCheckTask
   {
      IntervalIndexType intervalIdx;
      pgsTypes::LimitState ls;
      pgsTypes::StressType type;
   };
   std::vector<StressCheckTask> m_StressCheckTasks;
   void ConfigureStressCheckTasks(const CSegmentKey& segmentKey);


   // GROUP: DATA MEMBERS
   IBroker* m_pBroker;
   StatusGroupIDType m_StatusGroupID;

   // ID of the status callbacks we have registered
   StatusCallbackIDType m_scidLiveLoad;
   StatusCallbackIDType m_scidBridgeDescriptionError;

   pgsStrandDesignTool m_StrandDesignTool;
   pgsShearDesignTool  m_ShearDesignTool;
   pgsDesignCodes      m_DesignerOutcome;

   // This vector holds the critical section location for the
   // segment currently being designed or analyzed. The value in
   // the vector is a pair. The first element is the critical section details
   // the second element is a boolean value indicating if 0.18f'c < vu (LRFD 5.8.3.2)
   // and therefore a strut-and-tie analysis is required between the Face of Support
   // and the critical section
   std::vector<std::pair<CRITSECTDETAILS,bool>> m_CriticalSections;

   // defines the start and end of support zones
   // support zones are located between the CLBrg and the face of support
   // or the end of an end segment and the face of support
   struct SUPPORTZONE
   {
      Float64 Start;
      Float64 End;
      PierIndexType PierIdx;
      pgsTypes::PierFaceType PierFace;
   };
   std::vector<SUPPORTZONE> m_SupportZones;
   void InitSupportZones(const CSegmentKey& segmentKey);
   ZoneIndexType GetSupportZoneIndex(const pgsPointOfInterest& poi);

   bool m_bShippingDesignWithEqualCantilevers;
   bool m_bShippingDesignIgnoreConfigurationLimits;

   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS

   void CheckTendonStresses(const CGirderKey& girderKey,pgsTendonStressArtifact* pArtifact);
   void CheckStrandStresses(const CSegmentKey& segmentKey,pgsStrandStressArtifact* pArtifact);
   void CheckSegmentStressesAtRelease(const CSegmentKey& segmentKey, const GDRCONFIG* pConfig,pgsTypes::StressType type, pgsSegmentArtifact* pSegmentArtifact);
   void CheckSegmentStresses(const CSegmentKey& segmentKey,const std::vector<pgsPointOfInterest>& vPoi,const StressCheckTask& task,pgsSegmentArtifact* pSegmentArtifact);
   void CheckMomentCapacity(IntervalIndexType intervalIdx,pgsTypes::LimitState ls,pgsGirderArtifact* pGirderArtifact);
   void CheckShear(IntervalIndexType intervalIdx,pgsTypes::LimitState ls,pgsGirderArtifact* pGirderArtifact);
   void CheckShear(bool bDesign,const CSegmentKey&,IntervalIndexType intervalIdx,pgsTypes::LimitState ls,const GDRCONFIG* pConfig,pgsStirrupCheckArtifact* pStirrupArtifact);
   void CheckSplittingZone(const CSegmentKey& segmentKey,const GDRCONFIG* pConfig,pgsStirrupCheckArtifact* pStirrupArtifact);
   void CheckSegmentDetailing(const CSegmentKey& segmentKey,pgsSegmentArtifact* pGdrArtifact);
   void CheckStrandSlope(const CSegmentKey& segmentKey,pgsStrandSlopeArtifact* pArtifact);
   void CheckHoldDownForce(const CSegmentKey& segmentKey,pgsHoldDownForceArtifact* pArtifact);
   void CheckConstructability(const CSegmentKey& segmentKey,pgsConstructabilityArtifact* pArtifact);
   void CheckDebonding(const CSegmentKey& segmentKey,pgsTypes::StrandType strandType,pgsDebondArtifact* pArtifact);

   void UpdateSlabOffsetAdjustmentModel(pgsDesignArtifact* pArtifact);

   void CheckLiveLoadDeflection(const CGirderKey& girderKey,pgsGirderArtifact* pGdrArtifact);

   void GetHaunchDetails(const CSegmentKey& segmentKey,bool bUseConfig,const GDRCONFIG& config,HAUNCHDETAILS* pHaunchDetails);

   // Initialize the design artifact with a first guess of the design
   // variables
   void DesignMidZone(bool bUseCurrentStrands, const arDesignOptions& options,IProgress* pProgress);
   void DesignMidZoneInitialStrands(bool bUseCurrentStrands,IProgress* pProgress);
   void DesignSlabOffset(IProgress* pProgress);
   void DesignMidZoneFinalConcrete(IProgress* pProgress);
   void DesignMidZoneAtRelease(const arDesignOptions& options, IProgress* pProgress);
   void DesignEndZone(bool firstTime, arDesignOptions options, pgsDesignArtifact& artifact,IProgress* pProgress);
   void DesignForShipping(IProgress* pProgress);
   bool CheckShippingStressDesign(const CSegmentKey& segmentKey,const GDRCONFIG& config);

   void DesignEndZoneHarping(arDesignOptions options, pgsDesignArtifact& artifact,IProgress* pProgress);
   void DesignForLiftingHarping(const arDesignOptions& options, bool bAdjustingAfterShipping,IProgress* pProgress);
   void DesignEndZoneReleaseHarping(const arDesignOptions& options, IProgress* pProgress);
   bool CheckLiftingStressDesign(const CSegmentKey& segmentKey,const GDRCONFIG& config);

   void DesignEndZoneDebonding(bool firstPass, arDesignOptions options, pgsDesignArtifact& artifact, IProgress* pProgress);
   std::vector<DebondLevelType> DesignForLiftingDebonding(bool designConcrete, IProgress* pProgress);
   std::vector<DebondLevelType> DesignDebondingForLifting(HANDLINGCONFIG& liftConfig, IProgress* pProgress);
   std::vector<DebondLevelType> DesignEndZoneReleaseDebonding(IProgress* pProgress,bool bAbortOnFail = true);

   void DesignEndZoneReleaseStrength(IProgress* pProgress);
   void DesignConcreteRelease(Float64 topStress, Float64 botStress);

   void RefineDesignForAllowableStress(IProgress* pProgress);
   void RefineDesignForAllowableStress(StressCheckTask task,IProgress* pProgress);
   void RefineDesignForUltimateMoment(IntervalIndexType intervalIdx,pgsTypes::LimitState ls,IProgress* pProgress);
   pgsPointOfInterest GetControllingFinalMidZonePoi(const CSegmentKey& segmentKey);

   // Shear design
   void DesignShear(pgsDesignArtifact* pArtifact, bool bDoStartFromScratch, bool bDoDesignFlexure);

   Float64 GetAvsOverMin(const pgsPointOfInterest& poi,const SHEARCAPACITYDETAILS& scd);

   Float64 GetNormalFrictionForce(const pgsPointOfInterest& poi);

   void CreateFlexuralCapacityArtifact(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::LimitState ls,const GDRCONFIG& config,bool bPositiveMoment,pgsFlexuralCapacityArtifact* pArtifact);
   void CreateFlexuralCapacityArtifact(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::LimitState ls,bool bPositiveMoment,pgsFlexuralCapacityArtifact* pArtifact);
   void CreateFlexuralCapacityArtifact(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::LimitState ls,bool bPositiveMoment,const MOMENTCAPACITYDETAILS& mcd,const MINMOMENTCAPDETAILS& mmcd,pgsFlexuralCapacityArtifact* pArtifact);

   // poi based shear checks
   void CreateStirrupCheckAtPoisArtifact(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx, pgsTypes::LimitState ls, Float64 vu,
                                         Float64 fcSlab,Float64 fcGdr, Float64 fy, bool checkConfinement,const GDRCONFIG* pConfig,
                                         pgsStirrupCheckAtPoisArtifact* pArtifact);

   void InitShearCheck(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,pgsTypes::LimitState ls,const std::vector<CRITSECTDETAILS>& vCSDetails,const GDRCONFIG* pConfig);
   bool IsDeepSection( const pgsPointOfInterest& poi);
   ZoneIndexType GetCriticalSectionZone(const pgsPointOfInterest& poi);
   void CheckStirrupRequirement( const pgsPointOfInterest& poi, const SHEARCAPACITYDETAILS& scd, pgsVerticalShearArtifact* pArtifact );
   void CheckUltimateShearCapacity( const pgsPointOfInterest& poi, const SHEARCAPACITYDETAILS& scd, Float64 vu, const GDRCONFIG* pConfig, pgsVerticalShearArtifact* pArtifact );
   void CheckHorizontalShear( const pgsPointOfInterest& poi, Float64 vu,
                              Float64 fcSlab,Float64 fcGdr, Float64 fy,
                              const GDRCONFIG* pConfig,
                              pgsHorizontalShearArtifact* pArtifact );
   void CheckHorizontalShearMidZone( const pgsPointOfInterest& poi, Float64 vu,
                                     Float64 fcSlab,Float64 fcGdr, Float64 fy,
                                     const GDRCONFIG* pConfig,
                                     pgsHorizontalShearArtifact* pArtifact );

   void ComputeHorizAvs(const pgsPointOfInterest& poi, bool* pIsRoughened, bool* pDoAllStirrupsEngageDeck, const GDRCONFIG* pConfig, pgsHorizontalShearArtifact* pArtifact );

   void CheckFullStirrupDetailing( const pgsPointOfInterest& poi, const pgsVerticalShearArtifact& vertArtifact, 
                                   const SHEARCAPACITYDETAILS& scd, Float64 vu, 
                                   Float64 fcGdr, Float64 fy,
                                   const STIRRUPCONFIG* pConfig,
                                   pgsStirrupDetailArtifact* pArtifact );
public:
   // This function is needed by shear design tool
   void CheckLongReinfShear(const pgsPointOfInterest& poi, 
                            IntervalIndexType intervalIdx,
                            pgsTypes::LimitState ls,
                            const SHEARCAPACITYDETAILS& scd,
                            const GDRCONFIG* pConfig,
                            pgsLongReinfShearArtifact* pArtifact );
private:


   void CheckConfinement(const CSegmentKey& segmentKey, const GDRCONFIG* pConfig, pgsConfinementArtifact* pArtifact);

   // GROUP: ACCESS
   // GROUP: INQUIRY

   DECLARE_LOGFILE;

   bool CollapseZoneData(CShearZoneData zoneData[MAX_ZONES], ZoneIndexType numZones);


   // round slab offset to acceptable value
   Float64 RoundSlabOffset(Float64 offset);

   void GetBridgeAnalysisType(GirderIndexType gdr,const StressCheckTask& task,pgsTypes::BridgeAnalysisType& batTop,pgsTypes::BridgeAnalysisType& batBottom);


   friend pgsLoadRater;

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

#endif // INCLUDED_DESIGNER2_H_
