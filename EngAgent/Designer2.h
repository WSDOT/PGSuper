///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2016  Washington State Department of Transportation
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
#include <IFace\PrestressForce.h>

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


// Class for keeping number of strand iterations from bifurcating out of control
class StrandDesignController
{
   static const Int16 MaxDCRS = 3; // Only allow this many decreases from from/to the same state
public:
   StrandDesignController(pgsStrandDesignTool& designTool):
      m_StrandDesignTool(designTool)
   {
      Init(0);
   }

   bool WasSet() const {return m_State!=strsInitial;} // if false, no strands controlled

   StrandIndexType GetNsCurrent()
   {
      return m_CurrentState.m_NsCurrent;
   }

   void Init(StrandIndexType nsCurrent)
   {
      m_State=strsInitial;
      m_CurrentState.m_NsCurrent = nsCurrent;
      m_CurrentState.m_NsPrevious = INVALID_INDEX;
      m_Decreases.clear();
   }

   enum strUpdateResult
   {
      struValueWasSet,  // use pCurrNx for next iteration
      struUpdateFailed, // tried to set the same combination too many times - we are done iterating
      struConverged     // iteration has converged - use pCurrNx 
   };

   // Update current strand attempt - may return a different value if  bifurcation was detected
   strUpdateResult DoUpdate(StrandIndexType nsCurrent,StrandIndexType nsPrevious, StrandIndexType *pCurrNx)
   {
      *pCurrNx = nsCurrent; // unless otherwise

      if (m_State==strsInitial)
      {
         // first time through, just set 
         m_State = strsSetOnce;
         StoreCurrent( nsCurrent, nsPrevious);
         return struValueWasSet;
      }
      else if (m_State==strsSetOnce || m_State==strsSetDecrease)
      {
         // If have set only once before, or previously decreased; just store value
         if (nsCurrent == m_CurrentState.m_NsCurrent)
         {
            return struConverged;
         }
         else
         {
            m_State = nsCurrent > m_CurrentState.m_NsCurrent ? strsSetIncrease : strsSetDecrease;
            StoreCurrent(nsCurrent, nsPrevious);
            return struValueWasSet;
         }
      }
      else if (m_State==strsSetIncrease)
      {
         // Last update was an increase
         if (nsCurrent == m_CurrentState.m_NsCurrent)
         {
            return struConverged;
         }
         else if (nsCurrent > m_CurrentState.m_NsCurrent)
         {
            // allow new increases to go by
            m_State = strsSetIncrease;
            StoreCurrent(nsCurrent, nsPrevious);
            return struValueWasSet;
         }
         else
         {
            // We have a decrease after an increase. This could be a bifurcation
            Int16 nDCRS = ConditionsMatchDecrease(nsCurrent, nsPrevious);
            if (MaxDCRS >= nDCRS)
            {
               // Not bifurcating yet, count decrease and store value
               if (nDCRS == 0)
               {
                  StoreDecrease(nsCurrent, nsPrevious); // first decrease at this level
               }

               m_State = strsSetDecrease;
               StoreCurrent(nsCurrent, nsPrevious);
               return struValueWasSet;
            }
            else // (MaxDCRS < nDCRS)
            {
               // We are bifurcating. Let's try to resolve.
               // First see if there is room between current and previous to set a new strand
               StrandIndexType nxtn = m_StrandDesignTool.GetNextNumPermanentStrands(nsCurrent);
               if (nxtn < m_CurrentState.m_NsCurrent)
               {
                  // We still have decrease, just not as much as before - call ourself
                  // recursively and see if we can resolve to mid-ground. 
                  StrandIndexType newval;
                  strUpdateResult result = DoUpdate(nxtn,  nsPrevious, &newval);
                  *pCurrNx = newval;
                  return result;
               }
               else
               {
                  // No wiggle room - we probably will bifuctate forever.
                  // Assume we have converged on the higher strand value and move onward
                  *pCurrNx = nsPrevious;
                  return struConverged;
               }
            }
         }
      }
      else
      {
         ATLASSERT(0); // bad condition??
         return struUpdateFailed;
      }
   }

private:
   StrandDesignController(); // no default constr

   Int16 ConditionsMatchDecrease(StrandIndexType nsCurrent, StrandIndexType nsPrevious)
   {
      strDesignState local( nsCurrent, nsPrevious);
      DIterator it = std::find(m_Decreases.begin(), m_Decreases.end(), local);
      if (it != m_Decreases.end())
      {
         return ++it->m_RepeatCount;
      }
      else
      {
         return 0;
      }
   }

   void StoreCurrent(StrandIndexType nsCurrent, StrandIndexType nsPrevious)
   {
      m_CurrentState.m_NsCurrent   = nsCurrent;
      m_CurrentState.m_NsPrevious  = nsPrevious;
   }

   void StoreDecrease(StrandIndexType nsCurrent, StrandIndexType nsPrevious)
   {
      // assumption here is that ConditionsMatchDecrease() returned 0 before this call
      strDesignState local( nsCurrent, nsPrevious );
      local.m_RepeatCount = 1;
      m_Decreases.push_front(local);
   }

   enum strState
   {
      strsInitial,
      strsSetOnce,     // have a current value, but no decreases
      strsSetDecrease, // last update stored a decrease
      strsSetIncrease  // last update stored an increase
   };

   strState               m_State; // state we are in

   // currently set values for controlling limit state
   struct strDesignState
   {
      StrandIndexType m_NsCurrent;
      StrandIndexType m_NsPrevious;
      Int16           m_RepeatCount;

      strDesignState():
      m_RepeatCount(0), m_NsCurrent(0), m_NsPrevious(INVALID_INDEX)
      {;}

      strDesignState(StrandIndexType nsCurrent,StrandIndexType nsPrevious):
      m_RepeatCount(0), m_NsCurrent(nsCurrent), m_NsPrevious(nsPrevious)
      {;}

      bool operator == (const strDesignState& rOther) const
      {
         // note that repeat count is not part of operator
         return m_NsCurrent  == rOther.m_NsCurrent  &&
                m_NsPrevious == rOther.m_NsPrevious;
      }
   };

   strDesignState m_CurrentState;

   // List of all decreases
   typedef std::list<strDesignState> DContainer; 
   typedef DContainer::iterator DIterator;

   DContainer m_Decreases;
   pgsStrandDesignTool& m_StrandDesignTool;
};


#define NUM_LEGS  2
#define MAX_ZONES 4

// Structure and function for computing eccentricity envelope. 
//////////////////////////////////////////////////////////////
struct pgsEccEnvelope
{
   // Upper bound (ub) and Lower bound eccentricities
   Float64 m_UbEcc;
   pgsTypes::StressType     m_UbStressType; // stress type (tension or compression) 
   IntervalIndexType        m_UbInterval;   // controlling interval
   pgsTypes::LimitState     m_UbLimitState; // ""          ls
   Float64 m_LbEcc;
   pgsTypes::StressType     m_LbStressType; // stress type (tension or compression) 
   IntervalIndexType        m_LbInterval;   // controlling interval
   pgsTypes::LimitState     m_LbLimitState; // ""          ls

   pgsEccEnvelope():
   m_UbEcc(Float64_Max),
   m_LbEcc(-Float64_Max)
   {;}

   // Functions to save controlling Lb or Ub
   bool SaveControllingUpperBound(Float64 ecc, pgsTypes::StressType type, IntervalIndexType intervalIdx, pgsTypes::LimitState ls)
   {
      if (ecc < m_UbEcc)
      {
         m_UbEcc        = ecc;
         m_UbStressType = type;
         m_UbInterval   = intervalIdx;
         m_UbLimitState = ls;
         return true;
      }
      else
      {
         return false;
      }
   }

   bool SaveControllingLowerBound(Float64 ecc, pgsTypes::StressType type, IntervalIndexType intervalIdx, pgsTypes::LimitState ls)
   {
      if (m_LbEcc < ecc)
      {
         m_LbEcc        = ecc;
         m_LbStressType = type;
         m_LbInterval   = intervalIdx;
         m_LbLimitState = ls;
         return true;
      }
      else
      {
         return false;
      }
   }
};

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

   // Creates a girder check artifact.
   const pgsGirderArtifact* Check(const CGirderKey& girderKey);

   // Creates a lifting analysis artifact
   const pgsLiftingAnalysisArtifact* CheckLifting(const CSegmentKey& segmentKey);

   // Creates a hauling analysis artifact
   const pgsHaulingAnalysisArtifact* pgsDesigner2::CheckHauling(const CSegmentKey& segmentKey);

   pgsGirderDesignArtifact Design(const CGirderKey& girderKey,const std::vector<arDesignOptions>& DesOptionsColl);

   void GetHaunchDetails(const CSpanKey& spanKey,HAUNCHDETAILS* pHaunchDetails);
   void GetHaunchDetails(const CSpanKey& spanKey,const GDRCONFIG& config,HAUNCHDETAILS* pHaunchDetails);
   Float64 GetSectionGirderOrientationEffect(const pgsPointOfInterest& poi);

   pgsEccEnvelope GetEccentricityEnvelope(const pgsPointOfInterest& rpoi,const GDRCONFIG& config);

   // Returns a girder check artifact if the girder was already checked, otherwise returns NULL
   const pgsGirderArtifact* GetGirderArtifact(const CGirderKey& girderKey);

   // Returns a lifting analysis artifact if the segment was already checked, otherwise returns NULL
   const pgsLiftingAnalysisArtifact* GetLiftingAnalysisArtifact(const CSegmentKey& segmentKey);

   // Returns a nauling analysis artifact if the segment was already checked, otherwise returns NULL
   const pgsHaulingAnalysisArtifact* GetHaulingAnalysisArtifact(const CSegmentKey& segmentKey);

   // Clears all cached artifacts
   void ClearArtifacts();

protected:
   void DoDesign(const CGirderKey& girderKey, const arDesignOptions& options, pgsGirderDesignArtifact& artifact);
   void MakeCopy(const pgsDesigner2& rOther);
   virtual void MakeAssignment(const pgsDesigner2& rOther);

private:
   std::map<CGirderKey,boost::shared_ptr<pgsGirderArtifact>> m_CheckArtifacts;
   std::map<CSegmentKey,pgsLiftingAnalysisArtifact> m_LiftingAnalysisArtifacts;
   std::map<CSegmentKey,const pgsHaulingAnalysisArtifact*> m_HaulingAnalysisArtifacts;

   const pgsHaulingAnalysisArtifact* CheckHauling(const CSegmentKey& segmentKey, SHARED_LOGFILE LOGFILE);

   struct StressCheckTask
   {
      IntervalIndexType intervalIdx;
      pgsTypes::LimitState limitState;
      pgsTypes::StressType stressType;
      bool bIncludeLiveLoad; // if intervalIdx is a live load interval, live load is include in the prestressing if this parameter is tru
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

   void CheckTendonDetailing(const CGirderKey& girderKey,pgsGirderArtifact* pGirderArtifact);
   void CheckTendonStresses(const CGirderKey& girderKey,pgsGirderArtifact* pGirderArtifact);
   void CheckStrandStresses(const CSegmentKey& segmentKey,pgsStrandStressArtifact* pArtifact);
   void CheckSegmentStressesAtRelease(const CSegmentKey& segmentKey, const GDRCONFIG* pConfig,pgsTypes::StressType type, pgsSegmentArtifact* pSegmentArtifact);
   void CheckSegmentStresses(const CSegmentKey& segmentKey,const std::vector<pgsPointOfInterest>& vPoi,const StressCheckTask& task,pgsSegmentArtifact* pSegmentArtifact);
   void CheckMomentCapacity(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,pgsGirderArtifact* pGirderArtifact);
   void CheckShear(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,pgsGirderArtifact* pGirderArtifact);
   void CheckShear(bool bDesign,const CSegmentKey&,IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const GDRCONFIG* pConfig,pgsStirrupCheckArtifact* pStirrupArtifact);
   void CheckSplittingZone(const CSegmentKey& segmentKey,const GDRCONFIG* pConfig,pgsStirrupCheckArtifact* pStirrupArtifact);
   void CheckSegmentDetailing(const CSegmentKey& segmentKey,pgsSegmentArtifact* pGdrArtifact);
   void CheckStrandSlope(const CSegmentKey& segmentKey,pgsStrandSlopeArtifact* pArtifact);
   void CheckHoldDownForce(const CSegmentKey& segmentKey,pgsHoldDownForceArtifact* pArtifact);
   void CheckSegmentStability(const CSegmentKey& segmentKey,pgsSegmentStabilityArtifact* pArtifact);
   void CheckDebonding(const CSegmentKey& segmentKey,pgsTypes::StrandType strandType,pgsDebondArtifact* pArtifact);

   void CheckConstructability(const CGirderKey& girderKey,pgsConstructabilityArtifact* pArtifact);

   void UpdateSlabOffsetAdjustmentModel(pgsSegmentDesignArtifact* pArtifact);

   void CheckLiveLoadDeflection(const CGirderKey& girderKey,pgsGirderArtifact* pGdrArtifact);

   void GetHaunchDetails(const CSpanKey& spanKey,bool bUseConfig,const GDRCONFIG& config,HAUNCHDETAILS* pHaunchDetails);

   // Initialize the design artifact with a first guess of the design
   // variables
   void DesignMidZone(bool bUseCurrentStrands, const arDesignOptions& options,IProgress* pProgress);
   void DesignMidZoneInitialStrands(bool bUseCurrentStrands,IProgress* pProgress);
   void DesignSlabOffset(IProgress* pProgress);
   void DesignMidZoneFinalConcrete(IProgress* pProgress);
   void DesignMidZoneAtRelease(const arDesignOptions& options, IProgress* pProgress);
   void DesignEndZone(bool firstTime, arDesignOptions options, pgsSegmentDesignArtifact& artifact,IProgress* pProgress);
   void DesignForShipping(IProgress* pProgress);
   bool CheckShippingStressDesign(const CSegmentKey& segmentKey,const GDRCONFIG& config);

   void DesignEndZoneHarping(arDesignOptions options, pgsSegmentDesignArtifact& artifact,IProgress* pProgress);
   void DesignForLiftingHarping(const arDesignOptions& options, bool bAdjustingAfterShipping,IProgress* pProgress);
   void DesignEndZoneReleaseHarping(const arDesignOptions& options, IProgress* pProgress);
   bool CheckLiftingStressDesign(const CSegmentKey& segmentKey,const GDRCONFIG& config);

   void DesignEndZoneDebonding(bool firstPass, arDesignOptions options, pgsSegmentDesignArtifact& artifact, IProgress* pProgress);
   std::vector<DebondLevelType> DesignForLiftingDebonding(bool designConcrete, IProgress* pProgress);
   std::vector<DebondLevelType> DesignDebondingForLifting(HANDLINGCONFIG& liftConfig, IProgress* pProgress);
   std::vector<DebondLevelType> DesignEndZoneReleaseDebonding(IProgress* pProgress,bool bAbortOnFail = true);

   void DesignEndZoneReleaseStrength(IProgress* pProgress);
   void DesignConcreteRelease(Float64 topStress, Float64 botStress);

   void RefineDesignForAllowableStress(IProgress* pProgress);
   void RefineDesignForAllowableStress(const StressCheckTask& task,IProgress* pProgress);
   void RefineDesignForUltimateMoment(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,IProgress* pProgress);
   pgsPointOfInterest GetControllingFinalMidZonePoi(const CSegmentKey& segmentKey);

   // Shear design
   void DesignShear(pgsSegmentDesignArtifact* pArtifact, bool bDoStartFromScratch, bool bDoDesignFlexure);

   Float64 GetAvsOverMin(const pgsPointOfInterest& poi,const SHEARCAPACITYDETAILS& scd);

   Float64 GetNormalFrictionForce(const pgsPointOfInterest& poi);

   void CreateFlexuralCapacityArtifact(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const GDRCONFIG& config,bool bPositiveMoment,pgsFlexuralCapacityArtifact* pArtifact);
   void CreateFlexuralCapacityArtifact(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,bool bPositiveMoment,pgsFlexuralCapacityArtifact* pArtifact);
   void CreateFlexuralCapacityArtifact(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,bool bPositiveMoment,const MOMENTCAPACITYDETAILS& mcd,const MINMOMENTCAPDETAILS& mmcd,bool bDesign,pgsFlexuralCapacityArtifact* pArtifact);

   // poi based shear checks
   void CreateStirrupCheckAtPoisArtifact(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx, pgsTypes::LimitState limitState, Float64 vu,
                                         Float64 fcSlab,Float64 fcGdr, Float64 fy, bool checkConfinement,const GDRCONFIG* pConfig,
                                         pgsStirrupCheckAtPoisArtifact* pArtifact);

   void InitShearCheck(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const GDRCONFIG* pConfig);
   bool IsDeepSection( const pgsPointOfInterest& poi);
   ZoneIndexType GetCriticalSectionZone(const pgsPointOfInterest& poi,bool bIncludeCS=false);
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
                            pgsTypes::LimitState limitState,
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
   void ComputeConcreteStrength(pgsFlexuralStressArtifact& artifact,pgsTypes::StressLocation stressLocation,const pgsPointOfInterest& poi,const StressCheckTask& task);


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
