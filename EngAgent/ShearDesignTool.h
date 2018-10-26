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
#pragma once;

// SYSTEM INCLUDES
//

// PROJECT INCLUDES
//

#include <IFace\Artifact.h>
#include <IFace\GirderHandlingPointOfInterest.h>
#include <PgsExt\PoiMgr.h>
#include <psgLib\GirderLibraryEntry.h>

#include <algorithm>
#include<list>
#include<vector>

// LOCAL INCLUDES
//

// FORWARD DECLARATIONS
//
interface IBroker;
class StirrupZoneIter;

// MISCELLANEOUS
// Virtual class for checking long reinf for shear - we need this from Designer2,
// but don't need the whole enchilada
class LongReinfShearChecker
{
public:
   virtual void CheckLongReinfShear( const pgsPointOfInterest& poi, 
                                     pgsTypes::Stage stage,
                                     pgsTypes::LimitState ls,
                                     const SHEARCAPACITYDETAILS& scd,
                                     const GDRCONFIG* pConfig,
                                     pgsLongReinfShearArtifact* pArtifact )=0;
};

/*****************************************************************************
CLASS 
   pgsShearDesignTool

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

class pgsShearDesignTool 
{
public:

   enum ShearDesignOutcome
   {
      sdSuccess,
      sdFail,
      sdRestartWithAdditionalLongRebar, // This and next are caused by long reinf for shear failure
      sdRestartWithAdditionalStrands
   };

   // GROUP: LIFECYCLE

   //------------------------------------------------------------------------
   // Default constructor
   pgsShearDesignTool(SHARED_LOGFILE lf);
   
   void Initialize(IBroker* pBroker, LongReinfShearChecker* pLongShearChecker, 
                   StatusGroupIDType statusGroupID, pgsDesignArtifact* pArtifact, 
                   Float64 startConfinementZl, Float64 endConfinementZl,
                   bool bPermit, bool bDesignFromScratch);

   // Start a new design
   void ResetDesign(const std::vector< pgsPointOfInterest >& pois);

   // Primary design function
   ShearDesignOutcome DesignStirrups(Float64 leftCSS, Float64 rightCSS);

   // Area of steel required for long reinf shear if outcome requires design restart
   // Note that this can be area of strands, or area of #5 bars adjusted for development
   Float64 GetRequiredAsForLongReinfShear() const;

   // Pois for spec check and design
   std::vector<pgsPointOfInterest> GetDesignPoi();

   // Get our check artifact to hand off to spec check routine
   pgsStirrupCheckArtifact* GetStirrupCheckArtifact();

private:
   // Stirrup design from scratch
   bool LayoutPrimaryStirrupZones();

   // Design by Modifying existing stirrup layout 
   bool ModifyPreExistingStirrupDesign();
   bool DesignPreExistingStirrups(StirrupZoneIter& rIter, Float64 locCSS,  matRebar::Grade barGrade, matRebar::Type barType, lrfdRebarPool* pool);

   // Design additional horizontal shear bars if needed
   bool DetailHorizontalInterfaceShear();

   // Design additional splitting reinforcement if needed
   bool DetailAdditionalSplitting();

   // Design additional confinement reinforcement if needed
   bool DetailAdditionalConfinement();

   // Check and design longitudinal reinforcement for shear
   ShearDesignOutcome DesignLongReinfShear();

   // GROUP: OPERATIONS

   // ACCESS
   //////////

   GDRCONFIG GetGirderConfiguration();

   SpanIndexType GetSpan() const      { return m_Span; }
   GirderIndexType GetGirder() const  { return m_Girder; }

   Float64 GetGirderLength() const;

   // Design Control Values
   bool DoDesignForConfinement() const;
   bool DoDesignForSplitting() const;
   bool DoDesignFromScratch() const;

   IndexType GetNumStirrupSizeBarCombos() const;
   void GetStirrupSizeBarCombo(IndexType index, matRebar::Size* pSize, Float64* pNLegs, Float64* pAv) const;

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

   pgsDesignArtifact* m_pArtifact;
   CShearData m_ShearData; // in-progress shear design data'

   LongReinfShearChecker* m_pLongReinfShearChecker;

   GirderIndexType m_Girder;
   SpanIndexType m_Span;

   bool m_bIsPermit;

   Float64 m_GirderLength;
   Float64 m_SpanLength;
   Float64 m_StartConnectionLength;
   Float64 m_EndConnectionLength;
   Float64 m_LeftFosLocation; // face of support from start of girder
   Float64 m_RightFosLocation;

   bool m_bIsCurrentStirrupLayoutSymmetrical;

   bool m_bDoDesignForConfinement;
   bool m_bDoDesignForSplitting;
   bool m_bDoDesignFromScratch;

   // Confinement zone lengths at both ends of girder
   Float64 m_StartConfinementZl;
   Float64 m_EndConfinementZl;

   // locations of critical sections for shear
   Float64 m_LeftCSS;
   Float64 m_RightCSS;

   // Splitting zone lengths at both ends of girder
   Float64 m_StartSplittingZl;
   Float64 m_EndSplittingZl;

   // Area of steel required if longitudinal reinf for shear design kicks in
   Float64 m_LongReinfShearAs;

   // Shear design parameters
   struct BarLegCombo
   {
      matRebar::Size m_Size;
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

   // Transient values for each design iteration
   // -------------------------------------------
   pgsPoiMgr m_PoiMgr;

   // Points of interest to be used for design
   // Note that the following block of vectors are the same size, and in the same order as, 
   // m_DesignPois. This allows quick storage and retreival in poi order
   // -------------------------------------------
   std::vector<pgsPointOfInterest> m_DesignPois; // sorted along girder

   // Two ways to store same vert av/s demand data here, by POI, and by X location
   std::vector<Float64> m_VertShearAvsDemandAtPois;    // Avs demand by POI
   mathPwLinearFunction2dUsingPoints m_VertShearAvsDemandAtX; // Avs demand at locations along girder

   // Two ways to store same Horiz av/s demand data here, by POI, and by X location
   std::vector<Float64> m_HorizShearAvsDemandAtPois;    // Avs demand by POI
   mathPwLinearFunction2dUsingPoints m_HorizShearAvsDemandAtX; // Avs demand at locations along girder

   // Store entire stirrup check artifact
   pgsStirrupCheckArtifact m_StirrupCheckArtifact;

   // PoiIdx in the following functions refer to indexes into m_DesignPois
   // Max stirrup spacing for a given location
   Float64 ComputeMaxStirrupSpacing(IndexType PoiIdx);

   Float64 ComputeMaxStirrupSpacing(Float64 location);

   // Get min stirrup spacing for given bar size
   Float64 GetMinStirrupSpacing(matRebar::Size size);

   // Get bar size - nlegs - spacing for av/s demand
   bool GetBarSizeSpacingForAvs(Float64 avsDemand, Float64 maxSpacing, matRebar::Size* pSize, Float64* pNLegs, Float64* pAv, Float64* pSpacing);

   // Get bar size - nlegs - spacing for av/s demand - For a particular bar size
   bool GetBarSpacingForAvs(Float64 avsDemand, Float64 maxSpacing, matRebar::Size Size, Float64 Av, Float64* pSpacing);

   // Get next (or same) available bar size for a given min bar size
   bool GetMinAvailableBarSize(matRebar::Size minSize, matRebar::Grade barGrade, matRebar::Type barType, lrfdRebarPool* pool, matRebar::Size* pSize);

   // Av/S demand at poi
   Float64 GetVerticalAvsDemand(IndexType PoiIdx);

   // Av/S demand at locations along girder
   Float64 GetVerticalAvsDemand(Float64 distFromStart);

   // Horiz Av/S demand at poi
   Float64 GetHorizontalAvsDemand(IndexType PoiIdx);

   // Horiz Av/S demand at locations along girder
   Float64 GetHorizontalAvsDemand(Float64 distFromStart);

   // Av/S demand in a range - useful for stirrup zones
   Float64 GetVerticalAvsMaxInRange(Float64 leftBound, Float64 rightBound);

   // Horiz Av/S demand in a range - useful for stirrup zones
   Float64 GetHorizontalAvsMaxInRange(Float64 leftBound, Float64 rightBound);

   // Max stirrup spacing in next zone
   Float64 ComputeMaxNextSpacing(Float64 currentSpacing);

   // Find a design stirrup spacing that works for input spacing
   Float64 GetDesignSpacingFromList(Float64 spacing);

   // determine if a location is in confinement zone
   bool IsLocationInConfinementZone(Float64 distFromStart);

   // utility to get closeby poi for a location
   IndexType GetPoiIdxForLocation(Float64 location);

   // Av/S required for splitting
   Float64 GetAvsReqdForSplitting() const;


   // Private functions called at initialization
   ///////////////////////////////////////////
   // compute and cache pois
   void ValidatePointsOfInterest(const std::vector<pgsPointOfInterest>& pois);

   void Validate();
   // Compute and cache Av/S demand
   void ValidateVerticalAvsDemand();
   void ValidateHorizontalAvsDemand();

   void ProcessAvsDemand(std::vector<Float64>& rDemandAtPois, mathPwLinearFunction2dUsingPoints& rDemandAtLocations);

private:
	DECLARE_SHARED_LOGFILE;

};

// INLINE METHODS
//

// EXTERNAL REFERENCES
//

