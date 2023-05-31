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

#include "StdAfx.h"
#include <psgLib\SpecLibraryEntry.h>
#include "resource.h"
#include "SpecMainSheet.h"

#include <System\IStructuredSave.h>
#include <System\IStructuredLoad.h>
#include <System\XStructuredLoad.h>
#include <Units\Convert.h>

#include <MathEx.h>

#include <EAF\EAFApp.h>
#include <psgLib\LibraryEntryDifferenceItem.h>

#include <boost\algorithm\string\replace.hpp>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// During the development of PGSplice, there was an overlap in version numbers between the
// 2.9 and 3.0 branches. It is ok for loads to fail for 44.0 <= version <= MAX_OVERLAP_VERSION.
#define MAX_OVERLAP_VERSION 53.0 // overlap of data blocks between PGS 2.9 and 3.0 end with this version

// The develop (patches) branch started at version 64. We need to make room so
// the version number can increment. Jump our version number to 70
#define CURRENT_VERSION 82.0 

/****************************************************************************
CLASS
   SpecLibraryEntry
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
SpecLibraryEntry::SpecLibraryEntry() :
m_bUseCurrentSpecification(true),
m_SpecificationType(lrfdVersionMgr::NinthEdition2020),
m_SpecificationUnits(lrfdVersionMgr::US),
m_SectionPropertyMode(pgsTypes::spmGross),
m_DoCheckStrandSlope(true),
m_DoDesignStrandSlope(true),
m_MaxSlope05(6),
m_MaxSlope06(8),
m_MaxSlope07(10),
m_DoCheckHoldDown(false),
m_DoDesignHoldDown(false),
m_HoldDownForceType(HOLD_DOWN_TOTAL),
m_HoldDownForce(ConvertToSysUnits(45,WBFL::Units::Measure::Kip)),
m_HoldDownFriction(0.0),
m_bCheckHandlingWeightLimit(false),
m_HandlingWeightLimit(ConvertToSysUnits(130,WBFL::Units::Measure::Kip)),
m_DoCheckSplitting(true),
m_DoCheckConfinement(true),
m_DoDesignSplitting(true),
m_DoDesignConfinement(true),
m_CyLiftingCrackFs(1.0),
m_CyLiftingFailFs(1.5),
m_CyCompStressServ(0.45),
m_LiftingCompressionStressCoefficient_GlobalStress(0.65),
m_LiftingCompressionStressCoefficient_PeakStress(0.70),
m_CyTensStressServ(WBFL::Units::ConvertToSysUnits(0.0948,WBFL::Units::Measure::SqrtKSI)),
m_CyDoTensStressServMax(true),
m_CyTensStressServMax(WBFL::Units::ConvertToSysUnits(0.200,WBFL::Units::Measure::KSI)),
m_CyTensStressLifting(0),
m_CyDoTensStressLiftingMax(false),
m_CyTensStressLiftingMax(0),
m_CyTensStressServWithRebar( WBFL::Units::ConvertToSysUnits(0.24,WBFL::Units::Measure::SqrtKSI)),
m_TensStressLiftingWithRebar(WBFL::Units::ConvertToSysUnits(0.24,WBFL::Units::Measure::SqrtKSI)),
m_TensStressHaulingWithRebar{ WBFL::Units::ConvertToSysUnits(0.24,WBFL::Units::Measure::SqrtKSI),WBFL::Units::ConvertToSysUnits(0.24,WBFL::Units::Measure::SqrtKSI) },
m_SplittingZoneLengthFactor(4.0),
m_LiftingUpwardImpact(0),
m_LiftingDownwardImpact(0),
m_HaulingUpwardImpact(0),
m_HaulingDownwardImpact(0),
m_CuringMethod(CURING_ACCELERATED),
m_EnableLiftingCheck(true),
m_EnableLiftingDesign(true),
m_PickPointHeight(0),
m_MaxGirderSweepLifting(WBFL::Units::ConvertToSysUnits(1./16.,WBFL::Units::Measure::Inch)/WBFL::Units::ConvertToSysUnits(10.0,WBFL::Units::Measure::Feet)),
m_EnableHaulingCheck(true),
m_EnableHaulingDesign(true),
m_HaulingAnalysisMethod(pgsTypes::hmWSDOT),
m_MaxGirderSweepHauling(WBFL::Units::ConvertToSysUnits(1. / 8., WBFL::Units::Measure::Inch) / WBFL::Units::ConvertToSysUnits(10.0, WBFL::Units::Measure::Feet)),
m_HaulingSweepGrowth(0.0), // PCI's value is 1.0", but we've never used this before so we'll default to 0.0
m_HaulingSupportPlacementTolerance(ConvertToSysUnits(1.0,WBFL::Units::Measure::Inch)),
m_LiftingLoopTolerance(ConvertToSysUnits(1.0,WBFL::Units::Measure::Inch)),
m_MinCableInclination(ConvertToSysUnits(90.,WBFL::Units::Measure::Degree)),
m_GlobalCompStressHauling(0.6),
m_PeakCompStressHauling(0.6),
m_TensStressHauling{ WBFL::Units::ConvertToSysUnits(0.0948,WBFL::Units::Measure::SqrtKSI),WBFL::Units::ConvertToSysUnits(0.24,WBFL::Units::Measure::SqrtKSI) },
m_DoTensStressHaulingMax{ false,false },
m_TensStressHaulingMax{ 0,WBFL::Units::ConvertToSysUnits(0.2,WBFL::Units::Measure::KSI) },
m_HaulingCrackFs(1.0),
m_HaulingRollFs(1.5),
m_bHasOldHaulTruck(false),
m_HaulingImpactUsage(pgsTypes::NormalCrown),
m_RoadwayCrownSlope(0.0),
m_RoadwaySuperelevation(0.06),
m_TempStrandRemovalCompStress(0.45),
m_TempStrandRemovalTensStress(WBFL::Units::ConvertToSysUnits(0.19,WBFL::Units::Measure::SqrtKSI)),
m_TempStrandRemovalTensStressWithRebar(WBFL::Units::ConvertToSysUnits(0.24,WBFL::Units::Measure::SqrtKSI)),
m_TempStrandRemovalDoTensStressMax(false),
m_TempStrandRemovalTensStressMax(WBFL::Units::ConvertToSysUnits(0.2,WBFL::Units::Measure::KSI)),
m_bCheckTemporaryStresses(true), // true is consistent with the original default value
m_Bs1CompStress(0.6),
m_Bs1TensStress(WBFL::Units::ConvertToSysUnits(0.19,WBFL::Units::Measure::SqrtKSI)),
m_Bs1DoTensStressMax(false),
m_Bs1TensStressMax(ConvertToSysUnits(0.2,WBFL::Units::Measure::KSI)),
m_Bs2CompStress(0.6),
m_bCheckBs2Tension(false), // false is consistent with the original features of the program (it didn't do this)
m_Bs2TensStress(0.0),
m_Bs2DoTensStressMax(false),
m_Bs2TensStressMax(ConvertToSysUnits(0.2,WBFL::Units::Measure::KSI)),
m_TrafficBarrierDistributionType(pgsTypes::tbdGirder),
m_Bs2MaxGirdersTrafficBarrier(4),
m_OverlayLoadDistribution(pgsTypes::olDistributeEvenly),
m_Bs3CompStressServ(0.6),
m_Bs3CompStressService1A(0.4),
m_Bs3TensStressServNc(0),
m_Bs3DoTensStressServNcMax(false),
m_Bs3TensStressServNcMax(ConvertToSysUnits(0.2,WBFL::Units::Measure::KSI)),
m_Bs3TensStressServSc(0),   
m_Bs3DoTensStressServScMax(false),
m_Bs3TensStressServScMax(ConvertToSysUnits(0.2,WBFL::Units::Measure::KSI)),
m_Bs3IgnoreRangeOfApplicability(false),
m_Bs3LRFDOverReinforcedMomentCapacity(0),
m_CreepMethod(CREEP_LRFD),
m_XferTime(WBFL::Units::ConvertToSysUnits(24,WBFL::Units::Measure::Hour)),
m_CreepFactor(2.00),
m_CreepDuration1Min(WBFL::Units::ConvertToSysUnits(10,WBFL::Units::Measure::Day)),
m_CreepDuration2Min(WBFL::Units::ConvertToSysUnits(40,WBFL::Units::Measure::Day)),
m_CreepDuration1Max(WBFL::Units::ConvertToSysUnits(90,WBFL::Units::Measure::Day)),
m_CreepDuration2Max(WBFL::Units::ConvertToSysUnits(120,WBFL::Units::Measure::Day)),
m_TotalCreepDuration(WBFL::Units::ConvertToSysUnits(2000,WBFL::Units::Measure::Day)),
m_CamberVariability(0.50),
m_LossMethod(LOSSES_AASHTO_REFINED),
m_BeforeXferLosses(0),
m_AfterXferLosses(0),
m_LiftingLosses(0),
m_TimeDependentModel(TDM_AASHTO),
m_ShippingLosses(WBFL::Units::ConvertToSysUnits(20,WBFL::Units::Measure::KSI)),
m_FinalLosses(0),
m_ShippingTime(WBFL::Units::ConvertToSysUnits(10,WBFL::Units::Measure::Day)),
m_LldfMethod(LLDF_LRFD),
m_bIgnoreSkewReductionForMoment(false),
m_bUseRigidMethod(false),
m_BeforeTempStrandRemovalLosses(0),
m_AfterTempStrandRemovalLosses(0),
m_AfterDeckPlacementLosses(0),
m_AfterSIDLLosses(0),
m_bUpdatePTParameters(false),
m_Dset(WBFL::Units::ConvertToSysUnits(0.375,WBFL::Units::Measure::Inch)),
m_WobbleFriction(WBFL::Units::ConvertToSysUnits(0.0002,WBFL::Units::Measure::PerFeet)),
m_FrictionCoefficient(0.25),
m_SlabElasticGain(1.0),
m_SlabPadElasticGain(1.0),
m_DiaphragmElasticGain(1.0),
m_UserDCElasticGainBS1(1.0),
m_UserDWElasticGainBS1(1.0),
m_UserDCElasticGainBS2(1.0),
m_UserDWElasticGainBS2(1.0),
m_RailingSystemElasticGain(1.0),
m_OverlayElasticGain(1.0),
m_SlabShrinkageElasticGain(1.0),
m_LiveLoadElasticGain(0.0),
m_LongReinfShearMethod(LRFD_METHOD),
m_bDoEvaluateDeflection(true),
m_DeflectionLimit(800.0),
m_bIncludeStrand_NegMoment(false),
m_bConsiderReinforcementStrainLimit(false),
m_nMomentCapacitySlices(10),
m_bIncludeRebar_Moment(false),
m_bIncludeRebar_Shear(false),
m_AnalysisType(pgsTypes::Envelope),
m_EnableSlabOffsetCheck(true),
m_EnableSlabOffsetDesign(true),
m_DesignStrandFillType(ftMinimizeHarping),
m_EffFlangeWidthMethod(pgsTypes::efwmLRFD),
m_ShearFlowMethod(pgsTypes::sfmClassical),
m_MaxInterfaceShearConnectorSpacing(WBFL::Units::ConvertToSysUnits(48.0,WBFL::Units::Measure::Inch)),
m_bUseDeckWeightForPc(true),
m_ShearCapacityMethod(pgsTypes::scmBTEquations),
m_bLimitNetTensionStrainToPositiveValues(false),
m_CuringMethodTimeAdjustmentFactor(7),
m_MinLiftPoint(-1), // H
m_LiftPointAccuracy(WBFL::Units::ConvertToSysUnits(0.25,WBFL::Units::Measure::Feet)),
m_MinHaulPoint(-1), // H
m_HaulPointAccuracy(WBFL::Units::ConvertToSysUnits(0.5,WBFL::Units::Measure::Feet)),
m_UseMinTruckSupportLocationFactor(true),
m_MinTruckSupportLocationFactor(0.1),
m_OverhangGFactor(3.0),
m_InteriorGFactor(1.0),
m_PedestrianLoad(WBFL::Units::ConvertToSysUnits(0.075,WBFL::Units::Measure::KSF)),
m_MinSidewalkWidth(WBFL::Units::ConvertToSysUnits(2.0,WBFL::Units::Measure::Feet)),
m_MaxAngularDeviationBetweenGirders(WBFL::Units::ConvertToSysUnits(5.0,WBFL::Units::Measure::Degree)),
m_MinGirderStiffnessRatio(0.90),
m_LLDFGirderSpacingLocation(0.75),
m_bIncludeDualTandem(true),
m_LimitDistributionFactorsToLanesBeams(false),
m_ExteriorLiveLoadDistributionGTAdjacentInteriorRule(false),
m_PrestressTransferComputationType(pgsTypes::ptUsingSpecification),
m_bIncludeForNegMoment(true),
m_bAllowStraightStrandExtensions(false),
m_RelaxationLossMethod(RLM_REFINED),
m_FcgpComputationMethod(FCGP_07FPU),
m_ClosureCompStressAtStressing(0.60),
m_ClosureTensStressPTZAtStressing(0.0),
m_ClosureTensStressPTZWithRebarAtStressing(WBFL::Units::ConvertToSysUnits(0.0948,WBFL::Units::Measure::SqrtKSI)),
m_ClosureTensStressAtStressing(0.0),
m_ClosureTensStressWithRebarAtStressing(WBFL::Units::ConvertToSysUnits(0.19,WBFL::Units::Measure::SqrtKSI)),
m_ClosureCompStressAtService(0.45),
m_ClosureCompStressWithLiveLoadAtService(0.60),
m_ClosureTensStressPTZAtService(0.0),
m_ClosureTensStressPTZWithRebarAtService(WBFL::Units::ConvertToSysUnits(0.0948,WBFL::Units::Measure::SqrtKSI)),
m_ClosureTensStressAtService(0.0),
m_ClosureTensStressWithRebarAtService(WBFL::Units::ConvertToSysUnits(0.19,WBFL::Units::Measure::SqrtKSI)),
m_ClosureCompStressFatigue(0.40),
m_bCheckBottomFlangeClearance(false),
m_Cmin(WBFL::Units::ConvertToSysUnits(1.75,WBFL::Units::Measure::Feet)),
m_DuctAreaPushRatio(2),
m_DuctAreaPullRatio(2.5),
m_DuctDiameterRatio(0.4),
m_LimitStateConcreteStrength(pgsTypes::lscStrengthAtTimeOfLoading),
m_bUse90DayConcreteStrength(false),
m_90DayConcreteStrengthFactor(1.15),
m_HaunchLoadComputationType(pgsTypes::hlcZeroCamber),
m_HaunchLoadCamberTolerance(WBFL::Units::ConvertToSysUnits(0.5,WBFL::Units::Measure::Inch)),
m_HaunchLoadCamberFactor(1.0),
m_HaunchAnalysisSectionPropertiesType(pgsTypes::hspZeroHaunch),
m_LiftingWindType(pgsTypes::Speed),
m_LiftingWindLoad(0),
m_HaulingWindType(pgsTypes::Speed),
m_HaulingWindLoad(0),
m_CentrifugalForceType(pgsTypes::Favorable),
m_HaulingSpeed(0),
m_TurningRadius(WBFL::Units::ConvertToSysUnits(1000,WBFL::Units::Measure::Feet)),
m_bCheckGirderInclination(true),
m_InclinedGirder_FSmax(1.2),
m_LiftingCamberMultiplier(1.0),
m_HaulingCamberMultiplier(1.0),
m_FinishedElevationTolerance(WBFL::Units::ConvertToSysUnits(1.00,WBFL::Units::Measure::Inch)),
m_SlabOffsetRoundingMethod(pgsTypes::sormRoundNearest),
m_SlabOffsetRoundingTolerance(WBFL::Units::ConvertToSysUnits(0.25,WBFL::Units::Measure::Inch)),
m_PrincipalTensileStressMethod(pgsTypes::ptsmLRFD),
m_PrincipalTensileStressCoefficient(WBFL::Units::ConvertToSysUnits(0.110,WBFL::Units::Measure::SqrtKSI)),
m_PrincipalTensileStressTendonNearnessFactor(1.5),
m_PrincipalTensileStressUngroutedMultiplier(1.0),
m_PrincipalTensileStressGroutedMultiplier(0.0),
m_PrincipalTensileStressFcThreshold(WBFL::Units::ConvertToSysUnits(10.0,WBFL::Units::Measure::KSI)),
m_bAlertTaperedSolePlateRequirement(true),
m_TaperedSolePlateInclinationThreshold(0.01),
m_bUseImpactForBearingReactions(false)
{
   m_bCheckStrandStress[CSS_AT_JACKING]       = false;
   m_bCheckStrandStress[CSS_BEFORE_TRANSFER]  = true;
   m_bCheckStrandStress[CSS_AFTER_TRANSFER]   = false;
   m_bCheckStrandStress[CSS_AFTER_ALL_LOSSES] = true;

   m_StrandStressCoeff[CSS_AT_JACKING][STRESS_REL]       = 0.72;
   m_StrandStressCoeff[CSS_AT_JACKING][LOW_RELAX]        = 0.78;
   m_StrandStressCoeff[CSS_BEFORE_TRANSFER][STRESS_REL]  = 0.70;
   m_StrandStressCoeff[CSS_BEFORE_TRANSFER][LOW_RELAX]   = 0.75;
   m_StrandStressCoeff[CSS_AFTER_TRANSFER][STRESS_REL]   = 0.70;
   m_StrandStressCoeff[CSS_AFTER_TRANSFER][LOW_RELAX]    = 0.74;
   m_StrandStressCoeff[CSS_AFTER_ALL_LOSSES][STRESS_REL] = 0.80;
   m_StrandStressCoeff[CSS_AFTER_ALL_LOSSES][LOW_RELAX]  = 0.80;

   m_bUpdateLoadFactors = false;
   m_DCmin[pgsTypes::ServiceI]   = 1.0;         m_DCmax[pgsTypes::ServiceI]   = 1.0;
   m_DWmin[pgsTypes::ServiceI]   = 1.0;         m_DWmax[pgsTypes::ServiceI]   = 1.0;
   m_LLIMmin[pgsTypes::ServiceI] = 1.0;         m_LLIMmax[pgsTypes::ServiceI] = 1.0;

   m_DCmin[pgsTypes::ServiceIA]   = 0.5;        m_DCmax[pgsTypes::ServiceIA]   = 0.5;
   m_DWmin[pgsTypes::ServiceIA]   = 0.5;        m_DWmax[pgsTypes::ServiceIA]   = 0.5;
   m_LLIMmin[pgsTypes::ServiceIA] = 1.0;        m_LLIMmax[pgsTypes::ServiceIA] = 1.0;

   m_DCmin[pgsTypes::ServiceIII]   = 1.0;       m_DCmax[pgsTypes::ServiceIII]   = 1.0;
   m_DWmin[pgsTypes::ServiceIII]   = 1.0;       m_DWmax[pgsTypes::ServiceIII]   = 1.0;
   m_LLIMmin[pgsTypes::ServiceIII] = 0.8;       m_LLIMmax[pgsTypes::ServiceIII] = 0.8;

   m_DCmin[pgsTypes::StrengthI]   = 0.90;       m_DCmax[pgsTypes::StrengthI]   = 1.25;
   m_DWmin[pgsTypes::StrengthI]   = 0.65;       m_DWmax[pgsTypes::StrengthI]   = 1.50;
   m_LLIMmin[pgsTypes::StrengthI] = 1.75;       m_LLIMmax[pgsTypes::StrengthI] = 1.75;

   m_DCmin[pgsTypes::StrengthII]   = 0.90;      m_DCmax[pgsTypes::StrengthII]   = 1.25;
   m_DWmin[pgsTypes::StrengthII]   = 0.65;      m_DWmax[pgsTypes::StrengthII]   = 1.50;
   m_LLIMmin[pgsTypes::StrengthII] = 1.35;      m_LLIMmax[pgsTypes::StrengthII] = 1.35;

   m_DCmin[pgsTypes::FatigueI]   = 0.5;        m_DCmax[pgsTypes::FatigueI]   = 0.5;
   m_DWmin[pgsTypes::FatigueI]   = 0.5;        m_DWmax[pgsTypes::FatigueI]   = 0.5;
   m_LLIMmin[pgsTypes::FatigueI] = 1.5;        m_LLIMmax[pgsTypes::FatigueI] = 1.5;

   m_bCheckTendonStressAtJacking      = false;
   m_bCheckTendonStressPriorToSeating = true;
   m_TendonStressCoeff[CSS_AT_JACKING][STRESS_REL]               = 0.76;
   m_TendonStressCoeff[CSS_AT_JACKING][LOW_RELAX]                = 0.80;
   m_TendonStressCoeff[CSS_PRIOR_TO_SEATING][STRESS_REL]         = 0.90;
   m_TendonStressCoeff[CSS_PRIOR_TO_SEATING][LOW_RELAX]          = 0.90;
   m_TendonStressCoeff[CSS_ANCHORAGES_AFTER_SEATING][STRESS_REL] = 0.70;
   m_TendonStressCoeff[CSS_ANCHORAGES_AFTER_SEATING][LOW_RELAX]  = 0.70;
   m_TendonStressCoeff[CSS_ELSEWHERE_AFTER_SEATING][STRESS_REL]  = 0.70;
   m_TendonStressCoeff[CSS_ELSEWHERE_AFTER_SEATING][LOW_RELAX]   = 0.74;
   m_TendonStressCoeff[CSS_AFTER_ALL_LOSSES][STRESS_REL]         = 0.80;
   m_TendonStressCoeff[CSS_AFTER_ALL_LOSSES][LOW_RELAX]          = 0.80;


   m_FlexureModulusOfRuptureCoefficient[pgsTypes::Normal]          = WBFL::Units::ConvertToSysUnits(0.37,WBFL::Units::Measure::SqrtKSI);
   m_FlexureModulusOfRuptureCoefficient[pgsTypes::SandLightweight] = WBFL::Units::ConvertToSysUnits(0.20,WBFL::Units::Measure::SqrtKSI);
   m_FlexureModulusOfRuptureCoefficient[pgsTypes::AllLightweight]  = WBFL::Units::ConvertToSysUnits(0.17,WBFL::Units::Measure::SqrtKSI);

   m_ShearModulusOfRuptureCoefficient[pgsTypes::Normal]          = WBFL::Units::ConvertToSysUnits(0.20,WBFL::Units::Measure::SqrtKSI);
   m_ShearModulusOfRuptureCoefficient[pgsTypes::SandLightweight] = WBFL::Units::ConvertToSysUnits(0.20,WBFL::Units::Measure::SqrtKSI);
   m_ShearModulusOfRuptureCoefficient[pgsTypes::AllLightweight]  = WBFL::Units::ConvertToSysUnits(0.17,WBFL::Units::Measure::SqrtKSI);

   m_LiftingModulusOfRuptureCoefficient[pgsTypes::Normal]          = WBFL::Units::ConvertToSysUnits(0.24,WBFL::Units::Measure::SqrtKSI);
   m_LiftingModulusOfRuptureCoefficient[pgsTypes::SandLightweight] = WBFL::Units::ConvertToSysUnits(0.21,WBFL::Units::Measure::SqrtKSI);
   m_LiftingModulusOfRuptureCoefficient[pgsTypes::AllLightweight]  = WBFL::Units::ConvertToSysUnits(0.18,WBFL::Units::Measure::SqrtKSI);

   m_HaulingModulusOfRuptureCoefficient[pgsTypes::Normal]          = WBFL::Units::ConvertToSysUnits(0.24,WBFL::Units::Measure::SqrtKSI);
   m_HaulingModulusOfRuptureCoefficient[pgsTypes::SandLightweight] = WBFL::Units::ConvertToSysUnits(0.21,WBFL::Units::Measure::SqrtKSI);
   m_HaulingModulusOfRuptureCoefficient[pgsTypes::AllLightweight]  = WBFL::Units::ConvertToSysUnits(0.18,WBFL::Units::Measure::SqrtKSI);

   m_PhiFlexureTensionPS[pgsTypes::Normal]      = 1.00;
   m_PhiFlexureTensionRC[pgsTypes::Normal]      = 0.90;
   m_PhiFlexureTensionSpliced[pgsTypes::Normal] = 1.00;
   m_PhiFlexureCompression[pgsTypes::Normal]    = 0.75;

   m_PhiFlexureTensionPS[pgsTypes::SandLightweight]      = 1.00;
   m_PhiFlexureTensionRC[pgsTypes::SandLightweight]      = 0.90;
   m_PhiFlexureTensionSpliced[pgsTypes::SandLightweight] = 1.00;
   m_PhiFlexureCompression[pgsTypes::SandLightweight]    = 0.75;

   m_PhiFlexureTensionPS[pgsTypes::AllLightweight]      = 1.00;
   m_PhiFlexureTensionRC[pgsTypes::AllLightweight]      = 0.90;
   m_PhiFlexureTensionSpliced[pgsTypes::AllLightweight] = 1.00;
   m_PhiFlexureCompression[pgsTypes::AllLightweight]    = 0.75;

   m_PhiFlexureTensionPS[pgsTypes::PCI_UHPC]      = 1.00;
   m_PhiFlexureTensionRC[pgsTypes::PCI_UHPC]      = 0.90;
   m_PhiFlexureTensionSpliced[pgsTypes::PCI_UHPC] = 1.00;
   m_PhiFlexureCompression[pgsTypes::PCI_UHPC]    = 0.75;

   // These don't make sense since UHPC uses a variable phi based on ductility ratio.
   // The variable phi does have min/max values so this might be able to be used for that in the future
   // At this time, these are just placeholder values. There aren't any UI elements to modify them
   // and they aren't used in the main program.
   m_PhiFlexureTensionPS[pgsTypes::UHPC] = 0.90;
   m_PhiFlexureTensionRC[pgsTypes::UHPC] = 0.90;
   m_PhiFlexureTensionSpliced[pgsTypes::UHPC] = 1.00;
   m_PhiFlexureCompression[pgsTypes::UHPC] = 0.75;

   m_PhiShear[pgsTypes::Normal]          = 0.9;
   m_PhiShear[pgsTypes::SandLightweight] = 0.7;
   m_PhiShear[pgsTypes::AllLightweight]  = 0.7;
   m_PhiShear[pgsTypes::PCI_UHPC] = 0.9;
   m_PhiShear[pgsTypes::UHPC] = 0.9;

   m_PhiShearDebonded[pgsTypes::Normal]          = 0.85; // set defaults to 8th edition
   m_PhiShearDebonded[pgsTypes::SandLightweight] = 0.85;
   m_PhiShearDebonded[pgsTypes::AllLightweight]  = 0.85;
   m_PhiShearDebonded[pgsTypes::PCI_UHPC]        = 0.9; // PCI UHPC has 0.85, but this is going to get sunset, so we are going to set it to the AASHTO UHPC GS value
   m_PhiShearDebonded[pgsTypes::UHPC] = 0.9;

   m_PhiClosureJointFlexure[pgsTypes::Normal]          = 0.95;
   m_PhiClosureJointFlexure[pgsTypes::SandLightweight] = 0.90;
   m_PhiClosureJointFlexure[pgsTypes::AllLightweight]  = 0.90;
   m_PhiClosureJointFlexure[pgsTypes::PCI_UHPC] = 0.95;
   m_PhiClosureJointFlexure[pgsTypes::UHPC] = 0.95; // UHPC doesn't have a fixed phi for flexure

   m_PhiClosureJointShear[pgsTypes::Normal]          = 0.90;
   m_PhiClosureJointShear[pgsTypes::SandLightweight] = 0.70;
   m_PhiClosureJointShear[pgsTypes::AllLightweight]  = 0.70;
   m_PhiClosureJointShear[pgsTypes::PCI_UHPC] = 0.90;
   m_PhiClosureJointShear[pgsTypes::UHPC] = 0.90;


   m_MaxSlabFc[pgsTypes::Normal]             = WBFL::Units::ConvertToSysUnits(6.0,WBFL::Units::Measure::KSI);
   m_MaxSegmentFci[pgsTypes::Normal]         = WBFL::Units::ConvertToSysUnits(7.5,WBFL::Units::Measure::KSI);
   m_MaxSegmentFc[pgsTypes::Normal]          = WBFL::Units::ConvertToSysUnits(10.0,WBFL::Units::Measure::KSI);
   m_MaxClosureFci[pgsTypes::Normal]         = WBFL::Units::ConvertToSysUnits(6.0,WBFL::Units::Measure::KSI);
   m_MaxClosureFc[pgsTypes::Normal]          = WBFL::Units::ConvertToSysUnits(8.0,WBFL::Units::Measure::KSI);
   m_MaxConcreteUnitWeight[pgsTypes::Normal] = WBFL::Units::ConvertToSysUnits(165.,WBFL::Units::Measure::LbfPerFeet3);
   m_MaxConcreteAggSize[pgsTypes::Normal]    = WBFL::Units::ConvertToSysUnits(1.5,WBFL::Units::Measure::Inch);

   m_MaxSlabFc[pgsTypes::AllLightweight]             = WBFL::Units::ConvertToSysUnits(6.0,WBFL::Units::Measure::KSI);
   m_MaxSegmentFci[pgsTypes::AllLightweight]         = WBFL::Units::ConvertToSysUnits(7.5,WBFL::Units::Measure::KSI);
   m_MaxSegmentFc[pgsTypes::AllLightweight]          = WBFL::Units::ConvertToSysUnits(9.0,WBFL::Units::Measure::KSI);
   m_MaxClosureFci[pgsTypes::AllLightweight]         = WBFL::Units::ConvertToSysUnits(6.0,WBFL::Units::Measure::KSI);
   m_MaxClosureFc[pgsTypes::AllLightweight]          = WBFL::Units::ConvertToSysUnits(8.0,WBFL::Units::Measure::KSI);
   m_MaxConcreteUnitWeight[pgsTypes::AllLightweight] = WBFL::Units::ConvertToSysUnits(125.,WBFL::Units::Measure::LbfPerFeet3);
   m_MaxConcreteAggSize[pgsTypes::AllLightweight]    = WBFL::Units::ConvertToSysUnits(1.5,WBFL::Units::Measure::Inch);

   m_MaxSlabFc[pgsTypes::SandLightweight]             = WBFL::Units::ConvertToSysUnits(6.0,WBFL::Units::Measure::KSI);
   m_MaxSegmentFci[pgsTypes::SandLightweight]         = WBFL::Units::ConvertToSysUnits(7.5,WBFL::Units::Measure::KSI);
   m_MaxSegmentFc[pgsTypes::SandLightweight]          = WBFL::Units::ConvertToSysUnits(9.0,WBFL::Units::Measure::KSI);
   m_MaxClosureFci[pgsTypes::SandLightweight]         = WBFL::Units::ConvertToSysUnits(6.0,WBFL::Units::Measure::KSI);
   m_MaxClosureFc[pgsTypes::SandLightweight]          = WBFL::Units::ConvertToSysUnits(8.0,WBFL::Units::Measure::KSI);
   m_MaxConcreteUnitWeight[pgsTypes::SandLightweight] = WBFL::Units::ConvertToSysUnits(125.,WBFL::Units::Measure::LbfPerFeet3);
   m_MaxConcreteAggSize[pgsTypes::SandLightweight]    = WBFL::Units::ConvertToSysUnits(1.5,WBFL::Units::Measure::Inch);

   // Not using these limits for UHPCs yet
   //m_MaxSlabFc[pgsTypes::PCI_UHPC] = WBFL::Units::ConvertToSysUnits(6.0, WBFL::Units::Measure::KSI);
   //m_MaxSegmentFci[pgsTypes::PCI_UHPC] = WBFL::Units::ConvertToSysUnits(10.0, WBFL::Units::Measure::KSI);
   //m_MaxSegmentFc[pgsTypes::PCI_UHPC] = WBFL::Units::ConvertToSysUnits(20.0, WBFL::Units::Measure::KSI);
   //m_MaxClosureFci[pgsTypes::PCI_UHPC] = WBFL::Units::ConvertToSysUnits(6.0, WBFL::Units::Measure::KSI);
   //m_MaxClosureFc[pgsTypes::PCI_UHPC] = WBFL::Units::ConvertToSysUnits(8.0, WBFL::Units::Measure::KSI);
   //m_MaxConcreteUnitWeight[pgsTypes::PCI_UHPC] = WBFL::Units::ConvertToSysUnits(165., WBFL::Units::Measure::LbfPerFeet3);
   //m_MaxConcreteAggSize[pgsTypes::PCI_UHPC] = WBFL::Units::ConvertToSysUnits(1.5, WBFL::Units::Measure::Inch);

   //m_MaxSlabFc[pgsTypes::UHPC] = WBFL::Units::ConvertToSysUnits(6.0, WBFL::Units::Measure::KSI);
   //m_MaxSegmentFci[pgsTypes::UHPC] = WBFL::Units::ConvertToSysUnits(14.0, WBFL::Units::Measure::KSI);
   //m_MaxSegmentFc[pgsTypes::UHPC] = WBFL::Units::ConvertToSysUnits(20.0, WBFL::Units::Measure::KSI);
   //m_MaxClosureFci[pgsTypes::UHPC] = WBFL::Units::ConvertToSysUnits(6.0, WBFL::Units::Measure::KSI);
   //m_MaxClosureFc[pgsTypes::UHPC] = WBFL::Units::ConvertToSysUnits(8.0, WBFL::Units::Measure::KSI);
   //m_MaxConcreteUnitWeight[pgsTypes::UHPC] = WBFL::Units::ConvertToSysUnits(165., WBFL::Units::Measure::LbfPerFeet3);
   //m_MaxConcreteAggSize[pgsTypes::UHPC] = WBFL::Units::ConvertToSysUnits(1.5, WBFL::Units::Measure::Inch);

   m_DoCheckStirrupSpacingCompatibility = true;
   m_bCheckSag = true;
   m_SagCamberType = pgsTypes::LowerBoundCamber;

   m_StirrupSpacingCoefficient[0] = 0.8;
   m_StirrupSpacingCoefficient[1] = 0.4;
   m_MaxStirrupSpacing[0] = WBFL::Units::ConvertToSysUnits(24.0,WBFL::Units::Measure::Inch);
   m_MaxStirrupSpacing[1] = WBFL::Units::ConvertToSysUnits(12.0,WBFL::Units::Measure::Inch);
}

SpecLibraryEntry::SpecLibraryEntry(const SpecLibraryEntry& rOther) :
libLibraryEntry(rOther)
{
   MakeCopy(rOther);
}

SpecLibraryEntry::~SpecLibraryEntry()
{
}

//======================== OPERATORS  =======================================
SpecLibraryEntry& SpecLibraryEntry::operator= (const SpecLibraryEntry& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
bool SpecLibraryEntry::Edit(bool allowEditing,int nPage)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   // exchange data with dialog
   // make a temporary copy of this and have the dialog work on it.
   SpecLibraryEntry tmp(*this);
   if ( 0 < GetRefCount() )
   {
      tmp.AddRef();
   }

   CSpecMainSheet dlg(tmp, IDS_SPEC_SHEET, allowEditing);
   dlg.SetActivePage(nPage);
   INT_PTR i = dlg.DoModal();

   if ( 0 < GetRefCount() )
   {
      tmp.Release();
   }

   if (i==IDOK)
   {
      *this = tmp;
      return true;
   }

   return false;
}

HICON  SpecLibraryEntry::GetIcon() const
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   return ::LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_SPECIFICATION_ENTRY) );
}

bool SpecLibraryEntry::SaveMe(WBFL::System::IStructuredSave* pSave)
{
   pSave->BeginUnit(_T("SpecificationLibraryEntry"), CURRENT_VERSION);
   pSave->Property(_T("Name"),this->GetName().c_str());
   pSave->Property(_T("Description"), this->GetDescription(false).c_str());

   pSave->Property(_T("UseCurrentSpecification"), m_bUseCurrentSpecification); // added in version 77
   pSave->Property(_T("SpecificationType"),lrfdVersionMgr::GetVersionString(m_SpecificationType,true));

   if (m_SpecificationUnits==lrfdVersionMgr::SI)
   {
      pSave->Property(_T("SpecificationUnits"), _T("SiUnitsSpec"));
   }
   else if (m_SpecificationUnits==lrfdVersionMgr::US)
   {
      pSave->Property(_T("SpecificationUnits"), _T("UsUnitsSpec"));
   }
   else
   {
      ASSERT(0);
   }

   pSave->Property(_T("SectionPropertyType"),m_SectionPropertyMode); // added version 50

   pSave->Property(_T("DoCheckStrandSlope"), m_DoCheckStrandSlope);
   pSave->Property(_T("DoDesignStrandSlope"), m_DoDesignStrandSlope);
   pSave->Property(_T("MaxSlope05"), m_MaxSlope05);
   pSave->Property(_T("MaxSlope06"), m_MaxSlope06);
   pSave->Property(_T("MaxSlope07"), m_MaxSlope07); // added in version 35
   pSave->Property(_T("DoCheckHoldDown"), m_DoCheckHoldDown);
   pSave->Property(_T("DoDesignHoldDown"), m_DoDesignHoldDown);
   pSave->Property(_T("HoldDownForceType"), m_HoldDownForceType); // added in version 70
   pSave->Property(_T("HoldDownForce"), m_HoldDownForce);
   pSave->Property(_T("HoldDownFriction"), m_HoldDownFriction); // added in version 70
   pSave->Property(_T("DoCheckSplitting"), m_DoCheckSplitting);
   pSave->Property(_T("DoDesignSplitting"), m_DoDesignSplitting);
   pSave->Property(_T("DoCheckConfinement"), m_DoCheckConfinement);
   pSave->Property(_T("DoDesignConfinement"), m_DoDesignConfinement);

   pSave->Property(_T("DoCheckHandlingWeightLimit"), m_bCheckHandlingWeightLimit); // added version 70
   pSave->Property(_T("HandlingWeightLimit"), m_HandlingWeightLimit); // added version 70

   //pSave->Property(_T("MaxStirrupSpacing"), m_MaxStirrupSpacing); // removed in version 46
   pSave->Property(_T("StirrupSpacingCoefficient1"),m_StirrupSpacingCoefficient[0]); // added in version 46
   pSave->Property(_T("MaxStirrupSpacing1"),m_MaxStirrupSpacing[0]); // added in version 46
   pSave->Property(_T("StirrupSpacingCoefficient2"),m_StirrupSpacingCoefficient[1]); // added in version 46
   pSave->Property(_T("MaxStirrupSpacing2"),m_MaxStirrupSpacing[1]); // added in version 46
   pSave->Property(_T("CyLiftingCrackFs"), m_CyLiftingCrackFs);
   pSave->Property(_T("CyLiftingFailFs"), m_CyLiftingFailFs);
   pSave->Property(_T("CyCompStressServ"), m_CyCompStressServ);
   //pSave->Property(_T("CyCompStressLifting"), m_LiftingCompressionStressCoefficient_GlobalStress); // removed in version 65
   pSave->Property(_T("CyGlobalCompStressLifting"), m_LiftingCompressionStressCoefficient_GlobalStress); // added in version 65
   pSave->Property(_T("CyPeakCompStressLifting"), m_LiftingCompressionStressCoefficient_PeakStress); // added in version 65
   pSave->Property(_T("CyTensStressServ"), m_CyTensStressServ);
   pSave->Property(_T("CyDoTensStressServMax"), m_CyDoTensStressServMax);
   pSave->Property(_T("CyTensStressServMax"), m_CyTensStressServMax);
   pSave->Property(_T("CyTensStressLifting"), m_CyTensStressLifting);
   pSave->Property(_T("CyDoTensStressLiftingMax"),m_CyDoTensStressLiftingMax);
   pSave->Property(_T("CyTensStressLiftingMax"),m_CyTensStressLiftingMax);
   pSave->Property(_T("BurstingZoneLengthFactor"), m_SplittingZoneLengthFactor);
   //pSave->Property(_T("UHPCStregthAtFirstCrack"), m_UHPCStregthAtFirstCrack); // added in version 75, removed version 82
   pSave->Property(_T("LiftingUpwardImpact"),m_LiftingUpwardImpact);
   pSave->Property(_T("LiftingDownwardImpact"),m_LiftingDownwardImpact);
   pSave->Property(_T("HaulingUpwardImpact"),m_HaulingUpwardImpact);
   pSave->Property(_T("HaulingDownwardImpact"),m_HaulingDownwardImpact);
   pSave->Property(_T("CuringMethod"), (Int16)m_CuringMethod);
   pSave->Property(_T("EnableLiftingCheck"), m_EnableLiftingCheck);
   pSave->Property(_T("EnableLiftingDesign"), m_EnableLiftingDesign);
   pSave->Property(_T("PickPointHeight"), m_PickPointHeight);
   pSave->Property(_T("LiftingLoopTolerance"),m_LiftingLoopTolerance);
   pSave->Property(_T("MinCableInclination"),m_MinCableInclination);
   pSave->Property(_T("MaxGirderSweepLifting"), m_MaxGirderSweepLifting);
   //pSave->Property(_T("LiftingCamberMethod"),(Int32)m_LiftingCamberMethod); // added version 56, removed in version 65
   //pSave->Property(_T("LiftingCamberPercentEstimate"),m_LiftingCamberPercentEstimate); // added version 56, removed in version 65
   pSave->Property(_T("LiftingCamberMultiplier"), m_LiftingCamberMultiplier); // added version 58
   pSave->Property(_T("LiftingWindType"),(Int32)m_LiftingWindType); // added in version 56
   pSave->Property(_T("LiftingWindLoad"),m_LiftingWindLoad); // added in version 56
   //pSave->Property(_T("LiftingStressesPlumbGirder"), m_LiftingStressesPlumbGirder); // added in version 56, removed 59
   //pSave->Property(_T("LiftingStressesEquilibriumAngle"), m_bComputeLiftingStressesAtEquilibriumAngle); // added in version 59, removed in version 65
   pSave->Property(_T("EnableHaulingCheck"), m_EnableHaulingCheck);
   pSave->Property(_T("EnableHaulingDesign"), m_EnableHaulingDesign);
   pSave->Property(_T("HaulingAnalysisMethod"), (Int32)m_HaulingAnalysisMethod); // added version 43
   pSave->Property(_T("MaxGirderSweepHauling"), m_MaxGirderSweepHauling);
   pSave->Property(_T("SweepGrowthHauling"), m_HaulingSweepGrowth); // added version 66
   //pSave->Property(_T("HaulingSupportDistance"),m_HaulingSupportDistance); // removed version 56
   //pSave->Property(_T("MaxHaulingOverhang"), m_MaxHaulingOverhang); // removed version 56
   pSave->Property(_T("HaulingSupportPlacementTolerance"),m_HaulingSupportPlacementTolerance);
   //pSave->Property(_T("HaulingCamberMethod"),(Int32)m_HaulingCamberMethod); // added version 56, removed in version 65
   //pSave->Property(_T("HaulingCamberPercentEstimate"),m_HaulingCamberPercentEstimate); // added version 56, removed in version 65
   pSave->Property(_T("HaulingCamberMultiplier"), m_HaulingCamberMultiplier); // added version 58
   pSave->Property(_T("HaulingWindType"),(Int32)m_HaulingWindType); // added in version 56
   pSave->Property(_T("HaulingWindLoad"),m_HaulingWindLoad); // added in version 56
   pSave->Property(_T("CentrifugalForceType"),(Int32)m_CentrifugalForceType); // added in version 56
   pSave->Property(_T("HaulingSpeed"),m_HaulingSpeed); // added in version 56
   pSave->Property(_T("TurningRadius"),m_TurningRadius); // added in version 56
   //pSave->Property(_T("HaulingStressesEquilibriumAngle"), m_bComputeHaulingStressesAtEquilibriumAngle); // added in version 59, removed in version 65
   //pSave->Property(_T("CompStressHauling"), m_CompStressHauling); // removed in version 65
   pSave->Property(_T("GlobalCompStressHauling"), m_GlobalCompStressHauling); // added in version 65
   pSave->Property(_T("PeakCompStressHauling"), m_PeakCompStressHauling); // added in version 65

   //pSave->Property(_T("TensStressHauling"),m_TensStressHauling); // removed in version 56
   //pSave->Property(_T("DoTensStressHaulingMax"),m_DoTensStressHaulingMax);  // removed in version 56
   //pSave->Property(_T("TensStressHaulingMax"), m_TensStressHaulingMax); // removed in version 56

   pSave->Property(_T("TensStressHaulingNormalCrown"),m_TensStressHauling[pgsTypes::CrownSlope]); // added in version 56
   pSave->Property(_T("DoTensStressHaulingMaxNormalCrown"),m_DoTensStressHaulingMax[pgsTypes::CrownSlope]);  // added in version 56
   pSave->Property(_T("TensStressHaulingMaxNormalCrown"), m_TensStressHaulingMax[pgsTypes::CrownSlope]); // added in version 56

   pSave->Property(_T("TensStressHaulingMaxSuper"),m_TensStressHauling[pgsTypes::Superelevation]); // added in version 56
   pSave->Property(_T("DoTensStressHaulingMaxMaxSuper"),m_DoTensStressHaulingMax[pgsTypes::Superelevation]);  // added in version 56
   pSave->Property(_T("TensStressHaulingMaxMaxSuper"), m_TensStressHaulingMax[pgsTypes::Superelevation]); // added in version 56

   pSave->Property(_T("HeHaulingCrackFs"), m_HaulingCrackFs);
   pSave->Property(_T("HeHaulingFailFs"), m_HaulingRollFs);
   pSave->Property(_T("HaulingImpactUsage"), (Int32)m_HaulingImpactUsage); // added in version 56
   pSave->Property(_T("RoadwayCrownSlope"),m_RoadwayCrownSlope); // added in version 56
   pSave->Property(_T("RoadwaySuperelevation"), m_RoadwaySuperelevation);

   //pSave->Property(_T("TruckRollStiffnessMethod"), (long)m_TruckRollStiffnessMethod); // removed version 56
   //pSave->Property(_T("TruckRollStiffness"), m_TruckRollStiffness); // removed version 56
   //pSave->Property(_T("AxleWeightLimit"), m_AxleWeightLimit); // removed version 56
   //pSave->Property(_T("AxleStiffness"),m_AxleStiffness); // removed version 56
   //pSave->Property(_T("MinRollStiffness"),m_MinRollStiffness); // removed version 56
   //pSave->Property(_T("TruckGirderHeight"), m_TruckGirderHeight); // removed version 56
   //pSave->Property(_T("TruckRollCenterHeight"), m_TruckRollCenterHeight); // removed version 56
   //pSave->Property(_T("TruckAxleWidth"), m_TruckAxleWidth); // removed version 56
   //pSave->Property(_T("HeErectionCrackFs"), m_HeErectionCrackFs); // removed in version 55.0
   //pSave->Property(_T("HeErectionFailFs"), m_HeErectionFailFs); // removed in version 55.0
   //pSave->Property(_T("MaxGirderWgt"),m_MaxGirderWgt); // removed version 56

   // Added at version 53
   pSave->Property(_T("LimitStateConcreteStrength"),m_LimitStateConcreteStrength);

   // Added in version 72
   pSave->Property(_T("Use90DayConcreteStrength"), m_bUse90DayConcreteStrength);
   pSave->Property(_T("SlowCuringConcreteStrengthFactor"), m_90DayConcreteStrengthFactor);

   // modified in version 37 (see below)
   //pSave->Property(_T("HaulingModulusOfRuptureCoefficient"),m_HaulingModulusOfRuptureCoefficient); // added for version 12.0
   //pSave->Property(_T("LiftingModulusOfRuptureCoefficient"),m_LiftingModulusOfRuptureCoefficient); // added for version 20.0


   // Added at version 25
   pSave->Property(_T("MinLiftingPointLocation"),       m_MinLiftPoint        );
   pSave->Property(_T("LiftingPointLocationAccuracy"),  m_LiftPointAccuracy   );
   pSave->Property(_T("MinHaulingSupportLocation"),     m_MinHaulPoint        );
   pSave->Property(_T("HaulingSupportLocationAccuracy"),m_HaulPointAccuracy   );

   // KDOT values added at version 43
   pSave->Property(_T("UseMinTruckSupportLocationFactor"), m_UseMinTruckSupportLocationFactor);
   pSave->Property(_T("MinTruckSupportLocationFactor"), m_MinTruckSupportLocationFactor);
   pSave->Property(_T("OverhangGFactor"), m_OverhangGFactor);
   pSave->Property(_T("InteriorGFactor"), m_InteriorGFactor);

   // Added at version 4.0
   pSave->Property(_T("CastingYardTensileStressLimitWithMildRebar"),m_CyTensStressServWithRebar);
   pSave->Property(_T("LiftingTensileStressLimitWithMildRebar"),m_TensStressLiftingWithRebar);
   //pSave->Property(_T("HaulingTensileStressLimitWithMildRebar"),m_TensStressHaulingWithRebar); // removed in version 56
   pSave->Property(_T("HaulingTensileStressLimitWithMildRebarNormalCrown"),m_TensStressHaulingWithRebar[pgsTypes::CrownSlope]); // added in version 56
   pSave->Property(_T("HaulingTensileStressLimitWithMildRebarMaxSuper"),m_TensStressHaulingWithRebar[pgsTypes::Superelevation]); // added in version 56

   // Added at version 30
   pSave->Property(_T("TempStrandRemovalCompStress") ,     m_TempStrandRemovalCompStress);
   pSave->Property(_T("TempStrandRemovalTensStress") ,     m_TempStrandRemovalTensStress);
   pSave->Property(_T("TempStrandRemovalDoTensStressMax") ,m_TempStrandRemovalDoTensStressMax);
   pSave->Property(_T("TempStrandRemovalTensStressMax") ,  m_TempStrandRemovalTensStressMax);
   pSave->Property(_T("TempStrandRemovalTensStressWithRebar"), m_TempStrandRemovalTensStressWithRebar); // added version 49

   pSave->Property(_T("CheckTemporaryStresses"),m_bCheckTemporaryStresses); // added in version 47
   pSave->Property(_T("Bs1CompStress") ,     m_Bs1CompStress); // removed m_ in version 30
   pSave->Property(_T("Bs1TensStress") ,     m_Bs1TensStress); // removed m_ in version 30
   pSave->Property(_T("Bs1DoTensStressMax") ,m_Bs1DoTensStressMax); // removed m_ in version 30
   pSave->Property(_T("Bs1TensStressMax") ,  m_Bs1TensStressMax); // removed m_ in version 30
   pSave->Property(_T("Bs2CompStress") ,     m_Bs2CompStress); // removed m_ in version 30

   pSave->Property(_T("CheckBs2Tension"),    m_bCheckBs2Tension); // added in version 47
   pSave->Property(_T("Bs2TensStress") ,     m_Bs2TensStress); // added in version 47
   pSave->Property(_T("Bs2DoTensStressMax") ,m_Bs2DoTensStressMax); // added in version 47
   pSave->Property(_T("Bs2TensStressMax") ,  m_Bs2TensStressMax); // added in version 47

   pSave->Property(_T("Bs2TrafficBarrierDistributionType"),(Int16)m_TrafficBarrierDistributionType); // added in version 36
   pSave->Property(_T("Bs2MaxGirdersTrafficBarrier"), m_Bs2MaxGirdersTrafficBarrier);
   //pSave->Property(_T("Bs2MaxGirdersUtility"), m_Bs2MaxGirdersUtility); // removed in version 55
   pSave->Property(_T("OverlayLoadDistribution"), (Int32)m_OverlayLoadDistribution); // added in version 34
   pSave->Property(_T("HaunchLoadComputationType"), (Int32)m_HaunchLoadComputationType); // added in version 54
   pSave->Property(_T("HaunchLoadCamberTolerance"), m_HaunchLoadCamberTolerance);        // ""
   pSave->Property(_T("HaunchLoadCamberFactor"), m_HaunchLoadCamberFactor);  // added in version 61
   pSave->Property(_T("HaunchAnalysisComputationType"), (Int32)m_HaunchAnalysisSectionPropertiesType); // added in version 63
   pSave->Property(_T("Bs3CompStressServ"), m_Bs3CompStressServ);
   pSave->Property(_T("Bs3CompStressService1A"), m_Bs3CompStressService1A);
   pSave->Property(_T("Bs3TensStressServNc"), m_Bs3TensStressServNc);
   pSave->Property(_T("Bs3DoTensStressServNcMax"), m_Bs3DoTensStressServNcMax);
   pSave->Property(_T("Bs3TensStressServNcMax"), m_Bs3TensStressServNcMax);
   pSave->Property(_T("Bs3TensStressServSc"),    m_Bs3TensStressServSc);   
   pSave->Property(_T("Bs3DoTensStressServScMax"), m_Bs3DoTensStressServScMax);
   pSave->Property(_T("Bs3TensStressServScMax"), m_Bs3TensStressServScMax);

   // added in version 76
   pSave->Property(_T("PrincipalTensileStressMethod"), m_PrincipalTensileStressMethod);
   pSave->Property(_T("PrincipalTensileStressCoefficient"), m_PrincipalTensileStressCoefficient);
   pSave->Property(_T("PrincipalTensileStressTendonNearnessFactor"), m_PrincipalTensileStressTendonNearnessFactor); // added in version 77
   pSave->Property(_T("PrincipalTensileStressFcThreshold"), m_PrincipalTensileStressFcThreshold); // added in version 78
   pSave->Property(_T("PrincipalTensileStressUngroutedMultiplier"), m_PrincipalTensileStressUngroutedMultiplier); // added in version 80
   pSave->Property(_T("PrincipalTensileStressGroutedMultiplier"), m_PrincipalTensileStressGroutedMultiplier);     //   ""   ""  ""    80


   // removed in version 29
   // pSave->Property(_T("Bs3IgnoreRangeOfApplicability"), m_Bs3IgnoreRangeOfApplicability);

   // moved into MomentCapacity data block (version 37)
   //pSave->Property(_T("Bs3LRFDOverreinforcedMomentCapacity"),(Int16)m_Bs3LRFDOverReinforcedMomentCapacity);
   //pSave->Property(_T("IncludeRebar_MomentCapacity"),m_bIncludeRebar_Moment); // added for version 7.0

   // added in version 37
   pSave->BeginUnit(_T("MomentCapacity"),5.0);
      pSave->Property(_T("Bs3LRFDOverreinforcedMomentCapacity"),(Int16)m_Bs3LRFDOverReinforcedMomentCapacity);
      pSave->Property(_T("IncludeStrandForNegMoment"), m_bIncludeStrand_NegMoment); // added in version 4 of this data block
      pSave->Property(_T("IncludeRebarForCapacity"),m_bIncludeRebar_Moment);
      pSave->Property(_T("ConsiderReinforcementStrainLimit"),m_bConsiderReinforcementStrainLimit); // added in version 5 of this data block
      pSave->Property(_T("MomentCapacitySliceCount"), m_nMomentCapacitySlices); // added in version 5 of this data block
      pSave->Property(_T("IncludeNoncompositeMomentForNegMomentDesign"),m_bIncludeForNegMoment); // added version 2 of this data block
      pSave->BeginUnit(_T("ResistanceFactor"),1.0);
         pSave->BeginUnit(_T("NormalWeight"),2.0);
            pSave->Property(_T("TensionControlled_RC"),m_PhiFlexureTensionRC[pgsTypes::Normal]);
            pSave->Property(_T("TensionControlled_PS"),m_PhiFlexureTensionPS[pgsTypes::Normal]);
            pSave->Property(_T("TensionControlled_Spliced"),m_PhiFlexureTensionSpliced[pgsTypes::Normal]);
            pSave->Property(_T("CompressionControlled"),m_PhiFlexureCompression[pgsTypes::Normal]);
         pSave->EndUnit(); // NormalWeight
         pSave->BeginUnit(_T("AllLightweight"),2.0);
            pSave->Property(_T("TensionControlled_RC"),m_PhiFlexureTensionRC[pgsTypes::AllLightweight]);
            pSave->Property(_T("TensionControlled_PS"),m_PhiFlexureTensionPS[pgsTypes::AllLightweight]);
            pSave->Property(_T("TensionControlled_Spliced"),m_PhiFlexureTensionSpliced[pgsTypes::AllLightweight]);
            pSave->Property(_T("CompressionControlled"),m_PhiFlexureCompression[pgsTypes::AllLightweight]);
         pSave->EndUnit(); // AllLightweight
         pSave->BeginUnit(_T("SandLightweight"),2.0);
            pSave->Property(_T("TensionControlled_RC"),m_PhiFlexureTensionRC[pgsTypes::SandLightweight]);
            pSave->Property(_T("TensionControlled_PS"),m_PhiFlexureTensionPS[pgsTypes::SandLightweight]);
            pSave->Property(_T("TensionControlled_Spliced"),m_PhiFlexureTensionSpliced[pgsTypes::SandLightweight]);
            pSave->Property(_T("CompressionControlled"),m_PhiFlexureCompression[pgsTypes::SandLightweight]);
         pSave->EndUnit(); // SandLightweight
      pSave->EndUnit(); // ResistanceFactor

      // added ClosureJointResistanceFactor in version 3.0 of MomentCapacity data block
      pSave->BeginUnit(_T("ClosureJointResistanceFactor"),1.0);
         pSave->BeginUnit(_T("NormalWeight"),1.0);
            pSave->Property(_T("FullyBondedTendons"),m_PhiClosureJointFlexure[pgsTypes::Normal]);
         pSave->EndUnit(); // NormalWeight
         pSave->BeginUnit(_T("AllLightweight"),1.0);
            pSave->Property(_T("FullyBondedTendons"),m_PhiClosureJointFlexure[pgsTypes::AllLightweight]);
         pSave->EndUnit(); // AllLightweight
         pSave->BeginUnit(_T("SandLightweight"),1.0);
            pSave->Property(_T("FullyBondedTendons"),m_PhiClosureJointFlexure[pgsTypes::SandLightweight]);
         pSave->EndUnit(); // SandLightweight
      pSave->EndUnit(); // ClosureJointResistanceFactor
   pSave->EndUnit(); // MomentCapacity

   // changed in version 37
   //pSave->Property(_T("ModulusOfRuptureCoefficient"),m_FlexureModulusOfRuptureCoefficient); // added for version 9.0
   //pSave->Property(_T("ShearModulusOfRuptureCoefficient"),m_ShearModulusOfRuptureCoefficient); // added for version 18
   pSave->BeginUnit(_T("ModulusOfRuptureCoefficient"),1.0);
      pSave->BeginUnit(_T("Moment"),3.0);
         pSave->Property(_T("Normal"),m_FlexureModulusOfRuptureCoefficient[pgsTypes::Normal]);
         pSave->Property(_T("AllLightweight"),m_FlexureModulusOfRuptureCoefficient[pgsTypes::AllLightweight]);
         pSave->Property(_T("SandLightweight"), m_FlexureModulusOfRuptureCoefficient[pgsTypes::SandLightweight]);
         //pSave->Property(_T("UHPC"), m_FlexureModulusOfRuptureCoefficient[pgsTypes::PCI_UHPC]); // added in version 2 // removed in version 3
         pSave->EndUnit(); // Moment
      pSave->BeginUnit(_T("Shear"),3.0);
         pSave->Property(_T("Normal"),m_ShearModulusOfRuptureCoefficient[pgsTypes::Normal]);
         pSave->Property(_T("AllLightweight"),m_ShearModulusOfRuptureCoefficient[pgsTypes::AllLightweight]);
         pSave->Property(_T("SandLightweight"), m_ShearModulusOfRuptureCoefficient[pgsTypes::SandLightweight]);
         //pSave->Property(_T("UHPC"), m_ShearModulusOfRuptureCoefficient[pgsTypes::PCI_UHPC]); // added in version 2 // removed in version 3
         pSave->EndUnit(); // Shear
      pSave->BeginUnit(_T("Lifting"),3.0);
         pSave->Property(_T("Normal"),m_LiftingModulusOfRuptureCoefficient[pgsTypes::Normal]);
         pSave->Property(_T("AllLightweight"),m_LiftingModulusOfRuptureCoefficient[pgsTypes::AllLightweight]);
         pSave->Property(_T("SandLightweight"),m_LiftingModulusOfRuptureCoefficient[pgsTypes::SandLightweight]);
         //pSave->Property(_T("UHPC"), m_LiftingModulusOfRuptureCoefficient[pgsTypes::PCI_UHPC]); // added in version 2, removed in version 3
         pSave->EndUnit(); // Lifting
      pSave->BeginUnit(_T("Shipping"),3.0);
         pSave->Property(_T("Normal"),m_HaulingModulusOfRuptureCoefficient[pgsTypes::Normal]);
         pSave->Property(_T("AllLightweight"),m_HaulingModulusOfRuptureCoefficient[pgsTypes::AllLightweight]);
         pSave->Property(_T("SandLightweight"), m_HaulingModulusOfRuptureCoefficient[pgsTypes::SandLightweight]);
         //pSave->Property(_T("UHPC"), m_HaulingModulusOfRuptureCoefficient[pgsTypes::PCI_UHPC]); // added in version 2, removed in version 3
         pSave->EndUnit(); // Shipping
   pSave->EndUnit(); // ModulusOfRuptureCoefficient

   //pSave->Property(_T("UHPCFiberShearStrength"), m_UHPCFiberShearStrength); // added in version 75, removed in version 82

   pSave->Property(_T("BsLldfMethod"), (Int16)m_LldfMethod );  // added LLDF_TXDOT for version 21.0
   pSave->Property(_T("IgnoreSkewReductionForMoment"), m_bIgnoreSkewReductionForMoment); // added in version 74
   // added in version 29
   pSave->Property(_T("MaxAngularDeviationBetweenGirders"),m_MaxAngularDeviationBetweenGirders);
   pSave->Property(_T("MinGirderStiffnessRatio"),m_MinGirderStiffnessRatio);
   pSave->Property(_T("LLDFGirderSpacingLocation"),m_LLDFGirderSpacingLocation);
   pSave->Property(_T("UseRigidMethod"), m_bUseRigidMethod); // added in version 70

   pSave->Property(_T("IncludeDualTandem"), m_bIncludeDualTandem); // added in version 61

   // added in version 31
   pSave->Property(_T("LimitDistributionFactorsToLanesBeams"),m_LimitDistributionFactorsToLanesBeams);

   // added in version 81
   pSave->Property(_T("ExteriorLiveLoadDistributionGTAdjacentInteriorRule"),m_ExteriorLiveLoadDistributionGTAdjacentInteriorRule);

   // moved into Shear block in version 37
   //pSave->Property(_T("LongReinfShearMethod"),(Int16)m_LongReinfShearMethod); // added for version 1.2
   //pSave->Property(_T("IncludeRebar_Shear"),m_bIncludeRebar_Shear); // added for version 7.0
   pSave->Property(_T("CreepMethod"), (Int16)m_CreepMethod );
   pSave->Property(_T("CreepFactor"),m_CreepFactor);
   pSave->Property(_T("CreepDuration1Min"),m_CreepDuration1Min);
   pSave->Property(_T("CreepDuration1Max"),m_CreepDuration1Max);
   pSave->Property(_T("CreepDuration2Min"),m_CreepDuration2Min);
   pSave->Property(_T("CreepDuration2Max"),m_CreepDuration2Max);
   pSave->Property(_T("XferTime"),m_XferTime);
   pSave->Property(_T("TotalCreepDuration"),m_TotalCreepDuration);
   pSave->Property(_T("CamberVariability"),m_CamberVariability);

   pSave->Property(_T("LossMethod"),(Int16)m_LossMethod);
   pSave->Property(_T("TimeDependentModel"),(Int16)m_TimeDependentModel); // added in version 50
   //pSave->Property(_T("FinalLosses"),m_FinalLosses); // removed version 50
   pSave->Property(_T("ShippingLosses"),m_ShippingLosses);
   //pSave->Property(_T("BeforeXferLosses"),m_BeforeXferLosses);// removed version 50
   //pSave->Property(_T("AfterXferLosses"),m_AfterXferLosses);// removed version 50
   pSave->Property(_T("ShippingTime"),m_ShippingTime);

   // added in version 22
   //pSave->Property(_T("LiftingLosses"),m_LiftingLosses);// removed version 50
   //pSave->Property(_T("BeforeTempStrandRemovalLosses"),m_BeforeTempStrandRemovalLosses);// removed version 50
   //pSave->Property(_T("AfterTempStrandRemovalLosses"),m_AfterTempStrandRemovalLosses);// removed version 50
   //pSave->Property(_T("AfterDeckPlacementLosses"),m_AfterDeckPlacementLosses);// removed version 50
   //pSave->Property(_T("AfterSIDLLosses"),m_AfterSIDLLosses); // added in version 38// removed version 50


   pSave->Property(_T("CuringMethodFactor"),m_CuringMethodTimeAdjustmentFactor);

   // Added in version 1.5
   pSave->Property(_T("CheckStrandStressAtJacking"),m_bCheckStrandStress[CSS_AT_JACKING]);
   pSave->Property(_T("Coeff_AtJacking_StressRel"),m_StrandStressCoeff[CSS_AT_JACKING][STRESS_REL]);
   pSave->Property(_T("Coeff_AtJacking_LowRelax"),m_StrandStressCoeff[CSS_AT_JACKING][LOW_RELAX]);

   pSave->Property(_T("CheckStrandStressBeforeTransfer"),m_bCheckStrandStress[CSS_BEFORE_TRANSFER]);
   pSave->Property(_T("Coeff_BeforeTransfer_StressRel"),m_StrandStressCoeff[CSS_BEFORE_TRANSFER][STRESS_REL]);
   pSave->Property(_T("Coeff_BeforeTransfer_LowRelax"),m_StrandStressCoeff[CSS_BEFORE_TRANSFER][LOW_RELAX]);

   pSave->Property(_T("CheckStrandStressAfterTransfer"),m_bCheckStrandStress[CSS_AFTER_TRANSFER]);
   pSave->Property(_T("Coeff_AfterTransfer_StressRel"),m_StrandStressCoeff[CSS_AFTER_TRANSFER][STRESS_REL]);
   pSave->Property(_T("Coeff_AfterTransfer_LowRelax"),m_StrandStressCoeff[CSS_AFTER_TRANSFER][LOW_RELAX]);

   pSave->Property(_T("CheckStrandStressAfterAllLosses"),m_bCheckStrandStress[CSS_AFTER_ALL_LOSSES]);
   pSave->Property(_T("Coeff_AfterAllLosses_StressRel"),m_StrandStressCoeff[CSS_AFTER_ALL_LOSSES][STRESS_REL]);
   pSave->Property(_T("Coeff_AfterAllLosses_LowRelax"),m_StrandStressCoeff[CSS_AFTER_ALL_LOSSES][LOW_RELAX]);

   // added in version 50
   pSave->Property(_T("CheckTendonStressAtJacking"),m_bCheckTendonStressAtJacking);
   pSave->Property(_T("CheckTendonStressPriorToSeating"),m_bCheckTendonStressPriorToSeating);
   pSave->Property(_T("Coeff_AtJacking_StressRel"),m_TendonStressCoeff[CSS_AT_JACKING][STRESS_REL]);
   pSave->Property(_T("Coeff_AtJacking_LowRelax"),m_TendonStressCoeff[CSS_AT_JACKING][LOW_RELAX]);
   pSave->Property(_T("Coeff_PriorToSeating_StressRel"),m_TendonStressCoeff[CSS_PRIOR_TO_SEATING][STRESS_REL]);
   pSave->Property(_T("Coeff_PriorToSeating_LowRelax"),m_TendonStressCoeff[CSS_PRIOR_TO_SEATING][LOW_RELAX]);
   pSave->Property(_T("Coeff_AtAnchoragesAfterSeating_StressRel"),m_TendonStressCoeff[CSS_ANCHORAGES_AFTER_SEATING][STRESS_REL]);
   pSave->Property(_T("Coeff_AtAnchoragesAfterSeating_LowRelax"),m_TendonStressCoeff[CSS_ANCHORAGES_AFTER_SEATING][LOW_RELAX]);
   pSave->Property(_T("Coeff_ElsewhereAfterSeating_StressRel"),m_TendonStressCoeff[CSS_ELSEWHERE_AFTER_SEATING][STRESS_REL]);
   pSave->Property(_T("Coeff_ElsewhereAfterSeating_LowRelax"),m_TendonStressCoeff[CSS_ELSEWHERE_AFTER_SEATING][LOW_RELAX]);
   pSave->Property(_T("Coeff_AfterAllLosses_StressRel"),m_TendonStressCoeff[CSS_AFTER_ALL_LOSSES][STRESS_REL]);
   pSave->Property(_T("Coeff_AfterAllLosses_LowRelax"),m_TendonStressCoeff[CSS_AFTER_ALL_LOSSES][LOW_RELAX]);
   

   // added in version 23, removed version 50
   //pSave->Property(_T("AnchorSet"),m_Dset);
   //pSave->Property(_T("WobbleFriction"),m_WobbleFriction);
   //pSave->Property(_T("CoefficientOfFriction"),m_FrictionCoefficient);

   // added in version 40
   pSave->Property(_T("RelaxationLossMethod"),m_RelaxationLossMethod);
   pSave->Property(_T("SlabElasticGain"),m_SlabElasticGain);
   pSave->Property(_T("HaunchElasticGain"),m_SlabPadElasticGain);
   pSave->Property(_T("DiaphragmElasticGain"),m_DiaphragmElasticGain);
   pSave->Property(_T("UserDCElasticGainBS1"),m_UserDCElasticGainBS1);
   pSave->Property(_T("UserDWElasticGainBS1"),m_UserDWElasticGainBS1);
   pSave->Property(_T("UserDCElasticGainBS2"),m_UserDCElasticGainBS2);
   pSave->Property(_T("UserDWElasticGainBS2"),m_UserDWElasticGainBS2);
   pSave->Property(_T("RailingSystemElasticGain"),m_RailingSystemElasticGain);
   pSave->Property(_T("OverlayElasticGain"),m_OverlayElasticGain);
   pSave->Property(_T("SlabShrinkageElasticGain"),m_SlabShrinkageElasticGain);
   pSave->Property(_T("LiveLoadElasticGain"),m_LiveLoadElasticGain); // added in version 42

   // added in version 45
   if ( m_LossMethod == LOSSES_TXDOT_REFINED_2013 )
   {
      pSave->Property(_T("FcgpComputationMethod"),m_FcgpComputationMethod);
   }

   // Added in 1.7
   pSave->Property(_T("CheckLiveLoadDeflection"),m_bDoEvaluateDeflection);
   pSave->Property(_T("LiveLoadDeflectionLimit"),m_DeflectionLimit);

//   // Added in 8.0, removed in version 28
//   pSave->Property(_T("AnalysisType"),(long)m_AnalysisType);

   // Added in 10.0, updated in version 37
   pSave->BeginUnit(_T("Limits"),1.0);
      pSave->BeginUnit(_T("Normal"),2.0);
         pSave->Property(_T("MaxSlabFc"),             m_MaxSlabFc[pgsTypes::Normal]);
         pSave->Property(_T("MaxGirderFci"),          m_MaxSegmentFci[pgsTypes::Normal]);
         pSave->Property(_T("MaxGirderFc"),           m_MaxSegmentFc[pgsTypes::Normal]);
         pSave->Property(_T("MaxClosureFci"),         m_MaxClosureFci[pgsTypes::Normal]); // added in version 2.0
         pSave->Property(_T("MaxClosureFc"),          m_MaxClosureFc[pgsTypes::Normal]); // added in version 2.0
         pSave->Property(_T("MaxConcreteUnitWeight"), m_MaxConcreteUnitWeight[pgsTypes::Normal]);
         pSave->Property(_T("MaxConcreteAggSize"),    m_MaxConcreteAggSize[pgsTypes::Normal]);
      pSave->EndUnit(); // Normal;
      pSave->BeginUnit(_T("AllLightweight"),2.0);
         pSave->Property(_T("MaxSlabFc"),             m_MaxSlabFc[pgsTypes::AllLightweight]);
         pSave->Property(_T("MaxGirderFci"),          m_MaxSegmentFci[pgsTypes::AllLightweight]);
         pSave->Property(_T("MaxGirderFc"),           m_MaxSegmentFc[pgsTypes::AllLightweight]);
         pSave->Property(_T("MaxClosureFci"),         m_MaxClosureFci[pgsTypes::AllLightweight]); // added in version 2.0
         pSave->Property(_T("MaxClosureFc"),          m_MaxClosureFc[pgsTypes::AllLightweight]); // added in version 2.0
         pSave->Property(_T("MaxConcreteUnitWeight"), m_MaxConcreteUnitWeight[pgsTypes::AllLightweight]);
         pSave->Property(_T("MaxConcreteAggSize"),    m_MaxConcreteAggSize[pgsTypes::AllLightweight]);
      pSave->EndUnit(); // AllLightweight;
      pSave->BeginUnit(_T("SandLightweight"),2.0);
         pSave->Property(_T("MaxSlabFc"),             m_MaxSlabFc[pgsTypes::SandLightweight]);
         pSave->Property(_T("MaxGirderFci"),          m_MaxSegmentFci[pgsTypes::SandLightweight]);
         pSave->Property(_T("MaxGirderFc"),           m_MaxSegmentFc[pgsTypes::SandLightweight]);
         pSave->Property(_T("MaxClosureFci"),         m_MaxClosureFci[pgsTypes::SandLightweight]); // added in version 2.0
         pSave->Property(_T("MaxClosureFc"),          m_MaxClosureFc[pgsTypes::SandLightweight]); // added in version 2.0
         pSave->Property(_T("MaxConcreteUnitWeight"), m_MaxConcreteUnitWeight[pgsTypes::SandLightweight]);
         pSave->Property(_T("MaxConcreteAggSize"),    m_MaxConcreteAggSize[pgsTypes::SandLightweight]);
      pSave->EndUnit(); // SandLightweight;
   pSave->EndUnit(); // Limits

   // Added in version 44
   pSave->BeginUnit(_T("Warnings"),2.0);
         pSave->Property(_T("DoCheckStirrupSpacingCompatibility"), m_DoCheckStirrupSpacingCompatibility);
         pSave->Property(_T("CheckGirderSag"),m_bCheckSag); // added in version 2
         pSave->Property(_T("SagCamberType"),m_SagCamberType); // added in version 2
   pSave->EndUnit(); // Warnings

   // Added in 14.0 removed in version 41
   //std::_tstring strLimitState[] = {_T("ServiceI"),_T("ServiceIA"),_T("ServiceIII"),_T("StrengthI"),_T("StrengthII"),_T("FatigueI")};

   //pSave->BeginUnit(_T("LoadFactors"),3.0);
   //int nLimitStates = sizeof(strLimitState)/sizeof(std::_tstring); // Added StrengthII in version 26
   //for ( int i = 0; i < nLimitStates; i++ )
   //{
   //   pSave->BeginUnit(strLimitState[i].c_str(),1.0);
   //   
   //   pSave->Property(_T("DCmin"),  m_DCmin[i]);
   //   pSave->Property(_T("DCmax"),  m_DCmax[i]);
   //   pSave->Property(_T("DWmin"),  m_DWmin[i]);
   //   pSave->Property(_T("DWmax"),  m_DWmax[i]);
   //   pSave->Property(_T("LLIMmin"),m_LLIMmin[i]);
   //   pSave->Property(_T("LLIMmax"),m_LLIMmax[i]);

   //   pSave->EndUnit();
   //}
   //pSave->EndUnit();

   // added in version 15
   pSave->Property(_T("EnableSlabOffsetCheck"), m_EnableSlabOffsetCheck);
   pSave->Property(_T("EnableSlabOffsetDesign"), m_EnableSlabOffsetDesign);

   pSave->Property(_T("DesignStrandFillType"), (long)m_DesignStrandFillType);

   // added in version 16
   pSave->Property(_T("EffectiveFlangeWidthMethod"),(long)m_EffFlangeWidthMethod);

//   // added in version 17, removed version 24
//   pSave->Property(_T("SlabOffsetMethod"),(long)m_SlabOffsetMethod);

   // reconfigured in version 37 and added Phi
   pSave->BeginUnit(_T("Shear"),4.0);
      // moved here in version 37
      pSave->Property(_T("LongReinfShearMethod"),(Int16)m_LongReinfShearMethod); // added for version 1.2

      // moved here in version 37
      pSave->Property(_T("IncludeRebarForCapacity"),m_bIncludeRebar_Shear); // added for version 7.0

      // added in version 18 (moved into datablock in version 37)
      pSave->Property(_T("ShearFlowMethod"),(long)m_ShearFlowMethod);
      pSave->Property(_T("MaxInterfaceShearConnectorSpacing"),m_MaxInterfaceShearConnectorSpacing);
      pSave->Property(_T("UseDeckWeightForPc"), m_bUseDeckWeightForPc); // added in version 3 of Shear data block
      pSave->Property(_T("ShearCapacityMethod"),(long)m_ShearCapacityMethod);
      pSave->Property(_T("LimitNetTensionStrainToPositiveValues"), m_bLimitNetTensionStrainToPositiveValues); // added in version 4 of this data block

      // added in version 37
      pSave->BeginUnit(_T("ResistanceFactor"),1.0);
         pSave->Property(_T("Normal"),m_PhiShear[pgsTypes::Normal]);
         pSave->Property(_T("AllLightweight"),m_PhiShear[pgsTypes::AllLightweight]);
         pSave->Property(_T("SandLightweight"),m_PhiShear[pgsTypes::SandLightweight]);
      pSave->EndUnit(); // ResistanceFactor

      // added in version 64
      pSave->BeginUnit(_T("ResistanceFactorDebonded"),2.0);
         pSave->Property(_T("Normal"),m_PhiShearDebonded[pgsTypes::Normal]);
         pSave->Property(_T("AllLightweight"),m_PhiShearDebonded[pgsTypes::AllLightweight]);
         pSave->Property(_T("SandLightweight"),m_PhiShearDebonded[pgsTypes::SandLightweight]);
         pSave->Property(_T("PCI_UHPC"), m_PhiShearDebonded[pgsTypes::PCI_UHPC]); // added v2
         pSave->Property(_T("UHPC"), m_PhiShearDebonded[pgsTypes::UHPC]); // added v2
         pSave->EndUnit(); // ResistanceFactorDebonded

      // Added ClosureJointResistanceFactor in version 2 of Shear data block
      pSave->BeginUnit(_T("ClosureJointResistanceFactor"),1.0);
         pSave->Property(_T("Normal"),m_PhiClosureJointShear[pgsTypes::Normal]);
         pSave->Property(_T("AllLightweight"),m_PhiClosureJointShear[pgsTypes::AllLightweight]);
         pSave->Property(_T("SandLightweight"),m_PhiClosureJointShear[pgsTypes::SandLightweight]);
      pSave->EndUnit(); // ResistanceFactor
   pSave->EndUnit(); // Shear

   // added in version 26
   pSave->Property(_T("PedestrianLoad"),m_PedestrianLoad);
   pSave->Property(_T("MinSidewalkWidth"),m_MinSidewalkWidth);

   // added in vers 32
   pSave->Property(_T("PrestressTransferComputationType"),(long)m_PrestressTransferComputationType);

   pSave->BeginUnit(_T("StrandExtensions"),1.0);
   pSave->Property(_T("AllowStraightStrandExtensions"),m_bAllowStraightStrandExtensions);
   pSave->EndUnit(); // StrandExtensions

   // added in version 52
   pSave->BeginUnit(_T("DuctSize"),1.0);
      pSave->Property(_T("DuctAreaPushRatio"),m_DuctAreaPushRatio);
      pSave->Property(_T("DuctAreaPullRatio"),m_DuctAreaPullRatio);
      pSave->Property(_T("DuctDiameterRatio"),m_DuctDiameterRatio);
   pSave->EndUnit(); // DuctSize

   // added version 50
   pSave->BeginUnit(_T("ClosureJoint"),1.0);
      pSave->Property(_T("ClosureCompStressAtStressing"),             m_ClosureCompStressAtStressing);
      pSave->Property(_T("ClosureTensStressPTZAtStressing"),          m_ClosureTensStressPTZAtStressing);
      pSave->Property(_T("ClosureTensStressPTZWithRebarAtStressing"), m_ClosureTensStressPTZWithRebarAtStressing);
      pSave->Property(_T("ClosureTensStressAtStressing"),             m_ClosureTensStressAtStressing);
      pSave->Property(_T("ClosureTensStressWithRebarAtStressing"),    m_ClosureTensStressWithRebarAtStressing);
      pSave->Property(_T("ClosureCompStressAtService"),               m_ClosureCompStressAtService);
      pSave->Property(_T("ClosureCompStressWithLiveLoadAtService"),   m_ClosureCompStressWithLiveLoadAtService);
      pSave->Property(_T("ClosureTensStressPTZAtService"),            m_ClosureTensStressPTZAtService);
      pSave->Property(_T("ClosureTensStressPTZWithRebarAtService"),   m_ClosureTensStressPTZWithRebarAtService);
      pSave->Property(_T("ClosureTensStressAtService"),               m_ClosureTensStressAtService);
      pSave->Property(_T("ClosureTensStressWithRebarAtService"),      m_ClosureTensStressWithRebarAtService);
      pSave->Property(_T("ClosureCompStressFatigue"),                 m_ClosureCompStressFatigue);
   pSave->EndUnit(); // Closure Joint

   // added in version 48
   pSave->Property(_T("CheckBottomFlangeClearance"),m_bCheckBottomFlangeClearance);
   pSave->Property(_T("MinBottomFlangeClearance"),m_Cmin);

   // added in version 57
   pSave->Property(_T("CheckGirderInclination"), m_bCheckGirderInclination);
   //pSave->Property(_T("InclindedGirder_BrgPadDeduction"), m_InclinedGirder_BrgPadDeduction); // removed in version 71
   pSave->Property(_T("InclindedGirder_FSmax"), m_InclinedGirder_FSmax);

   // added in version 62
   pSave->Property(_T("FinishedElevationTolerance"), m_FinishedElevationTolerance);

   // added in version 73
   pSave->Property(_T("SlabOffsetRoundingMethod"), (long)m_SlabOffsetRoundingMethod);
   pSave->Property(_T("SlabOffsetRoundingTolerance"), m_SlabOffsetRoundingTolerance);

   // added in version 79
   pSave->BeginUnit(_T("Bearings"), 1.0);
      pSave->Property(_T("AlertTaperedSolePlateRequirement"), m_bAlertTaperedSolePlateRequirement);
      pSave->Property(_T("TaperedSolePlaneInclinationThreshold"), m_TaperedSolePlateInclinationThreshold);
      pSave->Property(_T("UseImpactForBearingReactions"), m_bUseImpactForBearingReactions);
   pSave->EndUnit(); // Bearings

   pSave->EndUnit();

   return true;
}

bool SpecLibraryEntry::LoadMe(WBFL::System::IStructuredLoad* pLoad)
{
   Int16 temp;
   pgsTypes::ShearCapacityMethod shear_capacity_method = m_ShearCapacityMethod; // used as a temporary storage for shear capacity method before file version 18

   if(pLoad->BeginUnit(_T("SpecificationLibraryEntry")))
   {
      Float64 version = pLoad->GetVersion();
      if (version < 1.0 || CURRENT_VERSION < version)
      {
         THROW_LOAD(BadVersion,pLoad);
      }

      std::_tstring name;
      if(pLoad->Property(_T("Name"),&name))
      {
         this->SetName(name.c_str());
      }
      else
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if(pLoad->Property(_T("Description"),&name))
      {
         this->SetDescription(name.c_str());
      }
      else
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if (76 < version)
      {
         // added in version 77
         if (!pLoad->Property(_T("UseCurrentSpecification"), &m_bUseCurrentSpecification))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }
      }
      else
      {
         // if loading an older file, set this to false because it wasn't an option
         // and we don't want to change the specification.
         // the default for new entires is true so we have to force the value to false here
         m_bUseCurrentSpecification = false;
      }

      std::_tstring tmp;
      if(pLoad->Property(_T("SpecificationType"),&tmp))
      {
         try
         {
            m_SpecificationType = lrfdVersionMgr::GetVersion(tmp.c_str());
         }
         catch(...)
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }
      }
      else
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }


      if(pLoad->Property(_T("SpecificationUnits"),&tmp))
      {
         if (tmp==_T("SiUnitsSpec"))
         {
            m_SpecificationUnits = lrfdVersionMgr::SI;
         }
         else if(tmp==_T("UsUnitsSpec"))
         {
            m_SpecificationUnits = lrfdVersionMgr::US;
         }
         else
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }
      }
      else
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( 50 <= version )
      {
         // added in version 50
         int value;
         if (!pLoad->Property(_T("SectionPropertyType"),&value))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }
         m_SectionPropertyMode = (pgsTypes::SectionPropertyMode)value;
      }

      if(!pLoad->Property(_T("DoCheckStrandSlope"), &m_DoCheckStrandSlope))
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if (version<15)
      {
         m_DoDesignStrandSlope = m_DoCheckStrandSlope;
      }
      else
      {
         if(!pLoad->Property(_T("DoDesignStrandSlope"), &m_DoDesignStrandSlope))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }
      }

      if(!pLoad->Property(_T("MaxSlope05"), &m_MaxSlope05))
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if(!pLoad->Property(_T("MaxSlope06"), &m_MaxSlope06))
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( 35 <= version )
      {
         if(!pLoad->Property(_T("MaxSlope07"), &m_MaxSlope07))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }
      }

      if(!pLoad->Property(_T("DoCheckHoldDown"), &m_DoCheckHoldDown))
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if (version<15)
      {
         m_DoDesignHoldDown = m_DoCheckHoldDown;
      }
      else
      {
         if(!pLoad->Property(_T("DoDesignHoldDown"), &m_DoDesignHoldDown))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }
      }

      if (69 < version)
      {
         // added in version 70
         if (!pLoad->Property(_T("HoldDownForceType"), &m_HoldDownForceType))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }
      }

      if(!pLoad->Property(_T("HoldDownForce"), &m_HoldDownForce))
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if (69 < version)
      {
         // added in version 70
         if (!pLoad->Property(_T("HoldDownFriction"), &m_HoldDownFriction))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }
      }

      if (32 < version && version < 39)
      {
         // added in version 33 and removed in version 38
         bool check_anchor;
         if(!pLoad->Property(_T("DoCheckAnchorage"), &check_anchor))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         m_DoCheckSplitting = check_anchor;
         m_DoCheckConfinement = check_anchor;
         m_DoDesignSplitting = check_anchor;
         m_DoDesignConfinement = check_anchor;
      }
      else if (39 <= version)
      {
         if(!pLoad->Property(_T("DoCheckSplitting"), &m_DoCheckSplitting))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if(!pLoad->Property(_T("DoDesignSplitting"), &m_DoDesignSplitting))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if(!pLoad->Property(_T("DoCheckConfinement"), &m_DoCheckConfinement))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if(!pLoad->Property(_T("DoDesignConfinement"), &m_DoDesignConfinement))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }
      }

      if (69 < version)
      {
         // added in version 70
         if (!pLoad->Property(_T("DoCheckHandlingWeightLimit"), &m_bCheckHandlingWeightLimit))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }
         
         if (!pLoad->Property(_T("HandlingWeightLimit"), &m_HandlingWeightLimit))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }
      }

      if ( version < 46 || version == 50 )
      {
         // removed in version 46
         // also used in version 50 (on the PGSplice, v3.0 branch)
         Float64 maxStirrupSpacing;
         if(!pLoad->Property(_T("MaxStirrupSpacing"), &maxStirrupSpacing))
            THROW_LOAD(InvalidFileFormat,pLoad);

         m_MaxStirrupSpacing[0] = maxStirrupSpacing;

         if ( m_SpecificationUnits == lrfdVersionMgr::SI )
         {
            // default value in SI units is 300mm
            // default value is US units is 12"
            // 12" = 305mm
            m_MaxStirrupSpacing[1] = WBFL::Units::ConvertToSysUnits(300.0,WBFL::Units::Measure::Millimeter);
         }

      }
      else
      {
         // added in version 46 or 51 (PGSplice, v3.0 branch)
         if ( !pLoad->Property(_T("StirrupSpacingCoefficient1"),&m_StirrupSpacingCoefficient[0]) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->Property(_T("MaxStirrupSpacing1"),&m_MaxStirrupSpacing[0]) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->Property(_T("StirrupSpacingCoefficient2"),&m_StirrupSpacingCoefficient[1]) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->Property(_T("MaxStirrupSpacing2"),&m_MaxStirrupSpacing[1]) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }
      }

      if(!pLoad->Property(_T("CyLiftingCrackFs"), &m_CyLiftingCrackFs))
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if(!pLoad->Property(_T("CyLiftingFailFs"), &m_CyLiftingFailFs))
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if(!pLoad->Property(_T("CyCompStressServ"), &m_CyCompStressServ))
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if (version < 65)
      {
         if (!pLoad->Property(_T("CyCompStressLifting"), &m_LiftingCompressionStressCoefficient_GlobalStress))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }
      }
      else
      {
         if (!pLoad->Property(_T("CyGlobalCompStressLifting"), &m_LiftingCompressionStressCoefficient_GlobalStress))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }

         if (!pLoad->Property(_T("CyPeakCompStressLifting"), &m_LiftingCompressionStressCoefficient_PeakStress))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }

      }

      if(!pLoad->Property(_T("CyTensStressServ"), &m_CyTensStressServ))
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if(!pLoad->Property(_T("CyDoTensStressServMax"), &m_CyDoTensStressServMax))
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if(!pLoad->Property(_T("CyTensStressServMax"), &m_CyTensStressServMax))
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if(!pLoad->Property(_T("CyTensStressLifting"), &m_CyTensStressLifting))
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if(!pLoad->Property(_T("CyDoTensStressLiftingMax"),&m_CyDoTensStressLiftingMax))
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if(!pLoad->Property(_T("CyTensStressLiftingMax"),&m_CyTensStressLiftingMax))
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

     // at version 27 we moved debonding to the girder library. Data here is ignored
	  if ( 6.0 <= version && version < 27)
	  {
        Float64 debond_junk;
		  if (!pLoad->Property(_T("MaxDebondStrands"),&debond_junk))
        {
			 THROW_LOAD(InvalidFileFormat,pLoad);
        }

		  if (!pLoad->Property(_T("MaxDebondStrandsPerRow"),&debond_junk))
        {
			 THROW_LOAD(InvalidFileFormat,pLoad);
        }

        if ( !pLoad->Property(_T("MaxNumDebondedStrandsPerSection"),&debond_junk))
        {
           THROW_LOAD(InvalidFileFormat,pLoad);
        }

        if ( !pLoad->Property(_T("MaxDebondedStrandsPerSection"),&debond_junk))
        {
           THROW_LOAD(InvalidFileFormat,pLoad);
        }

        if ( !pLoad->Property(_T("DefaultDebondLength"),&debond_junk))
        {
           THROW_LOAD(InvalidFileFormat,pLoad);
        }
	  }

      if ( 1.4 <= version )
      {
         if (!pLoad->Property(_T("BurstingZoneLengthFactor"), &m_SplittingZoneLengthFactor))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }
      }

      if (74 < version && version < 82)
      {
         // removed in version 82
         Float64 dummy; // goble up the value since it isn't useful anymore
         if (!pLoad->Property(_T("UHPCStregthAtFirstCrack"), &dummy))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }
      }

      if(!pLoad->Property(_T("LiftingUpwardImpact"),&m_LiftingUpwardImpact))
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if(!pLoad->Property(_T("LiftingDownwardImpact"),&m_LiftingDownwardImpact))
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if(!pLoad->Property(_T("HaulingUpwardImpact"),&m_HaulingUpwardImpact))
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if(!pLoad->Property(_T("HaulingDownwardImpact"),&m_HaulingDownwardImpact))
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( !pLoad->Property(_T("CuringMethod"), &temp ) )
      {
         THROW_LOAD(InvalidFileFormat, pLoad );
      }
      m_CuringMethod = temp;

      if (version<11)
      {
         m_EnableLiftingCheck = true;
         m_EnableLiftingDesign = true;
      }
      else
      {
         if(!pLoad->Property(_T("EnableLiftingCheck"), &m_EnableLiftingCheck))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if (version<15)
         {
            m_EnableLiftingDesign = true;
         }
         else
         {
            if(!pLoad->Property(_T("EnableLiftingDesign"), &m_EnableLiftingDesign))
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }
         }
      }

      if(!pLoad->Property(_T("PickPointHeight"), &m_PickPointHeight))
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if(!pLoad->Property(_T("LiftingLoopTolerance"), &m_LiftingLoopTolerance))
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if(!pLoad->Property(_T("MinCableInclination"), &m_MinCableInclination))
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if(!pLoad->Property(_T("MaxGirderSweepLifting"), &m_MaxGirderSweepLifting))
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( 55 < version )
      {
         // added in version 56, removed in version 65
         if (version < 65)
         {
            Int32 temp;
            if (!pLoad->Property(_T("LiftingCamberMethod"), &temp))
            {
               THROW_LOAD(InvalidFileFormat, pLoad);
            }
            //m_LiftingCamberMethod = (pgsTypes::CamberMethod)temp; ignore value

            Float64 liftingCamberPrecentEstimate;
            if (!pLoad->Property(_T("LiftingCamberPercentEstimate"), &liftingCamberPrecentEstimate/*&m_LiftingCamberPercentEstimate*/))
            {
               THROW_LOAD(InvalidFileFormat, pLoad);
            }
         }

         if (57 < version)
         {
            // added in version 58
            if (!pLoad->Property(_T("LiftingCamberMultiplier"), &m_LiftingCamberMultiplier))
            {
               THROW_LOAD(InvalidFileFormat, pLoad);
            }
         }

         if ( !pLoad->Property(_T("LiftingWindType"),&temp) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }
         m_LiftingWindType = (pgsTypes::WindType)temp;

         if ( !pLoad->Property(_T("LiftingWindLoad"),&m_LiftingWindLoad) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if (version < 59)
         {
            bool bPlumbGirder;
            if (!pLoad->Property(_T("LiftingStressesPlumbGirder"), &bPlumbGirder))
            {
               THROW_LOAD(InvalidFileFormat, pLoad);
            }
            //m_bComputeLiftingStressesAtEquilibriumAngle = !bPlumbGirder; removed in version 65
         }
         else if (version < 65)
         {
            // removed in version 65
            bool bComputeLiftingStressAtEquilibriumAngle;
            if (!pLoad->Property(_T("LiftingStressesEquilibriumAngle"), &bComputeLiftingStressAtEquilibriumAngle/*&m_bComputeLiftingStressesAtEquilibriumAngle*/))
            {
               THROW_LOAD(InvalidFileFormat, pLoad);
            }
         }
      }

      if (version<11)
      {
         m_EnableHaulingCheck = true;
         m_EnableHaulingDesign = true;
      }
      else
      {
         if(!pLoad->Property(_T("EnableHaulingCheck"), &m_EnableHaulingCheck))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if (version<15)
         {
            m_EnableHaulingDesign = true;
         }
         else
         {
            if(!pLoad->Property(_T("EnableHaulingDesign"), &m_EnableHaulingDesign))
               THROW_LOAD(InvalidFileFormat,pLoad);
         }
      }

      if (version < 43)
      {
         m_HaulingAnalysisMethod = pgsTypes::hmWSDOT;
      }
      else
      {
         Int32 tmp;
         if(!pLoad->Property(_T("HaulingAnalysisMethod"), &tmp))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         m_HaulingAnalysisMethod = (pgsTypes::HaulingAnalysisMethod)tmp;
      }

      if(!pLoad->Property(_T("MaxGirderSweepHauling"), &m_MaxGirderSweepHauling))
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if (65 < version)
      {
         // added version 66
         if (!pLoad->Property(_T("SweepGrowthHauling"), &m_HaulingSweepGrowth))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }
      }

      if ( version < 56 )
      {
         m_bHasOldHaulTruck = true;

         // removed in version 56
         if(!pLoad->Property(_T("HaulingSupportDistance"), &m_OldHaulTruck.m_Lmax))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( 2.0 <= version )
         {
            if(!pLoad->Property(_T("MaxHaulingOverhang"), &m_OldHaulTruck.m_MaxOH))
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }
         }
         else
         {
            m_OldHaulTruck.m_MaxOH = WBFL::Units::ConvertToSysUnits(15.0,WBFL::Units::Measure::Feet);
         }
      }

      if(!pLoad->Property(_T("HaulingSupportPlacementTolerance"), &m_HaulingSupportPlacementTolerance))
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( 55 < version )
      {
         // added in version 56, removed version 65
         if (version < 65)
         {
            Int32 temp;
            if (!pLoad->Property(_T("HaulingCamberMethod"), &temp))
            {
               THROW_LOAD(InvalidFileFormat, pLoad);
            }
            //m_HaulingCamberMethod = (pgsTypes::CamberMethod)temp;
         }

      }

      // removed in version 65
      if (version < 65)
      {
         Float64 haulingCamberPrecentEstimate;
         if (!pLoad->Property(_T("HaulingCamberPercentEstimate"), &haulingCamberPrecentEstimate/*&m_HaulingCamberPercentEstimate*/))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }
      }

      // Removed in version 65
      //if ( version < 56 )
      //{
         //// in version 56 this value changed from a whole percentage to a fraction
         //// e.g. 2% became 0.02
         //m_HaulingCamberPercentEstimate /= 100;
      //}

      if (57 < version)
      {
         // added in version 50
         if (!pLoad->Property(_T("HaulingCamberMultiplier"), &m_HaulingCamberMultiplier))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }
      }

      if ( 55 < version )
      {
         Int32 temp;
         if ( !pLoad->Property(_T("HaulingWindType"),&temp) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }
         m_LiftingWindType = (pgsTypes::WindType)temp;

         if ( !pLoad->Property(_T("HaulingWindLoad"),&m_HaulingWindLoad) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->Property(_T("CentrifugalForceType"),&temp) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }
         m_CentrifugalForceType = (pgsTypes::CFType)temp;

         if (!pLoad->Property(_T("HaulingSpeed"),&m_HaulingSpeed) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if (!pLoad->Property(_T("TurningRadius"),&m_TurningRadius) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if (58 < version && version < 65)
         {
            // added in version 59, removed in version 65
            bool bComputeHaulingStressAtEquilibriumAngle;
            if (!pLoad->Property(_T("HaulingStressesEquilibriumAngle"), &bComputeHaulingStressAtEquilibriumAngle/*&m_bComputeHaulingStressesAtEquilibriumAngle*/))
            {
               THROW_LOAD(InvalidFileFormat, pLoad);
            }
         }
      }

      if (version < 65)
      {
         if (!pLoad->Property(_T("CompStressHauling"), &m_GlobalCompStressHauling))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }
      }
      else
      {
         if (!pLoad->Property(_T("GlobalCompStressHauling"), &m_GlobalCompStressHauling))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }

         if (!pLoad->Property(_T("PeakCompStressHauling"), &m_PeakCompStressHauling))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }
      }

      if ( version < 56 )
      {
         if(!pLoad->Property(_T("TensStressHauling"), &m_TensStressHauling[pgsTypes::CrownSlope]))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if(!pLoad->Property(_T("DoTensStressHaulingMax"),&m_DoTensStressHaulingMax[pgsTypes::CrownSlope]))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if(!pLoad->Property(_T("TensStressHaulingMax"), &m_TensStressHaulingMax[pgsTypes::CrownSlope]))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }
      }
      else
      {
         if(!pLoad->Property(_T("TensStressHaulingNormalCrown"), &m_TensStressHauling[pgsTypes::CrownSlope]))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if(!pLoad->Property(_T("DoTensStressHaulingMaxNormalCrown"),&m_DoTensStressHaulingMax[pgsTypes::CrownSlope]))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if(!pLoad->Property(_T("TensStressHaulingMaxNormalCrown"), &m_TensStressHaulingMax[pgsTypes::CrownSlope]))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if(!pLoad->Property(_T("TensStressHaulingMaxSuper"), &m_TensStressHauling[pgsTypes::Superelevation]))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if(!pLoad->Property(_T("DoTensStressHaulingMaxMaxSuper"),&m_DoTensStressHaulingMax[pgsTypes::Superelevation]))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if(!pLoad->Property(_T("TensStressHaulingMaxMaxSuper"), &m_TensStressHaulingMax[pgsTypes::Superelevation]))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }
      }

      if(!pLoad->Property(_T("HeHaulingCrackFs"), &m_HaulingCrackFs))
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if(!pLoad->Property(_T("HeHaulingFailFs"), &m_HaulingRollFs))
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( 55 < version )
      {
         // added in version 56
         Int32 temp;
         if ( !pLoad->Property(_T("HaulingImpactUsage"),&temp) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }
         m_HaulingImpactUsage = (pgsTypes::HaulingImpact)temp;

         if ( !pLoad->Property(_T("RoadwayCrownSlope"),&m_RoadwayCrownSlope))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

      }

      if(!pLoad->Property(_T("RoadwaySuperelevation"), &m_RoadwaySuperelevation))
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( version < 56 )
      {
         // removed in version 56
         if ( version < 1.9 )
         {
            if(!pLoad->Property(_T("TruckRollStiffness"), &m_OldHaulTruck.m_TruckRollStiffness))
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }

            m_OldHaulTruck.m_TruckRollStiffnessMethod = 0;
         }
         else
         {
            long method;
            if(!pLoad->Property(_T("TruckRollStiffnessMethod"), &method))
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }

            m_OldHaulTruck.m_TruckRollStiffnessMethod = (int)method;

            if(!pLoad->Property(_T("TruckRollStiffness"), &m_OldHaulTruck.m_TruckRollStiffness))
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }

            if(!pLoad->Property(_T("AxleWeightLimit"), &m_OldHaulTruck.m_AxleWeightLimit))
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }

            if(!pLoad->Property(_T("AxleStiffness"), &m_OldHaulTruck.m_AxleStiffness))
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }

            if (!pLoad->Property(_T("MinRollStiffness"),&m_OldHaulTruck.m_MinRollStiffness))
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }
         }

         if(!pLoad->Property(_T("TruckGirderHeight"), &m_OldHaulTruck.m_Hbg))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if(!pLoad->Property(_T("TruckRollCenterHeight"), &m_OldHaulTruck.m_Hrc))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if(!pLoad->Property(_T("TruckAxleWidth"), &m_OldHaulTruck.m_Wcc))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }
      }

      if ( version < 55.0 )
      {
         // removed in version 55.0 (this parameters are never used)
         Float64 value;
         if(!pLoad->Property(_T("HeErectionCrackFs"), &value))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if(!pLoad->Property(_T("HeErectionFailFs"), &value))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }
      }

      if ( 1.3 <= version )
      {
         if ( version < 56 )
         {
            // removed in vesrion 56
            if (!pLoad->Property(_T("MaxGirderWgt"), &m_OldHaulTruck.m_MaxWeight))
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }
         }
      }
      else
      {
         m_OldHaulTruck.m_MaxWeight = WBFL::Units::ConvertToSysUnits(200,WBFL::Units::Measure::Kip);
      }

      if ( 52 < version )
      {
         // Added at version 53
         if ( !pLoad->Property(_T("LimitStateConcreteStrength"),(long*)&m_LimitStateConcreteStrength) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }
      }

      if (71 < version)
      {
         // added in version 72
         if (!pLoad->Property(_T("Use90DayConcreteStrength"), &m_bUse90DayConcreteStrength))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }

         if (!pLoad->Property(_T("SlowCuringConcreteStrengthFactor"), &m_90DayConcreteStrengthFactor))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }
      }

      if ( version < 37 )
      {
         // changed in version 37
         if ( 12 <= version )
         {
            if ( !pLoad->Property(_T("HaulingModulusOfRuptureCoefficient"),&m_HaulingModulusOfRuptureCoefficient[pgsTypes::Normal]) )
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }
         }

         if ( 20 <= version )
         {
            if ( !pLoad->Property(_T("LiftingModulusOfRuptureCoefficient"),&m_LiftingModulusOfRuptureCoefficient[pgsTypes::Normal]) )
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }
         }
      }

      if ( 25 <= version )
      {
         // Added at version 25
         if ( !pLoad->Property(_T("MinLiftingPointLocation"),&m_MinLiftPoint) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->Property(_T("LiftingPointLocationAccuracy"),&m_LiftPointAccuracy) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->Property(_T("MinHaulingSupportLocation"),&m_MinHaulPoint) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->Property(_T("HaulingSupportLocationAccuracy"),&m_HaulPointAccuracy) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }
      }

      if ( 43 <= version )
      {
         // KDOT values
         if ( !pLoad->Property(_T("UseMinTruckSupportLocationFactor"),&m_UseMinTruckSupportLocationFactor) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->Property(_T("MinTruckSupportLocationFactor"),&m_MinTruckSupportLocationFactor) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->Property(_T("OverhangGFactor"),&m_OverhangGFactor) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->Property(_T("InteriorGFactor"),&m_InteriorGFactor) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }
      }

      if ( 3.2 < version )
      {
         if (!pLoad->Property(_T("CastingYardTensileStressLimitWithMildRebar"),&m_CyTensStressServWithRebar) )
         {
             THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if (!pLoad->Property(_T("LiftingTensileStressLimitWithMildRebar"),&m_TensStressLiftingWithRebar) )
         {
             THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( version < 56 )
         {
            if (!pLoad->Property(_T("HaulingTensileStressLimitWithMildRebar"),&m_TensStressHaulingWithRebar[pgsTypes::CrownSlope]) )
            {
                THROW_LOAD(InvalidFileFormat,pLoad);
            }
         }
         else
         {
            if (!pLoad->Property(_T("HaulingTensileStressLimitWithMildRebarNormalCrown"),&m_TensStressHaulingWithRebar[pgsTypes::CrownSlope]) )
            {
                THROW_LOAD(InvalidFileFormat,pLoad);
            }

            if (!pLoad->Property(_T("HaulingTensileStressLimitWithMildRebarMaxSuper"),&m_TensStressHaulingWithRebar[pgsTypes::Superelevation]) )
            {
                THROW_LOAD(InvalidFileFormat,pLoad);
            }
         }
      }
      else
      {
          m_CyTensStressServWithRebar  = WBFL::Units::ConvertToSysUnits(0.24,WBFL::Units::Measure::SqrtKSI);
          m_TensStressLiftingWithRebar = WBFL::Units::ConvertToSysUnits(0.24,WBFL::Units::Measure::SqrtKSI);
          m_TensStressHaulingWithRebar[pgsTypes::CrownSlope] = WBFL::Units::ConvertToSysUnits(0.24,WBFL::Units::Measure::SqrtKSI);
          m_TensStressHaulingWithRebar[pgsTypes::Superelevation] = WBFL::Units::ConvertToSysUnits(0.24,WBFL::Units::Measure::SqrtKSI);
      }

      // deal with verson 1.1
      // in this version we added compressive and tensile stress limits for BSS 1 & 2
      // in version 1.0, limits were for all BS stages.
      if (version == 1.0)
      {
         if(!pLoad->Property(_T("BsCompStressServ"), &m_Bs3CompStressServ))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if(!pLoad->Property(_T("BsCompStressService1A"), &m_Bs3CompStressService1A))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         Float64 tmp;
         if(!pLoad->Property(_T("BsCompStressService1B"), &tmp))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         // stage 1 and 2 are permanent loads only
         m_Bs2CompStress = tmp;
         m_Bs1CompStress = tmp;

         if(!pLoad->Property(_T("BsTensStressServNc"), &m_Bs3TensStressServNc))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         m_Bs1TensStress = m_Bs3TensStressServNc;

         if(!pLoad->Property(_T("BsDoTensStressServNcMax"), &m_Bs3DoTensStressServNcMax))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         m_Bs1DoTensStressMax = m_Bs3DoTensStressServNcMax;

         if(!pLoad->Property(_T("BsTensStressServNcMax"), &m_Bs3TensStressServNcMax))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         m_Bs1TensStressMax = m_Bs3TensStressServNcMax;

         if(!pLoad->Property(_T("BsTensStressServSc"),    &m_Bs3TensStressServSc))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }
   
         if(!pLoad->Property(_T("BsDoTensStressServScMax"), &m_Bs3DoTensStressServScMax))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if(!pLoad->Property(_T("BsTensStressServScMax"), &m_Bs3TensStressServScMax))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if(!pLoad->Property(_T("BsMaxGirdersTrafficBarrier"), &m_Bs2MaxGirdersTrafficBarrier))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( version < 55 )
         {
            // this parameter never used... removed in version 55
            Float64 value;
            if(!pLoad->Property(_T("BsMaxGirdersUtility"), &value))
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }
         }

         m_TempStrandRemovalCompStress      = m_Bs1CompStress;
         m_TempStrandRemovalTensStress      = m_Bs1TensStress;
         m_TempStrandRemovalTensStressWithRebar = m_CyTensStressServWithRebar;
         m_TempStrandRemovalDoTensStressMax = m_Bs1DoTensStressMax;
         m_TempStrandRemovalTensStressMax   = m_Bs1TensStressMax;
      }
      else if (version>=1.1)
      {
         if ( 29 < version )
         {
            // added in version 30
            if(!pLoad->Property(_T("TempStrandRemovalCompStress") ,     &m_TempStrandRemovalCompStress))
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }

            if(!pLoad->Property(_T("TempStrandRemovalTensStress") ,     &m_TempStrandRemovalTensStress))
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }

            if(!pLoad->Property(_T("TempStrandRemovalDoTensStressMax"),  &m_TempStrandRemovalDoTensStressMax))
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }

            if(!pLoad->Property(_T("TempStrandRemovalTensStressMax") ,  &m_TempStrandRemovalTensStressMax))
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }

            if ( 48 < version )
            {
               // added version 49
               if(!pLoad->Property(_T("TempStrandRemovalTensStressWithRebar") ,     &m_TempStrandRemovalTensStressWithRebar))
               {
                  m_TempStrandRemovalTensStressWithRebar = m_CyTensStressServWithRebar; // make sure it is set to the defalut value
                  if ( MAX_OVERLAP_VERSION < version )
                  {
                     // This was added in version 49 for PGSuper version 2.9.
                     // At the same time, PGSuper 3.0 was being built. The data block version was
                     // MAX_OVERLAP_VERSION. It is ok to fail for 44 <= version <= MAX_OVERLAP_VERSION. If version is more than MAX_OVERLAP_VERSION
                     // then the data file format is invalid.
                     THROW_LOAD(InvalidFileFormat,pLoad);
                  }
               }
            }

            if ( 46 < version )
            {
               // added in version 47
               if ( !pLoad->Property(_T("CheckTemporaryStresses"),&m_bCheckTemporaryStresses) )
               {
                  m_bCheckTemporaryStresses = true; // make sure it is set to the default value
                  if ( MAX_OVERLAP_VERSION < version )
                  {
                     // This was added in version 47 for PGSuper version 2.9.
                     // At the same time, PGSuper 3.0 was being built. The data block version was
                     // MAX_OVERLAP_VERSION. It is ok to fail for 44 <= version <= MAX_OVERLAP_VERSION. If version is more than MAX_OVERLAP_VERSION
                     // then the data file format is invalid.
                     THROW_LOAD(InvalidFileFormat,pLoad);
                  }
               }
            }

            // for the following 5 items, the m_ was removed from the keyword in version 30
            if(!pLoad->Property(_T("Bs1CompStress") ,     &m_Bs1CompStress))
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }

            if(!pLoad->Property(_T("Bs1TensStress") ,     &m_Bs1TensStress))
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }

            if(!pLoad->Property(_T("Bs1DoTensStressMax"),  &m_Bs1DoTensStressMax))
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }

            if(!pLoad->Property(_T("Bs1TensStressMax") ,  &m_Bs1TensStressMax))
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }

            if(!pLoad->Property(_T("Bs2CompStress") ,     &m_Bs2CompStress))
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }

            if ( 46 < version )
            {
               bool bOKToFail = false;
               if ( version <= MAX_OVERLAP_VERSION )
               {
                  // This was added in version 45 for PGSuper version 2.9.
                  // At the same time, PGSuper 3.0 was being built. The data block version was
                  // MAX_OVERLAP_VERSION. It is ok to fail for 44 <= version <= MAX_OVERLAP_VERSION. If version is more than MAX_OVERLAP_VERSION
                  // then the data file format is invalid.
                  bOKToFail = true;
               }

               // added in version 47
               bool bSucceeded = pLoad->Property(_T("CheckBs2Tension"),&m_bCheckBs2Tension);
               if ( bSucceeded )
               {
                  if ( !pLoad->Property(_T("Bs2TensStress") , &m_Bs2TensStress) )
                     THROW_LOAD(InvalidFileFormat,pLoad);

                  if ( !pLoad->Property(_T("Bs2DoTensStressMax"), &m_Bs2DoTensStressMax) )
                     THROW_LOAD(InvalidFileFormat,pLoad);

                  if ( !pLoad->Property(_T("Bs2TensStressMax") ,  &m_Bs2TensStressMax) )
                     THROW_LOAD(InvalidFileFormat,pLoad);
               }
               else
               {
                  if ( !bOKToFail )
                  {
                     THROW_LOAD(InvalidFileFormat,pLoad);
                  }
               }
            }
         }
         else
         {
            if(!pLoad->Property(_T("m_Bs1CompStress") ,     &m_Bs1CompStress))
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }

            if(!pLoad->Property(_T("m_Bs1TensStress") ,     &m_Bs1TensStress))
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }

            if(!pLoad->Property(_T("m_Bs1DoTensStressMax"),  &m_Bs1DoTensStressMax))
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }

            if(!pLoad->Property(_T("m_Bs1TensStressMax") ,  &m_Bs1TensStressMax))
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }

            if(!pLoad->Property(_T("m_Bs2CompStress") ,     &m_Bs2CompStress))
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }

            m_TempStrandRemovalCompStress      = m_Bs1CompStress;
            m_TempStrandRemovalTensStress      = m_Bs1TensStress;
            m_TempStrandRemovalDoTensStressMax = m_Bs1DoTensStressMax;
            m_TempStrandRemovalTensStressMax   = m_Bs1TensStressMax;
            m_TempStrandRemovalTensStressWithRebar = m_CyTensStressServWithRebar;
         }

         if ( version < 5.0 )
         {
             // this parameters were removed.... just gobble them up if this is an older file
             Float64 dummy;
             if(!pLoad->Property(_T("m_Bs2TensStress") ,     &dummy))
             {
                THROW_LOAD(InvalidFileFormat,pLoad);
             }

             if(!pLoad->Property(_T("m_Bs2DoTensStressMax"),  &dummy))
             {
                THROW_LOAD(InvalidFileFormat,pLoad);
             }

             if(!pLoad->Property(_T("m_Bs2TensStressMax") ,  &dummy))
             {
                THROW_LOAD(InvalidFileFormat,pLoad);
             }
         }

         if ( 36 <= version )
         {
            Int16 value;
            if ( !pLoad->Property(_T("Bs2TrafficBarrierDistributionType"),&value) )
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }
            m_TrafficBarrierDistributionType = (pgsTypes::TrafficBarrierDistribution)value;
         }

         if(!pLoad->Property(_T("Bs2MaxGirdersTrafficBarrier"), &m_Bs2MaxGirdersTrafficBarrier))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( version < 55 )
         {
            // this parameter was never used. removed in version 55
            Float64 value;
            if(!pLoad->Property(_T("Bs2MaxGirdersUtility"), &value))
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }
         }

         if ( 33.0 < version )
         {
            long oldt;
            if(!pLoad->Property(_T("OverlayLoadDistribution"), &oldt))
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }

            m_OverlayLoadDistribution = (pgsTypes::OverlayLoadDistributionType)oldt==pgsTypes::olDistributeTributaryWidth ? 
                                        pgsTypes::olDistributeTributaryWidth : pgsTypes::olDistributeEvenly;
         }

         if ( 53.0 < version )
         {
            Int32 hlct;
            if(!pLoad->Property(_T("HaunchLoadComputationType"), &hlct))
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }

            m_HaunchLoadComputationType = (pgsTypes::HaunchLoadComputationType)hlct;

            if(!pLoad->Property(_T("HaunchLoadCamberTolerance"), &m_HaunchLoadCamberTolerance))
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }
         }

         if ( 59 < version )
         {
            if(!pLoad->Property(_T("HaunchLoadCamberFactor"), &m_HaunchLoadCamberFactor))
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }
         }

         if ( 62 < version )
         {
            Int32 hlct;
            if(!pLoad->Property(_T("HaunchAnalysisComputationType"), &hlct))
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }

            m_HaunchAnalysisSectionPropertiesType = (pgsTypes::HaunchAnalysisSectionPropertiesType)hlct;
         }

         if(!pLoad->Property(_T("Bs3CompStressServ"), &m_Bs3CompStressServ))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if(!pLoad->Property(_T("Bs3CompStressService1A"), &m_Bs3CompStressService1A))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if(!pLoad->Property(_T("Bs3TensStressServNc"), &m_Bs3TensStressServNc))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if(!pLoad->Property(_T("Bs3DoTensStressServNcMax"), &m_Bs3DoTensStressServNcMax))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if(!pLoad->Property(_T("Bs3TensStressServNcMax"), &m_Bs3TensStressServNcMax))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if(!pLoad->Property(_T("Bs3TensStressServSc"),    &m_Bs3TensStressServSc))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if(!pLoad->Property(_T("Bs3DoTensStressServScMax"), &m_Bs3DoTensStressServScMax))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if(!pLoad->Property(_T("Bs3TensStressServScMax"), &m_Bs3TensStressServScMax))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if (75 < version)
         {
            // added in version 76
            Int16 value;
            if (!pLoad->Property(_T("PrincipalTensileStressMethod"), &value))
            {
               THROW_LOAD(InvalidFileFormat, pLoad);
            }
            m_PrincipalTensileStressMethod = (pgsTypes::PrincipalTensileStressMethod)value;

            if (!pLoad->Property(_T("PrincipalTensileStressCoefficient"), &m_PrincipalTensileStressCoefficient))
            {
               THROW_LOAD(InvalidFileFormat, pLoad);
            }

            if (76 < version)
            {
               // added in version 77
               if (!pLoad->Property(_T("PrincipalTensileStressTendonNearnessFactor"), &m_PrincipalTensileStressTendonNearnessFactor))
               {
                  THROW_LOAD(InvalidFileFormat, pLoad);
               }
            }

            if (77 < version)
            {
               // added in version 78
               if (!pLoad->Property(_T("PrincipalTensileStressFcThreshold"), &m_PrincipalTensileStressFcThreshold))
               {
                  THROW_LOAD(InvalidFileFormat, pLoad);
               }
            }

            if (79 < version)
            {
               // added in version 78
               if (!pLoad->Property(_T("PrincipalTensileStressUngroutedMultiplier"), &m_PrincipalTensileStressUngroutedMultiplier))
               {
                  THROW_LOAD(InvalidFileFormat, pLoad);
               }

               if (!pLoad->Property(_T("PrincipalTensileStressGroutedMultiplier"), &m_PrincipalTensileStressGroutedMultiplier))
               {
                  THROW_LOAD(InvalidFileFormat, pLoad);
               }
            }
            else
            {
               // Older versions did not load the duct multipliers -> they were spec dependent. Use the spec version to determine the value
               DeterminePrincipalStressDuctDeductionMultiplier();
            }
         }
         else
         {
            DeterminePrincipalStressDuctDeductionMultiplier();
         }
      }

      if ( 1.4 <= version )
      {
         if ( version < 29 )
         {
            // removed in version 29
            if (!pLoad->Property(_T("Bs3IgnoreRangeOfApplicability"), &m_Bs3IgnoreRangeOfApplicability))
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }
         }

         // this parameter removed in version 18
         if ( version < 18 )
         {
            if (!pLoad->Property(_T("Bs3LRFDShear"),&temp))
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }

            switch(temp)
            {
            case 0: // scmGeneral -> pgsTypes::scmBTTables
               shear_capacity_method = pgsTypes::scmBTTables;
               break;

            case 1: // scmWSDOT -> pgsTypes::scmWSDOT2001
               shear_capacity_method = pgsTypes::scmWSDOT2001;
               break;
               
            case 2: // scmSimplified -> pgsTypes::scmVciVcw
               shear_capacity_method = pgsTypes::scmVciVcw;
               break;

            default:
               ATLASSERT(false);
            }
         }
      }
      else
      {
         shear_capacity_method = pgsTypes::scmBTTables;
      }

      if ( version < 37 )
      {
         if ( 1.8 <= version )
         {
            if (!pLoad->Property(_T("Bs3LRFDOverreinforcedMomentCapacity"),&temp))
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }
            m_Bs3LRFDOverReinforcedMomentCapacity = temp;
         }
      
         if ( 7.0 <= version )
         {
            if (!pLoad->Property(_T("IncludeRebar_MomentCapacity"),&temp))
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }

            m_bIncludeRebar_Moment = (temp == 0 ? false : true);
         }
      }
      else
      {
         if ( !pLoad->BeginUnit(_T("MomentCapacity")) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         Float64 mc_version = pLoad->GetVersion();

         if ( !pLoad->Property(_T("Bs3LRFDOverreinforcedMomentCapacity"),&temp) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         m_Bs3LRFDOverReinforcedMomentCapacity = temp;

         // added in version 4
         if (3 < mc_version)
         {
            if (!pLoad->Property(_T("IncludeStrandForNegMoment"), &temp) )
            {
               THROW_LOAD(InvalidFileFormat, pLoad);
            }
            m_bIncludeStrand_NegMoment = (temp == 0 ? false : true);
         }

         if ( !pLoad->Property(_T("IncludeRebarForCapacity"),&temp) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }
         m_bIncludeRebar_Moment = (temp == 0 ? false : true);


         if (4 < mc_version) // added in version 5
         {
            if (!pLoad->Property(_T("ConsiderReinforcementStrainLimit"), &temp))
            {
               THROW_LOAD(InvalidFileFormat, pLoad);
            }
            m_bConsiderReinforcementStrainLimit = (temp == 0 ? false : true);

            if (!pLoad->Property(_T("MomentCapacitySliceCount"), &m_nMomentCapacitySlices))
            {
               THROW_LOAD(InvalidFileFormat, pLoad);
            }
         }

         if ( 2 <= mc_version )
         {
            if ( !pLoad->Property(_T("IncludeNoncompositeMomentForNegMomentDesign"),&temp) ) // added version 2 of this data block
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }

            m_bIncludeForNegMoment = (temp == 0 ? false : true);
         }

         if ( mc_version < 3 )
         {
            if ( !pLoad->BeginUnit(_T("ReductionFactor")) )
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }
         }
         else
         {
            // fixed spelling error in version 3 of MomentCapacity data block
            if ( !pLoad->BeginUnit(_T("ResistanceFactor")) )
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }
         }

         if ( !pLoad->BeginUnit(_T("NormalWeight")) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->Property(_T("TensionControlled_RC"),&m_PhiFlexureTensionRC[pgsTypes::Normal]) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->Property(_T("TensionControlled_PS"),&m_PhiFlexureTensionPS[pgsTypes::Normal]) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         Float64 nwc_version = pLoad->GetVersion();
         if ( 1.0 < nwc_version )
         {
            if ( !pLoad->Property(_T("TensionControlled_Spliced"),&m_PhiFlexureTensionSpliced[pgsTypes::Normal]) )
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }
         }

         if ( !pLoad->Property(_T("CompressionControlled"),&m_PhiFlexureCompression[pgsTypes::Normal]) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

          if ( !pLoad->EndUnit() ) // NormalWeight
          {
            THROW_LOAD(InvalidFileFormat,pLoad);
          }

         if ( !pLoad->BeginUnit(_T("AllLightweight")) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->Property(_T("TensionControlled_RC"),&m_PhiFlexureTensionRC[pgsTypes::AllLightweight]) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->Property(_T("TensionControlled_PS"),&m_PhiFlexureTensionPS[pgsTypes::AllLightweight]) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         Float64 alc_version = pLoad->GetVersion();
         if ( 1.0 < alc_version )
         {
            if ( !pLoad->Property(_T("TensionControlled_Spliced"),&m_PhiFlexureTensionSpliced[pgsTypes::AllLightweight]) )
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }
         }

         if ( !pLoad->Property(_T("CompressionControlled"),&m_PhiFlexureCompression[pgsTypes::AllLightweight]) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

          if ( !pLoad->EndUnit() ) // AllLightweight
          {
            THROW_LOAD(InvalidFileFormat,pLoad);
          }


         if ( !pLoad->BeginUnit(_T("SandLightweight")) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->Property(_T("TensionControlled_RC"),&m_PhiFlexureTensionRC[pgsTypes::SandLightweight]) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->Property(_T("TensionControlled_PS"),&m_PhiFlexureTensionPS[pgsTypes::SandLightweight]) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         Float64 slc_version = pLoad->GetVersion();
         if ( 1.0 < slc_version )
         {
            if ( !pLoad->Property(_T("TensionControlled_Spliced"),&m_PhiFlexureTensionSpliced[pgsTypes::SandLightweight]) )
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }
         }

         if ( !pLoad->Property(_T("CompressionControlled"),&m_PhiFlexureCompression[pgsTypes::SandLightweight]) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }
         
          if ( !pLoad->EndUnit() ) // SandLightweight
          {
            THROW_LOAD(InvalidFileFormat,pLoad);
          }

          if ( !pLoad->EndUnit() ) // ResistanceFactor
          {
            THROW_LOAD(InvalidFileFormat,pLoad);
          }

          if ( 2 < mc_version )
          {
             // added ClosureJointResistanceFactor in version 3 of the MomentCapacity data block
             if ( !pLoad->BeginUnit(_T("ClosureJointResistanceFactor")) )
             {
                THROW_LOAD(InvalidFileFormat,pLoad);
             }

             if ( !pLoad->BeginUnit(_T("NormalWeight")) )
             {
                THROW_LOAD(InvalidFileFormat,pLoad);
             }

             if ( !pLoad->Property(_T("FullyBondedTendons"),&m_PhiClosureJointFlexure[pgsTypes::Normal]) )
             {
                THROW_LOAD(InvalidFileFormat,pLoad);
             }

             if ( !pLoad->EndUnit() ) // NormalWeight
             {
                THROW_LOAD(InvalidFileFormat,pLoad);
             }

             if ( !pLoad->BeginUnit(_T("AllLightweight")) )
             {
                THROW_LOAD(InvalidFileFormat,pLoad);
             }

             if ( !pLoad->Property(_T("FullyBondedTendons"),&m_PhiClosureJointFlexure[pgsTypes::AllLightweight]) )
             {
                THROW_LOAD(InvalidFileFormat,pLoad);
             }

             if ( !pLoad->EndUnit() ) // AllLightweight
             {
                THROW_LOAD(InvalidFileFormat,pLoad);
             }

             if ( !pLoad->BeginUnit(_T("SandLightweight")) )
             {
                THROW_LOAD(InvalidFileFormat,pLoad);
             }

             if ( !pLoad->Property(_T("FullyBondedTendons"),&m_PhiClosureJointFlexure[pgsTypes::SandLightweight]) )
             {
                THROW_LOAD(InvalidFileFormat,pLoad);
             }

             if ( !pLoad->EndUnit() ) // SandLightweight
             {
                THROW_LOAD(InvalidFileFormat,pLoad);
             }

             if ( !pLoad->EndUnit() ) // ClosureJointResistanceFactor
             {
                THROW_LOAD(InvalidFileFormat,pLoad);
             }
          }

        if ( !pLoad->EndUnit() ) // MomentCapacity
        {
            THROW_LOAD(InvalidFileFormat,pLoad);
        }
      }


      if ( version < 37 )
      {
         if ( 9.0 <= version )
         {
            if (!pLoad->Property(_T("ModulusOfRuptureCoefficient"),&m_FlexureModulusOfRuptureCoefficient[pgsTypes::Normal]))
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }
         }

         if ( 18 <= version )
         {
            // added in version 18
            if ( !pLoad->Property(_T("ShearModulusOfRuptureCoefficient"),&m_ShearModulusOfRuptureCoefficient[pgsTypes::Normal])) 
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }
         }
      }
      else 
      {
         if ( !pLoad->BeginUnit(_T("ModulusOfRuptureCoefficient")) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->BeginUnit(_T("Moment")) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         Float64 moment_version = pLoad->GetVersion();

         if ( !pLoad->Property(_T("Normal"),&m_FlexureModulusOfRuptureCoefficient[pgsTypes::Normal]) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->Property(_T("AllLightweight"),&m_FlexureModulusOfRuptureCoefficient[pgsTypes::AllLightweight]) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->Property(_T("SandLightweight"),&m_FlexureModulusOfRuptureCoefficient[pgsTypes::SandLightweight]) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if (1 < moment_version && moment_version < 3)
         {
            // added in version 2, removed in version 3
            // we are throwing out this value because it is no longer relavent
            Float64 dummy;
            if (!pLoad->Property(_T("UHPC"), &dummy/*&m_FlexureModulusOfRuptureCoefficient[pgsTypes::PCI_UHPC]*/))
            {
               THROW_LOAD(InvalidFileFormat, pLoad);
            }
         }

         if ( !pLoad->EndUnit() ) // Moment
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->BeginUnit(_T("Shear")) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         Float64 shear_version = pLoad->GetVersion();

         if ( !pLoad->Property(_T("Normal"),&m_ShearModulusOfRuptureCoefficient[pgsTypes::Normal]) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->Property(_T("AllLightweight"),&m_ShearModulusOfRuptureCoefficient[pgsTypes::AllLightweight]) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->Property(_T("SandLightweight"),&m_ShearModulusOfRuptureCoefficient[pgsTypes::SandLightweight]) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if (1 < shear_version && version < 3)
         {
            // added in version 2, removed in version 3
            // we are throwing out this value because it is no longer relavent
            Float64 dummy;
            if (!pLoad->Property(_T("UHPC"), &dummy/*&m_ShearModulusOfRuptureCoefficient[pgsTypes::PCI_UHPC]*/))
            {
               THROW_LOAD(InvalidFileFormat, pLoad);
            }
         }

         if ( !pLoad->EndUnit() ) // Shear
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->BeginUnit(_T("Lifting")) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }
         
         Float64 lifting_version = pLoad->GetVersion();

         if ( !pLoad->Property(_T("Normal"),&m_LiftingModulusOfRuptureCoefficient[pgsTypes::Normal]) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->Property(_T("AllLightweight"),&m_LiftingModulusOfRuptureCoefficient[pgsTypes::AllLightweight]) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->Property(_T("SandLightweight"),&m_LiftingModulusOfRuptureCoefficient[pgsTypes::SandLightweight]) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if (1 < lifting_version && lifting_version < 3)
         {
            // added in version 2, removed in version 3
            Float64 dummy;
            if (!pLoad->Property(_T("UHPC"), &dummy/*&m_LiftingModulusOfRuptureCoefficient[pgsTypes::PCI_UHPC]*/))
            {
               THROW_LOAD(InvalidFileFormat, pLoad);
            }
         }

         if ( !pLoad->EndUnit() ) // Lifting
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }


         if ( !pLoad->BeginUnit(_T("Shipping")) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         Float64 shipping_version = pLoad->GetVersion();

         if ( !pLoad->Property(_T("Normal"),&m_HaulingModulusOfRuptureCoefficient[pgsTypes::Normal]) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->Property(_T("AllLightweight"),&m_HaulingModulusOfRuptureCoefficient[pgsTypes::AllLightweight]) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->Property(_T("SandLightweight"),&m_HaulingModulusOfRuptureCoefficient[pgsTypes::SandLightweight]) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if (1 < shipping_version && shipping_version < 3)
         {
            // added in version 2, removed in version 3
            Float64 dummy;
            if (!pLoad->Property(_T("UHPC"), &dummy/*&m_HaulingModulusOfRuptureCoefficient[pgsTypes::PCI_UHPC]*/))
            {
               THROW_LOAD(InvalidFileFormat, pLoad);
            }
         }

         if ( !pLoad->EndUnit() ) // Shipping
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->EndUnit() ) // ModulusOfRuptureCoefficient
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }
      }

      if (74 < version && version < 82)
      {
         Float64 dummy;
         if (!pLoad->Property(_T("UHPCFiberShearStrength"), &dummy/*&m_UHPCFiberShearStrength*/)) // added in version 75, removed in version 82
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }
      }

      if ( !pLoad->Property(_T("BsLldfMethod"), &temp ) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }
      m_LldfMethod = temp;

      if (73 < version)
      {
         // added in version 74
         if (!pLoad->Property(_T("IgnoreSkewReductionForMoment"), &m_bIgnoreSkewReductionForMoment))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }
      }

      if ( 28 < version )
      {
         // added in version 29
         if ( !pLoad->Property(_T("MaxAngularDeviationBetweenGirders"),&m_MaxAngularDeviationBetweenGirders) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }
   
         if ( !pLoad->Property(_T("MinGirderStiffnessRatio"),&m_MinGirderStiffnessRatio) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->Property(_T("LLDFGirderSpacingLocation"),&m_LLDFGirderSpacingLocation) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }
      }

      if (69 < version)
      {
         // added in version 70
         if (!pLoad->Property(_T("UseRigidMethod"), &m_bUseRigidMethod))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }
      }

      if (60 < version)
      {
         // added in version 61
         if (!pLoad->Property(_T("IncludeDualTandem"), &m_bIncludeDualTandem))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }
      }

      if ( 30 < version )
      {
         if ( !pLoad->Property(_T("LimitDistributionFactorsToLanesBeams"),&m_LimitDistributionFactorsToLanesBeams) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }
      }

      if ( 80 < version )
      {
         if ( !pLoad->Property(_T("ExteriorLiveLoadDistributionGTAdjacentInteriorRule"),&m_ExteriorLiveLoadDistributionGTAdjacentInteriorRule) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }
      }

      if ( version < 37 )
      {
         // moved below in version 37

         // added longitudinal reinforcement for shear method to version 1.2 (this was the only change)
         if ( 1.2 <= version )
         {
            if ( !pLoad->Property(_T("LongReinfShearMethod"), &temp ) )
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }
            m_LongReinfShearMethod = temp;

            // WSDOT method has been recinded
            m_LongReinfShearMethod = LRFD_METHOD;
         }
         else
         {
            m_LongReinfShearMethod = LRFD_METHOD;
         }
 
   
         if ( 7.0 <= version )
         {
            if (!pLoad->Property(_T("IncludeRebar_Shear"),&temp))
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }

            m_bIncludeRebar_Shear = (temp == 0 ? false : true);
         }
      }

      if ( !pLoad->Property(_T("CreepMethod"),&temp) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }
      m_CreepMethod = temp;

      if (!pLoad->Property(_T("CreepFactor"),&m_CreepFactor))
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      // WSDOT has abandonded it's creep method and use the LRFD calculations
      // 2006
      m_CreepMethod = CREEP_LRFD;

      if ( 3.0 <= version )
      {
         if (!pLoad->Property(_T("CreepDuration1Min"),&m_CreepDuration1Min))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if (!pLoad->Property(_T("CreepDuration1Max"),&m_CreepDuration1Max))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if (!pLoad->Property(_T("CreepDuration2Min"),&m_CreepDuration2Min))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if (!pLoad->Property(_T("CreepDuration2Max"),&m_CreepDuration2Max))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->Property(_T("XferTime"),&m_XferTime))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( 3.1 <= version && version < 3.2 )
         {
            if ( !pLoad->Property(_T("NoncompositeCreepDuration"),&m_TotalCreepDuration))
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }
         }
         if ( 3.2 <= version )
         {
            if ( !pLoad->Property(_T("TotalCreepDuration"),&m_TotalCreepDuration))
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }
         }
      }
      else if (1.6 <= version && version < 3.0)
      {
         if (!pLoad->Property(_T("CreepDuration1"),&m_CreepDuration1Min))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         m_CreepDuration1Max = m_CreepDuration1Min;

         if (!pLoad->Property(_T("CreepDuration2"),&m_CreepDuration2Min))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         m_CreepDuration2Max = m_CreepDuration2Min;

         if ( !pLoad->Property(_T("XferTime"),&m_XferTime))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }
      }
      else
      {
         // only one creep duration was entered - need to make some assumptions here
         if (!pLoad->Property(_T("CreepDuration"),&m_CreepDuration2Min))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         m_CreepDuration2Max = m_CreepDuration2Min;

         if ( !pLoad->Property(_T("XferTime"),&m_XferTime))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         Float64 duration_days = WBFL::Units::ConvertFromSysUnits(m_CreepDuration2Min,WBFL::Units::Measure::Day);
         Float64 xfer_days     = WBFL::Units::ConvertFromSysUnits(m_XferTime,WBFL::Units::Measure::Day);
         if (duration_days-30 > xfer_days)
         {
            m_CreepDuration1Min = WBFL::Units::ConvertToSysUnits(duration_days-30,WBFL::Units::Measure::Day);
            m_CreepDuration1Max = m_CreepDuration1Min;
         }
         else
         {
            m_CreepDuration1Min = (m_CreepDuration2Min+m_XferTime) / 2.0;
            m_CreepDuration1Max = m_CreepDuration1Min;
         }
      }

      if ( 44 <= version )
      {
         if ( !pLoad->Property(_T("CamberVariability"),&m_CamberVariability))
         {
            m_CamberVariability = 0.50; // the call failed, make sure the variable is set to its default value (it gets changed in the call)

            // it is ok to fail it version is 50 or less
            if ( version < 50 )
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }
         }
      }
 
      if ( !pLoad->Property(_T("LossMethod"),&temp) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad );
      }

      m_LossMethod = temp;

      if ( 50 <= version )
      {
         // added in version 50
         if ( !pLoad->Property(_T("TimeDependentModel"),&temp) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         m_TimeDependentModel = temp;
      }

      if ( 50 <= version )
      {
         if ( !pLoad->Property(_T("ShippingLosses"),&m_ShippingLosses) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad );
         }

         if ( !pLoad->Property(_T("ShippingTime"),&m_ShippingTime) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad );
         }
      }
      else
      {
         if ( !pLoad->Property(_T("FinalLosses"),&m_FinalLosses) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad );
         }

         if ( !pLoad->Property(_T("ShippingLosses"),&m_ShippingLosses) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad );
         }

         if ( !pLoad->Property(_T("BeforeXferLosses"),&m_BeforeXferLosses) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad );
         }

         if ( !pLoad->Property(_T("AfterXferLosses"),&m_AfterXferLosses) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad );
         }

         if ( 13.0 <= version )
         {
            if ( !pLoad->Property(_T("ShippingTime"),&m_ShippingTime) )
            {
               THROW_LOAD(InvalidFileFormat,pLoad );
            }
         }

         if ( 22 <= version )
         {
            if ( !pLoad->Property(_T("LiftingLosses"),&m_LiftingLosses) )
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }

            if ( !pLoad->Property(_T("BeforeTempStrandRemovalLosses"),&m_BeforeTempStrandRemovalLosses) )
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }

            if ( !pLoad->Property(_T("AfterTempStrandRemovalLosses"),&m_AfterTempStrandRemovalLosses) )
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }

            if ( !pLoad->Property(_T("AfterDeckPlacementLosses"),&m_AfterDeckPlacementLosses) )
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }

            if ( 38 <= version )
            {
               if ( !pLoad->Property(_T("AfterSIDLLosses"),&m_AfterSIDLLosses) )
               {
                  THROW_LOAD(InvalidFileFormat,pLoad);
               }
            }
            else
            {
               m_AfterSIDLLosses = m_AfterDeckPlacementLosses;
            }
         }
         else
         {
            m_LiftingLosses                 = m_AfterXferLosses;
            m_BeforeTempStrandRemovalLosses = m_ShippingLosses < 0 ? m_LiftingLosses : m_ShippingLosses;
            m_AfterTempStrandRemovalLosses  = m_BeforeTempStrandRemovalLosses;
            m_AfterDeckPlacementLosses      = m_FinalLosses;
            m_AfterSIDLLosses               = m_FinalLosses;
         }
      }

      if ( 19 <= version )
      {
         if ( !pLoad->Property(_T("CuringMethodFactor"),&m_CuringMethodTimeAdjustmentFactor) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }
      }

      // added in version 1.6
      if ( 1.5 <= version )
      {
         if (!pLoad->Property(_T("CheckStrandStressAtJacking"),&m_bCheckStrandStress[CSS_AT_JACKING]))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if (!pLoad->Property(_T("Coeff_AtJacking_StressRel"),&m_StrandStressCoeff[CSS_AT_JACKING][STRESS_REL]))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if (!pLoad->Property(_T("Coeff_AtJacking_LowRelax"),&m_StrandStressCoeff[CSS_AT_JACKING][LOW_RELAX]))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }


         if (!pLoad->Property(_T("CheckStrandStressBeforeTransfer"),&m_bCheckStrandStress[CSS_BEFORE_TRANSFER]))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if (!pLoad->Property(_T("Coeff_BeforeTransfer_StressRel"),&m_StrandStressCoeff[CSS_BEFORE_TRANSFER][STRESS_REL]))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if (!pLoad->Property(_T("Coeff_BeforeTransfer_LowRelax"),&m_StrandStressCoeff[CSS_BEFORE_TRANSFER][LOW_RELAX]))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }


         if (!pLoad->Property(_T("CheckStrandStressAfterTransfer"),&m_bCheckStrandStress[CSS_AFTER_TRANSFER]))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if (!pLoad->Property(_T("Coeff_AfterTransfer_StressRel"),&m_StrandStressCoeff[CSS_AFTER_TRANSFER][STRESS_REL]))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if (!pLoad->Property(_T("Coeff_AfterTransfer_LowRelax"),&m_StrandStressCoeff[CSS_AFTER_TRANSFER][LOW_RELAX]))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }


         if (!pLoad->Property(_T("CheckStrandStressAfterAllLosses"),&m_bCheckStrandStress[CSS_AFTER_ALL_LOSSES]))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if (!pLoad->Property(_T("Coeff_AfterAllLosses_StressRel"),&m_StrandStressCoeff[CSS_AFTER_ALL_LOSSES][STRESS_REL]))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if (!pLoad->Property(_T("Coeff_AfterAllLosses_LowRelax"),&m_StrandStressCoeff[CSS_AFTER_ALL_LOSSES][LOW_RELAX]))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

      }
      else
      {
         switch( m_SpecificationType )
         {
         case lrfdVersionMgr::FirstEdition1994:
            m_bCheckStrandStress[CSS_AT_JACKING]       = true;
            m_bCheckStrandStress[CSS_BEFORE_TRANSFER]  = false;
            m_bCheckStrandStress[CSS_AFTER_TRANSFER]   = true;
            m_bCheckStrandStress[CSS_AFTER_ALL_LOSSES] = true;
            break;

         case lrfdVersionMgr::FirstEditionWith1996Interims:
         case lrfdVersionMgr::FirstEditionWith1997Interims:
         case lrfdVersionMgr::SecondEdition1998:
         case lrfdVersionMgr::SecondEditionWith1999Interims:
         case lrfdVersionMgr::SecondEditionWith2000Interims:
         case lrfdVersionMgr::SecondEditionWith2001Interims:
            m_bCheckStrandStress[CSS_AT_JACKING]       = false;
            m_bCheckStrandStress[CSS_BEFORE_TRANSFER]  = true;
            m_bCheckStrandStress[CSS_AFTER_TRANSFER]   = false;
            m_bCheckStrandStress[CSS_AFTER_ALL_LOSSES] = true;
            break;
         }
      }

      if ( 50 <= version )
      {
         // added in version 50
         if ( !pLoad->Property(_T("CheckTendonStressAtJacking"),&m_bCheckTendonStressAtJacking) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->Property(_T("CheckTendonStressPriorToSeating"),&m_bCheckTendonStressPriorToSeating) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->Property(_T("Coeff_AtJacking_StressRel"),&m_TendonStressCoeff[CSS_AT_JACKING][STRESS_REL]) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->Property(_T("Coeff_AtJacking_LowRelax"),&m_TendonStressCoeff[CSS_AT_JACKING][LOW_RELAX]) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->Property(_T("Coeff_PriorToSeating_StressRel"),&m_TendonStressCoeff[CSS_PRIOR_TO_SEATING][STRESS_REL]) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->Property(_T("Coeff_PriorToSeating_LowRelax"),&m_TendonStressCoeff[CSS_PRIOR_TO_SEATING][LOW_RELAX]) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->Property(_T("Coeff_AtAnchoragesAfterSeating_StressRel"),&m_TendonStressCoeff[CSS_ANCHORAGES_AFTER_SEATING][STRESS_REL]) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->Property(_T("Coeff_AtAnchoragesAfterSeating_LowRelax"),&m_TendonStressCoeff[CSS_ANCHORAGES_AFTER_SEATING][LOW_RELAX]) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->Property(_T("Coeff_ElsewhereAfterSeating_StressRel"),&m_TendonStressCoeff[CSS_ELSEWHERE_AFTER_SEATING][STRESS_REL]) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->Property(_T("Coeff_ElsewhereAfterSeating_LowRelax"),&m_TendonStressCoeff[CSS_ELSEWHERE_AFTER_SEATING][LOW_RELAX]) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->Property(_T("Coeff_AfterAllLosses_StressRel"),&m_TendonStressCoeff[CSS_AFTER_ALL_LOSSES][STRESS_REL]) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->Property(_T("Coeff_AfterAllLosses_LowRelax"),&m_TendonStressCoeff[CSS_AFTER_ALL_LOSSES][LOW_RELAX]) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }
      }

      if ( 22 < version && version < 50)
      {
         // added in version 23 and removed in version 50
         m_bUpdatePTParameters = true;
         if ( !pLoad->Property(_T("AnchorSet"),&m_Dset) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->Property(_T("WobbleFriction"),&m_WobbleFriction) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->Property(_T("CoefficientOfFriction"),&m_FrictionCoefficient) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }
      }

      if ( 39 < version )
      {
         // added in version 40
         if ( !pLoad->Property(_T("RelaxationLossMethod"),&m_RelaxationLossMethod) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->Property(_T("SlabElasticGain"),&m_SlabElasticGain) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->Property(_T("HaunchElasticGain"),&m_SlabPadElasticGain) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->Property(_T("DiaphragmElasticGain"),&m_DiaphragmElasticGain) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->Property(_T("UserDCElasticGainBS1"),&m_UserDCElasticGainBS1) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->Property(_T("UserDWElasticGainBS1"),&m_UserDWElasticGainBS1) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->Property(_T("UserDCElasticGainBS2"),&m_UserDCElasticGainBS2) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->Property(_T("UserDWElasticGainBS2"),&m_UserDWElasticGainBS2) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->Property(_T("RailingSystemElasticGain"),&m_RailingSystemElasticGain) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->Property(_T("OverlayElasticGain"),&m_OverlayElasticGain) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->Property(_T("SlabShrinkageElasticGain"),&m_SlabShrinkageElasticGain) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( 41 < version )
         {
            // added version 42
            if ( !pLoad->Property(_T("LiveLoadElasticGain"),&m_LiveLoadElasticGain) )
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }
         }

         if ( 44 < version )
         {
            if( m_LossMethod == LOSSES_TXDOT_REFINED_2013 )
            {
               if ( !pLoad->Property(_T("FcgpComputationMethod"),&m_FcgpComputationMethod) )
               {
                  if ( MAX_OVERLAP_VERSION < version )
                  {
                     // This was added in version 45 for PGSuper version 2.9.
                     // At the same time, PGSuper 3.0 was being built. The data block version was
                     // MAX_OVERLAP_VERSION. It is ok to fail for 44 <= version <= MAX_OVERLAP_VERSION. If version is more than MAX_OVERLAP_VERSION
                     // then the data file format is invalid.
                     THROW_LOAD(InvalidFileFormat,pLoad);
                  }
               }
            }
         }
      }

      // added in version 1.7
      if ( 1.6 < version )
      {
         if (!pLoad->Property(_T("CheckLiveLoadDeflection"),&m_bDoEvaluateDeflection))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if (!pLoad->Property(_T("LiveLoadDeflectionLimit"),&m_DeflectionLimit))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

      }
      else
      {
         m_bDoEvaluateDeflection = true;
         m_DeflectionLimit = 800.0;
      }

      // added in version 8.0 and removed in version 28
      if ( 8.0 <= version && version < 28 )
      {
         long value;
         if ( !pLoad->Property(_T("AnalysisType"),&value) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         m_AnalysisType = (pgsTypes::AnalysisType)(value);
      }

      if ( 10.0 <= version && version < 37 )
      {
         if ( !pLoad->Property(_T("MaxSlabFc"),&m_MaxSlabFc[pgsTypes::Normal]) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->Property(_T("MaxGirderFci"),&m_MaxSegmentFci[pgsTypes::Normal]) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->Property(_T("MaxGirderFc"),&m_MaxSegmentFc[pgsTypes::Normal]) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->Property(_T("MaxConcreteUnitWeight"),&m_MaxConcreteUnitWeight[pgsTypes::Normal]) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->Property(_T("MaxConcreteAggSize"),&m_MaxConcreteAggSize[pgsTypes::Normal]) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }
      }
      else if ( 37 <= version )
      {
         if ( !pLoad->BeginUnit(_T("Limits")) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

            if ( !pLoad->BeginUnit(_T("Normal")) )
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }

               Float64 normal_version = pLoad->GetVersion();

               if ( !pLoad->Property(_T("MaxSlabFc"),&m_MaxSlabFc[pgsTypes::Normal]) )
               {
                  THROW_LOAD(InvalidFileFormat,pLoad);
               }

               if ( !pLoad->Property(_T("MaxGirderFci"),&m_MaxSegmentFci[pgsTypes::Normal]) )
               {
                  THROW_LOAD(InvalidFileFormat,pLoad);
               }

               if ( !pLoad->Property(_T("MaxGirderFc"),&m_MaxSegmentFc[pgsTypes::Normal]) )
               {
                  THROW_LOAD(InvalidFileFormat,pLoad);
               }

               if ( 1.0 < normal_version ) // added in version 2.0
               {
                  if ( !pLoad->Property(_T("MaxClosureFci"),&m_MaxClosureFci[pgsTypes::Normal]) )
                  {
                     THROW_LOAD(InvalidFileFormat,pLoad);
                  }

                  if ( !pLoad->Property(_T("MaxClosureFc"),&m_MaxClosureFc[pgsTypes::Normal]) )
                  {
                     THROW_LOAD(InvalidFileFormat,pLoad);
                  }
               }

               if ( !pLoad->Property(_T("MaxConcreteUnitWeight"),&m_MaxConcreteUnitWeight[pgsTypes::Normal]) )
               {
                  THROW_LOAD(InvalidFileFormat,pLoad);
               }

               if ( !pLoad->Property(_T("MaxConcreteAggSize"),&m_MaxConcreteAggSize[pgsTypes::Normal]) )
               {
                  THROW_LOAD(InvalidFileFormat,pLoad);
               }

            if ( !pLoad->EndUnit() ) // Normal
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }


            if ( !pLoad->BeginUnit(_T("AllLightweight")) )
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }

               Float64 alw_version = pLoad->GetVersion();

               if ( !pLoad->Property(_T("MaxSlabFc"),&m_MaxSlabFc[pgsTypes::AllLightweight]) )
               {
                  THROW_LOAD(InvalidFileFormat,pLoad);
               }

               if ( !pLoad->Property(_T("MaxGirderFci"),&m_MaxSegmentFci[pgsTypes::AllLightweight]) )
               {
                  THROW_LOAD(InvalidFileFormat,pLoad);
               }

               if ( !pLoad->Property(_T("MaxGirderFc"),&m_MaxSegmentFc[pgsTypes::AllLightweight]) )
               {
                  THROW_LOAD(InvalidFileFormat,pLoad);
               }

               if ( 1.0 < alw_version ) // added in version 2.0
               {
                  if ( !pLoad->Property(_T("MaxClosureFci"),&m_MaxClosureFci[pgsTypes::AllLightweight]) )
                  {
                     THROW_LOAD(InvalidFileFormat,pLoad);
                  }

                  if ( !pLoad->Property(_T("MaxClosureFc"),&m_MaxClosureFc[pgsTypes::AllLightweight]) )
                  {
                     THROW_LOAD(InvalidFileFormat,pLoad);
                  }
               }

               if ( !pLoad->Property(_T("MaxConcreteUnitWeight"),&m_MaxConcreteUnitWeight[pgsTypes::AllLightweight]) )
               {
                  THROW_LOAD(InvalidFileFormat,pLoad);
               }

               if ( !pLoad->Property(_T("MaxConcreteAggSize"),&m_MaxConcreteAggSize[pgsTypes::AllLightweight]) )
               {
                  THROW_LOAD(InvalidFileFormat,pLoad);
               }

            if ( !pLoad->EndUnit() ) // AllLightweight
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }

            if ( !pLoad->BeginUnit(_T("SandLightweight")) )
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }

               Float64 slw_version = pLoad->GetVersion();

               if ( !pLoad->Property(_T("MaxSlabFc"),&m_MaxSlabFc[pgsTypes::SandLightweight]) )
               {
                  THROW_LOAD(InvalidFileFormat,pLoad);
               }

               if ( !pLoad->Property(_T("MaxGirderFci"),&m_MaxSegmentFci[pgsTypes::SandLightweight]) )
               {
                  THROW_LOAD(InvalidFileFormat,pLoad);
               }

               if ( !pLoad->Property(_T("MaxGirderFc"),&m_MaxSegmentFc[pgsTypes::SandLightweight]) )
               {
                  THROW_LOAD(InvalidFileFormat,pLoad);
               }

               if ( 1.0 < slw_version ) // added in version 2.0
               {
                  if ( !pLoad->Property(_T("MaxClosureFci"),&m_MaxClosureFci[pgsTypes::SandLightweight]) )
                  {
                     THROW_LOAD(InvalidFileFormat,pLoad);
                  }

                  if ( !pLoad->Property(_T("MaxClosureFc"),&m_MaxClosureFc[pgsTypes::SandLightweight]) )
                  {
                     THROW_LOAD(InvalidFileFormat,pLoad);
                  }
               }

               if ( !pLoad->Property(_T("MaxConcreteUnitWeight"),&m_MaxConcreteUnitWeight[pgsTypes::SandLightweight]) )
               {
                  THROW_LOAD(InvalidFileFormat,pLoad);
               }

               if ( !pLoad->Property(_T("MaxConcreteAggSize"),&m_MaxConcreteAggSize[pgsTypes::SandLightweight]) )
               {
                  THROW_LOAD(InvalidFileFormat,pLoad);
               }

            if ( !pLoad->EndUnit() ) // SandLightweight
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }

         if ( !pLoad->EndUnit() ) // Limits
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }
      }

      if ( 44 <= version )
      {
         bool bOKToFail = false;
         if ( version <= MAX_OVERLAP_VERSION )
         {
            // This was added in version 45 for PGSuper version 2.9.
            // At the same time, PGSuper 3.0 was being built. The data block version was
            // MAX_OVERLAP_VERSION. It is ok to fail for 44 <= version <= MAX_OVERLAP_VERSION. If version is more than MAX_OVERLAP_VERSION
            // then the data file format is invalid.
            bOKToFail = true;
         }

         bool bBeginUnit = pLoad->BeginUnit(_T("Warnings"));
         if ( !bBeginUnit && !bOKToFail)
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( bBeginUnit )
         {
            // reading the unit start successfully... keep going
            if ( !pLoad->Property(_T("DoCheckStirrupSpacingCompatibility"),&m_DoCheckStirrupSpacingCompatibility) )
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }

            Float64 warningsVersion = pLoad->GetVersion();
            if ( 2 <= warningsVersion )
            {
               if ( !pLoad->Property(_T("CheckGirderSag"),&m_bCheckSag) )
                  THROW_LOAD(InvalidFileFormat,pLoad);
   
               int value;
               if ( !pLoad->Property(_T("SagCamberType"),&value) )
                  THROW_LOAD(InvalidFileFormat,pLoad);
   
               m_SagCamberType = (pgsTypes::SagCamberType)value;
            }

            if ( !pLoad->EndUnit() ) // Warnings
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }
         }
      }

      // Added in 14.0, removed in version 41
      if ( 14.0 <= version && version < 41)
      {
         std::_tstring strLimitState[] = {_T("ServiceI"),_T("ServiceIA"),_T("ServiceIII"),_T("StrengthI"),_T("StrengthII"),_T("FatigueI")};

         pLoad->BeginUnit(_T("LoadFactors"));
         Float64 lf_version = pLoad->GetVersion();
         int nLimitStates = 4;
         if ( 2 <= lf_version  )
         {
            nLimitStates += 1; // added Strength II
         }

         if ( 3 <= lf_version )
         {
            nLimitStates += 1; // added Fatigue I
         }

         for ( int i = 0; i < nLimitStates; i++ )
         {
            pLoad->BeginUnit(strLimitState[i].c_str());

            pLoad->Property(_T("DCmin"),  &m_DCmin[i]);
            pLoad->Property(_T("DCmax"),  &m_DCmax[i]);
            pLoad->Property(_T("DWmin"),  &m_DWmin[i]);
            pLoad->Property(_T("DWmax"),  &m_DWmax[i]);
            pLoad->Property(_T("LLIMmin"),&m_LLIMmin[i]);
            pLoad->Property(_T("LLIMmax"),&m_LLIMmax[i]);

            pLoad->EndUnit();

            m_bUpdateLoadFactors = true; // need to update load factors in PGS with these values
         }
         pLoad->EndUnit();
      }

      // added in version 15
      if (version < 15)
      {
         m_EnableSlabOffsetCheck = true;
         m_EnableSlabOffsetDesign = true;
      }
      else
      {
         if(!pLoad->Property(_T("EnableSlabOffsetCheck"), &m_EnableSlabOffsetCheck))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if(!pLoad->Property(_T("EnableSlabOffsetDesign"), &m_EnableSlabOffsetDesign))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }
      }

      if (version < 15)
      {
         m_DesignStrandFillType = ftMinimizeHarping;
      }
      else
      {
         long ftype;
         if(!pLoad->Property(_T("DesignStrandFillType"), &ftype))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         m_DesignStrandFillType = ftype==(long)ftGridOrder ? ftGridOrder : ftMinimizeHarping;
      }

      if ( 16 <= version )
      {
         long value;
         if ( !pLoad->Property(_T("EffectiveFlangeWidthMethod"),&value) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         m_EffFlangeWidthMethod = (pgsTypes::EffectiveFlangeWidthMethod)(value);
      }

      // only a valid input between data block version 17 and 23
      if ( 17 <= version && version <= 23)
      {
         long value;
         if ( !pLoad->Property(_T("SlabOffsetMethod"),&value) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }
      }

      if ( 18 <= version && version < 37 )
      {
         long value;
         if ( !pLoad->Property(_T("ShearFlowMethod"),&value) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         m_ShearFlowMethod = (pgsTypes::ShearFlowMethod)(value);

         // MaxInterfaceShearConnectorSpacing wasn't available in this version of the input
         // the default value is 48". Set the value to match the spec
         if ( m_SpecificationType < lrfdVersionMgr::SeventhEdition2014 )
         {
            if ( m_SpecificationUnits == lrfdVersionMgr::US )
            {
               m_MaxInterfaceShearConnectorSpacing = WBFL::Units::ConvertToSysUnits(24.0,WBFL::Units::Measure::Inch);
            }
            else
            {
               m_MaxInterfaceShearConnectorSpacing = WBFL::Units::ConvertToSysUnits(0.6, WBFL::Units::Measure::Meter);
            }
         }

         if ( !pLoad->Property(_T("ShearCapacityMethod"),&value) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         switch(value)
         {
         case 0: // scmGeneral -> pgsTypes::scmBTEquations
            m_ShearCapacityMethod = pgsTypes::scmBTEquations;
            break;

         case 1: // scmWSDOT -> pgsTypes::scmWSDOT2001
            m_ShearCapacityMethod = pgsTypes::scmWSDOT2001;
            break;
            
         case 2: // scmSimplified -> pgsTypes::scmVciVcw
            m_ShearCapacityMethod = pgsTypes::scmVciVcw;
            break;

         default:
            //ATLASSERT(false); do nothing...
            // if value is 3 or 4 it is the correct stuff
            m_ShearCapacityMethod = (pgsTypes::ShearCapacityMethod)value;
         }

         // The general method from the 2007 spec becomes the tables method in the 2008 spec
         // make that adjustment here
         if ( m_SpecificationType < lrfdVersionMgr::FourthEditionWith2008Interims && m_ShearCapacityMethod == pgsTypes::scmBTEquations )
         {
            m_ShearCapacityMethod = pgsTypes::scmBTTables;
         }

         // if this is the 2008 spec, or later and if the shear method is WSDOT 2007, change it to Beta-Theta equations
         if ( lrfdVersionMgr::FourthEditionWith2008Interims <= m_SpecificationType && m_ShearCapacityMethod == pgsTypes::scmWSDOT2007 )
         {
            m_ShearCapacityMethod = pgsTypes::scmBTEquations;
         }
      }
      else if ( version < 18 )
      {
         // this is before Version 18... shear capacity method was read above in a now obsolete paremeter
         // translate that parameter here
         m_ShearCapacityMethod = (pgsTypes::ShearCapacityMethod)shear_capacity_method;
      }
      else
      {
         ATLASSERT( 37 <= version );
         if ( !pLoad->BeginUnit(_T("Shear")) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         Float64 shear_version = pLoad->GetVersion();

         if ( !pLoad->Property(_T("LongReinfShearMethod"), &temp ) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }
         m_LongReinfShearMethod = temp;

         // WSDOT method has been recinded
         m_LongReinfShearMethod = LRFD_METHOD;

         if (!pLoad->Property(_T("IncludeRebarForCapacity"),&temp))
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         m_bIncludeRebar_Shear = (temp == 0 ? false : true);

         long value;
         if ( !pLoad->Property(_T("ShearFlowMethod"),&value) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         m_ShearFlowMethod = (pgsTypes::ShearFlowMethod)(value);

         if ( 1 < shear_version )
         {
            // NOTE: PGSuper 2.9 changed the version number of this data block to 2
            // PGSuper 3.0 was using version 2 at the same time. There will be files
            // that were created with PGSuper 2.9 that will be read by PGSuper 3.0.
            // In those cases the following Property will fail. That's ok. Just move
            // on. If it succeeds, the MaxInterfaceShearConnectorSpacing information is available
            // so read it.
            // For this reason the return value from this pLoad->Property is not check. Everything
            // is OK if it passes or fails
            // 
            // ***** NOTE ***** DO NOT TEST THE RETULT VALUE
            pLoad->Property(_T("MaxInterfaceShearConnectorSpacing"),&m_MaxInterfaceShearConnectorSpacing);
         }
         else
         {
            // prior to 7th Edition 2014 max spacing was 24 inches... 
            if ( m_SpecificationType < lrfdVersionMgr::SeventhEdition2014 )
            {
               if ( m_SpecificationUnits == lrfdVersionMgr::US )
               {
                  m_MaxInterfaceShearConnectorSpacing = WBFL::Units::ConvertToSysUnits(24.0,WBFL::Units::Measure::Inch);
               }
               else
               {
                  m_MaxInterfaceShearConnectorSpacing = WBFL::Units::ConvertToSysUnits(0.6, WBFL::Units::Measure::Meter);
               }
            }
         }

         if (2 < shear_version)
         {
            if (!pLoad->Property(_T("UseDeckWeightForPc"), &m_bUseDeckWeightForPc)) // added in version 3 of Shear data block
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }
         }

         if ( !pLoad->Property(_T("ShearCapacityMethod"),&value) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         switch(value)
         {
         case 0: // scmGeneral -> pgsTypes::scmBTEquations
            m_ShearCapacityMethod = pgsTypes::scmBTEquations;
            break;

         case 1: // scmWSDOT -> pgsTypes::scmWSDOT2001
            m_ShearCapacityMethod = pgsTypes::scmWSDOT2001;
            break;
            
         case 2: // scmSimplified -> pgsTypes::scmVciVcw
            m_ShearCapacityMethod = pgsTypes::scmVciVcw;
            break;

         default:
            //ATLASSERT(false); do nothing...
            // if value is 3 or 4 it is the correct stuff
            m_ShearCapacityMethod = (pgsTypes::ShearCapacityMethod)value;
         }

         // The general method from the 2007 spec becomes the tables method in the 2008 spec
         // make that adjustment here
         if ( m_SpecificationType < lrfdVersionMgr::FourthEditionWith2008Interims && m_ShearCapacityMethod == pgsTypes::scmBTEquations )
         {
            m_ShearCapacityMethod = pgsTypes::scmBTTables;
         }

         // if this is the 2008 spec, or later and if the shear method is WSDOT 2007, change it to Beta-Theta equations
         if ( lrfdVersionMgr::FourthEditionWith2008Interims <= m_SpecificationType && m_ShearCapacityMethod == pgsTypes::scmWSDOT2007 )
         {
            m_ShearCapacityMethod = pgsTypes::scmBTEquations;
         }

         if (3 < shear_version)
         {
            if (!pLoad->Property(_T("LimitNetTensionStrainToPositiveValues"), &m_bLimitNetTensionStrainToPositiveValues)) // added in version 4 of this data block
            {
               THROW_LOAD(InvalidFileFormat, pLoad);
            }
         }

         // Fixed misspelling in version 2 of the Shear data block
         if ( shear_version < 2 )
         {
            // before version 2 it was named ReductionFactor
            if ( !pLoad->BeginUnit(_T("ReductionFactor")) )
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }
         }
         else if ( shear_version == 2)
         {
            // for version 2, because of the overlap, it can be ReductionFactor or ResistanceFactor
            if ( !pLoad->BeginUnit(_T("ReductionFactor")) && !pLoad->BeginUnit(_T("ResistanceFactor")) )
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }
         }
         else
         {
            ATLASSERT(2 < shear_version);
            // greater than version 2, ResistanceFactor
            if ( !pLoad->BeginUnit(_T("ResistanceFactor")) )
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }
         }

         if ( !pLoad->Property(_T("Normal"),&m_PhiShear[pgsTypes::Normal]) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->Property(_T("AllLightweight"),&m_PhiShear[pgsTypes::AllLightweight]) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->Property(_T("SandLightweight"),&m_PhiShear[pgsTypes::SandLightweight]) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->EndUnit() ) // ResistanceFactor
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if (64 <= version)
         {
            if ( !pLoad->BeginUnit(_T("ResistanceFactorDebonded")) )
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }

            Float64 data_block_version = pLoad->GetVersion();

            if (!pLoad->Property(_T("Normal"), &m_PhiShearDebonded[pgsTypes::Normal]))
            {
               THROW_LOAD(InvalidFileFormat, pLoad);
            }

            if (!pLoad->Property(_T("AllLightweight"), &m_PhiShearDebonded[pgsTypes::AllLightweight]))
            {
               THROW_LOAD(InvalidFileFormat, pLoad);
            }

            if (!pLoad->Property(_T("SandLightweight"), &m_PhiShearDebonded[pgsTypes::SandLightweight]))
            {
               THROW_LOAD(InvalidFileFormat, pLoad);
            }

            if (1 < data_block_version)
            {
               // added version 2 of this data block
               if (!pLoad->Property(_T("PCI_UHPC"), &m_PhiShearDebonded[pgsTypes::PCI_UHPC]))
               {
                  THROW_LOAD(InvalidFileFormat, pLoad);
               }

               if (!pLoad->Property(_T("UHPC"), &m_PhiShearDebonded[pgsTypes::UHPC]) 
                  && 
                  !pLoad->Property(_T("FHWA_UHPC"), &m_PhiShearDebonded[pgsTypes::UHPC]))
               {
                  // Early versions of this property used FHWA_UHPC. If UHPC fails, try FHWA_UHPC
                  THROW_LOAD(InvalidFileFormat, pLoad);
               }
            }

            if ( !pLoad->EndUnit() ) // ResistanceFactorDebonded
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }
         }

         // Added ClosureJointResistanceFactor in version 2 of the Shear data block
         if ( 1 < shear_version )
         {
            // NOTE: PGSuper 2.9 changed the version number of this data block to 2
            // PGSuper 3.0 was using version 2 at the same time. There will be files
            // that were created with PGSuper 2.9 that will be read by PGSuper 3.0.
            // In those cases the following BeginUnit will fail. That's ok. Just move
            // on. If it succeeds, the ClosureJointResistanceFactor information is available
            // so read it
            if ( pLoad->BeginUnit(_T("ClosureJointResistanceFactor")) )
            {
               if ( !pLoad->Property(_T("Normal"),&m_PhiClosureJointShear[pgsTypes::Normal]) )
               {
                  THROW_LOAD(InvalidFileFormat,pLoad );
               }

               if ( !pLoad->Property(_T("AllLightweight"),&m_PhiClosureJointShear[pgsTypes::AllLightweight]) )
               {
                  THROW_LOAD(InvalidFileFormat,pLoad );
               }

               if ( !pLoad->Property(_T("SandLightweight"),&m_PhiClosureJointShear[pgsTypes::SandLightweight]) )
               {
                  THROW_LOAD(InvalidFileFormat,pLoad );
               }

               if ( !pLoad->EndUnit() ) // ClosureJointResistanceFactor
               {
                  THROW_LOAD(InvalidFileFormat,pLoad );
               }
            }
         }

         if ( !pLoad->EndUnit() ) // Shear
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }
      }

      if ( version < 26 )
      {
         if ( m_SpecificationUnits == lrfdVersionMgr::SI )
         {
            m_PedestrianLoad   = WBFL::Units::ConvertToSysUnits(3.6e-3,WBFL::Units::Measure::MPa);
            m_MinSidewalkWidth = WBFL::Units::ConvertToSysUnits(600.,WBFL::Units::Measure::Millimeter);
         }
         else
         {
            m_PedestrianLoad   = WBFL::Units::ConvertToSysUnits(0.075,WBFL::Units::Measure::KSF);
            m_MinSidewalkWidth = WBFL::Units::ConvertToSysUnits(2.0,WBFL::Units::Measure::Feet);
         }
      }
      else
      {
         // added in version 26
         if ( !pLoad->Property(_T("PedestrianLoad"),&m_PedestrianLoad) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->Property(_T("MinSidewalkWidth"),&m_MinSidewalkWidth) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }
      }


      if (32 <= version)
      {
         if ( !pLoad->Property(_T("PrestressTransferComputationType"),(long*)&m_PrestressTransferComputationType) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }
      }


      if ( 38 <= version )
      {
         bool bBeginUnit = pLoad->BeginUnit(_T("StrandExtensions"));

         if ( bBeginUnit )
         {
            if ( !pLoad->Property(_T("AllowStraightStrandExtensions"),&m_bAllowStraightStrandExtensions) )
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }

            pLoad->EndUnit();
         }
      }

      if ( 52 <= version )
      {
         // added in version 52
         bool bBeginUnit = pLoad->BeginUnit(_T("DuctSize"));
         if ( bBeginUnit )
         {
            if ( !pLoad->Property(_T("DuctAreaPushRatio"),&m_DuctAreaPushRatio) )
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }

            if ( !pLoad->Property(_T("DuctAreaPullRatio"),&m_DuctAreaPullRatio) )
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }

            if ( !pLoad->Property(_T("DuctDiameterRatio"),&m_DuctDiameterRatio) )
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }

            pLoad->EndUnit(); // DuctSize
         }
      }


      if ( 50 <= version )
      {
         // added version 50
         pLoad->BeginUnit(_T("ClosureJoint"));
         if ( !pLoad->Property(_T("ClosureCompStressAtStressing"),             &m_ClosureCompStressAtStressing) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->Property(_T("ClosureTensStressPTZAtStressing"),          &m_ClosureTensStressPTZAtStressing) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->Property(_T("ClosureTensStressPTZWithRebarAtStressing"), &m_ClosureTensStressPTZWithRebarAtStressing) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->Property(_T("ClosureTensStressAtStressing"),             &m_ClosureTensStressAtStressing) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->Property(_T("ClosureTensStressWithRebarAtStressing"),    &m_ClosureTensStressWithRebarAtStressing) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->Property(_T("ClosureCompStressAtService"),               &m_ClosureCompStressAtService) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->Property(_T("ClosureCompStressWithLiveLoadAtService"),   &m_ClosureCompStressWithLiveLoadAtService) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->Property(_T("ClosureTensStressPTZAtService"),            &m_ClosureTensStressPTZAtService) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->Property(_T("ClosureTensStressPTZWithRebarAtService"),   &m_ClosureTensStressPTZWithRebarAtService) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->Property(_T("ClosureTensStressAtService"),               &m_ClosureTensStressAtService) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->Property(_T("ClosureTensStressWithRebarAtService"),      &m_ClosureTensStressWithRebarAtService) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( !pLoad->Property(_T("ClosureCompStressFatigue"),                 &m_ClosureCompStressFatigue) )
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         pLoad->EndUnit(); // Closure Joint
      }


      // added in version 48
      if ( 47 < version )
      {
         bool bOKToFail = false;
         if ( version <= MAX_OVERLAP_VERSION )
         {
            // This was added in version 48 for PGSuper version 2.9.
            // At the same time, PGSuper 3.0 was being built. The data block version was
            // MAX_OVERLAP_VERSION. It is ok to fail for 44 <= version <= MAX_OVERLAP_VERSION. If version is more than MAX_OVERLAP_VERSION
            // then the data file format is invalid.
            bOKToFail = true;
         }

         bool bSucceeded = pLoad->Property(_T("CheckBottomFlangeClearance"),&m_bCheckBottomFlangeClearance);
         if ( bSucceeded )
         {
            if (!pLoad->Property(_T("MinBottomFlangeClearance"), &m_Cmin))
            {
               THROW_LOAD(InvalidFileFormat, pLoad);
            }
         }
         else
         {
            if ( !bOKToFail )
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }
         }
      }


      // added in version 57
      if (56 < version)
      {
         if (!pLoad->Property(_T("CheckGirderInclination"), &m_bCheckGirderInclination))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }

         if (version < 71)
         {
            // removed in version 71
            Float64 value; // waste the value
            if (!pLoad->Property(_T("InclindedGirder_BrgPadDeduction"), &value))
            {
               THROW_LOAD(InvalidFileFormat, pLoad);
            }
         }

         if (!pLoad->Property(_T("InclindedGirder_FSmax"), &m_InclinedGirder_FSmax))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }
      }


      // added in version 62
      if (61 < version)
      {
         if (!pLoad->Property(_T("FinishedElevationTolerance"), &m_FinishedElevationTolerance))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }
      }

      // slab offset rounding option added in version 73
      if (72 < version)
      {
         if (!pLoad->Property(_T("SlabOffsetRoundingMethod"), (long*)&m_SlabOffsetRoundingMethod))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }

         if (!pLoad->Property(_T("SlabOffsetRoundingTolerance"), &m_SlabOffsetRoundingTolerance))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }
      }
      else
      {
         // This was the hard-coded default for versions before 5.x
         m_SlabOffsetRoundingMethod = pgsTypes::sormRoundNearest;
         m_SlabOffsetRoundingTolerance = m_SpecificationUnits == lrfdVersionMgr::US ? WBFL::Units::ConvertToSysUnits(0.25, WBFL::Units::Measure::Inch) : WBFL::Units::ConvertToSysUnits(5.0,WBFL::Units::Measure::Millimeter);
      }

      // Bearings was added in verison 79
      if (78 < version)
      {
         if (!pLoad->BeginUnit(_T("Bearings")))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }

         if (!pLoad->Property(_T("AlertTaperedSolePlateRequirement"), &m_bAlertTaperedSolePlateRequirement))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }
         
         if (!pLoad->Property(_T("TaperedSolePlaneInclinationThreshold"), &m_TaperedSolePlateInclinationThreshold))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }

         if (!pLoad->Property(_T("UseImpactForBearingReactions"), &m_bUseImpactForBearingReactions))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }
         
         if (!pLoad->EndUnit()) // Bearings
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }
      }

      if(!pLoad->EndUnit())
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if (version < 72)
      {
         if (m_LossMethod != LOSSES_TIME_STEP && m_LimitStateConcreteStrength == pgsTypes::lscStrengthAtTimeOfLoading)
         {
            // m_LimitStateConcreteStrength was only used for time step losses before version 72
            // now it is always used
            // strength at time of loading only a valid option for time step analysis... change it to a valid value
            m_LimitStateConcreteStrength = pgsTypes::lscSpecifiedStrength;
         }
      }
   }

   // sometimes the user may toggle between several loss methods. if they select time step and change the time dependent model
   // and then change to a different method, the time dependent model is saved in its last state. This makes the default time dependent
   // model be the last used value. Change it to the true default value here
   if (m_LossMethod != LOSSES_TIME_STEP)
   {
      m_TimeDependentModel = TDM_AASHTO;
   }

   return true;
}

bool SpecLibraryEntry::IsEqual(const SpecLibraryEntry& rOther,bool bConsiderName) const
{
   std::vector<pgsLibraryEntryDifferenceItem*> vDifferences;
   bool bMustRename;
   return Compare(rOther,vDifferences,bMustRename,true,bConsiderName);
}

bool SpecLibraryEntry::Compare(const SpecLibraryEntry& rOther, std::vector<pgsLibraryEntryDifferenceItem*>& vDifferences, bool& bMustRename, bool bReturnOnFirstDifference, bool considerName) const
{
   CEAFApp* pApp = EAFGetApp();
   const WBFL::Units::IndirectMeasure* pDisplayUnits = pApp->GetDisplayUnits();

   bMustRename = false;

   //
   // General Tab
   //
   if ( m_Description != rOther.m_Description )
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Description"),m_Description.c_str(),rOther.m_Description.c_str()));
   }

   if ( m_bUseCurrentSpecification != rOther.m_bUseCurrentSpecification || m_SpecificationType != rOther.m_SpecificationType )
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Basis is different"),lrfdVersionMgr::GetVersionString(m_SpecificationType),lrfdVersionMgr::GetVersionString(rOther.m_SpecificationType)));
   }

   if ( lrfdVersionMgr::ThirdEditionWith2006Interims < m_SpecificationType && m_SpecificationUnits != rOther.m_SpecificationUnits )
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Specification Units Systems are different"),_T(""),_T("")));
   }

   if ( m_SectionPropertyMode != rOther.m_SectionPropertyMode || m_EffFlangeWidthMethod != rOther.m_EffFlangeWidthMethod )
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Section Properties are different"),_T(""),_T("")));
   }

   //
   // Spec. Checking and Design Tab
   //
   if ( m_DoCheckHoldDown != rOther.m_DoCheckHoldDown || 
        m_DoDesignHoldDown != rOther.m_DoDesignHoldDown ||
        m_HoldDownForceType != rOther.m_HoldDownForceType ||
        !::IsEqual(m_HoldDownForce,rOther.m_HoldDownForce) || !::IsEqual(m_HoldDownFriction,rOther.m_HoldDownFriction))
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Hold Down Force requirements are different"),_T(""),_T("")));
   }

   if ( m_DoCheckStrandSlope != rOther.m_DoCheckStrandSlope ||
        m_DoDesignStrandSlope != rOther.m_DoDesignStrandSlope ||
        !::IsEqual(m_MaxSlope05, rOther.m_MaxSlope05) ||
        !::IsEqual(m_MaxSlope06, rOther.m_MaxSlope06) ||
        !::IsEqual(m_MaxSlope07, rOther.m_MaxSlope07) )
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Strand Slope requirements are different"),_T(""),_T("")));
   }

   if (m_bCheckHandlingWeightLimit != rOther.m_bCheckHandlingWeightLimit || !::IsEqual(m_HandlingWeightLimit, rOther.m_HandlingWeightLimit))
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Handling Weight Limit requirements are different"), _T(""), _T("")));
   }

   if ( m_DoCheckSplitting != rOther.m_DoCheckSplitting ||
        m_DoDesignSplitting != rOther.m_DoDesignSplitting ||
        !::IsEqual(m_SplittingZoneLengthFactor,rOther.m_SplittingZoneLengthFactor)
      )
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Splitting Resistance requirements are different"),_T(""),_T("")));
   }

   if ( m_DoCheckConfinement != rOther.m_DoCheckConfinement ||
        m_DoDesignConfinement != rOther.m_DoDesignConfinement )
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Confinement Reinforcement requirements are different"),_T(""),_T("")));
   }

   if ( m_EnableLiftingCheck != rOther.m_EnableLiftingCheck ||
      m_EnableLiftingDesign != rOther.m_EnableLiftingDesign ||
      !::IsEqual(m_MinLiftPoint,rOther.m_MinLiftPoint) ||
      !::IsEqual(m_LiftPointAccuracy,rOther.m_LiftPointAccuracy) )
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Lifting Check/Design Options are different"),_T(""),_T("")));
   }

   if ( m_EnableHaulingCheck != rOther.m_EnableHaulingCheck ||
        m_EnableHaulingDesign != rOther.m_EnableHaulingDesign ||
        !::IsEqual(m_MinHaulPoint, rOther.m_MinHaulPoint) ||
        !::IsEqual(m_HaulPointAccuracy, rOther.m_HaulPointAccuracy) ||
        (m_UseMinTruckSupportLocationFactor != rOther.m_UseMinTruckSupportLocationFactor || (m_UseMinTruckSupportLocationFactor == true && !::IsEqual(m_MinTruckSupportLocationFactor,rOther.m_MinTruckSupportLocationFactor))) )
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Hauling Check/Design Options are different"),_T(""),_T("")));
   }

   if ( m_EnableSlabOffsetCheck != rOther.m_EnableSlabOffsetCheck ||
        m_EnableSlabOffsetDesign != rOther.m_EnableSlabOffsetDesign )
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Slab Offset (\"A\" Dimension) Check/Design Options are different"),_T(""),_T("")));
   }

   if ( m_bDoEvaluateDeflection != rOther.m_bDoEvaluateDeflection ||
       (m_bDoEvaluateDeflection == true && !::IsEqual(m_DeflectionLimit, rOther.m_DeflectionLimit)) )
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Live Load Deflection Check Options are different"),_T(""),_T("")));
   }

   if ( m_bCheckBottomFlangeClearance != rOther.m_bCheckBottomFlangeClearance ||
       (m_bCheckBottomFlangeClearance == true && !::IsEqual(m_Cmin,rOther.m_Cmin)) )
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Bottom Flange Clearance Check Options are different"),_T(""),_T("")));
   }

   if (m_bCheckGirderInclination != rOther.m_bCheckGirderInclination || 
      (m_bCheckGirderInclination == true && !::IsEqual(m_InclinedGirder_FSmax, rOther.m_InclinedGirder_FSmax))
      )
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Inclinded Girder Check Options are different"), _T(""), _T("")));
   }


   if ( m_DesignStrandFillType != rOther.m_DesignStrandFillType )
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Harped Strand Design Strategies are different"),_T(""),_T("")));
   }

   if ( m_LimitStateConcreteStrength != rOther.m_LimitStateConcreteStrength ||
        m_bUse90DayConcreteStrength != rOther.m_bUse90DayConcreteStrength || 
      !::IsEqual(m_90DayConcreteStrengthFactor,rOther.m_90DayConcreteStrengthFactor))
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Concrete Strength for Limit State Evaluations are different"),_T(""),_T("")));
   }


   //
   // Prestressed Elements Tab
   //
   if ( !::IsEqual(m_CyCompStressServ, rOther.m_CyCompStressServ) ||
        !::IsEqual(m_CyTensStressServ, rOther.m_CyTensStressServ) ||
        !::IsEqual(m_CyTensStressServWithRebar, rOther.m_CyTensStressServWithRebar) ||
        (m_CyDoTensStressServMax != rOther.m_CyDoTensStressServMax || (m_CyDoTensStressServMax == true && !::IsEqual(m_CyTensStressServMax, rOther.m_CyTensStressServMax))) )
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Stress Limits for Temporary Stresses before Losses are different"),_T(""),_T("")));
   }

   bool bServiceITension = (m_bCheckBs2Tension == rOther.m_bCheckBs2Tension);
   if ( bServiceITension && !::IsEqual(m_Bs2TensStress, rOther.m_Bs2TensStress) )
   {
      bServiceITension = false;
   }
   if ( bServiceITension && m_Bs2DoTensStressMax != rOther.m_Bs2DoTensStressMax )
   {
      bServiceITension = false;
   }
   if ( bServiceITension && m_Bs2DoTensStressMax && !::IsEqual(m_Bs2TensStressMax,rOther.m_Bs2TensStressMax) )
   {
      bServiceITension = false;
   }

   if ( !::IsEqual(m_Bs2CompStress, rOther.m_Bs2CompStress) ||
        !::IsEqual(m_Bs3CompStressServ, rOther.m_Bs3CompStressServ) ||
        !bServiceITension ||
        !::IsEqual(m_Bs3TensStressServNc, rOther.m_Bs3TensStressServNc) ||
        (m_Bs3DoTensStressServNcMax != rOther.m_Bs3DoTensStressServNcMax  || (m_Bs3DoTensStressServNcMax == true && !::IsEqual(m_Bs3TensStressServNcMax, rOther.m_Bs3TensStressServNcMax))) ||
        !::IsEqual(m_Bs3TensStressServSc, rOther.m_Bs3TensStressServSc) ||
        (m_Bs3DoTensStressServScMax != rOther.m_Bs3DoTensStressServScMax || (m_Bs3DoTensStressServScMax == true && !::IsEqual(m_Bs3TensStressServScMax, rOther.m_Bs3TensStressServScMax))) )
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Stress Limits at Service Limit State after Losses are different"),_T(""),_T("")));
   }

   if ( !::IsEqual(m_Bs3CompStressService1A, rOther.m_Bs3CompStressService1A) )
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Allowable Concrete Stress at Fatigue Limit State are different"),_T(""),_T("")));
   }

   bool bTempStrandRemovalStresses = true;
   bool bDeckPlacementStresses = true;
   if ( m_bCheckTemporaryStresses != rOther.m_bCheckTemporaryStresses )
   {
      if ( !::IsEqual(m_TempStrandRemovalCompStress, rOther.m_TempStrandRemovalCompStress) ||
         !::IsEqual(m_TempStrandRemovalTensStress, rOther.m_TempStrandRemovalTensStress) ||
         (m_TempStrandRemovalDoTensStressMax != rOther.m_TempStrandRemovalDoTensStressMax || (m_TempStrandRemovalDoTensStressMax == true && !::IsEqual(m_TempStrandRemovalTensStressMax, rOther.m_TempStrandRemovalTensStressMax))) ||
         !::IsEqual(m_TempStrandRemovalTensStressWithRebar, rOther.m_TempStrandRemovalTensStressWithRebar) )
      {
         bTempStrandRemovalStresses = false;
      }

      if ( !::IsEqual(m_Bs1CompStress, rOther.m_Bs1CompStress) ||
           !::IsEqual(m_Bs1TensStress, rOther.m_Bs1TensStress) ||
           (m_Bs1DoTensStressMax != rOther.m_Bs1DoTensStressMax || (m_Bs2DoTensStressMax == true && !::IsEqual(m_Bs1TensStressMax, rOther.m_Bs1TensStressMax))) )
      {
         bDeckPlacementStresses = false;
      }
   }

   if ( m_bCheckTemporaryStresses != rOther.m_bCheckTemporaryStresses ||
      (m_bCheckTemporaryStresses == true && (!bTempStrandRemovalStresses || !bDeckPlacementStresses)) )
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Stress Limits for Temporary Loading Conditions are different"),_T(""),_T("")));
   }

   if (m_PrincipalTensileStressMethod != rOther.m_PrincipalTensileStressMethod || 
      !::IsEqual(m_PrincipalTensileStressCoefficient, rOther.m_PrincipalTensileStressCoefficient) ||
      !::IsEqual(m_PrincipalTensileStressTendonNearnessFactor,rOther.m_PrincipalTensileStressTendonNearnessFactor) ||
      !::IsEqual(m_PrincipalTensileStressFcThreshold,rOther.m_PrincipalTensileStressFcThreshold) ||
      !::IsEqual(m_PrincipalTensileStressUngroutedMultiplier,rOther.m_PrincipalTensileStressUngroutedMultiplier) ||
      !::IsEqual(m_PrincipalTensileStressGroutedMultiplier,rOther.m_PrincipalTensileStressGroutedMultiplier))
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Principal Tensile Stress in Web parameters are different"), _T(""), _T("")));
   }

   //
   // Closure Joints Tab
   //
   if ( !::IsEqual(m_ClosureCompStressAtStressing             , rOther.m_ClosureCompStressAtStressing) ||
        !::IsEqual(m_ClosureTensStressPTZAtStressing          , rOther.m_ClosureTensStressPTZAtStressing) ||
        !::IsEqual(m_ClosureTensStressPTZWithRebarAtStressing , rOther.m_ClosureTensStressPTZWithRebarAtStressing) ||
        !::IsEqual(m_ClosureTensStressAtStressing             , rOther.m_ClosureTensStressAtStressing) ||
        !::IsEqual(m_ClosureTensStressWithRebarAtStressing    , rOther.m_ClosureTensStressWithRebarAtStressing) )
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Closure Joint Stress Limits for Temporary Stresses before Losses are different"),_T(""),_T("")));
   }

   if ( !::IsEqual(m_ClosureCompStressAtService               , rOther.m_ClosureCompStressAtService) ||
        !::IsEqual(m_ClosureCompStressWithLiveLoadAtService   , rOther.m_ClosureCompStressWithLiveLoadAtService) ||
        !::IsEqual(m_ClosureTensStressPTZAtService            , rOther.m_ClosureTensStressPTZAtService) ||
        !::IsEqual(m_ClosureTensStressPTZWithRebarAtService   , rOther.m_ClosureTensStressPTZWithRebarAtService) ||
        !::IsEqual(m_ClosureTensStressAtService               , rOther.m_ClosureTensStressAtService) ||
        !::IsEqual(m_ClosureTensStressWithRebarAtService      , rOther.m_ClosureTensStressWithRebarAtService) )
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Closure Joint Stress Limits at Service Limit State after Losses are different"),_T(""),_T("")));
   }
   
   if ( !::IsEqual(m_ClosureCompStressFatigue, rOther.m_ClosureCompStressFatigue) )
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Closure Joint Allowable Concrete Stress at Fatigue Limit State are different"),_T(""),_T("")));
   }

   //
   // Prestressing Tab
   //
   bool bPSAtJacking = true;
   if ( m_bCheckStrandStress[CSS_AT_JACKING] != rOther.m_bCheckStrandStress[CSS_AT_JACKING] ||
      (m_bCheckStrandStress[CSS_AT_JACKING] == true && (!::IsEqual(m_StrandStressCoeff[CSS_AT_JACKING][STRESS_REL], rOther.m_StrandStressCoeff[CSS_AT_JACKING][STRESS_REL]) || 
                                                        !::IsEqual(m_StrandStressCoeff[CSS_AT_JACKING][LOW_RELAX], rOther.m_StrandStressCoeff[CSS_AT_JACKING][LOW_RELAX]))) )
   {
      bPSAtJacking = false;
   }

   bool bPSBeforeXfer = true;
   if ( m_bCheckStrandStress[CSS_BEFORE_TRANSFER] != rOther.m_bCheckStrandStress[CSS_BEFORE_TRANSFER] ||
      (m_bCheckStrandStress[CSS_BEFORE_TRANSFER] == true && (!::IsEqual(m_StrandStressCoeff[CSS_BEFORE_TRANSFER][STRESS_REL], rOther.m_StrandStressCoeff[CSS_BEFORE_TRANSFER][STRESS_REL]) || 
                                                        !::IsEqual(m_StrandStressCoeff[CSS_BEFORE_TRANSFER][LOW_RELAX], rOther.m_StrandStressCoeff[CSS_BEFORE_TRANSFER][LOW_RELAX]))) )
   {
      bPSBeforeXfer = false;
   }

   bool bPSAfterXfer = true;
   if ( m_bCheckStrandStress[CSS_AFTER_TRANSFER] != rOther.m_bCheckStrandStress[CSS_AFTER_TRANSFER] ||
      (m_bCheckStrandStress[CSS_AFTER_TRANSFER] == true && (!::IsEqual(m_StrandStressCoeff[CSS_AFTER_TRANSFER][STRESS_REL], rOther.m_StrandStressCoeff[CSS_AFTER_TRANSFER][STRESS_REL]) || 
                                                        !::IsEqual(m_StrandStressCoeff[CSS_AFTER_TRANSFER][LOW_RELAX], rOther.m_StrandStressCoeff[CSS_AFTER_TRANSFER][LOW_RELAX]))) )
   {
      bPSAfterXfer = false;
   }

   bool bPSFinal = true;
   if ( m_bCheckStrandStress[CSS_AFTER_ALL_LOSSES] != rOther.m_bCheckStrandStress[CSS_AFTER_ALL_LOSSES] ||
      (m_bCheckStrandStress[CSS_AFTER_ALL_LOSSES] == true && (!::IsEqual(m_StrandStressCoeff[CSS_AFTER_ALL_LOSSES][STRESS_REL], rOther.m_StrandStressCoeff[CSS_AFTER_ALL_LOSSES][STRESS_REL]) || 
                                                        !::IsEqual(m_StrandStressCoeff[CSS_AFTER_ALL_LOSSES][LOW_RELAX], rOther.m_StrandStressCoeff[CSS_AFTER_ALL_LOSSES][LOW_RELAX]))) )
   {
      bPSFinal = false;
   }

   if ( !bPSAtJacking || !bPSBeforeXfer || !bPSAfterXfer || !bPSFinal )
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Stress Limits for Prestressing are different"),_T(""),_T("")));
   }

   bool bPTAtJacking = true;
   if ( m_bCheckTendonStressAtJacking != rOther.m_bCheckTendonStressAtJacking ||
      (m_bCheckTendonStressAtJacking == true && (!::IsEqual(m_TendonStressCoeff[CSS_AT_JACKING][STRESS_REL],rOther.m_TendonStressCoeff[CSS_AT_JACKING][STRESS_REL]) || 
                                                 !::IsEqual(m_TendonStressCoeff[CSS_AT_JACKING][LOW_RELAX],rOther.m_TendonStressCoeff[CSS_AT_JACKING][LOW_RELAX]))) )
   {
      bPTAtJacking = false;
   }

   bool bPTPriorToSeating = true;
   if ( m_bCheckTendonStressPriorToSeating != rOther.m_bCheckTendonStressPriorToSeating ||
      (m_bCheckTendonStressPriorToSeating == true && ( !::IsEqual(m_TendonStressCoeff[CSS_PRIOR_TO_SEATING][STRESS_REL],rOther.m_TendonStressCoeff[CSS_PRIOR_TO_SEATING][STRESS_REL]) || 
                                                       !::IsEqual(m_TendonStressCoeff[CSS_PRIOR_TO_SEATING][LOW_RELAX],rOther.m_TendonStressCoeff[CSS_PRIOR_TO_SEATING][LOW_RELAX]))) )
   {
      bPTPriorToSeating = false;
   }

   bool bPTAfterSeating = true;
   if ( !::IsEqual(m_TendonStressCoeff[CSS_ANCHORAGES_AFTER_SEATING][STRESS_REL],rOther.m_TendonStressCoeff[CSS_ANCHORAGES_AFTER_SEATING][STRESS_REL]) ||
        !::IsEqual(m_TendonStressCoeff[CSS_ANCHORAGES_AFTER_SEATING][LOW_RELAX],rOther.m_TendonStressCoeff[CSS_ANCHORAGES_AFTER_SEATING][LOW_RELAX]) )
   {
      bPTAfterSeating = false;
   }

   bool bPTElsewhereAfterSeating = true;
   if ( !::IsEqual(m_TendonStressCoeff[CSS_ELSEWHERE_AFTER_SEATING][STRESS_REL],rOther.m_TendonStressCoeff[CSS_ELSEWHERE_AFTER_SEATING][STRESS_REL]) ||
        !::IsEqual(m_TendonStressCoeff[CSS_ELSEWHERE_AFTER_SEATING][LOW_RELAX],rOther.m_TendonStressCoeff[CSS_ELSEWHERE_AFTER_SEATING][LOW_RELAX]) )
   {
      bPTElsewhereAfterSeating = false;
   }

   bool bPTFinal = true;
   if ( !::IsEqual(m_TendonStressCoeff[CSS_AFTER_ALL_LOSSES][STRESS_REL],rOther.m_TendonStressCoeff[CSS_AFTER_ALL_LOSSES][STRESS_REL]) ||
        !::IsEqual(m_TendonStressCoeff[CSS_AFTER_ALL_LOSSES][LOW_RELAX],rOther.m_TendonStressCoeff[CSS_AFTER_ALL_LOSSES][LOW_RELAX]) )
   {
      bPTFinal = false;
   }

   if ( !bPTAtJacking || !bPTPriorToSeating || !bPTAfterSeating || !bPTElsewhereAfterSeating || !bPTFinal )
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Stress Limits for Post-tensioning are different"),_T(""),_T("")));
   }


   if ( m_bAllowStraightStrandExtensions != rOther.m_bAllowStraightStrandExtensions || 
        m_PrestressTransferComputationType != rOther.m_PrestressTransferComputationType )
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Pretensioned Strand Options are different"),_T(""),_T("")));
   }

   if ( !::IsEqual(m_DuctAreaPushRatio, rOther.m_DuctAreaPushRatio) ||
        !::IsEqual(m_DuctAreaPullRatio, rOther.m_DuctAreaPullRatio) ||
        !::IsEqual(m_DuctDiameterRatio, rOther.m_DuctDiameterRatio) )
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Size of Ducts parameters are different"),_T(""),_T("")));
   }

   //
   // Lifting Tab
   //
   if ( !::IsEqual(m_CyLiftingCrackFs, rOther.m_CyLiftingCrackFs) || 
        !::IsEqual(m_CyLiftingFailFs,  rOther.m_CyLiftingFailFs) )
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Lifting Factors of Safety are different"),_T(""),_T("")));
   }

   if ( !::IsEqual(m_LiftingModulusOfRuptureCoefficient[pgsTypes::Normal], rOther.m_LiftingModulusOfRuptureCoefficient[pgsTypes::Normal]) ||
      !::IsEqual(m_LiftingModulusOfRuptureCoefficient[pgsTypes::SandLightweight], rOther.m_LiftingModulusOfRuptureCoefficient[pgsTypes::SandLightweight]) ||
      (lrfdVersionMgr::SeventhEditionWith2016Interims <= GetSpecificationType() ? !::IsEqual(m_LiftingModulusOfRuptureCoefficient[pgsTypes::AllLightweight], rOther.m_LiftingModulusOfRuptureCoefficient[pgsTypes::AllLightweight]) : false) )
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Modulus of Rupture for Cracking Moment During Lifting are different"),_T(""),_T("")));
   }

   if ( !::IsEqual(m_LiftingUpwardImpact  , rOther.m_LiftingUpwardImpact) ||
        !::IsEqual(m_LiftingDownwardImpact, rOther.m_LiftingDownwardImpact) ||
        !::IsEqual(m_PickPointHeight      , rOther.m_PickPointHeight) ||
        !::IsEqual(m_LiftingLoopTolerance , rOther.m_LiftingLoopTolerance) ||
        !::IsEqual(m_MaxGirderSweepLifting, rOther.m_MaxGirderSweepLifting) ||
        !::IsEqual(m_MinCableInclination  , rOther.m_MinCableInclination) ||
        !::IsEqual(m_LiftingCamberMultiplier, rOther.m_LiftingCamberMultiplier) ||
        m_LiftingWindType != rOther.m_LiftingWindType ||
        !::IsEqual(m_LiftingWindLoad,rOther.m_LiftingWindLoad)
     )
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Lifting Analysis Parameters are different"),_T(""),_T("")));
   }

   if ( !::IsEqual(m_LiftingCompressionStressCoefficient_GlobalStress, rOther.m_LiftingCompressionStressCoefficient_GlobalStress) ||
        !::IsEqual(m_LiftingCompressionStressCoefficient_PeakStress, rOther.m_LiftingCompressionStressCoefficient_PeakStress) ||
        !::IsEqual(m_CyTensStressLifting        , rOther.m_CyTensStressLifting) ||
        (m_CyDoTensStressLiftingMax != rOther.m_CyDoTensStressLiftingMax || (m_CyDoTensStressLiftingMax == true && !::IsEqual(m_CyTensStressLiftingMax, rOther.m_CyTensStressLiftingMax))) ||
        !::IsEqual(m_TensStressLiftingWithRebar , rOther.m_TensStressLiftingWithRebar) ||
        !::IsEqual(m_TensStressHaulingWithRebar[pgsTypes::CrownSlope] , rOther.m_TensStressHaulingWithRebar[pgsTypes::CrownSlope]) ||
        !::IsEqual(m_TensStressHaulingWithRebar[pgsTypes::Superelevation] , rOther.m_TensStressHaulingWithRebar[pgsTypes::Superelevation])
      )
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Lifting Allowable Concrete Stresses are different"),_T(""),_T("")));
   }


   //
   // Hauling Tab
   //
   if ( m_HaulingAnalysisMethod != rOther.m_HaulingAnalysisMethod )
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Hauling Analysis Methods are different"),_T(""),_T("")));
   }
   else
   {
      if ( m_HaulingAnalysisMethod == pgsTypes::hmWSDOT )
      {
         // WSDOT method
         if ( !::IsEqual(m_HaulingCrackFs, rOther.m_HaulingCrackFs) ||
              !::IsEqual(m_HaulingRollFs, rOther.m_HaulingRollFs) )
         {
            RETURN_ON_DIFFERENCE;
            vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Hauling Factors of Safety are different"),_T(""),_T("")));
         }

         if ( !::IsEqual(m_HaulingModulusOfRuptureCoefficient[pgsTypes::Normal], rOther.m_HaulingModulusOfRuptureCoefficient[pgsTypes::Normal]) ||
            !::IsEqual(m_HaulingModulusOfRuptureCoefficient[pgsTypes::SandLightweight], rOther.m_HaulingModulusOfRuptureCoefficient[pgsTypes::SandLightweight]) ||
            (lrfdVersionMgr::SeventhEditionWith2016Interims <= GetSpecificationType() ? !::IsEqual(m_HaulingModulusOfRuptureCoefficient[pgsTypes::AllLightweight], rOther.m_HaulingModulusOfRuptureCoefficient[pgsTypes::AllLightweight]) : false) )
         {
            RETURN_ON_DIFFERENCE;
            vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Modulus of Rupture for Cracking Moment During Hauling are different"),_T(""),_T("")));
         }

         if ( !::IsEqual(m_HaulingUpwardImpact, rOther.m_HaulingUpwardImpact) ||
            !::IsEqual(m_HaulingDownwardImpact, rOther.m_HaulingDownwardImpact) ||
            m_HaulingImpactUsage != rOther.m_HaulingImpactUsage ||
            !::IsEqual(m_RoadwayCrownSlope,rOther.m_RoadwayCrownSlope) ||
            !::IsEqual(m_RoadwaySuperelevation, rOther.m_RoadwaySuperelevation) ||
            !::IsEqual(m_MaxGirderSweepHauling, rOther.m_MaxGirderSweepHauling) ||
            !::IsEqual(m_HaulingSweepGrowth,rOther.m_HaulingSweepGrowth) ||
            !::IsEqual(m_HaulingSupportPlacementTolerance, rOther.m_HaulingSupportPlacementTolerance) ||
            !::IsEqual(m_HaulingCamberMultiplier,rOther.m_HaulingCamberMultiplier) ||
            m_HaulingWindType != rOther.m_HaulingWindType ||
            !::IsEqual(m_HaulingWindLoad,rOther.m_HaulingWindLoad) ||
            m_CentrifugalForceType != rOther.m_CentrifugalForceType ||
            !::IsEqual(m_HaulingSpeed,rOther.m_HaulingSpeed) ||
            !::IsEqual(m_TurningRadius,rOther.m_TurningRadius)
            )
         {
            RETURN_ON_DIFFERENCE;
            vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Hauling Analysis Parameters are different"),_T(""),_T("")));
         }
      }
      else
      {
         // KDOT method
         if ( !::IsEqual(m_OverhangGFactor, rOther.m_OverhangGFactor) ||
              !::IsEqual(m_InteriorGFactor, rOther.m_InteriorGFactor) )
         {
            RETURN_ON_DIFFERENCE;
            vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Hauling Dynamic Load Factors are different"),_T(""),_T("")));
         }
      }

      // common to both methods
      if ( !::IsEqual(m_GlobalCompStressHauling, rOther.m_GlobalCompStressHauling) ||
           !::IsEqual(m_PeakCompStressHauling, rOther.m_PeakCompStressHauling) ||
           !::IsEqual(m_TensStressHauling[pgsTypes::CrownSlope], rOther.m_TensStressHauling[pgsTypes::CrownSlope]) ||
           !::IsEqual(m_TensStressHauling[pgsTypes::Superelevation], rOther.m_TensStressHauling[pgsTypes::Superelevation]) ||
           (m_DoTensStressHaulingMax[pgsTypes::CrownSlope] != rOther.m_DoTensStressHaulingMax[pgsTypes::CrownSlope] || (m_DoTensStressHaulingMax[pgsTypes::CrownSlope] == true && !::IsEqual(m_TensStressHaulingMax[pgsTypes::CrownSlope], rOther.m_TensStressHaulingMax[pgsTypes::CrownSlope]))) ||
           (m_DoTensStressHaulingMax[pgsTypes::Superelevation] != rOther.m_DoTensStressHaulingMax[pgsTypes::Superelevation] || (m_DoTensStressHaulingMax[pgsTypes::Superelevation] == true && !::IsEqual(m_TensStressHaulingMax[pgsTypes::Superelevation], rOther.m_TensStressHaulingMax[pgsTypes::Superelevation])))
        )
      {
         RETURN_ON_DIFFERENCE;
         vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Hauling Allowable Concrete Stresses are different"),_T(""),_T("")));
      }
   }

   //
   // Loads Tab
   //
   if ( m_Bs2MaxGirdersTrafficBarrier != rOther.m_Bs2MaxGirdersTrafficBarrier ||
        m_TrafficBarrierDistributionType != rOther.m_TrafficBarrierDistributionType )
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Distribution of Railing System Loads are different"),_T(""),_T("")));
   }

   if ( !::IsEqual(m_PedestrianLoad, rOther.m_PedestrianLoad) ||
        !::IsEqual(m_MinSidewalkWidth, rOther.m_MinSidewalkWidth) )
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Pedestrian Live Loads are different"),_T(""),_T("")));
   }

   if ( m_OverlayLoadDistribution != rOther.m_OverlayLoadDistribution )
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Distribution of Overlay Dead Load is different"),_T(""),_T("")));
   }

   if ( m_LldfMethod != rOther.m_LldfMethod ||
        m_bIgnoreSkewReductionForMoment != rOther.m_bIgnoreSkewReductionForMoment ||
        m_LimitDistributionFactorsToLanesBeams != rOther.m_LimitDistributionFactorsToLanesBeams ||
        !::IsEqual(m_MaxAngularDeviationBetweenGirders, rOther.m_MaxAngularDeviationBetweenGirders) ||
        !::IsEqual(m_MinGirderStiffnessRatio,           rOther.m_MinGirderStiffnessRatio) ||
        !::IsEqual(m_LLDFGirderSpacingLocation,         rOther.m_LLDFGirderSpacingLocation) ||
        m_bUseRigidMethod != rOther.m_bUseRigidMethod ||
        m_ExteriorLiveLoadDistributionGTAdjacentInteriorRule != rOther.m_ExteriorLiveLoadDistributionGTAdjacentInteriorRule)
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Live Load Distribution Factors are different"),_T(""),_T("")));
   }

   if (m_bIncludeDualTandem != rOther.m_bIncludeDualTandem)
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("HL93 Dual Tandem setting is different"), _T(""), _T("")));
   }

   if ( m_HaunchLoadComputationType != rOther.m_HaunchLoadComputationType ||
      (m_HaunchLoadComputationType == pgsTypes::hlcDetailedAnalysis && !::IsEqual(m_HaunchLoadCamberTolerance, rOther.m_HaunchLoadCamberTolerance)) )
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Haunch Loads are different"),_T(""),_T("")));
   }
   
   if ( m_HaunchLoadComputationType == pgsTypes::hlcDetailedAnalysis && !::IsEqual(m_HaunchLoadCamberFactor, rOther.m_HaunchLoadCamberFactor) )
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Haunch Loads Camber Factors are different"),_T(""),_T("")));
   }

   if ( m_HaunchAnalysisSectionPropertiesType != rOther.m_HaunchAnalysisSectionPropertiesType ||
      (m_HaunchAnalysisSectionPropertiesType == pgsTypes::hspDetailedDescription && !::IsEqual(m_HaunchLoadCamberTolerance, rOther.m_HaunchLoadCamberTolerance)) )
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Method using haunch geometry to compute composite section properties is different"),_T(""),_T("")));
   }

   //
   // Moment Capacity Tab
   //
   if ( (GetSpecificationType() <= lrfdVersionMgr::ThirdEditionWith2005Interims && m_Bs3LRFDOverReinforcedMomentCapacity != rOther.m_Bs3LRFDOverReinforcedMomentCapacity) ||
        m_bIncludeStrand_NegMoment != rOther.m_bIncludeStrand_NegMoment ||
      m_bConsiderReinforcementStrainLimit != rOther.m_bConsiderReinforcementStrainLimit ||
      m_nMomentCapacitySlices != rOther.m_nMomentCapacitySlices ||
        m_bIncludeRebar_Moment != rOther.m_bIncludeRebar_Moment ||
        !::IsEqual(m_FlexureModulusOfRuptureCoefficient[pgsTypes::Normal], rOther.m_FlexureModulusOfRuptureCoefficient[pgsTypes::Normal]) ||
        !::IsEqual(m_FlexureModulusOfRuptureCoefficient[pgsTypes::SandLightweight], rOther.m_FlexureModulusOfRuptureCoefficient[pgsTypes::SandLightweight]) ||
      (GetSpecificationType() < lrfdVersionMgr::SeventhEditionWith2016Interims && !::IsEqual(m_FlexureModulusOfRuptureCoefficient[pgsTypes::AllLightweight], rOther.m_FlexureModulusOfRuptureCoefficient[pgsTypes::AllLightweight])) )
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Moment Capacity parameters are different"),_T(""),_T("")));
   }

   bool bPhiFactors = true;
   for ( int i = 0; i < pgsTypes::ConcreteTypeCount && bPhiFactors == true; i++ )
   {
      pgsTypes::ConcreteType concreteType = pgsTypes::ConcreteType(i);
      if ( concreteType == pgsTypes::AllLightweight && lrfdVersionMgr::SeventhEditionWith2016Interims <= GetSpecificationType() )
      {
         // All Lightweight not used after LRFD2016, there is only Lightweight and thos parameters are stored with pgsTypes::SandLightweight
         continue;
      }

      if ( !::IsEqual(m_PhiFlexureTensionPS[concreteType],      rOther.m_PhiFlexureTensionPS[concreteType]) ||
           !::IsEqual(m_PhiFlexureTensionRC[concreteType],      rOther.m_PhiFlexureTensionRC[concreteType]) ||
           !::IsEqual(m_PhiFlexureTensionSpliced[concreteType], rOther.m_PhiFlexureTensionSpliced[concreteType]) ||
           !::IsEqual(m_PhiFlexureCompression[concreteType],    rOther.m_PhiFlexureCompression[concreteType]) ||
           !::IsEqual(m_PhiShear[concreteType],                 rOther.m_PhiShear[concreteType]) ||
           !::IsEqual(m_PhiShearDebonded[concreteType],         rOther.m_PhiShearDebonded[concreteType]) ||
           !::IsEqual(m_PhiClosureJointFlexure[concreteType],rOther.m_PhiClosureJointFlexure[concreteType]) ||
           !::IsEqual(m_PhiClosureJointShear[concreteType],  rOther.m_PhiClosureJointShear[concreteType]) )
      {
         bPhiFactors = false;
      }
   }
   if ( !bPhiFactors )
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Moment Resistance Factors are different"),_T(""),_T("")));
   }

   if ( m_bIncludeForNegMoment != rOther.m_bIncludeForNegMoment )
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Negative Moment Capacity parameters are different"),_T(""),_T("")));
   }

   //
   // Shear Capacity Tab
   //

   if ( m_ShearCapacityMethod != rOther.m_ShearCapacityMethod ||
      m_bLimitNetTensionStrainToPositiveValues != rOther.m_bLimitNetTensionStrainToPositiveValues ||
      !::IsEqual(m_ShearModulusOfRuptureCoefficient[pgsTypes::Normal], rOther.m_ShearModulusOfRuptureCoefficient[pgsTypes::Normal]) ||
      !::IsEqual(m_ShearModulusOfRuptureCoefficient[pgsTypes::SandLightweight], rOther.m_ShearModulusOfRuptureCoefficient[pgsTypes::SandLightweight]) || 
      (GetSpecificationType() < lrfdVersionMgr::SeventhEditionWith2016Interims && !::IsEqual(m_ShearModulusOfRuptureCoefficient[pgsTypes::AllLightweight], rOther.m_ShearModulusOfRuptureCoefficient[pgsTypes::AllLightweight]))
      )
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Shear Capacity parameters are different"),_T(""),_T("")));
   }

   bPhiFactors = true;
   for ( int i = 0; i < pgsTypes::ConcreteTypeCount && bPhiFactors == true; i++ )
   {
      pgsTypes::ConcreteType concreteType = pgsTypes::ConcreteType(i);
      if ( concreteType == pgsTypes::AllLightweight && lrfdVersionMgr::SeventhEditionWith2016Interims <= GetSpecificationType() )
      {
         // All Lightweight not used after LRFD2016, there is only Lightweight and thos parameters are stored with pgsTypes::SandLightweight
         continue;
      }

      if ( !::IsEqual(m_PhiShear[concreteType], rOther.m_PhiShear[concreteType]) )
      {
         bPhiFactors = false;
      }

      if ( !::IsEqual(m_PhiShearDebonded[concreteType], rOther.m_PhiShearDebonded[concreteType]) )
      {
         bPhiFactors = false;
      }
   }
   if ( !bPhiFactors )
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Shear Resistance Factors are different"),_T(""),_T("")));
   }

   if ( !::IsEqual(m_StirrupSpacingCoefficient[0], rOther.m_StirrupSpacingCoefficient[0]) ||
        !::IsEqual(m_StirrupSpacingCoefficient[1], rOther.m_StirrupSpacingCoefficient[1]) ||
        !::IsEqual(m_MaxStirrupSpacing[0],         rOther.m_MaxStirrupSpacing[0]          ) ||
        !::IsEqual(m_MaxStirrupSpacing[1],         rOther.m_MaxStirrupSpacing[1]          ) )
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Minimum Spacing of Transverse Reinforcement requirements are different"),_T(""),_T("")));
   }

   if ( m_LongReinfShearMethod != rOther.m_LongReinfShearMethod ||
        m_bIncludeRebar_Shear  != rOther.m_bIncludeRebar_Shear )
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Longitindal Reinforcement for Shear requirements are different"),_T(""),_T("")));
   }
   
   if ( m_ShearFlowMethod != rOther.m_ShearFlowMethod ||
        !::IsEqual(m_MaxInterfaceShearConnectorSpacing, rOther.m_MaxInterfaceShearConnectorSpacing) || 
        m_bUseDeckWeightForPc != rOther.m_bUseDeckWeightForPc)
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Horizontal Interface Shear requirements are different"),_T(""),_T("")));
   }


   //
   // Creep and Camber Tab
   //
   if ( !::IsEqual(m_XferTime, rOther.m_XferTime) ||
        !::IsEqual(m_CreepDuration1Min, rOther.m_CreepDuration1Min) ||
        !::IsEqual(m_CreepDuration2Min, rOther.m_CreepDuration2Min) ||
        !::IsEqual(m_CreepDuration1Max, rOther.m_CreepDuration1Max) ||
        !::IsEqual(m_CreepDuration2Max, rOther.m_CreepDuration2Max) ||
        !::IsEqual(m_TotalCreepDuration, rOther.m_TotalCreepDuration) ||
        !::IsEqual(m_CamberVariability, rOther.m_CamberVariability) )
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Creep and Camber parameters are different"),_T(""),_T("")));
   }

   if ( m_CuringMethod != rOther.m_CuringMethod ||
      (m_CuringMethod == CURING_ACCELERATED && (!::IsEqual(m_CuringMethodTimeAdjustmentFactor, rOther.m_CuringMethodTimeAdjustmentFactor))) )
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Curing of Precast Concrete parameters are different"),_T(""),_T("")));
   }

   ATLASSERT(m_CreepMethod == CREEP_LRFD);
   ATLASSERT(::IsEqual(m_CreepFactor,2.0));

   //
   // Losses Tab
   //

   // no elastic gains before LRFD2005
   if ( m_LossMethod != rOther.m_LossMethod )
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Prestress Loss Methods are different"),_T(""),_T("")));
   }
   else
   {
      if ( m_LossMethod == LOSSES_AASHTO_REFINED || m_LossMethod == LOSSES_WSDOT_REFINED )
      {
         if ( AreElasticGainsApplicable() )
         {
            if ( !::IsEqual(m_ShippingTime, rOther.m_ShippingTime) || 
                 m_RelaxationLossMethod != rOther.m_RelaxationLossMethod )
            {
               RETURN_ON_DIFFERENCE;
               vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Prestress Loss Parameters are different"),_T(""),_T("")));
            }

            if ( !::IsEqual(m_SlabElasticGain, rOther.m_SlabElasticGain) ||
                 !::IsEqual(m_SlabPadElasticGain, rOther.m_SlabPadElasticGain) ||
                 !::IsEqual(m_DiaphragmElasticGain, rOther.m_DiaphragmElasticGain) ||
                 !::IsEqual(m_UserDCElasticGainBS1, rOther.m_UserDCElasticGainBS1) ||
                 !::IsEqual(m_UserDWElasticGainBS1, rOther.m_UserDWElasticGainBS1) ||
                 !::IsEqual(m_UserDCElasticGainBS2, rOther.m_UserDCElasticGainBS2) ||
                 !::IsEqual(m_UserDWElasticGainBS2, rOther.m_UserDWElasticGainBS2) ||
                 !::IsEqual(m_RailingSystemElasticGain, rOther.m_RailingSystemElasticGain) ||
                 !::IsEqual(m_OverlayElasticGain, rOther.m_OverlayElasticGain) ||
                 !::IsEqual(m_SlabShrinkageElasticGain, rOther.m_SlabShrinkageElasticGain) ||
                 !::IsEqual(m_LiveLoadElasticGain, rOther.m_LiveLoadElasticGain) )
            {
               RETURN_ON_DIFFERENCE;
               vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Elastic Gains are different"),_T(""),_T("")));
            }
         }
         else
         {
            if ( !::IsEqual(m_ShippingLosses, rOther.m_ShippingLosses) ||
                 m_RelaxationLossMethod != rOther.m_RelaxationLossMethod )
            {
               RETURN_ON_DIFFERENCE;
               vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Prestress Loss Parameters are different"),_T(""),_T("")));
            }
         }
      }
      else if ( m_LossMethod == LOSSES_TXDOT_REFINED_2004 )
      {
         if ( GetSpecificationType() <= lrfdVersionMgr::ThirdEdition2004 )
         {
            if ( !::IsEqual(m_ShippingLosses, rOther.m_ShippingLosses) ||
                 m_RelaxationLossMethod != rOther.m_RelaxationLossMethod )
            {
               RETURN_ON_DIFFERENCE;
               vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Prestress Loss Parameters are different"),_T(""),_T("")));
            }
         }
         else
         {
            if ( !::IsEqual(m_ShippingTime, rOther.m_ShippingTime) || 
                 m_RelaxationLossMethod != rOther.m_RelaxationLossMethod )
            {
               RETURN_ON_DIFFERENCE;
               vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Prestress Loss Parameters are different"),_T(""),_T("")));
            }
         }
      }
      else if ( m_LossMethod == LOSSES_TXDOT_REFINED_2013 )
      {
         if ( !::IsEqual(m_ShippingLosses, rOther.m_ShippingLosses) ||
              m_RelaxationLossMethod != rOther.m_RelaxationLossMethod ||
              m_FcgpComputationMethod != rOther.m_FcgpComputationMethod )
         {
            RETURN_ON_DIFFERENCE;
            vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Prestress Loss Parameters are different"),_T(""),_T("")));
         }
      }
      else if ( m_LossMethod == LOSSES_AASHTO_LUMPSUM )
      {
         if ( AreElasticGainsApplicable() )
         {
            if ( !::IsEqual(m_SlabElasticGain, rOther.m_SlabElasticGain) ||
                 !::IsEqual(m_SlabPadElasticGain, rOther.m_SlabPadElasticGain) ||
                 !::IsEqual(m_DiaphragmElasticGain, rOther.m_DiaphragmElasticGain) ||
                 !::IsEqual(m_UserDCElasticGainBS1, rOther.m_UserDCElasticGainBS1) ||
                 !::IsEqual(m_UserDWElasticGainBS1, rOther.m_UserDWElasticGainBS1) ||
                 !::IsEqual(m_UserDCElasticGainBS2, rOther.m_UserDCElasticGainBS2) ||
                 !::IsEqual(m_UserDWElasticGainBS2, rOther.m_UserDWElasticGainBS2) ||
                 !::IsEqual(m_RailingSystemElasticGain, rOther.m_RailingSystemElasticGain) ||
                 !::IsEqual(m_OverlayElasticGain, rOther.m_OverlayElasticGain) ||
                 !::IsEqual(m_LiveLoadElasticGain, rOther.m_LiveLoadElasticGain) )
            {
               RETURN_ON_DIFFERENCE;
               vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Elastic Gains are different"),_T(""),_T("")));
            }
         }
         else
         {
            if ( !::IsEqual(m_ShippingLosses, rOther.m_ShippingLosses) ||
                 m_RelaxationLossMethod != rOther.m_RelaxationLossMethod )
            {
               RETURN_ON_DIFFERENCE;
               vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Prestress Loss Parameters are different"),_T(""),_T("")));
            }
         }
      }
      else if ( m_LossMethod == LOSSES_WSDOT_LUMPSUM )
      {
         RETURN_ON_DIFFERENCE;
         vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Prestress Loss Methods are different"),_T(""),_T("")));
      }
      else
      {
         ATLASSERT(m_LossMethod == LOSSES_TIME_STEP);
         if ( m_TimeDependentModel != rOther.m_TimeDependentModel )
         {
            RETURN_ON_DIFFERENCE;
            vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Time-Dependent Models are different"),_T(""),_T("")));
         }
      }
   }

   //
   // Limits and Warnings Tab
   //
   if ( m_DoCheckStirrupSpacingCompatibility != rOther.m_DoCheckStirrupSpacingCompatibility ||
        (m_bCheckSag != rOther.m_bCheckSag || (m_bCheckSag == true && m_SagCamberType != rOther.m_SagCamberType)) )
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("General Warnings are different"),_T(""),_T("")));
   }

   bool bConcreteLimits = true;
   auto count = m_MaxSlabFc.size();
   for ( int i = 0; i < count && bConcreteLimits == true; i++ )
   {
      pgsTypes::ConcreteType concreteType = pgsTypes::ConcreteType(i);
      if ( concreteType == pgsTypes::AllLightweight && lrfdVersionMgr::SeventhEditionWith2016Interims <= GetSpecificationType() )
      {
         // All Lightweight not used after LRFD2016, there is only Lightweight and those parameters are stored with pgsTypes::SandLightweight
         continue;
      }

      if ( !::IsEqual(m_MaxSlabFc[concreteType]             , rOther.m_MaxSlabFc[concreteType] )             ||
           !::IsEqual(m_MaxSegmentFci[concreteType]         , rOther.m_MaxSegmentFci[concreteType] )         ||
           !::IsEqual(m_MaxSegmentFc[concreteType]          , rOther.m_MaxSegmentFc[concreteType] )          ||
           !::IsEqual(m_MaxClosureFci[concreteType]         , rOther.m_MaxClosureFci[concreteType] )         ||
           !::IsEqual(m_MaxClosureFc[concreteType]          , rOther.m_MaxClosureFc[concreteType] )          ||
           !::IsEqual(m_MaxConcreteUnitWeight[concreteType] , rOther.m_MaxConcreteUnitWeight[concreteType] ) ||
           !::IsEqual(m_MaxConcreteAggSize[concreteType]    , rOther.m_MaxConcreteAggSize[concreteType] ) )
      {
         bConcreteLimits = false;
      }
   }
   if ( !bConcreteLimits )
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Concrete Limits are different"),_T(""),_T("")));
   }

   if (!::IsEqual(m_FinishedElevationTolerance, rOther.m_FinishedElevationTolerance))
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Finished/Design Elevation Tolerances are different"), _T(""), _T("")));
   }

   if (m_SlabOffsetRoundingMethod != rOther.m_SlabOffsetRoundingMethod)
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Required slab offset rounding methods are different"), _T(""), _T("")));
   }

   if (!::IsEqual(m_SlabOffsetRoundingTolerance, rOther.m_SlabOffsetRoundingTolerance))
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Required slab offset rounding tolerance values are different"), _T(""), _T("")));
   }


   // bearing reactions
   if (m_bAlertTaperedSolePlateRequirement != rOther.m_bAlertTaperedSolePlateRequirement)
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Alert tapered sole plate requirement are different"), _T(""), _T("")));
   }

   if (m_bAlertTaperedSolePlateRequirement && !::IsEqual(m_TaperedSolePlateInclinationThreshold, rOther.m_TaperedSolePlateInclinationThreshold))
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Tapered Sole Plate inclination thresholds are different"), _T(""), _T("")));
   }

   if (m_bUseImpactForBearingReactions != rOther.m_bUseImpactForBearingReactions)
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Bearing reactions dynamic load allowances are different"), _T(""), _T("")));
   }
      
   if (considerName &&  GetName() != rOther.GetName() )
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Name"),GetName().c_str(),rOther.GetName().c_str()));
   }


   // Don't compare the placehold values for abandonded parameters
   //if ( m_LossMethod == 3 /*this is the old LOSSES_GENERAL_LUMPSUM that existed prior to version 50*/ )
   //{
   //   TESTD(m_FinalLosses                , rOther.m_FinalLosses                );
   //   TESTD(m_BeforeXferLosses           , rOther.m_BeforeXferLosses           );
   //   TESTD(m_AfterXferLosses            , rOther.m_AfterXferLosses            );
   //   TESTD(m_LiftingLosses              , rOther.m_LiftingLosses              );
   //   TESTD(m_BeforeTempStrandRemovalLosses , rOther.m_BeforeTempStrandRemovalLosses );
   //   TESTD(m_AfterTempStrandRemovalLosses  , rOther.m_AfterTempStrandRemovalLosses );
   //   TESTD(m_AfterDeckPlacementLosses      , rOther.m_AfterDeckPlacementLosses );
   //   TESTD(m_AfterSIDLLosses               , rOther.m_AfterSIDLLosses );
   //}

   //if ( m_bUpdatePTParameters )
   //{
   //   TESTD(m_Dset,rOther.m_Dset);
   //   TESTD(m_WobbleFriction,rOther.m_WobbleFriction);
   //   TESTD(m_FrictionCoefficient,rOther.m_FrictionCoefficient);
   //}

   //TEST(m_bUpdateLoadFactors,rOther.m_bUpdateLoadFactors);
   //for ( int i = 0; i < 6; i++ )
   //{
   //   TESTD( m_DCmin[i], rOther.m_DCmin[i] );
   //   TESTD( m_DWmin[i], rOther.m_DWmin[i] );
   //   TESTD( m_LLIMmin[i], rOther.m_LLIMmin[i] );
   //   TESTD( m_DCmax[i], rOther.m_DCmax[i] );
   //   TESTD( m_DWmax[i], rOther.m_DWmax[i] );
   //   TESTD( m_LLIMmax[i], rOther.m_LLIMmax[i] );
   //}

   return vDifferences.size() == 0 ? true : false;
}

//======================== ACCESS     =======================================
void SpecLibraryEntry::UseCurrentSpecification(bool bUseCurrent)
{
   m_bUseCurrentSpecification = bUseCurrent;
}

bool SpecLibraryEntry::UseCurrentSpecification() const
{
   return m_bUseCurrentSpecification;
}

void SpecLibraryEntry::SetSpecificationType(lrfdVersionMgr::Version type)
{
   m_SpecificationType = type;
}

lrfdVersionMgr::Version SpecLibraryEntry::GetSpecificationType() const
{
   if (m_bUseCurrentSpecification)
   {
      return (lrfdVersionMgr::Version)((int)lrfdVersionMgr::LastVersion - 1);
   }
   else
   {
      return m_SpecificationType;
   }
}

void SpecLibraryEntry::SetSpecificationUnits(lrfdVersionMgr::Units units)
{
   m_SpecificationUnits = units;
}

lrfdVersionMgr::Units SpecLibraryEntry::GetSpecificationUnits() const
{
   return m_SpecificationUnits;
}

void SpecLibraryEntry::SetDescription(LPCTSTR name)
{
   m_Description.erase();
   m_Description = name;
}

std::_tstring SpecLibraryEntry::GetDescription(bool bApplySymbolSubstitution) const
{
   if (bApplySymbolSubstitution)
   {
      std::_tstring description(m_Description);
      std::_tstring strSubstitute(lrfdVersionMgr::GetCodeString());
      strSubstitute += _T(", ");
      strSubstitute += lrfdVersionMgr::GetVersionString();
      boost::replace_all(description,_T("%BDS%"), strSubstitute);
      return description;
   }
   else
   {
      return m_Description;
   }
}

void SpecLibraryEntry::SetSectionPropertyMode(pgsTypes::SectionPropertyMode spMode)
{
   m_SectionPropertyMode = spMode;
}

pgsTypes::SectionPropertyMode SpecLibraryEntry::GetSectionPropertyMode() const
{
   return m_SectionPropertyMode;
}

void SpecLibraryEntry::GetMaxStrandSlope(bool* doCheck, bool* doDesign, Float64* slope05, Float64* slope06,Float64* slope07) const
{
   *doCheck  = m_DoCheckStrandSlope;
   *doDesign = m_DoDesignStrandSlope;
   *slope05  = m_MaxSlope05;
   *slope06  = m_MaxSlope06;
   *slope07  = m_MaxSlope07;
}

void SpecLibraryEntry::SetMaxStrandSlope(bool doCheck, bool doDesign, Float64 slope05, Float64 slope06,Float64 slope07)
{
   m_DoCheckStrandSlope = doCheck;
   m_DoDesignStrandSlope = doCheck ? doDesign : false; // don't allow design without checking

   if (m_DoCheckStrandSlope)
   {
      m_MaxSlope05 = slope05;
      m_MaxSlope06 = slope06;
      m_MaxSlope07 = slope07;
   }
}

void SpecLibraryEntry::GetHoldDownForce(bool* doCheck, bool*doDesign, int* holdDownForceType,Float64* force,Float64* friction) const
{
   *doCheck  = m_DoCheckHoldDown;
   *doDesign = m_DoDesignHoldDown;
   *holdDownForceType = m_HoldDownForceType;
   *force    = m_HoldDownForce;
   *friction = m_HoldDownFriction;
}

void SpecLibraryEntry::SetHoldDownForce(bool doCheck, bool doDesign, int holdDownForceType,Float64 force,Float64 friction)
{
   m_DoCheckHoldDown = doCheck;
   m_DoDesignHoldDown = doCheck ? doDesign : false; // don't allow design without checking

   if (m_DoCheckHoldDown)
   {
      m_HoldDownForceType = holdDownForceType;
      m_HoldDownForce   = force;
      m_HoldDownFriction = friction;
   }
}

void SpecLibraryEntry::GetPlantHandlingWeightLimit(bool* pbDoCheck, Float64* pLimit) const
{
   *pbDoCheck = m_bCheckHandlingWeightLimit;
   *pLimit = m_HandlingWeightLimit;
}

void SpecLibraryEntry::SetPlantHandlingWeightLimit(bool bDoCheck, Float64 limit)
{
   m_bCheckHandlingWeightLimit = bDoCheck;
   m_HandlingWeightLimit = limit;
}

void SpecLibraryEntry::EnableSplittingCheck(bool enable)
{
   m_DoCheckSplitting = enable;
}

bool SpecLibraryEntry::IsSplittingCheckEnabled() const
{
   return m_DoCheckSplitting;
}

void SpecLibraryEntry::EnableSplittingDesign(bool enable)
{
   m_DoDesignSplitting = enable;
}

bool SpecLibraryEntry::IsSplittingDesignEnabled() const
{
   return m_DoDesignSplitting;
}


void SpecLibraryEntry::EnableConfinementCheck(bool enable)
{
   m_DoCheckConfinement = enable;
}

bool SpecLibraryEntry::IsConfinementCheckEnabled() const
{
   return m_DoCheckConfinement;
}

void SpecLibraryEntry::EnableConfinementDesign(bool enable)
{
   m_DoDesignConfinement = enable;
}

bool SpecLibraryEntry::IsConfinementDesignEnabled() const
{
   return m_DoDesignConfinement;
}

void SpecLibraryEntry::GetMaxStirrupSpacing(Float64* pK1,Float64* pS1,Float64* pK2,Float64* pS2) const
{
   *pK1 = m_StirrupSpacingCoefficient[0];
   *pS1 = m_MaxStirrupSpacing[0];
   *pK2 = m_StirrupSpacingCoefficient[1];
   *pS2 = m_MaxStirrupSpacing[1];
}

void SpecLibraryEntry::SetMaxStirrupSpacing(Float64 K1,Float64 S1,Float64 K2,Float64 S2)
{
   m_StirrupSpacingCoefficient[0] = K1;
   m_MaxStirrupSpacing[0]         = S1;
   m_StirrupSpacingCoefficient[1] = K2;
   m_MaxStirrupSpacing[1]         = S2;
}

void SpecLibraryEntry::EnableLiftingCheck(bool enable)
{
   m_EnableLiftingCheck = enable;
}

bool SpecLibraryEntry::IsLiftingAnalysisEnabled() const
{
   return m_EnableLiftingCheck;
}

void SpecLibraryEntry::EnableLiftingDesign(bool enable)
{
   m_EnableLiftingDesign = enable;
}

bool SpecLibraryEntry::IsLiftingDesignEnabled() const
{
   if (m_EnableLiftingCheck)
   {
      return m_EnableLiftingDesign;
   }
   else
   {
      ATLASSERT(!m_EnableLiftingDesign);
      return false;
   }
}

Float64 SpecLibraryEntry::GetCrackingFOSLifting() const
{
   return m_CyLiftingCrackFs;
}

void SpecLibraryEntry::SetCrackingFOSLifting(Float64 fs)
{
   m_CyLiftingCrackFs = fs;
}

Float64 SpecLibraryEntry::GetLiftingFailureFOS() const
{
   return m_CyLiftingFailFs;
}

void SpecLibraryEntry::SetLiftingFailureFOS(Float64 fs)
{
   m_CyLiftingFailFs = fs;
}


Float64 SpecLibraryEntry::GetLiftingUpwardImpactFactor() const
{
   return m_LiftingUpwardImpact;
}

void SpecLibraryEntry::SetLiftingUpwardImpactFactor(Float64 impact)
{
   m_LiftingUpwardImpact = impact;
}

Float64 SpecLibraryEntry::GetLiftingDownwardImpactFactor() const
{
   return m_LiftingDownwardImpact;
}

void SpecLibraryEntry::SetLiftingDownwardImpactFactor(Float64 impact)
{
   m_LiftingDownwardImpact = impact;
}

void SpecLibraryEntry::SetSplittingZoneLengthFactor(Float64 n)
{
   m_SplittingZoneLengthFactor = n;
}

Float64 SpecLibraryEntry::GetSplittingZoneLengthFactor() const
{
   return m_SplittingZoneLengthFactor;
}

void SpecLibraryEntry::EnableHaulingCheck(bool enable)
{
   m_EnableHaulingCheck = enable;
}

bool SpecLibraryEntry::IsHaulingAnalysisEnabled() const
{
   return m_EnableHaulingCheck;
}

void SpecLibraryEntry::EnableHaulingDesign(bool enable)
{
   m_EnableHaulingDesign = enable;
}

bool SpecLibraryEntry::IsHaulingDesignEnabled() const
{
   if (m_EnableHaulingCheck)
   {
      return m_EnableHaulingDesign;
   }
   else
   {
      ATLASSERT(!m_EnableHaulingDesign); // design should not be enabled if check is not
      return false;
   }
}

void SpecLibraryEntry::SetHaulingAnalysisMethod(pgsTypes::HaulingAnalysisMethod method)
{
   m_HaulingAnalysisMethod = method;
}

pgsTypes::HaulingAnalysisMethod SpecLibraryEntry::GetHaulingAnalysisMethod() const
{
   return m_HaulingAnalysisMethod;
}

Float64 SpecLibraryEntry::GetHaulingUpwardImpactFactor() const
{
   return m_HaulingUpwardImpact;
}

void SpecLibraryEntry::SetHaulingUpwardImpactFactor(Float64 impact)
{
   m_HaulingUpwardImpact = impact;
}

Float64 SpecLibraryEntry::GetHaulingDownwardImpactFactor() const
{
   return m_HaulingDownwardImpact;
}

void SpecLibraryEntry::SetHaulingDownwardImpactFactor(Float64 impact)
{
   m_HaulingDownwardImpact = impact;
}

Float64 SpecLibraryEntry::GetAtReleaseCompressionStressFactor() const
{
   return m_CyCompStressServ;
}

void SpecLibraryEntry::SetAtReleaseCompressionStressFactor(Float64 stress)
{
   m_CyCompStressServ = stress;
}

Float64 SpecLibraryEntry::GetLiftingCompressionGlobalStressFactor() const
{
   return m_LiftingCompressionStressCoefficient_GlobalStress;
}

void SpecLibraryEntry::SetLiftingCompressionGlobalStressFactor(Float64 stress)
{
   m_LiftingCompressionStressCoefficient_GlobalStress = stress;
}

Float64 SpecLibraryEntry::GetLiftingCompressionPeakStressFactor() const
{
   return m_LiftingCompressionStressCoefficient_PeakStress;
}

void SpecLibraryEntry::SetLiftingCompressionPeakStressFactor(Float64 stress)
{
   m_LiftingCompressionStressCoefficient_PeakStress = stress;
}

Float64 SpecLibraryEntry::GetAtReleaseTensionStressFactor() const
{
   return m_CyTensStressServ;
}

void SpecLibraryEntry::SetAtReleaseTensionStressFactor(Float64 stress)
{
   m_CyTensStressServ = stress;
}

void SpecLibraryEntry::GetAtReleaseMaximumTensionStress(bool* doCheck, Float64* stress) const
{
   *doCheck = m_CyDoTensStressServMax;
   *stress  = m_CyTensStressServMax;
}

void SpecLibraryEntry::SetAtReleaseMaximumTensionStress(bool doCheck, Float64 stress)
{
   m_CyDoTensStressServMax = doCheck;
   m_CyTensStressServMax   = stress;
}

Float64 SpecLibraryEntry::GetLiftingTensionStressFactor() const
{
   return m_CyTensStressLifting;
}

void SpecLibraryEntry::SetLiftingTensionStressFactor(Float64 stress)
{
   m_CyTensStressLifting = stress;
}

void SpecLibraryEntry::GetLiftingMaximumTensionStress(bool* doCheck, Float64* stress) const
{
   *doCheck = m_CyDoTensStressLiftingMax;
   *stress  = m_CyTensStressLiftingMax;
}

void SpecLibraryEntry::SetLiftingMaximumTensionStress(bool doCheck, Float64 stress)
{
   m_CyDoTensStressLiftingMax = doCheck;
   m_CyTensStressLiftingMax   = stress;
}

int SpecLibraryEntry::GetCuringMethod() const
{
   return m_CuringMethod;
}

void SpecLibraryEntry::SetCuringMethod(int method)
{
   m_CuringMethod = method;
}

Float64 SpecLibraryEntry::GetPickPointHeight() const
{
   return m_PickPointHeight;
}

void SpecLibraryEntry::SetPickPointHeight(Float64 hgt)
{
   m_PickPointHeight = hgt;
}

Float64 SpecLibraryEntry::GetLiftingLoopTolerance() const
{
   return m_LiftingLoopTolerance;
}

void SpecLibraryEntry::SetLiftingLoopTolerance(Float64 tol)
{
   m_LiftingLoopTolerance = tol;
}

Float64 SpecLibraryEntry::GetMinCableInclination() const
{
   return m_MinCableInclination;
}

void SpecLibraryEntry::SetMinCableInclination(Float64 angle)
{
   m_MinCableInclination = angle;
}

Float64 SpecLibraryEntry::GetLiftingMaximumGirderSweepTolerance() const
{
   return m_MaxGirderSweepLifting;
}

void SpecLibraryEntry::SetLiftingMaximumGirderSweepTolerance(Float64 sweep)
{
   m_MaxGirderSweepLifting = sweep;
}

Float64 SpecLibraryEntry::GetHaulingMaximumGirderSweepTolerance() const
{
   return m_MaxGirderSweepHauling;
}

void SpecLibraryEntry::SetHaulingMaximumGirderSweepTolerance(Float64 sweep)
{
   m_MaxGirderSweepHauling = sweep;
}

Float64 SpecLibraryEntry::GetHaulingSweepGrowth() const
{
   return m_HaulingSweepGrowth;
}

void SpecLibraryEntry::SetHaulingSweepGrowth(Float64 sweepGrowth)
{
   m_HaulingSweepGrowth = sweepGrowth;
}

Float64 SpecLibraryEntry::GetHaulingSupportPlacementTolerance() const
{
   return m_HaulingSupportPlacementTolerance;
}

void SpecLibraryEntry::SetHaulingSupportPlacementTolerance(Float64 tol)
{
   m_HaulingSupportPlacementTolerance = tol;
}

Float64 SpecLibraryEntry::GetHaulingCamberMultiplier() const
{
   return m_HaulingCamberMultiplier;
}

void SpecLibraryEntry::SetHaulingCamberMultiplier(Float64 m)
{
   m_HaulingCamberMultiplier = m;
}

const COldHaulTruck* SpecLibraryEntry::GetOldHaulTruck() const
{
   if ( m_bHasOldHaulTruck )
   {
      return &m_OldHaulTruck;
   }
   else
   {
      return nullptr;
   }
}

Float64 SpecLibraryEntry::GetHaulingCompressionGlobalStressFactor() const
{
   return m_GlobalCompStressHauling;
}

void SpecLibraryEntry::SetHaulingCompressionGlobalStressFactor(Float64 stress)
{
   m_GlobalCompStressHauling = stress;
}

Float64 SpecLibraryEntry::GetHaulingCompressionPeakStressFactor() const
{
   return m_PeakCompStressHauling;
}

void SpecLibraryEntry::SetHaulingCompressionPeakStressFactor(Float64 stress)
{
   m_PeakCompStressHauling = stress;
}

Float64 SpecLibraryEntry::GetHaulingTensionStressFactor(pgsTypes::HaulingSlope slope) const
{
   return m_TensStressHauling[slope];
}

void SpecLibraryEntry::SetHaulingTensionStressFactor(pgsTypes::HaulingSlope slope,Float64 stress)
{
   m_TensStressHauling[slope] = stress;
}

void SpecLibraryEntry::GetHaulingMaximumTensionStress(pgsTypes::HaulingSlope slope,bool* doCheck, Float64* stress) const
{
   *doCheck = m_DoTensStressHaulingMax[slope];
   *stress  = m_TensStressHaulingMax[slope];
}

void SpecLibraryEntry::SetHaulingMaximumTensionStress(pgsTypes::HaulingSlope slope,bool doCheck, Float64 stress)
{
   m_DoTensStressHaulingMax[slope] = doCheck;
   m_TensStressHaulingMax[slope] = stress;
}

Float64 SpecLibraryEntry::GetHaulingCrackingFOS() const
{
   return m_HaulingCrackFs;
}

void SpecLibraryEntry::SetHaulingCrackingFOS(Float64 fs)
{
   m_HaulingCrackFs = fs;
}

Float64 SpecLibraryEntry::GetHaulingFailureFOS() const
{
   return m_HaulingRollFs;
}

void SpecLibraryEntry::SetHaulingFailureFOS(Float64 fs)
{
   m_HaulingRollFs = fs;
}

void SpecLibraryEntry::SetHaulingImpactUsage(pgsTypes::HaulingImpact impactUsage)
{
   m_HaulingImpactUsage;
}

pgsTypes::HaulingImpact SpecLibraryEntry::GetHaulingImpactUsage() const
{
   return m_HaulingImpactUsage;
}

Float64 SpecLibraryEntry::GetRoadwayCrownSlope() const
{
   return m_RoadwayCrownSlope;
}

void SpecLibraryEntry::SetRoadwayCrownSlope(Float64 slope)
{
   m_RoadwayCrownSlope = slope;
}

Float64 SpecLibraryEntry::GetRoadwaySuperelevation() const
{
   return m_RoadwaySuperelevation;
}

void SpecLibraryEntry::SetRoadwaySuperelevation(Float64 dist)
{
   m_RoadwaySuperelevation = dist;
}

pgsTypes::WindType SpecLibraryEntry::GetHaulingWindType() const
{
   return m_HaulingWindType;
}

void SpecLibraryEntry::SetHaulingWindType(pgsTypes::WindType windType)
{
   m_HaulingWindType = windType;
}

Float64 SpecLibraryEntry::GetHaulingWindLoad() const
{
   return m_HaulingWindLoad;
}

void SpecLibraryEntry::SetHaulingWindLoad(Float64 wl)
{
   m_HaulingWindLoad = wl;
}

pgsTypes::CFType SpecLibraryEntry::GetCentrifugalForceType() const
{
   return m_CentrifugalForceType;
}

void SpecLibraryEntry::SetCentrifugalForceType(pgsTypes::CFType cfType)
{
   m_CentrifugalForceType = cfType;
}

Float64 SpecLibraryEntry::GetHaulingSpeed() const
{
   return m_HaulingSpeed;
}

void SpecLibraryEntry::SetHaulingSpeed(Float64 v)
{
   m_HaulingSpeed = v;
}

Float64 SpecLibraryEntry::GetTurningRadius() const
{
   return m_TurningRadius;
}

void SpecLibraryEntry::SetTurningRadius(Float64 r)
{
   m_TurningRadius = r;
}

void SpecLibraryEntry::SetHaulingModulusOfRuptureFactor(Float64 fr, pgsTypes::ConcreteType type)
{
   ATLASSERT(!IsUHPC(type));
   m_HaulingModulusOfRuptureCoefficient[type] = fr;
}

Float64 SpecLibraryEntry::GetHaulingModulusOfRuptureFactor(pgsTypes::ConcreteType type) const
{
   if (IsUHPC(type))
      return 0;

   return m_HaulingModulusOfRuptureCoefficient[type];
}

void SpecLibraryEntry::SetLiftingModulusOfRuptureFactor(Float64 fr,pgsTypes::ConcreteType type)
{
   ATLASSERT(!IsUHPC(type));
   m_LiftingModulusOfRuptureCoefficient[type] = fr;
}

Float64 SpecLibraryEntry::GetLiftingModulusOfRuptureFactor(pgsTypes::ConcreteType type) const
{
   if (IsUHPC(type))
      return 0;

   return m_LiftingModulusOfRuptureCoefficient[type];
}

Float64 SpecLibraryEntry::GetLiftingCamberMultiplier() const
{
   return m_LiftingCamberMultiplier;
}

void SpecLibraryEntry::SetLiftingCamberMultiplier(Float64 m)
{
   m_LiftingCamberMultiplier = m;
}

pgsTypes::WindType SpecLibraryEntry::GetLiftingWindType() const
{
   return m_LiftingWindType;
}

void SpecLibraryEntry::SetLiftingWindType(pgsTypes::WindType windType)
{
   m_LiftingWindType = windType;
}

Float64 SpecLibraryEntry::GetLiftingWindLoad() const
{
   return m_LiftingWindLoad;
}

void SpecLibraryEntry::SetLiftingWindLoad(Float64 wl)
{
   m_LiftingWindLoad = wl;
}

Float64 SpecLibraryEntry::GetAtReleaseTensionStressFactorWithRebar() const
{
   return m_CyTensStressServWithRebar;
}

void SpecLibraryEntry::SetAtReleaseTensionStressFactorWithRebar(Float64 stress)
{
   m_CyTensStressServWithRebar = stress;
}

Float64 SpecLibraryEntry::GetLiftingTensionStressFactorWithRebar() const
{
   return m_TensStressLiftingWithRebar;
}

void SpecLibraryEntry::SetLiftingTensionStressFactorWithRebar(Float64 stress)
{
    m_TensStressLiftingWithRebar = stress;
}

Float64 SpecLibraryEntry::GetHaulingTensionStressFactorWithRebar(pgsTypes::HaulingSlope slope) const
{
   return m_TensStressHaulingWithRebar[slope];
}

void SpecLibraryEntry::SetHaulingTensionStressFactorWithRebar(pgsTypes::HaulingSlope slope,Float64 stress)
{
    m_TensStressHaulingWithRebar[slope] = stress;
}

Float64 SpecLibraryEntry::GetTempStrandRemovalCompressionStressFactor() const
{
   return m_TempStrandRemovalCompStress;
}

void SpecLibraryEntry::SetTempStrandRemovalCompressionStressFactor(Float64 stress)
{
   m_TempStrandRemovalCompStress = stress;
}

Float64 SpecLibraryEntry::GetTempStrandRemovalTensionStressFactor() const
{
   return m_TempStrandRemovalTensStress;
}

void SpecLibraryEntry::SetTempStrandRemovalTensionStressFactor(Float64 stress)
{
   m_TempStrandRemovalTensStress = stress;
}

void SpecLibraryEntry::GetTempStrandRemovalMaximumTensionStress(bool* doCheck, Float64* stress) const
{
   *doCheck = m_TempStrandRemovalDoTensStressMax;
   *stress  = m_TempStrandRemovalTensStressMax;
}

void SpecLibraryEntry::SetTempStrandRemovalMaximumTensionStress(bool doCheck, Float64 stress)
{
   m_TempStrandRemovalDoTensStressMax = doCheck;
   m_TempStrandRemovalTensStressMax   = stress;
}

Float64 SpecLibraryEntry::GetTempStrandRemovalTensionStressFactorWithRebar() const
{
   return m_TempStrandRemovalTensStressWithRebar;
}

void SpecLibraryEntry::SetTempStrandRemovalTensionStressFactorWithRebar(Float64 stress)
{
   m_TempStrandRemovalTensStressWithRebar = stress;
}

void SpecLibraryEntry::CheckTemporaryStresses(bool bCheck)
{
   m_bCheckTemporaryStresses = bCheck;
}

bool SpecLibraryEntry::CheckTemporaryStresses() const
{
   return m_bCheckTemporaryStresses;
}

Float64 SpecLibraryEntry::GetErectionCompressionStressFactor() const
{
   return m_Bs1CompStress;
}

void SpecLibraryEntry::SetErectionCompressionStressFactor(Float64 stress)
{
   m_Bs1CompStress = stress;
}

Float64 SpecLibraryEntry::GetErectionTensionStressFactor() const
{
   return m_Bs1TensStress;
}

void SpecLibraryEntry::SetErectionTensionStressFactor(Float64 stress)
{
   m_Bs1TensStress = stress;
}

void SpecLibraryEntry::GetErectionMaximumTensionStress(bool* doCheck, Float64* stress) const
{
   *doCheck = m_Bs1DoTensStressMax;
   *stress  = m_Bs1TensStressMax;
}

void SpecLibraryEntry::SetErectionMaximumTensionStress(bool doCheck, Float64 stress)
{
   m_Bs1DoTensStressMax = doCheck;
   m_Bs1TensStressMax   = stress;
}

Float64 SpecLibraryEntry::GetFinalWithoutLiveLoadCompressionStressFactor() const
{
   return m_Bs2CompStress;
}

void SpecLibraryEntry::SetFinalWithoutLiveLoadCompressionStressFactor(Float64 stress)
{
   m_Bs2CompStress = stress;
}

void SpecLibraryEntry::CheckFinalTensionPermanentLoadStresses(bool bCheck)
{
   m_bCheckBs2Tension = bCheck;
}

bool SpecLibraryEntry::CheckFinalTensionPermanentLoadStresses() const
{
   return m_bCheckBs2Tension;
}

Float64 SpecLibraryEntry::GetFinalTensionPermanentLoadsStressFactor() const
{
   return m_Bs2TensStress;
}

void SpecLibraryEntry::SetFinalTensionPermanentLoadsStressFactor(Float64 stress)
{
   m_Bs2TensStress = stress;
}

void SpecLibraryEntry::GetFinalTensionPermanentLoadStressFactor(bool* doCheck, Float64* stress) const
{
   *doCheck = m_Bs2DoTensStressMax;
   *stress = m_Bs2TensStressMax;
}

void SpecLibraryEntry::SetFinalTensionPermanentLoadStressFactor(bool doCheck, Float64 stress)
{
   m_Bs2DoTensStressMax = doCheck;
   m_Bs2TensStressMax = stress;
}

Float64 SpecLibraryEntry::GetFinalWithLiveLoadCompressionStressFactor() const
{
   return m_Bs3CompStressServ;
}

void SpecLibraryEntry::SetFinalWithLiveLoadCompressionStressFactor(Float64 stress)
{
   m_Bs3CompStressServ = stress;
}

Float64 SpecLibraryEntry::GetFatigueCompressionStressFactor() const
{
   return m_Bs3CompStressService1A;
}

void SpecLibraryEntry::SetFatigueCompressionStressFactor(Float64 stress)
{
   m_Bs3CompStressService1A = stress;
}

void SpecLibraryEntry::SetPrincipalTensileStressInWebsParameters(pgsTypes::PrincipalTensileStressMethod principalTensileStressMethod, Float64 principalTensionCoefficient,Float64 ductNearnessFactor,
                                                                 Float64 principalTensileStressUngroutedMultiplier, Float64 principalTensileStressGroutedMultiplier,Float64 principalTensileStressFcThreshold)
{
   m_PrincipalTensileStressMethod = principalTensileStressMethod;
   m_PrincipalTensileStressCoefficient = principalTensionCoefficient;
   m_PrincipalTensileStressTendonNearnessFactor = ductNearnessFactor;
   m_PrincipalTensileStressFcThreshold = principalTensileStressFcThreshold;
   m_PrincipalTensileStressUngroutedMultiplier = principalTensileStressUngroutedMultiplier;
   m_PrincipalTensileStressGroutedMultiplier = principalTensileStressGroutedMultiplier;
}

void SpecLibraryEntry::GetPrincipalTensileStressInWebsParameters(pgsTypes::PrincipalTensileStressMethod* pPrincipalTensileStressMethod, Float64* pPrincipalTensionCoefficient,Float64* pDuctNearnessFactor,
                                                                 Float64* pPrincipalTensileStressUngroutedMultiplier, Float64* pPrincipalTensileStressGroutedMultiplier, Float64* principalTensileStressFcThreshold) const
{
   *pPrincipalTensileStressMethod = m_PrincipalTensileStressMethod;
   *pPrincipalTensionCoefficient = m_PrincipalTensileStressCoefficient;
   *pDuctNearnessFactor = m_PrincipalTensileStressTendonNearnessFactor;

   // note that magic value of -1 means "all" in library. Return zero to our clients, which means any f'c
   *principalTensileStressFcThreshold = m_PrincipalTensileStressFcThreshold==-1.0 ? 0.0 : m_PrincipalTensileStressFcThreshold;
   *pPrincipalTensileStressUngroutedMultiplier = m_PrincipalTensileStressUngroutedMultiplier;
   *pPrincipalTensileStressGroutedMultiplier = m_PrincipalTensileStressGroutedMultiplier;
}

Float64 SpecLibraryEntry::GetFinalTensionStressFactor(int exposureCondition) const
{
   return exposureCondition == EXPOSURE_NORMAL ? m_Bs3TensStressServNc : m_Bs3TensStressServSc;
}

void SpecLibraryEntry::SetFinalTensionStressFactor(int exposureCondition,Float64 stress)
{
   if ( exposureCondition == EXPOSURE_NORMAL )
   {
      m_Bs3TensStressServNc = stress;
   }
   else
   {
      m_Bs3TensStressServSc = stress;
   }
}

void SpecLibraryEntry::GetFinalTensionStressFactor(int exposureCondition,bool* doCheck, Float64* stress) const
{
   if ( exposureCondition == EXPOSURE_NORMAL )
   {
      *doCheck = m_Bs3DoTensStressServNcMax;
      *stress  = m_Bs3TensStressServNcMax;
   }
   else
   {
      *doCheck = m_Bs3DoTensStressServScMax;
      *stress  = m_Bs3TensStressServScMax;
   }
}

void SpecLibraryEntry::SetFinalTensionStressFactor(int exposureCondition,bool doCheck, Float64 stress)
{
   if ( exposureCondition == EXPOSURE_NORMAL )
   {
      m_Bs3DoTensStressServNcMax = doCheck;
      m_Bs3TensStressServNcMax   = stress;
   }
   else
   {
      m_Bs3DoTensStressServScMax = doCheck;
      m_Bs3TensStressServScMax   = stress;
   }
}

void SpecLibraryEntry::SetLRFDOverreinforcedMomentCapacity(bool bSet)
{
   m_Bs3LRFDOverReinforcedMomentCapacity = bSet ? 0 : 1;
}

bool SpecLibraryEntry::GetLRFDOverreinforcedMomentCapacity() const
{
   return m_Bs3LRFDOverReinforcedMomentCapacity == 0 ? true : false;
}

void SpecLibraryEntry::IgnoreRangeOfApplicabilityRequirements(bool bIgnore)
{
   m_Bs3IgnoreRangeOfApplicability = bIgnore;
}

bool SpecLibraryEntry::IgnoreRangeOfApplicabilityRequirements() const
{
   return m_Bs3IgnoreRangeOfApplicability;
}

void SpecLibraryEntry::CheckStrandStress(UINT stage,bool bCheck)
{
   m_bCheckStrandStress[stage] = bCheck;
}

bool SpecLibraryEntry::CheckStrandStress(UINT stage) const
{
   return m_bCheckStrandStress[stage];
}

void SpecLibraryEntry::SetStrandStressCoefficient(UINT stage,UINT strandType, Float64 coeff)
{
   m_StrandStressCoeff[stage][strandType] = coeff;
}

Float64 SpecLibraryEntry::GetStrandStressCoefficient(UINT stage,UINT strandType) const
{
   return m_StrandStressCoeff[stage][strandType];
}

bool SpecLibraryEntry::CheckTendonStressAtJacking() const
{
   return m_bCheckTendonStressAtJacking;
}

bool SpecLibraryEntry::CheckTendonStressPriorToSeating() const
{
   return m_bCheckTendonStressPriorToSeating;
}

Float64 SpecLibraryEntry::GetTendonStressCoefficient(UINT stage,UINT strandType) const
{
   return m_TendonStressCoeff[stage][strandType];
}

GirderIndexType SpecLibraryEntry::GetMaxGirdersDistTrafficBarrier() const
{
   return m_Bs2MaxGirdersTrafficBarrier;
}

void SpecLibraryEntry::SetMaxGirdersDistTrafficBarrier(GirderIndexType num)
{
   m_Bs2MaxGirdersTrafficBarrier = num;
}

void SpecLibraryEntry::SetTrafficBarrierDistibutionType(pgsTypes::TrafficBarrierDistribution tbd)
{
   m_TrafficBarrierDistributionType = tbd;
}

pgsTypes::TrafficBarrierDistribution SpecLibraryEntry::GetTrafficBarrierDistributionType() const
{
   return m_TrafficBarrierDistributionType;
}

pgsTypes::OverlayLoadDistributionType SpecLibraryEntry::GetOverlayLoadDistributionType() const
{
   return m_OverlayLoadDistribution;
}

void SpecLibraryEntry::SetOverlayLoadDistributionType(pgsTypes::OverlayLoadDistributionType type)
{
   m_OverlayLoadDistribution = type;
}

pgsTypes::HaunchLoadComputationType SpecLibraryEntry::GetHaunchLoadComputationType() const
{
   return m_HaunchLoadComputationType;
}

void SpecLibraryEntry::SetHaunchLoadComputationType(pgsTypes::HaunchLoadComputationType type)
{
   m_HaunchLoadComputationType = type;
}

Float64 SpecLibraryEntry::GetHaunchLoadCamberTolerance() const
{
   return m_HaunchLoadCamberTolerance;
}

void SpecLibraryEntry::SetHaunchLoadCamberTolerance(Float64 tol)
{
   m_HaunchLoadCamberTolerance = tol;
}

Float64 SpecLibraryEntry::GetHaunchLoadCamberFactor() const
{
   return m_HaunchLoadCamberFactor;
}

void SpecLibraryEntry::SetHaunchLoadCamberFactor(Float64 tol)
{
   m_HaunchLoadCamberFactor = tol;
}

pgsTypes:: HaunchAnalysisSectionPropertiesType SpecLibraryEntry::GetHaunchAnalysisSectionPropertiesType() const
{
   return m_HaunchAnalysisSectionPropertiesType;
}

void SpecLibraryEntry::SetHaunchAnalysisSectionPropertiesType(pgsTypes:: HaunchAnalysisSectionPropertiesType type)
{
   m_HaunchAnalysisSectionPropertiesType = type;
}

void SpecLibraryEntry::SetCreepMethod(int method)
{
   PRECONDITION( 0 <= method && method <= 1 );
   m_CreepMethod = method;
}

int SpecLibraryEntry::GetCreepMethod() const
{
   return m_CreepMethod;
}

void SpecLibraryEntry::SetXferTime(Float64 time)
{
   m_XferTime = time;
}

Float64 SpecLibraryEntry::GetXferTime() const
{
   return m_XferTime;
}

Float64 SpecLibraryEntry::GetCreepFactor() const
{
   return m_CreepFactor;
}

void SpecLibraryEntry::SetCreepFactor(Float64 cf)
{
   m_CreepFactor = cf;
}

Float64 SpecLibraryEntry::GetCreepDuration1Min() const
{
   return m_CreepDuration1Min;
}

Float64 SpecLibraryEntry::GetCreepDuration1Max() const
{
   return m_CreepDuration1Max;
}

void SpecLibraryEntry::SetCreepDuration1(Float64 min,Float64 max)
{
   m_CreepDuration1Min = min;
   m_CreepDuration1Max = max;
}

Float64 SpecLibraryEntry::GetCreepDuration2Min() const
{
   return m_CreepDuration2Min;
}

Float64 SpecLibraryEntry::GetCreepDuration2Max() const
{
   return m_CreepDuration2Max;
}

void SpecLibraryEntry::SetCreepDuration2(Float64 min,Float64 max)
{
   m_CreepDuration2Min = min;
   m_CreepDuration2Max = max;
}

void SpecLibraryEntry::SetTotalCreepDuration(Float64 duration)
{
   m_TotalCreepDuration = duration;
}

Float64 SpecLibraryEntry::GetTotalCreepDuration() const
{
   return m_TotalCreepDuration;
}

void SpecLibraryEntry::SetCamberVariability(Float64 var)
{
   m_CamberVariability = var;
}

Float64 SpecLibraryEntry::GetCamberVariability() const
{
   return m_CamberVariability;
}

void SpecLibraryEntry::CheckGirderSag(bool bCheck)
{
   m_bCheckSag = bCheck;
}

bool SpecLibraryEntry::CheckGirderSag() const
{
   return m_bCheckSag;
}

pgsTypes::SagCamberType SpecLibraryEntry::GetSagCamberType() const
{
   return m_SagCamberType;
}

void SpecLibraryEntry::SetSagCamberType(pgsTypes::SagCamberType type)
{
   m_SagCamberType = type;
}

int SpecLibraryEntry::GetLossMethod() const
{
   return m_LossMethod;
}

void SpecLibraryEntry::SetLossMethod(int method)
{
   PRECONDITION( 0 <= method && method <= 3 );
   m_LossMethod = method;
}

Float64 SpecLibraryEntry::GetFinalLosses() const
{
   return m_FinalLosses;
}

void SpecLibraryEntry::SetFinalLosses(Float64 loss)
{
   m_FinalLosses = loss;
}

int SpecLibraryEntry::GetTimeDependentModel() const
{
   return m_TimeDependentModel;
}

void SpecLibraryEntry::SetTimeDependentModel(int model)
{
   m_TimeDependentModel = model;
}

Float64 SpecLibraryEntry::GetShippingLosses() const
{
   return m_ShippingLosses;
}

void SpecLibraryEntry::SetShippingLosses(Float64 loss)
{
   m_ShippingLosses = loss;
}

Float64 SpecLibraryEntry::GetLiftingLosses() const
{
   return m_LiftingLosses;
}

void SpecLibraryEntry::SetLiftingLosses(Float64 loss)
{
   m_LiftingLosses = loss;
}

Float64 SpecLibraryEntry::GetBeforeXferLosses() const
{
   return m_BeforeXferLosses;
}

void SpecLibraryEntry::SetBeforeXferLosses(Float64 loss)
{
   m_BeforeXferLosses = loss;
}

Float64 SpecLibraryEntry::GetAfterXferLosses() const
{
   return m_AfterXferLosses;
}

void SpecLibraryEntry::SetAfterXferLosses(Float64 loss)
{
   m_AfterXferLosses = loss;
}

void SpecLibraryEntry::SetShippingTime(Float64 time)
{
   m_ShippingTime = time;
}

Float64 SpecLibraryEntry::GetShippingTime() const
{
   return m_ShippingTime;
}

Float64 SpecLibraryEntry::GetBeforeTempStrandRemovalLosses() const
{
   return m_BeforeTempStrandRemovalLosses;
}

void SpecLibraryEntry::SetBeforeTempStrandRemovalLosses(Float64 loss)
{
   m_BeforeTempStrandRemovalLosses = loss;
}

Float64 SpecLibraryEntry::GetAfterTempStrandRemovalLosses() const
{
   return m_AfterTempStrandRemovalLosses;
}

void SpecLibraryEntry::SetAfterTempStrandRemovalLosses(Float64 loss)
{
   m_AfterTempStrandRemovalLosses = loss;
}

Float64 SpecLibraryEntry::GetAfterDeckPlacementLosses() const
{
   return m_AfterDeckPlacementLosses;
}

void SpecLibraryEntry::SetAfterDeckPlacementLosses(Float64 loss)
{
   m_AfterDeckPlacementLosses = loss;
}

Float64 SpecLibraryEntry::GetAfterSIDLLosses() const
{
   return m_AfterSIDLLosses;
}

void SpecLibraryEntry::SetAfterSIDLLosses(Float64 loss)
{
   m_AfterSIDLLosses = loss;
}

bool SpecLibraryEntry::UpdatePTParameters() const
{
   return m_bUpdatePTParameters;
}

Float64 SpecLibraryEntry::GetAnchorSet() const
{
   return m_Dset;
}

void SpecLibraryEntry::SetAnchorSet(Float64 dset)
{
   m_Dset = dset;
}

Float64 SpecLibraryEntry::GetWobbleFrictionCoefficient() const
{
   return m_WobbleFriction;
}

void SpecLibraryEntry::SetWobbleFrictionCoefficient(Float64 K)
{
   m_WobbleFriction = K;
}

Float64 SpecLibraryEntry::GetFrictionCoefficient() const
{
   return m_FrictionCoefficient;
}

void SpecLibraryEntry::SetFrictionCoefficient(Float64 u)
{
   m_FrictionCoefficient = u;
}

Float64 SpecLibraryEntry::GetSlabElasticGain() const
{
   return m_SlabElasticGain;
}

void SpecLibraryEntry::SetSlabElasticGain(Float64 f)
{
   m_SlabElasticGain = f;
}

Float64 SpecLibraryEntry::GetSlabPadElasticGain() const
{
   return m_SlabPadElasticGain;
}

void SpecLibraryEntry::SetSlabPadElasticGain(Float64 f)
{
   m_SlabPadElasticGain = f;
}

Float64 SpecLibraryEntry::GetDiaphragmElasticGain() const
{
   return m_DiaphragmElasticGain;
}

void SpecLibraryEntry::SetDiaphragmElasticGain(Float64 f)
{
   m_DiaphragmElasticGain = f;
}

Float64 SpecLibraryEntry::GetUserLoadBeforeDeckDCElasticGain() const
{
   return m_UserDCElasticGainBS1;
}

Float64 SpecLibraryEntry::GetUserLoadAfterDeckDCElasticGain() const
{
   return m_UserDCElasticGainBS2;
}

void SpecLibraryEntry::SetUserLoadBeforeDeckDCElasticGain(Float64 f)
{
   m_UserDCElasticGainBS1 = f;
}

void SpecLibraryEntry::SetUserLoadAfterDeckDCElasticGain(Float64 f)
{
   m_UserDCElasticGainBS2 = f;
}

Float64 SpecLibraryEntry::GetUserLoadBeforeDeckDWElasticGain() const
{
   return m_UserDWElasticGainBS1;
}

Float64 SpecLibraryEntry::GetUserLoadAfterDeckDWElasticGain() const
{
   return m_UserDWElasticGainBS2;
}

void SpecLibraryEntry::SetUserLoadBeforeDeckDWElasticGain(Float64 f)
{
   m_UserDWElasticGainBS1 = f;
}

void SpecLibraryEntry::SetUserLoadAfterDeckDWElasticGain(Float64 f)
{
   m_UserDWElasticGainBS2 = f;
}

Float64 SpecLibraryEntry::GetRailingSystemElasticGain() const
{
   return m_RailingSystemElasticGain;
}

void SpecLibraryEntry::SetRailingSystemElasticGain(Float64 f)
{
   m_RailingSystemElasticGain = f;
}

Float64 SpecLibraryEntry::GetOverlayElasticGain() const
{
   return m_OverlayElasticGain;
}

void SpecLibraryEntry::SetOverlayElasticGain(Float64 f)
{
   m_OverlayElasticGain = f;
}

Float64 SpecLibraryEntry::GetDeckShrinkageElasticGain() const
{
   return m_SlabShrinkageElasticGain;
}

void SpecLibraryEntry::SetDeckShrinkageElasticGain(Float64 f)
{
   m_SlabShrinkageElasticGain = f;
}

Float64 SpecLibraryEntry::GetLiveLoadElasticGain() const
{
   return m_LiveLoadElasticGain;
}

void SpecLibraryEntry::SetLiveLoadElasticGain(Float64 f)
{
   m_LiveLoadElasticGain = f;
}

void SpecLibraryEntry::SetRelaxationLossMethod(Int16 method)
{
   m_RelaxationLossMethod = method;
}

Int16 SpecLibraryEntry::GetRelaxationLossMethod() const
{
   return m_RelaxationLossMethod;
}

void SpecLibraryEntry::SetFcgpComputationMethod(Int16 method)
{
   m_FcgpComputationMethod = method;
}

Int16 SpecLibraryEntry::GetFcgpComputationMethod() const
{
   return m_FcgpComputationMethod;
}

bool SpecLibraryEntry::AreElasticGainsApplicable() const
{
   bool isapp(false);
   if (lrfdVersionMgr::ThirdEdition2004 < this->GetSpecificationType())
   {
      int lm = this->GetLossMethod();
      if(LOSSES_AASHTO_REFINED == lm || LOSSES_WSDOT_REFINED  == lm || LOSSES_AASHTO_LUMPSUM == lm )
      {
         isapp = true;
      }
   }

   return isapp;
}

bool SpecLibraryEntry::IsDeckShrinkageApplicable() const
{
   bool isapp(false);
   if (lrfdVersionMgr::ThirdEdition2004 < this->GetSpecificationType())
   {
      int lm = this->GetLossMethod();
      if(LOSSES_AASHTO_REFINED == lm || LOSSES_WSDOT_REFINED  == lm )
      {
         isapp = true;
      }
   }

   return isapp;
}

Int16 SpecLibraryEntry::GetLiveLoadDistributionMethod() const
{
   return m_LldfMethod;
}

void SpecLibraryEntry::SetLiveLoadDistributionMethod(Int16 method)
{
   PRECONDITION(method == LLDF_LRFD || method == LLDF_WSDOT  || method == LLDF_TXDOT);

   m_LldfMethod = method;
}

void SpecLibraryEntry::IgnoreSkewReductionForMoment(bool bIgnore)
{
   m_bIgnoreSkewReductionForMoment = bIgnore;
}

bool SpecLibraryEntry::IgnoreSkewReductionForMoment() const
{
   return m_bIgnoreSkewReductionForMoment;
}

Int16 SpecLibraryEntry::GetLongReinfShearMethod() const
{
   // WSDOT method has been recinded
   ATLASSERT(m_LongReinfShearMethod == LRFD_METHOD);
   return m_LongReinfShearMethod;
}

void SpecLibraryEntry::SetLongReinfShearMethod(Int16 method)
{
   // WSDOT method has been recinded
   PRECONDITION(method == LRFD_METHOD/* || method == WSDOT_METHOD*/);

   m_LongReinfShearMethod = method;
}

bool SpecLibraryEntry::GetDoEvaluateLLDeflection() const
{
   return m_bDoEvaluateDeflection;
}

void SpecLibraryEntry::SetDoEvaluateLLDeflection(bool doit) 
{
   m_bDoEvaluateDeflection = doit;
}

Float64 SpecLibraryEntry::GetLLDeflectionLimit() const
{
   return m_DeflectionLimit;
}

void SpecLibraryEntry::SetLLDeflectionLimit(Float64 limit)
{
   ATLASSERT(limit>0.0);
   m_DeflectionLimit = limit;
}

void SpecLibraryEntry::ConsiderReinforcementStrainLimitForMomentCapacity(bool bConsider)
{
   m_bConsiderReinforcementStrainLimit = bConsider;
}

bool SpecLibraryEntry::ConsiderReinforcementStrainLimitForMomentCapacity() const
{
   return m_bConsiderReinforcementStrainLimit;
}

void SpecLibraryEntry::SetSliceCountForMomentCapacity(IndexType nSlices)
{
   m_nMomentCapacitySlices = ForceIntoRange((IndexType)10, nSlices, (IndexType)100);
}

IndexType SpecLibraryEntry::GetSliceCountForMomentCapacity() const
{
   return m_nMomentCapacitySlices;
}

void SpecLibraryEntry::IncludeStrandForNegativeMoment(bool bInclude)
{
   m_bIncludeStrand_NegMoment = bInclude;
}

bool SpecLibraryEntry::IncludeStrandForNegativeMoment() const
{
   return m_bIncludeStrand_NegMoment;
}

void SpecLibraryEntry::IncludeRebarForMoment(bool bInclude)
{
   m_bIncludeRebar_Moment = bInclude;
}

bool SpecLibraryEntry::IncludeRebarForMoment() const
{
   return m_bIncludeRebar_Moment;
}

void SpecLibraryEntry::IncludeRebarForShear(bool bInclude)
{
   m_bIncludeRebar_Shear = bInclude;
}

bool SpecLibraryEntry::IncludeRebarForShear() const
{
   return m_bIncludeRebar_Shear;
}

pgsTypes::AnalysisType SpecLibraryEntry::GetAnalysisType() const
{
   return m_AnalysisType;
}

void SpecLibraryEntry::SetFlexureModulusOfRuptureCoefficient(pgsTypes::ConcreteType type,Float64 fr)
{
   ATLASSERT(!IsUHPC(type));
   m_FlexureModulusOfRuptureCoefficient[type] = fr;
}

Float64 SpecLibraryEntry::GetFlexureModulusOfRuptureCoefficient(pgsTypes::ConcreteType type) const
{
   ATLASSERT(!IsUHPC(type));
   return m_FlexureModulusOfRuptureCoefficient[type];
}

void SpecLibraryEntry::SetShearModulusOfRuptureCoefficient(pgsTypes::ConcreteType type,Float64 fr)
{
   ATLASSERT(!IsUHPC(type));
   m_ShearModulusOfRuptureCoefficient[type] = fr;
}

Float64 SpecLibraryEntry::GetShearModulusOfRuptureCoefficient(pgsTypes::ConcreteType type) const
{
   ATLASSERT(!IsUHPC(type));
   return m_ShearModulusOfRuptureCoefficient[type];
}

void SpecLibraryEntry::SetMaxSlabFc(pgsTypes::ConcreteType type,Float64 fc)
{
   ATLASSERT(!IsUHPC(type));
   m_MaxSlabFc[type] = fc;
}

Float64 SpecLibraryEntry::GetMaxSlabFc(pgsTypes::ConcreteType type) const
{
   ATLASSERT(!IsUHPC(type));
   return m_MaxSlabFc[type];
}

void SpecLibraryEntry::SetMaxSegmentFc(pgsTypes::ConcreteType type,Float64 fc)
{
   ATLASSERT(!IsUHPC(type));
   m_MaxSegmentFc[type] = fc;
}

Float64 SpecLibraryEntry::GetMaxSegmentFc(pgsTypes::ConcreteType type) const
{
   ATLASSERT(!IsUHPC(type));
   return m_MaxSegmentFc[type];
}

void SpecLibraryEntry::SetMaxSegmentFci(pgsTypes::ConcreteType type,Float64 fci)
{
   ATLASSERT(!IsUHPC(type));
   m_MaxSegmentFci[type] = fci;
}

Float64 SpecLibraryEntry::GetMaxSegmentFci(pgsTypes::ConcreteType type) const
{
   ATLASSERT(!IsUHPC(type));
   return m_MaxSegmentFci[type];
}

void SpecLibraryEntry::SetMaxClosureFc(pgsTypes::ConcreteType type,Float64 fc)
{
   ATLASSERT(!IsUHPC(type));
   m_MaxClosureFc[type] = fc;
}

Float64 SpecLibraryEntry::GetMaxClosureFc(pgsTypes::ConcreteType type) const
{
   ATLASSERT(!IsUHPC(type));
   return m_MaxClosureFc[type];
}

void SpecLibraryEntry::SetMaxClosureFci(pgsTypes::ConcreteType type,Float64 fci)
{
   ATLASSERT(!IsUHPC(type));
   m_MaxClosureFci[type] = fci;
}

Float64 SpecLibraryEntry::GetMaxClosureFci(pgsTypes::ConcreteType type) const
{
   ATLASSERT(!IsUHPC(type));
   return m_MaxClosureFci[type];
}

void SpecLibraryEntry::SetMaxConcreteUnitWeight(pgsTypes::ConcreteType type,Float64 wc)
{
   ATLASSERT(!IsUHPC(type));
   m_MaxConcreteUnitWeight[type] = wc;
}

Float64 SpecLibraryEntry::GetMaxConcreteUnitWeight(pgsTypes::ConcreteType type) const
{
   ATLASSERT(!IsUHPC(type));
   return m_MaxConcreteUnitWeight[type];
}

void SpecLibraryEntry::SetMaxConcreteAggSize(pgsTypes::ConcreteType type,Float64 agg)
{
   ATLASSERT(!IsUHPC(type));
   m_MaxConcreteAggSize[type] = agg;
}

Float64 SpecLibraryEntry::GetMaxConcreteAggSize(pgsTypes::ConcreteType type) const
{
   ATLASSERT(!IsUHPC(type));
   return m_MaxConcreteAggSize[type];
}

void SpecLibraryEntry::SetDoCheckStirrupSpacingCompatibility(bool doCheck)
{
   m_DoCheckStirrupSpacingCompatibility = doCheck;
}

bool SpecLibraryEntry::GetDoCheckStirrupSpacingCompatibility() const
{
   return m_DoCheckStirrupSpacingCompatibility;
}

bool SpecLibraryEntry::UpdateLoadFactors() const
{
   return m_bUpdateLoadFactors;
}

void SpecLibraryEntry::GetDCLoadFactors(pgsTypes::LimitState ls,Float64* pDCmin,Float64* pDCmax) const
{
   *pDCmin = m_DCmin[ls];
   *pDCmax = m_DCmax[ls];
}

void SpecLibraryEntry::GetDWLoadFactors(pgsTypes::LimitState ls,Float64* pDWmin,Float64* pDWmax) const
{
   *pDWmin = m_DWmin[ls];
   *pDWmax = m_DWmax[ls];
}

void SpecLibraryEntry::GetLLIMLoadFactors(pgsTypes::LimitState ls,Float64* pLLIMmin,Float64* pLLIMmax) const
{
   *pLLIMmin = m_LLIMmin[ls];
   *pLLIMmax = m_LLIMmax[ls];
}

void SpecLibraryEntry::SetDCLoadFactors(pgsTypes::LimitState ls,Float64 DCmin,Float64 DCmax)
{
   m_DCmin[ls] = DCmin;
   m_DCmax[ls] = DCmax;
}

void SpecLibraryEntry::SetDWLoadFactors(pgsTypes::LimitState ls,Float64 DWmin,Float64 DWmax)
{
   m_DWmin[ls] = DWmin;
   m_DWmax[ls] = DWmax;
}

void SpecLibraryEntry::SetLLIMLoadFactors(pgsTypes::LimitState ls,Float64 LLIMmin,Float64 LLIMmax)
{
   m_LLIMmin[ls] = LLIMmin;
   m_LLIMmax[ls] = LLIMmax;
}

void SpecLibraryEntry::EnableSlabOffsetCheck(bool enable)
{
   m_EnableSlabOffsetCheck = enable;
}

bool SpecLibraryEntry::IsSlabOffsetCheckEnabled() const
{
   return m_EnableSlabOffsetCheck;
}

void SpecLibraryEntry::EnableSlabOffsetDesign(bool enable)
{
   m_EnableSlabOffsetDesign = enable;
}

bool SpecLibraryEntry::IsSlabOffsetDesignEnabled() const
{
   if (m_EnableSlabOffsetCheck)
   {
      return m_EnableSlabOffsetDesign;
   }
   else
   {
      ATLASSERT(!m_EnableSlabOffsetDesign); // design should not be enabled if check is not
      return false;
   }
}

void SpecLibraryEntry::SetDesignStrandFillType(arDesignStrandFillType type)
{
   m_DesignStrandFillType = type;
}

arDesignStrandFillType SpecLibraryEntry::GetDesignStrandFillType() const
{
   return m_DesignStrandFillType;
}

void SpecLibraryEntry::SetEffectiveFlangeWidthMethod(pgsTypes::EffectiveFlangeWidthMethod efwMethod)
{
   m_EffFlangeWidthMethod = efwMethod;
}

pgsTypes::EffectiveFlangeWidthMethod SpecLibraryEntry::GetEffectiveFlangeWidthMethod() const
{
   return m_EffFlangeWidthMethod;
}

void SpecLibraryEntry::SetShearFlowMethod(pgsTypes::ShearFlowMethod method)
{
   m_ShearFlowMethod = method;
}

pgsTypes::ShearFlowMethod SpecLibraryEntry::GetShearFlowMethod() const
{
   return m_ShearFlowMethod;
}

Float64 SpecLibraryEntry::GetMaxInterfaceShearConnectorSpacing() const
{
   return m_MaxInterfaceShearConnectorSpacing;
}

void SpecLibraryEntry::SetMaxInterfaceShearConnectionSpacing(Float64 sMax)
{
   m_MaxInterfaceShearConnectorSpacing = sMax;
}

void SpecLibraryEntry::UseDeckWeightForPermanentNetCompressiveForce(bool bUse)
{
   m_bUseDeckWeightForPc = bUse;
}

bool SpecLibraryEntry::UseDeckWeightForPermanentNetCompressiveForce() const
{
   return m_bUseDeckWeightForPc;
}

void SpecLibraryEntry::SetShearCapacityMethod(pgsTypes::ShearCapacityMethod method)
{
   m_ShearCapacityMethod = method;
}

pgsTypes::ShearCapacityMethod SpecLibraryEntry::GetShearCapacityMethod() const
{
   return m_ShearCapacityMethod;
}

void SpecLibraryEntry::LimitNetTensionStrainToPositiveValues(bool bLimit)
{
   m_bLimitNetTensionStrainToPositiveValues = bLimit;
}

bool SpecLibraryEntry::LimitNetTensionStrainToPositiveValues() const
{
   return m_bLimitNetTensionStrainToPositiveValues;
}

void SpecLibraryEntry::SetCuringMethodTimeAdjustmentFactor(Float64 f)
{
   m_CuringMethodTimeAdjustmentFactor = f;
}

Float64 SpecLibraryEntry::GetCuringMethodTimeAdjustmentFactor() const
{
   return m_CuringMethodTimeAdjustmentFactor;
}

void SpecLibraryEntry::SetMininumTruckSupportLocation(Float64 x)
{
   m_MinHaulPoint = x;
}

Float64 SpecLibraryEntry::GetMininumTruckSupportLocation() const
{
   return m_MinHaulPoint;
}

void SpecLibraryEntry::SetTruckSupportLocationAccuracy(Float64 x)
{
   m_HaulPointAccuracy = x;
}

Float64 SpecLibraryEntry::GetTruckSupportLocationAccuracy() const
{
   return m_HaulPointAccuracy;
}

void SpecLibraryEntry::SetLimitStateConcreteStrength(pgsTypes::LimitStateConcreteStrength lsFc)
{
   m_LimitStateConcreteStrength = lsFc;
}

pgsTypes::LimitStateConcreteStrength SpecLibraryEntry::GetLimitStateConcreteStrength() const
{
   return m_LimitStateConcreteStrength;
}

void SpecLibraryEntry::Use90DayStrengthForSlowCuringConcrete(bool bUse, Float64 factor)
{
   m_bUse90DayConcreteStrength = bUse;
   m_90DayConcreteStrengthFactor = factor;
}

void SpecLibraryEntry::Use90DayStrengthForSlowCuringConcrete(bool* pbUse, Float64* pfactor) const
{
   *pbUse = m_bUse90DayConcreteStrength;
   *pfactor = m_90DayConcreteStrengthFactor;
}

void SpecLibraryEntry::SetUseMinTruckSupportLocationFactor(bool factor)
{
   m_UseMinTruckSupportLocationFactor = factor;
}

bool SpecLibraryEntry::GetUseMinTruckSupportLocationFactor() const
{
   return m_UseMinTruckSupportLocationFactor;
}

void SpecLibraryEntry::SetMinTruckSupportLocationFactor(Float64 factor)
{
   m_MinTruckSupportLocationFactor = factor;
}

Float64 SpecLibraryEntry::GetMinTruckSupportLocationFactor() const
{
   return m_MinTruckSupportLocationFactor;
}

void SpecLibraryEntry::SetOverhangGFactor(Float64 factor)
{
   m_OverhangGFactor = factor;
}

Float64 SpecLibraryEntry::GetOverhangGFactor() const
{
   return m_OverhangGFactor;
}

void SpecLibraryEntry::SetInteriorGFactor(Float64 factor)
{
   m_InteriorGFactor = factor;
}

Float64 SpecLibraryEntry::GetInteriorGFactor() const
{
   return m_InteriorGFactor;
}

void SpecLibraryEntry::SetMininumLiftingPointLocation(Float64 x)
{
   m_MinLiftPoint = x;
}

Float64 SpecLibraryEntry::GetMininumLiftingPointLocation() const
{
   return m_MinLiftPoint;
}

void SpecLibraryEntry::SetLiftingPointLocationAccuracy(Float64 x)
{
   m_LiftPointAccuracy = x;
}

Float64 SpecLibraryEntry::GetLiftingPointLocationAccuracy() const
{
   return m_LiftPointAccuracy;
}

void SpecLibraryEntry::SetPedestrianLiveLoad(Float64 w)
{
   m_PedestrianLoad = w;
}

Float64 SpecLibraryEntry::GetPedestrianLiveLoad() const
{
   return m_PedestrianLoad;
}

void SpecLibraryEntry::SetMinSidewalkWidth(Float64 Wmin)
{
   m_MinSidewalkWidth = Wmin;
}

Float64 SpecLibraryEntry::GetMinSidewalkWidth() const
{
   return m_MinSidewalkWidth;
}

void SpecLibraryEntry::SetMaxAngularDeviationBetweenGirders(Float64 angle)
{
   m_MaxAngularDeviationBetweenGirders = angle;
}

Float64 SpecLibraryEntry::GetMaxAngularDeviationBetweenGirders() const
{
   return m_MaxAngularDeviationBetweenGirders;
}

void SpecLibraryEntry::SetMinGirderStiffnessRatio(Float64 r)
{
   m_MinGirderStiffnessRatio = r;
}

Float64 SpecLibraryEntry::GetMinGirderStiffnessRatio() const
{
   return m_MinGirderStiffnessRatio;
}

void SpecLibraryEntry::SetLLDFGirderSpacingLocation(Float64 fra)
{
   m_LLDFGirderSpacingLocation = fra;
}

Float64 SpecLibraryEntry::GetLLDFGirderSpacingLocation() const
{
   return m_LLDFGirderSpacingLocation;
}

void SpecLibraryEntry::IncludeDualTandem(bool bInclude)
{
   m_bIncludeDualTandem = bInclude;
}

bool SpecLibraryEntry::IncludeDualTandem() const
{
   return m_bIncludeDualTandem;
}

void SpecLibraryEntry::UseRigidMethod(bool bUseRigidMethod)
{
   m_bUseRigidMethod = bUseRigidMethod;
}

bool SpecLibraryEntry::UseRigidMethod() const
{
   return m_bUseRigidMethod;
}

void SpecLibraryEntry::LimitDistributionFactorsToLanesBeams(bool bLimit)
{
   m_LimitDistributionFactorsToLanesBeams = bLimit;
}

bool SpecLibraryEntry::LimitDistributionFactorsToLanesBeams() const
{
   return m_LimitDistributionFactorsToLanesBeams;
}

void SpecLibraryEntry::SetExteriorLiveLoadDistributionGTAdjacentInteriorRule(bool bValue)
{
   m_ExteriorLiveLoadDistributionGTAdjacentInteriorRule = bValue;
}

bool SpecLibraryEntry::GetExteriorLiveLoadDistributionGTAdjacentInteriorRule() const
{
   return m_ExteriorLiveLoadDistributionGTAdjacentInteriorRule;
}

pgsTypes::PrestressTransferComputationType SpecLibraryEntry::GetPrestressTransferComputationType() const
{
   return m_PrestressTransferComputationType;
}

void SpecLibraryEntry::SetPrestressTransferComputationType(pgsTypes::PrestressTransferComputationType type)
{
   m_PrestressTransferComputationType = type;
}

void SpecLibraryEntry::GetDuctAreaRatio(Float64* pPush,Float64* pPull) const
{
   *pPush = m_DuctAreaPushRatio;
   *pPull = m_DuctAreaPullRatio;
}

void SpecLibraryEntry::SetDuctAreaRatio(Float64 push,Float64 pull)
{
   m_DuctAreaPushRatio = push;
   m_DuctAreaPullRatio = pull;
}

Float64 SpecLibraryEntry::GetDuctDiameterRatio() const
{
   return m_DuctDiameterRatio;
}

void SpecLibraryEntry::SetDuctDiameterRatio(Float64 dr)
{
   m_DuctDiameterRatio = dr;
}

void SpecLibraryEntry::SetFlexureResistanceFactors(pgsTypes::ConcreteType type,Float64 phiTensionPS,Float64 phiTensionRC,Float64 phiTensionSpliced,Float64 phiCompression)
{
   ATLASSERT(type != pgsTypes::UHPC); // the values for UHPC are just placeholders at this time... there isn't a UI to modify them and they aren't used in the main program
   m_PhiFlexureTensionPS[type]      = phiTensionPS;
   m_PhiFlexureTensionRC[type]      = phiTensionRC;
   m_PhiFlexureTensionSpliced[type] = phiTensionSpliced;
   m_PhiFlexureCompression[type]    = phiCompression;
}

void SpecLibraryEntry::GetFlexureResistanceFactors(pgsTypes::ConcreteType type,Float64* phiTensionPS,Float64* phiTensionRC,Float64* phiTensionSpliced,Float64* phiCompression) const
{
   ATLASSERT(type != pgsTypes::UHPC); // the values for UHPC are just placeholders at this time... there isn't a UI to modify them and they aren't used in the main program
   *phiTensionPS      = m_PhiFlexureTensionPS[type];
   *phiTensionRC      = m_PhiFlexureTensionRC[type];
   *phiTensionSpliced = m_PhiFlexureTensionSpliced[type];
   *phiCompression    = m_PhiFlexureCompression[type];
}

void SpecLibraryEntry::SetClosureJointFlexureResistanceFactor(pgsTypes::ConcreteType type,Float64 phi)
{
   m_PhiClosureJointFlexure[type] = phi;
}

Float64 SpecLibraryEntry::GetClosureJointFlexureResistanceFactor(pgsTypes::ConcreteType type) const
{
   return m_PhiClosureJointFlexure[type];
}

void SpecLibraryEntry::SetShearResistanceFactor(bool isDebonded, pgsTypes::ConcreteType type,Float64 phi)
{
   if (isDebonded)
   {
      ATLASSERT(lrfdVersionMgr::EighthEdition2017 <= GetSpecificationType());
      m_PhiShearDebonded[type] = phi;
   }
   else
   {
      m_PhiShear[type] = phi;
   }
}

Float64 SpecLibraryEntry::GetShearResistanceFactor(bool isDebonded, pgsTypes::ConcreteType type) const
{
   if (isDebonded && lrfdVersionMgr::EighthEdition2017 <= GetSpecificationType() )
   {
      return m_PhiShearDebonded[type];
   }
   else
   {
      return m_PhiShear[type];
   }
}

void SpecLibraryEntry::SetClosureJointShearResistanceFactor(pgsTypes::ConcreteType type,Float64 phi)
{
   m_PhiClosureJointShear[type] = phi;
}

Float64 SpecLibraryEntry::GetClosureJointShearResistanceFactor(pgsTypes::ConcreteType type) const
{
   return m_PhiClosureJointShear[type];
}

void SpecLibraryEntry::IncludeNoncompositeMomentsForNegMomentDesign(bool bInclude)
{
   m_bIncludeForNegMoment = bInclude;
}

bool SpecLibraryEntry::IncludeNoncompositeMomentsForNegMomentDesign() const
{
   return m_bIncludeForNegMoment;
}

void SpecLibraryEntry::AllowStraightStrandExtensions(bool bAllow)
{
   m_bAllowStraightStrandExtensions = bAllow;
}

bool SpecLibraryEntry::AllowStraightStrandExtensions() const
{
   return m_bAllowStraightStrandExtensions;
}

Float64 SpecLibraryEntry::GetAtStressingCompressingStressFactor() const
{
   return m_ClosureCompStressAtStressing;
}

void SpecLibraryEntry::SetAtStressingCompressionStressFactor(Float64 stress)
{
   m_ClosureCompStressAtStressing = stress;
}

Float64 SpecLibraryEntry::GetAtStressingPrecompressedTensileZoneTensionStressFactor() const
{
   return m_ClosureTensStressPTZAtStressing;
}

void SpecLibraryEntry::SetAtStressingPrecompressedTensileZoneTensionStressFactor(Float64 stress)
{
   m_ClosureTensStressPTZAtStressing = stress;
}

Float64 SpecLibraryEntry::GetAtStressingPrecompressedTensileZoneTensionStressFactorWithRebar() const
{
   return m_ClosureTensStressPTZWithRebarAtStressing;
}

void SpecLibraryEntry::SetAtStressingPrecompressedTensileZoneTensionStressFactorWithRebar(Float64 stress)
{
   m_ClosureTensStressPTZWithRebarAtStressing = stress;
}

Float64 SpecLibraryEntry::GetAtStressingOtherLocationTensionStressFactor() const
{
   return m_ClosureTensStressAtStressing;
}

void SpecLibraryEntry::SetAtStressingOtherLocationTensileZoneTensionStressFactor(Float64 stress)
{
   m_ClosureTensStressAtStressing = stress;
}

Float64 SpecLibraryEntry::GetAtStressingOtherLocationTensionStressFactorWithRebar() const
{
   return m_ClosureTensStressWithRebarAtStressing;
}

void SpecLibraryEntry::SetAtStressingOtherLocationTensionStressFactorWithRebar(Float64 stress)
{
   m_ClosureTensStressWithRebarAtStressing = stress;
}

Float64 SpecLibraryEntry::GetAtServiceCompressingStressFactor() const
{
   return m_ClosureCompStressAtService;
}

void SpecLibraryEntry::SetAtServiceCompressionStressFactor(Float64 stress)
{
   m_ClosureCompStressAtService = stress;
}

Float64 SpecLibraryEntry::GetAtServiceWithLiveLoadCompressingStressFactor() const
{
   return m_ClosureCompStressWithLiveLoadAtService;
}

void SpecLibraryEntry::SetAtServiceWithLiveLoadCompressionStressFactor(Float64 stress)
{
   m_ClosureCompStressWithLiveLoadAtService = stress;
}

Float64 SpecLibraryEntry::GetAtServicePrecompressedTensileZoneTensionStressFactor() const
{
   return m_ClosureTensStressPTZAtService;
}

void SpecLibraryEntry::SetAtServicePrecompressedTensileZoneTensionStressFactor(Float64 stress)
{
   m_ClosureTensStressPTZAtService = stress;
}

Float64 SpecLibraryEntry::GetAtServicePrecompressedTensileZoneTensionStressFactorWithRebar() const
{
   return m_ClosureTensStressPTZWithRebarAtService;
}

void SpecLibraryEntry::SetAtServicePrecompressedTensileZoneTensionStressFactorWithRebar(Float64 stress)
{
   m_ClosureTensStressPTZWithRebarAtService = stress;
}

Float64 SpecLibraryEntry::GetAtServiceOtherLocationTensionStressFactor() const
{
   return m_ClosureTensStressAtService;
}

void SpecLibraryEntry::SetAtServiceOtherLocationTensileZoneTensionStressFactor(Float64 stress)
{
   m_ClosureTensStressAtService = stress;
}

Float64 SpecLibraryEntry::GetAtServiceOtherLocationTensionStressFactorWithRebar() const
{
   return m_ClosureTensStressWithRebarAtService;
}

void SpecLibraryEntry::SetAtServiceOtherLocationTensionStressFactorWithRebar(Float64 stress)
{
   m_ClosureTensStressWithRebarAtService = stress;
}

Float64 SpecLibraryEntry::GetClosureFatigueCompressionStressFactor() const
{
   return m_ClosureCompStressFatigue;
}

void SpecLibraryEntry::SetClosureFatigueCompressionStressFactor(Float64 stress)
{
   m_ClosureCompStressFatigue = stress;
}

void SpecLibraryEntry::CheckBottomFlangeClearance(bool bCheck)
{
   m_bCheckBottomFlangeClearance = bCheck;
}

bool SpecLibraryEntry::CheckBottomFlangeClearance() const
{
   return m_bCheckBottomFlangeClearance;
}

void SpecLibraryEntry::SetMinBottomFlangeClearance(Float64 Cmin)
{
   m_Cmin = Cmin;
}

Float64 SpecLibraryEntry::GetMinBottomFlangeClearance() const
{
   return m_Cmin;
}

void SpecLibraryEntry::CheckGirderInclination(bool bCheck)
{
   m_bCheckGirderInclination = bCheck;
}

bool SpecLibraryEntry::CheckGirderInclination() const
{
   return m_bCheckGirderInclination;
}

void SpecLibraryEntry::SetGirderInclinationFactorOfSafety(Float64 fs)
{
   m_InclinedGirder_FSmax = fs;
}

Float64 SpecLibraryEntry::GetGirderInclinationFactorOfSafety() const
{
   return m_InclinedGirder_FSmax;
}

void SpecLibraryEntry::SetRequiredSlabOffsetRoundingParameters(pgsTypes::SlabOffsetRoundingMethod method, Float64 tolerance)
{
   m_SlabOffsetRoundingMethod = method;
   m_SlabOffsetRoundingTolerance = tolerance;
}

void SpecLibraryEntry::GetRequiredSlabOffsetRoundingParameters(pgsTypes::SlabOffsetRoundingMethod * pMethod, Float64 * pTolerance) const
{
   *pMethod = m_SlabOffsetRoundingMethod;
   *pTolerance = m_SlabOffsetRoundingTolerance;
}

void SpecLibraryEntry::SetFinishedElevationTolerance(Float64 tol)
{
   m_FinishedElevationTolerance = tol;
}

Float64 SpecLibraryEntry::GetFinishedElevationTolerance() const
{
   return m_FinishedElevationTolerance;
}

void SpecLibraryEntry::AlertTaperedSolePlateRequirement(bool bAlert)
{
   m_bAlertTaperedSolePlateRequirement = bAlert;
}

bool SpecLibraryEntry::AlertTaperedSolePlateRequirement() const
{
   return m_bAlertTaperedSolePlateRequirement;
}

void SpecLibraryEntry::SetTaperedSolePlateInclinationThreshold(Float64 threshold)
{
   m_TaperedSolePlateInclinationThreshold = threshold;
}

Float64 SpecLibraryEntry::GetTaperedSolePlateInclinationThreshold() const
{
   return m_TaperedSolePlateInclinationThreshold;
}

void SpecLibraryEntry::UseImpactForBearingReactions(bool bUse)
{
   m_bUseImpactForBearingReactions = bUse;
}

bool SpecLibraryEntry::UseImpactForBearingReactions() const
{
   return m_bUseImpactForBearingReactions;
}

//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void SpecLibraryEntry::MakeCopy(const SpecLibraryEntry& rOther)
{
   m_bUseCurrentSpecification = rOther.m_bUseCurrentSpecification;
   m_SpecificationType          = rOther.m_SpecificationType;
   m_SpecificationUnits         = rOther.m_SpecificationUnits;
   m_Description.erase();
   m_Description                = rOther.m_Description;
   m_SectionPropertyMode        = rOther.m_SectionPropertyMode;
   m_DoCheckStrandSlope         = rOther.m_DoCheckStrandSlope;
   m_DoDesignStrandSlope        = rOther.m_DoDesignStrandSlope;
   m_MaxSlope05                 = rOther.m_MaxSlope05;
   m_MaxSlope06                 = rOther.m_MaxSlope06;
   m_MaxSlope07                 = rOther.m_MaxSlope07;
   m_DoCheckHoldDown            = rOther.m_DoCheckHoldDown;
   m_DoDesignHoldDown           = rOther.m_DoDesignHoldDown;
   m_HoldDownForceType = rOther.m_HoldDownForceType;
   m_HoldDownForce              = rOther.m_HoldDownForce;
   m_HoldDownFriction = rOther.m_HoldDownFriction;
   m_bCheckHandlingWeightLimit = rOther.m_bCheckHandlingWeightLimit;
   m_HandlingWeightLimit = rOther.m_HandlingWeightLimit;
   m_StirrupSpacingCoefficient[0] = rOther.m_StirrupSpacingCoefficient[0];
   m_MaxStirrupSpacing[0]         = rOther.m_MaxStirrupSpacing[0];
   m_StirrupSpacingCoefficient[1] = rOther.m_StirrupSpacingCoefficient[1];
   m_MaxStirrupSpacing[1]         = rOther.m_MaxStirrupSpacing[1];
   m_DoCheckConfinement         = rOther.m_DoCheckConfinement;
   m_DoDesignConfinement        = rOther.m_DoDesignConfinement;
   m_DoCheckSplitting           = rOther.m_DoCheckSplitting;
   m_DoDesignSplitting          = rOther.m_DoDesignSplitting;
   m_CyLiftingCrackFs           = rOther.m_CyLiftingCrackFs;
   m_CyLiftingFailFs            = rOther.m_CyLiftingFailFs;
   m_CyCompStressServ           = rOther.m_CyCompStressServ;
   m_LiftingCompressionStressCoefficient_GlobalStress        = rOther.m_LiftingCompressionStressCoefficient_GlobalStress;
   m_LiftingCompressionStressCoefficient_PeakStress = rOther.m_LiftingCompressionStressCoefficient_PeakStress;
   m_CyTensStressServ           = rOther.m_CyTensStressServ;
   m_CyDoTensStressServMax      = rOther.m_CyDoTensStressServMax;
   m_CyTensStressServMax        = rOther.m_CyTensStressServMax;
   m_CyTensStressLifting        = rOther.m_CyTensStressLifting;
   m_CyDoTensStressLiftingMax   = rOther.m_CyDoTensStressLiftingMax;
   m_CyTensStressLiftingMax     = rOther.m_CyTensStressLiftingMax;
   m_SplittingZoneLengthFactor   = rOther.m_SplittingZoneLengthFactor;
   m_LiftingUpwardImpact        = rOther.m_LiftingUpwardImpact;
   m_LiftingDownwardImpact      = rOther.m_LiftingDownwardImpact;
   m_HaulingUpwardImpact        = rOther.m_HaulingUpwardImpact;
   m_HaulingDownwardImpact      = rOther.m_HaulingDownwardImpact;
   m_CuringMethod               = rOther.m_CuringMethod;
   m_EnableLiftingCheck         = rOther.m_EnableLiftingCheck;
   m_EnableLiftingDesign        = rOther.m_EnableLiftingDesign;
   m_PickPointHeight            = rOther.m_PickPointHeight;
   m_LiftingLoopTolerance       = rOther.m_LiftingLoopTolerance;
   m_MinCableInclination        = rOther.m_MinCableInclination;
   m_MaxGirderSweepLifting      = rOther.m_MaxGirderSweepLifting;
   m_EnableHaulingCheck         = rOther.m_EnableHaulingCheck;
   m_EnableHaulingDesign        = rOther.m_EnableHaulingDesign;
   m_HaulingAnalysisMethod      = rOther.m_HaulingAnalysisMethod;
   m_MaxGirderSweepHauling      = rOther.m_MaxGirderSweepHauling;
   m_HaulingSweepGrowth = rOther.m_HaulingSweepGrowth;
   m_HaulingSupportPlacementTolerance = rOther.m_HaulingSupportPlacementTolerance;
   m_HaulingCamberMultiplier = rOther.m_HaulingCamberMultiplier;
   m_LiftingCamberMultiplier = rOther.m_LiftingCamberMultiplier;

   m_LiftingWindType = rOther.m_LiftingWindType;
   m_LiftingWindLoad = rOther.m_LiftingWindLoad;
   m_HaulingWindType = rOther.m_HaulingWindType;
   m_HaulingWindLoad = rOther.m_HaulingWindLoad;
   m_CentrifugalForceType = rOther.m_CentrifugalForceType;
   m_HaulingSpeed = rOther.m_HaulingSpeed;
   m_TurningRadius = rOther.m_TurningRadius;

   m_GlobalCompStressHauling          = rOther.m_GlobalCompStressHauling;
   m_PeakCompStressHauling = rOther.m_PeakCompStressHauling;
   m_TensStressHauling = rOther.m_TensStressHauling;
   m_DoTensStressHaulingMax = rOther.m_DoTensStressHaulingMax;
   m_TensStressHaulingMax = rOther.m_TensStressHaulingMax;
   m_HaulingCrackFs           = rOther.m_HaulingCrackFs;
   m_HaulingRollFs            = rOther.m_HaulingRollFs;

   m_HaulingModulusOfRuptureCoefficient = rOther.m_HaulingModulusOfRuptureCoefficient;

   m_LiftingModulusOfRuptureCoefficient = rOther.m_LiftingModulusOfRuptureCoefficient;

   m_CyTensStressServWithRebar  = rOther.m_CyTensStressServWithRebar;
   m_TensStressLiftingWithRebar = rOther.m_TensStressLiftingWithRebar;
   m_TensStressHaulingWithRebar = rOther.m_TensStressHaulingWithRebar;

   m_HaulingImpactUsage         = rOther.m_HaulingImpactUsage;
   m_RoadwayCrownSlope          = rOther.m_RoadwayCrownSlope;
   m_RoadwaySuperelevation      = rOther.m_RoadwaySuperelevation;

   m_LimitStateConcreteStrength = rOther.m_LimitStateConcreteStrength;
   m_bUse90DayConcreteStrength = rOther.m_bUse90DayConcreteStrength;
   m_90DayConcreteStrengthFactor = rOther.m_90DayConcreteStrengthFactor;

   m_TempStrandRemovalCompStress              = rOther.m_TempStrandRemovalCompStress;
   m_TempStrandRemovalTensStress              = rOther.m_TempStrandRemovalTensStress;
   m_TempStrandRemovalDoTensStressMax         = rOther.m_TempStrandRemovalDoTensStressMax;
   m_TempStrandRemovalTensStressMax           = rOther.m_TempStrandRemovalTensStressMax;
   m_TempStrandRemovalTensStressWithRebar     = rOther.m_TempStrandRemovalTensStressWithRebar;

   m_bCheckTemporaryStresses    = rOther.m_bCheckTemporaryStresses;
   m_Bs1CompStress              = rOther.m_Bs1CompStress;
   m_Bs1TensStress              = rOther.m_Bs1TensStress;
   m_Bs1DoTensStressMax         = rOther.m_Bs1DoTensStressMax;
   m_Bs1TensStressMax           = rOther.m_Bs1TensStressMax;
   m_Bs2CompStress              = rOther.m_Bs2CompStress;
   m_bCheckBs2Tension           = rOther.m_bCheckBs2Tension;
   m_Bs2TensStress              = rOther.m_Bs2TensStress;
   m_Bs2DoTensStressMax         = rOther.m_Bs2DoTensStressMax;
   m_Bs2TensStressMax           = rOther.m_Bs2TensStressMax;
   m_TrafficBarrierDistributionType = rOther.m_TrafficBarrierDistributionType;
   m_Bs2MaxGirdersTrafficBarrier= rOther.m_Bs2MaxGirdersTrafficBarrier;
   m_OverlayLoadDistribution    = rOther.m_OverlayLoadDistribution;
   m_HaunchLoadComputationType    = rOther.m_HaunchLoadComputationType;
   m_HaunchLoadCamberTolerance    = rOther.m_HaunchLoadCamberTolerance;
   m_HaunchLoadCamberFactor     = rOther.m_HaunchLoadCamberFactor;
   m_HaunchAnalysisSectionPropertiesType = rOther.m_HaunchAnalysisSectionPropertiesType;
   m_Bs3CompStressServ          = rOther.m_Bs3CompStressServ;
   m_Bs3CompStressService1A     = rOther.m_Bs3CompStressService1A;
   m_Bs3TensStressServNc        = rOther.m_Bs3TensStressServNc;
   m_Bs3DoTensStressServNcMax   = rOther.m_Bs3DoTensStressServNcMax;
   m_Bs3TensStressServNcMax     = rOther.m_Bs3TensStressServNcMax;
   m_Bs3TensStressServSc        = rOther.m_Bs3TensStressServSc;
   m_Bs3DoTensStressServScMax   = rOther.m_Bs3DoTensStressServScMax;
   m_Bs3TensStressServScMax     = rOther.m_Bs3TensStressServScMax;
   m_Bs3IgnoreRangeOfApplicability = rOther.m_Bs3IgnoreRangeOfApplicability;
   m_Bs3LRFDOverReinforcedMomentCapacity = rOther.m_Bs3LRFDOverReinforcedMomentCapacity;

   m_PrincipalTensileStressMethod = rOther.m_PrincipalTensileStressMethod;
   m_PrincipalTensileStressCoefficient = rOther.m_PrincipalTensileStressCoefficient;
   m_PrincipalTensileStressTendonNearnessFactor = rOther.m_PrincipalTensileStressTendonNearnessFactor;
   m_PrincipalTensileStressUngroutedMultiplier = rOther.m_PrincipalTensileStressUngroutedMultiplier;
   m_PrincipalTensileStressGroutedMultiplier = rOther.m_PrincipalTensileStressGroutedMultiplier;

   m_PrincipalTensileStressFcThreshold = rOther.m_PrincipalTensileStressFcThreshold;

   m_FlexureModulusOfRuptureCoefficient = rOther.m_FlexureModulusOfRuptureCoefficient;
   m_ShearModulusOfRuptureCoefficient = rOther.m_ShearModulusOfRuptureCoefficient;

   m_CreepMethod                = rOther.m_CreepMethod;
   m_XferTime                   = rOther.m_XferTime;
   m_CreepFactor                = rOther.m_CreepFactor;
   m_CreepDuration1Min          = rOther.m_CreepDuration1Min;
   m_CreepDuration1Max          = rOther.m_CreepDuration1Max;
   m_CreepDuration2Min          = rOther.m_CreepDuration2Min;
   m_CreepDuration2Max          = rOther.m_CreepDuration2Max;
   m_TotalCreepDuration  = rOther.m_TotalCreepDuration;
   m_CamberVariability   = rOther.m_CamberVariability;

   m_LossMethod                 = rOther.m_LossMethod;
   m_TimeDependentModel         = rOther.m_TimeDependentModel;
   m_FinalLosses                = rOther.m_FinalLosses;
   m_ShippingLosses             = rOther.m_ShippingLosses;
   m_BeforeXferLosses           = rOther.m_BeforeXferLosses;
   m_AfterXferLosses            = rOther.m_AfterXferLosses;
   m_LiftingLosses              = rOther.m_LiftingLosses;
   m_ShippingTime               = rOther.m_ShippingTime;
   m_BeforeTempStrandRemovalLosses = rOther.m_BeforeTempStrandRemovalLosses;
   m_AfterTempStrandRemovalLosses  = rOther.m_AfterTempStrandRemovalLosses;
   m_AfterDeckPlacementLosses      = rOther.m_AfterDeckPlacementLosses;
   m_AfterSIDLLosses               = rOther.m_AfterSIDLLosses;

   m_bUpdatePTParameters = rOther.m_bUpdatePTParameters;
   m_Dset = rOther.m_Dset;
   m_WobbleFriction = rOther.m_WobbleFriction;
   m_FrictionCoefficient = rOther.m_FrictionCoefficient;

   m_SlabElasticGain          = rOther.m_SlabElasticGain;
   m_SlabPadElasticGain       = rOther.m_SlabPadElasticGain;
   m_DiaphragmElasticGain     = rOther.m_DiaphragmElasticGain;
   m_UserDCElasticGainBS1     = rOther.m_UserDCElasticGainBS1;
   m_UserDWElasticGainBS1     = rOther.m_UserDWElasticGainBS1;
   m_UserDCElasticGainBS2     = rOther.m_UserDCElasticGainBS2;
   m_UserDWElasticGainBS2     = rOther.m_UserDWElasticGainBS2;
   m_RailingSystemElasticGain = rOther.m_RailingSystemElasticGain;
   m_OverlayElasticGain       = rOther.m_OverlayElasticGain;
   m_SlabShrinkageElasticGain = rOther.m_SlabShrinkageElasticGain;
   m_LiveLoadElasticGain      = rOther.m_LiveLoadElasticGain;

   m_LldfMethod                 = rOther.m_LldfMethod;
   m_bIgnoreSkewReductionForMoment = rOther.m_bIgnoreSkewReductionForMoment;
   m_bUseRigidMethod = rOther.m_bUseRigidMethod;
   m_LongReinfShearMethod       = rOther.m_LongReinfShearMethod;

   m_bCheckStrandStress[CSS_AT_JACKING]       = rOther.m_bCheckStrandStress[CSS_AT_JACKING];
   m_bCheckStrandStress[CSS_BEFORE_TRANSFER]  = rOther.m_bCheckStrandStress[CSS_BEFORE_TRANSFER];
   m_bCheckStrandStress[CSS_AFTER_TRANSFER]   = rOther.m_bCheckStrandStress[CSS_AFTER_TRANSFER];
   m_bCheckStrandStress[CSS_AFTER_ALL_LOSSES] = rOther.m_bCheckStrandStress[CSS_AFTER_ALL_LOSSES];

   m_StrandStressCoeff[CSS_AT_JACKING][STRESS_REL]       = rOther.m_StrandStressCoeff[CSS_AT_JACKING][STRESS_REL];
   m_StrandStressCoeff[CSS_AT_JACKING][LOW_RELAX]        = rOther.m_StrandStressCoeff[CSS_AT_JACKING][LOW_RELAX];
   m_StrandStressCoeff[CSS_BEFORE_TRANSFER][STRESS_REL]  = rOther.m_StrandStressCoeff[CSS_BEFORE_TRANSFER][STRESS_REL];
   m_StrandStressCoeff[CSS_BEFORE_TRANSFER][LOW_RELAX]   = rOther.m_StrandStressCoeff[CSS_BEFORE_TRANSFER][LOW_RELAX];
   m_StrandStressCoeff[CSS_AFTER_TRANSFER][STRESS_REL]   = rOther.m_StrandStressCoeff[CSS_AFTER_TRANSFER][STRESS_REL];
   m_StrandStressCoeff[CSS_AFTER_TRANSFER][LOW_RELAX]    = rOther.m_StrandStressCoeff[CSS_AFTER_TRANSFER][LOW_RELAX];
   m_StrandStressCoeff[CSS_AFTER_ALL_LOSSES][STRESS_REL] = rOther.m_StrandStressCoeff[CSS_AFTER_ALL_LOSSES][STRESS_REL];
   m_StrandStressCoeff[CSS_AFTER_ALL_LOSSES][LOW_RELAX]  = rOther.m_StrandStressCoeff[CSS_AFTER_ALL_LOSSES][LOW_RELAX];

   m_bCheckTendonStressAtJacking = rOther.m_bCheckTendonStressAtJacking;
   m_bCheckTendonStressPriorToSeating = rOther.m_bCheckTendonStressPriorToSeating;
   m_TendonStressCoeff[CSS_AT_JACKING][STRESS_REL] = rOther.m_TendonStressCoeff[CSS_AT_JACKING][STRESS_REL];
   m_TendonStressCoeff[CSS_AT_JACKING][LOW_RELAX] = rOther.m_TendonStressCoeff[CSS_AT_JACKING][LOW_RELAX];
   m_TendonStressCoeff[CSS_PRIOR_TO_SEATING][STRESS_REL] = rOther.m_TendonStressCoeff[CSS_PRIOR_TO_SEATING][STRESS_REL];
   m_TendonStressCoeff[CSS_PRIOR_TO_SEATING][LOW_RELAX] = rOther.m_TendonStressCoeff[CSS_PRIOR_TO_SEATING][LOW_RELAX];
   m_TendonStressCoeff[CSS_ANCHORAGES_AFTER_SEATING][STRESS_REL] = rOther.m_TendonStressCoeff[CSS_ANCHORAGES_AFTER_SEATING][STRESS_REL];
   m_TendonStressCoeff[CSS_ANCHORAGES_AFTER_SEATING][LOW_RELAX] = rOther.m_TendonStressCoeff[CSS_ANCHORAGES_AFTER_SEATING][LOW_RELAX];
   m_TendonStressCoeff[CSS_ELSEWHERE_AFTER_SEATING][STRESS_REL] = rOther.m_TendonStressCoeff[CSS_ELSEWHERE_AFTER_SEATING][STRESS_REL];
   m_TendonStressCoeff[CSS_ELSEWHERE_AFTER_SEATING][LOW_RELAX] = rOther.m_TendonStressCoeff[CSS_ELSEWHERE_AFTER_SEATING][LOW_RELAX];
   m_TendonStressCoeff[CSS_AFTER_ALL_LOSSES][STRESS_REL] = rOther.m_TendonStressCoeff[CSS_AFTER_ALL_LOSSES][STRESS_REL];
   m_TendonStressCoeff[CSS_AFTER_ALL_LOSSES][LOW_RELAX] = rOther.m_TendonStressCoeff[CSS_AFTER_ALL_LOSSES][LOW_RELAX];

   m_bDoEvaluateDeflection = rOther.m_bDoEvaluateDeflection;
   m_DeflectionLimit       = rOther.m_DeflectionLimit;

   m_bIncludeStrand_NegMoment = rOther.m_bIncludeStrand_NegMoment;
   m_bConsiderReinforcementStrainLimit = rOther.m_bConsiderReinforcementStrainLimit;
   m_nMomentCapacitySlices = rOther.m_nMomentCapacitySlices;
   m_bIncludeRebar_Moment = rOther.m_bIncludeRebar_Moment;
   m_bIncludeRebar_Shear = rOther.m_bIncludeRebar_Shear;

   m_DoCheckStirrupSpacingCompatibility = rOther.m_DoCheckStirrupSpacingCompatibility;
   m_bCheckSag = rOther.m_bCheckSag;
   m_SagCamberType = rOther.m_SagCamberType;

   m_bUpdateLoadFactors = rOther.m_bUpdateLoadFactors;
   m_DCmin   = rOther.m_DCmin;
   m_DWmin   = rOther.m_DWmin;
   m_LLIMmin = rOther.m_LLIMmin;
   m_DCmax   = rOther.m_DCmax;
   m_DWmax   = rOther.m_DWmax;
   m_LLIMmax = rOther.m_LLIMmax;

   m_EnableSlabOffsetCheck = rOther.m_EnableSlabOffsetCheck;
   m_EnableSlabOffsetDesign = rOther.m_EnableSlabOffsetDesign;

   m_DesignStrandFillType = rOther.m_DesignStrandFillType;
   m_EffFlangeWidthMethod = rOther.m_EffFlangeWidthMethod;

   m_ShearFlowMethod = rOther.m_ShearFlowMethod;
   m_MaxInterfaceShearConnectorSpacing = rOther.m_MaxInterfaceShearConnectorSpacing;
   m_bUseDeckWeightForPc = rOther.m_bUseDeckWeightForPc;

   m_ShearCapacityMethod = rOther.m_ShearCapacityMethod;

   m_bLimitNetTensionStrainToPositiveValues = rOther.m_bLimitNetTensionStrainToPositiveValues;

   m_CuringMethodTimeAdjustmentFactor = rOther.m_CuringMethodTimeAdjustmentFactor;

   m_MinLiftPoint      = rOther.m_MinLiftPoint;
   m_LiftPointAccuracy = rOther.m_LiftPointAccuracy;
   m_MinHaulPoint      = rOther.m_MinHaulPoint;
   m_HaulPointAccuracy = rOther.m_HaulPointAccuracy;

   m_UseMinTruckSupportLocationFactor = rOther.m_UseMinTruckSupportLocationFactor;
   m_MinTruckSupportLocationFactor = rOther.m_MinTruckSupportLocationFactor;
   m_OverhangGFactor = rOther.m_OverhangGFactor;
   m_InteriorGFactor = rOther.m_InteriorGFactor;

   m_PedestrianLoad   = rOther.m_PedestrianLoad;
   m_MinSidewalkWidth = rOther.m_MinSidewalkWidth;

   m_MaxAngularDeviationBetweenGirders = rOther.m_MaxAngularDeviationBetweenGirders;
   m_MinGirderStiffnessRatio           = rOther.m_MinGirderStiffnessRatio;
   m_LLDFGirderSpacingLocation         = rOther.m_LLDFGirderSpacingLocation;

   m_bIncludeDualTandem = rOther.m_bIncludeDualTandem;

   m_LimitDistributionFactorsToLanesBeams = rOther.m_LimitDistributionFactorsToLanesBeams;
   m_ExteriorLiveLoadDistributionGTAdjacentInteriorRule = rOther.m_ExteriorLiveLoadDistributionGTAdjacentInteriorRule;

   m_PrestressTransferComputationType = rOther.m_PrestressTransferComputationType;

   m_RelaxationLossMethod = rOther.m_RelaxationLossMethod;

   m_FcgpComputationMethod = rOther.m_FcgpComputationMethod;

   m_MaxSlabFc = rOther.m_MaxSlabFc;
   m_MaxSegmentFci = rOther.m_MaxSegmentFci;
   m_MaxSegmentFc = rOther.m_MaxSegmentFc;
   m_MaxClosureFci = rOther.m_MaxClosureFci;
   m_MaxClosureFc = rOther.m_MaxClosureFc;
   m_MaxConcreteUnitWeight = rOther.m_MaxConcreteUnitWeight;
   m_MaxConcreteAggSize = rOther.m_MaxConcreteAggSize;

   m_PhiFlexureTensionPS      = rOther.m_PhiFlexureTensionPS;
   m_PhiFlexureTensionRC      = rOther.m_PhiFlexureTensionRC;
   m_PhiFlexureTensionSpliced = rOther.m_PhiFlexureTensionSpliced;
   m_PhiFlexureCompression    = rOther.m_PhiFlexureCompression;
   m_PhiShear                 = rOther.m_PhiShear;
   m_PhiShearDebonded         = rOther.m_PhiShearDebonded;

   m_PhiClosureJointFlexure = rOther.m_PhiClosureJointFlexure;
   m_PhiClosureJointShear   = rOther.m_PhiClosureJointShear;

   m_bIncludeForNegMoment = rOther.m_bIncludeForNegMoment;
   m_bAllowStraightStrandExtensions = rOther.m_bAllowStraightStrandExtensions;

   m_ClosureCompStressAtStressing             = rOther.m_ClosureCompStressAtStressing;
   m_ClosureTensStressPTZAtStressing          = rOther.m_ClosureTensStressPTZAtStressing;
   m_ClosureTensStressPTZWithRebarAtStressing = rOther.m_ClosureTensStressPTZWithRebarAtStressing;
   m_ClosureTensStressAtStressing             = rOther.m_ClosureTensStressAtStressing;
   m_ClosureTensStressWithRebarAtStressing    = rOther.m_ClosureTensStressWithRebarAtStressing;
   m_ClosureCompStressAtService               = rOther.m_ClosureCompStressAtService;
   m_ClosureCompStressWithLiveLoadAtService   = rOther.m_ClosureCompStressWithLiveLoadAtService;
   m_ClosureTensStressPTZAtService            = rOther.m_ClosureTensStressPTZAtService;
   m_ClosureTensStressPTZWithRebarAtService   = rOther.m_ClosureTensStressPTZWithRebarAtService;
   m_ClosureTensStressAtService               = rOther.m_ClosureTensStressAtService;
   m_ClosureTensStressWithRebarAtService      = rOther.m_ClosureTensStressWithRebarAtService;
   m_ClosureCompStressFatigue                 = rOther.m_ClosureCompStressFatigue;

   m_DuctAreaPushRatio = rOther.m_DuctAreaPushRatio;
   m_DuctAreaPullRatio = rOther.m_DuctAreaPullRatio;
   m_DuctDiameterRatio = rOther.m_DuctDiameterRatio;


   m_bCheckBottomFlangeClearance = rOther.m_bCheckBottomFlangeClearance;
   m_Cmin = rOther.m_Cmin;

   m_bCheckGirderInclination = rOther.m_bCheckGirderInclination;
   m_InclinedGirder_FSmax = rOther.m_InclinedGirder_FSmax;

   m_FinishedElevationTolerance = rOther.m_FinishedElevationTolerance;

   m_SlabOffsetRoundingMethod = rOther.m_SlabOffsetRoundingMethod;
   m_SlabOffsetRoundingTolerance = rOther.m_SlabOffsetRoundingTolerance;


   m_bAlertTaperedSolePlateRequirement = rOther.m_bAlertTaperedSolePlateRequirement;
   m_TaperedSolePlateInclinationThreshold = rOther.m_TaperedSolePlateInclinationThreshold;
   m_bUseImpactForBearingReactions = rOther.m_bUseImpactForBearingReactions;
}

void SpecLibraryEntry::MakeAssignment(const SpecLibraryEntry& rOther)
{
   libLibraryEntry::MakeAssignment( rOther );
   MakeCopy( rOther );
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PRIVATE    ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================

void SpecLibraryEntry::DeterminePrincipalStressDuctDeductionMultiplier()
{
   // Before 2nd Edition, 2000 interims
   // LRFD 5.8.2.9
   // "In determining the web width at a particular level, the diameter of ungrouted ducts or
   // one-half the diameter of grouted ducts at that level shall be subtracted from the web width"
   //
   // 2nd Edtion, 2003 interims
   // "In determining the web width at a particular level, one-half the diameter of ungrouted ducts or
   // one-quarter the diameter of grouted ducts at that level shall be subtracted from the web width"
   //
   // 9th Edition, 2020
   // LRFD 5.7.2.8
   // The paragraph quoted above has been removed and the following is now the definition of bv
   // "bv = ... for grouted ducts, no modification is necessary. For ungrouted ducts, reduce bv by the diameter of the duct"

   if (m_bUseCurrentSpecification || m_SpecificationType >= lrfdVersionMgr::NinthEdition2020)
   {
      // 9th Edition, 2020 and later
      m_PrincipalTensileStressUngroutedMultiplier = 1.0;
      m_PrincipalTensileStressGroutedMultiplier   = 0.0;
   }
   else if (m_SpecificationType < lrfdVersionMgr::SecondEditionWith2000Interims)
   {
      m_PrincipalTensileStressUngroutedMultiplier = 1.0;
      m_PrincipalTensileStressGroutedMultiplier   = 0.5;
   }
   else 
   {
      // 2nd Edition, 2000 interms to 9th Edition 2020
      m_PrincipalTensileStressUngroutedMultiplier = 0.50;
      m_PrincipalTensileStressGroutedMultiplier   = 0.25;
   }
}
//======================== ACCESS     =======================================
//======================== INQUERY    =======================================
//
