///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2018  Washington State Department of Transportation
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

// ProjectAgentImp.cpp : Implementation of CProjectAgentImp
#include "stdafx.h"
#include "ProjectAgent.h"
#include "ProjectAgentImp.h"
#include "StatusItems.h"
#include <algorithm>
#include <typeinfo>
#include <map>
#include <MathEx.h>
#include <System\Time.h>
#include <Units\SysUnits.h>
#include <StrData.cpp>

#include <GeomModel\GeomModel.h>

#include <psglib\psglib.h>
#include <psgLib\StructuredLoad.h>
#include <psgLib\StructuredSave.h>
#include <psgLib\BeamFamilyManager.h>

#include <Lrfd\StrandPool.h>

#include <IFace\PrestressForce.h>
#include <IFace\StatusCenter.h>
#include <IFace\UpdateTemplates.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Intervals.h>
#include <IFace\Bridge.h>
#include <IFace\Transactions.h>
#include <IFace\StatusCenter.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\DocumentType.h>
#include <IFace\BeamFactory.h>

#include <EAF\EAFDocument.h>

// tranactions executed by this agent
#include "txnEditBridgeDescription.h"

#include <EAF\EAFAutoProgress.h>
#include <PgsExt\StatusItem.h>
#include <PgsExt\GirderLabel.h>
#include <PgsExt\StatusItem.h>

#include <PgsExt\GirderData.h>

#include <checks.h>
#include <comdef.h>

#include "PGSuperCatCom.h"
#include "XSectionData.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// set a tolerance for IsEqual's in this class. Main reason for having the 
// IsEqual's is to prevent errant data changes from conversion round-off in dialogs.
static const Float64 DLG_TOLER = 1.0e-06;
static const Float64 STN_TOLER = 1.0e-07;

template <class EntryType, class LibType>
void use_library_entry(pgsLibraryEntryObserver* pObserver, const std::_tstring& key, EntryType** ppEntry, LibType& prjLib)
{
   use_library_entry<EntryType, LibType>(pObserver, key.c_str(), ppEntry, prjLib);
}

template <class EntryType, class LibType>
void use_library_entry( pgsLibraryEntryObserver* pObserver, LPCTSTR lpcstrKey, EntryType** ppEntry, LibType& prjLib)
{
   // First check and see if it is already in the project library
   const EntryType* pProjectEntry = prjLib.LookupEntry( lpcstrKey );

   *ppEntry = pProjectEntry; // Reference count has been added.
   if ( *ppEntry )
   {
      (*ppEntry)->Attach( pObserver );
   }
}

template <class EntryType,class LibType>
void release_library_entry( pgsLibraryEntryObserver* pObserver, EntryType* pEntry, LibType& prjLib)
{
   if ( pEntry == nullptr )
   {
      return;
   }

   // Release from the project library
   pEntry->Detach( pObserver );

   // Release it because we are no longer using it for input data
   pEntry->Release();
   pEntry = 0;
}

// pre-declare some needed functions
static bool IsValidStraightStrandFill(const CDirectStrandFillCollection* pFill, const GirderLibraryEntry* pGirderEntry);
static bool IsValidHarpedStrandFill(const CDirectStrandFillCollection* pFill, const GirderLibraryEntry* pGirderEntry);
static bool IsValidTemporaryStrandFill(const CDirectStrandFillCollection* pFill, const GirderLibraryEntry* pGirderEntry);

#define COMPUTE_LLDF -1


/////////////////////////////////////////////////////////////////////////////
// CProjectAgentImp
CProjectAgentImp::CProjectAgentImp()
{
   m_pBroker = 0;
   m_pLibMgr = 0;

   m_BridgeName = _T("");
   m_BridgeID   = _T("");
   m_JobNumber  = _T("");
   m_Engineer   = _T("");
   m_Company    = _T("");
   m_Comments   = _T("");

   m_AnalysisType = pgsTypes::Envelope;
   m_bGetAnalysisTypeFromLibrary = false;

   // Initialize Environment Data
   m_ExposureCondition = expNormal;
   m_RelHumidity = 75.;

   // Initialize Alignment Data
   m_AlignmentOffset_Temp = 0;
   m_bUseTempAlignmentOffset = true;

   m_AlignmentData2.Direction = 0;
   m_AlignmentData2.xRefPoint = 0;
   m_AlignmentData2.yRefPoint = 0;
   m_AlignmentData2.RefStation = 0;

   m_ProfileData2.Station = 0;
   m_ProfileData2.Elevation = 0;
   m_ProfileData2.Grade = 0;

   CrownData2 cd;
   cd.Station = 0.0;
   cd.Left = -0.02;
   cd.Right = -0.02;
   cd.CrownPointOffset = 0;
   m_RoadwaySectionData.Superelevations.push_back(cd);

   m_DuctilityLevel   = ILoadModifiers::Normal;
   m_ImportanceLevel  = ILoadModifiers::Normal;
   m_RedundancyLevel  = ILoadModifiers::Normal;
   m_DuctilityFactor  = 1.0;
   m_ImportanceFactor = 1.0;
   m_RedundancyFactor = 1.0;

   m_pSpecEntry = 0;
   m_pRatingEntry = 0;

   m_LibObserver.SetAgent( this );

   m_PendingEvents = 0;
   m_EventHoldCount = 0;
   m_bFiringEvents = false;

   // Default live loads is HL-93 for design, nothing for permit
   LiveLoadSelection selection;
   selection.EntryName = _T("HL-93");
   m_SelectedLiveLoads[pgsTypes::lltDesign].insert(selection);

   selection.EntryName = _T("Fatigue");
   m_SelectedLiveLoads[pgsTypes::lltFatigue].insert(selection);

   selection.EntryName = _T("AASHTO Legal Loads");
   m_SelectedLiveLoads[pgsTypes::lltLegalRating_Routine].insert(selection);

   selection.EntryName = _T("Notional Rating Load (NRL)");
   m_SelectedLiveLoads[pgsTypes::lltLegalRating_Special].insert(selection);

   selection.EntryName = _T("Emergency Vehicles");
   m_SelectedLiveLoads[pgsTypes::lltLegalRating_Emergency].insert(selection);

   // Default impact factors
   m_TruckImpact[pgsTypes::lltDesign]  = 0.33;
   m_TruckImpact[pgsTypes::lltPermit]  = 0.00;
   m_TruckImpact[pgsTypes::lltFatigue] = 0.15;
   m_TruckImpact[pgsTypes::lltPedestrian] = -1000.0; // obvious bogus value
   m_TruckImpact[pgsTypes::lltLegalRating_Routine]  = 0.10; // assume new, smooth riding surface
   m_TruckImpact[pgsTypes::lltLegalRating_Special] = 0.10;
   m_TruckImpact[pgsTypes::lltLegalRating_Emergency] = 0.10;
   m_TruckImpact[pgsTypes::lltPermitRating_Routine] = 0.10;
   m_TruckImpact[pgsTypes::lltPermitRating_Special] = 0.10;

   m_LaneImpact[pgsTypes::lltDesign]  = 0.00;
   m_LaneImpact[pgsTypes::lltPermit]  = 0.00;
   m_LaneImpact[pgsTypes::lltFatigue] = 0.00;
   m_LaneImpact[pgsTypes::lltPedestrian] = -1000.0; // obvious bogus value
   m_LaneImpact[pgsTypes::lltLegalRating_Routine]  = 0.00;
   m_LaneImpact[pgsTypes::lltLegalRating_Special] = 0.00;
   m_LaneImpact[pgsTypes::lltLegalRating_Emergency] = 0.00;
   m_LaneImpact[pgsTypes::lltPermitRating_Routine]  = 0.00;
   m_LaneImpact[pgsTypes::lltPermitRating_Special]  = 0.00;

   // Default for versions of PGSuper before this was an option was to sum pedestrian with vehicular
   m_PedestrianLoadApplicationType[pgsTypes::lltDesign] = ILiveLoads::PedConcurrentWithVehicular;
   m_PedestrianLoadApplicationType[pgsTypes::lltPermit] = ILiveLoads::PedConcurrentWithVehicular;
   m_PedestrianLoadApplicationType[pgsTypes::lltFatigue] = ILiveLoads::PedDontApply;

   m_bExcludeLegalLoadLaneLoading = false;
   m_bIncludePedestrianLiveLoad = false;
   m_SpecialPermitType = pgsTypes::ptSingleTripWithEscort;

   m_LldfRangeOfApplicabilityAction = roaEnforce;
   m_bGetIgnoreROAFromLibrary = true;

   m_bUpdateJackingForce = true;

   // rating parameters
   m_ADTT = -1; // unknown
   m_SystemFactorFlexure = 1.0;
   m_SystemFactorShear   = 1.0;

   m_AllowableYieldStressCoefficient = 0.9;

   m_gDC.fill(0);
   m_gDW.fill(0);
   m_gCR.fill(0);
   m_gRE.fill(0);
   m_gSH.fill(0);
   m_gPS.fill(0);
   m_gLL.fill(0);

   m_gDC[pgsTypes::StrengthI_Inventory] = 1.25;
   m_gDW[pgsTypes::StrengthI_Inventory] = 1.50;
   m_gCR[pgsTypes::StrengthI_Inventory] = 1.00;
   m_gSH[pgsTypes::StrengthI_Inventory] = 1.00;
   m_gRE[pgsTypes::StrengthI_Inventory] = 1.00;
   m_gPS[pgsTypes::StrengthI_Inventory] = 1.00;
   m_gLL[pgsTypes::StrengthI_Inventory] = COMPUTE_LLDF;

   m_gDC[pgsTypes::StrengthI_Operating] = 1.25;
   m_gDW[pgsTypes::StrengthI_Operating] = 1.50;
   m_gCR[pgsTypes::StrengthI_Operating] = 1.00;
   m_gSH[pgsTypes::StrengthI_Operating] = 1.00;
   m_gRE[pgsTypes::StrengthI_Operating] = 1.00;
   m_gPS[pgsTypes::StrengthI_Operating] = 1.00;
   m_gLL[pgsTypes::StrengthI_Operating] = COMPUTE_LLDF;

   m_gDC[pgsTypes::ServiceIII_Inventory] = 1.00;
   m_gDW[pgsTypes::ServiceIII_Inventory] = 1.00;
   m_gCR[pgsTypes::ServiceIII_Inventory] = 1.00;
   m_gSH[pgsTypes::ServiceIII_Inventory] = 1.00;
   m_gRE[pgsTypes::ServiceIII_Inventory] = 1.00;
   m_gPS[pgsTypes::ServiceIII_Inventory] = 1.00;
   m_gLL[pgsTypes::ServiceIII_Inventory] = COMPUTE_LLDF;

   m_gDC[pgsTypes::ServiceIII_Operating] = 1.00;
   m_gDW[pgsTypes::ServiceIII_Operating] = 1.00;
   m_gCR[pgsTypes::ServiceIII_Operating] = 1.00;
   m_gSH[pgsTypes::ServiceIII_Operating] = 1.00;
   m_gRE[pgsTypes::ServiceIII_Operating] = 1.00;
   m_gPS[pgsTypes::ServiceIII_Operating] = 1.00;
   m_gLL[pgsTypes::ServiceIII_Operating] = COMPUTE_LLDF;

   m_gDC[pgsTypes::StrengthI_LegalRoutine] = 1.25;
   m_gDW[pgsTypes::StrengthI_LegalRoutine] = 1.50;
   m_gCR[pgsTypes::StrengthI_LegalRoutine] = 1.00;
   m_gSH[pgsTypes::StrengthI_LegalRoutine] = 1.00;
   m_gRE[pgsTypes::StrengthI_LegalRoutine] = 1.00;
   m_gPS[pgsTypes::StrengthI_LegalRoutine] = 1.00;
   m_gLL[pgsTypes::StrengthI_LegalRoutine] = COMPUTE_LLDF;

   m_gDC[pgsTypes::ServiceIII_LegalRoutine] = 1.00;
   m_gDW[pgsTypes::ServiceIII_LegalRoutine] = 1.00;
   m_gCR[pgsTypes::ServiceIII_LegalRoutine] = 1.00;
   m_gSH[pgsTypes::ServiceIII_LegalRoutine] = 1.00;
   m_gRE[pgsTypes::ServiceIII_LegalRoutine] = 1.00;
   m_gPS[pgsTypes::ServiceIII_LegalRoutine] = 1.00;
   m_gLL[pgsTypes::ServiceIII_LegalRoutine] = 1.00;

   m_gDC[pgsTypes::StrengthI_LegalSpecial] = 1.25;
   m_gDW[pgsTypes::StrengthI_LegalSpecial] = 1.50;
   m_gCR[pgsTypes::StrengthI_LegalSpecial] = 1.00;
   m_gSH[pgsTypes::StrengthI_LegalSpecial] = 1.00;
   m_gRE[pgsTypes::StrengthI_LegalSpecial] = 1.00;
   m_gPS[pgsTypes::StrengthI_LegalSpecial] = 1.00;
   m_gLL[pgsTypes::StrengthI_LegalSpecial] = COMPUTE_LLDF;

   m_gDC[pgsTypes::ServiceIII_LegalSpecial] = 1.00;
   m_gDW[pgsTypes::ServiceIII_LegalSpecial] = 1.00;
   m_gCR[pgsTypes::ServiceIII_LegalSpecial] = 1.00;
   m_gSH[pgsTypes::ServiceIII_LegalSpecial] = 1.00;
   m_gRE[pgsTypes::ServiceIII_LegalSpecial] = 1.00;
   m_gPS[pgsTypes::ServiceIII_LegalSpecial] = 1.00;
   m_gLL[pgsTypes::ServiceIII_LegalSpecial] = COMPUTE_LLDF;

   m_gDC[pgsTypes::StrengthI_LegalEmergency] = 1.25;
   m_gDW[pgsTypes::StrengthI_LegalEmergency] = 1.50;
   m_gCR[pgsTypes::StrengthI_LegalEmergency] = 1.00;
   m_gSH[pgsTypes::StrengthI_LegalEmergency] = 1.00;
   m_gRE[pgsTypes::StrengthI_LegalEmergency] = 1.00;
   m_gPS[pgsTypes::StrengthI_LegalEmergency] = 1.00;
   m_gLL[pgsTypes::StrengthI_LegalEmergency] = COMPUTE_LLDF;

   m_gDC[pgsTypes::ServiceIII_LegalEmergency] = 1.00;
   m_gDW[pgsTypes::ServiceIII_LegalEmergency] = 1.00;
   m_gCR[pgsTypes::ServiceIII_LegalEmergency] = 1.00;
   m_gSH[pgsTypes::ServiceIII_LegalEmergency] = 1.00;
   m_gRE[pgsTypes::ServiceIII_LegalEmergency] = 1.00;
   m_gPS[pgsTypes::ServiceIII_LegalEmergency] = 1.00;
   m_gLL[pgsTypes::ServiceIII_LegalEmergency] = COMPUTE_LLDF;

   m_gDC[pgsTypes::StrengthII_PermitRoutine] = 1.25;
   m_gDW[pgsTypes::StrengthII_PermitRoutine] = 1.50;
   m_gCR[pgsTypes::StrengthII_PermitRoutine] = 1.00;
   m_gSH[pgsTypes::StrengthII_PermitRoutine] = 1.00;
   m_gRE[pgsTypes::StrengthII_PermitRoutine] = 1.00;
   m_gPS[pgsTypes::StrengthII_PermitRoutine] = 1.00;
   m_gLL[pgsTypes::StrengthII_PermitRoutine] = COMPUTE_LLDF;

   m_gDC[pgsTypes::ServiceI_PermitRoutine] = 1.00;
   m_gDW[pgsTypes::ServiceI_PermitRoutine] = 1.00;
   m_gCR[pgsTypes::ServiceI_PermitRoutine] = 1.00;
   m_gSH[pgsTypes::ServiceI_PermitRoutine] = 1.00;
   m_gRE[pgsTypes::ServiceI_PermitRoutine] = 1.00;
   m_gPS[pgsTypes::ServiceI_PermitRoutine] = 1.00;
   m_gLL[pgsTypes::ServiceI_PermitRoutine] = COMPUTE_LLDF;

   m_gDC[pgsTypes::ServiceIII_PermitRoutine] = 1.00;
   m_gDW[pgsTypes::ServiceIII_PermitRoutine] = 1.00;
   m_gCR[pgsTypes::ServiceIII_PermitRoutine] = 1.00;
   m_gSH[pgsTypes::ServiceIII_PermitRoutine] = 1.00;
   m_gRE[pgsTypes::ServiceIII_PermitRoutine] = 1.00;
   m_gPS[pgsTypes::ServiceIII_PermitRoutine] = 1.00;
   m_gLL[pgsTypes::ServiceIII_PermitRoutine] = COMPUTE_LLDF;

   m_gDC[pgsTypes::StrengthII_PermitSpecial] = 1.25;
   m_gDW[pgsTypes::StrengthII_PermitSpecial] = 1.50;
   m_gCR[pgsTypes::StrengthII_PermitSpecial] = 1.00;
   m_gSH[pgsTypes::StrengthII_PermitSpecial] = 1.00;
   m_gRE[pgsTypes::StrengthII_PermitSpecial] = 1.00;
   m_gPS[pgsTypes::StrengthII_PermitSpecial] = 1.00;
   m_gLL[pgsTypes::StrengthII_PermitSpecial] = COMPUTE_LLDF;

   m_gDC[pgsTypes::ServiceI_PermitSpecial] = 1.00;
   m_gDW[pgsTypes::ServiceI_PermitSpecial] = 1.00;
   m_gCR[pgsTypes::ServiceI_PermitSpecial] = 1.00;
   m_gSH[pgsTypes::ServiceI_PermitSpecial] = 1.00;
   m_gRE[pgsTypes::ServiceI_PermitSpecial] = 1.00;
   m_gPS[pgsTypes::ServiceI_PermitSpecial] = 1.00;
   m_gLL[pgsTypes::ServiceI_PermitSpecial] = COMPUTE_LLDF;

   m_gDC[pgsTypes::ServiceIII_PermitSpecial] = 1.00;
   m_gDW[pgsTypes::ServiceIII_PermitSpecial] = 1.00;
   m_gCR[pgsTypes::ServiceIII_PermitSpecial] = 1.00;
   m_gSH[pgsTypes::ServiceIII_PermitSpecial] = 1.00;
   m_gRE[pgsTypes::ServiceIII_PermitSpecial] = 1.00;
   m_gPS[pgsTypes::ServiceIII_PermitSpecial] = 1.00;
   m_gLL[pgsTypes::ServiceIII_PermitSpecial] = COMPUTE_LLDF;

   m_AllowableTensionCoefficient[pgsTypes::lrDesign_Inventory] = ::ConvertToSysUnits(0.19,unitMeasure::SqrtKSI);
   m_AllowableTensionCoefficient[pgsTypes::lrDesign_Operating] = ::ConvertToSysUnits(0.19,unitMeasure::SqrtKSI);
   m_AllowableTensionCoefficient[pgsTypes::lrLegal_Routine]    = ::ConvertToSysUnits(0.19,unitMeasure::SqrtKSI);
   m_AllowableTensionCoefficient[pgsTypes::lrLegal_Special] = ::ConvertToSysUnits(0.19, unitMeasure::SqrtKSI);
   m_AllowableTensionCoefficient[pgsTypes::lrLegal_Emergency] = ::ConvertToSysUnits(0.19, unitMeasure::SqrtKSI);
   m_AllowableTensionCoefficient[pgsTypes::lrPermit_Routine]   = ::ConvertToSysUnits(0.19,unitMeasure::SqrtKSI);
   m_AllowableTensionCoefficient[pgsTypes::lrPermit_Special]   = ::ConvertToSysUnits(0.19,unitMeasure::SqrtKSI);

   m_bCheckYieldStress[pgsTypes::lrDesign_Inventory] = false;
   m_bCheckYieldStress[pgsTypes::lrDesign_Operating] = false;
   m_bCheckYieldStress[pgsTypes::lrLegal_Routine]    = false;
   m_bCheckYieldStress[pgsTypes::lrLegal_Special] = false;
   m_bCheckYieldStress[pgsTypes::lrLegal_Emergency] = false;
   m_bCheckYieldStress[pgsTypes::lrPermit_Routine]   = true;
   m_bCheckYieldStress[pgsTypes::lrPermit_Special]   = true;

   m_bRateForStress[pgsTypes::lrDesign_Inventory] = true;
   m_bRateForStress[pgsTypes::lrDesign_Operating] = false; // see MBE C6A.5.4.1
   m_bRateForStress[pgsTypes::lrLegal_Routine]    = true;
   m_bRateForStress[pgsTypes::lrLegal_Special] = true;
   m_bRateForStress[pgsTypes::lrLegal_Emergency] = true;
   m_bRateForStress[pgsTypes::lrPermit_Routine]   = false;
   m_bRateForStress[pgsTypes::lrPermit_Special]   = false;

   m_bRateForShear[pgsTypes::lrDesign_Inventory] = true;
   m_bRateForShear[pgsTypes::lrDesign_Operating] = true;
   m_bRateForShear[pgsTypes::lrLegal_Routine]    = true;
   m_bRateForShear[pgsTypes::lrLegal_Special] = true;
   m_bRateForShear[pgsTypes::lrLegal_Emergency] = true;
   m_bRateForShear[pgsTypes::lrPermit_Routine]   = true;
   m_bRateForShear[pgsTypes::lrPermit_Special]   = true;

   m_bEnableRating[pgsTypes::lrDesign_Inventory] = true;
   m_bEnableRating[pgsTypes::lrDesign_Operating] = true;
   m_bEnableRating[pgsTypes::lrLegal_Routine]    = true;
   m_bEnableRating[pgsTypes::lrLegal_Special] = true;
   m_bEnableRating[pgsTypes::lrLegal_Emergency] = true;
   m_bEnableRating[pgsTypes::lrPermit_Routine]   = false;
   m_bEnableRating[pgsTypes::lrPermit_Special]   = false;

   m_ReservedLiveLoads.reserve(6);
   m_ReservedLiveLoads.push_back(_T("HL-93"));
   m_ReservedLiveLoads.push_back(_T("Fatigue"));
   m_ReservedLiveLoads.push_back(_T("AASHTO Legal Loads"));
   m_ReservedLiveLoads.push_back(_T("Notional Rating Load (NRL)"));
   m_ReservedLiveLoads.push_back(_T("Single-Unit SHVs"));
   m_ReservedLiveLoads.push_back(_T("Emergency Vehicles"));

   m_ConstructionLoad = 0;

   m_bIgnoreEffectiveFlangeWidthLimits = false;

   m_bIgnoreCreepEffects = false;
   m_bIgnoreShrinkageEffects = false;
   m_bIgnoreRelaxationEffects = false;

   m_bGeneralLumpSum               = false;
   m_BeforeXferLosses              = 0;
   m_AfterXferLosses               = 0;
   m_LiftingLosses                 = 0;
   m_ShippingLosses                = ::ConvertToSysUnits(20,unitMeasure::KSI);
   m_BeforeTempStrandRemovalLosses = m_ShippingLosses;
   m_AfterTempStrandRemovalLosses  = m_ShippingLosses;
   m_AfterDeckPlacementLosses      = m_ShippingLosses;
   m_AfterSIDLLosses               = m_ShippingLosses;
   m_FinalLosses                   = m_ShippingLosses;

   m_Dset_PT                = ::ConvertToSysUnits(0.375,unitMeasure::Inch);
   m_WobbleFriction_PT      = ::ConvertToSysUnits(0.0002,unitMeasure::PerFeet);
   m_FrictionCoefficient_PT = 0.25;

   m_Dset_TTS                = ::ConvertToSysUnits(0.375,unitMeasure::Inch);
   m_WobbleFriction_TTS      = ::ConvertToSysUnits(0.0002,unitMeasure::PerFeet);
   m_FrictionCoefficient_TTS = 0.25;


   m_BridgeStabilityStatusItemID = INVALID_ID;

   m_objAngle.CoCreateInstance(CLSID_Angle);
   m_objDirection.CoCreateInstance(CLSID_Direction);

   m_LoadManager.SetTimelineManager(m_BridgeDescription.GetTimelineManager());

   m_bUpdateUserDefinedLoads = false;
}

CProjectAgentImp::~CProjectAgentImp()
{
}

HRESULT CProjectAgentImp::FinalConstruct()
{
   return S_OK;
}

void CProjectAgentImp::FinalRelease()
{
}

HRESULT CProjectAgentImp::SpecificationProc(IStructuredSave* pSave,IStructuredLoad* pLoad,IProgress* pProgress,CProjectAgentImp* pObj)
{
   USES_CONVERSION;

   HRESULT hr = S_OK;
   if ( pSave )
   {
      pSave->BeginUnit(_T("Specification"),2.0);
      hr = pSave->put_Property(_T("Spec"),CComVariant(pObj->m_Spec.c_str()));
      if ( FAILED(hr) )
      {
         return hr;
      }

      hr = pSave->put_Property(_T("AnalysisType"),CComVariant(pObj->m_AnalysisType));
      if ( FAILED(hr) )
      {
         return hr;
      }

      pSave->EndUnit();
   }
   else
   {
      pLoad->BeginUnit(_T("Specification"));

      Float64 version;

      pLoad->get_Version(&version);

      CComVariant var;
      var.vt = VT_BSTR;
      hr = pLoad->get_Property(_T("Spec"),&var);
      if ( FAILED(hr) )
      {
         return hr;
      }

      pObj->m_Spec = OLE2T(var.bstrVal);

      if ( 1 < version )
      {
         var.vt = VT_I4;
         hr = pLoad->get_Property(_T("AnalysisType"),&var);
         if ( FAILED(hr) )
         {
            return hr;
         }

         pObj->m_AnalysisType = (pgsTypes::AnalysisType)var.lVal;
         pObj->m_bGetAnalysisTypeFromLibrary = false;
      }
      else
      {
         pObj->m_bGetAnalysisTypeFromLibrary = true;
      }

      pLoad->EndUnit();
   }

   return S_OK;
}

HRESULT CProjectAgentImp::EffectiveFlangeWidthProc(IStructuredSave* pSave,IStructuredLoad* pLoad,IProgress* pProgress,CProjectAgentImp* pObj)
{
   HRESULT hr = S_OK;
   if ( pSave )
   {
      hr = pSave->BeginUnit(_T("EffectiveFlangeWidth"),1.0);
      if ( FAILED(hr) )
      {
         return hr;
      }

      hr = pSave->put_Property(_T("IgnoreLimits"),CComVariant(pObj->m_bIgnoreEffectiveFlangeWidthLimits ? VARIANT_TRUE : VARIANT_FALSE));
      if ( FAILED(hr) )
      {
         return hr;
      }

      hr = pSave->EndUnit();
      if ( FAILED(hr) )
      {
         return hr;
      }
   }
   else
   {
      Float64 parent_version;
      pLoad->get_Version(&parent_version);
      if ( 3.0 <= parent_version )
      {
         hr = pLoad->BeginUnit(_T("EffectiveFlangeWidth"));
         if ( FAILED(hr) )
         {
            return hr;
         }

         CComVariant var;
         var.vt = VT_BOOL;
         hr = pLoad->get_Property(_T("IgnoreLimits"),&var);
         if ( FAILED(hr) )
         {
            return hr;
         }

         pObj->m_bIgnoreEffectiveFlangeWidthLimits = (var.boolVal == VARIANT_TRUE ? true : false);

         hr = pLoad->EndUnit();
         if ( FAILED(hr) )
         {
            return hr;
         }

      }
   }

   return S_OK;
}

HRESULT CProjectAgentImp::RatingSpecificationProc(IStructuredSave* pSave,IStructuredLoad* pLoad,IProgress* pProgress,CProjectAgentImp* pObj)
{
   USES_CONVERSION;

   HRESULT hr = S_OK;
   if ( pSave )
   {
      pSave->BeginUnit(_T("RatingSpecification"),2.0);
      hr = pSave->put_Property(_T("Name"),CComVariant(pObj->m_RatingSpec.c_str()));
      if ( FAILED(hr) )
      {
         return hr;
      }

      pSave->put_Property(_T("SystemFactorFlexure"), CComVariant(pObj->m_SystemFactorFlexure));
      pSave->put_Property(_T("SystemFactorShear"),   CComVariant(pObj->m_SystemFactorShear));
      pSave->put_Property(_T("ADTT"),CComVariant(pObj->m_ADTT));
      pSave->put_Property(_T("IncludePedestrianLiveLoad"),CComVariant(pObj->m_bIncludePedestrianLiveLoad));

      pSave->BeginUnit(_T("DesignInventoryRating"),4.0);
      pSave->put_Property(_T("Enabled"),CComVariant(pObj->m_bEnableRating[pgsTypes::lrDesign_Inventory]));
      pSave->put_Property(_T("DC_StrengthI"),CComVariant(pObj->m_gDC[pgsTypes::StrengthI_Inventory]));
      pSave->put_Property(_T("DW_StrengthI"),CComVariant(pObj->m_gDW[pgsTypes::StrengthI_Inventory]));
      pSave->put_Property(_T("CR_StrengthI"),CComVariant(pObj->m_gCR[pgsTypes::StrengthI_Inventory]));
      pSave->put_Property(_T("SH_StrengthI"),CComVariant(pObj->m_gSH[pgsTypes::StrengthI_Inventory]));
      pSave->put_Property(_T("RE_StrengthI"),CComVariant(pObj->m_gRE[pgsTypes::StrengthI_Inventory]));
      pSave->put_Property(_T("PS_StrengthI"),CComVariant(pObj->m_gPS[pgsTypes::StrengthI_Inventory]));
      pSave->put_Property(_T("LL_StrengthI"),CComVariant(pObj->m_gLL[pgsTypes::StrengthI_Inventory]));
      pSave->put_Property(_T("DC_ServiceIII"),CComVariant(pObj->m_gDC[pgsTypes::ServiceIII_Inventory]));
      pSave->put_Property(_T("DW_ServiceIII"),CComVariant(pObj->m_gDW[pgsTypes::ServiceIII_Inventory]));
      pSave->put_Property(_T("CR_ServiceIII"),CComVariant(pObj->m_gCR[pgsTypes::ServiceIII_Inventory]));
      pSave->put_Property(_T("SH_ServiceIII"),CComVariant(pObj->m_gSH[pgsTypes::ServiceIII_Inventory]));
      pSave->put_Property(_T("RE_ServiceIII"),CComVariant(pObj->m_gRE[pgsTypes::ServiceIII_Inventory]));
      pSave->put_Property(_T("PS_ServiceIII"),CComVariant(pObj->m_gPS[pgsTypes::ServiceIII_Inventory]));
      pSave->put_Property(_T("LL_ServiceIII"),CComVariant(pObj->m_gLL[pgsTypes::ServiceIII_Inventory]));
      pSave->put_Property(_T("AllowableTensionCoefficient"),CComVariant(pObj->m_AllowableTensionCoefficient[pgsTypes::lrDesign_Inventory]));
      pSave->put_Property(_T("RateForShear"),CComVariant(pObj->m_bRateForShear[pgsTypes::lrDesign_Inventory]));
      pSave->EndUnit(); // DesignInventoryRating

      pSave->BeginUnit(_T("DesignOperatingRating"),4.0);
      pSave->put_Property(_T("Enabled"),CComVariant(pObj->m_bEnableRating[pgsTypes::lrDesign_Operating]));
      pSave->put_Property(_T("DC_StrengthI"),CComVariant(pObj->m_gDC[pgsTypes::StrengthI_Operating]));
      pSave->put_Property(_T("DW_StrengthI"),CComVariant(pObj->m_gDW[pgsTypes::StrengthI_Operating]));
      pSave->put_Property(_T("CR_StrengthI"),CComVariant(pObj->m_gCR[pgsTypes::StrengthI_Operating]));
      pSave->put_Property(_T("SH_StrengthI"),CComVariant(pObj->m_gSH[pgsTypes::StrengthI_Operating]));
      pSave->put_Property(_T("RE_StrengthI"),CComVariant(pObj->m_gRE[pgsTypes::StrengthI_Operating]));
      pSave->put_Property(_T("PS_StrengthI"),CComVariant(pObj->m_gPS[pgsTypes::StrengthI_Operating]));
      pSave->put_Property(_T("LL_StrengthI"),CComVariant(pObj->m_gLL[pgsTypes::StrengthI_Operating]));
      pSave->put_Property(_T("DC_ServiceIII"),CComVariant(pObj->m_gDC[pgsTypes::ServiceIII_Operating]));
      pSave->put_Property(_T("DW_ServiceIII"),CComVariant(pObj->m_gDW[pgsTypes::ServiceIII_Operating]));
      pSave->put_Property(_T("CR_ServiceIII"),CComVariant(pObj->m_gCR[pgsTypes::ServiceIII_Operating]));
      pSave->put_Property(_T("SH_ServiceIII"),CComVariant(pObj->m_gSH[pgsTypes::ServiceIII_Operating]));
      pSave->put_Property(_T("RE_ServiceIII"),CComVariant(pObj->m_gRE[pgsTypes::ServiceIII_Operating]));
      pSave->put_Property(_T("PS_ServiceIII"),CComVariant(pObj->m_gPS[pgsTypes::ServiceIII_Operating]));
      pSave->put_Property(_T("LL_ServiceIII"),CComVariant(pObj->m_gLL[pgsTypes::ServiceIII_Operating]));
      pSave->put_Property(_T("AllowableTensionCoefficient"),CComVariant(pObj->m_AllowableTensionCoefficient[pgsTypes::lrDesign_Operating]));
      pSave->put_Property(_T("RateForShear"),CComVariant(pObj->m_bRateForShear[pgsTypes::lrDesign_Operating]));
      pSave->EndUnit(); // DesignOperatingRating

      pSave->BeginUnit(_T("LegalRoutineRating"),4.0);
      pSave->put_Property(_T("Enabled"),CComVariant(pObj->m_bEnableRating[pgsTypes::lrLegal_Routine]));
      pSave->put_Property(_T("DC_StrengthI"),CComVariant(pObj->m_gDC[pgsTypes::StrengthI_LegalRoutine]));
      pSave->put_Property(_T("DW_StrengthI"),CComVariant(pObj->m_gDW[pgsTypes::StrengthI_LegalRoutine]));
      pSave->put_Property(_T("CR_StrengthI"),CComVariant(pObj->m_gCR[pgsTypes::StrengthI_LegalRoutine]));
      pSave->put_Property(_T("SH_StrengthI"),CComVariant(pObj->m_gSH[pgsTypes::StrengthI_LegalRoutine]));
      pSave->put_Property(_T("RE_StrengthI"),CComVariant(pObj->m_gRE[pgsTypes::StrengthI_LegalRoutine]));
      pSave->put_Property(_T("PS_StrengthI"),CComVariant(pObj->m_gPS[pgsTypes::StrengthI_LegalRoutine]));
      pSave->put_Property(_T("LL_StrengthI"),CComVariant(pObj->m_gLL[pgsTypes::StrengthI_LegalRoutine]));
      pSave->put_Property(_T("DC_ServiceIII"),CComVariant(pObj->m_gDC[pgsTypes::ServiceIII_LegalRoutine]));
      pSave->put_Property(_T("DW_ServiceIII"),CComVariant(pObj->m_gDW[pgsTypes::ServiceIII_LegalRoutine]));
      pSave->put_Property(_T("CR_ServiceIII"),CComVariant(pObj->m_gCR[pgsTypes::ServiceIII_LegalRoutine]));
      pSave->put_Property(_T("SH_ServiceIII"),CComVariant(pObj->m_gSH[pgsTypes::ServiceIII_LegalRoutine]));
      pSave->put_Property(_T("RE_ServiceIII"),CComVariant(pObj->m_gRE[pgsTypes::ServiceIII_LegalRoutine]));
      pSave->put_Property(_T("PS_ServiceIII"),CComVariant(pObj->m_gPS[pgsTypes::ServiceIII_LegalRoutine]));
      pSave->put_Property(_T("LL_ServiceIII"),CComVariant(pObj->m_gLL[pgsTypes::ServiceIII_LegalRoutine]));
      pSave->put_Property(_T("AllowableTensionCoefficient"),CComVariant(pObj->m_AllowableTensionCoefficient[pgsTypes::lrLegal_Routine]));
      pSave->put_Property(_T("RateForShear"),CComVariant(pObj->m_bRateForShear[pgsTypes::lrLegal_Routine]));
      pSave->put_Property(_T("RateForStress"),CComVariant(pObj->m_bRateForStress[pgsTypes::lrLegal_Routine]));
      pSave->put_Property(_T("ExcludeLegalLoadLaneLoading"),CComVariant(pObj->m_bExcludeLegalLoadLaneLoading));
      pSave->EndUnit(); // LegalRoutineRating

      pSave->BeginUnit(_T("LegalSpecialRating"),4.0);
      pSave->put_Property(_T("Enabled"),CComVariant(pObj->m_bEnableRating[pgsTypes::lrLegal_Special]));
      pSave->put_Property(_T("DC_StrengthI"),CComVariant(pObj->m_gDC[pgsTypes::StrengthI_LegalSpecial]));
      pSave->put_Property(_T("DW_StrengthI"),CComVariant(pObj->m_gDW[pgsTypes::StrengthI_LegalSpecial]));
      pSave->put_Property(_T("CR_StrengthI"),CComVariant(pObj->m_gCR[pgsTypes::StrengthI_LegalSpecial]));
      pSave->put_Property(_T("SH_StrengthI"),CComVariant(pObj->m_gSH[pgsTypes::StrengthI_LegalSpecial]));
      pSave->put_Property(_T("RE_StrengthI"),CComVariant(pObj->m_gRE[pgsTypes::StrengthI_LegalSpecial]));
      pSave->put_Property(_T("PS_StrengthI"),CComVariant(pObj->m_gPS[pgsTypes::StrengthI_LegalSpecial]));
      pSave->put_Property(_T("LL_StrengthI"),CComVariant(pObj->m_gLL[pgsTypes::StrengthI_LegalSpecial]));
      pSave->put_Property(_T("DC_ServiceIII"),CComVariant(pObj->m_gDC[pgsTypes::ServiceIII_LegalSpecial]));
      pSave->put_Property(_T("DW_ServiceIII"),CComVariant(pObj->m_gDW[pgsTypes::ServiceIII_LegalSpecial]));
      pSave->put_Property(_T("CR_ServiceIII"),CComVariant(pObj->m_gCR[pgsTypes::ServiceIII_LegalSpecial]));
      pSave->put_Property(_T("SH_ServiceIII"),CComVariant(pObj->m_gSH[pgsTypes::ServiceIII_LegalSpecial]));
      pSave->put_Property(_T("RE_ServiceIII"),CComVariant(pObj->m_gRE[pgsTypes::ServiceIII_LegalSpecial]));
      pSave->put_Property(_T("PS_ServiceIII"),CComVariant(pObj->m_gPS[pgsTypes::ServiceIII_LegalSpecial]));
      pSave->put_Property(_T("LL_ServiceIII"),CComVariant(pObj->m_gLL[pgsTypes::ServiceIII_LegalSpecial]));
      pSave->put_Property(_T("AllowableTensionCoefficient"),CComVariant(pObj->m_AllowableTensionCoefficient[pgsTypes::lrLegal_Special]));
      pSave->put_Property(_T("RateForShear"),CComVariant(pObj->m_bRateForShear[pgsTypes::lrLegal_Special]));
      pSave->put_Property(_T("RateForStress"),CComVariant(pObj->m_bRateForStress[pgsTypes::lrLegal_Special]));
      pSave->EndUnit(); // LegalSpecialRating

      // added in version 2
      pSave->BeginUnit(_T("LegalEmergencyRating"), 4.0);
      pSave->put_Property(_T("Enabled"), CComVariant(pObj->m_bEnableRating[pgsTypes::lrLegal_Emergency]));
      pSave->put_Property(_T("DC_StrengthI"), CComVariant(pObj->m_gDC[pgsTypes::StrengthI_LegalEmergency]));
      pSave->put_Property(_T("DW_StrengthI"), CComVariant(pObj->m_gDW[pgsTypes::StrengthI_LegalEmergency]));
      pSave->put_Property(_T("CR_StrengthI"), CComVariant(pObj->m_gCR[pgsTypes::StrengthI_LegalEmergency]));
      pSave->put_Property(_T("SH_StrengthI"), CComVariant(pObj->m_gSH[pgsTypes::StrengthI_LegalEmergency]));
      pSave->put_Property(_T("RE_StrengthI"), CComVariant(pObj->m_gRE[pgsTypes::StrengthI_LegalEmergency]));
      pSave->put_Property(_T("PS_StrengthI"), CComVariant(pObj->m_gPS[pgsTypes::StrengthI_LegalEmergency]));
      pSave->put_Property(_T("LL_StrengthI"), CComVariant(pObj->m_gLL[pgsTypes::StrengthI_LegalEmergency]));
      pSave->put_Property(_T("DC_ServiceIII"), CComVariant(pObj->m_gDC[pgsTypes::ServiceIII_LegalEmergency]));
      pSave->put_Property(_T("DW_ServiceIII"), CComVariant(pObj->m_gDW[pgsTypes::ServiceIII_LegalEmergency]));
      pSave->put_Property(_T("CR_ServiceIII"), CComVariant(pObj->m_gCR[pgsTypes::ServiceIII_LegalEmergency]));
      pSave->put_Property(_T("SH_ServiceIII"), CComVariant(pObj->m_gSH[pgsTypes::ServiceIII_LegalEmergency]));
      pSave->put_Property(_T("RE_ServiceIII"), CComVariant(pObj->m_gRE[pgsTypes::ServiceIII_LegalEmergency]));
      pSave->put_Property(_T("PS_ServiceIII"), CComVariant(pObj->m_gPS[pgsTypes::ServiceIII_LegalEmergency]));
      pSave->put_Property(_T("LL_ServiceIII"), CComVariant(pObj->m_gLL[pgsTypes::ServiceIII_LegalEmergency]));
      pSave->put_Property(_T("AllowableTensionCoefficient"), CComVariant(pObj->m_AllowableTensionCoefficient[pgsTypes::lrLegal_Emergency]));
      pSave->put_Property(_T("RateForShear"), CComVariant(pObj->m_bRateForShear[pgsTypes::lrLegal_Emergency]));
      pSave->put_Property(_T("RateForStress"), CComVariant(pObj->m_bRateForStress[pgsTypes::lrLegal_Emergency]));
      pSave->EndUnit(); // LegalEmergencyRating

      pSave->BeginUnit(_T("PermitRoutineRating"),5.0);
      pSave->put_Property(_T("Enabled"),CComVariant(pObj->m_bEnableRating[pgsTypes::lrPermit_Routine]));
      pSave->put_Property(_T("DC_StrengthII"),CComVariant(pObj->m_gDC[pgsTypes::StrengthII_PermitRoutine]));
      pSave->put_Property(_T("DW_StrengthII"),CComVariant(pObj->m_gDW[pgsTypes::StrengthII_PermitRoutine]));
      pSave->put_Property(_T("CR_StrengthII"),CComVariant(pObj->m_gCR[pgsTypes::StrengthII_PermitRoutine]));
      pSave->put_Property(_T("SH_StrengthII"),CComVariant(pObj->m_gSH[pgsTypes::StrengthII_PermitRoutine]));
      pSave->put_Property(_T("RE_StrengthII"),CComVariant(pObj->m_gRE[pgsTypes::StrengthII_PermitRoutine]));
      pSave->put_Property(_T("PS_StrengthII"),CComVariant(pObj->m_gPS[pgsTypes::StrengthII_PermitRoutine]));
      pSave->put_Property(_T("LL_StrengthII"),CComVariant(pObj->m_gLL[pgsTypes::StrengthII_PermitRoutine]));
      pSave->put_Property(_T("DC_ServiceI"),CComVariant(pObj->m_gDC[pgsTypes::ServiceI_PermitRoutine]));
      pSave->put_Property(_T("DW_ServiceI"),CComVariant(pObj->m_gDW[pgsTypes::ServiceI_PermitRoutine]));
      pSave->put_Property(_T("CR_ServiceI"),CComVariant(pObj->m_gCR[pgsTypes::ServiceI_PermitRoutine]));
      pSave->put_Property(_T("SH_ServiceI"),CComVariant(pObj->m_gSH[pgsTypes::ServiceI_PermitRoutine]));
      pSave->put_Property(_T("RE_ServiceI"),CComVariant(pObj->m_gRE[pgsTypes::ServiceI_PermitRoutine]));
      pSave->put_Property(_T("PS_ServiceI"),CComVariant(pObj->m_gPS[pgsTypes::ServiceI_PermitRoutine]));
      pSave->put_Property(_T("LL_ServiceI"),CComVariant(pObj->m_gLL[pgsTypes::ServiceI_PermitRoutine]));

      // Added in version 5
      pSave->put_Property(_T("DC_ServiceIII"),CComVariant(pObj->m_gDC[pgsTypes::ServiceIII_PermitRoutine]));
      pSave->put_Property(_T("DW_ServiceIII"),CComVariant(pObj->m_gDW[pgsTypes::ServiceIII_PermitRoutine]));
      pSave->put_Property(_T("CR_ServiceIII"),CComVariant(pObj->m_gCR[pgsTypes::ServiceIII_PermitRoutine]));
      pSave->put_Property(_T("SH_ServiceIII"),CComVariant(pObj->m_gSH[pgsTypes::ServiceIII_PermitRoutine]));
      pSave->put_Property(_T("RE_ServiceIII"),CComVariant(pObj->m_gRE[pgsTypes::ServiceIII_PermitRoutine]));
      pSave->put_Property(_T("PS_ServiceIII"),CComVariant(pObj->m_gPS[pgsTypes::ServiceIII_PermitRoutine]));
      pSave->put_Property(_T("LL_ServiceIII"),CComVariant(pObj->m_gLL[pgsTypes::ServiceIII_PermitRoutine]));
      pSave->put_Property(_T("CheckYieldStress"),CComVariant(pObj->m_bCheckYieldStress[pgsTypes::lrPermit_Routine]));
      pSave->put_Property(_T("AllowableTensionCoefficient"),CComVariant(pObj->m_AllowableTensionCoefficient[pgsTypes::lrPermit_Routine]));
      // End of added in version 5

      pSave->put_Property(_T("AllowableYieldStressCoefficient"),CComVariant(pObj->m_AllowableYieldStressCoefficient));
      pSave->put_Property(_T("RateForShear"),CComVariant(pObj->m_bRateForShear[pgsTypes::lrPermit_Routine]));
      pSave->put_Property(_T("RateForStress"),CComVariant(pObj->m_bRateForStress[pgsTypes::lrPermit_Routine]));
      pSave->EndUnit(); // PermitRoutineRating

      pSave->BeginUnit(_T("PermitSpecialRating"),5.0);
      pSave->put_Property(_T("Enabled"),CComVariant(pObj->m_bEnableRating[pgsTypes::lrPermit_Special]));
      pSave->put_Property(_T("DC_StrengthII"),CComVariant(pObj->m_gDC[pgsTypes::StrengthII_PermitSpecial]));
      pSave->put_Property(_T("DW_StrengthII"),CComVariant(pObj->m_gDW[pgsTypes::StrengthII_PermitSpecial]));
      pSave->put_Property(_T("CR_StrengthII"),CComVariant(pObj->m_gCR[pgsTypes::StrengthII_PermitSpecial]));
      pSave->put_Property(_T("SH_StrengthII"),CComVariant(pObj->m_gSH[pgsTypes::StrengthII_PermitSpecial]));
      pSave->put_Property(_T("RE_StrengthII"),CComVariant(pObj->m_gRE[pgsTypes::StrengthII_PermitSpecial]));
      pSave->put_Property(_T("PS_StrengthII"),CComVariant(pObj->m_gPS[pgsTypes::StrengthII_PermitSpecial]));
      pSave->put_Property(_T("LL_StrengthII"),CComVariant(pObj->m_gLL[pgsTypes::StrengthII_PermitSpecial]));
      pSave->put_Property(_T("DC_ServiceI"),CComVariant(pObj->m_gDC[pgsTypes::ServiceI_PermitSpecial]));
      pSave->put_Property(_T("DW_ServiceI"),CComVariant(pObj->m_gDW[pgsTypes::ServiceI_PermitSpecial]));
      pSave->put_Property(_T("CR_ServiceI"),CComVariant(pObj->m_gCR[pgsTypes::ServiceI_PermitSpecial]));
      pSave->put_Property(_T("SH_ServiceI"),CComVariant(pObj->m_gSH[pgsTypes::ServiceI_PermitSpecial]));
      pSave->put_Property(_T("RE_ServiceI"),CComVariant(pObj->m_gRE[pgsTypes::ServiceI_PermitSpecial]));
      pSave->put_Property(_T("PS_ServiceI"),CComVariant(pObj->m_gPS[pgsTypes::ServiceI_PermitSpecial]));
      pSave->put_Property(_T("LL_ServiceI"),CComVariant(pObj->m_gLL[pgsTypes::ServiceI_PermitSpecial]));


      // Added in version 5
      pSave->put_Property(_T("DC_ServiceIII"),CComVariant(pObj->m_gDC[pgsTypes::ServiceIII_PermitSpecial]));
      pSave->put_Property(_T("DW_ServiceIII"),CComVariant(pObj->m_gDW[pgsTypes::ServiceIII_PermitSpecial]));
      pSave->put_Property(_T("CR_ServiceIII"),CComVariant(pObj->m_gCR[pgsTypes::ServiceIII_PermitSpecial]));
      pSave->put_Property(_T("SH_ServiceIII"),CComVariant(pObj->m_gSH[pgsTypes::ServiceIII_PermitSpecial]));
      pSave->put_Property(_T("RE_ServiceIII"),CComVariant(pObj->m_gRE[pgsTypes::ServiceIII_PermitSpecial]));
      pSave->put_Property(_T("PS_ServiceIII"),CComVariant(pObj->m_gPS[pgsTypes::ServiceIII_PermitSpecial]));
      pSave->put_Property(_T("LL_ServiceIII"),CComVariant(pObj->m_gLL[pgsTypes::ServiceIII_PermitSpecial]));
      pSave->put_Property(_T("CheckYieldStress"),CComVariant(pObj->m_bCheckYieldStress[pgsTypes::lrPermit_Special]));
      pSave->put_Property(_T("AllowableTensionCoefficient"),CComVariant(pObj->m_AllowableTensionCoefficient[pgsTypes::lrPermit_Special]));
      // End of added in version 5

      pSave->put_Property(_T("SpecialPermitType"),CComVariant(pObj->m_SpecialPermitType));
      pSave->put_Property(_T("RateForShear"),CComVariant(pObj->m_bRateForShear[pgsTypes::lrPermit_Special]));
      pSave->put_Property(_T("RateForStress"),CComVariant(pObj->m_bRateForStress[pgsTypes::lrPermit_Special]));
      pSave->EndUnit(); // PermitSpecialRating

      pSave->EndUnit();
   }
   else
   {
      // it is ok if this fails... older versions files don't have this data block
      if (!FAILED(pLoad->BeginUnit(_T("RatingSpecification"))) )
      {
         Float64 top_version;

         pLoad->get_Version(&top_version);

         CComVariant var;
         var.vt = VT_BSTR;
         hr = pLoad->get_Property(_T("Name"),&var);
         if ( FAILED(hr) )
         {
            return hr;
         }

         pObj->m_RatingSpec = OLE2T(var.bstrVal);

         var.vt = VT_R8;
         hr = pLoad->get_Property(_T("SystemFactorFlexure"),&var);
         if ( FAILED(hr) )
         {
            return hr;
         }

         pObj->m_SystemFactorFlexure = var.dblVal;

         var.vt = VT_R8;
         hr = pLoad->get_Property(_T("SystemFactorShear"),&var);
         if ( FAILED(hr) )
         {
            return hr;
         }

         pObj->m_SystemFactorShear = var.dblVal;

         var.vt = VT_I4;
         hr = pLoad->get_Property(_T("ADTT"),&var);
         if ( FAILED(hr) )
         {
            return hr;
         }

         pObj->m_ADTT = var.iVal;

         var.vt = VT_BOOL;
         hr = pLoad->get_Property(_T("IncludePedestrianLiveLoad"),&var);
         if ( FAILED(hr) )
         {
            return hr;
         }

         pObj->m_bIncludePedestrianLiveLoad = (var.boolVal == VARIANT_TRUE ? true : false);

         //////////////////////////////////////////////////////
         {
         pLoad->BeginUnit(_T("DesignInventoryRating"));

         Float64 version;
         pLoad->get_Version(&version);

         /////////////////////////////
         // NOTE
         // Vesrion 1 of all of these data blocks stored the data incorrectly. For example, the DC load factor for
         // StrengthI_Inventory rating was stored in m_gDC[IndexFromLimitState(pgsTypes::StrengthI_Inventory)]. However,
         // when the data was written to the storage stream, the data from m_gDC[pgsTypes::StrengthI_Inventory] was used. 
         // The data written to the storage stream should have been m_gDC[0], but instead it was m_gDC[6].
         //
         // All data written for the limit state of pgsTypes::ServiceIII_LegalRoutine and higher (greater enum value)
         // went past the end of the array and is invalid
         //
         // So that we don't have this problem again, the size of m_gDC, m_gDW, and m_gLL changed from 12 to the size of the
         // pgsTypes::LimitState enum (pgsTypes::LimitStateCount). When data is restored from file, it is put in the array
         // at m_gDC[limitState].
         //
         // To restore the incorrectly stored data exactly as it was when it was stored, an indexOffset is added to limitState enum value.
         // This will put the data back the way it was when it was stored. The length of the arrays
         // were 12 at the time. Anything with an index of less that 6 is lost. Anything stored using pgsTypes::ServiceIII_LegalRoutine
         // and higher, is invalid and will be ignored when restored from file.
         IndexType indexOffset = 0;
         if ( version < 2.0 )
         {
            indexOffset = (IndexType)pgsTypes::StrengthI_Inventory;
         }

         var.vt = VT_BOOL;
         pLoad->get_Property(_T("Enabled"),&var);
         pObj->m_bEnableRating[pgsTypes::lrDesign_Inventory] = (var.boolVal == VARIANT_TRUE ? true : false);

         var.vt = VT_R8;
         pLoad->get_Property(_T("DC_StrengthI"),&var);
         pObj->m_gDC[pgsTypes::StrengthI_Inventory+indexOffset] = var.dblVal;

         pLoad->get_Property(_T("DW_StrengthI"),&var);
         pObj->m_gDW[pgsTypes::StrengthI_Inventory+indexOffset] = var.dblVal;

         if ( 2 < version )
         {
            pLoad->get_Property(_T("CR_StrengthI"),&var);
            pObj->m_gCR[pgsTypes::StrengthI_Inventory] = var.dblVal; // don't add indexOffset here because this data didn't exist in version 1

            pLoad->get_Property(_T("SH_StrengthI"),&var);
            pObj->m_gSH[pgsTypes::StrengthI_Inventory] = var.dblVal;

            if ( 3 < version )
            {
               pLoad->get_Property(_T("RE_StrengthI"),&var);
               pObj->m_gRE[pgsTypes::StrengthI_Inventory] = var.dblVal;
            }

            pLoad->get_Property(_T("PS_StrengthI"),&var);
            pObj->m_gPS[pgsTypes::StrengthI_Inventory] = var.dblVal;
         }

         pLoad->get_Property(_T("LL_StrengthI"),&var);
         pObj->m_gLL[pgsTypes::StrengthI_Inventory+indexOffset] = var.dblVal;

         pLoad->get_Property(_T("DC_ServiceIII"),&var);
         pObj->m_gDC[pgsTypes::ServiceIII_Inventory+indexOffset] = var.dblVal;

         pLoad->get_Property(_T("DW_ServiceIII"),&var);
         pObj->m_gDW[pgsTypes::ServiceIII_Inventory+indexOffset] = var.dblVal;

         if ( 2 < version )
         {
            pLoad->get_Property(_T("CR_ServiceIII"),&var);
            pObj->m_gCR[pgsTypes::ServiceIII_Inventory] = var.dblVal;

            pLoad->get_Property(_T("SH_ServiceIII"),&var);
            pObj->m_gSH[pgsTypes::ServiceIII_Inventory] = var.dblVal;

            if ( 3 < version )
            {
               pLoad->get_Property(_T("RE_ServiceIII"),&var);
               pObj->m_gRE[pgsTypes::ServiceIII_Inventory] = var.dblVal;
            }

            pLoad->get_Property(_T("PS_ServiceIII"),&var);
            pObj->m_gPS[pgsTypes::ServiceIII_Inventory] = var.dblVal;
         }

         pLoad->get_Property(_T("LL_ServiceIII"),&var);
         pObj->m_gLL[pgsTypes::ServiceIII_Inventory+indexOffset] = var.dblVal;

         pLoad->get_Property(_T("AllowableTensionCoefficient"),&var);
         pObj->m_AllowableTensionCoefficient[pgsTypes::lrDesign_Inventory] = var.dblVal;

         var.vt = VT_BOOL;
         pLoad->get_Property(_T("RateForShear"),&var);
         pObj->m_bRateForShear[pgsTypes::lrDesign_Inventory] = (var.boolVal == VARIANT_TRUE ? true : false);

         pLoad->EndUnit(); // DesignInventoryRating
         }
         //////////////////////////////////////////////////////

         //////////////////////////////////////////////////////
         {
         Float64 version;

         pLoad->BeginUnit(_T("DesignOperatingRating"));
         pLoad->get_Version(&version);

         IndexType indexOffset = 0;
         if ( version < 2.0 )
         {
            indexOffset = (IndexType)pgsTypes::StrengthI_Inventory;
         }

         var.vt = VT_BOOL;
         pLoad->get_Property(_T("Enabled"),&var);
         pObj->m_bEnableRating[pgsTypes::lrDesign_Operating] = (var.boolVal == VARIANT_TRUE ? true : false);

         var.vt = VT_R8;
         pLoad->get_Property(_T("DC_StrengthI"),&var);
         pObj->m_gDC[pgsTypes::StrengthI_Operating+indexOffset] = var.dblVal;

         pLoad->get_Property(_T("DW_StrengthI"),&var);
         pObj->m_gDW[pgsTypes::StrengthI_Operating+indexOffset] = var.dblVal;

         if ( 2 < version )
         {
            pLoad->get_Property(_T("CR_StrengthI"),&var);
            pObj->m_gCR[pgsTypes::StrengthI_Operating] = var.dblVal;

            pLoad->get_Property(_T("SH_StrengthI"),&var);
            pObj->m_gSH[pgsTypes::StrengthI_Operating] = var.dblVal;

            if ( 3 < version )
            {
               pLoad->get_Property(_T("RE_StrengthI"),&var);
               pObj->m_gRE[pgsTypes::StrengthI_Operating] = var.dblVal;
            }

            pLoad->get_Property(_T("PS_StrengthI"),&var);
            pObj->m_gPS[pgsTypes::StrengthI_Operating] = var.dblVal;
         }

         pLoad->get_Property(_T("LL_StrengthI"),&var);
         pObj->m_gLL[pgsTypes::StrengthI_Operating+indexOffset] = var.dblVal;

         pLoad->get_Property(_T("DC_ServiceIII"),&var);
         pObj->m_gDC[pgsTypes::ServiceIII_Operating+indexOffset] = var.dblVal;

         pLoad->get_Property(_T("DW_ServiceIII"),&var);
         pObj->m_gDW[pgsTypes::ServiceIII_Operating+indexOffset] = var.dblVal;

         if ( 2 < version )
         {
            pLoad->get_Property(_T("CR_ServiceIII"),&var);
            pObj->m_gCR[pgsTypes::ServiceIII_Operating] = var.dblVal;

            pLoad->get_Property(_T("SH_ServiceIII"),&var);
            pObj->m_gSH[pgsTypes::ServiceIII_Operating] = var.dblVal;

            if ( 3 < version ) 
            {
               pLoad->get_Property(_T("RE_ServiceIII"),&var);
               pObj->m_gRE[pgsTypes::ServiceIII_Operating] = var.dblVal;
            }

            pLoad->get_Property(_T("PS_ServiceIII"),&var);
            pObj->m_gPS[pgsTypes::ServiceIII_Operating] = var.dblVal;
         }

         pLoad->get_Property(_T("LL_ServiceIII"),&var);
         pObj->m_gLL[pgsTypes::ServiceIII_Operating+indexOffset] = var.dblVal;

         pLoad->get_Property(_T("AllowableTensionCoefficient"),&var);
         pObj->m_AllowableTensionCoefficient[pgsTypes::lrDesign_Operating] = var.dblVal;

         var.vt = VT_BOOL;
         pLoad->get_Property(_T("RateForShear"),&var);
         pObj->m_bRateForShear[pgsTypes::lrDesign_Operating] = (var.boolVal == VARIANT_TRUE ? true : false);

         pLoad->EndUnit(); // DesignOperatingRating
         }
         //////////////////////////////////////////////////////

         //////////////////////////////////////////////////////
         {
         Float64 version;

         pLoad->BeginUnit(_T("LegalRoutineRating"));

         pLoad->get_Version(&version);

         IndexType indexOffset = 0;
         if ( version < 2.0 )
         {
            indexOffset = (IndexType)pgsTypes::StrengthI_Inventory;
         }

         var.vt = VT_BOOL;
         pLoad->get_Property(_T("Enabled"),&var);
         pObj->m_bEnableRating[pgsTypes::lrLegal_Routine] = (var.boolVal == VARIANT_TRUE ? true : false);

         var.vt = VT_R8;
         pLoad->get_Property(_T("DC_StrengthI"),&var);
         pObj->m_gDC[pgsTypes::StrengthI_LegalRoutine+indexOffset] = var.dblVal;

         pLoad->get_Property(_T("DW_StrengthI"),&var);
         pObj->m_gDW[pgsTypes::StrengthI_LegalRoutine+indexOffset] = var.dblVal;

         if ( 2 < version )
         {
            pLoad->get_Property(_T("CR_StrengthI"),&var);
            pObj->m_gCR[pgsTypes::StrengthI_LegalRoutine] = var.dblVal;

            pLoad->get_Property(_T("SH_StrengthI"),&var);
            pObj->m_gSH[pgsTypes::StrengthI_LegalRoutine] = var.dblVal;

            if ( 3 < version )
            {
               pLoad->get_Property(_T("RE_StrengthI"),&var);
               pObj->m_gRE[pgsTypes::StrengthI_LegalRoutine] = var.dblVal;
            }

            pLoad->get_Property(_T("PS_StrengthI"),&var);
            pObj->m_gPS[pgsTypes::StrengthI_LegalRoutine] = var.dblVal;
         }

         pLoad->get_Property(_T("LL_StrengthI"),&var);
         pObj->m_gLL[pgsTypes::StrengthI_LegalRoutine+indexOffset] = var.dblVal;

         // from this point forward, all stored values for m_gDC, m_bDW, and m_gLL are invalid
         // if version less than 2 (if 1 < version, then the value is good)
         pLoad->get_Property(_T("DC_ServiceIII"),&var);
         if (1 < version)
         {
            pObj->m_gDC[pgsTypes::ServiceIII_LegalRoutine] = var.dblVal;
         }

         pLoad->get_Property(_T("DW_ServiceIII"),&var);
         if (1 < version)
         {
            pObj->m_gDW[pgsTypes::ServiceIII_LegalRoutine] = var.dblVal;
         }

         if ( 2 < version )
         {
            pLoad->get_Property(_T("CR_ServiceIII"),&var);
            pObj->m_gCR[pgsTypes::ServiceIII_LegalRoutine] = var.dblVal;

            pLoad->get_Property(_T("SH_ServiceIII"),&var);
            pObj->m_gSH[pgsTypes::ServiceIII_LegalRoutine] = var.dblVal;

            if ( 3 < version )
            {
               pLoad->get_Property(_T("RE_ServiceIII"),&var);
               pObj->m_gRE[pgsTypes::ServiceIII_LegalRoutine] = var.dblVal;
            }

            pLoad->get_Property(_T("PS_ServiceIII"),&var);
            pObj->m_gPS[pgsTypes::ServiceIII_LegalRoutine] = var.dblVal;
         }

         pLoad->get_Property(_T("LL_ServiceIII"),&var);
         if (1 < version)
         {
            pObj->m_gLL[pgsTypes::ServiceIII_LegalRoutine] = var.dblVal;
         }

         pLoad->get_Property(_T("AllowableTensionCoefficient"),&var);
         pObj->m_AllowableTensionCoefficient[pgsTypes::lrLegal_Routine] = var.dblVal;

         var.vt = VT_BOOL;
         pLoad->get_Property(_T("RateForShear"),&var);
         pObj->m_bRateForShear[pgsTypes::lrLegal_Routine] = (var.boolVal == VARIANT_TRUE ? true : false);

         pLoad->get_Property(_T("RateForStress"),&var);
         pObj->m_bRateForStress[pgsTypes::lrLegal_Routine] = (var.boolVal == VARIANT_TRUE ? true : false);

         pLoad->get_Property(_T("ExcludeLegalLoadLaneLoading"),&var);
         pObj->m_bExcludeLegalLoadLaneLoading = (var.boolVal == VARIANT_TRUE ? true : false);

         pLoad->EndUnit(); // LegalRoutineRating
         }
         //////////////////////////////////////////////////////

         //////////////////////////////////////////////////////
         {
         Float64 version;

         pLoad->BeginUnit(_T("LegalSpecialRating"));

         pLoad->get_Version(&version);

         var.vt = VT_BOOL;
         pLoad->get_Property(_T("Enabled"),&var);
         pObj->m_bEnableRating[pgsTypes::lrLegal_Special] = (var.boolVal == VARIANT_TRUE ? true : false);

         var.vt = VT_R8;
         pLoad->get_Property(_T("DC_StrengthI"),&var);
         if (1 < version)
         {
            pObj->m_gDC[pgsTypes::StrengthI_LegalSpecial] = var.dblVal;
         }

         pLoad->get_Property(_T("DW_StrengthI"),&var);
         if (1 < version)
         {
            pObj->m_gDW[pgsTypes::StrengthI_LegalSpecial] = var.dblVal;
         }

         if ( 2 < version )
         {
            pLoad->get_Property(_T("CR_StrengthI"),&var);
            pObj->m_gCR[pgsTypes::StrengthI_LegalSpecial] = var.dblVal;

            pLoad->get_Property(_T("SH_StrengthI"),&var);
            pObj->m_gSH[pgsTypes::StrengthI_LegalSpecial] = var.dblVal;

            if ( 3 < version )
            {
               pLoad->get_Property(_T("RE_StrengthI"),&var);
               pObj->m_gRE[pgsTypes::StrengthI_LegalSpecial] = var.dblVal;
            }

            pLoad->get_Property(_T("PS_StrengthI"),&var);
            pObj->m_gPS[pgsTypes::StrengthI_LegalSpecial] = var.dblVal;
         }

         pLoad->get_Property(_T("LL_StrengthI"),&var);
         if (1 < version)
         {
            pObj->m_gLL[pgsTypes::StrengthI_LegalSpecial] = var.dblVal;
         }

         pLoad->get_Property(_T("DC_ServiceIII"),&var);
         if (1 < version)
         {
            pObj->m_gDC[pgsTypes::ServiceIII_LegalSpecial] = var.dblVal;
         }

         pLoad->get_Property(_T("DW_ServiceIII"),&var);
         if (1 < version)
         {
            pObj->m_gDW[pgsTypes::ServiceIII_LegalSpecial] = var.dblVal;
         }

         if ( 2 < version )
         {
            pLoad->get_Property(_T("CR_ServiceIII"),&var);
            pObj->m_gCR[pgsTypes::ServiceIII_LegalSpecial] = var.dblVal;

            pLoad->get_Property(_T("SH_ServiceIII"),&var);
            pObj->m_gSH[pgsTypes::ServiceIII_LegalSpecial] = var.dblVal;

            if ( 3 < version )
            {
               pLoad->get_Property(_T("RE_ServiceIII"),&var);
               pObj->m_gRE[pgsTypes::ServiceIII_LegalSpecial] = var.dblVal;
            }

            pLoad->get_Property(_T("PS_ServiceIII"),&var);
            pObj->m_gPS[pgsTypes::ServiceIII_LegalSpecial] = var.dblVal;
         }

         pLoad->get_Property(_T("LL_ServiceIII"),&var);
         if (1 < version)
         {
            pObj->m_gLL[pgsTypes::ServiceIII_LegalSpecial] = var.dblVal;
         }

         pLoad->get_Property(_T("AllowableTensionCoefficient"),&var);
         pObj->m_AllowableTensionCoefficient[pgsTypes::lrLegal_Special] = var.dblVal;

         var.vt = VT_BOOL;
         pLoad->get_Property(_T("RateForShear"),&var);
         pObj->m_bRateForShear[pgsTypes::lrLegal_Special] = (var.boolVal == VARIANT_TRUE ? true : false);

         pLoad->get_Property(_T("RateForStress"),&var);
         pObj->m_bRateForStress[pgsTypes::lrLegal_Special] = (var.boolVal == VARIANT_TRUE ? true : false);

         pLoad->EndUnit(); // LegalSpecialRating
         }
         //////////////////////////////////////////////////////

         if (1 < top_version)
         {
            // added in top_version 2
            //////////////////////////////////////////////////////
            {
               pLoad->BeginUnit(_T("LegalEmergencyRating"));

               var.vt = VT_BOOL;
               pLoad->get_Property(_T("Enabled"), &var);
               pObj->m_bEnableRating[pgsTypes::lrLegal_Emergency] = (var.boolVal == VARIANT_TRUE ? true : false);

               var.vt = VT_R8;
               pLoad->get_Property(_T("DC_StrengthI"), &var);
               pObj->m_gDC[pgsTypes::StrengthI_LegalEmergency] = var.dblVal;

               pLoad->get_Property(_T("DW_StrengthI"), &var);
               pObj->m_gDW[pgsTypes::StrengthI_LegalEmergency] = var.dblVal;

               pLoad->get_Property(_T("CR_StrengthI"), &var);
               pObj->m_gCR[pgsTypes::StrengthI_LegalEmergency] = var.dblVal;

               pLoad->get_Property(_T("SH_StrengthI"), &var);
               pObj->m_gSH[pgsTypes::StrengthI_LegalEmergency] = var.dblVal;

               pLoad->get_Property(_T("RE_StrengthI"), &var);
               pObj->m_gRE[pgsTypes::StrengthI_LegalEmergency] = var.dblVal;

               pLoad->get_Property(_T("PS_StrengthI"), &var);
               pObj->m_gPS[pgsTypes::StrengthI_LegalEmergency] = var.dblVal;

               pLoad->get_Property(_T("LL_StrengthI"), &var);
               pObj->m_gLL[pgsTypes::StrengthI_LegalEmergency] = var.dblVal;

               pLoad->get_Property(_T("DC_ServiceIII"), &var);
               pObj->m_gDC[pgsTypes::ServiceIII_LegalEmergency] = var.dblVal;

               pLoad->get_Property(_T("DW_ServiceIII"), &var);
               pObj->m_gDW[pgsTypes::ServiceIII_LegalEmergency] = var.dblVal;

               pLoad->get_Property(_T("CR_ServiceIII"), &var);
               pObj->m_gCR[pgsTypes::ServiceIII_LegalEmergency] = var.dblVal;

               pLoad->get_Property(_T("SH_ServiceIII"), &var);
               pObj->m_gSH[pgsTypes::ServiceIII_LegalEmergency] = var.dblVal;

               pLoad->get_Property(_T("RE_ServiceIII"), &var);
               pObj->m_gRE[pgsTypes::ServiceIII_LegalEmergency] = var.dblVal;

               pLoad->get_Property(_T("PS_ServiceIII"), &var);
               pObj->m_gPS[pgsTypes::ServiceIII_LegalEmergency] = var.dblVal;

               pLoad->get_Property(_T("LL_ServiceIII"), &var);
               pObj->m_gLL[pgsTypes::ServiceIII_LegalEmergency] = var.dblVal;

               pLoad->get_Property(_T("AllowableTensionCoefficient"), &var);
               pObj->m_AllowableTensionCoefficient[pgsTypes::lrLegal_Emergency] = var.dblVal;

               var.vt = VT_BOOL;
               pLoad->get_Property(_T("RateForShear"), &var);
               pObj->m_bRateForShear[pgsTypes::lrLegal_Emergency] = (var.boolVal == VARIANT_TRUE ? true : false);

               pLoad->get_Property(_T("RateForStress"), &var);
               pObj->m_bRateForStress[pgsTypes::lrLegal_Emergency] = (var.boolVal == VARIANT_TRUE ? true : false);

               pLoad->EndUnit(); // LegalEmergencyRating
            }
            //////////////////////////////////////////////////////
         }

         //////////////////////////////////////////////////////
         {
         Float64 version;

         pLoad->BeginUnit(_T("PermitRoutineRating"));

         pLoad->get_Version(&version);

         var.vt = VT_BOOL;
         pLoad->get_Property(_T("Enabled"),&var);
         pObj->m_bEnableRating[pgsTypes::lrPermit_Routine] = (var.boolVal == VARIANT_TRUE ? true : false);

         var.vt = VT_R8;
         pLoad->get_Property(_T("DC_StrengthII"),&var);
         if (1 < version)
         {
            pObj->m_gDC[pgsTypes::StrengthII_PermitRoutine] = var.dblVal;
         }

         pLoad->get_Property(_T("DW_StrengthII"),&var);
         if (1 < version)
         {
            pObj->m_gDW[pgsTypes::StrengthII_PermitRoutine] = var.dblVal;
         }

         if ( 2 < version )
         {
            pLoad->get_Property(_T("CR_StrengthII"),&var);
            pObj->m_gCR[pgsTypes::StrengthII_PermitRoutine] = var.dblVal;

            pLoad->get_Property(_T("SH_StrengthII"),&var);
            pObj->m_gSH[pgsTypes::StrengthII_PermitRoutine] = var.dblVal;

            if ( 3 < version )
            {
               pLoad->get_Property(_T("RE_StrengthII"),&var);
               pObj->m_gRE[pgsTypes::StrengthII_PermitRoutine] = var.dblVal;
            }

            pLoad->get_Property(_T("PS_StrengthII"),&var);
            pObj->m_gPS[pgsTypes::StrengthII_PermitRoutine] = var.dblVal;
         }

         pLoad->get_Property(_T("LL_StrengthII"),&var);
         if (1 < version)
         {
            pObj->m_gLL[pgsTypes::StrengthII_PermitRoutine] = var.dblVal;
         }

         pLoad->get_Property(_T("DC_ServiceI"),&var);
         if (1 < version)
         {
            pObj->m_gDC[pgsTypes::ServiceI_PermitRoutine] = var.dblVal;
         }

         pLoad->get_Property(_T("DW_ServiceI"),&var);
         if (1 < version)
         {
            pObj->m_gDW[pgsTypes::ServiceI_PermitRoutine] = var.dblVal;
         }

         if ( 2 < version )
         {
            pLoad->get_Property(_T("CR_ServiceI"),&var);
            pObj->m_gCR[pgsTypes::ServiceI_PermitRoutine] = var.dblVal;

            pLoad->get_Property(_T("SH_ServiceI"),&var);
            pObj->m_gSH[pgsTypes::ServiceI_PermitRoutine] = var.dblVal;

            if ( 3 < version )
            {
               pLoad->get_Property(_T("RE_ServiceI"),&var);
               pObj->m_gRE[pgsTypes::ServiceI_PermitRoutine] = var.dblVal;
            }

            pLoad->get_Property(_T("PS_ServiceI"),&var);
            pObj->m_gPS[pgsTypes::ServiceI_PermitRoutine] = var.dblVal;
         }

         pLoad->get_Property(_T("LL_ServiceI"),&var);
         if (1 < version)
         {
            pObj->m_gLL[pgsTypes::ServiceI_PermitRoutine] = var.dblVal;
         }

         if ( 4 < version )
         {
            pLoad->get_Property(_T("DC_ServiceIII"),&var);
            pObj->m_gDC[pgsTypes::ServiceIII_PermitRoutine] = var.dblVal;

            pLoad->get_Property(_T("DW_ServiceIII"),&var);
            pObj->m_gDW[pgsTypes::ServiceIII_PermitRoutine] = var.dblVal;

            pLoad->get_Property(_T("CR_ServiceIII"),&var);
            pObj->m_gCR[pgsTypes::ServiceIII_PermitRoutine] = var.dblVal;

            pLoad->get_Property(_T("SH_ServiceIII"),&var);
            pObj->m_gSH[pgsTypes::ServiceIII_PermitRoutine] = var.dblVal;

            pLoad->get_Property(_T("RE_ServiceIII"),&var);
            pObj->m_gRE[pgsTypes::ServiceIII_PermitRoutine] = var.dblVal;

            pLoad->get_Property(_T("PS_ServiceIII"),&var);
            pObj->m_gPS[pgsTypes::ServiceIII_PermitRoutine] = var.dblVal;

            pLoad->get_Property(_T("LL_ServiceIII"),&var);
            pObj->m_gLL[pgsTypes::ServiceIII_PermitRoutine] = var.dblVal;
         
            var.vt = VT_BOOL;
            pLoad->get_Property(_T("CheckYieldStress"),&var);
            pObj->m_bCheckYieldStress[pgsTypes::lrPermit_Routine] = (var.boolVal == VARIANT_TRUE ? true : false);

            var.vt = VT_R8;
            pLoad->get_Property(_T("AllowableTensionCoefficient"),&var);
            pObj->m_AllowableTensionCoefficient[pgsTypes::lrPermit_Routine] = var.dblVal;
         }

         pLoad->get_Property(_T("AllowableYieldStressCoefficient"),&var);
         pObj->m_AllowableYieldStressCoefficient = var.dblVal;

         var.vt = VT_BOOL;
         pLoad->get_Property(_T("RateForShear"),&var);
         pObj->m_bRateForShear[pgsTypes::lrPermit_Routine] = (var.boolVal == VARIANT_TRUE ? true : false);
         
         pLoad->get_Property(_T("RateForStress"),&var);
         pObj->m_bRateForStress[pgsTypes::lrPermit_Routine] = (var.boolVal == VARIANT_TRUE ? true : false);
         if ( version < 5 )
         {
            // prior to version 5, the yield stress check was stored with the RateForStress tag
            // it moved to the CheckYieldStress tag in version 5. Make the adjustment here
            pObj->m_bCheckYieldStress[pgsTypes::lrPermit_Routine] = pObj->m_bRateForStress[pgsTypes::lrPermit_Routine];
            pObj->m_bRateForStress[pgsTypes::lrPermit_Routine] = false;
         }

         pLoad->EndUnit(); // PermitRoutineRating
         }
         //////////////////////////////////////////////////////

         //////////////////////////////////////////////////////
         {
         Float64 version;

         pLoad->BeginUnit(_T("PermitSpecialRating"));

         pLoad->get_Version(&version);

         var.vt = VT_BOOL;
         pLoad->get_Property(_T("Enabled"),&var);
         pObj->m_bEnableRating[pgsTypes::lrPermit_Special] = (var.boolVal == VARIANT_TRUE ? true : false);

         var.vt = VT_R8;
         pLoad->get_Property(_T("DC_StrengthII"),&var);
         if (1 < version)
         {
            pObj->m_gDC[pgsTypes::StrengthII_PermitSpecial] = var.dblVal;
         }

         pLoad->get_Property(_T("DW_StrengthII"),&var);
         if (1 < version)
         {
            pObj->m_gDW[pgsTypes::StrengthII_PermitSpecial] = var.dblVal;
         }

         if ( 2 < version )
         {
            pLoad->get_Property(_T("CR_StrengthII"),&var);
            pObj->m_gCR[pgsTypes::StrengthII_PermitSpecial] = var.dblVal;

            pLoad->get_Property(_T("SH_StrengthII"),&var);
            pObj->m_gSH[pgsTypes::StrengthII_PermitSpecial] = var.dblVal;

            if ( 3 < version )
            {
               pLoad->get_Property(_T("RE_StrengthII"),&var);
               pObj->m_gRE[pgsTypes::StrengthII_PermitSpecial] = var.dblVal;
            }

            pLoad->get_Property(_T("PS_StrengthII"),&var);
            pObj->m_gPS[pgsTypes::StrengthII_PermitSpecial] = var.dblVal;
         }

         pLoad->get_Property(_T("LL_StrengthII"),&var);
         if (1 < version)
         {
            pObj->m_gLL[pgsTypes::StrengthII_PermitSpecial] = var.dblVal;
         }

         pLoad->get_Property(_T("DC_ServiceI"),&var);
         if (1 < version)
         {
            pObj->m_gDC[pgsTypes::ServiceI_PermitSpecial] = var.dblVal;
         }

         pLoad->get_Property(_T("DW_ServiceI"),&var);
         if (1 < version)
         {
            pObj->m_gDW[pgsTypes::ServiceI_PermitSpecial] = var.dblVal;
         }

         if ( 2 < version )
         {
            pLoad->get_Property(_T("CR_ServiceI"),&var);
            pObj->m_gCR[pgsTypes::ServiceI_PermitSpecial] = var.dblVal;

            pLoad->get_Property(_T("SH_ServiceI"),&var);
            pObj->m_gSH[pgsTypes::ServiceI_PermitSpecial] = var.dblVal;

            if ( 3 < version )
            {
               pLoad->get_Property(_T("RE_ServiceI"),&var);
               pObj->m_gRE[pgsTypes::ServiceI_PermitSpecial] = var.dblVal;
            }

            pLoad->get_Property(_T("PS_ServiceI"),&var);
            pObj->m_gPS[pgsTypes::ServiceI_PermitSpecial] = var.dblVal;
         }

         pLoad->get_Property(_T("LL_ServiceI"),&var);
         if (1 < version)
         {
            pObj->m_gLL[pgsTypes::ServiceI_PermitSpecial] = var.dblVal;
         }


         if ( 4 < version )
         {
            pLoad->get_Property(_T("DC_ServiceIII"),&var);
            pObj->m_gDC[pgsTypes::ServiceIII_PermitSpecial] = var.dblVal;

            pLoad->get_Property(_T("DW_ServiceIII"),&var);
            pObj->m_gDW[pgsTypes::ServiceIII_PermitSpecial] = var.dblVal;

            pLoad->get_Property(_T("CR_ServiceIII"),&var);
            pObj->m_gCR[pgsTypes::ServiceIII_PermitSpecial] = var.dblVal;

            pLoad->get_Property(_T("SH_ServiceIII"),&var);
            pObj->m_gSH[pgsTypes::ServiceIII_PermitSpecial] = var.dblVal;

            pLoad->get_Property(_T("RE_ServiceIII"),&var);
            pObj->m_gRE[pgsTypes::ServiceIII_PermitSpecial] = var.dblVal;

            pLoad->get_Property(_T("PS_ServiceIII"),&var);
            pObj->m_gPS[pgsTypes::ServiceIII_PermitSpecial] = var.dblVal;

            pLoad->get_Property(_T("LL_ServiceIII"),&var);
            pObj->m_gLL[pgsTypes::ServiceIII_PermitSpecial] = var.dblVal;
         
            var.vt = VT_BOOL;
            pLoad->get_Property(_T("CheckYieldStress"),&var);
            pObj->m_bCheckYieldStress[pgsTypes::lrPermit_Special] = (var.boolVal == VARIANT_TRUE ? true : false);

            var.vt = VT_R8;
            pLoad->get_Property(_T("AllowableTensionCoefficient"),&var);
            pObj->m_AllowableTensionCoefficient[pgsTypes::lrPermit_Special] = var.dblVal;
         }

         var.vt = VT_I4;
         pLoad->get_Property(_T("SpecialPermitType"),&var);
         pObj->m_SpecialPermitType = (pgsTypes::SpecialPermitType)var.lVal;

         var.vt = VT_BOOL;
         pLoad->get_Property(_T("RateForShear"),&var);
         pObj->m_bRateForShear[pgsTypes::lrPermit_Special] = (var.boolVal == VARIANT_TRUE ? true : false);
         
         pLoad->get_Property(_T("RateForStress"),&var);
         pObj->m_bRateForStress[pgsTypes::lrPermit_Special] = (var.boolVal == VARIANT_TRUE ? true : false);
         if ( version < 5 )
         {
            // prior to version 5, the yield stress check was stored with the RateForStress tag
            // it moved to the CheckYieldStress tag in version 5. Make the adjustment here
            pObj->m_bCheckYieldStress[pgsTypes::lrPermit_Special] = pObj->m_bRateForStress[pgsTypes::lrPermit_Special];
            pObj->m_bRateForStress[pgsTypes::lrPermit_Special] = false;
         }

         pLoad->EndUnit(); // PermitSpecialRating

         if ( version < 2 )
         {
            // The bug in storing/reading load factors from data block version 1, described above, wrote out bad data for the PermitSpecial cases
            // This data was never accessed in PGSuper v 2.9.x and earlier so it was never an issue. However, the data is accessed in v3.0.x and later.
            // So that we don't have bad data, copy the data from the PermitRoute cases. 
            pObj->m_gDC[pgsTypes::StrengthII_PermitSpecial] = pObj->m_gDC[pgsTypes::StrengthII_PermitRoutine];
            pObj->m_gDW[pgsTypes::StrengthII_PermitSpecial] = pObj->m_gDW[pgsTypes::StrengthII_PermitRoutine];
            pObj->m_gCR[pgsTypes::StrengthII_PermitSpecial] = pObj->m_gCR[pgsTypes::StrengthII_PermitRoutine];
            pObj->m_gSH[pgsTypes::StrengthII_PermitSpecial] = pObj->m_gSH[pgsTypes::StrengthII_PermitRoutine];
            pObj->m_gRE[pgsTypes::StrengthII_PermitSpecial] = pObj->m_gRE[pgsTypes::StrengthII_PermitRoutine];
            pObj->m_gPS[pgsTypes::StrengthII_PermitSpecial] = pObj->m_gPS[pgsTypes::StrengthII_PermitRoutine];
            pObj->m_gLL[pgsTypes::StrengthII_PermitSpecial] = pObj->m_gLL[pgsTypes::StrengthII_PermitRoutine];

            pObj->m_gDC[pgsTypes::ServiceI_PermitSpecial] = pObj->m_gDC[pgsTypes::ServiceI_PermitRoutine];
            pObj->m_gDW[pgsTypes::ServiceI_PermitSpecial] = pObj->m_gDW[pgsTypes::ServiceI_PermitRoutine];
            pObj->m_gCR[pgsTypes::ServiceI_PermitSpecial] = pObj->m_gCR[pgsTypes::ServiceI_PermitRoutine];
            pObj->m_gSH[pgsTypes::ServiceI_PermitSpecial] = pObj->m_gSH[pgsTypes::ServiceI_PermitRoutine];
            pObj->m_gRE[pgsTypes::ServiceI_PermitSpecial] = pObj->m_gRE[pgsTypes::ServiceI_PermitRoutine];
            pObj->m_gPS[pgsTypes::ServiceI_PermitSpecial] = pObj->m_gPS[pgsTypes::ServiceI_PermitRoutine];
            pObj->m_gLL[pgsTypes::ServiceI_PermitSpecial] = pObj->m_gLL[pgsTypes::ServiceI_PermitRoutine];

            pObj->m_gDC[pgsTypes::ServiceIII_PermitSpecial] = pObj->m_gDC[pgsTypes::ServiceIII_PermitRoutine];
            pObj->m_gDW[pgsTypes::ServiceIII_PermitSpecial] = pObj->m_gDW[pgsTypes::ServiceIII_PermitRoutine];
            pObj->m_gCR[pgsTypes::ServiceIII_PermitSpecial] = pObj->m_gCR[pgsTypes::ServiceIII_PermitRoutine];
            pObj->m_gSH[pgsTypes::ServiceIII_PermitSpecial] = pObj->m_gSH[pgsTypes::ServiceIII_PermitRoutine];
            pObj->m_gRE[pgsTypes::ServiceIII_PermitSpecial] = pObj->m_gRE[pgsTypes::ServiceIII_PermitRoutine];
            pObj->m_gPS[pgsTypes::ServiceIII_PermitSpecial] = pObj->m_gPS[pgsTypes::ServiceIII_PermitRoutine];
            pObj->m_gLL[pgsTypes::ServiceIII_PermitSpecial] = pObj->m_gLL[pgsTypes::ServiceIII_PermitRoutine];
         }

         }
         //////////////////////////////////////////////////////

         pLoad->EndUnit();
      }
   }

   return S_OK;
}

HRESULT CProjectAgentImp::UnitModeProc(IStructuredSave* pSave,IStructuredLoad* pLoad,IProgress* pProgress,CProjectAgentImp* pObj)
{
   // The EAFDocProxy agent should be peristing this value since it is responsible for it
   // However, it would really mess up all of the existing PGSuper files.
   // It is just easier to persist it here.
   GET_IFACE2(pObj->m_pBroker,IEAFDisplayUnits,pDisplayUnits);
   eafTypes::UnitMode unitMode;

   HRESULT hr = S_OK;
   if ( pSave )
   {
      unitMode = pDisplayUnits->GetUnitMode();
      hr = pSave->put_Property(_T("Units"),CComVariant(unitMode));
      if ( FAILED(hr) )
      {
         return hr;
      }
   }
   else
   {
      eafTypes::UnitMode unitMode;
      CComVariant var;
      var.vt = VT_I4;
      hr = pLoad->get_Property(_T("Units"),&var);
      if ( FAILED(hr) )
      {
         return hr;
      }

      unitMode = (eafTypes::UnitMode)(var.iVal);
      pDisplayUnits->SetUnitMode(unitMode);
   }

   return hr;
}

HRESULT CProjectAgentImp::AlignmentProc(IStructuredSave* pSave,IStructuredLoad* pLoad,IProgress* pProgress,CProjectAgentImp* pObj)
{
   HRESULT hr = S_OK;

   if ( pSave )
   {
      // Save the alignment data
      hr = pSave->BeginUnit(_T("AlignmentData"),5.0);
      if ( FAILED(hr) )
      {
         return hr;
      }

// eliminated in version 4
//      hr = pSave->put_Property(_T("AlignmentOffset"),CComVariant(pObj->m_AlignmentOffset));
//      if ( FAILED(hr) )
//         return hr;
      
      // added in version 5
      hr = pSave->put_Property(_T("RefStation"),CComVariant(pObj->m_AlignmentData2.RefStation));
      if ( FAILED(hr) )
      {
         return hr;
      }
      
      // added in version 5
      hr = pSave->put_Property(_T("RefPointNorthing"),CComVariant(pObj->m_AlignmentData2.yRefPoint));
      if ( FAILED(hr) )
      {
         return hr;
      }
      
      // added in version 5
      hr = pSave->put_Property(_T("RefPointEasting"),CComVariant(pObj->m_AlignmentData2.xRefPoint));
      if ( FAILED(hr) )
      {
         return hr;
      }


      hr = pSave->put_Property(_T("Direction"),CComVariant(pObj->m_AlignmentData2.Direction));
      if ( FAILED(hr) )
      {
         return hr;
      }

      hr = pSave->put_Property(_T("HorzCurveCount"),CComVariant((long)pObj->m_AlignmentData2.HorzCurves.size()));
      if ( FAILED(hr) )
      {
         return hr;
      }

      std::vector<HorzCurveData>::iterator iter;
      for ( iter = pObj->m_AlignmentData2.HorzCurves.begin(); iter != pObj->m_AlignmentData2.HorzCurves.end(); iter++ )
      {
         HorzCurveData& hc = *iter;

         hr = pSave->BeginUnit(_T("HorzCurveData"),2.0);
         if ( FAILED(hr) )
         {
            return hr;
         }

         hr = pSave->put_Property(_T("PIStation"),CComVariant(hc.PIStation));
         if ( FAILED(hr) )
         {
            return hr;
         }

         hr = pSave->put_Property(_T("FwdTangent"),CComVariant(hc.FwdTangent));
         if ( FAILED(hr) )
         {
            return hr;
         }

         hr = pSave->put_Property(_T("Radius"),CComVariant(hc.Radius));
         if ( FAILED(hr) )
         {
            return hr;
         }

         hr = pSave->put_Property(_T("EntrySpiral"),CComVariant(hc.EntrySpiral));
         if ( FAILED(hr) )
         {
            return hr;
         }

         hr = pSave->put_Property(_T("ExitSpiral"),CComVariant(hc.ExitSpiral));
         if ( FAILED(hr) )
         {
            return hr;
         }

         hr = pSave->put_Property(_T("FwdTangentIsBearing"),CComVariant(hc.bFwdTangent));
         if ( FAILED(hr) )
         {
            return hr;
         }

         hr = pSave->EndUnit();
         if ( FAILED(hr) )
         {
            return hr;
         }
      }

      hr = pSave->EndUnit();
      if ( FAILED(hr) )
      {
         return hr;
      }
   }
   else
   {
      CComVariant var;

      bool bConvert = false;

      // If BeginUnit fails then this file was created before the unit was added
      // If this is the case, the Direction and Tangents need to be converted
      hr = pLoad->BeginUnit(_T("AlignmentData"));
      if ( FAILED(hr) )
      {
         bConvert = true; 
      }

      Float64 version;
      pLoad->get_Version(&version);
      if ( version < 2 )
      {
         // version 1 style data... read the data and hold in local variables
         pObj->m_AlignmentOffset_Temp = 0;
         pObj->m_bUseTempAlignmentOffset = true;
      }
      else if ( version < 4 )
      {
         // alignment offset was eliminated in version 4
         var.vt = VT_R8;
         hr = pLoad->get_Property(_T("AlignmentOffset"),&var);
         if ( FAILED(hr) )
         {
            return hr;
         }

         pObj->m_AlignmentOffset_Temp = var.dblVal;
         pObj->m_bUseTempAlignmentOffset = true;
      }
      else
      {
         // alignment offset is pier by pier and doesn't need to be updated at the end of loading
         pObj->m_bUseTempAlignmentOffset = false;
      }
    
       if ( version < 3 )
       {
         var.vt = VT_I2;
         hr = pLoad->get_Property(_T("AlignmentMethod"),&var);
         if ( FAILED(hr) )
         {
            return hr;
         }
         Int16 method = var.iVal;

         var.vt = VT_R8;
         hr = pLoad->get_Property(_T("AlignmentDirection"),&var);
         if ( FAILED(hr) )
         {
            return hr;
         }
         Float64 Direction = var.dblVal;

         var.vt = VT_R8;
         hr = pLoad->get_Property(_T("FwdTangent"),&var);
         if ( FAILED(hr) )
         {
            return hr;
         }
         Float64 FwdTangent = var.dblVal;

         var.vt = VT_R8;
         hr = pLoad->get_Property(_T("BkTangent"),&var);
         if ( FAILED(hr) )
         {
            return hr;
         }
         Float64 BkTangent = var.dblVal;

         var.vt = VT_R8;
         hr = pLoad->get_Property(_T("Radius"),&var);
         if ( FAILED(hr) )
         {
            return hr;
         }
         Float64 Radius = var.dblVal;

         var.vt = VT_R8;
         hr = pLoad->get_Property(_T("PIStation"),&var);
         if ( FAILED(hr) )
         {
            return hr;
         }
         Float64 PIStation = var.dblVal;

         if ( !bConvert )
         {
            // Don't need to convert, so don't need to call EndUnit
            hr = pLoad->EndUnit();
            if ( FAILED(hr) )
            {
               return hr;
            }
         }

         if ( bConvert )
         {
            // Need to do a coordinate transformation for the directions
            // Before 1.1... 0.0 was due north and increased clockwise
            // 1.1 and later... 0.0 is due east and increases counter-clockwise

            Direction  = PI_OVER_2 - Direction;
            if ( Direction < 0 )
            {
               Direction += 2*M_PI;
            }

            FwdTangent = PI_OVER_2 - FwdTangent;
            if (FwdTangent < 0)
            {
               FwdTangent += 2*M_PI;
            }

            BkTangent  = PI_OVER_2 - BkTangent;
            if ( BkTangent < 0 )
            {
               BkTangent += 2*M_PI;
            }
         }

         // now, store the data in the current data structures
         if ( method == ALIGN_STRAIGHT )
         {
            pObj->m_AlignmentData2.Direction = Direction;
            pObj->m_AlignmentData2.HorzCurves.clear();
         }
         else
         {
            ATLASSERT( method == ALIGN_CURVE );
            pObj->m_AlignmentData2.Direction = BkTangent;
            
            HorzCurveData hc;
            hc.PIStation = PIStation;
            hc.Radius = Radius;
            hc.FwdTangent = FwdTangent;
            hc.EntrySpiral = 0;
            hc.ExitSpiral = 0;
            pObj->m_AlignmentData2.HorzCurves.push_back(hc);
         }
      }
      else
      {
         if ( 5 <= version )
         {
            var.vt = VT_R8;
            hr = pLoad->get_Property(_T("RefStation"),&var);
            if ( FAILED(hr) )
            {
               return hr;
            }
            pObj->m_AlignmentData2.RefStation = var.dblVal;

            var.vt = VT_R8;
            hr = pLoad->get_Property(_T("RefPointNorthing"),&var);
            if ( FAILED(hr) )
            {
               return hr;
            }
            pObj->m_AlignmentData2.yRefPoint = var.dblVal;

            var.vt = VT_R8;
            hr = pLoad->get_Property(_T("RefPointEasting"),&var);
            if ( FAILED(hr) )
            {
               return hr;
            }
            pObj->m_AlignmentData2.xRefPoint = var.dblVal;
         }

         var.vt = VT_R8;
         hr = pLoad->get_Property(_T("Direction"),&var);
         if ( FAILED(hr) )
         {
            return hr;
         }
         pObj->m_AlignmentData2.Direction = var.dblVal;

         var.vt = VT_I4;
         hr = pLoad->get_Property(_T("HorzCurveCount"),&var);
         if ( FAILED(hr) )
         {
            return hr;
         }
         int nCurves = var.iVal;

         for ( int c = 0; c < nCurves; c++ )
         {
            HorzCurveData hc;

            hr = pLoad->BeginUnit(_T("HorzCurveData"));
            if ( FAILED(hr) )
            {
               return hr;
            }

            Float64 hc_version;
            pLoad->get_Version(&hc_version);

            var.vt = VT_R8;
            hr = pLoad->get_Property(_T("PIStation"),&var);
            if ( FAILED(hr) )
            {
               return hr;
            }
            hc.PIStation = var.dblVal;

            var.vt = VT_R8;
            hr = pLoad->get_Property(_T("FwdTangent"),&var);
            if ( FAILED(hr) )
            {
               return hr;
            }
            hc.FwdTangent = var.dblVal;

            var.vt = VT_R8;
            hr = pLoad->get_Property(_T("Radius"),&var);
            if ( FAILED(hr) )
            {
               return hr;
            }
            hc.Radius = var.dblVal;

            var.vt = VT_R8;
            hr = pLoad->get_Property(_T("EntrySpiral"),&var);
            if ( FAILED(hr) )
            {
               return hr;
            }
            hc.EntrySpiral = var.dblVal;

            var.vt = VT_R8;
            hr = pLoad->get_Property(_T("ExitSpiral"),&var);
            if ( FAILED(hr) )
            {
               return hr;
            }
            hc.ExitSpiral = var.dblVal;

            if ( 2.0 <= hc_version )
            {
               var.vt = VT_BOOL;
               hr = pLoad->get_Property(_T("FwdTangentIsBearing"),&var);
               if ( FAILED(hr) )
               {
                  return hr;
               }
               hc.bFwdTangent = (var.boolVal == VARIANT_TRUE);
            }
            else
            {
               hc.bFwdTangent = true;
            }


            pObj->m_AlignmentData2.HorzCurves.push_back(hc);

            hr = pLoad->EndUnit();
            if ( FAILED(hr) )
            {
               return hr;
            }
         }

         hr = pLoad->EndUnit();
         if ( FAILED(hr) )
         {
            return hr;
         }
      }
   }

   return S_OK;
}

HRESULT CProjectAgentImp::ProfileProc(IStructuredSave* pSave,IStructuredLoad* pLoad,IProgress*,CProjectAgentImp* pObj)
{
   HRESULT hr = S_OK;

   if ( pSave )
   {
      hr = pSave->BeginUnit(_T("ProfileData"),1.0);
      if ( FAILED(hr) )
      {
         return hr;
      }

      hr = pSave->put_Property(_T("Station"),CComVariant(pObj->m_ProfileData2.Station));
      if ( FAILED(hr) )
      {
         return hr;
      }

      hr = pSave->put_Property(_T("Elevation"),CComVariant(pObj->m_ProfileData2.Elevation));
      if ( FAILED(hr) )
      {
         return hr;
      }

      hr = pSave->put_Property(_T("Grade"),CComVariant(pObj->m_ProfileData2.Grade));
      if ( FAILED(hr) )
      {
         return hr;
      }

      hr = pSave->put_Property(_T("VertCurveCount"),CComVariant((long)pObj->m_ProfileData2.VertCurves.size()));
      if ( FAILED(hr) )
      {
         return hr;
      }

      std::vector<VertCurveData>::iterator iter;
      for ( iter = pObj->m_ProfileData2.VertCurves.begin(); iter != pObj->m_ProfileData2.VertCurves.end(); iter++ )
      {
         VertCurveData& vc = *iter;

         hr = pSave->BeginUnit(_T("VertCurveData"),1.0);
         if ( FAILED(hr) )
         {
            return hr;
         }

         hr = pSave->put_Property(_T("PVIStation"),CComVariant(vc.PVIStation));
         if ( FAILED(hr) )
         {
            return hr;
         }

         hr = pSave->put_Property(_T("L1"),CComVariant(vc.L1));
         if ( FAILED(hr) )
         {
            return hr;
         }

         hr = pSave->put_Property(_T("L2"),CComVariant(vc.L2));
         if ( FAILED(hr) )
         {
            return hr;
         }

         hr = pSave->put_Property(_T("ExitGrade"),CComVariant(vc.ExitGrade));
         if ( FAILED(hr) )
         {
            return hr;
         }

         hr = pSave->EndUnit();
         if ( FAILED(hr) )
         {
            return hr;
         }
      }

      hr = pSave->EndUnit();
      if ( FAILED(hr) )
      {
         return hr;
      }
   }
   else
   {
      CComVariant var;

      bool bNewFormat = true;
      hr = pLoad->BeginUnit(_T("ProfileData"));
      if ( FAILED(hr) )
      {
         bNewFormat = false;
      }

      if ( !bNewFormat )
      {
         // read the old data into temporary variables
         var.vt = VT_I4;
         hr = pLoad->get_Property(_T("ProfileMethod"),&var);
         if ( FAILED(hr) )
         {
            return hr;
         }
         long method = var.lVal;

         var.vt = VT_R8;
         hr = pLoad->get_Property(_T("Station"),&var);
         if ( FAILED(hr) )
         {
            return hr;
         }
         Float64 Station = var.dblVal;

         var.vt = VT_R8;
         hr = pLoad->get_Property(_T("Elevation"),&var);
         if ( FAILED(hr) )
         {
            return hr;
         }
         Float64 Elevation = var.dblVal;

         var.vt = VT_R8;
         hr = pLoad->get_Property(_T("Grade"),&var);
         if ( FAILED(hr) )
         {
            return hr;
         }
         Float64 Grade = var.dblVal;

         var.vt = VT_R8;
         hr = pLoad->get_Property(_T("G1"),&var);
         if ( FAILED(hr) )
         {
            return hr;
         }
         Float64 G1 = var.dblVal;

         var.vt = VT_R8;
         hr = pLoad->get_Property(_T("G2"),&var);
         if ( FAILED(hr) )
         {
            return hr;
         }
         Float64 G2 = var.dblVal;

         var.vt = VT_R8;
         hr = pLoad->get_Property(_T("PVIStation"),&var);
         if ( FAILED(hr) )
         {
            return hr;
         }
         Float64 PVIStation = var.dblVal;

         var.vt = VT_R8;
         hr = pLoad->get_Property(_T("PVIElevation"),&var);
         if ( FAILED(hr) )
         {
            return hr;
         }
         Float64 PVIElevation = var.dblVal;

         var.vt = VT_R8;
         hr = pLoad->get_Property(_T("Length"),&var);
         if ( FAILED(hr) )
         {
            return hr;
         }
         Float64 Length = var.dblVal;

         // put into new data structures
         if ( method == PROFILE_STRAIGHT )
         {
            pObj->m_ProfileData2.Station = Station;
            pObj->m_ProfileData2.Elevation = Elevation;
            pObj->m_ProfileData2.Grade = Grade/100;
         }
         else
         {
            ATLASSERT( method == PROFILE_VCURVE );
            pObj->m_ProfileData2.Station = PVIStation;
            pObj->m_ProfileData2.Elevation = PVIElevation;
            pObj->m_ProfileData2.Grade = G1/100;

            VertCurveData vc;
            vc.PVIStation = PVIStation;
            vc.L1 = RoundOff(Length/2,0.001);
            vc.L2 = RoundOff(Length/2,0.001);
            vc.ExitGrade = G2/100;

            pObj->m_ProfileData2.VertCurves.push_back(vc);
         }
      }
      else
      {
         // reading new format
         var.vt = VT_R8;
         hr = pLoad->get_Property(_T("Station"),&var);
         if ( FAILED(hr) )
         {
            return hr;
         }
         pObj->m_ProfileData2.Station = var.dblVal;

         var.vt = VT_R8;
         hr = pLoad->get_Property(_T("Elevation"),&var);
         if ( FAILED(hr) )
         {
            return hr;
         }
         pObj->m_ProfileData2.Elevation = var.dblVal;

         var.vt = VT_R8;
         hr = pLoad->get_Property(_T("Grade"),&var);
         if ( FAILED(hr) )
         {
            return hr;
         }
         pObj->m_ProfileData2.Grade = var.dblVal;

         var.vt = VT_I4;
         hr = pLoad->get_Property(_T("VertCurveCount"),&var);
         if ( FAILED(hr) )
         {
            return hr;
         }
         long nCurves = var.lVal;

         for ( int v = 0; v < nCurves; v++ )
         {
            VertCurveData vc;

            hr = pLoad->BeginUnit(_T("VertCurveData"));
            if ( FAILED(hr) )
            {
               return hr;
            }

            var.vt = VT_R8;
            hr = pLoad->get_Property(_T("PVIStation"),&var);
            if ( FAILED(hr) )
            {
               return hr;
            }
            vc.PVIStation = var.dblVal;

            var.vt = VT_R8;
            hr = pLoad->get_Property(_T("L1"),&var);
            if ( FAILED(hr) )
            {
               return hr;
            }
            vc.L1 = var.dblVal;

            hr = pLoad->get_Property(_T("L2"),&var);
            if ( FAILED(hr) )
            {
               return hr;
            }
            vc.L2 = var.dblVal;

            hr = pLoad->get_Property(_T("ExitGrade"),&var);
            if ( FAILED(hr) )
            {
               return hr;
            }
            vc.ExitGrade = var.dblVal;

            pObj->m_ProfileData2.VertCurves.push_back(vc);

            hr = pLoad->EndUnit();
            if ( FAILED(hr) )
            {
               return hr;
            }
         }
      }

      if ( bNewFormat )
      {
         hr = pLoad->EndUnit();
         if ( FAILED(hr) )
         {
            return hr;
         }
      }
   }

   return S_OK;
}

HRESULT CProjectAgentImp::SuperelevationProc(IStructuredSave* pSave,IStructuredLoad* pLoad,IProgress* pProgress,CProjectAgentImp* pObj)
{
   HRESULT hr = S_OK;

   if ( pSave )
   {
      hr = pSave->BeginUnit(_T("SuperelevationData"),1.0);
      if ( FAILED(hr) )
      {
         return hr;
      }

      hr = pSave->put_Property(_T("SectionCount"),CComVariant((long)pObj->m_RoadwaySectionData.Superelevations.size()));
      if ( FAILED(hr) )
      {
         return hr;
      }

      std::vector<CrownData2>::iterator iter;
      for ( iter = pObj->m_RoadwaySectionData.Superelevations.begin(); iter != pObj->m_RoadwaySectionData.Superelevations.end(); iter++ )
      {
         CrownData2& super = *iter;

         hr = pSave->BeginUnit(_T("CrownSlope"),1.0);
         if ( FAILED(hr) )
         {
            return hr;
         }

         hr = pSave->put_Property(_T("Station"),CComVariant(super.Station));
         if ( FAILED(hr) )
         {
            return hr;
         }

         hr = pSave->put_Property(_T("Left"),CComVariant(super.Left));
         if ( FAILED(hr) )
         {
            return hr;
         }

         hr = pSave->put_Property(_T("Right"),CComVariant(super.Right));
         if ( FAILED(hr) )
         {
            return hr;
         }

         hr = pSave->put_Property(_T("CrownPointOffset"),CComVariant(super.CrownPointOffset));
         if ( FAILED(hr) )
         {
            return hr;
         }

         hr = pSave->EndUnit();
         if ( FAILED(hr) )
         {
            return hr;
         }
      }

      hr = pSave->EndUnit();
      if ( FAILED(hr) )
      {
         return hr;
      }
   }
   else
   {
      CComVariant var;

      bool bNewFormat = true;
      hr = pLoad->BeginUnit(_T("SuperelevationData"));
      if ( FAILED(hr) )
      {
         bNewFormat = false;
      }

      pObj->m_RoadwaySectionData.Superelevations.clear();

      if ( !bNewFormat )
      {
         var.vt = VT_R8;
         hr = pLoad->get_Property(_T("Left"),&var);
         if ( FAILED(hr) )
         {
            return hr;
         }
         Float64 left = var.dblVal;

         var.vt = VT_R8;
         hr = pLoad->get_Property(_T("Right"),&var);
         if ( FAILED(hr) )
         {
            return hr;
         }
         Float64 right = var.dblVal;

         CrownData2 crown;
         crown.Station = 0;
         crown.Left = left;
         crown.Right = right;
         crown.CrownPointOffset = 0;
         pObj->m_RoadwaySectionData.Superelevations.push_back(crown);
      }
      else
      {
         var.vt = VT_I4;
         hr = pLoad->get_Property(_T("SectionCount"),&var);
         if ( FAILED(hr) )
         {
            return hr;
         }
         long nSections = var.lVal;

         for ( long s = 0; s < nSections; s++ )
         {
            CrownData2 super;

            hr = pLoad->BeginUnit(_T("CrownSlope"));
            if ( FAILED(hr) )
            {
               return hr;
            }

            var.vt = VT_R8;
            hr = pLoad->get_Property(_T("Station"),&var);
            if ( FAILED(hr) )
            {
               return hr;
            }
            super.Station = var.dblVal;

            var.vt = VT_R8;
            hr = pLoad->get_Property(_T("Left"),&var);
            if ( FAILED(hr) )
            {
               return hr;
            }
            super.Left = var.dblVal;

            var.vt = VT_R8;
            hr = pLoad->get_Property(_T("Right"),&var);
            if ( FAILED(hr) )
            {
               return hr;
            }
            super.Right = var.dblVal;

            var.vt = VT_R8;
            hr = pLoad->get_Property(_T("CrownPointOffset"),&var);
            if ( FAILED(hr) )
            {
               return hr;
            }
            super.CrownPointOffset = var.dblVal;

            pObj->m_RoadwaySectionData.Superelevations.push_back(super);

            hr = pLoad->EndUnit();
            if ( FAILED(hr) )
            {
               return hr;
            }
         }
      }

      if ( bNewFormat )
      {
         hr = pLoad->EndUnit();
         if ( FAILED(hr) ) 
         {
            return hr;
         }
      }
   }

   return S_OK;
}

HRESULT CProjectAgentImp::PierDataProc(IStructuredSave* pSave,IStructuredLoad* pLoad,IProgress* pProgress,CProjectAgentImp* pObj)
{
   HRESULT hr = S_OK;
   if ( pSave )
   {
      return S_OK; // not saving in this old format
   }
   else
   {
      Float64 version;
      pLoad->get_Version(&version);
      if ( version <= 1.0 ) // if ProjectData version 1 or less, load the old format, otherwise do nothing
      {
         pLoad->BeginUnit(_T("Piers"));
         hr = CProjectAgentImp::PierDataProc2(pSave,pLoad,pProgress,pObj);
         pLoad->EndUnit();
      }
   }

   return hr;
}

HRESULT CProjectAgentImp::PierDataProc2(IStructuredSave* pSave,IStructuredLoad* pLoad,IProgress* pProgress,CProjectAgentImp* pObj)
{
   HRESULT hr = S_OK;
   if ( pSave )
   {
      ATLASSERT(false); // should never get here
   }
   else
   {
      Float64 version;
      pLoad->get_Version(&version);

      bool bInputSymmetricalPierConnections = true;

      CComVariant var;
      if ( 2.0 == version )
      {
         // this parameter is only valid for version 2 of this data block
         var.vt = VT_BOOL;
         hr = pLoad->get_Property(_T("InputSymmetricalConnections"),&var);
         bInputSymmetricalPierConnections = (var.boolVal == VARIANT_TRUE ? true : false);
      }
      else
      {
         bInputSymmetricalPierConnections = true;
      }

      var.vt = VT_I4;
      hr = pLoad->get_Property(_T("PierCount"), &var );
      if ( FAILED(hr) )
      {
         return hr;
      }
      
      PierIndexType nPiers = var.lVal;

      CPierData2 firstPier;
      for ( PierIndexType i = 0; i < nPiers; i++ )
      {
         CPierData2 pd;
         if ( FAILED(pd.Load(pLoad,pProgress)) )
         {
            return STRLOAD_E_INVALIDFORMAT;
         }
         
         pd.SetIndex(i);


         if ( i == 0 )
         {
            firstPier = pd;
         }
         else if ( i == 1 )
         {
            CSpanData2 span;
            span.SetIndex(i-1);
            pObj->m_BridgeDescription.CreateFirstSpan(&firstPier,&span,&pd,INVALID_INDEX);
         }
         else
         {
            CSpanData2 span;
            span.SetIndex(i-1);
            pObj->m_BridgeDescription.AppendSpan(&span,&pd,true,INVALID_INDEX);
         }
      }
      
      // make sure that if the first and last pier are marked as continuous, change them to integral
      // continuous is not a valid input for the first and last pier, but bugs in the preview releases
      // made it possible to have this input
      CPierData2* pFirstPier = pObj->m_BridgeDescription.GetPier(0);
      if ( pFirstPier->GetBoundaryConditionType() == pgsTypes::bctContinuousAfterDeck )
      {
         pFirstPier->SetBoundaryConditionType(pgsTypes::bctIntegralAfterDeck );
      }
      else if ( pFirstPier->GetBoundaryConditionType() == pgsTypes::bctContinuousBeforeDeck )
      {
         pFirstPier->SetBoundaryConditionType( pgsTypes::bctIntegralBeforeDeck );
      }


      CPierData2* pLastPier = pObj->m_BridgeDescription.GetPier( pObj->m_BridgeDescription.GetPierCount()-1 );
      if ( pLastPier->GetBoundaryConditionType() == pgsTypes::bctContinuousAfterDeck )
      {
         pLastPier->SetBoundaryConditionType( pgsTypes::bctIntegralAfterDeck );
      }
      else if ( pLastPier->GetBoundaryConditionType() == pgsTypes::bctContinuousBeforeDeck )
      {
         pLastPier->SetBoundaryConditionType( pgsTypes::bctIntegralBeforeDeck );
      }
   }

   return S_OK;
}

HRESULT CProjectAgentImp::XSectionDataProc(IStructuredSave* pSave,IStructuredLoad* pLoad,IProgress* pProgress,CProjectAgentImp* pObj)
{
   HRESULT hr = S_OK;
   if ( pSave )
   {
      return S_OK; // not saving in this old format
   }
   else
   {
      Float64 version;
      pLoad->get_Version(&version);
      if ( version <= 1.0 ) // if ProjectData version 1 or less, load the old format, otherwise do nothing
      {
         pLoad->BeginUnit(_T("CrossSection"));
         hr = CProjectAgentImp::XSectionDataProc2(pSave,pLoad,pProgress,pObj);
         pLoad->EndUnit();
      }
   }

   return hr;
}

HRESULT CProjectAgentImp::XSectionDataProc2(IStructuredSave* pSave,IStructuredLoad* pLoad,IProgress* pProgress,CProjectAgentImp* pObj)
{
   if ( pSave )
   {
      ATLASSERT(false); // should never get here
      return E_FAIL;
   }
   else
   {
      CXSectionData xSectionData;
      HRESULT hr = xSectionData.Load( pLoad, pProgress, pObj );
      if ( FAILED(hr) )
      {
         return hr;
      }

      pObj->m_strOldGirderConcreteName = xSectionData.m_strGirderConcreteName;

      pObj->m_BridgeDescription.UseSameNumberOfGirdersInAllGroups(true);
      pObj->m_BridgeDescription.SetGirderCount(xSectionData.GdrLineCount);

      pObj->m_BridgeDescription.UseSameGirderForEntireBridge(true);
      pObj->m_BridgeDescription.SetGirderName(xSectionData.Girder.c_str());

      const GirderLibraryEntry* pGdrEntry = pObj->GetGirderEntry(xSectionData.Girder.c_str());
      CComPtr<IBeamFactory> factory;
      pGdrEntry->GetBeamFactory(&factory);
      pObj->m_BridgeDescription.SetGirderFamilyName( factory->GetGirderFamilyName().c_str() );

      if (factory->IsSupportedBeamSpacing(pgsTypes::sbsUniform) && xSectionData.DeckType != pgsTypes::sdtNone)
      {
         pObj->m_BridgeDescription.SetGirderSpacingType(pgsTypes::sbsUniform);
         pObj->m_BridgeDescription.SetGirderSpacing(xSectionData.GdrSpacing);
         pObj->m_BridgeDescription.SetGirderTopWidth(pgsTypes::twtSymmetric,0.0,0.0);
      }
      else
      {
         Float64 spacing, topWidth;
         pgsTypes::SupportedBeamSpacing spacingType;
         VERIFY(factory->ConvertBeamSpacing(pGdrEntry->GetDimensions(), pgsTypes::sbsUniform, xSectionData.GdrSpacing, &spacingType, &spacing, &topWidth));
         pObj->m_BridgeDescription.SetGirderSpacingType(spacingType);
         pObj->m_BridgeDescription.SetGirderSpacing(spacing);
         pObj->m_BridgeDescription.SetGirderTopWidth(pgsTypes::twtSymmetric,topWidth,0.0);
      }

      pObj->m_BridgeDescription.SetMeasurementType(xSectionData.GdrSpacingMeasurement == CXSectionData::Normal ? pgsTypes::NormalToItem : pgsTypes::AlongItem);
      pObj->m_BridgeDescription.SetMeasurementLocation(pgsTypes::AtPierLine);

      if (factory->IsSupportedGirderOrientation(xSectionData.GirderOrientation))
      {
         pObj->m_BridgeDescription.SetGirderOrientation((pgsTypes::GirderOrientationType)xSectionData.GirderOrientation);
      }
      else
      {
         pgsTypes::GirderOrientationType newOrientation = factory->ConvertGirderOrientation(xSectionData.GirderOrientation);
         pObj->m_BridgeDescription.SetGirderOrientation(newOrientation);
      }

      pObj->m_BridgeDescription.SetFillet(xSectionData.Fillet);

      // update the deck data
      CDeckDescription2* pDeck      = pObj->m_BridgeDescription.GetDeckDescription();
      pDeck->DeckRebarData          = xSectionData.DeckRebarData;
      pDeck->SetDeckType( xSectionData.DeckType );
      pDeck->GrossDepth             = xSectionData.GrossDepth;
      pDeck->OverhangEdgeDepth      = xSectionData.OverhangEdgeDepth;
      pDeck->OverhangTaper          = xSectionData.OverhangTaper;
      pDeck->OverlayWeight          = xSectionData.OverlayWeight;
      pDeck->OverlayDensity         = xSectionData.OverlayDensity;
      pDeck->OverlayDepth           = xSectionData.OverlayDepth;
      pDeck->PanelDepth             = xSectionData.PanelDepth;
      pDeck->PanelSupport           = xSectionData.PanelSupport;
      pDeck->SacrificialDepth       = xSectionData.SacrificialDepth;
      pDeck->Concrete.Ec            = xSectionData.SlabEc;
      pDeck->Concrete.Fc            = xSectionData.SlabFc;
      pDeck->Concrete.EcK1          = xSectionData.SlabEcK1;
      pDeck->Concrete.EcK2          = xSectionData.SlabEcK2;
      pDeck->Concrete.CreepK1            = xSectionData.SlabCreepK1;
      pDeck->Concrete.CreepK2            = xSectionData.SlabCreepK2;
      pDeck->Concrete.ShrinkageK1        = xSectionData.SlabShrinkageK1;
      pDeck->Concrete.ShrinkageK2        = xSectionData.SlabShrinkageK2;
      pDeck->Concrete.MaxAggregateSize   = xSectionData.SlabMaxAggregateSize;
      pDeck->Concrete.StrengthDensity    = xSectionData.SlabStrengthDensity;
      pDeck->Concrete.bUserEc            = xSectionData.SlabUserEc;
      pDeck->Concrete.WeightDensity      = xSectionData.SlabWeightDensity;
      pDeck->TransverseConnectivity = xSectionData.TransverseConnectivity;
      pDeck->WearingSurface         = xSectionData.WearingSurface;

      pObj->m_BridgeDescription.SetSlabOffset( xSectionData.SlabOffset );
      pObj->m_BridgeDescription.SetSlabOffsetType( pgsTypes::sotBridge );

      Float64 spacing_width = (xSectionData.GdrLineCount-1)*(xSectionData.GdrSpacing);
      ATLASSERT( xSectionData.GdrSpacingMeasurement == CXSectionData::Normal );
      CDeckPoint point;
      point.LeftEdge  = spacing_width/2 + xSectionData.LeftOverhang;
      point.RightEdge = spacing_width/2 + xSectionData.RightOverhang;
      point.Station = pObj->m_BridgeDescription.GetPier(0)->GetStation();
      point.MeasurementType     = pgsTypes::omtBridge;
      point.LeftTransitionType  = pgsTypes::dptLinear;
      point.RightTransitionType = pgsTypes::dptLinear;
      pDeck->DeckEdgePoints.push_back(point);

      // update the left railing
      CRailingSystem* pLeftRailingSystem = pObj->m_BridgeDescription.GetLeftRailingSystem();
      pLeftRailingSystem->strExteriorRailing = xSectionData.LeftTrafficBarrier;
      pLeftRailingSystem->Concrete = pDeck->Concrete;

      // update the right railing
      CRailingSystem* pRightRailingSystem = pObj->m_BridgeDescription.GetRightRailingSystem();
      pRightRailingSystem->strExteriorRailing = xSectionData.RightTrafficBarrier;
      pRightRailingSystem->Concrete           = pDeck->Concrete;

      pObj->m_BridgeDescription.CopyDown(true,true,true,true,true,true);
   }

   return S_OK;
}

HRESULT CProjectAgentImp::BridgeDescriptionProc(IStructuredSave* pSave,IStructuredLoad* pLoad,IProgress* pProgress,CProjectAgentImp* pObj)
{
   if ( pSave )
   {
      HRESULT hr = pObj->m_BridgeDescription.Save( pSave, pProgress );
      if ( FAILED(hr) )
      {
         return hr;
      }
   }
   else
   {
      Float64 version;
      pLoad->get_Version(&version);
      if ( version <= 1.0 ) // if ProjectData version 1 or less, load the old format, otherwise do nothing
      {
         return S_OK;
      }

      HRESULT hr = pObj->m_BridgeDescription.Load( pLoad, pProgress );
      if ( FAILED(hr) )
      {
         return hr;
      }

      if ( hr == LOADED_OLD_BRIDGE_TYPE )
      {
         // If the old bridge type was loaded, updated the segment heights to match that of the girder
         // This is the best place to do it because we know both the library and the bridge
         // (NOTE that pGirder->GetGirderLibraryEntry will return nullptr at this point because we haven't linked the two together)
         GroupIndexType nGroups = pObj->m_BridgeDescription.GetGirderGroupCount();
         for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
         {
            CGirderGroupData* pGroup = pObj->m_BridgeDescription.GetGirderGroup(grpIdx);
            GirderIndexType nGirders = pGroup->GetGirderCount();
            for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
            {
               CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);
               const GirderLibraryEntry* pGdrLibEntry = pObj->m_pLibMgr->GetGirderLibrary().LookupEntry( pGirder->GetGirderName() );
               SegmentIndexType nSegments = pGirder->GetSegmentCount();
               for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
               {
                  CPrecastSegmentData* pSegment = pGirder->GetSegment(segIdx);

                  Float64 HgStart = pGdrLibEntry->GetBeamHeight(pgsTypes::metStart);
                  Float64 HgEnd   = pGdrLibEntry->GetBeamHeight(pgsTypes::metEnd);
                  pSegment->SetVariationParameters(pgsTypes::sztLeftPrismatic, 0.0,HgStart,0.0);
                  pSegment->SetVariationParameters(pgsTypes::sztRightPrismatic,0.0,HgEnd,  0.0);
               }
               pGdrLibEntry->Release();
            }
         }
      }
   }

   return S_OK;
}

HRESULT CProjectAgentImp::PrestressingDataProc(IStructuredSave* pSave,IStructuredLoad* pLoad,IProgress* pProgress,CProjectAgentImp* pObj)
{
   HRESULT hr = S_OK;
   if ( pSave )
   {
      return S_OK; // not saving in this old format
   }
   else
   {
      // reading old format
      Float64 version;
      pLoad->get_Version(&version);
      if ( version <= 1.0 ) // if ProjectData version 1 or less, load the old format, otherwise do nothing
      {
         hr = pLoad->BeginUnit(_T("Prestressing"));
         hr = CProjectAgentImp::PrestressingDataProc2(pSave,pLoad,pProgress,pObj);
         pLoad->EndUnit();
      }
   }
   return S_OK;
}

HRESULT CProjectAgentImp::PrestressingDataProc2(IStructuredSave* pSave,IStructuredLoad* pLoad,IProgress* pProgress,CProjectAgentImp* pObj)
{
   if ( pSave )
   {
      ATLASSERT(false); // should never get here
      return E_FAIL;
   }
   else
   {
      // read the girder data in the old format at store in girderData
      std::map<SpanGirderHashType,CGirderData> girderDataMap;

      CComVariant var;
      var.vt = VT_I4;

      HRESULT hr;

      Float64 version;
      pLoad->get_Version(&version);

      // removed at version 3 of data block
      const matPsStrand* pStrandMaterial = 0;
      if ( version < 3 )
      {
         hr = pLoad->get_Property(_T("PrestressStrandKey"), &var );
         if ( FAILED(hr) )
         {
            return hr;
         }

         Int32 key = var.lVal;
         key |= matPsStrand::None;

         lrfdStrandPool* pPool = lrfdStrandPool::GetInstance();
         pStrandMaterial = pPool->GetStrand( key );
         ATLASSERT(pStrandMaterial!=0);
      }


      // A bug released in 2.07 can allow files to be saved with more girder data
      // than spans exist. we need to trap this
      SpanIndexType nspans = pObj->m_BridgeDescription.GetSpanCount();

      hr = pLoad->get_Property(_T("PrestressDataCount"), &var );
      if ( FAILED(hr) )
      {
         return hr;
      }

      int cnt = var.lVal;

      const ConcreteLibraryEntry* pConcreteLibraryEntry = pObj->GetConcreteEntry(pObj->m_strOldGirderConcreteName.c_str());
      for ( int i = 0; i < cnt; i++ )
      {
         HRESULT hr = pLoad->BeginUnit(_T("GirderPrestressData"));
         if ( FAILED(hr) )
         {
            return hr;
         }

         var.vt = VT_I4;
         hr = pLoad->get_Property(_T("Span"), &var );
         if ( FAILED(hr) )
         {
            return hr;
         }
         
         SpanIndexType span = var.iVal;

         hr = pLoad->get_Property(_T("Girder"), &var );
         if ( FAILED(hr) )
         {
            return hr;
         }

         GirderIndexType girder = var.iVal;
         SpanGirderHashType hashval = HashSpanGirder(span, girder);

         CGirderData girderData;
         if ( pConcreteLibraryEntry == nullptr )
         {
            hr = girderData.Load(pLoad,pProgress);
         }
         else
         {
            hr = girderData.Load( pLoad, pProgress, pConcreteLibraryEntry->GetFc(),
                                                     pConcreteLibraryEntry->GetWeightDensity(),
                                                     pConcreteLibraryEntry->GetStrengthDensity(),
                                                     pConcreteLibraryEntry->GetAggregateSize());
         }

         if ( FAILED(hr) )
         {
            return hr;
         }

         hr = pLoad->EndUnit();
         if ( FAILED(hr) )
         {
            return hr;
         }

         // pStrandMaterial will not be nullptr if this is a pre version 3 data block
         // in this case, the girderData object does not have a strand material set.
         // Set it now
         if ( pStrandMaterial != 0 )
         {
            ATLASSERT(girderData.Strands.GetStrandMaterial(pgsTypes::Straight)  == 0);
            ATLASSERT(girderData.Strands.GetStrandMaterial(pgsTypes::Harped)    == 0);
            ATLASSERT(girderData.Strands.GetStrandMaterial(pgsTypes::Temporary) == 0);
            girderData.Strands.SetStrandMaterial(pgsTypes::Straight,  pStrandMaterial);
            girderData.Strands.SetStrandMaterial(pgsTypes::Harped,    pStrandMaterial);
            girderData.Strands.SetStrandMaterial(pgsTypes::Temporary, pStrandMaterial);
         }

         if (span<nspans)
         {
            girderDataMap.insert(std::make_pair(hashval, girderData));
         }
         else
         {
            ATLASSERT(false); // file has data for span that doesn't exist
         }
      }

      // put girder data into the bridge description
      CTimelineManager* pTimelineMgr = pObj->m_BridgeDescription.GetTimelineManager();
      const CTimelineEvent* pCastingYardEvent;
      const CTimelineEvent* pGirderPlacementEvent;
      EventIndexType cyEventIdx;
      EventIndexType gpEventIdx;
      VERIFY(pTimelineMgr->FindEvent(_T("Construct Girders, Erect Piers"),&cyEventIdx,&pCastingYardEvent));
      VERIFY(pTimelineMgr->FindEvent(_T("Erect Girders"),&gpEventIdx,&pGirderPlacementEvent));

      std::map<SpanGirderHashType,CGirderData>::iterator iter;
      for ( iter = girderDataMap.begin(); iter != girderDataMap.end(); iter++ )
      {
         SpanGirderHashType hash = (*iter).first;
         CGirderData gdrData = (*iter).second;

         SpanIndexType spanIdx;
         GirderIndexType gdrIdx;
         UnhashSpanGirder(hash,&spanIdx,&gdrIdx);

         CGirderGroupData* pGirderGroup = pObj->m_BridgeDescription.GetGirderGroup(spanIdx);
         CSplicedGirderData* pGirder   = pGirderGroup->GetGirder(gdrIdx);
         CPrecastSegmentData* pSegment = pGirder->GetSegment(0);

         pSegment->HandlingData          = gdrData.HandlingData;
         pSegment->LongitudinalRebarData = gdrData.LongitudinalRebarData;
         pSegment->Material              = gdrData.Material;

         if ( gdrData.m_bUsedShearData2 )
         {
            pSegment->ShearData = gdrData.ShearData2;
         }
         else
         {
            pSegment->ShearData = gdrData.ShearData.Convert();
         }

         pSegment->Strands               = gdrData.Strands;

         pGirder->SetConditionFactor(gdrData.ConditionFactor);
         pGirder->SetConditionFactorType(gdrData.Condition);
         pGirder->SetGirderName(gdrData.m_GirderName.c_str());
         pGirder->SetGirderLibraryEntry(gdrData.m_pLibraryEntry);

         SegmentIDType segID = pSegment->GetID();
         pTimelineMgr->SetSegmentErectionEventByIndex(segID,gpEventIdx);
      }
   }

   return S_OK;
}

HRESULT CProjectAgentImp::ShearDataProc(IStructuredSave* pSave,IStructuredLoad* pLoad,IProgress* pProgress,CProjectAgentImp* pObj)
{
   HRESULT hr = S_OK;
   if ( pSave )
   {
      return S_OK; // not saving in this old format
   }
   else
   {
      // reading old format
      Float64 version;
      pLoad->get_Version(&version);
      if ( version <= 1.0 ) // if ProjectData version 1 or less, load the old format, otherwise do nothing
      {
         hr = pLoad->BeginUnit(_T("Shear"));
         hr = CProjectAgentImp::ShearDataProc2(pSave,pLoad,pProgress,pObj);
         pLoad->EndUnit();
      }
   }
   return S_OK;
}

HRESULT CProjectAgentImp::ShearDataProc2(IStructuredSave* pSave,IStructuredLoad* pLoad,IProgress* pProgress,CProjectAgentImp* pObj)
{
   if ( pSave )
   {
      ATLASSERT(false); // should never get here
      return E_FAIL;
   }
   else
   {
      std::map<SpanGirderHashType, CShearData> shearData;

      CComVariant var;

      Float64 version;
      pLoad->get_Version(&version);

      if (version < 3)
      {
         // version 3 and later... this data member is gone
         // just load it and pretend it doesn't matter
         var.vt = VT_BSTR;

         HRESULT hr = pLoad->get_Property(_T("StirrupMaterial"), &var );
         if ( FAILED(hr) )
         {
            return hr;
         }

         ::SysFreeString( var.bstrVal );
      }

      // A bug released in 2.07 can allow files to be saved with more girder data
      // than spans exist. we need to trap this
      SpanIndexType nspans = pObj->m_BridgeDescription.GetSpanCount();

      var.vt = VT_I4;
      HRESULT hr = pLoad->get_Property(_T("ShearDataCount"), &var );
      if ( FAILED(hr) )
      {
         return hr;
      }

      int cnt = var.lVal;

      for ( int i = 0; i < cnt; i++ )
      {
         HRESULT hr = pLoad->BeginUnit(_T("GirderShearData"));
         if ( FAILED(hr) )
         {
            return hr;
         }

         var.vt = VT_I4;
         hr = pLoad->get_Property(_T("Span"), &var );
         if ( FAILED(hr) )
         {
            return hr;
         }

         SpanIndexType span = var.iVal;

         hr = pLoad->get_Property(_T("Girder"), &var );
         if ( FAILED(hr) )
         {
            return hr;
         }

         GirderIndexType girder = var.iVal;

         SpanGirderHashType hashval = HashSpanGirder(span, girder);

         CShearData pd;
         CStructuredLoad load( pLoad );
         hr = pd.Load( &load );
         if ( FAILED(hr) )
         {
            return hr;
         }

         if (span < nspans)
         {
            shearData.insert( std::make_pair(hashval, pd) );
         }

         hr = pLoad->EndUnit();// GirderShearData
         if ( FAILED(hr) )
         {
            return hr;
         }
      }

      // put shear data into the bridge description
      std::map<SpanGirderHashType,CShearData>::iterator iter;
      for ( iter = shearData.begin(); iter != shearData.end(); iter++ )
      {
         SpanGirderHashType hash = (*iter).first;
         CShearData shear = (*iter).second;

         SpanIndexType spanIdx;
         GirderIndexType gdrIdx;
         UnhashSpanGirder(hash,&spanIdx,&gdrIdx);

         CGirderGroupData* pGirderGroup = pObj->m_BridgeDescription.GetGirderGroup(spanIdx);
         CSplicedGirderData* pGirder   = pGirderGroup->GetGirder(gdrIdx);
         CPrecastSegmentData* pSegment = pGirder->GetSegment(0);
         pSegment->ShearData = shear.Convert();
      }
   }

   return S_OK;
}

HRESULT CProjectAgentImp::LongitudinalRebarDataProc(IStructuredSave* pSave,IStructuredLoad* pLoad,IProgress* pProgress,CProjectAgentImp* pObj)
{
   HRESULT hr = S_OK;
   if ( pSave )
   {
      return S_OK; // not saving in this old format
   }
   else
   {
      // reading old format
      Float64 version;
      pLoad->get_Version(&version);
      if ( version <= 1.0 ) // if ProjectData version 1 or less, load the old format, otherwise do nothing
      {
         hr = CProjectAgentImp::LongitudinalRebarDataProc2(pSave,pLoad,pProgress,pObj);
      }
   }
   return S_OK;
}

HRESULT CProjectAgentImp::LongitudinalRebarDataProc2(IStructuredSave* pSave,IStructuredLoad* pLoad,IProgress* pProgress,CProjectAgentImp* pObj)
{
   if ( pSave )
   {
      ATLASSERT(false); // should never get here
      return E_FAIL;
   }
   else
   {
      Float64 top_version;
      pLoad->get_TopVersion(&top_version);

      // file version is before we started saving this data... set up some default data and get the heck outta here
      if ( top_version < 1.0 )
      {
         // set values from the library entry
         SpanIndexType nSpans;
         nSpans = pObj->m_BridgeDescription.GetSpanCount();

         for ( SpanIndexType span = 0; span < nSpans; span++ )
         {
            CGirderGroupData* pGroup = pObj->m_BridgeDescription.GetGirderGroup(span);
            GirderIndexType nGirders = pGroup->GetGirderCount();

            for ( GirderIndexType gdr = 0; gdr < nGirders; gdr++ )
            {
               // we have to get the girder library entry this way instead of from the bridge
               // description because the library references haven't been set up yet.
               CSplicedGirderData* pGirder = pGroup->GetGirder(gdr);
               LPCTSTR strGirderName = pGirder->GetGirderName();
               const GirderLibraryEntry* pGirderEntry = pObj->GetGirderEntry( strGirderName );

               CLongitudinalRebarData lrd;
               lrd.CopyGirderEntryData(pGirderEntry);

               CPrecastSegmentData* pSegment = pGirder->GetSegment(0);
               pSegment->LongitudinalRebarData = lrd;
            }
         }
         return S_OK;
      }
      else
      {
         CComVariant var;
         var.vt = VT_BSTR;

         HRESULT hr = pLoad->BeginUnit(_T("LongitudinalRebarData"));
         if ( FAILED(hr) )
         {
            return hr;
         }

         Float64 version;
         pLoad->get_Version(&version);
         if ( version < 2 )
         {
            // version 2 and later... this data member is gone
            // just load it and pretend it doesn't matter
            hr = pLoad->get_Property(_T("LongitudinalRebarMaterial"), &var );
            if ( FAILED(hr) )
            {
               return hr;
            }

            ::SysFreeString( var.bstrVal );
         }


         // A bug released in 2.07 can allow files to be saved with more girder data
         // than spans exist. we need to trap this
         SpanIndexType nspans = pObj->m_BridgeDescription.GetSpanCount();

         var.vt = VT_I4;
         hr = pLoad->get_Property(_T("LongitudinalRebarDataCount"), &var );
         if ( FAILED(hr) )
         {
            return hr;
         }

         int cnt = var.lVal;

         for ( int i = 0; i < cnt; i++ )
         {
            HRESULT hr = pLoad->BeginUnit(_T("GirderLongitudinalRebarData"));
            if ( FAILED(hr) )
            {
               return hr;
            }

            var.vt = VT_I4;
            hr = pLoad->get_Property(_T("Span"), &var );
            if ( FAILED(hr) )
            {
               return hr;
            }

            SpanIndexType span = var.iVal;

            hr = pLoad->get_Property(_T("Girder"), &var );
            if ( FAILED(hr) )
            {
               return hr;
            }
            
            GirderIndexType girder = var.iVal;

            CLongitudinalRebarData lrd;
            hr = lrd.Load( pLoad, pProgress );
            if ( FAILED(hr) )
            {
               return hr;
            }

            hr = pLoad->EndUnit();// GirderLongitudinalRebarData
            if ( FAILED(hr) )
            {
               return hr;
            }

            if (span < nspans)
            {
               CGirderGroupData* pGroup = pObj->m_BridgeDescription.GetGirderGroup(span);
               CSplicedGirderData* pGirder = pGroup->GetGirder(girder);
               CPrecastSegmentData* pSegment = pGirder->GetSegment(0);
               pSegment->LongitudinalRebarData = lrd;
            }
         }

         hr = pLoad->EndUnit();// LongitudinalRebarData
         if ( FAILED(hr) )
         {
            return hr;
         }
      }
   }

   return S_OK;
}

HRESULT CProjectAgentImp::LoadFactorsProc(IStructuredSave* pSave,IStructuredLoad* pLoad,IProgress* pProgress,CProjectAgentImp* pObj)
{
   HRESULT hr = S_OK;
   if ( pSave )
   {
      return pObj->m_LoadFactors.Save(pSave,pProgress);
   }
   else
   {
      Float64 version;
      pLoad->get_Version(&version);
      if ( 3 < version )
      {
         // added in version 4
         return pObj->m_LoadFactors.Load(pLoad,pProgress);
      }
   }

   return S_OK;
}

HRESULT CProjectAgentImp::LossesProc(IStructuredSave* pSave,IStructuredLoad* pLoad,IProgress* pProgress,CProjectAgentImp* pObj)
{
   if ( pSave )
   {
      pSave->put_Property(_T("UseLumpSumLosses"),CComVariant(pObj->m_bGeneralLumpSum));
      if ( pObj->m_bGeneralLumpSum )
      {
         pSave->BeginUnit(_T("LumpSumLosses"),1.0);
         pSave->put_Property(_T("BeforeXferLosses"),              CComVariant(pObj->m_BeforeXferLosses));
         pSave->put_Property(_T("AfterXferLosses"),               CComVariant(pObj->m_AfterXferLosses));
         pSave->put_Property(_T("LiftingLosses"),                 CComVariant(pObj->m_LiftingLosses));
         pSave->put_Property(_T("ShippingLosses"),                CComVariant(pObj->m_ShippingLosses));
         pSave->put_Property(_T("BeforeTempStrandRemovalLosses"), CComVariant(pObj->m_BeforeTempStrandRemovalLosses));
         pSave->put_Property(_T("AfterTempStrandRemovalLosses"),  CComVariant(pObj->m_AfterTempStrandRemovalLosses));
         pSave->put_Property(_T("AfterDeckPlacementLosses"),      CComVariant(pObj->m_AfterDeckPlacementLosses));
         pSave->put_Property(_T("AfterSIDLLosses"),               CComVariant(pObj->m_AfterSIDLLosses));
         pSave->put_Property(_T("FinalLosses"),                   CComVariant(pObj->m_FinalLosses));
         pSave->EndUnit(); // LumSumLosses
      }

      // added in version 6 of ProjectData data block (removed in version 8)
      //pSave->put_Property(_T("IgnoreTimeDependentEffects"),CComVariant(pObj->m_bIgnoreTimeDependentEffects));
      // added in version 8 of ProjectData data block
      pSave->put_Property(_T("IgnoreCreepEffects"),CComVariant(pObj->m_bIgnoreCreepEffects));
      pSave->put_Property(_T("IgnoreShrinkageEffects"),CComVariant(pObj->m_bIgnoreShrinkageEffects));
      pSave->put_Property(_T("IgnoreRelaxationEffects"),CComVariant(pObj->m_bIgnoreRelaxationEffects));

      pSave->BeginUnit(_T("PostTensioning"),1.0);
      pSave->put_Property(_T("AnchorSet"),          CComVariant(pObj->m_Dset_PT));
      pSave->put_Property(_T("WobbleFriction"),     CComVariant(pObj->m_WobbleFriction_PT));
      pSave->put_Property(_T("FrictionCoefficient"),CComVariant(pObj->m_FrictionCoefficient_PT));
      pSave->EndUnit(); // PostTensioning

      pSave->BeginUnit(_T("TemporaryStrandPT"),1.0);
      pSave->put_Property(_T("AnchorSet"),          CComVariant(pObj->m_Dset_TTS));
      pSave->put_Property(_T("WobbleFriction"),     CComVariant(pObj->m_WobbleFriction_TTS));
      pSave->put_Property(_T("FrictionCoefficient"),CComVariant(pObj->m_FrictionCoefficient_TTS));
      pSave->EndUnit(); // TemporaryStrandPT
   }
   else
   {
      CHRException hr;
      try
      {
         Float64 version;
         pLoad->get_Version(&version);
         if ( 4 < version )
         {
            // added in version 5
            CComVariant var;

            var.vt = VT_BOOL;
            hr = pLoad->get_Property(_T("UseLumpSumLosses"),&var);
            pObj->m_bGeneralLumpSum = (var.boolVal == VARIANT_TRUE ? true : false);
            if ( pObj->m_bGeneralLumpSum )
            {
               hr = pLoad->BeginUnit(_T("LumpSumLosses"));

               var.vt = VT_R8;
               hr = pLoad->get_Property(_T("BeforeXferLosses"),&var);
               pObj->m_BeforeXferLosses = var.dblVal;

               hr = pLoad->get_Property(_T("AfterXferLosses"),&var);
               pObj->m_AfterXferLosses = var.dblVal;

               hr = pLoad->get_Property(_T("LiftingLosses"),&var);
               pObj->m_LiftingLosses = var.dblVal;

               hr = pLoad->get_Property(_T("ShippingLosses"),&var);
               pObj->m_ShippingLosses = var.dblVal;

               hr = pLoad->get_Property(_T("BeforeTempStrandRemovalLosses"),&var);
               pObj->m_BeforeTempStrandRemovalLosses = var.dblVal;

               hr = pLoad->get_Property(_T("AfterTempStrandRemovalLosses"),&var);
               pObj->m_AfterTempStrandRemovalLosses = var.dblVal;

               hr = pLoad->get_Property(_T("AfterDeckPlacementLosses"),&var);
               pObj->m_AfterDeckPlacementLosses = var.dblVal;

               hr = pLoad->get_Property(_T("AfterSIDLLosses"),&var);
               pObj->m_AfterSIDLLosses = var.dblVal;

               hr = pLoad->get_Property(_T("FinalLosses"),&var);
               pObj->m_FinalLosses = var.dblVal;

               hr = pLoad->EndUnit();
            }

            if ( 6 < version && version < 8 )
            {
               var.vt = VT_BOOL;
               hr = pLoad->get_Property(_T("IgnoreTimeDependentEffects"),&var);
               pObj->m_bIgnoreCreepEffects = (var.boolVal == VARIANT_TRUE ? true : false);
               pObj->m_bIgnoreShrinkageEffects = (var.boolVal == VARIANT_TRUE ? true : false);
               pObj->m_bIgnoreRelaxationEffects = (var.boolVal == VARIANT_TRUE ? true : false);
            }
            else if ( 8 <= version )
            {
               var.vt = VT_BOOL;
               hr = pLoad->get_Property(_T("IgnoreCreepEffects"),&var);
               pObj->m_bIgnoreCreepEffects = (var.boolVal == VARIANT_TRUE ? true : false);

               hr = pLoad->get_Property(_T("IgnoreShrinkageEffects"),&var);
               pObj->m_bIgnoreShrinkageEffects = (var.boolVal == VARIANT_TRUE ? true : false);

               hr = pLoad->get_Property(_T("IgnoreRelaxationEffects"),&var);
               pObj->m_bIgnoreRelaxationEffects = (var.boolVal == VARIANT_TRUE ? true : false);
            }

            hr = pLoad->BeginUnit(_T("PostTensioning"));
            
            var.vt = VT_R8;
            hr = pLoad->get_Property(_T("AnchorSet"),&var);
            pObj->m_Dset_PT = var.dblVal;

            hr = pLoad->get_Property(_T("WobbleFriction"),&var);
            pObj->m_WobbleFriction_PT = var.dblVal;

            hr = pLoad->get_Property(_T("FrictionCoefficient"),&var);
            pObj->m_FrictionCoefficient_PT = var.dblVal;

            hr = pLoad->EndUnit(); // PostTensioning

            // added in version 6
            if ( 5 < version )
            {
               hr = pLoad->BeginUnit(_T("TemporaryStrandPT"));
               
               var.vt = VT_R8;
               hr = pLoad->get_Property(_T("AnchorSet"),&var);
               pObj->m_Dset_TTS = var.dblVal;

               hr = pLoad->get_Property(_T("WobbleFriction"),&var);
               pObj->m_WobbleFriction_TTS = var.dblVal;

               hr = pLoad->get_Property(_T("FrictionCoefficient"),&var);
               pObj->m_FrictionCoefficient_TTS = var.dblVal;

               hr = pLoad->EndUnit(); // TemporaryStrandPT
            }
            else
            {
               pObj->m_Dset_TTS                = pObj->m_Dset_PT;
               pObj->m_WobbleFriction_TTS      = pObj->m_WobbleFriction_PT;
               pObj->m_FrictionCoefficient_TTS = pObj->m_FrictionCoefficient_PT;
            }
         }
      }
      catch (HRESULT)
      {
         ATLASSERT(false);
         THROW_LOAD(InvalidFileFormat,pStrLoad);
      }
   }

   return S_OK;
}

HRESULT CProjectAgentImp::LiftingAndHaulingDataProc(IStructuredSave* pSave,IStructuredLoad* pLoad,IProgress* pProgress,CProjectAgentImp* pObj)
{
   HRESULT hr = S_OK;
   if ( pSave )
   {
      return S_OK; // not saving in this old format
   }
   else
   {
      // reading old format
      Float64 version;
      pLoad->get_Version(&version);
      if ( version <= 1.0 ) // if ProjectData version 1 or less, load the old format, otherwise do nothing
      {
         pLoad->BeginUnit(_T("LiftingAndHauling"));

         CComVariant var;

         Float64 version;
         pLoad->get_Version(&version);

         if ( version < 3 )
         {
            var.vt = VT_I4;
            HRESULT hr = pLoad->get_Property(_T("LiftingAndHaulingDataCount"), &var );
            if ( FAILED(hr) )
            {
               return hr;
            }

            long count = var.lVal;
            for ( int i = 0; i < count; i++ )
            {
               hr =  LiftingAndHaulingLoadDataProc(pLoad,pProgress,pObj);
               if ( FAILED(hr) )
               {
                  return hr;
               }
            }
         }
         else
         {
            // version 3 and later data blocks... count isn't listed because
            // the number of girders can vary per span
            SpanIndexType nSpans = pObj->m_BridgeDescription.GetSpanCount();
            for ( SpanIndexType spanIdx = 0; spanIdx < nSpans; spanIdx++ )
            {
               CGirderGroupData* pGroup = pObj->m_BridgeDescription.GetGirderGroup(spanIdx);
               GirderIndexType nGirders = pGroup->GetGirderCount();
               for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
               {
                  hr =  LiftingAndHaulingLoadDataProc(pLoad,pProgress,pObj);
                  if ( FAILED(hr) )
                  {
                     return hr;
                  }
               }
            }
         }

         pLoad->EndUnit();
      } // end if parentVersion
   } // end if pSave

   return S_OK;
}

HRESULT CProjectAgentImp::LiftingAndHaulingLoadDataProc(IStructuredLoad* pLoad,IProgress* pProgress,CProjectAgentImp* pObj)
{
   HRESULT hr = pLoad->BeginUnit(_T("LiftingAndHaulingData"));
   if ( FAILED(hr) )
   {
      return hr;
   }

   Float64 version;
   pLoad->get_Version(&version);

   CComVariant var;
   var.ChangeType( VT_I2 );
   hr = pLoad->get_Property(_T("Span"), &var );
   if ( FAILED(hr) )
   {
      return hr;
   }

   SpanIndexType span = var.iVal;

   var.ChangeType( VT_I2 );
   hr = pLoad->get_Property(_T("Girder"), &var );
   if ( FAILED(hr) )
   {
      return hr;
   }

   GirderIndexType girder = var.iVal;

   CHandlingData handlingData;

   if ( version < 1.1 )
   {
      // only 1 lift point and truck support point... use same at each end of girder
      var.ChangeType( VT_R8 );
      hr = pLoad->get_Property(_T("LiftingLoopLocation"), &var );
      if ( FAILED(hr) )
      {
         return hr;
      }

      handlingData.LeftLiftPoint  = var.dblVal;
      handlingData.RightLiftPoint = var.dblVal;

      var.ChangeType( VT_R8 );
      hr = pLoad->get_Property(_T("TruckSupportLocation"), &var );
      if ( FAILED(hr) )
      {
         return hr;
      }

      handlingData.LeadingSupportPoint  = var.dblVal;
      handlingData.TrailingSupportPoint = var.dblVal;

      CGirderGroupData* pGroup       = pObj->m_BridgeDescription.GetGirderGroup(span);
      CSplicedGirderData* pGirder   = pGroup->GetGirder(girder);
      CPrecastSegmentData* pSegment = pGirder->GetSegment(0);
      pSegment->HandlingData = handlingData;
   }
   else
   {
      var.ChangeType( VT_R8 );
      hr = pLoad->get_Property(_T("LeftLiftingLoopLocation"), &var );
      if ( FAILED(hr) )
      {
         return hr;
      }

      handlingData.LeftLiftPoint = var.dblVal;

      var.ChangeType( VT_R8 );
      hr = pLoad->get_Property(_T("RightLiftingLoopLocation"), &var );
      if ( FAILED(hr) )
      {
         return hr;
      }

      handlingData.RightLiftPoint = var.dblVal;

      var.ChangeType( VT_R8 );
      hr = pLoad->get_Property(_T("LeadingOverhang"), &var );
      if ( FAILED(hr) )
      {
         return hr;
      }

      handlingData.LeadingSupportPoint = var.dblVal;

      var.ChangeType( VT_R8 );
      hr = pLoad->get_Property(_T("TrailingOverhang"), &var );
      if ( FAILED(hr) )
      {
         return hr;
      }

      handlingData.TrailingSupportPoint = var.dblVal;

      CGirderGroupData* pGroup       = pObj->m_BridgeDescription.GetGirderGroup(span);
      CSplicedGirderData* pGirder   = pGroup->GetGirder(girder);
      CPrecastSegmentData* pSegment = pGirder->GetSegment(0);
      pSegment->HandlingData = handlingData;
   }

   hr = pLoad->EndUnit();// LiftingAndHaulingData
   if ( FAILED(hr) )
   {
      return hr;
   }

   return S_OK;
}

HRESULT CProjectAgentImp::DistFactorMethodDataProc(IStructuredSave* pSave,IStructuredLoad* pLoad,IProgress* pProgress,CProjectAgentImp* pObj)
{
   HRESULT hr = S_OK;
   if ( pSave )
   {
      return S_OK; // not saving in this old format
   }
   else
   {
      // reading old format
      Float64 version;
      pLoad->get_Version(&version);
      if ( version <= 1.0 ) // if ProjectData version 1 or less, load the old format, otherwise do nothing
      {
         if ( !FAILED(pLoad->BeginUnit(_T("DistFactorMethodDetails"))) )
         {
            hr = CProjectAgentImp::DistFactorMethodDataProc2(pSave,pLoad,pProgress,pObj);
            pLoad->EndUnit();
         }
      }
   }
   return S_OK;
}

HRESULT CProjectAgentImp::DistFactorMethodDataProc2(IStructuredSave* pSave,IStructuredLoad* pLoad,IProgress* pProgress,CProjectAgentImp* pObj)
{
   HRESULT hr = S_OK;
   if ( pSave )
   {
      ATLASSERT(false); // should never get here (not saving in old format)
      return E_FAIL;
   }
   else
   {
      // Direct input was actually provided

      CComVariant var;
      var.vt = VT_BSTR;
      hr = pLoad->get_Property(_T("CalculationMethod"), &var );
      if ( FAILED(hr) )
      {
         return hr;
      }

      _bstr_t bval(var.bstrVal);
      if (bval != _bstr_t(_T("DirectlyInput")))
      {
         return E_FAIL;
      }


      // set up the bridge model
      pObj->m_BridgeDescription.SetDistributionFactorMethod(pgsTypes::DirectlyInput);


      Float64 version;
      pLoad->get_Version(&version);

      if ( version < 1.1 )
      {
         // load data in pre- version 1.1 format
         // Load the old data into temporary variables
         var.vt = VT_R8 ;
         hr = pLoad->get_Property(_T("ShearDistFactor"), &var );
         if ( FAILED(hr) )
         {
            return hr;
         }

         Float64 V = var.dblVal;

         hr = pLoad->get_Property(_T("MomentDistFactor"), &var );
         if ( FAILED(hr) )
         {
            return hr;
         }

         Float64 M = var.dblVal;

         hr = pLoad->get_Property(_T("ReactionDistFactor"), &var );
         if ( FAILED(hr) )
         {
            return hr;
         }

         Float64 R = var.dblVal;

         // Fill up the data structures
         CPierData2* pPier = pObj->m_BridgeDescription.GetPier(0);
         CSpanData2* pSpan;
         do
         {
            pPier->SetLLDFNegMoment(pgsTypes::Interior,pgsTypes::StrengthI,M);
            pPier->SetLLDFNegMoment(pgsTypes::Exterior,pgsTypes::StrengthI,M);
            pPier->SetLLDFNegMoment(pgsTypes::Interior,pgsTypes::FatigueI,M);
            pPier->SetLLDFNegMoment(pgsTypes::Exterior,pgsTypes::FatigueI,M);

            pPier->SetLLDFReaction(pgsTypes::Interior,pgsTypes::StrengthI,R);
            pPier->SetLLDFReaction(pgsTypes::Exterior,pgsTypes::StrengthI,R);
            pPier->SetLLDFReaction(pgsTypes::Interior,pgsTypes::FatigueI,R);
            pPier->SetLLDFReaction(pgsTypes::Exterior,pgsTypes::FatigueI,R);

            pSpan = pPier->GetNextSpan();
            if ( pSpan )
            {
               pSpan->SetLLDFNegMoment(pgsTypes::Interior,pgsTypes::StrengthI,M);
               pSpan->SetLLDFPosMoment(pgsTypes::Interior,pgsTypes::StrengthI,M);
               pSpan->SetLLDFShear(pgsTypes::Interior,pgsTypes::StrengthI,V);
               pSpan->SetLLDFNegMoment(pgsTypes::Interior,pgsTypes::FatigueI,M);
               pSpan->SetLLDFPosMoment(pgsTypes::Interior,pgsTypes::FatigueI,M);
               pSpan->SetLLDFShear(pgsTypes::Interior,pgsTypes::FatigueI,V);

               pSpan->SetLLDFNegMoment(pgsTypes::Exterior,pgsTypes::StrengthI,M);
               pSpan->SetLLDFPosMoment(pgsTypes::Exterior,pgsTypes::StrengthI,M);
               pSpan->SetLLDFShear(pgsTypes::Exterior,pgsTypes::StrengthI,V);
               pSpan->SetLLDFNegMoment(pgsTypes::Exterior,pgsTypes::FatigueI,M);
               pSpan->SetLLDFPosMoment(pgsTypes::Exterior,pgsTypes::FatigueI,M);
               pSpan->SetLLDFShear(pgsTypes::Exterior,pgsTypes::FatigueI,V);

               pPier = pSpan->GetNextPier();
            }
         } while ( pSpan );
      }
      else if ( 1.1 <= version && version < 2.0 )
      {
         // Load data from 1.1 to pre-version 2.0 format into temporary variables

         Float64 M[2], V[2], R[2];

         var.vt = VT_R8 ;
         hr = pLoad->get_Property(_T("IntShearDistFactor"), &var );
         if ( FAILED(hr) )
         {
            return hr;
         }

         V[pgsTypes::Interior] = var.dblVal;

         hr = pLoad->get_Property(_T("IntMomentDistFactor"), &var );
         if ( FAILED(hr) )
         {
            return hr;
         }

         M[pgsTypes::Interior] = var.dblVal;

         hr = pLoad->get_Property(_T("IntReactionDistFactor"), &var );
         if ( FAILED(hr) )
         {
            return hr;
         }

         R[pgsTypes::Interior] = var.dblVal;

      
         hr = pLoad->get_Property(_T("ExtShearDistFactor"), &var );
         if ( FAILED(hr) )
         {
            return hr;
         }

         V[pgsTypes::Exterior] = var.dblVal;

         hr = pLoad->get_Property(_T("ExtMomentDistFactor"), &var );
         if ( FAILED(hr) )
         {
            return hr;
         }

         M[pgsTypes::Exterior] = var.dblVal;

         hr = pLoad->get_Property(_T("ExtReactionDistFactor"), &var );
         if ( FAILED(hr) )
         {
            return hr;
         }

         R[pgsTypes::Exterior] = var.dblVal;


         // Fill up the data structures
         CPierData2* pPier = pObj->m_BridgeDescription.GetPier(0);
         CSpanData2* pSpan;
         do
         {
            pSpan = pPier->GetNextSpan();
            if ( pSpan )
            {
               CGirderGroupData* pGroup = pObj->m_BridgeDescription.GetGirderGroup(pSpan);
               GirderIndexType ngdrs = pGroup->GetGirderCount();

               for ( int ls = 0; ls < 2; ls++ )
               {
                  pgsTypes::LimitState limitState = (ls == 0 ? pgsTypes::StrengthI : pgsTypes::FatigueI);

                  for (GirderIndexType ig=0; ig<ngdrs; ig++)
                  {
                     pgsTypes::GirderLocation type = (ig==0 || ig==ngdrs-1)? pgsTypes::Exterior : pgsTypes::Interior;
                     
                     pPier->SetLLDFNegMoment(type, limitState,M[type]);

                     pPier->SetLLDFReaction(type, limitState,R[type]);

                     if ( pSpan )
                     {
                        pSpan->SetLLDFNegMoment(ig,limitState,M[type]);
                        pSpan->SetLLDFPosMoment(ig,limitState,M[type]);
                        pSpan->SetLLDFShear(ig,limitState,V[type]);
                     }
                  }
               }

               pPier = pSpan->GetNextPier();
            }

         } while ( pSpan );
      }
      else
      {
         // load version 2 format
         for (int type = (int)pgsTypes::Interior; type <= (int)pgsTypes::Exterior; type++ )
         {
            CPierData2* pPier = pObj->m_BridgeDescription.GetPier(0);
            CSpanData2* pSpan;

            hr = pLoad->BeginUnit((pgsTypes::GirderLocation)type == pgsTypes::Interior ? _T("InteriorGirders") : _T("ExteriorGirders"));
            if ( FAILED(hr) )
            {
               return hr;
            }

            do
            {
               Float64 pM, nM, V, R;

               var.vt = VT_R8;
               hr = pLoad->BeginUnit(_T("Pier"));
               if ( FAILED(hr) )
               {
                  return hr;
               }

               hr = pLoad->get_Property(_T("pM"),&var);
               if ( FAILED(hr) )
               {
                  return hr;
               }

               hr = pLoad->get_Property(_T("nM"),&var);
               if ( FAILED(hr) )
               {
                  return hr;
               }


               pPier->SetLLDFNegMoment(type,pgsTypes::StrengthI,var.dblVal);
               pPier->SetLLDFNegMoment(type,pgsTypes::FatigueI, var.dblVal);

               hr = pLoad->get_Property(_T("V"), &var);
               if ( FAILED(hr) )
               {
                  return hr;
               }

               hr = pLoad->get_Property(_T("R"), &var);
               if ( FAILED(hr) )
               {
                  return hr;
               }

               pPier->SetLLDFReaction(type,pgsTypes::StrengthI,var.dblVal);
               pPier->SetLLDFReaction(type,pgsTypes::FatigueI, var.dblVal);

               pLoad->EndUnit();

               pSpan = pPier->GetNextSpan();

               if ( pSpan )
               {
                  hr = pLoad->BeginUnit(_T("Span"));
                  if ( FAILED(hr) )
                  {
                     return hr;
                  }


                  hr = pLoad->get_Property(_T("pM"),&var);
                  if ( FAILED(hr) )
                  {
                     return hr;
                  }

                  pM = var.dblVal;

                  hr = pLoad->get_Property(_T("nM"),&var);
                  if ( FAILED(hr) )
                  {
                     return hr;
                  }

                  nM = var.dblVal;


                  hr = pLoad->get_Property(_T("V"), &var);
                  if ( FAILED(hr) )
                  {
                     return hr;
                  }

                  V = var.dblVal;

                  hr = pLoad->get_Property(_T("R"), &var);
                  if ( FAILED(hr) )
                  {
                     return hr;
                  }

                  R = var.dblVal;

                  // Set for all girders
                  CGirderGroupData* pGroup = pObj->m_BridgeDescription.GetGirderGroup(pSpan);
                  GirderIndexType ngdrs = pGroup->GetGirderCount();
                  for (GirderIndexType ig=0; ig<ngdrs; ig++)
                  {
                     if ((pgsTypes::GirderLocation)type==pgsTypes::Interior && pGroup->IsInteriorGirder(ig) ||
                         (pgsTypes::GirderLocation)type==pgsTypes::Exterior && pGroup->IsExteriorGirder(ig))
                     {
                        pSpan->SetLLDFPosMoment(ig,pgsTypes::StrengthI,pM);
                        pSpan->SetLLDFPosMoment(ig,pgsTypes::FatigueI, pM);
                        pSpan->SetLLDFNegMoment(ig,pgsTypes::StrengthI,nM);
                        pSpan->SetLLDFNegMoment(ig,pgsTypes::FatigueI, nM);
                        pSpan->SetLLDFShear(ig,pgsTypes::StrengthI,V);
                        pSpan->SetLLDFShear(ig,pgsTypes::FatigueI, V);
                     }
                  }

                  pPier = pSpan->GetNextPier();

                  pLoad->EndUnit();
               }
            } while ( pSpan );
            pLoad->EndUnit();
         }
      }
   }

   return hr;
}

HRESULT CProjectAgentImp::UserLoadsDataProc(IStructuredSave* pSave,IStructuredLoad* pLoad,IProgress* pProgress,CProjectAgentImp* pObj)
{
   HRESULT hr = S_OK;
   if ( pSave )
   {
      pSave->BeginUnit(_T("UserDefinedLoads"),5.0);

      // starting with version 5 of this data block, loads are managed with the CLoadManager
      pObj->m_LoadManager.Save(pSave,pProgress);

      // removed in version 5
      //// starting with version 4 of this data block, the point, distributed, and moment loads save the loading event ID instead of index
      //pObj->SavePointLoads(pSave);
      //pObj->SaveDistributedLoads(pSave);
      //pObj->SaveMomentLoads(pSave);

      pSave->put_Property(_T("ConstructionLoad"),CComVariant(pObj->m_ConstructionLoad));
      
      pSave->EndUnit();
   }
   else
   {
      // only load if the user defined loads are in the project. older versions have none
      if (!FAILED(pLoad->BeginUnit(_T("UserDefinedLoads"))))
      {
         Float64 version;
         pLoad->get_Version(&version);

         hr = pObj->m_LoadManager.Load(pLoad,pProgress);
         if ( FAILED(hr) )
         {
            return hr;
         }

         if ( version < 4 )
         {
            pObj->m_bUpdateUserDefinedLoads = true;
         }

         if ( 3 <= version )
         {
            CComVariant var;
            var.vt = VT_R8;
            pLoad->get_Property(_T("ConstructionLoad"),&var);
            pObj->m_ConstructionLoad = var.dblVal;
         }

         hr = pLoad->EndUnit();// UserDefinedLoads
         if ( FAILED(hr) )
         {
            return hr;
         }
      }
   }

   return hr;
}

HRESULT CProjectAgentImp::LiveLoadsDataProc(IStructuredSave* pSave,IStructuredLoad* pLoad,IProgress* pProgress,CProjectAgentImp* pObj)
{
   HRESULT hr = S_OK;
   if ( pSave )
   {
      pSave->BeginUnit(_T("LiveLoads"),5.0);

      // version 4... added fatigue live load


      // added in version 2
      // changed to enum values in version 3
      int iaction= GetIntForLldfAction(pObj->m_LldfRangeOfApplicabilityAction);

      pSave->put_Property(_T("IngoreLLDFRangeOfApplicability"),CComVariant(iaction));

      CProjectAgentImp::SaveLiveLoad(pSave,pProgress,pObj,_T("DesignLiveLoads"),pgsTypes::lltDesign);
      CProjectAgentImp::SaveLiveLoad(pSave,pProgress,pObj,_T("PermitLiveLoads"),pgsTypes::lltPermit);
      CProjectAgentImp::SaveLiveLoad(pSave,pProgress,pObj,_T("FatigueLiveLoads"),pgsTypes::lltFatigue); // added in version 4

      CProjectAgentImp::SaveLiveLoad(pSave,pProgress,pObj,_T("LegalRoutineLiveLoads"),pgsTypes::lltLegalRating_Routine); // added in version 5
      CProjectAgentImp::SaveLiveLoad(pSave,pProgress,pObj,_T("LegalSpecialLiveLoads"),pgsTypes::lltLegalRating_Special); // added in version 5
      CProjectAgentImp::SaveLiveLoad(pSave,pProgress,pObj,_T("PermitRoutineLiveLoads"),pgsTypes::lltPermitRating_Routine); // added in version 5
      CProjectAgentImp::SaveLiveLoad(pSave,pProgress,pObj,_T("PermitSpecialLiveLoads"),pgsTypes::lltPermitRating_Special); // added in version 5

      pSave->EndUnit(); // LiveLoads
   }
   else
   {
      // only load if the live loads are in the project. older versions have none
      if (!FAILED(pLoad->BeginUnit(_T("LiveLoads"))))
      {
         CComVariant var;
         Float64 version;
         pLoad->get_Version(&version);
         if ( 2 < version )
         {
            // changed value to enum in Version 3
            var.vt = VT_I4;
            hr = pLoad->get_Property(_T("IngoreLLDFRangeOfApplicability"),&var);
            pObj->m_LldfRangeOfApplicabilityAction = GetLldfActionForInt(var.intVal);
            pObj->m_bGetIgnoreROAFromLibrary = false;
         }
         else if ( 2==version )
         {
            // value was bool in Version 2
            var.vt = VT_BOOL;
            hr = pLoad->get_Property(_T("IngoreLLDFRangeOfApplicability"),&var);
            pObj->m_LldfRangeOfApplicabilityAction =(var.boolVal == VARIANT_TRUE ? roaIgnore : roaEnforce);
            pObj->m_bGetIgnoreROAFromLibrary = false;
         }
         else
         {
            pObj->m_bGetIgnoreROAFromLibrary = true;
         }

         hr = CProjectAgentImp::LoadLiveLoad(pLoad,pProgress,pObj,_T("DesignLiveLoads"),pgsTypes::lltDesign);
         if ( FAILED(hr) )
         {
            return hr;
         }

         hr = CProjectAgentImp::LoadLiveLoad(pLoad,pProgress,pObj,_T("PermitLiveLoads"),pgsTypes::lltPermit);
         if ( FAILED(hr) )
         {
            return hr;
         }

         if ( 4 <= version )
         {
            hr = CProjectAgentImp::LoadLiveLoad(pLoad,pProgress,pObj,_T("FatigueLiveLoads"),pgsTypes::lltFatigue);
            if ( FAILED(hr) )
            {
               return hr;
            }
         }

         if ( 5 <= version )
         {
            hr = CProjectAgentImp::LoadLiveLoad(pLoad,pProgress,pObj,_T("LegalRoutineLiveLoads"),pgsTypes::lltLegalRating_Routine);
            if ( FAILED(hr) )
            {
               return hr;
            }

            hr = CProjectAgentImp::LoadLiveLoad(pLoad,pProgress,pObj,_T("LegalSpecialLiveLoads"),pgsTypes::lltLegalRating_Special);
            if ( FAILED(hr) )
            {
               return hr;
            }

            hr = CProjectAgentImp::LoadLiveLoad(pLoad,pProgress,pObj,_T("PermitRoutineLiveLoads"),pgsTypes::lltPermitRating_Routine);
            if ( FAILED(hr) )
            {
               return hr;
            }

            hr = CProjectAgentImp::LoadLiveLoad(pLoad,pProgress,pObj,_T("PermitSpecialLiveLoads"),pgsTypes::lltPermitRating_Special);
            if ( FAILED(hr) )
            {
               return hr;
            }
         }

         hr = pLoad->EndUnit(); // LiveLoads
         if ( FAILED(hr) )
         {
            return hr;
         }
      }
   }

   return hr;
}

HRESULT CProjectAgentImp::SaveLiveLoad(IStructuredSave* pSave,IProgress* pProgress,CProjectAgentImp* pObj,LPTSTR lpszUnitName,pgsTypes::LiveLoadType llType)
{
   // version 2 added m_PedestrianLoadApplicationType
   pSave->BeginUnit(lpszUnitName,2.0);

   pSave->put_Property(_T("TruckImpact"),CComVariant(pObj->m_TruckImpact[llType]));
   pSave->put_Property(_T("LaneImpact"), CComVariant(pObj->m_LaneImpact[llType]));

   LiveLoadSelectionContainer::size_type cnt = pObj->m_SelectedLiveLoads[llType].size();
   pSave->put_Property(_T("VehicleCount"), CComVariant(cnt));
   {
      pSave->BeginUnit(_T("Vehicles"),1.0);
      for (const auto& sel : pObj->m_SelectedLiveLoads[llType])
      {
         const std::_tstring& name = sel.EntryName;
         pSave->put_Property(_T("VehicleName"), CComVariant(name.c_str()));
      }

      pSave->EndUnit(); // Vehicles
   }

   // m_PedestrianLoadApplicationType
   // Save valid values only for non-rating llTypes
   LONG app_type = (llType<pgsTypes::lltDesign || llType>pgsTypes::lltFatigue) ? INVALID_INDEX : pObj->m_PedestrianLoadApplicationType[llType];
   pSave->put_Property(_T("PedestrianLoadApplicationType"), CComVariant(app_type));


   pSave->EndUnit();

   return S_OK;
}

HRESULT CProjectAgentImp::LoadLiveLoad(IStructuredLoad* pLoad,IProgress* pProgress,CProjectAgentImp* pObj,LPTSTR lpszUnitName,pgsTypes::LiveLoadType llType)
{
   const LiveLoadLibrary* pLiveLoadLibrary = pObj->m_pLibMgr->GetLiveLoadLibrary();

   HRESULT hr = pLoad->BeginUnit(lpszUnitName);
   if ( FAILED(hr) )
   {
      return hr;
   }

   Float64 vers;
   pLoad->get_Version(&vers);

   CComVariant var;
   var.vt = VT_R8;
   hr = pLoad->get_Property(_T("TruckImpact"),&var);
   if ( FAILED(hr) )
   {
      return hr;
   }

   pObj->m_TruckImpact[llType] = var.dblVal;

   var.Clear();
   var.vt = VT_R8;
   hr = pLoad->get_Property(_T("LaneImpact"),&var);
   if ( FAILED(hr) )
   {
      return hr;
   }

   pObj->m_LaneImpact[llType] = var.dblVal;

   var.Clear();
   var.vt = VT_INDEX;
   hr = pLoad->get_Property(_T("VehicleCount"), &var);
   if ( FAILED(hr) )
   {
      return hr;
   }

   IndexType nVehicles = VARIANT2INDEX(var);

   {
      hr = pLoad->BeginUnit(_T("Vehicles"));
      if ( FAILED(hr) )
      {
         return hr;
      }

      pObj->m_SelectedLiveLoads[llType].clear();

      bool was_pedestrian=false; // pedestrian loads used to be saved as vehicles
      for (IndexType vehIndex = 0; vehIndex < nVehicles; vehIndex++)
      {
         var.Clear();
         var.vt = VT_BSTR;
         hr = pLoad->get_Property(_T("VehicleName"), &var);
         if ( FAILED(hr) )
         {
            return hr;
         }

         _bstr_t vnam(var.bstrVal);

         if (vnam == _bstr_t(_T("Pedestrian on Sidewalk")))
         {
            ATLASSERT(vers<2.0); // should not been saving this in new versions
            was_pedestrian = true; 
         }
         else
         {
            LiveLoadSelection sel;
            sel.EntryName = vnam;
            pObj->m_SelectedLiveLoads[llType].insert(sel);
         }
      }

      // assocated library entry
      LiveLoadSelectionIterator iter(pObj->m_SelectedLiveLoads[llType].begin());
      LiveLoadSelectionIterator end(pObj->m_SelectedLiveLoads[llType].end());
      for ( ; iter != end; iter++ )
      {
         LiveLoadSelection& sel(const_cast<LiveLoadSelection&>(*iter));
         if ( !pObj->IsReservedLiveLoad(sel.EntryName) )
         {
            use_library_entry(&pObj->m_LibObserver,
                              sel.EntryName,
                              &sel.pEntry,
                              *pLiveLoadLibrary);
         }
      }

      hr = pLoad->EndUnit(); // Vehicles
      if ( FAILED(hr) )
      {
         return hr;
      }

      // m_PedestrianLoadApplicationType
      if (vers>1.0)
      {
         var.Clear();
         var.vt = VT_I4;
         hr = pLoad->get_Property(_T("PedestrianLoadApplicationType"),&var);
         if ( FAILED(hr) )
         {
            return hr;
         }

         // Save valid values only for non-rating llTypes
         if(!(llType<pgsTypes::lltDesign || llType>pgsTypes::lltFatigue))
         {
            ATLASSERT(var.lVal!=INVALID_INDEX);
            pObj->m_PedestrianLoadApplicationType[llType] = (PedestrianLoadApplicationType)var.lVal;
         }
      }
      else
      {
         if(!(llType<pgsTypes::lltDesign || llType>pgsTypes::lltFatigue))
         {
            pObj->m_PedestrianLoadApplicationType[llType] = was_pedestrian ? ILiveLoads::PedConcurrentWithVehicular : ILiveLoads::PedDontApply;
         }
      }
   }



   hr = pLoad->EndUnit();
   if ( FAILED(hr) )
   {
      return hr;
   }

   return S_OK;
}

#if defined _DEBUG
bool CProjectAgentImp::AssertValid() const
{
   // Check Libraries
   if (m_pLibMgr==0)
   {
      return false;
   }

   if ( !m_pLibMgr->AssertValid() )
   {
      return false;
   }

   // Check pier order
   const CSpanData2* pSpan = m_BridgeDescription.GetSpan(0);
   while ( pSpan )
   {
      const CPierData2* pPrevPier = pSpan->GetPrevPier();
      const CPierData2* pNextPier = pSpan->GetNextPier();

      if ( pNextPier->GetStation() <= pPrevPier->GetStation() )
      {
         // Error... piers not sorted correctly
         WATCH(_T("Piers not sorted correctly"));
         return false;
      }

      pSpan = pNextPier->GetNextSpan();
   }

   return true;
}
#endif // _DEBUG

BEGIN_STRSTORAGEMAP(CProjectAgentImp,_T("ProjectData"),8.0)
   BEGIN_UNIT(_T("ProjectProperties"),_T("Project Properties"),1.0)
      PROPERTY(_T("BridgeName"),SDT_STDSTRING, m_BridgeName )
      PROPERTY(_T("BridgeId"),  SDT_STDSTRING, m_BridgeID )
      PROPERTY(_T("JobNumber"), SDT_STDSTRING, m_JobNumber )
      PROPERTY(_T("Engineer"),  SDT_STDSTRING, m_Engineer )
      PROPERTY(_T("Company"),   SDT_STDSTRING, m_Company )
      PROPERTY(_T("Comments"),  SDT_STDSTRING, m_Comments )
   END_UNIT // Project Properties

   BEGIN_UNIT(_T("ProjectSettings"),_T("Project Settings"),1.0)
      PROP_CALLBACK(CProjectAgentImp::UnitModeProc)
      //PROPERTY(_T("Units"), SDT_I4, m_Units )
   END_UNIT // Project Settings

   BEGIN_UNIT(_T("Environment"),_T("Environmental Parameters"),1.0)
      PROPERTY(_T("ExpCond"), SDT_I4, m_ExposureCondition )
      PROPERTY(_T("RelHumidity"), SDT_R8, m_RelHumidity )
   END_UNIT // Environment

   BEGIN_UNIT(_T("Alignment"),_T("Roadway Alignment"),1.0)
      PROP_CALLBACK(CProjectAgentImp::AlignmentProc)
   END_UNIT // Alignment

   BEGIN_UNIT(_T("Profile"),_T("Roadway Profile"),1.0)
      PROP_CALLBACK(CProjectAgentImp::ProfileProc)
   END_UNIT // Profile

   BEGIN_UNIT(_T("Crown"),_T("Roadway Crown"),1.0)
      PROP_CALLBACK(CProjectAgentImp::SuperelevationProc)
   END_UNIT // Crown

   PROP_CALLBACK(CProjectAgentImp::PierDataProc)
   PROP_CALLBACK(CProjectAgentImp::XSectionDataProc)
   PROP_CALLBACK(CProjectAgentImp::BridgeDescriptionProc)
   PROP_CALLBACK(CProjectAgentImp::PrestressingDataProc )
   PROP_CALLBACK(CProjectAgentImp::ShearDataProc )
   PROP_CALLBACK(CProjectAgentImp::LongitudinalRebarDataProc )
   PROP_CALLBACK(CProjectAgentImp::SpecificationProc)

   BEGIN_UNIT(_T("LoadModifiers"),_T("Load Modifiers"), 1.0 )
      PROPERTY(_T("DuctilityLevel"),   SDT_I4, m_DuctilityLevel )
      PROPERTY(_T("DuctilityFactor"),  SDT_R8, m_DuctilityFactor )
      PROPERTY(_T("ImportanceLevel"),  SDT_I4, m_ImportanceLevel )
      PROPERTY(_T("ImportanceFactor"), SDT_R8, m_ImportanceFactor )
      PROPERTY(_T("RedundancyLevel"),  SDT_I4, m_RedundancyLevel )
      PROPERTY(_T("RedundancyFactor"), SDT_R8, m_RedundancyFactor )
   END_UNIT // Load Modifiers

   PROP_CALLBACK(CProjectAgentImp::LoadFactorsProc)

   PROP_CALLBACK(CProjectAgentImp::LossesProc)

   PROP_CALLBACK(CProjectAgentImp::LiftingAndHaulingDataProc )
   PROP_CALLBACK(CProjectAgentImp::DistFactorMethodDataProc )
   PROP_CALLBACK(CProjectAgentImp::UserLoadsDataProc )
   PROP_CALLBACK(CProjectAgentImp::LiveLoadsDataProc )

   PROP_CALLBACK(CProjectAgentImp::RatingSpecificationProc)
   PROP_CALLBACK(CProjectAgentImp::EffectiveFlangeWidthProc)

END_STRSTORAGEMAP

STDMETHODIMP CProjectAgentImp::SetBroker(IBroker* pBroker)
{
   EAF_AGENT_SET_BROKER(pBroker);
   return S_OK;
}

STDMETHODIMP CProjectAgentImp::RegInterfaces()
{
   CComQIPtr<IBrokerInitEx2,&IID_IBrokerInitEx2> pBrokerInit(m_pBroker);

   pBrokerInit->RegInterface( IID_IProjectProperties,    this );
   pBrokerInit->RegInterface( IID_IEnvironment,          this );
   pBrokerInit->RegInterface( IID_IRoadwayData,          this );
   pBrokerInit->RegInterface( IID_IBridgeDescription,    this );
   pBrokerInit->RegInterface( IID_ISegmentData,          this );
   pBrokerInit->RegInterface( IID_IShear,                this );
   pBrokerInit->RegInterface( IID_ILongitudinalRebar,    this );
   pBrokerInit->RegInterface( IID_ISpecification,        this );
   pBrokerInit->RegInterface( IID_IRatingSpecification,  this );
   pBrokerInit->RegInterface( IID_ILibraryNames,         this );
   pBrokerInit->RegInterface( IID_ILibrary,              this );
   pBrokerInit->RegInterface( IID_ILoadModifiers,        this );
   pBrokerInit->RegInterface( IID_ISegmentLifting,        this );
   pBrokerInit->RegInterface( IID_ISegmentHauling,        this );
   pBrokerInit->RegInterface( IID_IImportProjectLibrary, this );
   pBrokerInit->RegInterface( IID_IUserDefinedLoadData,  this );
   pBrokerInit->RegInterface( IID_IEvents,               this );
   pBrokerInit->RegInterface( IID_ILimits,               this );
   pBrokerInit->RegInterface( IID_ILoadFactors,          this );
   pBrokerInit->RegInterface( IID_ILiveLoads,            this );
   pBrokerInit->RegInterface( IID_IEventMap,             this );
   pBrokerInit->RegInterface( IID_IEffectiveFlangeWidth, this );
   pBrokerInit->RegInterface( IID_ILossParameters,       this );
   pBrokerInit->RegInterface( IID_IValidate,             this );

   return S_OK;
};

STDMETHODIMP CProjectAgentImp::Init()
{
   EAF_AGENT_INIT;

   ////
   //// Attach to connection points for interfaces this agent depends on
   ////
   //CComQIPtr<IBrokerInitEx2,&IID_IBrokerInitEx2> pBrokerInit(m_pBroker);
   //CComPtr<IConnectionPoint> pCP;
   //HRESULT hr = S_OK;

   m_scidBridgeDescriptionInfo    = pStatusCenter->RegisterCallback(new pgsInformationalStatusCallback(eafTypes::statusInformation));
   m_scidBridgeDescriptionWarning = pStatusCenter->RegisterCallback(new pgsBridgeDescriptionStatusCallback(m_pBroker, eafTypes::statusWarning));
   m_scidGirderDescriptionWarning = pStatusCenter->RegisterCallback(new pgsGirderDescriptionStatusCallback(m_pBroker,eafTypes::statusWarning));
   m_scidRebarStrengthWarning     = pStatusCenter->RegisterCallback(new pgsRebarStrengthStatusCallback());
   m_scidLoadDescriptionWarning   = pStatusCenter->RegisterCallback(new pgsInformationalStatusCallback(eafTypes::statusWarning));

   return S_OK;
}

STDMETHODIMP CProjectAgentImp::Reset()
{
   if ( m_pLibMgr )
   {
      ReleaseBridgeLibraryEntries();

      // Spec Library
      const SpecLibrary& rspeclib = *(m_pLibMgr->GetSpecLibrary());
      if (m_pSpecEntry!=0)
      {
         release_library_entry( &m_LibObserver,
                                m_pSpecEntry,
                                *(m_pLibMgr->GetSpecLibrary()) );
      }


      // Rating Spec Library
      release_library_entry( &m_LibObserver, 
                             m_pRatingEntry,
                             *(m_pLibMgr->GetRatingLibrary()) );


      // Live Load Library
      const LiveLoadLibrary& pLiveLoadLibrary = *(m_pLibMgr->GetLiveLoadLibrary());
      int nLLTypes = (int)pgsTypes::lltLiveLoadTypeCount;
      for ( int i = 0; i < nLLTypes; i++ )
      {
         pgsTypes::LiveLoadType llType = (pgsTypes::LiveLoadType)i;
         LiveLoadSelectionIterator it;
         for (it = m_SelectedLiveLoads[llType].begin(); it != m_SelectedLiveLoads[llType].end(); it++)
         {
            if ( it->pEntry )
            {
               release_library_entry( &m_LibObserver, 
                                      it->pEntry,
                                      pLiveLoadLibrary);
            }
         }
      }
   }

   m_BridgeDescription.Clear();

   return S_OK;
}

STDMETHODIMP CProjectAgentImp::Init2()
{
   return S_OK;
}

STDMETHODIMP CProjectAgentImp::GetClassID(CLSID* pCLSID)
{
   *pCLSID = CLSID_ProjectAgent;
   return S_OK;
}

STDMETHODIMP CProjectAgentImp::ShutDown()
{
   EAF_AGENT_CLEAR_INTERFACE_CACHE;
   return S_OK;
}

void CProjectAgentImp::UseBridgeLibraryEntries()
{
   UseGirderLibraryEntries();

   // left traffic barrier
   const TrafficBarrierLibrary& rbarlib = m_pLibMgr->GetTrafficBarrierLibrary();
   CRailingSystem* pRailingSystem = m_BridgeDescription.GetLeftRailingSystem();
   const TrafficBarrierEntry* pEntry = nullptr;

   use_library_entry( &m_LibObserver,
                      pRailingSystem->strExteriorRailing,
                      &pEntry,
                      m_pLibMgr->GetTrafficBarrierLibrary());
   pRailingSystem->SetExteriorRailing(pEntry);

   if ( pRailingSystem->bUseInteriorRailing )
   {
      use_library_entry( &m_LibObserver,
                         pRailingSystem->strInteriorRailing,
                         &pEntry,
                        m_pLibMgr->GetTrafficBarrierLibrary());
      
      pRailingSystem->SetInteriorRailing(pEntry);
   }

   // right traffic barrier
   pRailingSystem = m_BridgeDescription.GetRightRailingSystem();

   use_library_entry( &m_LibObserver,
                      pRailingSystem->strExteriorRailing,
                      &pEntry,
                      m_pLibMgr->GetTrafficBarrierLibrary());
   
   pRailingSystem->SetExteriorRailing(pEntry);

   if ( pRailingSystem->bUseInteriorRailing )
   {
      use_library_entry( &m_LibObserver,
                         pRailingSystem->strInteriorRailing,
                         &pEntry,
                         m_pLibMgr->GetTrafficBarrierLibrary());

      pRailingSystem->SetInteriorRailing(pEntry);
   }
}

void CProjectAgentImp::UseGirderLibraryEntries()
{
   UseDuctLibraryEntries();

   if ( m_pLibMgr )
   {
      // Girders
      const GirderLibrary&  girderLibrary = m_pLibMgr->GetGirderLibrary();
      const GirderLibraryEntry* pGirderEntry = nullptr;

      use_library_entry( &m_LibObserver,
                         m_BridgeDescription.GetGirderName(), 
                         &pGirderEntry, 
                         girderLibrary);
      m_BridgeDescription.SetGirderLibraryEntry(pGirderEntry);

#if defined _DEBUG
      if ( pGirderEntry )
      {
         ATLASSERT(m_BridgeDescription.GetGirderName() == m_BridgeDescription.GetGirderLibraryEntry()->GetName());
      }
#endif

      const HaulTruckLibrary* pHaulTruckLibrary = m_pLibMgr->GetHaulTruckLibrary();
      const HaulTruckLibraryEntry* pHaulTruckEntry;

      GroupIndexType nGroups = m_BridgeDescription.GetGirderGroupCount();
      for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
      {
         CGirderGroupData* pGroup = m_BridgeDescription.GetGirderGroup(grpIdx);
         GroupIndexType nGirderTypeGroups = pGroup->GetGirderTypeGroupCount();
         for ( GroupIndexType girderTypeGroupIdx = 0; girderTypeGroupIdx < nGirderTypeGroups; girderTypeGroupIdx++ )
         {
            GirderIndexType firstGdrIdx, lastGdrIdx;
            std::_tstring strGirderName;
            pGroup->GetGirderTypeGroup(girderTypeGroupIdx,&firstGdrIdx,&lastGdrIdx,&strGirderName);

            for ( GirderIndexType gdrIdx = firstGdrIdx; gdrIdx <= lastGdrIdx; gdrIdx++ )
            {
               use_library_entry(&m_LibObserver,
                                 strGirderName,
                                 &pGirderEntry,
                                 girderLibrary);

               CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);
               pGirder->SetGirderLibraryEntry(pGirderEntry);

               SegmentIndexType nSegments = pGirder->GetSegmentCount();
               for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
               {
                  CPrecastSegmentData* pSegment = pGirder->GetSegment(segIdx);
                  if ( pSegment->Strands.GetStrandMaterial(pgsTypes::Straight) == nullptr )
                  {
                     pSegment->Strands.SetStrandMaterial(pgsTypes::Straight,lrfdStrandPool::GetInstance()->GetStrand(matPsStrand::Gr1725,matPsStrand::StressRelieved,matPsStrand::None,matPsStrand::D635));
                  }

                  if ( pSegment->Strands.GetStrandMaterial(pgsTypes::Harped) == nullptr )
                  {
                     pSegment->Strands.SetStrandMaterial(pgsTypes::Harped,lrfdStrandPool::GetInstance()->GetStrand(matPsStrand::Gr1725,matPsStrand::StressRelieved,matPsStrand::None,matPsStrand::D635));
                  }

                  if ( pSegment->Strands.GetStrandMaterial(pgsTypes::Temporary)== nullptr )
                  {
                     pSegment->Strands.SetStrandMaterial(pgsTypes::Temporary,lrfdStrandPool::GetInstance()->GetStrand(matPsStrand::Gr1725,matPsStrand::StressRelieved,matPsStrand::None,matPsStrand::D635));
                  }

                  use_library_entry(&m_LibObserver,pSegment->HandlingData.HaulTruckName,&pHaulTruckEntry,*pHaulTruckLibrary);
                  pSegment->HandlingData.pHaulTruckLibraryEntry = pHaulTruckEntry;
               }// segment loop
            }// girder loop
         }// girder type loop
      }// group loop
   }
}

void CProjectAgentImp::UseDuctLibraryEntries()
{
   if ( m_pLibMgr )
   {
      // Ducts
      const DuctLibrary* pDuctLibrary = m_pLibMgr->GetDuctLibrary();
      const DuctLibraryEntry* pEntry;

      GroupIndexType nGroups = m_BridgeDescription.GetGirderGroupCount();
      for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
      {
         CGirderGroupData* pGroup = m_BridgeDescription.GetGirderGroup(grpIdx);
         GirderIndexType nGirders = pGroup->GetGirderCount();
         for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
         {
            CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);
            CPTData* pPTData = pGirder->GetPostTensioning();
            DuctIndexType nDucts = pPTData->GetDuctCount();
            for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
            {
               CDuctData* pDuct = pPTData->GetDuct(ductIdx);
               use_library_entry(&m_LibObserver,
                                 pDuct->Name,
                                 &pEntry,
                                 *pDuctLibrary);
               pDuct->pDuctLibEntry = pEntry;
            } // duct loop
         }// girder loop
      }// group loop
   }
}

void CProjectAgentImp::ReleaseBridgeLibraryEntries()
{
   if ( m_pLibMgr )
   {
      ReleaseGirderLibraryEntries();

      // traffic barrier
      const TrafficBarrierLibrary& rbarlib = m_pLibMgr->GetTrafficBarrierLibrary();

      if ( m_BridgeDescription.GetLeftRailingSystem()->GetExteriorRailing() )
      {
         release_library_entry( &m_LibObserver,
                                m_BridgeDescription.GetLeftRailingSystem()->GetExteriorRailing(),
                                m_pLibMgr->GetTrafficBarrierLibrary());
      }

      if ( m_BridgeDescription.GetLeftRailingSystem()->GetInteriorRailing() )
      {
         release_library_entry( &m_LibObserver,
                                m_BridgeDescription.GetLeftRailingSystem()->GetInteriorRailing(),
                                m_pLibMgr->GetTrafficBarrierLibrary());
      }


      if ( m_BridgeDescription.GetRightRailingSystem()->GetExteriorRailing() )
      {
         release_library_entry( &m_LibObserver,
                                m_BridgeDescription.GetRightRailingSystem()->GetExteriorRailing(),
                                m_pLibMgr->GetTrafficBarrierLibrary());
      }

      if ( m_BridgeDescription.GetRightRailingSystem()->GetInteriorRailing() )
      {
         release_library_entry( &m_LibObserver,
                                m_BridgeDescription.GetRightRailingSystem()->GetInteriorRailing(),
                                m_pLibMgr->GetTrafficBarrierLibrary());
      }
   }
}

void CProjectAgentImp::ReleaseGirderLibraryEntries()
{
   if ( m_pLibMgr )
   {
      ReleaseDuctLibraryEntries();

      // girder entry
      const GirderLibrary& girderLibrary = m_pLibMgr->GetGirderLibrary();
      if (m_BridgeDescription.GetGirderLibraryEntry() != nullptr)
      {
         release_library_entry( &m_LibObserver,
                                m_BridgeDescription.GetGirderLibraryEntry(), 
                                girderLibrary);
      
      }

      const HaulTruckLibrary* pHaulTruckLibrary = m_pLibMgr->GetHaulTruckLibrary();

      GroupIndexType nGroups = m_BridgeDescription.GetGirderGroupCount();
      for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
      {
         CGirderGroupData* pGroup = m_BridgeDescription.GetGirderGroup(grpIdx);
         GirderIndexType nGirders = pGroup->GetGirderCount();
         for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
         {
            CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);
            release_library_entry(&m_LibObserver,pGirder->GetGirderLibraryEntry(),girderLibrary);
            pGirder->SetGirderLibraryEntry(nullptr);

            SegmentIndexType nSegments = pGirder->GetSegmentCount();
            for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
            {
               CPrecastSegmentData* pSegment = pGirder->GetSegment(segIdx);
               release_library_entry(&m_LibObserver,pSegment->HandlingData.pHaulTruckLibraryEntry,*pHaulTruckLibrary);
               pSegment->HandlingData.pHaulTruckLibraryEntry = nullptr;
            }
         }
      }

      // this must be done dead last because the Girder could be referencing the
      // bridge's girder library entry
      m_BridgeDescription.SetGirderLibraryEntry(nullptr);
   }
}

void CProjectAgentImp::ReleaseDuctLibraryEntries()
{
   if ( m_pLibMgr )
   {
      // duct entry
      const DuctLibrary* pDuctLibrary = m_pLibMgr->GetDuctLibrary();

      GroupIndexType nGroups = m_BridgeDescription.GetGirderGroupCount();
      for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
      {
         CGirderGroupData* pGroup = m_BridgeDescription.GetGirderGroup(grpIdx);
         GirderIndexType nGirders = pGroup->GetGirderCount();
         for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
         {
            CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);
            CPTData* pPTData = pGirder->GetPostTensioning();
            DuctIndexType nDucts = pPTData->GetDuctCount();
            for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
            {
               CDuctData* pDuctData = pPTData->GetDuct(ductIdx);
               release_library_entry(&m_LibObserver,pDuctData->pDuctLibEntry,*pDuctLibrary);
               pDuctData->pDuctLibEntry = nullptr;
            }
         }
      }
   }
}

void CProjectAgentImp::UpdateConcreteMaterial()
{
   bool bAfter2015 = (lrfdVersionMgr::SeventhEditionWith2016Interims <= lrfdVersionMgr::GetVersion() ? true : false);
   // starting with LRFD 2016, AllLightweight is not a valid concrete type. Concrete is either Normal weight or lightweight.
   // We are using SandLightweight to mean lightweight so updated the concrete type if needed.

   // Also, LRFD changed the equation for modulus of elasticity. If Mod E is computed, recompute the value

   GroupIndexType nGroups = m_BridgeDescription.GetGirderGroupCount();
   for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
   {
      CGirderGroupData* pGroup = m_BridgeDescription.GetGirderGroup(grpIdx);
      GirderIndexType nGirders = pGroup->GetGirderCount();
      for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
      {
         CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);
         SegmentIndexType nSegments = pGirder->GetSegmentCount();
         for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
         {
            CPrecastSegmentData* pSegment = pGirder->GetSegment(segIdx);
            if ( !pSegment->Material.Concrete.bUserEci )
            {
               pSegment->Material.Concrete.Eci = lrfdConcreteUtil::ModE(pSegment->Material.Concrete.Fci,pSegment->Material.Concrete.StrengthDensity,false);
            }

            if ( !pSegment->Material.Concrete.bUserEc )
            {
               pSegment->Material.Concrete.Ec = lrfdConcreteUtil::ModE(pSegment->Material.Concrete.Fc,pSegment->Material.Concrete.StrengthDensity,false);
            }

            if ( bAfter2015 && pSegment->Material.Concrete.Type == pgsTypes::AllLightweight )
            {
               pSegment->Material.Concrete.Type = pgsTypes::SandLightweight;
            }

            CClosureJointData* pClosureJoint = pSegment->GetEndClosure();
            if ( pClosureJoint )
            {
               if ( !pClosureJoint->GetConcrete().bUserEci )
               {
                  pClosureJoint->GetConcrete().Eci = lrfdConcreteUtil::ModE(pClosureJoint->GetConcrete().Fci,pClosureJoint->GetConcrete().StrengthDensity,false);
               }

               if ( !pClosureJoint->GetConcrete().bUserEc )
               {
                  pClosureJoint->GetConcrete().Ec = lrfdConcreteUtil::ModE(pClosureJoint->GetConcrete().Fc,pClosureJoint->GetConcrete().StrengthDensity,false);
               }

               if ( bAfter2015 && pClosureJoint->GetConcrete().Type == pgsTypes::AllLightweight )
               {
                  pClosureJoint->GetConcrete().Type = pgsTypes::SandLightweight;
               }
            }
         }
      }
   }

   CDeckDescription2* pDeck = m_BridgeDescription.GetDeckDescription();
   if ( pDeck )
   {
      if ( !pDeck->Concrete.bUserEci )
      {
         pDeck->Concrete.Eci = lrfdConcreteUtil::ModE(pDeck->Concrete.Fci,pDeck->Concrete.StrengthDensity,false);
      }

      if ( !pDeck->Concrete.bUserEc )
      {
         pDeck->Concrete.Ec = lrfdConcreteUtil::ModE(pDeck->Concrete.Fc,pDeck->Concrete.StrengthDensity,false);
      }

      if ( bAfter2015 && pDeck->Concrete.Type == pgsTypes::AllLightweight )
      {
         pDeck->Concrete.Type = pgsTypes::SandLightweight;
      }
   }
}

void CProjectAgentImp::UpdateTimeDependentMaterials()
{
   // The spec was changed from one that did not use time-step analysis to
   // one that does. We want to keep the concrete parameters the same so
   // update them here
   GroupIndexType nGroups = m_BridgeDescription.GetGirderGroupCount();
   for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
   {
      CGirderGroupData* pGroup = m_BridgeDescription.GetGirderGroup(grpIdx);
      GirderIndexType nGirders = pGroup->GetGirderCount();
      for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
      {
         CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);
         SegmentIndexType nSegments = pGirder->GetSegmentCount();
         for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
         {
            CPrecastSegmentData* pSegment = pGirder->GetSegment(segIdx);

            EventIndexType eventIdx = m_BridgeDescription.GetTimelineManager()->GetSegmentErectionEventIndex(pSegment->GetID());
            const CTimelineEvent* pEvent = m_BridgeDescription.GetTimelineManager()->GetEventByIndex(eventIdx);
            Float64 ti = pEvent->GetConstructSegmentsActivity().GetAgeAtRelease();

            pSegment->Material.Concrete.bBasePropertiesOnInitialValues = false;

            pSegment->Material.Concrete.bACIUserParameters = true;
            matACI209Concrete::ComputeParameters(pSegment->Material.Concrete.Fci,ti,pSegment->Material.Concrete.Fc,28.0,&pSegment->Material.Concrete.A,&pSegment->Material.Concrete.B);
            pSegment->Material.Concrete.A = ::ConvertToSysUnits(pSegment->Material.Concrete.A,unitMeasure::Day);

            pSegment->Material.Concrete.bCEBFIPUserParameters = true;
            matCEBFIPConcrete::ComputeParameters(pSegment->Material.Concrete.Fci,ti,pSegment->Material.Concrete.Fc,28.0,&pSegment->Material.Concrete.S);

            CClosureJointData* pClosureJoint = pSegment->GetEndClosure();
            ATLASSERT(pClosureJoint == nullptr); // we can't go from a non-time step method to a time-step method unless we have a regular precast girder bridge. For a regular precast girder bridge, there aren't any closure joints
         }
      }
   }

   // There isn't an f'ci for the deck so we can't curve fit the time-dependent model
   // Do nothing and use the default values...
   //CDeckDescription2* pDeck = m_BridgeDescription.GetDeckDescription();
   //if ( pDeck )
   //{
   //   EventIndexType eventIdx = m_BridgeDescription.GetTimelineManager()->GetCastDeckEventIndex();
   //   const CTimelineEvent* pEvent = m_BridgeDescription.GetTimelineManager()->GetEventByIndex(eventIdx);
   //   Float64 ti = pEvent->GetCastDeckActivity().GetConcreteAgeAtContinuity();

   //   pDeck->Concrete.bBasePropertiesOnInitialValues = false;

   //   pDeck->Concrete.bACIUserParameters = true;
   //   matACI209Concrete::ComputeParameters(pDeck->Concrete.Fci,ti,pDeck->Concrete.Fc,28.0,&pDeck->Concrete.A,&pDeck->Concrete.B);
   //   pDeck->Concrete.A = ::ConvertToSysUnits(pDeck->Concrete.A,unitMeasure::Day);

   //   pDeck->Concrete.bCEBFIPUserParameters = true;
   //   matCEBFIPConcrete::ComputeParameters(pDeck->Concrete.Fci,ti,pDeck->Concrete.Fc,28.0,&pDeck->Concrete.S);
   //}
}

void CProjectAgentImp::UpdateStrandMaterial()
{
   // Get the lookup key for the strand material based on the current units
   lrfdStrandPool* pPool = lrfdStrandPool::GetInstance();

   std::map<CSegmentKey,Int32> strandKeys[3]; // map value is strand pool key, array index is strand type
   std::map<CGirderKey,Int32> tendonKeys; // map value of strand pool key for ducts

   GroupIndexType nGroups = m_BridgeDescription.GetGirderGroupCount();
   for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
   {
      CGirderGroupData* pGroup = m_BridgeDescription.GetGirderGroup(grpIdx);
      GirderIndexType nGirders = pGroup->GetGirderCount();
      for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
      {
         CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);
         CGirderKey girderKey(pGirder->GetGirderKey());

         CPTData* pPTData = pGirder->GetPostTensioning();
         Int32 strand_pool_key = pPool->GetStrandKey(pPTData->pStrand);
         tendonKeys.insert(std::make_pair(girderKey,strand_pool_key));

         SegmentIndexType nSegments = pGirder->GetSegmentCount();
         for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
         {
            CPrecastSegmentData* pSegment = pGirder->GetSegment(segIdx);

            CSegmentKey segmentKey(grpIdx,gdrIdx,segIdx);
            for ( int i = 0; i < 3; i++ )
            {
               pgsTypes::StrandType type = (pgsTypes::StrandType)i;
               const matPsStrand* pStrandMaterial = pSegment->Strands.GetStrandMaterial(type);
               strand_pool_key = pPool->GetStrandKey(pStrandMaterial);
               strandKeys[type].insert(std::make_pair(segmentKey,strand_pool_key));
            }
         }
      }
   }

   // change the units
   lrfdVersionMgr::SetVersion( m_pSpecEntry->GetSpecificationType() );
   lrfdVersionMgr::SetUnits( m_pSpecEntry->GetSpecificationUnits() );

   // Get the new strand material based on the new units
   for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
   {
      CGirderGroupData* pGroup = m_BridgeDescription.GetGirderGroup(grpIdx);
      GirderIndexType nGirders = pGroup->GetGirderCount();
      for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
      {
         CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);
         CGirderKey girderKey(pGirder->GetGirderKey());

         Int32 strand_pool_key = tendonKeys[girderKey];
         CPTData* pPTData = pGirder->GetPostTensioning();
         pPTData->pStrand = pPool->GetStrand(strand_pool_key);

         SegmentIndexType nSegments = pGirder->GetSegmentCount();
         for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
         {
            CPrecastSegmentData* pSegment = pGirder->GetSegment(segIdx);

            CSegmentKey segmentKey(grpIdx,gdrIdx,segIdx);
            for ( int i = 0; i < 3; i++ )
            {
               strand_pool_key = strandKeys[i][segmentKey];
               pSegment->Strands.SetStrandMaterial((pgsTypes::StrandType)i,pPool->GetStrand(strand_pool_key));
            }
         }
      }
   }

   m_bUpdateJackingForce = true;
}

void CProjectAgentImp::VerifyRebarGrade()
{
   GET_IFACE_NOCHECK(IEAFStatusCenter,pStatusCenter);

   GroupIndexType nGroups = m_BridgeDescription.GetGirderGroupCount();
   for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
   {
      CGirderGroupData* pGroup = m_BridgeDescription.GetGirderGroup(grpIdx);
      GirderIndexType nGirders = pGroup->GetGirderCount();
      for (GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
      {
         CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);
         SegmentIndexType nSegments = pGirder->GetSegmentCount();
         for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
         {
            CPrecastSegmentData* pSegment = pGirder->GetSegment(segIdx);

            if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::SixthEditionWith2013Interims && pSegment->LongitudinalRebarData.BarGrade == matRebar::Grade100 )
            {
               CString strMsg;
               strMsg.Format(_T("Grade 100 reinforcement can only be used with %s, %s or later.\nLongitudinal reinforcement for Group %d Girder %s Segment %d has been changed to %s"),
                              lrfdVersionMgr::GetCodeString(),
                              lrfdVersionMgr::GetVersionString(lrfdVersionMgr::SixthEditionWith2013Interims),
                              LABEL_GROUP(grpIdx),LABEL_GIRDER(gdrIdx),LABEL_SEGMENT(segIdx),
                              lrfdRebarPool::GetMaterialName(matRebar::A615,matRebar::Grade60).c_str());
               pgsRebarStrengthStatusItem* pStatusItem = new pgsRebarStrengthStatusItem(pSegment->GetSegmentKey(),pgsRebarStrengthStatusItem::Longitudinal,m_StatusGroupID,m_scidRebarStrengthWarning,strMsg);

               pStatusCenter->Add(pStatusItem);

               pSegment->LongitudinalRebarData.BarType = matRebar::A615;
               pSegment->LongitudinalRebarData.BarGrade = matRebar::Grade60;
            }

            if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::SixthEditionWith2013Interims && pSegment->ShearData.ShearBarGrade == matRebar::Grade100 )
            {
               CString strMsg;
               strMsg.Format(_T("Grade 100 reinforcement can only be used with %s, %s or later.\nTransverse reinforcement for Group %d Girder %s Segment %d has been changed to %s"),
                              lrfdVersionMgr::GetCodeString(),
                              lrfdVersionMgr::GetVersionString(lrfdVersionMgr::SixthEditionWith2013Interims),
                              LABEL_GROUP(grpIdx),LABEL_GIRDER(gdrIdx),LABEL_SEGMENT(segIdx),
                              lrfdRebarPool::GetMaterialName(matRebar::A615,matRebar::Grade60).c_str());
               pgsRebarStrengthStatusItem* pStatusItem = new pgsRebarStrengthStatusItem(pSegment->GetSegmentKey(),pgsRebarStrengthStatusItem::Transverse,m_StatusGroupID,m_scidRebarStrengthWarning,strMsg);

               pStatusCenter->Add(pStatusItem);

               pSegment->ShearData.ShearBarType  = matRebar::A615;
               pSegment->ShearData.ShearBarGrade = matRebar::Grade60;
            }

            CClosureJointData* pClosure = pSegment->GetEndClosure();
            if ( pClosure )
            {
               if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::SixthEditionWith2013Interims && pClosure->GetRebar().BarGrade == matRebar::Grade100 )
               {
                  CString strMsg;
                  strMsg.Format(_T("Grade 100 reinforcement can only be used with %s, %s or later.\nLongitudinal reinforcement for Group %d Girder %s Closure Joint %d has been changed to %s"),
                                 lrfdVersionMgr::GetCodeString(),
                                 lrfdVersionMgr::GetVersionString(lrfdVersionMgr::SixthEditionWith2013Interims),
                                 LABEL_GROUP(grpIdx),LABEL_GIRDER(gdrIdx),LABEL_SEGMENT(segIdx),
                                 lrfdRebarPool::GetMaterialName(matRebar::A615,matRebar::Grade60).c_str());
                  pgsRebarStrengthStatusItem* pStatusItem = new pgsRebarStrengthStatusItem(pClosure->GetClosureKey(),pgsRebarStrengthStatusItem::Longitudinal,m_StatusGroupID,m_scidRebarStrengthWarning,strMsg);

                  pStatusCenter->Add(pStatusItem);

                  pClosure->GetRebar().BarType = matRebar::A615;
                  pClosure->GetRebar().BarGrade = matRebar::Grade60;
               }

               if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::SixthEditionWith2013Interims && pClosure->GetStirrups().ShearBarGrade == matRebar::Grade100 )
               {
                  CString strMsg;
                  strMsg.Format(_T("Grade 100 reinforcement can only be used with %s, %s or later.\nTransverse reinforcement for Group %d Girder %s Closure Joint %d has been changed to %s"),
                                 lrfdVersionMgr::GetCodeString(),
                                 lrfdVersionMgr::GetVersionString(lrfdVersionMgr::SixthEditionWith2013Interims),
                                 LABEL_GROUP(grpIdx),LABEL_GIRDER(gdrIdx),LABEL_SEGMENT(segIdx),
                                 lrfdRebarPool::GetMaterialName(matRebar::A615,matRebar::Grade60).c_str());
                  pgsRebarStrengthStatusItem* pStatusItem = new pgsRebarStrengthStatusItem(pClosure->GetClosureKey(),pgsRebarStrengthStatusItem::Transverse,m_StatusGroupID,m_scidRebarStrengthWarning,strMsg);
                  pStatusCenter->Add(pStatusItem);

                  pClosure->GetStirrups().ShearBarType  = matRebar::A615;
                  pClosure->GetStirrups().ShearBarGrade = matRebar::Grade60;
               }
            }
         } // next segment
      } // next girder
   } // next group

   CDeckDescription2* pDeck = m_BridgeDescription.GetDeckDescription();
   if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::SixthEditionWith2013Interims && pDeck->DeckRebarData.TopRebarGrade == matRebar::Grade100 )
   {
      CString strMsg;
      strMsg.Format(_T("Grade 100 reinforcement can only be used with %s, %s or later.\nDeck reinforcement has been changed to %s"),
                     lrfdVersionMgr::GetCodeString(),
                     lrfdVersionMgr::GetVersionString(lrfdVersionMgr::SixthEditionWith2013Interims,false),
                     lrfdRebarPool::GetMaterialName(matRebar::A615,matRebar::Grade60).c_str());
      pgsRebarStrengthStatusItem* pStatusItem = new pgsRebarStrengthStatusItem(CSegmentKey(),pgsRebarStrengthStatusItem::Deck,m_StatusGroupID,m_scidRebarStrengthWarning,strMsg);

      pStatusCenter->Add(pStatusItem);

      pDeck->DeckRebarData.TopRebarType     = matRebar::A615;
      pDeck->DeckRebarData.TopRebarGrade    = matRebar::Grade60;
      pDeck->DeckRebarData.BottomRebarType  = matRebar::A615;
      pDeck->DeckRebarData.BottomRebarGrade = matRebar::Grade60;
   }
}

void CProjectAgentImp::ValidateBridgeModel()
{
   // Gets called from Fire_BridgeChanged so that every time the bridge gets changed we check the valid status

   GET_IFACE_NOCHECK(IEAFStatusCenter,pStatusCenter);

   if ( m_BridgeStabilityStatusItemID != INVALID_ID )
   {
      pStatusCenter->RemoveByID(m_BridgeStabilityStatusItemID);
      m_BridgeStabilityStatusItemID = INVALID_ID;
   }

   if ( !m_BridgeDescription.IsStable() )
   {
      pgsBridgeDescriptionStatusItem* pStatusItem = new pgsBridgeDescriptionStatusItem(m_StatusGroupID,m_scidBridgeDescriptionWarning,pgsBridgeDescriptionStatusItem::General,_T("Bridge model has geometric instabilities. Modify the boundary conditions."));
      m_BridgeStabilityStatusItemID = pStatusCenter->Add(pStatusItem);
   }
}

STDMETHODIMP CProjectAgentImp::Load(IStructuredLoad* pStrLoad)
{
   HRESULT hr = S_OK;

   m_bUpdateUserDefinedLoads = false; // assume we are loading a newer file and user defined loads don't need tweaking

//   GET_IFACE( IProgress, pProgress );
//   CEAFAutoProgress ap(pProgress);
   IProgress* pProgress = 0; // progress window causes big trouble running in windowless mode

   // Load the library data first into a temporary library. Then deal with entry
   // conflict resolution.
   // This library manager contains data that has been removed from some library entries
   psgLibraryManager temp_manager;
   try
   {
//      pProgress->UpdateMessage( _T(_T("Loading the Project Libraries")) );
      CStructuredLoad load( pStrLoad );
      if ( !temp_manager.LoadMe( &load ) )
      {
         return E_FAIL;
      }
   }
   catch( sysXStructuredLoad& e )
   {
      if ( e.GetExplicitReason() == sysXStructuredLoad::BadVersion )
      {
         WATCH(_T("Project library is new version"));
         return STRLOAD_E_BADVERSION;
      }
      else if ( e.GetExplicitReason() == sysXStructuredLoad::InvalidFileFormat )
      {
         WATCH(_T("Project library has bad format"));
         return STRLOAD_E_INVALIDFORMAT;
      }
      else if ( e.GetExplicitReason() == sysXStructuredLoad::UserDefined )
      {
         WATCH(_T("User defined error"));
         GET_IFACE(IEAFProjectLog,pLog);
         std::_tstring strMsg;
         e.GetErrorMessage(&strMsg);
         pLog->LogMessage(strMsg.c_str());
         return STRLOAD_E_USERDEFINED;
      }
      else
      {
         return E_FAIL;
      }
   }
   catch(...)
   {
      return E_FAIL;
   }

   // merge project library into master library and deal with conflicts
   GET_IFACE(IUpdateTemplates,pUpdateTemplates);
   bool bForceUpdate = pUpdateTemplates->UpdatingTemplates();

   ConflictList the_conflict_list;
   if (!psglibDealWithLibraryConflicts(&the_conflict_list, m_pLibMgr, temp_manager, false, bForceUpdate))
   {
      return E_FAIL;
   }


   // load project data - library conflicts not resolved yet (see call to ResolveLibraryConflicts below)
   STRSTG_LOAD( hr, pStrLoad, pProgress );
   if ( FAILED(hr) )
   {
      return hr;
   }

   // there were a couple of settings moved from the spec library entry into the main program
   // if the values for this project are still in the library entry, get them now
   const SpecLibrary* pTempSpecLib = temp_manager.GetSpecLibrary();
   const SpecLibraryEntry* pTempSpecEntry = pTempSpecLib->LookupEntry( m_Spec.c_str() );
   if ( m_bGetAnalysisTypeFromLibrary )
   {
      m_AnalysisType = pTempSpecEntry->GetAnalysisType();
   }

   if ( m_bGetIgnoreROAFromLibrary )
   {
      m_LldfRangeOfApplicabilityAction = pTempSpecEntry->IgnoreRangeOfApplicabilityRequirements() ? roaIgnore : roaEnforce;
   }

   // get the old haul truck configuration here because we have the library entry
   // though we aren't going to use it until later
   // want to release the library entry, so it doesn't have a bad ref count if we leave this method early
   const COldHaulTruck* pOldHaulTruck = pTempSpecEntry->GetOldHaulTruck();

   pTempSpecEntry->Release();

   // resolve library name conflicts and update references
   if (!ResolveLibraryConflicts(the_conflict_list))
   {
      return E_FAIL;
   }

   // check to see if the bridge girder spacing type is correct
   // because of changes in the way girder spacing is modeled, the girder spacing
   // type can take on invalid values... some girders now use the top width and joint spacing
   // spacing type rather than the adjacent girder spacing type.
   // this seems to be the best place to convert the spacing
   //

   CGirderGroupData* pGroup = m_BridgeDescription.GetGirderGroup(GroupIndexType(0));
   CSplicedGirderData* pGirder = pGroup->GetGirder(0);
   const GirderLibraryEntry* pEntry = pGirder->GetGirderLibraryEntry();
   CComPtr<IBeamFactory> beamFactory;
   pEntry->GetBeamFactory(&beamFactory);
   pgsTypes::SupportedBeamSpacings sbs = beamFactory->GetSupportedBeamSpacings();

   pgsTypes::SupportedBeamSpacing spacingType = m_BridgeDescription.GetGirderSpacingType();
   pgsTypes::SupportedBeamSpacings::iterator found = std::find(sbs.begin(), sbs.end(), spacingType);

   if (!beamFactory->IsSupportedBeamSpacing(spacingType))
   {
      if (spacingType == pgsTypes::sbsConstantAdjacent && !m_BridgeDescription.UseSameGirderForEntireBridge())
      {
         // an error in the previous version allowed inconsistent girder types to be modeled
         // if girders have a constant adjacent spacing, and different girders are used throughout
         // the bridge, the spacing isn't going to be consistent... force the bridge model into something
         // we can handle

         ReleaseGirderLibraryEntries();

         m_BridgeDescription.SetGirderName(pGirder->GetGirderName());
         m_BridgeDescription.UseSameGirderForEntireBridge(true);
         m_BridgeDescription.CopyDown(false, true, false, false, false, false);

         UseGirderLibraryEntries();
      
         GET_IFACE(IEAFStatusCenter, pStatusCenter);
         AfxMessageBox(_T("Your bridge model contains incompatible girder types. The model will be changed to use the same girder for the entire bridge."));

         CString strMsg(_T("Your bridge model contained incompatible girder types. The model has been changed to use the same girder for the entire bridge."));
         pgsBridgeDescriptionStatusItem* pStatusItem = new pgsBridgeDescriptionStatusItem(m_StatusGroupID, m_scidBridgeDescriptionWarning, pgsBridgeDescriptionStatusItem::General,strMsg);
         pStatusCenter->Add(pStatusItem);
      }

      // we don't have a valid spacing type, convert it
      if (m_BridgeDescription.UseSameGirderForEntireBridge())
      {
         pgsTypes::SupportedBeamSpacing newSpacingType;
         Float64 newSpacing, topWidth;

         GirderLibraryEntry::Dimensions dimensions = m_BridgeDescription.GetGirderLibraryEntry()->GetDimensions();

         VERIFY(beamFactory->ConvertBeamSpacing(dimensions, spacingType, m_BridgeDescription.GetGirderSpacing(), &newSpacingType, &newSpacing, &topWidth));
         m_BridgeDescription.SetGirderSpacingType(newSpacingType);
         m_BridgeDescription.SetGirderSpacing(newSpacing);
         m_BridgeDescription.SetGirderTopWidth(pgsTypes::twtSymmetric, topWidth, 0.0);
      }
      else
      {
         GroupIndexType nGroups = m_BridgeDescription.GetGirderGroupCount();
         for (GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++)
         {
            CGirderGroupData* pGroup = m_BridgeDescription.GetGirderGroup(grpIdx);
            GirderIndexType nGirders = pGroup->GetGirderCount();
            for (GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++)
            {
               CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);
               GirderLibraryEntry::Dimensions dimensions = pGirder->GetGirderLibraryEntry()->GetDimensions();

               pgsTypes::SupportedBeamSpacing newSpacingType;
               Float64 newSpacing, topWidth;

               VERIFY(beamFactory->ConvertBeamSpacing(dimensions, spacingType, m_BridgeDescription.GetGirderSpacing(), &newSpacingType, &newSpacing, &topWidth));
               if (grpIdx == 0 && gdrIdx == 0)
               {
                  m_BridgeDescription.SetGirderSpacingType(newSpacingType);
               }

               if (gdrIdx != nGirders - 1)
               {
                  CGirderSpacingData2* pStartSpacing = pGroup->GetPier(pgsTypes::metStart)->GetGirderSpacing(pgsTypes::Ahead);
                  GroupIndexType spacingGrpIdx = pStartSpacing->GetSpacingGroupIndex(gdrIdx);
                  pStartSpacing->SetGirderSpacing(spacingGrpIdx, newSpacing);

                  CGirderSpacingData2* pEndSpacing = pGroup->GetPier(pgsTypes::metEnd)->GetGirderSpacing(pgsTypes::Back);
                  spacingGrpIdx = pEndSpacing->GetSpacingGroupIndex(gdrIdx);
                  pEndSpacing->SetGirderSpacing(spacingGrpIdx, newSpacing);
               }
               pGirder->SetTopWidth(pgsTypes::twtSymmetric, topWidth, 0, topWidth, 0);
            }
         }
      }

      m_BridgeDescription.CopyDown(false, false, true, false, false,false); // copy the joint spacing down to the individual span/pier level

      if (m_pSpecEntry->GetLossMethod() != LOSSES_TIME_STEP)
      {
         // spacing type is a factor in the timeline... better update it
         CreatePrecastGirderBridgeTimelineEvents();
      }
   }
   else
   {
      if ( IsJointSpacing(m_BridgeDescription.GetGirderSpacingType()) )
      {
         // input is girder spacing, but should be joint width... get the girder width and subtract if from the girder spacing
         // the result will be the joint width

         // NOTE: The problem this code is correcting occur in versions prior to the TopWidth input for deck bulb tee and other similar girders,
         // therefore we don't have to worry that CreateGirderSection and get_TopWidth and get_BottomWidth only give nominal values for these
         // girder types.
         GirderLibraryEntry::Dimensions dimensions = pEntry->GetDimensions();
         CComPtr<IGirderSection> gdrSection;
         beamFactory->CreateGirderSection(nullptr,0,dimensions,-1,-1,&gdrSection);
         Float64 topWidth, botWidth;
         gdrSection->get_TopWidth(&topWidth);
         gdrSection->get_BottomWidth(&botWidth);
         Float64 width = Max(topWidth,botWidth);
         Float64 jointWidth = m_BridgeDescription.GetGirderSpacing() - width;
         jointWidth = IsZero(jointWidth) ? 0 : jointWidth;

         if ( 0 <= jointWidth )
         {
            m_BridgeDescription.SetGirderSpacing(jointWidth);
            m_BridgeDescription.CopyDown(false,false,IsBridgeSpacing(m_BridgeDescription.GetGirderSpacingType()),false,false,false); // copy the joint spacing down
         }
      }
   }


   if (!beamFactory->IsSupportedGirderOrientation(m_BridgeDescription.GetGirderOrientation()))
   {
      m_BridgeDescription.SetGirderOrientation(beamFactory->ConvertGirderOrientation(m_BridgeDescription.GetGirderOrientation()));
   }
  
   // old alignment offset was read... copy it to all the piers
   if ( m_bUseTempAlignmentOffset )
   {
      m_BridgeDescription.SetAlignmentOffset(m_AlignmentOffset_Temp);
   }

   // make sure no library problems snuck in the back door.
   DealWithGirderLibraryChanges(true);

   // Deal with missing library entries
   bool bUpdateLibraryUsage = false;

   ConcreteLibrary& concLib = GetConcreteLibrary();
   ATLASSERT(concLib.GetMinCount() == 0); // did this change???

   GirderLibrary& gdrLib = GetGirderLibrary();
   CollectionIndexType nEntries    = gdrLib.GetCount();
   CollectionIndexType nMinEntries = gdrLib.GetMinCount();
   if ( nEntries < nMinEntries )
   {
      CString strMsg;
      strMsg.Format(_T("The %s library needs at least %d entries. Default entries have been created."),gdrLib.GetDisplayName().c_str(),nMinEntries);
      AfxMessageBox(strMsg,MB_OK | MB_ICONEXCLAMATION);
      for ( CollectionIndexType i = 0; i < (nMinEntries-nEntries); i++ )
      {
         gdrLib.NewEntry(gdrLib.GetUniqueEntryName().c_str());
      }

      bUpdateLibraryUsage = true;

      libKeyListType keys;
      gdrLib.KeyList(keys);
      std::_tstring strGirderName = keys.front();
      if (m_BridgeDescription.UseSameGirderForEntireBridge())
      {
         m_BridgeDescription.SetGirderName(strGirderName.c_str());
      }
      else
      {
         GroupIndexType nGroups = m_BridgeDescription.GetGirderGroupCount();
         for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
         {
            CGirderGroupData* pGroup = m_BridgeDescription.GetGirderGroup(grpIdx);
            GroupIndexType nGirderTypeGroups = pGroup->GetGirderTypeGroupCount();
            for ( GroupIndexType girderTypeGroupIdx = 0; girderTypeGroupIdx < nGirderTypeGroups; girderTypeGroupIdx++ )
            {
               pGroup->SetGirderName(girderTypeGroupIdx,strGirderName.c_str());
            }
         }
      }
   }


   TrafficBarrierLibrary& tbLib = GetTrafficBarrierLibrary();
   nEntries    = tbLib.GetCount();
   nMinEntries = tbLib.GetMinCount();
   if ( nEntries < nMinEntries )
   {
      CString strMsg;
      strMsg.Format(_T("The %s library needs at least %d entries. Default entries have been created."),tbLib.GetDisplayName().c_str(),nMinEntries);
      AfxMessageBox(strMsg,MB_OK | MB_ICONEXCLAMATION);
      for ( CollectionIndexType i = 0; i < (nMinEntries-nEntries); i++ )
      {
         tbLib.NewEntry(tbLib.GetUniqueEntryName().c_str());
      }

      bUpdateLibraryUsage = true;

      libKeyListType keys;
      tbLib.KeyList(keys);
      std::_tstring strBarrierName = keys.front();
      CRailingSystem* pLeftRailing = m_BridgeDescription.GetLeftRailingSystem();
      pLeftRailing->strExteriorRailing = strBarrierName;
      if ( pLeftRailing->bUseInteriorRailing )
      {
         pLeftRailing->strInteriorRailing = strBarrierName;
      }

      CRailingSystem* pRightRailing = m_BridgeDescription.GetRightRailingSystem();
      pRightRailing->strExteriorRailing = strBarrierName;
      if ( pRightRailing->bUseInteriorRailing )
      {
         pRightRailing->strInteriorRailing = strBarrierName;
      }
   }

   SpecLibrary* pSpecLib = GetSpecLibrary();
   nEntries    = pSpecLib->GetCount();
   nMinEntries = pSpecLib->GetMinCount();
   if ( nEntries < nMinEntries )
   {
      CString strMsg;
      strMsg.Format(_T("The %s library needs at least %d entries. Default entries have been created."),pSpecLib->GetDisplayName().c_str(),nMinEntries);
      AfxMessageBox(strMsg,MB_OK | MB_ICONEXCLAMATION);
      for ( CollectionIndexType i = 0; i < (nMinEntries-nEntries); i++ )
      {
         pSpecLib->NewEntry(pSpecLib->GetUniqueEntryName().c_str());
      }

      libKeyListType keys;
      pSpecLib->KeyList(keys);
      std::_tstring strSpecName = keys.front();
      InitSpecification(strSpecName);
   }

   RatingLibrary* pRatingLib = GetRatingLibrary();
   nEntries    = pRatingLib->GetCount();
   nMinEntries = pRatingLib->GetMinCount();
   if ( nEntries < nMinEntries )
   {
      CString strMsg;
      strMsg.Format(_T("The %s library needs at least %d entries. Default entries have been created."),pRatingLib->GetDisplayName().c_str(),nMinEntries);
      AfxMessageBox(strMsg,MB_OK | MB_ICONEXCLAMATION);
      for ( CollectionIndexType i = 0; i < (nMinEntries-nEntries); i++ )
      {
         pRatingLib->NewEntry(pRatingLib->GetUniqueEntryName().c_str());
      }

      libKeyListType keys;
      pRatingLib->KeyList(keys);
      std::_tstring strSpecName = keys.front();
      InitRatingSpecification(strSpecName);
   }

   GET_IFACE(IDocumentType, pDocType);
   if (pDocType->IsPGSpliceDocument())
   {
      DuctLibrary* pDuctLibrary = GetDuctLibrary();
      nEntries = pDuctLibrary->GetCount();
      nMinEntries = pDuctLibrary->GetMinCount();
      if (nEntries < nMinEntries)
      {
         for (CollectionIndexType i = 0; i < (nMinEntries - nEntries); i++)
         {
            pDuctLibrary->NewEntry(pDuctLibrary->GetUniqueEntryName().c_str());
         }


         GET_IFACE(IEAFStatusCenter, pStatusCenter);
         CString strMsg;
         strMsg.Format(_T("The %s library needs at least %d entries. Default entries have been created."), pDuctLibrary->GetDisplayName().c_str(), nMinEntries);

         pgsInformationalStatusItem* pStatusItem = new pgsInformationalStatusItem(m_StatusGroupID, m_scidBridgeDescriptionInfo, strMsg);
         pStatusCenter->Add(pStatusItem);

         bUpdateLibraryUsage = true;

         libKeyListType keys;
         pDuctLibrary->KeyList(keys);
         std::_tstring strDuctName = keys.front();
         GroupIndexType nGroups = m_BridgeDescription.GetGirderGroupCount();
         for (GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++)
         {
            CGirderGroupData* pGroup = m_BridgeDescription.GetGirderGroup(grpIdx);
            GirderIndexType nGirders = pGroup->GetGirderCount();
            for (GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++)
            {
               CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);
               CPTData* pPTData = pGirder->GetPostTensioning();
               DuctIndexType nDucts = pPTData->GetDuctCount();
               for (DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++)
               {
                  CDuctData* pDuct = pPTData->GetDuct(ductIdx);
                  pDuct->Name = strDuctName;
               }
            }
         }
      }
   }

   if (pOldHaulTruck)
   {
      UpdateHaulTruck(pOldHaulTruck);
   }

   HaulTruckLibrary* pHaulTruckLibrary = GetHaulTruckLibrary();
   nEntries = pHaulTruckLibrary->GetCount();
   nMinEntries = pHaulTruckLibrary->GetMinCount();
   if ( nEntries < nMinEntries )
   {
      for ( CollectionIndexType i = 0; i < (nMinEntries-nEntries); i++ )
      {
         pHaulTruckLibrary->NewEntry(pHaulTruckLibrary->GetUniqueEntryName().c_str());
      }

      GET_IFACE(IEAFStatusCenter, pStatusCenter);
      CString strMsg;
      strMsg.Format(_T("The %s library needs at least %d entries. Default entries have been created."), pHaulTruckLibrary->GetDisplayName().c_str(), nMinEntries);

      pgsInformationalStatusItem* pStatusItem = new pgsInformationalStatusItem(m_StatusGroupID, m_scidBridgeDescriptionInfo, strMsg);
      pStatusCenter->Add(pStatusItem);

      bUpdateLibraryUsage = true;

      libKeyListType keys;
      pHaulTruckLibrary->KeyList(keys);
      std::_tstring strHaulTruckName = keys.front();
      GroupIndexType nGroups = m_BridgeDescription.GetGirderGroupCount();
      for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
      {
         CGirderGroupData* pGroup = m_BridgeDescription.GetGirderGroup(grpIdx);
         GirderIndexType nGirders = pGroup->GetGirderCount();
         for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
         {
            CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);
            SegmentIndexType nSegments = pGirder->GetSegmentCount();
            for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
            {
               CPrecastSegmentData* pSegment = pGirder->GetSegment(segIdx);
               pSegment->HandlingData.HaulTruckName = strHaulTruckName;
            }
         }
      }
   }

   LiveLoadLibrary* pLiveLoadLib = GetLiveLoadLibrary();
   ATLASSERT(pLiveLoadLib->GetMinCount() == 0); // did this change???

   if ( bUpdateLibraryUsage )
   {
      ReleaseBridgeLibraryEntries();
      UseBridgeLibraryEntries();
   }

   GirderLibrary& girderLibrary = GetGirderLibrary();
   libKeyListType keyList;
   girderLibrary.KeyList(keyList);
   for (const auto& key : keyList)
   {
      const libLibraryEntry* pEntry = girderLibrary.GetEntry(key.c_str());
      const GirderLibraryEntry* pGirderEntry = (GirderLibraryEntry*)pEntry;
      if (0 < pGirderEntry->GetRefCount())
      {
         // this girder type is in use
         CComPtr<IBeamFactory> beamFactory;
         pGirderEntry->GetBeamFactory(&beamFactory);

         CComQIPtr<IBeamFactoryCompatibility> compatibility(beamFactory);
         if (compatibility)
         {
            // the beam factory wants to do compatibility updates
            compatibility->UpdateBridgeModel(&m_BridgeDescription, pGirderEntry);
         }
      }
   }

   SpecificationChanged(false);  
   RatingSpecificationChanged(false);

   CString strBadLoads = m_LoadManager.FixBadLoads();
   if ( !strBadLoads.IsEmpty() )
   {
      CString strMsg;
      strMsg.Format(_T("%s\r\n\r\nSee Status Center for Details"),strBadLoads);
      AfxMessageBox(strMsg,MB_OK | MB_ICONEXCLAMATION);
      GET_IFACE(IEAFStatusCenter,pStatusCenter);
      pgsInformationalStatusItem* pStatusItem =  new pgsInformationalStatusItem(m_StatusGroupID,m_scidLoadDescriptionWarning,strBadLoads);
      pStatusCenter->Add(pStatusItem);

      GET_IFACE(IEAFDocument,pDoc);
      pDoc->SetModified();
   }

   if (IsNonstructuralDeck(m_BridgeDescription.GetDeckDescription()->GetDeckType()))
   {
      CPierData2* pPier = m_BridgeDescription.GetPier(0);
      bool bBCChanged = false;
      while (pPier != nullptr)
      {
         if (pPier->IsBoundaryPier())
         {
            pgsTypes::BoundaryConditionType bc = pPier->GetBoundaryConditionType();
            if (!IsNoDeckBoundaryCondition(bc))
            {
               pgsTypes::BoundaryConditionType newBC = GetNoDeckBoundaryCondition(bc);
               pPier->SetBoundaryConditionType(newBC);

               CString strMsg;
               strMsg.Format(_T("The \"%s\" boundary condition at %s %d is not compatible with the bridge model. The boundary condition has been changed to \"%s\"."), 
                  CPierData2::AsString(bc),
                  (pPier->IsAbutment() ? _T("Abutment") : _T("Pier")),
                  LABEL_PIER(pPier->GetIndex()),
                  CPierData2::AsString(newBC,true)
                  );
               GET_IFACE(IEAFStatusCenter, pStatusCenter);
               pgsInformationalStatusItem* pStatusItem = new pgsInformationalStatusItem(m_StatusGroupID, m_scidBridgeDescriptionInfo, strMsg);
               pStatusCenter->Add(pStatusItem);

               GET_IFACE(IEAFDocument, pDoc);
               pDoc->SetModified();

               bBCChanged = true;
            }
         }

         CSpanData2* pSpan = pPier->GetNextSpan();
         if (pSpan)
         {
            pPier = pSpan->GetNextPier();
         }
         else
         {
            pPier = nullptr;
         }
      }

      if ( bBCChanged )
      {
         AfxMessageBox(_T("Some boundary conditions were not compatible with the bridge model. The boundary conditions have been updated. See Status Center for more information."), MB_OK | MB_ICONEXCLAMATION);
      }
   }

   // Check if version 3.1 fillet data was read and corrected
   if (m_BridgeDescription.WasVersion3_1FilletRead() && m_BridgeDescription.GetDeckDescription()->GetDeckType() != pgsTypes::sdtNone)
   {
      Float64 fillval = ::ConvertFromSysUnits(m_BridgeDescription.GetFillet(), unitMeasure::Inch);
      CString strMsg;
      strMsg.Format(_T("Multiple fillet values were input in this file and are no longer supported in this version of PGSuper. The max value will be used. A single fillet value of %.3fin will be set for the entire bridge.\r\n\r\nSee Status Center for Details"), fillval);
//      AfxMessageBox(strMsg, MB_OK | MB_ICONEXCLAMATION);
      GET_IFACE(IEAFStatusCenter, pStatusCenter);
      pgsInformationalStatusItem* pStatusItem = new pgsInformationalStatusItem(m_StatusGroupID, m_scidBridgeDescriptionInfo, strMsg);
      pStatusCenter->Add(pStatusItem);
   }

   ValidateBridgeModel();

   ASSERTVALID;

   // make sure default bearing data is as up to date as possible
   UpgradeBearingData();

   Fire_BridgeChanged();

   return (the_conflict_list.AreThereAnyConflicts() ? S_FALSE : S_OK);
}

STDMETHODIMP CProjectAgentImp::Save(IStructuredSave* pStrSave)
{
   HRESULT hr = S_OK;

   GET_IFACE( IProgress, pProgress );
   CEAFAutoProgress ap(pProgress);

   //
   // Save the library data first
   // Want to save only entries that are project-specific (not read-only) and 
   // entries which are referenced. Do this by creating a temp copy of the library
   // with only these entries
   psgLibraryManager temp_manager;
   psglibMakeSaveableCopy(*m_pLibMgr, &temp_manager);


   try
   {
      pProgress->UpdateMessage( _T("Saving the Project Libraries") );
      CStructuredSave save( pStrSave );
      temp_manager.SaveMe( &save );
   } 
   catch(...)
   {
      return E_FAIL;
   }

   STRSTG_SAVE( hr, pStrSave, pProgress );



   return hr;
}

void CProjectAgentImp::ValidateStrands(const CSegmentKey& segmentKey,CPrecastSegmentData* pSegment,bool fromLibrary)
{
   if ( pSegment->Strands.GetStrandDefinitionType() == CStrandData::sdtDirectInput )
   {
      // user defined strands don't use strand information from the library
      return;
   }

   std::_tstring segmentLabel = SEGMENT_LABEL(segmentKey);

   const GirderLibraryEntry* pGirderEntry = pSegment->GetGirder()->GetGirderLibraryEntry();
   // If library entry forbids offset, reset vertical adjustment of harped strands to zero (default). This will partially avoid getting
   // bad data into input when adjustment is turned back on. No need to tell user
   // Bug: This however, does not handle the case when tighter offsets limits are input into library than are set in project data.
   //      Catching this would be considerably more work.
   if (pSegment->Strands.GetAdjustableStrandType() == pgsTypes::asHarped)
   {
      if (!pGirderEntry->IsVerticalAdjustmentAllowedEnd() && pSegment->Strands.GetHarpStrandOffsetMeasurementAtEnd() != hsoLEGACY)
      {
         pSegment->Strands.SetHarpStrandOffsetAtEnd(pgsTypes::metStart,0.0);
         pSegment->Strands.SetHarpStrandOffsetAtEnd(pgsTypes::metEnd,  0.0);
         pSegment->Strands.SetHarpStrandOffsetMeasurementAtEnd(hsoLEGACY);
      }

      if (!pGirderEntry->IsVerticalAdjustmentAllowedHP() && pSegment->Strands.GetHarpStrandOffsetMeasurementAtHarpPoint() != hsoLEGACY)
      {
         pSegment->Strands.SetHarpStrandOffsetAtHarpPoint(pgsTypes::metStart,0.0);
         pSegment->Strands.SetHarpStrandOffsetAtHarpPoint(pgsTypes::metEnd,  0.0);
         pSegment->Strands.SetHarpStrandOffsetMeasurementAtHarpPoint(hsoLEGACY);
      }
   }
   else
   {
      if (!pGirderEntry->IsVerticalAdjustmentAllowedStraight() && pSegment->Strands.GetHarpStrandOffsetMeasurementAtEnd() != hsoLEGACY)
      {
         pSegment->Strands.SetHarpStrandOffsetAtEnd(pgsTypes::metStart,0.0);
         pSegment->Strands.SetHarpStrandOffsetAtEnd(pgsTypes::metEnd,  0.0);
         pSegment->Strands.SetHarpStrandOffsetMeasurementAtEnd(hsoLEGACY);
      }
   }

   // There are many, many ways that strand data can get hosed if a library entry is changed for an existing project. 
   // If strands no longer fit as original, zero them out and inform user.
   bool clean = true;
   if (pSegment->Strands.GetStrandDefinitionType() == CStrandData::sdtDirectSelection )
   {
      // Direct Fill
      bool vst = IsValidStraightStrandFill(pSegment->Strands.GetDirectStrandFillStraight(), pGirderEntry);
      bool vhp = IsValidHarpedStrandFill(pSegment->Strands.GetDirectStrandFillHarped(), pGirderEntry);
      bool vtp = IsValidTemporaryStrandFill(pSegment->Strands.GetDirectStrandFillTemporary(), pGirderEntry);

      if ( !(vst&&vhp&&vtp) )
      {
         std::_tstring msg(_T("Direct filled strands no longer fit in girder because library entry changed. All strands were removed."));
         AddSegmentStatusItem(segmentKey, msg);

         clean = false;
         pSegment->Strands.ResetPrestressData();
      }

      // Check validity of debond data for direct filled strands
      bool debond_changed(false);
      std::vector<CDebondData>& vDebond(pSegment->Strands.GetDebonding(pgsTypes::Straight));
      if ( !vDebond.empty() )
      {
         StrandIndexType nStrandCoordinates = pGirderEntry->GetNumStraightStrandCoordinates();

         // Make sure selected strands are filled and debondable
         std::vector<CDebondData>::iterator it(vDebond.begin());
         std::vector<CDebondData>::iterator itend(vDebond.end());
         while(it!=itend)
         {
            bool can_db = true;

            // Get strand index and check if debonded strand is actually filled
            if( ! pSegment->Strands.GetDirectStrandFillStraight()->IsStrandFilled( it->strandTypeGridIdx ) )
            {
               can_db = false;
            }

            // Make sure strand fits in grid
            if (it->strandTypeGridIdx >= nStrandCoordinates)
            {
               can_db = false;
            }

            if (can_db)
            {
               // Check if strand can be debonded
               Float64 start_x,start_y,end_x,end_y;
               pGirderEntry->GetStraightStrandCoordinates(it->strandTypeGridIdx, &start_x, &start_y, &end_x, &end_y, &can_db);
            }

            if(!can_db)
            {
               // Erase invalid debond
               vDebond.erase(it);

               debond_changed = true;

               // restart loop
               it    = vDebond.begin();
               itend = vDebond.end();
            }
            else
            {
               it++;
            }
         }

         clean &= !debond_changed;

         if (fromLibrary && debond_changed) // no message if strands where trimmed in project due to a strand reduction
         {
            std::_tostringstream msg;
            msg << segmentLabel << _T(" specified debonding that is not allowed by the library entry. The invalid debond regions were removed.");
            AddSegmentStatusItem(segmentKey, msg.str());
         }
      }
   }
   else
   {
      if (pSegment->Strands.GetStrandDefinitionType() == CStrandData::sdtTotal )
      {
         // make sure number of strands fits library
         StrandIndexType ns, nh;
         bool st = pGirderEntry->GetPermStrandDistribution(pSegment->Strands.GetStrandCount(pgsTypes::Permanent), &ns, &nh);

         if (!st || pSegment->Strands.GetStrandCount(pgsTypes::Straight) !=ns || pSegment->Strands.GetStrandCount(pgsTypes::Harped) != nh)
         {
            std::_tostringstream msg;
            msg << segmentLabel << _T(": ") << pSegment->Strands.GetStrandCount(pgsTypes::Permanent)<<_T(" permanent strands no longer valid because library entry changed. All strands were removed.");
            AddSegmentStatusItem(segmentKey, msg.str());

            clean = false;
            pSegment->Strands.ResetPrestressData();
         }
      }
      else if (pSegment->Strands.GetStrandDefinitionType() == CStrandData::sdtStraightHarped ) // input is by straight/harped
      {
         bool vst = pGirderEntry->IsValidNumberOfStraightStrands(pSegment->Strands.GetStrandCount(pgsTypes::Straight));
         bool vhp = pGirderEntry->IsValidNumberOfHarpedStrands(pSegment->Strands.GetStrandCount(pgsTypes::Harped));

         if ( !(vst&&vhp) )
         {
            std::_tostringstream msg;
            msg << segmentLabel << _T(": ") << pSegment->Strands.GetStrandCount(pgsTypes::Straight) <<_T(" straight, ") << pSegment->Strands.GetStrandCount(pgsTypes::Harped) << _T(" harped strands is no longer valid because library entry changed. All strands were removed.");
            AddSegmentStatusItem(segmentKey, msg.str());

            clean = false;
            pSegment->Strands.ResetPrestressData();
         }
      }
      else
      {
         ATLASSERT(false); // should never get here
      }

      // Temporary Strands
      bool vhp = pGirderEntry->IsValidNumberOfTemporaryStrands(pSegment->Strands.GetStrandCount(pgsTypes::Temporary));
      if ( !vhp )
      {
         std::_tostringstream msg;
         msg<< segmentLabel << _T(": ") << pSegment->Strands.GetStrandCount(pgsTypes::Temporary) << _T(" temporary strands are no longer valid because library entry changed. All temporary strands were removed.");
         AddSegmentStatusItem(segmentKey, msg.str());

         clean = false;
         pSegment->Strands.ResetPrestressData();
      }

      // Debond information for sequentially filled strands
      if (clean)
      {
         // check validity of debond data - this can come from library, or project changes
         std::vector<CDebondData>& vDebond(pSegment->Strands.GetDebonding(pgsTypes::Straight));
         if ( !vDebond.empty() )
         {
            StrandIndexType nStrandCoordinates = pGirderEntry->GetNumStraightStrandCoordinates();

            // first build list of debondable strand grid indices
            std::vector<bool> debonds;
            debonds.reserve(nStrandCoordinates);

            StrandIndexType gridIndex = 0;
            while (gridIndex < nStrandCoordinates)
            {
               Float64 start_x,start_y,end_x,end_y;
               bool can_db;
               pGirderEntry->GetStraightStrandCoordinates(gridIndex, &start_x, &start_y, &end_x, &end_y, &can_db);

               debonds.push_back(can_db);
               gridIndex++;
            }
            
            // now walk list to see if we have only debonded valid strands
            bool debond_changed = false;
            std::vector<CDebondData>::iterator iter(vDebond.begin());
            std::vector<CDebondData>::iterator end(vDebond.end());
            for ( ; iter != end;  )
            {
               bool good=true;
               CDebondData& debond_info = *iter;

               if (debond_info.strandTypeGridIdx < nStrandCoordinates)
               {
                  good = debonds.at(debond_info.strandTypeGridIdx);
               }
               else
               {
                  good = false; // strand out of range
               }

               if (!good)
               {
                  // remove invalid debonding
                  debond_changed=true;
                  iter = vDebond.erase(iter);
                  end = vDebond.end();
               }
               else
               {
                  iter++;
               }
            }

            clean &= !debond_changed;

            if (fromLibrary && debond_changed) // no message if strands where trimmed in project due to a strand reduction
            {
               std::_tostringstream msg;
               msg << segmentLabel << _T(" specified debonding that is not allowed by the library entry. The invalid debond regions were removed.");
               AddSegmentStatusItem(segmentKey, msg.str());
            }
         }
      }
   }

   if (clean)
   {
      // clear out status items on a clean pass, because we made them
      RemoveSegmentStatusItems(segmentKey);
   }
}

//////////////////////////////////////////////////////////////////////
// IProjectProperties
LPCTSTR CProjectAgentImp::GetBridgeName() const
{
   return m_BridgeName.c_str();
}

void CProjectAgentImp::SetBridgeName(LPCTSTR name)
{
   if ( m_BridgeName != name )
   {
      m_BridgeName = name;
      Fire_ProjectPropertiesChanged();
   }
}

LPCTSTR CProjectAgentImp::GetBridgeID() const
{
   return m_BridgeID.c_str();
}

void CProjectAgentImp::SetBridgeID(LPCTSTR bid)
{
   if ( m_BridgeID != bid )
   {
      m_BridgeID = bid;
      Fire_ProjectPropertiesChanged();
   }
}

LPCTSTR CProjectAgentImp::GetJobNumber() const
{
   return m_JobNumber.c_str();
}

void CProjectAgentImp::SetJobNumber(LPCTSTR jid)
{
   if ( m_JobNumber != jid )
   {
      m_JobNumber = jid;
      Fire_ProjectPropertiesChanged();
   }
}

LPCTSTR CProjectAgentImp::GetEngineer() const
{
   return m_Engineer.c_str();
}

void CProjectAgentImp::SetEngineer(LPCTSTR eng)
{
   if ( m_Engineer != eng )
   {
      m_Engineer = eng;
      Fire_ProjectPropertiesChanged();
   }
}

LPCTSTR CProjectAgentImp::GetCompany() const
{
   return m_Company.c_str();
}

void CProjectAgentImp::SetCompany(LPCTSTR company)
{
   if ( m_Company != company )
   {
      m_Company = company;
      Fire_ProjectPropertiesChanged();
   }
}

LPCTSTR CProjectAgentImp::GetComments() const
{
   return m_Comments.c_str();
}

void CProjectAgentImp::SetComments(LPCTSTR comments)
{
   if ( m_Comments != comments )
   {
      m_Comments = comments;
      Fire_ProjectPropertiesChanged();
   }
}

////////////////////////////////////////////////////////////////////////
// IEnviroment
//
enumExposureCondition CProjectAgentImp::GetExposureCondition() const
{
   return m_ExposureCondition;
}

void CProjectAgentImp::SetExposureCondition(enumExposureCondition newVal)
{
   if ( m_ExposureCondition != newVal )
   {
      m_ExposureCondition = newVal;
      Fire_ExposureConditionChanged();
   }
}

Float64 CProjectAgentImp::GetRelHumidity() const
{
   return m_RelHumidity;
}

void CProjectAgentImp::SetRelHumidity(Float64 newVal)
{
   ATLASSERT( 0 <= newVal && newVal <= 100 );

   if ( !IsEqual( m_RelHumidity, newVal ) )
   {
      m_RelHumidity = newVal;
      Fire_RelHumidityChanged();
   }
}

////////////////////////////////////////////////////////////////////////
// IRoadwayData
//
void CProjectAgentImp::SetAlignmentData2(const AlignmentData2& data)
{
   if ( m_AlignmentData2 != data )
   {
      m_AlignmentData2 = data;
      Fire_BridgeChanged();
   }
}

AlignmentData2 CProjectAgentImp::GetAlignmentData2() const
{
   return m_AlignmentData2;
}

void CProjectAgentImp::SetProfileData2(const ProfileData2& data)
{
   if ( m_ProfileData2 != data )
   {
      m_ProfileData2 = data;
      Fire_BridgeChanged();
   }
}

ProfileData2 CProjectAgentImp::GetProfileData2() const
{
   return m_ProfileData2;
}

void CProjectAgentImp::SetRoadwaySectionData(const RoadwaySectionData& data)
{
   if ( m_RoadwaySectionData != data )
   {
      m_RoadwaySectionData = data;
      Fire_BridgeChanged();
   }
}

RoadwaySectionData CProjectAgentImp::GetRoadwaySectionData() const
{
   return m_RoadwaySectionData;
}

////////////////////////////////////////////////////////////////////////
// IBridgeDescription
const CBridgeDescription2* CProjectAgentImp::GetBridgeDescription() const
{
   return &m_BridgeDescription;
}

void CProjectAgentImp::SetBridgeDescription(const CBridgeDescription2& desc)
{
   if ( m_BridgeDescription != desc )
   {
      ReleaseBridgeLibraryEntries();

      m_BridgeDescription = desc; // make an assignement... copies everything including IDs and Indices

      UseBridgeLibraryEntries();

      Fire_BridgeChanged();
   }
}

const CDeckDescription2* CProjectAgentImp::GetDeckDescription() const
{
   return m_BridgeDescription.GetDeckDescription();
}

void CProjectAgentImp::SetDeckDescription(const CDeckDescription2& deck)
{
   if ( deck != (*m_BridgeDescription.GetDeckDescription()) )
   {
      m_BridgeDescription.GetDeckDescription()->CopyDeckData(&deck);
      Fire_BridgeChanged();
   }
}

SpanIndexType CProjectAgentImp::GetSpanCount() const
{
   return m_BridgeDescription.GetSpanCount();
}

const CSpanData2* CProjectAgentImp::GetSpan(SpanIndexType spanIdx) const
{
   return m_BridgeDescription.GetSpan(spanIdx);
}

void CProjectAgentImp::SetSpan(SpanIndexType spanIdx,const CSpanData2& spanData)
{
   if ( *m_BridgeDescription.GetSpan(spanIdx) != spanData )
   {
      ReleaseBridgeLibraryEntries();

      // copy the data, don't alter ID or Index
      m_BridgeDescription.GetSpan(spanIdx)->CopySpanData(&spanData);
      
      UseBridgeLibraryEntries();

      Fire_BridgeChanged();
   }
}

GroupIndexType CProjectAgentImp::GetGirderGroupCount() const
{
   return m_BridgeDescription.GetGirderGroupCount();
}

const CGirderGroupData* CProjectAgentImp::GetGirderGroup(GroupIndexType grpIdx) const
{
   return m_BridgeDescription.GetGirderGroup(grpIdx);
}

PierIndexType CProjectAgentImp::GetPierCount() const
{
   return m_BridgeDescription.GetPierCount();
}

const CPierData2* CProjectAgentImp::GetPier(PierIndexType pierIdx) const
{
   return m_BridgeDescription.GetPier(pierIdx);
}

const CPierData2* CProjectAgentImp::FindPier(PierIDType pierID) const
{
   return m_BridgeDescription.FindPier(pierID);
}

void CProjectAgentImp::SetPierByIndex(PierIndexType pierIdx,const CPierData2& pierData)
{
   if ( *m_BridgeDescription.GetPier(pierIdx) != pierData )
   {
      ReleaseBridgeLibraryEntries();
      
      // Copy data only, don't alter ID or Index
      m_BridgeDescription.GetPier(pierIdx)->CopyPierData(&pierData);

      UseBridgeLibraryEntries();
      
      Fire_BridgeChanged();
   }
}

void CProjectAgentImp::SetPierByID(PierIDType pierID,const CPierData2& pierData)
{
   const CPierData2* pPier = FindPier(pierID);
   SetPierByIndex(pPier->GetIndex(),pierData);
}

SupportIndexType CProjectAgentImp::GetTemporarySupportCount() const
{
   return m_BridgeDescription.GetTemporarySupportCount();
}

const CTemporarySupportData* CProjectAgentImp::GetTemporarySupport(SupportIndexType tsIdx) const
{
   return m_BridgeDescription.GetTemporarySupport(tsIdx);
}

const CTemporarySupportData* CProjectAgentImp::FindTemporarySupport(SupportIDType tsID) const
{
   return m_BridgeDescription.FindTemporarySupport(tsID);
}

void CProjectAgentImp::SetTemporarySupportByIndex(SupportIndexType tsIdx,const CTemporarySupportData& tsData)
{
   if ( *m_BridgeDescription.GetTemporarySupport(tsIdx) != tsData )
   {
      m_BridgeDescription.SetTemporarySupportByIndex(tsIdx,tsData);
      Fire_BridgeChanged();
   }
}

void CProjectAgentImp::SetTemporarySupportByID(SupportIDType tsID,const CTemporarySupportData& tsData)
{
   const CTemporarySupportData* pTS = m_BridgeDescription.FindTemporarySupport(tsID);
   SetTemporarySupportByIndex(pTS->GetIndex(),tsData);
}

const CSplicedGirderData* CProjectAgentImp::GetGirder(const CGirderKey& girderKey) const
{
   const CGirderGroupData* pGroup = m_BridgeDescription.GetGirderGroup(girderKey.groupIndex);
   return pGroup->GetGirder(girderKey.girderIndex);
}

const CSplicedGirderData* CProjectAgentImp::FindGirder(GirderIDType gdrID) const
{
   return m_BridgeDescription.FindGirder(gdrID);
}

void CProjectAgentImp::SetGirder(const CGirderKey& girderKey,const CSplicedGirderData& girder)
{
   CGirderGroupData* pGroup = m_BridgeDescription.GetGirderGroup(girderKey.groupIndex);
   CSplicedGirderData* pGirder = pGroup->GetGirder(girderKey.girderIndex);
   if ( *pGirder != girder )
   {
      ReleaseGirderLibraryEntries();

      ATLASSERT(pGirder->GetSegmentCount() == girder.GetSegmentCount() );

      // copy data only. Don't alter ID or Index
      pGirder->CopySplicedGirderData(&girder);

      UseGirderLibraryEntries();

      Fire_GirderChanged(girderKey,0/*no hint*/);
   }
}

const CPTData* CProjectAgentImp::GetPostTensioning(const CGirderKey& girderKey) const
{
   const CSplicedGirderData* pGirder = GetGirder(girderKey);
   return pGirder->GetPostTensioning();
}

void CProjectAgentImp::SetPostTensioning(const CGirderKey& girderKey,const CPTData& ptData)
{
   CGirderGroupData* pGroup = m_BridgeDescription.GetGirderGroup(girderKey.groupIndex);
   CSplicedGirderData* pGirder = pGroup->GetGirder(girderKey.girderIndex);
   CPTData* pPTData = pGirder->GetPostTensioning();
   if ( *pPTData != ptData )
   {
      *pPTData = ptData;
      Fire_GirderChanged(girderKey,GCH_POSTTENSIONING_CONFIGURATION);
   }
}

const CPrecastSegmentData* CProjectAgentImp::GetPrecastSegmentData(const CSegmentKey& segmentKey) const
{
   const CGirderGroupData* pGroup = m_BridgeDescription.GetGirderGroup(segmentKey.groupIndex);
   const CSplicedGirderData* pGirder = pGroup->GetGirder(segmentKey.girderIndex);
   return pGirder->GetSegment(segmentKey.segmentIndex);
}

void CProjectAgentImp::SetPrecastSegmentData(const CSegmentKey& segmentKey,const CPrecastSegmentData& segment)
{
   CGirderGroupData* pGroup = m_BridgeDescription.GetGirderGroup(segmentKey.groupIndex);
   CSplicedGirderData* pGirder = pGroup->GetGirder(segmentKey.girderIndex);
   CPrecastSegmentData* pSegment = pGirder->GetSegment(segmentKey.segmentIndex);

   if ( *pSegment != segment )
   {
      // copy only data. don't alter ID or Index
      const HaulTruckLibrary* pHaulTruckLibrary = m_pLibMgr->GetHaulTruckLibrary();
      release_library_entry(&m_LibObserver,pSegment->HandlingData.pHaulTruckLibraryEntry,*pHaulTruckLibrary);

      pSegment->CopySegmentData(&segment,true);

      const HaulTruckLibraryEntry* pHaulTruckEntry;
      use_library_entry(&m_LibObserver,pSegment->HandlingData.HaulTruckName,&pHaulTruckEntry,*pHaulTruckLibrary);
      pSegment->HandlingData.pHaulTruckLibraryEntry = pHaulTruckEntry;

      Fire_GirderChanged(segmentKey,0/*no hint*/);
   }
}

const CClosureJointData* CProjectAgentImp::GetClosureJointData(const CSegmentKey& closureKey) const
{
   const CGirderGroupData*   pGroup    = m_BridgeDescription.GetGirderGroup(closureKey.groupIndex);
   const CSplicedGirderData* pGirder   = pGroup->GetGirder(closureKey.girderIndex);
   const CPrecastSegmentData* pSegment = pGirder->GetSegment(closureKey.segmentIndex);
   return pSegment->GetEndClosure();
}

void CProjectAgentImp::SetClosureJointData(const CSegmentKey& closureKey,const CClosureJointData& closure)
{
   CGirderGroupData*    pGroup   = m_BridgeDescription.GetGirderGroup(closureKey.groupIndex);
   CSplicedGirderData*  pGirder  = pGroup->GetGirder(closureKey.girderIndex);
   CPrecastSegmentData* pSegment = pGirder->GetSegment(closureKey.segmentIndex);
   CClosureJointData*    pClosure = pSegment->GetEndClosure();

   // this method sets the right closure joint data... there is not a closure
   // at the right end of the last segment
   ATLASSERT(pGirder->GetSegmentCount()-1 != closureKey.segmentIndex);

   if ( *pClosure != closure )
   {
      // copy only data. don't alter ID or Index
      pClosure->CopyClosureJointData(&closure);
      Fire_GirderChanged(closureKey,0/*no hint*/);
   }
}

void CProjectAgentImp::SetSpanLength(SpanIndexType spanIdx,Float64 newLength)
{
   if ( m_BridgeDescription.SetSpanLength(spanIdx,newLength) )
   {
      Fire_BridgeChanged();
   }
}

void CProjectAgentImp::MovePier(PierIndexType pierIdx,Float64 newStation,pgsTypes::MovePierOption moveOption)
{
   if ( m_BridgeDescription.MovePier(pierIdx,newStation,moveOption) )
   {
      Fire_BridgeChanged();
   }
}

SupportIndexType CProjectAgentImp::MoveTemporarySupport(SupportIndexType tsIdx,Float64 newStation)
{
   SupportIndexType newIndex = m_BridgeDescription.MoveTemporarySupport(tsIdx,newStation);
   if ( newIndex != INVALID_INDEX )
   {
      Fire_BridgeChanged();
   }

   return newIndex;
}

void CProjectAgentImp::SetMeasurementType(PierIndexType pierIdx,pgsTypes::PierFaceType pierFace,pgsTypes::MeasurementType mt)
{
   CPierData2* pPier = m_BridgeDescription.GetPier(pierIdx);
   CGirderSpacing2* pSpacing = pPier->GetGirderSpacing(pierFace);

   if ( pSpacing->GetMeasurementType() != mt )
   {
      pSpacing->SetMeasurementType(mt);
      Fire_BridgeChanged();
   }
}

void CProjectAgentImp::SetMeasurementLocation(PierIndexType pierIdx,pgsTypes::PierFaceType pierFace,pgsTypes::MeasurementLocation ml)
{
   CPierData2* pPier = m_BridgeDescription.GetPier(pierIdx);
   CGirderSpacing2* pSpacing = pPier->GetGirderSpacing(pierFace);
   if ( pSpacing->GetMeasurementLocation() != ml )
   {
      pSpacing->SetMeasurementLocation(ml);
      Fire_BridgeChanged();
   }
}

void CProjectAgentImp::SetSlabOffset(Float64 slabOffset)
{
   if ( m_BridgeDescription.GetSlabOffsetType() != pgsTypes::sotBridge ||
        !IsEqual(slabOffset,m_BridgeDescription.GetSlabOffset()) )
   {
      // slab offset type and/or value is chaning
      m_BridgeDescription.SetSlabOffsetType(pgsTypes::sotBridge);
      m_BridgeDescription.SetSlabOffset(slabOffset);
      Fire_BridgeChanged();
   }
}

void CProjectAgentImp::SetWearingSurfaceType(pgsTypes::WearingSurfaceType wearingSurfaceType)
{
   if ( m_BridgeDescription.GetDeckDescription()->WearingSurface != wearingSurfaceType )
   {
      m_BridgeDescription.GetDeckDescription()->WearingSurface = wearingSurfaceType;
      Fire_BridgeChanged();
   }
}

void CProjectAgentImp::SetSlabOffsetType(pgsTypes::SlabOffsetType offsetType)
{
   if ( m_BridgeDescription.GetSlabOffsetType() != offsetType )
   {
      m_BridgeDescription.SetSlabOffsetType(offsetType);
      Fire_BridgeChanged();
   }
}

void CProjectAgentImp::SetSlabOffset(GroupIndexType grpIdx,PierIndexType pierIdx, Float64 offset)
{
   // Changing slab offset type to pier
   CGirderGroupData* pGroup = m_BridgeDescription.GetGirderGroup(grpIdx);
   if ( !IsEqual(pGroup->GetSlabOffset(pierIdx,0),offset) )
   {
      m_BridgeDescription.SetSlabOffsetType(pgsTypes::sotPier);
      pGroup->SetSlabOffset(pierIdx,offset);
      Fire_BridgeChanged();
   }
}

void CProjectAgentImp::SetSlabOffset(GroupIndexType grpIdx,PierIndexType pierIdx, GirderIndexType gdrIdx, Float64 offset)
{
   CGirderGroupData* pGroup = m_BridgeDescription.GetGirderGroup(grpIdx);
   if ( m_BridgeDescription.GetSlabOffsetType() != pgsTypes::sotGirder || !IsEqual(pGroup->GetSlabOffset(pierIdx,gdrIdx),offset) )
   {
      m_BridgeDescription.SetSlabOffsetType(pgsTypes::sotGirder);
      pGroup->SetSlabOffset(pierIdx,gdrIdx,offset);

      Fire_BridgeChanged();
   }
}

Float64 CProjectAgentImp::GetSlabOffset(GroupIndexType grpIdx,PierIndexType pierIdx, GirderIndexType gdrIdx) const
{
   const CGirderGroupData* pGroup = m_BridgeDescription.GetGirderGroup(grpIdx);
   return pGroup->GetSlabOffset(pierIdx,gdrIdx);
}

pgsTypes::SlabOffsetType CProjectAgentImp::GetSlabOffsetType() const
{
   return m_BridgeDescription.GetSlabOffsetType();
}

void CProjectAgentImp::SetFillet( Float64 Fillet)
{
   if ( !IsEqual(Fillet,m_BridgeDescription.GetFillet()) )
   {
      m_BridgeDescription.SetFillet(Fillet);
      Fire_BridgeChanged();
   }
}

Float64 CProjectAgentImp::GetFillet() const
{
   return m_BridgeDescription.GetFillet();
}

void CProjectAgentImp::SetAssExcessCamberType(pgsTypes::AssExcessCamberType type)
{
   if ( m_BridgeDescription.GetAssExcessCamberType() != type )
   {
      m_BridgeDescription.SetAssExcessCamberType(type);
      Fire_BridgeChanged();
   }
}

pgsTypes::AssExcessCamberType CProjectAgentImp::GetAssExcessCamberType() const
{
   return m_BridgeDescription.GetAssExcessCamberType();
}

void CProjectAgentImp::SetAssExcessCamber( Float64 camber)
{
   if ( m_BridgeDescription.GetAssExcessCamberType() != pgsTypes::aecBridge ||
        !IsEqual(camber,m_BridgeDescription.GetAssExcessCamber()) )
   {
      // AssExcessCamber type and/or value is changing
      m_BridgeDescription.SetAssExcessCamberType(pgsTypes::aecBridge);
      m_BridgeDescription.SetAssExcessCamber(camber);
      Fire_BridgeChanged();
   }
}

void CProjectAgentImp::SetAssExcessCamber(SpanIndexType spanIdx, Float64 camber)
{
   CSpanData2* pSpan = m_BridgeDescription.GetSpan(spanIdx);
   if ( m_BridgeDescription.GetAssExcessCamberType() != pgsTypes::aecSpan || 
        !IsEqual(pSpan->GetAssExcessCamber(0),camber) )
   {
      m_BridgeDescription.SetAssExcessCamberType(pgsTypes::aecSpan);
      pSpan->SetAssExcessCamber(camber);
      Fire_BridgeChanged();
   }
}

void CProjectAgentImp::SetAssExcessCamber( SpanIndexType spanIdx, GirderIndexType gdrIdx, Float64 camber)
{
   CSpanData2* pSpan = m_BridgeDescription.GetSpan(spanIdx);
   if ( m_BridgeDescription.GetAssExcessCamberType() != pgsTypes::aecGirder ||
       !IsEqual(pSpan->GetAssExcessCamber(gdrIdx),camber) )
   {
      m_BridgeDescription.SetAssExcessCamberType(pgsTypes::aecGirder);
      pSpan->SetAssExcessCamber(gdrIdx,camber);
      Fire_BridgeChanged();
   }
}

Float64 CProjectAgentImp::GetAssExcessCamber( SpanIndexType spanIdx, GirderIndexType gdrIdx) const
{
   CSpanData2* pSpan = m_BridgeDescription.GetSpan(spanIdx);
   return pSpan->GetAssExcessCamber(gdrIdx);
}

std::vector<pgsTypes::BoundaryConditionType> CProjectAgentImp::GetBoundaryConditionTypes(PierIndexType pierIdx) const
{
   return m_BridgeDescription.GetBoundaryConditionTypes(pierIdx);
}

std::vector<pgsTypes::PierSegmentConnectionType> CProjectAgentImp::GetPierSegmentConnectionTypes(PierIndexType pierIdx) const
{
   return m_BridgeDescription.GetPierSegmentConnectionTypes(pierIdx);
}

const CTimelineManager* CProjectAgentImp::GetTimelineManager() const
{
   return m_BridgeDescription.GetTimelineManager();
}

void CProjectAgentImp::SetTimelineManager(const CTimelineManager& timelineMgr)
{
   if (*m_BridgeDescription.GetTimelineManager() != timelineMgr)
   {
      m_BridgeDescription.SetTimelineManager(&timelineMgr);

      Fire_BridgeChanged();
   }
}

EventIndexType CProjectAgentImp::AddTimelineEvent(const CTimelineEvent& timelineEvent)
{
   EventIndexType idx;
   m_BridgeDescription.GetTimelineManager()->AddTimelineEvent(timelineEvent,true,&idx);

   Fire_BridgeChanged();
   return idx;
}

EventIndexType CProjectAgentImp::GetEventCount() const
{
   return m_BridgeDescription.GetTimelineManager()->GetEventCount();
}

const CTimelineEvent* CProjectAgentImp::GetEventByIndex(EventIndexType eventIdx) const
{
   return m_BridgeDescription.GetTimelineManager()->GetEventByIndex(eventIdx);
}

const CTimelineEvent* CProjectAgentImp::GetEventByID(EventIDType eventID) const
{
   return m_BridgeDescription.GetTimelineManager()->GetEventByID(eventID);
}

void CProjectAgentImp::SetEventByIndex(EventIndexType eventIdx,const CTimelineEvent& timelineEvent)
{
   const CTimelineEvent* pTimelineEvent = m_BridgeDescription.GetTimelineManager()->GetEventByIndex(eventIdx);
   if ( *pTimelineEvent != timelineEvent )
   {
      CTimelineEvent* pNewEvent = new CTimelineEvent(timelineEvent);
      m_BridgeDescription.GetTimelineManager()->SetEventByIndex(eventIdx,pNewEvent,true);
      Fire_BridgeChanged();
   }
}

void CProjectAgentImp::SetEventByID(EventIDType eventID,const CTimelineEvent& timelineEvent)
{
   const CTimelineEvent* pTimelineEvent = m_BridgeDescription.GetTimelineManager()->GetEventByID(eventID);
   if ( *pTimelineEvent != timelineEvent )
   {
      CTimelineEvent* pNewEvent = new CTimelineEvent(timelineEvent);
      m_BridgeDescription.GetTimelineManager()->SetEventByID(eventID,pNewEvent,true);
      Fire_BridgeChanged();
   }
}

void CProjectAgentImp::SetSegmentConstructionEventByIndex(const CSegmentKey& segmentKey,EventIndexType eventIdx)
{
   const CPrecastSegmentData* pSegment = m_BridgeDescription.GetGirderGroup(segmentKey.groupIndex)->GetGirder(segmentKey.girderIndex)->GetSegment(segmentKey.segmentIndex);
   SegmentIDType segmentID = pSegment->GetID();

   if ( eventIdx != m_BridgeDescription.GetTimelineManager()->GetSegmentConstructionEventIndex(segmentID) )
   {
      m_BridgeDescription.GetTimelineManager()->SetSegmentConstructionEventByIndex(segmentID,eventIdx);
      Fire_BridgeChanged();
   }
}

void CProjectAgentImp::SetSegmentConstructionEventByID(const CSegmentKey& segmentKey,EventIDType eventID)
{
   const CPrecastSegmentData* pSegment = m_BridgeDescription.GetGirderGroup(segmentKey.groupIndex)->GetGirder(segmentKey.girderIndex)->GetSegment(segmentKey.segmentIndex);
   SegmentIDType segmentID = pSegment->GetID();

   if ( eventID != m_BridgeDescription.GetTimelineManager()->GetSegmentConstructionEventID(segmentID) )
   {
      m_BridgeDescription.GetTimelineManager()->SetSegmentConstructionEventByID(segmentID,eventID);
      Fire_BridgeChanged();
   }
}

EventIndexType CProjectAgentImp::GetSegmentConstructionEventIndex(const CSegmentKey& segmentKey) const
{
   const CPrecastSegmentData* pSegment = m_BridgeDescription.GetGirderGroup(segmentKey.groupIndex)->GetGirder(segmentKey.girderIndex)->GetSegment(segmentKey.segmentIndex);
   SegmentIDType segmentID = pSegment->GetID();
   return m_BridgeDescription.GetTimelineManager()->GetSegmentConstructionEventIndex(segmentID);
}

EventIDType CProjectAgentImp::GetSegmentConstructionEventID(const CSegmentKey& segmentKey) const
{
   const CPrecastSegmentData* pSegment = m_BridgeDescription.GetGirderGroup(segmentKey.groupIndex)->GetGirder(segmentKey.girderIndex)->GetSegment(segmentKey.segmentIndex);
   SegmentIDType segmentID = pSegment->GetID();
   return m_BridgeDescription.GetTimelineManager()->GetSegmentConstructionEventID(segmentID);
}

EventIndexType CProjectAgentImp::GetPierErectionEvent(PierIndexType pierIdx) const
{
   const CPierData2* pPier = m_BridgeDescription.GetPier(pierIdx);
   PierIDType pierID = pPier->GetID();
   return m_BridgeDescription.GetTimelineManager()->GetPierErectionEventIndex(pierID);
}

void CProjectAgentImp::SetPierErectionEventByIndex(PierIndexType pierIdx,EventIndexType eventIdx)
{
   const CPierData2* pPier = m_BridgeDescription.GetPier(pierIdx);
   PierIDType pierID = pPier->GetID();

   if ( eventIdx != m_BridgeDescription.GetTimelineManager()->GetPierErectionEventIndex(pierID) )
   {
      m_BridgeDescription.GetTimelineManager()->SetPierErectionEventByIndex(pierID,eventIdx);
      Fire_BridgeChanged();
   }
}

void CProjectAgentImp::SetPierErectionEventByID(PierIndexType pierIdx,EventIDType eventID)
{
   const CPierData2* pPier = m_BridgeDescription.GetPier(pierIdx);
   PierIDType pierID = pPier->GetID();

   if ( eventID != m_BridgeDescription.GetTimelineManager()->GetPierErectionEventID(pierID) )
   {
      m_BridgeDescription.GetTimelineManager()->SetPierErectionEventByID(pierID,eventID);
      Fire_BridgeChanged();
   }
}

void CProjectAgentImp::SetTempSupportEventsByIndex(SupportIndexType tsIdx,EventIndexType erectIdx,EventIndexType removeIdx)
{
   const CTemporarySupportData* pTS = m_BridgeDescription.GetTemporarySupport(tsIdx);
   SupportIDType tsID = pTS->GetID();

   SetTempSupportEventsByID(tsID,erectIdx,removeIdx);
}

void CProjectAgentImp::SetTempSupportEventsByID(SupportIDType tsID,EventIndexType erectIdx,EventIndexType removeIdx)
{
   EventIndexType currErectEventIdx, currRemoveEventIdx;
   m_BridgeDescription.GetTimelineManager()->GetTempSupportEvents(tsID,&currErectEventIdx,&currRemoveEventIdx);
   if ( erectIdx != currErectEventIdx || removeIdx != currRemoveEventIdx )
   {
      m_BridgeDescription.GetTimelineManager()->SetTempSupportEvents(tsID,erectIdx,removeIdx);
      Fire_BridgeChanged();
   }
}

void CProjectAgentImp::SetSegmentErectionEventByIndex(const CSegmentKey& segmentKey,EventIndexType eventIdx)
{
   const CPrecastSegmentData* pSegment = m_BridgeDescription.GetGirderGroup(segmentKey.groupIndex)->GetGirder(segmentKey.girderIndex)->GetSegment(segmentKey.segmentIndex);
   SegmentIDType segmentID = pSegment->GetID();

   if ( eventIdx != m_BridgeDescription.GetTimelineManager()->GetSegmentErectionEventIndex(segmentID) )
   {
      m_BridgeDescription.GetTimelineManager()->SetSegmentErectionEventByIndex(segmentID,eventIdx);
      Fire_BridgeChanged();
   }
}

void CProjectAgentImp::SetSegmentErectionEventByID(const CSegmentKey& segmentKey,EventIDType eventID)
{
   const CPrecastSegmentData* pSegment = m_BridgeDescription.GetGirderGroup(segmentKey.groupIndex)->GetGirder(segmentKey.girderIndex)->GetSegment(segmentKey.segmentIndex);
   SegmentIDType segmentID = pSegment->GetID();

   if ( eventID != m_BridgeDescription.GetTimelineManager()->GetSegmentErectionEventID(segmentID) )
   {
      m_BridgeDescription.GetTimelineManager()->SetSegmentErectionEventByID(segmentID,eventID);
      Fire_BridgeChanged();
   }
}

EventIndexType CProjectAgentImp::GetSegmentErectionEventIndex(const CSegmentKey& segmentKey) const
{
   const CPrecastSegmentData* pSegment = m_BridgeDescription.GetGirderGroup(segmentKey.groupIndex)->GetGirder(segmentKey.girderIndex)->GetSegment(segmentKey.segmentIndex);
   SegmentIDType segmentID = pSegment->GetID();

   return m_BridgeDescription.GetTimelineManager()->GetSegmentErectionEventIndex(segmentID);
}

EventIDType CProjectAgentImp::GetSegmentErectionEventID(const CSegmentKey& segmentKey) const
{
   const CPrecastSegmentData* pSegment = m_BridgeDescription.GetGirderGroup(segmentKey.groupIndex)->GetGirder(segmentKey.girderIndex)->GetSegment(segmentKey.segmentIndex);
   SegmentIDType segmentID = pSegment->GetID();

   return m_BridgeDescription.GetTimelineManager()->GetSegmentErectionEventID(segmentID);
}

void CProjectAgentImp::SetSegmentEventsByIndex(const CSegmentKey& segmentKey,EventIndexType constructionEventIdx,EventIndexType erectionEventIdx)
{
   const CPrecastSegmentData* pSegment = m_BridgeDescription.GetGirderGroup(segmentKey.groupIndex)->GetGirder(segmentKey.girderIndex)->GetSegment(segmentKey.segmentIndex);
   SegmentIDType segmentID = pSegment->GetID();

   if ( constructionEventIdx != m_BridgeDescription.GetTimelineManager()->GetSegmentConstructionEventIndex(segmentID) && 
        erectionEventIdx     != m_BridgeDescription.GetTimelineManager()->GetSegmentErectionEventIndex(segmentID) )
   {
      m_BridgeDescription.GetTimelineManager()->SetSegmentEvents(segmentID,constructionEventIdx,erectionEventIdx);
      Fire_BridgeChanged();
   }
}

void CProjectAgentImp::SetSegmentEventsByID(const CSegmentKey& segmentKey,EventIDType constructionEventID,EventIDType erectionEventID)
{
   const CPrecastSegmentData* pSegment = m_BridgeDescription.GetGirderGroup(segmentKey.groupIndex)->GetGirder(segmentKey.girderIndex)->GetSegment(segmentKey.segmentIndex);
   SegmentIDType segmentID = pSegment->GetID();

   if ( constructionEventID != m_BridgeDescription.GetTimelineManager()->GetSegmentConstructionEventID(segmentID) && 
        erectionEventID     != m_BridgeDescription.GetTimelineManager()->GetSegmentErectionEventID(segmentID) )
   {
      m_BridgeDescription.GetTimelineManager()->SetSegmentConstructionEventByID(segmentID,constructionEventID);
      m_BridgeDescription.GetTimelineManager()->SetSegmentErectionEventByID(segmentID,erectionEventID);
      Fire_BridgeChanged();
   }
}

void CProjectAgentImp::GetSegmentEventsByIndex(const CSegmentKey& segmentKey,EventIndexType* constructionEventIdx,EventIndexType* erectionEventIdx) const
{
   *constructionEventIdx = GetSegmentConstructionEventIndex(segmentKey);
   *erectionEventIdx     = GetSegmentErectionEventIndex(segmentKey);
}

void CProjectAgentImp::GetSegmentEventsByID(const CSegmentKey& segmentKey,EventIDType* constructionEventID,EventIDType* erectionEventID) const
{
   *constructionEventID = GetSegmentConstructionEventID(segmentKey);
   *erectionEventID     = GetSegmentErectionEventID(segmentKey);
}

EventIndexType CProjectAgentImp::GetCastClosureJointEventIndex(GroupIndexType grpIdx,CollectionIndexType closureIdx) const
{
   const CClosureJointData* pClosure = m_BridgeDescription.GetGirderGroup(grpIdx)->GetGirder(0)->GetClosureJoint(closureIdx);
   return m_BridgeDescription.GetTimelineManager()->GetCastClosureJointEventIndex(pClosure);
}

EventIDType CProjectAgentImp::GetCastClosureJointEventID(GroupIndexType grpIdx,CollectionIndexType closureIdx) const
{
   const CClosureJointData* pClosure = m_BridgeDescription.GetGirderGroup(grpIdx)->GetGirder(0)->GetClosureJoint(closureIdx);
   return m_BridgeDescription.GetTimelineManager()->GetCastClosureJointEventID(pClosure);
}

void CProjectAgentImp::SetCastClosureJointEventByIndex(GroupIndexType grpIdx,CollectionIndexType closureIdx,EventIndexType eventIdx)
{
   const CClosureJointData* pClosure = m_BridgeDescription.GetGirderGroup(grpIdx)->GetGirder(0)->GetClosureJoint(closureIdx);
   if ( eventIdx != m_BridgeDescription.GetTimelineManager()->GetCastClosureJointEventIndex(pClosure) )
   {
      m_BridgeDescription.GetTimelineManager()->SetCastClosureJointEventByIndex(pClosure,eventIdx);
      Fire_BridgeChanged();
   }
}

void CProjectAgentImp::SetCastClosureJointEventByID(GroupIndexType grpIdx,CollectionIndexType closureIdx,IDType eventID)
{
   const CClosureJointData* pClosure = m_BridgeDescription.GetGirderGroup(grpIdx)->GetGirder(0)->GetClosureJoint(closureIdx);
   if ( eventID != m_BridgeDescription.GetTimelineManager()->GetCastClosureJointEventID(pClosure) )
   {
      m_BridgeDescription.GetTimelineManager()->SetCastClosureJointEventByID(pClosure,eventID);
      Fire_BridgeChanged();
   }
}

EventIndexType CProjectAgentImp::GetStressTendonEventIndex(const CGirderKey& girderKey,DuctIndexType ductIdx) const
{
   ASSERT_GIRDER_KEY(girderKey);
   GirderIDType girderID = m_BridgeDescription.GetGirderGroup(girderKey.groupIndex)->GetGirder(girderKey.girderIndex)->GetID();
   return m_BridgeDescription.GetTimelineManager()->GetStressTendonEventIndex(girderID,ductIdx);
}

EventIDType CProjectAgentImp::GetStressTendonEventID(const CGirderKey& girderKey,DuctIndexType ductIdx) const
{
   ASSERT_GIRDER_KEY(girderKey);
   GirderIDType girderID = m_BridgeDescription.GetGirderGroup(girderKey.groupIndex)->GetGirder(girderKey.girderIndex)->GetID();
   return m_BridgeDescription.GetTimelineManager()->GetStressTendonEventID(girderID,ductIdx);
}

void CProjectAgentImp::SetStressTendonEventByIndex(const CGirderKey& girderKey,DuctIndexType ductIdx,EventIndexType eventIdx)
{
   ASSERT_GIRDER_KEY(girderKey);
   GirderIDType girderID = m_BridgeDescription.GetGirderGroup(girderKey.groupIndex)->GetGirder(girderKey.girderIndex)->GetID();
   if ( eventIdx != m_BridgeDescription.GetTimelineManager()->GetStressTendonEventIndex(girderID,ductIdx) )
   {
      m_BridgeDescription.GetTimelineManager()->SetStressTendonEventByIndex(girderID,ductIdx,eventIdx);
      Fire_BridgeChanged();
   }
}

void CProjectAgentImp::SetStressTendonEventByID(const CGirderKey& girderKey,DuctIndexType ductIdx,EventIDType eventID)
{
   ASSERT_GIRDER_KEY(girderKey);
   GirderIDType girderID = m_BridgeDescription.GetGirderGroup(girderKey.groupIndex)->GetGirder(girderKey.girderIndex)->GetID();
   if ( eventID != m_BridgeDescription.GetTimelineManager()->GetStressTendonEventID(girderID,ductIdx) )
   {
      m_BridgeDescription.GetTimelineManager()->SetStressTendonEventByID(girderID,ductIdx,eventID);
      Fire_BridgeChanged();
   }
}

EventIndexType CProjectAgentImp::GetCastLongitudinalJointEventIndex() const
{
   return m_BridgeDescription.GetTimelineManager()->GetCastLongitudinalJointEventIndex();
}

EventIDType CProjectAgentImp::GetCastLongitudinalJointEventID() const
{
   return m_BridgeDescription.GetTimelineManager()->GetCastLongitudinalJointEventID();
}

int CProjectAgentImp::SetCastLongitudinalJointEventByIndex(EventIndexType eventIdx, bool bAdjustTimeline)
{
   int result = TLM_SUCCESS;
   if (eventIdx != m_BridgeDescription.GetTimelineManager()->GetCastLongitudinalJointEventIndex())
   {
      result = m_BridgeDescription.GetTimelineManager()->SetCastLongitudinalJointEventByIndex(eventIdx, bAdjustTimeline);
      if (result == TLM_SUCCESS)
      {
         Fire_BridgeChanged();
      }
   }

   return result;
}

int CProjectAgentImp::SetCastLongitudinalJointEventByID(EventIDType eventID, bool bAdjustTimeline)
{
   int result = TLM_SUCCESS;
   if (eventID != m_BridgeDescription.GetTimelineManager()->GetCastLongitudinalJointEventID())
   {
      result = m_BridgeDescription.GetTimelineManager()->SetCastLongitudinalJointEventByID(eventID, bAdjustTimeline);
      if (result == TLM_SUCCESS)
      {
         Fire_BridgeChanged();
      }
   }
   return result;
}

EventIndexType CProjectAgentImp::GetCastDeckEventIndex() const
{
   return m_BridgeDescription.GetTimelineManager()->GetCastDeckEventIndex();
}

EventIDType CProjectAgentImp::GetCastDeckEventID() const
{
   return m_BridgeDescription.GetTimelineManager()->GetCastDeckEventID();
}

int CProjectAgentImp::SetCastDeckEventByIndex(EventIndexType eventIdx,bool bAdjustTimeline)
{
   int result = TLM_SUCCESS;
   if ( eventIdx != m_BridgeDescription.GetTimelineManager()->GetCastDeckEventIndex() )
   {
      result = m_BridgeDescription.GetTimelineManager()->SetCastDeckEventByIndex(eventIdx,bAdjustTimeline);
      if ( result == TLM_SUCCESS )
      {
         Fire_BridgeChanged();
      }
   }

   return result;
}

int CProjectAgentImp::SetCastDeckEventByID(EventIDType eventID,bool bAdjustTimeline)
{
   int result = TLM_SUCCESS;
   if ( eventID != m_BridgeDescription.GetTimelineManager()->GetCastDeckEventID() )
   {
      result = m_BridgeDescription.GetTimelineManager()->SetCastDeckEventByID(eventID,bAdjustTimeline);
      if ( result == TLM_SUCCESS )
      {
         Fire_BridgeChanged();
      }
   }
   return result;
}

EventIndexType CProjectAgentImp::GetIntermediateDiaphragmsLoadEventIndex() const
{
   return m_BridgeDescription.GetTimelineManager()->GetIntermediateDiaphragmsLoadEventIndex();
}

EventIDType CProjectAgentImp::GetIntermediateDiaphragmsLoadEventID() const
{
   return m_BridgeDescription.GetTimelineManager()->GetIntermediateDiaphragmsLoadEventID();
}

void CProjectAgentImp::SetIntermediateDiaphragmsLoadEventByIndex(EventIndexType eventIdx)
{
   if (eventIdx != m_BridgeDescription.GetTimelineManager()->GetIntermediateDiaphragmsLoadEventIndex())
   {
      m_BridgeDescription.GetTimelineManager()->SetIntermediateDiaphragmsLoadEventByIndex(eventIdx);
      Fire_BridgeChanged();
   }
}

void CProjectAgentImp::SetIntermediateDiaphragmsLoadEventByID(EventIDType eventID)
{
   if (eventID != m_BridgeDescription.GetTimelineManager()->GetIntermediateDiaphragmsLoadEventID())
   {
      m_BridgeDescription.GetTimelineManager()->SetIntermediateDiaphragmsLoadEventByID(eventID);
      Fire_BridgeChanged();
   }
}

EventIndexType CProjectAgentImp::GetRailingSystemLoadEventIndex() const
{
   return m_BridgeDescription.GetTimelineManager()->GetRailingSystemLoadEventIndex();
}

EventIDType CProjectAgentImp::GetRailingSystemLoadEventID() const
{
   return m_BridgeDescription.GetTimelineManager()->GetRailingSystemLoadEventID();
}

void CProjectAgentImp::SetRailingSystemLoadEventByIndex(EventIndexType eventIdx)
{
   if ( eventIdx != m_BridgeDescription.GetTimelineManager()->GetRailingSystemLoadEventIndex() )
   {
      m_BridgeDescription.GetTimelineManager()->SetRailingSystemLoadEventByIndex(eventIdx);
      Fire_BridgeChanged();
   }
}

void CProjectAgentImp::SetRailingSystemLoadEventByID(EventIDType eventID)
{
   if ( eventID != m_BridgeDescription.GetTimelineManager()->GetRailingSystemLoadEventID() )
   {
      m_BridgeDescription.GetTimelineManager()->SetRailingSystemLoadEventByID(eventID);
      Fire_BridgeChanged();
   }
}

EventIndexType CProjectAgentImp::GetOverlayLoadEventIndex() const
{
   return m_BridgeDescription.GetTimelineManager()->GetOverlayLoadEventIndex();
}

EventIDType CProjectAgentImp::GetOverlayLoadEventID() const
{
   return m_BridgeDescription.GetTimelineManager()->GetOverlayLoadEventID();
}

void CProjectAgentImp::SetOverlayLoadEventByIndex(EventIndexType eventIdx)
{
   if ( eventIdx != m_BridgeDescription.GetTimelineManager()->GetOverlayLoadEventIndex() )
   {
      m_BridgeDescription.GetTimelineManager()->SetOverlayLoadEventByIndex(eventIdx);
      Fire_BridgeChanged();
   }
}

void CProjectAgentImp::SetOverlayLoadEventByID(EventIDType eventID)
{
   if ( eventID != m_BridgeDescription.GetTimelineManager()->GetOverlayLoadEventID() )
   {
      m_BridgeDescription.GetTimelineManager()->SetOverlayLoadEventByID(eventID);
      Fire_BridgeChanged();
   }
}

EventIndexType CProjectAgentImp::GetLiveLoadEventIndex() const
{
   return m_BridgeDescription.GetTimelineManager()->GetLiveLoadEventIndex();
}

EventIDType CProjectAgentImp::GetLiveLoadEventID() const
{
   return m_BridgeDescription.GetTimelineManager()->GetLiveLoadEventID();
}

void CProjectAgentImp::SetLiveLoadEventByIndex(EventIndexType eventIdx)
{
   if ( eventIdx != m_BridgeDescription.GetTimelineManager()->GetLiveLoadEventIndex() )
   {
      m_BridgeDescription.GetTimelineManager()->SetLiveLoadEventByIndex(eventIdx);
      Fire_BridgeChanged();
   }
}

void CProjectAgentImp::SetLiveLoadEventByID(EventIDType eventID)
{
   if ( eventID != m_BridgeDescription.GetTimelineManager()->GetLiveLoadEventID() )
   {
      m_BridgeDescription.GetTimelineManager()->SetLiveLoadEventByID(eventID);
      Fire_BridgeChanged();
   }
}

GroupIDType CProjectAgentImp::GetGroupID(GroupIndexType groupIdx) const
{
   const CGirderGroupData* pGroup = m_BridgeDescription.GetGirderGroup(groupIdx);
   if ( pGroup == nullptr )
   {
      return INVALID_ID;
   }

   return pGroup->GetID();
}

GirderIDType CProjectAgentImp::GetGirderID(const CGirderKey& girderKey) const
{
   const CGirderGroupData* pGroup = m_BridgeDescription.GetGirderGroup(girderKey.groupIndex);
   if ( pGroup == nullptr )
   {
      return INVALID_ID;
   }

   const CSplicedGirderData* pGirder = pGroup->GetGirder(girderKey.girderIndex);
   if ( pGirder == nullptr )
   {
      return INVALID_ID;
   }

   return pGirder->GetID();
}

SegmentIDType CProjectAgentImp::GetSegmentID(const CSegmentKey& segmentKey) const
{
   const CGirderGroupData* pGroup = m_BridgeDescription.GetGirderGroup(segmentKey.groupIndex);
   if ( pGroup == nullptr )
   {
      return INVALID_ID;
   }

   const CSplicedGirderData* pGirder = pGroup->GetGirder(segmentKey.girderIndex);
   if ( pGirder == nullptr )
   {
      return INVALID_ID;
   }

   const CPrecastSegmentData* pSegment = pGirder->GetSegment(segmentKey.segmentIndex);
   if (pSegment == nullptr )
   {
      return INVALID_ID;
   }

   return pSegment->GetID();
}

void CProjectAgentImp::SetBearingType(pgsTypes::BearingType offsetType)
{
   if (offsetType != GetBearingType())
   {
      m_BridgeDescription.SetBearingType(offsetType);
      Fire_BridgeChanged();
   }
}

pgsTypes::BearingType CProjectAgentImp::GetBearingType() const
{
   return m_BridgeDescription.GetBearingType();
}

void CProjectAgentImp::SetBearingData(const CBearingData2* pBearingData)
{
   if ( m_BridgeDescription.GetBearingType() != pgsTypes::brtBridge ||
        *pBearingData != *m_BridgeDescription.GetBearingData() )
   {
      m_BridgeDescription.SetBearingType(pgsTypes::brtBridge);
      m_BridgeDescription.SetBearingData(*pBearingData);
      Fire_BridgeChanged();
   }
}

void CProjectAgentImp::SetBearingData(GroupIndexType grpIdx, PierIndexType pierIdx, pgsTypes::PierFaceType face, const CBearingData2* pBearingData)
{
   if ( m_BridgeDescription.GetBearingType() != pgsTypes::brtPier ||
        *pBearingData != *m_BridgeDescription.GetPier(pierIdx)->GetBearingData(0,face))
   {
      m_BridgeDescription.SetBearingType(pgsTypes::brtPier);
      m_BridgeDescription.GetPier(pierIdx)->SetBearingData(0,face,*pBearingData);
      Fire_BridgeChanged();
   }
}

void CProjectAgentImp::SetBearingData(GroupIndexType grpIdx, PierIndexType pierIdx, pgsTypes::PierFaceType face, GirderIndexType gdrIdx, const CBearingData2* pBearingData)
{
   if ( m_BridgeDescription.GetBearingType() != pgsTypes::brtGirder ||
        *pBearingData != *m_BridgeDescription.GetPier(pierIdx)->GetBearingData(gdrIdx,face))
   {
      m_BridgeDescription.SetBearingType(pgsTypes::brtGirder);
      m_BridgeDescription.GetPier(pierIdx)->SetBearingData(gdrIdx,face,*pBearingData);
      Fire_BridgeChanged();
   }
}

const CBearingData2* CProjectAgentImp::GetBearingData(PierIDType pierIdx, pgsTypes::PierFaceType face, GirderIndexType gdrIdx) const
{
   if (m_BridgeDescription.GetBearingType() == pgsTypes::brtBridge)
   {
      return m_BridgeDescription.GetBearingData();
   }
   else
   {
      GirderIndexType gdr = m_BridgeDescription.GetBearingType() == pgsTypes::brtGirder ? gdrIdx : 0;

      const CPierData2* pPier = m_BridgeDescription.GetPier(pierIdx);
      return pPier->GetBearingData(gdr,face);
   }
}

bool CProjectAgentImp::IsCompatibleGirder(const CGirderKey& girderKey, LPCTSTR lpszGirderName) const
{
   ASSERT_GIRDER_KEY(girderKey);
   const CGirderGroupData* pGroup = m_BridgeDescription.GetGirderGroup(girderKey.groupIndex);
   std::vector<std::_tstring> vGirderNames;
   GirderIndexType nGirders = pGroup->GetGirderCount();
   for (GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++)
   {
      const CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);
      vGirderNames.push_back(pGirder->GetGirderName());
   }
   vGirderNames[girderKey.girderIndex] = lpszGirderName;
   return AreGirdersCompatible(vGirderNames);
}

bool CProjectAgentImp::AreGirdersCompatible(GroupIndexType groupIdx) const
{
   GroupIndexType firstGrpIdx = (groupIdx == ALL_GROUPS ? 0 : groupIdx);
   GroupIndexType lastGrpIdx = (groupIdx == ALL_GROUPS ? m_BridgeDescription.GetGirderGroupCount() - 1 : groupIdx);
   for (GroupIndexType grpIdx = firstGrpIdx; grpIdx <= lastGrpIdx; grpIdx++)
   {
      const CGirderGroupData* pGroup = m_BridgeDescription.GetGirderGroup(grpIdx);

      std::vector<std::_tstring> vGirderNames;
      GirderIndexType nGirders = pGroup->GetGirderCount();
      for (GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++)
      {
         const CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);
         vGirderNames.push_back(pGirder->GetGirderName());
      }
      
      if (!AreGirdersCompatible(vGirderNames))
      {
         return false;
      }
   }
   return true;
}

bool CProjectAgentImp::AreGirdersCompatible(const std::vector<std::_tstring>& vGirderNames) const
{
   return AreGirdersCompatible(m_BridgeDescription, vGirderNames);
}

bool CProjectAgentImp::AreGirdersCompatible(const CBridgeDescription2& bridgeDescription,const std::vector<std::_tstring>& vGirderNames) const
{
   pgsTypes::SupportedDeckType deckType = bridgeDescription.GetDeckDescription()->GetDeckType();
   pgsTypes::SupportedBeamSpacing spacingType = bridgeDescription.GetGirderSpacingType();
   if (spacingType != pgsTypes::sbsConstantAdjacent)
   {
      // compatibilty check only applies to constant adjacent spacing
      return true;
   }

   const GirderLibraryEntry* pGirderEntry = GetGirderEntry(vGirderNames.front().c_str());
   CComPtr<IBeamFactory> factory;
   pGirderEntry->GetBeamFactory(&factory);
   const auto& dimensions = pGirderEntry->GetDimensions();
   Float64 Smin, Smax;
   factory->GetAllowableSpacingRange(dimensions, deckType, spacingType, &Smin, &Smax);

   auto& iter = vGirderNames.cbegin();
   iter++;
   const auto& end = vGirderNames.cend();
   for (; iter != end; iter++)
   {
      const auto& strGirderName(*iter);
      pGirderEntry = GetGirderEntry(strGirderName.c_str());
      factory.Release();
      pGirderEntry->GetBeamFactory(&factory);
      const auto& dimensions = pGirderEntry->GetDimensions();
      Float64 smin, smax;
      factory->GetAllowableSpacingRange(dimensions, deckType, spacingType, &smin, &smax);
      if (!InRange(Smin, smin, Smax) && !InRange(Smin, smax, Smax))
      {
         return false;
      }
   }
   return true;
}

void CProjectAgentImp::SetGirderSpacing(PierIndexType pierIdx,pgsTypes::PierFaceType face,const CGirderSpacing2& spacing)
{
   CPierData2* pPier = m_BridgeDescription.GetPier(pierIdx);
   CGirderSpacing2* pSpacing = pPier->GetGirderSpacing(face);

   if ( *pSpacing != spacing )
   {
      *pSpacing = spacing;
      Fire_BridgeChanged();
   }
}

void CProjectAgentImp::SetGirderSpacingAtStartOfGroup(GroupIndexType grpIdx,const CGirderSpacing2& spacing)
{
   CGirderGroupData* pGroup = m_BridgeDescription.GetGirderGroup(grpIdx);
   CPierData2* pPier = pGroup->GetPier(pgsTypes::metStart);
   PierIndexType pierIdx = pPier->GetIndex();
   SetGirderSpacing(pierIdx,pgsTypes::Ahead,spacing);
}

void CProjectAgentImp::SetGirderSpacingAtEndOfGroup(GroupIndexType grpIdx,const CGirderSpacing2& spacing)
{
   CGirderGroupData* pGroup = m_BridgeDescription.GetGirderGroup(grpIdx);
   CPierData2* pPier = pGroup->GetPier(pgsTypes::metEnd);
   PierIndexType pierIdx = pPier->GetIndex();
   SetGirderSpacing(pierIdx,pgsTypes::Back,spacing);
}

void CProjectAgentImp::SetGirderName(const CGirderKey& girderKey, LPCTSTR strGirderName)
{
   ATLASSERT( !m_BridgeDescription.UseSameGirderForEntireBridge() );
   CSplicedGirderData* pGirder = m_BridgeDescription.GetGirderGroup(girderKey.groupIndex)->GetGirder(girderKey.girderIndex);

   if ( std::_tstring(pGirder->GetGirderName()) != std::_tstring(strGirderName) )
   {
      ReleaseGirderLibraryEntries();

      GirderIndexType gdrIdx = pGirder->GetIndex();
      GroupIndexType girderTypeGroupIdx = pGirder->GetGirderGroup()->CreateGirderTypeGroup(girderKey.girderIndex,girderKey.girderIndex);
      pGirder->GetGirderGroup()->SetGirderName(girderTypeGroupIdx,strGirderName);

      UseGirderLibraryEntries();

#if defined _DEBUG
      ATLASSERT( pGirder->GetGirderLibraryEntry()->GetName() == std::_tstring(pGirder->GetGirderName()) );
#endif

      Fire_BridgeChanged();
   }
}

void CProjectAgentImp::SetGirderGroup(GroupIndexType grpIdx,const CGirderGroupData& girderGroup)
{
   CGirderGroupData* pGroup = m_BridgeDescription.GetGirderGroup(grpIdx);
   if ( *pGroup != girderGroup )
   {
      ReleaseBridgeLibraryEntries();
      pGroup->CopyGirderGroupData(&girderGroup,true/*copy data only*/);
      UseBridgeLibraryEntries();

      if ( !m_BridgeDescription.UseSameGirderForEntireBridge() || !m_BridgeDescription.UseSameNumberOfGirdersInAllGroups() )
      {
         Fire_BridgeChanged();
      }
   }
}

void CProjectAgentImp::SetGirderCount(GroupIndexType grpIdx,GirderIndexType nGirders)
{
   CGirderGroupData* pGroup = m_BridgeDescription.GetGirderGroup(grpIdx);
   if ( pGroup->GetGirderCount() != nGirders )
   {
      ReleaseGirderLibraryEntries();
      
      pGroup->SetGirderCount(nGirders);

      UseGirderLibraryEntries();
      Fire_BridgeChanged();
   }
}

void CProjectAgentImp::SetBoundaryCondition(PierIndexType pierIdx,pgsTypes::BoundaryConditionType connectionType)
{
   CPierData2* pPier = m_BridgeDescription.GetPier(pierIdx);
   ATLASSERT(pPier->IsBoundaryPier());// this should be a boundary pier
   if ( pPier->GetBoundaryConditionType() != connectionType )
   {
      pPier->SetBoundaryConditionType(connectionType);
      Fire_BridgeChanged();
   }
}

void CProjectAgentImp::SetBoundaryCondition(PierIndexType pierIdx,pgsTypes::PierSegmentConnectionType connectionType,EventIndexType castClosureEventIdx)
{
   CPierData2* pPier = m_BridgeDescription.GetPier(pierIdx);
   ATLASSERT(pPier->IsInteriorPier());// this should not be a boundary pier
   if ( pPier->GetSegmentConnectionType() != connectionType )
   {
      pPier->SetSegmentConnectionType(connectionType,castClosureEventIdx);
      Fire_BridgeChanged();
   }
}

void CProjectAgentImp::DeletePier(PierIndexType pierIdx,pgsTypes::PierFaceType faceForSpan)
{
   ATLASSERT( 1 < m_BridgeDescription.GetSpanCount() );
   if ( m_BridgeDescription.GetSpanCount() < 2 )
      return;

   SpanIndexType spanIdx;
   pgsTypes::RemovePierType removePierType;

   if (faceForSpan == pgsTypes::Back )
   {
      spanIdx = pierIdx - 1;
      removePierType = pgsTypes::NextPier;
   }
   else
   {
      spanIdx = pierIdx;
      removePierType = pgsTypes::PrevPier;
   }

   ReleaseBridgeLibraryEntries();

   m_BridgeDescription.RemoveSpan(spanIdx,removePierType);

   UseBridgeLibraryEntries();

   CBridgeChangedHint* pHint = new CBridgeChangedHint;
   pHint->PierIdx = pierIdx;
   pHint->PierFace = faceForSpan;
   pHint->bAdded = false;
   Fire_BridgeChanged(pHint);
}

void CProjectAgentImp::InsertSpan(PierIndexType refPierIdx,pgsTypes::PierFaceType pierFace,Float64 spanLength,const CSpanData2* pSpanData,const CPierData2* pPierData,bool bCreateNewGroup,EventIndexType eventIdx)
{
   ReleaseBridgeLibraryEntries();
   
   m_BridgeDescription.InsertSpan(refPierIdx,pierFace,spanLength,pSpanData,pPierData,bCreateNewGroup,eventIdx);
   
   UseBridgeLibraryEntries();

   CBridgeChangedHint* pHint = new CBridgeChangedHint;
   pHint->PierIdx = refPierIdx;
   pHint->PierFace = pierFace;
   pHint->bAdded = true;
   Fire_BridgeChanged(pHint);
}

void CProjectAgentImp::InsertTemporarySupport(CTemporarySupportData* pTSData,EventIndexType erectionEventIdx,EventIndexType removalEventIdx,EventIndexType castClosureJointEventIdx)
{
   m_BridgeDescription.AddTemporarySupport(pTSData,erectionEventIdx,removalEventIdx,castClosureJointEventIdx);
   Fire_BridgeChanged();
}

void CProjectAgentImp::DeleteTemporarySupportByIndex(SupportIndexType tsIdx)
{
   m_BridgeDescription.RemoveTemporarySupportByIndex(tsIdx);
   Fire_BridgeChanged();
}

void CProjectAgentImp::DeleteTemporarySupportByID(SupportIDType tsID)
{
   m_BridgeDescription.RemoveTemporarySupportByID(tsID);
   Fire_BridgeChanged();
}

void CProjectAgentImp::SetLiveLoadDistributionFactorMethod(pgsTypes::DistributionFactorMethod method)
{
   if ( m_BridgeDescription.GetDistributionFactorMethod() != method )
   {
      m_BridgeDescription.SetDistributionFactorMethod(method);
      Fire_LiveLoadChanged();
   }
}

pgsTypes::DistributionFactorMethod CProjectAgentImp::GetLiveLoadDistributionFactorMethod() const
{
   return m_BridgeDescription.GetDistributionFactorMethod();
}

void CProjectAgentImp::UseSameNumberOfGirdersInAllGroups(bool bUseSame)
{
   if ( m_BridgeDescription.UseSameNumberOfGirdersInAllGroups() != bUseSame )
   {
      ReleaseBridgeLibraryEntries();

      m_BridgeDescription.UseSameNumberOfGirdersInAllGroups(bUseSame);

      UseBridgeLibraryEntries();

      Fire_BridgeChanged();
   }
}

bool CProjectAgentImp::UseSameNumberOfGirdersInAllGroups() const
{
   return m_BridgeDescription.UseSameNumberOfGirdersInAllGroups();
}

void CProjectAgentImp::SetGirderCount(GirderIndexType nGirders)
{
   if ( m_BridgeDescription.GetGirderCount() != nGirders )
   {
      m_BridgeDescription.SetGirderCount(nGirders);
      if ( m_BridgeDescription.UseSameNumberOfGirdersInAllGroups() )
      {
         Fire_BridgeChanged();
      }
   }
}

void CProjectAgentImp::UseSameGirderForEntireBridge(bool bSame)
{
   if ( m_BridgeDescription.UseSameGirderForEntireBridge() != bSame )
   {
      ReleaseBridgeLibraryEntries();
      
      m_BridgeDescription.UseSameGirderForEntireBridge(bSame);
      
      UseBridgeLibraryEntries();

      Fire_BridgeChanged();
   }
}

bool CProjectAgentImp::UseSameGirderForEntireBridge() const
{
   return m_BridgeDescription.UseSameGirderForEntireBridge();
}

void CProjectAgentImp::SetGirderName(LPCTSTR strGirderName)
{
   if (std::_tstring(m_BridgeDescription.GetGirderName()) != std::_tstring(strGirderName))
   {
      ReleaseBridgeLibraryEntries();
      
      m_BridgeDescription.SetGirderName(strGirderName);
      
      UseBridgeLibraryEntries();

      if ( m_BridgeDescription.UseSameGirderForEntireBridge() )
      {
         Fire_BridgeChanged();
      }
   }
}

void CProjectAgentImp::SetGirderSpacingType(pgsTypes::SupportedBeamSpacing sbs)
{
   if ( m_BridgeDescription.GetGirderSpacingType() != sbs )
   {
      m_BridgeDescription.SetGirderSpacingType(sbs);
      Fire_BridgeChanged();
   }
}

pgsTypes::SupportedBeamSpacing CProjectAgentImp::GetGirderSpacingType() const
{
   return m_BridgeDescription.GetGirderSpacingType();
}

void CProjectAgentImp::SetGirderSpacing(Float64 spacing)
{
   if ( !IsEqual(m_BridgeDescription.GetGirderSpacing(),spacing) )
   {
      m_BridgeDescription.SetGirderSpacing(spacing);
      if ( IsBridgeSpacing(m_BridgeDescription.GetGirderSpacingType()) )
      {
         Fire_BridgeChanged();
      }
   }
}

void CProjectAgentImp::SetMeasurementType(pgsTypes::MeasurementType mt)
{
   if ( m_BridgeDescription.GetMeasurementType() != mt )
   {
      m_BridgeDescription.SetMeasurementType(mt);
      if ( IsBridgeSpacing(m_BridgeDescription.GetGirderSpacingType()) )
      {
         Fire_BridgeChanged();
      }
   }
}

pgsTypes::MeasurementType CProjectAgentImp::GetMeasurementType() const
{
   return m_BridgeDescription.GetMeasurementType();
}


void CProjectAgentImp::SetMeasurementLocation(pgsTypes::MeasurementLocation ml)
{
   if ( m_BridgeDescription.GetMeasurementLocation() != ml )
   {
      m_BridgeDescription.SetMeasurementLocation(ml);
      if ( IsBridgeSpacing(m_BridgeDescription.GetGirderSpacingType()) )
      {
         Fire_BridgeChanged();
      }
   }
}

pgsTypes::MeasurementLocation CProjectAgentImp::GetMeasurementLocation() const
{
   return  m_BridgeDescription.GetMeasurementLocation();
}

////////////////////////////////////////////////////////////////////////
// ISegmentData Methods
//
const matPsStrand* CProjectAgentImp::GetStrandMaterial(const CSegmentKey& segmentKey,pgsTypes::StrandType type) const
{
   if ( type == pgsTypes::Permanent )
   {
      type = pgsTypes::Straight;
   }

   const CGirderGroupData* pGroup = m_BridgeDescription.GetGirderGroup(segmentKey.groupIndex);
   const CSplicedGirderData* pGirder = pGroup->GetGirder(segmentKey.girderIndex);
   const CPrecastSegmentData* pSegment = pGirder->GetSegment(segmentKey.segmentIndex);

   return pSegment->Strands.GetStrandMaterial(type);
}

void CProjectAgentImp::SetStrandMaterial(const CSegmentKey& segmentKey,pgsTypes::StrandType type,const matPsStrand* pMaterial)
{
   ATLASSERT(pMaterial != nullptr);

   if ( type == pgsTypes::Permanent )
   {
      type = pgsTypes::Straight;
   }

   CGirderGroupData* pGroup = m_BridgeDescription.GetGirderGroup(segmentKey.groupIndex);
   CSplicedGirderData* pGirder = pGroup->GetGirder(segmentKey.girderIndex);
   CPrecastSegmentData* pSegment = pGirder->GetSegment(segmentKey.segmentIndex);
   if ( pSegment->Strands.GetStrandMaterial(type) != pMaterial )
   {
      pSegment->Strands.SetStrandMaterial(type,pMaterial);
      m_bUpdateJackingForce = true;
      Fire_GirderChanged(segmentKey,GCH_STRAND_MATERIAL);
   }
}

const CGirderMaterial* CProjectAgentImp::GetSegmentMaterial(const CSegmentKey& segmentKey) const
{
   const CPrecastSegmentData* pSegment = GetSegment(segmentKey);
   return &pSegment->Material;
}

void CProjectAgentImp::SetSegmentMaterial(const CSegmentKey& segmentKey,const CGirderMaterial& material)
{
   CPrecastSegmentData* pSegment = GetSegment(segmentKey);
   if ( pSegment->Material != material )
   {
      pSegment->Material = material;
      Fire_GirderChanged(segmentKey,GCH_CONCRETE);
   }
}

const CStrandData* CProjectAgentImp::GetStrandData(const CSegmentKey& segmentKey) const
{
   const CPrecastSegmentData* pSegment = GetSegment(segmentKey);
   return &pSegment->Strands;
}

void CProjectAgentImp::SetStrandData(const CSegmentKey& segmentKey,const CStrandData& strands)
{
   CPrecastSegmentData* pSegment = GetSegment(segmentKey);
   if ( pSegment->Strands != strands )
   {
      pSegment->Strands = strands;
      Fire_GirderChanged(segmentKey,GCH_PRESTRESSING_CONFIGURATION);
   }
}

const CHandlingData* CProjectAgentImp::GetHandlingData(const CSegmentKey& segmentKey) const
{
   const CPrecastSegmentData* pSegment = GetSegment(segmentKey);
   return &pSegment->HandlingData;
}

void CProjectAgentImp::ConvertLegacyDebondData(CPrecastSegmentData* pSegment, const GirderLibraryEntry* pGdrEntry)
{
   // legacy versions only had straight debonding
   std::vector<CDebondData>& rdebonds = pSegment->Strands.GetDebonding(pgsTypes::Straight);

   std::vector<CDebondData>::iterator it = rdebonds.begin();
   std::vector<CDebondData>::iterator itend = rdebonds.end();
   while(it != itend)
   {
      bool found = true;
      if (it->needsConversion)
      {
         // debond data comes from old version where strand id's (of total straight strands) were used
         found = false;
         StrandIndexType strandIdx = (StrandIndexType)(it->strandTypeGridIdx);
         ATLASSERT(strandIdx != INVALID_INDEX);
         // Determine grid index from the strand index
         StrandIndexType nStrands = 0;
         GridIndexType nGridSize = pGdrEntry->GetNumStraightStrandCoordinates();
         for (GridIndexType gridIdx = 0; gridIdx < nGridSize; gridIdx++)
         {
            Float64 xs, ys, xe, ye;
            bool bIsDebondable;
            pGdrEntry->GetStraightStrandCoordinates(gridIdx, &xs, &ys, &xe, &ye, &bIsDebondable);
            nStrands += (0.0 < xs || 0.0 < xe) ? 2 : 1; // number of strands at location
            if (strandIdx < nStrands)
            {
               // found our strand - make conversion if it's debondable
               if (bIsDebondable)
               {
                  it->strandTypeGridIdx = gridIdx;
                  found = true;
               }

               break;
            }
         }
      }

      if (!found)
      {
         // debonded strand not found - could be due to a library change, but assert for testing purposes
         ATLASSERT(false);
         rdebonds.erase(it);
         it = rdebonds.begin();  // restart loop
         itend = rdebonds.end();
      }
      else
      {
         it++;
      }
   }
}

void CProjectAgentImp::ConvertLegacyExtendedStrandData(CPrecastSegmentData* pSegment, const GirderLibraryEntry* pGdrEntry)
{
   if ( pSegment->Strands.m_bConvertExtendedStrands == false )
   {
      return; // no conversion needed
   }

   for ( int i = 0; i < 2; i++ )
   {
      pgsTypes::MemberEndType endType = (pgsTypes::MemberEndType)i;

      // legacy versions only had straight extended strands
      const std::vector<GridIndexType>& oldExtendStrands( pSegment->Strands.GetExtendedStrands(pgsTypes::Straight,endType) );
      std::vector<GridIndexType> newExtendedStrands;

      std::vector<GridIndexType>::const_iterator it(oldExtendStrands.begin());
      std::vector<GridIndexType>::const_iterator itend(oldExtendStrands.end());
      while(it != itend)
      {
         // extended strand data comes from old version where strand index (of total straight strands) were used
         StrandIndexType strandIdx = (StrandIndexType)(*it);
         ATLASSERT(strandIdx != INVALID_INDEX);

         // Determine grid index from the strand index
         StrandIndexType nStrands = 0;
         GridIndexType nGridSize = pGdrEntry->GetNumStraightStrandCoordinates();
         for (GridIndexType gridIdx = 0; gridIdx < nGridSize; gridIdx++)
         {
            Float64 xs, ys, xe, ye;
            bool bIsDebondable;
            pGdrEntry->GetStraightStrandCoordinates(gridIdx, &xs, &ys, &xe, &ye, &bIsDebondable);

            StrandIndexType nStrandsAtGridPosition = (0.0 < xs || 0.0 < xe) ? 2 : 1;
            nStrands += nStrandsAtGridPosition;
            if (strandIdx < nStrands)
            {
               newExtendedStrands.push_back(gridIdx);
               if ( 1 < nStrandsAtGridPosition )
               {
                  // if there are 2 strands for this grid position, skip the next
                  // extended strand (it is the companion to the one we just dealt with)
                  it++;
               }
               break;
            }
         }

         it++;
      }
      std::sort(newExtendedStrands.begin(),newExtendedStrands.end());
      pSegment->Strands.SetExtendedStrands(pgsTypes::Straight,endType,newExtendedStrands);
   }
}

void CProjectAgentImp::SetHandlingData(const CSegmentKey& segmentKey,const CHandlingData& handling)
{
   CPrecastSegmentData* pSegment = GetSegment(segmentKey);
   if ( pSegment->HandlingData != handling )
   {
      const HaulTruckLibrary* pHaulTruckLibrary = m_pLibMgr->GetHaulTruckLibrary();

      release_library_entry(&m_LibObserver,pSegment->HandlingData.pHaulTruckLibraryEntry,*pHaulTruckLibrary);

      pSegment->HandlingData = handling;

      use_library_entry(&m_LibObserver,pSegment->HandlingData.HaulTruckName,&pSegment->HandlingData.pHaulTruckLibraryEntry,*pHaulTruckLibrary);

      Fire_GirderChanged(segmentKey,GCH_LIFTING_CONFIGURATION | GCH_SHIPPING_CONFIGURATION);
   }
}

////////////////////////////////////////////////////////////////////////
// IShear Methods
//
std::_tstring CProjectAgentImp::GetSegmentStirrupMaterial(const CSegmentKey& segmentKey) const
{
   const CShearData2* pShearData = GetSegmentShearData(segmentKey);
   return lrfdRebarPool::GetMaterialName(pShearData->ShearBarType,pShearData->ShearBarGrade);
}

void CProjectAgentImp::GetSegmentStirrupMaterial(const CSegmentKey& segmentKey,matRebar::Type& type,matRebar::Grade& grade) const
{
   const CShearData2* pShearData = GetSegmentShearData(segmentKey);
   type = pShearData->ShearBarType;
   grade = pShearData->ShearBarGrade;
}

void CProjectAgentImp::SetSegmentStirrupMaterial(const CSegmentKey& segmentKey,matRebar::Type type,matRebar::Grade grade)
{
   CPrecastSegmentData* pSegment = GetSegment(segmentKey);
   if ( pSegment->ShearData.ShearBarType != type || pSegment->ShearData.ShearBarGrade != grade)
   {
      pSegment->ShearData.ShearBarType = type;
      pSegment->ShearData.ShearBarGrade = grade;
      Fire_GirderChanged(segmentKey,GCH_STIRRUPS);
   }
}

const CShearData2* CProjectAgentImp::GetSegmentShearData(const CSegmentKey& segmentKey) const
{
   const CPrecastSegmentData* pSegment = GetSegment(segmentKey);
   return &pSegment->ShearData;
}

void CProjectAgentImp::SetSegmentShearData(const CSegmentKey& segmentKey,const CShearData2& shearData)
{
   CPrecastSegmentData* pSegment = GetSegment(segmentKey);
   if ( pSegment->ShearData != shearData )
   {
      pSegment->ShearData = shearData;
      Fire_GirderChanged(segmentKey,GCH_STIRRUPS);
   }
}

std::_tstring CProjectAgentImp::GetClosureJointStirrupMaterial(const CClosureKey& closureKey) const
{
   const CShearData2* pShearData = GetClosureJointShearData(closureKey);
   return lrfdRebarPool::GetMaterialName(pShearData->ShearBarType,pShearData->ShearBarGrade);
}

void CProjectAgentImp::GetClosureJointStirrupMaterial(const CClosureKey& closureKey,matRebar::Type& type,matRebar::Grade& grade) const
{
   const CShearData2* pShearData = GetClosureJointShearData(closureKey);
   type = pShearData->ShearBarType;
   grade = pShearData->ShearBarGrade;
}

void CProjectAgentImp::SetClosureJointStirrupMaterial(const CClosureKey& closureKey,matRebar::Type type,matRebar::Grade grade)
{
   CPrecastSegmentData* pSegment = GetSegment(closureKey);
   CClosureJointData* pClosureJoint = pSegment->GetEndClosure();
   if ( pClosureJoint->GetStirrups().ShearBarType != type || pClosureJoint->GetStirrups().ShearBarGrade != grade)
   {
      pClosureJoint->GetStirrups().ShearBarType = type;
      pClosureJoint->GetStirrups().ShearBarGrade = grade;
      Fire_GirderChanged(closureKey,GCH_STIRRUPS);
   }
}

const CShearData2* CProjectAgentImp::GetClosureJointShearData(const CClosureKey& closureKey) const
{
   const CPrecastSegmentData* pSegment = GetSegment(closureKey);
   const CClosureJointData* pClosureJoint = pSegment->GetEndClosure();
   if ( pClosureJoint == nullptr )
   {
      return nullptr;
   }

   return &pClosureJoint->GetStirrups();
}

void CProjectAgentImp::SetClosureJointShearData(const CClosureKey& closureKey,const CShearData2& shearData)
{
   CPrecastSegmentData* pSegment = GetSegment(closureKey);
   CClosureJointData* pClosureJoint = pSegment->GetEndClosure();
   if ( pClosureJoint && pClosureJoint->GetStirrups() != shearData )
   {
      pClosureJoint->SetStirrups(shearData);
      Fire_GirderChanged(closureKey,GCH_STIRRUPS);
   }
}

/////////////////////////////////////////////////
// ILongitudinalRebar
std::_tstring CProjectAgentImp::GetSegmentLongitudinalRebarMaterial(const CSegmentKey& segmentKey) const
{
   const CLongitudinalRebarData* pLRD = GetSegmentLongitudinalRebarData(segmentKey);
   return lrfdRebarPool::GetMaterialName(pLRD->BarType,pLRD->BarGrade);
}

void CProjectAgentImp::GetSegmentLongitudinalRebarMaterial(const CSegmentKey& segmentKey,matRebar::Type& type,matRebar::Grade& grade) const
{
   const CLongitudinalRebarData* pLRD = GetSegmentLongitudinalRebarData(segmentKey);
   grade = pLRD->BarGrade;
   type = pLRD->BarType;
}

void CProjectAgentImp::SetSegmentLongitudinalRebarMaterial(const CSegmentKey& segmentKey,matRebar::Type type,matRebar::Grade grade)
{
   CPrecastSegmentData* pSegment = GetSegment(segmentKey);
   if ( pSegment->LongitudinalRebarData.BarGrade != grade || pSegment->LongitudinalRebarData.BarType != type )
   {
      pSegment->LongitudinalRebarData.BarGrade = grade;
      pSegment->LongitudinalRebarData.BarType = type;
      Fire_GirderChanged(segmentKey,GCH_LONGITUDINAL_REBAR);
   }
}

const CLongitudinalRebarData* CProjectAgentImp::GetSegmentLongitudinalRebarData(const CSegmentKey& segmentKey) const
{
   const CPrecastSegmentData* pSegment = GetSegment(segmentKey);
   return &pSegment->LongitudinalRebarData;
}

void CProjectAgentImp::SetSegmentLongitudinalRebarData(const CSegmentKey& segmentKey,const CLongitudinalRebarData& data)
{
   CPrecastSegmentData* pSegment = GetSegment(segmentKey);
   if ( pSegment->LongitudinalRebarData != data )
   {
      pSegment->LongitudinalRebarData = data;
      Fire_GirderChanged(segmentKey,GCH_LONGITUDINAL_REBAR);
   }
}

std::_tstring CProjectAgentImp::GetClosureJointLongitudinalRebarMaterial(const CClosureKey& closureKey) const
{
   const CLongitudinalRebarData* pLRD = GetClosureJointLongitudinalRebarData(closureKey);
   return lrfdRebarPool::GetMaterialName(pLRD->BarType,pLRD->BarGrade);
}

void CProjectAgentImp::GetClosureJointLongitudinalRebarMaterial(const CClosureKey& closureKey,matRebar::Type& type,matRebar::Grade& grade) const
{
   const CLongitudinalRebarData* pLRD = GetClosureJointLongitudinalRebarData(closureKey);
   grade = pLRD->BarGrade;
   type = pLRD->BarType;
}

void CProjectAgentImp::SetClosureJointLongitudinalRebarMaterial(const CClosureKey& closureKey,matRebar::Type type,matRebar::Grade grade)
{
   CPrecastSegmentData* pSegment = GetSegment(closureKey);
   CClosureJointData* pClosureJoint = pSegment->GetEndClosure();
   if ( pClosureJoint->GetRebar().BarGrade != grade || pClosureJoint->GetRebar().BarType != type )
   {
      pClosureJoint->GetRebar().BarGrade = grade;
      pClosureJoint->GetRebar().BarType = type;
      Fire_GirderChanged(closureKey,GCH_LONGITUDINAL_REBAR);
   }
}

const CLongitudinalRebarData* CProjectAgentImp::GetClosureJointLongitudinalRebarData(const CClosureKey& closureKey) const
{
   const CPrecastSegmentData* pSegment = GetSegment(closureKey);
   const CClosureJointData* pClosureJoint = pSegment->GetEndClosure();
   return &pClosureJoint->GetRebar();
}

void CProjectAgentImp::SetClosureJointLongitudinalRebarData(const CClosureKey& closureKey,const CLongitudinalRebarData& data)
{
   CPrecastSegmentData* pSegment = GetSegment(closureKey);
   CClosureJointData* pClosureJoint = pSegment->GetEndClosure();
   if ( pClosureJoint->GetRebar() != data )
   {
      pClosureJoint->GetRebar() = data;
      Fire_GirderChanged(closureKey,GCH_LONGITUDINAL_REBAR);
   }
}

////////////////////////////////////////////////////////////////////////
// ISegmentLifting Methods
//
Float64 CProjectAgentImp::GetLeftLiftingLoopLocation(const CSegmentKey& segmentKey) const
{
   const CPrecastSegmentData* pSegment = GetSegment(segmentKey);
   return pSegment->HandlingData.LeftLiftPoint;
}

Float64 CProjectAgentImp::GetRightLiftingLoopLocation(const CSegmentKey& segmentKey) const
{
   const CPrecastSegmentData* pSegment = GetSegment(segmentKey);
   return pSegment->HandlingData.RightLiftPoint;
}

void CProjectAgentImp::SetLiftingLoopLocations(const CSegmentKey& segmentKey,Float64 left,Float64 right)
{
   CPrecastSegmentData* pSegment = GetSegment(segmentKey);
   if ( !IsEqual(pSegment->HandlingData.LeftLiftPoint,left) ||
        !IsEqual(pSegment->HandlingData.RightLiftPoint,right) )
   {
      pSegment->HandlingData.LeftLiftPoint = left;
      pSegment->HandlingData.RightLiftPoint = right;
      Fire_GirderChanged(segmentKey,GCH_LIFTING_CONFIGURATION);
   }
}

////////////////////////////////////////////////////////////////////////
// ISegmentHauling Methods
//

Float64 CProjectAgentImp::GetTrailingOverhang(const CSegmentKey& segmentKey) const
{
   const CPrecastSegmentData* pSegment = GetSegment(segmentKey);
   return pSegment->HandlingData.TrailingSupportPoint;
}

Float64 CProjectAgentImp::GetLeadingOverhang(const CSegmentKey& segmentKey) const
{
   const CPrecastSegmentData* pSegment = GetSegment(segmentKey);
   return pSegment->HandlingData.LeadingSupportPoint;
}

void CProjectAgentImp::SetTruckSupportLocations(const CSegmentKey& segmentKey,Float64 trailing,Float64 leading)
{
   CPrecastSegmentData* pSegment = GetSegment(segmentKey);
   if ( !IsEqual(pSegment->HandlingData.TrailingSupportPoint,trailing) ||
        !IsEqual(pSegment->HandlingData.LeadingSupportPoint,leading) )
   {
      pSegment->HandlingData.TrailingSupportPoint = trailing;
      pSegment->HandlingData.LeadingSupportPoint = leading;
      Fire_GirderChanged(segmentKey,GCH_SHIPPING_CONFIGURATION);
   }
}

LPCTSTR CProjectAgentImp::GetHaulTruck(const CSegmentKey& segmentKey) const
{
   const CPrecastSegmentData* pSegment = GetSegment(segmentKey);
   return pSegment->HandlingData.HaulTruckName.c_str();
}

void CProjectAgentImp::SetHaulTruck(const CSegmentKey& segmentKey,LPCTSTR lpszHaulTruck)
{
   CPrecastSegmentData* pSegment = GetSegment(segmentKey);
   if ( pSegment->HandlingData.HaulTruckName != std::_tstring(lpszHaulTruck) )
   {
      const HaulTruckLibrary* pHaulTruckLibrary = m_pLibMgr->GetHaulTruckLibrary();
      release_library_entry(&m_LibObserver,pSegment->HandlingData.pHaulTruckLibraryEntry,*pHaulTruckLibrary);

      pSegment->HandlingData.HaulTruckName = lpszHaulTruck;

      const HaulTruckLibraryEntry* pHaulTruckEntry;
      use_library_entry(&m_LibObserver,pSegment->HandlingData.HaulTruckName,&pHaulTruckEntry,*pHaulTruckLibrary);
      pSegment->HandlingData.pHaulTruckLibraryEntry = pHaulTruckEntry;

      Fire_GirderChanged(segmentKey,GCH_SHIPPING_CONFIGURATION);
   }
}

////////////////////////////////////////////////////////////////////////
// IRatingSpecification Methods
//
bool CProjectAgentImp::AlwaysLoadRate() const
{
   const RatingLibraryEntry* pRatingEntry = GetRatingEntry(m_RatingSpec.c_str());
   return pRatingEntry->AlwaysLoadRate();
}

bool CProjectAgentImp::IsRatingEnabled() const
{
   for (int i = 0; i < (int)pgsTypes::lrLoadRatingTypeCount; i++)
   {
      pgsTypes::LoadRatingType ratingType = (pgsTypes::LoadRatingType)i;
      if (m_bEnableRating[ratingType])
      {
         return true;
      }
   }
   return false;
}

bool CProjectAgentImp::IsRatingEnabled(pgsTypes::LoadRatingType ratingType) const
{
   return m_bEnableRating[ratingType];
}

void CProjectAgentImp::EnableRating(pgsTypes::LoadRatingType ratingType,bool bEnable)
{
   if ( m_bEnableRating[ratingType] != bEnable )
   {
      m_bEnableRating[ratingType] = bEnable;
      RatingSpecificationChanged(true);
   }
}

std::_tstring CProjectAgentImp::GetRatingSpecification() const
{
   return m_RatingSpec;
}

void CProjectAgentImp::SetRatingSpecification(const std::_tstring& spec)
{
   if ( m_RatingSpec != spec )
   {
      InitRatingSpecification(spec);
      RatingSpecificationChanged(true);
   }
}

void CProjectAgentImp::IncludePedestrianLiveLoad(bool bInclude)
{
   if ( m_bIncludePedestrianLiveLoad != bInclude )
   {
      m_bIncludePedestrianLiveLoad = bInclude;
      RatingSpecificationChanged(true);
   }
}

bool CProjectAgentImp::IncludePedestrianLiveLoad() const
{
   return m_bIncludePedestrianLiveLoad;
}

void CProjectAgentImp::SetADTT(Int16 adtt)
{
   if ( m_ADTT != adtt )
   {
      m_ADTT = adtt;
      RatingSpecificationChanged(true);
   }
}

Int16 CProjectAgentImp::GetADTT() const
{
   return m_ADTT;
}

void CProjectAgentImp::SetGirderConditionFactor(const CSegmentKey& girderKey,pgsTypes::ConditionFactorType conditionFactorType,Float64 conditionFactor)
{
   CGirderGroupData* pGroup = m_BridgeDescription.GetGirderGroup(girderKey.groupIndex);
   CSplicedGirderData* pGirder = pGroup->GetGirder(girderKey.girderIndex);

   if ( pGirder->GetConditionFactorType() != conditionFactorType || !IsEqual(pGirder->GetConditionFactor(),conditionFactor) )
   {
      pGirder->SetConditionFactorType(conditionFactorType);
      pGirder->SetConditionFactor(conditionFactor);
#pragma Reminder("UPDATE: fire an event???")
      // change condition factor and see if load rating report refreshes
   }
}

void CProjectAgentImp::GetGirderConditionFactor(const CSegmentKey& girderKey,pgsTypes::ConditionFactorType* pConditionFactorType,Float64 *pConditionFactor) const
{
   const CSplicedGirderData* pGirder = GetGirder(girderKey);
   *pConditionFactor = pGirder->GetConditionFactor();
   *pConditionFactorType = pGirder->GetConditionFactorType();
}

Float64 CProjectAgentImp::GetGirderConditionFactor(const CSegmentKey& girderKey) const
{
   const CSplicedGirderData* pGirder = GetGirder(girderKey);
   return pGirder->GetConditionFactor();
}

void CProjectAgentImp::SetDeckConditionFactor(pgsTypes::ConditionFactorType conditionFactorType,Float64 conditionFactor)
{
   CDeckDescription2* pDeck = m_BridgeDescription.GetDeckDescription();

   if ( pDeck->Condition != conditionFactorType || !IsEqual(pDeck->ConditionFactor,conditionFactor) )
   {
      pDeck->Condition = conditionFactorType;
      pDeck->ConditionFactor = conditionFactor;

      RatingSpecificationChanged(true);
   }
}

void CProjectAgentImp::GetDeckConditionFactor(pgsTypes::ConditionFactorType* pConditionFactorType,Float64 *pConditionFactor) const
{
   const CDeckDescription2* pDeck = m_BridgeDescription.GetDeckDescription();
   *pConditionFactorType = pDeck->Condition;
   *pConditionFactor     = pDeck->ConditionFactor;
}

Float64 CProjectAgentImp::GetDeckConditionFactor() const
{
   const CDeckDescription2* pDeck = m_BridgeDescription.GetDeckDescription();
   return pDeck->ConditionFactor;
}

void CProjectAgentImp::SetSystemFactorFlexure(Float64 sysFactor)
{
   if ( !IsEqual(m_SystemFactorFlexure,sysFactor) )
   {
      m_SystemFactorFlexure = sysFactor;
      RatingSpecificationChanged(true);
   }
}

Float64 CProjectAgentImp::GetSystemFactorFlexure() const
{
   return m_SystemFactorFlexure;
}

void CProjectAgentImp::SetSystemFactorShear(Float64 sysFactor)
{
   if ( !IsEqual(m_SystemFactorShear,sysFactor) )
   {
      m_SystemFactorShear = sysFactor;
      RatingSpecificationChanged(true);
   }
}

Float64 CProjectAgentImp::GetSystemFactorShear() const
{
   return m_SystemFactorShear;
}

void CProjectAgentImp::SetDeadLoadFactor(pgsTypes::LimitState ls,Float64 gDC)
{
   if ( !IsEqual(m_gDC[ls],gDC) )
   {
      m_gDC[ls] = gDC;
      RatingSpecificationChanged(true);
   }
}

Float64 CProjectAgentImp::GetDeadLoadFactor(pgsTypes::LimitState ls) const
{
   return m_gDC[ls];
}

void CProjectAgentImp::SetWearingSurfaceFactor(pgsTypes::LimitState ls,Float64 gDW)
{
   if ( !IsEqual(m_gDW[ls],gDW) )
   {
      m_gDW[ls] = gDW;
      RatingSpecificationChanged(true);
   }
}

Float64 CProjectAgentImp::GetWearingSurfaceFactor(pgsTypes::LimitState ls) const
{
   return m_gDW[ls];
}

void CProjectAgentImp::SetCreepFactor(pgsTypes::LimitState ls,Float64 gCR)
{
   if ( !IsEqual(m_gCR[ls],gCR) )
   {
      m_gCR[ls] = gCR;
      RatingSpecificationChanged(true);
   }
}

Float64 CProjectAgentImp::GetCreepFactor(pgsTypes::LimitState ls) const
{
   return m_gCR[ls];
}

void CProjectAgentImp::SetShrinkageFactor(pgsTypes::LimitState ls,Float64 gSH)
{
   if ( !IsEqual(m_gSH[ls],gSH) )
   {
      m_gSH[ls] = gSH;
      RatingSpecificationChanged(true);
   }
}

Float64 CProjectAgentImp::GetShrinkageFactor(pgsTypes::LimitState ls) const
{
   return m_gSH[ls];
}


void CProjectAgentImp::SetRelaxationFactor(pgsTypes::LimitState ls,Float64 gRE)
{
   if ( !IsEqual(m_gRE[ls],gRE) )
   {
      m_gRE[ls] = gRE;
      RatingSpecificationChanged(true);
   }
}

Float64 CProjectAgentImp::GetRelaxationFactor(pgsTypes::LimitState ls) const
{
   return m_gRE[ls];
}

void CProjectAgentImp::SetSecondaryEffectsFactor(pgsTypes::LimitState ls,Float64 gPS)
{
   if ( !IsEqual(m_gPS[ls],gPS) )
   {
      m_gPS[ls] = gPS;
      RatingSpecificationChanged(true);
   }
}

Float64 CProjectAgentImp::GetSecondaryEffectsFactor(pgsTypes::LimitState ls) const
{
   return m_gPS[ls];
}

void CProjectAgentImp::SetLiveLoadFactor(pgsTypes::LimitState ls,Float64 gLL)
{
   if ( !IsEqual(m_gLL[ls],gLL) )
   {
      m_gLL[ls] = gLL;
      RatingSpecificationChanged(true);
   }
}

Float64 CProjectAgentImp::GetLiveLoadFactor(pgsTypes::LimitState ls,bool bResolveIfDefault) const
{
   const RatingLibraryEntry* pRatingEntry = GetRatingEntry( m_RatingSpec.c_str() );
   return GetLiveLoadFactor(ls,GetSpecialPermitType(),GetADTT(),pRatingEntry,bResolveIfDefault);
}

Float64 CProjectAgentImp::GetLiveLoadFactor(pgsTypes::LimitState ls,pgsTypes::SpecialPermitType specialPermitType,Int16 adtt,const RatingLibraryEntry* pRatingEntry,bool bResolveIfDefault) const
{
   // returns < 0 if needs to be computed from rating library entry
   Float64 gLL = m_gLL[ls];
   if ( gLL < 0 && bResolveIfDefault )
   {
      pgsTypes::LoadRatingType ratingType = ::RatingTypeFromLimitState(ls);
      if ( pRatingEntry->GetSpecificationVersion() < lrfrVersionMgr::SecondEditionWith2013Interims )
      {
         CLiveLoadFactorModel model;
         if ( ratingType == pgsTypes::lrPermit_Routine )
         {
            model = pRatingEntry->GetLiveLoadFactorModel(pgsTypes::lrPermit_Routine);
         }
         else if ( ratingType == pgsTypes::lrPermit_Special )
         {
            model = pRatingEntry->GetLiveLoadFactorModel( GetSpecialPermitType() );
         }
         else
         {
            model = pRatingEntry->GetLiveLoadFactorModel(ratingType);
         }

         if ( ::IsStrengthLimitState(ls) )
         {
            if ( model.GetLiveLoadFactorType() != pgsTypes::gllBilinearWithWeight )
            {
               gLL = model.GetStrengthLiveLoadFactor(adtt,0);
            }

            // gLL will be < 0 if gLL is a function of axle weight and load combination poi
         }
         else
         {
            gLL = model.GetServiceLiveLoadFactor(adtt);
         }
      }
      else
      {
         CLiveLoadFactorModel2 model;
         if ( ratingType == pgsTypes::lrPermit_Routine )
         {
            model = pRatingEntry->GetLiveLoadFactorModel2(pgsTypes::lrPermit_Routine);
         }
         else if ( ratingType == pgsTypes::lrPermit_Special )
         {
            model = pRatingEntry->GetLiveLoadFactorModel2( specialPermitType );
         }
         else
         {
            model = pRatingEntry->GetLiveLoadFactorModel2(ratingType);
         }

         if ( ::IsStrengthLimitState(ls) )
         {
            if ( model.GetLiveLoadFactorType() != pgsTypes::gllBilinearWithWeight )
            {
               gLL = model.GetStrengthLiveLoadFactor(adtt,0);
            }

            // gLL will be < 0 if gLL is a function of axle weight and load combination poi
         }
         else
         {
            gLL = model.GetServiceLiveLoadFactor(adtt);
         }
      }
   }

   return gLL; // this will return < 0 if gLL is a function of axle weight and load combination poi
}

void CProjectAgentImp::SetAllowableTensionCoefficient(pgsTypes::LoadRatingType ratingType,Float64 t)
{
   if ( !IsEqual(m_AllowableTensionCoefficient[ratingType],t) )
   {
      m_AllowableTensionCoefficient[ratingType] = t;
      RatingSpecificationChanged(true);
   }
}

Float64 CProjectAgentImp::GetAllowableTensionCoefficient(pgsTypes::LoadRatingType ratingType) const
{
   return m_AllowableTensionCoefficient[ratingType];
}

void CProjectAgentImp::RateForStress(pgsTypes::LoadRatingType ratingType,bool bRateForStress)
{
   if ( m_bRateForStress[ratingType] != bRateForStress )
   {
      m_bRateForStress[ratingType] = bRateForStress;
      RatingSpecificationChanged(true);
   }
}

bool CProjectAgentImp::RateForStress(pgsTypes::LoadRatingType ratingType) const
{
   return m_bRateForStress[ratingType];
}

void CProjectAgentImp::RateForShear(pgsTypes::LoadRatingType ratingType,bool bRateForShear)
{
   if ( m_bRateForShear[ratingType] != bRateForShear )
   {
      m_bRateForShear[ratingType] = bRateForShear;
      RatingSpecificationChanged(true);
   }
}

bool CProjectAgentImp::RateForShear(pgsTypes::LoadRatingType ratingType) const
{
   return m_bRateForShear[ratingType];
}

void CProjectAgentImp::ExcludeLegalLoadLaneLoading(bool bExclude)
{
   if ( m_bExcludeLegalLoadLaneLoading != bExclude )
   {
      m_bExcludeLegalLoadLaneLoading = bExclude;
      RatingSpecificationChanged(true);
   }
}

bool CProjectAgentImp::ExcludeLegalLoadLaneLoading() const
{
   return m_bExcludeLegalLoadLaneLoading;
}

void CProjectAgentImp::CheckYieldStress(pgsTypes::LoadRatingType ratingType,bool bCheckYieldStress)
{
   if ( m_bCheckYieldStress[ratingType] != bCheckYieldStress )
   {
      m_bCheckYieldStress[ratingType] = bCheckYieldStress;
      RatingSpecificationChanged(true);
   }
}

bool CProjectAgentImp::CheckYieldStress(pgsTypes::LoadRatingType ratingType) const
{
   return m_bCheckYieldStress[ratingType];
}

void CProjectAgentImp::SetYieldStressLimitCoefficient(Float64 x)
{
   if ( !IsEqual(x,m_AllowableYieldStressCoefficient) )
   {
      m_AllowableYieldStressCoefficient = x;
      RatingSpecificationChanged(true);
   }
}

Float64 CProjectAgentImp::GetYieldStressLimitCoefficient() const
{
   return m_AllowableYieldStressCoefficient;
}

void CProjectAgentImp::SetSpecialPermitType(pgsTypes::SpecialPermitType type)
{
   if ( type != m_SpecialPermitType )
   {
      m_SpecialPermitType = type;
      RatingSpecificationChanged(true);
   }
}

pgsTypes::SpecialPermitType CProjectAgentImp::GetSpecialPermitType() const
{
   return m_SpecialPermitType;
}

Float64 CProjectAgentImp::GetStrengthLiveLoadFactor(pgsTypes::LoadRatingType ratingType,AxleConfiguration& axleConfig) const
{
   ATLASSERT(::IsPermitRatingType(ratingType));

#if defined _DEBUG
   pgsTypes::LimitState ls = ::GetStrengthLimitStateType(ratingType);
   ATLASSERT( GetLiveLoadFactor(ls,true) < 0 );
#endif

   Float64 sum_axle_weight = 0; // sum of axle weights on the bridge
   Float64 firstAxleLocation = -1;
   Float64 lastAxleLocation = 0;
   for (const auto& axle_placement : axleConfig)
   {
      sum_axle_weight += axle_placement.Weight;

      if ( !IsZero(axle_placement.Weight) )
      {
         if ( firstAxleLocation < 0 )
         {
            firstAxleLocation = axle_placement.Location;
         }

         lastAxleLocation = axle_placement.Location;
      }
   }
   
   Float64 AL = fabs(firstAxleLocation - lastAxleLocation); // front axle to rear axle length (for axles on the bridge)

   Float64 gLL = 0;
   const RatingLibraryEntry* pRatingEntry = GetRatingEntry( GetRatingSpecification().c_str() );
   if ( pRatingEntry->GetSpecificationVersion() < lrfrVersionMgr::SecondEditionWith2013Interims )
   {
      CLiveLoadFactorModel model;
      if ( ratingType == pgsTypes::lrPermit_Special )
      {
         model = pRatingEntry->GetLiveLoadFactorModel(GetSpecialPermitType());
      }
      else
      {
         model = pRatingEntry->GetLiveLoadFactorModel(ratingType);
      }

      gLL = model.GetStrengthLiveLoadFactor(GetADTT(),sum_axle_weight);
   }
   else
   {
      Float64 GVW = sum_axle_weight;
      Float64 PermitWeightRatio = IsZero(AL) ? 0 : GVW/AL;
      CLiveLoadFactorModel2 model;
      if ( ratingType == pgsTypes::lrPermit_Special )
      {
         model = pRatingEntry->GetLiveLoadFactorModel2(GetSpecialPermitType());
      }
      else
      {
         model = pRatingEntry->GetLiveLoadFactorModel2(ratingType);
      }

      gLL = model.GetStrengthLiveLoadFactor(GetADTT(),PermitWeightRatio);
   }

   return gLL;
}

Float64 CProjectAgentImp::GetServiceLiveLoadFactor(pgsTypes::LoadRatingType ratingType) const
{
   ATLASSERT(::IsPermitRatingType(ratingType));

#if defined _DEBUG
   pgsTypes::LimitState ls = ::GetStrengthLimitStateType(ratingType);
   ATLASSERT( GetLiveLoadFactor(ls,true) < 0 );
#endif

   Float64 gLL = 0;
   const RatingLibraryEntry* pRatingEntry = GetRatingEntry( GetRatingSpecification().c_str() );
   if ( pRatingEntry->GetSpecificationVersion() < lrfrVersionMgr::SecondEditionWith2013Interims )
   {
      CLiveLoadFactorModel model;
      if ( ratingType == pgsTypes::lrPermit_Special )
      {
         model = pRatingEntry->GetLiveLoadFactorModel(GetSpecialPermitType());
      }
      else
      {
         model = pRatingEntry->GetLiveLoadFactorModel(ratingType);
      }

      gLL = model.GetServiceLiveLoadFactor(GetADTT());
   }
   else
   {
      CLiveLoadFactorModel2 model;
      if ( ratingType == pgsTypes::lrPermit_Special )
      {
         model = pRatingEntry->GetLiveLoadFactorModel2(GetSpecialPermitType());
      }
      else
      {
         model = pRatingEntry->GetLiveLoadFactorModel2(ratingType);
      }

      gLL = model.GetServiceLiveLoadFactor(GetADTT());
   }

   return gLL;
}

Float64 CProjectAgentImp::GetReactionStrengthLiveLoadFactor(PierIndexType pierIdx,GirderIndexType gdrIdx,pgsTypes::LoadRatingType ratingType,VehicleIndexType vehicleIdx) const
{
   pgsTypes::LimitState ls = ::GetStrengthLimitStateType(ratingType);
   Float64 gLL = GetLiveLoadFactor(ls,true);
   if ( gLL < 0 )
   {
      gLL = GetServiceLiveLoadFactor(ratingType);
   }
   return gLL;
}

Float64 CProjectAgentImp::GetReactionServiceLiveLoadFactor(PierIndexType pierIdx,GirderIndexType gdrIdx,pgsTypes::LoadRatingType ratingType,VehicleIndexType vehicleIdx) const
{
   pgsTypes::LimitState ls = ::GetServiceLimitStateType(ratingType);
   Float64 gLL = GetLiveLoadFactor(ls,true);
   if ( gLL < 0 )
   {
      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();

      pgsTypes::LiveLoadType llType = ::GetLiveLoadType(ratingType);

      GET_IFACE(IProductForces,pProductForces);
      pgsTypes::BridgeAnalysisType bat = pProductForces->GetBridgeAnalysisType(pgsTypes::Maximize);

      GET_IFACE(IReactions,pReactions);

      GroupIndexType grpIdx;
      if ( pierIdx == 0 )
      {
         grpIdx = 0;
      }
      else if ( pierIdx == m_BridgeDescription.GetPierCount()-1 )
      {
         grpIdx = m_BridgeDescription.GetGirderGroupCount() - 1;
      }
      else
      {
         grpIdx = m_BridgeDescription.GetPier(pierIdx)->GetGirderGroup(pgsTypes::Ahead)->GetIndex();
      }
      CGirderKey girderKey(grpIdx,gdrIdx);

      REACTION Rmin, Rmax;
      AxleConfiguration minAxleConfig, maxAxleConfig;
      if ( vehicleIdx == INVALID_INDEX )
      {
         IndexType minVehicleIdx, maxVehicleIdx;
         pReactions->GetLiveLoadReaction(liveLoadIntervalIdx,llType,pierIdx,girderKey,bat,true/*include impact*/,false/*no LLDF*/,pgsTypes::fetFy, &Rmin,&Rmax,&minVehicleIdx,&maxVehicleIdx);
         
         REACTION rmin,rmax;
         pReactions->GetVehicularLiveLoadReaction(liveLoadIntervalIdx,llType,maxVehicleIdx,pierIdx,girderKey,bat,true/*include impact*/,false/*no LLDF*/,&rmin,&rmax,nullptr,&maxAxleConfig);
         ATLASSERT(Rmax == rmax);
      }
      else
      {
         pReactions->GetVehicularLiveLoadReaction(liveLoadIntervalIdx,llType,vehicleIdx,pierIdx,girderKey,bat,true/*include impact*/,false/*no LLDF*/,&Rmin,&Rmax,&minAxleConfig,&maxAxleConfig);
      }

      gLL = GetStrengthLiveLoadFactor(ratingType,maxAxleConfig);
   }
   return gLL;
}

////////////////////////////////////////////////////////////////////////
// ISpecification Methods
//
std::_tstring CProjectAgentImp::GetSpecification() const
{
   return m_Spec;
}

void CProjectAgentImp::SetSpecification(const std::_tstring& spec)
{
   if ( m_Spec != spec )
   {
      bool oldSpecTimeStepMethod = m_pSpecEntry->GetLossMethod() == LOSSES_TIME_STEP ? true : false;
      
      InitSpecification(spec);
      
      bool newSpecTimeStepMethod = m_pSpecEntry->GetLossMethod() == LOSSES_TIME_STEP ? true : false;

      if ( oldSpecTimeStepMethod == false && newSpecTimeStepMethod == true )
      {
         // must do this before calling SpecificationChanged
         UpdateTimeDependentMaterials();
      }
      
      SpecificationChanged(true);
   }
}

void CProjectAgentImp::GetTrafficBarrierDistribution(GirderIndexType* pNGirders,pgsTypes::TrafficBarrierDistribution* pDistType) const
{
   *pNGirders = m_pSpecEntry->GetMaxGirdersDistTrafficBarrier();
   *pDistType = m_pSpecEntry->GetTrafficBarrierDistributionType();
}

pgsTypes::OverlayLoadDistributionType CProjectAgentImp::GetOverlayLoadDistributionType() const
{
   return m_pSpecEntry->GetOverlayLoadDistributionType();
}

pgsTypes::HaunchLoadComputationType CProjectAgentImp::GetHaunchLoadComputationType() const
{
   if(m_pSpecEntry->GetLossMethod() == LOSSES_TIME_STEP)
   {
      // Practically impossible to compute excess camber on the fly for spliced girders. Don't even try
      return pgsTypes::hlcZeroCamber;
   }
   else
   {
      return m_pSpecEntry->GetHaunchLoadComputationType();
   }
}

Float64 CProjectAgentImp::GetCamberTolerance() const
{
   ATLASSERT(m_pSpecEntry->GetLossMethod() != LOSSES_TIME_STEP);
   ATLASSERT( m_pSpecEntry->GetHaunchLoadComputationType()==pgsTypes::hlcAccountForCamber);
   return m_pSpecEntry->GetHaunchLoadCamberTolerance();
}

Float64 CProjectAgentImp::GetHaunchLoadCamberFactor() const
{
   ATLASSERT(m_pSpecEntry->GetLossMethod() != LOSSES_TIME_STEP);
   ATLASSERT( m_pSpecEntry->GetHaunchLoadComputationType()==pgsTypes::hlcAccountForCamber);
   return m_pSpecEntry->GetHaunchLoadCamberFactor();
}

bool CProjectAgentImp::IsAssExcessCamberInputEnabled(bool considerDeckType) const
{
   if (m_pSpecEntry->GetHaunchLoadComputationType() == pgsTypes::hlcAccountForCamber)
   {
      if (!considerDeckType || m_BridgeDescription.GetDeckDescription()->GetDeckType() != pgsTypes::sdtNone)
      {
         return true;
      }
      else
      {
         return false;
      }
   }
   else
   {
      return false;
   }
}

Uint16 CProjectAgentImp::GetMomentCapacityMethod() const
{
   return m_pSpecEntry->GetLRFDOverreinforcedMomentCapacity() == true ? LRFD_METHOD : WSDOT_METHOD;
}

void CProjectAgentImp::SetAnalysisType(pgsTypes::AnalysisType analysisType)
{
   if ( m_AnalysisType != analysisType )
   {
      m_AnalysisType = analysisType;
      Fire_AnalysisTypeChanged();
   }
}

pgsTypes::AnalysisType CProjectAgentImp::GetAnalysisType() const
{
   return m_AnalysisType;
}

bool CProjectAgentImp::IsSlabOffsetDesignEnabled() const
{
   GET_IFACE(ILibrary,pLib);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(m_Spec.c_str());

   return pSpecEntry->IsSlabOffsetDesignEnabled();
}

std::vector<arDesignOptions> CProjectAgentImp::GetDesignOptions(const CGirderKey& girderKey) const
{
   const CSplicedGirderData* pGirder = GetGirder(girderKey);
   const GirderLibraryEntry* pGirderEntry = pGirder->GetGirderLibraryEntry();

   std::vector<arDesignOptions> options;

   GET_IFACE(ILibrary,pLib);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(m_Spec.c_str());

   // For each girder we can have multiple design strategies
   IndexType nStrategies = pGirderEntry->GetNumPrestressDesignStrategies();
   for(IndexType strategyIdx = 0; strategyIdx < nStrategies; strategyIdx++)
   {
      arFlexuralDesignType design_type;
      Float64 fc_max, fci_max;
      pGirderEntry->GetPrestressDesignStrategy(strategyIdx, &design_type, &fci_max, &fc_max);

      arDesignOptions option;

      option.doDesignForFlexure = design_type;
      option.maxFci = fci_max;
      option.maxFc  = fc_max;

      option.doDesignSlabOffset = pSpecEntry->IsSlabOffsetDesignEnabled() ? sodAandAssExcessCamber : sodNoADesign; // option same as in 2.9x versions
      option.doDesignHauling = pSpecEntry->IsHaulingDesignEnabled();
      option.doDesignLifting = pSpecEntry->IsLiftingDesignEnabled();

      if (option.doDesignForFlexure == dtDesignForHarping)
      {
         bool check, design;
         Float64 d1, d2, d3;
         pSpecEntry->GetHoldDownForce(&check, &design, &d1);
         option.doDesignHoldDown = design;

         pSpecEntry->GetMaxStrandSlope(&check, &design, &d1, &d2, &d3);
         option.doDesignSlope =  design;
      }
      else
      {
         option.doDesignHoldDown = false;
         option.doDesignSlope    = false;
      }

      option.doForceHarpedStrandsStraight = option.doDesignForFlexure  != dtDesignForHarping;

      if (!option.doForceHarpedStrandsStraight)
      {
         // Harping uses spec order (until design algorithm is changed to work for other option)
         option.doStrandFillType = pSpecEntry->GetDesignStrandFillType();
      }
      else if (option.doDesignForFlexure == dtDesignFullyBondedRaised ||
               option.doDesignForFlexure == dtDesignForDebondingRaised)
      {
         // Raised strand designs use direct fill
         option.doStrandFillType = ftDirectFill;
      }
      else
      {
         // For other straight designs we always fill using grid order 
         option.doStrandFillType = ftGridOrder;
      }

      options.push_back(option);
   }

   ATLASSERT(!options.empty());

   return options;
}
////////////////////////////////////////////////////////////////////////
// ILibraryNames Methods
//
void CProjectAgentImp::EnumGdrConnectionNames( std::vector<std::_tstring>* pNames ) const
{
   const ConnectionLibrary& prj_lib = m_pLibMgr->GetConnectionLibrary();
   psglibCreateLibNameEnum( pNames, prj_lib);
}

void CProjectAgentImp::EnumGirderNames( std::vector<std::_tstring>* pNames ) const
{
   const GirderLibrary& prj_lib = m_pLibMgr->GetGirderLibrary();
   psglibCreateLibNameEnum( pNames, prj_lib);
}

void CProjectAgentImp::EnumGirderNames( LPCTSTR strGirderFamily, std::vector<std::_tstring>* pNames ) const
{
   const GirderLibrary& prj_lib = m_pLibMgr->GetGirderLibrary();
   pNames->clear();

   libKeyListType keys;
   prj_lib.KeyList(keys);

   libKeyListType::iterator iter(keys.begin());
   libKeyListType::iterator iterEnd(keys.end());
   for ( ; iter != iterEnd; iter++ )
   {
      const libLibraryEntry* pEntry = prj_lib.GetEntry( (*iter).c_str() );
      const GirderLibraryEntry* pGirderEntry = (GirderLibraryEntry*)pEntry;

      std::_tstring strFamilyName = pGirderEntry->GetGirderFamilyName();

      if ( strFamilyName == strGirderFamily )
      {
         pNames->push_back( pGirderEntry->GetName() );
      }
   }
}

void CProjectAgentImp::EnumConcreteNames( std::vector<std::_tstring>* pNames ) const
{
   const ConcreteLibrary& prj_lib = m_pLibMgr->GetConcreteLibrary();
   psglibCreateLibNameEnum( pNames, prj_lib);
}

void CProjectAgentImp::EnumDiaphragmNames( std::vector<std::_tstring>* pNames ) const
{
   const DiaphragmLayoutLibrary& prj_lib = m_pLibMgr->GetDiaphragmLayoutLibrary();
   psglibCreateLibNameEnum( pNames, prj_lib);
}

void CProjectAgentImp::EnumTrafficBarrierNames( std::vector<std::_tstring>* pNames ) const
{
   const TrafficBarrierLibrary& prj_lib = m_pLibMgr->GetTrafficBarrierLibrary();
   psglibCreateLibNameEnum( pNames, prj_lib);
}

void CProjectAgentImp::EnumSpecNames( std::vector<std::_tstring>* pNames) const
{
   const SpecLibrary& prj_lib = *(m_pLibMgr->GetSpecLibrary());
   psglibCreateLibNameEnum( pNames, prj_lib);
}

void CProjectAgentImp::EnumRatingCriteriaNames( std::vector<std::_tstring>* pNames) const
{
   const RatingLibrary& prj_lib = *(m_pLibMgr->GetRatingLibrary());
   psglibCreateLibNameEnum( pNames, prj_lib);
}

void CProjectAgentImp::EnumLiveLoadNames( std::vector<std::_tstring>* pNames) const
{
   const LiveLoadLibrary& prj_lib = *(m_pLibMgr->GetLiveLoadLibrary());
   psglibCreateLibNameEnum( pNames, prj_lib);

   std::vector<std::_tstring>::const_iterator iter(m_ReservedLiveLoads.begin());
   for ( ; iter != m_ReservedLiveLoads.end(); iter++ )
   {
      pNames->insert(pNames->begin(),*iter);
   }
}

void CProjectAgentImp::EnumDuctNames( std::vector<std::_tstring>* pNames ) const
{
   const DuctLibrary& prj_lib = *(m_pLibMgr->GetDuctLibrary());
   psglibCreateLibNameEnum( pNames, prj_lib);
}

void CProjectAgentImp::EnumHaulTruckNames( std::vector<std::_tstring>* pNames) const
{
   const HaulTruckLibrary& prj_lib = *(m_pLibMgr->GetHaulTruckLibrary());
   psglibCreateLibNameEnum( pNames, prj_lib);
}

void CProjectAgentImp::EnumGirderFamilyNames( std::vector<std::_tstring>* pNames ) const
{
   USES_CONVERSION;
   if ( m_GirderFamilyNames.size() == 0 )
   {
      std::vector<CString> names = CBeamFamilyManager::GetBeamFamilyNames();
      std::vector<CString>::iterator iter(names.begin());
      std::vector<CString>::iterator iterEnd(names.end());
      for ( ; iter != iterEnd; iter++ )
      {
         m_GirderFamilyNames.push_back( std::_tstring(*iter) );
      }
   }

   pNames->clear();
   pNames->insert(pNames->begin(),m_GirderFamilyNames.begin(),m_GirderFamilyNames.end());
}

void CProjectAgentImp::GetBeamFactory(const std::_tstring& strBeamFamily,const std::_tstring& strBeamName,IBeamFactory** ppFactory)
{
   std::vector<std::_tstring> strBeamNames;
   EnumGirderNames(strBeamFamily.c_str(),&strBeamNames);

   ATLASSERT( strBeamNames.size() != 0 );

   std::vector<std::_tstring>::iterator found = std::find(strBeamNames.begin(),strBeamNames.end(),strBeamName);
   if ( found == strBeamNames.end() )
   {
      ATLASSERT(false); // beam not found
      return;
   }

   const GirderLibraryEntry* pGdrEntry = GetGirderEntry( (*found).c_str() );

   pGdrEntry->GetBeamFactory(ppFactory);
}

////////////////////////////////////////////////////////////////////////
// ILibrary methods
//
void CProjectAgentImp::SetLibraryManager(psgLibraryManager* pNewLibMgr)
{
   ATLASSERT(pNewLibMgr!=0);
   m_pLibMgr = pNewLibMgr;

   // The spec libraries must always be set so use the first one listed as
   // the default
   SpecLibrary* pSpecLib = GetSpecLibrary();
   if ( pSpecLib )
   {
      libKeyListType keys;
      pSpecLib->KeyList(keys);

      if ( 0 < keys.size() )
      {
         InitSpecification(keys.front());
         SpecificationChanged(false);
      }
   }

   RatingLibrary* pRatingLib = GetRatingLibrary();
   if ( pRatingLib )
   {
      libKeyListType keys;
      pRatingLib->KeyList(keys);

      if ( 0 < keys.size() )
      {
         InitRatingSpecification(keys.front());
         RatingSpecificationChanged(false);
      }
   }
}

psgLibraryManager* CProjectAgentImp::GetLibraryManager()
{
   return m_pLibMgr;
}

const psgLibraryManager* CProjectAgentImp::GetLibraryManager() const
{
   return m_pLibMgr;
}

const ConnectionLibraryEntry* CProjectAgentImp::GetConnectionEntry(LPCTSTR lpszName ) const
{
   const ConnectionLibraryEntry* pEntry;
   const ConnectionLibrary& prj_lib = m_pLibMgr->GetConnectionLibrary();
   pEntry = prj_lib.LookupEntry( lpszName );

   if (pEntry!=0)
   {
      pEntry->Release();
   }

   return pEntry;
}

const GirderLibraryEntry* CProjectAgentImp::GetGirderEntry( LPCTSTR lpszName ) const
{
   const GirderLibraryEntry* pEntry;
   const GirderLibrary& prj_lib = m_pLibMgr->GetGirderLibrary();
   pEntry = prj_lib.LookupEntry( lpszName );

   if (pEntry!=0)
   {
      pEntry->Release();
   }

   return pEntry;
}

const ConcreteLibraryEntry* CProjectAgentImp::GetConcreteEntry( LPCTSTR lpszName ) const
{
   const ConcreteLibraryEntry* pEntry;
   const ConcreteLibrary& prj_lib = m_pLibMgr->GetConcreteLibrary();
   pEntry = prj_lib.LookupEntry( lpszName );

   if (pEntry!=0)
   {
      pEntry->Release();
   }

   return pEntry;
}

const DiaphragmLayoutEntry* CProjectAgentImp::GetDiaphragmEntry( LPCTSTR lpszName ) const
{
   const DiaphragmLayoutEntry* pEntry;
   const DiaphragmLayoutLibrary& prj_lib = m_pLibMgr->GetDiaphragmLayoutLibrary();
   pEntry = prj_lib.LookupEntry( lpszName );

   if (pEntry!=0)
   {
      pEntry->Release();
   }

   return pEntry;
}

const TrafficBarrierEntry* CProjectAgentImp::GetTrafficBarrierEntry( LPCTSTR lpszName ) const
{
   const TrafficBarrierEntry* pEntry;
   const TrafficBarrierLibrary& prj_lib = m_pLibMgr->GetTrafficBarrierLibrary();
   pEntry = prj_lib.LookupEntry( lpszName );

   if (pEntry!=0)
   {
      pEntry->Release();
   }

   return pEntry;
}

const SpecLibraryEntry* CProjectAgentImp::GetSpecEntry( LPCTSTR lpszName ) const
{
   const SpecLibraryEntry* pEntry;
   const SpecLibrary& prj_lib = *(m_pLibMgr->GetSpecLibrary());
   pEntry = prj_lib.LookupEntry( lpszName );

   if (pEntry!=0)
   {
      pEntry->Release();
   }

   return pEntry;
}

LiveLoadLibrary*        CProjectAgentImp::GetLiveLoadLibrary()
{
   return m_pLibMgr->GetLiveLoadLibrary();
}

ConcreteLibrary&        CProjectAgentImp::GetConcreteLibrary()
{
   return m_pLibMgr->GetConcreteLibrary();
}

ConnectionLibrary&      CProjectAgentImp::GetConnectionLibrary()
{
   return m_pLibMgr->GetConnectionLibrary();
}

GirderLibrary&          CProjectAgentImp::GetGirderLibrary()
{
   return m_pLibMgr->GetGirderLibrary();
}

DiaphragmLayoutLibrary& CProjectAgentImp::GetDiaphragmLayoutLibrary()
{
   return m_pLibMgr->GetDiaphragmLayoutLibrary();
}

TrafficBarrierLibrary&  CProjectAgentImp::GetTrafficBarrierLibrary()
{
   return m_pLibMgr->GetTrafficBarrierLibrary();
}

SpecLibrary* CProjectAgentImp::GetSpecLibrary()
{
   return m_pLibMgr->GetSpecLibrary();
}

DuctLibrary* CProjectAgentImp::GetDuctLibrary()
{
   return m_pLibMgr->GetDuctLibrary();
}

HaulTruckLibrary* CProjectAgentImp::GetHaulTruckLibrary()
{
   return m_pLibMgr->GetHaulTruckLibrary();
}

RatingLibrary* CProjectAgentImp::GetRatingLibrary()
{
   return m_pLibMgr->GetRatingLibrary();
}

const RatingLibrary* CProjectAgentImp::GetRatingLibrary() const
{
   return m_pLibMgr->GetRatingLibrary();
}

std::vector<libEntryUsageRecord> CProjectAgentImp::GetLibraryUsageRecords() const
{
   return m_pLibMgr->GetInUseLibraryEntries();
}

void CProjectAgentImp::GetMasterLibraryInfo(std::_tstring& strPublisher,std::_tstring& strMasterLib,sysTime& time) const
{
   m_pLibMgr->GetMasterLibraryInfo(strPublisher,strMasterLib);
   time = m_pLibMgr->GetTimeStamp();
}

const LiveLoadLibraryEntry* CProjectAgentImp::GetLiveLoadEntry( LPCTSTR lpszName ) const
{
   const LiveLoadLibraryEntry* pEntry;
   const LiveLoadLibrary* prj_lib = m_pLibMgr->GetLiveLoadLibrary();
   pEntry = prj_lib->LookupEntry( lpszName );

   if (pEntry!=0)
   {
      pEntry->Release();
   }

   return pEntry;
}

const RatingLibraryEntry* CProjectAgentImp::GetRatingEntry( LPCTSTR lpszName ) const
{
   const RatingLibraryEntry* pEntry;
   const RatingLibrary* prj_lib = m_pLibMgr->GetRatingLibrary();
   pEntry = prj_lib->LookupEntry( lpszName );

   if (pEntry!=0)
   {
      pEntry->Release();
   }

   return pEntry;
}

const DuctLibraryEntry* CProjectAgentImp::GetDuctEntry( LPCTSTR lpszName ) const
{
   const DuctLibraryEntry* pEntry;
   const DuctLibrary* prj_lib = m_pLibMgr->GetDuctLibrary();
   pEntry = prj_lib->LookupEntry( lpszName );

   if (pEntry!=0)
   {
      pEntry->Release();
   }

   return pEntry;
}

const HaulTruckLibraryEntry* CProjectAgentImp::GetHaulTruckEntry(LPCTSTR lpszName) const
{
   const HaulTruckLibraryEntry* pEntry;
   const HaulTruckLibrary* prj_lib = m_pLibMgr->GetHaulTruckLibrary();
   pEntry = prj_lib->LookupEntry( lpszName );

   if (pEntry!=0)
   {
      pEntry->Release();
   }

   return pEntry;
}

////////////////////////////////////////////////////////////////////////
// ILoadModifiers
//
void CProjectAgentImp::SetDuctilityFactor(ILoadModifiers::Level level,Float64 value)
{
   if ( level != m_DuctilityLevel || !IsEqual(m_DuctilityFactor,value) )
   {
      m_DuctilityLevel = level;
      m_DuctilityFactor = value;
      Fire_LoadModifiersChanged();
   }
}

void CProjectAgentImp::SetImportanceFactor(ILoadModifiers::Level level,Float64 value)
{
   if ( level != m_ImportanceLevel || !IsEqual(m_ImportanceFactor,value) )
   {
      m_ImportanceLevel = level;
      m_ImportanceFactor = value;
      Fire_LoadModifiersChanged();
   }
}

void CProjectAgentImp::SetRedundancyFactor(ILoadModifiers::Level level,Float64 value)
{
   if ( level != m_RedundancyLevel || !IsEqual(m_RedundancyFactor,value) )
   {
      m_RedundancyLevel = level;
      m_RedundancyFactor = value;
      Fire_LoadModifiersChanged();
   }
}

Float64 CProjectAgentImp::GetDuctilityFactor() const
{
   return m_DuctilityFactor;
}

Float64 CProjectAgentImp::GetImportanceFactor() const
{
   return m_ImportanceFactor;
}

Float64 CProjectAgentImp::GetRedundancyFactor() const
{
   return m_RedundancyFactor;
}

ILoadModifiers::Level CProjectAgentImp::GetDuctilityLevel() const
{
   return m_DuctilityLevel;
}

ILoadModifiers::Level CProjectAgentImp::GetImportanceLevel() const
{
   return m_ImportanceLevel;
}

ILoadModifiers::Level CProjectAgentImp::GetRedundancyLevel() const
{
   return m_RedundancyLevel;
}

////////////////////////////////////////////////////////////////////////
// IImportProjectLibrary
bool CProjectAgentImp::ImportProjectLibraries(IStructuredLoad* pStrLoad)
{
   return psglibImportEntries(pStrLoad,m_pLibMgr);
}

//////////////////////////////////////////////////////////////////
// Events?
void CProjectAgentImp::SpecificationChanged(bool bFireEvent)
{
   UpdateConcreteMaterial();
   UpdateStrandMaterial();
   VerifyRebarGrade();

   // analysis type must be continuous if the spec is using the time step loss method
   if ( m_pSpecEntry->GetLossMethod() == LOSSES_TIME_STEP && m_AnalysisType != pgsTypes::Continuous )
   {
      m_AnalysisType = pgsTypes::Continuous;
   }

   if ( m_pSpecEntry->GetLossMethod() != LOSSES_TIME_STEP )
   {
      // the timeline is based on settings in the project criteria for non-time-step loss methods (using the Creep and Camber timing parameters)
      CreatePrecastGirderBridgeTimelineEvents();
   }

   if ( bFireEvent )
   {
      Fire_SpecificationChanged();
   }
}

void CProjectAgentImp::RatingSpecificationChanged(bool bFireEvent)
{
   if ( bFireEvent )
   {
      Fire_RatingSpecificationChanged();
   }
}

/////////////////////////////////////////////////////////////////////////////
// IUserDefinedLoadData
bool CProjectAgentImp::HasUserDC(const CGirderKey& girderKey) const
{
   return HasUserLoad(girderKey,UserLoads::DC);
}

bool CProjectAgentImp::HasUserDW(const CGirderKey& girderKey) const
{
   return HasUserLoad(girderKey,UserLoads::DW);
}

bool CProjectAgentImp::HasUserLLIM(const CGirderKey& girderKey) const
{
   return HasUserLoad(girderKey,UserLoads::LL_IM);
}

CollectionIndexType CProjectAgentImp::GetPointLoadCount() const
{
   return m_LoadManager.GetPointLoadCount();
}

CollectionIndexType CProjectAgentImp::AddPointLoad(EventIDType eventID,const CPointLoadData& pld)
{
   CollectionIndexType idx = m_LoadManager.AddPointLoad(eventID,pld);
   FireContinuityRelatedSpanChange(pld.m_SpanKey,GCH_LOADING_ADDED);
   return idx;
}

const CPointLoadData* CProjectAgentImp::GetPointLoad(CollectionIndexType idx) const
{
   return m_LoadManager.GetPointLoad(idx);
}

const CPointLoadData* CProjectAgentImp::FindPointLoad(LoadIDType loadID) const
{
   return m_LoadManager.FindPointLoad(loadID);
}

EventIndexType CProjectAgentImp::GetPointLoadEventIndex(LoadIDType loadID) const
{
   return m_LoadManager.GetPointLoadEventIndex(loadID);
}

EventIDType CProjectAgentImp::GetPointLoadEventID(LoadIDType loadID) const
{
   return m_LoadManager.GetPointLoadEventID(loadID);
}

void CProjectAgentImp::UpdatePointLoad(CollectionIndexType idx, EventIDType eventID,const CPointLoadData& pld)
{
   bool bMovedGirders;
   CSpanKey prevKey;
   if ( m_LoadManager.UpdatePointLoad(idx,eventID,pld,&bMovedGirders,&prevKey) )
   {
      // must fire a delete event if load is moved to another girder
      if ( bMovedGirders )
      {
         FireContinuityRelatedSpanChange(prevKey,GCH_LOADING_REMOVED);
      }

      FireContinuityRelatedSpanChange(pld.m_SpanKey,GCH_LOADING_CHANGED);
   }
}

void CProjectAgentImp::DeletePointLoad(CollectionIndexType idx)
{
   CSpanKey key;
   m_LoadManager.DeletePointLoad(idx,&key);
   FireContinuityRelatedSpanChange(key,GCH_LOADING_REMOVED);
}

std::vector<CPointLoadData> CProjectAgentImp::GetPointLoads(const CSpanKey& spanKey) const
{
   return m_LoadManager.GetPointLoads(spanKey);
}

CollectionIndexType CProjectAgentImp::GetDistributedLoadCount() const
{
   return m_LoadManager.GetDistributedLoadCount();
}

CollectionIndexType CProjectAgentImp::AddDistributedLoad(EventIDType eventID,const CDistributedLoadData& pld)
{
   CollectionIndexType idx = m_LoadManager.AddDistributedLoad(eventID,pld);
   FireContinuityRelatedSpanChange(pld.m_SpanKey,GCH_LOADING_ADDED);
   return idx;
}

const CDistributedLoadData* CProjectAgentImp::GetDistributedLoad(CollectionIndexType idx) const
{
   return m_LoadManager.GetDistributedLoad(idx);
}

const CDistributedLoadData* CProjectAgentImp::FindDistributedLoad(LoadIDType loadID) const
{
   return m_LoadManager.FindDistributedLoad(loadID);
}

EventIndexType CProjectAgentImp::GetDistributedLoadEventIndex(LoadIDType loadID) const
{
   return m_LoadManager.GetDistributedLoadEventIndex(loadID);
}

EventIDType CProjectAgentImp::GetDistributedLoadEventID(LoadIDType loadID) const
{
   return m_LoadManager.GetDistributedLoadEventID(loadID);
}

void CProjectAgentImp::UpdateDistributedLoad(CollectionIndexType idx, EventIDType eventID,const CDistributedLoadData& pld)
{
   bool bMovedGirder;
   CSpanKey prevKey;
   if ( m_LoadManager.UpdateDistributedLoad(idx,eventID,pld,&bMovedGirder,&prevKey) )
   {
      if ( bMovedGirder )
      {
         FireContinuityRelatedSpanChange(prevKey,GCH_LOADING_REMOVED);
      }
      FireContinuityRelatedSpanChange(pld.m_SpanKey,GCH_LOADING_CHANGED);
   }
}

void CProjectAgentImp::DeleteDistributedLoad(CollectionIndexType idx)
{
   CSpanKey key;
   m_LoadManager.DeleteDistributedLoad(idx,&key);
   FireContinuityRelatedSpanChange(key,GCH_LOADING_REMOVED);
}

std::vector<CDistributedLoadData> CProjectAgentImp::GetDistributedLoads(const CSpanKey& spanKey) const
{
   return m_LoadManager.GetDistributedLoads(spanKey);
}

CollectionIndexType CProjectAgentImp::GetMomentLoadCount() const
{
   return m_LoadManager.GetMomentLoadCount();
}

CollectionIndexType CProjectAgentImp::AddMomentLoad(EventIDType eventID,const CMomentLoadData& pld)
{
   CollectionIndexType idx = m_LoadManager.AddMomentLoad(eventID,pld);
   FireContinuityRelatedSpanChange(pld.m_SpanKey,GCH_LOADING_ADDED);
   return idx;
}

const CMomentLoadData* CProjectAgentImp::GetMomentLoad(CollectionIndexType idx) const
{
   return m_LoadManager.GetMomentLoad(idx);
}

const CMomentLoadData* CProjectAgentImp::FindMomentLoad(LoadIDType loadID) const
{
   return m_LoadManager.FindMomentLoad(loadID);
}

EventIndexType CProjectAgentImp::GetMomentLoadEventIndex(LoadIDType loadID) const
{
   return m_LoadManager.GetMomentLoadEventIndex(loadID);
}

EventIDType CProjectAgentImp::GetMomentLoadEventID(LoadIDType loadID) const
{
   return m_LoadManager.GetMomentLoadEventID(loadID);
}

void CProjectAgentImp::UpdateMomentLoad(CollectionIndexType idx, EventIDType eventID,const CMomentLoadData& pld)
{
   bool bMovedGirder;
   CSpanKey prevKey;
   if ( m_LoadManager.UpdateMomentLoad(idx,eventID,pld,&bMovedGirder,&prevKey) )
   {
      if ( bMovedGirder )
      {
         FireContinuityRelatedSpanChange(prevKey,GCH_LOADING_REMOVED);
      }
      FireContinuityRelatedSpanChange(pld.m_SpanKey,GCH_LOADING_CHANGED);
   }
}

void CProjectAgentImp::DeleteMomentLoad(CollectionIndexType idx)
{
   CSpanKey key;
   m_LoadManager.DeleteMomentLoad(idx,&key);
   FireContinuityRelatedSpanChange(key,GCH_LOADING_REMOVED);
}

std::vector<CMomentLoadData> CProjectAgentImp::GetMomentLoads(const CSpanKey& spanKey) const
{
   return m_LoadManager.GetMomentLoads(spanKey);
}

void CProjectAgentImp::SetConstructionLoad(Float64 load)
{
   if ( !IsEqual(load,m_ConstructionLoad) )
   {
      m_ConstructionLoad = load;
      Fire_ConstructionLoadChanged();
   }
}

Float64 CProjectAgentImp::GetConstructionLoad() const
{
   return m_ConstructionLoad;
}

////////////////////////////////////////////////////////////////////////
// IEvents
void CProjectAgentImp::HoldEvents()
{
   ATLASSERT(0 <= m_EventHoldCount);
   m_EventHoldCount++;


   Fire_OnHoldEvents();

   GET_IFACE(IUIEvents,pUIEvents);
   pUIEvents->HoldEvents(true);
}

void CProjectAgentImp::FirePendingEvents()
{
   if ( m_EventHoldCount == 0 )
   {
      return;
   }

   try
   {
      if (m_EventHoldCount == 1)
      {
         m_EventHoldCount--;
         m_bFiringEvents = true;


         if (sysFlags<Uint32>::IsSet(m_PendingEvents, EVT_PROJECTPROPERTIES))
         {
            Fire_ProjectPropertiesChanged();
         }

         //if ( sysFlags<Uint32>::IsSet(m_PendingEvents,EVT_UNITS) )
         //   Fire_UnitsChanged(m_Units);

         if (sysFlags<Uint32>::IsSet(m_PendingEvents, EVT_ANALYSISTYPE))
         {
            Fire_AnalysisTypeChanged();
         }

         if (sysFlags<Uint32>::IsSet(m_PendingEvents, EVT_EXPOSURECONDITION))
         {
            Fire_ExposureConditionChanged();
         }

         if (sysFlags<Uint32>::IsSet(m_PendingEvents, EVT_RELHUMIDITY))
         {
            Fire_RelHumidityChanged();
         }

         if (sysFlags<Uint32>::IsSet(m_PendingEvents, EVT_GIRDERFAMILY))
         {
            Fire_BridgeChanged(nullptr);
         }

         if (sysFlags<Uint32>::IsSet(m_PendingEvents, EVT_BRIDGE))
         {
            // eliminate duplicates
            m_PendingBridgeChangedHints.erase(std::unique(m_PendingBridgeChangedHints.begin(), m_PendingBridgeChangedHints.end()), m_PendingBridgeChangedHints.end());
            std::vector<CBridgeChangedHint*>::iterator iter(m_PendingBridgeChangedHints.begin());
            std::vector<CBridgeChangedHint*>::iterator end(m_PendingBridgeChangedHints.end());
            for (; iter != end; iter++)
            {
               CBridgeChangedHint* pHint = *iter;
               Fire_BridgeChanged(pHint);
            }
            m_PendingBridgeChangedHints.clear();
         }

         //
         // pretty much all the event handers do the same thing when the bridge or girder family changes
         // no sence firing two events
         //   if ( sysFlags<Uint32>::IsSet(m_PendingEvents,EVT_GIRDERFAMILY) )
         //      Fire_GirderFamilyChanged();


         if (sysFlags<Uint32>::IsSet(m_PendingEvents, EVT_SPECIFICATION))
         {
            SpecificationChanged(true);
         }

         if (sysFlags<Uint32>::IsSet(m_PendingEvents, EVT_RATING_SPECIFICATION))
         {
            RatingSpecificationChanged(true);
         }

         if (sysFlags<Uint32>::IsSet(m_PendingEvents, EVT_LIBRARYCONFLICT))
         {
            Fire_OnLibraryConflictResolved();
         }

         if (sysFlags<Uint32>::IsSet(m_PendingEvents, EVT_LOADMODIFIER))
         {
            Fire_LoadModifiersChanged();
         }

         if (sysFlags<Uint32>::IsSet(m_PendingEvents, EVT_CONSTRUCTIONLOAD))
         {
            Fire_ConstructionLoadChanged();
         }

         if (sysFlags<Uint32>::IsSet(m_PendingEvents, EVT_LIVELOAD))
         {
            Fire_LiveLoadChanged();
         }

         if (sysFlags<Uint32>::IsSet(m_PendingEvents, EVT_LOSSPARAMETERS))
         {
            Fire_OnLossParametersChanged();
         }

         for ( const auto& item : m_PendingEventsHash )
         {
            const CGirderKey& girderKey(item.first);
            Uint32 lHint = item.second;

            Fire_GirderChanged(girderKey, lHint);
         }

         m_PendingEventsHash.clear();
         m_PendingEvents = 0;

         m_bFiringEvents = false;
      }
      else
      {
         m_EventHoldCount--;
      }

      Fire_OnFirePendingEvents();

      GET_IFACE(IUIEvents, pUIEvents);
      pUIEvents->FirePendingEvents();
   }
   catch (...)
   {
      m_PendingEventsHash.clear();
      m_PendingEvents = 0;

      m_bFiringEvents = false;
      throw;
   }
}

void CProjectAgentImp::CancelPendingEvents()
{
   m_EventHoldCount--;
   if ( m_EventHoldCount <= 0 && !m_bFiringEvents )
   {
      m_EventHoldCount = 0;
      m_PendingEventsHash.clear();
      m_PendingEvents = 0;
   }

   Fire_OnCancelPendingEvents();

   GET_IFACE(IUIEvents,pUIEvents);
   pUIEvents->CancelPendingEvents();
}

////////////////////////////////////////////////////////////////////////
// ILimits
Float64 CProjectAgentImp::GetMaxSlabFc(pgsTypes::ConcreteType concType) const
{
   return m_pSpecEntry->GetMaxSlabFc(concType);
}

Float64 CProjectAgentImp::GetMaxSegmentFci(pgsTypes::ConcreteType concType) const
{
   return m_pSpecEntry->GetMaxSegmentFci(concType);
}

Float64 CProjectAgentImp::GetMaxSegmentFc(pgsTypes::ConcreteType concType) const
{
   return m_pSpecEntry->GetMaxSegmentFc(concType);
}

Float64 CProjectAgentImp::GetMaxClosureFci(pgsTypes::ConcreteType concType) const
{
   return m_pSpecEntry->GetMaxClosureFci(concType);
}

Float64 CProjectAgentImp::GetMaxClosureFc(pgsTypes::ConcreteType concType) const
{
   return m_pSpecEntry->GetMaxClosureFc(concType);
}

Float64 CProjectAgentImp::GetMaxConcreteUnitWeight(pgsTypes::ConcreteType concType) const
{
   return m_pSpecEntry->GetMaxConcreteUnitWeight(concType);
}

Float64 CProjectAgentImp::GetMaxConcreteAggSize(pgsTypes::ConcreteType concType) const
{
   return m_pSpecEntry->GetMaxConcreteAggSize(concType);
}

/////////////////////////////////////////////////////////////////////////
// IEventMap
CComBSTR CProjectAgentImp::GetEventName(EventIndexType eventIdx) const
{
   // returns a unique stage name
   CString strName;
   const CTimelineEvent* pTimelineEvent = m_BridgeDescription.GetTimelineManager()->GetEventByIndex(eventIdx);
   strName.Format(_T("Event %d: %s"),LABEL_EVENT(eventIdx),pTimelineEvent->GetDescription());

   return CComBSTR(strName);
}

EventIndexType CProjectAgentImp::GetEventIndex(CComBSTR bstrEvent) const
{
   // decodes the event index out of the event name. event name must be created
   // with GetEventName above

   USES_CONVERSION;
   CString strName(OLE2T(bstrEvent));
   int pos1 = strName.Find(_T("Event "));
   int pos2 = strName.Find(_T(":"));
   EventIndexType eventIdx = (EventIndexType)_ttol(strName.Mid(pos1+6,pos2-pos1)); // 6 is the length of :Event "
   return eventIdx-1;
}

////////////////////////////////////////////////////////////////////////
// ILoadFactors
const CLoadFactors* CProjectAgentImp::GetLoadFactors() const
{
   return &m_LoadFactors;
}

void CProjectAgentImp::SetLoadFactors(const CLoadFactors& loadFactors)
{
   m_LoadFactors = loadFactors;
   Fire_LoadModifiersChanged(); // not exactly the right event, but will cause everything to be re combined
}

// ILiveLoads
bool CProjectAgentImp::IsLiveLoadDefined(pgsTypes::LiveLoadType llType) const
{
   if ( m_SelectedLiveLoads[llType].empty() )
   {
      return false;
   }

   return true;
}

ILiveLoads::PedestrianLoadApplicationType CProjectAgentImp::GetPedestrianLoadApplication(pgsTypes::LiveLoadType llType) const
{
   if (llType<pgsTypes::lltDesign || llType>pgsTypes::lltFatigue)
   {
      // Rating Live Loads
      return m_bIncludePedestrianLiveLoad ? ILiveLoads::PedConcurrentWithVehicular : ILiveLoads::PedDontApply;
   }
   else
   {
      return m_PedestrianLoadApplicationType[llType];
   }
}

void CProjectAgentImp::SetPedestrianLoadApplication(pgsTypes::LiveLoadType llType, PedestrianLoadApplicationType PedLoad)
{
   if (llType<pgsTypes::lltDesign || llType>pgsTypes::lltFatigue)
   {
      // This function is not applicable for Rating Live Loads
      ATLASSERT(false);
   }
   else
   {
      if (m_PedestrianLoadApplicationType[llType] != PedLoad)
      {
         m_PedestrianLoadApplicationType[llType] = PedLoad;
         Fire_LiveLoadChanged();
      }
   }
}

std::vector<std::_tstring> CProjectAgentImp::GetLiveLoadNames(pgsTypes::LiveLoadType llType) const
{
   std::vector<std::_tstring> strNames;
   strNames.reserve(m_SelectedLiveLoads[llType].size());

   LiveLoadSelectionIterator iter(m_SelectedLiveLoads[llType].begin());
   LiveLoadSelectionIterator iterEnd(m_SelectedLiveLoads[llType].end());
   for ( ; iter != iterEnd; iter++ )
   {
      const LiveLoadSelection& lls = *iter;

      strNames.push_back(lls.EntryName);
   }

   return strNames;
}

void CProjectAgentImp::SetLiveLoadNames(pgsTypes::LiveLoadType llType,const std::vector<std::_tstring>& names)
{
   bool change = false;

   // first see of any data has changed
   if (names.size() == m_SelectedLiveLoads[llType].size())
   {
      for (const auto& selection : m_SelectedLiveLoads[llType])
      {
         std::vector<std::_tstring>::const_iterator its = std::find(names.begin(), names.end(), selection.EntryName);
         if (its == names.end())
         {
            change = true;
            break;
         }
      }
   }
   else
   {
      change = true;
   }

   if (change)
   {
      LiveLoadLibrary* pLiveLoadLibrary = m_pLibMgr->GetLiveLoadLibrary();

      // one of the selected entries have changed - first release all entries
      for (const auto& selection : m_SelectedLiveLoads[llType])
      {
         if ( selection.pEntry != nullptr )
         {
            release_library_entry( &m_LibObserver, 
                                   selection.pEntry,
                                   pLiveLoadLibrary);
         }
      }

      m_SelectedLiveLoads[llType].clear();

      // add new references
      std::vector<std::_tstring>::const_iterator its(names.begin());
      std::vector<std::_tstring>::const_iterator itsEnd(names.end());
      for ( ; its != itsEnd; its++)
      {
         const std::_tstring& strLLName = *its;

         LiveLoadSelection selection;
         selection.EntryName = strLLName;

         if ( !IsReservedLiveLoad(strLLName) )
         {
            use_library_entry( &m_LibObserver,
                               strLLName, 
                               &selection.pEntry, 
                               *pLiveLoadLibrary);
         }

         m_SelectedLiveLoads[llType].insert(selection);
      }

      Fire_LiveLoadChanged();
   }
}

Float64 CProjectAgentImp::GetTruckImpact(pgsTypes::LiveLoadType llType) const
{
   return m_TruckImpact[llType];
}

void CProjectAgentImp::SetTruckImpact(pgsTypes::LiveLoadType llType,Float64 impact)
{
   if ( !IsEqual(m_TruckImpact[llType],impact) )
   {
      m_TruckImpact[llType] = impact;
      Fire_LiveLoadChanged();
   }
}

Float64 CProjectAgentImp::GetLaneImpact(pgsTypes::LiveLoadType llType) const
{
   return m_LaneImpact[llType];
}

void CProjectAgentImp::SetLaneImpact(pgsTypes::LiveLoadType llType,Float64 impact)
{
   if ( !IsEqual(m_LaneImpact[llType],impact) )
   {
      m_LaneImpact[llType] = impact;
      Fire_LiveLoadChanged();
   }
}

bool CProjectAgentImp::IsReservedLiveLoad(const std::_tstring& strName) const
{
   std::vector<std::_tstring>::const_iterator iter(m_ReservedLiveLoads.begin());
   std::vector<std::_tstring>::const_iterator iter_end(m_ReservedLiveLoads.end());
   for ( ; iter != iter_end; iter++ )
   {
      const std::_tstring& rname = *iter;
      if ( rname == strName )
      {
         return true;
      }
   }

   return false;
}

void CProjectAgentImp::SetLldfRangeOfApplicabilityAction(LldfRangeOfApplicabilityAction action)
{
   if ( m_LldfRangeOfApplicabilityAction != action )
   {
      m_LldfRangeOfApplicabilityAction = action;
      Fire_LiveLoadChanged();
   }
}

LldfRangeOfApplicabilityAction CProjectAgentImp::GetLldfRangeOfApplicabilityAction() const
{
   return m_LldfRangeOfApplicabilityAction;
}

bool CProjectAgentImp::IgnoreLLDFRangeOfApplicability() const
{
   if (m_BridgeDescription.GetDistributionFactorMethod() == pgsTypes::Calculated)
   {
      return m_LldfRangeOfApplicabilityAction != roaEnforce;
   }
   else if (m_BridgeDescription.GetDistributionFactorMethod() == pgsTypes::LeverRule)
   {
      return true;
   }
   else
   {
      return false;
   }
}

std::_tstring CProjectAgentImp::GetLLDFSpecialActionText() const
{
   if (m_BridgeDescription.GetDistributionFactorMethod() == pgsTypes::DirectlyInput)
   {
      return std::_tstring(_T(" Note: The project criteria was overriden: Live load distribution factors input directly by user."));
   }
   if (m_BridgeDescription.GetDistributionFactorMethod() == pgsTypes::LeverRule)
   {
      return std::_tstring(_T("  Note: The project criteria was overriden: All live load distribution factors computed using the Lever Rule."));
   }
   else if (m_LldfRangeOfApplicabilityAction==roaIgnore)
   {
      return std::_tstring(_T(" Note that range of applicability requirements are ignored. The equations in LRFD 4.6.2.2 are used regardless of their validity."));
   }
   else if (m_LldfRangeOfApplicabilityAction==roaIgnoreUseLeverRule)
   {
      return std::_tstring(_T(" Note that the lever rule used to compute distribution factors if the range of applicability requirements are exceeded."));
   }
   else
   {
      return std::_tstring(); //  nothing special
   }
}

////////////////////////////////////////////////////////////////////////
// IEffectiveFlangeWidth
bool CProjectAgentImp::IgnoreEffectiveFlangeWidthLimits() const
{
   return m_bIgnoreEffectiveFlangeWidthLimits;
}

void CProjectAgentImp::IgnoreEffectiveFlangeWidthLimits(bool bIgnore)
{
   if ( bIgnore != m_bIgnoreEffectiveFlangeWidthLimits )
   {
      m_bIgnoreEffectiveFlangeWidthLimits = bIgnore;
      Fire_BridgeChanged();
   }
}

////////////////////////////////////////////////////////////////////////
// ILossParameters
pgsTypes::LossMethod CProjectAgentImp::GetLossMethod() const
{
   pgsTypes::LossMethod loss_method;
   if ( m_bGeneralLumpSum )
   {
      loss_method = pgsTypes::GENERAL_LUMPSUM;
   }
   else
   {
      const SpecLibraryEntry* pSpecEntry = GetSpecEntry(m_Spec.c_str());
      loss_method = (pgsTypes::LossMethod)pSpecEntry->GetLossMethod();
   }

   return loss_method;
}

pgsTypes::TimeDependentModel CProjectAgentImp::GetTimeDependentModel() const
{
   const SpecLibraryEntry* pSpecEntry = GetSpecEntry(m_Spec.c_str());
   return (pgsTypes::TimeDependentModel)pSpecEntry->GetTimeDependentModel();
}

void CProjectAgentImp::IgnoreCreepEffects(bool bIgnore)
{
   if ( m_bIgnoreCreepEffects != bIgnore )
   {
      m_bIgnoreCreepEffects = bIgnore;
      Fire_OnLossParametersChanged();
   }
}

bool CProjectAgentImp::IgnoreCreepEffects() const
{
   return m_bIgnoreCreepEffects;
}

void CProjectAgentImp::IgnoreShrinkageEffects(bool bIgnore)
{
   if ( m_bIgnoreShrinkageEffects != bIgnore )
   {
      m_bIgnoreShrinkageEffects = bIgnore;
      Fire_OnLossParametersChanged();
   }
}

bool CProjectAgentImp::IgnoreShrinkageEffects() const
{
   return m_bIgnoreShrinkageEffects;
}

void CProjectAgentImp::IgnoreRelaxationEffects(bool bIgnore)
{
   if ( m_bIgnoreRelaxationEffects != bIgnore )
   {
      m_bIgnoreRelaxationEffects = bIgnore;
      Fire_OnLossParametersChanged();
   }
}

bool CProjectAgentImp::IgnoreRelaxationEffects() const
{
   return m_bIgnoreRelaxationEffects;
}

void CProjectAgentImp::IgnoreTimeDependentEffects(bool bIgnoreCreep,bool bIgnoreShrinkage,bool bIgnoreRelaxation)
{
   if ( m_bIgnoreCreepEffects != bIgnoreCreep || m_bIgnoreShrinkageEffects != bIgnoreShrinkage || m_bIgnoreRelaxationEffects != bIgnoreRelaxation )
   {
      m_bIgnoreCreepEffects      = bIgnoreCreep;
      m_bIgnoreShrinkageEffects  = bIgnoreShrinkage;
      m_bIgnoreRelaxationEffects = bIgnoreRelaxation;
      Fire_OnLossParametersChanged();
   }
}

void CProjectAgentImp::SetTendonPostTensionParameters(Float64 Dset,Float64 wobble,Float64 friction)
{
   if ( !IsEqual(m_Dset_PT,Dset) || !IsEqual(m_WobbleFriction_PT,wobble) || !IsEqual(m_FrictionCoefficient_PT,friction) )
   {
      m_Dset_PT                = Dset;
      m_WobbleFriction_PT      = wobble;
      m_FrictionCoefficient_PT = friction;
      Fire_OnLossParametersChanged();
   }
}

void CProjectAgentImp::GetTendonPostTensionParameters(Float64* Dset,Float64* wobble,Float64* friction) const
{
   *Dset     = m_Dset_PT;
   *wobble   = m_WobbleFriction_PT;
   *friction = m_FrictionCoefficient_PT;
}

void CProjectAgentImp::SetTemporaryStrandPostTensionParameters(Float64 Dset,Float64 wobble,Float64 friction)
{
   if ( !IsEqual(m_Dset_TTS,Dset) || !IsEqual(m_WobbleFriction_TTS,wobble) || !IsEqual(m_FrictionCoefficient_TTS,friction) )
   {
      m_Dset_TTS                = Dset;
      m_WobbleFriction_TTS      = wobble;
      m_FrictionCoefficient_TTS = friction;
      Fire_OnLossParametersChanged();
   }
}

void CProjectAgentImp::GetTemporaryStrandPostTensionParameters(Float64* Dset,Float64* wobble,Float64* friction) const
{
   *Dset     = m_Dset_TTS;
   *wobble   = m_WobbleFriction_TTS;
   *friction = m_FrictionCoefficient_TTS;
}

void CProjectAgentImp::UseGeneralLumpSumLosses(bool bLumpSum)
{
   if ( bLumpSum != m_bGeneralLumpSum )
   {
      m_bGeneralLumpSum = bLumpSum;
      Fire_OnLossParametersChanged();
   }
}

bool CProjectAgentImp::UseGeneralLumpSumLosses() const
{
   return m_bGeneralLumpSum;
}

Float64 CProjectAgentImp::GetBeforeXferLosses() const
{
   return m_BeforeXferLosses;
}

void CProjectAgentImp::SetBeforeXferLosses(Float64 loss)
{
   if ( !IsEqual(m_BeforeXferLosses,loss) )
   {
      m_BeforeXferLosses = loss;
      Fire_OnLossParametersChanged();
   }
}

Float64 CProjectAgentImp::GetAfterXferLosses() const
{
   return m_AfterXferLosses;
}

void CProjectAgentImp::SetAfterXferLosses(Float64 loss)
{
   if ( !IsEqual(m_AfterXferLosses,loss) )
   {
      m_AfterXferLosses = loss;
      Fire_OnLossParametersChanged();
   }
}

Float64 CProjectAgentImp::GetLiftingLosses() const
{
   return m_LiftingLosses;
}

void CProjectAgentImp::SetLiftingLosses(Float64 loss)
{
   if ( !IsEqual(m_LiftingLosses,loss) )
   {
      m_LiftingLosses = loss;
      Fire_OnLossParametersChanged();
   }
}

Float64 CProjectAgentImp::GetShippingLosses() const
{
   return m_ShippingLosses;
}

void CProjectAgentImp::SetShippingLosses(Float64 loss)
{
   if ( !IsEqual(m_ShippingLosses,loss) )
   {
      m_ShippingLosses = loss;
      Fire_OnLossParametersChanged();
   }
}

Float64 CProjectAgentImp::GetBeforeTempStrandRemovalLosses() const
{
   return m_BeforeTempStrandRemovalLosses;
}

void CProjectAgentImp::SetBeforeTempStrandRemovalLosses(Float64 loss)
{
   if ( !IsEqual(m_BeforeTempStrandRemovalLosses,loss) )
   {
      m_BeforeTempStrandRemovalLosses = loss;
      Fire_OnLossParametersChanged();
   }
}

Float64 CProjectAgentImp::GetAfterTempStrandRemovalLosses() const
{
   return m_AfterTempStrandRemovalLosses;
}

void CProjectAgentImp::SetAfterTempStrandRemovalLosses(Float64 loss)
{
   if ( !IsEqual(m_AfterTempStrandRemovalLosses,loss) )
   {
      m_AfterTempStrandRemovalLosses = loss;
      Fire_OnLossParametersChanged();
   }
}

Float64 CProjectAgentImp::GetAfterDeckPlacementLosses() const
{
   return m_AfterDeckPlacementLosses;
}

void CProjectAgentImp::SetAfterDeckPlacementLosses(Float64 loss)
{
   if ( !IsEqual(m_AfterDeckPlacementLosses,loss) )
   {
      m_AfterDeckPlacementLosses = loss;
      Fire_OnLossParametersChanged();
   }
}

Float64 CProjectAgentImp::GetAfterSIDLLosses() const
{
   return m_AfterSIDLLosses;
}

void CProjectAgentImp::SetAfterSIDLLosses(Float64 loss)
{
   if ( !IsEqual(m_AfterSIDLLosses,loss) )
   {
      m_AfterSIDLLosses = loss;
      Fire_OnLossParametersChanged();
   }
}

Float64 CProjectAgentImp::GetFinalLosses() const
{
   return m_FinalLosses;
}

void CProjectAgentImp::SetFinalLosses(Float64 loss)
{
   if ( !IsEqual(m_FinalLosses,loss) )
   {
      m_FinalLosses = loss;
      Fire_OnLossParametersChanged();
   }
}

////////////////////////////////////////////////////////////////////////
// IValidate
UINT CProjectAgentImp::Orientation(LPCTSTR lpszOrientation)
{
   CString strOrientation(lpszOrientation);
   strOrientation.MakeUpper(); // do comparisons in upper case

   if (strOrientation.IsEmpty())
   {
      VALIDATE_INVALID;
   }

   if ( strOrientation == _T("NORMAL") || (strOrientation.GetLength() == 1 && strOrientation[0] == 'N') )
   {
      return VALIDATE_SUCCESS;
   }

   HRESULT hr_angle = m_objAngle->FromString(CComBSTR(strOrientation));
   if ( SUCCEEDED(hr_angle) )
   {
      Float64 value;
      m_objAngle->get_Value(&value);
      if ( value < -MAX_SKEW_ANGLE || MAX_SKEW_ANGLE < value )
      {
         return VALIDATE_SKEW_ANGLE;
      }
      else
      {
         return VALIDATE_SUCCESS;
      }
   }

   HRESULT hr_direction = m_objDirection->FromString(CComBSTR( strOrientation ));
   if ( SUCCEEDED(hr_direction) )
   {
      return VALIDATE_SUCCESS;
   }

   return VALIDATE_INVALID;
}


////////////////////////////////////////////////////////////////////////
// Helper Methods
//
bool CProjectAgentImp::ResolveLibraryConflicts(const ConflictList& rList)
{
   // NOTE: that the name of this function is misleading. It not only deals with conflicts,
   //       it also creates the library entries.
   std::_tstring new_name;

   //
   // Girders
   //
   
   // if girder name conflicts, update the name
   const GirderLibrary&  girderLibrary = m_pLibMgr->GetGirderLibrary();

   // only update at bridge level if gider is set for entire bridge
   if (m_BridgeDescription.UseSameGirderForEntireBridge() &&
       rList.IsConflict(girderLibrary, m_BridgeDescription.GetGirderName(), &new_name))
   {
      m_BridgeDescription.RenameGirder(new_name.c_str());
   }

   GroupIndexType nGroups = m_BridgeDescription.GetGirderGroupCount();
   for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
   {
      CGirderGroupData* pGroup = m_BridgeDescription.GetGirderGroup(grpIdx);
      GroupIndexType nGirderTypeGroups = pGroup->GetGirderTypeGroupCount();
      for ( GroupIndexType girderTypeGroupIdx = 0; girderTypeGroupIdx < nGirderTypeGroups; girderTypeGroupIdx++ )
      {
         GirderIndexType firstGdrIdx,lastGdrIdx;
         std::_tstring strGirderName;

         pGroup->GetGirderTypeGroup(girderTypeGroupIdx,&firstGdrIdx,&lastGdrIdx,&strGirderName);
         if (rList.IsConflict(girderLibrary, strGirderName, &new_name))
         {
            pGroup->RenameGirder(girderTypeGroupIdx,new_name.c_str());
         }
      }
   }

   // now use all the girder library entries that are in this model
   UseGirderLibraryEntries();

   // left traffic barrier
   const TrafficBarrierLibrary& rbarlib = m_pLibMgr->GetTrafficBarrierLibrary();
   CRailingSystem* pRailingSystem = m_BridgeDescription.GetLeftRailingSystem();

   if (rList.IsConflict(rbarlib, pRailingSystem->strExteriorRailing, &new_name))
   {
      pRailingSystem->strExteriorRailing = new_name;
   }

   const TrafficBarrierEntry* pEntry = nullptr;

   use_library_entry( &m_LibObserver,
                      pRailingSystem->strExteriorRailing,
                      &pEntry,
                      m_pLibMgr->GetTrafficBarrierLibrary());

   pRailingSystem->SetExteriorRailing(pEntry);

   if ( pRailingSystem->bUseInteriorRailing )
   {
      if (rList.IsConflict(rbarlib, pRailingSystem->strInteriorRailing, &new_name))
      {
         pRailingSystem->strInteriorRailing = new_name;
      }

      use_library_entry( &m_LibObserver,
                         pRailingSystem->strInteriorRailing,
                         &pEntry,
                        m_pLibMgr->GetTrafficBarrierLibrary());

      pRailingSystem->SetInteriorRailing(pEntry);
   }

   // right traffic barrier
   pRailingSystem = m_BridgeDescription.GetRightRailingSystem();
   if (rList.IsConflict(rbarlib, pRailingSystem->strExteriorRailing, &new_name))
   {
      pRailingSystem->strExteriorRailing = new_name;
   }

   use_library_entry( &m_LibObserver,
                      pRailingSystem->strExteriorRailing,
                      &pEntry,
                      m_pLibMgr->GetTrafficBarrierLibrary());

   pRailingSystem->SetExteriorRailing(pEntry);

   if ( pRailingSystem->bUseInteriorRailing )
   {
      if (rList.IsConflict(rbarlib, pRailingSystem->strInteriorRailing, &new_name))
      {
         pRailingSystem->strInteriorRailing = new_name;
      }

      use_library_entry( &m_LibObserver,
                         pRailingSystem->strInteriorRailing,
                         &pEntry,
                         m_pLibMgr->GetTrafficBarrierLibrary());

      pRailingSystem->SetInteriorRailing(pEntry);
   }

   // Spec Library
   const SpecLibrary& rspeclib = *(m_pLibMgr->GetSpecLibrary());
   if (rList.IsConflict(rspeclib, m_Spec, &new_name))
   {
      m_Spec = new_name;
   }


   // a little game here to set up the spec library entry
   // when the library manager is set, PGSuper latches onto the first
   // spec lib entry. We need to release that entry and hook onto the
   // the real entry. Save the current name, clear the m_Spec data entry
   // so it looks like the spec is being changed, then set the specification
   std::_tstring specName = m_Spec;
   m_Spec = _T("");
   InitSpecification(specName);

   // now that the spec library entry has been set, see if there are any obsolete
   // values in the library that need to be brought into the project agent

   if ( m_pSpecEntry->UpdateLoadFactors() )
   {
      // recover parameters that are no longer part of the spec library entry
      for ( int i = 0; i < 6; i++ )
      {
         pgsTypes::LimitState ls = (pgsTypes::LimitState)i;
         Float64 min, max;
         m_pSpecEntry->GetDCLoadFactors(ls, &min, &max);
         m_LoadFactors.SetDC(ls,min, max);

         m_pSpecEntry->GetDWLoadFactors(ls, &min, &max);
         m_LoadFactors.SetDW(ls, min, max);

         m_pSpecEntry->GetLLIMLoadFactors(ls, &min, &max);
         m_LoadFactors.SetLLIM(ls, min, max);
      }
   }

   if ( m_pSpecEntry->GetLossMethod() == 3 ) // old general lump sum method
   {
      m_bGeneralLumpSum               = true;
      m_BeforeXferLosses              = m_pSpecEntry->GetBeforeXferLosses();
      m_AfterXferLosses               = m_pSpecEntry->GetAfterXferLosses();
      m_LiftingLosses                 = m_pSpecEntry->GetLiftingLosses();
      m_ShippingLosses                = m_pSpecEntry->GetShippingLosses();
      m_BeforeTempStrandRemovalLosses = m_pSpecEntry->GetBeforeTempStrandRemovalLosses();
      m_AfterTempStrandRemovalLosses  = m_pSpecEntry->GetAfterTempStrandRemovalLosses();
      m_AfterDeckPlacementLosses      = m_pSpecEntry->GetAfterDeckPlacementLosses();
      m_AfterSIDLLosses               = m_pSpecEntry->GetAfterSIDLLosses();
      m_FinalLosses                   = m_pSpecEntry->GetFinalLosses();
   }

   if ( m_pSpecEntry->UpdatePTParameters() )
   {
      m_Dset_PT                = m_pSpecEntry->GetAnchorSet();
      m_WobbleFriction_PT      = m_pSpecEntry->GetWobbleFrictionCoefficient();
      m_FrictionCoefficient_PT = m_pSpecEntry->GetFrictionCoefficient();

      m_Dset_TTS                = m_Dset_PT;
      m_WobbleFriction_TTS      = m_WobbleFriction_PT;
      m_FrictionCoefficient_TTS = m_FrictionCoefficient_PT;
   }


   // Rating Library
   const RatingLibrary& rratinglib = *(m_pLibMgr->GetRatingLibrary());
   if (rList.IsConflict(rratinglib, m_RatingSpec, &new_name))
   {
      m_RatingSpec = new_name;
   }


   // a little game here to set up the spec library entry
   // when the library manager is set, PGSuper latches onto the first
   // spec lib entry. We need to release that entry and hook onto the
   // the real entry. Save the current name, clear the m_Spec data entry
   // so it looks like the spec is being changed, then set the specification
   std::_tstring ratingSpecName = m_RatingSpec;
   m_RatingSpec = _T("");
   InitRatingSpecification(ratingSpecName);

   if (rList.AreThereAnyConflicts())
   {
      Fire_OnLibraryConflictResolved();
   }
  
   SpecificationChanged(true);
   RatingSpecificationChanged(true);

   return true;
}

void CProjectAgentImp::UpdateJackingForce()
{
   if ( !m_bUpdateJackingForce )
   {
      return; // jacking force is up to date
   }

   // this parameter is to prevent a problem with recurssion
   // it must be a static data member
   static bool bUpdating = true;
   if ( bUpdating )
   {
      return;
   }

   bUpdating = true;

   // If the jacking force in the strands is computed by PGSuper, update the
   // jacking force to reflect the maximum value for the current specifications
   GroupIndexType nGroups = m_BridgeDescription.GetGirderGroupCount();
   for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
   {
      CGirderGroupData* pGroup = m_BridgeDescription.GetGirderGroup(grpIdx);
      GirderIndexType nGirders = pGroup->GetGirderCount();
      for ( GirderIndexType gdrIdx = 0; gdrIdx = nGirders; gdrIdx++ )
      {
         CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);
         SegmentIndexType nSegments = pGirder->GetSegmentCount();
         for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
         {
            CSegmentKey segmentKey(grpIdx,gdrIdx,segIdx);
            CPrecastSegmentData* pSegment = pGirder->GetSegment(segIdx);
            for ( Uint16 i = 0; i < 4; i++ )
            {
               pgsTypes::StrandType strandType = (pgsTypes::StrandType)(i);
               if ( pSegment->Strands.IsPjackCalculated(strandType) )
               {
                  pSegment->Strands.SetPjack(strandType,GetMaxPjack(segmentKey,strandType, pSegment->Strands.GetStrandCount(strandType)));
               }
            }
         } // segment loop
      } // girder loop
   } // group loop

   m_bUpdateJackingForce = false;
   bUpdating = false;
}

void CProjectAgentImp::UpdateJackingForce(const CSegmentKey& segmentKey)
{
   CPrecastSegmentData* pSegment = GetSegment(segmentKey);

   for ( Uint16 i = 0; i < 4; i++ )
   {
      pgsTypes::StrandType strandType = (pgsTypes::StrandType)i;
      if ( pSegment->Strands.IsPjackCalculated(strandType) )
      {
         pSegment->Strands.SetPjack(strandType,GetMaxPjack(segmentKey,strandType,pSegment->Strands.GetStrandCount(strandType)));
      }
   }
}

void CProjectAgentImp::DealWithGirderLibraryChanges(bool fromLibrary)
{
// more work required here - check number of strands, pjack, all stuff changed in CGirderData::ResetPrestressData()
//  PROGRAM WILL CRASH IF:
   // number of strands is incompatible with library strand grid
   // debonding data when number of strands in grid changes - so just blast debond data
   // ATLASSERT IF
   // debond length exceeds 1/2 girder length
#pragma Reminder("Need thorough check of library changes affect to project data")

   GET_IFACE(IPretensionForce,pPrestress);

   GroupIndexType nGroups = m_BridgeDescription.GetGirderGroupCount();
   for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
   {
      CGirderGroupData* pGroup = m_BridgeDescription.GetGirderGroup(grpIdx);

      GirderIndexType nGirders = pGroup->GetGirderCount();
      for (GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
      {
         CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);

         const GirderLibraryEntry* pGdrEntry = pGirder->GetGirderLibraryEntry();

         SegmentIndexType nSegments = pGirder->GetSegmentCount();
         for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
         {
            CPrecastSegmentData* pSegment = pGirder->GetSegment(segIdx);

            CSegmentKey segmentKey(grpIdx,gdrIdx,segIdx);

            // Convert legacy debond data if needed
            ConvertLegacyDebondData(pSegment, pGdrEntry);
            ConvertLegacyExtendedStrandData(pSegment, pGdrEntry);

            // rdp - 11/1/2014 - Mantis 421
            // Added capability to change adjustable strand type from straight to harped in project data. Previously,
            // this was set in library entry only. This is the earliest possible location to intercept discrepancy 
            // between library entry and project data and to set project data correctly for a given library
            // setting.
            pgsTypes::AdjustableStrandType asType    = pSegment->Strands.GetAdjustableStrandType();
            pgsTypes::AdjustableStrandType asLibType = pGdrEntry->GetAdjustableStrandType();
            
            if(asLibType==pgsTypes::asStraight && asType==pgsTypes::asHarped)
            {
               // Library and project are out of sync - this is probably due to 421 version update, but may
               // also be due to a library change. Change project data to match library
               pSegment->Strands.SetAdjustableStrandType(pgsTypes::asStraight);
            }
            else if(asLibType==pgsTypes::asHarped && asType==pgsTypes::asStraight)
            {
               pSegment->Strands.SetAdjustableStrandType(pgsTypes::asHarped);
            }

            ValidateStrands(segmentKey,pSegment,fromLibrary);
   
            Float64 xfer_length = pPrestress->GetXferLength(segmentKey,pgsTypes::Permanent);
            Float64 min_xfer = pGdrEntry->GetMinDebondSectionLength(); 

            if (min_xfer < xfer_length)
            {
               std::_tostringstream os;
               SegmentIndexType nSegments = pGirder->GetSegmentCount();
               if ( 1 < nSegments )
               {
                  os << _T("Group ") << LABEL_GROUP(segmentKey.groupIndex) << _T(" Girder ") << LABEL_GIRDER(segmentKey.girderIndex) << _T(" Segment ") << LABEL_SEGMENT(segmentKey.segmentIndex) << _T(": The minimum debond section length in the girder library is shorter than the transfer length (e.g., 60*Db). This may cause the debonding design algorithm to generate designs that do not pass a specification check.") << std::endl;
               }
               else
               {
                  os << _T("Span ") << LABEL_SPAN(segmentKey.groupIndex) << _T(" Girder ") << LABEL_GIRDER(segmentKey.girderIndex) << _T(": The minimum debond section length in the girder library is shorter than the transfer length (e.g., 60*Db). This may cause the debonding design algorithm to generate designs that do not pass a specification check.") << std::endl;
               }
               AddSegmentStatusItem(segmentKey, os.str() );
            }
         } // segment loop
      } // girder loop
   } // grooup loop
}



void CProjectAgentImp::AddSegmentStatusItem(const CSegmentKey& segmentKey,std::_tstring& message)
{
#pragma Reminder("UPDATE: status items per girder?")
   // should statis items be by girder insteady of by segment?
   // maybe segment is ok for things like segment concrete out of range

   // first post message
   GET_IFACE(IEAFStatusCenter,pStatusCenter);
   pgsGirderDescriptionStatusItem* pStatusItem =  new pgsGirderDescriptionStatusItem(segmentKey,0,m_StatusGroupID,m_scidGirderDescriptionWarning,message.c_str());
   StatusItemIDType st_id = pStatusCenter->Add(pStatusItem);

   // then store message id's for a segment
   StatusIterator iter = m_CurrentGirderStatusItems.find(segmentKey);
   if (iter == m_CurrentGirderStatusItems.end())
   {
      // not found, must insert
      std::pair<StatusContainer::iterator, bool> itbp = m_CurrentGirderStatusItems.insert(StatusContainer::value_type(segmentKey, std::vector<StatusItemIDType>() ));
      ATLASSERT(itbp.second);
      itbp.first->second.push_back(st_id);
   }
   else
   {
      iter->second.push_back(st_id);
   }
}

void CProjectAgentImp::RemoveSegmentStatusItems(const CSegmentKey& segmentKey)
{
   GET_IFACE_NOCHECK(IEAFStatusCenter,pStatusCenter);

   StatusIterator iter( m_CurrentGirderStatusItems.find(segmentKey) );
   if (iter != m_CurrentGirderStatusItems.end())
   {
      std::vector<StatusItemIDType>& rvec = iter->second;

      // remove all status items
      std::vector<StatusItemIDType>::iterator viter(rvec.begin());
      std::vector<StatusItemIDType>::iterator viterEnd(rvec.end());
      for ( ; viter != viterEnd; viter++)
      {
         StatusItemIDType id = *viter;
         pStatusCenter->RemoveByID(id);
      }

      rvec.clear();
   }
}

void CProjectAgentImp::InitSpecification(const std::_tstring& spec)
{
   if ( m_Spec != spec )
   {
      m_Spec.erase();
      m_Spec = spec;

      release_library_entry( &m_LibObserver, 
                             m_pSpecEntry,
                             *(m_pLibMgr->GetSpecLibrary()) );

      use_library_entry( &m_LibObserver,
                         m_Spec,
                         &m_pSpecEntry,
                         *(m_pLibMgr->GetSpecLibrary()) );

      lrfdVersionMgr::SetVersion( m_pSpecEntry->GetSpecificationType() );
      lrfdVersionMgr::SetUnits( m_pSpecEntry->GetSpecificationUnits() );
   }
}

void CProjectAgentImp::InitRatingSpecification(const std::_tstring& spec)
{
   if ( m_RatingSpec != spec )
   {
      m_RatingSpec.erase();
      m_RatingSpec = spec;

      release_library_entry( &m_LibObserver, 
                             m_pRatingEntry,
                             *(m_pLibMgr->GetRatingLibrary()) );

      use_library_entry( &m_LibObserver,
                         m_RatingSpec,
                         &m_pRatingEntry,
                         *(m_pLibMgr->GetRatingLibrary()) );

      lrfrVersionMgr::SetVersion( m_pRatingEntry->GetSpecificationVersion() );

      // update live load factors
      if ( m_pRatingEntry->GetSpecificationVersion() < lrfrVersionMgr::SecondEditionWith2013Interims )
      {
         const CLiveLoadFactorModel& design_inventory_model = m_pRatingEntry->GetLiveLoadFactorModel(pgsTypes::lrDesign_Inventory);
         if ( !design_inventory_model.AllowUserOverride() )
         {
            m_gLL[pgsTypes::StrengthI_Inventory]  = COMPUTE_LLDF;
            m_gLL[pgsTypes::ServiceIII_Inventory] = COMPUTE_LLDF;
         }

         const CLiveLoadFactorModel& design_operating_model = m_pRatingEntry->GetLiveLoadFactorModel(pgsTypes::lrDesign_Operating);
         if ( !design_operating_model.AllowUserOverride() )
         {
            m_gLL[pgsTypes::StrengthI_Operating]  = COMPUTE_LLDF;
            m_gLL[pgsTypes::ServiceIII_Operating] = COMPUTE_LLDF;
         }

         const CLiveLoadFactorModel& legal_routine_model = m_pRatingEntry->GetLiveLoadFactorModel(pgsTypes::lrLegal_Routine);
         if ( !legal_routine_model.AllowUserOverride() )
         {
            m_gLL[pgsTypes::StrengthI_LegalRoutine]  = COMPUTE_LLDF;
            m_gLL[pgsTypes::ServiceIII_LegalRoutine] = COMPUTE_LLDF;
         }

         const CLiveLoadFactorModel& legal_special_model = m_pRatingEntry->GetLiveLoadFactorModel(pgsTypes::lrLegal_Special);
         if ( !legal_special_model.AllowUserOverride() )
         {
            m_gLL[pgsTypes::StrengthI_LegalSpecial]  = COMPUTE_LLDF;
            m_gLL[pgsTypes::ServiceIII_LegalSpecial] = COMPUTE_LLDF;
         }

         const CLiveLoadFactorModel& permit_routine_model = m_pRatingEntry->GetLiveLoadFactorModel(pgsTypes::lrPermit_Routine);
         if ( !permit_routine_model.AllowUserOverride() )
         {
            m_gLL[pgsTypes::StrengthII_PermitRoutine]  = COMPUTE_LLDF;
            m_gLL[pgsTypes::ServiceI_PermitRoutine]    = COMPUTE_LLDF;
         }

         if ( m_SpecialPermitType == pgsTypes::ptSingleTripWithEscort )
         {
            const CLiveLoadFactorModel& permit_special_single_trip_escorted_model = m_pRatingEntry->GetLiveLoadFactorModel(pgsTypes::ptSingleTripWithEscort);
            if ( !permit_special_single_trip_escorted_model.AllowUserOverride() )
            {
               m_gLL[pgsTypes::StrengthII_PermitSpecial]  = COMPUTE_LLDF;
               m_gLL[pgsTypes::ServiceI_PermitSpecial]    = COMPUTE_LLDF;
            }
         }
         else if ( m_SpecialPermitType == pgsTypes::ptSingleTripWithTraffic )
         {
            const CLiveLoadFactorModel& permit_special_single_trip_traffic_model = m_pRatingEntry->GetLiveLoadFactorModel(pgsTypes::ptSingleTripWithTraffic);
            if ( !permit_special_single_trip_traffic_model.AllowUserOverride() )
            {
               m_gLL[pgsTypes::StrengthII_PermitSpecial]  = COMPUTE_LLDF;
               m_gLL[pgsTypes::ServiceI_PermitSpecial]    = COMPUTE_LLDF;
            }
         }
         else if ( m_SpecialPermitType == pgsTypes::ptMultipleTripWithTraffic )
         {
            const CLiveLoadFactorModel& permit_special_multiple_trip_traffic_model = m_pRatingEntry->GetLiveLoadFactorModel(pgsTypes::ptMultipleTripWithTraffic);
            if ( !permit_special_multiple_trip_traffic_model.AllowUserOverride() )
            {
               m_gLL[pgsTypes::StrengthII_PermitSpecial]  = COMPUTE_LLDF;
               m_gLL[pgsTypes::ServiceI_PermitSpecial]    = COMPUTE_LLDF;
            }
         }
         else
         {
            ATLASSERT(false); // should never get here
         }
      }
      else
      {
         const CLiveLoadFactorModel2& design_inventory_model = m_pRatingEntry->GetLiveLoadFactorModel2(pgsTypes::lrDesign_Inventory);
         if ( !design_inventory_model.AllowUserOverride() )
         {
            m_gLL[pgsTypes::StrengthI_Inventory]  = COMPUTE_LLDF;
            m_gLL[pgsTypes::ServiceIII_Inventory] = COMPUTE_LLDF;
         }

         const CLiveLoadFactorModel2& design_operating_model = m_pRatingEntry->GetLiveLoadFactorModel2(pgsTypes::lrDesign_Operating);
         if ( !design_operating_model.AllowUserOverride() )
         {
            m_gLL[pgsTypes::StrengthI_Operating]  = COMPUTE_LLDF;
            m_gLL[pgsTypes::ServiceIII_Operating] = COMPUTE_LLDF;
         }

         const CLiveLoadFactorModel2& legal_routine_model = m_pRatingEntry->GetLiveLoadFactorModel2(pgsTypes::lrLegal_Routine);
         if ( !legal_routine_model.AllowUserOverride() )
         {
            m_gLL[pgsTypes::StrengthI_LegalRoutine]  = COMPUTE_LLDF;
            m_gLL[pgsTypes::ServiceIII_LegalRoutine] = COMPUTE_LLDF;
         }

         const CLiveLoadFactorModel2& legal_special_model = m_pRatingEntry->GetLiveLoadFactorModel2(pgsTypes::lrLegal_Special);
         if ( !legal_special_model.AllowUserOverride() )
         {
            m_gLL[pgsTypes::StrengthI_LegalSpecial]  = COMPUTE_LLDF;
            m_gLL[pgsTypes::ServiceIII_LegalSpecial] = COMPUTE_LLDF;
         }

         const CLiveLoadFactorModel2& emergency_model = m_pRatingEntry->GetLiveLoadFactorModel2(pgsTypes::lrLegal_Emergency);
         if (!emergency_model.AllowUserOverride())
         {
            m_gLL[pgsTypes::StrengthI_LegalEmergency] = COMPUTE_LLDF;
            m_gLL[pgsTypes::ServiceIII_LegalEmergency] = COMPUTE_LLDF;
         }

         const CLiveLoadFactorModel2& permit_routine_model = m_pRatingEntry->GetLiveLoadFactorModel2(pgsTypes::lrPermit_Routine);
         if ( !permit_routine_model.AllowUserOverride() )
         {
            m_gLL[pgsTypes::StrengthII_PermitRoutine]  = COMPUTE_LLDF;
            m_gLL[pgsTypes::ServiceI_PermitRoutine]    = COMPUTE_LLDF;
         }

         if ( m_SpecialPermitType == pgsTypes::ptSingleTripWithEscort )
         {
            const CLiveLoadFactorModel2& permit_special_single_trip_escorted_model = m_pRatingEntry->GetLiveLoadFactorModel2(pgsTypes::ptSingleTripWithEscort);
            if ( !permit_special_single_trip_escorted_model.AllowUserOverride() )
            {
               m_gLL[pgsTypes::StrengthII_PermitSpecial]  = COMPUTE_LLDF;
               m_gLL[pgsTypes::ServiceI_PermitSpecial]    = COMPUTE_LLDF;
            }
         }
         else if ( m_SpecialPermitType == pgsTypes::ptSingleTripWithTraffic )
         {
            const CLiveLoadFactorModel2& permit_special_single_trip_traffic_model = m_pRatingEntry->GetLiveLoadFactorModel2(pgsTypes::ptSingleTripWithTraffic);
            if ( !permit_special_single_trip_traffic_model.AllowUserOverride() )
            {
               m_gLL[pgsTypes::StrengthII_PermitSpecial]  = COMPUTE_LLDF;
               m_gLL[pgsTypes::ServiceI_PermitSpecial]    = COMPUTE_LLDF;
            }
         }
         else if ( m_SpecialPermitType == pgsTypes::ptMultipleTripWithTraffic )
         {
            const CLiveLoadFactorModel2& permit_special_multiple_trip_traffic_model = m_pRatingEntry->GetLiveLoadFactorModel2(pgsTypes::ptMultipleTripWithTraffic);
            if ( !permit_special_multiple_trip_traffic_model.AllowUserOverride() )
            {
               m_gLL[pgsTypes::StrengthII_PermitSpecial]  = COMPUTE_LLDF;
               m_gLL[pgsTypes::ServiceI_PermitSpecial]    = COMPUTE_LLDF;
            }
         }
         else
         {
            ATLASSERT(false); // should never get here
         }
      }
   }
}

Float64 CProjectAgentImp::GetMaxPjack(const CSegmentKey& segmentKey,pgsTypes::StrandType type,StrandIndexType nStrands) const
{
   if ( type == pgsTypes::Permanent )
   {
      type = pgsTypes::Straight;
   }

   GET_IFACE(IPretensionForce,pPrestress);
   return pPrestress->GetPjackMax(segmentKey,*GetStrandMaterial(segmentKey,type),nStrands);
}

Float64 CProjectAgentImp::GetMaxPjack(const CSegmentKey& segmentKey,StrandIndexType nStrands,const matPsStrand* pStrand) const
{
   GET_IFACE(IPretensionForce,pPrestress);
   return pPrestress->GetPjackMax(segmentKey,*pStrand,nStrands);
}


HRESULT CProjectAgentImp::FireContinuityRelatedSpanChange(const CSpanKey& spanKey,Uint32 lHint)
{
   // Some individual girder changes can affect an entire girderline
   SpanIndexType startSpanIdx = (spanKey.spanIndex == ALL_SPANS ? 0 : spanKey.spanIndex);
   SpanIndexType endSpanIdx   = (spanKey.spanIndex == ALL_SPANS ? m_BridgeDescription.GetSpanCount()-1 : startSpanIdx);
   for ( SpanIndexType spanIdx = startSpanIdx; spanIdx <= endSpanIdx; spanIdx++ )
   {
      const CSpanData2* pSpan = m_BridgeDescription.GetSpan(spanIdx);
      if (nullptr==pSpan)
      {
         // Events should not be coming for non-existent spans, but let's avoid crashing...
         ATLASSERT(0);
         Fire_BridgeChanged();
         continue;
      }

      const CGirderGroupData* pGroup = m_BridgeDescription.GetGirderGroup(pSpan);
      if (nullptr==pGroup)
      {
         ATLASSERT(0);
         Fire_BridgeChanged();
         continue;
      }

      GroupIndexType grpIdx = pGroup->GetIndex();

      GirderIndexType continuityGirderIdx = (spanKey.girderIndex == ALL_GIRDERS) ? 0 : spanKey.girderIndex;
      CGirderKey girderKey(grpIdx,continuityGirderIdx);
      
      GET_IFACE(IContinuity,pContinuity);
      if (pContinuity->IsContinuityFullyEffective(girderKey))
      {
         grpIdx = ALL_GROUPS; // assume the entire girder line is affected...specify change affects all groups
      }

      CGirderKey key(grpIdx,spanKey.girderIndex);
      HRESULT hr = Fire_GirderChanged(key, lHint); 
      if ( FAILED(hr) )
      {
         return hr;
      }

      // we only want to do this once per girder, which a girder can cover more than one span.
      // get the last span in the current group
      const CSpanData2* pLastSpan = pGroup->GetPier(pgsTypes::metEnd)->GetPrevSpan();

      // move the span index to the last span in this group
      // so that when we increment spanIdx it goes into the next group
      spanIdx = pLastSpan->GetIndex();
   }
   return S_OK;
}

CPrecastSegmentData* CProjectAgentImp::GetSegment(const CSegmentKey& segmentKey)
{
   CGirderGroupData* pGroup = m_BridgeDescription.GetGirderGroup(segmentKey.groupIndex);
   CSplicedGirderData* pGirder = pGroup->GetGirder(segmentKey.girderIndex);
   return pGirder->GetSegment(segmentKey.segmentIndex);
}

const CPrecastSegmentData* CProjectAgentImp::GetSegment(const CSegmentKey& segmentKey) const
{
   const CGirderGroupData* pGroup = m_BridgeDescription.GetGirderGroup(segmentKey.groupIndex);
   const CSplicedGirderData* pGirder = pGroup->GetGirder(segmentKey.girderIndex);
   return pGirder->GetSegment(segmentKey.segmentIndex);
}

void CProjectAgentImp::CreatePrecastGirderBridgeTimelineEvents()
{
   const SpecLibraryEntry* pSpecEntry = GetSpecEntry(m_Spec.c_str());

   // need to capture the mapping between user defined load IDs and event indicies
   // when we clear the timeline, the mapping is lost
   CTimelineManager* pTimelineManager = m_BridgeDescription.GetTimelineManager();
   std::map<IDType,IndexType> loadMap;
   bool bOldLoads = false; // lets us know if we have old loads
   IndexType oldBridgeSite1EventIndex(INVALID_INDEX), oldBridgeSite2EventIndex(INVALID_INDEX), oldBridgeSite3EventIndex(INVALID_INDEX); // we will capture the event index for the old bridge site loads here... we'll need them for old lad mapper
   if ( m_bUpdateUserDefinedLoads )
   {
      // this is an old file so get the loading from the load manager
      bOldLoads = true;
      IndexType nLoads = m_LoadManager.GetPointLoadCount();
      for ( IndexType loadIdx = 0; loadIdx < nLoads; loadIdx++ )
      {
         const CPointLoadData* pLoad = m_LoadManager.GetPointLoad(loadIdx);
         ATLASSERT(pLoad->m_StageIndex != INVALID_INDEX);
         loadMap.insert(std::make_pair(pLoad->m_ID,pLoad->m_StageIndex));
      }

      nLoads = m_LoadManager.GetDistributedLoadCount();
      for ( IndexType loadIdx = 0; loadIdx < nLoads; loadIdx++ )
      {
         const CDistributedLoadData* pLoad = m_LoadManager.GetDistributedLoad(loadIdx);
         ATLASSERT(pLoad->m_StageIndex != INVALID_INDEX);
         loadMap.insert(std::make_pair(pLoad->m_ID,pLoad->m_StageIndex));
      }

      nLoads = m_LoadManager.GetMomentLoadCount();
      for ( IndexType loadIdx = 0; loadIdx < nLoads; loadIdx++ )
      {
         const CMomentLoadData* pLoad = m_LoadManager.GetMomentLoad(loadIdx);
         ATLASSERT(pLoad->m_StageIndex != INVALID_INDEX);
         loadMap.insert(std::make_pair(pLoad->m_ID,pLoad->m_StageIndex));
      }

      m_bUpdateUserDefinedLoads = false;
   }
   else
   {
      EventIndexType nEvents = pTimelineManager->GetEventCount();
      for ( EventIndexType eventIdx = 0; eventIdx < nEvents; eventIdx++ )
      {
         CTimelineEvent* pEvent = pTimelineManager->GetEventByIndex(eventIdx);
         CApplyLoadActivity& loadActivity = pEvent->GetApplyLoadActivity();
         if ( loadActivity.IsEnabled() && loadActivity.IsUserLoadApplied() )
         {
            IndexType nUserLoads = loadActivity.GetUserLoadCount();
            for ( IndexType userLoadIdx = 0; userLoadIdx < nUserLoads; userLoadIdx++ )
            {
               LoadIDType loadID = loadActivity.GetUserLoadID(userLoadIdx);

               if (pEvent->GetCastDeckActivity().IsEnabled() || pEvent->GetApplyLoadActivity().IsIntermediateDiaphragmLoadApplied() )
               {
                  loadMap.insert(std::make_pair(loadID, 2));
               }
               else if (pEvent->GetApplyLoadActivity().IsRailingSystemLoadApplied())
               {
                  loadMap.insert(std::make_pair(loadID, 3));
               }
               else if (pEvent->GetApplyLoadActivity().IsLiveLoadApplied())
               {
                  loadMap.insert(std::make_pair(loadID, 4));
               }
               else
               {
                  ATLASSERT(false);
               }
            }
         }
      }
   }

   pTimelineManager->Clear();

   // get a list of all the segment IDs. it is needed in a couple locations below
   std::set<SegmentIDType> segmentIDs;
   GroupIndexType nGroups = m_BridgeDescription.GetGirderGroupCount();
   for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
   {
      const CGirderGroupData* pGroup = m_BridgeDescription.GetGirderGroup(grpIdx);
      GirderIndexType nGirders = pGroup->GetGirderCount();
      for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
      {
         const CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);
         SegmentIndexType nSegments = pGirder->GetSegmentCount();
         for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
         {
            const CPrecastSegmentData* pSegment = pGirder->GetSegment(segIdx);
            SegmentIDType segmentID = pSegment->GetID();
            std::pair<std::set<SegmentIDType>::iterator,bool> result = segmentIDs.insert(segmentID);
            ATLASSERT(result.second == true);
         }
      }
   }

   // NOTE: The actual timing doesn't matter since we aren't doing a true time-step analysis
   // We will just use reasonable times so the sequence is correct

   // Casting yard stage... starts at day 0 when strands are stressed
   // The activities in this stage includes prestress release, lifting and storage
   std::unique_ptr<CTimelineEvent> pTimelineEvent = std::make_unique<CTimelineEvent>();
   pTimelineEvent->SetDay(0);
   pTimelineEvent->SetDescription(_T("Construct Girders, Erect Piers"));
   pTimelineEvent->GetConstructSegmentsActivity().Enable();
   pTimelineEvent->GetConstructSegmentsActivity().SetAgeAtRelease(  ::ConvertFromSysUnits(pSpecEntry->GetXferTime(),unitMeasure::Day));
   pTimelineEvent->GetConstructSegmentsActivity().SetRelaxationTime(::ConvertFromSysUnits(pSpecEntry->GetXferTime(),unitMeasure::Day));
   pTimelineEvent->GetConstructSegmentsActivity().AddSegments(segmentIDs);

   // assume piers are erected at the same time girders are being constructed
   pTimelineEvent->GetErectPiersActivity().Enable();
   PierIndexType nPiers = m_BridgeDescription.GetPierCount();
   for ( PierIndexType pierIdx = 0; pierIdx < nPiers; pierIdx++ )
   {
      const CPierData2* pPier = m_BridgeDescription.GetPier(pierIdx);
      PierIDType pierID = pPier->GetID();
      pTimelineEvent->GetErectPiersActivity().AddPier(pierID);
   }

   EventIndexType eventIdx;
   pTimelineManager->AddTimelineEvent(pTimelineEvent.release(),true,&eventIdx);

   // Erect girders. It is assumed that girders are transported, erected, and temporary strands 
   // are removed all on the same day. Assuming max construction sequence (D120). The actual
   // don't matter unless the user switches to time-step analysis.
   pTimelineEvent = std::make_unique<CTimelineEvent>();
   Float64 day = ::ConvertFromSysUnits(pSpecEntry->GetXferTime()+pSpecEntry->GetCreepDuration1Max(),unitMeasure::Day);
   Float64 maxDay = 28.0;
   day = Max(day,maxDay);
   maxDay += 1.0;
   pTimelineEvent->SetDay( day );
   pTimelineEvent->SetDescription(_T("Erect Girders"));
   pTimelineEvent->GetErectSegmentsActivity().Enable();
   pTimelineEvent->GetErectSegmentsActivity().AddSegments(segmentIDs);
   pTimelineManager->AddTimelineEvent(pTimelineEvent.release(),true,&eventIdx);

   pgsTypes::SupportedDeckType deckType = m_BridgeDescription.GetDeckDescription()->GetDeckType();

   Float64 deck_diaphragm_curing_duration = Min(::ConvertFromSysUnits(pSpecEntry->GetTotalCreepDuration() - pSpecEntry->GetCreepDuration2Max(),unitMeasure::Day),28.0);
   if ( IsJointSpacing(m_BridgeDescription.GetGirderSpacingType()) && m_BridgeDescription.HasLongitudinalJoints() )
   {
      // No deck
      // 1) Diaphragms
      // 2) Joints

      // deck or overlay
      // 1) Diaphragms
      // 2) Joints
      // 3) Deck
      pTimelineEvent = std::make_unique<CTimelineEvent>();
      day = ::ConvertFromSysUnits(pSpecEntry->GetXferTime() + pSpecEntry->GetCreepDuration2Max(), unitMeasure::Day);
      day = Max(day, maxDay);
      maxDay += 1.0;
      pTimelineEvent->SetDay(day);
      pTimelineEvent->SetDescription(_T("Cast Diaphragms"));
      pTimelineEvent->GetApplyLoadActivity().ApplyIntermediateDiaphragmLoad();
      pTimelineManager->AddTimelineEvent(pTimelineEvent.release(), true, &eventIdx);

      pTimelineEvent = std::make_unique<CTimelineEvent>();
      day = ::ConvertFromSysUnits(pSpecEntry->GetXferTime() + pSpecEntry->GetCreepDuration2Max(), unitMeasure::Day) + 1.0;
      day = Max(day, maxDay);
      maxDay += 1.0;
      pTimelineEvent->SetDay(day);
      pTimelineEvent->SetDescription(_T("Cast Longitudinal Joints"));

      pTimelineEvent->GetCastLongitudinalJointActivity().Enable();
      pTimelineEvent->GetCastLongitudinalJointActivity().SetConcreteAgeAtContinuity(1.0); // day
      pTimelineEvent->GetCastLongitudinalJointActivity().SetCuringDuration(1.0); // day
      pTimelineManager->AddTimelineEvent(pTimelineEvent.release(), true, &eventIdx);
      oldBridgeSite1EventIndex = eventIdx;

      if ( deckType != pgsTypes::sdtNone)
      {
         pTimelineEvent = std::make_unique<CTimelineEvent>();
         day = ::ConvertFromSysUnits(pSpecEntry->GetXferTime() + pSpecEntry->GetCreepDuration2Max(), unitMeasure::Day) + 2.0;
         day = Max(day, maxDay);
         maxDay += 1.0;
         pTimelineEvent->SetDay(day);


         pTimelineEvent->SetDescription(GetCastDeckEventName(deckType));
         pTimelineEvent->GetCastDeckActivity().Enable();
         pTimelineEvent->GetCastDeckActivity().SetConcreteAgeAtContinuity(deck_diaphragm_curing_duration); // day
         pTimelineEvent->GetCastDeckActivity().SetCuringDuration(deck_diaphragm_curing_duration); // day
         pTimelineManager->AddTimelineEvent(pTimelineEvent.release(), true, &eventIdx);
         oldBridgeSite1EventIndex = eventIdx;
      }
   }
   else
   {
      // Cast deck & diaphragms
      pTimelineEvent = std::make_unique<CTimelineEvent>();
      day = ::ConvertFromSysUnits(pSpecEntry->GetXferTime() + pSpecEntry->GetCreepDuration2Max(), unitMeasure::Day);
      day = Max(day, maxDay);
      maxDay += 1.0;
      pTimelineEvent->SetDay(day);
      pTimelineEvent->SetDescription(_T("Cast Diaphragms"));

      pTimelineEvent->GetApplyLoadActivity().ApplyIntermediateDiaphragmLoad();
      pTimelineManager->AddTimelineEvent(pTimelineEvent.release(), true, &eventIdx);
      oldBridgeSite1EventIndex = eventIdx;


      if (deckType != pgsTypes::sdtNone)
      {
         pTimelineEvent = std::make_unique<CTimelineEvent>();
         day = ::ConvertFromSysUnits(pSpecEntry->GetXferTime() + pSpecEntry->GetCreepDuration2Max(), unitMeasure::Day);
         day = Max(day, maxDay);
         maxDay += 1.0;
         pTimelineEvent->SetDay(day);
         pTimelineEvent->SetDescription(GetCastDeckEventName(deckType));
         pTimelineEvent->GetCastDeckActivity().Enable();
         pTimelineEvent->GetCastDeckActivity().SetConcreteAgeAtContinuity(deck_diaphragm_curing_duration); // day
         pTimelineEvent->GetCastDeckActivity().SetCuringDuration(deck_diaphragm_curing_duration); // day
         pTimelineManager->AddTimelineEvent(pTimelineEvent.release(), true, &eventIdx);
         oldBridgeSite1EventIndex = eventIdx;
      }
   }

   // traffic barrier/superimposed dead loads
   pTimelineEvent = std::make_unique<CTimelineEvent>();
   day = ::ConvertFromSysUnits(pSpecEntry->GetXferTime()+pSpecEntry->GetCreepDuration2Max(),unitMeasure::Day) + deck_diaphragm_curing_duration;
   day = Max(day,maxDay);
   maxDay += 1.0;
   pTimelineEvent->SetDay( day ); // deck is continuous
   pTimelineEvent->GetApplyLoadActivity().ApplyRailingSystemLoad();

   pgsTypes::WearingSurfaceType wearingSurface = m_BridgeDescription.GetDeckDescription()->WearingSurface;
   if (  wearingSurface == pgsTypes::wstSacrificialDepth || wearingSurface == pgsTypes::wstOverlay )
   {
      pTimelineEvent->SetDescription(_T("Final without Live Load"));
      if ( wearingSurface == pgsTypes::wstOverlay )
      {
         pTimelineEvent->GetApplyLoadActivity().ApplyOverlayLoad();
      }
   }
   else
   {
      pTimelineEvent->SetDescription(_T("Install Railing System"));
   }
   pTimelineManager->AddTimelineEvent(pTimelineEvent.release(),true,&eventIdx);
   oldBridgeSite2EventIndex = eventIdx; // "bridge site 2 loads are always applied with the railing system

   if ( wearingSurface == pgsTypes::wstFutureOverlay )
   {
      pTimelineEvent = std::make_unique<CTimelineEvent>();
      day = ::ConvertFromSysUnits(pSpecEntry->GetXferTime() + pSpecEntry->GetTotalCreepDuration(), unitMeasure::Day) + 1.0;
      day = Max(day, maxDay);
      maxDay += 1.0;
      pTimelineEvent->SetDay( day ); 
      pTimelineEvent->SetDescription(_T("Final without Live Load"));
      pTimelineEvent->GetApplyLoadActivity().ApplyOverlayLoad();
      pTimelineManager->AddTimelineEvent(pTimelineEvent.release(),true,&eventIdx);
   }

   // live load
   pTimelineEvent = std::make_unique<CTimelineEvent>();
   day = ::ConvertFromSysUnits(pSpecEntry->GetXferTime()+pSpecEntry->GetTotalCreepDuration(),unitMeasure::Day) + 1.0;
   day = Max(day, maxDay);
   maxDay += 1.0;
   pTimelineEvent->SetDay( day );
   pTimelineEvent->SetDescription(_T("Final with Live Load"));
   pTimelineEvent->GetApplyLoadActivity().ApplyLiveLoad();
   pTimelineEvent->GetApplyLoadActivity().ApplyRatingLiveLoad();
   pTimelineManager->AddTimelineEvent(pTimelineEvent.release(),true,&eventIdx);
   oldBridgeSite3EventIndex = eventIdx;

   // user defined loads
   std::map<IDType,IndexType>::iterator loadIter(loadMap.begin());
   std::map<IDType,IndexType>::iterator loadIterEnd(loadMap.end());
   for ( ; loadIter != loadIterEnd; loadIter++ )
   {
      IDType loadID = loadIter->first;
      EventIndexType eventIdx = loadIter->second;
      switch (eventIdx)
      {
      case 2:
         eventIdx = oldBridgeSite1EventIndex;
         break;
      case 3:
         eventIdx = oldBridgeSite2EventIndex;
         break;
      case 4:
         eventIdx = oldBridgeSite3EventIndex;
         break;

      default:
         ATLASSERT(false); // should never get here
      }
      CTimelineEvent* pEvent = pTimelineManager->GetEventByIndex(eventIdx);
      pEvent->GetApplyLoadActivity().AddUserLoad(loadID);
   }
}

// Static functions for checking direct strand fills
bool IsValidStraightStrandFill(const CDirectStrandFillCollection* pFill, const GirderLibraryEntry* pGirderEntry)
{
    StrandIndexType ns =  pGirderEntry->GetNumStraightStrandCoordinates();

    CDirectStrandFillCollection::const_iterator it    = pFill->begin();
    CDirectStrandFillCollection::const_iterator itend = pFill->end();

    while(it != itend)
    {
       const CDirectStrandFillInfo& rInfo = *it;
       if (rInfo.permStrandGridIdx < ns)
       {
          Float64 xs, xe, ys, ye;
          bool candb;
          pGirderEntry->GetStraightStrandCoordinates(rInfo.permStrandGridIdx, &xs, &ys, &xe, &ye, &candb);
          ATLASSERT(rInfo.numFilled ==1 || rInfo.numFilled ==2);
          if (rInfo.numFilled ==2)
          {
             if (xs==0.0 && xe==0.0)
             {
                return false;
             }
          }
       }
       else
       {
          // fill doesn't fit into grid
          return false;
       }

       it++;
    }

    return true;
}

bool IsValidHarpedStrandFill(const CDirectStrandFillCollection* pFill, const GirderLibraryEntry* pGirderEntry)
{
    StrandIndexType ns =  pGirderEntry->GetNumHarpedStrandCoordinates();

    CDirectStrandFillCollection::const_iterator it    = pFill->begin();
    CDirectStrandFillCollection::const_iterator itend = pFill->end();

    while(it != itend)
    {
       const CDirectStrandFillInfo& rInfo = *it;
       if (rInfo.permStrandGridIdx < ns)
       {
          Float64 xs, xe, xh, ys, ye, yh;
          pGirderEntry->GetHarpedStrandCoordinates(rInfo.permStrandGridIdx, &xs, &ys, &xh, &yh, &xe, &ye);
          ATLASSERT(rInfo.numFilled ==1 || rInfo.numFilled ==2);
          if (rInfo.numFilled ==2)
          {
             if (xs==0.0 && xh==0.0 && xe==0.0)
             {
                return false; // can only fill 1
             }
          }
       }
       else
       {
          // fill doesn't fit into grid
          return false;
       }

       it++;
    }

    return true;
}

bool IsValidTemporaryStrandFill(const CDirectStrandFillCollection* pFill, const GirderLibraryEntry* pGirderEntry)
{
    StrandIndexType ns =  pGirderEntry->GetNumTemporaryStrandCoordinates();

    CDirectStrandFillCollection::const_iterator it    = pFill->begin();
    CDirectStrandFillCollection::const_iterator itend = pFill->end();

    while(it != itend)
    {
       const CDirectStrandFillInfo& rInfo = *it;
       if (rInfo.permStrandGridIdx < ns)
       {
          Float64 xs, xe, ys, ye;
          pGirderEntry->GetTemporaryStrandCoordinates(rInfo.permStrandGridIdx, &xs, &ys, &xe, &ye);
          ATLASSERT(rInfo.numFilled ==1 || rInfo.numFilled ==2);
          if (rInfo.numFilled ==2)
          {
             if (xs==0.0 && xe==0.0)
             {
                return false;
             }
          }
       }
       else
       {
          // fill doesn't fit into grid
          return false;
       }

       it++;
    }

    return true;
}

bool CProjectAgentImp::HasUserLoad(const CGirderKey& girderKey,UserLoads::LoadCase lcType) const
{
   return m_LoadManager.HasUserLoad(girderKey,lcType);
}

void CProjectAgentImp::UpdateHaulTruck(const COldHaulTruck* pOldHaulTruck)
{
   // The spec library that is used with this file was stored in an older format.
   // The haul truck definition was part of the spec. We now have a Haul Truck library
   // that has multiple haul truck definitions. We have to update the bridge model
   // so that each segment references a haul truck library entry

   if ( pOldHaulTruck->m_TruckRollStiffnessMethod == ROLLSTIFFNESS_LUMPSUM )
   {
      // the truck roll stiffness was specified directly...

      // first see if an entry in the Haul Truck Library matches the old haul truck
      HaulTruckLibrary* pHaulTruckLibrary = GetHaulTruckLibrary();
      const HaulTruckLibraryEntry* pEntry = FindHaulTruckLibraryEntry(pOldHaulTruck);
      if ( pEntry == nullptr )
      {
         // Nope... create a new entry
         std::_tstring strName = pHaulTruckLibrary->GetUniqueEntryName(_T("Old Haul Truck -"));
         VERIFY(pHaulTruckLibrary->NewEntry(strName.c_str()));
         pEntry = (const HaulTruckLibraryEntry*)pHaulTruckLibrary->GetEntry(strName.c_str());
         HaulTruckLibraryEntry* pncEntry = const_cast<HaulTruckLibraryEntry*>(pEntry);
         pOldHaulTruck->InitEntry(pncEntry);
      }

      ATLASSERT(pEntry != nullptr);
      std::_tstring strName = pEntry->GetName();

      GroupIndexType nGroups = m_BridgeDescription.GetGirderGroupCount();
      for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
      {
         CGirderGroupData* pGroup = m_BridgeDescription.GetGirderGroup(grpIdx);
         GirderIndexType nGirders = pGroup->GetGirderCount();
         for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
         {
            CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);
            SegmentIndexType nSegments = pGirder->GetSegmentCount();
            for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
            {
               CPrecastSegmentData* pSegment = pGirder->GetSegment(segIdx);
               pSegment->HandlingData.HaulTruckName = strName;

               const HaulTruckLibraryEntry* pHaulTruckEntry;
               use_library_entry(&m_LibObserver,pSegment->HandlingData.HaulTruckName,&pHaulTruckEntry,*pHaulTruckLibrary);
               pSegment->HandlingData.pHaulTruckLibraryEntry = pHaulTruckEntry;
            }
         }
      }
   }
   else
   {
      // truck roll stiffness is a function of the girder weight
      HaulTruckLibrary* pHaulTruckLibrary = GetHaulTruckLibrary();
      GET_IFACE(ISectionProperties,pSectProps);
      GroupIndexType nGroups = m_BridgeDescription.GetGirderGroupCount();
      for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
      {
         CGirderGroupData* pGroup = m_BridgeDescription.GetGirderGroup(grpIdx);
         GirderIndexType nGirders = pGroup->GetGirderCount();
         for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
         {
            CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);
            SegmentIndexType nSegments = pGirder->GetSegmentCount();
            for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
            {
               CSegmentKey segmentKey(grpIdx,gdrIdx,segIdx);
               Float64 W = pSectProps->GetSegmentWeight(segmentKey);
               int nAxles = int(W / pOldHaulTruck->m_AxleWeightLimit) + 1;
               Float64 ktheta = Max(nAxles * pOldHaulTruck->m_AxleStiffness, pOldHaulTruck->m_MinRollStiffness);

               const HaulTruckLibraryEntry* pEntry = FindHaulTruckLibraryEntry(ktheta,pOldHaulTruck);
               if ( pEntry == nullptr )
               {
                  // Nope... create a new entry
                  std::_tstring strName = pHaulTruckLibrary->GetUniqueEntryName(_T("Old Haul Truck-"));
                  VERIFY(pHaulTruckLibrary->NewEntry(strName.c_str()));
                  pEntry = (const HaulTruckLibraryEntry*)pHaulTruckLibrary->GetEntry(strName.c_str());
                  HaulTruckLibraryEntry* pncEntry = const_cast<HaulTruckLibraryEntry*>(pEntry);
                  pOldHaulTruck->InitEntry(ktheta,pncEntry);
               }

               ATLASSERT(pEntry != nullptr);
               std::_tstring strName = pEntry->GetName();

               CPrecastSegmentData* pSegment = pGirder->GetSegment(segIdx);
               pSegment->HandlingData.HaulTruckName = strName;
               const HaulTruckLibraryEntry* pHaulTruckEntry;
               use_library_entry(&m_LibObserver,pSegment->HandlingData.HaulTruckName,&pHaulTruckEntry,*pHaulTruckLibrary);
               pSegment->HandlingData.pHaulTruckLibraryEntry = pHaulTruckEntry;
            }
         }
      }
   }
}

const HaulTruckLibraryEntry* CProjectAgentImp::FindHaulTruckLibraryEntry(const COldHaulTruck* pOldHaulTruck)
{
   HaulTruckLibrary* pLib = GetHaulTruckLibrary();
   IndexType nEntries = pLib->GetCount();
   std::vector<std::_tstring> vKeys;
   pLib->KeyList(vKeys);
   for ( IndexType entryIdx = 0; entryIdx < nEntries; entryIdx++ )
   {
      const HaulTruckLibraryEntry* pEntry = (const HaulTruckLibraryEntry*)pLib->GetEntry(vKeys[entryIdx].c_str());
      if ( pOldHaulTruck->IsEqual(pEntry) )
      {
         return pEntry;
      }
   }

   return nullptr;
}

const HaulTruckLibraryEntry* CProjectAgentImp::FindHaulTruckLibraryEntry(Float64 kTheta,const COldHaulTruck* pOldHaulTruck)
{
   HaulTruckLibrary* pLib = GetHaulTruckLibrary();
   IndexType nEntries = pLib->GetCount();
   std::vector<std::_tstring> vKeys;
   pLib->KeyList(vKeys);
   for ( IndexType entryIdx = 0; entryIdx < nEntries; entryIdx++ )
   {
      const HaulTruckLibraryEntry* pEntry = (const HaulTruckLibraryEntry*)pLib->GetEntry(vKeys[entryIdx].c_str());
      if ( pOldHaulTruck->IsEqual(kTheta,pEntry) )
      {
         return pEntry;
      }
   }

   return nullptr;
}

void CProjectAgentImp::UpgradeBearingData()
{
   if (m_BridgeDescription.GetBearingType() == pgsTypes::brtPier) // this is default setting
   {
      CPierData2* pPier = m_BridgeDescription.GetPier(0);

      bool postedMsg(false);
      while (pPier != nullptr && !postedMsg)
      {
         for (Uint32 i = 0; i < 2; i++)
         {
            pgsTypes::PierFaceType face = (pgsTypes::PierFaceType)i;

            const CSpanData2* pSpan = GetSpan(face); // only 
            if (pSpan)
            {
               const CBearingData2* pbd = pPier->GetBearingData(0, face);
               if (pbd->bNeedsDefaults)
               {
                  // Bearing data was not loaded at start up. Need to tell user that data should be initialized
                  CString strMsg(_T("New Bearing Data at Piers was added in this version of the program. The data was not available when this file was last saved. You may want to investigate the bearing information and make sure it is appropriate for this project."));
                  pgsBridgeDescriptionStatusItem* pStatusItem = 
                     new pgsBridgeDescriptionStatusItem(m_StatusGroupID,m_scidBridgeDescriptionWarning,pgsBridgeDescriptionStatusItem::Bearings,strMsg);
                  GET_IFACE(IEAFStatusCenter,pStatusCenter);
                  pStatusCenter->Add(pStatusItem);

                  postedMsg = true;
                  break;
               }
            }
         }

         CSpanData2* pSpan = pPier->GetNextSpan();
         if (pSpan)
         {
            pPier = pSpan->GetNextPier();
         }
         else
         {
            pPier = nullptr;
         }
      }
   }
}
