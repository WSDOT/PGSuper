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

#include "StdAfx.h"
#include <psgLib\SpecLibraryEntry.h>
#include "resource.h"
#include "SpecMainSheet.h"

#include <System\IStructuredSave.h>
#include <System\IStructuredLoad.h>
#include <System\XStructuredLoad.h>
#include <Units\sysUnits.h>

#include <MathEx.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define CURRENT_VERSION 36.0

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
m_DoCheckAnchorage(true),
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
m_Dset(::ConvertToSysUnits(0.375,unitMeasure::Inch)),
m_WobbleFriction(::ConvertToSysUnits(0.0002,unitMeasure::PerFeet)),
m_FrictionCoefficient(0.25),
m_LongReinfShearMethod(LRFD_METHOD),
m_bDoEvaluateDeflection(true),
m_DeflectionLimit(800.0),
m_bIncludeRebar_Moment(false),
m_bIncludeRebar_Shear(false),
m_AnalysisType(pgsTypes::Envelope),
m_FlexureModulusOfRuptureCoefficient(::ConvertToSysUnits(0.37,unitMeasure::SqrtKSI)),
m_ShearModulusOfRuptureCoefficient(::ConvertToSysUnits(0.20,unitMeasure::SqrtKSI)),
m_HaulingModulusOfRuptureCoefficient(::ConvertToSysUnits(0.24,unitMeasure::SqrtKSI)),
m_LiftingModulusOfRuptureCoefficient(::ConvertToSysUnits(0.24,unitMeasure::SqrtKSI)),
m_MaxSlabFc(::ConvertToSysUnits(6.0,unitMeasure::KSI)),
m_MaxGirderFci(::ConvertToSysUnits(7.5,unitMeasure::KSI)),
m_MaxGirderFc(::ConvertToSysUnits(10.0,unitMeasure::KSI)),          
m_MaxConcreteUnitWeight(::ConvertToSysUnits(165.,unitMeasure::LbfPerFeet3)),
m_MaxConcreteAggSize(::ConvertToSysUnits(1.5,unitMeasure::Inch)),
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
m_PrestressTransferComputationType(pgsTypes::ptUsingSpecification)
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
   int i = dlg.DoModal();
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
   pSave->BeginUnit("SpecificationLibraryEntry", CURRENT_VERSION);
   pSave->Property("Name",this->GetName().c_str());
   pSave->Property("Description",this->GetDescription().c_str());

   if (m_SpecificationType==lrfdVersionMgr::FirstEdition1994)
      pSave->Property("SpecificationType", "AashtoLrfd1994");
   else if (m_SpecificationType==lrfdVersionMgr::FirstEditionWith1996Interims)
      pSave->Property("SpecificationType", "AashtoLrfd1996");
   else if (m_SpecificationType==lrfdVersionMgr::FirstEditionWith1997Interims)
      pSave->Property("SpecificationType", "AashtoLrfd1997");
   else if (m_SpecificationType==lrfdVersionMgr::SecondEdition1998)
      pSave->Property("SpecificationType", "AashtoLrfd1998");
   else if (m_SpecificationType==lrfdVersionMgr::SecondEditionWith1999Interims)
      pSave->Property("SpecificationType", "AashtoLrfd1999");
   else if (m_SpecificationType==lrfdVersionMgr::SecondEditionWith2000Interims)
      pSave->Property("SpecificationType", "AashtoLrfd2000");
   else if (m_SpecificationType==lrfdVersionMgr::SecondEditionWith2001Interims)
      pSave->Property("SpecificationType", "AashtoLrfd2001");
   else if (m_SpecificationType==lrfdVersionMgr::SecondEditionWith2002Interims)
      pSave->Property("SpecificationType", "AashtoLrfd2002");
   else if (m_SpecificationType==lrfdVersionMgr::SecondEditionWith2003Interims)
      pSave->Property("SpecificationType", "AashtoLrfd2003");
   else if (m_SpecificationType==lrfdVersionMgr::ThirdEdition2004)
      pSave->Property("SpecificationType", "AashtoLrfd2004");
   else if (m_SpecificationType==lrfdVersionMgr::ThirdEditionWith2005Interims)
      pSave->Property("SpecificationType", "AashtoLrfd2005");
   else if (m_SpecificationType==lrfdVersionMgr::ThirdEditionWith2006Interims)
      pSave->Property("SpecificationType", "AashtoLrfd2006");
   else if (m_SpecificationType==lrfdVersionMgr::FourthEdition2007)
      pSave->Property("SpecificationType", "AashtoLrfd2007");
   else if (m_SpecificationType==lrfdVersionMgr::FourthEditionWith2008Interims)
      pSave->Property("SpecificationType", "AashtoLrfd2008");
   else if (m_SpecificationType==lrfdVersionMgr::FourthEditionWith2009Interims)
      pSave->Property("SpecificationType", "AashtoLrfd2009");
   else
      ASSERT(0);

   if (m_SpecificationUnits==lrfdVersionMgr::SI)
      pSave->Property("SpecificationUnits", "SiUnitsSpec");
   else if (m_SpecificationUnits==lrfdVersionMgr::US)
      pSave->Property("SpecificationUnits", "UsUnitsSpec");
   else
      ASSERT(0);

   pSave->Property("DoCheckStrandSlope", m_DoCheckStrandSlope);
   pSave->Property("DoDesignStrandSlope", m_DoDesignStrandSlope);
   pSave->Property("MaxSlope05", m_MaxSlope05);
   pSave->Property("MaxSlope06", m_MaxSlope06);
   pSave->Property("MaxSlope07", m_MaxSlope07); // added in version 35
   pSave->Property("DoCheckHoldDown", m_DoCheckHoldDown);
   pSave->Property("DoDesignHoldDown", m_DoDesignHoldDown);
   pSave->Property("HoldDownForce", m_HoldDownForce);
   pSave->Property("DoCheckAnchorage", m_DoCheckAnchorage);
   pSave->Property("MaxStirrupSpacing", m_MaxStirrupSpacing);
   pSave->Property("CyLiftingCrackFs", m_CyLiftingCrackFs);
   pSave->Property("CyLiftingFailFs", m_CyLiftingFailFs);
   pSave->Property("CyCompStressServ", m_CyCompStressServ);
   pSave->Property("CyCompStressLifting", m_CyCompStressLifting);
   pSave->Property("CyTensStressServ", m_CyTensStressServ);
   pSave->Property("CyDoTensStressServMax", m_CyDoTensStressServMax);
   pSave->Property("CyTensStressServMax", m_CyTensStressServMax);
   pSave->Property("CyTensStressLifting", m_CyTensStressLifting);
   pSave->Property("CyDoTensStressLiftingMax",m_CyDoTensStressLiftingMax);
   pSave->Property("CyTensStressLiftingMax",m_CyTensStressLiftingMax);
   pSave->Property("BurstingZoneLengthFactor", m_SplittingZoneLengthFactor),
   pSave->Property("LiftingUpwardImpact",m_LiftingUpwardImpact);
   pSave->Property("LiftingDownwardImpact",m_LiftingDownwardImpact);
   pSave->Property("HaulingUpwardImpact",m_HaulingUpwardImpact);
   pSave->Property("HaulingDownwardImpact",m_HaulingDownwardImpact);
   pSave->Property("CuringMethod", (Int16)m_CuringMethod);
   pSave->Property("EnableLiftingCheck", m_EnableLiftingCheck);
   pSave->Property("EnableLiftingDesign", m_EnableLiftingDesign);
   pSave->Property("PickPointHeight", m_PickPointHeight);
   pSave->Property("LiftingLoopTolerance",m_LiftingLoopTolerance);
   pSave->Property("MinCableInclination",m_MinCableInclination);
   pSave->Property("MaxGirderSweepLifting", m_MaxGirderSweepLifting);
   pSave->Property("EnableHaulingCheck", m_EnableHaulingCheck);
   pSave->Property("EnableHaulingDesign", m_EnableHaulingDesign);
   pSave->Property("MaxGirderSweepHauling", m_MaxGirderSweepHauling);
   pSave->Property("HaulingSupportDistance",m_HaulingSupportDistance);
   pSave->Property("MaxHaulingOverhang", m_MaxHaulingOverhang);
   pSave->Property("HaulingSupportPlacementTolerance",m_HaulingSupportPlacementTolerance);
   pSave->Property("HaulingCamberPercentEstimate",m_HaulingCamberPercentEstimate);
   pSave->Property("CompStressHauling", m_CompStressHauling);
   pSave->Property("TensStressHauling",m_TensStressHauling);
   pSave->Property("DoTensStressHaulingMax",m_DoTensStressHaulingMax); 
   pSave->Property("TensStressHaulingMax", m_TensStressHaulingMax);
   pSave->Property("HeHaulingCrackFs", m_HeHaulingCrackFs);
   pSave->Property("HeHaulingFailFs", m_HeHaulingRollFs);
   pSave->Property("RoadwaySuperelevation", m_RoadwaySuperelevation);
   pSave->Property("TruckRollStiffnessMethod", (long)m_TruckRollStiffnessMethod);
   pSave->Property("TruckRollStiffness", m_TruckRollStiffness);
   pSave->Property("AxleWeightLimit", m_AxleWeightLimit);
   pSave->Property("AxleStiffness",m_AxleStiffness);
   pSave->Property("MinRollStiffness",m_MinRollStiffness);
   pSave->Property("TruckGirderHeight", m_TruckGirderHeight);
   pSave->Property("TruckRollCenterHeight", m_TruckRollCenterHeight);
   pSave->Property("TruckAxleWidth", m_TruckAxleWidth);
   pSave->Property("HeErectionCrackFs", m_HeErectionCrackFs);
   pSave->Property("HeErectionFailFs", m_HeErectionFailFs);
   pSave->Property("MaxGirderWgt",m_MaxGirderWgt);
   pSave->Property("HaulingModulusOfRuptureCoefficient",m_HaulingModulusOfRuptureCoefficient); // added for version 12.0
   pSave->Property("LiftingModulusOfRuptureCoefficient",m_LiftingModulusOfRuptureCoefficient); // added for version 20.0

   // Added at version 25
   pSave->Property("MinLiftingPointLocation",       m_MinLiftPoint        );
   pSave->Property("LiftingPointLocationAccuracy",  m_LiftPointAccuracy   );
   pSave->Property("MinHaulingSupportLocation",     m_MinHaulPoint        );
   pSave->Property("HaulingSupportLocationAccuracy",m_HaulPointAccuracy   );

   // Added at version 4.0
   pSave->Property("CastingYardTensileStressLimitWithMildRebar",m_CyTensStressServWithRebar);
   pSave->Property("LiftingTensileStressLimitWithMildRebar",m_TensStressLiftingWithRebar);
   pSave->Property("HaulingTensileStressLimitWithMildRebar",m_TensStressHaulingWithRebar);

   // Added at version 30
   pSave->Property("TempStrandRemovalCompStress" ,     m_TempStrandRemovalCompStress);
   pSave->Property("TempStrandRemovalTensStress" ,     m_TempStrandRemovalTensStress);
   pSave->Property("TempStrandRemovalDoTensStressMax" ,m_TempStrandRemovalDoTensStressMax);
   pSave->Property("TempStrandRemovalTensStressMax" ,  m_TempStrandRemovalTensStressMax);

   pSave->Property("Bs1CompStress" ,     m_Bs1CompStress); // removed m_ in version 30
   pSave->Property("Bs1TensStress" ,     m_Bs1TensStress); // removed m_ in version 30
   pSave->Property("Bs1DoTensStressMax" ,m_Bs1DoTensStressMax); // removed m_ in version 30
   pSave->Property("Bs1TensStressMax" ,  m_Bs1TensStressMax); // removed m_ in version 30
   pSave->Property("Bs2CompStress" ,     m_Bs2CompStress); // removed m_ in version 30
   pSave->Property("Bs2TrafficBarrierDistributionType",(Int16)m_TrafficBarrierDistributionType); // added in version 36
   pSave->Property("Bs2MaxGirdersTrafficBarrier", m_Bs2MaxGirdersTrafficBarrier);
   pSave->Property("Bs2MaxGirdersUtility", m_Bs2MaxGirdersUtility);
   pSave->Property("OverlayLoadDistribution", (Int32)m_OverlayLoadDistribution); // added in version 34
   pSave->Property("Bs3CompStressServ", m_Bs3CompStressServ);
   pSave->Property("Bs3CompStressService1A", m_Bs3CompStressService1A);
   pSave->Property("Bs3TensStressServNc", m_Bs3TensStressServNc);
   pSave->Property("Bs3DoTensStressServNcMax", m_Bs3DoTensStressServNcMax);
   pSave->Property("Bs3TensStressServNcMax", m_Bs3TensStressServNcMax);
   pSave->Property("Bs3TensStressServSc",    m_Bs3TensStressServSc);   
   pSave->Property("Bs3DoTensStressServScMax", m_Bs3DoTensStressServScMax);
   pSave->Property("Bs3TensStressServScMax", m_Bs3TensStressServScMax);

   // removed in version 29
   // pSave->Property("Bs3IgnoreRangeOfApplicability", m_Bs3IgnoreRangeOfApplicability);

   pSave->Property("Bs3LRFDOverreinforcedMomentCapacity",(Int16)m_Bs3LRFDOverReinforcedMomentCapacity);
   pSave->Property("IncludeRebar_MomentCapacity",m_bIncludeRebar_Moment); // added for version 7.0
   pSave->Property("ModulusOfRuptureCoefficient",m_FlexureModulusOfRuptureCoefficient); // added for version 9.0
   pSave->Property("ShearModulusOfRuptureCoefficient",m_ShearModulusOfRuptureCoefficient); // added for version 18

   pSave->Property("BsLldfMethod", (Int16)m_LldfMethod );  // added LLDF_TXDOT for version 21.0
   // added in version 29
   pSave->Property("MaxAngularDeviationBetweenGirders",m_MaxAngularDeviationBetweenGirders);
   pSave->Property("MinGirderStiffnessRatio",m_MinGirderStiffnessRatio);
   pSave->Property("LLDFGirderSpacingLocation",m_LLDFGirderSpacingLocation);

   // added in version 31
   pSave->Property("LimitDistributionFactorsToLanesBeams",m_LimitDistributionFactorsToLanesBeams);

   pSave->Property("LongReinfShearMethod",(Int16)m_LongReinfShearMethod); // added for version 1.2
   pSave->Property("IncludeRebar_Shear",m_bIncludeRebar_Shear); // added for version 7.0
   pSave->Property("CreepMethod", (Int16)m_CreepMethod );
   pSave->Property("CreepFactor",m_CreepFactor);
   pSave->Property("CreepDuration1Min",m_CreepDuration1Min);
   pSave->Property("CreepDuration1Max",m_CreepDuration1Max);
   pSave->Property("CreepDuration2Min",m_CreepDuration2Min);
   pSave->Property("CreepDuration2Max",m_CreepDuration2Max);
   pSave->Property("XferTime",m_XferTime);
   pSave->Property("TotalCreepDuration",m_TotalCreepDuration);

   pSave->Property("LossMethod",(Int16)m_LossMethod);
   pSave->Property("FinalLosses",m_FinalLosses);
   pSave->Property("ShippingLosses",m_ShippingLosses);
   pSave->Property("BeforeXferLosses",m_BeforeXferLosses);
   pSave->Property("AfterXferLosses",m_AfterXferLosses);
   pSave->Property("ShippingTime",m_ShippingTime);

   // added in version 22
   pSave->Property("LiftingLosses",m_LiftingLosses);
   pSave->Property("BeforeTempStrandRemovalLosses",m_BeforeTempStrandRemovalLosses);
   pSave->Property("AfterTempStrandRemovalLosses",m_AfterTempStrandRemovalLosses);
   pSave->Property("AfterDeckPlacementLosses",m_AfterDeckPlacementLosses);


   pSave->Property("CuringMethodFactor",m_CuringMethodTimeAdjustmentFactor);

   // Added in version 1.5
   pSave->Property("CheckStrandStressAtJacking",m_bCheckStrandStress[AT_JACKING]);
   pSave->Property("Coeff_AtJacking_StressRel",m_StrandStressCoeff[AT_JACKING][STRESS_REL]);
   pSave->Property("Coeff_AtJacking_LowRelax",m_StrandStressCoeff[AT_JACKING][LOW_RELAX]);

   pSave->Property("CheckStrandStressBeforeTransfer",m_bCheckStrandStress[BEFORE_TRANSFER]);
   pSave->Property("Coeff_BeforeTransfer_StressRel",m_StrandStressCoeff[BEFORE_TRANSFER][STRESS_REL]);
   pSave->Property("Coeff_BeforeTransfer_LowRelax",m_StrandStressCoeff[BEFORE_TRANSFER][LOW_RELAX]);

   pSave->Property("CheckStrandStressAfterTransfer",m_bCheckStrandStress[AFTER_TRANSFER]);
   pSave->Property("Coeff_AfterTransfer_StressRel",m_StrandStressCoeff[AFTER_TRANSFER][STRESS_REL]);
   pSave->Property("Coeff_AfterTransfer_LowRelax",m_StrandStressCoeff[AFTER_TRANSFER][LOW_RELAX]);

   pSave->Property("CheckStrandStressAfterAllLosses",m_bCheckStrandStress[AFTER_ALL_LOSSES]);
   pSave->Property("Coeff_AfterAllLosses_StressRel",m_StrandStressCoeff[AFTER_ALL_LOSSES][STRESS_REL]);
   pSave->Property("Coeff_AfterAllLosses_LowRelax",m_StrandStressCoeff[AFTER_ALL_LOSSES][LOW_RELAX]);

   // added in version 23
   pSave->Property("AnchorSet",m_Dset);
   pSave->Property("WobbleFriction",m_WobbleFriction);
   pSave->Property("CoefficientOfFriction",m_FrictionCoefficient);

   // Added in 1.7
   pSave->Property("CheckLiveLoadDeflection",m_bDoEvaluateDeflection);
   pSave->Property("LiveLoadDeflectionLimit",m_DeflectionLimit);

//   // Added in 8.0, removed in version 28
//   pSave->Property("AnalysisType",(long)m_AnalysisType);

   // Added in 10.0
   pSave->Property("MaxSlabFc",m_MaxSlabFc);
   pSave->Property("MaxGirderFci",m_MaxGirderFci);
   pSave->Property("MaxGirderFc",m_MaxGirderFc);
   pSave->Property("MaxConcreteUnitWeight",m_MaxConcreteUnitWeight);
   pSave->Property("MaxConcreteAggSize",m_MaxConcreteAggSize);


   // Added in 14.0
   std::string strLimitState[] = {"ServiceI","ServiceIA","ServiceIII","StrengthI","StrengthII","FatigueI"};

   pSave->BeginUnit("LoadFactors",3.0);
   int nLimitStates = sizeof(strLimitState)/sizeof(std::string); // Added StrengthII in version 26
   for ( int i = 0; i < nLimitStates; i++ )
   {
      pSave->BeginUnit(strLimitState[i].c_str(),1.0);
      
      pSave->Property("DCmin",  m_DCmin[i]);
      pSave->Property("DCmax",  m_DCmax[i]);
      pSave->Property("DWmin",  m_DWmin[i]);
      pSave->Property("DWmax",  m_DWmax[i]);
      pSave->Property("LLIMmin",m_LLIMmin[i]);
      pSave->Property("LLIMmax",m_LLIMmax[i]);

      pSave->EndUnit();
   }
   pSave->EndUnit();

   // added in version 15
   pSave->Property("EnableSlabOffsetCheck", m_EnableSlabOffsetCheck);
   pSave->Property("EnableSlabOffsetDesign", m_EnableSlabOffsetDesign);

   pSave->Property("DesignStrandFillType", (long)m_DesignStrandFillType);

   // added in version 16
   pSave->Property("EffectiveFlangeWidthMethod",(long)m_EffFlangeWidthMethod);

//   // added in version 17, removed version 24
//   pSave->Property("SlabOffsetMethod",(long)m_SlabOffsetMethod);

   // added in version 18
   pSave->Property("ShearFlowMethod",(long)m_ShearFlowMethod);
   pSave->Property("ShearCapacityMethod",(long)m_ShearCapacityMethod);

   // added in version 26
   pSave->Property("PedestrianLoad",m_PedestrianLoad);
   pSave->Property("MinSidewalkWidth",m_MinSidewalkWidth);

   // added in vers 32
   pSave->Property("PrestressTransferComputationType",(long)m_PrestressTransferComputationType);

   pSave->EndUnit();

   return true;
}

bool SpecLibraryEntry::LoadMe(sysIStructuredLoad* pLoad)
{
   Int16 temp;
   ShearCapacityMethod shear_capacity_method = m_ShearCapacityMethod; // used as a temporary storage for shear capacity method before file version 18

   if(pLoad->BeginUnit("SpecificationLibraryEntry"))
   {
      Float64 version = pLoad->GetVersion();
      if (version < 1.0 || CURRENT_VERSION < version)
         THROW_LOAD(BadVersion,pLoad);

      std::string name;
      if(pLoad->Property("Name",&name))
         this->SetName(name.c_str());
      else
         THROW_LOAD(InvalidFileFormat,pLoad);

      if(pLoad->Property("Description",&name))
         this->SetDescription(name.c_str());
      else
         THROW_LOAD(InvalidFileFormat,pLoad);

      std::string tmp;
      if(pLoad->Property("SpecificationType",&tmp))
      {
         if(tmp=="AashtoLrfd2009")
            m_SpecificationType = lrfdVersionMgr::FourthEditionWith2009Interims;
         else if(tmp=="AashtoLrfd2008")
            m_SpecificationType = lrfdVersionMgr::FourthEditionWith2008Interims;
         else if(tmp=="AashtoLrfd2007")
            m_SpecificationType = lrfdVersionMgr::FourthEdition2007;
         else if(tmp=="AashtoLrfd2006")
            m_SpecificationType = lrfdVersionMgr::ThirdEditionWith2006Interims;
         else if(tmp=="AashtoLrfd2005")
            m_SpecificationType = lrfdVersionMgr::ThirdEditionWith2005Interims;
         else if(tmp=="AashtoLrfd2004")
            m_SpecificationType = lrfdVersionMgr::ThirdEdition2004;
         else if(tmp=="AashtoLrfd2003")
            m_SpecificationType = lrfdVersionMgr::SecondEditionWith2003Interims;
         else if(tmp=="AashtoLrfd2002")
            m_SpecificationType = lrfdVersionMgr::SecondEditionWith2002Interims;
         else if(tmp=="AashtoLrfd2001")
            m_SpecificationType = lrfdVersionMgr::SecondEditionWith2001Interims;
         else if(tmp=="AashtoLrfd2000")
            m_SpecificationType = lrfdVersionMgr::SecondEditionWith2000Interims;
         else if(tmp=="AashtoLrfd1999")
            m_SpecificationType = lrfdVersionMgr::SecondEditionWith1999Interims;
         else if(tmp=="AashtoLrfd1998")
            m_SpecificationType = lrfdVersionMgr::SecondEdition1998;
         else if(tmp=="AashtoLrfd1997")
            m_SpecificationType = lrfdVersionMgr::FirstEditionWith1997Interims;
         else if(tmp=="AashtoLrfd1996")
            m_SpecificationType = lrfdVersionMgr::FirstEditionWith1996Interims;
         else if (tmp=="AashtoLrfd1994")
            m_SpecificationType = lrfdVersionMgr::FirstEdition1994;
         else
            THROW_LOAD(InvalidFileFormat,pLoad);
      }
      else
         THROW_LOAD(InvalidFileFormat,pLoad);


      if(pLoad->Property("SpecificationUnits",&tmp))
      {
         if (tmp=="SiUnitsSpec")
            m_SpecificationUnits = lrfdVersionMgr::SI;
         else if(tmp=="UsUnitsSpec")
            m_SpecificationUnits = lrfdVersionMgr::US;
         else
            THROW_LOAD(InvalidFileFormat,pLoad);
      }
      else
         THROW_LOAD(InvalidFileFormat,pLoad);

      if(!pLoad->Property("DoCheckStrandSlope", &m_DoCheckStrandSlope))
         THROW_LOAD(InvalidFileFormat,pLoad);

      if (version<15)
      {
         m_DoDesignStrandSlope = m_DoCheckStrandSlope;
      }
      else
      {
         if(!pLoad->Property("DoDesignStrandSlope", &m_DoDesignStrandSlope))
            THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if(!pLoad->Property("MaxSlope05", &m_MaxSlope05))
         THROW_LOAD(InvalidFileFormat,pLoad);

      if(!pLoad->Property("MaxSlope06", &m_MaxSlope06))
         THROW_LOAD(InvalidFileFormat,pLoad);

      if ( 35 <= version )
      {
         if(!pLoad->Property("MaxSlope07", &m_MaxSlope07))
            THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if(!pLoad->Property("DoCheckHoldDown", &m_DoCheckHoldDown))
         THROW_LOAD(InvalidFileFormat,pLoad);

      if (version<15)
      {
         m_DoDesignHoldDown = m_DoCheckHoldDown;
      }
      else
      {
         if(!pLoad->Property("DoDesignHoldDown", &m_DoDesignHoldDown))
            THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if(!pLoad->Property("HoldDownForce", &m_HoldDownForce))
         THROW_LOAD(InvalidFileFormat,pLoad);

      if (version>32)
      {
         if(!pLoad->Property("DoCheckAnchorage", &m_DoCheckAnchorage))
            THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if(!pLoad->Property("MaxStirrupSpacing", &m_MaxStirrupSpacing))
         THROW_LOAD(InvalidFileFormat,pLoad);

      if(!pLoad->Property("CyLiftingCrackFs", &m_CyLiftingCrackFs))
         THROW_LOAD(InvalidFileFormat,pLoad);

      if(!pLoad->Property("CyLiftingFailFs", &m_CyLiftingFailFs))
         THROW_LOAD(InvalidFileFormat,pLoad);

      if(!pLoad->Property("CyCompStressServ", &m_CyCompStressServ))
         THROW_LOAD(InvalidFileFormat,pLoad);

      if(!pLoad->Property("CyCompStressLifting", &m_CyCompStressLifting))
         THROW_LOAD(InvalidFileFormat,pLoad);

      if(!pLoad->Property("CyTensStressServ", &m_CyTensStressServ))
         THROW_LOAD(InvalidFileFormat,pLoad);

      if(!pLoad->Property("CyDoTensStressServMax", &m_CyDoTensStressServMax))
         THROW_LOAD(InvalidFileFormat,pLoad);

      if(!pLoad->Property("CyTensStressServMax", &m_CyTensStressServMax))
         THROW_LOAD(InvalidFileFormat,pLoad);

      if(!pLoad->Property("CyTensStressLifting", &m_CyTensStressLifting))
         THROW_LOAD(InvalidFileFormat,pLoad);

      if(!pLoad->Property("CyDoTensStressLiftingMax",&m_CyDoTensStressLiftingMax))
         THROW_LOAD(InvalidFileFormat,pLoad);

      if(!pLoad->Property("CyTensStressLiftingMax",&m_CyTensStressLiftingMax))
         THROW_LOAD(InvalidFileFormat,pLoad);

     // at version 27 we moved debonding to the girder library. Data here is ignored
	  if ( 6.0 <= version && 27 > version)
	  {
        double debond_junk;
		  if (!pLoad->Property("MaxDebondStrands",&debond_junk))
			 THROW_LOAD(InvalidFileFormat,pLoad);

		  if (!pLoad->Property("MaxDebondStrandsPerRow",&debond_junk))
			 THROW_LOAD(InvalidFileFormat,pLoad);

        if ( !pLoad->Property("MaxNumDebondedStrandsPerSection",&debond_junk))
           THROW_LOAD(InvalidFileFormat,pLoad);

        if ( !pLoad->Property("MaxDebondedStrandsPerSection",&debond_junk))
           THROW_LOAD(InvalidFileFormat,pLoad);

        if ( !pLoad->Property("DefaultDebondLength",&debond_junk))
           THROW_LOAD(InvalidFileFormat,pLoad);
	  }

      if ( 1.4 <= version )
      {
         if (!pLoad->Property("BurstingZoneLengthFactor", &m_SplittingZoneLengthFactor))
            THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if(!pLoad->Property("LiftingUpwardImpact",&m_LiftingUpwardImpact))
         THROW_LOAD(InvalidFileFormat,pLoad);

      if(!pLoad->Property("LiftingDownwardImpact",&m_LiftingDownwardImpact))
         THROW_LOAD(InvalidFileFormat,pLoad);

      if(!pLoad->Property("HaulingUpwardImpact",&m_HaulingUpwardImpact))
         THROW_LOAD(InvalidFileFormat,pLoad);

      if(!pLoad->Property("HaulingDownwardImpact",&m_HaulingDownwardImpact))
         THROW_LOAD(InvalidFileFormat,pLoad);

      if ( !pLoad->Property("CuringMethod", &temp ) )
         THROW_LOAD(InvalidFileFormat, pLoad );
      m_CuringMethod = temp;

      if (version<11)
      {
         m_EnableLiftingCheck = true;
         m_EnableLiftingDesign = true;
      }
      else
      {
         if(!pLoad->Property("EnableLiftingCheck", &m_EnableLiftingCheck))
            THROW_LOAD(InvalidFileFormat,pLoad);

         if (version<15)
         {
            m_EnableLiftingDesign = true;
         }
         else
         {
            if(!pLoad->Property("EnableLiftingDesign", &m_EnableLiftingDesign))
               THROW_LOAD(InvalidFileFormat,pLoad);
         }
      }

      if(!pLoad->Property("PickPointHeight", &m_PickPointHeight))
         THROW_LOAD(InvalidFileFormat,pLoad);

      if(!pLoad->Property("LiftingLoopTolerance", &m_LiftingLoopTolerance))
         THROW_LOAD(InvalidFileFormat,pLoad);

      if(!pLoad->Property("MinCableInclination", &m_MinCableInclination))
         THROW_LOAD(InvalidFileFormat,pLoad);

      if(!pLoad->Property("MaxGirderSweepLifting", &m_MaxGirderSweepLifting))
         THROW_LOAD(InvalidFileFormat,pLoad);

      if (version<11)
      {
         m_EnableHaulingCheck = true;
         m_EnableHaulingDesign = true;
      }
      else
      {
         if(!pLoad->Property("EnableHaulingCheck", &m_EnableHaulingCheck))
            THROW_LOAD(InvalidFileFormat,pLoad);

         if (version<15)
         {
            m_EnableHaulingDesign = true;
         }
         else
         {
            if(!pLoad->Property("EnableHaulingDesign", &m_EnableHaulingDesign))
               THROW_LOAD(InvalidFileFormat,pLoad);
         }
      }

      if(!pLoad->Property("MaxGirderSweepHauling", &m_MaxGirderSweepHauling))
         THROW_LOAD(InvalidFileFormat,pLoad);

      if(!pLoad->Property("HaulingSupportDistance", &m_HaulingSupportDistance))
         THROW_LOAD(InvalidFileFormat,pLoad);

      if ( 2.0 <= version )
      {
         if(!pLoad->Property("MaxHaulingOverhang", &m_MaxHaulingOverhang))
            THROW_LOAD(InvalidFileFormat,pLoad);
      }
      else
      {
         m_MaxHaulingOverhang = ::ConvertToSysUnits(15.0,unitMeasure::Feet);
      }

      if(!pLoad->Property("HaulingSupportPlacementTolerance", &m_HaulingSupportPlacementTolerance))
         THROW_LOAD(InvalidFileFormat,pLoad);

      if(!pLoad->Property("HaulingCamberPercentEstimate", &m_HaulingCamberPercentEstimate))
         THROW_LOAD(InvalidFileFormat,pLoad);

      if(!pLoad->Property("CompStressHauling", &m_CompStressHauling))
         THROW_LOAD(InvalidFileFormat,pLoad);

      if(!pLoad->Property("TensStressHauling", &m_TensStressHauling))
         THROW_LOAD(InvalidFileFormat,pLoad);

      if(!pLoad->Property("DoTensStressHaulingMax",&m_DoTensStressHaulingMax))
         THROW_LOAD(InvalidFileFormat,pLoad);

      if(!pLoad->Property("TensStressHaulingMax", &m_TensStressHaulingMax))
         THROW_LOAD(InvalidFileFormat,pLoad);

      if(!pLoad->Property("HeHaulingCrackFs", &m_HeHaulingCrackFs))
         THROW_LOAD(InvalidFileFormat,pLoad);

      if(!pLoad->Property("HeHaulingFailFs", &m_HeHaulingRollFs))
         THROW_LOAD(InvalidFileFormat,pLoad);

      if(!pLoad->Property("RoadwaySuperelevation", &m_RoadwaySuperelevation))
         THROW_LOAD(InvalidFileFormat,pLoad);

      if ( version < 1.9 )
      {
         if(!pLoad->Property("TruckRollStiffness", &m_TruckRollStiffness))
            THROW_LOAD(InvalidFileFormat,pLoad);

         m_TruckRollStiffnessMethod = 0;
      }
      else
      {
         long method;
         if(!pLoad->Property("TruckRollStiffnessMethod", &method))
            THROW_LOAD(InvalidFileFormat,pLoad);

         m_TruckRollStiffnessMethod = (int)method;

         if(!pLoad->Property("TruckRollStiffness", &m_TruckRollStiffness))
            THROW_LOAD(InvalidFileFormat,pLoad);

         if(!pLoad->Property("AxleWeightLimit", &m_AxleWeightLimit))
            THROW_LOAD(InvalidFileFormat,pLoad);

         if(!pLoad->Property("AxleStiffness", &m_AxleStiffness))
            THROW_LOAD(InvalidFileFormat,pLoad);

         if (!pLoad->Property("MinRollStiffness",&m_MinRollStiffness))
            THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if(!pLoad->Property("TruckGirderHeight", &m_TruckGirderHeight))
         THROW_LOAD(InvalidFileFormat,pLoad);

      if(!pLoad->Property("TruckRollCenterHeight", &m_TruckRollCenterHeight))
         THROW_LOAD(InvalidFileFormat,pLoad);

      if(!pLoad->Property("TruckAxleWidth", &m_TruckAxleWidth))
         THROW_LOAD(InvalidFileFormat,pLoad);

      if(!pLoad->Property("HeErectionCrackFs", &m_HeErectionCrackFs))
         THROW_LOAD(InvalidFileFormat,pLoad);

      if(!pLoad->Property("HeErectionFailFs", &m_HeErectionFailFs))
         THROW_LOAD(InvalidFileFormat,pLoad);

      if ( version >= 1.3 )
      {
         if (!pLoad->Property("MaxGirderWgt", &m_MaxGirderWgt))
            THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( 12 <= version )
      {
         if ( !pLoad->Property("HaulingModulusOfRuptureCoefficient",&m_HaulingModulusOfRuptureCoefficient) )
            THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( 20 <= version )
      {
         if ( !pLoad->Property("LiftingModulusOfRuptureCoefficient",&m_LiftingModulusOfRuptureCoefficient) )
            THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( 25 <= version )
      {
         // Added at version 25
         if ( !pLoad->Property("MinLiftingPointLocation",&m_MinLiftPoint) )
            THROW_LOAD(InvalidFileFormat,pLoad);

         if ( !pLoad->Property("LiftingPointLocationAccuracy",&m_LiftPointAccuracy) )
            THROW_LOAD(InvalidFileFormat,pLoad);

         if ( !pLoad->Property("MinHaulingSupportLocation",&m_MinHaulPoint) )
            THROW_LOAD(InvalidFileFormat,pLoad);

         if ( !pLoad->Property("HaulingSupportLocationAccuracy",&m_HaulPointAccuracy) )
            THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( 3.2 < version )
      {
         if (!pLoad->Property("CastingYardTensileStressLimitWithMildRebar",&m_CyTensStressServWithRebar) )
             THROW_LOAD(InvalidFileFormat,pLoad);

         if (!pLoad->Property("LiftingTensileStressLimitWithMildRebar",&m_TensStressLiftingWithRebar) )
             THROW_LOAD(InvalidFileFormat,pLoad);

         if (!pLoad->Property("HaulingTensileStressLimitWithMildRebar",&m_TensStressHaulingWithRebar) )
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
         if(!pLoad->Property("BsCompStressServ", &m_Bs3CompStressServ))
            THROW_LOAD(InvalidFileFormat,pLoad);

         if(!pLoad->Property("BsCompStressService1A", &m_Bs3CompStressService1A))
            THROW_LOAD(InvalidFileFormat,pLoad);

         Float64 tmp;
         if(!pLoad->Property("BsCompStressService1B", &tmp))
            THROW_LOAD(InvalidFileFormat,pLoad);

         // stage 1 and 2 are permanent loads only
         m_Bs2CompStress = tmp;
         m_Bs1CompStress = tmp;

         if(!pLoad->Property("BsTensStressServNc", &m_Bs3TensStressServNc))
            THROW_LOAD(InvalidFileFormat,pLoad);

         m_Bs1TensStress = m_Bs3TensStressServNc;

         if(!pLoad->Property("BsDoTensStressServNcMax", &m_Bs3DoTensStressServNcMax))
            THROW_LOAD(InvalidFileFormat,pLoad);

         m_Bs1DoTensStressMax = m_Bs3DoTensStressServNcMax;

         if(!pLoad->Property("BsTensStressServNcMax", &m_Bs3TensStressServNcMax))
            THROW_LOAD(InvalidFileFormat,pLoad);

         m_Bs1TensStressMax = m_Bs3TensStressServNcMax;

         if(!pLoad->Property("BsTensStressServSc",    &m_Bs3TensStressServSc))
            THROW_LOAD(InvalidFileFormat,pLoad);
   
         if(!pLoad->Property("BsDoTensStressServScMax", &m_Bs3DoTensStressServScMax))
            THROW_LOAD(InvalidFileFormat,pLoad);

         if(!pLoad->Property("BsTensStressServScMax", &m_Bs3TensStressServScMax))
            THROW_LOAD(InvalidFileFormat,pLoad);

         if(!pLoad->Property("BsMaxGirdersTrafficBarrier", &m_Bs2MaxGirdersTrafficBarrier))
            THROW_LOAD(InvalidFileFormat,pLoad);

         if(!pLoad->Property("BsMaxGirdersUtility", &m_Bs2MaxGirdersUtility))
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
            if(!pLoad->Property("TempStrandRemovalCompStress" ,     &m_TempStrandRemovalCompStress))
               THROW_LOAD(InvalidFileFormat,pLoad);

            if(!pLoad->Property("TempStrandRemovalTensStress" ,     &m_TempStrandRemovalTensStress))
               THROW_LOAD(InvalidFileFormat,pLoad);

            if(!pLoad->Property("TempStrandRemovalDoTensStressMax",  &m_TempStrandRemovalDoTensStressMax))
               THROW_LOAD(InvalidFileFormat,pLoad);

            if(!pLoad->Property("TempStrandRemovalTensStressMax" ,  &m_TempStrandRemovalTensStressMax))
               THROW_LOAD(InvalidFileFormat,pLoad);

            // for the following 5 items, the m_ was removed from the keyword in version 30
            if(!pLoad->Property("Bs1CompStress" ,     &m_Bs1CompStress))
               THROW_LOAD(InvalidFileFormat,pLoad);

            if(!pLoad->Property("Bs1TensStress" ,     &m_Bs1TensStress))
               THROW_LOAD(InvalidFileFormat,pLoad);

            if(!pLoad->Property("Bs1DoTensStressMax",  &m_Bs1DoTensStressMax))
               THROW_LOAD(InvalidFileFormat,pLoad);

            if(!pLoad->Property("Bs1TensStressMax" ,  &m_Bs1TensStressMax))
               THROW_LOAD(InvalidFileFormat,pLoad);

            if(!pLoad->Property("Bs2CompStress" ,     &m_Bs2CompStress))
               THROW_LOAD(InvalidFileFormat,pLoad);
         }
         else
         {
            if(!pLoad->Property("m_Bs1CompStress" ,     &m_Bs1CompStress))
               THROW_LOAD(InvalidFileFormat,pLoad);

            if(!pLoad->Property("m_Bs1TensStress" ,     &m_Bs1TensStress))
               THROW_LOAD(InvalidFileFormat,pLoad);

            if(!pLoad->Property("m_Bs1DoTensStressMax",  &m_Bs1DoTensStressMax))
               THROW_LOAD(InvalidFileFormat,pLoad);

            if(!pLoad->Property("m_Bs1TensStressMax" ,  &m_Bs1TensStressMax))
               THROW_LOAD(InvalidFileFormat,pLoad);

            if(!pLoad->Property("m_Bs2CompStress" ,     &m_Bs2CompStress))
               THROW_LOAD(InvalidFileFormat,pLoad);

            m_TempStrandRemovalCompStress      = m_Bs1CompStress;
            m_TempStrandRemovalTensStress      = m_Bs1TensStress;
            m_TempStrandRemovalDoTensStressMax = m_Bs1DoTensStressMax;
            m_TempStrandRemovalTensStressMax   = m_Bs1TensStressMax;
         }

         if ( version < 5.0 )
         {
             // this parameters were removed.... just gobble them up if this is an older file
             double dummy;
             if(!pLoad->Property("m_Bs2TensStress" ,     &dummy))
            THROW_LOAD(InvalidFileFormat,pLoad);

             if(!pLoad->Property("m_Bs2DoTensStressMax",  &dummy))
            THROW_LOAD(InvalidFileFormat,pLoad);

             if(!pLoad->Property("m_Bs2TensStressMax" ,  &dummy))
            THROW_LOAD(InvalidFileFormat,pLoad);
         }

         if ( 36 <= version )
         {
            Int16 value;
            if ( !pLoad->Property("Bs2TrafficBarrierDistributionType",&value) )
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }
            m_TrafficBarrierDistributionType = (pgsTypes::TrafficBarrierDistribution)value;
         }

         if(!pLoad->Property("Bs2MaxGirdersTrafficBarrier", &m_Bs2MaxGirdersTrafficBarrier))
            THROW_LOAD(InvalidFileFormat,pLoad);

         if(!pLoad->Property("Bs2MaxGirdersUtility", &m_Bs2MaxGirdersUtility))
            THROW_LOAD(InvalidFileFormat,pLoad);

         if ( version > 33.0 )
         {
            long oldt;
            if(!pLoad->Property("OverlayLoadDistribution", &oldt))
               THROW_LOAD(InvalidFileFormat,pLoad);

            m_OverlayLoadDistribution = (pgsTypes::OverlayLoadDistributionType)oldt==pgsTypes::olDistributeTributaryWidth ? 
                                        pgsTypes::olDistributeTributaryWidth : pgsTypes::olDistributeEvenly;
         }

         if(!pLoad->Property("Bs3CompStressServ", &m_Bs3CompStressServ))
            THROW_LOAD(InvalidFileFormat,pLoad);

         if(!pLoad->Property("Bs3CompStressService1A", &m_Bs3CompStressService1A))
            THROW_LOAD(InvalidFileFormat,pLoad);

         if(!pLoad->Property("Bs3TensStressServNc", &m_Bs3TensStressServNc))
            THROW_LOAD(InvalidFileFormat,pLoad);

         if(!pLoad->Property("Bs3DoTensStressServNcMax", &m_Bs3DoTensStressServNcMax))
            THROW_LOAD(InvalidFileFormat,pLoad);

         if(!pLoad->Property("Bs3TensStressServNcMax", &m_Bs3TensStressServNcMax))
            THROW_LOAD(InvalidFileFormat,pLoad);

         if(!pLoad->Property("Bs3TensStressServSc",    &m_Bs3TensStressServSc))
            THROW_LOAD(InvalidFileFormat,pLoad);

         if(!pLoad->Property("Bs3DoTensStressServScMax", &m_Bs3DoTensStressServScMax))
            THROW_LOAD(InvalidFileFormat,pLoad);

         if(!pLoad->Property("Bs3TensStressServScMax", &m_Bs3TensStressServScMax))
            THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( version >= 1.4 )
      {
         if ( version < 29 )
         {
            // removed in version 29
            if (!pLoad->Property("Bs3IgnoreRangeOfApplicability", &m_Bs3IgnoreRangeOfApplicability))
               THROW_LOAD(InvalidFileFormat,pLoad);
         }

         // this parameter removed in version 18
         if ( version < 18 )
         {
            if (!pLoad->Property("Bs3LRFDShear",&temp))
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

      if ( version >= 1.8 )
      {
         if (!pLoad->Property("Bs3LRFDOverreinforcedMomentCapacity",&temp))
            THROW_LOAD(InvalidFileFormat,pLoad);
         m_Bs3LRFDOverReinforcedMomentCapacity = temp;
      }
   
      if ( version >= 7.0 )
      {
         if (!pLoad->Property("IncludeRebar_MomentCapacity",&temp))
            THROW_LOAD(InvalidFileFormat,pLoad);

         m_bIncludeRebar_Moment = (temp == 0 ? false : true);
      }


      if ( version >= 9.0 )
      {
         if (!pLoad->Property("ModulusOfRuptureCoefficient",&m_FlexureModulusOfRuptureCoefficient))
            THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( 18 <= version )
      {
         // added in version 18
         if ( !pLoad->Property("ShearModulusOfRuptureCoefficient",&m_ShearModulusOfRuptureCoefficient)) 
            THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( !pLoad->Property("BsLldfMethod", &temp ) )
         THROW_LOAD(InvalidFileFormat,pLoad);
      m_LldfMethod = temp;

      if ( 28 < version )
      {
         // added in version 29
         if ( !pLoad->Property("MaxAngularDeviationBetweenGirders",&m_MaxAngularDeviationBetweenGirders) )
            THROW_LOAD(InvalidFileFormat,pLoad);
   
         if ( !pLoad->Property("MinGirderStiffnessRatio",&m_MinGirderStiffnessRatio) )
            THROW_LOAD(InvalidFileFormat,pLoad);

         if ( !pLoad->Property("LLDFGirderSpacingLocation",&m_LLDFGirderSpacingLocation) )
            THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( 30 < version )
      {
         if ( !pLoad->Property("LimitDistributionFactorsToLanesBeams",&m_LimitDistributionFactorsToLanesBeams) )
            THROW_LOAD(InvalidFileFormat,pLoad);
      }

      // added longitudinal reinforcement for shear method to version 1.2 (this was the only change)
      if (version >= 1.2)
      {
         if ( !pLoad->Property("LongReinfShearMethod", &temp ) )
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
         if (!pLoad->Property("IncludeRebar_Shear",&temp))
            THROW_LOAD(InvalidFileFormat,pLoad);

         m_bIncludeRebar_Shear = (temp == 0 ? false : true);
      }

      if ( !pLoad->Property("CreepMethod",&temp) )
         THROW_LOAD(InvalidFileFormat,pLoad);
      m_CreepMethod = temp;

      if (!pLoad->Property("CreepFactor",&m_CreepFactor))
         THROW_LOAD(InvalidFileFormat,pLoad);

      // WSDOT has abandonded it's creep method and use the LRFD calculations
      // 2006
      m_CreepMethod = CREEP_LRFD;

      if ( 3.0 <= version )
      {
         if (!pLoad->Property("CreepDuration1Min",&m_CreepDuration1Min))
            THROW_LOAD(InvalidFileFormat,pLoad);

         if (!pLoad->Property("CreepDuration1Max",&m_CreepDuration1Max))
            THROW_LOAD(InvalidFileFormat,pLoad);

         if (!pLoad->Property("CreepDuration2Min",&m_CreepDuration2Min))
            THROW_LOAD(InvalidFileFormat,pLoad);

         if (!pLoad->Property("CreepDuration2Max",&m_CreepDuration2Max))
            THROW_LOAD(InvalidFileFormat,pLoad);

         if ( !pLoad->Property("XferTime",&m_XferTime))
            THROW_LOAD(InvalidFileFormat,pLoad);

         if ( 3.1 <= version && version < 3.2 )
         {
            if ( !pLoad->Property("NoncompositeCreepDuration",&m_TotalCreepDuration))
               THROW_LOAD(InvalidFileFormat,pLoad);
         }
         if ( 3.2 <= version )
         {
            if ( !pLoad->Property("TotalCreepDuration",&m_TotalCreepDuration))
               THROW_LOAD(InvalidFileFormat,pLoad);
         }
      }
      else if (1.6 <= version && version < 3.0)
      {
         if (!pLoad->Property("CreepDuration1",&m_CreepDuration1Min))
            THROW_LOAD(InvalidFileFormat,pLoad);

         m_CreepDuration1Max = m_CreepDuration1Min;

         if (!pLoad->Property("CreepDuration2",&m_CreepDuration2Min))
            THROW_LOAD(InvalidFileFormat,pLoad);

         m_CreepDuration2Max = m_CreepDuration2Min;

         if ( !pLoad->Property("XferTime",&m_XferTime))
            THROW_LOAD(InvalidFileFormat,pLoad);
      }
      else
      {
         // only one creep duration was entered - need to make some assumptions here
         if (!pLoad->Property("CreepDuration",&m_CreepDuration2Min))
            THROW_LOAD(InvalidFileFormat,pLoad);

         m_CreepDuration2Max = m_CreepDuration2Min;

         if ( !pLoad->Property("XferTime",&m_XferTime))
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
 
      if ( !pLoad->Property("LossMethod",&temp) )
         THROW_LOAD(InvalidFileFormat,pLoad );
      m_LossMethod = temp;

      if ( !pLoad->Property("FinalLosses",&m_FinalLosses) )
         THROW_LOAD(InvalidFileFormat,pLoad );

      if ( !pLoad->Property("ShippingLosses",&m_ShippingLosses) )
         THROW_LOAD(InvalidFileFormat,pLoad );

      if ( !pLoad->Property("BeforeXferLosses",&m_BeforeXferLosses) )
         THROW_LOAD(InvalidFileFormat,pLoad );

      if ( !pLoad->Property("AfterXferLosses",&m_AfterXferLosses) )
         THROW_LOAD(InvalidFileFormat,pLoad );

      if ( 13.0 <= version )
      {
         if ( !pLoad->Property("ShippingTime",&m_ShippingTime) )
            THROW_LOAD(InvalidFileFormat,pLoad );
      }

      if ( 22 <= version )
      {
         if ( !pLoad->Property("LiftingLosses",&m_LiftingLosses) )
            THROW_LOAD(InvalidFileFormat,pLoad);

         if ( !pLoad->Property("BeforeTempStrandRemovalLosses",&m_BeforeTempStrandRemovalLosses) )
            THROW_LOAD(InvalidFileFormat,pLoad);

         if ( !pLoad->Property("AfterTempStrandRemovalLosses",&m_AfterTempStrandRemovalLosses) )
            THROW_LOAD(InvalidFileFormat,pLoad);

         if ( !pLoad->Property("AfterDeckPlacementLosses",&m_AfterDeckPlacementLosses) )
            THROW_LOAD(InvalidFileFormat,pLoad);
      }
      else
      {
         m_LiftingLosses                 = m_AfterXferLosses;
         m_BeforeTempStrandRemovalLosses = m_ShippingLosses < 0 ? m_LiftingLosses : m_ShippingLosses;
         m_AfterTempStrandRemovalLosses  = m_BeforeTempStrandRemovalLosses;
         m_AfterDeckPlacementLosses      = m_FinalLosses;
      }

      if ( 19 <= version )
      {
         if ( !pLoad->Property("CuringMethodFactor",&m_CuringMethodTimeAdjustmentFactor) )
            THROW_LOAD(InvalidFileFormat,pLoad);
      }

      // added in version 1.6
      if ( version >= 1.5 )
      {
         if (!pLoad->Property("CheckStrandStressAtJacking",&m_bCheckStrandStress[AT_JACKING]))
            THROW_LOAD(InvalidFileFormat,pLoad);

         if (!pLoad->Property("Coeff_AtJacking_StressRel",&m_StrandStressCoeff[AT_JACKING][STRESS_REL]))
            THROW_LOAD(InvalidFileFormat,pLoad);

         if (!pLoad->Property("Coeff_AtJacking_LowRelax",&m_StrandStressCoeff[AT_JACKING][LOW_RELAX]))
            THROW_LOAD(InvalidFileFormat,pLoad);


         if (!pLoad->Property("CheckStrandStressBeforeTransfer",&m_bCheckStrandStress[BEFORE_TRANSFER]))
            THROW_LOAD(InvalidFileFormat,pLoad);

         if (!pLoad->Property("Coeff_BeforeTransfer_StressRel",&m_StrandStressCoeff[BEFORE_TRANSFER][STRESS_REL]))
            THROW_LOAD(InvalidFileFormat,pLoad);

         if (!pLoad->Property("Coeff_BeforeTransfer_LowRelax",&m_StrandStressCoeff[BEFORE_TRANSFER][LOW_RELAX]))
            THROW_LOAD(InvalidFileFormat,pLoad);


         if (!pLoad->Property("CheckStrandStressAfterTransfer",&m_bCheckStrandStress[AFTER_TRANSFER]))
            THROW_LOAD(InvalidFileFormat,pLoad);

         if (!pLoad->Property("Coeff_AfterTransfer_StressRel",&m_StrandStressCoeff[AFTER_TRANSFER][STRESS_REL]))
            THROW_LOAD(InvalidFileFormat,pLoad);

         if (!pLoad->Property("Coeff_AfterTransfer_LowRelax",&m_StrandStressCoeff[AFTER_TRANSFER][LOW_RELAX]))
            THROW_LOAD(InvalidFileFormat,pLoad);


         if (!pLoad->Property("CheckStrandStressAfterAllLosses",&m_bCheckStrandStress[AFTER_ALL_LOSSES]))
            THROW_LOAD(InvalidFileFormat,pLoad);

         if (!pLoad->Property("Coeff_AfterAllLosses_StressRel",&m_StrandStressCoeff[AFTER_ALL_LOSSES][STRESS_REL]))
            THROW_LOAD(InvalidFileFormat,pLoad);

         if (!pLoad->Property("Coeff_AfterAllLosses_LowRelax",&m_StrandStressCoeff[AFTER_ALL_LOSSES][LOW_RELAX]))
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
         if ( !pLoad->Property("AnchorSet",&m_Dset) )
            THROW_LOAD(InvalidFileFormat,pLoad);

         if ( !pLoad->Property("WobbleFriction",&m_WobbleFriction) )
            THROW_LOAD(InvalidFileFormat,pLoad);

         if ( !pLoad->Property("CoefficientOfFriction",&m_FrictionCoefficient) )
            THROW_LOAD(InvalidFileFormat,pLoad);
      }

      // added in version 1.7
      if ( 1.6 < version )
      {
         if (!pLoad->Property("CheckLiveLoadDeflection",&m_bDoEvaluateDeflection))
            THROW_LOAD(InvalidFileFormat,pLoad);

         if (!pLoad->Property("LiveLoadDeflectionLimit",&m_DeflectionLimit))
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
         long value;
         if ( !pLoad->Property("AnalysisType",&value) )
            THROW_LOAD(InvalidFileFormat,pLoad);

         m_AnalysisType = (pgsTypes::AnalysisType)(value);
      }

      if ( 10.0 <= version )
      {
         if ( !pLoad->Property("MaxSlabFc",&m_MaxSlabFc) )
            THROW_LOAD(InvalidFileFormat,pLoad);

         if ( !pLoad->Property("MaxGirderFci",&m_MaxGirderFci) )
            THROW_LOAD(InvalidFileFormat,pLoad);

         if ( !pLoad->Property("MaxGirderFc",&m_MaxGirderFc) )
            THROW_LOAD(InvalidFileFormat,pLoad);

         if ( !pLoad->Property("MaxConcreteUnitWeight",&m_MaxConcreteUnitWeight) )
            THROW_LOAD(InvalidFileFormat,pLoad);

         if ( !pLoad->Property("MaxConcreteAggSize",&m_MaxConcreteAggSize) )
            THROW_LOAD(InvalidFileFormat,pLoad);
      }

      // Added in 14.0
      if ( 14.0 <= version )
      {
         std::string strLimitState[] = {"ServiceI","ServiceIA","ServiceIII","StrengthI","StrengthII","FatigueI"};

         pLoad->BeginUnit("LoadFactors");
         Float64 lf_version = pLoad->GetVersion();
         int nLimitStates = 4;
         if ( 2 <= lf_version  )
            nLimitStates += 1; // added Strength II

         if ( 3 <= lf_version )
            nLimitStates += 1; // added Fatigue I

         for ( int i = 0; i < nLimitStates; i++ )
         {
            pLoad->BeginUnit(strLimitState[i].c_str());

            pLoad->Property("DCmin",  &m_DCmin[i]);
            pLoad->Property("DCmax",  &m_DCmax[i]);
            pLoad->Property("DWmin",  &m_DWmin[i]);
            pLoad->Property("DWmax",  &m_DWmax[i]);
            pLoad->Property("LLIMmin",&m_LLIMmin[i]);
            pLoad->Property("LLIMmax",&m_LLIMmax[i]);

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
         if(!pLoad->Property("EnableSlabOffsetCheck", &m_EnableSlabOffsetCheck))
            THROW_LOAD(InvalidFileFormat,pLoad);

         if(!pLoad->Property("EnableSlabOffsetDesign", &m_EnableSlabOffsetDesign))
            THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if (version<15)
      {
         m_DesignStrandFillType = ftMinimizeHarping;
      }
      else
      {
         long ftype;
         if(!pLoad->Property("DesignStrandFillType", &ftype))
            THROW_LOAD(InvalidFileFormat,pLoad);

         m_DesignStrandFillType = ftype==(long)ftGridOrder ? ftGridOrder : ftMinimizeHarping;
      }

      if ( 16 <= version )
      {
         long value;
         if ( !pLoad->Property("EffectiveFlangeWidthMethod",&value) )
            THROW_LOAD(InvalidFileFormat,pLoad);

         m_EffFlangeWidthMethod = (pgsTypes::EffectiveFlangeWidthMethod)(value);
      }

      // only a valid input between data block version 17 and 23
      if ( 17 <= version && version <= 23)
      {
         long value;
         if ( !pLoad->Property("SlabOffsetMethod",&value) )
            THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( 18 <= version )
      {
         long value;
         if ( !pLoad->Property("ShearFlowMethod",&value) )
            THROW_LOAD(InvalidFileFormat,pLoad);

         m_ShearFlowMethod = (ShearFlowMethod)(value);

         if ( !pLoad->Property("ShearCapacityMethod",&value) )
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
      else
      {
         // this is before Version 18... shear capacity method was read above in a now obsolete paremeter
         // translate that parameter here
         m_ShearCapacityMethod = (ShearCapacityMethod)shear_capacity_method;
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
         if ( !pLoad->Property("PedestrianLoad",&m_PedestrianLoad) )
            THROW_LOAD(InvalidFileFormat,pLoad);

         if ( !pLoad->Property("MinSidewalkWidth",&m_MinSidewalkWidth) )
            THROW_LOAD(InvalidFileFormat,pLoad);
      }


      if (32 <= version)
      {
         if ( !pLoad->Property("PrestressTransferComputationType",(long*)&m_PrestressTransferComputationType) )
            THROW_LOAD(InvalidFileFormat,pLoad);
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
   TEST (m_DoCheckAnchorage           , rOther.m_DoCheckAnchorage           );
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
   TESTD(m_HaulingModulusOfRuptureCoefficient , rOther.m_HaulingModulusOfRuptureCoefficient );
   TESTD(m_LiftingModulusOfRuptureCoefficient , rOther.m_LiftingModulusOfRuptureCoefficient );

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
   TESTD(m_FlexureModulusOfRuptureCoefficient , rOther.m_FlexureModulusOfRuptureCoefficient );
   TESTD(m_ShearModulusOfRuptureCoefficient , rOther.m_ShearModulusOfRuptureCoefficient );

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

   TESTD(m_Dset,rOther.m_Dset);
   TESTD(m_WobbleFriction,rOther.m_WobbleFriction);
   TESTD(m_FrictionCoefficient,rOther.m_FrictionCoefficient);

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

   TESTD(m_MaxSlabFc             , rOther.m_MaxSlabFc );
   TESTD(m_MaxGirderFci          , rOther.m_MaxGirderFci );
   TESTD(m_MaxGirderFc           , rOther.m_MaxGirderFc );
   TESTD(m_MaxConcreteUnitWeight , rOther.m_MaxConcreteUnitWeight );
   TESTD(m_MaxConcreteAggSize    , rOther.m_MaxConcreteAggSize );

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

   for ( int i = 0; i < 6; i++ )
   {
      TESTD( m_DCmin[i], rOther.m_DCmin[i] );
      TESTD( m_DWmin[i], rOther.m_DWmin[i] );
      TESTD( m_LLIMmin[i], rOther.m_LLIMmin[i] );
      TESTD( m_DCmax[i], rOther.m_DCmax[i] );
      TESTD( m_DWmax[i], rOther.m_DWmax[i] );
      TESTD( m_LLIMmax[i], rOther.m_LLIMmax[i] );
   }

   TESTD(m_PedestrianLoad,   rOther.m_PedestrianLoad);
   TESTD(m_MinSidewalkWidth, rOther.m_MinSidewalkWidth);

   TESTD(m_MaxAngularDeviationBetweenGirders, rOther.m_MaxAngularDeviationBetweenGirders);
   TESTD(m_MinGirderStiffnessRatio,           rOther.m_MinGirderStiffnessRatio);
   TESTD(m_LLDFGirderSpacingLocation,         rOther.m_LLDFGirderSpacingLocation);

   TEST (m_LimitDistributionFactorsToLanesBeams             , rOther.m_LimitDistributionFactorsToLanesBeams );
   TEST (m_PrestressTransferComputationType, rOther.m_PrestressTransferComputationType);
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

void SpecLibraryEntry::SetDescription(const char* name)
{
   m_Description.erase();
   m_Description = name;
}

std::string SpecLibraryEntry::GetDescription() const
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

void SpecLibraryEntry::EnableAnchorageCheck(bool enable)
{
   m_DoCheckAnchorage = enable;
}

bool SpecLibraryEntry::IsAnchorageCheckEnabled() const
{
   return m_DoCheckAnchorage;
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

void SpecLibraryEntry::SetMaxHaulingOverhang(double oh)
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

void SpecLibraryEntry::SetAxleWeightLimit(double limit)
{
   m_AxleWeightLimit = limit;
}

Float64 SpecLibraryEntry::GetAxleStiffness() const
{
   return m_AxleStiffness;
}

void SpecLibraryEntry::SetAxleStiffness(double stiffness)
{
   m_AxleStiffness = stiffness;
}

Float64 SpecLibraryEntry::GetMinRollStiffness() const
{
   return m_MinRollStiffness;
}

void SpecLibraryEntry::SetMinRollStiffness(double stiffness)
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

void SpecLibraryEntry::SetHaulingModulusOfRuptureCoefficient(double fr)
{
   m_HaulingModulusOfRuptureCoefficient = fr;
}

double SpecLibraryEntry::GetHaulingModulusOfRuptureCoefficient() const
{
   return m_HaulingModulusOfRuptureCoefficient;
}

void SpecLibraryEntry::SetLiftingModulusOfRuptureCoefficient(double fr)
{
   m_LiftingModulusOfRuptureCoefficient = fr;
}

double SpecLibraryEntry::GetLiftingModulusOfRuptureCoefficient() const
{
   return m_LiftingModulusOfRuptureCoefficient;
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

void SpecLibraryEntry::SetStrandStressCoefficient(UINT stage,UINT strandType, double coeff)
{
   m_StrandStressCoeff[stage][strandType] = coeff;
}

double SpecLibraryEntry::GetStrandStressCoefficient(UINT stage,UINT strandType) const
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

void SpecLibraryEntry::SetXferTime(double time)
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

double SpecLibraryEntry::GetFinalLosses() const
{
   return m_FinalLosses;
}

void SpecLibraryEntry::SetFinalLosses(double loss)
{
   m_FinalLosses = loss;
}

double SpecLibraryEntry::GetShippingLosses() const
{
   return m_ShippingLosses;
}

void SpecLibraryEntry::SetShippingLosses(double loss)
{
   m_ShippingLosses = loss;
}

double SpecLibraryEntry::GetLiftingLosses() const
{
   return m_LiftingLosses;
}

void SpecLibraryEntry::SetLiftingLosses(double loss)
{
   m_LiftingLosses = loss;
}

double SpecLibraryEntry::GetBeforeXferLosses() const
{
   return m_BeforeXferLosses;
}

void SpecLibraryEntry::SetBeforeXferLosses(double loss)
{
   m_BeforeXferLosses = loss;
}

double SpecLibraryEntry::GetAfterXferLosses() const
{
   return m_AfterXferLosses;
}

void SpecLibraryEntry::SetAfterXferLosses(double loss)
{
   m_AfterXferLosses = loss;
}

void SpecLibraryEntry::SetShippingTime(double time)
{
   m_ShippingTime = time;
}

double SpecLibraryEntry::GetShippingTime() const
{
   return m_ShippingTime;
}

double SpecLibraryEntry::GetBeforeTempStrandRemovalLosses() const
{
   return m_BeforeTempStrandRemovalLosses;
}

void SpecLibraryEntry::SetBeforeTempStrandRemovalLosses(double loss)
{
   m_BeforeTempStrandRemovalLosses = loss;
}

double SpecLibraryEntry::GetAfterTempStrandRemovalLosses() const
{
   return m_AfterTempStrandRemovalLosses;
}

void SpecLibraryEntry::SetAfterTempStrandRemovalLosses(double loss)
{
   m_AfterTempStrandRemovalLosses = loss;
}

double SpecLibraryEntry::GetAfterDeckPlacementLosses() const
{
   return m_AfterDeckPlacementLosses;
}

void SpecLibraryEntry::SetAfterDeckPlacementLosses(double loss)
{
   m_AfterDeckPlacementLosses = loss;
}

double SpecLibraryEntry::GetAnchorSet() const
{
   return m_Dset;
}

void SpecLibraryEntry::SetAnchorSet(double dset)
{
   m_Dset = dset;
}

double SpecLibraryEntry::GetWobbleFrictionCoefficient() const
{
   return m_WobbleFriction;
}

void SpecLibraryEntry::SetWobbleFrictionCoefficient(double K)
{
   m_WobbleFriction = K;
}

double SpecLibraryEntry::GetFrictionCoefficient() const
{
   return m_FrictionCoefficient;
}

void SpecLibraryEntry::SetFrictionCoefficient(double u)
{
   m_FrictionCoefficient = u;
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

double SpecLibraryEntry::GetLLDeflectionLimit() const
{
   return m_DeflectionLimit;
}

void SpecLibraryEntry::SetLLDeflectionLimit(double limit)
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

void SpecLibraryEntry::SetFlexureModulusOfRuptureCoefficient(double fr)
{
   m_FlexureModulusOfRuptureCoefficient = fr;
}

double SpecLibraryEntry::GetFlexureModulusOfRuptureCoefficient() const
{
   return m_FlexureModulusOfRuptureCoefficient;
}

void SpecLibraryEntry::SetShearModulusOfRuptureCoefficient(double fr)
{
   m_ShearModulusOfRuptureCoefficient = fr;
}

double SpecLibraryEntry::GetShearModulusOfRuptureCoefficient() const
{
   return m_ShearModulusOfRuptureCoefficient;
}

void SpecLibraryEntry::SetMaxSlabFc(double fc)
{
   m_MaxSlabFc = fc;
}

double SpecLibraryEntry::GetMaxSlabFc() const
{
   return m_MaxSlabFc;
}

void SpecLibraryEntry::SetMaxGirderFc(double fc)
{
   m_MaxGirderFc = fc;
}

double SpecLibraryEntry::GetMaxGirderFc() const
{
   return m_MaxGirderFc;
}

void SpecLibraryEntry::SetMaxGirderFci(double fci)
{
   m_MaxGirderFci = fci;
}

double SpecLibraryEntry::GetMaxGirderFci() const
{
   return m_MaxGirderFci;
}

void SpecLibraryEntry::SetMaxConcreteUnitWeight(double wc)
{
   m_MaxConcreteUnitWeight = wc;
}

double SpecLibraryEntry::GetMaxConcreteUnitWeight() const
{
   return m_MaxConcreteUnitWeight;
}

void SpecLibraryEntry::SetMaxConcreteAggSize(double agg)
{
   m_MaxConcreteAggSize = agg;
}

double SpecLibraryEntry::GetMaxConcreteAggSize() const
{
   return m_MaxConcreteAggSize;
}

void SpecLibraryEntry::GetDCLoadFactors(pgsTypes::LimitState ls,double* pDCmin,double* pDCmax) const
{
   *pDCmin = m_DCmin[ls];
   *pDCmax = m_DCmax[ls];
}

void SpecLibraryEntry::GetDWLoadFactors(pgsTypes::LimitState ls,double* pDWmin,double* pDWmax) const
{
   *pDWmin = m_DWmin[ls];
   *pDWmax = m_DWmax[ls];
}

void SpecLibraryEntry::GetLLIMLoadFactors(pgsTypes::LimitState ls,double* pLLIMmin,double* pLLIMmax) const
{
   *pLLIMmin = m_LLIMmin[ls];
   *pLLIMmax = m_LLIMmax[ls];
}

void SpecLibraryEntry::SetDCLoadFactors(pgsTypes::LimitState ls,double DCmin,double DCmax)
{
   m_DCmin[ls] = DCmin;
   m_DCmax[ls] = DCmax;
}

void SpecLibraryEntry::SetDWLoadFactors(pgsTypes::LimitState ls,double DWmin,double DWmax)
{
   m_DWmin[ls] = DWmin;
   m_DWmax[ls] = DWmax;
}

void SpecLibraryEntry::SetLLIMLoadFactors(pgsTypes::LimitState ls,double LLIMmin,double LLIMmax)
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

void SpecLibraryEntry::SetCuringMethodTimeAdjustmentFactor(double f)
{
   m_CuringMethodTimeAdjustmentFactor = f;
}

double SpecLibraryEntry::GetCuringMethodTimeAdjustmentFactor() const
{
   return m_CuringMethodTimeAdjustmentFactor;
}

void SpecLibraryEntry::SetMininumTruckSupportLocation(double x)
{
   m_MinHaulPoint = x;
}

double SpecLibraryEntry::GetMininumTruckSupportLocation() const
{
   return m_MinHaulPoint;
}

void SpecLibraryEntry::SetTruckSupportLocationAccuracy(double x)
{
   m_HaulPointAccuracy = x;
}

double SpecLibraryEntry::GetTruckSupportLocationAccuracy() const
{
   return m_HaulPointAccuracy;
}

void SpecLibraryEntry::SetMininumLiftingPointLocation(double x)
{
   m_MinLiftPoint = x;
}

double SpecLibraryEntry::GetMininumLiftingPointLocation() const
{
   return m_MinLiftPoint;
}

void SpecLibraryEntry::SetLiftingPointLocationAccuracy(double x)
{
   m_LiftPointAccuracy = x;
}

double SpecLibraryEntry::GetLiftingPointLocationAccuracy() const
{
   return m_LiftPointAccuracy;
}

void SpecLibraryEntry::SetPedestrianLiveLoad(double w)
{
   m_PedestrianLoad = w;
}

double SpecLibraryEntry::GetPedestrianLiveLoad() const
{
   return m_PedestrianLoad;
}

void SpecLibraryEntry::SetMinSidewalkWidth(double Wmin)
{
   m_MinSidewalkWidth = Wmin;
}

double SpecLibraryEntry::GetMinSidewalkWidth() const
{
   return m_MinSidewalkWidth;
}

void SpecLibraryEntry::SetMaxAngularDeviationBetweenGirders(double angle)
{
   m_MaxAngularDeviationBetweenGirders = angle;
}

double SpecLibraryEntry::GetMaxAngularDeviationBetweenGirders() const
{
   return m_MaxAngularDeviationBetweenGirders;
}

void SpecLibraryEntry::SetMinGirderStiffnessRatio(double r)
{
   m_MinGirderStiffnessRatio = r;
}

double SpecLibraryEntry::GetMinGirderStiffnessRatio() const
{
   return m_MinGirderStiffnessRatio;
}

void SpecLibraryEntry::SetLLDFGirderSpacingLocation(double fra)
{
   m_LLDFGirderSpacingLocation = fra;
}

double SpecLibraryEntry::GetLLDFGirderSpacingLocation() const
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
   m_DoCheckAnchorage           = rOther.m_DoCheckAnchorage;
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
   m_HaulingModulusOfRuptureCoefficient = rOther.m_HaulingModulusOfRuptureCoefficient;
   m_LiftingModulusOfRuptureCoefficient = rOther.m_LiftingModulusOfRuptureCoefficient;

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
   m_Dset = rOther.m_Dset;
   m_WobbleFriction = rOther.m_WobbleFriction;
   m_FrictionCoefficient = rOther.m_FrictionCoefficient;

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

   m_MaxSlabFc             = rOther.m_MaxSlabFc;
   m_MaxGirderFci          = rOther.m_MaxGirderFci;
   m_MaxGirderFc           = rOther.m_MaxGirderFc;
   m_MaxConcreteUnitWeight = rOther.m_MaxConcreteUnitWeight;
   m_MaxConcreteAggSize    = rOther.m_MaxConcreteAggSize;

   for ( int i = 0; i < 6; i++ )
   {
      m_DCmin[i]   = rOther.m_DCmin[i];
      m_DWmin[i]   = rOther.m_DWmin[i];
      m_LLIMmin[i] = rOther.m_LLIMmin[i];
      m_DCmax[i]   = rOther.m_DCmax[i];
      m_DWmax[i]   = rOther.m_DWmax[i];
      m_LLIMmax[i] = rOther.m_LLIMmax[i];
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
