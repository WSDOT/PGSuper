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
#pragma once

// SYSTEM INCLUDES
//

// PROJECT INCLUDES
//

#include <IFace\Artifact.h>
#include <IFace\PointOfInterest.h>
#include <PgsExt\PgsExt.h>
#include <psgLib\GirderLibraryEntry.h>

// LOCAL INCLUDES
//

// FORWARD DECLARATIONS
//
interface IBroker;
class StirrupZoneIter;


class PoiIsOutsideOfBearings
{
public:
   PoiIsOutsideOfBearings(const CSegmentKey& segmentKey,Float64 startBrg,Float64 endBrg) 
   {
      m_SegmentKey = segmentKey;
      m_StartBrg = startBrg; 
      m_EndBrg = endBrg;
   }

   bool operator()(const pgsPointOfInterest& poi) const
   {
      // returning true causes the poi to be excluded from the container
      // return false for those poi that match the search criteria... return false for the poi we want to keep
      if ( poi.GetSegmentKey() != m_SegmentKey )
      {
         return false; // want to keep
      }

      if ( poi.HasAttribute(POI_CLOSURE) )
      {
         return false; // want to keep
      }

      Float64 Xpoi = poi.GetDistFromStart();
      if ( 
           (IsLT(Xpoi,m_StartBrg) || IsLT(m_EndBrg,Xpoi)) ||
           (IsEqual(Xpoi,m_StartBrg) && poi.HasAttribute(POI_SPAN | POI_CANTILEVER)) || // POI is at the left bearing but it is on the cantilever side
           (IsEqual(Xpoi,m_EndBrg)   && poi.HasAttribute(POI_SPAN | POI_CANTILEVER))    // POI is at the right bearing but it is on the cantilever side
         )
      {
         return true; // don't want to keep
      }

      return false; // want to keep
   }

private:
   CSegmentKey m_SegmentKey;
   Float64 m_StartBrg;
   Float64 m_EndBrg;
};


// MISCELLANEOUS
// Virtual class for checking long reinf for shear - we need this from Designer2,
// but don't need the whole enchilada
class LongReinfShearChecker
{
public:
   virtual void CheckLongReinfShear( const pgsPointOfInterest& poi, 
                                     IntervalIndexType intervalIdx,
                                     pgsTypes::LimitState ls,
                                     const SHEARCAPACITYDETAILS& scd,
                                     const GDRCONFIG* pConfig,
                                     pgsLongReinfShearArtifact* pArtifact ) const = 0;
};

/*****************************************************************************
CLASS 
   pgsShearDesignTool

   Encapsulates the design strategy


DESCRIPTION
   Encapsulates the design strategy

LOG
   rdp : 3.22.2007 : Created file 
*****************************************************************************/

class pgsShearDesignTool 
{
public:

   enum ShearDesignOutcome
   {
      sdSuccess,
      sdFail,
      sdRestartWithAdditionalLongRebar, // This and next are caused by long reinf for shear failure
      sdRestartWithAdditionalStrands,
      sdDesignFailedFromShearStress  // Shear stress exceeded 0.18f'c. Tool will compute required f'c
   };

   // GROUP: LIFECYCLE

   //------------------------------------------------------------------------
   // Default constructor
   pgsShearDesignTool(SHARED_LOGFILE lf);
   
   void Initialize(IBroker* pBroker, const LongReinfShearChecker* pLongShearChecker, 
                   StatusGroupIDType statusGroupID, pgsSegmentDesignArtifact* pArtifact, 
                   Float64 startConfinementZl, Float64 endConfinementZl,
                   bool bPermit, bool bDesignFromScratch);

   // Start a new design
   void ResetDesign(const PoiList& pois);

   // Primary design function
   ShearDesignOutcome DesignStirrups(Float64 leftCSS, Float64 rightCSS) const;

   // Area of steel required for long reinf shear if outcome requires design restart
   // Note that this can be area of strands, or area of #5 bars adjusted for development
   Float64 GetRequiredAsForLongReinfShear() const;

   // F'c required if shear stress fails - if sdDesignFailedFromShearStress is returned
   Float64 GetFcRequiredForShearStress() const;

   // Pois for spec check and design
   const PoiList& GetDesignPoi() const;

   PoiList GetCriticalSections() const;

   // Get our check artifact to hand off to spec check routine
   pgsStirrupCheckArtifact* GetStirrupCheckArtifact();

private:
   // Stirrup design from scratch
   bool LayoutPrimaryStirrupZones() const;

   // Design by Modifying existing stirrup layout 
   bool ModifyPreExistingStirrupDesign() const;
   bool DesignPreExistingStirrups(StirrupZoneIter& rIter, Float64 locCSS,  WBFL::Materials::Rebar::Grade barGrade, WBFL::Materials::Rebar::Type barType, lrfdRebarPool* pool) const;
   void ExpandStirrupZoneLengths(CShearData2::ShearZoneVec& ShearZones) const;

   // Design additional horizontal shear bars if needed
   bool DetailHorizontalInterfaceShear() const;

   // Design additional splitting reinforcement if needed
   bool DetailAdditionalSplitting() const;

   // Design additional confinement reinforcement if needed
   bool DetailAdditionalConfinement() const;

   // Check and design longitudinal reinforcement for shear
   ShearDesignOutcome DesignLongReinfShear() const;

   // GROUP: OPERATIONS

   // ACCESS
   //////////

   GDRCONFIG GetSegmentConfiguration() const;

   const CSegmentKey& GetSegmentKey() const;

   // Design Control Values
   bool DoDesignForConfinement() const;
   bool DoDesignForSplitting() const;
   bool DoDesignFromScratch() const;

   IndexType GetNumStirrupSizeBarCombos() const;
   void GetStirrupSizeBarCombo(IndexType index, WBFL::Materials::Rebar::Size* pSize, Float64* pNLegs, Float64* pAv) const;

   // Available bar spacings for design
   IndexType GetNumAvailableBarSpacings() const;
   Float64 GetAvailableBarSpacing(IndexType index) const;
   void GetMinZoneLength(Uint32* pSpacings, Float64* pLength) const;
public:
   bool GetIsCompositeDeck() const;
   bool GetIsTopFlangeRoughened() const;
   bool GetExtendBarsIntoDeck() const;
   bool GetDoPrimaryBarsProvideSplittingCapacity() const;
   bool GetBarsActAsConfinement() const;
   GirderLibraryEntry::LongShearCapacityIncreaseMethod GetLongShearCapacityIncreaseMethod() const;
   // We can run into a case where LRS fails and cannot easily be remedied by adding long steel. Another
   // way to deal with this is to tighten stirrup spacing at the ends of the girder
   void SetLongShearCapacityRequiresStirrupTightening(bool req);
   bool GetLongShearCapacityRequiresStirrupTightening() const;
private:
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

   IBroker* m_pBroker;
   StatusGroupIDType m_StatusGroupID;

   pgsSegmentDesignArtifact* m_pArtifact;
   mutable CShearData2 m_ShearData; // in-progress shear design data'

   const LongReinfShearChecker* m_pLongReinfShearChecker;

   CSegmentKey m_SegmentKey;

   bool m_bIsPermit;

   Float64 m_SegmentLength;
   Float64 m_SpanLength;
   Float64 m_StartConnectionLength;
   Float64 m_EndConnectionLength;
   Float64 m_LeftFosLocation; // face of support from start of girder
   Float64 m_RightFosLocation;

   bool m_bIsCurrentStirrupLayoutSymmetrical;

   bool m_bDoDesignForConfinement;
   bool m_bDoDesignForSplitting;
   bool m_bDoDesignFromScratch;

   pgsTypes::ConcreteType m_ConcreteType;

   // Confinement zone lengths at both ends of girder
   Float64 m_StartConfinementZl;
   Float64 m_EndConfinementZl;

   // locations of critical sections for shear
   mutable Float64 m_LeftCSS;
   mutable Float64 m_RightCSS;

   // Splitting zone lengths at both ends of girder
   Float64 m_StartSplittingZl;
   Float64 m_EndSplittingZl;

   // Area of steel required if longitudinal reinf for shear design kicks in
   mutable Float64 m_LongReinfShearAs;

   // f'c required for shear stress
   mutable Float64 m_RequiredFcForShearStress;

   // Shear design parameters
   struct BarLegCombo
   {
      WBFL::Materials::Rebar::Size m_Size;
      Float64        m_Legs;
      Float64        m_Av; // area of size*legs
   };
   typedef std::vector<BarLegCombo> BarLegCollection;
   typedef BarLegCollection::iterator BarLegCollectionIterator;

   BarLegCollection m_BarLegCollection;

   std::vector<Float64> m_AvailableBarSpacings;

   Float64 m_SMaxMax; // max possible bar spacing for design

   Float64 m_MaxSpacingChangeInZone;
   Float64 m_MaxShearCapacityChangeInZone;
   Uint32 m_MinZoneLengthSpacings;
   Float64 m_MinZoneLengthLength;
   bool m_IsCompositeDeck;
   bool m_IsTopFlangeRoughened;
   bool m_DoExtendBarsIntoDeck;
   bool m_DoBarsProvideSplittingCapacity;
   bool m_DoBarsActAsConfinement;
   GirderLibraryEntry::LongShearCapacityIncreaseMethod m_LongShearCapacityIncreaseMethod;
   bool m_bIsLongShearCapacityIncreaseMethodProblem; // can have issue in spec library where rebar is solution, but can't be used
   bool m_bLongShearCapacityRequiresStirrupTightening; // LRS design calls for tightening stirrup spacing

   // Transient values for each design iteration
   // -------------------------------------------
   mutable pgsPoiMgr m_PoiMgr;

   // Points of interest to be used for design
   // Note that the following block of vectors are the same size, and in the same order as, 
   // m_DesignPois. This allows quick storage and retreival in poi order
   // -------------------------------------------
   mutable PoiList m_DesignPois; // sorted along girder

   // Two ways to store same vert av/s demand data here, by POI, and by X location
   mutable std::vector<std::pair<Float64,bool>> m_VertShearAvsDemandAtPois;    // Avs demand by POI, bool indicates if stirrups are required regardless of Avs = 0
   mutable WBFL::Math::PiecewiseFunction m_VertShearAvsDemandAtX; // Avs demand at locations along girder

   // Two ways to store same Horiz av/s demand data here, by POI, and by X location
   mutable std::vector<std::pair<Float64,bool>> m_HorizShearAvsDemandAtPois;    // Avs demand by POI
   mutable WBFL::Math::PiecewiseFunction m_HorizShearAvsDemandAtX; // Avs demand at locations along girder

   // Store entire stirrup check artifact
   pgsStirrupCheckArtifact m_StirrupCheckArtifact;

   // PoiIdx in the following functions refer to indexes into m_DesignPois
   // Max stirrup spacing for a given location
   Float64 ComputeMaxStirrupSpacing(IndexType PoiIdx) const;

   Float64 ComputeMaxStirrupSpacing(Float64 location) const;

   // Get min stirrup spacing for given bar size
   Float64 GetMinStirrupSpacing(WBFL::Materials::Rebar::Size size) const;

   // Get bar size - nlegs - spacing for av/s demand
   bool GetBarSizeSpacingForAvs(Float64 avsDemand, Float64 maxSpacing, WBFL::Materials::Rebar::Size* pSize, Float64* pNLegs, Float64* pAv, Float64* pSpacing) const;

   // Get bar size - nlegs - spacing for av/s demand - For a particular bar size
   bool GetBarSpacingForAvs(Float64 avsDemand, Float64 maxSpacing, WBFL::Materials::Rebar::Size Size, Float64 Av, Float64* pSpacing) const;

   // Get next (or same) available bar size for a given min bar size
   bool GetMinAvailableBarSize(WBFL::Materials::Rebar::Size minSize, WBFL::Materials::Rebar::Grade barGrade, WBFL::Materials::Rebar::Type barType, lrfdRebarPool* pool, WBFL::Materials::Rebar::Size* pSize) const;

   // Av/S demand at poi
   Float64 GetVerticalAvsDemand(IndexType PoiIdx) const;
   bool GetAreVerticalStirrupsRequired(IndexType PoiIdx) const;

   // Av/S demand at locations along girder
   Float64 GetVerticalAvsDemand(Float64 distFromStart) const;

   // Horiz Av/S demand at poi
   Float64 GetHorizontalAvsDemand(IndexType PoiIdx) const;

   // Horiz Av/S demand at locations along girder
   Float64 GetHorizontalAvsDemand(Float64 distFromStart) const;

   // Av/S demand in a range - useful for stirrup zones
   Float64 GetVerticalAvsMaxInRange(Float64 leftBound, Float64 rightBound) const;

   // Horiz Av/S demand in a range - useful for stirrup zones
   Float64 GetHorizontalAvsMaxInRange(Float64 leftBound, Float64 rightBound) const;

   // Max stirrup spacing in next zone
   Float64 ComputeMaxNextSpacing(Float64 currentSpacing) const;

   // Find a design stirrup spacing that works for input spacing
   Float64 GetDesignSpacingFromList(Float64 spacing) const;

   // determine if a location is in confinement zone
   bool IsLocationInConfinementZone(Float64 distFromStart) const;

   // utility to get closeby poi for a location
   IndexType GetPoiIdxForLocation(Float64 location) const;

   // Av/S required for splitting
   Float64 GetAvsReqdForSplitting() const;


   // Private functions called at initialization
   ///////////////////////////////////////////
   // compute and cache pois
   void ValidatePointsOfInterest(const PoiList& pois) const;

   ShearDesignOutcome Validate() const;
   // Compute and cache Av/S demand
   ShearDesignOutcome ValidateVerticalAvsDemand() const;
   void ValidateHorizontalAvsDemand() const;

   void ProcessAvsDemand(std::vector<std::pair<Float64,bool>>& rDemandAtPois, WBFL::Math::PiecewiseFunction& rDemandAtLocations) const;

private:
	DECLARE_SHARED_LOGFILE;

};

// INLINE METHODS
//

// EXTERNAL REFERENCES
//

