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

#ifndef INCLUDED_STRANDDESIGNTOOL_H_
#define INCLUDED_STRANDDESIGNTOOL_H_

// SYSTEM INCLUDES
//

// PROJECT INCLUDES
//

#include <IFace\Artifact.h>
#include <IFace\GirderHandlingPointOfInterest.h>
#include <PgsExt\PoiMgr.h>

#include <algorithm>
#include<list>
#include<vector>

// LOCAL INCLUDES
//

typedef Int16 DebondLevelType; 

inline static std::string DumpIntVector(const std::vector<DebondLevelType>& rvec)
{
   std::stringstream os;
   for (std::vector<DebondLevelType>::const_iterator it=rvec.begin(); it!=rvec.end(); it++)
   {
      os<<*it<<", ";
   }

   std::string str(os.str());
   DebondLevelType n = str.size();
   if (0 < n)
      str.erase(n-2,2); // get rid of trailing ", "

   return str;
}

// FORWARD DECLARATIONS
//
interface IBroker;
interface IPrestressForce;

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


COPYRIGHT
   Copyright © 1997-1998
   Washington State Department Of Transportation
   All Rights Reserved

LOG
   rdp : 3.22.2007 : Created file 
*****************************************************************************/

class pgsStrandDesignTool : public IGirderLiftingDesignPointsOfInterest, public IGirderHaulingDesignPointsOfInterest
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
   
   void Initialize(IBroker* pBroker, long statusGroupID, pgsDesignArtifact* pArtifact);

   void InitReleaseStrength(Float64 fci);

   void RestoreDefaults(bool retainProportioning);

   // GROUP: OPERATIONS
   void FillArtifactWithFlexureValues();

   Float64 GetGirderLength() const; // a little utility function to return a commonly used value

   // adding and removing strands
   StrandIndexType GetNumPermanentStrands();
   StrandIndexType GetMaxPermanentStrands();
   StrandIndexType GetNumTotalStrands();
   StrandIndexType GetNh();
   StrandIndexType GetNs();
   StrandIndexType GetNt();

   void SetConcreteAccuracy(Float64 accuracy);
   Float64 GetConcreteAccuracy() const;

   bool SetNumPermanentStrands(StrandIndexType num);
   bool SetNumStraightHarped(StrandIndexType ns, StrandIndexType nh);
   bool SetNumTempStrands(StrandIndexType num);
   bool SwapStraightForHarped();
   bool AddStrands();
   bool AddTempStrands();

   StrandIndexType GetNextNumPermanentStrands(StrandIndexType prevNum);
   StrandIndexType GetPreviousNumPermanentStrands(StrandIndexType nextNum); 
   bool IsValidNumPermanentStrands(StrandIndexType num);

   void SetMinimumPermanentStrands(StrandIndexType num);
   StrandIndexType GetMinimumPermanentStrands() const;

   StrandIndexType GuessInitialStrands();
   arDesignStrandFillType GetOriginalStrandFillType() const;

   // Prestress forces
   Float64 GetPjackStraightStrands() const;
   Float64 GetPjackTempStrands() const;
   Float64 GetPjackHarpedStrands() const;

   // returns the prestress force at lifting
   Float64 GetPrestressForceAtLifting(const GDRCONFIG& guess,const pgsPointOfInterest& poi);

   Float64 GetPrestressForceMz(pgsTypes::Stage stage,const pgsPointOfInterest& poi);

   // if Uint32_Max, cannot handle force
   StrandIndexType ComputePermanentStrandsRequiredForPrestressForce(const pgsPointOfInterest& poi,Float64 force);

   Float64 ComputeEccentricity(const pgsPointOfInterest& poi, pgsTypes::Stage eccStage);

   Float64 GetTransferLength(pgsTypes::StrandType strandType) const;

   // Set end zone strand configuration to maximize debonding or harping depending on design type
   bool ResetEndZoneStrandConfig();

   // Harped Strand Design
   ///////////////////////
   // Harp adjustments
   Float64 GetMinimumFinalMzEccentricity(); 
   void SetMinimumFinalMzEccentricity(Float64 ecc); 

   // offsets are measured from original strand grid location from library
   Float64 GetHarpStrandOffsetEnd() const;
   Float64 GetHarpStrandOffsetHp() const;

   void SetHarpStrandOffsetEnd(Float64 off);
   void SetHarpStrandOffsetHp(Float64 off);

   void GetEndOffsetBounds(Float64* pLower, Float64* pUpper) const;
   void GetHpOffsetBounds(Float64* pLower, Float64* pUpper) const;

   Float64 ComputeEndOffsetForEccentricity(const pgsPointOfInterest& poi, Float64 ecc);
   bool ComputeMinHarpedForEzEccentricity(const pgsPointOfInterest& poi, Float64 ecc, pgsTypes::Stage eccStage, StrandIndexType* pNs, StrandIndexType* pNh);

   bool ComputeAddHarpedForMzReleaseEccentricity(const pgsPointOfInterest& poi, Float64 ecc, Float64 minEcc, StrandIndexType* pNs, StrandIndexType* pNh);

   Float64 ComputeHpOffsetForEccentricity(const pgsPointOfInterest& poi, Float64 ecc,pgsTypes::Stage eccStage);

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
      Float64 m_TopStress;
      Float64 m_BottomStress;
   };

   std::vector<DebondLevelType> ComputeDebondsForDemand(const std::vector<StressDemand>& demands, StrandIndexType nss, Float64 psForcePerStrand, Float64 allowTens, Float64 allowComp);


   // section zero is at GetDebondSectionLength from end of girder
   Float64 GetDebondSectionLocation(SectionIndexType sectionIdx, DebondEndType end) const;

   // Get Indices of Section just inboard and outboard of location and distance from outboard section to location
   void GetDebondSectionForLocation(Float64 location, SectionIndexType* pOutBoardSectionIdx, SectionIndexType* pInBoardSectionIdx, Float64* pOutToInDistance); 

   // Given an amount of stress demand, return the minimum debond level to relieve the stress
   void GetDebondLevelForTopTension(Float64 psForcePerStrand, StrandIndexType nss, Float64 tensDemand, Float64 outboardDistance,
                                    Float64 Yb, Float64 Ag, Float64 St,
                                    DebondLevelType* pOutboardLevel, DebondLevelType* pInboardLevel);

   void GetDebondLevelForBottomCompression(Float64 psForcePerStrand, StrandIndexType nss, Float64 tensDemand, Float64 outboardDistance,
                                           Float64 Yb, Float64 Ag, Float64 Sb,
                                            DebondLevelType* pOutboardLevel, DebondLevelType* pInboardLevel);

   // Debonding levels are integer values used to quantify the amount of strands debonded at
   // a given section.
   // Get max level based on given number of strands, and number of sections outboard of desired section
   DebondLevelType GetMaxDebondLevel(StrandIndexType numStrands, SectionIndexType numLeadingSections );

   // returns the maximum physically debonding the girder can handle based on the current number of strands
   std::vector<DebondLevelType> GetMaxPhysicalDebonding();

   // take a raw debond level layout, smooth debonding if necessary, and check that it is 
   // within maximum limits. Return an empty vector if we fail
   void RefineDebondLevels( std::vector<DebondLevelType>& rDebondLevelsAtSections );

   // workhorse function to go from debond levels to set actual debond layout. 
   // Returns false if layout cannot be created
   bool LayoutDebonding(const std::vector<DebondLevelType>& rDebondLevelsAtSections);


   // ACCESS
   //////////
   GDRCONFIG GetGirderConfiguration();

   arFlexuralDesignType GetFlexuralDesignType() const;

   SpanIndexType GetSpan() const
   {
      return m_Span;
   }

   GirderIndexType GetGirder() const
   {
      return m_Girder;
   }

   // left and right ends of mid-zone. Measured from girder start
   void GetMidZoneBoundaries(Float64* leftEnd, Float64* rightEnd); 

   // POI's for design
   std::vector<pgsPointOfInterest> GetDesignPoi(pgsTypes::Stage stage,PoiAttributeType attrib);
   std::vector<pgsPointOfInterest> GetDesignPoiEndZone(pgsTypes::Stage stage,PoiAttributeType attrib);

   pgsPointOfInterest GetDebondSamplingPOI(pgsTypes::Stage stage) const;
   // interface IGirderLiftingDesignPointsOfInterest
   // locations of points of interest
   virtual std::vector<pgsPointOfInterest> GetLiftingDesignPointsOfInterest(SpanIndexType span,GirderIndexType gdr,Float64 overhang,PoiAttributeType attrib,Uint32 mode);

   //IGirderLiftingDesignPointsOfInterest
   virtual std::vector<pgsPointOfInterest> GetHaulingDesignPointsOfInterest(SpanIndexType span,GirderIndexType gdr,Float64 leftOverhang,Float64 rightOverhang,PoiAttributeType attrib,Uint32 mode = POIFIND_AND);

   // Concrete
   ////////////
   // 

   Float64 GetConcreteStrength() const;
   Float64 GetReleaseStrength() const;
   Float64 GetReleaseStrength(ConcStrengthResultType* pStrengthResult) const;

   Float64 GetMinimumReleaseStrength() const;
   Float64 GetMaximumReleaseStrength() const;
   Float64 GetMinimumConcreteStrength() const;
   Float64 GetMaximumConcreteStrength() const;

   bool UpdateConcreteStrength(Float64 fcRequired,pgsTypes::Stage stage,pgsTypes::LimitState limitState,pgsTypes::StressType stressType,pgsTypes::StressLocation StressLocation);
   bool UpdateReleaseStrength(Float64 fciRequired,ConcStrengthResultType strengthResult,pgsTypes::Stage stage,pgsTypes::LimitState limitState,pgsTypes::StressType stressType,pgsTypes::StressLocation StressLocation);
   bool Bump500(pgsTypes::Stage stage,pgsTypes::LimitState limitState,pgsTypes::StressType stressType,pgsTypes::StressLocation stressLocation);

   ConcStrengthResultType ComputeRequiredConcreteStrength(double fControl,pgsTypes::Stage stage,pgsTypes::LimitState ls,pgsTypes::StressType stressType,double* pfc);

   // "A"
   void SetSlabOffset(pgsTypes::MemberEndType end,Float64 offset);
   Float64 GetSlabOffset(pgsTypes::MemberEndType end) const; 

   // minimum A for design
   void SetMinimumSlabOffset(Float64 offset);
   Float64 GetMinimumSlabOffset() const; // "A" dimension

   // Lifting and hauling
   void SetLiftingLocations(Float64 left,Float64 right);
   Float64 GetLeftLiftingLocation() const;
   Float64 GetRightLiftingLocation() const;

   void SetTruckSupportLocations(Float64 left,Float64 right);
   Float64 GetLeadingOverhang() const;
   Float64 GetTrailingOverhang() const;

   void SetOutcome(pgsDesignArtifact::Outcome outcome);

   pgsDesignArtifact::ConcreteStrengthDesignState GetReleaseConcreteDesignState() const;
   pgsDesignArtifact::ConcreteStrengthDesignState GetFinalConcreteDesignState() const;

   // GROUP: INQUIRY
   void DumpDesignParameters();

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
   void UpdateJackingForces();

   // move strands at ends and hp's to highest and lowest possible positions, respectfully.
   bool ResetHarpedStrandConfiguration();

   // make sure harped strands don't go out of allowable bounds when adding strands
   bool KeepHarpedStrandsInBounds();

   // compute min strands until eccentricity is non-negative
   void ComputeMinStrands();


   IBroker* m_pBroker;
   long m_StatusGroupID;

   Float64 m_ConcreteAccuracy; // 100 PSI

   pgsDesignArtifact* m_pArtifact;
   GirderIndexType m_Girder;
   SpanIndexType m_Span;
   arDesignOptions m_DesignOptions;

   arDesignStrandFillType m_StrandFillType;
   Float64                m_HarpedRatio;

   StrandIndexType        m_MinPermanentStrands;
   StrandIndexType        m_MinTempStrands;
   Float64                m_MinSlabOffset;

   // values cached for performance
   Float64 m_Aps[3]; // area of straight, harped, and temporary strand (use pgsTypes::StrandType enum)
   Float64 m_GirderLength;
   Float64 m_SpanLength;
   Float64 m_StartConnectionLength;
   Float64 m_XFerLength[3];

   // mid-zone locations
   Float64 m_lftMz;
   Float64 m_rgtMz;

   // Points of interest to be used for design
   pgsPoiMgr m_PoiMgr;

   // Classes to store information on what controlled release strength
   // and to control when to set values. 
   struct DesignState
   {
      Float64                  m_Strength;
      pgsTypes::Stage          m_Stage;      // controlling stage
      pgsTypes::StressType     m_StressType; // stress type (tension or compression) 
      pgsTypes::LimitState     m_LimitState; // 
      pgsTypes::StressLocation m_StressLocation;
      Int16                    m_RepeatCount;

      DesignState():
      m_RepeatCount(0), m_Strength(0.0)
      {;}

      DesignState(Float64 strength, pgsTypes::Stage stage, 
                       pgsTypes::StressType stressType, pgsTypes::LimitState limitState,
                       pgsTypes::StressLocation stressLocation):
      m_RepeatCount(0), m_Strength(strength), m_Stage(stage), m_StressType(stressType),
      m_LimitState(limitState), m_StressLocation(stressLocation)
      {;}

      bool operator == (const DesignState& rOther) const
      {
         // note that repeat count is not part of operator
         return m_Strength == rOther.m_Strength  &&
                m_Stage == rOther.m_Stage  &&
                m_StressType == rOther.m_StressType  &&
                m_LimitState == rOther.m_LimitState  &&
                m_StressLocation == rOther.m_StressLocation;
      }
   };


   struct ConcreteStrengthController
   {
      ConcreteStrengthController()
      {
         Init(0.0);
      }

      bool WasSet() const {return m_Control!=fciInitial;} // if false, minimum strength controlled

      Float64    Strength() const {return m_CurrentState.m_Strength;}
      pgsTypes::Stage Stage() const {return m_CurrentState.m_Stage;}
      pgsTypes::StressType StressType() const {return m_CurrentState.m_StressType;}
      pgsTypes::LimitState LimitState() const {return m_CurrentState.m_LimitState;}
      pgsTypes::StressLocation StressLocation() const {return m_CurrentState.m_StressLocation;}

      void Init(Float64 strength)
      {
         m_Control=fciInitial;
         m_CurrentState.m_Strength = strength;
         m_Decreases.clear();
      }

      bool DoUpdate(Float64 strength, pgsTypes::Stage stage, 
                       pgsTypes::StressType stressType, pgsTypes::LimitState limitState,
                       pgsTypes::StressLocation stressLocation, Float64* pCurrStrength)
      {
         bool retval = false;
         if (m_Control==fciInitial)
         {
            // first time through, just set strength
            m_Control = fciSetOnce;

            retval = !IsEqual(strength,m_CurrentState.m_Strength);
            StoreCurrent(strength, stage, stressType, limitState, stressLocation);

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
               StoreCurrent(strength, stage, stressType, limitState, stressLocation);
               retval = true;
            }
            else if ( ConditionsMatchCurrent(stage, stressType, limitState, stressLocation) )
            {
               // Controlling state matches current state. We can potentially store a decrease.
               // Update only if new value is more than 150 psi less than current
               if ( (m_CurrentState.m_Strength-::ConvertToSysUnits(0.15,unitMeasure::KSI)) > strength)
               {
                  // We have a decrease, see if it's been stored before
                  Int16 incr; // note assignment below - a bit tricky
                  if (m_Control==fciSetDecrease && 0 < (incr=ConditionsMatchDecrease(strength, stage, stressType, limitState, stressLocation)) )
                  {
                     // This decrease has been tried before for this limit state, which means that it's probably part of a deadlock.
                     // Try a exponentially higher strength, but not more than current value
                     // NOTE: I first tried a raw 100psi increment and reg014.pgs, reg021.pgs girder B would not converge. 
                     //       Hence, 100,200,400,1600... ~rdp
                     Float64 factor = pow(2.,incr-1);
                     Float64 test_strength = strength + factor * ::ConvertToSysUnits(0.1,unitMeasure::KSI);

                     // this ISA decrease, so don't allow an increase
                     test_strength = min(test_strength, m_CurrentState.m_Strength);

                     // allow 3 decreases only for each design state
                     if (incr<4 && test_strength<m_CurrentState.m_Strength)
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
                     StoreDecrease(strength, stage, stressType, limitState, stressLocation);

                     m_Control = fciSetDecrease; // we have a decrease for comparisons
                     retval = true;
                  }

                  if (retval)
                  {
                     StoreCurrent(strength, stage, stressType, limitState, stressLocation);
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
         else
         {
            ATLASSERT(0); // bad condition??
         }

         *pCurrStrength = m_CurrentState.m_Strength;
         return retval;
      }


private:
      bool ConditionsMatchCurrent(pgsTypes::Stage stage, pgsTypes::StressType stressType, pgsTypes::LimitState limitState, pgsTypes::StressLocation stressLocation)
      {
         return (m_CurrentState.m_Stage          == stage      &&
                 m_CurrentState.m_LimitState     == limitState &&
                 m_CurrentState.m_StressType     == stressType &&
                 m_CurrentState.m_StressLocation == stressLocation);
      }

      Int16 ConditionsMatchDecrease(Float64 strength, pgsTypes::Stage stage, pgsTypes::StressType stressType, pgsTypes::LimitState limitState, pgsTypes::StressLocation stressLocation)
      {
         DesignState local( strength, stage, stressType, limitState, stressLocation);
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

      void StoreCurrent(Float64 strength, pgsTypes::Stage stage, pgsTypes::StressType stressType, pgsTypes::LimitState limitState, pgsTypes::StressLocation stressLocation)
      {
         m_CurrentState.m_Strength       = strength;
         m_CurrentState.m_Stage          = stage;
         m_CurrentState.m_LimitState     = limitState;
         m_CurrentState.m_StressType     = stressType;
         m_CurrentState.m_StressLocation = stressLocation;
      }

      void StoreDecrease(Float64 strength, pgsTypes::Stage stage, pgsTypes::StressType stressType, pgsTypes::LimitState limitState, pgsTypes::StressLocation stressLocation)
      {
         // assumption here is that ConditionsMatchDecrease() returned 0 before this call
         DesignState local( strength, stage, stressType, limitState, stressLocation);
         local.m_RepeatCount = 1;
         m_Decreases.push_front(local);
      }

      enum fciControl
      {
         fciInitial,
         fciSetOnce,     // have a current value, but no decreases
         fciSetDecrease  // have current and decreases
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

   bool AdjustForStrandSlope();
   bool AdjustForHoldDownForce();

   StrandIndexType ComputeNextNumProportionalStrands(StrandIndexType prevNum, StrandIndexType* ns, StrandIndexType* nh);

   bool AdjustStrandsForSlope(Float64 targetSlope, Float64 currentSlope, StrandIndexType nh, IStrandGeometry* pStrandGeom);

   // Private functions called from Initialize
   ///////////////////////////////////////////
   // compute hold-down and strand slope limits
   void InitHarpedPhysicalBounds(const matPsStrand* pstrand);
   // locate mid-zone
   void ComputeMidZoneBoundaries();
   // compute and cache pois
   void ValidatePointsOfInterest();
   void AddPOI(pgsPointOfInterest& rpoi, Float64 lft_conn, Float64 rgt_conn,PoiAttributeType attribute);

   // Private Debond Design Stuff
   //////////////////////////////
   // set debonding to max in order to compute concrete strengths
   bool MaximizeDebonding();

   // compute possible debond levels for the current span/girder
   void InitDebondData();
   void ComputeDebondLevels(IPrestressForce* pPrestressForce);
   void DumpDebondLevels();
   bool SmoothDebondLevelsAtSections(std::vector<DebondLevelType>& rDebondLevelsAtSections);
   DebondLevelType GetMinAdjacentDebondLevel(DebondLevelType currLevel, StrandIndexType maxDbsTermAtSection);

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

      void Init(IPoint2dCollection* strandLocations);
      // Stress relief from debonding at this level
      Float64 ComputeReliefStress(Float64 psForcePerStrand,Float64 Yb, Float64 Ag, Float64 S) const;
   };

   typedef std::vector<DebondLevel>                DebondLevelCollection;
   typedef DebondLevelCollection::iterator         DebondLevelIterator;
   typedef DebondLevelCollection::reverse_iterator DebondLevelReverseIterator;

   DebondLevelCollection m_DebondLevels;

   // debond section spacings
   SectionIndexType m_NumDebondSections;
   Float64 m_DebondSectionLength;
   // limits at sections
   Float64 m_MaxPercentDebondSection;
   StrandIndexType   m_MaxDebondSection;

   // maximum debond levels due to physical contrants at any section
   std::vector<DebondLevelType> m_MaxPhysicalDebondLevels;



private:
	DECLARE_SHARED_LOGFILE;

};

// INLINE METHODS
//

// EXTERNAL REFERENCES
//

#endif // INCLUDED_STRANDDESIGNTOOL_H_
