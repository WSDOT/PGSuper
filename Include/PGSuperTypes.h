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

#ifndef INCLUDED_PGSUPERTYPES_H_
#define INCLUDED_PGSUPERTYPES_H_
#pragma once

#include <WBFLTypes.h>
#include <WBFLTools.h>
#include <EAF\EAFTypes.h>
#include <MathEx.h>
#include <vector>
#include <array>

#include <PgsExt\Keys.h> // goes with GDRCONFIG
static long g_Ncopies = 0; // keeps track of the number of times GDRCONFIG is copied

#include <Materials/Rebar.h>

// Constants for consistent behavior of tooltips
#define TOOLTIP_WIDTH 400 // 400 characters
#define TOOLTIP_DURATION 20000 // 20 seconds

// defines the method used for computing creep coefficients
#define CREEP_LRFD              0
#define CREEP_WSDOT             1

#define CREEP_SPEC_PRE_2005     1 // creep based on pre 2005 provisions
#define CREEP_SPEC_2005         2 // creep based on 2005 interim provisions

// curing method (effects ti for loss and creep calculations)
#define CURING_NORMAL           0
#define CURING_ACCELERATED      1

// truck roll stiffness method
#define ROLLSTIFFNESS_LUMPSUM 0
#define ROLLSTIFFNESS_PERAXLE 1

// exposure conditions
#define EXPOSURE_NORMAL 0
#define EXPOSURE_SEVERE 1

// defines the creep time frame (ie. D40 and D120 for WSDOT)
#define CREEP_MINTIME           0
#define CREEP_MAXTIME           1

// method for computing fcgp - the concrete stress at the center of gravity of prestressing tendons at transfer
// this method is only used for the TxDOT 2013 loss method
#define FCGP_07FPU              0 // assume stress is 0.7fpu 
#define FCGP_ITERATIVE          1 // iterate to find value
#define FCGP_HYBRID             2 // iterate unless initial stress is 0.75fpu, then use 0.7fpu

// methods for distribution factor calculation
#define LLDF_LRFD               0
#define LLDF_WSDOT              1
// #define LLDF_DIRECT_INPUT       2 // Value is stored in project, not library.
#define LLDF_TXDOT              2 

// analysis or spec check method type
#define LRFD_METHOD               0
#define WSDOT_METHOD              1 // wsdot-modified range for check

// maximum girder spacing - used to indicate there is not an upper limit on girder spacing
// can't use DBL_MAX because in many places girder spacing is multiplied by a skew correction factor
// this can cause the floating point number to overflow
#define MAX_GIRDER_SPACING        DBL_MAX/3

// this can be any number of girders, however we'll set the limit to something practical
// 26*2 = 52 = 2 times through the alphabet... Girder A - Girder AZ
#define MAX_GIRDERS_PER_SPAN              52

// need to have a reasonable upper limit of skew angle... use 88 degrees
#define MAX_SKEW_ANGLE ::ToRadians(88.0)

#define NO_LIVE_LOAD_DEFINED _T("No Live Load Defined")

// When required concrete strength is set to this value it indicates that 
// no concrete strength will satisfy the stress limits
#define NO_AVAILABLE_CONCRETE_STRENGTH -99999

#define MIN_CURVE_RADIUS WBFL::Units::ConvertToSysUnits(0.01,WBFL::Units::Measure::Feet)

typedef struct pgsTypes
{
   typedef enum SectionCoordinateType
   {
      scBridge, // Bridge Section Coordinates
      scGirder, // Girder Section Coordinates
      scCentroid // Centroid/Stress Point Coordinates
   } SectionCoordinateType;

   typedef enum PlanCoordinateType
   {
      pcLocal, // coordinate is in the local coordinate system
      pcGlobal // coordinate is in the global coordinate system
   } PlanCoordinateType;

   // Defines the section property mode that is being used for analysis
   typedef enum SectionPropertyMode
   {
      spmGross,        // Based on concrete outline only
      spmTransformed   // Strand, rebar, tendons are transformed
   } SectionPropertyMode;

   // Defines a type of section property
   typedef enum SectionPropertyType
   {
      sptGrossNoncomposite,       // Based on concrete outline only. Gross section of non-composite girder only, regardless of analysis interval
      sptGross,                   // Based on concrete outline only. Deck is transformed to equivalent girder concrete
      sptTransformedNoncomposite, // Transformed section of non-composite girder only, regardless of analysis interval. All materials transformed to equivalent girder concrete
      sptTransformed,             // All materials including strand, rebar, tendons are transformed to equivalent girder concrete
      sptNetGirder,               // Non-composite girder section with holes for strands/rebar/ducts
      sptNetDeck,                 // Deck section with holes for rebar
      sptSectionPropertyTypeCount
   } SectionPropertyType;

   // describes how a precast segment varies along its length
   typedef enum SegmentVariationType
   {
      svtLinear,        // linear variation from one end to the other
      svtParabolic,     // similar to linear, except with a parabolic transition
      svtDoubleLinear,    // linear segments on each side of pier
      svtDoubleParabolic, // parabolic segments on each side of pier
      svtNone           // segment is constant depth and has dimensions as specified in the girder library
   } SegmentVariationType;

   // enum for the zones in a girder segment
   // applicability depends on section variation type
   typedef enum SegmentZoneType
   {
      sztLeftPrismatic,
      sztLeftTapered,
      sztRightTapered,
      sztRightPrismatic
   } SegmentZoneType;

   typedef enum DiaphragmType
   {
      dtPrecast,    // cast with the girder (like U beams or Box Beams)
      dtCastInPlace // cast at the time of deck casting
   } DiaphragmType;
   typedef std::vector<DiaphragmType> SupportedDiaphragmTypes;

   typedef enum DiaphragmLocationType
   {
      dltInternal, // cast between webs (like U beams or inside of Box Beams
      dltExternal  // cast between girders, with the deck
   } DiaphragmLocationType;
   typedef std::vector<DiaphragmLocationType> SupportedDiaphragmLocationTypes;

   typedef enum OptimizationType
   {
      Minimize,
      Maximize
   } OptimizationType;

   typedef enum BridgeAnalysisType
   { 
      SimpleSpan, 
      ContinuousSpan, 
      MinSimpleContinuousEnvelope, 
      MaxSimpleContinuousEnvelope 
   } BridgeAnalysisType;

   // Defines the meaning of a generic "SupportIndexType". Support indices are used
   // in some methods to represent either a pier or a temporary support. This enum is used
   // to tell to what the support index refers
   typedef enum SupportType
   {
      stPier,
      stTemporary
   } SupportType;

   typedef enum ColumnTransverseFixityType
   {
      ctftTopFixedBottomFixed,
      ctftTopFixedBottomPinned,
      ctftTopPinnedBottomFixed
   } ColumnTransverseFixityType;

   typedef enum ColumnLongitudinalBaseFixityType
   {
      cftFixed,
      cftPinned
   } ColumnLongitudinalBaseFixityType;

   typedef enum TemporarySupportType
   {
      ErectionTower,
      StrongBack
   } TemporarySupportType;

   // Defines segment to segment connection types
   // at a temporary support.
   typedef enum TempSupportSegmentConnectionType
   {
      tsctClosureJoint,
      tsctContinuousSegment
   } TempSupportSegmentConnectionType;

   // Defines segment to segment connection types
   // at an intermediate/permanent pier.
   typedef enum PierSegmentConnectionType
   {
      psctContinousClosureJoint, // CIP closure joint that makes the adjacent segments continuous but there is no moment connection with the pier
      psctIntegralClosureJoint,  // CIP closure joint that makes the adjacent segments and pier integral
      psctContinuousSegment,    // Precast segment spans over pier with no moment connection to the pier
      psctIntegralSegment       // Precast segment spans over pier and has a moment connection with the pier
   } PierSegmentConnectionType;

   // NOTE: Enum values are out of order so that they match values used in earlier
   // versions of the software
   typedef enum BoundaryConditionType { 
                             bctHinge = 1, 
                             bctRoller = 6,
                             bctContinuousAfterDeck = 2,        // continuous are for interior piers only
                             bctContinuousBeforeDeck = 3, 
                             bctIntegralAfterDeck = 4,          // these two are fixed both sides (unless abutments. obviously)
                             bctIntegralBeforeDeck = 5,
                             bctIntegralAfterDeckHingeBack  = 7, // interior piers only, left or right hinge
                             bctIntegralBeforeDeckHingeBack = 8,
                             bctIntegralAfterDeckHingeAhead = 9,
                             bctIntegralBeforeDeckHingeAhead = 10
   } BoundaryConditionType;

   typedef enum PierModelType
   {
      pmtIdealized, // pier is modeled with an idealized support object
      pmtPhysical   // pier is modeled with a physical description
   } PierModelType;

   // Bridge models can begin and/or end with a pier or an abutment starting with an arbitrary number (e.g., Pier 3)
   // This enum indicates what type of permanent support is at either end of the bridge
   typedef enum DisplayEndSupportType
   {
      desAbutment,
      desPier
   } DisplayEndSupportType;

   typedef enum TopFlangeThickeningType
   {
      tftNone, // top flange is not thickened
      tftEnds, // top flange is thickened at ends
      tftMiddle // top flange is thickened at the middle
   } TopFlangeThickeningType;

   typedef enum TopWidthType
   {
      twtSymmetric, // input defines the full top flange width. the top flange is centered on the web
      twtCenteredCG, // input defines the full top flange width. the left and right overhangs are automatically adjusted so the center of gravity aligns with the center of the web
      twtAsymmetric // input defines the left and right overhangs explicitly
   } TopWidthType;

   typedef enum StressType { Compression, Tension } StressType;
   
   typedef enum LimitState { 
                     ServiceI   = 0,   // Full dead load and live load
                     ServiceIA  = 1,   // Half dead load and live load (Fatigue) - not used after LRFD 2008
                     ServiceIII = 2, 
                     StrengthI  = 3,
                     StrengthII = 4,
                     FatigueI   = 5,   // added in LRFD 2009 (replaces Service IA)
                     StrengthI_Inventory = 6,
                     StrengthI_Operating = 7,
                     ServiceIII_Inventory = 8,
                     ServiceIII_Operating = 9,
                     StrengthI_LegalRoutine = 10,
                     StrengthI_LegalSpecial = 11,
                     ServiceIII_LegalRoutine = 12,
                     ServiceIII_LegalSpecial = 13,
                     StrengthII_PermitRoutine = 14,
                     ServiceI_PermitRoutine = 15,
                     ServiceIII_PermitRoutine = 16, // for WSDOT BDM Load Rating Requirements (See BDM Chapter 13)
                     StrengthII_PermitSpecial = 17,
                     ServiceI_PermitSpecial = 18,
                     ServiceIII_PermitSpecial = 19, // for WSDOT BDM Load Rating Requirements (See BDM Chapter 13)
                     StrengthI_LegalEmergency = 20,
                     ServiceIII_LegalEmergency = 21,
                     LimitStateCount // this should always be the last one as it will define the total number of limit states
                   } LimitState; 

   typedef enum StressLocation { BottomGirder, TopGirder, BottomDeck, TopDeck } StressLocation;

   // defines the method used for defining the pretensioned strands
   typedef enum StrandDefinitionType {
      sdtTotal, // input is total number of permanent strands
      sdtStraightHarped, // input is number of harped and number of straight strands
      sdtDirectSelection, // input is a fill array of strand positions in the girder strand grid
      sdtDirectRowInput, // input is direct row input by user. horizontal rows of strands are defined. the strand grid in the girder library is ignored
      sdtDirectStrandInput // input is direct input of individual strands by the user. the strand grid in the girder library is ignored
   } StrandDefinitionType;

   
   // Note that Permanent was added below when input for total permanent strands was added in 12/06
   typedef enum StrandType { Straight, Harped, Temporary, Permanent } StrandType;


   typedef enum AnalysisType { Simple, Continuous, Envelope } AnalysisType;

   // Adjustable strands can be straight or harped. In library only; they can be either
   typedef enum AdjustableStrandType {asHarped=0, asStraight=1, asStraightOrHarped = 2} AdjustableStrandType;

   // temporary top strand usage
   typedef enum TTSUsage
   {
      ttsPretensioned     = 0,
      ttsPTBeforeShipping = 1,
      ttsPTAfterLifting   = 2,
      ttsPTBeforeLifting  = 3,
   } TTSUsage;

   // prestress deflection datum
   // defines the datum from which prestress deflections are measured
   typedef enum PrestressDeflectionDatum
   {
      pddRelease, // relative to support locations at release
      pddLifting, // relative to support locations at lifting
      pddStorage, // relative to support locations during storage
      pddHauling, // relative to support locations at hauling
      pddErected  // relative to support locations after erection
   } PrestressDeflectionDatum;


   typedef enum LiveLoadType
   {
      lltDesign     = 0,   // for design limit states
      lltPermit     = 1,   // for permit limit state during design (Strength II)
      lltFatigue    = 2,   // for fatigue limit states
      lltPedestrian = 3,   // for pedestrian loads to be combined in any limit state
      lltLegalRating_Routine = 4,  // for legal load ratings for routine commercial traffic
      lltLegalRating_Special = 5,  // for legal load ratings for specialized hauling vehicles
      lltLegalRating_Emergency = 6, // for legal load ratings for emergency vehicles
      lltPermitRating_Routine = 7,  // for routine permit load ratings
      lltPermitRating_Special = 8,   // for special permit load ratings
      lltLiveLoadTypeCount
   } LiveLoadType;

   typedef enum LiveLoadApplicabilityType { llaEntireStructure, llaContraflexure, llaNegMomentAndInteriorPierReaction } LiveLoadApplicabilityType;

   typedef enum DebondLengthControl   // which criteria controlled for max debond length
   {mdbDefault, mbdFractional, mdbHardLength} DebondLengthControl;


   typedef enum EffectiveFlangeWidthMethod
   {
      efwmLRFD,       // effective flange width is based on LRFD
      efwmTribWidth   // effective flange width is always the tributary width
   } EffectiveFlangeWidthMethod;

   typedef enum MovePierOption
   {
      MoveBridge,
      AdjustPrevSpan,
      AdjustNextSpan,
      AdjustAdjacentSpans
   } MovePierOption;

   typedef enum MeasurementType
   {
      NormalToItem    = 0, // measured normal to the item (alignment, cl pier, cl bearing, cl girder, etc)
      AlongItem       = 1, // measured along centerline of item (pier, bearing, girder, etc)
   } MeasurementType;

   typedef enum MeasurementLocation
   {
      AtPierLine          = 0, // measured at pier datum line
      AtCenterlineBearing = 1, // measured at centerline bearing
   } MeasurementLocation;

   // member can be girder, span, column, etc
   typedef enum MemberEndType
   {
      metStart = 0,  // start of member
      metEnd   = 1   // end of member
   } MemberEndType;

   // Debonding can be at either, end, or start
   typedef enum DebondMemberEndType
   {
      dbetStart = 1,  // start of member
      dbetEnd   = 2,  // end of member
      dbetEither  = 3 // either end
   } DebondMemberEndType;

   typedef enum PierFaceType
   {
      Back  = pgsTypes::metEnd,   // back side of a pier is at the end of a span
      Ahead = pgsTypes::metStart  // ahead side of pier is at the start of a span
   } PierFaceType;

   typedef enum SideType
   {
      stLeft,
      stRight
   } SideType;

   typedef enum GirderOrientationType
   {
      Plumb = 0,     // Girder is plumb
      StartNormal,   // Girder is normal to deck at start of span
      MidspanNormal, // Girder is normal to deck at midspan
      EndNormal,     // Girder is normal to deck at end of span
      Balanced       // Orient girder to minimize clearance at opposing corners
   } GirderOrientationType;

   typedef enum DeckOverhangTaper
   {
      dotNone            = 0,  // No taper, constant thickness deck in overhangs
      dotTopTopFlange    = 1,  // Taper overhang to top of girder top flange
      dotBottomTopFlange = 2   // Taper overhang to bottom of girder top flange
   } DeckOverhangTaper;

   typedef enum OffsetMeasurementType
   {
      omtAlignment,  // offset measured from alignment
      omtBridge      // offset measured from CL bridge
   } OffsetMeasurementType;

   typedef enum DeckPointTransitionType
   {
      dptLinear,  // deck edge is linear between deck points
      dptSpline,  // deck edge is a cubic spline between points
      dptParallel // deck edge parallels alignment. deck points on both sides of transition must be the same
   } DeckPointTransitionType;

   typedef enum DeckRebarMatType
   {
      drmTop,   // top mat
      drmBottom // bottom mat
   } DeckRebarMatType;

   typedef enum DeckRebarBarType
   {
      drbIndividual,
      drbLumpSum,
      drbAll
   } DeckRebarBarType;

   typedef enum DeckRebarCategoryType
   {
      drcPrimary,
      drcSupplemental,
      drcAll
   } DeckRebarCategoryType;

   typedef enum WearingSurfaceType
   {
      wstSacrificialDepth  = 0,
      wstOverlay           = 1,
      wstFutureOverlay     = 2
   } WearingSurfaceType;

   typedef enum SupportedDeckType 
   {
      // NOTE: Assigning explicit values to guarantee consistency with deck type constants
      //       used prior to adding non-composite sections to PGSuper
      sdtCompositeCIP = 0,       // Composite cast-in-place deck
      sdtCompositeSIP = 1,       // Composite stay-in-place deck panels
      sdtCompositeOverlay = 2,       // Composite structural overlay (for adjacent beams)
      sdtNonstructuralOverlay = 4, // Composite overlay for adjacent beams, but does not contribute to the structural section
      sdtNone = 3                   // No deck is used... but an asphalt wearing surface can be
   } SupportedDeckType;


   typedef enum SupportedBeamSpacing
   {
      sbsUniform,                // Spacing is a constant value everywhere in the bridge
      sbsGeneral,                // Spacing is defined pier by pier (or span by span). Spacing between each girder can be different
      sbsUniformAdjacent,        // Beams are adjacent and joint width is the same in all spans. Top width is defined by girder.
      sbsGeneralAdjacent,        // Beams are adjacent and joint width can vary by span. Top width is defined by girder.
      sbsConstantAdjacent,       // Beams are adjacent and vary in top width is computed from girder spacing. The same girder spacing is used in all spans. No joints
      sbsUniformAdjacentWithTopWidth, // Beams are adjacent and top width and joint width is the same in all spans. Top width is input by user.
      sbsGeneralAdjacentWithTopWidth // Beams are adjacent and top width and joint width can vary by span. Top width is input by user.
   } SupportedBeamSpacing;

   typedef enum WorkPointLocation  // Vertical location of working point
   {
      wplTopGirder,
      wplBottomGirder
   } WorkPointLocation;

   //// Enums controlling direct input of haunch depths ////
   typedef enum HaunchInputDepthType
   {
      hidACamber,               // Use "A" (slab offset) input for haunch. This is for PGSuper only
      hidHaunchDirectly,        // Input haunch depths (top of girder to bottom of slab) directly 
      hidHaunchPlusSlabDirectly // Input haunch depths as total depth from top of girder to top of slab
   } HaunchInputDepthType;

   typedef enum HaunchInputLocationType 
   { 
      hilSame4Bridge,       // Use same haunch distribution for entire bridge
      hilSame4AllGirders,   // Use same distribution for all girders in a span or segment
      hilPerEach            // Unique haunch distribution for each segment or girder
   } HaunchInputLocationType;

   typedef enum HaunchLayoutType
   { 
      hltAlongSpans,    // Layout haunch distribution on per span basis
      hltAlongSegments  // Layout haunch distribution per segment
   } HaunchLayoutType;

   typedef enum HaunchInputDistributionType // Values of enums below represent number of values needed to define each method
   { 
      hidUniform = 1,      // Apply haunch uniformly along span or segment
      hidAtEnds = 2,       // Apply haunch linearly between ends of span or segment
      hidParabolic = 3,    // Haunch is distributed parabolically along span or segment. Control points at ends and middle of element
      hidQuarterPoints = 5,// Haunch is linearly distributed along quarter points of span or segment
      hidTenthPoints = 11  // Haunch is linearly distributed along 10th points of span or segment
   } HaunchInputDistributionType;

///////// Slab Offset and Assumed Excess Camber are older haunch definition methods and are used only to define haunch depths in PGSuper  ///////////
   typedef enum SlabOffsetType
   {
      sotBridge,  // a single slab offset is used in all spans
      sotBearingLine, // the slab offset is defined at each bearing line at the ends of segments (single value for entire bearing line)
      sotSegment,  // the slab offset is defined at the end of each segment individually
   } SlabOffsetType;

   typedef enum AssumedExcessCamberType 
   {
      aecBridge,  // a single camber is used in all spans
      aecSpan,    // the camber is defined at each span
      aecGirder,  // the camber is defined at each girder
   } AssumedExcessCamberType;

   typedef enum BearingType
   {
      brtBridge,  // same bearing data is used in all spans
      brtPier,    // unique bearing is defined at each abutment, pier, and temporary support and applies to all segments supported by that element
      brtGirder,  // unique bearing at each pier for each girder
   } BearingType;

   // Define connectivity (per AASHTO jargon) of adjacent beams.
   // This is only used if SupportedBeamSpacing==sbsUniformAdjacent or sbsGeneralAdjacent
   typedef enum AdjacentTransverseConnectivity
   {
      atcConnectedAsUnit,
      atcConnectedRelativeDisplacement
   } AdjacentTransverseConnectivity;

   typedef std::vector<SupportedDeckType>    SupportedDeckTypes;
   typedef std::vector<SupportedBeamSpacing> SupportedBeamSpacings;
   typedef std::vector<WorkPointLocation> WorkPointLocations;

   typedef enum RemovePierType
   {
      PrevPier,
      NextPier
   } RemovePierType;

   typedef enum DistributionFactorMethod
   { Calculated = 0, DirectlyInput = 1, LeverRule=2 } DistributionFactorMethod;

   typedef enum OverlayLoadDistributionType
   { olDistributeEvenly=0, olDistributeTributaryWidth=1 } OverlayLoadDistributionType;

   // For computing haunch load
   // hlcZeroCamber: Use geometric data to compute haunch assuming girder is flat
   // hlcDetailedAnalysis:
   //       - If HaunchInputDepthType==hidACamber, use assumed excess camber, "A" and geometry to determine haunch depth
   //       - else, direct haunch input. Use input values directly
   typedef enum HaunchLoadComputationType
   { hlcZeroCamber, hlcDetailedAnalysis } HaunchLoadComputationType;

   typedef enum HaunchAnalysisSectionPropertiesType
   { hspZeroHaunch, hspConstFilletDepth, hspDetailedDescription } HaunchAnalysisSectionPropertiesType;

   typedef enum GirderLocation
   { Interior = 0, Exterior = 1 } GirderLocation;

   typedef enum FaceType 
   {TopFace, BottomFace} FaceType;

   // Method for computing prestress transfer length
   typedef enum PrestressTransferComputationType { ptUsingSpecification=60, // use current spec
                                                   ptMinuteValue=0          // As close to zero length as we are comfortable
   } PrestressTransferComputationType;

   /// Defines the transfer length type. Transfer length is an uncertain value. The transfer length model
   /// supports a minimum and maximum length value so a short or long transfer length can be used when
   /// it is most critical.
   typedef enum TransferLengthType
   {
      tltMinimum, ///< Short transfer length, typically critical for Service and Fatigue Limit States
      tltMaximum ///< Long transfer length, typically critical for Strength and Extreme Event Limit States
   } TransferLengthType;

   typedef enum TrafficBarrierOrientation
   {
      tboLeft,
      tboRight
   } TrafficBarrierOrientation;

   // Defines how the traffic barrier/railing system loading is to be distributed
   typedef enum TrafficBarrierDistribution
   {
      tbdGirder,
      tbdMatingSurface,
      tbdWebs
   } TrafficBarrierDistribution;

   typedef enum SplittingDirection
   {
      sdVertical,
      sdHorizontal
   } SplittingDirection;

   // enum to represent condition factors (MBE 6A.4.2.3)
   typedef enum ConditionFactorType
   {
      cfGood,
      cfFair,
      cfPoor,
      cfOther
   } ConditionFactorType;

   typedef enum LoadRatingType
   {
      lrDesign_Inventory,  // design rating at the inventory level
      lrDesign_Operating,  // design rating at the operating level
      lrLegal_Routine,     // legal rating for routine commercial traffic
      lrLegal_Special,     // legal rating for specialized hauling vehicles
      lrLegal_Emergency,   // legal rating for emergency vehicles
      lrPermit_Routine,    // routine permit ratings
      lrPermit_Special,     // special permit ratings
      lrLoadRatingTypeCount
   } LoadRatingType;

   typedef enum SpecialPermitType
   {
      ptSingleTripWithEscort,    // special or limited crossing permit
      ptSingleTripWithTraffic,   // special or limited crossing permit
      ptMultipleTripWithTraffic,  // special or limited crossing permit
   } SpecialPermitType;

   // describes the model used for determining live load factors for rating
   typedef enum LiveLoadFactorType
   {
      gllSingleValue,  // a single value is used
      gllStepped,      // ADTT < a1 gll = g1, otherwise gll = g2
      gllLinear,       // ADTT < a1 gll = g1, ADTT > a2 gll = g2
      gllBilinear,     // ADTT < a1 gll = g1, ADTT = a2, gll = g2, ADTT > a3, gll = g3
      gllBilinearWithWeight // same as glBilinear with second set of values base on vehicle weight
   } LiveLoadFactorType;

   typedef enum LiveLoadFactorModifier
   {
      gllmInterpolate, // linear interpolate between ADTT values
      gllmRoundUp      // round up the ADTT value to match a control value
   } LiveLoadFactorModifier;

   typedef enum ConcreteType
   {
      Normal,
      AllLightweight, // Starting with AASHTO LRFD 7th Edition, 2016 Interims, the distinction between All Lightweight and Sand Lightweight is removed. AllLightweight is considered an invalid parameter and it automatically gets converted to SandLightweight
      SandLightweight, // Starting with AASHTO LRFD 7th Edition, 2016 Interims SandLightweight means "Lightweight" for all types of lightweight concrete
      PCI_UHPC, // Concrete is defined by PCI definition of UHPC
      UHPC, // Concrete is defined by AASHTO UHPC Guide Specification definition of UHPC 
      ConcreteTypeCount // this should always be the last value in the enum
   } ConcreteType;

   // Rebar layout defines where longitudinal rebar is placed along girder. 
   typedef enum RebarLayoutType 
   {
      blFullLength,      // extends full length
      blFromLeft,        // measured from left end of girder
      blFromRight,       // measured from right end of girder
      blMidGirderLength, // centered at mid-girder - fixed length
      blMidGirderEnds    // centered at mid-girder - measured from ends of girder
   } RebarLayoutType;

   // Hauling analysis
   typedef enum HaulingAnalysisMethod {hmWSDOT, hmKDOT } HaulingAnalysisMethod;

   typedef enum SagCamberType
   {
      UpperBoundCamber,
      AverageCamber,
      LowerBoundCamber
   } SagCamberType;

   typedef enum CureMethod
   {
      Moist,
      Steam
   } CureMethod;

   typedef enum ACI209CementType
   {
      TypeI,
      TypeIII
   } ACI209CementType;

   typedef enum CEBFIPCementType
   {
      RS, // rapid hardening, high strength
      N,  // normal hardening
      R,  // rapid hardening
      SL  // slow hardening
   } CEBFIPCementType;

   // indicates the relative point of time
   // within a time-step interval
   typedef enum IntervalTimeType
   {
      Start,
      Middle,
      End
   } IntervalTimeType;

   typedef enum LossMethod
   {
      AASHTO_REFINED      = 0,
      AASHTO_LUMPSUM      = 1,
      GENERAL_LUMPSUM     = 3,
      WSDOT_LUMPSUM       = 4, // same as PPR = 1.0 in aashto eqn's
      AASHTO_LUMPSUM_2005 = 5, // 2005 AASHTO code
      AASHTO_REFINED_2005 = 6, // 2005 AASHTO code
      WSDOT_LUMPSUM_2005  = 7, // 2005 AASHTO, WSDOT (includes initial relaxation loss)
      WSDOT_REFINED_2005  = 8, // 2005 AASHTO, WSDOT (includes initial relaxation loss)
      WSDOT_REFINED       = 9,
      TXDOT_REFINED_2004  = 10, // TxDOT's May, 09 decision is to use refined losses from AASHTO 2004
      TXDOT_REFINED_2013  = 11, // TxDOT's Method based on Report No. FHWA/TX-12/0-6374-2
      TIME_STEP           = 12  // Losses are computed with a time-step method
   } LossMethod;

   typedef enum TimeDependentModel
   {
      tdmAASHTO, 
      tdmACI209,
      tdmCEBFIP
   } TimeDependentModel;

   typedef enum JackingEndType
   {
      jeStart,
      jeEnd,
      jeBoth
   } JackingEndType;

   typedef enum DuctType
   {
      // See LRFD 5.4.6.1
      dtMetal,   // galvanized ferrous metal
      dtPlastic, // polyethylene
      dtFormed   // formed in concrete with removable cores
   } DuctType;

   typedef enum StrandInstallationType
   {
      // Defines strand installation type for post-tensioning tendons
      // See LRFD 5.4.6.2
      sitPush,
      sitPull
   } StrandInstallationType;

   typedef enum SegmentPTEventType
   {
      // Defines when plant installed segment PT tendons are installed
      sptetRelease, // immediately after release
      sptetStorage, // immediately after beginning of storage
      sptetHauling // immediately prior to hauling
   } SegmentPTEventType;

   typedef enum ProductForceType 
   { 
      // externally applied loads
      pftGirder,
      pftConstruction,
      pftSlab, 
      pftSlabPad, 
      pftSlabPanel, 
      pftDiaphragm, 
      pftOverlay,
      pftSidewalk,
      pftTrafficBarrier, 
      pftUserDC, 
      pftUserDW, 
      pftUserLLIM,
      pftShearKey,
      pftLongitudinalJoint,

      // pseudo externally applied loads (really internal loads)
      pftPretension,       // P*e for pretension
      pftPostTensioning,   // P*e for post-tension
      pftSecondaryEffects,

      // time-depending effects
      pftCreep,
      pftShrinkage,
      pftRelaxation,

      // special cases
      pftOverlayRating,

      pftProductForceTypeCount
   } ProductForceType;

   typedef enum ForceEffectType
   {
      fetFx = 0,
      fetFy = 1,
      fetMz = 2,

      fetDx = 0,
      fetDy = 1,
      fetRz = 2
   } ForceEffectType;

   typedef enum LimitStateConcreteStrength
   {
      lscStrengthAtTimeOfLoading, // use f'ci and f'c from the time-strength curve
      lscSpecifiedStrength // use f'ci at release and f'c at day 28.
   } LimitStateConcreteStrength;

   typedef enum HaunchShapeType
   {
      hsSquare,    // Haunch is square (vertical from edge of girder)
      hsFilleted   // Haunch cut at 45 degrees (like WSDOT)
   } HaunchShapeType;

   typedef enum WindType
   {
      Speed,
      Pressure
   } WindType;

   typedef enum CFType // centrifugal force type
   {
      Adverse, // CF is towards the left (increases lateral deflection and roll over)
      Favorable // CF is towards the right
   } CFType;

   typedef enum HaulingImpact
   {
      NormalCrown, // impact applied only to the normal crown condition
      MaxSuper,    // impact applied only to the max superelevation condition   
      Both         // impact applied to both conditions
   } HaulingImpact;

   typedef enum HaulingSlope
   {
      CrownSlope, // hauling at normal crown slope
      Superelevation // hauling at maximum superelevation
   } HaulingSlope;

   typedef enum SectionBias
   {
      sbLeft,
      sbRight
   } SectionBias;

   typedef enum SlabOffsetRoundingMethod
   {
      sormRoundUp,
      sormRoundNearest
   } SlabOffsetRoundingMethod;

   typedef enum DeckCastingRegionBoundary
   {
      dcrbNormalToAlignment, // deck casting region boundaries are normal to the alignment
      dcrbParallelToPier // deck casting region boundaries are parallel to their reference pier
   } DeckCastingRegionBoundary;

   typedef enum PrincipalTensileStressMethod
   {
      ptsmLRFD,
      ptsmNCHRP
   } PrincipalTensileStressMethod;

   typedef enum ShearFlowMethod
   {
      // compute horizontal interface shear flow between slab and girder using
      sfmLRFD,     // LRFD simplified equation
      sfmClassical // Classical equation (VQ/I)
   } ShearFlowMethod;

   typedef enum ShearCapacityMethod
   {
      // NOTE: enum values are in a weird order... the constants are set so that they
      //       are consistent with previous versions of PGSuper. DO NOT CHANGE CONSTANTS
      scmBTEquations = 0, // LRFD 5.7.3.5 (pre2017: 5.8.3.5)
      scmVciVcw = 2, // LRFD 5.8.3.6 (Vci, Vcw - added in 2007, removed in 2017)
      scmWSDOT2001 = 1, // WSDOT BDM Method (June 2001 Design Memo)
      scmBTTables = 3, // LRFD B5.1
      scmWSDOT2007 = 4  // WSDOT BDM Method (August 2007 Design Memo - Use new BT equations from to be published 2008 interims)
   } ShearCapacityMethod;

   // Segments at erection can be Drop ins. In this case, one or both ends are free to translate  
   // in order to meet elevation with the supporting segment
   typedef enum DropInType
   { 
      ditNotDropIn, 
      ditYesFreeBothEnds, 
      ditYesFreeStartEnd, 
      ditYesFreeEndEnd 
   } DropInType;

   // The Geometry Control Event is when elevations of segment chords in the bridge model are matched to
   // controlling haunch depths. Other geometry control activities can be created to specify when alternate roadway
   // geometry spec checks occur, or when only reporting of elevations are requested
   typedef enum GeometryControlActivityType
   {
      gcaDisabled = 0,
      gcaGeometryReportingEvent = 2,  // Generate report only
      gcaSpecCheckEvent = 4,          // Generate spec check and geometry report
      gcaGeometryControlEvent = 8,    // Controlling event. There can be only one activity of this type
   } GeometryControlActivityType;

} pgsTypes;

//-----------------------------------------------------------------------------
// Struct for stirrup information.
struct STIRRUPCONFIG
{
   // First, some needed types
   // =========================
   struct SHEARZONEDATA // Primary shear zone
   {
      Float64 ZoneLength;
      Float64 BarSpacing;
      WBFL::Materials::Rebar::Size VertBarSize;
      Float64 nVertBars;
      Float64 nHorzInterfaceBars;
      WBFL::Materials::Rebar::Size ConfinementBarSize;

      // pre-computed values
      Float64 VertABar; // Area of single bar

      // This struct is complex enough to need a good constructor
      SHEARZONEDATA():
      ZoneLength(0.0), VertBarSize(WBFL::Materials::Rebar::Size::bsNone), BarSpacing(0.0), nVertBars(0.0), 
      nHorzInterfaceBars(0.0), ConfinementBarSize(WBFL::Materials::Rebar::Size::bsNone), VertABar(0.0)
      {;}

      bool operator==(const SHEARZONEDATA& other) const
      {
         if( !IsEqual(ZoneLength , other.ZoneLength) ) return false;
         if(VertBarSize != other.VertBarSize) return false;
         if( !IsEqual(BarSpacing , other.BarSpacing) ) return false;
         if( !IsEqual(nVertBars , other.nVertBars) ) return false;
         if( !IsEqual(nHorzInterfaceBars , other.nHorzInterfaceBars) ) return false;
         if(ConfinementBarSize != other.ConfinementBarSize) return false;

         return true;
      }

      bool operator!=(const SHEARZONEDATA& other) const
      {
         return !operator==(other);
      }
   };

   typedef std::vector<SHEARZONEDATA> ShearZoneVec;
   typedef ShearZoneVec::iterator ShearZoneIterator;
   typedef ShearZoneVec::const_iterator ShearZoneConstIterator;
   typedef ShearZoneVec::const_reverse_iterator ShearZoneConstReverseIterator;

   struct HORIZONTALINTERFACEZONEDATA // Additional horizontal interface zone
   {
      Float64 ZoneLength;
      Float64 BarSpacing;
      WBFL::Materials::Rebar::Size BarSize;
      Float64 nBars;

      // pre-computed values
      Float64 ABar; // area of single bar

      // default constructor
      HORIZONTALINTERFACEZONEDATA():
      ZoneLength(0.0), BarSize(WBFL::Materials::Rebar::Size::bsNone),BarSpacing(0.0),nBars(0.0), ABar(0.0)
      {;}

      bool operator==(const HORIZONTALINTERFACEZONEDATA& other) const
      {
         if( !IsEqual(ZoneLength , other.ZoneLength) ) return false;
         if(BarSize != other.BarSize) return false;
         if( !IsEqual(BarSpacing , other.BarSpacing) ) return false;
         if( !IsEqual(nBars , other.nBars) ) return false;

         return true;
      }

      bool operator!=(const HORIZONTALINTERFACEZONEDATA& other) const
      {
         return !operator==(other);
      }
   };

   typedef std::vector<HORIZONTALINTERFACEZONEDATA> HorizontalInterfaceZoneVec;
   typedef HorizontalInterfaceZoneVec::iterator HorizontalInterfaceZoneIterator;
   typedef HorizontalInterfaceZoneVec::const_iterator HorizontalInterfaceZoneConstIterator;

   // Our Data
   // ========
   ShearZoneVec ShearZones;
   HorizontalInterfaceZoneVec HorizontalInterfaceZones;

   bool bIsRoughenedSurface;
   bool bUsePrimaryForSplitting;
   bool bAreZonesSymmetrical;

   WBFL::Materials::Rebar::Size SplittingBarSize; // additional splitting bars
   Float64 SplittingBarSpacing;
   Float64 SplittingZoneLength;
   Float64 nSplittingBars;

   WBFL::Materials::Rebar::Size ConfinementBarSize; // additional confinement bars - only used if primary not used for confinement
   Float64 ConfinementBarSpacing;
   Float64 ConfinementZoneLength;

   STIRRUPCONFIG():
   bIsRoughenedSurface(true), bUsePrimaryForSplitting(false), bAreZonesSymmetrical(true),
   SplittingBarSize(WBFL::Materials::Rebar::Size::bsNone), SplittingBarSpacing(0.0), SplittingZoneLength(0.0), nSplittingBars(0.0),
   ConfinementBarSize(WBFL::Materials::Rebar::Size::bsNone), ConfinementBarSpacing(0.0), ConfinementZoneLength(0.0)
   {;}

   bool operator==(const STIRRUPCONFIG& other) const
   {
      if(ShearZones != other.ShearZones) return false;
      if(HorizontalInterfaceZones != other.HorizontalInterfaceZones) return false;

      if(bIsRoughenedSurface != other.bIsRoughenedSurface) return false;
      if(bUsePrimaryForSplitting != other.bUsePrimaryForSplitting) return false;
      if(bAreZonesSymmetrical != other.bAreZonesSymmetrical) return false;
      if(SplittingBarSize != other.SplittingBarSize) return false;

      if ( !IsEqual(SplittingBarSpacing,  other.SplittingBarSpacing) ) return false;
      if ( !IsEqual(SplittingZoneLength,  other.SplittingZoneLength) ) return false;
      if ( !IsEqual(nSplittingBars,  other.nSplittingBars) ) return false;

      if(ConfinementBarSize != other.ConfinementBarSize) return false;
      if ( !IsEqual(ConfinementBarSpacing,  other.ConfinementBarSpacing) ) return false;
      if ( !IsEqual(ConfinementZoneLength,  other.ConfinementZoneLength) ) return false;


      return true;
   }

   bool operator!=(const STIRRUPCONFIG& other) const
   {
      return !operator==(other);
   }
};
// NOTE: Data here is not used, but may be useful for someone in the future.
//       This is a preliminary design for modeling longitudinal rebar.
//       After some effort, I determined that the data is not necessary for the 
//       design algorithm since it is only used for longitudinal reinforcement for shear,
//       and does not need to be iterated on. (e.g., the algorithm just picks a value
//       and submits it directly to the final design).
//-----------------------------------------------------------------------------
// Struct for longitudinal rebar information.
/*
struct LONGITUDINALREBARCONFIG
{
public:
   struct RebarRow 
   {
      pgsTypes::FaceType  Face;
      WBFL::Materials::Rebar::Size BarSize;
      Int32       NumberOfBars;
      Float64     Cover;
      Float64     BarSpacing;

      bool operator==(const RebarRow& other) const
      {
         if(Face != other.Face) return false;
         if(BarSize != other.BarSize) return false;
         if ( !IsEqual(Cover,  other.Cover) ) return false;
         if ( !IsEqual(BarSpacing,  other.BarSpacing) ) return false;
         if ( !IsEqual(NumberOfBars,  other.NumberOfBars) ) return false;

         return true;
      };
   };

   WBFL::Materials::Rebar::Type BarType;
   WBFL::Materials::Rebar::Grade BarGrade;
   std::vector<RebarRow> RebarRows;

   bool operator==(const LONGITUDINALREBARCONFIG& other) const
   {
      if(BarType != other.BarType) return false;
      if(BarGrade != other.BarGrade) return false;
      if(RebarRows != other.RebarRows) return false;
   }

   bool operator!=(const LONGITUDINALREBARCONFIG& other) const
   {
      return !operator==(other);
   }

};
*/

//-----------------------------------------------------------------------------
// Individual strand debond information
struct DEBONDCONFIG
{
   StrandIndexType strandIdx; // index of strand that is debonded (indexed by total # of filled strand locations)
   std::array<Float64,2> DebondLength;   // length of debond at left end of the girder

   bool operator<(const DEBONDCONFIG& other) const
   {
      return strandIdx < other.strandIdx;
   }

   bool operator==(const DEBONDCONFIG& other) const
   {
      bool bEqual = true;
      bEqual &= (strandIdx == other.strandIdx);
      bEqual &= IsEqual(DebondLength[pgsTypes::metStart],other.DebondLength[pgsTypes::metStart]);
      bEqual &= IsEqual(DebondLength[pgsTypes::metEnd],other.DebondLength[pgsTypes::metEnd]);
      return bEqual;
   }
};
// handy typedefs for storing debond info
typedef std::vector<DEBONDCONFIG>               DebondConfigCollection;
typedef DebondConfigCollection::iterator        DebondConfigIterator;
typedef DebondConfigCollection::const_iterator  DebondConfigConstIterator;

// fill array
typedef std::vector<StrandIndexType>            ConfigStrandFillVector;
typedef ConfigStrandFillVector::iterator        ConfigStrandFillIterator;
typedef ConfigStrandFillVector::const_iterator  ConfigStrandFillConstIterator;

struct PRESTRESSCONFIG
{
   // Set/Get strand fill. This array is the same length as the number strand coordinate locations in GirderLibraryEntry
   // Each collection entry defines the number of strands filled at that location. 
   // Valid values are 0, 1, or 2 strands filled. 2 strands cannot be filled if X=0.0 at location.
   void SetStrandFill(pgsTypes::StrandType type, const ConfigStrandFillVector& fillArray); 
   // Zero out all fill data
   void ClearStrandFill(pgsTypes::StrandType type); 
   const ConfigStrandFillVector& GetStrandFill(pgsTypes::StrandType type) const; 

   StrandIndexType GetStrandCount(pgsTypes::StrandType type) const; // Number of strands in current strand fill

   static StrandIndexType CountStrandsInFill(const ConfigStrandFillVector& fillArray); 

   const std::vector<StrandIndexType>& GetExtendedStrands(pgsTypes::StrandType strandType,pgsTypes::MemberEndType endType) const;
   void SetExtendedStrands(pgsTypes::StrandType strandType,pgsTypes::MemberEndType endType,const std::vector<StrandIndexType>& extStrands);

   // use one of the pgsTypes::StrandType constants to for array index
   std::array<std::vector<DEBONDCONFIG>,3> Debond; // Information about debonded strands (key is strand index into total number of filled strands)
   std::array<Float64,3> Pjack;  // Jacking force

   // array index is pgsTypes::MemberEndType
   std::array<Float64,2> EndOffset; // Offset of harped strands at end of girder
   std::array<Float64,2> HpOffset;  // Offset of harped strands at the harping point
   pgsTypes::TTSUsage TempStrandUsage;

   pgsTypes::AdjustableStrandType AdjustableStrandType; // can be asHarped or asStraight only

   PRESTRESSCONFIG():
   TempStrandUsage(pgsTypes::ttsPretensioned), AdjustableStrandType(pgsTypes::asHarped)
   {
      EndOffset[pgsTypes::metStart] = 0;
      EndOffset[pgsTypes::metEnd] = 0;
      HpOffset[pgsTypes::metStart] = 0;
      HpOffset[pgsTypes::metEnd] = 0;
      for (int i=0; i<3; i++)
      {
         Pjack[i] = 0.0;
         NstrandsCached[i] = 0;
      }
   }

   bool operator==(const PRESTRESSCONFIG& other) const;
   bool operator!=(const PRESTRESSCONFIG& other) const
   {
      return !operator==(other);
   }

   PRESTRESSCONFIG(const PRESTRESSCONFIG& rOther)
   {
      MakeCopy( rOther );
   }

   PRESTRESSCONFIG& operator=(const PRESTRESSCONFIG& rOther)
   {
      if ( this != &rOther )
         MakeAssignment( rOther );

      return *this;
   }

private:
   std::array<StrandIndexType,3> NstrandsCached;  // Number of strands
   std::array<ConfigStrandFillVector,3> StrandFill;
   std::vector<StrandIndexType> NextendedStrands[3][2]; // Holds index of extended strands (array index [pgsTypes::StrandType][pgsTypes::MemberEndType])

   void MakeAssignment( const PRESTRESSCONFIG& rOther )
   {
      MakeCopy( rOther );
   }

   void MakeCopy( const PRESTRESSCONFIG& rOther );

};

inline void PRESTRESSCONFIG::SetStrandFill(pgsTypes::StrandType type, const ConfigStrandFillVector& fillArray)
{
   ATLASSERT(type==pgsTypes::Straight || type==pgsTypes::Harped || type==pgsTypes::Temporary);

   StrandFill[type] = fillArray;

   // count strands and cache value
   StrandIndexType ns = CountStrandsInFill(StrandFill[type]);

   NstrandsCached[type] = ns;
}

inline StrandIndexType PRESTRESSCONFIG::CountStrandsInFill(const ConfigStrandFillVector& fillArray)
{
   StrandIndexType ns(0);
   ConfigStrandFillConstIterator it = fillArray.begin();
   ConfigStrandFillConstIterator itend = fillArray.end();
   while ( it != itend )
   {
      ATLASSERT(*it==0 || *it==1 || *it==2);
      ns += *it;
      it++;     
   }

   return ns;
}

inline void PRESTRESSCONFIG::ClearStrandFill(pgsTypes::StrandType type)
{
   // Set all fills to zero
   if (!StrandFill[type].empty())
   {
      StrandFill[type].assign( StrandFill[type].size(), 0);
   }

   NstrandsCached[type] = 0;
}

inline const ConfigStrandFillVector&  PRESTRESSCONFIG::GetStrandFill(pgsTypes::StrandType type) const
{
   ATLASSERT(type==pgsTypes::Straight || type==pgsTypes::Harped || type==pgsTypes::Temporary);
   return StrandFill[type];
}

inline StrandIndexType PRESTRESSCONFIG::GetStrandCount(pgsTypes::StrandType type) const
{
   if ( type == pgsTypes::Permanent )
      return NstrandsCached[pgsTypes::Straight] + NstrandsCached[pgsTypes::Harped];
   else
      return NstrandsCached[type];
}

inline const std::vector<StrandIndexType>& PRESTRESSCONFIG::GetExtendedStrands(pgsTypes::StrandType strandType,pgsTypes::MemberEndType endType) const
{
   return NextendedStrands[strandType][endType];
}

inline void PRESTRESSCONFIG::SetExtendedStrands(pgsTypes::StrandType strandType,pgsTypes::MemberEndType endType,const std::vector<StrandIndexType>& extStrands)
{
   NextendedStrands[strandType][endType] = extStrands;
}

inline bool PRESTRESSCONFIG::operator==(const PRESTRESSCONFIG& other) const
{
   for (int i=0; i<3; i++)
   {
      if (Debond[i] != other.Debond[i])
         return false;

      if( !IsEqual(Pjack[i], other.Pjack[i]) )
         return false;

      if ( StrandFill[i] != other.StrandFill[i] )
         return false;

      if ( NextendedStrands[i][pgsTypes::metStart] != other.NextendedStrands[i][pgsTypes::metStart] )
         return false;

      if ( NextendedStrands[i][pgsTypes::metEnd] != other.NextendedStrands[i][pgsTypes::metEnd] )
         return false;

      ATLASSERT(NstrandsCached[i] == other.NstrandsCached[i]); // this should be impossible
   }

   for ( int i = 0; i < 2; i++ )
   {
      if( !IsEqual(EndOffset[i], other.EndOffset[i]) )
         return false;

      if( !IsEqual(HpOffset[i], other.HpOffset[i]) )
         return false;
   }

   if (TempStrandUsage != other.TempStrandUsage)
      return false;

   if (AdjustableStrandType != other.AdjustableStrandType)
      return false;

   return true;
}

inline void PRESTRESSCONFIG::MakeCopy( const PRESTRESSCONFIG& other )
{
   for (int i=0; i<3; i++)
   {
      Debond[i] = other.Debond[i];

      Pjack[i] = other.Pjack[i];

      StrandFill[i] = other.StrandFill[i];

      NstrandsCached[i] = other.NstrandsCached[i];
   
      NextendedStrands[i][pgsTypes::metStart] = other.NextendedStrands[i][pgsTypes::metStart];
      NextendedStrands[i][pgsTypes::metEnd]   = other.NextendedStrands[i][pgsTypes::metEnd];
   }

   for ( int i = 0; i < 2; i++ )
   {
      EndOffset[i] = other.EndOffset[i];
      HpOffset[i]  = other.HpOffset[i];
   }

   TempStrandUsage = other.TempStrandUsage;

   AdjustableStrandType = other.AdjustableStrandType;
}

//-----------------------------------------------------------------------------
// Girder configuration
struct GDRCONFIG
{
   CSegmentKey SegmentKey;

   PRESTRESSCONFIG PrestressConfig; // all prestressing information

   // fc/fc28 - The 115% f'c allowance per LRFD 5.12.3.2.5 is only applicable
   // to stresses. Use Fc28 for strength analysis
   Float64 fc28;      // 28 day concrete strength (used for strength)
   Float64 fc;        // 28 or 90 day compressive strength (used for stresses)
   Float64 fci;       // Concrete release strength
   pgsTypes::ConcreteType ConcType;
   bool bHasFct;
   Float64 Fct;

   bool bUserEci;
   bool bUserEc;
   Float64 Eci;
   Float64 Ec;

   std::array<Float64,2> SlabOffset; // slab offset at start and end of the girder (use pgsTypes::MemberEndType for array index)
   Float64 AssumedExcessCamber;

   STIRRUPCONFIG StirrupConfig; // All of our transverse rebar information

//   LONGITUDINALREBARCONFIG LongitudinalRebarConfig; // Girder-length rebars
   GDRCONFIG()
   {;}

   GDRCONFIG(const GDRCONFIG& rOther)
   {
      MakeCopy( rOther );
   }

   GDRCONFIG& operator=(const GDRCONFIG& rOther)
   {
      if ( this != &rOther )
         MakeAssignment( rOther );

      return *this;
   }

   // Check equality for only flexural date (not stirrups)
   bool IsFlexuralDataEqual(const GDRCONFIG& other) const
   {
      if (PrestressConfig != other.PrestressConfig) return false;

      if (!IsEqual(fci, other.fci)) return false;
      if (!IsEqual(fc, other.fc)) return false;
      if (!IsEqual(fc28, other.fc28)) return false;

      if (bUserEci != other.bUserEci) return false;
      if (bUserEc  != other.bUserEc)  return false;
      if (!IsEqual(Eci,other.Eci)) return false;
      if (!IsEqual(Ec, other.Ec)) return false;

      if ( !IsEqual(SlabOffset[pgsTypes::metStart],other.SlabOffset[pgsTypes::metStart]) ) return false;
      if ( !IsEqual(SlabOffset[pgsTypes::metEnd],  other.SlabOffset[pgsTypes::metEnd])   ) return false;

      if (!IsEqual(AssumedExcessCamber, other.AssumedExcessCamber)) return false;

      return true;
   }

   bool operator==(const GDRCONFIG& other) const
   {
      if (SegmentKey != other.SegmentKey)
         return false;

       if(!IsFlexuralDataEqual(other))
           return false;

      if (StirrupConfig != other.StirrupConfig) return false;

      return true;
   }

   bool operator!=(const GDRCONFIG& other) const
   {
      return !operator==(other);
   }

private:
void MakeCopy( const GDRCONFIG& rOther )
{
   g_Ncopies++;

   SegmentKey = rOther.SegmentKey;

   PrestressConfig = rOther.PrestressConfig;

   fc = rOther.fc;
   fc28 = rOther.fc28;
   fci = rOther.fci;
   ConcType = rOther.ConcType;
   bHasFct = rOther.bHasFct;
   Fct = rOther.Fct;

   bUserEci = rOther.bUserEci;
   bUserEc = rOther.bUserEc;
   Eci = rOther.Eci;
   Ec = rOther.Ec;

   SlabOffset = rOther.SlabOffset;

   AssumedExcessCamber = rOther.AssumedExcessCamber;

   StirrupConfig = rOther.StirrupConfig;
}

void MakeAssignment( const GDRCONFIG& rOther )
{
   MakeCopy( rOther );
}

};

class HaulTruckLibraryEntry;
struct HANDLINGCONFIG
{
   HANDLINGCONFIG() { bIgnoreGirderConfig = true; pHaulTruckEntry = nullptr; }

   bool bIgnoreGirderConfig; // set true, the GdrConfig is ignored and the current parameters are used
                             // only the overhang parameters are used from this config.
   GDRCONFIG GdrConfig;
   Float64 LeftOverhang;
   Float64 RightOverhang;  // overhang closest to cab of truck when used from hauling

   const HaulTruckLibraryEntry* pHaulTruckEntry;
};

enum arFlexuralDesignType { dtNoDesign, dtDesignForHarping, dtDesignForDebonding, dtDesignFullyBonded,
                            dtDesignFullyBondedRaised, dtDesignForDebondingRaised }; // raised straight strands
enum arConcreteDesignType { cdPreserveStrength, cdDesignForMinStrength };
enum arDesignStrandFillType { ftGridOrder, ftMinimizeHarping, ftDirectFill }; // direct fill used for raised straight
enum arSlabOffsetDesignType { sodPreserveHaunch, sodDesignHaunch, sodDefault }; 
enum arShearDesignType { sdtNoDesign, sdtLayoutStirrups, sdtRetainExistingLayout };

struct arDesignOptions
{
   arFlexuralDesignType doDesignForFlexure;
   arConcreteDesignType doDesignConcreteStrength;
   arDesignStrandFillType doStrandFillType;
   bool doForceHarpedStrandsStraight;
   arSlabOffsetDesignType doDesignSlabOffset;
   arShearDesignType doDesignForShear;
   bool doDesignLifting;
   bool doDesignHauling;
   bool doDesignSlope;
   bool doDesignHoldDown;

   // max concrete strength for this option
   Float64 maxFci;
   Float64 maxFc;


   arDesignOptions(): doDesignConcreteStrength(cdDesignForMinStrength),doDesignForFlexure(dtNoDesign), doDesignSlabOffset(sodPreserveHaunch), doDesignLifting(false), doDesignHauling(false),
                      doDesignSlope(false), doDesignHoldDown(false), 
                      doStrandFillType(ftMinimizeHarping), doDesignForShear(sdtLayoutStirrups),
                      doForceHarpedStrandsStraight(false),
                      maxFci(0.0),
                      maxFc(0.0)
   {;}
};


struct StressCheckTask
{
   IntervalIndexType intervalIdx;
   pgsTypes::LimitState limitState;
   pgsTypes::StressType stressType;
   bool bIncludeLiveLoad; // if intervalIdx is a live load interval, live load is include in the prestressing if this parameter is true
                          // bIncludeLiveLoad should always be set to true unless you explicitly want to exclude live load in the stress check
                          // bIncludeLiveLoad is used when comparing tasks, show you must match this parameters in queries.

   StressCheckTask()
   {
      intervalIdx = INVALID_INDEX;
      limitState = pgsTypes::ServiceI;
      stressType = pgsTypes::Compression;
      bIncludeLiveLoad = true;
   }

   StressCheckTask(IntervalIndexType intervalIdx, pgsTypes::LimitState limitState, pgsTypes::StressType stressType, bool bIncludeLiveLoad = true) :
      intervalIdx(intervalIdx), limitState(limitState), stressType(stressType), bIncludeLiveLoad(bIncludeLiveLoad)
   {
   }

   bool operator==(const StressCheckTask& other) const
   {
      return intervalIdx == other.intervalIdx && limitState == other.limitState && stressType == other.stressType && bIncludeLiveLoad == other.bIncludeLiveLoad;
   }

   bool operator<(const StressCheckTask& other) const
   {
      if (intervalIdx < other.intervalIdx)
         return true;

      if (other.intervalIdx < intervalIdx)
         return false;

      ATLASSERT(intervalIdx == other.intervalIdx);

      if (limitState < other.limitState)
         return true;

      if (other.limitState < limitState)
         return false;

      ATLASSERT(limitState == other.limitState);

      if (stressType < other.stressType)
         return true;

      if (other.stressType < stressType)
         return false;

      ATLASSERT(stressType == other.stressType);

      if ((int)bIncludeLiveLoad < (int)other.bIncludeLiveLoad)
         return true;

      if ((int)other.bIncludeLiveLoad < (int)bIncludeLiveLoad)
         return false;

      ATLASSERT(bIncludeLiveLoad == other.bIncludeLiveLoad);

      return false;

   }
};


// measurement methods of harped strand offsets
typedef enum HarpedStrandOffsetType
{
   hsoLEGACY        = -1,    // Method used pre-version 6.0
   hsoCGFROMTOP     = 0,    // CG of harped strands from top
   hsoCGFROMBOTTOM  = 1,    // CG of   "       "      "  bottom
   hsoTOP2TOP       = 2,    // Top-most strand to top of girder
   hsoTOP2BOTTOM    = 3,    // Top-most strand to bottom of girder
   hsoBOTTOM2BOTTOM = 4,    // Bottom-most strand to bottom of girder
   hsoECCENTRICITY  = 5     // Eccentricity of total strand group
} HarpedStrandOffsetType;

// functions to hash and un-hash span/girder for file loading and saving
inline DWORD HashGirderSpacing(pgsTypes::MeasurementLocation ml,pgsTypes::MeasurementType mt)
{
   return MAKELONG(ml,mt);
}

inline void UnhashGirderSpacing(DWORD_PTR girderSpacingHash,pgsTypes::MeasurementLocation *ml,pgsTypes::MeasurementType *mt)
{
   *ml = (pgsTypes::MeasurementLocation)LOWORD(girderSpacingHash);
   *mt = (pgsTypes::MeasurementType)HIWORD(girderSpacingHash);
}

inline bool IsGirderSpacing(pgsTypes::SupportedBeamSpacing sbs)
{
   // spacing type is a girder spacing and not a joint spacing
   if (sbs == pgsTypes::sbsUniform ||
      sbs == pgsTypes::sbsGeneral ||
      sbs == pgsTypes::sbsConstantAdjacent
      )
   {
      return true;
   }
   else
   {
      return false;
   }
}

inline bool IsJointSpacing(pgsTypes::SupportedBeamSpacing sbs)
{
   return !IsGirderSpacing(sbs);
}

inline bool IsTopWidthSpacing(pgsTypes::SupportedBeamSpacing sbs)
{
   return (sbs == pgsTypes::sbsUniformAdjacentWithTopWidth || sbs == pgsTypes::sbsGeneralAdjacentWithTopWidth) ? true : false;
}

inline bool IsBridgeSpacing(pgsTypes::SupportedBeamSpacing sbs)
{
   // spacing type is for the whole bridge and not span-by-span
   if (sbs == pgsTypes::sbsUniform ||
      sbs == pgsTypes::sbsUniformAdjacent ||
      sbs == pgsTypes::sbsConstantAdjacent ||
      sbs == pgsTypes::sbsUniformAdjacentWithTopWidth
      )
   {
      return true;
   }
   else
   {
      return false;
   }
}

inline bool IsNonstructuralDeck(pgsTypes::SupportedDeckType deckType)
{
   return (deckType == pgsTypes::sdtNone || deckType == pgsTypes::sdtNonstructuralOverlay);
}

inline bool IsStructuralDeck(pgsTypes::SupportedDeckType deckType)
{
   return !IsNonstructuralDeck(deckType);
}

inline bool IsOverlayDeck(pgsTypes::SupportedDeckType deckType)
{
   return (deckType == pgsTypes::sdtCompositeOverlay || deckType == pgsTypes::sdtNonstructuralOverlay);
}

inline bool IsConstantWidthDeck(pgsTypes::SupportedDeckType deckType)
{
   return (deckType == pgsTypes::sdtNone || IsOverlayDeck(deckType));
}

inline bool IsAdjustableWidthDeck(pgsTypes::SupportedDeckType deckType)
{
   return !IsConstantWidthDeck(deckType);
}

inline bool IsCastDeck(pgsTypes::SupportedDeckType deckType)
{
   return (deckType == pgsTypes::sdtCompositeCIP || deckType == pgsTypes::sdtCompositeSIP);
}

inline bool IsSpanSpacing(pgsTypes::SupportedBeamSpacing sbs)
{
   return !IsBridgeSpacing(sbs);
}

inline bool IsSpreadSpacing(pgsTypes::SupportedBeamSpacing sbs)
{
   // spacing type is for spread beams
   if (sbs == pgsTypes::sbsUniform || sbs == pgsTypes::sbsGeneral )
   {
      return true;
   }
   else
   {
      return false;
   }
}

inline bool IsAdjacentSpacing(pgsTypes::SupportedBeamSpacing sbs)
{
   // spacing type is for adjacent beams
   return !IsSpreadSpacing(sbs);
}

inline pgsTypes::LiveLoadType LiveLoadTypeFromLimitState(pgsTypes::LimitState ls)
{
   pgsTypes::LiveLoadType llType;
   switch(ls)
   {
   case pgsTypes::ServiceI:
   case pgsTypes::ServiceIA:
   case pgsTypes::ServiceIII:
   case pgsTypes::StrengthI:
   case pgsTypes::StrengthI_Inventory:
   case pgsTypes::StrengthI_Operating:
   case pgsTypes::ServiceIII_Inventory:
   case pgsTypes::ServiceIII_Operating:
      llType = pgsTypes::lltDesign;
      break;

   case pgsTypes::StrengthII:
      llType = pgsTypes::lltPermit;
      break;

   case pgsTypes::FatigueI:
      llType = pgsTypes::lltFatigue;
      break;

   case pgsTypes::StrengthI_LegalRoutine:
   case pgsTypes::ServiceIII_LegalRoutine:
      llType = pgsTypes::lltLegalRating_Routine;
      break;

   case pgsTypes::StrengthI_LegalSpecial:
   case pgsTypes::ServiceIII_LegalSpecial:
      llType = pgsTypes::lltLegalRating_Special;
      break;

   case pgsTypes::StrengthI_LegalEmergency:
   case pgsTypes::ServiceIII_LegalEmergency:
      llType = pgsTypes::lltLegalRating_Emergency;
      break;

   case pgsTypes::StrengthII_PermitRoutine:
   case pgsTypes::ServiceI_PermitRoutine:
   case pgsTypes::ServiceIII_PermitRoutine:
      llType = pgsTypes::lltPermitRating_Routine;
      break;

   case pgsTypes::StrengthII_PermitSpecial:
   case pgsTypes::ServiceI_PermitSpecial:
   case pgsTypes::ServiceIII_PermitSpecial:
      llType = pgsTypes::lltPermitRating_Special;
      break;

   default:
      ATLASSERT(false); // should never get here
   }

   return llType;
}

inline bool IsRatingLimitState(pgsTypes::LimitState ls)
{
   if ( ls == pgsTypes::StrengthI_Inventory ||
        ls == pgsTypes::StrengthI_Operating ||
        ls == pgsTypes::ServiceIII_Inventory ||
        ls == pgsTypes::ServiceIII_Operating ||
        ls == pgsTypes::StrengthI_LegalRoutine ||
        ls == pgsTypes::StrengthI_LegalSpecial ||
        ls == pgsTypes::StrengthI_LegalEmergency ||
        ls == pgsTypes::ServiceIII_LegalRoutine ||
        ls == pgsTypes::ServiceIII_LegalSpecial ||
        ls == pgsTypes::ServiceIII_LegalEmergency ||
        ls == pgsTypes::StrengthII_PermitRoutine ||
        ls == pgsTypes::ServiceI_PermitRoutine ||
        ls == pgsTypes::ServiceIII_PermitRoutine ||
        ls == pgsTypes::StrengthII_PermitSpecial ||
        ls == pgsTypes::ServiceI_PermitSpecial ||
        ls == pgsTypes::ServiceIII_PermitSpecial
     )
   {
      return true;
   }
   else
   {
      return false;
   }
}

inline bool IsDesignLimitState(pgsTypes::LimitState ls)
{
   if (ls == pgsTypes::ServiceI ||
      ls == pgsTypes::ServiceIA ||
      ls == pgsTypes::ServiceIII ||
      ls == pgsTypes::StrengthI ||
      ls == pgsTypes::StrengthII ||
      ls == pgsTypes::FatigueI)
   {
      ATLASSERT(!IsRatingLimitState(ls));
      return true;
   }
   return false;
}

inline bool IsStrengthILimitState(pgsTypes::LimitState ls)
{
   if ( ls == pgsTypes::StrengthI              || 
        ls == pgsTypes::StrengthI_Inventory    ||
        ls == pgsTypes::StrengthI_Operating    ||
        ls == pgsTypes::StrengthI_LegalRoutine ||
        ls == pgsTypes::StrengthI_LegalSpecial ||
        ls == pgsTypes::StrengthI_LegalEmergency
      )
   {
      return true;
   }
   else
   {
      return false;
   }
}

inline bool IsStrengthIILimitState(pgsTypes::LimitState ls)
{
   if ( ls == pgsTypes::StrengthII             ||
        ls == pgsTypes::StrengthII_PermitRoutine ||
        ls == pgsTypes::StrengthII_PermitSpecial 
      )
   {
      return true;
   }
   else
   {
      return false;
   }
}

inline bool IsStrengthLimitState(pgsTypes::LimitState ls)
{
   if ( ls == pgsTypes::StrengthI              || 
        ls == pgsTypes::StrengthII             ||
        ls == pgsTypes::StrengthI_Inventory    ||
        ls == pgsTypes::StrengthI_Operating    ||
        ls == pgsTypes::StrengthI_LegalRoutine ||
        ls == pgsTypes::StrengthI_LegalSpecial ||
        ls == pgsTypes::StrengthI_LegalEmergency ||
        ls == pgsTypes::StrengthII_PermitRoutine ||
        ls == pgsTypes::StrengthII_PermitSpecial 
      )
   {
      return true;
   }
   else
   {
      return false;
   }
}

inline bool IsFatigueLimitState(pgsTypes::LimitState ls)
{
   return (ls == pgsTypes::FatigueI || ls == pgsTypes::ServiceIA);
}

inline bool IsServiceLimitState(pgsTypes::LimitState ls)
{
   return !IsStrengthLimitState(ls) && !IsFatigueLimitState(ls);
}

inline bool IsServiceILimitState(pgsTypes::LimitState ls)
{
   return (ls == pgsTypes::ServiceI ||
      ls == pgsTypes::ServiceI_PermitRoutine ||
      ls == pgsTypes::ServiceI_PermitSpecial ) ? true : false;
}

inline bool IsServiceIIILimitState(pgsTypes::LimitState ls)
{
   return (ls == pgsTypes::ServiceIII ||
           ls == pgsTypes::ServiceIII_Inventory ||
           ls == pgsTypes::ServiceIII_Operating ||
           ls == pgsTypes::ServiceIII_LegalRoutine ||
           ls == pgsTypes::ServiceIII_LegalSpecial ||
           ls == pgsTypes::ServiceIII_LegalEmergency ||
           ls == pgsTypes::ServiceIII_PermitRoutine ||
           ls == pgsTypes::ServiceIII_PermitSpecial) ? true : false;
}

#include <LRFD\LrfdTypes.h>
inline lrfdTypes::LimitState PGSLimitStateToLRFDLimitState(pgsTypes::LimitState ls)
{
   lrfdTypes::LimitState lrfdLimitState;
   if (IsStrengthILimitState(ls))
   {
      lrfdLimitState = lrfdTypes::StrengthI;
   }
   else if (IsStrengthIILimitState(ls))
   {
      lrfdLimitState = lrfdTypes::StrengthII;
   }
   else if (IsServiceILimitState(ls))
   {
      lrfdLimitState = lrfdTypes::ServiceI;
   }
   else if (IsServiceIIILimitState(ls))
   {
      lrfdLimitState = lrfdTypes::ServiceIII;
   }
   else if (ls == pgsTypes::ServiceIA)
   {
      lrfdLimitState = lrfdTypes::ServiceIA;
   }
   else if (ls == pgsTypes::FatigueI)
   {
      lrfdLimitState = lrfdTypes::FatigueI;
   }
   else
   {
      ATLASSERT(false);
   }

   return lrfdLimitState;
}

inline bool IsRatingLiveLoad(pgsTypes::LiveLoadType llType)
{
   if ( llType == pgsTypes::lltDesign              || // doubles as a design and rating live load
        llType == pgsTypes::lltLegalRating_Routine ||
        llType == pgsTypes::lltLegalRating_Special ||
        llType == pgsTypes::lltLegalRating_Emergency ||
        llType == pgsTypes::lltPermitRating_Routine ||
        llType == pgsTypes::lltPermitRating_Special
      )
   {
      return true;
   }
   else
   {
      return false;
   }
}

inline bool IsDesignLiveLoad(pgsTypes::LiveLoadType llType)
{
   if ( llType == pgsTypes::lltDesign )
      return true;
   else
      return !IsRatingLiveLoad(llType);
}

inline std::vector<pgsTypes::LimitState> GetRatingLimitStates(pgsTypes::LoadRatingType ratingType)
{
   std::vector<pgsTypes::LimitState> vLimitStates;
   switch (ratingType)
   {
   case pgsTypes::lrDesign_Inventory:
      vLimitStates.push_back(pgsTypes::ServiceIII_Inventory);
      vLimitStates.push_back(pgsTypes::StrengthI_Inventory);
      break;
   case pgsTypes::lrDesign_Operating:
      vLimitStates.push_back(pgsTypes::ServiceIII_Operating);
      vLimitStates.push_back(pgsTypes::StrengthI_Operating);
      break;
   case pgsTypes::lrLegal_Routine:
      vLimitStates.push_back(pgsTypes::ServiceIII_LegalRoutine);
      vLimitStates.push_back(pgsTypes::StrengthI_LegalRoutine);
      break;
   case pgsTypes::lrLegal_Special:
      vLimitStates.push_back(pgsTypes::ServiceIII_LegalSpecial);
      vLimitStates.push_back(pgsTypes::StrengthI_LegalSpecial);
      break;
   case pgsTypes::lrLegal_Emergency:
      vLimitStates.push_back(pgsTypes::ServiceIII_LegalEmergency);
      vLimitStates.push_back(pgsTypes::StrengthI_LegalEmergency);
      break;
   case pgsTypes::lrPermit_Routine:
      vLimitStates.push_back(pgsTypes::ServiceI_PermitRoutine);
      vLimitStates.push_back(pgsTypes::ServiceIII_PermitRoutine);
      vLimitStates.push_back(pgsTypes::StrengthII_PermitRoutine);
      break;
   case pgsTypes::lrPermit_Special:
      vLimitStates.push_back(pgsTypes::ServiceI_PermitSpecial);
      vLimitStates.push_back(pgsTypes::ServiceIII_PermitSpecial);
      vLimitStates.push_back(pgsTypes::StrengthII_PermitSpecial);
      break;
   default:
      ATLASSERT(false); // is there a new rating type???
   }
   return vLimitStates;
}

inline pgsTypes::LoadRatingType RatingTypeFromLimitState(pgsTypes::LimitState ls)
{
   pgsTypes::LoadRatingType ratingType;
   switch(ls)
   {
   case pgsTypes::StrengthI_Inventory:
   case pgsTypes::ServiceIII_Inventory:
      ratingType = pgsTypes::lrDesign_Inventory;
      break;

   case pgsTypes::StrengthI_Operating:
   case pgsTypes::ServiceIII_Operating:
      ratingType = pgsTypes::lrDesign_Operating;
      break;

   case pgsTypes::StrengthI_LegalRoutine:
   case pgsTypes::ServiceIII_LegalRoutine:
      ratingType = pgsTypes::lrLegal_Routine;
      break;

   case pgsTypes::StrengthI_LegalSpecial:
   case pgsTypes::ServiceIII_LegalSpecial:
      ratingType = pgsTypes::lrLegal_Special;
      break;

   case pgsTypes::StrengthI_LegalEmergency:
   case pgsTypes::ServiceIII_LegalEmergency:
      ratingType = pgsTypes::lrLegal_Emergency;
      break;

   case pgsTypes::StrengthII_PermitRoutine:
   case pgsTypes::ServiceI_PermitRoutine:
   case pgsTypes::ServiceIII_PermitRoutine:
      ratingType = pgsTypes::lrPermit_Routine;
      break;

   case pgsTypes::StrengthII_PermitSpecial:
   case pgsTypes::ServiceI_PermitSpecial:
   case pgsTypes::ServiceIII_PermitSpecial:
      ratingType = pgsTypes::lrPermit_Special;
      break;

   default:
      ATLASSERT(false); // either there is a new rating type or you used a design limit state
      ratingType = pgsTypes::lrDesign_Inventory;
   }

   return ratingType;
}

inline pgsTypes::LimitState GetStrengthLimitStateType(pgsTypes::LoadRatingType ratingType)
{
   pgsTypes::LimitState ls;
   switch(ratingType)
   {
   case pgsTypes::lrDesign_Inventory:
      ls = pgsTypes::StrengthI_Inventory;
      break;

   case pgsTypes::lrDesign_Operating:
      ls = pgsTypes::StrengthI_Operating;
      break;

   case pgsTypes::lrLegal_Routine:
      ls = pgsTypes::StrengthI_LegalRoutine;
      break;

   case pgsTypes::lrLegal_Special:
      ls = pgsTypes::StrengthI_LegalSpecial;
      break;

   case pgsTypes::lrLegal_Emergency:
      ls = pgsTypes::StrengthI_LegalEmergency;
      break;

   case pgsTypes::lrPermit_Routine:
      ls = pgsTypes::StrengthII_PermitRoutine;
      break;

   case pgsTypes::lrPermit_Special:
      ls = pgsTypes::StrengthII_PermitSpecial;
      break;

   default:
      ATLASSERT(false); // SHOULD NEVER GET HERE (unless there is a new rating type)
   }

   return ls;
}

inline pgsTypes::LimitState GetServiceLimitStateType(pgsTypes::LoadRatingType ratingType)
{
   pgsTypes::LimitState ls;
   switch(ratingType)
   {
   case pgsTypes::lrDesign_Inventory:
      ls = pgsTypes::ServiceIII_Inventory;
      break;

   case pgsTypes::lrDesign_Operating:
      ls = pgsTypes::ServiceIII_Operating;
      break;

   case pgsTypes::lrLegal_Routine:
      ls = pgsTypes::ServiceIII_LegalRoutine;
      break;

   case pgsTypes::lrLegal_Special:
      ls = pgsTypes::ServiceIII_LegalSpecial;
      break;

   case pgsTypes::lrLegal_Emergency:
      ls = pgsTypes::ServiceIII_LegalEmergency;
      break;

   case pgsTypes::lrPermit_Routine:
      ls = pgsTypes::ServiceIII_PermitRoutine;
      break;

   case pgsTypes::lrPermit_Special:
      ls = pgsTypes::ServiceIII_PermitSpecial;
      break;

   default:
      ATLASSERT(false); // SHOULD NEVER GET HERE (unless there is a new rating type)
   }

   return ls;
}

inline pgsTypes::LiveLoadType GetLiveLoadType(pgsTypes::LoadRatingType ratingType)
{
   pgsTypes::LiveLoadType llType;
   switch( ratingType )
   {
   case pgsTypes::lrDesign_Inventory:
   case pgsTypes::lrDesign_Operating:
      llType = pgsTypes::lltDesign;
      break;

   case pgsTypes::lrLegal_Routine:
      llType = pgsTypes::lltLegalRating_Routine;
      break;

   case pgsTypes::lrLegal_Special:
      llType = pgsTypes::lltLegalRating_Special;
      break;

   case pgsTypes::lrLegal_Emergency:
      llType = pgsTypes::lltLegalRating_Emergency;
      break;

   case pgsTypes::lrPermit_Routine:
      llType = pgsTypes::lltPermitRating_Routine;
      break;

   case pgsTypes::lrPermit_Special:
      llType = pgsTypes::lltPermitRating_Special;
      break;

   default:
      ATLASSERT(false); // SHOULD NEVER GET HERE
   }

   return llType;
}

inline bool IsDesignRatingType(pgsTypes::LoadRatingType ratingType)
{
   return (ratingType == pgsTypes::lrDesign_Inventory || ratingType == pgsTypes::lrDesign_Operating) ? true : false;
}

inline bool IsLegalRatingType(pgsTypes::LoadRatingType ratingType)
{
   return (ratingType == pgsTypes::lrLegal_Routine || ratingType == pgsTypes::lrLegal_Special || ratingType == pgsTypes::lrLegal_Emergency) ? true : false;
}

inline bool IsEmergencyRatingType(pgsTypes::LoadRatingType ratingType)
{
   return (ratingType == pgsTypes::lrLegal_Emergency ? true : false);
}

inline bool IsPermitRatingType(pgsTypes::LoadRatingType ratingType)
{
   return (ratingType == pgsTypes::lrPermit_Routine || ratingType == pgsTypes::lrPermit_Special) ? true : false;
}

inline bool IsTopStressLocation(pgsTypes::StressLocation stressLocation)
{
   return (stressLocation == pgsTypes::TopDeck || stressLocation == pgsTypes::TopGirder);
}

inline bool IsBottomStressLocation(pgsTypes::StressLocation stressLocation)
{
   return !IsTopStressLocation(stressLocation);
}

inline bool IsGirderStressLocation(pgsTypes::StressLocation stressLocation)
{
   return (stressLocation == pgsTypes::TopGirder || stressLocation == pgsTypes::BottomGirder);
}

inline bool IsDeckStressLocation(pgsTypes::StressLocation stressLocation)
{
   return !IsGirderStressLocation(stressLocation);
}

inline bool IsNoDeckBoundaryCondition(pgsTypes::BoundaryConditionType bc)
{
   // All "before deck" boundary conditions are not "No Deck" boundary conditions
   return (bc == pgsTypes::bctHinge ||
      bc == pgsTypes::bctRoller ||
      bc == pgsTypes::bctContinuousAfterDeck ||
      bc == pgsTypes::bctIntegralAfterDeck ||
      bc == pgsTypes::bctIntegralAfterDeckHingeBack ||
      bc == pgsTypes::bctIntegralAfterDeckHingeAhead);
}

inline pgsTypes::BoundaryConditionType GetNoDeckBoundaryCondition(pgsTypes::BoundaryConditionType bc)
{
   // convert the provided boundary condition to its equivalent no deck boundary condition
   switch (bc)
   {
   case pgsTypes::bctContinuousBeforeDeck:
      bc = pgsTypes::bctContinuousAfterDeck;
      break;

   case pgsTypes::bctIntegralBeforeDeck:
      bc = pgsTypes::bctIntegralAfterDeck;
      break;

   case pgsTypes::bctIntegralBeforeDeckHingeBack:
      bc = pgsTypes::bctIntegralAfterDeckHingeBack;
      break;

   case pgsTypes::bctIntegralBeforeDeckHingeAhead:
      bc = pgsTypes::bctIntegralAfterDeckHingeAhead;
      break;
   }

   return bc;
}

inline bool IsContinuousBoundaryCondition(pgsTypes::BoundaryConditionType bc)
{
   if (bc == pgsTypes::bctContinuousAfterDeck || bc == pgsTypes::bctContinuousBeforeDeck ||
      bc == pgsTypes::bctIntegralAfterDeck || bc == pgsTypes::bctIntegralBeforeDeck)
   {
      return true;
   }
   else
   {
      return false;
   }
}

inline bool IsContinuousBoundaryConditionAheadSide(pgsTypes::BoundaryConditionType bc)
{
   return IsContinuousBoundaryCondition(bc) || bc == pgsTypes::bctIntegralBeforeDeckHingeBack || bc == pgsTypes::bctIntegralAfterDeckHingeBack;
}

inline bool IsContinuousBoundaryConditionBackSide(pgsTypes::BoundaryConditionType bc)
{
   return IsContinuousBoundaryCondition(bc) || bc == pgsTypes::bctIntegralBeforeDeckHingeAhead || bc == pgsTypes::bctIntegralAfterDeckHingeAhead;
}

inline bool IsContinuousBoundaryCondition(pgsTypes::PierFaceType pierFace, pgsTypes::BoundaryConditionType bc)
{
   return (pierFace == pgsTypes::Ahead ? IsContinuousBoundaryConditionAheadSide(bc) : IsContinuousBoundaryConditionBackSide(bc));
}

inline bool IsParabolicVariation(pgsTypes::SegmentVariationType variationType)
{
   return (variationType == pgsTypes::svtParabolic || variationType == pgsTypes::svtDoubleParabolic ? true : false);
}


inline bool IsSegmentContinuousOverPier(pgsTypes::PierSegmentConnectionType connectionType)
{
   return (connectionType == pgsTypes::psctContinuousSegment || connectionType == pgsTypes::psctIntegralSegment) ? true : false;
}

inline bool IsGridBasedStrandModel(pgsTypes::StrandDefinitionType strandModelType)
{
   return (strandModelType == pgsTypes::sdtTotal || strandModelType == pgsTypes::sdtStraightHarped || strandModelType == pgsTypes::sdtDirectSelection) ? true : false;
}

inline bool IsDirectStrandModel(pgsTypes::StrandDefinitionType strandModelType)
{
   return (strandModelType == pgsTypes::sdtDirectRowInput || strandModelType == pgsTypes::sdtDirectStrandInput) ? true : false;
}

inline bool IsLNWC(pgsTypes::ConcreteType type)
{
   return type == pgsTypes::Normal;
}

inline bool IsLWC(pgsTypes::ConcreteType type)
{
   return (type == pgsTypes::AllLightweight || type == pgsTypes::SandLightweight);
}

inline bool IsUHPC(pgsTypes::ConcreteType type)
{
   return (type == pgsTypes::PCI_UHPC || type == pgsTypes::UHPC) ? true : false;
}

#endif // INCLUDED_PGSUPERTYPES_H_