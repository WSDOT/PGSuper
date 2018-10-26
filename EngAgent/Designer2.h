///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2015  Washington State Department of Transportation
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
   pgsTypes::Stage          m_UbStage;      // controlling stage
   pgsTypes::LimitState     m_UbLimitState; // ""          ls
   Float64 m_LbEcc;
   pgsTypes::StressType     m_LbStressType; // stress type (tension or compression) 
   pgsTypes::Stage          m_LbStage;      // controlling stage
   pgsTypes::LimitState     m_LbLimitState; // ""          ls

   pgsEccEnvelope():
   m_UbEcc(Float64_Max),
   m_LbEcc(-Float64_Max)
   {;}

   // Functions to save controlling Lb or Ub
   bool SaveControllingUpperBound(Float64 ecc, pgsTypes::StressType type, pgsTypes::Stage stage, pgsTypes::LimitState ls)
   {
      if (ecc < m_UbEcc)
      {
         m_UbEcc        = ecc;
         m_UbStressType = type;
         m_UbStage      = stage;
         m_UbLimitState = ls;
         return true;
      }
      else
         return false;
   }

   bool SaveControllingLowerBound(Float64 ecc, pgsTypes::StressType type, pgsTypes::Stage stage, pgsTypes::LimitState ls)
   {
      if (ecc > m_LbEcc)
      {
         m_LbEcc        = ecc;
         m_LbStressType = type;
         m_LbStage      = stage;
         m_LbLimitState = ls;
         return true;
      }
      else
         return false;
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
   struct ALLOWSTRESSCHECKTASK
   {
      pgsTypes::Stage stage;
      pgsTypes::LimitState ls;
      pgsTypes::StressType type;
   };

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

   pgsGirderArtifact Check(SpanIndexType span,GirderIndexType gdr);
   pgsDesignArtifact Design(SpanIndexType span,GirderIndexType gdr,const std::vector<arDesignOptions>& DesOptionsColl);

   void GetHaunchDetails(SpanIndexType span,GirderIndexType gdr,HAUNCHDETAILS* pHaunchDetails);
   void GetHaunchDetails(SpanIndexType span,GirderIndexType gdr,const GDRCONFIG& config,HAUNCHDETAILS* pHaunchDetails);

 
   pgsEccEnvelope GetEccentricityEnvelope(const pgsPointOfInterest& rpoi,const GDRCONFIG& config);

   // GROUP: ACCESS
   // GROUP: INQUIRY

protected:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS

   void DoDesign(SpanIndexType span,GirderIndexType gdr, const arDesignOptions& options, pgsDesignArtifact& artifact);

   //------------------------------------------------------------------------
   void MakeCopy(const pgsDesigner2& rOther);

   //------------------------------------------------------------------------
   void MakeAssignment(const pgsDesigner2& rOther);

   // GROUP: ACCESS
   // GROUP: INQUIRY

private:

   // GROUP: DATA MEMBERS
   IBroker* m_pBroker;
   StatusGroupIDType m_StatusGroupID;

   // ID of the status callbacks we have registered
   StatusCallbackIDType m_scidLiveLoad;
   StatusCallbackIDType m_scidBridgeDescriptionError;

   pgsStrandDesignTool m_StrandDesignTool;
   pgsShearDesignTool  m_ShearDesignTool;
   pgsDesignCodes      m_DesignerOutcome;

   pgsPointOfInterest m_LeftCS;
   pgsPointOfInterest m_RightCS;
   bool m_bLeftCS_StrutAndTieRequired;
   bool m_bRightCS_StrutAndTieRequired;

   bool m_bShippingDesignWithEqualCantilevers;
   bool m_bShippingDesignIgnoreConfigurationLimits;

   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS

   void CheckStrandStresses(SpanIndexType span,GirderIndexType gdr,pgsStrandStressArtifact* pArtifact);
   void CheckGirderStresses(SpanIndexType span,GirderIndexType gdr,const ALLOWSTRESSCHECKTASK& task,pgsGirderArtifact* pGdrArtifact);
   void CheckGirderStresses(SpanIndexType span,GirderIndexType gdr,const ALLOWSTRESSCHECKTASK& task,const GDRCONFIG* pConfig,pgsGirderArtifact* pGdrArtifact);
   void CheckMomentCapacity(SpanIndexType span,GirderIndexType gdr,pgsTypes::Stage stage,pgsTypes::LimitState ls,pgsGirderArtifact* pGdrArtifact);
   void CheckShear(SpanIndexType span,GirderIndexType gdr,const std::vector<pgsPointOfInterest>& rVPoi,pgsTypes::LimitState ls,const GDRCONFIG* pConfig,pgsStirrupCheckArtifact* pStirrupArtifact);
   void CheckSplittingZone(SpanIndexType span,GirderIndexType gdr,const GDRCONFIG* pConfig,pgsStirrupCheckArtifact* pStirrupArtifact);
   void CheckGirderDetailing(SpanIndexType span,GirderIndexType gdr,pgsGirderArtifact* pGdrArtifact);
   void CheckStrandSlope(SpanIndexType span,GirderIndexType gdr,pgsStrandSlopeArtifact* pArtifact);
   void CheckHoldDownForce(SpanIndexType span,GirderIndexType gdr,pgsHoldDownForceArtifact* pArtifact);
   void CheckConstructability(SpanIndexType span,GirderIndexType gdr,pgsConstructabilityArtifact* pArtifact);
   void CheckDebonding(SpanIndexType span,GirderIndexType gdr,pgsTypes::StrandType strandType,pgsDebondArtifact* pArtifact);


   void UpdateSlabOffsetAdjustmentModel(pgsDesignArtifact* pArtifact);

   void CheckLiveLoadDeflection(SpanIndexType span,GirderIndexType gdr,pgsDeflectionCheckArtifact* pArtifact);

   void GetHaunchDetails(SpanIndexType span,GirderIndexType gdr,bool bUseConfig,const GDRCONFIG& config,HAUNCHDETAILS* pHaunchDetails);

   // Initialize the design artifact with a first guess of the design
   // variables
   void DesignMidZone(bool bUseCurrentStrands, const arDesignOptions& options,IProgress* pProgress);
   void DesignMidZoneInitialStrands(bool bUseCurrentStrands,IProgress* pProgress);
   void DesignSlabOffset(IProgress* pProgress);
   void DesignMidZoneFinalConcrete(IProgress* pProgress);
   void DesignMidZoneAtRelease(const arDesignOptions& options, IProgress* pProgress);
   void DesignEndZone(bool firstTime, arDesignOptions options, pgsDesignArtifact& artifact,IProgress* pProgress);
   void DesignForShipping(IProgress* pProgress);
   bool CheckShippingStressDesign(SpanIndexType span,GirderIndexType gdr,const GDRCONFIG& config);

   void DesignEndZoneHarping(arDesignOptions options, pgsDesignArtifact& artifact,IProgress* pProgress);
   void DesignForLiftingHarping(const arDesignOptions& options, bool bAdjustingAfterShipping,IProgress* pProgress);
   void DesignEndZoneReleaseHarping(const arDesignOptions& options, IProgress* pProgress);
   bool CheckLiftingStressDesign(SpanIndexType span,GirderIndexType gdr,const GDRCONFIG& config);

   void DesignEndZoneDebonding(bool firstPass, arDesignOptions options, pgsDesignArtifact& artifact, IProgress* pProgress);
   std::vector<DebondLevelType> DesignForLiftingDebonding(bool designConcrete, IProgress* pProgress);
   std::vector<DebondLevelType> DesignDebondingForLifting(HANDLINGCONFIG& liftConfig, IProgress* pProgress);
   std::vector<DebondLevelType> DesignEndZoneReleaseDebonding(IProgress* pProgress,bool bAbortOnFail = true);

   void DesignEndZoneReleaseStrength(IProgress* pProgress);
   void DesignConcreteRelease(Float64 topStress, Float64 botStress);

   void RefineDesignForAllowableStress(IProgress* pProgress);
   void RefineDesignForAllowableStress(const ALLOWSTRESSCHECKTASK& task,IProgress* pProgress);
   void RefineDesignForUltimateMoment(pgsTypes::Stage stage,pgsTypes::LimitState ls,IProgress* pProgress);
   pgsPointOfInterest GetControllingFinalMidZonePoi(SpanIndexType span,GirderIndexType gdr);

   // Shear design
   void DesignShear(pgsDesignArtifact* pArtifact, bool bDoStartFromScratch, bool bDoDesignFlexure);

   Float64 GetAvsOverMin(const pgsPointOfInterest& poi,const SHEARCAPACITYDETAILS& scd);

   Float64 GetNormalFrictionForce(const pgsPointOfInterest& poi);

   pgsFlexuralCapacityArtifact CreateFlexuralCapacityArtifact(const pgsPointOfInterest& poi,pgsTypes::Stage stage,pgsTypes::LimitState ls,const GDRCONFIG& config,bool bPositiveMoment);
   pgsFlexuralCapacityArtifact CreateFlexuralCapacityArtifact(const pgsPointOfInterest& poi,pgsTypes::Stage stage,pgsTypes::LimitState ls,bool bPositiveMoment);
   pgsFlexuralCapacityArtifact CreateFlexuralCapacityArtifact(const pgsPointOfInterest& poi,pgsTypes::Stage stage,pgsTypes::LimitState ls,bool bPositiveMoment,const MOMENTCAPACITYDETAILS& mcd,const MINMOMENTCAPDETAILS& mmcd,bool bDesign);

   // poi based shear checks
   pgsStirrupCheckAtPoisArtifact CreateStirrupCheckAtPoisArtifact(const pgsPointOfInterest& poi,pgsTypes::Stage stage, pgsTypes::LimitState ls, Float64 vu,
                                                                  Float64 fcSlab,Float64 fcGdr, Float64 fy, bool checkConfinement,const GDRCONFIG* pConfig);

   void InitShearCheck(SpanIndexType span,GirderIndexType gdr,const std::vector<pgsPointOfInterest>& VPoi,pgsTypes::LimitState ls,const GDRCONFIG* pConfig);
   bool IsDeepSection( const pgsPointOfInterest& poi);
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
                            pgsTypes::Stage stage,
                            pgsTypes::LimitState ls,
                            const SHEARCAPACITYDETAILS& scd,
                            const GDRCONFIG* pConfig,
                            pgsLongReinfShearArtifact* pArtifact );

private:


   void CheckConfinement(SpanIndexType span, GirderIndexType gdr, const GDRCONFIG* pConfig, pgsConfinementArtifact* pArtifact);

   // GROUP: ACCESS
   // GROUP: INQUIRY

   DECLARE_LOGFILE;

   bool CollapseZoneData(CShearZoneData zoneData[MAX_ZONES], Uint32 numZones);


   // round slab offset to acceptable value
   Float64 RoundSlabOffset(Float64 offset);

   void GetBridgeAnalysisType(GirderIndexType gdr,const ALLOWSTRESSCHECKTASK& task,BridgeAnalysisType& batTop,BridgeAnalysisType& batBottom);


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
