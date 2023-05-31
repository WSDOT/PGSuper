///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2023  Washington State Department of Transportation
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

#ifndef INCLUDED_STRANDDESIGNTOOL_H_
#define INCLUDED_STRANDDESIGNTOOL_H_

// SYSTEM INCLUDES
//

// PROJECT INCLUDES
//

#include <IFace\Artifact.h>
#include <IFace\Bridge.h>
#include <IFace\PointOfInterest.h>
#include <PgsExt\PoiMgr.h>

#include "RaisedStraightStrandDesignTool.h"

#include <algorithm>
#include<list>
#include<vector>

// LOCAL INCLUDES
//

struct InitialDesignParameters
{
   StressCheckTask task;
   std::_tstring strLimitState;
   std::_tstring strStressLocation;
   pgsTypes::StressLocation stress_location;
   Float64 fmin;
   Float64 fmax;
   Float64 fAllow;
   Float64 fpre;
   Float64 Preqd;
   StrandIndexType Np;
   Float64 fN;

   InitialDesignParameters(IntervalIndexType intervalIdx,bool bIncludeLiveLoad,
                           pgsTypes::LimitState limitState,LPCTSTR lpszLimitState,
                           pgsTypes::StressLocation stressLocation,LPCTSTR lpszStressLocation,
                           pgsTypes::StressType stressType) :
      task(intervalIdx,limitState,stressType,bIncludeLiveLoad),strLimitState(lpszLimitState),
      stress_location(stressLocation),strStressLocation(lpszStressLocation)
   {}
};

typedef Int32 DebondLevelType; // NEED this to be a signed type!

static std::_tstring DumpIntVector(const std::vector<DebondLevelType>& rvec)
{
   std::_tostringstream os;
   std::for_each(std::cbegin(rvec), std::prev(std::cend(rvec)), [&os](const auto& value) {os << value << _T(", "); });
   os << rvec.back();
   return os.str();
}

// FORWARD DECLARATIONS
//
interface IBroker;
interface IPretensionForce;

// MISCELLANEOUS
// Strand adjustment outcomes
#define STRAND_ADJUST_UNCHANGED               2
#define STRAND_ADJUST_SUCCESS                 1
#define STRAND_ADJUST_FAILURE                 0

// enum to determine if computation of concrete strength succeeded, and if it did,
// was minimum rebar required
enum ConcStrengthResultType {ConcFailed, ConcSuccess, ConcSuccessWithRebar};

/*****************************************************************************
CLASS 
   pgsStrandDesignTool

   Encapsulates the design strategy


DESCRIPTION
   Encapsulates the design strategy

LOG
   rdp : 3.22.2007 : Created file 
*****************************************************************************/

class pgsStrandDesignTool : public ISegmentLiftingDesignPointsOfInterest, public ISegmentHaulingDesignPointsOfInterest
{
public:
   // Reasons for adjusting strands
   enum AdjustReason
   {
      CompTopAtEnds,
      CompBotAtEnds,
      CompTopBtwnHp,
      CompBotBtwnHp,
      TensTopAtEnds,
      TensBotAtEnds,
      TensTopBtwnHp,
      TensBotBtwnHp
   };

   // GROUP: LIFECYCLE

   //------------------------------------------------------------------------
   // Default constructor
   pgsStrandDesignTool(SHARED_LOGFILE lf);
   
   void Initialize(IBroker* pBroker, StatusGroupIDType statusGroupID, pgsSegmentDesignArtifact* pArtifact);

   void InitReleaseStrength(Float64 fci,IntervalIndexType intervalIdx);
   void InitFinalStrength(Float64 fc,IntervalIndexType intervalIdx);

   void RestoreDefaults(bool retainProportioning, bool justAddedRaisedStrands);

   // GROUP: OPERATIONS
   void FillArtifactWithFlexureValues();

   Float64 GetSegmentLength() const; // a little utility function to return a commonly used value

   // adding and removing strands
   StrandIndexType GetNumPermanentStrands() const;
   StrandIndexType GetMaxPermanentStrands() const;
   StrandIndexType GetNumTotalStrands() const;
   StrandIndexType GetNh() const;
   StrandIndexType GetNs() const;
   StrandIndexType GetNt() const;

   void SetConcreteAccuracy(Float64 accuracy);
   Float64 GetConcreteAccuracy() const;

   bool SetNumPermanentStrands(StrandIndexType num);
   bool SetNumStraightHarped(StrandIndexType ns, StrandIndexType nh);
   bool SetNumTempStrands(StrandIndexType num);
   bool SwapStraightForHarped();
   bool AddStrands();
   bool AddTempStrands();

   // This really doesn't actually add strands, it resequences fill to add raised straight strands for all straight designs. 
   // Will fail if IsDesignRaisedStraight is false
   bool AddRaisedStraightStrands();

   // If fill order can be simplified - do it at final end of flexural design
   void SimplifyDesignFillOrder(pgsSegmentDesignArtifact* pArtifact) const;

   StrandIndexType GetNextNumPermanentStrands(StrandIndexType prevNum) const;
   StrandIndexType GetPreviousNumPermanentStrands(StrandIndexType nextNum) const;
   bool IsValidNumPermanentStrands(StrandIndexType num) const;

   void SetMinimumPermanentStrands(StrandIndexType num);
   StrandIndexType GetMinimumPermanentStrands() const;

   StrandIndexType GuessInitialStrands();
   arDesignStrandFillType GetOriginalStrandFillType() const;

   // Prestress forces
   Float64 GetPjackStraightStrands() const;
   Float64 GetPjackTempStrands() const;
   Float64 GetPjackHarpedStrands() const;

   // returns the prestress force at lifting
   Float64 GetPrestressForceAtLifting(const GDRCONFIG& guess,const pgsPointOfInterest& poi) const;

   Float64 GetPrestressForceMidZone(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi) const;

   // if Np is set to INVALID_INDEX, cannot handle force
   void ComputePermanentStrandsRequiredForPrestressForce(const pgsPointOfInterest& poi,InitialDesignParameters* pDesignParams) const;

   Float64 ComputeEccentricity(const pgsPointOfInterest& poi, IntervalIndexType intervalIdx) const;

   Float64 GetTransferLength(pgsTypes::StrandType strandType) const;

   // Set end zone strand configuration to maximize debonding or harping depending on design type
   bool ResetEndZoneStrandConfig();

   // Harped Strand Design
   ///////////////////////
   // Harp adjustments
   Float64 GetMinimumFinalMidZoneEccentricity() const;
   void SetMinimumFinalMidZoneEccentricity(Float64 ecc); 

   // offsets are measured from original strand grid location from library
   Float64 GetHarpStrandOffsetEnd(pgsTypes::MemberEndType endType) const;
   Float64 GetHarpStrandOffsetHp(pgsTypes::MemberEndType endType) const;

   void SetHarpStrandOffsetEnd(pgsTypes::MemberEndType endType,Float64 off);
   void SetHarpStrandOffsetHp(pgsTypes::MemberEndType endType,Float64 off);

   Float64 GetHarpedHpOffsetIncrement(IStrandGeometry* pStrandGeom) const;
   Float64 GetHarpedEndOffsetIncrement(IStrandGeometry* pStrandGeom) const;

   Float64 ComputeEndOffsetForEccentricity(const pgsPointOfInterest& poi, Float64 ecc) const;
   bool ComputeMinHarpedForEndZoneEccentricity(const pgsPointOfInterest& poi, Float64 ecc, IntervalIndexType intervalIdx, StrandIndexType* pNs, StrandIndexType* pNh) const;

   bool ComputeAddHarpedForMidZoneReleaseEccentricity(const pgsPointOfInterest& poi, Float64 ecc, Float64 minEcc, StrandIndexType* pNs, StrandIndexType* pNh) const;

   Float64 ComputeHpOffsetForEccentricity(const pgsPointOfInterest& poi, Float64 ecc,IntervalIndexType intervalIdx) const;

   // Debonded Strand Design
   /////////////////////////
   enum DebondEndType {dbLeft, dbRight};

   // Max number of equal-length debond sections at each end of girder (debonding must be symmetric for design)
   SectionIndexType GetMaxNumberOfDebondSections() const;
   Float64 GetDebondSectionLength() const;

   // function and structure for computing debond layout for stress demand
   struct StressDemand
   {
      pgsPointOfInterest m_Poi;
      Float64 m_PrestressForcePerStrand; // this value changes along section
      Float64 m_TopStress;
      Float64 m_BottomStress;
   };

   std::vector<DebondLevelType> ComputeDebondsForDemand(const std::vector<StressDemand>& demands, const GDRCONFIG& fullyBondedConfig, Float64 cgFullyBonded, 
                                                        IntervalIndexType interval, Float64 allowTens, Float64 allowComp) const;

   // section zero is at GetDebondSectionLength from end of girder
   Float64 GetDebondSectionLocation(SectionIndexType sectionIdx, DebondEndType end) const;

   // Get Indices of Section just inboard and outboard of location and distance from outboard section to location
   void GetDebondSectionForLocation(Float64 location, SectionIndexType* pOutBoardSectionIdx, SectionIndexType* pInBoardSectionIdx, Float64* pOutToInDistance) const;

   // Given an amount of stress demand, return the minimum debond level to relieve the stress
   void GetDebondLevelForTopTension(const StressDemand& demand, const GDRCONFIG& fullyBondedConfig, Float64 cgFullyBonded, IntervalIndexType interval, Float64 tensDemand, Float64 outboardDistance,
                                    Float64 Hg, Float64 Yb,  Float64 eccX, Float64 Ca, Float64 Cmx, Float64 Cmy,
                                    DebondLevelType* pOutboardLevel, DebondLevelType* pInboardLevel) const;

   void GetDebondLevelForBottomCompression(const StressDemand& demand, const GDRCONFIG& fullyBondedConfig, Float64 cgFullyBonded, IntervalIndexType interval, Float64 tensDemand, Float64 outboardDistance,
                                           Float64 Hg, Float64 Yb, Float64 eccX, Float64 Ca, Float64 Cmx, Float64 Cmy,
                                           DebondLevelType* pOutboardLevel, DebondLevelType* pInboardLevel) const;

   // Debonding levels are integer values used to quantify the amount of strands debonded at
   // a given section.
   // Get max level based on given number of strands, and number of sections outboard of desired section
   DebondLevelType GetMaxDebondLevel(StrandIndexType numStrands, SectionIndexType numLeadingSections ) const;

   // returns the maximum physically debonding the girder can handle based on the current number of strands
   const std::vector<DebondLevelType>& GetMaxPhysicalDebonding() const;

   // take a raw debond level layout, smooth debonding if necessary, and check that it is 
   // within maximum limits. Return an empty vector if we fail
   void RefineDebondLevels( std::vector<DebondLevelType>& rDebondLevelsAtSections ) const;

   // workhorse function to go from debond levels to set actual debond layout. 
   // Returns false if layout cannot be created
   bool LayoutDebonding(const std::vector<DebondLevelType>& rDebondLevelsAtSections);


   // ACCESS
   //////////
   const GDRCONFIG& GetSegmentConfiguration() const;

   bool IsDesignDebonding() const;
   bool IsDesignHarping() const;
   bool IsDesignRaisedStraight() const;

   const CSegmentKey& GetSegmentKey() const
   {
      return m_SegmentKey;
   }

   // left and right ends of mid-zone. Measured from girder start
   void GetMidZoneBoundaries(Float64* leftEnd, Float64* rightEnd) const;

   // POI's for design
   pgsPointOfInterest GetPointOfInterest(const CSegmentKey& segmentKey,Float64 Xpoi) const;
   void GetPointsOfInterest(const CSegmentKey& segmentKey, PoiAttributeType attrib,PoiList* pPoiList) const;
   void GetDesignPoi(IntervalIndexType intervalIdx, PoiList* pPoiList) const;
   void GetDesignPoi(IntervalIndexType intervalIdx,PoiAttributeType attrib, PoiList* pPoiList) const;
   void GetDesignPoiEndZone(IntervalIndexType intervalIdx, PoiList* pPoiList) const;
   void GetDesignPoiEndZone(IntervalIndexType intervalIdx,PoiAttributeType attrib, PoiList* pPoiList) const;

   pgsPointOfInterest GetDebondSamplingPOI(IntervalIndexType intervalIdx) const;
   // interface ISegmentLiftingDesignPointsOfInterest
   // locations of points of interest
   virtual void GetLiftingDesignPointsOfInterest(const CSegmentKey& segmentKey, Float64 overhang, PoiAttributeType attrib, std::vector<pgsPointOfInterest>* pvPoi, Uint32 mode = POIFIND_OR) const override;

   //ISegmentLiftingDesignPointsOfInterest
   virtual void GetHaulingDesignPointsOfInterest(const CSegmentKey& segmentKey, Uint16 nPnts, Float64 leftOverhang, Float64 rightOverhang, PoiAttributeType attrib, std::vector<pgsPointOfInterest>* pvPoi,Uint32 mode = POIFIND_OR) const override;

   // Concrete
   ////////////
   // 

   Float64 GetConcreteStrength() const;
   Float64 GetReleaseStrength() const;
   Float64 GetReleaseStrength(ConcStrengthResultType* pStrengthResult) const;
   bool DoesReleaseRequireAdditionalRebar() const;

   Float64 GetMinimumReleaseStrength() const;
   Float64 GetMaximumReleaseStrength() const;
   Float64 GetMinimumConcreteStrength() const;
   Float64 GetMaximumConcreteStrength() const;

   bool UpdateConcreteStrength(Float64 fcRequired, const StressCheckTask& task,pgsTypes::StressLocation StressLocation);
   bool UpdateReleaseStrength(Float64 fciRequired,ConcStrengthResultType strengthResult, const StressCheckTask& task,pgsTypes::StressLocation StressLocation);
   bool Bump500(const StressCheckTask& task,pgsTypes::StressLocation stressLocation);
   bool UpdateConcreteStrengthForShear(Float64 fcRequired,IntervalIndexType intervalIdx,pgsTypes::LimitState limitState);

   ConcStrengthResultType ComputeRequiredConcreteStrength(Float64 fControl, const StressCheckTask& task,Float64* pfc) const;

   // "A"
   bool IsDesignSlabOffset() const; 
   void SetSlabOffset(pgsTypes::MemberEndType end,Float64 offset);
   Float64 GetSlabOffset(pgsTypes::MemberEndType end) const; 

   // minimum A for design
   void SetMinimumSlabOffset(Float64 offset);
   Float64 GetMinimumSlabOffset() const; // "A" dimension

   Float64 GetAbsoluteMinimumSlabOffset() const; // based on girder library entry

   bool IsDesignExcessCamber() const;
   Float64 GetAssumedExcessCamberTolerance() const;

   void SetAssumedExcessCamber(Float64 camber);
   Float64 GetAssumedExcessCamber() const;

   // Lifting and hauling
   void SetLiftingLocations(Float64 left,Float64 right);
   Float64 GetLeftLiftingLocation() const;
   Float64 GetRightLiftingLocation() const;

   void SetTruckSupportLocations(Float64 left,Float64 right);
   Float64 GetLeadingOverhang() const;
   Float64 GetTrailingOverhang() const;
   
   void SetHaulTruck(LPCTSTR lpszHaulTruck);
   LPCTSTR GetHaulTruck() const;

   void SetOutcome(pgsSegmentDesignArtifact::Outcome outcome);

   pgsSegmentDesignArtifact::ConcreteStrengthDesignState GetReleaseConcreteDesignState() const;
   pgsSegmentDesignArtifact::ConcreteStrengthDesignState GetFinalConcreteDesignState() const;

   // GROUP: INQUIRY
   void DumpDesignParameters() const;

protected:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   // GROUP: ACCESS
   // GROUP: INQUIRY

   // GROUP: DATA MEMBERS

   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS

   // GROUP: ACCESS
   // GROUP: INQUIRY


private:
   // updates jacking forces with current design information
   void UpdateJackingForces() const;

   // move strands at ends and hp's to highest and lowest possible positions, respectfully.
   bool ResetHarpedStrandConfiguration();

   // make sure harped strands don't go out of allowable bounds when adding strands
   bool KeepHarpedStrandsInBounds();

   // compute min strands until eccentricity is non-negative
   void ComputeMinStrands();


   IBroker* m_pBroker;
   StatusGroupIDType m_StatusGroupID;

   Float64 m_ConcreteAccuracy; // 100 PSI

   pgsSegmentDesignArtifact* m_pArtifact;
   CSegmentKey m_SegmentKey;
   arDesignOptions m_DesignOptions;

   const GirderLibraryEntry* m_pGirderEntry;
   std::_tstring m_GirderEntryName;


   mutable GDRCONFIG m_CachedConfig;
   mutable bool m_bConfigDirty;

   arDesignStrandFillType m_StrandFillType;
   Float64                m_HarpedRatio;

   mutable StrandIndexType        m_MinPermanentStrands;
   StrandIndexType        m_MinTempStrands;

   bool m_bIsDesignSlabOffset;
   Float64                m_MinSlabOffset;
   Float64                m_AbsoluteMinimumSlabOffset;

   Float64 m_HgStart;
   Float64 m_HgHp1;
   Float64 m_HgHp2;
   Float64 m_HgEnd;

   // values cached for performance
   std::array<Float64, 3> m_aps; // nominal area of single strand (use pgsTypes::StrandType enum)
   Float64 m_SegmentLength;
   Float64 m_SpanLength;
   Float64 m_StartConnectionLength;
   std::array<Float64, 3> m_XFerLength;

   // mid-zone locations
   Float64 m_lftMz;
   Float64 m_rgtMz;

   // Are we designing assumed excess camber?
   bool m_bIsDesignExcessCamber;
   Float64 m_AssumedExcessCamberTolerance;

   // Points of interest to be used for design
   mutable pgsPoiMgr m_PoiMgr;

   // Tool for dealing with raised straight strand design - only used if this is the design type
   std::shared_ptr<pgsRaisedStraightStrandDesignTool> m_pRaisedStraightStrandDesignTool;

   // Classes to store information on what controlled concrete strength or number of strands
   // and to control when to set values. 
   struct DesignState
   {
      Float64                  m_Strength;   // concrete strength (release or final), or number of strands
      StressCheckTask          m_Task; // controlling stress checking task
      pgsTypes::StressLocation m_StressLocation;
      Int16                    m_RepeatCount;

      DesignState():
      m_RepeatCount(0), m_Strength(0.0)
      {;}

      DesignState(Float64 strength, const StressCheckTask& task,  pgsTypes::StressLocation stressLocation):
      m_RepeatCount(0), m_Strength(strength), m_Task(task), m_StressLocation(stressLocation)
      {;}

      bool operator == (const DesignState& rOther) const
      {
         // note that repeat count is not part of operator
         // also note the bIncludeLiveLoad element of m_Task is not part of operator
         // that is why we don't do m_Task == rOther.m_Task
         return m_Strength == rOther.m_Strength  &&
            m_Task.intervalIdx == rOther.m_Task.intervalIdx  &&
            m_Task.stressType == rOther.m_Task.stressType  &&
            m_Task.limitState == rOther.m_Task.limitState  &&
            m_StressLocation == rOther.m_StressLocation;
      }
   };


   struct ConcreteStrengthController
   {
      ConcreteStrengthController()
      {
         Init(0.0,INVALID_INDEX);
      }

      bool WasSet() const {return m_Control!=fciInitial;} // if false, minimum strength controlled


      pgsSegmentDesignArtifact::ConcreteStrengthDesignState::Action ControllingAction() const
      {
         return m_Control==fciSetShear ? pgsSegmentDesignArtifact::ConcreteStrengthDesignState::actShear : 
                                      pgsSegmentDesignArtifact::ConcreteStrengthDesignState::actStress;
      }
      Float64    Strength() const {return m_CurrentState.m_Strength;}
      IntervalIndexType Interval() const {return m_CurrentState.m_Task.intervalIdx;}
      pgsTypes::StressType StressType() const {return m_CurrentState.m_Task.stressType;}
      pgsTypes::LimitState LimitState() const {return m_CurrentState.m_Task.limitState;}
      pgsTypes::StressLocation StressLocation() const {return m_CurrentState.m_StressLocation;}

      void Init(Float64 strength,IntervalIndexType intervalIdx)
      {
         m_Control=fciInitial;
         m_CurrentState.m_Strength = strength;
         m_CurrentState.m_Task.intervalIdx = intervalIdx;
         m_Decreases.clear();
      }

      bool DoUpdate(Float64 strength, const StressCheckTask& task, pgsTypes::StressLocation stressLocation, Float64* pCurrStrength)
      {
         bool retval = false;
         if (m_Control==fciInitial)
         {
            // first time through, just set strength
            m_Control = fciSetOnce;

            retval = !IsEqual(strength,m_CurrentState.m_Strength);
            StoreCurrent(strength, task, stressLocation);

         }
         else if (m_Control==fciSetOnce || m_Control==fciSetDecrease)
         {
            // we have set once before. save current values if we need to update
            if (IsEqual(strength,m_CurrentState.m_Strength))
            {
               // no change, no need to update
               return false;
            }
            else if (m_CurrentState.m_Strength < strength)
            {
               // strength is new high - set it
               StoreCurrent(strength, task, stressLocation);
               retval = true;
            }
            else if ( ConditionsMatchCurrent(task, stressLocation) )
            {
               // Controlling state matches current state. We can potentially store a decrease.
               // Update only if new value is more than 150 psi less than current
               if ( strength < (m_CurrentState.m_Strength-WBFL::Units::ConvertToSysUnits(0.15,WBFL::Units::Measure::KSI)) )
               {
                  // We have a decrease, see if it's been stored before
                  Int16 incr; // note assignment below - a bit tricky
                  if (m_Control==fciSetDecrease && 0 < (incr=ConditionsMatchDecrease(strength, task, stressLocation)) )
                  {
                     // This decrease has been tried before for this limit state, which means that it's probably part of a deadlock.
                     // Try a exponentially higher strength, but not more than current value
                     // NOTE: I first tried a raw 100psi increment and reg014.pgs, reg021.pgs girder B would not converge. 
                     //       Hence, 100,200,400,1600... ~rdp
                     Float64 factor = pow(2.,incr-1);
                     Float64 test_strength = strength + factor * WBFL::Units::ConvertToSysUnits(0.1,WBFL::Units::Measure::KSI);

                     // this ISA decrease, so don't allow an increase
                     test_strength = Min(test_strength, m_CurrentState.m_Strength);

                     // allow 3 decreases only for each design state
                     if (incr < 4 && test_strength < m_CurrentState.m_Strength)
                     {
                        strength = test_strength;
                        retval = true;
                     }
                     else
                     {
                        retval = false; // don't increase
                     }
                  }
                  else
                  {
                     // the first decrease for this limit state. Store it and set state
                     StoreDecrease(strength, task, stressLocation);

                     m_Control = fciSetDecrease; // we have a decrease for comparisons
                     retval = true;
                  }

                  if (retval)
                  {
                     StoreCurrent(strength, task, stressLocation);
                  }
               }
               else
               {
                  retval = false;
               }
            }
            else
            {
               // reduction is not from current limit state. No need to update
               retval = false;
            }
         }
         else if (m_Control==fciSetShear)
         {
            retval = false; // never update if shear strength has previously controlled
         }
         else
         {
            ATLASSERT(false); // bad condition??
         }

         *pCurrStrength = m_CurrentState.m_Strength;
         return retval;
      }

      void DoUpdateForShear(Float64 strength, IntervalIndexType intervalIdx,  pgsTypes::LimitState limitState)
      {
         ATLASSERT(m_Control!=fciSetShear); // this should only ever happen once
         m_Control=fciSetShear;

         m_CurrentState.m_Strength = strength;
         m_CurrentState.m_Task.intervalIdx = intervalIdx;
         m_CurrentState.m_Task.limitState = limitState;
      }

private:
      bool ConditionsMatchCurrent(const StressCheckTask& task, pgsTypes::StressLocation stressLocation)
      {
         return (m_CurrentState.m_Task == task &&
                 m_CurrentState.m_StressLocation == stressLocation);
      }

      Int16 ConditionsMatchDecrease(Float64 strength, const StressCheckTask& task, pgsTypes::StressLocation stressLocation)
      {
         DesignState local( strength, task, stressLocation);
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

      void StoreCurrent(Float64 strength, const StressCheckTask& task, pgsTypes::StressLocation stressLocation)
      {
         m_CurrentState.m_Strength       = strength;
         m_CurrentState.m_Task = task;
         m_CurrentState.m_StressLocation = stressLocation;
      }

      void StoreDecrease(Float64 strength, const StressCheckTask& task, pgsTypes::StressLocation stressLocation)
      {
         // assumption here is that ConditionsMatchDecrease() returned 0 before this call
         DesignState local( strength, task, stressLocation);
         local.m_RepeatCount = 1;
         m_Decreases.push_front(local);
      }

      enum fciControl
      {
         fciInitial,
         fciSetOnce,     // have a current value, but no decreases
         fciSetDecrease, // have current and decreases
         fciSetShear     // shear controlled - we cannot change strength anymore
      };

      fciControl               m_Control; // state we are in

      // currently set values for controlling limit state
      DesignState m_CurrentState;

      // List of all decreases
      typedef std::list<DesignState> DContainer; 
      typedef DContainer::iterator DIterator;

      DContainer m_Decreases;

   };

   ConcreteStrengthController m_FciControl;
   ConcreteStrengthController m_FcControl;

   Float64 m_MinFci;
   Float64 m_MaxFci;
   Float64 m_MinFc;
   Float64 m_MaxFc;

   // store whether release strength required additional rebar
   ConcStrengthResultType m_ReleaseStrengthResult;

   // Eccentricity may not be less than this or Final Service stress will be exceeded in Mid-Zone
   // this is used for refined strand adjustments
   Float64 m_MinimumFinalMzEccentricity; 

   // strand slope and hold down
   bool m_DoDesignForStrandSlope;
   Float64 m_AllowableStrandSlope;
   bool m_DoDesignForHoldDownForce;
   Float64 m_AllowableHoldDownForce;
   Float64 m_HoldDownFriction;
   bool m_bTotalHoldDownForce;

   bool AdjustForStrandSlope();
   bool AdjustForHoldDownForce();

   StrandIndexType ComputeNextNumProportionalStrands(StrandIndexType prevNum, StrandIndexType* ns, StrandIndexType* nh) const;

   bool AdjustStrandsForSlope(Float64 targetSlope, Float64 currentSlope, pgsTypes::MemberEndType endType,StrandIndexType nh, IStrandGeometry* pStrandGeom);

   // Private functions called from Initialize
   ///////////////////////////////////////////
   // compute hold-down and strand slope limits
   void InitHarpedPhysicalBounds(const WBFL::Materials::PsStrand* pstrand);
   // locate mid-zone
   void ComputeMidZoneBoundaries();
   // compute and cache pois
   void ClearHandingAttributes(pgsPointOfInterest& poi);
   void ValidatePointsOfInterest();
   void AddPOI(pgsPointOfInterest& rpoi, Float64 lft_conn, Float64 rgt_conn);

   // Private Debond Design Stuff
   //////////////////////////////
   // set debonding to max in order to compute concrete strengths
   bool MaximizeDebonding();

   // compute possible debond levels for the current span/girder
   void InitDebondData();
   void ComputeDebondLevels(IPretensionForce* pPrestressForce);
   void DumpDebondLevels(Float64 Hg);
   bool SmoothDebondLevelsAtSections(std::vector<DebondLevelType>& rDebondLevelsAtSections) const;
   DebondLevelType GetMinAdjacentDebondLevel(DebondLevelType currLevel, StrandIndexType maxDbsTermAtSection) const;

   // Each debond level represents the constraints needed to debond a given list of strands
   // 
   struct DebondLevel
   {
      std::vector<StrandIndexType> StrandsDebonded;
      StrandIndexType MinTotalStrandsRequired;

      Float64 m_DebondedStrandsCg; // cg of debonded strands

      DebondLevel():
      MinTotalStrandsRequired(0),m_DebondedStrandsCg(-Float64_Max)
      {;}

      void Init(Float64 Hg,IPoint2dCollection* strandLocations);
      // Stress relief from debonding at this level
      Float64 ComputeReliefStress(Float64 pePerStrandFullyBonded, Float64 pePerStrandDebonded, StrandIndexType nperm, StrandIndexType ntemp, Float64 cgtot, Float64 Hg, 
                                  Float64 Yb, Float64 eccX, Float64 Ca, Float64 Cmx, Float64 Cmy, SHARED_LOGFILE LOGFILE) const;
   };

   typedef std::vector<DebondLevel>                DebondLevelCollection;
   typedef DebondLevelCollection::iterator         DebondLevelIterator;
   typedef DebondLevelCollection::reverse_iterator DebondLevelReverseIterator;

   DebondLevelCollection m_DebondLevels;

   // debond section spacings
   SectionIndexType m_NumDebondSections;
   Float64 m_DebondSectionLength;
   
   // limits at sections
   StrandIndexType m_MaxDebondSection10orLess;
   StrandIndexType   m_MaxDebondSection;
   bool m_bCheckMaxFraAtSection;
   Float64 m_MaxPercentDebondSection;

   // maximum debond levels due to physical contrants at any section
   std::vector<DebondLevelType> m_MaxPhysicalDebondLevels;


   Float64 pgsStrandDesignTool::ComputePrestressForcePerStrand(const GDRCONFIG& fullyBondedConfig, const StressDemand& demand, const DebondLevel& lvl, IntervalIndexType interval, IPretensionForce* pPrestressForce) const;
   void GetHandlingDesignPointsOfInterest(const CSegmentKey& segmentKey,Float64 leftOverhang,Float64 rightOverhang,PoiAttributeType poiReference,PoiAttributeType supportAttribute, std::vector<pgsPointOfInterest>* pvPoi, Uint32 mode) const;


private:
	DECLARE_SHARED_LOGFILE;

};

// INLINE METHODS
//

// EXTERNAL REFERENCES
//

#endif // INCLUDED_STRANDDESIGNTOOL_H_
