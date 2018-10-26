///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2012  Washington State Department of Transportation
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
#include <Units\sysUnits.h>

#include <MathEx.h>

#include <psgLib\ProjectLibraryManager.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define CURRENT_VERSION 42.0


/****************************************************************************
CLASS
   SpecLibraryEntry
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
SpecLibraryEntry::SpecLibraryEntry() :
m_SpecificationType(lrfdVersionMgr::FirstEditionWith1997Interims),
m_SpecificationUnits(lrfdVersionMgr::SI),
m_DoCheckStrandSlope(true),
m_DoDesignStrandSlope(true),
m_MaxSlope05(6),
m_MaxSlope06(8),
m_MaxSlope07(10),
m_DoCheckHoldDown(false),
m_DoDesignHoldDown(false),
m_HoldDownForce(ConvertToSysUnits(45,unitMeasure::Kip)),
m_DoCheckSplitting(true),
m_DoCheckConfinement(true),
m_DoDesignSplitting(true),
m_DoDesignConfinement(true),
m_MaxStirrupSpacing(ConvertToSysUnits(18,unitMeasure::Inch)),
m_CyLiftingCrackFs(1.0),
m_CyLiftingFailFs(1.5),
m_CyCompStressServ(0.45),
m_CyCompStressLifting(0.6),
m_CyTensStressServ(0),
m_CyDoTensStressServMax(false),
m_CyTensStressServMax(0),
m_CyTensStressLifting(0),
m_CyDoTensStressLiftingMax(false),
m_CyTensStressLiftingMax(0),
m_CyTensStressServWithRebar( ::ConvertToSysUnits(0.24,unitMeasure::SqrtKSI)),
m_TensStressLiftingWithRebar(::ConvertToSysUnits(0.24,unitMeasure::SqrtKSI)),
m_TensStressHaulingWithRebar(::ConvertToSysUnits(0.24,unitMeasure::SqrtKSI)),
m_SplittingZoneLengthFactor(4.0),
m_LiftingUpwardImpact(0),
m_LiftingDownwardImpact(0),
m_HaulingUpwardImpact(0),
m_HaulingDownwardImpact(0),
m_CuringMethod(CURING_ACCELERATED),
m_EnableLiftingCheck(true),
m_EnableLiftingDesign(true),
m_PickPointHeight(0),
m_MaxGirderSweepLifting(0),
m_EnableHaulingCheck(true),
m_EnableHaulingDesign(true),
m_MaxGirderSweepHauling(0),
m_HaulingSupportDistance(ConvertToSysUnits(200.0,unitMeasure::Feet)),
m_HaulingSupportPlacementTolerance(ConvertToSysUnits(1.0,unitMeasure::Inch)),
m_HaulingCamberPercentEstimate(2.0),
m_LiftingLoopTolerance(ConvertToSysUnits(1.0,unitMeasure::Inch)),
m_MinCableInclination(ConvertToSysUnits(90.,unitMeasure::Degree)),
m_MaxGirderWgt(ConvertToSysUnits(200.0,unitMeasure::Kip)),
m_CompStressHauling(0.6),
m_TensStressHauling(0),
m_DoTensStressHaulingMax(false),
m_TensStressHaulingMax(0),
m_HeHaulingCrackFs(1.0),
m_HeHaulingRollFs(1.5),
m_TruckRollStiffnessMethod(1),
m_TruckRollStiffness(ConvertToSysUnits(40000.,unitMeasure::KipInchPerRadian)),
m_AxleWeightLimit(ConvertToSysUnits(18.,unitMeasure::Kip)),
m_AxleStiffness(ConvertToSysUnits(4000.,unitMeasure::KipInchPerRadian)),
m_MinRollStiffness(ConvertToSysUnits(28000.,unitMeasure::KipInchPerRadian)),
m_TruckGirderHeight(ConvertToSysUnits(108.0,unitMeasure::Inch)),
m_TruckRollCenterHeight(ConvertToSysUnits(24.0,unitMeasure::Inch)),
m_TruckAxleWidth(ConvertToSysUnits(36.0,unitMeasure::Inch)),
m_HeErectionCrackFs(1.0),
m_HeErectionFailFs(1.5),
m_RoadwaySuperelevation(0.06),
m_TempStrandRemovalCompStress(0.45),
m_TempStrandRemovalTensStress(0.0),
m_TempStrandRemovalDoTensStressMax(false),
m_TempStrandRemovalTensStressMax(0.0),
m_Bs1CompStress(0.6),
m_Bs1TensStress(0.0),
m_Bs1DoTensStressMax(false),
m_Bs1TensStressMax(0.0),
m_Bs2CompStress(0.6),
m_TrafficBarrierDistributionType(pgsTypes::tbdGirder),
m_Bs2MaxGirdersTrafficBarrier(4),
m_Bs2MaxGirdersUtility(4),
m_OverlayLoadDistribution(pgsTypes::olDistributeEvenly),
m_Bs3CompStressServ(0.6),
m_Bs3CompStressService1A(0.4),
m_Bs3TensStressServNc(0),
m_Bs3DoTensStressServNcMax(false),
m_Bs3TensStressServNcMax(0),
m_Bs3TensStressServSc(0),   
m_Bs3DoTensStressServScMax(false),
m_Bs3TensStressServScMax(0),
m_Bs3IgnoreRangeOfApplicability(false),
m_Bs3LRFDOverReinforcedMomentCapacity(0),
m_CreepMethod(CREEP_LRFD),
m_XferTime(::ConvertToSysUnits(24,unitMeasure::Hour)),
m_CreepFactor(2.00),
m_CreepDuration1Min(::ConvertToSysUnits(10,unitMeasure::Day)),
m_CreepDuration2Min(::ConvertToSysUnits(40,unitMeasure::Day)),
m_CreepDuration1Max(::ConvertToSysUnits(90,unitMeasure::Day)),
m_CreepDuration2Max(::ConvertToSysUnits(120,unitMeasure::Day)),
m_TotalCreepDuration(::ConvertToSysUnits(2000,unitMeasure::Day)),
m_LossMethod(LOSSES_AASHTO_REFINED),
m_BeforeXferLosses(0),
m_AfterXferLosses(0),
m_LiftingLosses(0),
m_ShippingLosses(::ConvertToSysUnits(20,unitMeasure::KSI)),
m_FinalLosses(0),
m_ShippingTime(::ConvertToSysUnits(10,unitMeasure::Day)),
m_LldfMethod(LLDF_LRFD),
m_BeforeTempStrandRemovalLosses(0),
m_AfterTempStrandRemovalLosses(0),
m_AfterDeckPlacementLosses(0),
m_AfterSIDLLosses(0),
m_Dset(::ConvertToSysUnits(0.375,unitMeasure::Inch)),
m_WobbleFriction(::ConvertToSysUnits(0.0002,unitMeasure::PerFeet)),
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
m_bIncludeRebar_Moment(false),
m_bIncludeRebar_Shear(false),
m_AnalysisType(pgsTypes::Envelope),
m_EnableSlabOffsetCheck(true),
m_EnableSlabOffsetDesign(true),
m_DesignStrandFillType(ftMinimizeHarping),
m_EffFlangeWidthMethod(pgsTypes::efwmLRFD),
m_ShearFlowMethod(sfmClassical),
m_ShearCapacityMethod(scmBTTables),
m_CuringMethodTimeAdjustmentFactor(7),
m_MinLiftPoint(-1), // H
m_LiftPointAccuracy(::ConvertToSysUnits(0.25,unitMeasure::Feet)),
m_MinHaulPoint(-1), // H
m_HaulPointAccuracy(::ConvertToSysUnits(0.5,unitMeasure::Feet)),
m_PedestrianLoad(::ConvertToSysUnits(0.075,unitMeasure::KSF)),
m_MinSidewalkWidth(::ConvertToSysUnits(2.0,unitMeasure::Feet)),
m_MaxAngularDeviationBetweenGirders(::ConvertToSysUnits(5.0,unitMeasure::Degree)),
m_MinGirderStiffnessRatio(0.90),
m_LLDFGirderSpacingLocation(0.75),
m_LimitDistributionFactorsToLanesBeams(false),
m_PrestressTransferComputationType(pgsTypes::ptUsingSpecification),
m_bIncludeForNegMoment(true),
m_bAllowStraightStrandExtensions(false),
m_RelaxationLossMethod(RLM_REFINED)
{
   m_bCheckStrandStress[AT_JACKING]       = false;
   m_bCheckStrandStress[BEFORE_TRANSFER]  = true;
   m_bCheckStrandStress[AFTER_TRANSFER]   = false;
   m_bCheckStrandStress[AFTER_ALL_LOSSES] = true;

   m_StrandStressCoeff[AT_JACKING][STRESS_REL]       = 0.72;
   m_StrandStressCoeff[AT_JACKING][LOW_RELAX]        = 0.78;
   m_StrandStressCoeff[BEFORE_TRANSFER][STRESS_REL]  = 0.70;
   m_StrandStressCoeff[BEFORE_TRANSFER][LOW_RELAX]   = 0.75;
   m_StrandStressCoeff[AFTER_TRANSFER][STRESS_REL]   = 0.70;
   m_StrandStressCoeff[AFTER_TRANSFER][LOW_RELAX]    = 0.74;
   m_StrandStressCoeff[AFTER_ALL_LOSSES][STRESS_REL] = 0.80;
   m_StrandStressCoeff[AFTER_ALL_LOSSES][LOW_RELAX]  = 0.80;


   m_FlexureModulusOfRuptureCoefficient[pgsTypes::Normal]          = ::ConvertToSysUnits(0.37,unitMeasure::SqrtKSI);
   m_FlexureModulusOfRuptureCoefficient[pgsTypes::SandLightweight] = ::ConvertToSysUnits(0.20,unitMeasure::SqrtKSI);
   m_FlexureModulusOfRuptureCoefficient[pgsTypes::AllLightweight]  = ::ConvertToSysUnits(0.17,unitMeasure::SqrtKSI);
   
   m_ShearModulusOfRuptureCoefficient[pgsTypes::Normal]          = ::ConvertToSysUnits(0.20,unitMeasure::SqrtKSI);
   m_ShearModulusOfRuptureCoefficient[pgsTypes::SandLightweight] = ::ConvertToSysUnits(0.20,unitMeasure::SqrtKSI);
   m_ShearModulusOfRuptureCoefficient[pgsTypes::AllLightweight]  = ::ConvertToSysUnits(0.17,unitMeasure::SqrtKSI);

   m_LiftingModulusOfRuptureCoefficient[pgsTypes::Normal]          = ::ConvertToSysUnits(0.24,unitMeasure::SqrtKSI);
   m_LiftingModulusOfRuptureCoefficient[pgsTypes::SandLightweight] = ::ConvertToSysUnits(0.21,unitMeasure::SqrtKSI);
   m_LiftingModulusOfRuptureCoefficient[pgsTypes::AllLightweight]  = ::ConvertToSysUnits(0.18,unitMeasure::SqrtKSI);

   m_HaulingModulusOfRuptureCoefficient[pgsTypes::Normal] = ::ConvertToSysUnits(0.24,unitMeasure::SqrtKSI);
   m_HaulingModulusOfRuptureCoefficient[pgsTypes::SandLightweight] = ::ConvertToSysUnits(0.21,unitMeasure::SqrtKSI);
   m_HaulingModulusOfRuptureCoefficient[pgsTypes::AllLightweight] = ::ConvertToSysUnits(0.18,unitMeasure::SqrtKSI);

   m_PhiFlexureTensionPS[pgsTypes::Normal] = 1.0;
   m_PhiFlexureTensionRC[pgsTypes::Normal] = 0.9;
   m_PhiFlexureCompression[pgsTypes::Normal] = 0.75;

   m_PhiFlexureTensionPS[pgsTypes::SandLightweight] = 1.0;
   m_PhiFlexureTensionRC[pgsTypes::SandLightweight] = 0.9;
   m_PhiFlexureCompression[pgsTypes::SandLightweight] = 0.75;

   m_PhiFlexureTensionPS[pgsTypes::AllLightweight] = 1.0;
   m_PhiFlexureTensionRC[pgsTypes::AllLightweight] = 0.9;
   m_PhiFlexureCompression[pgsTypes::AllLightweight] = 0.75;

   m_PhiShear[pgsTypes::Normal] = 0.9;
   m_PhiShear[pgsTypes::SandLightweight] = 0.7;
   m_PhiShear[pgsTypes::AllLightweight] = 0.7;

   m_MaxSlabFc[pgsTypes::Normal]             = ::ConvertToSysUnits(6.0,unitMeasure::KSI);
   m_MaxGirderFci[pgsTypes::Normal]          = ::ConvertToSysUnits(7.5,unitMeasure::KSI);
   m_MaxGirderFc[pgsTypes::Normal]           = ::ConvertToSysUnits(10.0,unitMeasure::KSI);
   m_MaxConcreteUnitWeight[pgsTypes::Normal] = ::ConvertToSysUnits(165.,unitMeasure::LbfPerFeet3);
   m_MaxConcreteAggSize[pgsTypes::Normal]    = ::ConvertToSysUnits(1.5,unitMeasure::Inch);

   m_MaxSlabFc[pgsTypes::AllLightweight]             = ::ConvertToSysUnits(6.0,unitMeasure::KSI);
   m_MaxGirderFci[pgsTypes::AllLightweight]          = ::ConvertToSysUnits(7.5,unitMeasure::KSI);
   m_MaxGirderFc[pgsTypes::AllLightweight]           = ::ConvertToSysUnits(9.0,unitMeasure::KSI);
   m_MaxConcreteUnitWeight[pgsTypes::AllLightweight] = ::ConvertToSysUnits(125.,unitMeasure::LbfPerFeet3);
   m_MaxConcreteAggSize[pgsTypes::AllLightweight]    = ::ConvertToSysUnits(1.5,unitMeasure::Inch);

   m_MaxSlabFc[pgsTypes::SandLightweight]             = ::ConvertToSysUnits(6.0,unitMeasure::KSI);
   m_MaxGirderFci[pgsTypes::SandLightweight]          = ::ConvertToSysUnits(7.5,unitMeasure::KSI);
   m_MaxGirderFc[pgsTypes::SandLightweight]           = ::ConvertToSysUnits(9.0,unitMeasure::KSI);
   m_MaxConcreteUnitWeight[pgsTypes::SandLightweight] = ::ConvertToSysUnits(125.,unitMeasure::LbfPerFeet3);
   m_MaxConcreteAggSize[pgsTypes::SandLightweight]    = ::ConvertToSysUnits(1.5,unitMeasure::Inch);
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
bool SpecLibraryEntry::Edit(bool allowEditing)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   // exchange data with dialog
   // make a temporary copy of this and have the dialog work on it.
   SpecLibraryEntry tmp(*this);

   CSpecMainSheet dlg(tmp, IDS_SPEC_SHEET, allowEditing);
   INT_PTR i = dlg.DoModal();
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

bool SpecLibraryEntry::SaveMe(sysIStructuredSave* pSave)
{
   pSave->BeginUnit(_T("SpecificationLibraryEntry"), CURRENT_VERSION);
   pSave->Property(_T("Name"),this->GetName().c_str());
   pSave->Property(_T("Description"),this->GetDescription().c_str());

   if (m_SpecificationType==lrfdVersionMgr::FirstEdition1994)
      pSave->Property(_T("SpecificationType"), _T("AashtoLrfd1994"));
   else if (m_SpecificationType==lrfdVersionMgr::FirstEditionWith1996Interims)
      pSave->Property(_T("SpecificationType"), _T("AashtoLrfd1996"));
   else if (m_SpecificationType==lrfdVersionMgr::FirstEditionWith1997Interims)
      pSave->Property(_T("SpecificationType"), _T("AashtoLrfd1997"));
   else if (m_SpecificationType==lrfdVersionMgr::SecondEdition1998)
      pSave->Property(_T("SpecificationType"), _T("AashtoLrfd1998"));
   else if (m_SpecificationType==lrfdVersionMgr::SecondEditionWith1999Interims)
      pSave->Property(_T("SpecificationType"), _T("AashtoLrfd1999"));
   else if (m_SpecificationType==lrfdVersionMgr::SecondEditionWith2000Interims)
      pSave->Property(_T("SpecificationType"), _T("AashtoLrfd2000"));
   else if (m_SpecificationType==lrfdVersionMgr::SecondEditionWith2001Interims)
      pSave->Property(_T("SpecificationType"), _T("AashtoLrfd2001"));
   else if (m_SpecificationType==lrfdVersionMgr::SecondEditionWith2002Interims)
      pSave->Property(_T("SpecificationType"), _T("AashtoLrfd2002"));
   else if (m_SpecificationType==lrfdVersionMgr::SecondEditionWith2003Interims)
      pSave->Property(_T("SpecificationType"), _T("AashtoLrfd2003"));
   else if (m_SpecificationType==lrfdVersionMgr::ThirdEdition2004)
      pSave->Property(_T("SpecificationType"), _T("AashtoLrfd2004"));
   else if (m_SpecificationType==lrfdVersionMgr::ThirdEditionWith2005Interims)
      pSave->Property(_T("SpecificationType"), _T("AashtoLrfd2005"));
   else if (m_SpecificationType==lrfdVersionMgr::ThirdEditionWith2006Interims)
      pSave->Property(_T("SpecificationType"), _T("AashtoLrfd2006"));
   else if (m_SpecificationType==lrfdVersionMgr::FourthEdition2007)
      pSave->Property(_T("SpecificationType"), _T("AashtoLrfd2007"));
   else if (m_SpecificationType==lrfdVersionMgr::FourthEditionWith2008Interims)
      pSave->Property(_T("SpecificationType"), _T("AashtoLrfd2008"));
   else if (m_SpecificationType==lrfdVersionMgr::FourthEditionWith2009Interims)
      pSave->Property(_T("SpecificationType"), _T("AashtoLrfd2009"));
   else if (m_SpecificationType==lrfdVersionMgr::FifthEdition2010)
      pSave->Property(_T("SpecificationType"), _T("AashtoLrfd2010"));
   else if (m_SpecificationType==lrfdVersionMgr::SixthEdition2012)
      pSave->Property(_T("SpecificationType"), _T("AashtoLrfd2012"));
   else
      ASSERT(false); // is there a new version?

   if (m_SpecificationUnits==lrfdVersionMgr::SI)
      pSave->Property(_T("SpecificationUnits"), _T("SiUnitsSpec"));
   else if (m_SpecificationUnits==lrfdVersionMgr::US)
      pSave->Property(_T("SpecificationUnits"), _T("UsUnitsSpec"));
   else
      ASSERT(0);

   pSave->Property(_T("DoCheckStrandSlope"), m_DoCheckStrandSlope);
   pSave->Property(_T("DoDesignStrandSlope"), m_DoDesignStrandSlope);
   pSave->Property(_T("MaxSlope05"), m_MaxSlope05);
   pSave->Property(_T("MaxSlope06"), m_MaxSlope06);
   pSave->Property(_T("MaxSlope07"), m_MaxSlope07); // added in version 35
   pSave->Property(_T("DoCheckHoldDown"), m_DoCheckHoldDown);
   pSave->Property(_T("DoDesignHoldDown"), m_DoDesignHoldDown);
   pSave->Property(_T("HoldDownForce"), m_HoldDownForce);
   pSave->Property(_T("DoCheckSplitting"), m_DoCheckSplitting);
   pSave->Property(_T("DoDesignSplitting"), m_DoDesignSplitting);
   pSave->Property(_T("DoCheckConfinement"), m_DoCheckConfinement);
   pSave->Property(_T("DoDesignConfinement"), m_DoDesignConfinement);
   pSave->Property(_T("MaxStirrupSpacing"), m_MaxStirrupSpacing);
   pSave->Property(_T("CyLiftingCrackFs"), m_CyLiftingCrackFs);
   pSave->Property(_T("CyLiftingFailFs"), m_CyLiftingFailFs);
   pSave->Property(_T("CyCompStressServ"), m_CyCompStressServ);
   pSave->Property(_T("CyCompStressLifting"), m_CyCompStressLifting);
   pSave->Property(_T("CyTensStressServ"), m_CyTensStressServ);
   pSave->Property(_T("CyDoTensStressServMax"), m_CyDoTensStressServMax);
   pSave->Property(_T("CyTensStressServMax"), m_CyTensStressServMax);
   pSave->Property(_T("CyTensStressLifting"), m_CyTensStressLifting);
   pSave->Property(_T("CyDoTensStressLiftingMax"),m_CyDoTensStressLiftingMax);
   pSave->Property(_T("CyTensStressLiftingMax"),m_CyTensStressLiftingMax);
   pSave->Property(_T("BurstingZoneLengthFactor"), m_SplittingZoneLengthFactor),
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
   pSave->Property(_T("EnableHaulingCheck"), m_EnableHaulingCheck);
   pSave->Property(_T("EnableHaulingDesign"), m_EnableHaulingDesign);
   pSave->Property(_T("MaxGirderSweepHauling"), m_MaxGirderSweepHauling);
   pSave->Property(_T("HaulingSupportDistance"),m_HaulingSupportDistance);
   pSave->Property(_T("MaxHaulingOverhang"), m_MaxHaulingOverhang);
   pSave->Property(_T("HaulingSupportPlacementTolerance"),m_HaulingSupportPlacementTolerance);
   pSave->Property(_T("HaulingCamberPercentEstimate"),m_HaulingCamberPercentEstimate);
   pSave->Property(_T("CompStressHauling"), m_CompStressHauling);
   pSave->Property(_T("TensStressHauling"),m_TensStressHauling);
   pSave->Property(_T("DoTensStressHaulingMax"),m_DoTensStressHaulingMax); 
   pSave->Property(_T("TensStressHaulingMax"), m_TensStressHaulingMax);
   pSave->Property(_T("HeHaulingCrackFs"), m_HeHaulingCrackFs);
   pSave->Property(_T("HeHaulingFailFs"), m_HeHaulingRollFs);
   pSave->Property(_T("RoadwaySuperelevation"), m_RoadwaySuperelevation);
   pSave->Property(_T("TruckRollStiffnessMethod"), (long)m_TruckRollStiffnessMethod);
   pSave->Property(_T("TruckRollStiffness"), m_TruckRollStiffness);
   pSave->Property(_T("AxleWeightLimit"), m_AxleWeightLimit);
   pSave->Property(_T("AxleStiffness"),m_AxleStiffness);
   pSave->Property(_T("MinRollStiffness"),m_MinRollStiffness);
   pSave->Property(_T("TruckGirderHeight"), m_TruckGirderHeight);
   pSave->Property(_T("TruckRollCenterHeight"), m_TruckRollCenterHeight);
   pSave->Property(_T("TruckAxleWidth"), m_TruckAxleWidth);
   pSave->Property(_T("HeErectionCrackFs"), m_HeErectionCrackFs);
   pSave->Property(_T("HeErectionFailFs"), m_HeErectionFailFs);
   pSave->Property(_T("MaxGirderWgt"),m_MaxGirderWgt);

   // modified in version 37 (see below)
   //pSave->Property(_T("HaulingModulusOfRuptureCoefficient"),m_HaulingModulusOfRuptureCoefficient); // added for version 12.0
   //pSave->Property(_T("LiftingModulusOfRuptureCoefficient"),m_LiftingModulusOfRuptureCoefficient); // added for version 20.0


   // Added at version 25
   pSave->Property(_T("MinLiftingPointLocation"),       m_MinLiftPoint        );
   pSave->Property(_T("LiftingPointLocationAccuracy"),  m_LiftPointAccuracy   );
   pSave->Property(_T("MinHaulingSupportLocation"),     m_MinHaulPoint        );
   pSave->Property(_T("HaulingSupportLocationAccuracy"),m_HaulPointAccuracy   );

   // Added at version 4.0
   pSave->Property(_T("CastingYardTensileStressLimitWithMildRebar"),m_CyTensStressServWithRebar);
   pSave->Property(_T("LiftingTensileStressLimitWithMildRebar"),m_TensStressLiftingWithRebar);
   pSave->Property(_T("HaulingTensileStressLimitWithMildRebar"),m_TensStressHaulingWithRebar);

   // Added at version 30
   pSave->Property(_T("TempStrandRemovalCompStress") ,     m_TempStrandRemovalCompStress);
   pSave->Property(_T("TempStrandRemovalTensStress") ,     m_TempStrandRemovalTensStress);
   pSave->Property(_T("TempStrandRemovalDoTensStressMax") ,m_TempStrandRemovalDoTensStressMax);
   pSave->Property(_T("TempStrandRemovalTensStressMax") ,  m_TempStrandRemovalTensStressMax);

   pSave->Property(_T("Bs1CompStress") ,     m_Bs1CompStress); // removed m_ in version 30
   pSave->Property(_T("Bs1TensStress") ,     m_Bs1TensStress); // removed m_ in version 30
   pSave->Property(_T("Bs1DoTensStressMax") ,m_Bs1DoTensStressMax); // removed m_ in version 30
   pSave->Property(_T("Bs1TensStressMax") ,  m_Bs1TensStressMax); // removed m_ in version 30
   pSave->Property(_T("Bs2CompStress") ,     m_Bs2CompStress); // removed m_ in version 30
   pSave->Property(_T("Bs2TrafficBarrierDistributionType"),(Int16)m_TrafficBarrierDistributionType); // added in version 36
   pSave->Property(_T("Bs2MaxGirdersTrafficBarrier"), m_Bs2MaxGirdersTrafficBarrier);
   pSave->Property(_T("Bs2MaxGirdersUtility"), m_Bs2MaxGirdersUtility);
   pSave->Property(_T("OverlayLoadDistribution"), (Int32)m_OverlayLoadDistribution); // added in version 34
   pSave->Property(_T("Bs3CompStressServ"), m_Bs3CompStressServ);
   pSave->Property(_T("Bs3CompStressService1A"), m_Bs3CompStressService1A);
   pSave->Property(_T("Bs3TensStressServNc"), m_Bs3TensStressServNc);
   pSave->Property(_T("Bs3DoTensStressServNcMax"), m_Bs3DoTensStressServNcMax);
   pSave->Property(_T("Bs3TensStressServNcMax"), m_Bs3TensStressServNcMax);
   pSave->Property(_T("Bs3TensStressServSc"),    m_Bs3TensStressServSc);   
   pSave->Property(_T("Bs3DoTensStressServScMax"), m_Bs3DoTensStressServScMax);
   pSave->Property(_T("Bs3TensStressServScMax"), m_Bs3TensStressServScMax);

   // removed in version 29
   // pSave->Property(_T("Bs3IgnoreRangeOfApplicability"), m_Bs3IgnoreRangeOfApplicability);

   // moved into MomentCapacity data block (version 37)
   //pSave->Property(_T("Bs3LRFDOverreinforcedMomentCapacity"),(Int16)m_Bs3LRFDOverReinforcedMomentCapacity);
   //pSave->Property(_T("IncludeRebar_MomentCapacity"),m_bIncludeRebar_Moment); // added for version 7.0

   // added in version 37
   pSave->BeginUnit(_T("MomentCapacity"),2.0);
      pSave->Property(_T("Bs3LRFDOverreinforcedMomentCapacity"),(Int16)m_Bs3LRFDOverReinforcedMomentCapacity);
      pSave->Property(_T("IncludeRebarForCapacity"),m_bIncludeRebar_Moment);
      pSave->Property(_T("IncludeNoncompositeMomentForNegMomentDesign"),m_bIncludeForNegMoment); // added version 2 of this data block
      pSave->BeginUnit(_T("ReductionFactor"),1.0);
         pSave->BeginUnit(_T("NormalWeight"),1.0);
            pSave->Property(_T("TensionControlled_RC"),m_PhiFlexureTensionRC[pgsTypes::Normal]);
            pSave->Property(_T("TensionControlled_PS"),m_PhiFlexureTensionPS[pgsTypes::Normal]);
            pSave->Property(_T("CompressionControlled"),m_PhiFlexureCompression[pgsTypes::Normal]);
         pSave->EndUnit(); // NormalWeight
         pSave->BeginUnit(_T("AllLightweight"),1.0);
            pSave->Property(_T("TensionControlled_RC"),m_PhiFlexureTensionRC[pgsTypes::AllLightweight]);
            pSave->Property(_T("TensionControlled_PS"),m_PhiFlexureTensionPS[pgsTypes::AllLightweight]);
            pSave->Property(_T("CompressionControlled"),m_PhiFlexureCompression[pgsTypes::AllLightweight]);
         pSave->EndUnit(); // AllLightweight
         pSave->BeginUnit(_T("SandLightweight"),1.0);
            pSave->Property(_T("TensionControlled_RC"),m_PhiFlexureTensionRC[pgsTypes::SandLightweight]);
            pSave->Property(_T("TensionControlled_PS"),m_PhiFlexureTensionPS[pgsTypes::SandLightweight]);
            pSave->Property(_T("CompressionControlled"),m_PhiFlexureCompression[pgsTypes::SandLightweight]);
         pSave->EndUnit(); // SandLightweight
      pSave->EndUnit(); // ReductionFactor
   pSave->EndUnit(); // MomentCapacity

   // changed in version 37
   //pSave->Property(_T("ModulusOfRuptureCoefficient"),m_FlexureModulusOfRuptureCoefficient); // added for version 9.0
   //pSave->Property(_T("ShearModulusOfRuptureCoefficient"),m_ShearModulusOfRuptureCoefficient); // added for version 18
   pSave->BeginUnit(_T("ModulusOfRuptureCoefficient"),1.0);
      pSave->BeginUnit(_T("Moment"),1.0);
         pSave->Property(_T("Normal"),m_FlexureModulusOfRuptureCoefficient[pgsTypes::Normal]);
         pSave->Property(_T("AllLightweight"),m_FlexureModulusOfRuptureCoefficient[pgsTypes::AllLightweight]);
         pSave->Property(_T("SandLightweight"),m_FlexureModulusOfRuptureCoefficient[pgsTypes::SandLightweight]);
      pSave->EndUnit(); // Moment
      pSave->BeginUnit(_T("Shear"),1.0);
         pSave->Property(_T("Normal"),m_ShearModulusOfRuptureCoefficient[pgsTypes::Normal]);
         pSave->Property(_T("AllLightweight"),m_ShearModulusOfRuptureCoefficient[pgsTypes::AllLightweight]);
         pSave->Property(_T("SandLightweight"),m_ShearModulusOfRuptureCoefficient[pgsTypes::SandLightweight]);
      pSave->EndUnit(); // Shear
      pSave->BeginUnit(_T("Lifting"),1.0);
         pSave->Property(_T("Normal"),m_LiftingModulusOfRuptureCoefficient[pgsTypes::Normal]);
         pSave->Property(_T("AllLightweight"),m_LiftingModulusOfRuptureCoefficient[pgsTypes::AllLightweight]);
         pSave->Property(_T("SandLightweight"),m_LiftingModulusOfRuptureCoefficient[pgsTypes::SandLightweight]);
      pSave->EndUnit(); // Lifting
      pSave->BeginUnit(_T("Shipping"),1.0);
         pSave->Property(_T("Normal"),m_HaulingModulusOfRuptureCoefficient[pgsTypes::Normal]);
         pSave->Property(_T("AllLightweight"),m_HaulingModulusOfRuptureCoefficient[pgsTypes::AllLightweight]);
         pSave->Property(_T("SandLightweight"),m_HaulingModulusOfRuptureCoefficient[pgsTypes::SandLightweight]);
      pSave->EndUnit(); // Shipping
   pSave->EndUnit(); // ModulusOfRuptureCoefficient

   pSave->Property(_T("BsLldfMethod"), (Int16)m_LldfMethod );  // added LLDF_TXDOT for version 21.0
   // added in version 29
   pSave->Property(_T("MaxAngularDeviationBetweenGirders"),m_MaxAngularDeviationBetweenGirders);
   pSave->Property(_T("MinGirderStiffnessRatio"),m_MinGirderStiffnessRatio);
   pSave->Property(_T("LLDFGirderSpacingLocation"),m_LLDFGirderSpacingLocation);

   // added in version 31
   pSave->Property(_T("LimitDistributionFactorsToLanesBeams"),m_LimitDistributionFactorsToLanesBeams);

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

   pSave->Property(_T("LossMethod"),(Int16)m_LossMethod);
   pSave->Property(_T("FinalLosses"),m_FinalLosses);
   pSave->Property(_T("ShippingLosses"),m_ShippingLosses);
   pSave->Property(_T("BeforeXferLosses"),m_BeforeXferLosses);
   pSave->Property(_T("AfterXferLosses"),m_AfterXferLosses);
   pSave->Property(_T("ShippingTime"),m_ShippingTime);

   // added in version 22
   pSave->Property(_T("LiftingLosses"),m_LiftingLosses);
   pSave->Property(_T("BeforeTempStrandRemovalLosses"),m_BeforeTempStrandRemovalLosses);
   pSave->Property(_T("AfterTempStrandRemovalLosses"),m_AfterTempStrandRemovalLosses);
   pSave->Property(_T("AfterDeckPlacementLosses"),m_AfterDeckPlacementLosses);
   pSave->Property(_T("AfterSIDLLosses"),m_AfterSIDLLosses); // added in version 38


   pSave->Property(_T("CuringMethodFactor"),m_CuringMethodTimeAdjustmentFactor);

   // Added in version 1.5
   pSave->Property(_T("CheckStrandStressAtJacking"),m_bCheckStrandStress[AT_JACKING]);
   pSave->Property(_T("Coeff_AtJacking_StressRel"),m_StrandStressCoeff[AT_JACKING][STRESS_REL]);
   pSave->Property(_T("Coeff_AtJacking_LowRelax"),m_StrandStressCoeff[AT_JACKING][LOW_RELAX]);

   pSave->Property(_T("CheckStrandStressBeforeTransfer"),m_bCheckStrandStress[BEFORE_TRANSFER]);
   pSave->Property(_T("Coeff_BeforeTransfer_StressRel"),m_StrandStressCoeff[BEFORE_TRANSFER][STRESS_REL]);
   pSave->Property(_T("Coeff_BeforeTransfer_LowRelax"),m_StrandStressCoeff[BEFORE_TRANSFER][LOW_RELAX]);

   pSave->Property(_T("CheckStrandStressAfterTransfer"),m_bCheckStrandStress[AFTER_TRANSFER]);
   pSave->Property(_T("Coeff_AfterTransfer_StressRel"),m_StrandStressCoeff[AFTER_TRANSFER][STRESS_REL]);
   pSave->Property(_T("Coeff_AfterTransfer_LowRelax"),m_StrandStressCoeff[AFTER_TRANSFER][LOW_RELAX]);

   pSave->Property(_T("CheckStrandStressAfterAllLosses"),m_bCheckStrandStress[AFTER_ALL_LOSSES]);
   pSave->Property(_T("Coeff_AfterAllLosses_StressRel"),m_StrandStressCoeff[AFTER_ALL_LOSSES][STRESS_REL]);
   pSave->Property(_T("Coeff_AfterAllLosses_LowRelax"),m_StrandStressCoeff[AFTER_ALL_LOSSES][LOW_RELAX]);

   // added in version 23
   pSave->Property(_T("AnchorSet"),m_Dset);
   pSave->Property(_T("WobbleFriction"),m_WobbleFriction);
   pSave->Property(_T("CoefficientOfFriction"),m_FrictionCoefficient);

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

   // Added in 1.7
   pSave->Property(_T("CheckLiveLoadDeflection"),m_bDoEvaluateDeflection);
   pSave->Property(_T("LiveLoadDeflectionLimit"),m_DeflectionLimit);

//   // Added in 8.0, removed in version 28
//   pSave->Property(_T("AnalysisType"),(long)m_AnalysisType);

   // Added in 10.0, updated in version 37
   pSave->BeginUnit(_T("Limits"),1.0);
      pSave->BeginUnit(_T("Normal"),1.0);
         pSave->Property(_T("MaxSlabFc"),             m_MaxSlabFc[pgsTypes::Normal]);
         pSave->Property(_T("MaxGirderFci"),          m_MaxGirderFci[pgsTypes::Normal]);
         pSave->Property(_T("MaxGirderFc"),           m_MaxGirderFc[pgsTypes::Normal]);
         pSave->Property(_T("MaxConcreteUnitWeight"), m_MaxConcreteUnitWeight[pgsTypes::Normal]);
         pSave->Property(_T("MaxConcreteAggSize"),    m_MaxConcreteAggSize[pgsTypes::Normal]);
      pSave->EndUnit(); // Normal;
      pSave->BeginUnit(_T("AllLightweight"),1.0);
         pSave->Property(_T("MaxSlabFc"),             m_MaxSlabFc[pgsTypes::AllLightweight]);
         pSave->Property(_T("MaxGirderFci"),          m_MaxGirderFci[pgsTypes::AllLightweight]);
         pSave->Property(_T("MaxGirderFc"),           m_MaxGirderFc[pgsTypes::AllLightweight]);
         pSave->Property(_T("MaxConcreteUnitWeight"), m_MaxConcreteUnitWeight[pgsTypes::AllLightweight]);
         pSave->Property(_T("MaxConcreteAggSize"),    m_MaxConcreteAggSize[pgsTypes::AllLightweight]);
      pSave->EndUnit(); // AllLightweight;
      pSave->BeginUnit(_T("SandLightweight"),1.0);
         pSave->Property(_T("MaxSlabFc"),             m_MaxSlabFc[pgsTypes::SandLightweight]);
         pSave->Property(_T("MaxGirderFci"),          m_MaxGirderFci[pgsTypes::SandLightweight]);
         pSave->Property(_T("MaxGirderFc"),           m_MaxGirderFc[pgsTypes::SandLightweight]);
         pSave->Property(_T("MaxConcreteUnitWeight"), m_MaxConcreteUnitWeight[pgsTypes::SandLightweight]);
         pSave->Property(_T("MaxConcreteAggSize"),    m_MaxConcreteAggSize[pgsTypes::SandLightweight]);
      pSave->EndUnit(); // SandLightweight;
   pSave->EndUnit(); // Limits


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
   pSave->BeginUnit(_T("Shear"),1.0);
      // moved here in version 37
      pSave->Property(_T("LongReinfShearMethod"),(Int16)m_LongReinfShearMethod); // added for version 1.2

      // moved here in version 37
      pSave->Property(_T("IncludeRebarForCapacity"),m_bIncludeRebar_Shear); // added for version 7.0

      // added in version 18 (moved into datablock in version 37)
      pSave->Property(_T("ShearFlowMethod"),(long)m_ShearFlowMethod);
      pSave->Property(_T("ShearCapacityMethod"),(long)m_ShearCapacityMethod);

      // added inv ersion 37
      pSave->BeginUnit(_T("ReductionFactor"),1.0);
         pSave->Property(_T("Normal"),m_PhiShear[pgsTypes::Normal]);
         pSave->Property(_T("AllLightweight"),m_PhiShear[pgsTypes::AllLightweight]);
         pSave->Property(_T("SandLightweight"),m_PhiShear[pgsTypes::SandLightweight]);
      pSave->EndUnit(); // ReductionFactor
   pSave->EndUnit(); // Shear

   // added in version 26
   pSave->Property(_T("PedestrianLoad"),m_PedestrianLoad);
   pSave->Property(_T("MinSidewalkWidth"),m_MinSidewalkWidth);

   // added in vers 32
   pSave->Property(_T("PrestressTransferComputationType"),(long)m_PrestressTransferComputationType);

   pSave->BeginUnit(_T("StrandExtensions"),1.0);
   pSave->Property(_T("AllowStraightStrandExtensions"),m_bAllowStraightStrandExtensions);
   pSave->EndUnit();

   pSave->EndUnit();

   return true;
}

bool SpecLibraryEntry::LoadMe(sysIStructuredLoad* pLoad)
{
   Int16 temp;
   ShearCapacityMethod shear_capacity_method = m_ShearCapacityMethod; // used as a temporary storage for shear capacity method before file version 18

   if(pLoad->BeginUnit(_T("SpecificationLibraryEntry")))
   {
      Float64 version = pLoad->GetVersion();
      if (version < 1.0 || CURRENT_VERSION < version)
         THROW_LOAD(BadVersion,pLoad);

      std::_tstring name;
      if(pLoad->Property(_T("Name"),&name))
         this->SetName(name.c_str());
      else
         THROW_LOAD(InvalidFileFormat,pLoad);

      if(pLoad->Property(_T("Description"),&name))
         this->SetDescription(name.c_str());
      else
         THROW_LOAD(InvalidFileFormat,pLoad);

      std::_tstring tmp;
      if(pLoad->Property(_T("SpecificationType"),&tmp))
      {
         if(tmp==_T("AashtoLrfd2012"))
            m_SpecificationType = lrfdVersionMgr::SixthEdition2012;
         else if(tmp==_T("AashtoLrfd2010"))
            m_SpecificationType = lrfdVersionMgr::FifthEdition2010;
         else if(tmp==_T("AashtoLrfd2009"))
            m_SpecificationType = lrfdVersionMgr::FourthEditionWith2009Interims;
         else if(tmp==_T("AashtoLrfd2008"))
            m_SpecificationType = lrfdVersionMgr::FourthEditionWith2008Interims;
         else if(tmp==_T("AashtoLrfd2007"))
            m_SpecificationType = lrfdVersionMgr::FourthEdition2007;
         else if(tmp==_T("AashtoLrfd2006"))
            m_SpecificationType = lrfdVersionMgr::ThirdEditionWith2006Interims;
         else if(tmp==_T("AashtoLrfd2005"))
            m_SpecificationType = lrfdVersionMgr::ThirdEditionWith2005Interims;
         else if(tmp==_T("AashtoLrfd2004"))
            m_SpecificationType = lrfdVersionMgr::ThirdEdition2004;
         else if(tmp==_T("AashtoLrfd2003"))
            m_SpecificationType = lrfdVersionMgr::SecondEditionWith2003Interims;
         else if(tmp==_T("AashtoLrfd2002"))
            m_SpecificationType = lrfdVersionMgr::SecondEditionWith2002Interims;
         else if(tmp==_T("AashtoLrfd2001"))
            m_SpecificationType = lrfdVersionMgr::SecondEditionWith2001Interims;
         else if(tmp==_T("AashtoLrfd2000"))
            m_SpecificationType = lrfdVersionMgr::SecondEditionWith2000Interims;
         else if(tmp==_T("AashtoLrfd1999"))
            m_SpecificationType = lrfdVersionMgr::SecondEditionWith1999Interims;
         else if(tmp==_T("AashtoLrfd1998"))
            m_SpecificationType = lrfdVersionMgr::SecondEdition1998;
         else if(tmp==_T("AashtoLrfd1997"))
            m_SpecificationType = lrfdVersionMgr::FirstEditionWith1997Interims;
         else if(tmp==_T("AashtoLrfd1996"))
            m_SpecificationType = lrfdVersionMgr::FirstEditionWith1996Interims;
         else if (tmp==_T("AashtoLrfd1994"))
            m_SpecificationType = lrfdVersionMgr::FirstEdition1994;
         else
            THROW_LOAD(InvalidFileFormat,pLoad);
      }
      else
         THROW_LOAD(InvalidFileFormat,pLoad);


      if(pLoad->Property(_T("SpecificationUnits"),&tmp))
      {
         if (tmp==_T("SiUnitsSpec"))
            m_SpecificationUnits = lrfdVersionMgr::SI;
         else if(tmp==_T("UsUnitsSpec"))
            m_SpecificationUnits = lrfdVersionMgr::US;
         else
            THROW_LOAD(InvalidFileFormat,pLoad);
      }
      else
         THROW_LOAD(InvalidFileFormat,pLoad);

      if(!pLoad->Property(_T("DoCheckStrandSlope"), &m_DoCheckStrandSlope))
         THROW_LOAD(InvalidFileFormat,pLoad);

      if (version<15)
      {
         m_DoDesignStrandSlope = m_DoCheckStrandSlope;
      }
      else
      {
         if(!pLoad->Property(_T("DoDesignStrandSlope"), &m_DoDesignStrandSlope))
            THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if(!pLoad->Property(_T("MaxSlope05"), &m_MaxSlope05))
         THROW_LOAD(InvalidFileFormat,pLoad);

      if(!pLoad->Property(_T("MaxSlope06"), &m_MaxSlope06))
         THROW_LOAD(InvalidFileFormat,pLoad);

      if ( 35 <= version )
      {
         if(!pLoad->Property(_T("MaxSlope07"), &m_MaxSlope07))
            THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if(!pLoad->Property(_T("DoCheckHoldDown"), &m_DoCheckHoldDown))
         THROW_LOAD(InvalidFileFormat,pLoad);

      if (version<15)
      {
         m_DoDesignHoldDown = m_DoCheckHoldDown;
      }
      else
      {
         if(!pLoad->Property(_T("DoDesignHoldDown"), &m_DoDesignHoldDown))
            THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if(!pLoad->Property(_T("HoldDownForce"), &m_HoldDownForce))
         THROW_LOAD(InvalidFileFormat,pLoad);

      if (version>32 && version<39)
      {
         bool check_anchor;
         if(!pLoad->Property(_T("DoCheckAnchorage"), &check_anchor))
            THROW_LOAD(InvalidFileFormat,pLoad);

         m_DoCheckSplitting = check_anchor;
         m_DoCheckConfinement = check_anchor;
         m_DoDesignSplitting = check_anchor;
         m_DoDesignConfinement = check_anchor;
      }
      else if(version>=39)
      {
         if(!pLoad->Property(_T("DoCheckSplitting"), &m_DoCheckSplitting))
            THROW_LOAD(InvalidFileFormat,pLoad);

         if(!pLoad->Property(_T("DoDesignSplitting"), &m_DoDesignSplitting))
            THROW_LOAD(InvalidFileFormat,pLoad);

         if(!pLoad->Property(_T("DoCheckConfinement"), &m_DoCheckConfinement))
            THROW_LOAD(InvalidFileFormat,pLoad);

         if(!pLoad->Property(_T("DoDesignConfinement"), &m_DoDesignConfinement))
            THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if(!pLoad->Property(_T("MaxStirrupSpacing"), &m_MaxStirrupSpacing))
         THROW_LOAD(InvalidFileFormat,pLoad);

      if(!pLoad->Property(_T("CyLiftingCrackFs"), &m_CyLiftingCrackFs))
         THROW_LOAD(InvalidFileFormat,pLoad);

      if(!pLoad->Property(_T("CyLiftingFailFs"), &m_CyLiftingFailFs))
         THROW_LOAD(InvalidFileFormat,pLoad);

      if(!pLoad->Property(_T("CyCompStressServ"), &m_CyCompStressServ))
         THROW_LOAD(InvalidFileFormat,pLoad);

      if(!pLoad->Property(_T("CyCompStressLifting"), &m_CyCompStressLifting))
         THROW_LOAD(InvalidFileFormat,pLoad);

      if(!pLoad->Property(_T("CyTensStressServ"), &m_CyTensStressServ))
         THROW_LOAD(InvalidFileFormat,pLoad);

      if(!pLoad->Property(_T("CyDoTensStressServMax"), &m_CyDoTensStressServMax))
         THROW_LOAD(InvalidFileFormat,pLoad);

      if(!pLoad->Property(_T("CyTensStressServMax"), &m_CyTensStressServMax))
         THROW_LOAD(InvalidFileFormat,pLoad);

      if(!pLoad->Property(_T("CyTensStressLifting"), &m_CyTensStressLifting))
         THROW_LOAD(InvalidFileFormat,pLoad);

      if(!pLoad->Property(_T("CyDoTensStressLiftingMax"),&m_CyDoTensStressLiftingMax))
         THROW_LOAD(InvalidFileFormat,pLoad);

      if(!pLoad->Property(_T("CyTensStressLiftingMax"),&m_CyTensStressLiftingMax))
         THROW_LOAD(InvalidFileFormat,pLoad);

     // at version 27 we moved debonding to the girder library. Data here is ignored
	  if ( 6.0 <= version && 27 > version)
	  {
        Float64 debond_junk;
		  if (!pLoad->Property(_T("MaxDebondStrands"),&debond_junk))
			 THROW_LOAD(InvalidFileFormat,pLoad);

		  if (!pLoad->Property(_T("MaxDebondStrandsPerRow"),&debond_junk))
			 THROW_LOAD(InvalidFileFormat,pLoad);

        if ( !pLoad->Property(_T("MaxNumDebondedStrandsPerSection"),&debond_junk))
           THROW_LOAD(InvalidFileFormat,pLoad);

        if ( !pLoad->Property(_T("MaxDebondedStrandsPerSection"),&debond_junk))
           THROW_LOAD(InvalidFileFormat,pLoad);

        if ( !pLoad->Property(_T("DefaultDebondLength"),&debond_junk))
           THROW_LOAD(InvalidFileFormat,pLoad);
	  }

      if ( 1.4 <= version )
      {
         if (!pLoad->Property(_T("BurstingZoneLengthFactor"), &m_SplittingZoneLengthFactor))
            THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if(!pLoad->Property(_T("LiftingUpwardImpact"),&m_LiftingUpwardImpact))
         THROW_LOAD(InvalidFileFormat,pLoad);

      if(!pLoad->Property(_T("LiftingDownwardImpact"),&m_LiftingDownwardImpact))
         THROW_LOAD(InvalidFileFormat,pLoad);

      if(!pLoad->Property(_T("HaulingUpwardImpact"),&m_HaulingUpwardImpact))
         THROW_LOAD(InvalidFileFormat,pLoad);

      if(!pLoad->Property(_T("HaulingDownwardImpact"),&m_HaulingDownwardImpact))
         THROW_LOAD(InvalidFileFormat,pLoad);

      if ( !pLoad->Property(_T("CuringMethod"), &temp ) )
         THROW_LOAD(InvalidFileFormat, pLoad );
      m_CuringMethod = temp;

      if (version<11)
      {
         m_EnableLiftingCheck = true;
         m_EnableLiftingDesign = true;
      }
      else
      {
         if(!pLoad->Property(_T("EnableLiftingCheck"), &m_EnableLiftingCheck))
            THROW_LOAD(InvalidFileFormat,pLoad);

         if (version<15)
         {
            m_EnableLiftingDesign = true;
         }
         else
         {
            if(!pLoad->Property(_T("EnableLiftingDesign"), &m_EnableLiftingDesign))
               THROW_LOAD(InvalidFileFormat,pLoad);
         }
      }

      if(!pLoad->Property(_T("PickPointHeight"), &m_PickPointHeight))
         THROW_LOAD(InvalidFileFormat,pLoad);

      if(!pLoad->Property(_T("LiftingLoopTolerance"), &m_LiftingLoopTolerance))
         THROW_LOAD(InvalidFileFormat,pLoad);

      if(!pLoad->Property(_T("MinCableInclination"), &m_MinCableInclination))
         THROW_LOAD(InvalidFileFormat,pLoad);

      if(!pLoad->Property(_T("MaxGirderSweepLifting"), &m_MaxGirderSweepLifting))
         THROW_LOAD(InvalidFileFormat,pLoad);

      if (version<11)
      {
         m_EnableHaulingCheck = true;
         m_EnableHaulingDesign = true;
      }
      else
      {
         if(!pLoad->Property(_T("EnableHaulingCheck"), &m_EnableHaulingCheck))
            THROW_LOAD(InvalidFileFormat,pLoad);

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

      if(!pLoad->Property(_T("MaxGirderSweepHauling"), &m_MaxGirderSweepHauling))
         THROW_LOAD(InvalidFileFormat,pLoad);

      if(!pLoad->Property(_T("HaulingSupportDistance"), &m_HaulingSupportDistance))
         THROW_LOAD(InvalidFileFormat,pLoad);

      if ( 2.0 <= version )
      {
         if(!pLoad->Property(_T("MaxHaulingOverhang"), &m_MaxHaulingOverhang))
            THROW_LOAD(InvalidFileFormat,pLoad);
      }
      else
      {
         m_MaxHaulingOverhang = ::ConvertToSysUnits(15.0,unitMeasure::Feet);
      }

      if(!pLoad->Property(_T("HaulingSupportPlacementTolerance"), &m_HaulingSupportPlacementTolerance))
         THROW_LOAD(InvalidFileFormat,pLoad);

      if(!pLoad->Property(_T("HaulingCamberPercentEstimate"), &m_HaulingCamberPercentEstimate))
         THROW_LOAD(InvalidFileFormat,pLoad);

      if(!pLoad->Property(_T("CompStressHauling"), &m_CompStressHauling))
         THROW_LOAD(InvalidFileFormat,pLoad);

      if(!pLoad->Property(_T("TensStressHauling"), &m_TensStressHauling))
         THROW_LOAD(InvalidFileFormat,pLoad);

      if(!pLoad->Property(_T("DoTensStressHaulingMax"),&m_DoTensStressHaulingMax))
         THROW_LOAD(InvalidFileFormat,pLoad);

      if(!pLoad->Property(_T("TensStressHaulingMax"), &m_TensStressHaulingMax))
         THROW_LOAD(InvalidFileFormat,pLoad);

      if(!pLoad->Property(_T("HeHaulingCrackFs"), &m_HeHaulingCrackFs))
         THROW_LOAD(InvalidFileFormat,pLoad);

      if(!pLoad->Property(_T("HeHaulingFailFs"), &m_HeHaulingRollFs))
         THROW_LOAD(InvalidFileFormat,pLoad);

      if(!pLoad->Property(_T("RoadwaySuperelevation"), &m_RoadwaySuperelevation))
         THROW_LOAD(InvalidFileFormat,pLoad);

      if ( version < 1.9 )
      {
         if(!pLoad->Property(_T("TruckRollStiffness"), &m_TruckRollStiffness))
            THROW_LOAD(InvalidFileFormat,pLoad);

         m_TruckRollStiffnessMethod = 0;
      }
      else
      {
         int method;
         if(!pLoad->Property(_T("TruckRollStiffnessMethod"), &method))
            THROW_LOAD(InvalidFileFormat,pLoad);

         m_TruckRollStiffnessMethod = (int)method;

         if(!pLoad->Property(_T("TruckRollStiffness"), &m_TruckRollStiffness))
            THROW_LOAD(InvalidFileFormat,pLoad);

         if(!pLoad->Property(_T("AxleWeightLimit"), &m_AxleWeightLimit))
            THROW_LOAD(InvalidFileFormat,pLoad);

         if(!pLoad->Property(_T("AxleStiffness"), &m_AxleStiffness))
            THROW_LOAD(InvalidFileFormat,pLoad);

         if (!pLoad->Property(_T("MinRollStiffness"),&m_MinRollStiffness))
            THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if(!pLoad->Property(_T("TruckGirderHeight"), &m_TruckGirderHeight))
         THROW_LOAD(InvalidFileFormat,pLoad);

      if(!pLoad->Property(_T("TruckRollCenterHeight"), &m_TruckRollCenterHeight))
         THROW_LOAD(InvalidFileFormat,pLoad);

      if(!pLoad->Property(_T("TruckAxleWidth"), &m_TruckAxleWidth))
         THROW_LOAD(InvalidFileFormat,pLoad);

      if(!pLoad->Property(_T("HeErectionCrackFs"), &m_HeErectionCrackFs))
         THROW_LOAD(InvalidFileFormat,pLoad);

      if(!pLoad->Property(_T("HeErectionFailFs"), &m_HeErectionFailFs))
         THROW_LOAD(InvalidFileFormat,pLoad);

      if ( version >= 1.3 )
      {
         if (!pLoad->Property(_T("MaxGirderWgt"), &m_MaxGirderWgt))
            THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( version < 37 )
      {
         // changed in version 37
         if ( 12 <= version )
         {
            if ( !pLoad->Property(_T("HaulingModulusOfRuptureCoefficient"),&m_HaulingModulusOfRuptureCoefficient[pgsTypes::Normal]) )
               THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( 20 <= version )
         {
            if ( !pLoad->Property(_T("LiftingModulusOfRuptureCoefficient"),&m_LiftingModulusOfRuptureCoefficient[pgsTypes::Normal]) )
               THROW_LOAD(InvalidFileFormat,pLoad);
         }
      }

      if ( 25 <= version )
      {
         // Added at version 25
         if ( !pLoad->Property(_T("MinLiftingPointLocation"),&m_MinLiftPoint) )
            THROW_LOAD(InvalidFileFormat,pLoad);

         if ( !pLoad->Property(_T("LiftingPointLocationAccuracy"),&m_LiftPointAccuracy) )
            THROW_LOAD(InvalidFileFormat,pLoad);

         if ( !pLoad->Property(_T("MinHaulingSupportLocation"),&m_MinHaulPoint) )
            THROW_LOAD(InvalidFileFormat,pLoad);

         if ( !pLoad->Property(_T("HaulingSupportLocationAccuracy"),&m_HaulPointAccuracy) )
            THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( 3.2 < version )
      {
         if (!pLoad->Property(_T("CastingYardTensileStressLimitWithMildRebar"),&m_CyTensStressServWithRebar) )
             THROW_LOAD(InvalidFileFormat,pLoad);

         if (!pLoad->Property(_T("LiftingTensileStressLimitWithMildRebar"),&m_TensStressLiftingWithRebar) )
             THROW_LOAD(InvalidFileFormat,pLoad);

         if (!pLoad->Property(_T("HaulingTensileStressLimitWithMildRebar"),&m_TensStressHaulingWithRebar) )
             THROW_LOAD(InvalidFileFormat,pLoad);
      }
      else
      {
          m_CyTensStressServWithRebar  = ::ConvertToSysUnits(0.24,unitMeasure::SqrtKSI);
          m_TensStressLiftingWithRebar = ::ConvertToSysUnits(0.24,unitMeasure::SqrtKSI);
          m_TensStressHaulingWithRebar = ::ConvertToSysUnits(0.24,unitMeasure::SqrtKSI);
      }

      // deal with verson 1.1
      // in this version we added compressive and tensile stress limits for BSS 1 & 2
      // in version 1.0, limits were for all BS stages.
      if (version==1.0)
      {
         if(!pLoad->Property(_T("BsCompStressServ"), &m_Bs3CompStressServ))
            THROW_LOAD(InvalidFileFormat,pLoad);

         if(!pLoad->Property(_T("BsCompStressService1A"), &m_Bs3CompStressService1A))
            THROW_LOAD(InvalidFileFormat,pLoad);

         Float64 tmp;
         if(!pLoad->Property(_T("BsCompStressService1B"), &tmp))
            THROW_LOAD(InvalidFileFormat,pLoad);

         // stage 1 and 2 are permanent loads only
         m_Bs2CompStress = tmp;
         m_Bs1CompStress = tmp;

         if(!pLoad->Property(_T("BsTensStressServNc"), &m_Bs3TensStressServNc))
            THROW_LOAD(InvalidFileFormat,pLoad);

         m_Bs1TensStress = m_Bs3TensStressServNc;

         if(!pLoad->Property(_T("BsDoTensStressServNcMax"), &m_Bs3DoTensStressServNcMax))
            THROW_LOAD(InvalidFileFormat,pLoad);

         m_Bs1DoTensStressMax = m_Bs3DoTensStressServNcMax;

         if(!pLoad->Property(_T("BsTensStressServNcMax"), &m_Bs3TensStressServNcMax))
            THROW_LOAD(InvalidFileFormat,pLoad);

         m_Bs1TensStressMax = m_Bs3TensStressServNcMax;

         if(!pLoad->Property(_T("BsTensStressServSc"),    &m_Bs3TensStressServSc))
            THROW_LOAD(InvalidFileFormat,pLoad);
   
         if(!pLoad->Property(_T("BsDoTensStressServScMax"), &m_Bs3DoTensStressServScMax))
            THROW_LOAD(InvalidFileFormat,pLoad);

         if(!pLoad->Property(_T("BsTensStressServScMax"), &m_Bs3TensStressServScMax))
            THROW_LOAD(InvalidFileFormat,pLoad);

         if(!pLoad->Property(_T("BsMaxGirdersTrafficBarrier"), &m_Bs2MaxGirdersTrafficBarrier))
            THROW_LOAD(InvalidFileFormat,pLoad);

         if(!pLoad->Property(_T("BsMaxGirdersUtility"), &m_Bs2MaxGirdersUtility))
            THROW_LOAD(InvalidFileFormat,pLoad);

         m_TempStrandRemovalCompStress      = m_Bs1CompStress;
         m_TempStrandRemovalTensStress      = m_Bs1TensStress;
         m_TempStrandRemovalDoTensStressMax = m_Bs1DoTensStressMax;
         m_TempStrandRemovalTensStressMax   = m_Bs1TensStressMax;
      }
      else if (version>=1.1)
      {
         if ( 29 < version )
         {
            // added in version 30
            if(!pLoad->Property(_T("TempStrandRemovalCompStress") ,     &m_TempStrandRemovalCompStress))
               THROW_LOAD(InvalidFileFormat,pLoad);

            if(!pLoad->Property(_T("TempStrandRemovalTensStress") ,     &m_TempStrandRemovalTensStress))
               THROW_LOAD(InvalidFileFormat,pLoad);

            if(!pLoad->Property(_T("TempStrandRemovalDoTensStressMax"),  &m_TempStrandRemovalDoTensStressMax))
               THROW_LOAD(InvalidFileFormat,pLoad);

            if(!pLoad->Property(_T("TempStrandRemovalTensStressMax") ,  &m_TempStrandRemovalTensStressMax))
               THROW_LOAD(InvalidFileFormat,pLoad);

            // for the following 5 items, the m_ was removed from the keyword in version 30
            if(!pLoad->Property(_T("Bs1CompStress") ,     &m_Bs1CompStress))
               THROW_LOAD(InvalidFileFormat,pLoad);

            if(!pLoad->Property(_T("Bs1TensStress") ,     &m_Bs1TensStress))
               THROW_LOAD(InvalidFileFormat,pLoad);

            if(!pLoad->Property(_T("Bs1DoTensStressMax"),  &m_Bs1DoTensStressMax))
               THROW_LOAD(InvalidFileFormat,pLoad);

            if(!pLoad->Property(_T("Bs1TensStressMax") ,  &m_Bs1TensStressMax))
               THROW_LOAD(InvalidFileFormat,pLoad);

            if(!pLoad->Property(_T("Bs2CompStress") ,     &m_Bs2CompStress))
               THROW_LOAD(InvalidFileFormat,pLoad);
         }
         else
         {
            if(!pLoad->Property(_T("m_Bs1CompStress") ,     &m_Bs1CompStress))
               THROW_LOAD(InvalidFileFormat,pLoad);

            if(!pLoad->Property(_T("m_Bs1TensStress") ,     &m_Bs1TensStress))
               THROW_LOAD(InvalidFileFormat,pLoad);

            if(!pLoad->Property(_T("m_Bs1DoTensStressMax"),  &m_Bs1DoTensStressMax))
               THROW_LOAD(InvalidFileFormat,pLoad);

            if(!pLoad->Property(_T("m_Bs1TensStressMax") ,  &m_Bs1TensStressMax))
               THROW_LOAD(InvalidFileFormat,pLoad);

            if(!pLoad->Property(_T("m_Bs2CompStress") ,     &m_Bs2CompStress))
               THROW_LOAD(InvalidFileFormat,pLoad);

            m_TempStrandRemovalCompStress      = m_Bs1CompStress;
            m_TempStrandRemovalTensStress      = m_Bs1TensStress;
            m_TempStrandRemovalDoTensStressMax = m_Bs1DoTensStressMax;
            m_TempStrandRemovalTensStressMax   = m_Bs1TensStressMax;
         }

         if ( version < 5.0 )
         {
             // this parameters were removed.... just gobble them up if this is an older file
             Float64 dummy;
             if(!pLoad->Property(_T("m_Bs2TensStress") ,     &dummy))
            THROW_LOAD(InvalidFileFormat,pLoad);

             if(!pLoad->Property(_T("m_Bs2DoTensStressMax"),  &dummy))
            THROW_LOAD(InvalidFileFormat,pLoad);

             if(!pLoad->Property(_T("m_Bs2TensStressMax") ,  &dummy))
            THROW_LOAD(InvalidFileFormat,pLoad);
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
            THROW_LOAD(InvalidFileFormat,pLoad);

         if(!pLoad->Property(_T("Bs2MaxGirdersUtility"), &m_Bs2MaxGirdersUtility))
            THROW_LOAD(InvalidFileFormat,pLoad);

         if ( version > 33.0 )
         {
            int oldt;
            if(!pLoad->Property(_T("OverlayLoadDistribution"), &oldt))
               THROW_LOAD(InvalidFileFormat,pLoad);

            m_OverlayLoadDistribution = (pgsTypes::OverlayLoadDistributionType)oldt==pgsTypes::olDistributeTributaryWidth ? 
                                        pgsTypes::olDistributeTributaryWidth : pgsTypes::olDistributeEvenly;
         }

         if(!pLoad->Property(_T("Bs3CompStressServ"), &m_Bs3CompStressServ))
            THROW_LOAD(InvalidFileFormat,pLoad);

         if(!pLoad->Property(_T("Bs3CompStressService1A"), &m_Bs3CompStressService1A))
            THROW_LOAD(InvalidFileFormat,pLoad);

         if(!pLoad->Property(_T("Bs3TensStressServNc"), &m_Bs3TensStressServNc))
            THROW_LOAD(InvalidFileFormat,pLoad);

         if(!pLoad->Property(_T("Bs3DoTensStressServNcMax"), &m_Bs3DoTensStressServNcMax))
            THROW_LOAD(InvalidFileFormat,pLoad);

         if(!pLoad->Property(_T("Bs3TensStressServNcMax"), &m_Bs3TensStressServNcMax))
            THROW_LOAD(InvalidFileFormat,pLoad);

         if(!pLoad->Property(_T("Bs3TensStressServSc"),    &m_Bs3TensStressServSc))
            THROW_LOAD(InvalidFileFormat,pLoad);

         if(!pLoad->Property(_T("Bs3DoTensStressServScMax"), &m_Bs3DoTensStressServScMax))
            THROW_LOAD(InvalidFileFormat,pLoad);

         if(!pLoad->Property(_T("Bs3TensStressServScMax"), &m_Bs3TensStressServScMax))
            THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( version >= 1.4 )
      {
         if ( version < 29 )
         {
            // removed in version 29
            if (!pLoad->Property(_T("Bs3IgnoreRangeOfApplicability"), &m_Bs3IgnoreRangeOfApplicability))
               THROW_LOAD(InvalidFileFormat,pLoad);
         }

         // this parameter removed in version 18
         if ( version < 18 )
         {
            if (!pLoad->Property(_T("Bs3LRFDShear"),&temp))
               THROW_LOAD(InvalidFileFormat,pLoad);

            switch(temp)
            {
            case 0: // scmGeneral -> scmBTTables
               shear_capacity_method = scmBTTables;
               break;

            case 1: // scmWSDOT -> scmWSDOT2001
               shear_capacity_method = scmWSDOT2001;
               break;
               
            case 2: // scmSimplified -> scmVciVcw
               shear_capacity_method = scmVciVcw;
               break;

            default:
               ATLASSERT(false);
            }
         }
      }
      else
      {
         shear_capacity_method = scmBTTables;
      }

      if ( version < 37 )
      {
         if ( version >= 1.8 )
         {
            if (!pLoad->Property(_T("Bs3LRFDOverreinforcedMomentCapacity"),&temp))
               THROW_LOAD(InvalidFileFormat,pLoad);
            m_Bs3LRFDOverReinforcedMomentCapacity = temp;
         }
      
         if ( version >= 7.0 )
         {
            if (!pLoad->Property(_T("IncludeRebar_MomentCapacity"),&temp))
               THROW_LOAD(InvalidFileFormat,pLoad);

            m_bIncludeRebar_Moment = (temp == 0 ? false : true);
         }
      }
      else
      {
         if ( !pLoad->BeginUnit(_T("MomentCapacity")) )
            THROW_LOAD(InvalidFileFormat,pLoad);

            Float64 mc_version = pLoad->GetVersion();

            if ( !pLoad->Property(_T("Bs3LRFDOverreinforcedMomentCapacity"),&temp) )
               THROW_LOAD(InvalidFileFormat,pLoad);

            m_Bs3LRFDOverReinforcedMomentCapacity = temp;

            if ( !pLoad->Property(_T("IncludeRebarForCapacity"),&temp) )
               THROW_LOAD(InvalidFileFormat,pLoad);

            m_bIncludeRebar_Moment = (temp == 0 ? false : true);

            if ( 2 <= mc_version )
            {
               if ( !pLoad->Property(_T("IncludeNoncompositeMomentForNegMomentDesign"),&temp) ) // added version 2 of this data block
                  THROW_LOAD(InvalidFileFormat,pLoad);

               m_bIncludeForNegMoment = (temp == 0 ? false : true);
            }

            if ( !pLoad->BeginUnit(_T("ReductionFactor")) )
               THROW_LOAD(InvalidFileFormat,pLoad);

            if ( !pLoad->BeginUnit(_T("NormalWeight")) )
               THROW_LOAD(InvalidFileFormat,pLoad);

            if ( !pLoad->Property(_T("TensionControlled_RC"),&m_PhiFlexureTensionRC[pgsTypes::Normal]) )
               THROW_LOAD(InvalidFileFormat,pLoad);

            if ( !pLoad->Property(_T("TensionControlled_PS"),&m_PhiFlexureTensionPS[pgsTypes::Normal]) )
               THROW_LOAD(InvalidFileFormat,pLoad);

            if ( !pLoad->Property(_T("CompressionControlled"),&m_PhiFlexureCompression[pgsTypes::Normal]) )
               THROW_LOAD(InvalidFileFormat,pLoad);

             if ( !pLoad->EndUnit() ) // NormalWeight
               THROW_LOAD(InvalidFileFormat,pLoad);

            if ( !pLoad->BeginUnit(_T("AllLightweight")) )
               THROW_LOAD(InvalidFileFormat,pLoad);

            if ( !pLoad->Property(_T("TensionControlled_RC"),&m_PhiFlexureTensionRC[pgsTypes::AllLightweight]) )
               THROW_LOAD(InvalidFileFormat,pLoad);

            if ( !pLoad->Property(_T("TensionControlled_PS"),&m_PhiFlexureTensionPS[pgsTypes::AllLightweight]) )
               THROW_LOAD(InvalidFileFormat,pLoad);

            if ( !pLoad->Property(_T("CompressionControlled"),&m_PhiFlexureCompression[pgsTypes::AllLightweight]) )
               THROW_LOAD(InvalidFileFormat,pLoad);

             if ( !pLoad->EndUnit() ) // AllLightweight
               THROW_LOAD(InvalidFileFormat,pLoad);


            if ( !pLoad->BeginUnit(_T("SandLightweight")) )
               THROW_LOAD(InvalidFileFormat,pLoad);

            if ( !pLoad->Property(_T("TensionControlled_RC"),&m_PhiFlexureTensionRC[pgsTypes::SandLightweight]) )
               THROW_LOAD(InvalidFileFormat,pLoad);

            if ( !pLoad->Property(_T("TensionControlled_PS"),&m_PhiFlexureTensionPS[pgsTypes::SandLightweight]) )
               THROW_LOAD(InvalidFileFormat,pLoad);

            if ( !pLoad->Property(_T("CompressionControlled"),&m_PhiFlexureCompression[pgsTypes::SandLightweight]) )
               THROW_LOAD(InvalidFileFormat,pLoad);
            
             if ( !pLoad->EndUnit() ) // SandLightweight
               THROW_LOAD(InvalidFileFormat,pLoad);

             if ( !pLoad->EndUnit() ) // ReductionFactor
               THROW_LOAD(InvalidFileFormat,pLoad);

        if ( !pLoad->EndUnit() ) // MomentCapacity
            THROW_LOAD(InvalidFileFormat,pLoad);
      }


      if ( version < 37 )
      {
         if ( version >= 9.0 )
         {
            if (!pLoad->Property(_T("ModulusOfRuptureCoefficient"),&m_FlexureModulusOfRuptureCoefficient[pgsTypes::Normal]))
               THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( 18 <= version && version )
         {
            // added in version 18
            if ( !pLoad->Property(_T("ShearModulusOfRuptureCoefficient"),&m_ShearModulusOfRuptureCoefficient[pgsTypes::Normal])) 
               THROW_LOAD(InvalidFileFormat,pLoad);
         }
      }
      else 
      {
         if ( !pLoad->BeginUnit(_T("ModulusOfRuptureCoefficient")) )
            THROW_LOAD(InvalidFileFormat,pLoad);

         if ( !pLoad->BeginUnit(_T("Moment")) )
            THROW_LOAD(InvalidFileFormat,pLoad);

         if ( !pLoad->Property(_T("Normal"),&m_FlexureModulusOfRuptureCoefficient[pgsTypes::Normal]) )
            THROW_LOAD(InvalidFileFormat,pLoad);

         if ( !pLoad->Property(_T("AllLightweight"),&m_FlexureModulusOfRuptureCoefficient[pgsTypes::AllLightweight]) )
            THROW_LOAD(InvalidFileFormat,pLoad);

         if ( !pLoad->Property(_T("SandLightweight"),&m_FlexureModulusOfRuptureCoefficient[pgsTypes::SandLightweight]) )
            THROW_LOAD(InvalidFileFormat,pLoad);

         if ( !pLoad->EndUnit() ) // Moment
            THROW_LOAD(InvalidFileFormat,pLoad);

         if ( !pLoad->BeginUnit(_T("Shear")) )
            THROW_LOAD(InvalidFileFormat,pLoad);

         if ( !pLoad->Property(_T("Normal"),&m_ShearModulusOfRuptureCoefficient[pgsTypes::Normal]) )
            THROW_LOAD(InvalidFileFormat,pLoad);

         if ( !pLoad->Property(_T("AllLightweight"),&m_ShearModulusOfRuptureCoefficient[pgsTypes::AllLightweight]) )
            THROW_LOAD(InvalidFileFormat,pLoad);

         if ( !pLoad->Property(_T("SandLightweight"),&m_ShearModulusOfRuptureCoefficient[pgsTypes::SandLightweight]) )
            THROW_LOAD(InvalidFileFormat,pLoad);

         if ( !pLoad->EndUnit() ) // Shear
            THROW_LOAD(InvalidFileFormat,pLoad);

         if ( !pLoad->BeginUnit(_T("Lifting")) )
            THROW_LOAD(InvalidFileFormat,pLoad);

         if ( !pLoad->Property(_T("Normal"),&m_LiftingModulusOfRuptureCoefficient[pgsTypes::Normal]) )
            THROW_LOAD(InvalidFileFormat,pLoad);

         if ( !pLoad->Property(_T("AllLightweight"),&m_LiftingModulusOfRuptureCoefficient[pgsTypes::AllLightweight]) )
            THROW_LOAD(InvalidFileFormat,pLoad);

         if ( !pLoad->Property(_T("SandLightweight"),&m_LiftingModulusOfRuptureCoefficient[pgsTypes::SandLightweight]) )
            THROW_LOAD(InvalidFileFormat,pLoad);

         if ( !pLoad->EndUnit() ) // Lifting
            THROW_LOAD(InvalidFileFormat,pLoad);


         if ( !pLoad->BeginUnit(_T("Shipping")) )
            THROW_LOAD(InvalidFileFormat,pLoad);

         if ( !pLoad->Property(_T("Normal"),&m_HaulingModulusOfRuptureCoefficient[pgsTypes::Normal]) )
            THROW_LOAD(InvalidFileFormat,pLoad);

         if ( !pLoad->Property(_T("AllLightweight"),&m_HaulingModulusOfRuptureCoefficient[pgsTypes::AllLightweight]) )
            THROW_LOAD(InvalidFileFormat,pLoad);

         if ( !pLoad->Property(_T("SandLightweight"),&m_HaulingModulusOfRuptureCoefficient[pgsTypes::SandLightweight]) )
            THROW_LOAD(InvalidFileFormat,pLoad);

         if ( !pLoad->EndUnit() ) // Shipping
            THROW_LOAD(InvalidFileFormat,pLoad);

         if ( !pLoad->EndUnit() ) // ModulusOfRuptureCoefficient
            THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( !pLoad->Property(_T("BsLldfMethod"), &temp ) )
         THROW_LOAD(InvalidFileFormat,pLoad);
      m_LldfMethod = temp;

      if ( 28 < version )
      {
         // added in version 29
         if ( !pLoad->Property(_T("MaxAngularDeviationBetweenGirders"),&m_MaxAngularDeviationBetweenGirders) )
            THROW_LOAD(InvalidFileFormat,pLoad);
   
         if ( !pLoad->Property(_T("MinGirderStiffnessRatio"),&m_MinGirderStiffnessRatio) )
            THROW_LOAD(InvalidFileFormat,pLoad);

         if ( !pLoad->Property(_T("LLDFGirderSpacingLocation"),&m_LLDFGirderSpacingLocation) )
            THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( 30 < version )
      {
         if ( !pLoad->Property(_T("LimitDistributionFactorsToLanesBeams"),&m_LimitDistributionFactorsToLanesBeams) )
            THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( version < 37 )
      {
         // moved below in version 37

         // added longitudinal reinforcement for shear method to version 1.2 (this was the only change)
         if (version >= 1.2)
         {
            if ( !pLoad->Property(_T("LongReinfShearMethod"), &temp ) )
               THROW_LOAD(InvalidFileFormat,pLoad);
            m_LongReinfShearMethod = temp;

            // WSDOT method has been recinded
            m_LongReinfShearMethod = LRFD_METHOD;
         }
         else
         {
            m_LongReinfShearMethod = LRFD_METHOD;
         }
 
   
         if ( version >= 7.0 )
         {
            if (!pLoad->Property(_T("IncludeRebar_Shear"),&temp))
               THROW_LOAD(InvalidFileFormat,pLoad);

            m_bIncludeRebar_Shear = (temp == 0 ? false : true);
         }
      }

      if ( !pLoad->Property(_T("CreepMethod"),&temp) )
         THROW_LOAD(InvalidFileFormat,pLoad);
      m_CreepMethod = temp;

      if (!pLoad->Property(_T("CreepFactor"),&m_CreepFactor))
         THROW_LOAD(InvalidFileFormat,pLoad);

      // WSDOT has abandonded it's creep method and use the LRFD calculations
      // 2006
      m_CreepMethod = CREEP_LRFD;

      if ( 3.0 <= version )
      {
         if (!pLoad->Property(_T("CreepDuration1Min"),&m_CreepDuration1Min))
            THROW_LOAD(InvalidFileFormat,pLoad);

         if (!pLoad->Property(_T("CreepDuration1Max"),&m_CreepDuration1Max))
            THROW_LOAD(InvalidFileFormat,pLoad);

         if (!pLoad->Property(_T("CreepDuration2Min"),&m_CreepDuration2Min))
            THROW_LOAD(InvalidFileFormat,pLoad);

         if (!pLoad->Property(_T("CreepDuration2Max"),&m_CreepDuration2Max))
            THROW_LOAD(InvalidFileFormat,pLoad);

         if ( !pLoad->Property(_T("XferTime"),&m_XferTime))
            THROW_LOAD(InvalidFileFormat,pLoad);

         if ( 3.1 <= version && version < 3.2 )
         {
            if ( !pLoad->Property(_T("NoncompositeCreepDuration"),&m_TotalCreepDuration))
               THROW_LOAD(InvalidFileFormat,pLoad);
         }
         if ( 3.2 <= version )
         {
            if ( !pLoad->Property(_T("TotalCreepDuration"),&m_TotalCreepDuration))
               THROW_LOAD(InvalidFileFormat,pLoad);
         }
      }
      else if (1.6 <= version && version < 3.0)
      {
         if (!pLoad->Property(_T("CreepDuration1"),&m_CreepDuration1Min))
            THROW_LOAD(InvalidFileFormat,pLoad);

         m_CreepDuration1Max = m_CreepDuration1Min;

         if (!pLoad->Property(_T("CreepDuration2"),&m_CreepDuration2Min))
            THROW_LOAD(InvalidFileFormat,pLoad);

         m_CreepDuration2Max = m_CreepDuration2Min;

         if ( !pLoad->Property(_T("XferTime"),&m_XferTime))
            THROW_LOAD(InvalidFileFormat,pLoad);
      }
      else
      {
         // only one creep duration was entered - need to make some assumptions here
         if (!pLoad->Property(_T("CreepDuration"),&m_CreepDuration2Min))
            THROW_LOAD(InvalidFileFormat,pLoad);

         m_CreepDuration2Max = m_CreepDuration2Min;

         if ( !pLoad->Property(_T("XferTime"),&m_XferTime))
            THROW_LOAD(InvalidFileFormat,pLoad);

         Float64 duration_days = ::ConvertFromSysUnits(m_CreepDuration2Min,unitMeasure::Day);
         Float64 xfer_days     = ::ConvertFromSysUnits(m_XferTime,unitMeasure::Day);
         if (duration_days-30 > xfer_days)
         {
            m_CreepDuration1Min = ::ConvertToSysUnits(duration_days-30,unitMeasure::Day);
            m_CreepDuration1Max = m_CreepDuration1Min;
         }
         else
         {
            m_CreepDuration1Min = (m_CreepDuration2Min+m_XferTime) / 2.0;
            m_CreepDuration1Max = m_CreepDuration1Min;
         }
      }
 
      if ( !pLoad->Property(_T("LossMethod"),&temp) )
         THROW_LOAD(InvalidFileFormat,pLoad );
      m_LossMethod = temp;

      if ( !pLoad->Property(_T("FinalLosses"),&m_FinalLosses) )
         THROW_LOAD(InvalidFileFormat,pLoad );

      if ( !pLoad->Property(_T("ShippingLosses"),&m_ShippingLosses) )
         THROW_LOAD(InvalidFileFormat,pLoad );

      if ( !pLoad->Property(_T("BeforeXferLosses"),&m_BeforeXferLosses) )
         THROW_LOAD(InvalidFileFormat,pLoad );

      if ( !pLoad->Property(_T("AfterXferLosses"),&m_AfterXferLosses) )
         THROW_LOAD(InvalidFileFormat,pLoad );

      if ( 13.0 <= version )
      {
         if ( !pLoad->Property(_T("ShippingTime"),&m_ShippingTime) )
            THROW_LOAD(InvalidFileFormat,pLoad );
      }

      if ( 22 <= version )
      {
         if ( !pLoad->Property(_T("LiftingLosses"),&m_LiftingLosses) )
            THROW_LOAD(InvalidFileFormat,pLoad);

         if ( !pLoad->Property(_T("BeforeTempStrandRemovalLosses"),&m_BeforeTempStrandRemovalLosses) )
            THROW_LOAD(InvalidFileFormat,pLoad);

         if ( !pLoad->Property(_T("AfterTempStrandRemovalLosses"),&m_AfterTempStrandRemovalLosses) )
            THROW_LOAD(InvalidFileFormat,pLoad);

         if ( !pLoad->Property(_T("AfterDeckPlacementLosses"),&m_AfterDeckPlacementLosses) )
            THROW_LOAD(InvalidFileFormat,pLoad);

         if ( 38 <= version )
         {
            if ( !pLoad->Property(_T("AfterSIDLLosses"),&m_AfterSIDLLosses) )
               THROW_LOAD(InvalidFileFormat,pLoad);
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

      if ( 19 <= version )
      {
         if ( !pLoad->Property(_T("CuringMethodFactor"),&m_CuringMethodTimeAdjustmentFactor) )
            THROW_LOAD(InvalidFileFormat,pLoad);
      }

      // added in version 1.6
      if ( version >= 1.5 )
      {
         if (!pLoad->Property(_T("CheckStrandStressAtJacking"),&m_bCheckStrandStress[AT_JACKING]))
            THROW_LOAD(InvalidFileFormat,pLoad);

         if (!pLoad->Property(_T("Coeff_AtJacking_StressRel"),&m_StrandStressCoeff[AT_JACKING][STRESS_REL]))
            THROW_LOAD(InvalidFileFormat,pLoad);

         if (!pLoad->Property(_T("Coeff_AtJacking_LowRelax"),&m_StrandStressCoeff[AT_JACKING][LOW_RELAX]))
            THROW_LOAD(InvalidFileFormat,pLoad);


         if (!pLoad->Property(_T("CheckStrandStressBeforeTransfer"),&m_bCheckStrandStress[BEFORE_TRANSFER]))
            THROW_LOAD(InvalidFileFormat,pLoad);

         if (!pLoad->Property(_T("Coeff_BeforeTransfer_StressRel"),&m_StrandStressCoeff[BEFORE_TRANSFER][STRESS_REL]))
            THROW_LOAD(InvalidFileFormat,pLoad);

         if (!pLoad->Property(_T("Coeff_BeforeTransfer_LowRelax"),&m_StrandStressCoeff[BEFORE_TRANSFER][LOW_RELAX]))
            THROW_LOAD(InvalidFileFormat,pLoad);


         if (!pLoad->Property(_T("CheckStrandStressAfterTransfer"),&m_bCheckStrandStress[AFTER_TRANSFER]))
            THROW_LOAD(InvalidFileFormat,pLoad);

         if (!pLoad->Property(_T("Coeff_AfterTransfer_StressRel"),&m_StrandStressCoeff[AFTER_TRANSFER][STRESS_REL]))
            THROW_LOAD(InvalidFileFormat,pLoad);

         if (!pLoad->Property(_T("Coeff_AfterTransfer_LowRelax"),&m_StrandStressCoeff[AFTER_TRANSFER][LOW_RELAX]))
            THROW_LOAD(InvalidFileFormat,pLoad);


         if (!pLoad->Property(_T("CheckStrandStressAfterAllLosses"),&m_bCheckStrandStress[AFTER_ALL_LOSSES]))
            THROW_LOAD(InvalidFileFormat,pLoad);

         if (!pLoad->Property(_T("Coeff_AfterAllLosses_StressRel"),&m_StrandStressCoeff[AFTER_ALL_LOSSES][STRESS_REL]))
            THROW_LOAD(InvalidFileFormat,pLoad);

         if (!pLoad->Property(_T("Coeff_AfterAllLosses_LowRelax"),&m_StrandStressCoeff[AFTER_ALL_LOSSES][LOW_RELAX]))
            THROW_LOAD(InvalidFileFormat,pLoad);

      }
      else
      {
         switch( m_SpecificationType )
         {
         case lrfdVersionMgr::FirstEdition1994:
            m_bCheckStrandStress[AT_JACKING]       = true;
            m_bCheckStrandStress[BEFORE_TRANSFER]  = false;
            m_bCheckStrandStress[AFTER_TRANSFER]   = true;
            m_bCheckStrandStress[AFTER_ALL_LOSSES] = true;
            break;

         case lrfdVersionMgr::FirstEditionWith1996Interims:
         case lrfdVersionMgr::FirstEditionWith1997Interims:
         case lrfdVersionMgr::SecondEdition1998:
         case lrfdVersionMgr::SecondEditionWith1999Interims:
         case lrfdVersionMgr::SecondEditionWith2000Interims:
         case lrfdVersionMgr::SecondEditionWith2001Interims:
            m_bCheckStrandStress[AT_JACKING]       = false;
            m_bCheckStrandStress[BEFORE_TRANSFER]  = true;
            m_bCheckStrandStress[AFTER_TRANSFER]   = false;
            m_bCheckStrandStress[AFTER_ALL_LOSSES] = true;
            break;
         }
      }

      if ( 22 < version )
      {
         // added in version 23
         if ( !pLoad->Property(_T("AnchorSet"),&m_Dset) )
            THROW_LOAD(InvalidFileFormat,pLoad);

         if ( !pLoad->Property(_T("WobbleFriction"),&m_WobbleFriction) )
            THROW_LOAD(InvalidFileFormat,pLoad);

         if ( !pLoad->Property(_T("CoefficientOfFriction"),&m_FrictionCoefficient) )
            THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( 39 < version )
      {
         // added in version 40
         if ( !pLoad->Property(_T("RelaxationLossMethod"),&m_RelaxationLossMethod) )
            THROW_LOAD(InvalidFileFormat,pLoad);

         if ( !pLoad->Property(_T("SlabElasticGain"),&m_SlabElasticGain) )
            THROW_LOAD(InvalidFileFormat,pLoad);

         if ( !pLoad->Property(_T("HaunchElasticGain"),&m_SlabPadElasticGain) )
            THROW_LOAD(InvalidFileFormat,pLoad);

         if ( !pLoad->Property(_T("DiaphragmElasticGain"),&m_DiaphragmElasticGain) )
            THROW_LOAD(InvalidFileFormat,pLoad);

         if ( !pLoad->Property(_T("UserDCElasticGainBS1"),&m_UserDCElasticGainBS1) )
            THROW_LOAD(InvalidFileFormat,pLoad);

         if ( !pLoad->Property(_T("UserDWElasticGainBS1"),&m_UserDWElasticGainBS1) )
            THROW_LOAD(InvalidFileFormat,pLoad);

         if ( !pLoad->Property(_T("UserDCElasticGainBS2"),&m_UserDCElasticGainBS2) )
            THROW_LOAD(InvalidFileFormat,pLoad);

         if ( !pLoad->Property(_T("UserDWElasticGainBS2"),&m_UserDWElasticGainBS2) )
            THROW_LOAD(InvalidFileFormat,pLoad);

         if ( !pLoad->Property(_T("RailingSystemElasticGain"),&m_RailingSystemElasticGain) )
            THROW_LOAD(InvalidFileFormat,pLoad);

         if ( !pLoad->Property(_T("OverlayElasticGain"),&m_OverlayElasticGain) )
            THROW_LOAD(InvalidFileFormat,pLoad);

         if ( !pLoad->Property(_T("SlabShrinkageElasticGain"),&m_SlabShrinkageElasticGain) )
            THROW_LOAD(InvalidFileFormat,pLoad);

         if ( 41 < version )
         {
            // added version 42
            if ( !pLoad->Property(_T("LiveLoadElasticGain"),&m_LiveLoadElasticGain) )
               THROW_LOAD(InvalidFileFormat,pLoad);
         }
      }

      // added in version 1.7
      if ( 1.6 < version )
      {
         if (!pLoad->Property(_T("CheckLiveLoadDeflection"),&m_bDoEvaluateDeflection))
            THROW_LOAD(InvalidFileFormat,pLoad);

         if (!pLoad->Property(_T("LiveLoadDeflectionLimit"),&m_DeflectionLimit))
            THROW_LOAD(InvalidFileFormat,pLoad);

      }
      else
      {
         m_bDoEvaluateDeflection = true;
         m_DeflectionLimit = 800.0;
      }

      // added in version 8.0 and removed in version 28
      if ( 8.0 <= version && version < 28 )
      {
         int value;
         if ( !pLoad->Property(_T("AnalysisType"),&value) )
            THROW_LOAD(InvalidFileFormat,pLoad);

         m_AnalysisType = (pgsTypes::AnalysisType)(value);
      }

      if ( 10.0 <= version && version < 37 )
      {
         if ( !pLoad->Property(_T("MaxSlabFc"),&m_MaxSlabFc[pgsTypes::Normal]) )
            THROW_LOAD(InvalidFileFormat,pLoad);

         if ( !pLoad->Property(_T("MaxGirderFci"),&m_MaxGirderFci[pgsTypes::Normal]) )
            THROW_LOAD(InvalidFileFormat,pLoad);

         if ( !pLoad->Property(_T("MaxGirderFc"),&m_MaxGirderFc[pgsTypes::Normal]) )
            THROW_LOAD(InvalidFileFormat,pLoad);

         if ( !pLoad->Property(_T("MaxConcreteUnitWeight"),&m_MaxConcreteUnitWeight[pgsTypes::Normal]) )
            THROW_LOAD(InvalidFileFormat,pLoad);

         if ( !pLoad->Property(_T("MaxConcreteAggSize"),&m_MaxConcreteAggSize[pgsTypes::Normal]) )
            THROW_LOAD(InvalidFileFormat,pLoad);
      }
      else if ( 37 <= version )
      {
         if ( !pLoad->BeginUnit(_T("Limits")) )
            THROW_LOAD(InvalidFileFormat,pLoad);

            if ( !pLoad->BeginUnit(_T("Normal")) )
               THROW_LOAD(InvalidFileFormat,pLoad);

               if ( !pLoad->Property(_T("MaxSlabFc"),&m_MaxSlabFc[pgsTypes::Normal]) )
                  THROW_LOAD(InvalidFileFormat,pLoad);

               if ( !pLoad->Property(_T("MaxGirderFci"),&m_MaxGirderFci[pgsTypes::Normal]) )
                  THROW_LOAD(InvalidFileFormat,pLoad);

               if ( !pLoad->Property(_T("MaxGirderFc"),&m_MaxGirderFc[pgsTypes::Normal]) )
                  THROW_LOAD(InvalidFileFormat,pLoad);

               if ( !pLoad->Property(_T("MaxConcreteUnitWeight"),&m_MaxConcreteUnitWeight[pgsTypes::Normal]) )
                  THROW_LOAD(InvalidFileFormat,pLoad);

               if ( !pLoad->Property(_T("MaxConcreteAggSize"),&m_MaxConcreteAggSize[pgsTypes::Normal]) )
                  THROW_LOAD(InvalidFileFormat,pLoad);

            if ( !pLoad->EndUnit() ) // Normal
               THROW_LOAD(InvalidFileFormat,pLoad);


            if ( !pLoad->BeginUnit(_T("AllLightweight")) )
               THROW_LOAD(InvalidFileFormat,pLoad);

               if ( !pLoad->Property(_T("MaxSlabFc"),&m_MaxSlabFc[pgsTypes::AllLightweight]) )
                  THROW_LOAD(InvalidFileFormat,pLoad);

               if ( !pLoad->Property(_T("MaxGirderFci"),&m_MaxGirderFci[pgsTypes::AllLightweight]) )
                  THROW_LOAD(InvalidFileFormat,pLoad);

               if ( !pLoad->Property(_T("MaxGirderFc"),&m_MaxGirderFc[pgsTypes::AllLightweight]) )
                  THROW_LOAD(InvalidFileFormat,pLoad);

               if ( !pLoad->Property(_T("MaxConcreteUnitWeight"),&m_MaxConcreteUnitWeight[pgsTypes::AllLightweight]) )
                  THROW_LOAD(InvalidFileFormat,pLoad);

               if ( !pLoad->Property(_T("MaxConcreteAggSize"),&m_MaxConcreteAggSize[pgsTypes::AllLightweight]) )
                  THROW_LOAD(InvalidFileFormat,pLoad);

            if ( !pLoad->EndUnit() ) // AllLightweight
               THROW_LOAD(InvalidFileFormat,pLoad);

            if ( !pLoad->BeginUnit(_T("SandLightweight")) )
               THROW_LOAD(InvalidFileFormat,pLoad);

               if ( !pLoad->Property(_T("MaxSlabFc"),&m_MaxSlabFc[pgsTypes::SandLightweight]) )
                  THROW_LOAD(InvalidFileFormat,pLoad);

               if ( !pLoad->Property(_T("MaxGirderFci"),&m_MaxGirderFci[pgsTypes::SandLightweight]) )
                  THROW_LOAD(InvalidFileFormat,pLoad);

               if ( !pLoad->Property(_T("MaxGirderFc"),&m_MaxGirderFc[pgsTypes::SandLightweight]) )
                  THROW_LOAD(InvalidFileFormat,pLoad);

               if ( !pLoad->Property(_T("MaxConcreteUnitWeight"),&m_MaxConcreteUnitWeight[pgsTypes::SandLightweight]) )
                  THROW_LOAD(InvalidFileFormat,pLoad);

               if ( !pLoad->Property(_T("MaxConcreteAggSize"),&m_MaxConcreteAggSize[pgsTypes::SandLightweight]) )
                  THROW_LOAD(InvalidFileFormat,pLoad);

            if ( !pLoad->EndUnit() ) // SandLightweight
               THROW_LOAD(InvalidFileFormat,pLoad);

         if ( !pLoad->EndUnit() ) // Limits
            THROW_LOAD(InvalidFileFormat,pLoad);
      }

      // Added in 14.0, removed in version 41
      if ( 14.0 <= version && version < 41)
      {
         std::_tstring strLimitState[] = {_T("ServiceI"),_T("ServiceIA"),_T("ServiceIII"),_T("StrengthI"),_T("StrengthII"),_T("FatigueI")};

         pLoad->BeginUnit(_T("LoadFactors"));
         Float64 lf_version = pLoad->GetVersion();
         int nLimitStates = 4;
         if ( 2 <= lf_version  )
            nLimitStates += 1; // added Strength II

         if ( 3 <= lf_version )
            nLimitStates += 1; // added Fatigue I

         for ( int i = 0; i < nLimitStates; i++ )
         {
            pLoad->BeginUnit(strLimitState[i].c_str());

            Float64 dcMin, dcMax, dwMin, dwMax, llimMin, llimMax;
            pLoad->Property(_T("DCmin"),  &dcMin);
            pLoad->Property(_T("DCmax"),  &dcMax);
            pLoad->Property(_T("DWmin"),  &dwMin);
            pLoad->Property(_T("DWmax"),  &dwMax);
            pLoad->Property(_T("LLIMmin"),&llimMin);
            pLoad->Property(_T("LLIMmax"),&llimMax);

            const libILibrary* pLib = GetLibrary();
            psgProjectLibraryManager* pLibMgr = dynamic_cast<psgProjectLibraryManager*>(pLib->GetLibraryManager());
            if ( pLibMgr )
            {
               // the cast above is successful if we are loading the library entry that is in use.... capture
               // the data so that we don't change the user's actual data.
               pLibMgr->m_DCmin[i]   = dcMin;
               pLibMgr->m_DWmin[i]   = dwMin;
               pLibMgr->m_LLIMmin[i] = llimMin;
               pLibMgr->m_DCmax[i]   = dcMax;
               pLibMgr->m_DWmax[i]   = dwMax;
               pLibMgr->m_LLIMmax[i] = llimMax;
            }

            pLoad->EndUnit();
         }
         pLoad->EndUnit();
      }

      // added in version 15
      if (version<15)
      {
         m_EnableSlabOffsetCheck = true;
         m_EnableSlabOffsetDesign = true;
      }
      else
      {
         if(!pLoad->Property(_T("EnableSlabOffsetCheck"), &m_EnableSlabOffsetCheck))
            THROW_LOAD(InvalidFileFormat,pLoad);

         if(!pLoad->Property(_T("EnableSlabOffsetDesign"), &m_EnableSlabOffsetDesign))
            THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if (version<15)
      {
         m_DesignStrandFillType = ftMinimizeHarping;
      }
      else
      {
         int ftype;
         if(!pLoad->Property(_T("DesignStrandFillType"), &ftype))
            THROW_LOAD(InvalidFileFormat,pLoad);

         m_DesignStrandFillType = ftype==(long)ftGridOrder ? ftGridOrder : ftMinimizeHarping;
      }

      if ( 16 <= version )
      {
         int value;
         if ( !pLoad->Property(_T("EffectiveFlangeWidthMethod"),&value) )
            THROW_LOAD(InvalidFileFormat,pLoad);

         m_EffFlangeWidthMethod = (pgsTypes::EffectiveFlangeWidthMethod)(value);
      }

      // only a valid input between data block version 17 and 23
      if ( 17 <= version && version <= 23)
      {
         int value;
         if ( !pLoad->Property(_T("SlabOffsetMethod"),&value) )
            THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( 18 <= version && version < 37 )
      {
         int value;
         if ( !pLoad->Property(_T("ShearFlowMethod"),&value) )
            THROW_LOAD(InvalidFileFormat,pLoad);

         m_ShearFlowMethod = (ShearFlowMethod)(value);

         if ( !pLoad->Property(_T("ShearCapacityMethod"),&value) )
            THROW_LOAD(InvalidFileFormat,pLoad);

         switch(value)
         {
         case 0: // scmGeneral -> scmBTEquations
            m_ShearCapacityMethod = scmBTEquations;
            break;

         case 1: // scmWSDOT -> scmWSDOT2001
            m_ShearCapacityMethod = scmWSDOT2001;
            break;
            
         case 2: // scmSimplified -> scmVciVcw
            m_ShearCapacityMethod = scmVciVcw;
            break;

         default:
            //ATLASSERT(false); do nothing...
            // if value is 3 or 4 it is the correct stuff
            m_ShearCapacityMethod = (ShearCapacityMethod)value;
         }

         // The general method from the 2007 spec becomes the tables method in the 2008 spec
         // make that adjustment here
         if ( m_SpecificationType < lrfdVersionMgr::FourthEditionWith2008Interims && m_ShearCapacityMethod == scmBTEquations )
            m_ShearCapacityMethod = scmBTTables;

         // if this is the 2008 spec, or later and if the shear method is WSDOT 2007, change it to Beta-Theta equations
         if ( lrfdVersionMgr::FourthEditionWith2008Interims <= m_SpecificationType && m_ShearCapacityMethod == scmWSDOT2007 )
            m_ShearCapacityMethod = scmBTEquations;
      }
      else if ( version < 18 )
      {
         // this is before Version 18... shear capacity method was read above in a now obsolete paremeter
         // translate that parameter here
         m_ShearCapacityMethod = (ShearCapacityMethod)shear_capacity_method;
      }
      else
      {
         ATLASSERT( 37 <= version );
         if ( !pLoad->BeginUnit(_T("Shear")) )
            THROW_LOAD(InvalidFileFormat,pLoad);

         if ( !pLoad->Property(_T("LongReinfShearMethod"), &temp ) )
            THROW_LOAD(InvalidFileFormat,pLoad);
         m_LongReinfShearMethod = temp;

         // WSDOT method has been recinded
         m_LongReinfShearMethod = LRFD_METHOD;

         if (!pLoad->Property(_T("IncludeRebarForCapacity"),&temp))
            THROW_LOAD(InvalidFileFormat,pLoad);

         m_bIncludeRebar_Shear = (temp == 0 ? false : true);

         int value;
         if ( !pLoad->Property(_T("ShearFlowMethod"),&value) )
            THROW_LOAD(InvalidFileFormat,pLoad);

         m_ShearFlowMethod = (ShearFlowMethod)(value);

         if ( !pLoad->Property(_T("ShearCapacityMethod"),&value) )
            THROW_LOAD(InvalidFileFormat,pLoad);

         switch(value)
         {
         case 0: // scmGeneral -> scmBTEquations
            m_ShearCapacityMethod = scmBTEquations;
            break;

         case 1: // scmWSDOT -> scmWSDOT2001
            m_ShearCapacityMethod = scmWSDOT2001;
            break;
            
         case 2: // scmSimplified -> scmVciVcw
            m_ShearCapacityMethod = scmVciVcw;
            break;

         default:
            //ATLASSERT(false); do nothing...
            // if value is 3 or 4 it is the correct stuff
            m_ShearCapacityMethod = (ShearCapacityMethod)value;
         }

         // The general method from the 2007 spec becomes the tables method in the 2008 spec
         // make that adjustment here
         if ( m_SpecificationType < lrfdVersionMgr::FourthEditionWith2008Interims && m_ShearCapacityMethod == scmBTEquations )
            m_ShearCapacityMethod = scmBTTables;

         // if this is the 2008 spec, or later and if the shear method is WSDOT 2007, change it to Beta-Theta equations
         if ( lrfdVersionMgr::FourthEditionWith2008Interims <= m_SpecificationType && m_ShearCapacityMethod == scmWSDOT2007 )
            m_ShearCapacityMethod = scmBTEquations;

         if ( !pLoad->BeginUnit(_T("ReductionFactor")) )
            THROW_LOAD(InvalidFileFormat,pLoad);

            if ( !pLoad->Property(_T("Normal"),&m_PhiShear[pgsTypes::Normal]) )
               THROW_LOAD(InvalidFileFormat,pLoad);

            if ( !pLoad->Property(_T("AllLightweight"),&m_PhiShear[pgsTypes::AllLightweight]) )
               THROW_LOAD(InvalidFileFormat,pLoad);

            if ( !pLoad->Property(_T("SandLightweight"),&m_PhiShear[pgsTypes::SandLightweight]) )
               THROW_LOAD(InvalidFileFormat,pLoad);

         if ( !pLoad->EndUnit() ) // ReductionFactor
            THROW_LOAD(InvalidFileFormat,pLoad);

         if ( !pLoad->EndUnit() ) // Shear
            THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( version < 26 )
      {
         if ( m_SpecificationUnits == lrfdVersionMgr::SI )
         {
            m_PedestrianLoad   = ::ConvertToSysUnits(3.6e-3,unitMeasure::MPa);
            m_MinSidewalkWidth = ::ConvertToSysUnits(600.,unitMeasure::Millimeter);
         }
         else
         {
            m_PedestrianLoad   = ::ConvertToSysUnits(0.075,unitMeasure::KSF);
            m_MinSidewalkWidth = ::ConvertToSysUnits(2.0,unitMeasure::Feet);
         }
      }
      else
      {
         // added in version 26
         if ( !pLoad->Property(_T("PedestrianLoad"),&m_PedestrianLoad) )
            THROW_LOAD(InvalidFileFormat,pLoad);

         if ( !pLoad->Property(_T("MinSidewalkWidth"),&m_MinSidewalkWidth) )
            THROW_LOAD(InvalidFileFormat,pLoad);
      }


      if (32 <= version)
      {
         if ( !pLoad->Property(_T("PrestressTransferComputationType"),(int*)&m_PrestressTransferComputationType) )
            THROW_LOAD(InvalidFileFormat,pLoad);
      }


      if ( 38 <= version )
      {
         bool bBeginUnit = pLoad->BeginUnit(_T("StrandExtensions"));

         if ( bBeginUnit )
         {
            if ( !pLoad->Property(_T("AllowStraightStrandExtensions"),&m_bAllowStraightStrandExtensions) )
               THROW_LOAD(InvalidFileFormat,pLoad);

            pLoad->EndUnit();
         }
      }

      if(!pLoad->EndUnit())
         THROW_LOAD(InvalidFileFormat,pLoad);
   }


   return true;

}

#define TEST(a,b) if ( a != b ) return false
#define TESTD(a,b) if ( !::IsEqual(a,b) ) return false

bool SpecLibraryEntry::IsEqual(const SpecLibraryEntry& rOther, bool considerName) const
{
   TEST (m_SpecificationType          , rOther.m_SpecificationType          );
   TEST (m_SpecificationUnits         , rOther.m_SpecificationUnits         );
   TEST (m_Description                , rOther.m_Description                );
   TEST (m_DoCheckStrandSlope         , rOther.m_DoCheckStrandSlope         );
   TEST (m_DoDesignStrandSlope        , rOther.m_DoDesignStrandSlope        );
   TESTD(m_MaxSlope05                 , rOther.m_MaxSlope05                 );
   TESTD(m_MaxSlope06                 , rOther.m_MaxSlope06                 );
   TESTD(m_MaxSlope07                 , rOther.m_MaxSlope07                 );
   TEST (m_DoCheckHoldDown            , rOther.m_DoCheckHoldDown            );
   TEST (m_DoDesignHoldDown           , rOther.m_DoDesignHoldDown            );
   TESTD(m_HoldDownForce              , rOther.m_HoldDownForce              );
   TEST (m_DoCheckSplitting           , rOther.m_DoCheckSplitting           );
   TEST (m_DoDesignSplitting           , rOther.m_DoDesignSplitting         );
   TEST (m_DoCheckConfinement           , rOther.m_DoCheckConfinement       );
   TEST (m_DoDesignConfinement           , rOther.m_DoDesignConfinement     );
   TESTD(m_MaxStirrupSpacing          , rOther.m_MaxStirrupSpacing          );
   TESTD(m_CyLiftingCrackFs          , rOther.m_CyLiftingCrackFs          );
   TESTD(m_CyLiftingFailFs           , rOther.m_CyLiftingFailFs           );
   TESTD(m_CyCompStressServ           , rOther.m_CyCompStressServ           );
   TESTD(m_CyCompStressLifting       , rOther.m_CyCompStressLifting       );
   TESTD(m_CyTensStressServ           , rOther.m_CyTensStressServ           );
   TEST (m_CyDoTensStressServMax      , rOther.m_CyDoTensStressServMax      );
   TESTD(m_CyTensStressServMax        , rOther.m_CyTensStressServMax        );
   TESTD(m_CyTensStressLifting        , rOther.m_CyTensStressLifting        );
   TEST (m_CyDoTensStressLiftingMax   , rOther.m_CyDoTensStressLiftingMax   );
   TESTD(m_CyTensStressLiftingMax     , rOther.m_CyTensStressLiftingMax     );
   TESTD(m_SplittingZoneLengthFactor,    rOther.m_SplittingZoneLengthFactor   );
   TESTD(m_LiftingUpwardImpact        , rOther.m_LiftingUpwardImpact        );
   TESTD(m_LiftingDownwardImpact      , rOther.m_LiftingDownwardImpact      );
   TESTD(m_HaulingUpwardImpact        , rOther.m_HaulingUpwardImpact        );
   TESTD(m_HaulingDownwardImpact      , rOther.m_HaulingDownwardImpact      );
   TEST (m_CuringMethod               , rOther.m_CuringMethod               );
   TEST (m_EnableLiftingCheck         , rOther.m_EnableLiftingCheck         );
   TEST (m_EnableLiftingDesign        , rOther.m_EnableLiftingDesign        );
   TESTD(m_PickPointHeight            , rOther.m_PickPointHeight            );
   TESTD(m_MaxGirderWgt,                rOther.m_MaxGirderWgt               );
   TESTD(m_LiftingLoopTolerance       , rOther.m_LiftingLoopTolerance       );
   TESTD(m_MinCableInclination        , rOther.m_MinCableInclination        );
   TESTD(m_MaxGirderSweepLifting      , rOther.m_MaxGirderSweepLifting      );
   TESTD(m_MaxGirderSweepHauling      , rOther.m_MaxGirderSweepHauling      );
   TEST (m_EnableHaulingCheck         , rOther.m_EnableHaulingCheck            );
   TEST (m_EnableHaulingDesign        , rOther.m_EnableHaulingDesign           );
   TESTD(m_HaulingSupportDistance          , rOther.m_HaulingSupportDistance );
   TESTD(m_HaulingSupportPlacementTolerance, rOther.m_HaulingSupportPlacementTolerance );
   TESTD(m_HaulingCamberPercentEstimate    , rOther.m_HaulingCamberPercentEstimate );
   TESTD(m_CompStressHauling       , rOther.m_CompStressHauling         );
   TESTD(m_TensStressHauling          , rOther.m_TensStressHauling          );
   TEST (m_DoTensStressHaulingMax     , rOther.m_DoTensStressHaulingMax     );
   TESTD(m_TensStressHaulingMax       , rOther.m_TensStressHaulingMax       );
   TESTD(m_HeHaulingCrackFs           , rOther.m_HeHaulingCrackFs           );
   TESTD(m_HeHaulingRollFs            , rOther.m_HeHaulingRollFs            );
   TESTD(m_RoadwaySuperelevation      , rOther.m_RoadwaySuperelevation      );
   TEST (m_TruckRollStiffnessMethod   , rOther.m_TruckRollStiffnessMethod   );
   TESTD(m_TruckRollStiffness         , rOther.m_TruckRollStiffness         );
   TESTD(m_AxleWeightLimit            , rOther.m_AxleWeightLimit            );
   TESTD(m_AxleStiffness              , rOther.m_AxleStiffness              );
   TESTD(m_MinRollStiffness           , rOther.m_MinRollStiffness           );
   TESTD(m_MaxHaulingOverhang         , rOther.m_MaxHaulingOverhang         );
   TESTD(m_TruckGirderHeight          , rOther.m_TruckGirderHeight          );
   TESTD(m_TruckRollCenterHeight      , rOther.m_TruckRollCenterHeight      );
   TESTD(m_TruckAxleWidth             , rOther.m_TruckAxleWidth             );
   TESTD(m_HeErectionCrackFs          , rOther.m_HeErectionCrackFs          );
   TESTD(m_HeErectionFailFs           , rOther.m_HeErectionFailFs           );
   TESTD(m_HaulingModulusOfRuptureCoefficient[pgsTypes::Normal] , rOther.m_HaulingModulusOfRuptureCoefficient[pgsTypes::Normal] );
   TESTD(m_HaulingModulusOfRuptureCoefficient[pgsTypes::AllLightweight] , rOther.m_HaulingModulusOfRuptureCoefficient[pgsTypes::AllLightweight] );
   TESTD(m_HaulingModulusOfRuptureCoefficient[pgsTypes::SandLightweight] , rOther.m_HaulingModulusOfRuptureCoefficient[pgsTypes::SandLightweight] );

   TESTD(m_LiftingModulusOfRuptureCoefficient[pgsTypes::Normal] , rOther.m_LiftingModulusOfRuptureCoefficient[pgsTypes::Normal] );
   TESTD(m_LiftingModulusOfRuptureCoefficient[pgsTypes::AllLightweight] , rOther.m_LiftingModulusOfRuptureCoefficient[pgsTypes::AllLightweight] );
   TESTD(m_LiftingModulusOfRuptureCoefficient[pgsTypes::SandLightweight] , rOther.m_LiftingModulusOfRuptureCoefficient[pgsTypes::SandLightweight] );

   TESTD(m_CyTensStressServWithRebar  , rOther.m_CyTensStressServWithRebar );
   TESTD(m_TensStressLiftingWithRebar , rOther.m_TensStressLiftingWithRebar );
   TESTD(m_TensStressHaulingWithRebar , rOther.m_TensStressHaulingWithRebar );

   TESTD(m_TempStrandRemovalCompStress     , rOther.m_TempStrandRemovalCompStress              );
   TESTD(m_TempStrandRemovalTensStress     , rOther.m_TempStrandRemovalTensStress              );
   TEST (m_TempStrandRemovalDoTensStressMax, rOther.m_TempStrandRemovalDoTensStressMax         );
   TESTD(m_TempStrandRemovalTensStressMax  , rOther.m_TempStrandRemovalTensStressMax           );

   TESTD(m_Bs1CompStress              , rOther.m_Bs1CompStress              );
   TESTD(m_Bs1TensStress              , rOther.m_Bs1TensStress              );
   TEST (m_Bs1DoTensStressMax         , rOther.m_Bs1DoTensStressMax         );
   TESTD(m_Bs1TensStressMax           , rOther.m_Bs1TensStressMax           );
   TESTD(m_Bs2CompStress              , rOther.m_Bs2CompStress              );
   TEST (m_TrafficBarrierDistributionType, rOther.m_TrafficBarrierDistributionType);
   TEST (m_Bs2MaxGirdersTrafficBarrier, rOther.m_Bs2MaxGirdersTrafficBarrier );
   TEST (m_Bs2MaxGirdersUtility       , rOther.m_Bs2MaxGirdersUtility        );
   TEST (m_OverlayLoadDistribution    , rOther.m_OverlayLoadDistribution     );
   TESTD(m_Bs3CompStressServ          , rOther.m_Bs3CompStressServ           );
   TESTD(m_Bs3CompStressService1A     , rOther.m_Bs3CompStressService1A      );
   TESTD(m_Bs3TensStressServNc        , rOther.m_Bs3TensStressServNc         );
   TEST (m_Bs3DoTensStressServNcMax   , rOther.m_Bs3DoTensStressServNcMax    );
   TESTD(m_Bs3TensStressServNcMax     , rOther.m_Bs3TensStressServNcMax      );
   TESTD(m_Bs3TensStressServSc        , rOther.m_Bs3TensStressServSc         );
   TEST (m_Bs3DoTensStressServScMax   , rOther.m_Bs3DoTensStressServScMax    );
   TESTD(m_Bs3TensStressServScMax     , rOther.m_Bs3TensStressServScMax      );
//   TEST (m_Bs3IgnoreRangeOfApplicability , rOther.m_Bs3IgnoreRangeOfApplicability );
   TEST (m_Bs3LRFDOverReinforcedMomentCapacity , rOther.m_Bs3LRFDOverReinforcedMomentCapacity );
   TESTD(m_FlexureModulusOfRuptureCoefficient[pgsTypes::Normal] , rOther.m_FlexureModulusOfRuptureCoefficient[pgsTypes::Normal] );
   TESTD(m_FlexureModulusOfRuptureCoefficient[pgsTypes::SandLightweight] , rOther.m_FlexureModulusOfRuptureCoefficient[pgsTypes::SandLightweight] );
   TESTD(m_FlexureModulusOfRuptureCoefficient[pgsTypes::AllLightweight] , rOther.m_FlexureModulusOfRuptureCoefficient[pgsTypes::AllLightweight] );
   TESTD(m_ShearModulusOfRuptureCoefficient[pgsTypes::Normal] , rOther.m_ShearModulusOfRuptureCoefficient[pgsTypes::Normal] );
   TESTD(m_ShearModulusOfRuptureCoefficient[pgsTypes::SandLightweight] , rOther.m_ShearModulusOfRuptureCoefficient[pgsTypes::SandLightweight] );
   TESTD(m_ShearModulusOfRuptureCoefficient[pgsTypes::AllLightweight] , rOther.m_ShearModulusOfRuptureCoefficient[pgsTypes::AllLightweight] );

   TEST (m_CreepMethod                , rOther.m_CreepMethod                );
   TESTD(m_XferTime                   , rOther.m_XferTime                   );
   TESTD(m_CreepFactor                , rOther.m_CreepFactor                );
   TESTD(m_CreepDuration1Min          , rOther.m_CreepDuration1Min          );
   TESTD(m_CreepDuration2Min          , rOther.m_CreepDuration2Min          );
   TESTD(m_CreepDuration1Max          , rOther.m_CreepDuration1Max          );
   TESTD(m_CreepDuration2Max          , rOther.m_CreepDuration2Max          );
   TESTD(m_TotalCreepDuration  , rOther.m_TotalCreepDuration  );
   TEST (m_LossMethod                 , rOther.m_LossMethod                 );
   TESTD(m_FinalLosses                , rOther.m_FinalLosses                );
   TESTD(m_ShippingLosses             , rOther.m_ShippingLosses             );
   TESTD(m_BeforeXferLosses           , rOther.m_BeforeXferLosses           );
   TESTD(m_AfterXferLosses            , rOther.m_AfterXferLosses            );
   TESTD(m_ShippingTime               , rOther.m_ShippingTime               );
   TESTD(m_LiftingLosses              , rOther.m_LiftingLosses              );
   TESTD(m_BeforeTempStrandRemovalLosses , rOther.m_BeforeTempStrandRemovalLosses );
   TESTD(m_AfterTempStrandRemovalLosses  , rOther.m_AfterTempStrandRemovalLosses );
   TESTD(m_AfterDeckPlacementLosses      , rOther.m_AfterDeckPlacementLosses );
   TESTD(m_AfterSIDLLosses               , rOther.m_AfterSIDLLosses );

   TESTD(m_Dset,rOther.m_Dset);
   TESTD(m_WobbleFriction,rOther.m_WobbleFriction);
   TESTD(m_FrictionCoefficient,rOther.m_FrictionCoefficient);

   TESTD(m_SlabElasticGain          , rOther.m_SlabElasticGain);
   TESTD(m_SlabPadElasticGain       , rOther.m_SlabPadElasticGain);
   TESTD(m_DiaphragmElasticGain     , rOther.m_DiaphragmElasticGain);
   TESTD(m_UserDCElasticGainBS1     , rOther.m_UserDCElasticGainBS1);
   TESTD(m_UserDWElasticGainBS1     , rOther.m_UserDWElasticGainBS1);
   TESTD(m_UserDCElasticGainBS2     , rOther.m_UserDCElasticGainBS2);
   TESTD(m_UserDWElasticGainBS2     , rOther.m_UserDWElasticGainBS2);
   TESTD(m_RailingSystemElasticGain , rOther.m_RailingSystemElasticGain);
   TESTD(m_OverlayElasticGain       , rOther.m_OverlayElasticGain);
   TESTD(m_SlabShrinkageElasticGain , rOther.m_SlabShrinkageElasticGain);
   TESTD(m_LiveLoadElasticGain      , rOther.m_LiveLoadElasticGain);

   TEST (m_LldfMethod                 , rOther.m_LldfMethod                 );
   TEST (m_LongReinfShearMethod       , rOther.m_LongReinfShearMethod       );

   TEST (m_bCheckStrandStress[AT_JACKING],        rOther.m_bCheckStrandStress[AT_JACKING]);
   TEST (m_bCheckStrandStress[BEFORE_TRANSFER],   rOther.m_bCheckStrandStress[BEFORE_TRANSFER]);
   TEST (m_bCheckStrandStress[AFTER_TRANSFER],   rOther.m_bCheckStrandStress[AFTER_TRANSFER]);
   TEST (m_bCheckStrandStress[AFTER_ALL_LOSSES], rOther.m_bCheckStrandStress[AFTER_ALL_LOSSES]);

   TESTD(m_StrandStressCoeff[AT_JACKING][STRESS_REL], rOther.m_StrandStressCoeff[AT_JACKING][STRESS_REL]       );
   TESTD(m_StrandStressCoeff[AT_JACKING][LOW_RELAX], rOther.m_StrandStressCoeff[AT_JACKING][LOW_RELAX]        );
   TESTD(m_StrandStressCoeff[BEFORE_TRANSFER][STRESS_REL], rOther.m_StrandStressCoeff[BEFORE_TRANSFER][STRESS_REL]  );
   TESTD(m_StrandStressCoeff[BEFORE_TRANSFER][LOW_RELAX], rOther.m_StrandStressCoeff[BEFORE_TRANSFER][LOW_RELAX]   );
   TESTD(m_StrandStressCoeff[AFTER_TRANSFER][STRESS_REL], rOther.m_StrandStressCoeff[AFTER_TRANSFER][STRESS_REL]   );
   TESTD(m_StrandStressCoeff[AFTER_TRANSFER][LOW_RELAX], rOther.m_StrandStressCoeff[AFTER_TRANSFER][LOW_RELAX]    );
   TESTD(m_StrandStressCoeff[AFTER_ALL_LOSSES][STRESS_REL], rOther.m_StrandStressCoeff[AFTER_ALL_LOSSES][STRESS_REL] );
   TESTD(m_StrandStressCoeff[AFTER_ALL_LOSSES][LOW_RELAX], rOther.m_StrandStressCoeff[AFTER_ALL_LOSSES][LOW_RELAX]  );

   TEST (m_bDoEvaluateDeflection , rOther.m_bDoEvaluateDeflection );
   TESTD(m_DeflectionLimit       , rOther.m_DeflectionLimit );

   TEST (m_bIncludeRebar_Moment , rOther.m_bIncludeRebar_Moment );
   TEST (m_bIncludeRebar_Shear , rOther.m_bIncludeRebar_Shear );
   
//   TEST (m_AnalysisType , rOther.m_AnalysisType );

   for ( int i = 0; i < 3; i++ )
   {
      TESTD(m_MaxSlabFc[i]             , rOther.m_MaxSlabFc[i] );
      TESTD(m_MaxGirderFci[i]          , rOther.m_MaxGirderFci[i] );
      TESTD(m_MaxGirderFc[i]           , rOther.m_MaxGirderFc[i] );
      TESTD(m_MaxConcreteUnitWeight[i] , rOther.m_MaxConcreteUnitWeight[i] );
      TESTD(m_MaxConcreteAggSize[i]    , rOther.m_MaxConcreteAggSize[i] );
   }

   TEST (m_EnableSlabOffsetCheck         , rOther.m_EnableSlabOffsetCheck            );
   TEST (m_EnableSlabOffsetDesign        , rOther.m_EnableSlabOffsetDesign );

   TEST (m_DesignStrandFillType            , rOther.m_DesignStrandFillType );
   TEST (m_EffFlangeWidthMethod            , rOther.m_EffFlangeWidthMethod );
   TEST (m_ShearFlowMethod                 , rOther.m_ShearFlowMethod );
   TEST (m_ShearCapacityMethod             , rOther.m_ShearCapacityMethod );
   TESTD(m_CuringMethodTimeAdjustmentFactor , rOther.m_CuringMethodTimeAdjustmentFactor );

   TESTD(m_MinLiftPoint      , rOther.m_MinLiftPoint );
   TESTD(m_LiftPointAccuracy , rOther.m_LiftPointAccuracy );
   TESTD(m_MinHaulPoint      , rOther.m_MinHaulPoint );
   TESTD(m_HaulPointAccuracy , rOther.m_HaulPointAccuracy);

   TESTD(m_PedestrianLoad,   rOther.m_PedestrianLoad);
   TESTD(m_MinSidewalkWidth, rOther.m_MinSidewalkWidth);

   TESTD(m_MaxAngularDeviationBetweenGirders, rOther.m_MaxAngularDeviationBetweenGirders);
   TESTD(m_MinGirderStiffnessRatio,           rOther.m_MinGirderStiffnessRatio);
   TESTD(m_LLDFGirderSpacingLocation,         rOther.m_LLDFGirderSpacingLocation);

   TEST (m_LimitDistributionFactorsToLanesBeams             , rOther.m_LimitDistributionFactorsToLanesBeams );
   TEST (m_PrestressTransferComputationType, rOther.m_PrestressTransferComputationType);

   TEST(m_RelaxationLossMethod,rOther.m_RelaxationLossMethod);

   for ( int i = 0; i < 3; i++ )
   {
      TESTD(m_PhiFlexureTensionPS[i],rOther.m_PhiFlexureTensionPS[i]);
      TESTD(m_PhiFlexureTensionRC[i],rOther.m_PhiFlexureTensionRC[i]);
      TESTD(m_PhiFlexureCompression[i],rOther.m_PhiFlexureCompression[i]);
      TESTD(m_PhiShear[i],rOther.m_PhiShear[i]);
   }

   TEST( m_bIncludeForNegMoment, rOther.m_bIncludeForNegMoment);
   TEST( m_bAllowStraightStrandExtensions, rOther.m_bAllowStraightStrandExtensions);

   if (considerName)
   {
      if ( GetName() != rOther.GetName() )
         return false;
   }

   return true;
}

//======================== ACCESS     =======================================

void SpecLibraryEntry::SetSpecificationType(lrfdVersionMgr::Version type)
{
   m_SpecificationType = type;
}

lrfdVersionMgr::Version SpecLibraryEntry::GetSpecificationType() const
{
   return m_SpecificationType;
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

std::_tstring SpecLibraryEntry::GetDescription() const
{
   return m_Description;
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

void SpecLibraryEntry::GetHoldDownForce(bool* doCheck, bool*doDesign, Float64* force) const
{
   *doCheck  = m_DoCheckHoldDown;
   *doDesign = m_DoDesignHoldDown;
   *force    = m_HoldDownForce;
}

void SpecLibraryEntry::SetHoldDownForce(bool doCheck, bool doDesign, Float64 force)
{
   m_DoCheckHoldDown = doCheck;
   m_DoDesignHoldDown = doCheck ? doDesign : false; // don't allow design without checking

   if (m_DoCheckHoldDown)
      m_HoldDownForce   = force;
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


Float64 SpecLibraryEntry::GetMaxStirrupSpacing() const
{
   return m_MaxStirrupSpacing;
}

void SpecLibraryEntry::SetMaxStirrupSpacing(Float64 space)
{
   m_MaxStirrupSpacing = space;
}

void SpecLibraryEntry::EnableLiftingCheck(bool enable)
{
   m_EnableLiftingCheck = enable;
}

bool SpecLibraryEntry::IsLiftingCheckEnabled() const
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

Float64 SpecLibraryEntry::GetCyLiftingCrackFs() const
{
   return m_CyLiftingCrackFs;
}

void SpecLibraryEntry::SetCyLiftingCrackFs(Float64 fs)
{
   m_CyLiftingCrackFs = fs;
}

Float64 SpecLibraryEntry::GetCyLiftingFailFs() const
{
   return m_CyLiftingFailFs;
}

void SpecLibraryEntry::SetCyLiftingFailFs(Float64 fs)
{
   m_CyLiftingFailFs = fs;
}


Float64 SpecLibraryEntry::GetCyLiftingUpwardImpact() const
{
   return m_LiftingUpwardImpact;
}

void SpecLibraryEntry::SetCyLiftingUpwardImpact(Float64 impact)
{
   m_LiftingUpwardImpact = impact;
}

Float64 SpecLibraryEntry::GetCyLiftingDownwardImpact() const
{
   return m_LiftingDownwardImpact;
}

void SpecLibraryEntry::SetCyLiftingDownwardImpact(Float64 impact)
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

bool SpecLibraryEntry::IsHaulingCheckEnabled() const
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

Float64 SpecLibraryEntry::GetHaulingUpwardImpact() const
{
   return m_HaulingUpwardImpact;
}

void SpecLibraryEntry::SetHaulingUpwardImpact(Float64 impact)
{
   m_HaulingUpwardImpact = impact;
}

Float64 SpecLibraryEntry::GetHaulingDownwardImpact() const
{
   return m_HaulingDownwardImpact;
}

void SpecLibraryEntry::SetHaulingDownwardImpact(Float64 impact)
{
   m_HaulingDownwardImpact = impact;
}

Float64 SpecLibraryEntry::GetCyCompStressService() const
{
   return m_CyCompStressServ;
}

void SpecLibraryEntry::SetCyCompStressService(Float64 stress)
{
   m_CyCompStressServ = stress;
}

Float64 SpecLibraryEntry::GetCyCompStressLifting() const
{
   return m_CyCompStressLifting;
}

void SpecLibraryEntry::SetCyCompStressLifting(Float64 stress)
{
   m_CyCompStressLifting = stress;
}

Float64 SpecLibraryEntry::GetCyMaxConcreteTens() const
{
   return m_CyTensStressServ;
}

void SpecLibraryEntry::SetCyMaxConcreteTens(Float64 stress)
{
   m_CyTensStressServ = stress;
}

void SpecLibraryEntry::GetCyAbsMaxConcreteTens(bool* doCheck, Float64* stress) const
{
   *doCheck = m_CyDoTensStressServMax;
   *stress  = m_CyTensStressServMax;
}

void SpecLibraryEntry::SetCyAbsMaxConcreteTens(bool doCheck, Float64 stress)
{
   m_CyDoTensStressServMax = doCheck;
   m_CyTensStressServMax   = stress;
}

Float64 SpecLibraryEntry::GetCyMaxConcreteTensLifting() const
{
   return m_CyTensStressLifting;
}

void SpecLibraryEntry::SetCyMaxConcreteTensLifting(Float64 stress)
{
   m_CyTensStressLifting = stress;
}

void SpecLibraryEntry::GetCyAbsMaxConcreteTensLifting(bool* doCheck, Float64* stress) const
{
   *doCheck = m_CyDoTensStressLiftingMax;
   *stress  = m_CyTensStressLiftingMax;
}

void SpecLibraryEntry::SetCyAbsMaxConcreteTensLifting(bool doCheck, Float64 stress)
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

Float64 SpecLibraryEntry::GetMaxGirderSweepLifting() const
{
   return m_MaxGirderSweepLifting;
}

void SpecLibraryEntry::SetMaxGirderSweepLifting(Float64 sweep)
{
   m_MaxGirderSweepLifting = sweep;
}

Float64 SpecLibraryEntry::GetMaxGirderSweepHauling() const
{
   return m_MaxGirderSweepHauling;
}

void SpecLibraryEntry::SetMaxGirderSweepHauling(Float64 sweep)
{
   m_MaxGirderSweepHauling = sweep;
}

Float64 SpecLibraryEntry::GetHaulingSupportDistance() const
{
   return m_HaulingSupportDistance;
}

void SpecLibraryEntry::SetHaulingSupportDistance(Float64 d)
{
   m_HaulingSupportDistance = d;
}

Float64 SpecLibraryEntry::GetMaxHaulingOverhang() const
{
   return m_MaxHaulingOverhang;
}

void SpecLibraryEntry::SetMaxHaulingOverhang(Float64 oh)
{
   m_MaxHaulingOverhang = oh;
}

Float64 SpecLibraryEntry::GetHaulingSupportPlacementTolerance() const
{
   return m_HaulingSupportPlacementTolerance;
}

void SpecLibraryEntry::SetHaulingSupportPlacementTolerance(Float64 tol)
{
   m_HaulingSupportPlacementTolerance = tol;
}

Float64 SpecLibraryEntry::GetHaulingCamberPercentEstimate() const
{
   return m_HaulingCamberPercentEstimate;
}

void SpecLibraryEntry::SetHaulingCamberPercentEstimate(Float64 per)
{
   m_HaulingCamberPercentEstimate = per;
}

Float64 SpecLibraryEntry::GetHaulingCompStress() const
{
   return m_CompStressHauling;
}

void SpecLibraryEntry::SetHaulingCompStress(Float64 stress)
{
   m_CompStressHauling = stress;
}

Float64 SpecLibraryEntry::GetMaxConcreteTensHauling() const
{
   return m_TensStressHauling;
}

void SpecLibraryEntry::SetMaxConcreteTensHauling(Float64 stress)
{
   m_TensStressHauling = stress;
}

void SpecLibraryEntry::GetAbsMaxConcreteTensHauling(bool* doCheck, Float64* stress) const
{
   *doCheck = m_DoTensStressHaulingMax;
   *stress  = m_TensStressHaulingMax;
}

void SpecLibraryEntry::SetAbsMaxConcreteTensHauling(bool doCheck, Float64 stress)
{
   m_DoTensStressHaulingMax = doCheck;
   m_TensStressHaulingMax = stress;
}

Float64 SpecLibraryEntry::GetHaulingCrackFs() const
{
   return m_HeHaulingCrackFs;
}

void SpecLibraryEntry::SetHaulingCrackFs(Float64 fs)
{
   m_HeHaulingCrackFs = fs;
}

Float64 SpecLibraryEntry::GetHaulingFailFs() const
{
   return m_HeHaulingRollFs;
}

void SpecLibraryEntry::SetHaulingFailFs(Float64 fs)
{
   m_HeHaulingRollFs = fs;
}

int SpecLibraryEntry::GetTruckRollStiffnessMethod() const
{
   return m_TruckRollStiffnessMethod;
}

void SpecLibraryEntry::SetTruckRollStiffnessMethod(int method)
{
   m_TruckRollStiffnessMethod = method;
}

Float64 SpecLibraryEntry::GetAxleWeightLimit() const
{
   return m_AxleWeightLimit;
}

void SpecLibraryEntry::SetAxleWeightLimit(Float64 limit)
{
   m_AxleWeightLimit = limit;
}

Float64 SpecLibraryEntry::GetAxleStiffness() const
{
   return m_AxleStiffness;
}

void SpecLibraryEntry::SetAxleStiffness(Float64 stiffness)
{
   m_AxleStiffness = stiffness;
}

Float64 SpecLibraryEntry::GetMinRollStiffness() const
{
   return m_MinRollStiffness;
}

void SpecLibraryEntry::SetMinRollStiffness(Float64 stiffness)
{
   m_MinRollStiffness = stiffness;
}

Float64 SpecLibraryEntry::GetTruckRollStiffness() const
{
   return m_TruckRollStiffness;
}

void SpecLibraryEntry::SetTruckRollStiffness(Float64 stiff)
{
   m_TruckRollStiffness = stiff;
}

Float64 SpecLibraryEntry::GetTruckGirderHeight() const
{
   return m_TruckGirderHeight;
}

void SpecLibraryEntry::SetTruckGirderHeight(Float64 height)
{
   m_TruckGirderHeight = height;
}

Float64 SpecLibraryEntry::GetTruckRollCenterHeight() const
{
   return m_TruckRollCenterHeight;
}

void SpecLibraryEntry::SetTruckRollCenterHeight(Float64 height)
{
   m_TruckRollCenterHeight = height;
}

Float64 SpecLibraryEntry::GetTruckAxleWidth() const
{
   return m_TruckAxleWidth;
}

void SpecLibraryEntry::SetTruckAxleWidth(Float64 dist)
{
   m_TruckAxleWidth = dist;
}

Float64 SpecLibraryEntry::GetRoadwaySuperelevation() const
{
   return m_RoadwaySuperelevation;
}

void SpecLibraryEntry::SetRoadwaySuperelevation(Float64 dist)
{
   m_RoadwaySuperelevation = dist;
}

Float64 SpecLibraryEntry::GetErectionCrackFs() const
{
   return m_HeErectionCrackFs;
}

void SpecLibraryEntry::SetErectionCrackFs(Float64 fs)
{
   m_HeErectionCrackFs = fs;
}

Float64 SpecLibraryEntry::GetErectionFailFs() const
{
   return m_HeErectionFailFs;
}

void SpecLibraryEntry::SetErectionFailFs(Float64 fs)
{
   m_HeErectionFailFs = fs;
}

void SpecLibraryEntry::SetHaulingModulusOfRuptureCoefficient(Float64 fr,pgsTypes::ConcreteType type)
{
   m_HaulingModulusOfRuptureCoefficient[type] = fr;
}

Float64 SpecLibraryEntry::GetHaulingModulusOfRuptureCoefficient(pgsTypes::ConcreteType type) const
{
   return m_HaulingModulusOfRuptureCoefficient[type];
}

void SpecLibraryEntry::SetLiftingModulusOfRuptureCoefficient(Float64 fr,pgsTypes::ConcreteType type)
{
   m_LiftingModulusOfRuptureCoefficient[type] = fr;
}

Float64 SpecLibraryEntry::GetLiftingModulusOfRuptureCoefficient(pgsTypes::ConcreteType type) const
{
   return m_LiftingModulusOfRuptureCoefficient[type];
}

void SpecLibraryEntry::SetMaxGirderWeight(Float64 wgt)
{
   m_MaxGirderWgt = wgt;
}

Float64 SpecLibraryEntry::GetMaxGirderWeight() const
{
   return m_MaxGirderWgt;
}

Float64 SpecLibraryEntry::GetCyMaxConcreteTensWithRebar() const
{
   return m_CyTensStressServWithRebar;
}

void SpecLibraryEntry::SetCyMaxConcreteTensWithRebar(Float64 stress)
{
   m_CyTensStressServWithRebar = stress;
}

Float64 SpecLibraryEntry::GetMaxConcreteTensWithRebarLifting() const
{
   return m_TensStressLiftingWithRebar;
}

void SpecLibraryEntry::SetMaxConcreteTensWithRebarLifting(Float64 stress)
{
    m_TensStressLiftingWithRebar = stress;
}

Float64 SpecLibraryEntry::GetMaxConcreteTensWithRebarHauling() const
{
   return m_TensStressHaulingWithRebar;
}

void SpecLibraryEntry::SetMaxConcreteTensWithRebarHauling(Float64 stress)
{
    m_TensStressHaulingWithRebar = stress;
}


Float64 SpecLibraryEntry::GetTempStrandRemovalCompStress() const
{
   return m_TempStrandRemovalCompStress;
}

void SpecLibraryEntry::SetTempStrandRemovalCompStress(Float64 stress)
{
   m_TempStrandRemovalCompStress = stress;
}

Float64 SpecLibraryEntry::GetTempStrandRemovalMaxConcreteTens() const
{
   return m_TempStrandRemovalTensStress;
}

void SpecLibraryEntry::SetTempStrandRemovalMaxConcreteTens(Float64 stress)
{
   m_TempStrandRemovalTensStress = stress;
}

void SpecLibraryEntry::GetTempStrandRemovalAbsMaxConcreteTens(bool* doCheck, Float64* stress) const
{
   *doCheck = m_TempStrandRemovalDoTensStressMax;
   *stress  = m_TempStrandRemovalTensStressMax;
}

void SpecLibraryEntry::SetTempStrandRemovalAbsMaxConcreteTens(bool doCheck, Float64 stress)
{
   m_TempStrandRemovalDoTensStressMax = doCheck;
   m_TempStrandRemovalTensStressMax   = stress;
}




Float64 SpecLibraryEntry::GetBs1CompStress() const
{
   return m_Bs1CompStress;
}

void SpecLibraryEntry::SetBs1CompStress(Float64 stress)
{
   m_Bs1CompStress = stress;
}

Float64 SpecLibraryEntry::GetBs1MaxConcreteTens() const
{
   return m_Bs1TensStress;
}

void SpecLibraryEntry::SetBs1MaxConcreteTens(Float64 stress)
{
   m_Bs1TensStress = stress;
}

void SpecLibraryEntry::GetBs1AbsMaxConcreteTens(bool* doCheck, Float64* stress) const
{
   *doCheck = m_Bs1DoTensStressMax;
   *stress  = m_Bs1TensStressMax;
}

void SpecLibraryEntry::SetBs1AbsMaxConcreteTens(bool doCheck, Float64 stress)
{
   m_Bs1DoTensStressMax = doCheck;
   m_Bs1TensStressMax   = stress;
}

Float64 SpecLibraryEntry::GetBs2CompStress() const
{
   return m_Bs2CompStress;
}

void SpecLibraryEntry::SetBs2CompStress(Float64 stress)
{
   m_Bs2CompStress = stress;
}

Float64 SpecLibraryEntry::GetBs3CompStressService() const
{
   return m_Bs3CompStressServ;
}

void SpecLibraryEntry::SetBs3CompStressService(Float64 stress)
{
   m_Bs3CompStressServ = stress;
}

Float64 SpecLibraryEntry::GetBs3CompStressService1A() const
{
   return m_Bs3CompStressService1A;
}

void SpecLibraryEntry::SetBs3CompStressService1A(Float64 stress)
{
   m_Bs3CompStressService1A = stress;
}

Float64 SpecLibraryEntry::GetBs3MaxConcreteTensNc() const
{
   return m_Bs3TensStressServNc;
}

void SpecLibraryEntry::SetBs3MaxConcreteTensNc(Float64 stress)
{
   m_Bs3TensStressServNc = stress;
}

void SpecLibraryEntry::GetBs3AbsMaxConcreteTensNc(bool* doCheck, Float64* stress) const
{
   *doCheck = m_Bs3DoTensStressServNcMax;
   *stress  = m_Bs3TensStressServNcMax;
}

void SpecLibraryEntry::SetBs3AbsMaxConcreteTensNc(bool doCheck, Float64 stress)
{
   m_Bs3DoTensStressServNcMax = doCheck;
   m_Bs3TensStressServNcMax   = stress;
}

Float64 SpecLibraryEntry::GetBs3MaxConcreteTensSc() const
{
   return m_Bs3TensStressServSc;
}

void SpecLibraryEntry::SetBs3MaxConcreteTensSc(Float64 stress)
{
   m_Bs3TensStressServSc = stress;
}

void SpecLibraryEntry::GetBs3AbsMaxConcreteTensSc(bool* doCheck, Float64* stress) const
{
   *doCheck = m_Bs3DoTensStressServNcMax;
   *stress  = m_Bs3TensStressServNcMax;
}

void SpecLibraryEntry::SetBs3AbsMaxConcreteTensSc(bool doCheck, Float64 stress)
{
   m_Bs3DoTensStressServNcMax = doCheck;
   m_Bs3TensStressServNcMax   = stress;
}

void SpecLibraryEntry::SetBs3LRFDOverreinforcedMomentCapacity(bool bSet)
{
   m_Bs3LRFDOverReinforcedMomentCapacity = bSet ? 0 : 1;
}

bool SpecLibraryEntry::GetBs3LRFDOverreinforcedMomentCapacity() const
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

Float64 SpecLibraryEntry::GetUserDCElasticGain(pgsTypes::Stage stage) const
{
   ATLASSERT(stage == pgsTypes::BridgeSite1 || stage == pgsTypes::BridgeSite2);
   if ( stage == pgsTypes::BridgeSite1 )
      return m_UserDCElasticGainBS1;
   else
      return m_UserDCElasticGainBS2;
}

void SpecLibraryEntry::SetUserDCElasticGain(pgsTypes::Stage stage,Float64 f)
{
   ATLASSERT(stage == pgsTypes::BridgeSite1 || stage == pgsTypes::BridgeSite2);
   if ( stage == pgsTypes::BridgeSite1 )
      m_UserDCElasticGainBS1 = f;
   else
      m_UserDCElasticGainBS2 = f;
}

Float64 SpecLibraryEntry::GetUserDWElasticGain(pgsTypes::Stage stage) const
{
   ATLASSERT(stage == pgsTypes::BridgeSite1 || stage == pgsTypes::BridgeSite2);
   if ( stage == pgsTypes::BridgeSite1 )
      return m_UserDWElasticGainBS1;
   else
      return m_UserDWElasticGainBS2;
}

void SpecLibraryEntry::SetUserDWElasticGain(pgsTypes::Stage stage,Float64 f)
{
   ATLASSERT(stage == pgsTypes::BridgeSite1 || stage == pgsTypes::BridgeSite2);
   if ( stage == pgsTypes::BridgeSite1 )
      m_UserDWElasticGainBS1 = f;
   else
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

Int16 SpecLibraryEntry::GetLiveLoadDistributionMethod() const
{
   return m_LldfMethod;
}

void SpecLibraryEntry::SetLiveLoadDistributionMethod(Int16 method)
{
   PRECONDITION(method == LLDF_LRFD || method == LLDF_WSDOT  || method == LLDF_TXDOT);

   m_LldfMethod = method;
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
   CHECK(limit>0.0);
   m_DeflectionLimit = limit;
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
   m_FlexureModulusOfRuptureCoefficient[type] = fr;
}

Float64 SpecLibraryEntry::GetFlexureModulusOfRuptureCoefficient(pgsTypes::ConcreteType type) const
{
   return m_FlexureModulusOfRuptureCoefficient[type];
}

void SpecLibraryEntry::SetShearModulusOfRuptureCoefficient(pgsTypes::ConcreteType type,Float64 fr)
{
   m_ShearModulusOfRuptureCoefficient[type] = fr;
}

Float64 SpecLibraryEntry::GetShearModulusOfRuptureCoefficient(pgsTypes::ConcreteType type) const
{
   return m_ShearModulusOfRuptureCoefficient[type];
}

void SpecLibraryEntry::SetMaxSlabFc(pgsTypes::ConcreteType type,Float64 fc)
{
   m_MaxSlabFc[type] = fc;
}

Float64 SpecLibraryEntry::GetMaxSlabFc(pgsTypes::ConcreteType type) const
{
   return m_MaxSlabFc[type];
}

void SpecLibraryEntry::SetMaxGirderFc(pgsTypes::ConcreteType type,Float64 fc)
{
   m_MaxGirderFc[type] = fc;
}

Float64 SpecLibraryEntry::GetMaxGirderFc(pgsTypes::ConcreteType type) const
{
   return m_MaxGirderFc[type];
}

void SpecLibraryEntry::SetMaxGirderFci(pgsTypes::ConcreteType type,Float64 fci)
{
   m_MaxGirderFci[type] = fci;
}

Float64 SpecLibraryEntry::GetMaxGirderFci(pgsTypes::ConcreteType type) const
{
   return m_MaxGirderFci[type];
}

void SpecLibraryEntry::SetMaxConcreteUnitWeight(pgsTypes::ConcreteType type,Float64 wc)
{
   m_MaxConcreteUnitWeight[type] = wc;
}

Float64 SpecLibraryEntry::GetMaxConcreteUnitWeight(pgsTypes::ConcreteType type) const
{
   return m_MaxConcreteUnitWeight[type];
}

void SpecLibraryEntry::SetMaxConcreteAggSize(pgsTypes::ConcreteType type,Float64 agg)
{
   m_MaxConcreteAggSize[type] = agg;
}

Float64 SpecLibraryEntry::GetMaxConcreteAggSize(pgsTypes::ConcreteType type) const
{
   return m_MaxConcreteAggSize[type];
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

void SpecLibraryEntry::SetShearFlowMethod(ShearFlowMethod method)
{
   m_ShearFlowMethod = method;
}

ShearFlowMethod SpecLibraryEntry::GetShearFlowMethod() const
{
   return m_ShearFlowMethod;
}

void SpecLibraryEntry::SetShearCapacityMethod(ShearCapacityMethod method)
{
   m_ShearCapacityMethod = method;
}

ShearCapacityMethod SpecLibraryEntry::GetShearCapacityMethod() const
{
   return m_ShearCapacityMethod;
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

void SpecLibraryEntry::LimitDistributionFactorsToLanesBeams(bool bLimit)
{
   m_LimitDistributionFactorsToLanesBeams = bLimit;
}

bool SpecLibraryEntry::LimitDistributionFactorsToLanesBeams() const
{
   return m_LimitDistributionFactorsToLanesBeams;
}

pgsTypes::PrestressTransferComputationType SpecLibraryEntry::GetPrestressTransferComputationType() const
{
   return m_PrestressTransferComputationType;
}

void SpecLibraryEntry::SetPrestressTransferComputationType(pgsTypes::PrestressTransferComputationType type)
{
   m_PrestressTransferComputationType = type;
}

void SpecLibraryEntry::SetFlexureResistanceFactors(pgsTypes::ConcreteType type,Float64 phiTensionPS,Float64 phiTensionRC,Float64 phiCompression)
{
   m_PhiFlexureTensionPS[type]   = phiTensionPS;
   m_PhiFlexureTensionRC[type]   = phiTensionRC;
   m_PhiFlexureCompression[type] = phiCompression;
}

void SpecLibraryEntry::GetFlexureResistanceFactors(pgsTypes::ConcreteType type,Float64* phiTensionPS,Float64* phiTensionRC,Float64* phiCompression) const
{
   *phiTensionPS   = m_PhiFlexureTensionPS[type];
   *phiTensionRC   = m_PhiFlexureTensionRC[type];
   *phiCompression = m_PhiFlexureCompression[type];
}

void SpecLibraryEntry::SetShearResistanceFactor(pgsTypes::ConcreteType type,Float64 phi)
{
   m_PhiShear[type] = phi;
}

Float64 SpecLibraryEntry::GetShearResistanceFactor(pgsTypes::ConcreteType type) const
{
   return m_PhiShear[type];
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

//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void SpecLibraryEntry::MakeCopy(const SpecLibraryEntry& rOther)
{
   m_SpecificationType          = rOther.m_SpecificationType;
   m_SpecificationUnits         = rOther.m_SpecificationUnits;
   m_Description.erase();
   m_Description                = rOther.m_Description;
   m_DoCheckStrandSlope         = rOther.m_DoCheckStrandSlope;
   m_DoDesignStrandSlope        = rOther.m_DoDesignStrandSlope;
   m_MaxSlope05                 = rOther.m_MaxSlope05;
   m_MaxSlope06                 = rOther.m_MaxSlope06;
   m_MaxSlope07                 = rOther.m_MaxSlope07;
   m_DoCheckHoldDown            = rOther.m_DoCheckHoldDown;
   m_DoDesignHoldDown           = rOther.m_DoDesignHoldDown;
   m_HoldDownForce              = rOther.m_HoldDownForce;
   m_MaxStirrupSpacing          = rOther.m_MaxStirrupSpacing;
   m_DoCheckConfinement         = rOther.m_DoCheckConfinement;
   m_DoDesignConfinement        = rOther.m_DoDesignConfinement;
   m_DoCheckSplitting           = rOther.m_DoCheckSplitting;
   m_DoDesignSplitting          = rOther.m_DoDesignSplitting;
   m_CyLiftingCrackFs           = rOther.m_CyLiftingCrackFs;
   m_CyLiftingFailFs            = rOther.m_CyLiftingFailFs;
   m_CyCompStressServ           = rOther.m_CyCompStressServ;
   m_CyCompStressLifting        = rOther.m_CyCompStressLifting;
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
   m_MaxGirderSweepHauling      = rOther.m_MaxGirderSweepHauling;
   m_MaxHaulingOverhang         = rOther.m_MaxHaulingOverhang;
   m_HaulingSupportDistance          = rOther.m_HaulingSupportDistance;
   m_HaulingSupportPlacementTolerance= rOther.m_HaulingSupportPlacementTolerance;
   m_HaulingCamberPercentEstimate    = rOther.m_HaulingCamberPercentEstimate;
   m_CompStressHauling          = rOther.m_CompStressHauling;
   m_TensStressHauling          = rOther.m_TensStressHauling;
   m_DoTensStressHaulingMax     = rOther.m_DoTensStressHaulingMax;
   m_TensStressHaulingMax       = rOther.m_TensStressHaulingMax;
   m_HeHaulingCrackFs           = rOther.m_HeHaulingCrackFs;
   m_HeHaulingRollFs            = rOther.m_HeHaulingRollFs;

   m_HaulingModulusOfRuptureCoefficient[pgsTypes::Normal] = rOther.m_HaulingModulusOfRuptureCoefficient[pgsTypes::Normal];
   m_HaulingModulusOfRuptureCoefficient[pgsTypes::AllLightweight] = rOther.m_HaulingModulusOfRuptureCoefficient[pgsTypes::AllLightweight];
   m_HaulingModulusOfRuptureCoefficient[pgsTypes::SandLightweight] = rOther.m_HaulingModulusOfRuptureCoefficient[pgsTypes::SandLightweight];

   m_LiftingModulusOfRuptureCoefficient[pgsTypes::Normal] = rOther.m_LiftingModulusOfRuptureCoefficient[pgsTypes::Normal];
   m_LiftingModulusOfRuptureCoefficient[pgsTypes::AllLightweight] = rOther.m_LiftingModulusOfRuptureCoefficient[pgsTypes::AllLightweight];
   m_LiftingModulusOfRuptureCoefficient[pgsTypes::SandLightweight] = rOther.m_LiftingModulusOfRuptureCoefficient[pgsTypes::SandLightweight];

   m_CyTensStressServWithRebar  = rOther.m_CyTensStressServWithRebar;
   m_TensStressLiftingWithRebar = rOther.m_TensStressLiftingWithRebar;
   m_TensStressHaulingWithRebar = rOther.m_TensStressHaulingWithRebar;

   m_TruckRollStiffnessMethod   = rOther.m_TruckRollStiffnessMethod;
   m_TruckRollStiffness         = rOther.m_TruckRollStiffness;
   m_AxleWeightLimit            = rOther.m_AxleWeightLimit;
   m_AxleStiffness              = rOther.m_AxleStiffness;
   m_MinRollStiffness           = rOther.m_MinRollStiffness;

   m_TruckGirderHeight          = rOther.m_TruckGirderHeight;
   m_TruckRollCenterHeight      = rOther.m_TruckRollCenterHeight;
   m_TruckAxleWidth             = rOther.m_TruckAxleWidth;
   m_HeErectionCrackFs          = rOther.m_HeErectionCrackFs;
   m_HeErectionFailFs           = rOther.m_HeErectionFailFs;
   m_RoadwaySuperelevation      = rOther.m_RoadwaySuperelevation;
   m_MaxGirderWgt               = rOther.m_MaxGirderWgt;

   m_TempStrandRemovalCompStress              = rOther.m_TempStrandRemovalCompStress;
   m_TempStrandRemovalTensStress              = rOther.m_TempStrandRemovalTensStress;
   m_TempStrandRemovalDoTensStressMax         = rOther.m_TempStrandRemovalDoTensStressMax;
   m_TempStrandRemovalTensStressMax           = rOther.m_TempStrandRemovalTensStressMax;

   m_Bs1CompStress              = rOther.m_Bs1CompStress;
   m_Bs1TensStress              = rOther.m_Bs1TensStress;
   m_Bs1DoTensStressMax         = rOther.m_Bs1DoTensStressMax;
   m_Bs1TensStressMax           = rOther.m_Bs1TensStressMax;
   m_Bs2CompStress              = rOther.m_Bs2CompStress;
   m_TrafficBarrierDistributionType = rOther.m_TrafficBarrierDistributionType;
   m_Bs2MaxGirdersTrafficBarrier= rOther.m_Bs2MaxGirdersTrafficBarrier;
   m_Bs2MaxGirdersUtility       = rOther.m_Bs2MaxGirdersUtility;
   m_OverlayLoadDistribution    = rOther.m_OverlayLoadDistribution;
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
   m_FlexureModulusOfRuptureCoefficient[pgsTypes::Normal] = rOther.m_FlexureModulusOfRuptureCoefficient[pgsTypes::Normal];
   m_FlexureModulusOfRuptureCoefficient[pgsTypes::AllLightweight] = rOther.m_FlexureModulusOfRuptureCoefficient[pgsTypes::AllLightweight];
   m_FlexureModulusOfRuptureCoefficient[pgsTypes::SandLightweight] = rOther.m_FlexureModulusOfRuptureCoefficient[pgsTypes::SandLightweight];
   m_ShearModulusOfRuptureCoefficient[pgsTypes::Normal] = rOther.m_ShearModulusOfRuptureCoefficient[pgsTypes::Normal];
   m_ShearModulusOfRuptureCoefficient[pgsTypes::AllLightweight] = rOther.m_ShearModulusOfRuptureCoefficient[pgsTypes::AllLightweight];
   m_ShearModulusOfRuptureCoefficient[pgsTypes::SandLightweight] = rOther.m_ShearModulusOfRuptureCoefficient[pgsTypes::SandLightweight];

   m_CreepMethod                = rOther.m_CreepMethod;
   m_XferTime                   = rOther.m_XferTime;
   m_CreepFactor                = rOther.m_CreepFactor;
   m_CreepDuration1Min          = rOther.m_CreepDuration1Min;
   m_CreepDuration1Max          = rOther.m_CreepDuration1Max;
   m_CreepDuration2Min          = rOther.m_CreepDuration2Min;
   m_CreepDuration2Max          = rOther.m_CreepDuration2Max;
   m_TotalCreepDuration  = rOther.m_TotalCreepDuration;

   m_LossMethod                 = rOther.m_LossMethod;
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
   m_LongReinfShearMethod       = rOther.m_LongReinfShearMethod;

   m_bCheckStrandStress[AT_JACKING]       = rOther.m_bCheckStrandStress[AT_JACKING];
   m_bCheckStrandStress[BEFORE_TRANSFER]  = rOther.m_bCheckStrandStress[BEFORE_TRANSFER];
   m_bCheckStrandStress[AFTER_TRANSFER]   = rOther.m_bCheckStrandStress[AFTER_TRANSFER];
   m_bCheckStrandStress[AFTER_ALL_LOSSES] = rOther.m_bCheckStrandStress[AFTER_ALL_LOSSES];

   m_StrandStressCoeff[AT_JACKING][STRESS_REL]       = rOther.m_StrandStressCoeff[AT_JACKING][STRESS_REL];
   m_StrandStressCoeff[AT_JACKING][LOW_RELAX]        = rOther.m_StrandStressCoeff[AT_JACKING][LOW_RELAX];
   m_StrandStressCoeff[BEFORE_TRANSFER][STRESS_REL]  = rOther.m_StrandStressCoeff[BEFORE_TRANSFER][STRESS_REL];
   m_StrandStressCoeff[BEFORE_TRANSFER][LOW_RELAX]   = rOther.m_StrandStressCoeff[BEFORE_TRANSFER][LOW_RELAX];
   m_StrandStressCoeff[AFTER_TRANSFER][STRESS_REL]   = rOther.m_StrandStressCoeff[AFTER_TRANSFER][STRESS_REL];
   m_StrandStressCoeff[AFTER_TRANSFER][LOW_RELAX]    = rOther.m_StrandStressCoeff[AFTER_TRANSFER][LOW_RELAX];
   m_StrandStressCoeff[AFTER_ALL_LOSSES][STRESS_REL] = rOther.m_StrandStressCoeff[AFTER_ALL_LOSSES][STRESS_REL];
   m_StrandStressCoeff[AFTER_ALL_LOSSES][LOW_RELAX]  = rOther.m_StrandStressCoeff[AFTER_ALL_LOSSES][LOW_RELAX];

   m_bDoEvaluateDeflection = rOther.m_bDoEvaluateDeflection;
   m_DeflectionLimit       = rOther.m_DeflectionLimit;

   m_bIncludeRebar_Moment = rOther.m_bIncludeRebar_Moment;
   m_bIncludeRebar_Shear = rOther.m_bIncludeRebar_Shear;

//   m_AnalysisType = rOther.m_AnalysisType;

   for ( int i = 0; i < 3; i++ )
   {
      m_MaxSlabFc[i]             = rOther.m_MaxSlabFc[i];
      m_MaxGirderFci[i]          = rOther.m_MaxGirderFci[i];
      m_MaxGirderFc[i]           = rOther.m_MaxGirderFc[i];
      m_MaxConcreteUnitWeight[i] = rOther.m_MaxConcreteUnitWeight[i];
      m_MaxConcreteAggSize[i]    = rOther.m_MaxConcreteAggSize[i];
   }

   m_EnableSlabOffsetCheck = rOther.m_EnableSlabOffsetCheck;
   m_EnableSlabOffsetDesign = rOther.m_EnableSlabOffsetDesign;

   m_DesignStrandFillType = rOther.m_DesignStrandFillType;
   m_EffFlangeWidthMethod = rOther.m_EffFlangeWidthMethod;

   m_ShearFlowMethod = rOther.m_ShearFlowMethod;

   m_ShearCapacityMethod = rOther.m_ShearCapacityMethod;

   m_CuringMethodTimeAdjustmentFactor = rOther.m_CuringMethodTimeAdjustmentFactor;

   m_MinLiftPoint      = rOther.m_MinLiftPoint;
   m_LiftPointAccuracy = rOther.m_LiftPointAccuracy;
   m_MinHaulPoint      = rOther.m_MinHaulPoint;
   m_HaulPointAccuracy = rOther.m_HaulPointAccuracy;

   m_PedestrianLoad   = rOther.m_PedestrianLoad;
   m_MinSidewalkWidth = rOther.m_MinSidewalkWidth;

   m_MaxAngularDeviationBetweenGirders = rOther.m_MaxAngularDeviationBetweenGirders;
   m_MinGirderStiffnessRatio           = rOther.m_MinGirderStiffnessRatio;
   m_LLDFGirderSpacingLocation         = rOther.m_LLDFGirderSpacingLocation;

   m_LimitDistributionFactorsToLanesBeams = rOther.m_LimitDistributionFactorsToLanesBeams;

   m_PrestressTransferComputationType = rOther.m_PrestressTransferComputationType;

   m_RelaxationLossMethod = rOther.m_RelaxationLossMethod;

   for ( int i = 0; i < 3; i++ )
   {
      m_PhiFlexureTensionPS[i]   = rOther.m_PhiFlexureTensionPS[i];
      m_PhiFlexureTensionRC[i]   = rOther.m_PhiFlexureTensionRC[i];
      m_PhiFlexureCompression[i] = rOther.m_PhiFlexureCompression[i];
      m_PhiShear[i]              = rOther.m_PhiShear[i];
   }

   m_bIncludeForNegMoment = rOther.m_bIncludeForNegMoment;
   m_bAllowStraightStrandExtensions = rOther.m_bAllowStraightStrandExtensions;
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
//======================== ACCESS     =======================================
//======================== INQUERY    =======================================
//
