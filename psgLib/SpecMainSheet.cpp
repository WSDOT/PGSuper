///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2025  Washington State Department of Transportation
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

// SpecMainSheet.cpp : implementation file
//

#include "stdafx.h"
#include <psgLib\psglib.h>
#include "SpecMainSheet.h"
#include "SpecLibraryEntryImpl.h"
#include <MfcTools\CustomDDX.h>

#include <Units\Convert.h>
#include <EAF\EAFApp.h>

#include <IFace\DocumentType.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSpecMainSheet

IMPLEMENT_DYNAMIC(CSpecMainSheet, CPropertySheet)

CSpecMainSheet::CSpecMainSheet( SpecLibraryEntry& rentry, UINT nIDCaption, 
                                   bool allowEditing,
                                   CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(nIDCaption, pParentWnd, iSelectPage),
   m_Entry(rentry),
   m_bAllowEditing(allowEditing)
{
   Init();
}

CSpecMainSheet::CSpecMainSheet( SpecLibraryEntry& rentry, LPCTSTR pszCaption,
                                   bool allowEditing,
                                   CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(pszCaption, pParentWnd, iSelectPage),
   m_Entry(rentry),
   m_bAllowEditing(allowEditing)
{
   Init();
}

CSpecMainSheet::~CSpecMainSheet()
{
}


BEGIN_MESSAGE_MAP(CSpecMainSheet, CPropertySheet)
	//{{AFX_MSG_MAP(CSpecMainSheet)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSpecMainSheet message handlers
void CSpecMainSheet::Init()
{
   // Turn on help for the property sheet
   m_psh.dwFlags |= PSH_HASHELP | PSH_NOAPPLYNOW;

   m_SpecDescrPage.m_psp.dwFlags           |= PSP_HASHELP;
   m_SpecLiftingPage.m_psp.dwFlags         |= PSP_HASHELP;
   m_SpecHaulingErectionPage.m_psp.dwFlags |= PSP_HASHELP;
   m_SpecMomentPage.m_psp.dwFlags          |= PSP_HASHELP;
   m_SpecShearPage.m_psp.dwFlags           |= PSP_HASHELP;
   m_SpecCreepPage.m_psp.dwFlags           |= PSP_HASHELP;
   m_SpecLossPage.m_psp.dwFlags            |= PSP_HASHELP;
   m_SpecStrandStressPage.m_psp.dwFlags    |= PSP_HASHELP;
   m_SpecLimitsPage.m_psp.dwFlags          |= PSP_HASHELP;
   m_SpecDesignPage.m_psp.dwFlags          |= PSP_HASHELP;
   m_SpecClosurePage.m_psp.dwFlags         |= PSP_HASHELP;

   m_SpecDeadLoadsPage.m_psp.dwFlags |= PSP_HASHELP;
   m_SpecLiveLoadsPage.m_psp.dwFlags |= PSP_HASHELP;
   m_SpecGirderStressPage.m_psp.dwFlags    |= PSP_HASHELP;

   m_SpecBearingsPage.m_psp.dwFlags |= PSP_HASHELP;

   AddPage(&m_SpecDescrPage);
   AddPage(&m_SpecDesignPage);
   AddPage(&m_SpecGirderStressPage);
   AddPage(&m_SpecClosurePage);
   AddPage(&m_SpecStrandStressPage);
   AddPage(&m_SpecLiftingPage);
   AddPage(&m_SpecHaulingErectionPage);
   AddPage(&m_SpecDeadLoadsPage);
   AddPage(&m_SpecLiveLoadsPage);
   AddPage(&m_SpecMomentPage);
   AddPage(&m_SpecShearPage);
   AddPage(&m_SpecCreepPage);
   AddPage(&m_SpecLossPage);
   AddPage(&m_SpecLimitsPage);
   AddPage(&m_SpecBearingsPage);
}

void CSpecMainSheet::ExchangeDescriptionData(CDataExchange* pDX)
{
   // specification type
   DDX_CBItemData(pDX,IDC_SPECIFICATION,m_Entry.m_pImpl->m_SpecificationCriteria.Edition);
   DDX_Check_Bool(pDX, IDC_USE_CURRENT_VERSION, m_Entry.m_pImpl->m_SpecificationCriteria.bUseCurrentSpecification);

   // Section Properties
   DDX_RadioEnum(pDX,IDC_GROSS,m_Entry.m_pImpl->m_SectionPropertiesCriteria.SectionPropertyMode);
   DDX_CBEnum(pDX,IDC_EFF_FLANGE_WIDTH,m_Entry.m_pImpl->m_SectionPropertiesCriteria.EffectiveFlangeWidthMethod);


   if (pDX->m_bSaveAndValidate)
   {
      DDX_Text(pDX, IDC_NAME, m_Name);
      if (m_Name.IsEmpty())
      {
         AfxMessageBox(_T("Name cannot be blank"));
         pDX->Fail();
      }
      m_Entry.SetName(m_Name);

	  DDX_Text(pDX, IDC_EDIT_DESCRIPTION, m_Description);
      m_Entry.m_pImpl->m_SpecificationCriteria.Description = m_Description;

      DDX_Text(pDX, IDC_THERMAL, m_Thermal);
      m_Entry.m_pImpl->m_ThermalMovementCriteria.ThermalMovementFactor = m_Thermal;

      // specification units
      int chk = m_SpecDescrPage.GetCheckedRadioButton( IDC_SPEC_UNITS_SI,IDC_SPEC_UNITS_US);
      if (chk==IDC_SPEC_UNITS_SI)
         m_Entry.m_pImpl->m_SpecificationCriteria.Units = WBFL::LRFD::BDSManager::Units::SI;
      else if (chk==IDC_SPEC_UNITS_US)
         m_Entry.m_pImpl->m_SpecificationCriteria.Units = WBFL::LRFD::BDSManager::Units::US;
      else
         ASSERT(false); // should never get here
   }
   else
   {
      // name
      m_Name = m_Entry.GetName().c_str();
	   DDX_Text(pDX, IDC_NAME, m_Name);

      m_Description = m_Entry.m_pImpl->m_SpecificationCriteria.Description.c_str();
	   DDX_Text(pDX, IDC_EDIT_DESCRIPTION, m_Description);

      m_Thermal = m_Entry.m_pImpl->m_ThermalMovementCriteria.ThermalMovementFactor;
       DDX_Text(pDX, IDC_THERMAL, m_Thermal);

      // spec units
      WBFL::LRFD::BDSManager::Units Units = m_Entry.m_pImpl->m_SpecificationCriteria.Units;
      int unit;
      if (Units == WBFL::LRFD::BDSManager::Units::SI)
         unit = IDC_SPEC_UNITS_SI;
      else if (Units == WBFL::LRFD::BDSManager::Units::US)
         unit = IDC_SPEC_UNITS_US;
      else
         ASSERT(0);

      m_SpecDescrPage.CheckRadioButton( IDC_SPEC_UNITS_SI, IDC_SPEC_UNITS_US, unit);
   }

   if ( pDX->m_bSaveAndValidate )
   {
      CheckShearCapacityMethod();
   }
}

void CSpecMainSheet::ExchangeDeadLoadsData(CDataExchange* pDX)
{
   CEAFApp* pApp = EAFGetApp();
   const WBFL::Units::IndirectMeasure* pDisplayUnits = pApp->GetDisplayUnits();

   // railing system distribution
   DDX_CBEnum(pDX,IDC_DIST_TRAFFIC_BARRIER_BASIS,m_Entry.m_pImpl->m_DeadLoadDistributionCriteria.TrafficBarrierDistribution);
	DDX_Text(pDX, IDC_DIST_TRAFFIC_BARRIER, m_Entry.m_pImpl->m_DeadLoadDistributionCriteria.MaxGirdersTrafficBarrier);

   // overlay load distribution
   DDX_CBEnum(pDX, IDC_OVERLAY_DISTR,m_Entry.m_pImpl->m_DeadLoadDistributionCriteria.OverlayDistribution);
}

void CSpecMainSheet::ExchangeLiveLoadsData(CDataExchange* pDX)
{
   CEAFApp* pApp = EAFGetApp();
   const WBFL::Units::IndirectMeasure* pDisplayUnits = pApp->GetDisplayUnits();

   DDX_Check_Bool(pDX, IDC_DUAL_TANDEM, m_Entry.m_pImpl->m_LiveLoadCriteria.bIncludeDualTandem);

   // pedestrian live loads
   DDX_UnitValueAndTag(pDX, IDC_PED_LIVE_LOAD, IDC_PED_LIVE_LOAD_UNIT, m_Entry.m_pImpl->m_LiveLoadCriteria.PedestrianLoad, pDisplayUnits->SmallStress);
   DDV_UnitValueZeroOrMore(pDX, IDC_PED_LIVE_LOAD, m_Entry.m_pImpl->m_LiveLoadCriteria.PedestrianLoad, pDisplayUnits->SmallStress);
   DDX_UnitValueAndTag(pDX, IDC_MIN_SIDEWALK_WIDTH, IDC_MIN_SIDEWALK_WIDTH_UNIT, m_Entry.m_pImpl->m_LiveLoadCriteria.MinSidewalkWidth, pDisplayUnits->SpanLength);
   DDV_UnitValueZeroOrMore(pDX, IDC_MIN_SIDEWALK_WIDTH, m_Entry.m_pImpl->m_LiveLoadCriteria.MinSidewalkWidth, pDisplayUnits->SpanLength);

   // live load distribution
   DDX_CBEnum(pDX, IDC_LLDF, m_Entry.m_pImpl->m_LiveLoadDistributionCriteria.LldfMethod);
   DDX_Check_Bool(pDX, IDC_IGNORE_SKEW_REDUCTION, m_Entry.m_pImpl->m_LiveLoadDistributionCriteria.bIgnoreSkewReductionForMoment);

   DDX_UnitValueAndTag(pDX, IDC_MAXGIRDERANGLE, IDC_MAXGIRDERANGLE_UNIT, m_Entry.m_pImpl->m_LiveLoadDistributionCriteria.MaxAngularDeviationBetweenGirders, pDisplayUnits->Angle);
   DDX_Text(pDX, IDC_GIRDERSTIFFNESSRATIO, m_Entry.m_pImpl->m_LiveLoadDistributionCriteria.MinGirderStiffnessRatio);
   DDV_MinMaxDouble(pDX, m_Entry.m_pImpl->m_LiveLoadDistributionCriteria.MinGirderStiffnessRatio, 0.0, 1.0);

   DDX_Text(pDX, IDC_GIRDER_SPACING_LOCATION, m_Entry.m_pImpl->m_LiveLoadDistributionCriteria.GirderSpacingLocation);
   DDV_MinMaxDouble(pDX, m_Entry.m_pImpl->m_LiveLoadDistributionCriteria.GirderSpacingLocation, 0.0, 1.0);

   DDX_Check_Bool(pDX, IDC_LANESBEAMS, m_Entry.m_pImpl->m_LiveLoadDistributionCriteria.bLimitDistributionFactorsToLanesBeams);
   DDX_Check_Bool(pDX, IDC_EXT_ADJACENT_LLDF_RULE, m_Entry.m_pImpl->m_LiveLoadDistributionCriteria.bExteriorBeamLiveLoadDistributionGTInteriorBeam);

   DDX_Check_Bool(pDX, IDC_RIGID_METHOD, m_Entry.m_pImpl->m_LiveLoadDistributionCriteria.bUseRigidMethod);
}

void CSpecMainSheet::ExchangeGirderData(CDataExchange* pDX)
{
   CEAFApp* pApp = EAFGetApp();
   const WBFL::Units::IndirectMeasure* pDisplayUnits = pApp->GetDisplayUnits();

   CString fciTag = (pApp->GetUnitsMode() == eafTypes::umSI ? _T("sqrt(f'ci (MPa))") : _T("sqrt(f'ci (KSI))"));
   CString fcTag  = (pApp->GetUnitsMode() == eafTypes::umSI ? _T("sqrt(f'c (MPa))")  : _T("sqrt(f'c (KSI))"));

   if ( WBFL::LRFD::BDSManager::Edition::SeventhEditionWith2016Interims <= WBFL::LRFD::BDSManager::GetEdition() )
   {
      fciTag = _T("(lambda)") + fciTag;
      fcTag  = _T("(lambda)") + fcTag;
   }

   // Allowable concrete stress at prestress release
	DDX_Text(pDX, IDC_RELEASE_COMPRESSION, m_Entry.m_pImpl->m_PrestressedElementCriteria.CompressionStressCoefficient_BeforeLosses);
   DDV_GreaterThanZero(pDX, IDC_RELEASE_COMPRESSION, m_Entry.m_pImpl->m_PrestressedElementCriteria.CompressionStressCoefficient_BeforeLosses);

   DDX_UnitValueAndTag(pDX, IDC_RELEASE_TENSION, IDC_RELEASE_TENSION_UNIT, m_Entry.m_pImpl->m_PrestressedElementCriteria.TensionStressLimit_OtherAreas_WithoutReinforcement_BeforeLosses.Coefficient, pDisplayUnits->SqrtPressure );
   DDX_Text(pDX,IDC_RELEASE_TENSION_UNIT,fciTag);
   DDV_UnitValueZeroOrMore(pDX, IDC_RELEASE_TENSION_UNIT,m_Entry.m_pImpl->m_PrestressedElementCriteria.TensionStressLimit_OtherAreas_WithoutReinforcement_BeforeLosses.Coefficient, pDisplayUnits->SqrtPressure);
   DDX_Check_Bool(pDX, IDC_CHECK_RELEASE_TENSION_MAX, m_Entry.m_pImpl->m_PrestressedElementCriteria.TensionStressLimit_OtherAreas_WithoutReinforcement_BeforeLosses.bHasMaxValue);
   DDX_UnitValueAndTag(pDX, IDC_RELEASE_TENSION_MAX, IDC_RELEASE_TENSION_MAX_UNIT, m_Entry.m_pImpl->m_PrestressedElementCriteria.TensionStressLimit_OtherAreas_WithoutReinforcement_BeforeLosses.MaxValue, pDisplayUnits->Stress );
   if (m_Entry.m_pImpl->m_PrestressedElementCriteria.TensionStressLimit_OtherAreas_WithoutReinforcement_BeforeLosses.bHasMaxValue)
   {
      DDV_UnitValueGreaterThanZero(pDX, IDC_RELEASE_TENSION_MAX,m_Entry.m_pImpl->m_PrestressedElementCriteria.TensionStressLimit_OtherAreas_WithoutReinforcement_BeforeLosses.MaxValue, pDisplayUnits->Stress );
   }

   DDX_UnitValueAndTag(pDX, IDC_RELEASE_TENSION_WITH_REBAR, IDC_SERVICE_III_TENSION_UNIT, m_Entry.m_pImpl->m_PrestressedElementCriteria.TensionStressLimit_WithReinforcement_BeforeLosses.Coefficient, pDisplayUnits->SqrtPressure);
   DDX_Text(pDX,IDC_RELEASE_TENSION_WITH_REBAR_UNIT,fciTag);
   DDV_UnitValueZeroOrMore(pDX, IDC_RELEASE_TENSION_WITH_REBAR,m_Entry.m_pImpl->m_PrestressedElementCriteria.TensionStressLimit_WithReinforcement_BeforeLosses.Coefficient, pDisplayUnits->SqrtPressure );

   if (pDX->m_bSaveAndValidate && m_Entry.m_pImpl->m_PrestressedElementCriteria.TensionStressLimit_WithReinforcement_BeforeLosses.Coefficient < m_Entry.m_pImpl->m_PrestressedElementCriteria.TensionStressLimit_OtherAreas_WithoutReinforcement_BeforeLosses.Coefficient)
   {
      AfxMessageBox(_T("Stress limits for Temporary Stresses before Losses (LRFD 5.9.4.1): Tensile stress limit with bonded reinforcement must be greater than or equal to than without"),MB_OK | MB_ICONWARNING);
      pDX->Fail();
   }

   // Allowable concrete stress at service limit states
	DDX_Text(pDX, IDC_SERVICE_COMPRESSION, m_Entry.m_pImpl->m_PrestressedElementCriteria.CompressionStressCoefficient_PermanentLoadsOnly_AfterLosses);
   DDV_GreaterThanZero(pDX, IDC_SERVICE_COMPRESSION, m_Entry.m_pImpl->m_PrestressedElementCriteria.CompressionStressCoefficient_PermanentLoadsOnly_AfterLosses);

	DDX_Text(pDX, IDC_SERVICE_COMPRESSION_WITH_LIVELOAD, m_Entry.m_pImpl->m_PrestressedElementCriteria.CompressionStressCoefficient_AllLoads_AfterLosses);
   DDV_GreaterThanZero(pDX, IDC_SERVICE_COMPRESSION_WITH_LIVELOAD, m_Entry.m_pImpl->m_PrestressedElementCriteria.CompressionStressCoefficient_AllLoads_AfterLosses);


   DDX_Check_Bool(pDX, IDC_CHECK_SERVICE_I_TENSION, m_Entry.m_pImpl->m_PrestressedElementCriteria.bCheckFinalServiceITension);
   DDX_UnitValueAndTag(pDX, IDC_SERVICE_I_TENSION, IDC_SERVICE_I_TENSION_UNIT, m_Entry.m_pImpl->m_PrestressedElementCriteria.TensionStressLimit_ServiceI_PermanentLoadsOnly_AfterLosses.Coefficient, pDisplayUnits->SqrtPressure );
   DDX_Text(pDX,IDC_SERVICE_I_TENSION_UNIT,fcTag);
   DDV_UnitValueZeroOrMore(pDX, IDC_SERVICE_I_TENSION,m_Entry.m_pImpl->m_PrestressedElementCriteria.TensionStressLimit_ServiceI_PermanentLoadsOnly_AfterLosses.Coefficient, pDisplayUnits->SqrtPressure );
   DDX_Check_Bool(pDX, IDC_CHECK_SERVICE_I_TENSION_MAX, m_Entry.m_pImpl->m_PrestressedElementCriteria.TensionStressLimit_ServiceI_PermanentLoadsOnly_AfterLosses.bHasMaxValue);
   DDX_UnitValueAndTag(pDX, IDC_SERVICE_I_TENSION_MAX, IDC_SERVICE_I_TENSION_MAX_UNIT, m_Entry.m_pImpl->m_PrestressedElementCriteria.TensionStressLimit_ServiceI_PermanentLoadsOnly_AfterLosses.MaxValue, pDisplayUnits->Stress );
   if (m_Entry.m_pImpl->m_PrestressedElementCriteria.TensionStressLimit_ServiceI_PermanentLoadsOnly_AfterLosses.bHasMaxValue)
   {
      DDV_UnitValueGreaterThanZero(pDX, IDC_SERVICE_I_TENSION_MAX,m_Entry.m_pImpl->m_PrestressedElementCriteria.TensionStressLimit_ServiceI_PermanentLoadsOnly_AfterLosses.MaxValue, pDisplayUnits->Stress );
   }


   DDX_UnitValueAndTag(pDX, IDC_SERVICE_III_TENSION, IDC_SERVICE_III_TENSION_UNIT, m_Entry.m_pImpl->m_PrestressedElementCriteria.TensionStressLimit_ServiceIII_InPTZ_ModerateCorrosionConditions_AfterLosses.Coefficient, pDisplayUnits->SqrtPressure );
   DDX_Text(pDX,IDC_SERVICE_III_TENSION_UNIT,fcTag);
   DDV_UnitValueZeroOrMore(pDX, IDC_SERVICE_III_TENSION,m_Entry.m_pImpl->m_PrestressedElementCriteria.TensionStressLimit_ServiceIII_InPTZ_ModerateCorrosionConditions_AfterLosses.Coefficient, pDisplayUnits->SqrtPressure );
   DDX_Check_Bool(pDX, IDC_CHECK_SERVICE_III_TENSION_MAX, m_Entry.m_pImpl->m_PrestressedElementCriteria.TensionStressLimit_ServiceIII_InPTZ_ModerateCorrosionConditions_AfterLosses.bHasMaxValue);
   DDX_UnitValueAndTag(pDX, IDC_SERVICE_III_TENSION_MAX, IDC_SERVICE_III_TENSION_MAX_UNIT, m_Entry.m_pImpl->m_PrestressedElementCriteria.TensionStressLimit_ServiceIII_InPTZ_ModerateCorrosionConditions_AfterLosses.MaxValue, pDisplayUnits->Stress );
   if (m_Entry.m_pImpl->m_PrestressedElementCriteria.TensionStressLimit_ServiceIII_InPTZ_ModerateCorrosionConditions_AfterLosses.bHasMaxValue)
   {
      DDV_UnitValueGreaterThanZero(pDX, IDC_SERVICE_III_TENSION_MAX,m_Entry.m_pImpl->m_PrestressedElementCriteria.TensionStressLimit_ServiceIII_InPTZ_ModerateCorrosionConditions_AfterLosses.MaxValue, pDisplayUnits->Stress );
   }

   DDX_UnitValueAndTag(pDX, IDC_SEVERE_SERVICE_III_TENSION, IDC_SEVERE_SERVICE_III_TENSION_UNIT, m_Entry.m_pImpl->m_PrestressedElementCriteria.TensionStressLimit_ServiceIII_InPTZ_SevereCorrosionConditions_AfterLosses.Coefficient, pDisplayUnits->SqrtPressure );
   DDX_Text(pDX,IDC_SEVERE_SERVICE_III_TENSION_UNIT,fcTag);
   DDV_UnitValueZeroOrMore(pDX, IDC_SEVERE_SERVICE_III_TENSION,m_Entry.m_pImpl->m_PrestressedElementCriteria.TensionStressLimit_ServiceIII_InPTZ_SevereCorrosionConditions_AfterLosses.Coefficient, pDisplayUnits->SqrtPressure );
   DDX_Check_Bool(pDX, IDC_CHECK_SEVERE_SERVICE_III_TENSION_MAX, m_Entry.m_pImpl->m_PrestressedElementCriteria.TensionStressLimit_ServiceIII_InPTZ_SevereCorrosionConditions_AfterLosses.bHasMaxValue);
   DDX_UnitValueAndTag(pDX, IDC_SEVERE_SERVICE_III_TENSION_MAX, IDC_SEVERE_SERVICE_III_TENSION_MAX_UNIT, m_Entry.m_pImpl->m_PrestressedElementCriteria.TensionStressLimit_ServiceIII_InPTZ_SevereCorrosionConditions_AfterLosses.MaxValue, pDisplayUnits->Stress );
   if (m_Entry.m_pImpl->m_PrestressedElementCriteria.TensionStressLimit_ServiceIII_InPTZ_SevereCorrosionConditions_AfterLosses.bHasMaxValue)
   {
      DDV_UnitValueGreaterThanZero(pDX, IDC_SEVERE_SERVICE_III_TENSION_MAX,m_Entry.m_pImpl->m_PrestressedElementCriteria.TensionStressLimit_ServiceIII_InPTZ_SevereCorrosionConditions_AfterLosses.MaxValue, pDisplayUnits->Stress );
   }

   // Allowable concrete stress at fatigue limit state
   DDX_Text(pDX, IDC_FATIGUE_COMPRESSION, m_Entry.m_pImpl->m_PrestressedElementCriteria.CompressionStressCoefficient_Fatigue);
   DDV_GreaterThanZero(pDX, IDC_FATIGUE_COMPRESSION, m_Entry.m_pImpl->m_PrestressedElementCriteria.CompressionStressCoefficient_Fatigue);

   // Temporary Loading Condition (PGSuper only)
   DDX_Check_Bool(pDX,IDC_CHECK_TEMPORARY_STRESSES,m_Entry.m_pImpl->m_PrestressedElementCriteria.bCheckTemporaryStresses);

   // Allowable concrete stress after Temporary Strand Removal
	DDX_Text(pDX, IDC_TS_REMOVAL_COMPRESSION, m_Entry.m_pImpl->m_PrestressedElementCriteria.CompressionStressCoefficient_TemporaryStrandRemoval);
   DDV_GreaterThanZero(pDX, IDC_TS_REMOVAL_COMPRESSION, m_Entry.m_pImpl->m_PrestressedElementCriteria.CompressionStressCoefficient_TemporaryStrandRemoval);

   DDX_UnitValueAndTag(pDX, IDC_TS_REMOVAL_TENSION, IDC_TS_REMOVAL_TENSION_UNIT, m_Entry.m_pImpl->m_PrestressedElementCriteria.TensionStressLimit_WithoutReinforcement_TemporaryStrandRemoval.Coefficient, pDisplayUnits->SqrtPressure );
   DDX_Text(pDX,IDC_TS_REMOVAL_TENSION_UNIT,fcTag);
   DDV_UnitValueZeroOrMore(pDX,IDC_TS_REMOVAL_TENSION, m_Entry.m_pImpl->m_PrestressedElementCriteria.TensionStressLimit_WithoutReinforcement_TemporaryStrandRemoval.Coefficient, pDisplayUnits->SqrtPressure );
   DDX_Check_Bool(pDX, IDC_CHECK_TS_REMOVAL_TENSION_MAX, m_Entry.m_pImpl->m_PrestressedElementCriteria.TensionStressLimit_WithoutReinforcement_TemporaryStrandRemoval.bHasMaxValue);
   DDX_UnitValueAndTag(pDX, IDC_TS_REMOVAL_TENSION_MAX, IDC_TS_REMOVAL_TENSION_MAX_UNIT, m_Entry.m_pImpl->m_PrestressedElementCriteria.TensionStressLimit_WithoutReinforcement_TemporaryStrandRemoval.MaxValue, pDisplayUnits->Stress );
   if (m_Entry.m_pImpl->m_PrestressedElementCriteria.TensionStressLimit_WithoutReinforcement_TemporaryStrandRemoval.bHasMaxValue)
   {
      DDV_UnitValueGreaterThanZero(pDX, IDC_TS_REMOVAL_TENSION_MAX,m_Entry.m_pImpl->m_PrestressedElementCriteria.TensionStressLimit_WithoutReinforcement_TemporaryStrandRemoval.MaxValue, pDisplayUnits->Stress );
   }

   DDX_UnitValueAndTag(pDX, IDC_TS_TENSION_WITH_REBAR, IDC_TS_TENSION_WITH_REBAR_UNIT, m_Entry.m_pImpl->m_PrestressedElementCriteria.TensionStressLimit_WithReinforcement_TemporaryStrandRemoval.Coefficient, pDisplayUnits->SqrtPressure);
   DDX_Text(pDX,IDC_TS_TENSION_WITH_REBAR_UNIT,fcTag);
   DDV_UnitValueZeroOrMore(pDX, IDC_TS_TENSION_WITH_REBAR, m_Entry.m_pImpl->m_PrestressedElementCriteria.TensionStressLimit_WithReinforcement_TemporaryStrandRemoval.Coefficient, pDisplayUnits->SqrtPressure);

   // Allowable concrete stress after deck placement
	DDX_Text(pDX, IDC_AFTER_DECK_COMPRESSION, m_Entry.m_pImpl->m_PrestressedElementCriteria.CompressionStressCoefficient_AfterDeckPlacement);
   DDV_GreaterThanZero(pDX, IDC_AFTER_DECK_COMPRESSION, m_Entry.m_pImpl->m_PrestressedElementCriteria.CompressionStressCoefficient_AfterDeckPlacement);

   DDX_UnitValueAndTag(pDX, IDC_AFTER_DECK_TENSION, IDC_AFTER_DECK_TENSION_UNIT, m_Entry.m_pImpl->m_PrestressedElementCriteria.TensionStressLimit_AfterDeckPlacement.Coefficient, pDisplayUnits->SqrtPressure );
   DDX_Text(pDX,IDC_AFTER_DECK_TENSION_UNIT,fcTag);
   DDV_UnitValueZeroOrMore(pDX, IDC_AFTER_DECK_TENSION,m_Entry.m_pImpl->m_PrestressedElementCriteria.TensionStressLimit_AfterDeckPlacement.Coefficient, pDisplayUnits->SqrtPressure );
   DDX_Check_Bool(pDX, IDC_CHECK_AFTER_DECK_TENSION_MAX, m_Entry.m_pImpl->m_PrestressedElementCriteria.TensionStressLimit_AfterDeckPlacement.bHasMaxValue);
   DDX_UnitValueAndTag(pDX, IDC_AFTER_DECK_TENSION_MAX, IDC_AFTER_DECK_TENSION_MAX_UNIT, m_Entry.m_pImpl->m_PrestressedElementCriteria.TensionStressLimit_AfterDeckPlacement.MaxValue, pDisplayUnits->Stress );
   if (m_Entry.m_pImpl->m_PrestressedElementCriteria.TensionStressLimit_AfterDeckPlacement.bHasMaxValue)
   {
      DDV_UnitValueGreaterThanZero(pDX, IDC_AFTER_DECK_TENSION_MAX,m_Entry.m_pImpl->m_PrestressedElementCriteria.TensionStressLimit_AfterDeckPlacement.MaxValue, pDisplayUnits->Stress );
   }

   // Principal Tension Stress in Webs
   DDX_UnitValueAndTag(pDX, IDC_PRINCIPAL_TENSION, IDC_PRINCIPAL_TENSION_UNIT, m_Entry.m_pImpl->m_PrincipalTensionStressCriteria.Coefficient, pDisplayUnits->SqrtPressure);
   DDX_Text(pDX, IDC_PRINCIPAL_TENSION_UNIT, fcTag);
   DDX_CBEnum(pDX, IDC_PRINCIPAL_TENSION_METHOD, m_Entry.m_pImpl->m_PrincipalTensionStressCriteria.Method);
   DDX_Text(pDX, IDC_TENDON_NEARNESS_FACTOR, m_Entry.m_pImpl->m_PrincipalTensionStressCriteria.TendonNearnessFactor);
   DDX_Text(pDX, IDC_UNGROUTED_MULTIPLIER, m_Entry.m_pImpl->m_PrincipalTensionStressCriteria.UngroutedMultiplier);
   DDX_Text(pDX, IDC_GROUTED_MULTIPLIER, m_Entry.m_pImpl->m_PrincipalTensionStressCriteria.GroutedMultiplier);


   DDX_KeywordUnitValueAndTag(pDX, IDC_PRINCIPAL_FC_THRESHOLD, IDC_PRINCIPAL_FC_THRESHOLD_UNIT, _T("All"), m_Entry.m_pImpl->m_PrincipalTensionStressCriteria.FcThreshold, pDisplayUnits->Stress );
   if (pDX->m_bSaveAndValidate && m_Entry.m_pImpl->m_PrincipalTensionStressCriteria.FcThreshold < 0.0 && m_Entry.m_pImpl->m_PrincipalTensionStressCriteria.FcThreshold != -1.0)
   {
      AfxMessageBox(_T("f'c threshold value must be zero or greater or keyword \"All\""));
      pDX->PrepareCtrl(IDC_PRINCIPAL_FC_THRESHOLD);
      pDX->Fail();
   }
}

void CSpecMainSheet::ExchangeLiftingData(CDataExchange* pDX)
{
   CEAFApp* pApp = EAFGetApp();
   const WBFL::Units::IndirectMeasure* pDisplayUnits = pApp->GetDisplayUnits();

	DDX_Text(pDX, IDC_FS_CY_CRACK, m_Entry.m_pImpl->m_LiftingCriteria.FsCracking);
   DDV_NonNegativeDouble(pDX, IDC_FS_CY_CRACK, m_Entry.m_pImpl->m_LiftingCriteria.FsCracking);
	DDX_Text(pDX, IDC_FS_CY_FAIL, m_Entry.m_pImpl->m_LiftingCriteria.FsFailure);
   DDV_NonNegativeDouble(pDX, IDC_FS_CY_FAIL, m_Entry.m_pImpl->m_LiftingCriteria.FsFailure);

   CString tag;
   if ( WBFL::LRFD::BDSManager::GetEdition() < WBFL::LRFD::BDSManager::Edition::SeventhEditionWith2016Interims )
   {
      tag = pApp->GetUnitsMode() == eafTypes::umSI ? _T("sqrt(f'ci (MPa))") : _T("sqrt(f'ci (KSI))");
   }
   else
   {
      tag = pApp->GetUnitsMode() == eafTypes::umSI ? _T("(lambda)sqrt(f'ci (MPa))") : _T("(lambda)sqrt(f'ci (KSI))");
   }

   DDX_UnitValueAndTag(pDX, IDC_FR, IDC_FR_UNIT, m_Entry.m_pImpl->m_LiftingCriteria.ModulusOfRuptureCoefficient[pgsTypes::Normal], pDisplayUnits->SqrtPressure );
   DDX_Text(pDX,IDC_FR_UNIT,tag);

   DDX_UnitValueAndTag(pDX, IDC_ALWC_FR, IDC_ALWC_FR_UNIT, m_Entry.m_pImpl->m_LiftingCriteria.ModulusOfRuptureCoefficient[pgsTypes::AllLightweight], pDisplayUnits->SqrtPressure );
   DDX_Text(pDX,IDC_ALWC_FR_UNIT,tag);

   DDX_UnitValueAndTag(pDX, IDC_SLWC_FR, IDC_SLWC_FR_UNIT, m_Entry.m_pImpl->m_LiftingCriteria.ModulusOfRuptureCoefficient[pgsTypes::SandLightweight], pDisplayUnits->SqrtPressure );
   DDX_Text(pDX,IDC_SLWC_FR_UNIT,tag);

   DDX_UnitValueAndTag(pDX, IDC_PICK_POINT_HEIGHT, IDC_PICK_POINT_HEIGHT_UNITS, m_Entry.m_pImpl->m_LiftingCriteria.PickPointHeight, pDisplayUnits->ComponentDim);
   DDV_UnitValueZeroOrMore(pDX, IDC_PICK_POINT_HEIGHT,m_Entry.m_pImpl->m_LiftingCriteria.PickPointHeight, pDisplayUnits->ComponentDim );
   DDX_UnitValueAndTag(pDX, IDC_LIFTING_LOOP_TOLERANCE, IDC_LIFTING_LOOP_TOLERANCE_UNITS, m_Entry.m_pImpl->m_LiftingCriteria.LiftingLoopTolerance, pDisplayUnits->ComponentDim );
   DDV_UnitValueZeroOrMore(pDX, IDC_LIFTING_LOOP_TOLERANCE,m_Entry.m_pImpl->m_LiftingCriteria.LiftingLoopTolerance, pDisplayUnits->ComponentDim );
   DDX_UnitValueAndTag(pDX, IDC_MIN_CABLE_ANGLE, IDC_MIN_CABLE_ANGLE_UNITS, m_Entry.m_pImpl->m_LiftingCriteria.MinCableInclination, pDisplayUnits->Angle);
   DDV_UnitValueRange(pDX, IDC_MIN_CABLE_ANGLE,m_Entry.m_pImpl->m_LiftingCriteria.MinCableInclination, 0.0, 90., pDisplayUnits->Angle );

   Float64 sweepTolerance = m_Entry.m_pImpl->m_LiftingCriteria.SweepTolerance;
   if ( pApp->GetUnitsMode() == eafTypes::umSI )
   {
      sweepTolerance *= 1000;
      DDX_Text(pDX,IDC_GIRDER_SWEEP_TOL,sweepTolerance);
      if ( pDX->m_bSaveAndValidate )
      {
         sweepTolerance /= 1000;
      }
      else
      {
         m_SpecLiftingPage.GetDlgItem(IDC_GIRDER_SWEEP_TOL_LABEL)->SetWindowText(_T(""));
         m_SpecLiftingPage.GetDlgItem(IDC_GIRDER_SWEEP_TOL_UNIT)->SetWindowText(_T("mm/m"));
      }
   }
   else
   {
      INT x = (INT)::RoundOff((1.0/(sweepTolerance*120.0)),1.0);
      DDX_Text(pDX,IDC_GIRDER_SWEEP_TOL,x);

      if ( pDX->m_bSaveAndValidate )
      {
         sweepTolerance = 1.0/(x*120.0);
      }
      else
      {
         m_SpecLiftingPage.GetDlgItem(IDC_GIRDER_SWEEP_TOL_LABEL)->SetWindowText(_T("1/"));
         m_SpecLiftingPage.GetDlgItem(IDC_GIRDER_SWEEP_TOL_UNIT)->SetWindowText(_T("in/10 ft"));
      }
   }
   DDV_NonNegativeDouble(pDX, IDC_GIRDER_SWEEP_TOL, sweepTolerance);
   m_Entry.m_pImpl->m_LiftingCriteria.SweepTolerance = sweepTolerance;

   DDX_Text(pDX, IDC_LIFTING_GLOBAL_COMPRESSION, m_Entry.m_pImpl->m_LiftingCriteria.CompressionStressCoefficient_GlobalStress);
   DDV_GreaterThanZero(pDX, IDC_LIFTING_GLOBAL_COMPRESSION, m_Entry.m_pImpl->m_LiftingCriteria.CompressionStressCoefficient_GlobalStress);
   DDX_Text(pDX, IDC_LIFTING_PEAK_COMPRESSION, m_Entry.m_pImpl->m_LiftingCriteria.CompressionStressCoefficient_PeakStress);
   DDV_GreaterThanZero(pDX, IDC_LIFTING_PEAK_COMPRESSION, m_Entry.m_pImpl->m_LiftingCriteria.CompressionStressCoefficient_PeakStress);

   DDX_UnitValueAndTag(pDX, IDC_LIFTING_TENSION, IDC_LIFTING_TENSION_UNIT, m_Entry.m_pImpl->m_LiftingCriteria.TensionStressLimitWithoutReinforcement.Coefficient, pDisplayUnits->SqrtPressure );
   DDX_Text(pDX,IDC_LIFTING_TENSION_UNIT,tag);
   DDV_UnitValueZeroOrMore(pDX, IDC_LIFTING_TENSION_UNIT,m_Entry.m_pImpl->m_LiftingCriteria.TensionStressLimitWithoutReinforcement.Coefficient, pDisplayUnits->SqrtPressure );
   DDX_Check_Bool(pDX, IDC_CHECK_LIFTING_TENSION_MAX, m_Entry.m_pImpl->m_LiftingCriteria.TensionStressLimitWithoutReinforcement.bHasMaxValue);
   DDX_UnitValueAndTag(pDX, IDC_LIFTING_TENSION_MAX, IDC_LIFTING_TENSION_MAX_UNIT, m_Entry.m_pImpl->m_LiftingCriteria.TensionStressLimitWithoutReinforcement.MaxValue, pDisplayUnits->Stress );
   if (m_Entry.m_pImpl->m_LiftingCriteria.TensionStressLimitWithoutReinforcement.bHasMaxValue)
   {
      DDV_UnitValueGreaterThanZero(pDX, IDC_LIFTING_TENSION_MAX,m_Entry.m_pImpl->m_LiftingCriteria.TensionStressLimitWithoutReinforcement.MaxValue, pDisplayUnits->Stress );
   }

   DDX_UnitValueAndTag(pDX, IDC_LIFTING_TENSION_WITH_REBAR, IDC_LIFTING_TENSION_WITH_REBAR_UNIT, m_Entry.m_pImpl->m_LiftingCriteria.TensionStressLimitWithReinforcement.Coefficient, pDisplayUnits->SqrtPressure );
   DDX_Text(pDX,IDC_LIFTING_TENSION_WITH_REBAR_UNIT,tag);
   DDV_UnitValueZeroOrMore(pDX, IDC_LIFTING_TENSION_WITH_REBAR_UNIT,m_Entry.m_pImpl->m_LiftingCriteria.TensionStressLimitWithReinforcement.Coefficient, pDisplayUnits->SqrtPressure );

   if (pDX->m_bSaveAndValidate && m_Entry.m_pImpl->m_LiftingCriteria.TensionStressLimitWithReinforcement.Coefficient < m_Entry.m_pImpl->m_LiftingCriteria.TensionStressLimitWithoutReinforcement.Coefficient)
   {
      AfxMessageBox(_T("Allowable tensile stress with bonded reinforcement must be greater than or equal to that without"),MB_OK | MB_ICONWARNING);
      pDX->Fail();
   }

   DDX_Percentage(pDX, IDC_IMPACT_UPWARD_LIFTING, m_Entry.m_pImpl->m_LiftingCriteria.ImpactUp);
   DDV_MinMaxDouble(pDX, m_Entry.m_pImpl->m_LiftingCriteria.ImpactUp, 0.0, 1.0);
	DDX_Percentage(pDX, IDC_IMPACT_DOWNWARD_LIFTING, m_Entry.m_pImpl->m_LiftingCriteria.ImpactDown);
   DDV_MinMaxDouble(pDX, m_Entry.m_pImpl->m_LiftingCriteria.ImpactDown, 0.0, 1.0);

   DDX_Text(pDX, IDC_CAMBER_MULTIPLIER, m_Entry.m_pImpl->m_LiftingCriteria.CamberMultiplier);

   DDX_CBItemData(pDX,IDC_WIND_TYPE,m_Entry.m_pImpl->m_LiftingCriteria.WindLoadType);
   if ( m_Entry.m_pImpl->m_LiftingCriteria.WindLoadType == WBFL::Stability::WindLoadType::Pressure )
   {
      DDX_UnitValueAndTag(pDX,IDC_WIND_LOAD,IDC_WIND_LOAD_UNIT,m_Entry.m_pImpl->m_LiftingCriteria.WindLoad,pDisplayUnits->WindPressure);
   }
   else
   {
      DDX_UnitValueAndTag(pDX,IDC_WIND_LOAD,IDC_WIND_LOAD_UNIT,m_Entry.m_pImpl->m_LiftingCriteria.WindLoad,pDisplayUnits->Velocity);
   }
   DDV_NonNegativeDouble(pDX, IDC_WIND_LOAD, m_Entry.m_pImpl->m_LiftingCriteria.WindLoad);
}

bool CSpecMainSheet::IsHaulingEnabled() const
{
   return m_Entry.m_pImpl->m_HaulingCriteria.bCheck;
}

void CSpecMainSheet::ExchangeWsdotHaulingData(CDataExchange* pDX)
{

   CEAFApp* pApp = EAFGetApp();
   const WBFL::Units::IndirectMeasure* pDisplayUnits = pApp->GetDisplayUnits();

   if (pDX->m_bSaveAndValidate)
   {
      ATLASSERT(m_SpecHaulingErectionPage.m_HaulingAnalysisMethod==pgsTypes::HaulingAnalysisMethod::WSDOT);
      m_Entry.m_pImpl->m_HaulingCriteria.AnalysisMethod = pgsTypes::HaulingAnalysisMethod::WSDOT;
   }

	DDX_Text(pDX, IDC_HE_HAULING_FS_CRACK, m_Entry.m_pImpl->m_HaulingCriteria.WSDOT.FsCracking);
   DDV_NonNegativeDouble(pDX, IDC_HE_HAULING_FS_CRACK, m_Entry.m_pImpl->m_HaulingCriteria.WSDOT.FsCracking);

   DDX_Text(pDX, IDC_HE_HAULING_FS_ROLLOVER, m_Entry.m_pImpl->m_HaulingCriteria.WSDOT.FsFailure);
   DDV_NonNegativeDouble(pDX, IDC_HE_HAULING_FS_ROLLOVER, m_Entry.m_pImpl->m_HaulingCriteria.WSDOT.FsFailure);

   DDX_Text(pDX, IDC_HAULING_GLOBAL_COMPRESSION, m_Entry.m_pImpl->m_HaulingCriteria.WSDOT.CompressionStressCoefficient_GlobalStress);
   DDV_GreaterThanZero(pDX, IDC_HAULING_GLOBAL_COMPRESSION, m_Entry.m_pImpl->m_HaulingCriteria.WSDOT.CompressionStressCoefficient_GlobalStress);

   DDX_Text(pDX, IDC_HAULING_PEAK_COMPRESSION, m_Entry.m_pImpl->m_HaulingCriteria.WSDOT.CompressionStressCoefficient_PeakStress);
   DDV_GreaterThanZero(pDX, IDC_HAULING_PEAK_COMPRESSION, m_Entry.m_pImpl->m_HaulingCriteria.WSDOT.CompressionStressCoefficient_PeakStress);

   CString tag;
   if ( WBFL::LRFD::BDSManager::GetEdition() < WBFL::LRFD::BDSManager::Edition::SeventhEditionWith2016Interims )
   {
      tag = pApp->GetUnitsMode() == eafTypes::umSI ? _T("sqrt(f'c (MPa))") : _T("sqrt(f'c (KSI))");
   }
   else
   {
      tag = pApp->GetUnitsMode() == eafTypes::umSI ? _T("(lambda)sqrt(f'c (MPa))") : _T("(lambda)sqrt(f'c (KSI))");
   }

   DDX_UnitValueAndTag(pDX, IDC_FR, IDC_FR_UNIT, m_Entry.m_pImpl->m_HaulingCriteria.WSDOT.ModulusOfRuptureCoefficient[pgsTypes::Normal], pDisplayUnits->SqrtPressure );
   DDX_Text(pDX,IDC_FR_UNIT,tag);

   DDX_UnitValueAndTag(pDX, IDC_ALWC_FR, IDC_ALWC_FR_UNIT, m_Entry.m_pImpl->m_HaulingCriteria.WSDOT.ModulusOfRuptureCoefficient[pgsTypes::AllLightweight], pDisplayUnits->SqrtPressure );
   DDX_Text(pDX,IDC_ALWC_FR_UNIT,tag);

   DDX_UnitValueAndTag(pDX, IDC_SLWC_FR, IDC_SLWC_FR_UNIT, m_Entry.m_pImpl->m_HaulingCriteria.WSDOT.ModulusOfRuptureCoefficient[pgsTypes::SandLightweight], pDisplayUnits->SqrtPressure );
   DDX_Text(pDX,IDC_SLWC_FR_UNIT,tag);


   DDX_UnitValueAndTag(pDX, IDC_HAULING_TENSION_CROWN, IDC_HAULING_TENSION_CROWN_UNIT, m_Entry.m_pImpl->m_HaulingCriteria.WSDOT.TensionStressLimitWithoutReinforcement[+WBFL::Stability::HaulingSlope::CrownSlope].Coefficient, pDisplayUnits->SqrtPressure );
   DDX_Text(pDX,IDC_HAULING_TENSION_CROWN_UNIT,tag);
   DDV_UnitValueZeroOrMore(pDX, IDC_HAULING_TENSION_CROWN_UNIT, m_Entry.m_pImpl->m_HaulingCriteria.WSDOT.TensionStressLimitWithoutReinforcement[+WBFL::Stability::HaulingSlope::CrownSlope].Coefficient, pDisplayUnits->SqrtPressure );
   DDX_Check_Bool(pDX, IDC_CHECK_HAULING_TENSION_MAX_CROWN, m_Entry.m_pImpl->m_HaulingCriteria.WSDOT.TensionStressLimitWithoutReinforcement[+WBFL::Stability::HaulingSlope::CrownSlope].bHasMaxValue);
   DDX_UnitValueAndTag(pDX, IDC_HAULING_TENSION_MAX_CROWN, IDC_HAULING_TENSION_MAX_UNIT_CROWN, m_Entry.m_pImpl->m_HaulingCriteria.WSDOT.TensionStressLimitWithoutReinforcement[+WBFL::Stability::HaulingSlope::CrownSlope].MaxValue, pDisplayUnits->Stress );
   if (m_Entry.m_pImpl->m_HaulingCriteria.WSDOT.TensionStressLimitWithoutReinforcement[+WBFL::Stability::HaulingSlope::CrownSlope].bHasMaxValue)
   {
      DDV_UnitValueGreaterThanZero(pDX, IDC_HAULING_TENSION_MAX_CROWN, m_Entry.m_pImpl->m_HaulingCriteria.WSDOT.TensionStressLimitWithoutReinforcement[+WBFL::Stability::HaulingSlope::CrownSlope].MaxValue, pDisplayUnits->Stress );
   }

   DDX_UnitValueAndTag(pDX, IDC_HAULING_TENSION_WITH_REBAR_CROWN, IDC_HAULING_TENSION_WITH_REBAR_UNIT_CROWN, m_Entry.m_pImpl->m_HaulingCriteria.WSDOT.TensionStressLimitWithReinforcement[+WBFL::Stability::HaulingSlope::CrownSlope].Coefficient, pDisplayUnits->SqrtPressure  );
   DDX_Text(pDX,IDC_HAULING_TENSION_WITH_REBAR_UNIT_CROWN,tag);
   DDV_UnitValueZeroOrMore(pDX, IDC_HAULING_TENSION_WITH_REBAR_CROWN, m_Entry.m_pImpl->m_HaulingCriteria.WSDOT.TensionStressLimitWithReinforcement[+WBFL::Stability::HaulingSlope::CrownSlope].Coefficient, pDisplayUnits->SqrtPressure );

   if (pDX->m_bSaveAndValidate && m_Entry.m_pImpl->m_HaulingCriteria.WSDOT.TensionStressLimitWithReinforcement[+WBFL::Stability::HaulingSlope::CrownSlope].Coefficient < m_Entry.m_pImpl->m_HaulingCriteria.WSDOT.TensionStressLimitWithoutReinforcement[+WBFL::Stability::HaulingSlope::CrownSlope].Coefficient)
   {
      AfxMessageBox(_T("Allowable tensile stress with bonded reinforcement for the normal crown case must be greater than or equal to that without"),MB_OK | MB_ICONWARNING);
      pDX->Fail();
   }


   
   DDX_UnitValueAndTag(pDX, IDC_HAULING_TENSION_SUPER, IDC_HAULING_TENSION_SUPER_UNIT, m_Entry.m_pImpl->m_HaulingCriteria.WSDOT.TensionStressLimitWithoutReinforcement[+WBFL::Stability::HaulingSlope::Superelevation].Coefficient, pDisplayUnits->SqrtPressure );
   DDX_Text(pDX,IDC_HAULING_TENSION_SUPER_UNIT,tag);
   DDV_UnitValueZeroOrMore(pDX, IDC_HAULING_TENSION_SUPER_UNIT, m_Entry.m_pImpl->m_HaulingCriteria.WSDOT.TensionStressLimitWithoutReinforcement[+WBFL::Stability::HaulingSlope::Superelevation].Coefficient, pDisplayUnits->SqrtPressure );
   DDX_Check_Bool(pDX, IDC_CHECK_HAULING_TENSION_MAX_SUPER, m_Entry.m_pImpl->m_HaulingCriteria.WSDOT.TensionStressLimitWithoutReinforcement[+WBFL::Stability::HaulingSlope::Superelevation].bHasMaxValue);
   DDX_UnitValueAndTag(pDX, IDC_HAULING_TENSION_MAX_SUPER, IDC_HAULING_TENSION_MAX_UNIT_SUPER, m_Entry.m_pImpl->m_HaulingCriteria.WSDOT.TensionStressLimitWithoutReinforcement[+WBFL::Stability::HaulingSlope::Superelevation].MaxValue, pDisplayUnits->Stress );
   if (m_Entry.m_pImpl->m_HaulingCriteria.WSDOT.TensionStressLimitWithoutReinforcement[+WBFL::Stability::HaulingSlope::Superelevation].bHasMaxValue)
   {
      DDV_UnitValueGreaterThanZero(pDX, IDC_HAULING_TENSION_MAX_SUPER, m_Entry.m_pImpl->m_HaulingCriteria.WSDOT.TensionStressLimitWithoutReinforcement[+WBFL::Stability::HaulingSlope::Superelevation].MaxValue, pDisplayUnits->Stress );
   }

   DDX_UnitValueAndTag(pDX, IDC_HAULING_TENSION_WITH_REBAR_SUPER, IDC_HAULING_TENSION_WITH_REBAR_UNIT_SUPER, m_Entry.m_pImpl->m_HaulingCriteria.WSDOT.TensionStressLimitWithReinforcement[+WBFL::Stability::HaulingSlope::Superelevation].Coefficient, pDisplayUnits->SqrtPressure  );
   DDX_Text(pDX,IDC_HAULING_TENSION_WITH_REBAR_UNIT_SUPER,tag);
   DDV_UnitValueZeroOrMore(pDX, IDC_HAULING_TENSION_WITH_REBAR_SUPER, m_Entry.m_pImpl->m_HaulingCriteria.WSDOT.TensionStressLimitWithReinforcement[+WBFL::Stability::HaulingSlope::Superelevation].Coefficient, pDisplayUnits->SqrtPressure );

   if (pDX->m_bSaveAndValidate && m_Entry.m_pImpl->m_HaulingCriteria.WSDOT.TensionStressLimitWithReinforcement[+WBFL::Stability::HaulingSlope::Superelevation].Coefficient < m_Entry.m_pImpl->m_HaulingCriteria.WSDOT.TensionStressLimitWithoutReinforcement[+WBFL::Stability::HaulingSlope::Superelevation].Coefficient)
   {
      AfxMessageBox(_T("Allowable tensile stress with bonded reinforcement for the max. superelevation case must be greater than or equal to that without"),MB_OK | MB_ICONWARNING);
      pDX->Fail();
   }

   // Haul Criteria
	DDX_Percentage(pDX, IDC_IMPACT_UPWARD_HAULING, m_Entry.m_pImpl->m_HaulingCriteria.WSDOT.ImpactUp);
   DDV_MinMaxDouble(pDX, m_Entry.m_pImpl->m_HaulingCriteria.WSDOT.ImpactUp, 0.0, 1.0);

   DDX_Percentage(pDX, IDC_IMPACT_DOWNWARD_HAULING, m_Entry.m_pImpl->m_HaulingCriteria.WSDOT.ImpactDown);
   DDV_MinMaxDouble(pDX, m_Entry.m_pImpl->m_HaulingCriteria.WSDOT.ImpactDown, 0.0, 1.0);

   DDX_CBItemData(pDX, IDC_IMPACT_USAGE, m_Entry.m_pImpl->m_HaulingCriteria.WSDOT.ImpactUsage); // don't use DDX_CBEnum since the combo list is in a different order than the enum

   CString slope_unit(pApp->GetUnitsMode() == eafTypes::umSI ? _T("m/m") : _T("ft/ft"));

   DDX_Text(pDX, IDC_CROWN_SLOPE, m_Entry.m_pImpl->m_HaulingCriteria.WSDOT.RoadwayCrownSlope);
   DDX_Text(pDX, IDC_CROWN_SLOPE_UNIT, slope_unit);

   DDX_Text(pDX, IDC_HE_ROADWAY_SUPERELEVATION, m_Entry.m_pImpl->m_HaulingCriteria.WSDOT.RoadwaySuperelevation);
   DDX_Text(pDX, IDC_HE_ROADWAY_SUPERELEVATION_UNIT, slope_unit);

   Float64 sweepTolerance = m_Entry.m_pImpl->m_HaulingCriteria.WSDOT.SweepTolerance;
   if ( pApp->GetUnitsMode() == eafTypes::umSI )
   {
      sweepTolerance *= 1000;
      DDX_Text(pDX,IDC_GIRDER_SWEEP_TOL,sweepTolerance);
      if ( pDX->m_bSaveAndValidate )
      {
         sweepTolerance /= 1000;
      }
      else
      {
         m_SpecHaulingErectionPage.m_WsdotHaulingDlg.GetDlgItem(IDC_GIRDER_SWEEP_TOL_LABEL)->SetWindowText(_T(""));
         m_SpecHaulingErectionPage.m_WsdotHaulingDlg.GetDlgItem(IDC_GIRDER_SWEEP_TOL_UNIT)->SetWindowText(_T("mm/m"));
      }
   }
   else
   {
      INT x = (INT)::RoundOff((1.0/(sweepTolerance*120.0)),1.0);
      DDX_Text(pDX,IDC_GIRDER_SWEEP_TOL,x);

      if ( pDX->m_bSaveAndValidate )
      {
         sweepTolerance = 1.0/(x*120.0);
      }
      else
      {
         m_SpecHaulingErectionPage.m_WsdotHaulingDlg.GetDlgItem(IDC_GIRDER_SWEEP_TOL_LABEL)->SetWindowText(_T("1/"));
         m_SpecHaulingErectionPage.m_WsdotHaulingDlg.GetDlgItem(IDC_GIRDER_SWEEP_TOL_UNIT)->SetWindowText(_T("in/10 ft"));
      }
   }
   DDV_NonNegativeDouble(pDX, IDC_GIRDER_SWEEP_TOL, sweepTolerance);
   m_Entry.m_pImpl->m_HaulingCriteria.WSDOT.SweepTolerance = sweepTolerance;

   DDX_UnitValueAndTag(pDX, IDC_SWEEP_GROWTH, IDC_SWEEP_GROWTH_UNIT, m_Entry.m_pImpl->m_HaulingCriteria.WSDOT.SweepGrowth, pDisplayUnits->ComponentDim);

   DDX_UnitValueAndTag(pDX, IDC_SUPPORT_PLACEMENT_TOLERANCE, IDC_SUPPORT_PLACEMENT_TOLERANCE_UNITS, m_Entry.m_pImpl->m_HaulingCriteria.WSDOT.SupportPlacementTolerance, pDisplayUnits->ComponentDim );
   DDV_UnitValueGreaterThanZero(pDX, IDC_SUPPORT_PLACEMENT_TOLERANCE,m_Entry.m_pImpl->m_HaulingCriteria.WSDOT.SupportPlacementTolerance, pDisplayUnits->ComponentDim );

   DDX_Text(pDX, IDC_CAMBER_MULTIPLIER, m_Entry.m_pImpl->m_HaulingCriteria.WSDOT.CamberMultiplier);


   DDX_CBItemData(pDX,IDC_WIND_TYPE,m_Entry.m_pImpl->m_HaulingCriteria.WSDOT.WindLoadType); // Don't use DDX_CBEnum because the settings in the list are not in the same order as the enum
   if ( m_Entry.m_pImpl->m_HaulingCriteria.WSDOT.WindLoadType == WBFL::Stability::WindLoadType::Pressure )
   {
      DDX_UnitValueAndTag(pDX,IDC_WIND_LOAD,IDC_WIND_LOAD_UNIT,m_Entry.m_pImpl->m_HaulingCriteria.WSDOT.WindLoad,pDisplayUnits->WindPressure);
   }
   else
   {
      DDX_UnitValueAndTag(pDX,IDC_WIND_LOAD,IDC_WIND_LOAD_UNIT,m_Entry.m_pImpl->m_HaulingCriteria.WSDOT.WindLoad,pDisplayUnits->Velocity);
   }
   DDV_NonNegativeDouble(pDX, IDC_WIND_LOAD, m_Entry.m_pImpl->m_HaulingCriteria.WSDOT.WindLoad);

   DDX_CBItemData(pDX,IDC_CF_TYPE,m_Entry.m_pImpl->m_HaulingCriteria.WSDOT.CentrifugalForceType); // Don't use DDX_CBEnum because the settings in the list are not in the same order as the enum
   DDX_UnitValueAndTag(pDX,IDC_HAUL_SPEED,IDC_HAUL_SPEED_UNIT,m_Entry.m_pImpl->m_HaulingCriteria.WSDOT.HaulingSpeed,pDisplayUnits->Velocity);
   DDV_NonNegativeDouble(pDX, IDC_WIND_LOAD, m_Entry.m_pImpl->m_HaulingCriteria.WSDOT.HaulingSpeed);

   DDX_UnitValueAndTag(pDX,IDC_RADIUS,IDC_RADIUS_UNIT,m_Entry.m_pImpl->m_HaulingCriteria.WSDOT.TurningRadius,pDisplayUnits->SpanLength);
   DDV_UnitValueGreaterThanZero(pDX, IDC_RADIUS,m_Entry.m_pImpl->m_HaulingCriteria.WSDOT.TurningRadius, pDisplayUnits->SpanLength );
}

void CSpecMainSheet::ExchangeKdotHaulingData(CDataExchange* pDX)
{
   CEAFApp* pApp = EAFGetApp();
   const WBFL::Units::IndirectMeasure* pDisplayUnits = pApp->GetDisplayUnits();

   if (pDX->m_bSaveAndValidate)
   {
      ATLASSERT(m_SpecHaulingErectionPage.m_HaulingAnalysisMethod==pgsTypes::HaulingAnalysisMethod::KDOT);
      m_Entry.m_pImpl->m_HaulingCriteria.AnalysisMethod = pgsTypes::HaulingAnalysisMethod::KDOT;
   }

	DDX_Text(pDX, IDC_HAULING_COMPRESSION, m_Entry.m_pImpl->m_HaulingCriteria.KDOT.CompressionStressLimitCoefficient);
   DDV_GreaterThanZero(pDX, IDC_HAULING_COMPRESSION, m_Entry.m_pImpl->m_HaulingCriteria.KDOT.CompressionStressLimitCoefficient);

   CString tag;
   if ( WBFL::LRFD::BDSManager::GetEdition() < WBFL::LRFD::BDSManager::Edition::SeventhEditionWith2016Interims )
   {
      tag = pApp->GetUnitsMode() == eafTypes::umSI ? _T("sqrt(f'c (MPa))") : _T("sqrt(f'c (KSI))");
   }
   else
   {
      tag = pApp->GetUnitsMode() == eafTypes::umSI ? _T("(lambda)sqrt(f'c (MPa))") : _T("(lambda)sqrt(f'c (KSI))");
   }

   // Use the normal crown values for KDOT
   DDX_UnitValueAndTag(pDX, IDC_HAULING_TENSION, IDC_HAULING_TENSION_UNIT, m_Entry.m_pImpl->m_HaulingCriteria.KDOT.TensionStressLimitWithoutReinforcement.Coefficient, pDisplayUnits->SqrtPressure );
   DDX_Text(pDX,IDC_HAULING_TENSION_UNIT,tag);
   DDV_UnitValueZeroOrMore(pDX, IDC_HAULING_TENSION_UNIT, m_Entry.m_pImpl->m_HaulingCriteria.KDOT.TensionStressLimitWithoutReinforcement.Coefficient, pDisplayUnits->SqrtPressure );
   DDX_Check_Bool(pDX, IDC_CHECK_HAULING_TENSION_MAX, m_Entry.m_pImpl->m_HaulingCriteria.KDOT.TensionStressLimitWithoutReinforcement.bHasMaxValue);
   DDX_UnitValueAndTag(pDX, IDC_HAULING_TENSION_MAX, IDC_HAULING_TENSION_MAX_UNIT, m_Entry.m_pImpl->m_HaulingCriteria.KDOT.TensionStressLimitWithoutReinforcement.MaxValue, pDisplayUnits->Stress );
   if (m_Entry.m_pImpl->m_HaulingCriteria.KDOT.TensionStressLimitWithoutReinforcement.bHasMaxValue)
   {
      DDV_UnitValueGreaterThanZero(pDX, IDC_HAULING_TENSION_MAX, m_Entry.m_pImpl->m_HaulingCriteria.KDOT.TensionStressLimitWithoutReinforcement.MaxValue, pDisplayUnits->Stress );
   }

   DDX_UnitValueAndTag(pDX, IDC_HAULING_TENSION_WITH_REBAR, IDC_HAULING_TENSION_WITH_REBAR_UNIT, m_Entry.m_pImpl->m_HaulingCriteria.KDOT.TensionStressLimitWithReinforcement.Coefficient, pDisplayUnits->SqrtPressure  );
   DDX_Text(pDX,IDC_HAULING_TENSION_WITH_REBAR_UNIT,tag);
   DDV_UnitValueZeroOrMore(pDX, IDC_HAULING_TENSION_WITH_REBAR, m_Entry.m_pImpl->m_HaulingCriteria.KDOT.TensionStressLimitWithReinforcement.Coefficient, pDisplayUnits->SqrtPressure );

   if (pDX->m_bSaveAndValidate && m_Entry.m_pImpl->m_HaulingCriteria.KDOT.TensionStressLimitWithReinforcement.Coefficient < m_Entry.m_pImpl->m_HaulingCriteria.KDOT.TensionStressLimitWithoutReinforcement.Coefficient)
   {
      AfxMessageBox(_T("Allowable tensile stress with bonded reinforcement must be greater than or equal to that without"),MB_OK | MB_ICONWARNING);
      pDX->Fail();
   }

	DDX_Text(pDX, IDC_G_OVERHANG, m_Entry.m_pImpl->m_HaulingCriteria.KDOT.OverhangGFactor);
   DDV_MinMaxDouble(pDX, m_Entry.m_pImpl->m_HaulingCriteria.KDOT.OverhangGFactor, 1.0, 100.0);
	DDX_Text(pDX, IDC_G_INTERIOR, m_Entry.m_pImpl->m_HaulingCriteria.KDOT.InteriorGFactor);
   DDV_MinMaxDouble(pDX, m_Entry.m_pImpl->m_HaulingCriteria.KDOT.InteriorGFactor, 1.0, 100.0);
}


void CSpecMainSheet::ExchangeMomentCapacityData(CDataExchange* pDX)
{
   CEAFApp* pApp = EAFGetApp();
   const WBFL::Units::IndirectMeasure* pDisplayUnits = pApp->GetDisplayUnits();

   DDX_CBIndex(pDX, IDC_MOMENT, m_Entry.m_pImpl->m_MomentCapacityCriteria.OverReinforcedMomentCapacity );
   DDX_CBItemData(pDX, IDC_NEG_MOMENT, m_Entry.m_pImpl->m_MomentCapacityCriteria.bIncludeNoncompositeMomentsForNegMomentDesign);
   DDX_Check_Bool(pDX, IDC_INCLUDE_STRAND_FOR_NEG_MOMENT, m_Entry.m_pImpl->m_MomentCapacityCriteria.bIncludeStrandForNegMoment);
   DDX_Check_Bool(pDX, IDC_INCLUDE_REBAR_MOMENT, m_Entry.m_pImpl->m_MomentCapacityCriteria.bIncludeRebar );
   DDX_Check_Bool(pDX, IDC_CONSIDER_REINFORCEMENT_STRAIN_LIMITS, m_Entry.m_pImpl->m_MomentCapacityCriteria.bConsiderReinforcementStrainLimit);

   DDX_Text(pDX, IDC_SLICE_COUNT, m_Entry.m_pImpl->m_MomentCapacityCriteria.nMomentCapacitySlices);
   DDV_MinMaxULongLong(pDX, m_Entry.m_pImpl->m_MomentCapacityCriteria.nMomentCapacitySlices, 10, 100);

   CString tag;
   if ( WBFL::LRFD::BDSManager::GetEdition() < WBFL::LRFD::BDSManager::Edition::SeventhEditionWith2016Interims )
   {
      tag = pApp->GetUnitsMode() == eafTypes::umSI ? _T("sqrt(f'c (MPa))") : _T("sqrt(f'c (KSI))");
   }
   else
   {
      tag = pApp->GetUnitsMode() == eafTypes::umSI ? _T("(lambda)sqrt(f'c (MPa))") : _T("(lambda)sqrt(f'c (KSI))");
   }

   DDX_UnitValueAndTag(pDX, IDC_FR,      IDC_FR_LABEL,      m_Entry.m_pImpl->m_MomentCapacityCriteria.ModulusOfRuptureCoefficient[pgsTypes::Normal],          pDisplayUnits->SqrtPressure );
   DDX_UnitValueAndTag(pDX, IDC_ALWC_FR, IDC_ALWC_FR_LABEL, m_Entry.m_pImpl->m_MomentCapacityCriteria.ModulusOfRuptureCoefficient[pgsTypes::AllLightweight],  pDisplayUnits->SqrtPressure );
   DDX_UnitValueAndTag(pDX, IDC_SLWC_FR, IDC_SLWC_FR_LABEL, m_Entry.m_pImpl->m_MomentCapacityCriteria.ModulusOfRuptureCoefficient[pgsTypes::SandLightweight], pDisplayUnits->SqrtPressure );

   DDX_Text(pDX, IDC_FR_UNIT,     tag);
   DDX_Text(pDX, IDC_ALWC_FR_UNIT,tag);
   DDX_Text(pDX, IDC_SLWC_FR_UNIT,tag);

   DDV_UnitValueZeroOrMore(pDX, IDC_FR,      m_Entry.m_pImpl->m_MomentCapacityCriteria.ModulusOfRuptureCoefficient[pgsTypes::Normal],          pDisplayUnits->SqrtPressure );
   DDV_UnitValueZeroOrMore(pDX, IDC_ALWC_FR, m_Entry.m_pImpl->m_MomentCapacityCriteria.ModulusOfRuptureCoefficient[pgsTypes::AllLightweight],  pDisplayUnits->SqrtPressure );
   DDV_UnitValueZeroOrMore(pDX, IDC_SLWC_FR, m_Entry.m_pImpl->m_MomentCapacityCriteria.ModulusOfRuptureCoefficient[pgsTypes::SandLightweight], pDisplayUnits->SqrtPressure );

   // NOTE: this looks goofy, but it is correct. There is only one LWC entry for both all and sand lightweight
   // but it is easier to have 3 sets of values so the application is consistent.
   DDX_Text(pDX,IDC_NWC_PHI_TENSION_RC,      m_Entry.m_pImpl->m_MomentCapacityCriteria.PhiTensionRC[pgsTypes::Normal]);
   DDX_Text(pDX,IDC_NWC_PHI_TENSION_PS,      m_Entry.m_pImpl->m_MomentCapacityCriteria.PhiTensionPS[pgsTypes::Normal]);
   DDX_Text(pDX,IDC_NWC_PHI_TENSION_SPLICED, m_Entry.m_pImpl->m_MomentCapacityCriteria.PhiTensionSpliced[pgsTypes::Normal]);
   DDX_Text(pDX,IDC_NWC_PHI_COMPRESSION,     m_Entry.m_pImpl->m_MomentCapacityCriteria.PhiCompression[pgsTypes::Normal]);

   DDX_Text(pDX,IDC_LWC_PHI_TENSION_RC,      m_Entry.m_pImpl->m_MomentCapacityCriteria.PhiTensionRC[pgsTypes::AllLightweight]);
   DDX_Text(pDX,IDC_LWC_PHI_TENSION_PS,      m_Entry.m_pImpl->m_MomentCapacityCriteria.PhiTensionPS[pgsTypes::AllLightweight]);
   DDX_Text(pDX,IDC_LWC_PHI_TENSION_SPLICED, m_Entry.m_pImpl->m_MomentCapacityCriteria.PhiTensionSpliced[pgsTypes::AllLightweight]);
   DDX_Text(pDX,IDC_LWC_PHI_COMPRESSION,     m_Entry.m_pImpl->m_MomentCapacityCriteria.PhiCompression[pgsTypes::AllLightweight]);

   DDX_Text(pDX,IDC_LWC_PHI_TENSION_RC,      m_Entry.m_pImpl->m_MomentCapacityCriteria.PhiTensionRC[pgsTypes::SandLightweight]);
   DDX_Text(pDX,IDC_LWC_PHI_TENSION_PS,      m_Entry.m_pImpl->m_MomentCapacityCriteria.PhiTensionPS[pgsTypes::SandLightweight]);
   DDX_Text(pDX,IDC_LWC_PHI_TENSION_SPLICED, m_Entry.m_pImpl->m_MomentCapacityCriteria.PhiTensionSpliced[pgsTypes::SandLightweight]);
   DDX_Text(pDX,IDC_LWC_PHI_COMPRESSION,     m_Entry.m_pImpl->m_MomentCapacityCriteria.PhiCompression[pgsTypes::SandLightweight]);

   DDX_Text(pDX, IDC_UHPC_PHI_TENSION_RC,      m_Entry.m_pImpl->m_MomentCapacityCriteria.PhiTensionRC[pgsTypes::PCI_UHPC]);
   DDX_Text(pDX, IDC_UHPC_PHI_TENSION_PS,      m_Entry.m_pImpl->m_MomentCapacityCriteria.PhiTensionPS[pgsTypes::PCI_UHPC]);
   DDX_Text(pDX, IDC_UHPC_PHI_TENSION_SPLICED, m_Entry.m_pImpl->m_MomentCapacityCriteria.PhiTensionSpliced[pgsTypes::PCI_UHPC]);
   DDX_Text(pDX, IDC_UHPC_PHI_COMPRESSION,     m_Entry.m_pImpl->m_MomentCapacityCriteria.PhiCompression[pgsTypes::PCI_UHPC]);

   DDX_Text(pDX, IDC_NWC_JOINT_PHI,           m_Entry.m_pImpl->m_MomentCapacityCriteria.PhiClosureJoint[pgsTypes::Normal]);
   DDX_Text(pDX, IDC_LWC_JOINT_PHI,           m_Entry.m_pImpl->m_MomentCapacityCriteria.PhiClosureJoint[pgsTypes::AllLightweight]);
   DDX_Text(pDX, IDC_LWC_JOINT_PHI,           m_Entry.m_pImpl->m_MomentCapacityCriteria.PhiClosureJoint[pgsTypes::SandLightweight]);
   DDX_Text(pDX, IDC_UHPC_JOINT_PHI,          m_Entry.m_pImpl->m_MomentCapacityCriteria.PhiClosureJoint[pgsTypes::PCI_UHPC]);
}

void CSpecMainSheet::CheckShearCapacityMethod()
{
   // makes sure the shear capacity method is consistent with the specification version that is selected
   if ( GetSpecVersion() < WBFL::LRFD::BDSManager::Edition::FourthEdition2007 &&  // if we are before 4th Edition
        m_Entry.m_pImpl->m_ShearCapacityCriteria.CapacityMethod == pgsTypes::scmVciVcw ) // Vci/Vcw is not a valid option
   {
      // force to the general method
      if ( GetSpecVersion() <= WBFL::LRFD::BDSManager::Edition::FourthEdition2007 )
      {
         m_Entry.m_pImpl->m_ShearCapacityCriteria.CapacityMethod = pgsTypes::scmBTTables;
      }
      else
      {
         m_Entry.m_pImpl->m_ShearCapacityCriteria.CapacityMethod = pgsTypes::scmBTEquations;
      }
   }

   // The general method from the 2007 spec becomes the tables method in the 2008 spec
   // make that adjustment here
   if ( GetSpecVersion() < WBFL::LRFD::BDSManager::Edition::FourthEditionWith2008Interims && m_Entry.m_pImpl->m_ShearCapacityCriteria.CapacityMethod == pgsTypes::scmBTEquations )
   {
      m_Entry.m_pImpl->m_ShearCapacityCriteria.CapacityMethod = pgsTypes::scmBTTables;
   }

   if ( GetSpecVersion() < WBFL::LRFD::BDSManager::Edition::SecondEditionWith2000Interims &&  // if we are before 2nd Edition + 2000
        m_Entry.m_pImpl->m_ShearCapacityCriteria.CapacityMethod == pgsTypes::scmWSDOT2001 ) // WSDOT 2001 is not a valid option
   {
      // force to the general method
      if ( GetSpecVersion() <= WBFL::LRFD::BDSManager::Edition::FourthEdition2007 )
      {
         m_Entry.m_pImpl->m_ShearCapacityCriteria.CapacityMethod = pgsTypes::scmBTTables;
      }
      else
      {
         m_Entry.m_pImpl->m_ShearCapacityCriteria.CapacityMethod = pgsTypes::scmBTEquations;
      }
   }

   if ( GetSpecVersion() < WBFL::LRFD::BDSManager::Edition::FourthEdition2007 &&  // if we are before 4th Edition
        m_Entry.m_pImpl->m_ShearCapacityCriteria.CapacityMethod == pgsTypes::scmWSDOT2007 ) // WSDOT 2007 is not a valid option
   {
      m_Entry.m_pImpl->m_ShearCapacityCriteria.CapacityMethod = pgsTypes::scmWSDOT2001; // force to WSDOT 2001
   }
}

void CSpecMainSheet::ExchangeShearCapacityData(CDataExchange* pDX)
{
   CEAFApp* pApp = EAFGetApp();
   const WBFL::Units::IndirectMeasure* pDisplayUnits = pApp->GetDisplayUnits();

   DDX_Check_Bool(pDX, IDC_LIMIT_STRAIN, m_Entry.m_pImpl->m_ShearCapacityCriteria.bLimitNetTensionStrainToPositiveValues);

   DDX_CBEnum(pDX, IDC_LRSH, m_Entry.m_pImpl->m_ShearCapacityCriteria.LongitudinalReinforcementForShearMethod);
   DDX_Check_Bool(pDX, IDC_INCLUDE_REBAR_SHEAR, m_Entry.m_pImpl->m_ShearCapacityCriteria.bIncludeRebar );
   DDX_Check_Bool(pDX, IDC_USE_DECK_FOR_PC, m_Entry.m_pImpl->m_InterfaceShearCriteria.bUseDeckWeightForPc);

   DDX_CBEnum(pDX,IDC_SHEAR_FLOW_METHOD,m_Entry.m_pImpl->m_InterfaceShearCriteria.ShearFlowMethod);

   // we have to use item data here because the enum constants are out of order
   DDX_CBItemData(pDX, IDC_SHEAR_CAPACITY, m_Entry.m_pImpl->m_ShearCapacityCriteria.CapacityMethod );

   // Not every shear method is available with every spec. The user could have set the method
   // then changed the spec. We have keep the data valid

   if ( pDX->m_bSaveAndValidate )
   {
      CheckShearCapacityMethod();
   }

   CString tag;
   if ( WBFL::LRFD::BDSManager::GetEdition() < WBFL::LRFD::BDSManager::Edition::SeventhEditionWith2016Interims )
   {
      tag = pApp->GetUnitsMode() == eafTypes::umSI ? _T("sqrt(f'c (MPa))") : _T("sqrt(f'c (KSI))");
   }
   else
   {
      tag = pApp->GetUnitsMode() == eafTypes::umSI ? _T("(lambda)sqrt(f'c (MPa))") : _T("(lambda)sqrt(f'c (KSI))");
   }

   DDX_UnitValueAndTag(pDX, IDC_FR,      IDC_FR_LABEL,      m_Entry.m_pImpl->m_ShearCapacityCriteria.ModulusOfRuptureCoefficient[pgsTypes::Normal],          pDisplayUnits->SqrtPressure );
   DDX_UnitValueAndTag(pDX, IDC_ALWC_FR, IDC_FR_LABEL_ALWC, m_Entry.m_pImpl->m_ShearCapacityCriteria.ModulusOfRuptureCoefficient[pgsTypes::AllLightweight],  pDisplayUnits->SqrtPressure );
   DDX_UnitValueAndTag(pDX, IDC_SLWC_FR, IDC_FR_LABEL_SLWC, m_Entry.m_pImpl->m_ShearCapacityCriteria.ModulusOfRuptureCoefficient[pgsTypes::SandLightweight], pDisplayUnits->SqrtPressure );

   DDX_Text(pDX, IDC_FR_UNIT,      tag);
   DDX_Text(pDX, IDC_ALWC_FR_UNIT, tag);
   DDX_Text(pDX, IDC_SLWC_FR_UNIT, tag);

   DDV_UnitValueZeroOrMore(pDX, IDC_FR ,     m_Entry.m_pImpl->m_ShearCapacityCriteria.ModulusOfRuptureCoefficient[pgsTypes::Normal],          pDisplayUnits->SqrtPressure );
   DDV_UnitValueZeroOrMore(pDX, IDC_SLWC_FR, m_Entry.m_pImpl->m_ShearCapacityCriteria.ModulusOfRuptureCoefficient[pgsTypes::SandLightweight], pDisplayUnits->SqrtPressure );
   DDV_UnitValueZeroOrMore(pDX, IDC_ALWC_FR, m_Entry.m_pImpl->m_ShearCapacityCriteria.ModulusOfRuptureCoefficient[pgsTypes::AllLightweight],  pDisplayUnits->SqrtPressure );

   DDX_Text(pDX,IDC_NWC_PHI,m_Entry.m_pImpl->m_ShearCapacityCriteria.Phi[pgsTypes::Normal]);
   DDX_Text(pDX, IDC_LWC_PHI, m_Entry.m_pImpl->m_ShearCapacityCriteria.Phi[pgsTypes::SandLightweight]);
   DDX_Text(pDX, IDC_LWC_PHI, m_Entry.m_pImpl->m_ShearCapacityCriteria.Phi[pgsTypes::AllLightweight]);
   DDX_Text(pDX, IDC_UHPC_PHI, m_Entry.m_pImpl->m_ShearCapacityCriteria.Phi[pgsTypes::PCI_UHPC]);

   DDX_Text(pDX,IDC_NWC_PHI_DEBOND,m_Entry.m_pImpl->m_ShearCapacityCriteria.PhiDebonded[pgsTypes::Normal]);
   DDX_Text(pDX, IDC_LWC_PHI_DEBOND, m_Entry.m_pImpl->m_ShearCapacityCriteria.PhiDebonded[pgsTypes::SandLightweight]);
   DDX_Text(pDX, IDC_LWC_PHI_DEBOND, m_Entry.m_pImpl->m_ShearCapacityCriteria.PhiDebonded[pgsTypes::AllLightweight]);
   DDX_Text(pDX, IDC_UHPC_PHI_DEBOND, m_Entry.m_pImpl->m_ShearCapacityCriteria.PhiDebonded[pgsTypes::PCI_UHPC]);

   DDX_Text(pDX,IDC_NWC_JOINT_PHI,  m_Entry.m_pImpl->m_ShearCapacityCriteria.PhiClosureJoint[pgsTypes::Normal]);
   DDX_Text(pDX, IDC_LWC_JOINT_PHI, m_Entry.m_pImpl->m_ShearCapacityCriteria.PhiClosureJoint[pgsTypes::AllLightweight]);
   DDX_Text(pDX, IDC_LWC_JOINT_PHI, m_Entry.m_pImpl->m_ShearCapacityCriteria.PhiClosureJoint[pgsTypes::SandLightweight]);
   DDX_Text(pDX, IDC_UHPC_JOINT_PHI, m_Entry.m_pImpl->m_ShearCapacityCriteria.PhiClosureJoint[pgsTypes::PCI_UHPC]);

   if (pDX->m_bSaveAndValidate)
   {
      // these values share a single control
      m_Entry.m_pImpl->m_ShearCapacityCriteria.Phi[pgsTypes::UHPC] = m_Entry.m_pImpl->m_ShearCapacityCriteria.Phi[pgsTypes::PCI_UHPC];
      m_Entry.m_pImpl->m_ShearCapacityCriteria.PhiDebonded[pgsTypes::UHPC] = m_Entry.m_pImpl->m_ShearCapacityCriteria.PhiDebonded[pgsTypes::PCI_UHPC];
      m_Entry.m_pImpl->m_ShearCapacityCriteria.PhiClosureJoint[pgsTypes::UHPC] = m_Entry.m_pImpl->m_ShearCapacityCriteria.PhiClosureJoint[pgsTypes::PCI_UHPC];
   }

   DDX_UnitValueAndTag(pDX, IDC_SMAX, IDC_SMAX_UNIT, m_Entry.m_pImpl->m_InterfaceShearCriteria.MaxInterfaceShearConnectorSpacing, pDisplayUnits->ComponentDim );

   DDX_Text(pDX, IDC_SPACING_COEFFICIENT_1, m_Entry.m_pImpl->m_ShearCapacityCriteria.StirrupSpacingCoefficient[0]);
   DDX_UnitValueAndTag(pDX, IDC_MAX_SPACING_1, IDC_MAX_SPACING_1_UNIT, m_Entry.m_pImpl->m_ShearCapacityCriteria.MaxStirrupSpacing[0], pDisplayUnits->ComponentDim );
   DDX_Text(pDX, IDC_SPACING_COEFFICIENT_2, m_Entry.m_pImpl->m_ShearCapacityCriteria.StirrupSpacingCoefficient[1]);
   DDX_UnitValueAndTag(pDX, IDC_MAX_SPACING_2, IDC_MAX_SPACING_2_UNIT, m_Entry.m_pImpl->m_ShearCapacityCriteria.MaxStirrupSpacing[1], pDisplayUnits->ComponentDim );

   DDV_UnitValueGreaterThanZero(pDX, IDC_MAX_SPACING_1, m_Entry.m_pImpl->m_ShearCapacityCriteria.MaxStirrupSpacing[0], pDisplayUnits->ComponentDim );
   DDV_UnitValueGreaterThanZero(pDX, IDC_MAX_SPACING_2, m_Entry.m_pImpl->m_ShearCapacityCriteria.MaxStirrupSpacing[1], pDisplayUnits->ComponentDim );
}

void CSpecMainSheet::ExchangeCreepData(CDataExchange* pDX)
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   CEAFApp* pApp = EAFGetApp();
   const WBFL::Units::IndirectMeasure* pDisplayUnits = pApp->GetDisplayUnits();

   DDX_UnitValueAndTag(pDX, IDC_XFER_TIME, IDC_XFER_TIME_TAG, m_Entry.m_pImpl->m_CreepCriteria.XferTime, pDisplayUnits->Time );
   DDV_UnitValueGreaterThanZero(pDX, IDC_XFER_TIME,m_Entry.m_pImpl->m_CreepCriteria.XferTime, pDisplayUnits->Time );

   DDX_UnitValueAndTag(pDX, IDC_CREEP_DURATION1_MIN, IDC_CREEP_DURATION1_TAG, m_Entry.m_pImpl->m_CreepCriteria.CreepDuration1Min, pDisplayUnits->Time2 );
   DDV_UnitValueGreaterThanZero(pDX, IDC_CREEP_DURATION1_MIN,m_Entry.m_pImpl->m_CreepCriteria.CreepDuration1Min, pDisplayUnits->Time2 );

   DDX_UnitValueAndTag(pDX, IDC_CREEP_DURATION1_MAX, IDC_CREEP_DURATION1_TAG, m_Entry.m_pImpl->m_CreepCriteria.CreepDuration1Max, pDisplayUnits->Time2 );
   DDV_UnitValueLimitOrMore(pDX, IDC_CREEP_DURATION1_MAX,m_Entry.m_pImpl->m_CreepCriteria.CreepDuration1Max, m_Entry.m_pImpl->m_CreepCriteria.CreepDuration1Min, pDisplayUnits->Time2 );

   DDX_UnitValueAndTag(pDX, IDC_CREEP_DURATION2_MIN, IDC_CREEP_DURATION2_TAG, m_Entry.m_pImpl->m_CreepCriteria.CreepDuration2Min, pDisplayUnits->Time2 );
   DDV_UnitValueGreaterThanLimit(pDX, IDC_CREEP_DURATION2_MIN,m_Entry.m_pImpl->m_CreepCriteria.CreepDuration2Min, m_Entry.m_pImpl->m_CreepCriteria.XferTime, pDisplayUnits->Time2 );

   DDX_UnitValueAndTag(pDX, IDC_CREEP_DURATION2_MAX, IDC_CREEP_DURATION2_TAG, m_Entry.m_pImpl->m_CreepCriteria.CreepDuration2Max, pDisplayUnits->Time2 );
   DDV_UnitValueLimitOrMore(pDX, IDC_CREEP_DURATION2_MAX,m_Entry.m_pImpl->m_CreepCriteria.CreepDuration2Max, m_Entry.m_pImpl->m_CreepCriteria.CreepDuration2Min, pDisplayUnits->Time2 );

   if ( pDX->m_bSaveAndValidate )
   {
      if ( m_Entry.m_pImpl->m_CreepCriteria.CreepDuration2Min < m_Entry.m_pImpl->m_CreepCriteria.CreepDuration1Min )
      {
         pDX->PrepareEditCtrl(IDC_CREEP_DURATION2_MIN);
         AfxMessageBox(_T("The time from prestress transfer to deck casting must be greater than the time from prestress transfer to temporary strand removal/diaphragm casting."));
         pDX->Fail();
      }

      if ( m_Entry.m_pImpl->m_CreepCriteria.CreepDuration2Max < m_Entry.m_pImpl->m_CreepCriteria.CreepDuration1Max )
      {
         pDX->PrepareEditCtrl(IDC_CREEP_DURATION2_MAX);
         AfxMessageBox(_T("The time from prestress transfer to deck casting must be greater than the time from prestress transfer to temporary strand removal/diaphragm casting."));
         pDX->Fail();
      }
   }

   DDX_UnitValueAndTag(pDX, IDC_NC_CREEP, IDC_NC_CREEP_TAG, m_Entry.m_pImpl->m_CreepCriteria.TotalCreepDuration, pDisplayUnits->Time2 );
   DDV_UnitValueLimitOrMore(pDX, IDC_NC_CREEP,m_Entry.m_pImpl->m_CreepCriteria.TotalCreepDuration, m_Entry.m_pImpl->m_CreepCriteria.CreepDuration2Min, pDisplayUnits->Time2 );

   DDX_RadioEnum(pDX,IDC_CURING_METHOD,m_Entry.m_pImpl->m_CreepCriteria.CuringMethod);

   DDX_Text(pDX,IDC_CURING_TIME_FACTOR,m_Entry.m_pImpl->m_CreepCriteria.CuringMethodTimeAdjustmentFactor);
   DDV_MinMaxDouble(pDX,m_Entry.m_pImpl->m_CreepCriteria.CuringMethodTimeAdjustmentFactor,1,999);

   DDX_Percentage(pDX,IDC_CAMBER_VARIABILITY, m_Entry.m_pImpl->m_CreepCriteria.CamberVariability);
   if ( pDX->m_bSaveAndValidate )
   {
      Float64 val = m_Entry.m_pImpl->m_CreepCriteria.CamberVariability * 100.0;
      DDV_Range( pDX, mfcDDV::LE,mfcDDV::GE,val,0.0,100.0);
   }

   // computation of haunch load
   DDX_CBEnum(pDX, IDC_HAUNCH_COMP_CB,m_Entry.m_pImpl->m_HaunchCriteria.HaunchLoadComputationType);
   DDX_UnitValueAndTag(pDX, IDC_HAUNCH_TOLER, IDC_HAUNCH_TOLER_UNIT, m_Entry.m_pImpl->m_HaunchCriteria.HaunchLoadCamberTolerance, pDisplayUnits->ComponentDim);
   DDV_UnitValueGreaterThanZero(pDX, IDC_HAUNCH_TOLER,m_Entry.m_pImpl->m_HaunchCriteria.HaunchLoadCamberTolerance, pDisplayUnits->ComponentDim );

   DDX_Percentage(pDX, IDC_HAUNCH_FACTOR, m_Entry.m_pImpl->m_HaunchCriteria.HaunchLoadCamberFactor);
   if (pDX->m_bSaveAndValidate)
   {
      if (0.0 >= m_Entry.m_pImpl->m_HaunchCriteria.HaunchLoadCamberFactor)
      {
         ::AfxMessageBox(_T("Haunch load camber factor must be greater than zero. If you want zero camber, select the flat girder (zero camber) option"));
         pDX->Fail();
      }
      else if (1.0 < m_Entry.m_pImpl->m_HaunchCriteria.HaunchLoadCamberFactor)
      {
         ::AfxMessageBox(_T("Haunch load camber factor must be less than or equal to 100%"));
         pDX->Fail();
      }
   }   

   // computation of haunch composite section properties
   DDX_CBEnum(pDX, IDC_HAUNCH_COMP_PROPS_CB,m_Entry.m_pImpl->m_HaunchCriteria.HaunchAnalysisSectionPropertiesType);
}

void CSpecMainSheet::ExchangeLossData(CDataExchange* pDX)
{
   CEAFApp* pApp = EAFGetApp();
   const WBFL::Units::IndirectMeasure* pDisplayUnits = pApp->GetDisplayUnits();

   DDX_UnitValueAndTag(pDX, IDC_SHIPPING_TIME, IDC_SHIPPING_TIME_TAG, m_Entry.m_pImpl->m_PrestressLossCriteria.ShippingTime, pDisplayUnits->Time2);

   DDX_Percentage(pDX,IDC_EG_SLAB,m_Entry.m_pImpl->m_PrestressLossCriteria.SlabElasticGain);
   DDX_Percentage(pDX,IDC_EG_SLABPAD,m_Entry.m_pImpl->m_PrestressLossCriteria.SlabPadElasticGain);
   DDX_Percentage(pDX,IDC_EG_DIAPHRAGM,m_Entry.m_pImpl->m_PrestressLossCriteria.DiaphragmElasticGain);
   DDX_Percentage(pDX,IDC_EG_DC_BS2,m_Entry.m_pImpl->m_PrestressLossCriteria.UserDCElasticGain_BeforeDeckPlacement);
   DDX_Percentage(pDX,IDC_EG_DW_BS2,m_Entry.m_pImpl->m_PrestressLossCriteria.UserDWElasticGain_BeforeDeckPlacement);
   DDX_Percentage(pDX,IDC_EG_DC_BS3,m_Entry.m_pImpl->m_PrestressLossCriteria.UserDCElasticGain_AfterDeckPlacement);
   DDX_Percentage(pDX,IDC_EG_DW_BS3,m_Entry.m_pImpl->m_PrestressLossCriteria.UserDWElasticGain_AfterDeckPlacement);
   DDX_Percentage(pDX,IDC_EG_RAILING,m_Entry.m_pImpl->m_PrestressLossCriteria.RailingSystemElasticGain);
   DDX_Percentage(pDX,IDC_EG_OVERLAY,m_Entry.m_pImpl->m_PrestressLossCriteria.OverlayElasticGain);
   DDX_Percentage(pDX,IDC_EG_SHRINKAGE,m_Entry.m_pImpl->m_PrestressLossCriteria.SlabShrinkageElasticGain);
   DDX_Percentage(pDX,IDC_EG_LIVELOAD,m_Entry.m_pImpl->m_PrestressLossCriteria.LiveLoadElasticGain);

   DDX_CBEnum(pDX,IDC_RELAXATION_LOSS_METHOD,m_Entry.m_pImpl->m_PrestressLossCriteria.RelaxationLossMethod);
   DDX_CBEnum(pDX,IDC_FCPG_COMBO,m_Entry.m_pImpl->m_PrestressLossCriteria.FcgpComputationMethod);

   // have to map loss method to comb box ordering in dialog
   PrestressLossCriteria::LossMethodType map[] ={
      PrestressLossCriteria::LossMethodType::AASHTO_REFINED,
      PrestressLossCriteria::LossMethodType::WSDOT_REFINED,
      PrestressLossCriteria::LossMethodType::TXDOT_REFINED_2004,
      PrestressLossCriteria::LossMethodType::TXDOT_REFINED_2013,
      PrestressLossCriteria::LossMethodType::AASHTO_LUMPSUM,
      PrestressLossCriteria::LossMethodType::WSDOT_LUMPSUM,
      PrestressLossCriteria::LossMethodType::TIME_STEP
   };

   int map_size = sizeof(map)/sizeof(int);

   DDX_CBItemData(pDX,IDC_TIME_DEPENDENT_MODEL,m_Entry.m_pImpl->m_PrestressLossCriteria.TimeDependentConcreteModel);

   if ( pDX->m_bSaveAndValidate )
   {
      // data is coming out of the dialog
      int rad_ord;
      DDX_CBIndex(pDX,IDC_LOSS_METHOD,rad_ord);
      ATLASSERT(0 <= rad_ord && rad_ord < map_size);
      m_Entry.m_pImpl->m_PrestressLossCriteria.LossMethod = map[rad_ord];

      CComPtr<IBroker> pBroker;
      EAFGetBroker(&pBroker);
      if ( pBroker )
      {
         CComPtr<IDocumentType> pDocType;
         pBroker->GetInterface(IID_IDocumentType,(IUnknown**)&pDocType);
         if ( pDocType->IsPGSpliceDocument() && m_Entry.m_pImpl->m_PrestressLossCriteria.LossMethod != PrestressLossCriteria::LossMethodType::TIME_STEP && 0 <  m_Entry.GetRefCount() )
         {
            AfxMessageBox(_T("Time-step method must be selected for spliced girder bridges"));
            pDX->PrepareCtrl(IDC_LOSS_METHOD);
            pDX->Fail();
         }
      }

      if (m_Entry.m_pImpl->m_SectionPropertiesCriteria.SectionPropertyMode == pgsTypes::spmGross && m_Entry.m_pImpl->m_PrestressLossCriteria.LossMethod == PrestressLossCriteria::LossMethodType::TIME_STEP )
      {
         int result = AfxMessageBox(_T("Time-step method can only be used with transformed section properties.\n\nWould you like to change the section properties to transformed?"),MB_YESNO);
         if (result == IDYES )
         {
            m_Entry.m_pImpl->m_SectionPropertiesCriteria.SectionPropertyMode = pgsTypes::spmTransformed;
         }
         else
         {
            pDX->PrepareCtrl(IDC_LOSS_METHOD);
            pDX->Fail();
         }
      }

      if ( m_Entry.m_pImpl->m_PrestressLossCriteria.LossMethod == PrestressLossCriteria::LossMethodType::AASHTO_REFINED || m_Entry.m_pImpl->m_PrestressLossCriteria.LossMethod == PrestressLossCriteria::LossMethodType::WSDOT_REFINED ||
         m_Entry.m_pImpl->m_PrestressLossCriteria.LossMethod == PrestressLossCriteria::LossMethodType::AASHTO_LUMPSUM || m_Entry.m_pImpl->m_PrestressLossCriteria.LossMethod == PrestressLossCriteria::LossMethodType::WSDOT_LUMPSUM ||
         m_Entry.m_pImpl->m_PrestressLossCriteria.LossMethod == PrestressLossCriteria::LossMethodType::TXDOT_REFINED_2004 || m_Entry.m_pImpl->m_PrestressLossCriteria.LossMethod == PrestressLossCriteria::LossMethodType::TXDOT_REFINED_2013)
      {
         DDX_CBIndex(pDX,IDC_SHIPPING_LOSS_METHOD,rad_ord);
         if( rad_ord == 0 )
         {
            // Lump sum shipping loss
      	   DDX_UnitValueAndTag(pDX, IDC_SHIPPING, IDC_SHIPPING_TAG, m_Entry.m_pImpl->m_PrestressLossCriteria.ShippingLosses, pDisplayUnits->Stress);
         }
         else
         {
            // Fractional
            Float64 value;
            DDX_Text(pDX,IDC_SHIPPING,value);
            value /= -100.0;
            m_Entry.m_pImpl->m_PrestressLossCriteria.ShippingLosses = value;
         }
      }
      else
      {
         m_Entry.m_pImpl->m_PrestressLossCriteria.ShippingLosses = 0;
      }
   }
   else
   {
      // Data is going into the dialog
      // check buttons - map to loss method
      int idx=0;
      while(m_Entry.m_pImpl->m_PrestressLossCriteria.LossMethod != map[idx])
      {
         ATLASSERT(idx<map_size);
         if (map_size < idx)
         {
            idx = 0;
            break;
         }
         idx++;
      }

      DDX_CBIndex(pDX,IDC_LOSS_METHOD,idx);

      Float64 dummy = 0;
      if ( m_Entry.m_pImpl->m_PrestressLossCriteria.ShippingLosses < 0 )
      {
         /// Shipping losses are fractional
         Float64 value = m_Entry.m_pImpl->m_PrestressLossCriteria.ShippingLosses * -100.0;
         DDX_Text(pDX,IDC_SHIPPING,value);

         CString strTag(_T("%"));
         DDX_Text(pDX,IDC_SHIPPING_TAG,strTag);

         idx = 1;
         DDX_CBIndex(pDX,IDC_SHIPPING_LOSS_METHOD,idx);
      }
      else
      {
   	   DDX_UnitValueAndTag(pDX, IDC_SHIPPING,  IDC_SHIPPING_TAG,  m_Entry.m_pImpl->m_PrestressLossCriteria.ShippingLosses, pDisplayUnits->Stress);

         idx = 0;
         DDX_CBIndex(pDX,IDC_SHIPPING_LOSS_METHOD,idx);
      }
   }
}

void CSpecMainSheet::ExchangeStrandData(CDataExchange* pDX)
{
   // Strand Data
   DDX_Check_Bool(pDX, IDC_CHECK_PS_AT_JACKING, m_Entry.m_pImpl->m_StrandStressCriteria.bCheckStrandStress[+StrandStressCriteria::CheckStage::AtJacking]);
 	DDX_Text(pDX, IDC_PS_AT_JACKING_SR, m_Entry.m_pImpl->m_StrandStressCriteria.StrandStressCoeff[+StrandStressCriteria::CheckStage::AtJacking][+StrandStressCriteria::StrandType::StressRelieved]);
   DDV_GreaterThanZero(pDX, IDC_PS_AT_JACKING_SR, m_Entry.m_pImpl->m_StrandStressCriteria.StrandStressCoeff[+StrandStressCriteria::CheckStage::AtJacking][+StrandStressCriteria::StrandType::StressRelieved]);
 	DDX_Text(pDX, IDC_PS_AT_JACKING_LR, m_Entry.m_pImpl->m_StrandStressCriteria.StrandStressCoeff[+StrandStressCriteria::CheckStage::AtJacking][+StrandStressCriteria::StrandType::LowRelaxation]);
   DDV_GreaterThanZero(pDX, IDC_PS_AT_JACKING_LR, m_Entry.m_pImpl->m_StrandStressCriteria.StrandStressCoeff[+StrandStressCriteria::CheckStage::AtJacking][+StrandStressCriteria::StrandType::LowRelaxation]);

   DDX_Check_Bool(pDX, IDC_CHECK_PS_BEFORE_TRANSFER, m_Entry.m_pImpl->m_StrandStressCriteria.bCheckStrandStress[+StrandStressCriteria::CheckStage::BeforeTransfer]);
 	DDX_Text(pDX, IDC_PS_BEFORE_TRANSFER_SR, m_Entry.m_pImpl->m_StrandStressCriteria.StrandStressCoeff[+StrandStressCriteria::CheckStage::BeforeTransfer][+StrandStressCriteria::StrandType::StressRelieved]);
   DDV_GreaterThanZero(pDX, IDC_PS_BEFORE_TRANSFER_SR, m_Entry.m_pImpl->m_StrandStressCriteria.StrandStressCoeff[+StrandStressCriteria::CheckStage::BeforeTransfer][+StrandStressCriteria::StrandType::StressRelieved]);
 	DDX_Text(pDX, IDC_PS_BEFORE_TRANSFER_LR, m_Entry.m_pImpl->m_StrandStressCriteria.StrandStressCoeff[+StrandStressCriteria::CheckStage::BeforeTransfer][+StrandStressCriteria::StrandType::LowRelaxation]);
   DDV_GreaterThanZero(pDX, IDC_PS_BEFORE_TRANSFER_LR, m_Entry.m_pImpl->m_StrandStressCriteria.StrandStressCoeff[+StrandStressCriteria::CheckStage::BeforeTransfer][+StrandStressCriteria::StrandType::LowRelaxation]);

   DDX_Check_Bool(pDX, IDC_CHECK_PS_AFTER_TRANSFER, m_Entry.m_pImpl->m_StrandStressCriteria.bCheckStrandStress[+StrandStressCriteria::CheckStage::AfterTransfer]);
 	DDX_Text(pDX, IDC_PS_AFTER_TRANSFER_SR, m_Entry.m_pImpl->m_StrandStressCriteria.StrandStressCoeff[+StrandStressCriteria::CheckStage::AfterTransfer][+StrandStressCriteria::StrandType::StressRelieved]);
   DDV_GreaterThanZero(pDX, IDC_PS_AFTER_TRANSFER_SR, m_Entry.m_pImpl->m_StrandStressCriteria.StrandStressCoeff[+StrandStressCriteria::CheckStage::AfterTransfer][+StrandStressCriteria::StrandType::StressRelieved]);
 	DDX_Text(pDX, IDC_PS_AFTER_TRANSFER_LR, m_Entry.m_pImpl->m_StrandStressCriteria.StrandStressCoeff[+StrandStressCriteria::CheckStage::AfterTransfer][+StrandStressCriteria::StrandType::LowRelaxation]);
   DDV_GreaterThanZero(pDX, IDC_PS_AFTER_TRANSFER_LR, m_Entry.m_pImpl->m_StrandStressCriteria.StrandStressCoeff[+StrandStressCriteria::CheckStage::AfterTransfer][+StrandStressCriteria::StrandType::LowRelaxation]);

   m_Entry.m_pImpl->m_StrandStressCriteria.bCheckStrandStress[+StrandStressCriteria::CheckStage::AfterAllLosses] = true;
 	DDX_Text(pDX, IDC_PS_AFTER_ALL_LOSSES_SR, m_Entry.m_pImpl->m_StrandStressCriteria.StrandStressCoeff[+StrandStressCriteria::CheckStage::AfterAllLosses][+StrandStressCriteria::StrandType::StressRelieved]);
   DDV_GreaterThanZero(pDX, IDC_PS_AFTER_ALL_LOSSES_SR, m_Entry.m_pImpl->m_StrandStressCriteria.StrandStressCoeff[+StrandStressCriteria::CheckStage::AfterAllLosses][+StrandStressCriteria::StrandType::StressRelieved]);
 	DDX_Text(pDX, IDC_PS_AFTER_ALL_LOSSES_LR, m_Entry.m_pImpl->m_StrandStressCriteria.StrandStressCoeff[+StrandStressCriteria::CheckStage::AfterAllLosses][+StrandStressCriteria::StrandType::LowRelaxation]);
   DDV_GreaterThanZero(pDX, IDC_PS_AFTER_ALL_LOSSES_LR, m_Entry.m_pImpl->m_StrandStressCriteria.StrandStressCoeff[+StrandStressCriteria::CheckStage::AfterAllLosses][+StrandStressCriteria::StrandType::LowRelaxation]);

   // Tendon Data
   DDX_Check_Bool(pDX, IDC_CHECK_PT_AT_JACKING, m_Entry.m_pImpl->m_TendonStressCriteria.bCheckAtJacking);
   DDX_Text(pDX,IDC_PT_AT_JACKING_SR,m_Entry.m_pImpl->m_TendonStressCriteria.TendonStressCoeff[+TendonStressCriteria::CheckStage::AtJacking][+TendonStressCriteria::StrandType::StressRelieved]);
   DDV_GreaterThanZero(pDX, IDC_PT_AT_JACKING_SR, m_Entry.m_pImpl->m_TendonStressCriteria.TendonStressCoeff[+TendonStressCriteria::CheckStage::AtJacking][+TendonStressCriteria::StrandType::StressRelieved]);
   DDX_Text(pDX,IDC_PT_AT_JACKING_LR,m_Entry.m_pImpl->m_TendonStressCriteria.TendonStressCoeff[+TendonStressCriteria::CheckStage::AtJacking][+TendonStressCriteria::StrandType::LowRelaxation]);
   DDV_GreaterThanZero(pDX, IDC_PT_AT_JACKING_LR, m_Entry.m_pImpl->m_TendonStressCriteria.TendonStressCoeff[+TendonStressCriteria::CheckStage::AtJacking][+TendonStressCriteria::StrandType::LowRelaxation]);

   DDX_Check_Bool(pDX, IDC_CHECK_PT_BEFORE_TRANSFER, m_Entry.m_pImpl->m_TendonStressCriteria.bCheckPriorToSeating);
   DDX_Text(pDX,IDC_PT_BEFORE_TRANSFER_SR,m_Entry.m_pImpl->m_TendonStressCriteria.TendonStressCoeff[+TendonStressCriteria::CheckStage::PriorToSeating][+TendonStressCriteria::StrandType::StressRelieved]);
   DDV_GreaterThanZero(pDX, IDC_PT_BEFORE_TRANSFER_SR, m_Entry.m_pImpl->m_TendonStressCriteria.TendonStressCoeff[+TendonStressCriteria::CheckStage::PriorToSeating][+TendonStressCriteria::StrandType::StressRelieved]);
   DDX_Text(pDX,IDC_PT_BEFORE_TRANSFER_LR,m_Entry.m_pImpl->m_TendonStressCriteria.TendonStressCoeff[+TendonStressCriteria::CheckStage::PriorToSeating][+TendonStressCriteria::StrandType::LowRelaxation]);
   DDV_GreaterThanZero(pDX, IDC_PT_BEFORE_TRANSFER_LR, m_Entry.m_pImpl->m_TendonStressCriteria.TendonStressCoeff[+TendonStressCriteria::CheckStage::PriorToSeating][+TendonStressCriteria::StrandType::LowRelaxation]);

   DDX_Text(pDX,IDC_PT_ANCHORAGE_AFTER_ANCHORSET_SR,m_Entry.m_pImpl->m_TendonStressCriteria.TendonStressCoeff[+TendonStressCriteria::CheckStage::AtAnchoragesAfterSeating][+TendonStressCriteria::StrandType::StressRelieved]);
   DDV_GreaterThanZero(pDX, IDC_PT_ANCHORAGE_AFTER_ANCHORSET_SR, m_Entry.m_pImpl->m_TendonStressCriteria.TendonStressCoeff[+TendonStressCriteria::CheckStage::AtAnchoragesAfterSeating][+TendonStressCriteria::StrandType::StressRelieved]);
   DDX_Text(pDX,IDC_PT_ANCHORAGE_AFTER_ANCHORSET_LR,m_Entry.m_pImpl->m_TendonStressCriteria.TendonStressCoeff[+TendonStressCriteria::CheckStage::AtAnchoragesAfterSeating][+TendonStressCriteria::StrandType::LowRelaxation]);
   DDV_GreaterThanZero(pDX, IDC_PT_ANCHORAGE_AFTER_ANCHORSET_LR, m_Entry.m_pImpl->m_TendonStressCriteria.TendonStressCoeff[+TendonStressCriteria::CheckStage::AtAnchoragesAfterSeating][+TendonStressCriteria::StrandType::LowRelaxation]);

   DDX_Text(pDX,IDC_PT_ELSEWHERE_AFTER_ANCHORSET_SR,m_Entry.m_pImpl->m_TendonStressCriteria.TendonStressCoeff[+TendonStressCriteria::CheckStage::ElsewhereAfterSeating][+TendonStressCriteria::StrandType::StressRelieved]);
   DDV_GreaterThanZero(pDX, IDC_PT_ELSEWHERE_AFTER_ANCHORSET_SR, m_Entry.m_pImpl->m_TendonStressCriteria.TendonStressCoeff[+TendonStressCriteria::CheckStage::ElsewhereAfterSeating][+TendonStressCriteria::StrandType::StressRelieved]);
   DDX_Text(pDX,IDC_PT_ELSEWHERE_AFTER_ANCHORSET_LR,m_Entry.m_pImpl->m_TendonStressCriteria.TendonStressCoeff[+TendonStressCriteria::CheckStage::ElsewhereAfterSeating][+TendonStressCriteria::StrandType::LowRelaxation]);
   DDV_GreaterThanZero(pDX, IDC_PT_ELSEWHERE_AFTER_ANCHORSET_LR, m_Entry.m_pImpl->m_TendonStressCriteria.TendonStressCoeff[+TendonStressCriteria::CheckStage::ElsewhereAfterSeating][+TendonStressCriteria::StrandType::LowRelaxation]);

   DDX_Text(pDX,IDC_PT_AFTER_ALL_LOSSES_SR,m_Entry.m_pImpl->m_TendonStressCriteria.TendonStressCoeff[+TendonStressCriteria::CheckStage::AfterAllLosses][+TendonStressCriteria::StrandType::StressRelieved]);
   DDV_GreaterThanZero(pDX, IDC_PT_AFTER_ALL_LOSSES_SR, m_Entry.m_pImpl->m_TendonStressCriteria.TendonStressCoeff[+TendonStressCriteria::CheckStage::AfterAllLosses][+TendonStressCriteria::StrandType::StressRelieved]);
   DDX_Text(pDX,IDC_PT_AFTER_ALL_LOSSES_LR,m_Entry.m_pImpl->m_TendonStressCriteria.TendonStressCoeff[+TendonStressCriteria::CheckStage::AfterAllLosses][+TendonStressCriteria::StrandType::LowRelaxation]);
   DDV_GreaterThanZero(pDX, IDC_PT_AFTER_ALL_LOSSES_LR, m_Entry.m_pImpl->m_TendonStressCriteria.TendonStressCoeff[+TendonStressCriteria::CheckStage::AfterAllLosses][+TendonStressCriteria::StrandType::LowRelaxation]);

   // Pretension Strand Options
   DDX_CBEnum(pDX,IDC_COMBO_TRANSFER, m_Entry.m_pImpl->m_TransferLengthCriteria.CalculationMethod);

   // Duct Size
   DDX_Text(pDX,IDC_PUSH_METHOD, m_Entry.m_pImpl->m_DuctSizeCriteria.DuctAreaPushRatio);
   DDX_Text(pDX,IDC_PULL_METHOD, m_Entry.m_pImpl->m_DuctSizeCriteria.DuctAreaPullRatio);
   DDX_Text(pDX,IDC_DUCT_SIZE_RATIO, m_Entry.m_pImpl->m_DuctSizeCriteria.DuctDiameterRatio);

   if (pDX->m_bSaveAndValidate)
   {
      if (WBFL::LRFD::BDSManager::Edition::NinthEdition2020 <= m_Entry.m_pImpl->m_SpecificationCriteria.GetEdition())
      {
         if (0.70 < m_Entry.m_pImpl->m_DuctSizeCriteria.DuctDiameterRatio)
         {
            // if the ratio is more than 0.7, lambda_duct becomes negative
            pDX->PrepareEditCtrl(IDC_DUCT_SIZE_RATIO);
            AfxMessageBox(_T("The ratio of the outside duct diameter to the least gross concrete thickness at the duct cannot exceed 0.7"), MB_ICONEXCLAMATION | MB_OK);
            pDX->Fail();
         }
      }
      else
      {
         if ( !(m_Entry.m_pImpl->m_DuctSizeCriteria.DuctDiameterRatio < 1.0) )
         {
            // duct diameter can't be greater than web
            pDX->PrepareEditCtrl(IDC_DUCT_SIZE_RATIO);
            AfxMessageBox(_T("The ratio of the outside duct diameter to the least gross concrete thickness at the duct must be less than 1.0"), MB_ICONEXCLAMATION | MB_OK);
            pDX->Fail();
         }
      }
   }
}

void CSpecMainSheet::ExchangeLimitsData(CDataExchange* pDX)
{
   CEAFApp* pApp = EAFGetApp();
   const WBFL::Units::IndirectMeasure* pDisplayUnits = pApp->GetDisplayUnits();

   DDX_UnitValueAndTag(pDX, IDC_NWC_FC_SLAB,     IDC_NWC_FC_SLAB_UNIT,     m_Entry.m_pImpl->m_LimitsCriteria.MaxSlabFc[pgsTypes::Normal],             pDisplayUnits->Stress);
   DDX_UnitValueAndTag(pDX, IDC_NWC_GIRDER_FCI,  IDC_NWC_GIRDER_FCI_UNIT,  m_Entry.m_pImpl->m_LimitsCriteria.MaxSegmentFci[pgsTypes::Normal],          pDisplayUnits->Stress);
   DDX_UnitValueAndTag(pDX, IDC_NWC_GIRDER_FC,   IDC_NWC_GIRDER_FC_UNIT,   m_Entry.m_pImpl->m_LimitsCriteria.MaxSegmentFc[pgsTypes::Normal],           pDisplayUnits->Stress);
   DDX_UnitValueAndTag(pDX, IDC_NWC_CLOSURE_FCI, IDC_NWC_CLOSURE_FCI_UNIT, m_Entry.m_pImpl->m_LimitsCriteria.MaxClosureFci[pgsTypes::Normal],          pDisplayUnits->Stress);
   DDX_UnitValueAndTag(pDX, IDC_NWC_CLOSURE_FC,  IDC_NWC_CLOSURE_FC_UNIT,  m_Entry.m_pImpl->m_LimitsCriteria.MaxClosureFc[pgsTypes::Normal],           pDisplayUnits->Stress);
   DDX_UnitValueAndTag(pDX, IDC_NWC_UNIT_WEIGHT, IDC_NWC_UNIT_WEIGHT_UNIT, m_Entry.m_pImpl->m_LimitsCriteria.MaxConcreteUnitWeight[pgsTypes::Normal], pDisplayUnits->Density);
   DDX_UnitValueAndTag(pDX, IDC_NWC_AGG_SIZE,    IDC_NWC_AGG_SIZE_UNIT,    m_Entry.m_pImpl->m_LimitsCriteria.MaxConcreteAggSize[pgsTypes::Normal],    pDisplayUnits->ComponentDim);

   DDX_UnitValueAndTag(pDX, IDC_LWC_FC_SLAB,     IDC_LWC_FC_SLAB_UNIT,     m_Entry.m_pImpl->m_LimitsCriteria.MaxSlabFc[pgsTypes::SandLightweight],             pDisplayUnits->Stress);
   DDX_UnitValueAndTag(pDX, IDC_LWC_GIRDER_FCI,  IDC_LWC_GIRDER_FCI_UNIT,  m_Entry.m_pImpl->m_LimitsCriteria.MaxSegmentFci[pgsTypes::SandLightweight],          pDisplayUnits->Stress);
   DDX_UnitValueAndTag(pDX, IDC_LWC_GIRDER_FC,   IDC_LWC_GIRDER_FC_UNIT,   m_Entry.m_pImpl->m_LimitsCriteria.MaxSegmentFc[pgsTypes::SandLightweight],           pDisplayUnits->Stress);
   DDX_UnitValueAndTag(pDX, IDC_LWC_CLOSURE_FCI, IDC_LWC_CLOSURE_FCI_UNIT, m_Entry.m_pImpl->m_LimitsCriteria.MaxClosureFci[pgsTypes::SandLightweight],          pDisplayUnits->Stress);
   DDX_UnitValueAndTag(pDX, IDC_LWC_CLOSURE_FC,  IDC_LWC_CLOSURE_FC_UNIT,  m_Entry.m_pImpl->m_LimitsCriteria.MaxClosureFc[pgsTypes::SandLightweight],           pDisplayUnits->Stress);
   DDX_UnitValueAndTag(pDX, IDC_LWC_UNIT_WEIGHT, IDC_LWC_UNIT_WEIGHT_UNIT, m_Entry.m_pImpl->m_LimitsCriteria.MaxConcreteUnitWeight[pgsTypes::SandLightweight], pDisplayUnits->Density);
   DDX_UnitValueAndTag(pDX, IDC_LWC_AGG_SIZE,    IDC_LWC_AGG_SIZE_UNIT,    m_Entry.m_pImpl->m_LimitsCriteria.MaxConcreteAggSize[pgsTypes::SandLightweight],    pDisplayUnits->ComponentDim);

   DDX_UnitValueAndTag(pDX, IDC_LWC_FC_SLAB,     IDC_LWC_FC_SLAB_UNIT,     m_Entry.m_pImpl->m_LimitsCriteria.MaxSlabFc[pgsTypes::AllLightweight],             pDisplayUnits->Stress);
   DDX_UnitValueAndTag(pDX, IDC_LWC_GIRDER_FCI,  IDC_LWC_GIRDER_FCI_UNIT,  m_Entry.m_pImpl->m_LimitsCriteria.MaxSegmentFci[pgsTypes::AllLightweight],          pDisplayUnits->Stress);
   DDX_UnitValueAndTag(pDX, IDC_LWC_GIRDER_FC,   IDC_LWC_GIRDER_FC_UNIT,   m_Entry.m_pImpl->m_LimitsCriteria.MaxSegmentFc[pgsTypes::AllLightweight],           pDisplayUnits->Stress);
   DDX_UnitValueAndTag(pDX, IDC_LWC_CLOSURE_FCI, IDC_LWC_CLOSURE_FCI_UNIT, m_Entry.m_pImpl->m_LimitsCriteria.MaxClosureFci[pgsTypes::AllLightweight],          pDisplayUnits->Stress);
   DDX_UnitValueAndTag(pDX, IDC_LWC_CLOSURE_FC,  IDC_LWC_CLOSURE_FC_UNIT,  m_Entry.m_pImpl->m_LimitsCriteria.MaxClosureFc[pgsTypes::AllLightweight],           pDisplayUnits->Stress);
   DDX_UnitValueAndTag(pDX, IDC_LWC_UNIT_WEIGHT, IDC_LWC_UNIT_WEIGHT_UNIT, m_Entry.m_pImpl->m_LimitsCriteria.MaxConcreteUnitWeight[pgsTypes::AllLightweight], pDisplayUnits->Density);
   DDX_UnitValueAndTag(pDX, IDC_LWC_AGG_SIZE,    IDC_LWC_AGG_SIZE_UNIT,    m_Entry.m_pImpl->m_LimitsCriteria.MaxConcreteAggSize[pgsTypes::AllLightweight],    pDisplayUnits->ComponentDim);

   // UHPC doesn't really have well established normal "maximum" values yet.
   //DDX_UnitValueAndTag(pDX, IDC_UHPC_FC_SLAB, IDC_UHPC_FC_SLAB_UNIT, m_Entry.m_pImpl->m_MaxSlabFc[pgsTypes::PCI_UHPC], pDisplayUnits->Stress);
   //DDX_UnitValueAndTag(pDX, IDC_UHPC_GIRDER_FCI, IDC_UHPC_GIRDER_FCI_UNIT, m_Entry.m_pImpl->m_MaxSegmentFci[pgsTypes::PCI_UHPC], pDisplayUnits->Stress);
   //DDX_UnitValueAndTag(pDX, IDC_UHPC_GIRDER_FC, IDC_UHPC_GIRDER_FC_UNIT, m_Entry.m_pImpl->m_MaxSegmentFc[pgsTypes::PCI_UHPC], pDisplayUnits->Stress);
   //DDX_UnitValueAndTag(pDX, IDC_UHPC_CLOSURE_FCI, IDC_UHPC_CLOSURE_FCI_UNIT, m_Entry.m_pImpl->m_MaxClosureFci[pgsTypes::PCI_UHPC], pDisplayUnits->Stress);
   //DDX_UnitValueAndTag(pDX, IDC_UHPC_CLOSURE_FC, IDC_UHPC_CLOSURE_FC_UNIT, m_Entry.m_pImpl->m_MaxClosureFc[pgsTypes::PCI_UHPC], pDisplayUnits->Stress);
   //DDX_UnitValueAndTag(pDX, IDC_UHPC_UNIT_WEIGHT, IDC_UHPC_UNIT_WEIGHT_UNIT, m_Entry.m_pImpl->m_MaxConcreteUnitWeight[pgsTypes::PCI_UHPC], pDisplayUnits->Density);
   //DDX_UnitValueAndTag(pDX, IDC_UHPC_AGG_SIZE, IDC_UHPC_AGG_SIZE_UNIT, m_Entry.m_pImpl->m_MaxConcreteAggSize[pgsTypes::PCI_UHPC], pDisplayUnits->ComponentDim);

   //if (pDX->m_bSaveAndValidate)
   //{
   //   // these share the same UI so make them identical
   //   m_Entry.m_pImpl->m_MaxSlabFc[pgsTypes::UHPC]  = m_Entry.m_pImpl->m_MaxSlabFc[pgsTypes::PCI_UHPC];
   //   m_Entry.m_pImpl->m_MaxSegmentFci[pgsTypes::UHPC] = m_Entry.m_pImpl->m_MaxSegmentFci[pgsTypes::PCI_UHPC];
   //   m_Entry.m_pImpl->m_MaxSegmentFc[pgsTypes::UHPC] = m_Entry.m_pImpl->m_MaxSegmentFc[pgsTypes::PCI_UHPC];
   //   m_Entry.m_pImpl->m_MaxClosureFci[pgsTypes::UHPC] = m_Entry.m_pImpl->m_MaxClosureFci[pgsTypes::PCI_UHPC];
   //   m_Entry.m_pImpl->m_MaxClosureFc[pgsTypes::UHPC] = m_Entry.m_pImpl->m_MaxClosureFc[pgsTypes::PCI_UHPC];
   //   m_Entry.m_pImpl->m_MaxConcreteUnitWeight[pgsTypes::UHPC] = m_Entry.m_pImpl->m_MaxConcreteUnitWeight[pgsTypes::PCI_UHPC];
   //   m_Entry.m_pImpl->m_MaxConcreteAggSize[pgsTypes::UHPC] = m_Entry.m_pImpl->m_MaxConcreteAggSize[pgsTypes::PCI_UHPC];
   //}

   DDX_Check_Bool(pDX, IDC_CHECK_STIRRUP_COMPATIBILITY, m_Entry.m_pImpl->m_LimitsCriteria.bCheckStirrupSpacingCompatibility);
   DDX_Check_Bool(pDX, IDC_CHECK_GIRDER_SAG, m_Entry.m_pImpl->m_LimitsCriteria.bCheckSag);
   DDX_CBEnum(pDX, IDC_SAG_OPTIONS, m_Entry.m_pImpl->m_LimitsCriteria.SagCamber);
}

void CSpecMainSheet::ExchangeClosureData(CDataExchange* pDX)
{
   CEAFApp* pApp = EAFGetApp();
   const WBFL::Units::IndirectMeasure* pDisplayUnits = pApp->GetDisplayUnits();

   CString tagBeforeLosses = (pApp->GetUnitsMode() == eafTypes::umSI ? _T("sqrt(f'ci (MPa))") : _T("sqrt(f'ci (KSI))"));
   CString tagAfterLosses  = (pApp->GetUnitsMode() == eafTypes::umSI ? _T("sqrt(f'c (MPa))") : _T("sqrt(f'c (KSI))"));

   if ( WBFL::LRFD::BDSManager::Edition::SeventhEditionWith2016Interims <= WBFL::LRFD::BDSManager::GetEdition() )
   {
      tagBeforeLosses = _T("(lambda)") + tagBeforeLosses;
      tagAfterLosses  = _T("(lambda)") + tagAfterLosses;
   }

   DDX_Text(pDX,IDC_RELEASE_COMPRESSION,m_Entry.m_pImpl->m_ClosureJointCriteria.CompressionStressCoefficient_BeforeLosses);
   DDV_GreaterThanZero(pDX, IDC_RELEASE_COMPRESSION,m_Entry.m_pImpl->m_ClosureJointCriteria.CompressionStressCoefficient_BeforeLosses);

   DDX_UnitValueAndTag(pDX, IDC_RELEASE_PTZ_TENSION, IDC_RELEASE_PTZ_TENSION_UNIT, m_Entry.m_pImpl->m_ClosureJointCriteria.TensionStressLimit_InPTZ_WithoutReinforcement_BeforeLosses.Coefficient, pDisplayUnits->SqrtPressure );
   DDX_Text(pDX,IDC_RELEASE_PTZ_TENSION_UNIT,tagBeforeLosses);
   DDV_UnitValueZeroOrMore(pDX, IDC_RELEASE_PTZ_TENSION_UNIT,m_Entry.m_pImpl->m_ClosureJointCriteria.TensionStressLimit_InPTZ_WithoutReinforcement_BeforeLosses.Coefficient, pDisplayUnits->SqrtPressure);

   DDX_UnitValueAndTag(pDX, IDC_RELEASE_PTZ_TENSION_WITH_REBAR, IDC_RELEASE_PTZ_TENSION_WITH_REBAR_UNIT, m_Entry.m_pImpl->m_ClosureJointCriteria.TensionStressLimit_InPTZ_WithReinforcement_BeforeLosses.Coefficient, pDisplayUnits->SqrtPressure );
   DDX_Text(pDX,IDC_RELEASE_PTZ_TENSION_WITH_REBAR_UNIT,tagBeforeLosses);
   DDV_UnitValueZeroOrMore(pDX, IDC_RELEASE_PTZ_TENSION_WITH_REBAR_UNIT,m_Entry.m_pImpl->m_ClosureJointCriteria.TensionStressLimit_InPTZ_WithReinforcement_BeforeLosses.Coefficient, pDisplayUnits->SqrtPressure);

   DDX_UnitValueAndTag(pDX, IDC_RELEASE_TENSION, IDC_RELEASE_TENSION_UNIT, m_Entry.m_pImpl->m_ClosureJointCriteria.TensionStressLimit_OtherAreas_WithoutReinforcement_BeforeLosses.Coefficient, pDisplayUnits->SqrtPressure );
   DDX_Text(pDX,IDC_RELEASE_TENSION_UNIT,tagBeforeLosses);
   DDV_UnitValueZeroOrMore(pDX, IDC_RELEASE_TENSION_UNIT,m_Entry.m_pImpl->m_ClosureJointCriteria.TensionStressLimit_OtherAreas_WithoutReinforcement_BeforeLosses.Coefficient, pDisplayUnits->SqrtPressure);

   DDX_UnitValueAndTag(pDX, IDC_RELEASE_TENSION_WITH_REBAR, IDC_RELEASE_TENSION_WITH_REBAR_UNIT, m_Entry.m_pImpl->m_ClosureJointCriteria.TensionStressLimit_OtherAreas_WithReinforcement_BeforeLosses.Coefficient, pDisplayUnits->SqrtPressure );
   DDX_Text(pDX,IDC_RELEASE_TENSION_WITH_REBAR_UNIT,tagBeforeLosses);
   DDV_UnitValueZeroOrMore(pDX, IDC_RELEASE_TENSION_WITH_REBAR_UNIT,m_Entry.m_pImpl->m_ClosureJointCriteria.TensionStressLimit_OtherAreas_WithReinforcement_BeforeLosses.Coefficient, pDisplayUnits->SqrtPressure);

   DDX_Text(pDX,IDC_SERVICE_COMPRESSION,m_Entry.m_pImpl->m_ClosureJointCriteria.CompressionStressCoefficient_PermanentLoadsOnly_AfterLosses);
   DDV_GreaterThanZero(pDX, IDC_SERVICE_COMPRESSION,m_Entry.m_pImpl->m_ClosureJointCriteria.CompressionStressCoefficient_PermanentLoadsOnly_AfterLosses);

   DDX_Text(pDX,IDC_SERVICE_COMPRESSION_WITH_LIVELOAD,m_Entry.m_pImpl->m_ClosureJointCriteria.CompressionStressCoefficient_AllLoads_AfterLosses);
   DDV_GreaterThanZero(pDX, IDC_SERVICE_COMPRESSION_WITH_LIVELOAD,m_Entry.m_pImpl->m_ClosureJointCriteria.CompressionStressCoefficient_AllLoads_AfterLosses);

   // Activation of optional service I check after losses is dependent on setting in m_PrestressedElementCriteria. Synchronize here
   if (!pDX->m_bSaveAndValidate)
   {
      m_Entry.m_pImpl->m_ClosureJointCriteria.bCheckFinalServiceITension = m_Entry.m_pImpl->m_PrestressedElementCriteria.bCheckFinalServiceITension;
   }

   DDX_UnitValueAndTag(pDX,IDC_SERVICE_I_TENSION,IDC_SERVICE_I_TENSION_UNIT,m_Entry.m_pImpl->m_ClosureJointCriteria.TensionStressLimit_ServiceI_PermanentLoadsOnly_AfterLosses.Coefficient,pDisplayUnits->SqrtPressure);
   DDX_Text(pDX,IDC_SERVICE_I_TENSION_UNIT,tagAfterLosses);
   DDV_UnitValueZeroOrMore(pDX,IDC_SERVICE_I_TENSION,m_Entry.m_pImpl->m_ClosureJointCriteria.TensionStressLimit_ServiceI_PermanentLoadsOnly_AfterLosses.Coefficient,pDisplayUnits->SqrtPressure);
   DDX_Check_Bool(pDX,IDC_CHECK_SERVICE_I_TENSION_MAX,m_Entry.m_pImpl->m_ClosureJointCriteria.TensionStressLimit_ServiceI_PermanentLoadsOnly_AfterLosses.bHasMaxValue);
   DDX_UnitValueAndTag(pDX,IDC_SERVICE_I_TENSION_MAX,IDC_SERVICE_I_TENSION_MAX_UNIT,m_Entry.m_pImpl->m_ClosureJointCriteria.TensionStressLimit_ServiceI_PermanentLoadsOnly_AfterLosses.MaxValue,pDisplayUnits->Stress);
   if (m_Entry.m_pImpl->m_ClosureJointCriteria.TensionStressLimit_ServiceI_PermanentLoadsOnly_AfterLosses.bHasMaxValue)
   {
      DDV_UnitValueGreaterThanZero(pDX,IDC_SERVICE_I_TENSION_MAX,m_Entry.m_pImpl->m_ClosureJointCriteria.TensionStressLimit_ServiceI_PermanentLoadsOnly_AfterLosses.MaxValue,pDisplayUnits->Stress);
   }

   DDX_UnitValueAndTag(pDX, IDC_SERVICE_PTZ_TENSION, IDC_SERVICE_PTZ_TENSION_UNIT, m_Entry.m_pImpl->m_ClosureJointCriteria.TensionStressLimit_InPTZ_WithoutReinforcement_AfterLosses.Coefficient, pDisplayUnits->SqrtPressure );
   DDX_Text(pDX,IDC_SERVICE_PTZ_TENSION_UNIT,tagAfterLosses);
   DDV_UnitValueZeroOrMore(pDX, IDC_SERVICE_PTZ_TENSION_UNIT,m_Entry.m_pImpl->m_ClosureJointCriteria.TensionStressLimit_InPTZ_WithoutReinforcement_AfterLosses.Coefficient, pDisplayUnits->SqrtPressure);
   DDX_Check_Bool(pDX,IDC_CHECK_SERVICE_PTZ_TENSION_MAX,m_Entry.m_pImpl->m_ClosureJointCriteria.TensionStressLimit_InPTZ_WithoutReinforcement_AfterLosses.bHasMaxValue);
   DDX_UnitValueAndTag(pDX,IDC_SERVICE_PTZ_TENSION_MAX,IDC_SERVICE_PTZ_TENSION_MAX_UNIT,m_Entry.m_pImpl->m_ClosureJointCriteria.TensionStressLimit_InPTZ_WithoutReinforcement_AfterLosses.MaxValue,pDisplayUnits->Stress);
   if (m_Entry.m_pImpl->m_ClosureJointCriteria.TensionStressLimit_InPTZ_WithoutReinforcement_AfterLosses.bHasMaxValue)
   {
      DDV_UnitValueGreaterThanZero(pDX,IDC_SERVICE_PTZ_TENSION_MAX,m_Entry.m_pImpl->m_ClosureJointCriteria.TensionStressLimit_InPTZ_WithoutReinforcement_AfterLosses.MaxValue,pDisplayUnits->Stress);
   }

   DDX_UnitValueAndTag(pDX, IDC_SERVICE_PTZ_TENSION_WITH_REBAR, IDC_SERVICE_PTZ_TENSION_WITH_REBAR_UNIT, m_Entry.m_pImpl->m_ClosureJointCriteria.TensionStressLimit_InPTZ_WithReinforcement_AfterLosses.Coefficient, pDisplayUnits->SqrtPressure );
   DDX_Text(pDX,IDC_SERVICE_PTZ_TENSION_WITH_REBAR_UNIT,tagAfterLosses);
   DDV_UnitValueZeroOrMore(pDX, IDC_SERVICE_PTZ_TENSION_WITH_REBAR_UNIT,m_Entry.m_pImpl->m_ClosureJointCriteria.TensionStressLimit_InPTZ_WithReinforcement_AfterLosses.Coefficient, pDisplayUnits->SqrtPressure);
   DDX_Check_Bool(pDX,IDC_CHECK_SERVICE_PTZ_TENSION_WITH_REBAR_MAX,m_Entry.m_pImpl->m_ClosureJointCriteria.TensionStressLimit_InPTZ_WithReinforcement_AfterLosses.bHasMaxValue);
   DDX_UnitValueAndTag(pDX,IDC_SERVICE_PTZ_TENSION_WITH_REBAR_MAX,IDC_SERVICE_PTZ_TENSION_WITH_REBAR_MAX_UNIT,m_Entry.m_pImpl->m_ClosureJointCriteria.TensionStressLimit_InPTZ_WithReinforcement_AfterLosses.MaxValue,pDisplayUnits->Stress);
   if (m_Entry.m_pImpl->m_ClosureJointCriteria.TensionStressLimit_InPTZ_WithReinforcement_AfterLosses.bHasMaxValue)
   {
      DDV_UnitValueGreaterThanZero(pDX,IDC_SERVICE_PTZ_TENSION_WITH_REBAR_MAX,m_Entry.m_pImpl->m_ClosureJointCriteria.TensionStressLimit_InPTZ_WithReinforcement_AfterLosses.MaxValue,pDisplayUnits->Stress);
   }

   DDX_UnitValueAndTag(pDX, IDC_SERVICE_III_TENSION, IDC_SERVICE_III_TENSION_UNIT, m_Entry.m_pImpl->m_ClosureJointCriteria.TensionStressLimit_OtherAreas_WithoutReinforcement_AfterLosses.Coefficient, pDisplayUnits->SqrtPressure );
   DDX_Text(pDX,IDC_SERVICE_III_TENSION_UNIT,tagAfterLosses);
   DDV_UnitValueZeroOrMore(pDX, IDC_SERVICE_III_TENSION_UNIT,m_Entry.m_pImpl->m_ClosureJointCriteria.TensionStressLimit_OtherAreas_WithoutReinforcement_AfterLosses.Coefficient, pDisplayUnits->SqrtPressure);
   DDX_Check_Bool(pDX,IDC_CHECK_SERVICE_III_TENSION_MAX,m_Entry.m_pImpl->m_ClosureJointCriteria.TensionStressLimit_OtherAreas_WithoutReinforcement_AfterLosses.bHasMaxValue);
   DDX_UnitValueAndTag(pDX,IDC_SERVICE_III_TENSION_MAX,IDC_SERVICE_III_TENSION_MAX_UNIT,m_Entry.m_pImpl->m_ClosureJointCriteria.TensionStressLimit_OtherAreas_WithoutReinforcement_AfterLosses.MaxValue,pDisplayUnits->Stress);
   if (m_Entry.m_pImpl->m_ClosureJointCriteria.TensionStressLimit_OtherAreas_WithoutReinforcement_AfterLosses.bHasMaxValue)
   {
      DDV_UnitValueGreaterThanZero(pDX,IDC_SERVICE_III_TENSION_MAX,m_Entry.m_pImpl->m_ClosureJointCriteria.TensionStressLimit_OtherAreas_WithoutReinforcement_AfterLosses.MaxValue,pDisplayUnits->Stress);
   }

   DDX_UnitValueAndTag(pDX, IDC_SERVICE_III_TENSION_WITH_REBAR, IDC_SERVICE_III_TENSION_WITH_REBAR_UNIT, m_Entry.m_pImpl->m_ClosureJointCriteria.TensionStressLimit_OtherAreas_WithReinforcement_AfterLosses.Coefficient, pDisplayUnits->SqrtPressure );
   DDX_Text(pDX,IDC_SERVICE_III_TENSION_WITH_REBAR_UNIT,tagAfterLosses);
   DDV_UnitValueZeroOrMore(pDX, IDC_SERVICE_III_TENSION_WITH_REBAR_UNIT,m_Entry.m_pImpl->m_ClosureJointCriteria.TensionStressLimit_OtherAreas_WithReinforcement_AfterLosses.Coefficient, pDisplayUnits->SqrtPressure);
   DDX_Check_Bool(pDX,IDC_CHECK_SERVICE_III_TENSION_WITH_REBAR_MAX,m_Entry.m_pImpl->m_ClosureJointCriteria.TensionStressLimit_OtherAreas_WithReinforcement_AfterLosses.bHasMaxValue);
   DDX_UnitValueAndTag(pDX,IDC_SERVICE_III_TENSION_WITH_REBAR_MAX,IDC_SERVICE_III_TENSION_WITH_REBAR_MAX_UNIT,m_Entry.m_pImpl->m_ClosureJointCriteria.TensionStressLimit_OtherAreas_WithReinforcement_AfterLosses.MaxValue,pDisplayUnits->Stress);
   if (m_Entry.m_pImpl->m_ClosureJointCriteria.TensionStressLimit_OtherAreas_WithReinforcement_AfterLosses.bHasMaxValue)
   {
      DDV_UnitValueGreaterThanZero(pDX,IDC_SERVICE_III_TENSION_WITH_REBAR_MAX,m_Entry.m_pImpl->m_ClosureJointCriteria.TensionStressLimit_OtherAreas_WithReinforcement_AfterLosses.MaxValue,pDisplayUnits->Stress);
   }

   DDX_Text(pDX,IDC_FATIGUE_COMPRESSION,m_Entry.m_pImpl->m_ClosureJointCriteria.CompressionStressCoefficient_Fatigue);
   DDV_GreaterThanZero(pDX, IDC_FATIGUE_COMPRESSION,m_Entry.m_pImpl->m_ClosureJointCriteria.CompressionStressCoefficient_Fatigue);
}

void CSpecMainSheet::ExchangeDesignData(CDataExchange* pDX)
{
   CEAFApp* pApp = EAFGetApp();
   const WBFL::Units::IndirectMeasure* pDisplayUnits = pApp->GetDisplayUnits();

   // Harped Strand Hold Down Force
	DDX_Check_Bool(pDX, IDC_CHECK_HD,  m_Entry.m_pImpl->m_HoldDownCriteria.bCheck);
	DDX_Check_Bool(pDX, IDC_DESIGN_HD, m_Entry.m_pImpl->m_HoldDownCriteria.bDesign);

   DDX_CBEnum(pDX, IDC_HOLD_DOWN_FORCE_TYPE, m_Entry.m_pImpl->m_HoldDownCriteria.type);

   DDX_UnitValueAndTag(pDX, IDC_HOLD_DOWN_FORCE, IDC_HOLD_DOWN_FORCE_UNITS, m_Entry.m_pImpl->m_HoldDownCriteria.force_limit, pDisplayUnits->GeneralForce );
   if (m_Entry.m_pImpl->m_HoldDownCriteria.bCheck)
   {
      DDV_UnitValueGreaterThanZero(pDX, IDC_HOLD_DOWN_FORCE,m_Entry.m_pImpl->m_HoldDownCriteria.force_limit, pDisplayUnits->GeneralForce );
   }
   DDX_Percentage(pDX, IDC_FRICTION, m_Entry.m_pImpl->m_HoldDownCriteria.friction);

   // Harped Strand Slope
	DDX_Check_Bool(pDX, IDC_CHECK_SLOPE,  m_Entry.m_pImpl->m_StrandSlopeCriteria.bCheck);
	DDX_Check_Bool(pDX, IDC_DESIGN_SLOPE, m_Entry.m_pImpl->m_StrandSlopeCriteria.bDesign);

	DDX_Text(pDX, IDC_STRAND_SLOPE_05, m_Entry.m_pImpl->m_StrandSlopeCriteria.MaxSlope05);
   if (m_Entry.m_pImpl->m_StrandSlopeCriteria.bCheck)
   {
      DDV_NonNegativeDouble(pDX, IDC_STRAND_SLOPE_05, m_Entry.m_pImpl->m_StrandSlopeCriteria.MaxSlope05);
   }

	DDX_Text(pDX, IDC_STRAND_SLOPE_06, m_Entry.m_pImpl->m_StrandSlopeCriteria.MaxSlope06);
   if (m_Entry.m_pImpl->m_StrandSlopeCriteria.bCheck)
   {
      DDV_NonNegativeDouble(pDX, IDC_STRAND_SLOPE_06, m_Entry.m_pImpl->m_StrandSlopeCriteria.MaxSlope06);
   }

	DDX_Text(pDX, IDC_STRAND_SLOPE_07, m_Entry.m_pImpl->m_StrandSlopeCriteria.MaxSlope07);
   if (m_Entry.m_pImpl->m_StrandSlopeCriteria.bCheck)
   {
      DDV_NonNegativeDouble(pDX, IDC_STRAND_SLOPE_07, m_Entry.m_pImpl->m_StrandSlopeCriteria.MaxSlope07);
   }
   
   // Handling Weight
   DDX_Check_Bool(pDX, IDC_CHECK_HANDLING_WEIGHT, m_Entry.m_pImpl->m_PlantHandlingCriteria.bCheck);
   DDX_UnitValueAndTag(pDX, IDC_HANDLING_WEIGHT, IDC_HANDLING_WEIGHT_UNIT, m_Entry.m_pImpl->m_PlantHandlingCriteria.WeightLimit, pDisplayUnits->GeneralForce);
   if (m_Entry.m_pImpl->m_PlantHandlingCriteria.bCheck)
   {
      DDV_UnitValueGreaterThanZero(pDX, IDC_HANDLING_WEIGHT, m_Entry.m_pImpl->m_PlantHandlingCriteria.WeightLimit, pDisplayUnits->GeneralForce);
   }

   // Splitting and Confinement
	DDX_Check_Bool(pDX, IDC_CHECK_SPLITTING,  m_Entry.m_pImpl->m_EndZoneCriteria.bCheckSplitting);
	DDX_Check_Bool(pDX, IDC_DESIGN_SPLITTING, m_Entry.m_pImpl->m_EndZoneCriteria.bDesignSplitting);
	DDX_Text(pDX, IDC_N, m_Entry.m_pImpl->m_EndZoneCriteria.SplittingZoneLengthFactor);
   DDV_GreaterThanZero(pDX, IDC_N, m_Entry.m_pImpl->m_EndZoneCriteria.SplittingZoneLengthFactor);
	DDX_Check_Bool(pDX, IDC_CHECK_CONFINEMENT,  m_Entry.m_pImpl->m_EndZoneCriteria.bCheckConfinement);
	DDX_Check_Bool(pDX, IDC_DESIGN_CONFINEMENT, m_Entry.m_pImpl->m_EndZoneCriteria.bDesignConfinement);

   // Lifting
	DDX_Check_Bool(pDX, IDC_CHECK_LIFTING,  m_Entry.m_pImpl->m_LiftingCriteria.bCheck);
	DDX_Check_Bool(pDX, IDC_DESIGN_LIFTING, m_Entry.m_pImpl->m_LiftingCriteria.bDesign);
 
   DDX_KeywordUnitValueAndTag(pDX,IDC_MIN_LIFTING_POINT,IDC_MIN_LIFTING_POINT_UNIT,_T("H"),m_Entry.m_pImpl->m_LiftingCriteria.MinPickPoint, pDisplayUnits->SpanLength);

   DDX_UnitValueAndTag(pDX, IDC_LIFTING_POINT_LOCATION_ACCURACY,IDC_LIFTING_POINT_LOCATION_ACCURACY_UNIT,m_Entry.m_pImpl->m_LiftingCriteria.PickPointAccuracy, pDisplayUnits->SpanLength );
   DDV_UnitValueGreaterThanZero(pDX, IDC_LIFTING_POINT_LOCATION_ACCURACY,m_Entry.m_pImpl->m_LiftingCriteria.PickPointAccuracy, pDisplayUnits->SpanLength );

   // Hauling
   DDX_Check_Bool(pDX, IDC_CHECK_HAULING, m_Entry.m_pImpl->m_HaulingCriteria.bCheck);
	DDX_Check_Bool(pDX, IDC_DESIGN_HAULING, m_Entry.m_pImpl->m_HaulingCriteria.bDesign);

   DDX_KeywordUnitValueAndTag(pDX,IDC_MIN_TRUCK_SUPPORT,IDC_MIN_TRUCK_SUPPORT_UNIT,_T("H"),m_Entry.m_pImpl->m_HaulingCriteria.MinBunkPoint, pDisplayUnits->SpanLength);
   DDX_UnitValueAndTag(pDX, IDC_TRUCK_SUPPORT_LOCATION_ACCURACY,IDC_TRUCK_SUPPORT_LOCATION_ACCURACY_UNIT,m_Entry.m_pImpl->m_HaulingCriteria.BunkPointAccuracy, pDisplayUnits->SpanLength );
   DDV_UnitValueGreaterThanZero(pDX, IDC_TRUCK_SUPPORT_LOCATION_ACCURACY,m_Entry.m_pImpl->m_HaulingCriteria.BunkPointAccuracy, pDisplayUnits->SpanLength );

   DDX_Check_Bool(pDX, IDC_IS_SUPPORT_LESS_THAN, m_Entry.m_pImpl->m_HaulingCriteria.bUseMinBunkPointLimit);
	DDX_Text(pDX, IDC_SUPPORT_LESS_THAN, m_Entry.m_pImpl->m_HaulingCriteria.MinBunkPointLimitFactor);
   DDV_MinMaxDouble(pDX, m_Entry.m_pImpl->m_HaulingCriteria.MinBunkPointLimitFactor, 0.0, 0.4);

   // Slab Offset
	DDX_Check_Bool(pDX, IDC_CHECK_A, m_Entry.m_pImpl->m_SlabOffsetCriteria.bCheck);
	DDX_Check_Bool(pDX, IDC_DESIGN_A, m_Entry.m_pImpl->m_SlabOffsetCriteria.bDesign);
   DDX_CBEnum(pDX,IDC_A_ROUNDING_CB,m_Entry.m_pImpl->m_SlabOffsetCriteria.RoundingMethod);
   DDX_UnitValueAndTag(pDX, IDC_A_ROUNDING_EDIT,IDC_A_ROUNDING_UNIT,m_Entry.m_pImpl->m_SlabOffsetCriteria.SlabOffsetTolerance, pDisplayUnits->ComponentDim );
   DDV_UnitValueZeroOrMore(pDX, IDC_A_ROUNDING_EDIT,m_Entry.m_pImpl->m_SlabOffsetCriteria.SlabOffsetTolerance, pDisplayUnits->ComponentDim );

   // Live Load Deflections
   DDX_Check_Bool(pDX, IDC_LL_DEFLECTION, m_Entry.m_pImpl->m_LiveLoadDeflectionCriteria.bCheck);
 	DDX_Text(pDX, IDC_DEFLECTION_LIMIT, m_Entry.m_pImpl->m_LiveLoadDeflectionCriteria.DeflectionLimit);
   DDV_GreaterThanZero(pDX, IDC_DEFLECTION_LIMIT, m_Entry.m_pImpl->m_LiveLoadDeflectionCriteria.DeflectionLimit);

   // Bottom Flange Clearance
   DDX_Check_Bool(pDX,IDC_CHECK_BOTTOM_FLANGE_CLEARANCE,m_Entry.m_pImpl->m_BottomFlangeClearanceCriteria.bCheck);
   DDX_UnitValueAndTag(pDX,IDC_CLEARANCE,IDC_CLEARANCE_UNIT,m_Entry.m_pImpl->m_BottomFlangeClearanceCriteria.MinClearance,pDisplayUnits->SpanLength);
   DDV_UnitValueZeroOrMore(pDX,IDC_CLEARANCE,m_Entry.m_pImpl->m_BottomFlangeClearanceCriteria.MinClearance,pDisplayUnits->SpanLength);

   // Girder Inclination
   DDX_Check_Bool(pDX, IDC_CHECK_INCLINDED_GIRDER, m_Entry.m_pImpl->m_GirderInclinationCriteria.bCheck);
   DDX_Text(pDX, IDC_INCLINDED_GIRDER_FS, m_Entry.m_pImpl->m_GirderInclinationCriteria.FS);

   // Strand Fill
   int value = (int)m_Entry.m_pImpl->m_HarpedStrandDesignCriteria.StrandFillType;
   DDX_Radio(pDX,IDC_RADIO_FILL_PERMANENT,value);
   if ( pDX->m_bSaveAndValidate )
   {
      m_Entry.m_pImpl->m_HarpedStrandDesignCriteria.StrandFillType = (arDesignStrandFillType)value;
   }

   value = (int)m_Entry.m_pImpl->m_LimitStateConcreteStrengthCriteria.LimitStateConcreteStrength;
   DDX_Radio(pDX,IDC_FC1,value);
   if ( pDX->m_bSaveAndValidate )
   {
      m_Entry.m_pImpl->m_LimitStateConcreteStrengthCriteria.LimitStateConcreteStrength = (pgsTypes::LimitStateConcreteStrength)value;
   }

   DDX_Check_Bool(pDX, IDC_USE_90_DAY_STRENGTH, m_Entry.m_pImpl->m_LimitStateConcreteStrengthCriteria.bUse90DayConcreteStrength);
   DDX_Percentage(pDX, IDC_90_DAY_STRENGTH_FACTOR, m_Entry.m_pImpl->m_LimitStateConcreteStrengthCriteria.SlowCuringConcreteStrengthFactor);
   if (pDX->m_bSaveAndValidate)
   {
      if (m_Entry.m_pImpl->m_LimitStateConcreteStrengthCriteria.SlowCuringConcreteStrengthFactor < 1.0)
      {
         pDX->PrepareEditCtrl(IDC_90_DAY_STRENGTH_FACTOR);
         AfxMessageBox(_T("Factor for 90 day concrete strength must be at least 100%"));
         pDX->Fail();
      }
   }

   // Roadway elevations
   DDX_UnitValueAndTag(pDX, IDC_ELEVATION_TOLERANCE, IDC_ELEVATION_TOLERANCE_UNIT, m_Entry.m_pImpl->m_SlabOffsetCriteria.FinishedElevationTolerance, pDisplayUnits->ComponentDim);
   DDV_UnitValueZeroOrMore(pDX, IDC_ELEVATION_TOLERANCE, m_Entry.m_pImpl->m_SlabOffsetCriteria.FinishedElevationTolerance, pDisplayUnits->ComponentDim);
}

void CSpecMainSheet::ExchangeBearingsData(CDataExchange* pDX)
{
    CEAFApp* pApp = EAFGetApp();
    const WBFL::Units::IndirectMeasure* pDisplayUnits = pApp->GetDisplayUnits();

   DDX_Check_Bool(pDX, IDC_TAPERED_SOLE_PLATE_REQUIRED, m_Entry.m_pImpl->m_BearingCriteria.bAlertTaperedSolePlateRequirement);
   DDX_Text(pDX, IDC_TAPERED_SOLE_PLATE_THRESHOLD, m_Entry.m_pImpl->m_BearingCriteria.TaperedSolePlateInclinationThreshold);
   DDV_LimitOrMore(pDX, IDC_TAPERED_SOLE_PLATE_THRESHOLD, m_Entry.m_pImpl->m_BearingCriteria.TaperedSolePlateInclinationThreshold, 0.0);
   DDX_Check_Bool(pDX, IDC_BEARING_REACTION_IMPACT, m_Entry.m_pImpl->m_BearingCriteria.bUseImpactForBearingReactions);

   int value = (int)m_Entry.m_pImpl->m_BearingCriteria.BearingDesignMethod;
   DDX_Radio(pDX, IDC_BEARING_METHOD_A, value);

   DDX_UnitValueAndTag(pDX, IDC_SHEAR_MOD_MIN_LIMIT, IDC_SHEAR_MOD_MIN_LIMIT_UNIT,
       m_Entry.m_pImpl->m_BearingCriteria.MinimumElastomerShearModulus, pDisplayUnits->ModE);

   DDX_UnitValueAndTag(pDX, IDC_SHEAR_MOD_MAX_LIMIT, IDC_SHEAR_MOD_MAX_LIMIT_UNIT,
       m_Entry.m_pImpl->m_BearingCriteria.MaximumElastomerShearModulus, pDisplayUnits->ModE);

   DDX_Check_Bool(pDX, IDC_REQ_INT_LAYER_THICK_CHECK, m_Entry.m_pImpl->m_BearingCriteria.bRequiredIntermediateElastomerThickness);
   DDX_UnitValueAndTag(pDX, IDC_REQ_INT_LAYER_THICK, IDC_REQ_INT_LAYER_THICK_UNIT,
       m_Entry.m_pImpl->m_BearingCriteria.RequiredIntermediateElastomerThickness, pDisplayUnits->ComponentDim);

   DDX_Check_Bool(pDX, IDC_MIN_BEARING_HEIGHT_CHECK, m_Entry.m_pImpl->m_BearingCriteria.bMinimumTotalBearingHeight);
   DDX_UnitValueAndTag(pDX, IDC_MIN_BEARING_HEIGHT, IDC_MIN_BEARING_HEIGHT_UNIT,
       m_Entry.m_pImpl->m_BearingCriteria.MinimumTotalBearingHeight, pDisplayUnits->ComponentDim);

   DDX_Check_Bool(pDX, IDC_MIN_BEARING_GIRDER_EDGE_CHECK, m_Entry.m_pImpl->m_BearingCriteria.bMinimumBearingEdgeToGirderEdgeDistance);
   DDX_UnitValueAndTag(pDX, IDC_MIN_BEARING_GIRDER_EDGE, IDC_MIN_BEARING_GIRDER_EDGE_UNIT,
       m_Entry.m_pImpl->m_BearingCriteria.MinimumTotalBearingHeight, pDisplayUnits->ComponentDim);

   DDX_Check_Bool(pDX, IDC_MAX_BEARING_GIRDER_EDGE_CHECK, m_Entry.m_pImpl->m_BearingCriteria.bMaximumBearingEdgeToGirderEdgeDistance);
   DDX_UnitValueAndTag(pDX, IDC_MAX_BEARING_GIRDER_EDGE, IDC_MAX_BEARING_GIRDER_EDGE_UNIT,
       m_Entry.m_pImpl->m_BearingCriteria.MaximumBearingEdgeToGirderEdgeDistance, pDisplayUnits->ComponentDim);

   DDX_Check_Bool(pDX, IDC_REQ_BEARING_GIRDER_EDGE_CHECK, m_Entry.m_pImpl->m_BearingCriteria.bRequiredBearingEdgeToGirderEdgeDistance);
   DDX_UnitValueAndTag(pDX, IDC_REQ_BEARING_GIRDER_EDGE, IDC_REQ_BEARING_GIRDER_EDGE_UNIT,
       m_Entry.m_pImpl->m_BearingCriteria.RequiredBearingEdgeToGirderEdgeDistance, pDisplayUnits->ComponentDim);

   DDX_UnitValueAndTag(pDX, IDC_MAX_LL_DEF_LIMIT, IDC_MAX_LL_DEF_LIMIT_UNIT,
       m_Entry.m_pImpl->m_BearingCriteria.MaximumLiveLoadDeflection, pDisplayUnits->ComponentDim);

   DDX_Check_Bool(pDX, IDC_MAX_TOTAL_BEARING_LOAD_CHECK, m_Entry.m_pImpl->m_BearingCriteria.bRequiredBearingEdgeToGirderEdgeDistance);
   DDX_UnitValueAndTag(pDX, IDC_MAX_BEARING_TL, IDC_MAX_TL_UNIT,
       m_Entry.m_pImpl->m_BearingCriteria.MaximumTotalLoad, pDisplayUnits->GeneralForce);

}

BOOL CSpecMainSheet::OnInitDialog() 
{
	BOOL bResult = CPropertySheet::OnInitDialog();
	
   // disable OK button if editing not allowed
   CString head;
   GetWindowText(head);
   head += _T(" - ");
   head += m_Entry.GetName().c_str();
	if (!m_bAllowEditing)
   {
      CWnd* pOK = GetDlgItem(IDOK);
      pOK->ShowWindow(SW_HIDE);

      CWnd* pCancel = GetDlgItem(IDCANCEL);
      pCancel->SetWindowText(_T("Close"));

      head += _T(" (Read Only)");
   }
   SetWindowText(head);
	
	return bResult;
}

WBFL::LRFD::BDSManager::Edition CSpecMainSheet::GetSpecVersion()
{
   return m_SpecDescrPage.GetSpecVersion();
}