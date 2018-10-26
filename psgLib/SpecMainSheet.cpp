///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2014  Washington State Department of Transportation
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
#include "..\htmlhelp\HelpTopics.hh"

#include <EAF\EAFApp.h>

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
   m_AllowEditing(allowEditing)
{
   Init();
}

CSpecMainSheet::CSpecMainSheet( SpecLibraryEntry& rentry, LPCTSTR pszCaption,
                                   bool allowEditing,
                                   CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(pszCaption, pParentWnd, iSelectPage),
   m_Entry(rentry),
   m_AllowEditing(allowEditing)
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
   m_SpecCastingYardPage.m_psp.dwFlags     |= PSP_HASHELP;
   m_SpecLiftingPage.m_psp.dwFlags         |= PSP_HASHELP;
   m_SpecHaulingErectionPage.m_psp.dwFlags |= PSP_HASHELP;
   m_SpecBridgeSite1Page.m_psp.dwFlags     |= PSP_HASHELP;
   m_SpecBridgeSite2Page.m_psp.dwFlags     |= PSP_HASHELP;
   m_SpecBridgeSite3Page.m_psp.dwFlags     |= PSP_HASHELP;
   m_SpecMomentPage.m_psp.dwFlags          |= PSP_HASHELP;
   m_SpecShearPage.m_psp.dwFlags           |= PSP_HASHELP;
   m_SpecCreepPage.m_psp.dwFlags           |= PSP_HASHELP;
   m_SpecLossPage.m_psp.dwFlags            |= PSP_HASHELP;
   m_SpecPSLimitPage.m_psp.dwFlags         |= PSP_HASHELP;
   m_SpecLimitsPage.m_psp.dwFlags          |= PSP_HASHELP;
   m_SpecDesignPage.m_psp.dwFlags          |= PSP_HASHELP;
   m_SpecDeflectionsPage.m_psp.dwFlags     |= PSP_HASHELP;

   AddPage(&m_SpecDescrPage);
   AddPage(&m_SpecCastingYardPage);
   AddPage(&m_SpecLiftingPage);
   AddPage(&m_SpecHaulingErectionPage);
   AddPage(&m_SpecBridgeSite1Page);
   AddPage(&m_SpecBridgeSite2Page);
   AddPage(&m_SpecBridgeSite3Page);
   AddPage(&m_SpecMomentPage);
   AddPage(&m_SpecShearPage);
   AddPage(&m_SpecCreepPage);
   AddPage(&m_SpecLossPage);
   AddPage(&m_SpecPSLimitPage);
   AddPage(&m_SpecLimitsPage);
   AddPage(&m_SpecDesignPage);
   AddPage(&m_SpecDeflectionsPage);
}

void CSpecMainSheet::ExchangeDescrData(CDataExchange* pDX)
{
   // specification type
   DDX_CBItemData(pDX,IDC_SPECIFICATION,m_Entry.m_SpecificationType);

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

      m_Description = m_Entry.GetDescription().c_str();
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
      CheckShearCapacityMethod();
}

void CSpecMainSheet::ExchangeCyData(CDataExchange* pDX)
{
   CEAFApp* pApp = EAFGetApp();
   const unitmgtIndirectMeasure* pDisplayUnits = pApp->GetDisplayUnits();

   DDX_Check_Bool(pDX,IDC_STRAND_EXTENSIONS,m_Entry.m_bAllowStraightStrandExtensions);


   if (!pDX->m_bSaveAndValidate)
   {
      m_SpecCastingYardPage.m_DoCheckHoldDown    = m_Entry.m_DoCheckHoldDown;
      m_SpecCastingYardPage.m_DoCheckStrandSlope = m_Entry.m_DoCheckStrandSlope;
      m_SpecCastingYardPage.m_DoCheckSplitting = m_Entry.m_DoCheckSplitting;

      // set statics for strand slope
      CString sl05, sl06, sl07;
      if (pApp->GetUnitsMode() == eafTypes::umSI)
      {
         VERIFY(sl05.LoadString(IDS_SLOPE_O5_SI));
         VERIFY(sl06.LoadString(IDS_SLOPE_O6_SI));
         VERIFY(sl07.LoadString(IDS_SLOPE_O7_SI));
      }
      else
      {
         VERIFY(sl05.LoadString(IDS_SLOPE_O5_US));
         VERIFY(sl06.LoadString(IDS_SLOPE_O6_US));
         VERIFY(sl07.LoadString(IDS_SLOPE_O7_US));
      }

      m_SpecCastingYardPage.m_StaticSlope05.SetWindowText(sl05);
      m_SpecCastingYardPage.m_StaticSlope06.SetWindowText(sl06);
      m_SpecCastingYardPage.m_StaticSlope07.SetWindowText(sl07);
   }

   DDX_Radio(pDX,IDC_CURING_METHOD,m_Entry.m_CuringMethod);

	DDX_Text(pDX, IDC_STRAND_SLOPE_05, m_Entry.m_MaxSlope05);
   if (m_Entry.m_DoCheckStrandSlope) 
      DDV_NonNegativeDouble(pDX, IDC_STRAND_SLOPE_05, m_Entry.m_MaxSlope05);

	DDX_Text(pDX, IDC_STRAND_SLOPE_06, m_Entry.m_MaxSlope06);
   if (m_Entry.m_DoCheckStrandSlope) 
      DDV_NonNegativeDouble(pDX, IDC_STRAND_SLOPE_06, m_Entry.m_MaxSlope06);

	DDX_Text(pDX, IDC_STRAND_SLOPE_07, m_Entry.m_MaxSlope07);
   if (m_Entry.m_DoCheckStrandSlope) 
      DDV_NonNegativeDouble(pDX, IDC_STRAND_SLOPE_07, m_Entry.m_MaxSlope07);

   DDX_UnitValueAndTag(pDX, IDC_HOLD_DOWN_FORCE, IDC_HOLD_DOWN_FORCE_UNITS, m_Entry.m_HoldDownForce, pDisplayUnits->GeneralForce );
   if (m_Entry.m_DoCheckHoldDown)
      DDV_UnitValueGreaterThanZero(pDX, IDC_HOLD_DOWN_FORCE,m_Entry.m_HoldDownForce, pDisplayUnits->GeneralForce );

   DDX_UnitValueAndTag(pDX, IDC_MAX_STIRRUP_SPACING, IDC_MAX_STIRRUP_SPACING_UNITS, m_Entry.m_MaxStirrupSpacing, pDisplayUnits->ComponentDim );
   DDV_UnitValueGreaterThanZero(pDX, IDC_MAX_STIRRUP_SPACING,m_Entry.m_MaxStirrupSpacing, pDisplayUnits->ComponentDim);

	DDX_Text(pDX, IDC_CY_ALLOW_SERVICE_COMP, m_Entry.m_CyCompStressServ);
   DDV_GreaterThanZero(pDX, IDC_CY_ALLOW_SERVICE_COMP, m_Entry.m_CyCompStressServ);

   DDX_UnitValueAndTag(pDX, IDC_NORMAL_MAX_SQRT2, IDC_NORMAL_MAX_SQRT_UNITS, m_Entry.m_CyTensStressServ, pDisplayUnits->SqrtPressure );
   CString tag = (pApp->GetUnitsMode() == eafTypes::umSI ? _T("sqrt( f'ci (MPa) )") : _T("sqrt( f'ci (KSI) )"));
   DDX_Text(pDX,IDC_CYS_TENS_BYLINE,tag);
   DDV_UnitValueZeroOrMore(pDX, IDC_CYS_TENS_BYLINE,m_Entry.m_CyTensStressServ, pDisplayUnits->SqrtPressure);
   DDX_Check_Bool(pDX, IDC_CHECK_NORMAL_MAX_MAX2, m_Entry.m_CyDoTensStressServMax);
   DDX_UnitValueAndTag(pDX, IDC_NORMAL_MAX_MAX2, IDC_NORMAL_MAX_MAX_UNITS2, m_Entry.m_CyTensStressServMax, pDisplayUnits->Stress );
   if (m_Entry.m_CyDoTensStressServMax)
      DDV_UnitValueGreaterThanZero(pDX, IDC_NORMAL_MAX_MAX2,m_Entry.m_CyTensStressServMax, pDisplayUnits->Stress );

   DDX_UnitValueAndTag(pDX, IDC_NORMAL_MAX_SQRT3, IDC_NORMAL_MAX_SQRT_UNITS, m_Entry.m_CyTensStressServWithRebar, pDisplayUnits->SqrtPressure);
   DDX_Text(pDX,IDC_CYS_TENS_BYLINE2,tag);
   DDV_UnitValueZeroOrMore(pDX, IDC_CYS_TENS_BYLINE2,m_Entry.m_CyTensStressServWithRebar, pDisplayUnits->SqrtPressure );

   if (pDX->m_bSaveAndValidate && m_Entry.m_CyTensStressServWithRebar < m_Entry.m_CyTensStressServ)
   {
      AfxMessageBox(_T("Allowable tensile stress with bonded reinforcement must be greater than or equal to that without"),MB_OK | MB_ICONWARNING);
      pDX->Fail();
   }

	DDX_Text(pDX, IDC_N, m_Entry.m_SplittingZoneLengthFactor);
   DDV_GreaterThanZero(pDX, IDC_N, m_Entry.m_SplittingZoneLengthFactor);
}

void CSpecMainSheet::ExchangeLiftingData(CDataExchange* pDX)
{
   CEAFApp* pApp = EAFGetApp();
   const unitmgtIndirectMeasure* pDisplayUnits = pApp->GetDisplayUnits();


   m_SpecLiftingPage.m_IsLiftingEnabled = m_Entry.m_EnableLiftingCheck;

	DDX_Text(pDX, IDC_FS_CY_CRACK, m_Entry.m_CyLiftingCrackFs);
   DDV_NonNegativeDouble(pDX, IDC_FS_CY_CRACK, m_Entry.m_CyLiftingCrackFs);
	DDX_Text(pDX, IDC_FS_CY_FAIL, m_Entry.m_CyLiftingFailFs);
   DDV_NonNegativeDouble(pDX, IDC_FS_CY_FAIL, m_Entry.m_CyLiftingFailFs);

   CString tag = (pApp->GetUnitsMode() == eafTypes::umSI ? _T("sqrt(f'ci MPa)") : _T("sqrt(f'ci KSI)"));
   DDX_UnitValueAndTag(pDX, IDC_FR, IDC_FR_UNIT, m_Entry.m_LiftingModulusOfRuptureCoefficient[pgsTypes::Normal], pDisplayUnits->SqrtPressure );
   DDX_Text(pDX,IDC_FR_SQRT,tag);

   DDX_UnitValueAndTag(pDX, IDC_ALWC_FR, IDC_ALWC_FR_UNIT, m_Entry.m_LiftingModulusOfRuptureCoefficient[pgsTypes::AllLightweight], pDisplayUnits->SqrtPressure );
   DDX_Text(pDX,IDC_ALWC_FR_SQRT,tag);

   DDX_UnitValueAndTag(pDX, IDC_SLWC_FR, IDC_SLWC_FR_UNIT, m_Entry.m_LiftingModulusOfRuptureCoefficient[pgsTypes::SandLightweight], pDisplayUnits->SqrtPressure );
   DDX_Text(pDX,IDC_SLWC_FR_SQRT,tag);

   DDX_UnitValueAndTag(pDX, IDC_PICK_POINT_HEIGHT, IDC_PICK_POINT_HEIGHT_UNITS, m_Entry.m_PickPointHeight, pDisplayUnits->ComponentDim);
   DDV_UnitValueZeroOrMore(pDX, IDC_PICK_POINT_HEIGHT,m_Entry.m_PickPointHeight, pDisplayUnits->ComponentDim );
   DDX_UnitValueAndTag(pDX, IDC_LIFTING_LOOP_TOLERANCE, IDC_LIFTING_LOOP_TOLERANCE_UNITS, m_Entry.m_LiftingLoopTolerance, pDisplayUnits->ComponentDim );
   DDV_UnitValueZeroOrMore(pDX, IDC_LIFTING_LOOP_TOLERANCE,m_Entry.m_LiftingLoopTolerance, pDisplayUnits->ComponentDim );
	DDX_Text(pDX, IDC_GIRDER_SWEEP_TOL, m_Entry.m_MaxGirderSweepLifting);
   DDV_NonNegativeDouble(pDX, IDC_GIRDER_SWEEP_TOL, m_Entry.m_MaxGirderSweepLifting);
   DDX_UnitValueAndTag(pDX, IDC_MIN_CABLE_ANGLE, IDC_MIN_CABLE_ANGLE_UNITS, m_Entry.m_MinCableInclination, pDisplayUnits->Angle);
   DDV_UnitValueRange(pDX, IDC_MIN_CABLE_ANGLE,m_Entry.m_MinCableInclination, 0.0, 90., pDisplayUnits->Angle );

	DDX_Text(pDX, IDC_CY_ALLOW_LIFTING_COMP, m_Entry.m_CyCompStressLifting);
   DDV_GreaterThanZero(pDX, IDC_CY_ALLOW_LIFTING_COMP, m_Entry.m_CyCompStressLifting);

   DDX_UnitValueAndTag(pDX, IDC_LIFTING_NORMAL_MAX_SQRT, IDC_LIFTING_NORMAL_MAX_SQRT_UNITS, m_Entry.m_CyTensStressLifting, pDisplayUnits->SqrtPressure );
   DDX_Text(pDX,IDC_LIFT_TENS,tag);
   DDV_UnitValueZeroOrMore(pDX, IDC_LIFT_TENS,m_Entry.m_CyTensStressLifting, pDisplayUnits->SqrtPressure );
   DDX_Check_Bool(pDX, IDC_CHECK_LIFTING_NORMAL_MAX_MAX, m_Entry.m_CyDoTensStressLiftingMax);
   DDX_UnitValueAndTag(pDX, IDC_LIFTING_NORMAL_MAX_MAX, IDC_LIFTING_NORMAL_MAX_MAX_UNITS, m_Entry.m_CyTensStressLiftingMax, pDisplayUnits->Stress );
   if (m_Entry.m_CyDoTensStressLiftingMax)
      DDV_UnitValueGreaterThanZero(pDX, IDC_LIFTING_NORMAL_MAX_MAX,m_Entry.m_CyTensStressLiftingMax, pDisplayUnits->Stress );

   DDX_UnitValueAndTag(pDX, IDC_LIFTING_NORMAL_MAX_SQRT2, IDC_LIFTING_NORMAL_MAX_SQRT_UNITS, m_Entry.m_TensStressLiftingWithRebar, pDisplayUnits->SqrtPressure );
   DDX_Text(pDX,IDC_LIFT_TENS2,tag);
   DDV_UnitValueZeroOrMore(pDX, IDC_LIFT_TENS2,m_Entry.m_TensStressLiftingWithRebar, pDisplayUnits->SqrtPressure );

   if (pDX->m_bSaveAndValidate && m_Entry.m_TensStressLiftingWithRebar < m_Entry.m_CyTensStressLifting)
   {
      AfxMessageBox(_T("Allowable tensile stress with bonded reinforcement must be greater than or equal to that without"),MB_OK | MB_ICONWARNING);
      pDX->Fail();
   }

   DDX_Percentage(pDX, IDC_IMPACT_UPWARD_LIFTING, m_Entry.m_LiftingUpwardImpact);
   DDV_MinMaxDouble(pDX, m_Entry.m_LiftingUpwardImpact, 0.0, 1.0);
	DDX_Percentage(pDX, IDC_IMPACT_DOWNWARD_LIFTING, m_Entry.m_LiftingDownwardImpact);
   DDV_MinMaxDouble(pDX, m_Entry.m_LiftingDownwardImpact, 0.0, 1.0);
 
   DDX_KeywordUnitValueAndTag(pDX,IDC_MIN_LIFTING_POINT,IDC_MIN_LIFTING_POINT_UNIT,_T("H"),m_Entry.m_MinLiftPoint, pDisplayUnits->SpanLength);

   DDX_UnitValueAndTag(pDX, IDC_LIFTING_POINT_LOCATION_ACCURACY,IDC_LIFTING_POINT_LOCATION_ACCURACY_UNIT,m_Entry.m_LiftPointAccuracy, pDisplayUnits->SpanLength );
   DDV_UnitValueGreaterThanZero(pDX, IDC_LIFTING_POINT_LOCATION_ACCURACY,m_Entry.m_LiftPointAccuracy, pDisplayUnits->SpanLength );

   if (!pDX->m_bSaveAndValidate)
   {
      m_SpecLiftingPage.HideLiftingControls(!m_Entry.m_EnableLiftingCheck);
   }
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

	DDX_Text(pDX, IDC_HE_HAULING_FS_CRACK, m_Entry.m_HeHaulingCrackFs);
   DDV_NonNegativeDouble(pDX, IDC_HE_HAULING_FS_CRACK, m_Entry.m_HeHaulingCrackFs);
	DDX_Text(pDX, IDC_HE_HAULING_FS_ROLLOVER, m_Entry.m_HeHaulingRollFs);
   DDV_NonNegativeDouble(pDX, IDC_HE_HAULING_FS_ROLLOVER, m_Entry.m_HeHaulingRollFs);

	DDX_Text(pDX, IDC_HAULING_ALLOW_COMP, m_Entry.m_CompStressHauling);
   DDV_GreaterThanZero(pDX, IDC_HAULING_ALLOW_COMP, m_Entry.m_CompStressHauling);

   CString tag = (pApp->GetUnitsMode() == eafTypes::umSI ? _T("sqrt(f'c MPa)") : _T("sqrt(f'c KSI)"));
   DDX_UnitValueAndTag(pDX, IDC_FR, IDC_FR_UNIT, m_Entry.m_HaulingModulusOfRuptureCoefficient[pgsTypes::Normal], pDisplayUnits->SqrtPressure );
   DDX_Text(pDX,IDC_FR_SQRT,tag);

   DDX_UnitValueAndTag(pDX, IDC_ALWC_FR, IDC_ALWC_FR_UNIT, m_Entry.m_HaulingModulusOfRuptureCoefficient[pgsTypes::AllLightweight], pDisplayUnits->SqrtPressure );
   DDX_Text(pDX,IDC_ALWC_FR_SQRT,tag);

   DDX_UnitValueAndTag(pDX, IDC_SLWC_FR, IDC_SLWC_FR_UNIT, m_Entry.m_HaulingModulusOfRuptureCoefficient[pgsTypes::SandLightweight], pDisplayUnits->SqrtPressure );
   DDX_Text(pDX,IDC_SLWC_FR_SQRT,tag);

   DDX_UnitValueAndTag(pDX, IDC_HAULING_MAX_TENSSQRT, IDC_HAULING_MAX_TENSSQRT_UNITS, m_Entry.m_TensStressHauling, pDisplayUnits->SqrtPressure );
   DDX_Text(pDX,IDC_HAUL_TENS,tag);
   DDV_UnitValueZeroOrMore(pDX, IDC_HAUL_TENS,m_Entry.m_TensStressHauling, pDisplayUnits->SqrtPressure );
   DDX_Check_Bool(pDX, IDC_CHECK_HAULING_TENS_MAX, m_Entry.m_DoTensStressHaulingMax);
   DDX_UnitValueAndTag(pDX, IDC_HAULING_TENS_MAX, IDC_HAULING_TENS_MAX_UNITS, m_Entry.m_TensStressHaulingMax, pDisplayUnits->Stress );
   if (m_Entry.m_DoTensStressHaulingMax)
      DDV_UnitValueGreaterThanZero(pDX, IDC_HAULING_TENS_MAX,m_Entry.m_TensStressHaulingMax, pDisplayUnits->Stress );

   DDX_UnitValueAndTag(pDX, IDC_HAULING_MAX_TENSSQRT2, IDC_HAULING_MAX_TENSSQRT_UNITS, m_Entry.m_TensStressHaulingWithRebar, pDisplayUnits->SqrtPressure  );
   DDX_Text(pDX,IDC_HAUL_TENS2,tag);
   DDV_UnitValueZeroOrMore(pDX, IDC_HAULING_MAX_TENSSQRT2,m_Entry.m_TensStressHaulingWithRebar, pDisplayUnits->SqrtPressure );

   if (pDX->m_bSaveAndValidate && m_Entry.m_TensStressHaulingWithRebar < m_Entry.m_TensStressHauling)
   {
      AfxMessageBox(_T("Allowable tensile stress with bonded reinforcement must be greater than or equal to that without"),MB_OK | MB_ICONWARNING);
      pDX->Fail();
   }

   // Validate truck roll stiffness input
   DDX_Radio(pDX, IDC_LUMPSUM_METHOD, m_Entry.m_TruckRollStiffnessMethod );
   DDX_UnitValueAndTag(pDX, IDC_ROLL_STIFFNESS, IDC_ROLL_STIFFNESS_UNITS, m_Entry.m_TruckRollStiffness, pDisplayUnits->MomentPerAngle);
   DDX_UnitValueAndTag(pDX, IDC_AXLE_WEIGHT, IDC_AXLE_WEIGHT_UNITS, m_Entry.m_AxleWeightLimit, pDisplayUnits->GeneralForce );
   DDX_UnitValueAndTag(pDX, IDC_AXLE_STIFFNESS, IDC_AXLE_STIFFNESS_UNITS, m_Entry.m_AxleStiffness, pDisplayUnits->MomentPerAngle );
   DDX_UnitValueAndTag(pDX, IDC_MIN_ROLL_STIFFNESS, IDC_MIN_ROLL_STIFFNESS_UNITS, m_Entry.m_MinRollStiffness, pDisplayUnits->MomentPerAngle );
   DDV_UnitValueGreaterThanZero(pDX, IDC_ROLL_STIFFNESS, m_Entry.m_TruckRollStiffness, pDisplayUnits->MomentPerAngle );
   DDV_UnitValueGreaterThanZero(pDX, IDC_AXLE_WEIGHT,m_Entry.m_AxleWeightLimit, pDisplayUnits->GeneralForce );
   DDV_UnitValueGreaterThanZero(pDX, IDC_AXLE_STIFFNESS,m_Entry.m_AxleStiffness, pDisplayUnits->MomentPerAngle );
   DDV_UnitValueGreaterThanZero(pDX, IDC_MIN_ROLL_STIFFNESS,m_Entry.m_MinRollStiffness, pDisplayUnits->MomentPerAngle );

   
   DDX_UnitValueAndTag(pDX, IDC_HEIGHT_GIRDER_BOTTOM, IDC_HEIGHT_GIRDER_BOTTOM_UNITS, m_Entry.m_TruckGirderHeight, pDisplayUnits->ComponentDim );
   DDV_UnitValueGreaterThanZero(pDX, IDC_HEIGHT_GIRDER_BOTTOM,m_Entry.m_TruckGirderHeight, pDisplayUnits->ComponentDim );
   DDX_UnitValueAndTag(pDX, IDC_HEIGHT_ROLL_CENTER, IDC_HEIGHT_ROLL_CENTER_UNITS, m_Entry.m_TruckRollCenterHeight, pDisplayUnits->ComponentDim );
   DDV_UnitValueGreaterThanZero(pDX, IDC_HEIGHT_ROLL_CENTER,m_Entry.m_TruckRollCenterHeight, pDisplayUnits->ComponentDim );
   DDX_UnitValueAndTag(pDX, IDC_AXLE_SPACING, IDC_AXLE_SPACING_UNITS, m_Entry.m_TruckAxleWidth, pDisplayUnits->ComponentDim );
   DDV_UnitValueGreaterThanZero(pDX, IDC_AXLE_SPACING,m_Entry.m_TruckAxleWidth, pDisplayUnits->ComponentDim );
   DDX_UnitValueAndTag(pDX, IDC_MAX_HAULING_SUPPORT_LENGTH, IDC_MAX_HAULING_SUPPORT_LENGTH_UNITS, m_Entry.m_HaulingSupportDistance, pDisplayUnits->SpanLength );
   DDV_UnitValueGreaterThanZero(pDX, IDC_MAX_HAULING_SUPPORT_LENGTH,m_Entry.m_HaulingSupportDistance, pDisplayUnits->SpanLength );
   DDX_UnitValueAndTag(pDX, IDC_MAXOVERHANG, IDC_MAXOVERHANG_UNITS, m_Entry.m_MaxHaulingOverhang, pDisplayUnits->SpanLength );
   DDV_UnitValueGreaterThanZero(pDX, IDC_MAXOVERHANG,m_Entry.m_MaxHaulingOverhang, pDisplayUnits->SpanLength );
   DDX_Text(pDX, IDC_HE_ROADWAY_SUPERELEVATION, m_Entry.m_RoadwaySuperelevation);
   DDV_MinMaxDouble(pDX, m_Entry.m_RoadwaySuperelevation, 0.0, 0.5);
	DDX_Text(pDX, IDC_GIRDER_SWEEP_TOL, m_Entry.m_MaxGirderSweepHauling);
   DDV_MinMaxDouble(pDX, m_Entry.m_MaxGirderSweepHauling, 0.0, 1.0);
   DDX_UnitValueAndTag(pDX, IDC_SUPPORT_PLACEMENT_TOLERANCE, IDC_SUPPORT_PLACEMENT_TOLERANCE_UNITS, m_Entry.m_HaulingSupportPlacementTolerance, pDisplayUnits->ComponentDim );
   DDV_UnitValueGreaterThanZero(pDX, IDC_SUPPORT_PLACEMENT_TOLERANCE,m_Entry.m_HaulingSupportPlacementTolerance, pDisplayUnits->ComponentDim );
	DDX_Text(pDX, IDC_CAMBER_CG_ADJUSTMENT, m_Entry.m_HaulingCamberPercentEstimate);
   DDV_MinMaxDouble(pDX, m_Entry.m_HaulingCamberPercentEstimate, 0.0, 100.0);
	DDX_Percentage(pDX, IDC_IMPACT_UPWARD_HAULING, m_Entry.m_HaulingUpwardImpact);
   DDV_MinMaxDouble(pDX, m_Entry.m_HaulingUpwardImpact, 0.0, 1.0);
	DDX_Percentage(pDX, IDC_IMPACT_DOWNWARD_HAULING, m_Entry.m_HaulingDownwardImpact);
   DDV_MinMaxDouble(pDX, m_Entry.m_HaulingDownwardImpact, 0.0, 1.0);
   DDX_UnitValueAndTag(pDX, IDC_MAXWGT, IDC_MAXWGT_UNITS, m_Entry.m_MaxGirderWgt, pDisplayUnits->GeneralForce );
   DDV_UnitValueGreaterThanZero(pDX, IDC_MAXWGT,m_Entry.m_MaxGirderWgt, pDisplayUnits->GeneralForce );
 
   DDX_KeywordUnitValueAndTag(pDX,IDC_MIN_TRUCK_SUPPORT,IDC_MIN_TRUCK_SUPPORT_UNIT,_T("H"),m_Entry.m_MinHaulPoint, pDisplayUnits->SpanLength);

   DDX_UnitValueAndTag(pDX, IDC_TRUCK_SUPPORT_LOCATION_ACCURACY,IDC_TRUCK_SUPPORT_LOCATION_ACCURACY_UNIT,m_Entry.m_HaulPointAccuracy, pDisplayUnits->SpanLength );
   DDV_UnitValueGreaterThanZero(pDX, IDC_TRUCK_SUPPORT_LOCATION_ACCURACY,m_Entry.m_HaulPointAccuracy, pDisplayUnits->SpanLength );
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

	DDX_Text(pDX, IDC_HAULING_ALLOW_COMP, m_Entry.m_CompStressHauling);
   DDV_GreaterThanZero(pDX, IDC_HAULING_ALLOW_COMP, m_Entry.m_CompStressHauling);

   CString tag = (pApp->GetUnitsMode() == eafTypes::umSI ? _T("sqrt(f'c MPa)") : _T("sqrt(f'c KSI)"));

   DDX_UnitValueAndTag(pDX, IDC_HAULING_MAX_TENSSQRT, IDC_HAULING_MAX_TENSSQRT_UNITS, m_Entry.m_TensStressHauling, pDisplayUnits->SqrtPressure );
   DDX_Text(pDX,IDC_HAUL_TENS,tag);
   DDV_UnitValueZeroOrMore(pDX, IDC_HAUL_TENS,m_Entry.m_TensStressHauling, pDisplayUnits->SqrtPressure );
   DDX_Check_Bool(pDX, IDC_CHECK_HAULING_TENS_MAX, m_Entry.m_DoTensStressHaulingMax);
   DDX_UnitValueAndTag(pDX, IDC_HAULING_TENS_MAX, IDC_HAULING_TENS_MAX_UNITS, m_Entry.m_TensStressHaulingMax, pDisplayUnits->Stress );
   if (m_Entry.m_DoTensStressHaulingMax)
      DDV_UnitValueGreaterThanZero(pDX, IDC_HAULING_TENS_MAX,m_Entry.m_TensStressHaulingMax, pDisplayUnits->Stress );

   DDX_UnitValueAndTag(pDX, IDC_HAULING_MAX_TENSSQRT2, IDC_HAULING_MAX_TENSSQRT_UNITS, m_Entry.m_TensStressHaulingWithRebar, pDisplayUnits->SqrtPressure  );
   DDX_Text(pDX,IDC_HAUL_TENS2,tag);
   DDV_UnitValueZeroOrMore(pDX, IDC_HAULING_MAX_TENSSQRT2,m_Entry.m_TensStressHaulingWithRebar, pDisplayUnits->SqrtPressure );

   if (pDX->m_bSaveAndValidate && m_Entry.m_TensStressHaulingWithRebar < m_Entry.m_TensStressHauling)
   {
      AfxMessageBox(_T("Allowable tensile stress with bonded reinforcement must be greater than or equal to that without"),MB_OK | MB_ICONWARNING);
      pDX->Fail();
   }

	DDX_Text(pDX, IDC_G_OVERHANG, m_Entry.m_OverhangGFactor);
   DDV_MinMaxDouble(pDX, m_Entry.m_OverhangGFactor, 1.0, 100.0);
	DDX_Text(pDX, IDC_G_INTERIOR, m_Entry.m_InteriorGFactor);
   DDV_MinMaxDouble(pDX, m_Entry.m_InteriorGFactor, 1.0, 100.0);

   DDX_KeywordUnitValueAndTag(pDX,IDC_MIN_TRUCK_SUPPORT,IDC_MIN_TRUCK_SUPPORT_UNIT,_T("H"),m_Entry.m_MinHaulPoint, pDisplayUnits->SpanLength);

   DDX_UnitValueAndTag(pDX, IDC_TRUCK_SUPPORT_DESIGN_ACCURACY,IDC_TRUCK_SUPPORT_DESIGN_ACCURACY_UNIT,m_Entry.m_HaulPointAccuracy, pDisplayUnits->SpanLength );
   DDV_UnitValueGreaterThanZero(pDX, IDC_TRUCK_SUPPORT_DESIGN_ACCURACY,m_Entry.m_HaulPointAccuracy, pDisplayUnits->SpanLength );

   DDX_Check_Bool(pDX, IDC_IS_SUPPORT_LESS_THAN, m_Entry.m_UseMinTruckSupportLocationFactor);

	DDX_Text(pDX, IDC_SUPPORT_LESS_THAN, m_Entry.m_MinTruckSupportLocationFactor);
   DDV_MinMaxDouble(pDX, m_Entry.m_MinTruckSupportLocationFactor, 0.0, 0.4);
}


void CSpecMainSheet::ExchangeBs1Data(CDataExchange* pDX)
{
   CEAFApp* pApp = EAFGetApp();
   const unitmgtIndirectMeasure* pDisplayUnits = pApp->GetDisplayUnits();


	DDX_Text(pDX, IDC_TEMP_REMOVE_ALLOW_SERVICE_COMP, m_Entry.m_TempStrandRemovalCompStress);
   DDV_GreaterThanZero(pDX, IDC_TEMP_REMOVE_ALLOW_SERVICE_COMP, m_Entry.m_TempStrandRemovalCompStress);

   DDX_UnitValueAndTag(pDX, IDC_NORMAL_MAX_SQRT3, IDC_NORMAL_MAX_SQRT_UNITS3, m_Entry.m_TempStrandRemovalTensStress, pDisplayUnits->SqrtPressure );
   CString tag = (pApp->GetUnitsMode() == eafTypes::umSI ? _T("sqrt( f'c (MPa) )") : _T("sqrt( f'c (KSI) )"));
   DDX_Text(pDX,IDC_CYS_TENS_BYLINE2,tag);
   DDV_UnitValueZeroOrMore(pDX,IDC_CYS_TENS_BYLINE2, m_Entry.m_TempStrandRemovalTensStress, pDisplayUnits->SqrtPressure );
   DDX_Check_Bool(pDX, IDC_CHECK_NORMAL_MAX_MAX3, m_Entry.m_TempStrandRemovalDoTensStressMax);
   DDX_UnitValueAndTag(pDX, IDC_NORMAL_MAX_MAX3, IDC_NORMAL_MAX_MAX_UNITS3, m_Entry.m_TempStrandRemovalTensStressMax, pDisplayUnits->Stress );
   if (m_Entry.m_TempStrandRemovalDoTensStressMax)
      DDV_UnitValueGreaterThanZero(pDX, IDC_NORMAL_MAX_MAX3,m_Entry.m_TempStrandRemovalTensStressMax, pDisplayUnits->Stress );



	DDX_Text(pDX, IDC_CY_ALLOW_SERVICE_COMP, m_Entry.m_Bs1CompStress);
   DDV_GreaterThanZero(pDX, IDC_CY_ALLOW_SERVICE_COMP, m_Entry.m_Bs1CompStress);

   DDX_UnitValueAndTag(pDX, IDC_NORMAL_MAX_SQRT2, IDC_NORMAL_MAX_SQRT_UNITS, m_Entry.m_Bs1TensStress, pDisplayUnits->SqrtPressure );
   DDX_Text(pDX,IDC_CYS_TENS_BYLINE,tag);
   DDV_UnitValueZeroOrMore(pDX, IDC_CYS_TENS_BYLINE,m_Entry.m_Bs1TensStress, pDisplayUnits->SqrtPressure );
   DDX_Check_Bool(pDX, IDC_CHECK_NORMAL_MAX_MAX2, m_Entry.m_Bs1DoTensStressMax);
   DDX_UnitValueAndTag(pDX, IDC_NORMAL_MAX_MAX2, IDC_NORMAL_MAX_MAX_UNITS2, m_Entry.m_Bs1TensStressMax, pDisplayUnits->Stress );
   if (m_Entry.m_Bs1DoTensStressMax)
      DDV_UnitValueGreaterThanZero(pDX, IDC_NORMAL_MAX_MAX2,m_Entry.m_Bs1TensStressMax, pDisplayUnits->Stress );
}

void CSpecMainSheet::ExchangeBs2Data(CDataExchange* pDX)
{
	DDX_Text(pDX, IDC_CY_ALLOW_SERVICE_COMP, m_Entry.m_Bs2CompStress);
   DDV_GreaterThanZero(pDX, IDC_CY_ALLOW_SERVICE_COMP, m_Entry.m_Bs2CompStress);
   DDX_CBItemData(pDX,IDC_DIST_TRAFFIC_BARRIER_BASIS,m_Entry.m_TrafficBarrierDistributionType);

	DDX_Text(pDX, IDC_DIST_TRAFFIC_BARRIER, m_Entry.m_Bs2MaxGirdersTrafficBarrier);
	DDX_CBEnum(pDX, IDC_OVERLAY_DISTR,m_Entry.m_OverlayLoadDistribution);
   DDX_CBEnum(pDX,IDC_EFF_FLANGE_WIDTH,m_Entry.m_EffFlangeWidthMethod);
}

void CSpecMainSheet::ExchangeBsData(CDataExchange* pDX)
{
   CEAFApp* pApp = EAFGetApp();
   const unitmgtIndirectMeasure* pDisplayUnits = pApp->GetDisplayUnits();

	DDX_Text(pDX, IDC_BS_COMP_STRESS_SERV, m_Entry.m_Bs3CompStressServ);
   DDV_GreaterThanZero(pDX, IDC_BS_COMP_STRESS_SERV, m_Entry.m_Bs3CompStressServ);
 	DDX_Text(pDX, IDC_BS_COMP_STRESS_SERVICE1A, m_Entry.m_Bs3CompStressService1A);
   DDV_GreaterThanZero(pDX, IDC_BS_COMP_STRESS_SERVICE1A, m_Entry.m_Bs3CompStressService1A);

   DDX_UnitValueAndTag(pDX, IDC_NORMAL_MAX_SQRT, IDC_NORMAL_MAX_SQRT_UNITS, m_Entry.m_Bs3TensStressServNc, pDisplayUnits->SqrtPressure );
   CString tag = (pApp->GetUnitsMode() == eafTypes::umSI ? _T("sqrt( f'c (MPa) )") : _T("sqrt( f'c (KSI) )"));
   DDX_Text(pDX,IDC_BS_NORMAL_TENS,tag);
   DDV_UnitValueZeroOrMore(pDX, IDC_BS_NORMAL_TENS,m_Entry.m_Bs3TensStressServNc, pDisplayUnits->SqrtPressure );
   DDX_Check_Bool(pDX, IDC_CHECK_NORMAL_MAX_MAX, m_Entry.m_Bs3DoTensStressServNcMax);
   DDX_UnitValueAndTag(pDX, IDC_NORMAL_MAX_MAX, IDC_NORMAL_MAX_MAX_UNITS, m_Entry.m_Bs3TensStressServNcMax, pDisplayUnits->Stress );
   if (m_Entry.m_Bs3DoTensStressServNcMax)
      DDV_UnitValueGreaterThanZero(pDX, IDC_NORMAL_MAX_MAX,m_Entry.m_Bs3TensStressServNcMax, pDisplayUnits->Stress );

   DDX_UnitValueAndTag(pDX, IDC_EXTREME_MAX_SQRT, IDC_EXTREME_MAX_SQRT_UNITS, m_Entry.m_Bs3TensStressServSc, pDisplayUnits->SqrtPressure );
   DDX_Text(pDX,IDC_BS_EXTREME_TENS,tag);
   DDV_UnitValueZeroOrMore(pDX, IDC_BS_EXTREME_TENS,m_Entry.m_Bs3TensStressServSc, pDisplayUnits->SqrtPressure );
   DDX_Check_Bool(pDX, IDC_CHECK_EXTREME_MAX_MAX, m_Entry.m_Bs3DoTensStressServScMax);
   DDX_UnitValueAndTag(pDX, IDC_EXTREME_MAX_MAX, IDC_EXTREME_MAX_MAX_UNITS, m_Entry.m_Bs3TensStressServScMax, pDisplayUnits->Stress );
   if (m_Entry.m_Bs3DoTensStressServScMax)
      DDV_UnitValueGreaterThanZero(pDX, IDC_EXTREME_MAX_MAX,m_Entry.m_Bs3TensStressServScMax, pDisplayUnits->Stress );

   DDX_CBIndex(pDX, IDC_LLDF, m_Entry.m_LldfMethod);

   DDX_UnitValueAndTag(pDX, IDC_MAXGIRDERANGLE, IDC_MAXGIRDERANGLE_UNIT, m_Entry.m_MaxAngularDeviationBetweenGirders, pDisplayUnits->Angle );
   DDX_Text(pDX,IDC_GIRDERSTIFFNESSRATIO,m_Entry.m_MinGirderStiffnessRatio);
   DDV_MinMaxDouble(pDX,m_Entry.m_MinGirderStiffnessRatio,0.0,1.0);

   DDX_Text(pDX,IDC_GIRDER_SPACING_LOCATION,m_Entry.m_LLDFGirderSpacingLocation);
   DDV_MinMaxDouble(pDX,m_Entry.m_LLDFGirderSpacingLocation,0.0,1.0);

   DDX_Check_Bool(pDX, IDC_LANESBEAMS, m_Entry.m_LimitDistributionFactorsToLanesBeams);

   DDX_UnitValueAndTag(pDX, IDC_PED_LIVE_LOAD, IDC_PED_LIVE_LOAD_UNIT, m_Entry.m_PedestrianLoad, pDisplayUnits->SmallStress );
   DDV_UnitValueZeroOrMore(pDX, IDC_PED_LIVE_LOAD,m_Entry.m_PedestrianLoad, pDisplayUnits->SpanLength );
   DDX_UnitValueAndTag(pDX, IDC_MIN_SIDEWALK_WIDTH, IDC_MIN_SIDEWALK_WIDTH_UNIT, m_Entry.m_MinSidewalkWidth, pDisplayUnits->SpanLength );
   DDV_UnitValueZeroOrMore(pDX, IDC_MIN_SIDEWALK_WIDTH,m_Entry.m_MinSidewalkWidth, pDisplayUnits->SpanLength );
}

void CSpecMainSheet::ExchangeMomentCapacityData(CDataExchange* pDX)
{
   CEAFApp* pApp = EAFGetApp();
   const unitmgtIndirectMeasure* pDisplayUnits = pApp->GetDisplayUnits();

   DDX_CBIndex(pDX, IDC_MOMENT, m_Entry.m_Bs3LRFDOverReinforcedMomentCapacity );
   DDX_CBEnum(pDX, IDC_NEG_MOMENT, m_Entry.m_bIncludeForNegMoment);

   DDX_Check_Bool(pDX, IDC_INCLUDE_REBAR_MOMENT, m_Entry.m_bIncludeRebar_Moment );

   CString tag = (pApp->GetUnitsMode() == eafTypes::umSI ? _T("sqrt( f'c (MPa) )") : _T("sqrt( f'c (KSI) )"));
   DDX_UnitValueAndTag(pDX, IDC_FR,      IDC_FR_LABEL,      m_Entry.m_FlexureModulusOfRuptureCoefficient[pgsTypes::Normal],          pDisplayUnits->SqrtPressure );
   DDX_UnitValueAndTag(pDX, IDC_ALWC_FR, IDC_ALWC_FR_LABEL, m_Entry.m_FlexureModulusOfRuptureCoefficient[pgsTypes::AllLightweight],  pDisplayUnits->SqrtPressure );
   DDX_UnitValueAndTag(pDX, IDC_SLWC_FR, IDC_SLWC_FR_LABEL, m_Entry.m_FlexureModulusOfRuptureCoefficient[pgsTypes::SandLightweight], pDisplayUnits->SqrtPressure );

   DDX_Text(pDX,IDC_FR_UNIT,     tag);
   DDX_Text(pDX,IDC_ALWC_FR_UNIT,tag);
   DDX_Text(pDX,IDC_SLWC_FR_UNIT,tag);

   DDV_UnitValueZeroOrMore(pDX, IDC_FR,      m_Entry.m_FlexureModulusOfRuptureCoefficient[pgsTypes::Normal],          pDisplayUnits->SqrtPressure );
   DDV_UnitValueZeroOrMore(pDX, IDC_ALWC_FR, m_Entry.m_FlexureModulusOfRuptureCoefficient[pgsTypes::AllLightweight],  pDisplayUnits->SqrtPressure );
   DDV_UnitValueZeroOrMore(pDX, IDC_SLWC_FR, m_Entry.m_FlexureModulusOfRuptureCoefficient[pgsTypes::SandLightweight], pDisplayUnits->SqrtPressure );

   DDX_Text(pDX,IDC_NWC_PHI_TENSION_RC,  m_Entry.m_PhiFlexureTensionRC[pgsTypes::Normal]);
   DDX_Text(pDX,IDC_NWC_PHI_TENSION_PS,  m_Entry.m_PhiFlexureTensionPS[pgsTypes::Normal]);
   DDX_Text(pDX,IDC_NWC_PHI_COMPRESSION, m_Entry.m_PhiFlexureCompression[pgsTypes::Normal]);

   DDX_Text(pDX,IDC_LWC_PHI_TENSION_RC,  m_Entry.m_PhiFlexureTensionRC[pgsTypes::AllLightweight]);
   DDX_Text(pDX,IDC_LWC_PHI_TENSION_PS,  m_Entry.m_PhiFlexureTensionPS[pgsTypes::AllLightweight]);
   DDX_Text(pDX,IDC_LWC_PHI_COMPRESSION, m_Entry.m_PhiFlexureCompression[pgsTypes::AllLightweight]);

   DDX_Text(pDX,IDC_LWC_PHI_TENSION_RC,  m_Entry.m_PhiFlexureTensionRC[pgsTypes::SandLightweight]);
   DDX_Text(pDX,IDC_LWC_PHI_TENSION_PS,  m_Entry.m_PhiFlexureTensionPS[pgsTypes::SandLightweight]);
   DDX_Text(pDX,IDC_LWC_PHI_COMPRESSION, m_Entry.m_PhiFlexureCompression[pgsTypes::SandLightweight]);

   // NOTE: this looks goofy, but it is correct. There is only one LWC entry for both all and sand lightweight
   // but it is easier to have 3 sets of values so the application is consistent.
}

void CSpecMainSheet::CheckShearCapacityMethod()
{
   // makes sure the shear capacity method is consistent with the specification version that is selected
   if ( GetSpecVersion() < lrfdVersionMgr::FourthEdition2007 &&  // if we are before 4th Edition
        m_Entry.m_ShearCapacityMethod == scmVciVcw ) // Vci/Vcw is not a valid option
   {
      // force to the general method
      if ( GetSpecVersion() <= lrfdVersionMgr::FourthEdition2007 )
         m_Entry.m_ShearCapacityMethod = scmBTTables;
      else
         m_Entry.m_ShearCapacityMethod = scmBTEquations;
   }

   // The general method from the 2007 spec becomes the tables method in the 2008 spec
   // make that adjustment here
   if ( GetSpecVersion() < lrfdVersionMgr::FourthEditionWith2008Interims && m_Entry.m_ShearCapacityMethod == scmBTEquations )
   {
      m_Entry.m_ShearCapacityMethod = scmBTTables;
   }

   if ( GetSpecVersion() < lrfdVersionMgr::SecondEditionWith2000Interims &&  // if we are before 2nd Edition + 2000
        m_Entry.m_ShearCapacityMethod == scmWSDOT2001 ) // WSDOT 2001 is not a valid option
   {
      // force to the general method
      if ( GetSpecVersion() <= lrfdVersionMgr::FourthEdition2007 )
         m_Entry.m_ShearCapacityMethod = scmBTTables;
      else
         m_Entry.m_ShearCapacityMethod = scmBTEquations;
   }

   if ( GetSpecVersion() < lrfdVersionMgr::FourthEdition2007 &&  // if we are before 4th Edition
        m_Entry.m_ShearCapacityMethod == scmWSDOT2007 ) // WSDOT 2007 is not a valid option
   {
      m_Entry.m_ShearCapacityMethod = scmWSDOT2001; // force to WSDOT 2001
   }
}

void CSpecMainSheet::ExchangeShearCapacityData(CDataExchange* pDX)
{
   CEAFApp* pApp = EAFGetApp();
   const unitmgtIndirectMeasure* pDisplayUnits = pApp->GetDisplayUnits();

   DDX_CBIndex(pDX, IDC_LRSH, m_Entry.m_LongReinfShearMethod); 
   DDX_Check_Bool(pDX, IDC_INCLUDE_REBAR_SHEAR, m_Entry.m_bIncludeRebar_Shear );

   DDX_CBEnum(pDX,IDC_SHEAR_FLOW_METHOD,m_Entry.m_ShearFlowMethod);

   // we have to use item data here because the enum constants are out of order
   DDX_CBItemData(pDX, IDC_SHEAR_CAPACITY, m_Entry.m_ShearCapacityMethod );

   // Not every shear method is available with every spec. The user could have set the method
   // then changed the spec. We have keep the data valid

   if ( pDX->m_bSaveAndValidate )
      CheckShearCapacityMethod();

   CString tag = (pApp->GetUnitsMode() == eafTypes::umSI ? _T("sqrt( f'c (MPa) )") : _T("sqrt( f'c (KSI) )"));
   DDX_UnitValueAndTag(pDX, IDC_FR,     IDC_FR_LABEL,     m_Entry.m_ShearModulusOfRuptureCoefficient[pgsTypes::Normal], pDisplayUnits->SqrtPressure );
   DDX_UnitValueAndTag(pDX, IDC_ALWC_FR, IDC_FR_LABEL_ALWC, m_Entry.m_ShearModulusOfRuptureCoefficient[pgsTypes::AllLightweight], pDisplayUnits->SqrtPressure );
   DDX_UnitValueAndTag(pDX, IDC_SLWC_FR, IDC_FR_LABEL_SLWC, m_Entry.m_ShearModulusOfRuptureCoefficient[pgsTypes::SandLightweight], pDisplayUnits->SqrtPressure );

   DDX_Text(pDX,IDC_FR_UNIT,tag);
   DDX_Text(pDX,IDC_ALWC_FR_UNIT,tag);
   DDX_Text(pDX,IDC_SLWC_FR_UNIT,tag);

   DDV_UnitValueZeroOrMore(pDX, IDC_FR ,     m_Entry.m_ShearModulusOfRuptureCoefficient[pgsTypes::Normal],          pDisplayUnits->SqrtPressure );
   DDV_UnitValueZeroOrMore(pDX, IDC_SLWC_FR, m_Entry.m_ShearModulusOfRuptureCoefficient[pgsTypes::SandLightweight], pDisplayUnits->SqrtPressure );
   DDV_UnitValueZeroOrMore(pDX, IDC_ALWC_FR, m_Entry.m_ShearModulusOfRuptureCoefficient[pgsTypes::AllLightweight],  pDisplayUnits->SqrtPressure );

   DDX_Text(pDX,IDC_NWC_PHI,m_Entry.m_PhiShear[pgsTypes::Normal]);
   DDX_Text(pDX,IDC_LWC_PHI,m_Entry.m_PhiShear[pgsTypes::SandLightweight]);
   DDX_Text(pDX,IDC_LWC_PHI,m_Entry.m_PhiShear[pgsTypes::AllLightweight]);
}

void CSpecMainSheet::ExchangeDeflectionsData(CDataExchange* pDX)
{
   DDX_Check_Bool(pDX, IDC_LL_DEFLECTION, m_Entry.m_bDoEvaluateDeflection );

 	DDX_Text(pDX, IDC_DEFLECTION_LIMIT, m_Entry.m_DeflectionLimit);
   DDV_GreaterThanZero(pDX, IDC_DEFLECTION_LIMIT, m_Entry.m_DeflectionLimit);
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
         AfxMessageBox(_T("The time from prestress transfer to slab casting must be greater than the time from prestress transfer to temporary strand removal/diaphragm casting."));
         pDX->Fail();
      }

      if ( m_Entry.m_CreepDuration2Max < m_Entry.m_CreepDuration1Max )
      {
         pDX->PrepareEditCtrl(IDC_CREEP_DURATION2_MAX);
         AfxMessageBox(_T("The time from prestress transfer to slab casting must be greater than the time from prestress transfer to temporary strand removal/diaphragm casting."));
         pDX->Fail();
      }
   }

   DDX_UnitValueAndTag(pDX, IDC_NC_CREEP, IDC_NC_CREEP_TAG, m_Entry.m_TotalCreepDuration, pDisplayUnits->Time2 );
   DDV_UnitValueGreaterThanLimit(pDX, IDC_NC_CREEP,m_Entry.m_TotalCreepDuration, m_Entry.m_CreepDuration2Min, pDisplayUnits->Time2 );

   DDX_Text(pDX,IDC_CURING_TIME_FACTOR,m_Entry.m_CuringMethodTimeAdjustmentFactor);
   DDV_MinMaxDouble(pDX,m_Entry.m_CuringMethodTimeAdjustmentFactor,1,999);

   DDX_Percentage(pDX,IDC_CAMBER_VARIABILITY, m_Entry.m_CamberVariability);
   if ( pDX->m_bSaveAndValidate )
   {
      Float64 val = m_Entry.m_CamberVariability * 100.0;
      DDV_Range( pDX, mfcDDV::LE,mfcDDV::GE,val,0.0,100.0);
   }
}

void CSpecMainSheet::ExchangeLossData(CDataExchange* pDX)
{
   CEAFApp* pApp = EAFGetApp();
   const unitmgtIndirectMeasure* pDisplayUnits = pApp->GetDisplayUnits();

   DDX_UnitValueAndTag(pDX, IDC_FINAL, IDC_FINAL_TAG, m_Entry.m_FinalLosses, pDisplayUnits->Stress);
	DDX_UnitValueAndTag(pDX, IDC_BEFORE_XFER, IDC_BEFORE_XFER_TAG, m_Entry.m_BeforeXferLosses, pDisplayUnits->Stress);
	DDX_UnitValueAndTag(pDX, IDC_AFTER_XFER, IDC_AFTER_XFER_TAG, m_Entry.m_AfterXferLosses, pDisplayUnits->Stress);
   DDX_UnitValueAndTag(pDX, IDC_LIFTING, IDC_LIFTING_TAG, m_Entry.m_LiftingLosses, pDisplayUnits->Stress);
   DDX_UnitValueAndTag(pDX, IDC_BEFORE_TEMP_STRAND_REMOVAL, IDC_BEFORE_TEMP_STRAND_REMOVAL_TAG, m_Entry.m_BeforeTempStrandRemovalLosses, pDisplayUnits->Stress);
   DDX_UnitValueAndTag(pDX, IDC_AFTER_TEMP_STRAND_REMOVAL,  IDC_AFTER_TEMP_STRAND_REMOVAL_TAG,  m_Entry.m_AfterTempStrandRemovalLosses, pDisplayUnits->Stress);
   DDX_UnitValueAndTag(pDX, IDC_AFTER_DECK_PLACEMENT,  IDC_AFTER_DECK_PLACEMENT_TAG,  m_Entry.m_AfterDeckPlacementLosses, pDisplayUnits->Stress);
   DDX_UnitValueAndTag(pDX, IDC_AFTER_SIDL,  IDC_AFTER_SIDL_TAG,  m_Entry.m_AfterSIDLLosses, pDisplayUnits->Stress);

   DDX_UnitValueAndTag(pDX, IDC_SHIPPING_TIME, IDC_SHIPPING_TIME_TAG, m_Entry.m_ShippingTime, pDisplayUnits->Time2);

   DDX_UnitValueAndTag(pDX, IDC_ANCHORSET,  IDC_ANCHORSET_TAG,  m_Entry.m_Dset, pDisplayUnits->ComponentDim);
   DDX_UnitValueAndTag(pDX, IDC_WOBBLE, IDC_WOBBLE_TAG,m_Entry.m_WobbleFriction, pDisplayUnits->PerLength);
   DDX_Text(pDX,IDC_FRICTION,m_Entry.m_FrictionCoefficient);

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
   int map[] ={LOSSES_AASHTO_REFINED,
               LOSSES_WSDOT_REFINED,
               LOSSES_TXDOT_REFINED_2004,
               LOSSES_TXDOT_REFINED_2013,
               LOSSES_AASHTO_LUMPSUM,
               LOSSES_WSDOT_LUMPSUM,
               LOSSES_GENERAL_LUMPSUM};

   int map_size = sizeof(map)/sizeof(int);

   if ( pDX->m_bSaveAndValidate )
   {
      // data is coming out of the dialog
      int rad_ord;
      DDX_CBIndex(pDX,IDC_LOSS_METHOD,rad_ord);
      CHECK(0 <= rad_ord && rad_ord < map_size);
      m_Entry.m_LossMethod = map[rad_ord];

      if ( m_Entry.m_LossMethod == LOSSES_AASHTO_REFINED     || m_Entry.m_LossMethod == LOSSES_WSDOT_REFINED || 
           m_Entry.m_LossMethod == LOSSES_AASHTO_LUMPSUM     || m_Entry.m_LossMethod == LOSSES_WSDOT_LUMPSUM ||
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
      else if (m_Entry.m_LossMethod == LOSSES_GENERAL_LUMPSUM )
      {
         DDX_UnitValueAndTag(pDX, IDC_SHIPPING2, IDC_SHIPPING2_TAG, m_Entry.m_ShippingLosses, pDisplayUnits->Stress);
      }
      else
      {
         m_Entry.m_ShippingLosses = 0;
      }

      // transfer length data
      DDX_CBIndex(pDX,IDC_COMBO_TRANSFER,rad_ord);
      CHECK(0 <= rad_ord && rad_ord < 2);
      m_Entry.m_PrestressTransferComputationType = (rad_ord==1) ? pgsTypes::ptMinuteValue : pgsTypes::ptUsingSpecification;
   }
   else
   {
      // Data is going into the dialog
      // check buttons - map to loss method
      int idx=0;
      while(m_Entry.m_LossMethod != map[idx])
      {
         CHECK(idx<map_size);
         if (map_size < idx)
         {
            idx = 0;
            break;
         }
         idx++;
      }

      DDX_CBIndex(pDX,IDC_LOSS_METHOD,idx);

      Float64 dummy = 0;
      if ( m_Entry.m_LossMethod == LOSSES_AASHTO_REFINED     || m_Entry.m_LossMethod == LOSSES_WSDOT_REFINED || 
           m_Entry.m_LossMethod == LOSSES_AASHTO_LUMPSUM     || m_Entry.m_LossMethod == LOSSES_WSDOT_LUMPSUM ||
           m_Entry.m_LossMethod == LOSSES_TXDOT_REFINED_2004 ||  m_Entry.m_LossMethod == LOSSES_TXDOT_REFINED_2013)
      {
         if ( m_Entry.m_ShippingLosses < 0 )
         {
            /// Shipping losses are fractional
            Float64 value = m_Entry.m_ShippingLosses * -100.0;
            DDX_Text(pDX,IDC_SHIPPING,value);

            DDX_UnitValueAndTag(pDX, IDC_SHIPPING2, IDC_SHIPPING2_TAG, m_Entry.m_LiftingLosses, pDisplayUnits->Stress);

            CString strTag(_T("%"));
            DDX_Text(pDX,IDC_SHIPPING_TAG,strTag);

            idx = 1;
            DDX_CBIndex(pDX,IDC_SHIPPING_LOSS_METHOD,idx);
         }
         else
         {
      	   DDX_UnitValueAndTag(pDX, IDC_SHIPPING,  IDC_SHIPPING_TAG,  m_Entry.m_ShippingLosses, pDisplayUnits->Stress);
            DDX_UnitValueAndTag(pDX, IDC_SHIPPING2, IDC_SHIPPING2_TAG, m_Entry.m_ShippingLosses, pDisplayUnits->Stress);

            idx = 0;
            DDX_CBIndex(pDX,IDC_SHIPPING_LOSS_METHOD,idx);
         }

      }
      else if ( m_Entry.m_LossMethod == LOSSES_GENERAL_LUMPSUM )
      {
   	   DDX_UnitValueAndTag(pDX, IDC_SHIPPING, IDC_SHIPPING_TAG, dummy, pDisplayUnits->Stress);
   	   DDX_UnitValueAndTag(pDX, IDC_SHIPPING2, IDC_SHIPPING2_TAG, m_Entry.m_ShippingLosses, pDisplayUnits->Stress);
      }
      else
      {
         ATLASSERT(FALSE); // should never get here
      }

      // transfer length comp
      idx = m_Entry.m_PrestressTransferComputationType==pgsTypes::ptMinuteValue ? 1 : 0;
      DDX_CBIndex(pDX,IDC_COMBO_TRANSFER,idx);

   }
}

void CSpecMainSheet::ExchangePSLimitData(CDataExchange* pDX)
{
   DDX_Check_Bool(pDX, IDC_CHECK_PS_AT_JACKING, m_Entry.m_bCheckStrandStress[AT_JACKING]);
 	DDX_Text(pDX, IDC_PS_AT_JACKING_SR, m_Entry.m_StrandStressCoeff[AT_JACKING][STRESS_REL]);
   DDV_GreaterThanZero(pDX, IDC_PS_AT_JACKING_SR, m_Entry.m_StrandStressCoeff[AT_JACKING][STRESS_REL]);
 	DDX_Text(pDX, IDC_PS_AT_JACKING_LR, m_Entry.m_StrandStressCoeff[AT_JACKING][LOW_RELAX]);
   DDV_GreaterThanZero(pDX, IDC_PS_AT_JACKING_LR, m_Entry.m_StrandStressCoeff[AT_JACKING][LOW_RELAX]);

   DDX_Check_Bool(pDX, IDC_CHECK_PS_BEFORE_TRANSFER, m_Entry.m_bCheckStrandStress[BEFORE_TRANSFER]);
 	DDX_Text(pDX, IDC_PS_BEFORE_TRANSFER_SR, m_Entry.m_StrandStressCoeff[BEFORE_TRANSFER][STRESS_REL]);
   DDV_GreaterThanZero(pDX, IDC_PS_BEFORE_TRANSFER_SR, m_Entry.m_StrandStressCoeff[BEFORE_TRANSFER][STRESS_REL]);
 	DDX_Text(pDX, IDC_PS_BEFORE_TRANSFER_LR, m_Entry.m_StrandStressCoeff[BEFORE_TRANSFER][LOW_RELAX]);
   DDV_GreaterThanZero(pDX, IDC_PS_BEFORE_TRANSFER_LR, m_Entry.m_StrandStressCoeff[BEFORE_TRANSFER][LOW_RELAX]);

   DDX_Check_Bool(pDX, IDC_CHECK_PS_AFTER_TRANSFER, m_Entry.m_bCheckStrandStress[AFTER_TRANSFER]);
 	DDX_Text(pDX, IDC_PS_AFTER_TRANSFER_SR, m_Entry.m_StrandStressCoeff[AFTER_TRANSFER][STRESS_REL]);
   DDV_GreaterThanZero(pDX, IDC_PS_AFTER_TRANSFER_SR, m_Entry.m_StrandStressCoeff[AFTER_TRANSFER][STRESS_REL]);
 	DDX_Text(pDX, IDC_PS_AFTER_TRANSFER_LR, m_Entry.m_StrandStressCoeff[AFTER_TRANSFER][LOW_RELAX]);
   DDV_GreaterThanZero(pDX, IDC_PS_AFTER_TRANSFER_LR, m_Entry.m_StrandStressCoeff[AFTER_TRANSFER][LOW_RELAX]);

//   DDX_Check_Bool(pDX, IDC_CHECK_PS_AFTER_ALL_LOSSES, m_Entry.m_bCheckStrandStress[AFTER_ALL_LOSSES]);
   m_Entry.m_bCheckStrandStress[AFTER_ALL_LOSSES] = true;
 	DDX_Text(pDX, IDC_PS_AFTER_ALL_LOSSES_SR, m_Entry.m_StrandStressCoeff[AFTER_ALL_LOSSES][STRESS_REL]);
   DDV_GreaterThanZero(pDX, IDC_PS_AFTER_ALL_LOSSES_SR, m_Entry.m_StrandStressCoeff[AFTER_ALL_LOSSES][STRESS_REL]);
 	DDX_Text(pDX, IDC_PS_AFTER_ALL_LOSSES_LR, m_Entry.m_StrandStressCoeff[AFTER_ALL_LOSSES][LOW_RELAX]);
   DDV_GreaterThanZero(pDX, IDC_PS_AFTER_ALL_LOSSES_LR, m_Entry.m_StrandStressCoeff[AFTER_ALL_LOSSES][LOW_RELAX]);
}

void CSpecMainSheet::ExchangeLimitsData(CDataExchange* pDX)
{
   CEAFApp* pApp = EAFGetApp();
   const unitmgtIndirectMeasure* pDisplayUnits = pApp->GetDisplayUnits();

   DDX_UnitValueAndTag(pDX, IDC_NWC_FC_SLAB,     IDC_NWC_FC_SLAB_UNIT,     m_Entry.m_MaxSlabFc[pgsTypes::Normal],             pDisplayUnits->Stress);
   DDX_UnitValueAndTag(pDX, IDC_NWC_GIRDER_FCI,  IDC_NWC_GIRDER_FCI_UNIT,  m_Entry.m_MaxGirderFci[pgsTypes::Normal],          pDisplayUnits->Stress);
   DDX_UnitValueAndTag(pDX, IDC_NWC_GIRDER_FC,   IDC_NWC_GIRDER_FC_UNIT,   m_Entry.m_MaxGirderFc[pgsTypes::Normal],           pDisplayUnits->Stress);
   DDX_UnitValueAndTag(pDX, IDC_NWC_UNIT_WEIGHT, IDC_NWC_UNIT_WEIGHT_UNIT, m_Entry.m_MaxConcreteUnitWeight[pgsTypes::Normal], pDisplayUnits->Density);
   DDX_UnitValueAndTag(pDX, IDC_NWC_AGG_SIZE,    IDC_NWC_AGG_SIZE_UNIT,    m_Entry.m_MaxConcreteAggSize[pgsTypes::Normal],    pDisplayUnits->ComponentDim);

   DDX_UnitValueAndTag(pDX, IDC_LWC_FC_SLAB,     IDC_LWC_FC_SLAB_UNIT,     m_Entry.m_MaxSlabFc[pgsTypes::SandLightweight],             pDisplayUnits->Stress);
   DDX_UnitValueAndTag(pDX, IDC_LWC_GIRDER_FCI,  IDC_LWC_GIRDER_FCI_UNIT,  m_Entry.m_MaxGirderFci[pgsTypes::SandLightweight],          pDisplayUnits->Stress);
   DDX_UnitValueAndTag(pDX, IDC_LWC_GIRDER_FC,   IDC_LWC_GIRDER_FC_UNIT,   m_Entry.m_MaxGirderFc[pgsTypes::SandLightweight],           pDisplayUnits->Stress);
   DDX_UnitValueAndTag(pDX, IDC_LWC_UNIT_WEIGHT, IDC_LWC_UNIT_WEIGHT_UNIT, m_Entry.m_MaxConcreteUnitWeight[pgsTypes::SandLightweight], pDisplayUnits->Density);
   DDX_UnitValueAndTag(pDX, IDC_LWC_AGG_SIZE,    IDC_LWC_AGG_SIZE_UNIT,    m_Entry.m_MaxConcreteAggSize[pgsTypes::SandLightweight],    pDisplayUnits->ComponentDim);

   DDX_UnitValueAndTag(pDX, IDC_LWC_FC_SLAB,     IDC_LWC_FC_SLAB_UNIT,     m_Entry.m_MaxSlabFc[pgsTypes::AllLightweight],             pDisplayUnits->Stress);
   DDX_UnitValueAndTag(pDX, IDC_LWC_GIRDER_FCI,  IDC_LWC_GIRDER_FCI_UNIT,  m_Entry.m_MaxGirderFci[pgsTypes::AllLightweight],          pDisplayUnits->Stress);
   DDX_UnitValueAndTag(pDX, IDC_LWC_GIRDER_FC,   IDC_LWC_GIRDER_FC_UNIT,   m_Entry.m_MaxGirderFc[pgsTypes::AllLightweight],           pDisplayUnits->Stress);
   DDX_UnitValueAndTag(pDX, IDC_LWC_UNIT_WEIGHT, IDC_LWC_UNIT_WEIGHT_UNIT, m_Entry.m_MaxConcreteUnitWeight[pgsTypes::AllLightweight], pDisplayUnits->Density);
   DDX_UnitValueAndTag(pDX, IDC_LWC_AGG_SIZE,    IDC_LWC_AGG_SIZE_UNIT,    m_Entry.m_MaxConcreteAggSize[pgsTypes::AllLightweight],    pDisplayUnits->ComponentDim);

   DDX_Check_Bool(pDX, IDC_CHECK_STIRRUP_COMPATIBILITY, m_Entry.m_DoCheckStirrupSpacingCompatibility);
}
 
void CSpecMainSheet::UploadDesignData(CDataExchange* pDX)
{
   m_SpecDesignPage.m_CheckA = m_Entry.m_EnableSlabOffsetCheck;
   m_SpecDesignPage.m_CheckHauling = m_Entry.m_EnableHaulingCheck;
   m_SpecDesignPage.m_CheckHoldDown = m_Entry.m_DoCheckHoldDown;
   m_SpecDesignPage.m_CheckLifting = m_Entry.m_EnableLiftingCheck;
   m_SpecDesignPage.m_CheckSlope = m_Entry.m_DoCheckStrandSlope;
   m_SpecDesignPage.m_CheckSplitting = m_Entry.m_DoCheckSplitting;
   m_SpecDesignPage.m_CheckConfinement = m_Entry.m_DoCheckConfinement;

   m_SpecDesignPage.m_DesignA = m_Entry.m_EnableSlabOffsetDesign;
   m_SpecDesignPage.m_DesignHauling = m_Entry.m_EnableHaulingDesign;
   m_SpecDesignPage.m_DesignHoldDown = m_Entry.m_DoDesignHoldDown;
   m_SpecDesignPage.m_DesignLifting = m_Entry.m_EnableLiftingDesign;
   m_SpecDesignPage.m_DesignSlope = m_Entry.m_DoDesignStrandSlope;
   m_SpecDesignPage.m_DesignSplitting = m_Entry.m_DoDesignSplitting;
   m_SpecDesignPage.m_DesignConfinement = m_Entry.m_DoDesignConfinement;

   m_SpecDesignPage.m_FillMethod = (int)m_Entry.m_DesignStrandFillType;
}

// mini function to convert BOOL to bool
inline bool B2b(BOOL val) { return val!=0; }


void CSpecMainSheet::DownloadDesignData(CDataExchange* pDX)
{
   m_Entry.m_EnableSlabOffsetCheck = B2b( m_SpecDesignPage.m_CheckA);
   m_Entry.m_EnableHaulingCheck = B2b( m_SpecDesignPage.m_CheckHauling);
   m_Entry.m_DoCheckHoldDown    = B2b( m_SpecDesignPage.m_CheckHoldDown);
   m_Entry.m_EnableLiftingCheck = B2b( m_SpecDesignPage.m_CheckLifting);
   m_Entry.m_DoCheckStrandSlope = B2b( m_SpecDesignPage.m_CheckSlope);
   m_Entry.m_DoCheckSplitting = B2b( m_SpecDesignPage.m_CheckSplitting);
   m_Entry.m_DoCheckConfinement = B2b( m_SpecDesignPage.m_CheckConfinement);

   m_Entry.m_EnableSlabOffsetDesign = B2b( m_SpecDesignPage.m_DesignA);
   m_Entry.m_EnableHaulingDesign = B2b( m_SpecDesignPage.m_DesignHauling);
   m_Entry.m_DoDesignHoldDown    = B2b( m_SpecDesignPage.m_DesignHoldDown);
   m_Entry.m_EnableLiftingDesign = B2b( m_SpecDesignPage.m_DesignLifting);
   m_Entry.m_DoDesignStrandSlope = B2b( m_SpecDesignPage.m_DesignSlope);
   m_Entry.m_DoDesignSplitting = B2b( m_SpecDesignPage.m_DesignSplitting);
   m_Entry.m_DoDesignConfinement = B2b( m_SpecDesignPage.m_DesignConfinement);

   m_Entry.m_DesignStrandFillType = m_SpecDesignPage.m_FillMethod==(int)ftGridOrder ?ftGridOrder : ftMinimizeHarping;
}

BOOL CSpecMainSheet::OnInitDialog() 
{
	BOOL bResult = CPropertySheet::OnInitDialog();
	
   // disable OK button if editing not allowed
   CString head;
   GetWindowText(head);
   head += _T(" - ");
   head += m_Entry.GetName().c_str();
	if (!m_AllowEditing)
   {
      CWnd* pbut = GetDlgItem(IDOK);
      ASSERT(pbut);
      pbut->EnableWindow(m_AllowEditing);
      head += _T(" (Read Only)");
   }
   SetWindowText(head);
	
	return bResult;
}

lrfdVersionMgr::Version CSpecMainSheet::GetSpecVersion()
{
   return m_SpecDescrPage.GetSpecVersion();
}
