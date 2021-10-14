///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2021  Washington State Department of Transportation
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
#include <MfcTools\CustomDDX.h>

#include <Units\sysUnits.h>
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
   DDX_CBItemData(pDX,IDC_SPECIFICATION,m_Entry.m_SpecificationType);
   DDX_Check_Bool(pDX, IDC_USE_CURRENT_VERSION, m_Entry.m_bUseCurrentSpecification);

   // Section Properties
   DDX_RadioEnum(pDX,IDC_GROSS,m_Entry.m_SectionPropertyMode);
   DDX_CBEnum(pDX,IDC_EFF_FLANGE_WIDTH,m_Entry.m_EffFlangeWidthMethod);

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
      m_Entry.SetDescription(m_Description);


      // specification units
      int chk = m_SpecDescrPage.GetCheckedRadioButton( IDC_SPEC_UNITS_SI,IDC_SPEC_UNITS_US);
      if (chk==IDC_SPEC_UNITS_SI)
         m_Entry.SetSpecificationUnits(lrfdVersionMgr::SI);
      else if (chk==IDC_SPEC_UNITS_US)
         m_Entry.SetSpecificationUnits(lrfdVersionMgr::US);
      else
         ASSERT(0);
   }
   else
   {
      // name
      m_Name = m_Entry.GetName().c_str();
	   DDX_Text(pDX, IDC_NAME, m_Name);

      m_Description = m_Entry.GetDescription(false).c_str();
	   DDX_Text(pDX, IDC_EDIT_DESCRIPTION, m_Description);

      // spec units
      lrfdVersionMgr::Units Units = m_Entry.GetSpecificationUnits();
      int unit;
      if (Units == lrfdVersionMgr::SI)
         unit = IDC_SPEC_UNITS_SI;
      else if (Units == lrfdVersionMgr::US)
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
   const unitmgtIndirectMeasure* pDisplayUnits = pApp->GetDisplayUnits();

   // railing system distribution
   DDX_CBItemData(pDX,IDC_DIST_TRAFFIC_BARRIER_BASIS,m_Entry.m_TrafficBarrierDistributionType);
	DDX_Text(pDX, IDC_DIST_TRAFFIC_BARRIER, m_Entry.m_Bs2MaxGirdersTrafficBarrier);

   // overlay load distribution
   DDX_CBEnum(pDX, IDC_OVERLAY_DISTR,m_Entry.m_OverlayLoadDistribution);

}

void CSpecMainSheet::ExchangeLiveLoadsData(CDataExchange* pDX)
{
   CEAFApp* pApp = EAFGetApp();
   const unitmgtIndirectMeasure* pDisplayUnits = pApp->GetDisplayUnits();

   DDX_Check_Bool(pDX, IDC_DUAL_TANDEM, m_Entry.m_bIncludeDualTandem);

   // pedestrian live loads
   DDX_UnitValueAndTag(pDX, IDC_PED_LIVE_LOAD, IDC_PED_LIVE_LOAD_UNIT, m_Entry.m_PedestrianLoad, pDisplayUnits->SmallStress);
   DDV_UnitValueZeroOrMore(pDX, IDC_PED_LIVE_LOAD, m_Entry.m_PedestrianLoad, pDisplayUnits->SpanLength);
   DDX_UnitValueAndTag(pDX, IDC_MIN_SIDEWALK_WIDTH, IDC_MIN_SIDEWALK_WIDTH_UNIT, m_Entry.m_MinSidewalkWidth, pDisplayUnits->SpanLength);
   DDV_UnitValueZeroOrMore(pDX, IDC_MIN_SIDEWALK_WIDTH, m_Entry.m_MinSidewalkWidth, pDisplayUnits->SpanLength);

   // live load distribution
   DDX_CBIndex(pDX, IDC_LLDF, m_Entry.m_LldfMethod);
   DDX_Check_Bool(pDX, IDC_IGNORE_SKEW_REDUCTION, m_Entry.m_bIgnoreSkewReductionForMoment);

   DDX_UnitValueAndTag(pDX, IDC_MAXGIRDERANGLE, IDC_MAXGIRDERANGLE_UNIT, m_Entry.m_MaxAngularDeviationBetweenGirders, pDisplayUnits->Angle);
   DDX_Text(pDX, IDC_GIRDERSTIFFNESSRATIO, m_Entry.m_MinGirderStiffnessRatio);
   DDV_MinMaxDouble(pDX, m_Entry.m_MinGirderStiffnessRatio, 0.0, 1.0);

   DDX_Text(pDX, IDC_GIRDER_SPACING_LOCATION, m_Entry.m_LLDFGirderSpacingLocation);
   DDV_MinMaxDouble(pDX, m_Entry.m_LLDFGirderSpacingLocation, 0.0, 1.0);

   DDX_Check_Bool(pDX, IDC_LANESBEAMS, m_Entry.m_LimitDistributionFactorsToLanesBeams);
   DDX_Check_Bool(pDX, IDC_EXT_ADJACENT_LLDF_RULE, m_Entry.m_ExteriorLiveLoadDistributionGTAdjacentInteriorRule);

   DDX_Check_Bool(pDX, IDC_RIGID_METHOD, m_Entry.m_bUseRigidMethod);
}

void CSpecMainSheet::ExchangeGirderData(CDataExchange* pDX)
{
   CEAFApp* pApp = EAFGetApp();
   const unitmgtIndirectMeasure* pDisplayUnits = pApp->GetDisplayUnits();

   CString fciTag = (pApp->GetUnitsMode() == eafTypes::umSI ? _T("sqrt(f'ci (MPa))") : _T("sqrt(f'ci (KSI))"));
   CString fcTag  = (pApp->GetUnitsMode() == eafTypes::umSI ? _T("sqrt(f'c (MPa))")  : _T("sqrt(f'c (KSI))"));

   if ( lrfdVersionMgr::SeventhEditionWith2016Interims <= lrfdVersionMgr::GetVersion() )
   {
      fciTag = _T("(lambda)") + fciTag;
      fcTag  = _T("(lambda)") + fcTag;
   }

   // Allowable concrete stress at prestress release
	DDX_Text(pDX, IDC_RELEASE_COMPRESSION, m_Entry.m_CyCompStressServ);
   DDV_GreaterThanZero(pDX, IDC_RELEASE_COMPRESSION, m_Entry.m_CyCompStressServ);

   DDX_UnitValueAndTag(pDX, IDC_RELEASE_TENSION, IDC_RELEASE_TENSION_UNIT, m_Entry.m_CyTensStressServ, pDisplayUnits->SqrtPressure );
   DDX_Text(pDX,IDC_RELEASE_TENSION_UNIT,fciTag);
   DDV_UnitValueZeroOrMore(pDX, IDC_RELEASE_TENSION_UNIT,m_Entry.m_CyTensStressServ, pDisplayUnits->SqrtPressure);
   DDX_Check_Bool(pDX, IDC_CHECK_RELEASE_TENSION_MAX, m_Entry.m_CyDoTensStressServMax);
   DDX_UnitValueAndTag(pDX, IDC_RELEASE_TENSION_MAX, IDC_RELEASE_TENSION_MAX_UNIT, m_Entry.m_CyTensStressServMax, pDisplayUnits->Stress );
   if (m_Entry.m_CyDoTensStressServMax)
   {
      DDV_UnitValueGreaterThanZero(pDX, IDC_RELEASE_TENSION_MAX,m_Entry.m_CyTensStressServMax, pDisplayUnits->Stress );
   }

   DDX_UnitValueAndTag(pDX, IDC_RELEASE_TENSION_WITH_REBAR, IDC_SERVICE_III_TENSION_UNIT, m_Entry.m_CyTensStressServWithRebar, pDisplayUnits->SqrtPressure);
   DDX_Text(pDX,IDC_RELEASE_TENSION_WITH_REBAR_UNIT,fciTag);
   DDV_UnitValueZeroOrMore(pDX, IDC_RELEASE_TENSION_WITH_REBAR,m_Entry.m_CyTensStressServWithRebar, pDisplayUnits->SqrtPressure );

   if (pDX->m_bSaveAndValidate && m_Entry.m_CyTensStressServWithRebar < m_Entry.m_CyTensStressServ)
   {
      AfxMessageBox(_T("Allowable tensile stress with bonded reinforcement must be greater than or equal to that without"),MB_OK | MB_ICONWARNING);
      pDX->Fail();
   }
   // Allowable concrete stress at service limit states
	DDX_Text(pDX, IDC_SERVICE_COMPRESSION, m_Entry.m_Bs2CompStress);
   DDV_GreaterThanZero(pDX, IDC_SERVICE_COMPRESSION, m_Entry.m_Bs2CompStress);

	DDX_Text(pDX, IDC_SERVICE_COMPRESSION_WITH_LIVELOAD, m_Entry.m_Bs3CompStressServ);
   DDV_GreaterThanZero(pDX, IDC_SERVICE_COMPRESSION_WITH_LIVELOAD, m_Entry.m_Bs3CompStressServ);


   DDX_Check_Bool(pDX, IDC_CHECK_SERVICE_I_TENSION, m_Entry.m_bCheckBs2Tension );
   DDX_UnitValueAndTag(pDX, IDC_SERVICE_I_TENSION, IDC_SERVICE_I_TENSION_UNIT, m_Entry.m_Bs2TensStress, pDisplayUnits->SqrtPressure );
   DDX_Text(pDX,IDC_SERVICE_I_TENSION_UNIT,fcTag);
   DDV_UnitValueZeroOrMore(pDX, IDC_SERVICE_I_TENSION,m_Entry.m_Bs2TensStress, pDisplayUnits->SqrtPressure );
   DDX_Check_Bool(pDX, IDC_CHECK_SERVICE_I_TENSION_MAX, m_Entry.m_Bs2DoTensStressMax);
   DDX_UnitValueAndTag(pDX, IDC_SERVICE_I_TENSION_MAX, IDC_SERVICE_I_TENSION_MAX_UNIT, m_Entry.m_Bs2TensStressMax, pDisplayUnits->Stress );
   if (m_Entry.m_Bs2DoTensStressMax)
   {
      DDV_UnitValueGreaterThanZero(pDX, IDC_SERVICE_I_TENSION_MAX,m_Entry.m_Bs2TensStressMax, pDisplayUnits->Stress );
   }


   DDX_UnitValueAndTag(pDX, IDC_SERVICE_III_TENSION, IDC_SERVICE_III_TENSION_UNIT, m_Entry.m_Bs3TensStressServNc, pDisplayUnits->SqrtPressure );
   DDX_Text(pDX,IDC_SERVICE_III_TENSION_UNIT,fcTag);
   DDV_UnitValueZeroOrMore(pDX, IDC_SERVICE_III_TENSION,m_Entry.m_Bs3TensStressServNc, pDisplayUnits->SqrtPressure );
   DDX_Check_Bool(pDX, IDC_CHECK_SERVICE_III_TENSION_MAX, m_Entry.m_Bs3DoTensStressServNcMax);
   DDX_UnitValueAndTag(pDX, IDC_SERVICE_III_TENSION_MAX, IDC_SERVICE_III_TENSION_MAX_UNIT, m_Entry.m_Bs3TensStressServNcMax, pDisplayUnits->Stress );
   if (m_Entry.m_Bs3DoTensStressServNcMax)
   {
      DDV_UnitValueGreaterThanZero(pDX, IDC_SERVICE_III_TENSION_MAX,m_Entry.m_Bs3TensStressServNcMax, pDisplayUnits->Stress );
   }

   DDX_UnitValueAndTag(pDX, IDC_SEVERE_SERVICE_III_TENSION, IDC_SEVERE_SERVICE_III_TENSION_UNIT, m_Entry.m_Bs3TensStressServSc, pDisplayUnits->SqrtPressure );
   DDX_Text(pDX,IDC_SEVERE_SERVICE_III_TENSION_UNIT,fcTag);
   DDV_UnitValueZeroOrMore(pDX, IDC_SEVERE_SERVICE_III_TENSION,m_Entry.m_Bs3TensStressServSc, pDisplayUnits->SqrtPressure );
   DDX_Check_Bool(pDX, IDC_CHECK_SEVERE_SERVICE_III_TENSION_MAX, m_Entry.m_Bs3DoTensStressServScMax);
   DDX_UnitValueAndTag(pDX, IDC_SEVERE_SERVICE_III_TENSION_MAX, IDC_SEVERE_SERVICE_III_TENSION_MAX_UNIT, m_Entry.m_Bs3TensStressServScMax, pDisplayUnits->Stress );
   if (m_Entry.m_Bs3DoTensStressServScMax)
   {
      DDV_UnitValueGreaterThanZero(pDX, IDC_SEVERE_SERVICE_III_TENSION_MAX,m_Entry.m_Bs3TensStressServScMax, pDisplayUnits->Stress );
   }

   // Allowable concrete stress at fatigue limit state
   DDX_Text(pDX, IDC_FATIGUE_COMPRESSION, m_Entry.m_Bs3CompStressService1A);
   DDV_GreaterThanZero(pDX, IDC_FATIGUE_COMPRESSION, m_Entry.m_Bs3CompStressService1A);

   // Temporary Loading Condition (PGSuper only)
   DDX_Check_Bool(pDX,IDC_CHECK_TEMPORARY_STRESSES,m_Entry.m_bCheckTemporaryStresses);

   // Allowable concrete stress after Temporary Strand Removal
	DDX_Text(pDX, IDC_TS_REMOVAL_COMPRESSION, m_Entry.m_TempStrandRemovalCompStress);
   DDV_GreaterThanZero(pDX, IDC_TS_REMOVAL_COMPRESSION, m_Entry.m_TempStrandRemovalCompStress);

   DDX_UnitValueAndTag(pDX, IDC_TS_REMOVAL_TENSION, IDC_TS_REMOVAL_TENSION_UNIT, m_Entry.m_TempStrandRemovalTensStress, pDisplayUnits->SqrtPressure );
   DDX_Text(pDX,IDC_TS_REMOVAL_TENSION_UNIT,fcTag);
   DDV_UnitValueZeroOrMore(pDX,IDC_TS_REMOVAL_TENSION, m_Entry.m_TempStrandRemovalTensStress, pDisplayUnits->SqrtPressure );
   DDX_Check_Bool(pDX, IDC_CHECK_TS_REMOVAL_TENSION_MAX, m_Entry.m_TempStrandRemovalDoTensStressMax);
   DDX_UnitValueAndTag(pDX, IDC_TS_REMOVAL_TENSION_MAX, IDC_TS_REMOVAL_TENSION_MAX_UNIT, m_Entry.m_TempStrandRemovalTensStressMax, pDisplayUnits->Stress );
   if (m_Entry.m_TempStrandRemovalDoTensStressMax)
   {
      DDV_UnitValueGreaterThanZero(pDX, IDC_TS_REMOVAL_TENSION_MAX,m_Entry.m_TempStrandRemovalTensStressMax, pDisplayUnits->Stress );
   }

   DDX_UnitValueAndTag(pDX, IDC_TS_TENSION_WITH_REBAR, IDC_TS_TENSION_WITH_REBAR_UNIT, m_Entry.m_TempStrandRemovalTensStressWithRebar, pDisplayUnits->SqrtPressure);
   DDX_Text(pDX,IDC_TS_TENSION_WITH_REBAR_UNIT,fcTag);

   // Allowable concrete stress after deck placement
	DDX_Text(pDX, IDC_AFTER_DECK_COMPRESSION, m_Entry.m_Bs1CompStress);
   DDV_GreaterThanZero(pDX, IDC_AFTER_DECK_COMPRESSION, m_Entry.m_Bs1CompStress);

   DDX_UnitValueAndTag(pDX, IDC_AFTER_DECK_TENSION, IDC_AFTER_DECK_TENSION_UNIT, m_Entry.m_Bs1TensStress, pDisplayUnits->SqrtPressure );
   DDX_Text(pDX,IDC_AFTER_DECK_TENSION_UNIT,fcTag);
   DDV_UnitValueZeroOrMore(pDX, IDC_AFTER_DECK_TENSION,m_Entry.m_Bs1TensStress, pDisplayUnits->SqrtPressure );
   DDX_Check_Bool(pDX, IDC_CHECK_AFTER_DECK_TENSION_MAX, m_Entry.m_Bs1DoTensStressMax);
   DDX_UnitValueAndTag(pDX, IDC_AFTER_DECK_TENSION_MAX, IDC_AFTER_DECK_TENSION_MAX_UNIT, m_Entry.m_Bs1TensStressMax, pDisplayUnits->Stress );
   if (m_Entry.m_Bs1DoTensStressMax)
   {
      DDV_UnitValueGreaterThanZero(pDX, IDC_AFTER_DECK_TENSION_MAX,m_Entry.m_Bs1TensStressMax, pDisplayUnits->Stress );
   }

   // Principal Tension Stress in Webs
   DDX_UnitValueAndTag(pDX, IDC_PRINCIPAL_TENSION, IDC_PRINCIPAL_TENSION_UNIT, m_Entry.m_PrincipalTensileStressCoefficient, pDisplayUnits->SqrtPressure);
   DDX_Text(pDX, IDC_PRINCIPAL_TENSION_UNIT, fcTag);
   DDX_CBEnum(pDX, IDC_PRINCIPAL_TENSION_METHOD, m_Entry.m_PrincipalTensileStressMethod);
   DDX_Text(pDX, IDC_TENDON_NEARNESS_FACTOR, m_Entry.m_PrincipalTensileStressTendonNearnessFactor);
   DDX_Text(pDX, IDC_UNGROUTED_MULTIPLIER, m_Entry.m_PrincipalTensileStressUngroutedMultiplier);
   DDX_Text(pDX, IDC_GROUTED_MULTIPLIER, m_Entry.m_PrincipalTensileStressGroutedMultiplier);


   DDX_KeywordUnitValueAndTag(pDX, IDC_PRINCIPAL_FC_THRESHOLD, IDC_PRINCIPAL_FC_THRESHOLD_UNIT, _T("All"), m_Entry.m_PrincipalTensileStressFcThreshold, pDisplayUnits->Stress );
   if (pDX->m_bSaveAndValidate && m_Entry.m_PrincipalTensileStressFcThreshold < 0.0 && m_Entry.m_PrincipalTensileStressFcThreshold != -1.0)
   {
      AfxMessageBox(_T("f'c threshold value must be zero or greater or keyword \"All\""));
      pDX->PrepareCtrl(IDC_PRINCIPAL_FC_THRESHOLD);
      pDX->Fail();
   }
}

void CSpecMainSheet::ExchangeLiftingData(CDataExchange* pDX)
{
   CEAFApp* pApp = EAFGetApp();
   const unitmgtIndirectMeasure* pDisplayUnits = pApp->GetDisplayUnits();

	DDX_Text(pDX, IDC_FS_CY_CRACK, m_Entry.m_CyLiftingCrackFs);
   DDV_NonNegativeDouble(pDX, IDC_FS_CY_CRACK, m_Entry.m_CyLiftingCrackFs);
	DDX_Text(pDX, IDC_FS_CY_FAIL, m_Entry.m_CyLiftingFailFs);
   DDV_NonNegativeDouble(pDX, IDC_FS_CY_FAIL, m_Entry.m_CyLiftingFailFs);

   CString tag;
   if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::SeventhEditionWith2016Interims )
   {
      tag = pApp->GetUnitsMode() == eafTypes::umSI ? _T("sqrt(f'ci (MPa))") : _T("sqrt(f'ci (KSI))");
   }
   else
   {
      tag = pApp->GetUnitsMode() == eafTypes::umSI ? _T("(lambda)sqrt(f'ci (MPa))") : _T("(lambda)sqrt(f'ci (KSI))");
   }

   DDX_UnitValueAndTag(pDX, IDC_FR, IDC_FR_UNIT, m_Entry.m_LiftingModulusOfRuptureCoefficient[pgsTypes::Normal], pDisplayUnits->SqrtPressure );
   DDX_Text(pDX,IDC_FR_UNIT,tag);

   DDX_UnitValueAndTag(pDX, IDC_ALWC_FR, IDC_ALWC_FR_UNIT, m_Entry.m_LiftingModulusOfRuptureCoefficient[pgsTypes::AllLightweight], pDisplayUnits->SqrtPressure );
   DDX_Text(pDX,IDC_ALWC_FR_UNIT,tag);

   DDX_UnitValueAndTag(pDX, IDC_SLWC_FR, IDC_SLWC_FR_UNIT, m_Entry.m_LiftingModulusOfRuptureCoefficient[pgsTypes::SandLightweight], pDisplayUnits->SqrtPressure );
   DDX_Text(pDX,IDC_SLWC_FR_UNIT,tag);

   DDX_UnitValueAndTag(pDX, IDC_UHPC_FR, IDC_UHPC_FR_UNIT, m_Entry.m_LiftingModulusOfRuptureCoefficient[pgsTypes::UHPC], pDisplayUnits->SqrtPressure);
   DDX_Text(pDX, IDC_UHPC_FR_UNIT, tag);

   DDX_UnitValueAndTag(pDX, IDC_PICK_POINT_HEIGHT, IDC_PICK_POINT_HEIGHT_UNITS, m_Entry.m_PickPointHeight, pDisplayUnits->ComponentDim);
   DDV_UnitValueZeroOrMore(pDX, IDC_PICK_POINT_HEIGHT,m_Entry.m_PickPointHeight, pDisplayUnits->ComponentDim );
   DDX_UnitValueAndTag(pDX, IDC_LIFTING_LOOP_TOLERANCE, IDC_LIFTING_LOOP_TOLERANCE_UNITS, m_Entry.m_LiftingLoopTolerance, pDisplayUnits->ComponentDim );
   DDV_UnitValueZeroOrMore(pDX, IDC_LIFTING_LOOP_TOLERANCE,m_Entry.m_LiftingLoopTolerance, pDisplayUnits->ComponentDim );
   DDX_UnitValueAndTag(pDX, IDC_MIN_CABLE_ANGLE, IDC_MIN_CABLE_ANGLE_UNITS, m_Entry.m_MinCableInclination, pDisplayUnits->Angle);
   DDV_UnitValueRange(pDX, IDC_MIN_CABLE_ANGLE,m_Entry.m_MinCableInclination, 0.0, 90., pDisplayUnits->Angle );

   Float64 sweepTolerance = m_Entry.m_MaxGirderSweepLifting;
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
   m_Entry.m_MaxGirderSweepLifting = sweepTolerance;

   DDX_Text(pDX, IDC_LIFTING_GLOBAL_COMPRESSION, m_Entry.m_LiftingCompressionStressCoefficient_GlobalStress);
   DDV_GreaterThanZero(pDX, IDC_LIFTING_GLOBAL_COMPRESSION, m_Entry.m_LiftingCompressionStressCoefficient_GlobalStress);
   DDX_Text(pDX, IDC_LIFTING_PEAK_COMPRESSION, m_Entry.m_LiftingCompressionStressCoefficient_PeakStress);
   DDV_GreaterThanZero(pDX, IDC_LIFTING_PEAK_COMPRESSION, m_Entry.m_LiftingCompressionStressCoefficient_PeakStress);

   DDX_UnitValueAndTag(pDX, IDC_LIFTING_TENSION, IDC_LIFTING_TENSION_UNIT, m_Entry.m_CyTensStressLifting, pDisplayUnits->SqrtPressure );
   DDX_Text(pDX,IDC_LIFTING_TENSION_UNIT,tag);
   DDV_UnitValueZeroOrMore(pDX, IDC_LIFTING_TENSION_UNIT,m_Entry.m_CyTensStressLifting, pDisplayUnits->SqrtPressure );
   DDX_Check_Bool(pDX, IDC_CHECK_LIFTING_TENSION_MAX, m_Entry.m_CyDoTensStressLiftingMax);
   DDX_UnitValueAndTag(pDX, IDC_LIFTING_TENSION_MAX, IDC_LIFTING_TENSION_MAX_UNIT, m_Entry.m_CyTensStressLiftingMax, pDisplayUnits->Stress );
   if (m_Entry.m_CyDoTensStressLiftingMax)
   {
      DDV_UnitValueGreaterThanZero(pDX, IDC_LIFTING_TENSION_MAX,m_Entry.m_CyTensStressLiftingMax, pDisplayUnits->Stress );
   }

   DDX_UnitValueAndTag(pDX, IDC_LIFTING_TENSION_WITH_REBAR, IDC_LIFTING_TENSION_WITH_REBAR_UNIT, m_Entry.m_TensStressLiftingWithRebar, pDisplayUnits->SqrtPressure );
   DDX_Text(pDX,IDC_LIFTING_TENSION_WITH_REBAR_UNIT,tag);
   DDV_UnitValueZeroOrMore(pDX, IDC_LIFTING_TENSION_WITH_REBAR_UNIT,m_Entry.m_TensStressLiftingWithRebar, pDisplayUnits->SqrtPressure );

   if (pDX->m_bSaveAndValidate && m_Entry.m_TensStressLiftingWithRebar < m_Entry.m_CyTensStressLifting)
   {
      AfxMessageBox(_T("Allowable tensile stress with bonded reinforcement must be greater than or equal to that without"),MB_OK | MB_ICONWARNING);
      pDX->Fail();
   }

   DDX_Percentage(pDX, IDC_IMPACT_UPWARD_LIFTING, m_Entry.m_LiftingUpwardImpact);
   DDV_MinMaxDouble(pDX, m_Entry.m_LiftingUpwardImpact, 0.0, 1.0);
	DDX_Percentage(pDX, IDC_IMPACT_DOWNWARD_LIFTING, m_Entry.m_LiftingDownwardImpact);
   DDV_MinMaxDouble(pDX, m_Entry.m_LiftingDownwardImpact, 0.0, 1.0);

   DDX_Text(pDX, IDC_CAMBER_MULTIPLIER, m_Entry.m_LiftingCamberMultiplier);

   DDX_CBItemData(pDX,IDC_WIND_TYPE,m_Entry.m_LiftingWindType);
   if ( m_Entry.m_LiftingWindType == pgsTypes::Pressure )
   {
      DDX_UnitValueAndTag(pDX,IDC_WIND_LOAD,IDC_WIND_LOAD_UNIT,m_Entry.m_LiftingWindLoad,pDisplayUnits->WindPressure);
   }
   else
   {
      DDX_UnitValueAndTag(pDX,IDC_WIND_LOAD,IDC_WIND_LOAD_UNIT,m_Entry.m_LiftingWindLoad,pDisplayUnits->Velocity);
   }
   DDV_NonNegativeDouble(pDX, IDC_WIND_LOAD, m_Entry.m_LiftingWindLoad);
}

bool CSpecMainSheet::IsHaulingEnabled() const
{
   return m_Entry.m_EnableHaulingCheck;
}

void CSpecMainSheet::ExchangeWsdotHaulingData(CDataExchange* pDX)
{

   CEAFApp* pApp = EAFGetApp();
   const unitmgtIndirectMeasure* pDisplayUnits = pApp->GetDisplayUnits();

   if (pDX->m_bSaveAndValidate)
   {
      ATLASSERT(m_SpecHaulingErectionPage.m_HaulingAnalysisMethod==pgsTypes::hmWSDOT);
      m_Entry.m_HaulingAnalysisMethod = pgsTypes::hmWSDOT;
   }

	DDX_Text(pDX, IDC_HE_HAULING_FS_CRACK, m_Entry.m_HaulingCrackFs);
   DDV_NonNegativeDouble(pDX, IDC_HE_HAULING_FS_CRACK, m_Entry.m_HaulingCrackFs);

   DDX_Text(pDX, IDC_HE_HAULING_FS_ROLLOVER, m_Entry.m_HaulingRollFs);
   DDV_NonNegativeDouble(pDX, IDC_HE_HAULING_FS_ROLLOVER, m_Entry.m_HaulingRollFs);

   DDX_Text(pDX, IDC_HAULING_GLOBAL_COMPRESSION, m_Entry.m_GlobalCompStressHauling);
   DDV_GreaterThanZero(pDX, IDC_HAULING_GLOBAL_COMPRESSION, m_Entry.m_GlobalCompStressHauling);

   DDX_Text(pDX, IDC_HAULING_PEAK_COMPRESSION, m_Entry.m_PeakCompStressHauling);
   DDV_GreaterThanZero(pDX, IDC_HAULING_PEAK_COMPRESSION, m_Entry.m_PeakCompStressHauling);

   CString tag;
   if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::SeventhEditionWith2016Interims )
   {
      tag = pApp->GetUnitsMode() == eafTypes::umSI ? _T("sqrt(f'c (MPa))") : _T("sqrt(f'c (KSI))");
   }
   else
   {
      tag = pApp->GetUnitsMode() == eafTypes::umSI ? _T("(lambda)sqrt(f'c (MPa))") : _T("(lambda)sqrt(f'c (KSI))");
   }

   DDX_UnitValueAndTag(pDX, IDC_FR, IDC_FR_UNIT, m_Entry.m_HaulingModulusOfRuptureCoefficient[pgsTypes::Normal], pDisplayUnits->SqrtPressure );
   DDX_Text(pDX,IDC_FR_UNIT,tag);

   DDX_UnitValueAndTag(pDX, IDC_ALWC_FR, IDC_ALWC_FR_UNIT, m_Entry.m_HaulingModulusOfRuptureCoefficient[pgsTypes::AllLightweight], pDisplayUnits->SqrtPressure );
   DDX_Text(pDX,IDC_ALWC_FR_UNIT,tag);

   DDX_UnitValueAndTag(pDX, IDC_SLWC_FR, IDC_SLWC_FR_UNIT, m_Entry.m_HaulingModulusOfRuptureCoefficient[pgsTypes::SandLightweight], pDisplayUnits->SqrtPressure );
   DDX_Text(pDX,IDC_SLWC_FR_UNIT,tag);

   DDX_UnitValueAndTag(pDX, IDC_UHPC_FR, IDC_UHPC_FR_UNIT, m_Entry.m_HaulingModulusOfRuptureCoefficient[pgsTypes::UHPC], pDisplayUnits->SqrtPressure);
   DDX_Text(pDX, IDC_UHPC_FR_UNIT, tag);

   DDX_UnitValueAndTag(pDX, IDC_HAULING_TENSION_CROWN, IDC_HAULING_TENSION_CROWN_UNIT, m_Entry.m_TensStressHaulingNormalCrown, pDisplayUnits->SqrtPressure );
   DDX_Text(pDX,IDC_HAULING_TENSION_CROWN_UNIT,tag);
   DDV_UnitValueZeroOrMore(pDX, IDC_HAULING_TENSION_CROWN_UNIT,m_Entry.m_TensStressHaulingNormalCrown, pDisplayUnits->SqrtPressure );
   DDX_Check_Bool(pDX, IDC_CHECK_HAULING_TENSION_MAX_CROWN, m_Entry.m_DoTensStressHaulingMaxNormalCrown);
   DDX_UnitValueAndTag(pDX, IDC_HAULING_TENSION_MAX_CROWN, IDC_HAULING_TENSION_MAX_UNIT_CROWN, m_Entry.m_TensStressHaulingMaxNormalCrown, pDisplayUnits->Stress );
   if (m_Entry.m_DoTensStressHaulingMaxNormalCrown)
   {
      DDV_UnitValueGreaterThanZero(pDX, IDC_HAULING_TENSION_MAX_CROWN,m_Entry.m_TensStressHaulingMaxNormalCrown, pDisplayUnits->Stress );
   }

   DDX_UnitValueAndTag(pDX, IDC_HAULING_TENSION_WITH_REBAR_CROWN, IDC_HAULING_TENSION_WITH_REBAR_UNIT_CROWN, m_Entry.m_TensStressHaulingWithRebarNormalCrown, pDisplayUnits->SqrtPressure  );
   DDX_Text(pDX,IDC_HAULING_TENSION_WITH_REBAR_UNIT_CROWN,tag);
   DDV_UnitValueZeroOrMore(pDX, IDC_HAULING_TENSION_WITH_REBAR_CROWN,m_Entry.m_TensStressHaulingWithRebarNormalCrown, pDisplayUnits->SqrtPressure );

   if (pDX->m_bSaveAndValidate && m_Entry.m_TensStressHaulingWithRebarNormalCrown < m_Entry.m_TensStressHaulingNormalCrown)
   {
      AfxMessageBox(_T("Allowable tensile stress with bonded reinforcement for the normal crown case must be greater than or equal to that without"),MB_OK | MB_ICONWARNING);
      pDX->Fail();
   }


   
   DDX_UnitValueAndTag(pDX, IDC_HAULING_TENSION_SUPER, IDC_HAULING_TENSION_SUPER_UNIT, m_Entry.m_TensStressHaulingMaxSuper, pDisplayUnits->SqrtPressure );
   DDX_Text(pDX,IDC_HAULING_TENSION_SUPER_UNIT,tag);
   DDV_UnitValueZeroOrMore(pDX, IDC_HAULING_TENSION_SUPER_UNIT,m_Entry.m_TensStressHaulingMaxSuper, pDisplayUnits->SqrtPressure );
   DDX_Check_Bool(pDX, IDC_CHECK_HAULING_TENSION_MAX_SUPER, m_Entry.m_DoTensStressHaulingMaxMaxSuper);
   DDX_UnitValueAndTag(pDX, IDC_HAULING_TENSION_MAX_SUPER, IDC_HAULING_TENSION_MAX_UNIT_SUPER, m_Entry.m_TensStressHaulingMaxMaxSuper, pDisplayUnits->Stress );
   if (m_Entry.m_DoTensStressHaulingMaxMaxSuper)
   {
      DDV_UnitValueGreaterThanZero(pDX, IDC_HAULING_TENSION_MAX_SUPER,m_Entry.m_TensStressHaulingMaxMaxSuper, pDisplayUnits->Stress );
   }

   DDX_UnitValueAndTag(pDX, IDC_HAULING_TENSION_WITH_REBAR_SUPER, IDC_HAULING_TENSION_WITH_REBAR_UNIT_SUPER, m_Entry.m_TensStressHaulingWithRebarMaxSuper, pDisplayUnits->SqrtPressure  );
   DDX_Text(pDX,IDC_HAULING_TENSION_WITH_REBAR_UNIT_SUPER,tag);
   DDV_UnitValueZeroOrMore(pDX, IDC_HAULING_TENSION_WITH_REBAR_SUPER,m_Entry.m_TensStressHaulingWithRebarMaxSuper, pDisplayUnits->SqrtPressure );

   if (pDX->m_bSaveAndValidate && m_Entry.m_TensStressHaulingWithRebarMaxSuper < m_Entry.m_TensStressHaulingMaxSuper)
   {
      AfxMessageBox(_T("Allowable tensile stress with bonded reinforcement for the max. superelevation case must be greater than or equal to that without"),MB_OK | MB_ICONWARNING);
      pDX->Fail();
   }

   // Haul Criteria
	DDX_Percentage(pDX, IDC_IMPACT_UPWARD_HAULING, m_Entry.m_HaulingUpwardImpact);
   DDV_MinMaxDouble(pDX, m_Entry.m_HaulingUpwardImpact, 0.0, 1.0);

   DDX_Percentage(pDX, IDC_IMPACT_DOWNWARD_HAULING, m_Entry.m_HaulingDownwardImpact);
   DDV_MinMaxDouble(pDX, m_Entry.m_HaulingDownwardImpact, 0.0, 1.0);

   DDX_CBItemData(pDX, IDC_IMPACT_USAGE, m_Entry.m_HaulingImpactUsage);

   CString slope_unit(pApp->GetUnitsMode() == eafTypes::umSI ? _T("m/m") : _T("ft/ft"));

   DDX_Text(pDX, IDC_CROWN_SLOPE, m_Entry.m_RoadwayCrownSlope);
   DDX_Text(pDX, IDC_CROWN_SLOPE_UNIT, slope_unit);

   DDX_Text(pDX, IDC_HE_ROADWAY_SUPERELEVATION, m_Entry.m_RoadwaySuperelevation);
   DDX_Text(pDX, IDC_HE_ROADWAY_SUPERELEVATION_UNIT, slope_unit);

   Float64 sweepTolerance = m_Entry.m_MaxGirderSweepHauling;
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
   m_Entry.m_MaxGirderSweepHauling = sweepTolerance;

   DDX_UnitValueAndTag(pDX, IDC_SWEEP_GROWTH, IDC_SWEEP_GROWTH_UNIT, m_Entry.m_HaulingSweepGrowth, pDisplayUnits->ComponentDim);

   DDX_UnitValueAndTag(pDX, IDC_SUPPORT_PLACEMENT_TOLERANCE, IDC_SUPPORT_PLACEMENT_TOLERANCE_UNITS, m_Entry.m_HaulingSupportPlacementTolerance, pDisplayUnits->ComponentDim );
   DDV_UnitValueGreaterThanZero(pDX, IDC_SUPPORT_PLACEMENT_TOLERANCE,m_Entry.m_HaulingSupportPlacementTolerance, pDisplayUnits->ComponentDim );

   DDX_Text(pDX, IDC_CAMBER_MULTIPLIER, m_Entry.m_HaulingCamberMultiplier);


   DDX_CBItemData(pDX,IDC_WIND_TYPE,m_Entry.m_HaulingWindType);
   if ( m_Entry.m_HaulingWindType == pgsTypes::Pressure )
   {
      DDX_UnitValueAndTag(pDX,IDC_WIND_LOAD,IDC_WIND_LOAD_UNIT,m_Entry.m_HaulingWindLoad,pDisplayUnits->WindPressure);
   }
   else
   {
      DDX_UnitValueAndTag(pDX,IDC_WIND_LOAD,IDC_WIND_LOAD_UNIT,m_Entry.m_HaulingWindLoad,pDisplayUnits->Velocity);
   }
   DDV_NonNegativeDouble(pDX, IDC_WIND_LOAD, m_Entry.m_HaulingWindLoad);

   DDX_CBItemData(pDX,IDC_CF_TYPE,m_Entry.m_CentrifugalForceType);
   DDX_UnitValueAndTag(pDX,IDC_HAUL_SPEED,IDC_HAUL_SPEED_UNIT,m_Entry.m_HaulingSpeed,pDisplayUnits->Velocity);
   DDV_NonNegativeDouble(pDX, IDC_WIND_LOAD, m_Entry.m_HaulingSpeed);

   DDX_UnitValueAndTag(pDX,IDC_RADIUS,IDC_RADIUS_UNIT,m_Entry.m_TurningRadius,pDisplayUnits->SpanLength);
   DDV_UnitValueGreaterThanZero(pDX, IDC_RADIUS,m_Entry.m_TurningRadius, pDisplayUnits->SpanLength );
}

void CSpecMainSheet::ExchangeKdotHaulingData(CDataExchange* pDX)
{
   CEAFApp* pApp = EAFGetApp();
   const unitmgtIndirectMeasure* pDisplayUnits = pApp->GetDisplayUnits();

   if (pDX->m_bSaveAndValidate)
   {
      ATLASSERT(m_SpecHaulingErectionPage.m_HaulingAnalysisMethod==pgsTypes::hmKDOT);
      m_Entry.m_HaulingAnalysisMethod = pgsTypes::hmKDOT;
   }

	DDX_Text(pDX, IDC_HAULING_COMPRESSION, m_Entry.m_GlobalCompStressHauling);
   DDV_GreaterThanZero(pDX, IDC_HAULING_COMPRESSION, m_Entry.m_GlobalCompStressHauling);

   CString tag;
   if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::SeventhEditionWith2016Interims )
   {
      tag = pApp->GetUnitsMode() == eafTypes::umSI ? _T("sqrt(f'c (MPa))") : _T("sqrt(f'c (KSI))");
   }
   else
   {
      tag = pApp->GetUnitsMode() == eafTypes::umSI ? _T("(lambda)sqrt(f'c (MPa))") : _T("(lambda)sqrt(f'c (KSI))");
   }

   // Use the normal crown values for KDOT
   DDX_UnitValueAndTag(pDX, IDC_HAULING_TENSION, IDC_HAULING_TENSION_UNIT, m_Entry.m_TensStressHaulingNormalCrown, pDisplayUnits->SqrtPressure );
   DDX_Text(pDX,IDC_HAULING_TENSION_UNIT,tag);
   DDV_UnitValueZeroOrMore(pDX, IDC_HAULING_TENSION_UNIT,m_Entry.m_TensStressHaulingNormalCrown, pDisplayUnits->SqrtPressure );
   DDX_Check_Bool(pDX, IDC_CHECK_HAULING_TENSION_MAX, m_Entry.m_DoTensStressHaulingMaxNormalCrown);
   DDX_UnitValueAndTag(pDX, IDC_HAULING_TENSION_MAX, IDC_HAULING_TENSION_MAX_UNIT, m_Entry.m_TensStressHaulingMaxNormalCrown, pDisplayUnits->Stress );
   if (m_Entry.m_DoTensStressHaulingMaxNormalCrown)
   {
      DDV_UnitValueGreaterThanZero(pDX, IDC_HAULING_TENSION_MAX,m_Entry.m_TensStressHaulingMaxNormalCrown, pDisplayUnits->Stress );
   }

   DDX_UnitValueAndTag(pDX, IDC_HAULING_TENSION_WITH_REBAR, IDC_HAULING_TENSION_WITH_REBAR_UNIT, m_Entry.m_TensStressHaulingWithRebarNormalCrown, pDisplayUnits->SqrtPressure  );
   DDX_Text(pDX,IDC_HAULING_TENSION_WITH_REBAR_UNIT,tag);
   DDV_UnitValueZeroOrMore(pDX, IDC_HAULING_TENSION_WITH_REBAR,m_Entry.m_TensStressHaulingWithRebarNormalCrown, pDisplayUnits->SqrtPressure );

   if (pDX->m_bSaveAndValidate && m_Entry.m_TensStressHaulingWithRebarNormalCrown < m_Entry.m_TensStressHaulingNormalCrown)
   {
      AfxMessageBox(_T("Allowable tensile stress with bonded reinforcement must be greater than or equal to that without"),MB_OK | MB_ICONWARNING);
      pDX->Fail();
   }

	DDX_Text(pDX, IDC_G_OVERHANG, m_Entry.m_OverhangGFactor);
   DDV_MinMaxDouble(pDX, m_Entry.m_OverhangGFactor, 1.0, 100.0);
	DDX_Text(pDX, IDC_G_INTERIOR, m_Entry.m_InteriorGFactor);
   DDV_MinMaxDouble(pDX, m_Entry.m_InteriorGFactor, 1.0, 100.0);
}


void CSpecMainSheet::ExchangeMomentCapacityData(CDataExchange* pDX)
{
   CEAFApp* pApp = EAFGetApp();
   const unitmgtIndirectMeasure* pDisplayUnits = pApp->GetDisplayUnits();

   DDX_CBIndex(pDX, IDC_MOMENT, m_Entry.m_Bs3LRFDOverReinforcedMomentCapacity );
   DDX_CBItemData(pDX, IDC_NEG_MOMENT, m_Entry.m_bIncludeForNegMoment);
   DDX_Check_Bool(pDX, IDC_INCLUDE_STRAND_FOR_NEG_MOMENT, m_Entry.m_bIncludeStrand_NegMoment);
   DDX_Check_Bool(pDX, IDC_INCLUDE_REBAR_MOMENT, m_Entry.m_bIncludeRebar_Moment );

   CString tag;
   if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::SeventhEditionWith2016Interims )
   {
      tag = pApp->GetUnitsMode() == eafTypes::umSI ? _T("sqrt(f'c (MPa))") : _T("sqrt(f'c (KSI))");
   }
   else
   {
      tag = pApp->GetUnitsMode() == eafTypes::umSI ? _T("(lambda)sqrt(f'c (MPa))") : _T("(lambda)sqrt(f'c (KSI))");
   }

   DDX_UnitValueAndTag(pDX, IDC_FR,      IDC_FR_LABEL,      m_Entry.m_FlexureModulusOfRuptureCoefficient[pgsTypes::Normal],          pDisplayUnits->SqrtPressure );
   DDX_UnitValueAndTag(pDX, IDC_ALWC_FR, IDC_ALWC_FR_LABEL, m_Entry.m_FlexureModulusOfRuptureCoefficient[pgsTypes::AllLightweight],  pDisplayUnits->SqrtPressure );
   DDX_UnitValueAndTag(pDX, IDC_SLWC_FR, IDC_SLWC_FR_LABEL, m_Entry.m_FlexureModulusOfRuptureCoefficient[pgsTypes::SandLightweight], pDisplayUnits->SqrtPressure );
   DDX_UnitValueAndTag(pDX, IDC_UHPC_FR, IDC_UHPC_FR_LABEL, m_Entry.m_FlexureModulusOfRuptureCoefficient[pgsTypes::UHPC],            pDisplayUnits->SqrtPressure);

   DDX_Text(pDX, IDC_FR_UNIT,     tag);
   DDX_Text(pDX, IDC_ALWC_FR_UNIT,tag);
   DDX_Text(pDX, IDC_SLWC_FR_UNIT,tag);
   DDX_Text(pDX, IDC_UHPC_FR_UNIT, tag);

   DDV_UnitValueZeroOrMore(pDX, IDC_FR,      m_Entry.m_FlexureModulusOfRuptureCoefficient[pgsTypes::Normal],          pDisplayUnits->SqrtPressure );
   DDV_UnitValueZeroOrMore(pDX, IDC_ALWC_FR, m_Entry.m_FlexureModulusOfRuptureCoefficient[pgsTypes::AllLightweight],  pDisplayUnits->SqrtPressure );
   DDV_UnitValueZeroOrMore(pDX, IDC_SLWC_FR, m_Entry.m_FlexureModulusOfRuptureCoefficient[pgsTypes::SandLightweight], pDisplayUnits->SqrtPressure );
   DDV_UnitValueZeroOrMore(pDX, IDC_UHPC_FR, m_Entry.m_FlexureModulusOfRuptureCoefficient[pgsTypes::UHPC],            pDisplayUnits->SqrtPressure);

   // NOTE: this looks goofy, but it is correct. There is only one LWC entry for both all and sand lightweight
   // but it is easier to have 3 sets of values so the application is consistent.
   DDX_Text(pDX,IDC_NWC_PHI_TENSION_RC,      m_Entry.m_PhiFlexureTensionRC[pgsTypes::Normal]);
   DDX_Text(pDX,IDC_NWC_PHI_TENSION_PS,      m_Entry.m_PhiFlexureTensionPS[pgsTypes::Normal]);
   DDX_Text(pDX,IDC_NWC_PHI_TENSION_SPLICED, m_Entry.m_PhiFlexureTensionSpliced[pgsTypes::Normal]);
   DDX_Text(pDX,IDC_NWC_PHI_COMPRESSION,     m_Entry.m_PhiFlexureCompression[pgsTypes::Normal]);

   DDX_Text(pDX,IDC_LWC_PHI_TENSION_RC,      m_Entry.m_PhiFlexureTensionRC[pgsTypes::AllLightweight]);
   DDX_Text(pDX,IDC_LWC_PHI_TENSION_PS,      m_Entry.m_PhiFlexureTensionPS[pgsTypes::AllLightweight]);
   DDX_Text(pDX,IDC_LWC_PHI_TENSION_SPLICED, m_Entry.m_PhiFlexureTensionSpliced[pgsTypes::AllLightweight]);
   DDX_Text(pDX,IDC_LWC_PHI_COMPRESSION,     m_Entry.m_PhiFlexureCompression[pgsTypes::AllLightweight]);

   DDX_Text(pDX,IDC_LWC_PHI_TENSION_RC,      m_Entry.m_PhiFlexureTensionRC[pgsTypes::SandLightweight]);
   DDX_Text(pDX,IDC_LWC_PHI_TENSION_PS,      m_Entry.m_PhiFlexureTensionPS[pgsTypes::SandLightweight]);
   DDX_Text(pDX,IDC_LWC_PHI_TENSION_SPLICED, m_Entry.m_PhiFlexureTensionSpliced[pgsTypes::SandLightweight]);
   DDX_Text(pDX,IDC_LWC_PHI_COMPRESSION,     m_Entry.m_PhiFlexureCompression[pgsTypes::SandLightweight]);

   DDX_Text(pDX, IDC_UHPC_PHI_TENSION_RC,      m_Entry.m_PhiFlexureTensionRC[pgsTypes::UHPC]);
   DDX_Text(pDX, IDC_UHPC_PHI_TENSION_PS,      m_Entry.m_PhiFlexureTensionPS[pgsTypes::UHPC]);
   DDX_Text(pDX, IDC_UHPC_PHI_TENSION_SPLICED, m_Entry.m_PhiFlexureTensionSpliced[pgsTypes::UHPC]);
   DDX_Text(pDX, IDC_UHPC_PHI_COMPRESSION,     m_Entry.m_PhiFlexureCompression[pgsTypes::UHPC]);

   DDX_Text(pDX, IDC_NWC_JOINT_PHI,           m_Entry.m_PhiClosureJointFlexure[pgsTypes::Normal]);
   DDX_Text(pDX, IDC_LWC_JOINT_PHI,           m_Entry.m_PhiClosureJointFlexure[pgsTypes::AllLightweight]);
   DDX_Text(pDX, IDC_LWC_JOINT_PHI,           m_Entry.m_PhiClosureJointFlexure[pgsTypes::SandLightweight]);
   DDX_Text(pDX, IDC_UHPC_JOINT_PHI,          m_Entry.m_PhiClosureJointFlexure[pgsTypes::UHPC]);
}

void CSpecMainSheet::CheckShearCapacityMethod()
{
   // makes sure the shear capacity method is consistent with the specification version that is selected
   if ( GetSpecVersion() < lrfdVersionMgr::FourthEdition2007 &&  // if we are before 4th Edition
        m_Entry.m_ShearCapacityMethod == pgsTypes::scmVciVcw ) // Vci/Vcw is not a valid option
   {
      // force to the general method
      if ( GetSpecVersion() <= lrfdVersionMgr::FourthEdition2007 )
      {
         m_Entry.m_ShearCapacityMethod = pgsTypes::scmBTTables;
      }
      else
      {
         m_Entry.m_ShearCapacityMethod = pgsTypes::scmBTEquations;
      }
   }

   // The general method from the 2007 spec becomes the tables method in the 2008 spec
   // make that adjustment here
   if ( GetSpecVersion() < lrfdVersionMgr::FourthEditionWith2008Interims && m_Entry.m_ShearCapacityMethod == pgsTypes::scmBTEquations )
   {
      m_Entry.m_ShearCapacityMethod = pgsTypes::scmBTTables;
   }

   if ( GetSpecVersion() < lrfdVersionMgr::SecondEditionWith2000Interims &&  // if we are before 2nd Edition + 2000
        m_Entry.m_ShearCapacityMethod == pgsTypes::scmWSDOT2001 ) // WSDOT 2001 is not a valid option
   {
      // force to the general method
      if ( GetSpecVersion() <= lrfdVersionMgr::FourthEdition2007 )
      {
         m_Entry.m_ShearCapacityMethod = pgsTypes::scmBTTables;
      }
      else
      {
         m_Entry.m_ShearCapacityMethod = pgsTypes::scmBTEquations;
      }
   }

   if ( GetSpecVersion() < lrfdVersionMgr::FourthEdition2007 &&  // if we are before 4th Edition
        m_Entry.m_ShearCapacityMethod == pgsTypes::scmWSDOT2007 ) // WSDOT 2007 is not a valid option
   {
      m_Entry.m_ShearCapacityMethod = pgsTypes::scmWSDOT2001; // force to WSDOT 2001
   }
}

void CSpecMainSheet::ExchangeShearCapacityData(CDataExchange* pDX)
{
   CEAFApp* pApp = EAFGetApp();
   const unitmgtIndirectMeasure* pDisplayUnits = pApp->GetDisplayUnits();

   DDX_Check_Bool(pDX, IDC_LIMIT_STRAIN, m_Entry.m_bLimitNetTensionStrainToPositiveValues);

   DDX_CBIndex(pDX, IDC_LRSH, m_Entry.m_LongReinfShearMethod); 
   DDX_Check_Bool(pDX, IDC_INCLUDE_REBAR_SHEAR, m_Entry.m_bIncludeRebar_Shear );
   DDX_Check_Bool(pDX, IDC_USE_DECK_FOR_PC, m_Entry.m_bUseDeckWeightForPc);

   DDX_CBEnum(pDX,IDC_SHEAR_FLOW_METHOD,m_Entry.m_ShearFlowMethod);

   // we have to use item data here because the enum constants are out of order
   DDX_CBItemData(pDX, IDC_SHEAR_CAPACITY, m_Entry.m_ShearCapacityMethod );

   // Not every shear method is available with every spec. The user could have set the method
   // then changed the spec. We have keep the data valid

   if ( pDX->m_bSaveAndValidate )
   {
      CheckShearCapacityMethod();
   }

   CString tag;
   if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::SeventhEditionWith2016Interims )
   {
      tag = pApp->GetUnitsMode() == eafTypes::umSI ? _T("sqrt(f'c (MPa))") : _T("sqrt(f'c (KSI))");
   }
   else
   {
      tag = pApp->GetUnitsMode() == eafTypes::umSI ? _T("(lambda)sqrt(f'c (MPa))") : _T("(lambda)sqrt(f'c (KSI))");
   }

   DDX_UnitValueAndTag(pDX, IDC_FR,      IDC_FR_LABEL,      m_Entry.m_ShearModulusOfRuptureCoefficient[pgsTypes::Normal],          pDisplayUnits->SqrtPressure );
   DDX_UnitValueAndTag(pDX, IDC_ALWC_FR, IDC_FR_LABEL_ALWC, m_Entry.m_ShearModulusOfRuptureCoefficient[pgsTypes::AllLightweight],  pDisplayUnits->SqrtPressure );
   DDX_UnitValueAndTag(pDX, IDC_SLWC_FR, IDC_FR_LABEL_SLWC, m_Entry.m_ShearModulusOfRuptureCoefficient[pgsTypes::SandLightweight], pDisplayUnits->SqrtPressure );
   DDX_UnitValueAndTag(pDX, IDC_UHPC_FR, IDC_FR_LABEL_UHPC, m_Entry.m_ShearModulusOfRuptureCoefficient[pgsTypes::UHPC],            pDisplayUnits->SqrtPressure);

   DDX_Text(pDX, IDC_FR_UNIT,      tag);
   DDX_Text(pDX, IDC_ALWC_FR_UNIT, tag);
   DDX_Text(pDX, IDC_SLWC_FR_UNIT, tag);
   DDX_Text(pDX, IDC_UHPC_FR_UNIT, tag);

   DDV_UnitValueZeroOrMore(pDX, IDC_FR ,     m_Entry.m_ShearModulusOfRuptureCoefficient[pgsTypes::Normal],          pDisplayUnits->SqrtPressure );
   DDV_UnitValueZeroOrMore(pDX, IDC_SLWC_FR, m_Entry.m_ShearModulusOfRuptureCoefficient[pgsTypes::SandLightweight], pDisplayUnits->SqrtPressure );
   DDV_UnitValueZeroOrMore(pDX, IDC_ALWC_FR, m_Entry.m_ShearModulusOfRuptureCoefficient[pgsTypes::AllLightweight],  pDisplayUnits->SqrtPressure );
   DDV_UnitValueZeroOrMore(pDX, IDC_UHPC_FR, m_Entry.m_ShearModulusOfRuptureCoefficient[pgsTypes::UHPC],            pDisplayUnits->SqrtPressure);

   DDX_UnitValueAndTag(pDX, IDC_UHPC_FIBER_SHEAR_STRENGTH, IDC_UHPC_FIBER_SHEAR_STRENGTH_UNIT, m_Entry.m_UHPCFiberShearStrength, pDisplayUnits->Stress);

   DDX_Text(pDX,IDC_NWC_PHI,m_Entry.m_PhiShear[pgsTypes::Normal]);
   DDX_Text(pDX,IDC_LWC_PHI,m_Entry.m_PhiShear[pgsTypes::SandLightweight]);
   DDX_Text(pDX, IDC_LWC_PHI, m_Entry.m_PhiShear[pgsTypes::AllLightweight]);
   DDX_Text(pDX, IDC_UHPC_PHI, m_Entry.m_PhiShear[pgsTypes::UHPC]);

   DDX_Text(pDX,IDC_NWC_PHI_DEBOND,m_Entry.m_PhiShearDebonded[pgsTypes::Normal]);
   DDX_Text(pDX,IDC_LWC_PHI_DEBOND,m_Entry.m_PhiShearDebonded[pgsTypes::SandLightweight]);
   DDX_Text(pDX, IDC_LWC_PHI_DEBOND, m_Entry.m_PhiShearDebonded[pgsTypes::AllLightweight]);
   DDX_Text(pDX, IDC_UHPC_PHI_DEBOND, m_Entry.m_PhiShearDebonded[pgsTypes::UHPC]);

   DDX_Text(pDX,IDC_NWC_JOINT_PHI,  m_Entry.m_PhiClosureJointShear[pgsTypes::Normal]);
   DDX_Text(pDX,IDC_LWC_JOINT_PHI,  m_Entry.m_PhiClosureJointShear[pgsTypes::AllLightweight]);
   DDX_Text(pDX, IDC_LWC_JOINT_PHI, m_Entry.m_PhiClosureJointShear[pgsTypes::SandLightweight]);
   DDX_Text(pDX, IDC_UHPC_JOINT_PHI, m_Entry.m_PhiClosureJointShear[pgsTypes::UHPC]);

   DDX_UnitValueAndTag(pDX, IDC_SMAX, IDC_SMAX_UNIT, m_Entry.m_MaxInterfaceShearConnectorSpacing, pDisplayUnits->ComponentDim );

   DDX_Text(pDX, IDC_SPACING_COEFFICIENT_1, m_Entry.m_StirrupSpacingCoefficient[0]);
   DDX_UnitValueAndTag(pDX, IDC_MAX_SPACING_1, IDC_MAX_SPACING_1_UNIT, m_Entry.m_MaxStirrupSpacing[0], pDisplayUnits->ComponentDim );
   DDX_Text(pDX, IDC_SPACING_COEFFICIENT_2, m_Entry.m_StirrupSpacingCoefficient[1]);
   DDX_UnitValueAndTag(pDX, IDC_MAX_SPACING_2, IDC_MAX_SPACING_2_UNIT, m_Entry.m_MaxStirrupSpacing[1], pDisplayUnits->ComponentDim );

   DDV_UnitValueGreaterThanZero(pDX, IDC_MAX_SPACING_1, m_Entry.m_MaxStirrupSpacing[0], pDisplayUnits->ComponentDim );
   DDV_UnitValueGreaterThanZero(pDX, IDC_MAX_SPACING_2, m_Entry.m_MaxStirrupSpacing[1], pDisplayUnits->ComponentDim );
}

void CSpecMainSheet::ExchangeCreepData(CDataExchange* pDX)
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   CEAFApp* pApp = EAFGetApp();
   const unitmgtIndirectMeasure* pDisplayUnits = pApp->GetDisplayUnits();

   DDX_UnitValueAndTag(pDX, IDC_XFER_TIME, IDC_XFER_TIME_TAG, m_Entry.m_XferTime, pDisplayUnits->Time );
   DDV_UnitValueGreaterThanZero(pDX, IDC_XFER_TIME,m_Entry.m_XferTime, pDisplayUnits->Time );

   DDX_UnitValueAndTag(pDX, IDC_CREEP_DURATION1_MIN, IDC_CREEP_DURATION1_TAG, m_Entry.m_CreepDuration1Min, pDisplayUnits->Time2 );
   DDV_UnitValueGreaterThanZero(pDX, IDC_CREEP_DURATION1_MIN,m_Entry.m_CreepDuration1Min, pDisplayUnits->Time2 );

   DDX_UnitValueAndTag(pDX, IDC_CREEP_DURATION1_MAX, IDC_CREEP_DURATION1_TAG, m_Entry.m_CreepDuration1Max, pDisplayUnits->Time2 );
   DDV_UnitValueLimitOrMore(pDX, IDC_CREEP_DURATION1_MAX,m_Entry.m_CreepDuration1Max, m_Entry.m_CreepDuration1Min, pDisplayUnits->Time2 );

   DDX_UnitValueAndTag(pDX, IDC_CREEP_DURATION2_MIN, IDC_CREEP_DURATION2_TAG, m_Entry.m_CreepDuration2Min, pDisplayUnits->Time2 );
   DDV_UnitValueGreaterThanLimit(pDX, IDC_CREEP_DURATION2_MIN,m_Entry.m_CreepDuration2Min, m_Entry.m_XferTime, pDisplayUnits->Time2 );

   DDX_UnitValueAndTag(pDX, IDC_CREEP_DURATION2_MAX, IDC_CREEP_DURATION2_TAG, m_Entry.m_CreepDuration2Max, pDisplayUnits->Time2 );
   DDV_UnitValueLimitOrMore(pDX, IDC_CREEP_DURATION2_MAX,m_Entry.m_CreepDuration2Max, m_Entry.m_CreepDuration2Min, pDisplayUnits->Time2 );

   if ( pDX->m_bSaveAndValidate )
   {
      if ( m_Entry.m_CreepDuration2Min < m_Entry.m_CreepDuration1Min )
      {
         pDX->PrepareEditCtrl(IDC_CREEP_DURATION2_MIN);
         AfxMessageBox(_T("The time from prestress transfer to deck casting must be greater than the time from prestress transfer to temporary strand removal/diaphragm casting."));
         pDX->Fail();
      }

      if ( m_Entry.m_CreepDuration2Max < m_Entry.m_CreepDuration1Max )
      {
         pDX->PrepareEditCtrl(IDC_CREEP_DURATION2_MAX);
         AfxMessageBox(_T("The time from prestress transfer to deck casting must be greater than the time from prestress transfer to temporary strand removal/diaphragm casting."));
         pDX->Fail();
      }
   }

   DDX_UnitValueAndTag(pDX, IDC_NC_CREEP, IDC_NC_CREEP_TAG, m_Entry.m_TotalCreepDuration, pDisplayUnits->Time2 );
   DDV_UnitValueLimitOrMore(pDX, IDC_NC_CREEP,m_Entry.m_TotalCreepDuration, m_Entry.m_CreepDuration2Min, pDisplayUnits->Time2 );

   DDX_Radio(pDX,IDC_CURING_METHOD,m_Entry.m_CuringMethod);

   DDX_Text(pDX,IDC_CURING_TIME_FACTOR,m_Entry.m_CuringMethodTimeAdjustmentFactor);
   DDV_MinMaxDouble(pDX,m_Entry.m_CuringMethodTimeAdjustmentFactor,1,999);

   DDX_Percentage(pDX,IDC_CAMBER_VARIABILITY, m_Entry.m_CamberVariability);
   if ( pDX->m_bSaveAndValidate )
   {
      Float64 val = m_Entry.m_CamberVariability * 100.0;
      DDV_Range( pDX, mfcDDV::LE,mfcDDV::GE,val,0.0,100.0);
   }

   // computation of haunch load
   DDX_CBEnum(pDX, IDC_HAUNCH_COMP_CB,m_Entry.m_HaunchLoadComputationType);
   DDX_UnitValueAndTag(pDX, IDC_HAUNCH_TOLER, IDC_HAUNCH_TOLER_UNIT, m_Entry.m_HaunchLoadCamberTolerance, pDisplayUnits->ComponentDim);
   DDV_UnitValueGreaterThanZero(pDX, IDC_HAUNCH_TOLER,m_Entry.m_HaunchLoadCamberTolerance, pDisplayUnits->ComponentDim );

   DDX_Percentage(pDX, IDC_HAUNCH_FACTOR, m_Entry.m_HaunchLoadCamberFactor);
   if (pDX->m_bSaveAndValidate)
   {
      if (0.0 >= m_Entry.m_HaunchLoadCamberFactor)
      {
         ::AfxMessageBox(_T("Haunch load camber factor must be greater than zero. If you want zero camber, select the flat girder (zero camber) option"));
         pDX->Fail();
      }
      else if (1.0 < m_Entry.m_HaunchLoadCamberFactor)
      {
         ::AfxMessageBox(_T("Haunch load camber factor must be less than or equal to 100%"));
         pDX->Fail();
      }
   }   

   // computation of haunch composite section properties
   DDX_CBEnum(pDX, IDC_HAUNCH_COMP_PROPS_CB,m_Entry.m_HaunchAnalysisSectionPropertiesType);
}

void CSpecMainSheet::ExchangeLossData(CDataExchange* pDX)
{
   CEAFApp* pApp = EAFGetApp();
   const unitmgtIndirectMeasure* pDisplayUnits = pApp->GetDisplayUnits();

   DDX_UnitValueAndTag(pDX, IDC_SHIPPING_TIME, IDC_SHIPPING_TIME_TAG, m_Entry.m_ShippingTime, pDisplayUnits->Time2);

   DDX_Percentage(pDX,IDC_EG_SLAB,m_Entry.m_SlabElasticGain);
   DDX_Percentage(pDX,IDC_EG_SLABPAD,m_Entry.m_SlabPadElasticGain);
   DDX_Percentage(pDX,IDC_EG_DIAPHRAGM,m_Entry.m_DiaphragmElasticGain);
   DDX_Percentage(pDX,IDC_EG_DC_BS2,m_Entry.m_UserDCElasticGainBS1);
   DDX_Percentage(pDX,IDC_EG_DW_BS2,m_Entry.m_UserDWElasticGainBS1);
   DDX_Percentage(pDX,IDC_EG_DC_BS3,m_Entry.m_UserDCElasticGainBS2);
   DDX_Percentage(pDX,IDC_EG_DW_BS3,m_Entry.m_UserDWElasticGainBS2);
   DDX_Percentage(pDX,IDC_EG_RAILING,m_Entry.m_RailingSystemElasticGain);
   DDX_Percentage(pDX,IDC_EG_OVERLAY,m_Entry.m_OverlayElasticGain);
   DDX_Percentage(pDX,IDC_EG_SHRINKAGE,m_Entry.m_SlabShrinkageElasticGain);
   DDX_Percentage(pDX,IDC_EG_LIVELOAD,m_Entry.m_LiveLoadElasticGain);

   DDX_CBEnum(pDX,IDC_RELAXATION_LOSS_METHOD,m_Entry.m_RelaxationLossMethod);
   DDX_CBEnum(pDX,IDC_FCPG_COMBO,m_Entry.m_FcgpComputationMethod);

   // have to map loss method to comb box ordering in dialog
   int map[] = {LOSSES_AASHTO_REFINED,
                LOSSES_WSDOT_REFINED,
                LOSSES_TXDOT_REFINED_2004,
                LOSSES_TXDOT_REFINED_2013,
                LOSSES_AASHTO_LUMPSUM,
                LOSSES_WSDOT_LUMPSUM,
                LOSSES_TIME_STEP};

   int map_size = sizeof(map)/sizeof(int);

   DDX_CBItemData(pDX,IDC_TIME_DEPENDENT_MODEL,m_Entry.m_TimeDependentModel);

   if ( pDX->m_bSaveAndValidate )
   {
      // data is coming out of the dialog
      int rad_ord;
      DDX_CBIndex(pDX,IDC_LOSS_METHOD,rad_ord);
      ATLASSERT(0 <= rad_ord && rad_ord < map_size);
      m_Entry.m_LossMethod = map[rad_ord];

      CComPtr<IBroker> pBroker;
      EAFGetBroker(&pBroker);
      if ( pBroker )
      {
         CComPtr<IDocumentType> pDocType;
         pBroker->GetInterface(IID_IDocumentType,(IUnknown**)&pDocType);
         if ( pDocType->IsPGSpliceDocument() && m_Entry.m_LossMethod != LOSSES_TIME_STEP && 0 <  m_Entry.GetRefCount() )
         {
            AfxMessageBox(_T("Time-step method must be selected for spliced girder bridges"));
            pDX->PrepareCtrl(IDC_LOSS_METHOD);
            pDX->Fail();
         }
      }

      if (m_Entry.m_SectionPropertyMode == pgsTypes::spmGross && m_Entry.m_LossMethod == LOSSES_TIME_STEP )
      {
         int result = AfxMessageBox(_T("Time-step method can only be used with transformed section properties.\n\nWould you like to change the section properties to transformed?"),MB_YESNO);
         if (result == IDYES )
         {
            m_Entry.m_SectionPropertyMode = pgsTypes::spmTransformed;
         }
         else
         {
            pDX->PrepareCtrl(IDC_LOSS_METHOD);
            pDX->Fail();
         }
      }

      if ( m_Entry.m_LossMethod == LOSSES_AASHTO_REFINED || m_Entry.m_LossMethod == LOSSES_WSDOT_REFINED || 
           m_Entry.m_LossMethod == LOSSES_AASHTO_LUMPSUM || m_Entry.m_LossMethod == LOSSES_WSDOT_LUMPSUM ||
           m_Entry.m_LossMethod == LOSSES_TXDOT_REFINED_2004 || m_Entry.m_LossMethod == LOSSES_TXDOT_REFINED_2013)
      {
         DDX_CBIndex(pDX,IDC_SHIPPING_LOSS_METHOD,rad_ord);
         if( rad_ord == 0 )
         {
            // Lump sum shipping loss
      	   DDX_UnitValueAndTag(pDX, IDC_SHIPPING, IDC_SHIPPING_TAG, m_Entry.m_ShippingLosses, pDisplayUnits->Stress);
         }
         else
         {
            // Fractional
            Float64 value;
            DDX_Text(pDX,IDC_SHIPPING,value);
            value /= -100.0;
            m_Entry.m_ShippingLosses = value;
         }
      }
      else
      {
         m_Entry.m_ShippingLosses = 0;
      }
   }
   else
   {
      // Data is going into the dialog
      // check buttons - map to loss method
      int idx=0;
      while(m_Entry.m_LossMethod != map[idx])
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
      if ( m_Entry.m_ShippingLosses < 0 )
      {
         /// Shipping losses are fractional
         Float64 value = m_Entry.m_ShippingLosses * -100.0;
         DDX_Text(pDX,IDC_SHIPPING,value);

         CString strTag(_T("%"));
         DDX_Text(pDX,IDC_SHIPPING_TAG,strTag);

         idx = 1;
         DDX_CBIndex(pDX,IDC_SHIPPING_LOSS_METHOD,idx);
      }
      else
      {
   	   DDX_UnitValueAndTag(pDX, IDC_SHIPPING,  IDC_SHIPPING_TAG,  m_Entry.m_ShippingLosses, pDisplayUnits->Stress);

         idx = 0;
         DDX_CBIndex(pDX,IDC_SHIPPING_LOSS_METHOD,idx);
      }
   }
}

void CSpecMainSheet::ExchangeStrandData(CDataExchange* pDX)
{
   // Strand Data
   DDX_Check_Bool(pDX, IDC_CHECK_PS_AT_JACKING, m_Entry.m_bCheckStrandStress[CSS_AT_JACKING]);
 	DDX_Text(pDX, IDC_PS_AT_JACKING_SR, m_Entry.m_StrandStressCoeff[CSS_AT_JACKING][STRESS_REL]);
   DDV_GreaterThanZero(pDX, IDC_PS_AT_JACKING_SR, m_Entry.m_StrandStressCoeff[CSS_AT_JACKING][STRESS_REL]);
 	DDX_Text(pDX, IDC_PS_AT_JACKING_LR, m_Entry.m_StrandStressCoeff[CSS_AT_JACKING][LOW_RELAX]);
   DDV_GreaterThanZero(pDX, IDC_PS_AT_JACKING_LR, m_Entry.m_StrandStressCoeff[CSS_AT_JACKING][LOW_RELAX]);

   DDX_Check_Bool(pDX, IDC_CHECK_PS_BEFORE_TRANSFER, m_Entry.m_bCheckStrandStress[CSS_BEFORE_TRANSFER]);
 	DDX_Text(pDX, IDC_PS_BEFORE_TRANSFER_SR, m_Entry.m_StrandStressCoeff[CSS_BEFORE_TRANSFER][STRESS_REL]);
   DDV_GreaterThanZero(pDX, IDC_PS_BEFORE_TRANSFER_SR, m_Entry.m_StrandStressCoeff[CSS_BEFORE_TRANSFER][STRESS_REL]);
 	DDX_Text(pDX, IDC_PS_BEFORE_TRANSFER_LR, m_Entry.m_StrandStressCoeff[CSS_BEFORE_TRANSFER][LOW_RELAX]);
   DDV_GreaterThanZero(pDX, IDC_PS_BEFORE_TRANSFER_LR, m_Entry.m_StrandStressCoeff[CSS_BEFORE_TRANSFER][LOW_RELAX]);

   DDX_Check_Bool(pDX, IDC_CHECK_PS_AFTER_TRANSFER, m_Entry.m_bCheckStrandStress[CSS_AFTER_TRANSFER]);
 	DDX_Text(pDX, IDC_PS_AFTER_TRANSFER_SR, m_Entry.m_StrandStressCoeff[CSS_AFTER_TRANSFER][STRESS_REL]);
   DDV_GreaterThanZero(pDX, IDC_PS_AFTER_TRANSFER_SR, m_Entry.m_StrandStressCoeff[CSS_AFTER_TRANSFER][STRESS_REL]);
 	DDX_Text(pDX, IDC_PS_AFTER_TRANSFER_LR, m_Entry.m_StrandStressCoeff[CSS_AFTER_TRANSFER][LOW_RELAX]);
   DDV_GreaterThanZero(pDX, IDC_PS_AFTER_TRANSFER_LR, m_Entry.m_StrandStressCoeff[CSS_AFTER_TRANSFER][LOW_RELAX]);

   m_Entry.m_bCheckStrandStress[CSS_AFTER_ALL_LOSSES] = true;
 	DDX_Text(pDX, IDC_PS_AFTER_ALL_LOSSES_SR, m_Entry.m_StrandStressCoeff[CSS_AFTER_ALL_LOSSES][STRESS_REL]);
   DDV_GreaterThanZero(pDX, IDC_PS_AFTER_ALL_LOSSES_SR, m_Entry.m_StrandStressCoeff[CSS_AFTER_ALL_LOSSES][STRESS_REL]);
 	DDX_Text(pDX, IDC_PS_AFTER_ALL_LOSSES_LR, m_Entry.m_StrandStressCoeff[CSS_AFTER_ALL_LOSSES][LOW_RELAX]);
   DDV_GreaterThanZero(pDX, IDC_PS_AFTER_ALL_LOSSES_LR, m_Entry.m_StrandStressCoeff[CSS_AFTER_ALL_LOSSES][LOW_RELAX]);

   // Tendon Data
   DDX_Check_Bool(pDX, IDC_CHECK_PT_AT_JACKING, m_Entry.m_bCheckTendonStressAtJacking);
   DDX_Text(pDX,IDC_PT_AT_JACKING_SR,m_Entry.m_TendonStressCoeff[CSS_AT_JACKING][STRESS_REL]);
   DDV_GreaterThanZero(pDX, IDC_PT_AT_JACKING_SR, m_Entry.m_TendonStressCoeff[CSS_AT_JACKING][STRESS_REL]);
   DDX_Text(pDX,IDC_PT_AT_JACKING_LR,m_Entry.m_TendonStressCoeff[CSS_AT_JACKING][LOW_RELAX]);
   DDV_GreaterThanZero(pDX, IDC_PT_AT_JACKING_LR, m_Entry.m_TendonStressCoeff[CSS_AT_JACKING][LOW_RELAX]);

   DDX_Check_Bool(pDX, IDC_CHECK_PT_BEFORE_TRANSFER, m_Entry.m_bCheckTendonStressPriorToSeating);
   DDX_Text(pDX,IDC_PT_BEFORE_TRANSFER_SR,m_Entry.m_TendonStressCoeff[CSS_PRIOR_TO_SEATING][STRESS_REL]);
   DDV_GreaterThanZero(pDX, IDC_PT_BEFORE_TRANSFER_SR, m_Entry.m_TendonStressCoeff[CSS_PRIOR_TO_SEATING][STRESS_REL]);
   DDX_Text(pDX,IDC_PT_BEFORE_TRANSFER_LR,m_Entry.m_TendonStressCoeff[CSS_PRIOR_TO_SEATING][LOW_RELAX]);
   DDV_GreaterThanZero(pDX, IDC_PT_BEFORE_TRANSFER_LR, m_Entry.m_TendonStressCoeff[CSS_PRIOR_TO_SEATING][LOW_RELAX]);

   DDX_Text(pDX,IDC_PT_ANCHORAGE_AFTER_ANCHORSET_SR,m_Entry.m_TendonStressCoeff[CSS_ANCHORAGES_AFTER_SEATING][STRESS_REL]);
   DDV_GreaterThanZero(pDX, IDC_PT_ANCHORAGE_AFTER_ANCHORSET_SR, m_Entry.m_TendonStressCoeff[CSS_ANCHORAGES_AFTER_SEATING][STRESS_REL]);
   DDX_Text(pDX,IDC_PT_ANCHORAGE_AFTER_ANCHORSET_LR,m_Entry.m_TendonStressCoeff[CSS_ANCHORAGES_AFTER_SEATING][LOW_RELAX]);
   DDV_GreaterThanZero(pDX, IDC_PT_ANCHORAGE_AFTER_ANCHORSET_LR, m_Entry.m_TendonStressCoeff[CSS_ANCHORAGES_AFTER_SEATING][LOW_RELAX]);

   DDX_Text(pDX,IDC_PT_ELSEWHERE_AFTER_ANCHORSET_SR,m_Entry.m_TendonStressCoeff[CSS_ELSEWHERE_AFTER_SEATING][STRESS_REL]);
   DDV_GreaterThanZero(pDX, IDC_PT_ELSEWHERE_AFTER_ANCHORSET_SR, m_Entry.m_TendonStressCoeff[CSS_ELSEWHERE_AFTER_SEATING][STRESS_REL]);
   DDX_Text(pDX,IDC_PT_ELSEWHERE_AFTER_ANCHORSET_LR,m_Entry.m_TendonStressCoeff[CSS_ELSEWHERE_AFTER_SEATING][LOW_RELAX]);
   DDV_GreaterThanZero(pDX, IDC_PT_ELSEWHERE_AFTER_ANCHORSET_LR, m_Entry.m_TendonStressCoeff[CSS_ELSEWHERE_AFTER_SEATING][LOW_RELAX]);

   DDX_Text(pDX,IDC_PT_AFTER_ALL_LOSSES_SR,m_Entry.m_TendonStressCoeff[CSS_AFTER_ALL_LOSSES][STRESS_REL]);
   DDV_GreaterThanZero(pDX, IDC_PT_AFTER_ALL_LOSSES_SR, m_Entry.m_TendonStressCoeff[CSS_AFTER_ALL_LOSSES][STRESS_REL]);
   DDX_Text(pDX,IDC_PT_AFTER_ALL_LOSSES_LR,m_Entry.m_TendonStressCoeff[CSS_AFTER_ALL_LOSSES][LOW_RELAX]);
   DDV_GreaterThanZero(pDX, IDC_PT_AFTER_ALL_LOSSES_LR, m_Entry.m_TendonStressCoeff[CSS_AFTER_ALL_LOSSES][LOW_RELAX]);

   // Pretension Strand Options
   DDX_Check_Bool(pDX,IDC_STRAND_EXTENSIONS,m_Entry.m_bAllowStraightStrandExtensions);

   int value = (int)(m_Entry.m_PrestressTransferComputationType == pgsTypes::ptMinuteValue ? 1 : 0);
   DDX_CBIndex(pDX,IDC_COMBO_TRANSFER,value);
   if ( pDX->m_bSaveAndValidate )
   {
      m_Entry.m_PrestressTransferComputationType = (value==1) ? pgsTypes::ptMinuteValue : pgsTypes::ptUsingSpecification;
   }

   // Duct Size
   DDX_Text(pDX,IDC_PUSH_METHOD, m_Entry.m_DuctAreaPushRatio);
   DDX_Text(pDX,IDC_PULL_METHOD, m_Entry.m_DuctAreaPullRatio);
   DDX_Text(pDX,IDC_DUCT_SIZE_RATIO, m_Entry.m_DuctDiameterRatio);

   if (pDX->m_bSaveAndValidate)
   {
      if (lrfdVersionMgr::NinthEdition2020 <= m_Entry.m_SpecificationType)
      {
         if (0.70 < m_Entry.m_DuctDiameterRatio)
         {
            // if the ratio is more than 0.7, lambda_duct becomes negative
            pDX->PrepareEditCtrl(IDC_DUCT_SIZE_RATIO);
            AfxMessageBox(_T("The ratio of the outside duct diameter to the least gross concrete thickness at the duct cannot exceed 0.7"), MB_ICONEXCLAMATION | MB_OK);
            pDX->Fail();
         }
      }
      else
      {
         if ( !(m_Entry.m_DuctDiameterRatio < 1.0) )
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
   const unitmgtIndirectMeasure* pDisplayUnits = pApp->GetDisplayUnits();

   DDX_UnitValueAndTag(pDX, IDC_NWC_FC_SLAB,     IDC_NWC_FC_SLAB_UNIT,     m_Entry.m_MaxSlabFc[pgsTypes::Normal],             pDisplayUnits->Stress);
   DDX_UnitValueAndTag(pDX, IDC_NWC_GIRDER_FCI,  IDC_NWC_GIRDER_FCI_UNIT,  m_Entry.m_MaxSegmentFci[pgsTypes::Normal],          pDisplayUnits->Stress);
   DDX_UnitValueAndTag(pDX, IDC_NWC_GIRDER_FC,   IDC_NWC_GIRDER_FC_UNIT,   m_Entry.m_MaxSegmentFc[pgsTypes::Normal],           pDisplayUnits->Stress);
   DDX_UnitValueAndTag(pDX, IDC_NWC_CLOSURE_FCI, IDC_NWC_CLOSURE_FCI_UNIT, m_Entry.m_MaxClosureFci[pgsTypes::Normal],          pDisplayUnits->Stress);
   DDX_UnitValueAndTag(pDX, IDC_NWC_CLOSURE_FC,  IDC_NWC_CLOSURE_FC_UNIT,  m_Entry.m_MaxClosureFc[pgsTypes::Normal],           pDisplayUnits->Stress);
   DDX_UnitValueAndTag(pDX, IDC_NWC_UNIT_WEIGHT, IDC_NWC_UNIT_WEIGHT_UNIT, m_Entry.m_MaxConcreteUnitWeight[pgsTypes::Normal], pDisplayUnits->Density);
   DDX_UnitValueAndTag(pDX, IDC_NWC_AGG_SIZE,    IDC_NWC_AGG_SIZE_UNIT,    m_Entry.m_MaxConcreteAggSize[pgsTypes::Normal],    pDisplayUnits->ComponentDim);

   DDX_UnitValueAndTag(pDX, IDC_LWC_FC_SLAB,     IDC_LWC_FC_SLAB_UNIT,     m_Entry.m_MaxSlabFc[pgsTypes::SandLightweight],             pDisplayUnits->Stress);
   DDX_UnitValueAndTag(pDX, IDC_LWC_GIRDER_FCI,  IDC_LWC_GIRDER_FCI_UNIT,  m_Entry.m_MaxSegmentFci[pgsTypes::SandLightweight],          pDisplayUnits->Stress);
   DDX_UnitValueAndTag(pDX, IDC_LWC_GIRDER_FC,   IDC_LWC_GIRDER_FC_UNIT,   m_Entry.m_MaxSegmentFc[pgsTypes::SandLightweight],           pDisplayUnits->Stress);
   DDX_UnitValueAndTag(pDX, IDC_LWC_CLOSURE_FCI, IDC_LWC_CLOSURE_FCI_UNIT, m_Entry.m_MaxClosureFci[pgsTypes::SandLightweight],          pDisplayUnits->Stress);
   DDX_UnitValueAndTag(pDX, IDC_LWC_CLOSURE_FC,  IDC_LWC_CLOSURE_FC_UNIT,  m_Entry.m_MaxClosureFc[pgsTypes::SandLightweight],           pDisplayUnits->Stress);
   DDX_UnitValueAndTag(pDX, IDC_LWC_UNIT_WEIGHT, IDC_LWC_UNIT_WEIGHT_UNIT, m_Entry.m_MaxConcreteUnitWeight[pgsTypes::SandLightweight], pDisplayUnits->Density);
   DDX_UnitValueAndTag(pDX, IDC_LWC_AGG_SIZE,    IDC_LWC_AGG_SIZE_UNIT,    m_Entry.m_MaxConcreteAggSize[pgsTypes::SandLightweight],    pDisplayUnits->ComponentDim);

   DDX_UnitValueAndTag(pDX, IDC_LWC_FC_SLAB,     IDC_LWC_FC_SLAB_UNIT,     m_Entry.m_MaxSlabFc[pgsTypes::AllLightweight],             pDisplayUnits->Stress);
   DDX_UnitValueAndTag(pDX, IDC_LWC_GIRDER_FCI,  IDC_LWC_GIRDER_FCI_UNIT,  m_Entry.m_MaxSegmentFci[pgsTypes::AllLightweight],          pDisplayUnits->Stress);
   DDX_UnitValueAndTag(pDX, IDC_LWC_GIRDER_FC,   IDC_LWC_GIRDER_FC_UNIT,   m_Entry.m_MaxSegmentFc[pgsTypes::AllLightweight],           pDisplayUnits->Stress);
   DDX_UnitValueAndTag(pDX, IDC_LWC_CLOSURE_FCI, IDC_LWC_CLOSURE_FCI_UNIT, m_Entry.m_MaxClosureFci[pgsTypes::AllLightweight],          pDisplayUnits->Stress);
   DDX_UnitValueAndTag(pDX, IDC_LWC_CLOSURE_FC,  IDC_LWC_CLOSURE_FC_UNIT,  m_Entry.m_MaxClosureFc[pgsTypes::AllLightweight],           pDisplayUnits->Stress);
   DDX_UnitValueAndTag(pDX, IDC_LWC_UNIT_WEIGHT, IDC_LWC_UNIT_WEIGHT_UNIT, m_Entry.m_MaxConcreteUnitWeight[pgsTypes::AllLightweight], pDisplayUnits->Density);
   DDX_UnitValueAndTag(pDX, IDC_LWC_AGG_SIZE,    IDC_LWC_AGG_SIZE_UNIT,    m_Entry.m_MaxConcreteAggSize[pgsTypes::AllLightweight],    pDisplayUnits->ComponentDim);

   DDX_UnitValueAndTag(pDX, IDC_UHPC_FC_SLAB, IDC_UHPC_FC_SLAB_UNIT, m_Entry.m_MaxSlabFc[pgsTypes::UHPC], pDisplayUnits->Stress);
   DDX_UnitValueAndTag(pDX, IDC_UHPC_GIRDER_FCI, IDC_UHPC_GIRDER_FCI_UNIT, m_Entry.m_MaxSegmentFci[pgsTypes::UHPC], pDisplayUnits->Stress);
   DDX_UnitValueAndTag(pDX, IDC_UHPC_GIRDER_FC, IDC_UHPC_GIRDER_FC_UNIT, m_Entry.m_MaxSegmentFc[pgsTypes::UHPC], pDisplayUnits->Stress);
   DDX_UnitValueAndTag(pDX, IDC_UHPC_CLOSURE_FCI, IDC_UHPC_CLOSURE_FCI_UNIT, m_Entry.m_MaxClosureFci[pgsTypes::UHPC], pDisplayUnits->Stress);
   DDX_UnitValueAndTag(pDX, IDC_UHPC_CLOSURE_FC, IDC_UHPC_CLOSURE_FC_UNIT, m_Entry.m_MaxClosureFc[pgsTypes::UHPC], pDisplayUnits->Stress);
   DDX_UnitValueAndTag(pDX, IDC_UHPC_UNIT_WEIGHT, IDC_UHPC_UNIT_WEIGHT_UNIT, m_Entry.m_MaxConcreteUnitWeight[pgsTypes::UHPC], pDisplayUnits->Density);
   DDX_UnitValueAndTag(pDX, IDC_UHPC_AGG_SIZE, IDC_UHPC_AGG_SIZE_UNIT, m_Entry.m_MaxConcreteAggSize[pgsTypes::UHPC], pDisplayUnits->ComponentDim);

   DDX_Check_Bool(pDX, IDC_CHECK_STIRRUP_COMPATIBILITY, m_Entry.m_DoCheckStirrupSpacingCompatibility);
   DDX_Check_Bool(pDX, IDC_CHECK_GIRDER_SAG, m_Entry.m_bCheckSag);
   DDX_CBEnum(pDX, IDC_SAG_OPTIONS, m_Entry.m_SagCamberType);
}

void CSpecMainSheet::ExchangeClosureData(CDataExchange* pDX)
{
   CEAFApp* pApp = EAFGetApp();
   const unitmgtIndirectMeasure* pDisplayUnits = pApp->GetDisplayUnits();

   CString tagBeforeLosses = (pApp->GetUnitsMode() == eafTypes::umSI ? _T("sqrt(f'ci (MPa))") : _T("sqrt(f'ci (KSI))"));
   CString tagAfterLosses  = (pApp->GetUnitsMode() == eafTypes::umSI ? _T("sqrt(f'c (MPa))") : _T("sqrt(f'c (KSI))"));

   if ( lrfdVersionMgr::SeventhEditionWith2016Interims <= lrfdVersionMgr::GetVersion() )
   {
      tagBeforeLosses = _T("(lambda)") + tagBeforeLosses;
      tagAfterLosses  = _T("(lambda)") + tagAfterLosses;
   }

   DDX_Text(pDX,IDC_RELEASE_COMPRESSION,m_Entry.m_ClosureCompStressAtStressing);
   DDV_GreaterThanZero(pDX, IDC_RELEASE_COMPRESSION,m_Entry.m_ClosureCompStressAtStressing);

   DDX_UnitValueAndTag(pDX, IDC_RELEASE_PTZ_TENSION, IDC_RELEASE_PTZ_TENSION_UNIT, m_Entry.m_ClosureTensStressPTZAtStressing, pDisplayUnits->SqrtPressure );
   DDX_Text(pDX,IDC_RELEASE_PTZ_TENSION_UNIT,tagBeforeLosses);
   DDV_UnitValueZeroOrMore(pDX, IDC_RELEASE_PTZ_TENSION_UNIT,m_Entry.m_ClosureTensStressPTZAtStressing, pDisplayUnits->SqrtPressure);

   DDX_UnitValueAndTag(pDX, IDC_RELEASE_PTZ_TENSION_WITH_REBAR, IDC_RELEASE_PTZ_TENSION_WITH_REBAR_UNIT, m_Entry.m_ClosureTensStressPTZWithRebarAtStressing, pDisplayUnits->SqrtPressure );
   DDX_Text(pDX,IDC_RELEASE_PTZ_TENSION_WITH_REBAR_UNIT,tagBeforeLosses);
   DDV_UnitValueZeroOrMore(pDX, IDC_RELEASE_PTZ_TENSION_WITH_REBAR_UNIT,m_Entry.m_ClosureTensStressPTZWithRebarAtStressing, pDisplayUnits->SqrtPressure);

   DDX_UnitValueAndTag(pDX, IDC_RELEASE_TENSION, IDC_RELEASE_TENSION_UNIT, m_Entry.m_ClosureTensStressAtStressing, pDisplayUnits->SqrtPressure );
   DDX_Text(pDX,IDC_RELEASE_TENSION_UNIT,tagBeforeLosses);
   DDV_UnitValueZeroOrMore(pDX, IDC_RELEASE_TENSION_UNIT,m_Entry.m_ClosureTensStressAtStressing, pDisplayUnits->SqrtPressure);

   DDX_UnitValueAndTag(pDX, IDC_RELEASE_TENSION_WITH_REBAR, IDC_RELEASE_TENSION_WITH_REBAR_UNIT, m_Entry.m_ClosureTensStressWithRebarAtStressing, pDisplayUnits->SqrtPressure );
   DDX_Text(pDX,IDC_RELEASE_TENSION_WITH_REBAR_UNIT,tagBeforeLosses);
   DDV_UnitValueZeroOrMore(pDX, IDC_RELEASE_TENSION_WITH_REBAR_UNIT,m_Entry.m_ClosureTensStressWithRebarAtStressing, pDisplayUnits->SqrtPressure);

   DDX_Text(pDX,IDC_SERVICE_COMPRESSION,m_Entry.m_ClosureCompStressAtService);
   DDV_GreaterThanZero(pDX, IDC_SERVICE_COMPRESSION,m_Entry.m_ClosureCompStressAtService);

   DDX_Text(pDX,IDC_SERVICE_COMPRESSION_WITH_LIVELOAD,m_Entry.m_ClosureCompStressWithLiveLoadAtService);
   DDV_GreaterThanZero(pDX, IDC_SERVICE_COMPRESSION_WITH_LIVELOAD,m_Entry.m_ClosureCompStressWithLiveLoadAtService);

   DDX_UnitValueAndTag(pDX, IDC_SERVICE_PTZ_TENSION, IDC_SERVICE_PTZ_TENSION_UNIT, m_Entry.m_ClosureTensStressPTZAtService, pDisplayUnits->SqrtPressure );
   DDX_Text(pDX,IDC_SERVICE_PTZ_TENSION_UNIT,tagAfterLosses);
   DDV_UnitValueZeroOrMore(pDX, IDC_SERVICE_PTZ_TENSION_UNIT,m_Entry.m_ClosureTensStressPTZAtService, pDisplayUnits->SqrtPressure);

   DDX_UnitValueAndTag(pDX, IDC_SERVICE_PTZ_TENSION_WITH_REBAR, IDC_SERVICE_PTZ_TENSION_WITH_REBAR_UNIT, m_Entry.m_ClosureTensStressPTZWithRebarAtService, pDisplayUnits->SqrtPressure );
   DDX_Text(pDX,IDC_SERVICE_PTZ_TENSION_WITH_REBAR_UNIT,tagAfterLosses);
   DDV_UnitValueZeroOrMore(pDX, IDC_SERVICE_PTZ_TENSION_WITH_REBAR_UNIT,m_Entry.m_ClosureTensStressPTZWithRebarAtService, pDisplayUnits->SqrtPressure);

   DDX_UnitValueAndTag(pDX, IDC_SERVICE_III_TENSION, IDC_SERVICE_III_TENSION_UNIT, m_Entry.m_ClosureTensStressAtService, pDisplayUnits->SqrtPressure );
   DDX_Text(pDX,IDC_SERVICE_III_TENSION_UNIT,tagAfterLosses);
   DDV_UnitValueZeroOrMore(pDX, IDC_SERVICE_III_TENSION_UNIT,m_Entry.m_ClosureTensStressAtService, pDisplayUnits->SqrtPressure);

   DDX_UnitValueAndTag(pDX, IDC_SERVICE_III_TENSION_WITH_REBAR, IDC_SERVICE_III_TENSION_WITH_REBAR_UNIT, m_Entry.m_ClosureTensStressWithRebarAtService, pDisplayUnits->SqrtPressure );
   DDX_Text(pDX,IDC_SERVICE_III_TENSION_WITH_REBAR_UNIT,tagAfterLosses);
   DDV_UnitValueZeroOrMore(pDX, IDC_SERVICE_III_TENSION_WITH_REBAR_UNIT,m_Entry.m_ClosureTensStressWithRebarAtService, pDisplayUnits->SqrtPressure);

   DDX_Text(pDX,IDC_FATIGUE_COMPRESSION,m_Entry.m_ClosureCompStressFatigue);
   DDV_GreaterThanZero(pDX, IDC_FATIGUE_COMPRESSION,m_Entry.m_ClosureCompStressFatigue);
}

void CSpecMainSheet::ExchangeDesignData(CDataExchange* pDX)
{
   CEAFApp* pApp = EAFGetApp();
   const unitmgtIndirectMeasure* pDisplayUnits = pApp->GetDisplayUnits();

   // Harped Strand Hold Down Force
	DDX_Check_Bool(pDX, IDC_CHECK_HD,  m_Entry.m_DoCheckHoldDown);
	DDX_Check_Bool(pDX, IDC_DESIGN_HD, m_Entry.m_DoDesignHoldDown);

   DDX_CBIndex(pDX, IDC_HOLD_DOWN_FORCE_TYPE, m_Entry.m_HoldDownForceType);

   DDX_UnitValueAndTag(pDX, IDC_HOLD_DOWN_FORCE, IDC_HOLD_DOWN_FORCE_UNITS, m_Entry.m_HoldDownForce, pDisplayUnits->GeneralForce );
   if (m_Entry.m_DoCheckHoldDown)
   {
      DDV_UnitValueGreaterThanZero(pDX, IDC_HOLD_DOWN_FORCE,m_Entry.m_HoldDownForce, pDisplayUnits->GeneralForce );
   }
   DDX_Percentage(pDX, IDC_FRICTION, m_Entry.m_HoldDownFriction);

   // Harped Strand Slope
	DDX_Check_Bool(pDX, IDC_CHECK_SLOPE,  m_Entry.m_DoCheckStrandSlope);
	DDX_Check_Bool(pDX, IDC_DESIGN_SLOPE, m_Entry.m_DoDesignStrandSlope);

	DDX_Text(pDX, IDC_STRAND_SLOPE_05, m_Entry.m_MaxSlope05);
   if (m_Entry.m_DoCheckStrandSlope)
   {
      DDV_NonNegativeDouble(pDX, IDC_STRAND_SLOPE_05, m_Entry.m_MaxSlope05);
   }

	DDX_Text(pDX, IDC_STRAND_SLOPE_06, m_Entry.m_MaxSlope06);
   if (m_Entry.m_DoCheckStrandSlope)
   {
      DDV_NonNegativeDouble(pDX, IDC_STRAND_SLOPE_06, m_Entry.m_MaxSlope06);
   }

	DDX_Text(pDX, IDC_STRAND_SLOPE_07, m_Entry.m_MaxSlope07);
   if (m_Entry.m_DoCheckStrandSlope)
   {
      DDV_NonNegativeDouble(pDX, IDC_STRAND_SLOPE_07, m_Entry.m_MaxSlope07);
   }
   
   // Handling Weight
   DDX_Check_Bool(pDX, IDC_CHECK_HANDLING_WEIGHT, m_Entry.m_bCheckHandlingWeightLimit);
   DDX_UnitValueAndTag(pDX, IDC_HANDLING_WEIGHT, IDC_HANDLING_WEIGHT_UNIT, m_Entry.m_HandlingWeightLimit, pDisplayUnits->GeneralForce);
   if (m_Entry.m_bCheckHandlingWeightLimit)
   {
      DDV_UnitValueGreaterThanZero(pDX, IDC_HANDLING_WEIGHT, m_Entry.m_HandlingWeightLimit, pDisplayUnits->GeneralForce);
   }

   // Splitting Resistance
	DDX_Check_Bool(pDX, IDC_CHECK_SPLITTING,  m_Entry.m_DoCheckSplitting);
	DDX_Check_Bool(pDX, IDC_DESIGN_SPLITTING, m_Entry.m_DoDesignSplitting);
	DDX_Text(pDX, IDC_N, m_Entry.m_SplittingZoneLengthFactor);
   DDV_GreaterThanZero(pDX, IDC_N, m_Entry.m_SplittingZoneLengthFactor);

   DDX_UnitValueAndTag(pDX, IDC_UHPC_F1, IDC_UHPC_F1_UNIT, m_Entry.m_UHPCStregthAtFirstCrack, pDisplayUnits->Stress);
   DDV_UnitValueZeroOrMore(pDX, IDC_UHPC_F1, m_Entry.m_UHPCStregthAtFirstCrack, pDisplayUnits->Stress);

   // Confinement Reinforcement
	DDX_Check_Bool(pDX, IDC_CHECK_CONFINEMENT,  m_Entry.m_DoCheckConfinement);
	DDX_Check_Bool(pDX, IDC_DESIGN_CONFINEMENT, m_Entry.m_DoDesignConfinement);

   // Lifting
	DDX_Check_Bool(pDX, IDC_CHECK_LIFTING,  m_Entry.m_EnableLiftingCheck);
	DDX_Check_Bool(pDX, IDC_DESIGN_LIFTING, m_Entry.m_EnableLiftingDesign);
 
   DDX_KeywordUnitValueAndTag(pDX,IDC_MIN_LIFTING_POINT,IDC_MIN_LIFTING_POINT_UNIT,_T("H"),m_Entry.m_MinLiftPoint, pDisplayUnits->SpanLength);

   DDX_UnitValueAndTag(pDX, IDC_LIFTING_POINT_LOCATION_ACCURACY,IDC_LIFTING_POINT_LOCATION_ACCURACY_UNIT,m_Entry.m_LiftPointAccuracy, pDisplayUnits->SpanLength );
   DDV_UnitValueGreaterThanZero(pDX, IDC_LIFTING_POINT_LOCATION_ACCURACY,m_Entry.m_LiftPointAccuracy, pDisplayUnits->SpanLength );

   // Hauling
   DDX_Check_Bool(pDX, IDC_CHECK_HAULING, m_Entry.m_EnableHaulingCheck);
	DDX_Check_Bool(pDX, IDC_DESIGN_HAULING, m_Entry.m_EnableHaulingDesign);

   DDX_KeywordUnitValueAndTag(pDX,IDC_MIN_TRUCK_SUPPORT,IDC_MIN_TRUCK_SUPPORT_UNIT,_T("H"),m_Entry.m_MinHaulPoint, pDisplayUnits->SpanLength);
   DDX_UnitValueAndTag(pDX, IDC_TRUCK_SUPPORT_LOCATION_ACCURACY,IDC_TRUCK_SUPPORT_LOCATION_ACCURACY_UNIT,m_Entry.m_HaulPointAccuracy, pDisplayUnits->SpanLength );
   DDV_UnitValueGreaterThanZero(pDX, IDC_TRUCK_SUPPORT_LOCATION_ACCURACY,m_Entry.m_HaulPointAccuracy, pDisplayUnits->SpanLength );

   DDX_Check_Bool(pDX, IDC_IS_SUPPORT_LESS_THAN, m_Entry.m_UseMinTruckSupportLocationFactor);
	DDX_Text(pDX, IDC_SUPPORT_LESS_THAN, m_Entry.m_MinTruckSupportLocationFactor);
   DDV_MinMaxDouble(pDX, m_Entry.m_MinTruckSupportLocationFactor, 0.0, 0.4);

   // Slab Offset
	DDX_Check_Bool(pDX, IDC_CHECK_A, m_Entry.m_EnableSlabOffsetCheck);
	DDX_Check_Bool(pDX, IDC_DESIGN_A, m_Entry.m_EnableSlabOffsetDesign);
   DDX_CBEnum(pDX,IDC_A_ROUNDING_CB,m_Entry.m_SlabOffsetRoundingMethod);
   DDX_UnitValueAndTag(pDX, IDC_A_ROUNDING_EDIT,IDC_A_ROUNDING_UNIT,m_Entry.m_SlabOffsetRoundingTolerance, pDisplayUnits->ComponentDim );
   DDV_UnitValueZeroOrMore(pDX, IDC_A_ROUNDING_EDIT,m_Entry.m_SlabOffsetRoundingTolerance, pDisplayUnits->ComponentDim );

   // Live Load Deflections
   DDX_Check_Bool(pDX, IDC_LL_DEFLECTION, m_Entry.m_bDoEvaluateDeflection );
 	DDX_Text(pDX, IDC_DEFLECTION_LIMIT, m_Entry.m_DeflectionLimit);
   DDV_GreaterThanZero(pDX, IDC_DEFLECTION_LIMIT, m_Entry.m_DeflectionLimit);

   // Bottom Flange Clearance
   DDX_Check_Bool(pDX,IDC_CHECK_BOTTOM_FLANGE_CLEARANCE,m_Entry.m_bCheckBottomFlangeClearance);
   DDX_UnitValueAndTag(pDX,IDC_CLEARANCE,IDC_CLEARANCE_UNIT,m_Entry.m_Cmin,pDisplayUnits->SpanLength);
   DDV_UnitValueZeroOrMore(pDX,IDC_CLEARANCE,m_Entry.m_Cmin,pDisplayUnits->SpanLength);

   // Girder Inclination
   DDX_Check_Bool(pDX, IDC_CHECK_INCLINDED_GIRDER, m_Entry.m_bCheckGirderInclination);
   DDX_Text(pDX, IDC_INCLINDED_GIRDER_FS, m_Entry.m_InclinedGirder_FSmax);

   // Strand Fill
   int value = (int)m_Entry.m_DesignStrandFillType;
   DDX_Radio(pDX,IDC_RADIO_FILL_PERMANENT,value);
   if ( pDX->m_bSaveAndValidate )
   {
      m_Entry.m_DesignStrandFillType = (arDesignStrandFillType)value;
   }

   value = (int)m_Entry.m_LimitStateConcreteStrength;
   DDX_Radio(pDX,IDC_FC1,value);
   if ( pDX->m_bSaveAndValidate )
   {
      m_Entry.m_LimitStateConcreteStrength = (pgsTypes::LimitStateConcreteStrength)value;
   }

   DDX_Check_Bool(pDX, IDC_USE_90_DAY_STRENGTH, m_Entry.m_bUse90DayConcreteStrength);
   DDX_Percentage(pDX, IDC_90_DAY_STRENGTH_FACTOR, m_Entry.m_90DayConcreteStrengthFactor);
   if (pDX->m_bSaveAndValidate)
   {
      if (m_Entry.m_90DayConcreteStrengthFactor < 1.0)
      {
         pDX->PrepareEditCtrl(IDC_90_DAY_STRENGTH_FACTOR);
         AfxMessageBox(_T("Factor for 90 day concrete strength must be at least 100%"));
         pDX->Fail();
      }
   }

   // Roadway elevations
   DDX_UnitValueAndTag(pDX, IDC_ELEVATION_TOLERANCE, IDC_ELEVATION_TOLERANCE_UNIT, m_Entry.m_FinishedElevationTolerance, pDisplayUnits->ComponentDim);
   DDV_UnitValueZeroOrMore(pDX, IDC_ELEVATION_TOLERANCE, m_Entry.m_FinishedElevationTolerance, pDisplayUnits->ComponentDim);
}

void CSpecMainSheet::ExchangeBearingsData(CDataExchange* pDX)
{
   DDX_Check_Bool(pDX, IDC_TAPERED_SOLE_PLATE_REQUIRED, m_Entry.m_bAlertTaperedSolePlateRequirement);
   DDX_Text(pDX, IDC_TAPERED_SOLE_PLATE_THRESHOLD, m_Entry.m_TaperedSolePlateInclinationThreshold);
   DDV_LimitOrMore(pDX, IDC_TAPERED_SOLE_PLATE_THRESHOLD, m_Entry.m_TaperedSolePlateInclinationThreshold, 0.0);
   DDX_Check_Bool(pDX, IDC_BEARING_REACTION_IMPACT, m_Entry.m_bUseImpactForBearingReactions);
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

lrfdVersionMgr::Version CSpecMainSheet::GetSpecVersion()
{
   return m_SpecDescrPage.GetSpecVersion();
}