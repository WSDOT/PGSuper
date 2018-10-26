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

// ProjectAgentImp.cpp : Implementation of CProjectAgentImp
#include "stdafx.h"
#include "ProjectAgent.h"
#include "ProjectAgent_i.h"
#include "ProjectAgentImp.h"
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
#include <psgLib\ProjectLibraryManager.h>

#include <Lrfd\StrandPool.h>
#include <BridgeModeling\GirderProfile.h>

#include <IFace\PrestressForce.h>
#include <IFace\StatusCenter.h>
#include <IFace\UpdateTemplates.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Bridge.h>
#include <IFace\Transactions.h>
#include <EAF\EAFDisplayUnits.h>

// tranactions executed by this agent
#include "txnEditBridgeDescription.h"

#include <EAF\EAFAutoProgress.h>
#include <PgsExt\StatusItem.h>
#include <PgsExt\GirderLabel.h>

#include <checks.h>
#include <comdef.h>

#include "PGSuperCatCom.h"
#include "XSectionData.h"

#include "DesignConfigUtil.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CollectionIndexType IndexFromLimitState(pgsTypes::LimitState ls)
{
   CollectionIndexType index = (int)ls - (int)pgsTypes::StrengthI_Inventory;
   return index;
}

// set a tolerance for IsEqual's in this class. Main reason for having the 
// IsEqual's is to prevent errant data changes from conversion round-off in dialogs.
static const Float64 DLG_TOLER = 1.0e-06;
static const Float64 STN_TOLER = 1.0e-07;

template <class EntryType, class LibType>
void use_library_entry( pgsLibraryEntryObserver* pObserver, const std::_tstring& key, EntryType** ppEntry, LibType& prjLib)
{
   if ( key.empty() )
   {
      *ppEntry = NULL;
      return;
   }

   // First check and see if it is already in the project library
   const EntryType* pProjectEntry = prjLib.LookupEntry( key.c_str() );
   PRECONDITION(pProjectEntry !=0);

   *ppEntry = pProjectEntry; // Reference count has been added.
   (*ppEntry)->Attach( pObserver );
   return;
}

template <class EntryType,class LibType>
void release_library_entry( pgsLibraryEntryObserver* pObserver, EntryType* pEntry, LibType& prjLib)
{
   if ( pEntry == NULL )
      return;

   // Release from the project library
   pEntry->Detach( pObserver );

   // Release it because we are no longer using it for input data
   pEntry->Release();
   pEntry = 0;
}

// pre-declare some needed functions
static bool IsValidStraightStrandFill(const DirectStrandFillCollection* pFill, const GirderLibraryEntry* pGirderEntry);
static bool IsValidHarpedStrandFill(const DirectStrandFillCollection* pFill, const GirderLibraryEntry* pGirderEntry);
static bool IsValidTemporaryStrandFill(const DirectStrandFillCollection* pFill, const GirderLibraryEntry* pGirderEntry);


/////////////////////////////////////////////////////////////////////////////
// CProjectAgentImp
CProjectAgentImp::CProjectAgentImp()
{
   m_pBroker = 0;
   m_pLibMgr = 0;

   m_BridgeName = _T("");
   m_BridgeId   = _T("");
   m_JobNumber  = _T("");
   m_Engineer   = _T("");
   m_Company    = _T("");
   m_Comments   = _T("");
   m_bPropertyUpdatesEnabled = true;
   m_bPropertyUpdatesPending = false;

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
   m_bHoldingEvents = false;

   // Default live loads is HL-93 for design, nothing for permit
   LiveLoadSelection selection;
   selection.EntryName = _T("HL-93");
   m_SelectedLiveLoads[pgsTypes::lltDesign].push_back(selection);

   selection.EntryName = _T("Fatigue");
   m_SelectedLiveLoads[pgsTypes::lltFatigue].push_back(selection);

   selection.EntryName = _T("AASHTO Legal Loads");
   m_SelectedLiveLoads[pgsTypes::lltLegalRating_Routine].push_back(selection);

   selection.EntryName = _T("Notional Rating Load (NRL)");
   m_SelectedLiveLoads[pgsTypes::lltLegalRating_Special].push_back(selection);

   // Default impact factors
   m_TruckImpact[pgsTypes::lltDesign]  = 0.33;
   m_TruckImpact[pgsTypes::lltPermit]  = 0.00;
   m_TruckImpact[pgsTypes::lltFatigue] = 0.15;
   m_TruckImpact[pgsTypes::lltPedestrian] = -1000.0; // obvious bogus value
   m_TruckImpact[pgsTypes::lltLegalRating_Routine]  = 0.10; // assume new, smooth riding surface
   m_TruckImpact[pgsTypes::lltLegalRating_Special]  = 0.10;
   m_TruckImpact[pgsTypes::lltPermitRating_Routine] = 0.10;
   m_TruckImpact[pgsTypes::lltPermitRating_Special] = 0.10;

   m_LaneImpact[pgsTypes::lltDesign]  = 0.00;
   m_LaneImpact[pgsTypes::lltPermit]  = 0.00;
   m_LaneImpact[pgsTypes::lltFatigue] = 0.00;
   m_LaneImpact[pgsTypes::lltPedestrian] = -1000.0; // obvious bogus value
   m_LaneImpact[pgsTypes::lltLegalRating_Routine]  = 0.00;
   m_LaneImpact[pgsTypes::lltLegalRating_Special]  = 0.00;
   m_LaneImpact[pgsTypes::lltPermitRating_Routine]  = 0.00;
   m_LaneImpact[pgsTypes::lltPermitRating_Special]  = 0.00;

   // Default for versions of PGSuper before this was an option was to sum pedestrian with vehicular
   m_PedestrianLoadApplicationType[pgsTypes::lltDesign] = ILiveLoads::PedConcurrentWithVehiculuar;
   m_PedestrianLoadApplicationType[pgsTypes::lltPermit] = ILiveLoads::PedConcurrentWithVehiculuar;
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


   m_gDC[IndexFromLimitState(pgsTypes::StrengthI_Inventory)] = 1.25;
   m_gDW[IndexFromLimitState(pgsTypes::StrengthI_Inventory)] = 1.50;
   m_gLL[IndexFromLimitState(pgsTypes::StrengthI_Inventory)] = -1;

   m_gDC[IndexFromLimitState(pgsTypes::StrengthI_Operating)] = 1.25;
   m_gDW[IndexFromLimitState(pgsTypes::StrengthI_Operating)] = 1.50;
   m_gLL[IndexFromLimitState(pgsTypes::StrengthI_Operating)] = -1;

   m_gDC[IndexFromLimitState(pgsTypes::ServiceIII_Inventory)] = 1.00;
   m_gDW[IndexFromLimitState(pgsTypes::ServiceIII_Inventory)] = 1.00;
   m_gLL[IndexFromLimitState(pgsTypes::ServiceIII_Inventory)] = -1;

   m_gDC[IndexFromLimitState(pgsTypes::ServiceIII_Operating)] = 1.00;
   m_gDW[IndexFromLimitState(pgsTypes::ServiceIII_Operating)] = 1.00;
   m_gLL[IndexFromLimitState(pgsTypes::ServiceIII_Operating)] = -1;

   m_gDC[IndexFromLimitState(pgsTypes::StrengthI_LegalRoutine)] = 1.25;
   m_gDW[IndexFromLimitState(pgsTypes::StrengthI_LegalRoutine)] = 1.50;
   m_gLL[IndexFromLimitState(pgsTypes::StrengthI_LegalRoutine)] = -1;

   m_gDC[IndexFromLimitState(pgsTypes::ServiceIII_LegalRoutine)] = 1.00;
   m_gDW[IndexFromLimitState(pgsTypes::ServiceIII_LegalRoutine)] = 1.00;
   m_gLL[IndexFromLimitState(pgsTypes::ServiceIII_LegalRoutine)] = 1.00;

   m_gDC[IndexFromLimitState(pgsTypes::StrengthI_LegalSpecial)] = 1.25;
   m_gDW[IndexFromLimitState(pgsTypes::StrengthI_LegalSpecial)] = 1.50;
   m_gLL[IndexFromLimitState(pgsTypes::StrengthI_LegalSpecial)] = -1;

   m_gDC[IndexFromLimitState(pgsTypes::ServiceIII_LegalSpecial)] = 1.00;
   m_gDW[IndexFromLimitState(pgsTypes::ServiceIII_LegalSpecial)] = 1.00;
   m_gLL[IndexFromLimitState(pgsTypes::ServiceIII_LegalSpecial)] = -1;

   m_gDC[IndexFromLimitState(pgsTypes::StrengthII_PermitRoutine)] = 1.25;
   m_gDW[IndexFromLimitState(pgsTypes::StrengthII_PermitRoutine)] = 1.50;
   m_gLL[IndexFromLimitState(pgsTypes::StrengthII_PermitRoutine)] = -1;

   m_gDC[IndexFromLimitState(pgsTypes::ServiceI_PermitRoutine)] = 1.00;
   m_gDW[IndexFromLimitState(pgsTypes::ServiceI_PermitRoutine)] = 1.00;
   m_gLL[IndexFromLimitState(pgsTypes::ServiceI_PermitRoutine)] = -1;

   m_gDC[IndexFromLimitState(pgsTypes::StrengthII_PermitSpecial)] = 1.25;
   m_gDW[IndexFromLimitState(pgsTypes::StrengthII_PermitSpecial)] = 1.50;
   m_gLL[IndexFromLimitState(pgsTypes::StrengthII_PermitSpecial)] = -1;

   m_gDC[IndexFromLimitState(pgsTypes::ServiceI_PermitSpecial)] = 1.00;
   m_gDW[IndexFromLimitState(pgsTypes::ServiceI_PermitSpecial)] = 1.00;
   m_gLL[IndexFromLimitState(pgsTypes::ServiceI_PermitSpecial)] = -1;

   m_AllowableTensionCoefficient[pgsTypes::lrDesign_Inventory] = ::ConvertToSysUnits(0.19,unitMeasure::SqrtKSI);
   m_AllowableTensionCoefficient[pgsTypes::lrDesign_Operating] = ::ConvertToSysUnits(0.19,unitMeasure::SqrtKSI);
   m_AllowableTensionCoefficient[pgsTypes::lrLegal_Routine]    = ::ConvertToSysUnits(0.19,unitMeasure::SqrtKSI);
   m_AllowableTensionCoefficient[pgsTypes::lrLegal_Special]    = ::ConvertToSysUnits(0.19,unitMeasure::SqrtKSI);
   m_AllowableTensionCoefficient[pgsTypes::lrPermit_Routine]   = ::ConvertToSysUnits(0.19,unitMeasure::SqrtKSI);
   m_AllowableTensionCoefficient[pgsTypes::lrPermit_Special]   = ::ConvertToSysUnits(0.19,unitMeasure::SqrtKSI);

   m_bRateForStress[pgsTypes::lrDesign_Inventory] = true;
   m_bRateForStress[pgsTypes::lrDesign_Operating] = false; // see MBE C6A.5.4.1
   m_bRateForStress[pgsTypes::lrLegal_Routine]    = true;
   m_bRateForStress[pgsTypes::lrLegal_Special]    = true;
   m_bRateForStress[pgsTypes::lrPermit_Routine]   = true;
   m_bRateForStress[pgsTypes::lrPermit_Special]   = true;

   m_bRateForShear[pgsTypes::lrDesign_Inventory] = true;
   m_bRateForShear[pgsTypes::lrDesign_Operating] = true;
   m_bRateForShear[pgsTypes::lrLegal_Routine]    = true;
   m_bRateForShear[pgsTypes::lrLegal_Special]    = true;
   m_bRateForShear[pgsTypes::lrPermit_Routine]   = true;
   m_bRateForShear[pgsTypes::lrPermit_Special]   = true;

   m_bEnableRating[pgsTypes::lrDesign_Inventory] = true;
   m_bEnableRating[pgsTypes::lrDesign_Operating] = true;
   m_bEnableRating[pgsTypes::lrLegal_Routine]    = true;
   m_bEnableRating[pgsTypes::lrLegal_Special]    = true;
   m_bEnableRating[pgsTypes::lrPermit_Routine]   = false;
   m_bEnableRating[pgsTypes::lrPermit_Special]   = false;

   m_ReservedLiveLoads.reserve(5);
   m_ReservedLiveLoads.push_back(_T("HL-93"));
   m_ReservedLiveLoads.push_back(_T("Fatigue"));
   m_ReservedLiveLoads.push_back(_T("AASHTO Legal Loads"));
   m_ReservedLiveLoads.push_back(_T("Notional Rating Load (NRL)"));
   m_ReservedLiveLoads.push_back(_T("Single-Unit SHVs"));

   m_ConstructionLoad = 0;

   m_bIgnoreEffectiveFlangeWidthLimits = false;
}

CProjectAgentImp::~CProjectAgentImp()
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
         return hr;

      hr = pSave->put_Property(_T("AnalysisType"),CComVariant(pObj->m_AnalysisType));
      if ( FAILED(hr) )
         return hr;

      pSave->EndUnit();
   }
   else
   {
      pLoad->BeginUnit(_T("Specification"));

      double version;

      pLoad->get_Version(&version);

      CComVariant var;
      var.vt = VT_BSTR;
      hr = pLoad->get_Property(_T("Spec"),&var);
      if ( FAILED(hr) )
         return hr;

      pObj->m_Spec = OLE2T(var.bstrVal);

      if ( 1 < version )
      {
         var.vt = VT_I4;
         hr = pLoad->get_Property(_T("AnalysisType"),&var);
         if ( FAILED(hr) )
            return hr;

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
         return hr;

      hr = pSave->put_Property(_T("IgnoreLimits"),CComVariant(pObj->m_bIgnoreEffectiveFlangeWidthLimits ? VARIANT_TRUE : VARIANT_FALSE));
      if ( FAILED(hr) )
         return hr;

      hr = pSave->EndUnit();
      if ( FAILED(hr) )
         return hr;
   }
   else
   {
      Float64 parent_version;
      pLoad->get_Version(&parent_version);
      if ( 3.0 <= parent_version )
      {
         hr = pLoad->BeginUnit(_T("EffectiveFlangeWidth"));
         if ( FAILED(hr) )
            return hr;

         CComVariant var;
         var.vt = VT_BOOL;
         hr = pLoad->get_Property(_T("IgnoreLimits"),&var);
         if ( FAILED(hr) )
            return hr;

         pObj->m_bIgnoreEffectiveFlangeWidthLimits = (var.boolVal == VARIANT_TRUE ? true : false);

         hr = pLoad->EndUnit();
         if ( FAILED(hr) )
            return hr;

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
      pSave->BeginUnit(_T("RatingSpecification"),1.0);
      hr = pSave->put_Property(_T("Name"),CComVariant(pObj->m_RatingSpec.c_str()));
      if ( FAILED(hr) )
         return hr;

      pSave->put_Property(_T("SystemFactorFlexure"), CComVariant(pObj->m_SystemFactorFlexure));
      pSave->put_Property(_T("SystemFactorShear"),   CComVariant(pObj->m_SystemFactorShear));
      pSave->put_Property(_T("ADTT"),CComVariant(pObj->m_ADTT));
      pSave->put_Property(_T("IncludePedestrianLiveLoad"),CComVariant(pObj->m_bIncludePedestrianLiveLoad));

      pSave->BeginUnit(_T("DesignInventoryRating"),1.0);
      pSave->put_Property(_T("Enabled"),CComVariant(pObj->m_bEnableRating[pgsTypes::lrDesign_Inventory]));
      pSave->put_Property(_T("DC_StrengthI"),CComVariant(pObj->m_gDC[pgsTypes::StrengthI_Inventory]));
      pSave->put_Property(_T("DW_StrengthI"),CComVariant(pObj->m_gDW[pgsTypes::StrengthI_Inventory]));
      pSave->put_Property(_T("LL_StrengthI"),CComVariant(pObj->m_gLL[pgsTypes::StrengthI_Inventory]));
      pSave->put_Property(_T("DC_ServiceIII"),CComVariant(pObj->m_gDC[pgsTypes::ServiceIII_Inventory]));
      pSave->put_Property(_T("DW_ServiceIII"),CComVariant(pObj->m_gDW[pgsTypes::ServiceIII_Inventory]));
      pSave->put_Property(_T("LL_ServiceIII"),CComVariant(pObj->m_gLL[pgsTypes::ServiceIII_Inventory]));
      pSave->put_Property(_T("AllowableTensionCoefficient"),CComVariant(pObj->m_AllowableTensionCoefficient[pgsTypes::lrDesign_Inventory]));
      pSave->put_Property(_T("RateForShear"),CComVariant(pObj->m_bRateForShear[pgsTypes::lrDesign_Inventory]));
      pSave->EndUnit(); // DesignInventoryRating

      pSave->BeginUnit(_T("DesignOperatingRating"),1.0);
      pSave->put_Property(_T("Enabled"),CComVariant(pObj->m_bEnableRating[pgsTypes::lrDesign_Operating]));
      pSave->put_Property(_T("DC_StrengthI"),CComVariant(pObj->m_gDC[pgsTypes::StrengthI_Operating]));
      pSave->put_Property(_T("DW_StrengthI"),CComVariant(pObj->m_gDW[pgsTypes::StrengthI_Operating]));
      pSave->put_Property(_T("LL_StrengthI"),CComVariant(pObj->m_gLL[pgsTypes::StrengthI_Operating]));
      pSave->put_Property(_T("DC_ServiceIII"),CComVariant(pObj->m_gDC[pgsTypes::ServiceIII_Operating]));
      pSave->put_Property(_T("DW_ServiceIII"),CComVariant(pObj->m_gDW[pgsTypes::ServiceIII_Operating]));
      pSave->put_Property(_T("LL_ServiceIII"),CComVariant(pObj->m_gLL[pgsTypes::ServiceIII_Operating]));
      pSave->put_Property(_T("AllowableTensionCoefficient"),CComVariant(pObj->m_AllowableTensionCoefficient[pgsTypes::lrDesign_Operating]));
      pSave->put_Property(_T("RateForShear"),CComVariant(pObj->m_bRateForShear[pgsTypes::lrDesign_Operating]));
      pSave->EndUnit(); // DesignOperatingRating

      pSave->BeginUnit(_T("LegalRoutineRating"),1.0);
      pSave->put_Property(_T("Enabled"),CComVariant(pObj->m_bEnableRating[pgsTypes::lrLegal_Routine]));
      pSave->put_Property(_T("DC_StrengthI"),CComVariant(pObj->m_gDC[pgsTypes::StrengthI_LegalRoutine]));
      pSave->put_Property(_T("DW_StrengthI"),CComVariant(pObj->m_gDW[pgsTypes::StrengthI_LegalRoutine]));
      pSave->put_Property(_T("LL_StrengthI"),CComVariant(pObj->m_gLL[pgsTypes::StrengthI_LegalRoutine]));
      pSave->put_Property(_T("DC_ServiceIII"),CComVariant(pObj->m_gDC[pgsTypes::ServiceIII_LegalRoutine]));
      pSave->put_Property(_T("DW_ServiceIII"),CComVariant(pObj->m_gDW[pgsTypes::ServiceIII_LegalRoutine]));
      pSave->put_Property(_T("LL_ServiceIII"),CComVariant(pObj->m_gLL[pgsTypes::ServiceIII_LegalRoutine]));
      pSave->put_Property(_T("AllowableTensionCoefficient"),CComVariant(pObj->m_AllowableTensionCoefficient[pgsTypes::lrLegal_Routine]));
      pSave->put_Property(_T("RateForShear"),CComVariant(pObj->m_bRateForShear[pgsTypes::lrLegal_Routine]));
      pSave->put_Property(_T("RateForStress"),CComVariant(pObj->m_bRateForStress[pgsTypes::lrLegal_Routine]));
      pSave->put_Property(_T("ExcludeLegalLoadLaneLoading"),CComVariant(pObj->m_bExcludeLegalLoadLaneLoading));
      pSave->EndUnit(); // LegalRoutineRating

      pSave->BeginUnit(_T("LegalSpecialRating"),1.0);
      pSave->put_Property(_T("Enabled"),CComVariant(pObj->m_bEnableRating[pgsTypes::lrLegal_Special]));
      pSave->put_Property(_T("DC_StrengthI"),CComVariant(pObj->m_gDC[pgsTypes::StrengthI_LegalSpecial]));
      pSave->put_Property(_T("DW_StrengthI"),CComVariant(pObj->m_gDW[pgsTypes::StrengthI_LegalSpecial]));
      pSave->put_Property(_T("LL_StrengthI"),CComVariant(pObj->m_gLL[pgsTypes::StrengthI_LegalSpecial]));
      pSave->put_Property(_T("DC_ServiceIII"),CComVariant(pObj->m_gDC[pgsTypes::ServiceIII_LegalSpecial]));
      pSave->put_Property(_T("DW_ServiceIII"),CComVariant(pObj->m_gDW[pgsTypes::ServiceIII_LegalSpecial]));
      pSave->put_Property(_T("LL_ServiceIII"),CComVariant(pObj->m_gLL[pgsTypes::ServiceIII_LegalSpecial]));
      pSave->put_Property(_T("AllowableTensionCoefficient"),CComVariant(pObj->m_AllowableTensionCoefficient[pgsTypes::lrLegal_Special]));
      pSave->put_Property(_T("RateForShear"),CComVariant(pObj->m_bRateForShear[pgsTypes::lrLegal_Special]));
      pSave->put_Property(_T("RateForStress"),CComVariant(pObj->m_bRateForStress[pgsTypes::lrLegal_Special]));
      pSave->EndUnit(); // LegalSpecialRating

      pSave->BeginUnit(_T("PermitRoutineRating"),1.0);
      pSave->put_Property(_T("Enabled"),CComVariant(pObj->m_bEnableRating[pgsTypes::lrPermit_Routine]));
      pSave->put_Property(_T("DC_StrengthII"),CComVariant(pObj->m_gDC[pgsTypes::StrengthII_PermitRoutine]));
      pSave->put_Property(_T("DW_StrengthII"),CComVariant(pObj->m_gDW[pgsTypes::StrengthII_PermitRoutine]));
      pSave->put_Property(_T("LL_StrengthII"),CComVariant(pObj->m_gLL[pgsTypes::StrengthII_PermitRoutine]));
      pSave->put_Property(_T("DC_ServiceI"),CComVariant(pObj->m_gDC[pgsTypes::ServiceI_PermitRoutine]));
      pSave->put_Property(_T("DW_ServiceI"),CComVariant(pObj->m_gDW[pgsTypes::ServiceI_PermitRoutine]));
      pSave->put_Property(_T("LL_ServiceI"),CComVariant(pObj->m_gLL[pgsTypes::ServiceI_PermitRoutine]));
      pSave->put_Property(_T("AllowableYieldStressCoefficient"),CComVariant(pObj->m_AllowableYieldStressCoefficient));
      pSave->put_Property(_T("RateForShear"),CComVariant(pObj->m_bRateForShear[pgsTypes::lrPermit_Routine]));
      pSave->put_Property(_T("RateForStress"),CComVariant(pObj->m_bRateForStress[pgsTypes::lrPermit_Routine]));
      pSave->EndUnit(); // PermitRoutineRating

      pSave->BeginUnit(_T("PermitSpecialRating"),1.0);
      pSave->put_Property(_T("Enabled"),CComVariant(pObj->m_bEnableRating[pgsTypes::lrPermit_Special]));
      pSave->put_Property(_T("DC_StrengthII"),CComVariant(pObj->m_gDC[pgsTypes::StrengthII_PermitSpecial]));
      pSave->put_Property(_T("DW_StrengthII"),CComVariant(pObj->m_gDW[pgsTypes::StrengthII_PermitSpecial]));
      pSave->put_Property(_T("LL_StrengthII"),CComVariant(pObj->m_gLL[pgsTypes::StrengthII_PermitSpecial]));
      pSave->put_Property(_T("DC_ServiceI"),CComVariant(pObj->m_gDC[pgsTypes::ServiceI_PermitSpecial]));
      pSave->put_Property(_T("DW_ServiceI"),CComVariant(pObj->m_gDW[pgsTypes::ServiceI_PermitSpecial]));
      pSave->put_Property(_T("LL_ServiceI"),CComVariant(pObj->m_gLL[pgsTypes::ServiceI_PermitSpecial]));
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
         double version;

         pLoad->get_Version(&version);

         CComVariant var;
         var.vt = VT_BSTR;
         hr = pLoad->get_Property(_T("Name"),&var);
         if ( FAILED(hr) )
            return hr;

         pObj->m_RatingSpec = OLE2T(var.bstrVal);

         var.vt = VT_R8;
         hr = pLoad->get_Property(_T("SystemFactorFlexure"),&var);
         if ( FAILED(hr) )
            return hr;

         pObj->m_SystemFactorFlexure = var.dblVal;

         var.vt = VT_R8;
         hr = pLoad->get_Property(_T("SystemFactorShear"),&var);
         if ( FAILED(hr) )
            return hr;

         pObj->m_SystemFactorShear = var.dblVal;

         var.vt = VT_I4;
         hr = pLoad->get_Property(_T("ADTT"),&var);
         if ( FAILED(hr) )
            return hr;

         pObj->m_ADTT = var.iVal;

         var.vt = VT_BOOL;
         hr = pLoad->get_Property(_T("IncludePedestrianLiveLoad"),&var);
         if ( FAILED(hr) )
            return hr;

         pObj->m_bIncludePedestrianLiveLoad = (var.boolVal == VARIANT_TRUE ? true : false);

         //////////////////////////////////////////////////////
         pLoad->BeginUnit(_T("DesignInventoryRating"));
         var.vt = VT_BOOL;
         pLoad->get_Property(_T("Enabled"),&var);
         pObj->m_bEnableRating[pgsTypes::lrDesign_Inventory] = (var.boolVal == VARIANT_TRUE ? true : false);

         var.vt = VT_R8;
         pLoad->get_Property(_T("DC_StrengthI"),&var);
         pObj->m_gDC[pgsTypes::StrengthI_Inventory] = var.dblVal;

         pLoad->get_Property(_T("DW_StrengthI"),&var);
         pObj->m_gDW[pgsTypes::StrengthI_Inventory] = var.dblVal;

         pLoad->get_Property(_T("LL_StrengthI"),&var);
         pObj->m_gLL[pgsTypes::StrengthI_Inventory] = var.dblVal;

         pLoad->get_Property(_T("DC_ServiceIII"),&var);
         pObj->m_gDC[pgsTypes::ServiceIII_Inventory] = var.dblVal;

         pLoad->get_Property(_T("DW_ServiceIII"),&var);
         pObj->m_gDW[pgsTypes::ServiceIII_Inventory] = var.dblVal;

         pLoad->get_Property(_T("LL_ServiceIII"),&var);
         pObj->m_gLL[pgsTypes::ServiceIII_Inventory] = var.dblVal;

         pLoad->get_Property(_T("AllowableTensionCoefficient"),&var);
         pObj->m_AllowableTensionCoefficient[pgsTypes::lrDesign_Inventory] = var.dblVal;

         var.vt = VT_BOOL;
         pLoad->get_Property(_T("RateForShear"),&var);
         pObj->m_bRateForShear[pgsTypes::lrDesign_Inventory] = (var.boolVal == VARIANT_TRUE ? true : false);

         pLoad->EndUnit(); // DesignInventoryRating
         //////////////////////////////////////////////////////

         //////////////////////////////////////////////////////
         pLoad->BeginUnit(_T("DesignOperatingRating"));
         var.vt = VT_BOOL;
         pLoad->get_Property(_T("Enabled"),&var);
         pObj->m_bEnableRating[pgsTypes::lrDesign_Operating] = (var.boolVal == VARIANT_TRUE ? true : false);

         var.vt = VT_R8;
         pLoad->get_Property(_T("DC_StrengthI"),&var);
         pObj->m_gDC[pgsTypes::StrengthI_Operating] = var.dblVal;

         pLoad->get_Property(_T("DW_StrengthI"),&var);
         pObj->m_gDW[pgsTypes::StrengthI_Operating] = var.dblVal;

         pLoad->get_Property(_T("LL_StrengthI"),&var);
         pObj->m_gLL[pgsTypes::StrengthI_Operating] = var.dblVal;

         pLoad->get_Property(_T("DC_ServiceIII"),&var);
         pObj->m_gDC[pgsTypes::ServiceIII_Operating] = var.dblVal;

         pLoad->get_Property(_T("DW_ServiceIII"),&var);
         pObj->m_gDW[pgsTypes::ServiceIII_Operating] = var.dblVal;

         pLoad->get_Property(_T("LL_ServiceIII"),&var);
         pObj->m_gLL[pgsTypes::ServiceIII_Operating] = var.dblVal;

         pLoad->get_Property(_T("AllowableTensionCoefficient"),&var);
         pObj->m_AllowableTensionCoefficient[pgsTypes::lrDesign_Operating] = var.dblVal;

         var.vt = VT_BOOL;
         pLoad->get_Property(_T("RateForShear"),&var);
         pObj->m_bRateForShear[pgsTypes::lrDesign_Operating] = (var.boolVal == VARIANT_TRUE ? true : false);

         pLoad->EndUnit(); // DesignOperatingRating
         //////////////////////////////////////////////////////

         //////////////////////////////////////////////////////
         pLoad->BeginUnit(_T("LegalRoutineRating"));

         var.vt = VT_BOOL;
         pLoad->get_Property(_T("Enabled"),&var);
         pObj->m_bEnableRating[pgsTypes::lrLegal_Routine] = (var.boolVal == VARIANT_TRUE ? true : false);

         var.vt = VT_R8;
         pLoad->get_Property(_T("DC_StrengthI"),&var);
         pObj->m_gDC[pgsTypes::StrengthI_LegalRoutine] = var.dblVal;

         pLoad->get_Property(_T("DW_StrengthI"),&var);
         pObj->m_gDW[pgsTypes::StrengthI_LegalRoutine] = var.dblVal;

         pLoad->get_Property(_T("LL_StrengthI"),&var);
         pObj->m_gLL[pgsTypes::StrengthI_LegalRoutine] = var.dblVal;

         pLoad->get_Property(_T("DC_ServiceIII"),&var);
         pObj->m_gDC[pgsTypes::ServiceIII_LegalRoutine] = var.dblVal;

         pLoad->get_Property(_T("DW_ServiceIII"),&var);
         pObj->m_gDW[pgsTypes::ServiceIII_LegalRoutine] = var.dblVal;

         pLoad->get_Property(_T("LL_ServiceIII"),&var);
         pObj->m_gLL[pgsTypes::ServiceIII_LegalRoutine] = var.dblVal;

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
         //////////////////////////////////////////////////////

         //////////////////////////////////////////////////////
         pLoad->BeginUnit(_T("LegalSpecialRating"));

         var.vt = VT_BOOL;
         pLoad->get_Property(_T("Enabled"),&var);
         pObj->m_bEnableRating[pgsTypes::lrLegal_Special] = (var.boolVal == VARIANT_TRUE ? true : false);

         var.vt = VT_R8;
         pLoad->get_Property(_T("DC_StrengthI"),&var);
         pObj->m_gDC[pgsTypes::StrengthI_LegalSpecial] = var.dblVal;

         pLoad->get_Property(_T("DW_StrengthI"),&var);
         pObj->m_gDW[pgsTypes::StrengthI_LegalSpecial] = var.dblVal;

         pLoad->get_Property(_T("LL_StrengthI"),&var);
         pObj->m_gLL[pgsTypes::StrengthI_LegalSpecial] = var.dblVal;

         pLoad->get_Property(_T("DC_ServiceIII"),&var);
         pObj->m_gDC[pgsTypes::ServiceIII_LegalSpecial] = var.dblVal;

         pLoad->get_Property(_T("DW_ServiceIII"),&var);
         pObj->m_gDW[pgsTypes::ServiceIII_LegalSpecial] = var.dblVal;

         pLoad->get_Property(_T("LL_ServiceIII"),&var);
         pObj->m_gLL[pgsTypes::ServiceIII_LegalSpecial] = var.dblVal;

         pLoad->get_Property(_T("AllowableTensionCoefficient"),&var);
         pObj->m_AllowableTensionCoefficient[pgsTypes::lrLegal_Special] = var.dblVal;

         var.vt = VT_BOOL;
         pLoad->get_Property(_T("RateForShear"),&var);
         pObj->m_bRateForShear[pgsTypes::lrLegal_Special] = (var.boolVal == VARIANT_TRUE ? true : false);

         pLoad->get_Property(_T("RateForStress"),&var);
         pObj->m_bRateForStress[pgsTypes::lrLegal_Special] = (var.boolVal == VARIANT_TRUE ? true : false);

         pLoad->EndUnit(); // LegalSpecialRating
         //////////////////////////////////////////////////////

         //////////////////////////////////////////////////////
         pLoad->BeginUnit(_T("PermitRoutineRating"));

         var.vt = VT_BOOL;
         pLoad->get_Property(_T("Enabled"),&var);
         pObj->m_bEnableRating[pgsTypes::lrPermit_Routine] = (var.boolVal == VARIANT_TRUE ? true : false);

         var.vt = VT_R8;
         pLoad->get_Property(_T("DC_StrengthII"),&var);
         pObj->m_gDC[pgsTypes::StrengthII_PermitRoutine] = var.dblVal;

         pLoad->get_Property(_T("DW_StrengthII"),&var);
         pObj->m_gDW[pgsTypes::StrengthII_PermitRoutine] = var.dblVal;

         pLoad->get_Property(_T("LL_StrengthII"),&var);
         pObj->m_gLL[pgsTypes::StrengthII_PermitRoutine] = var.dblVal;

         pLoad->get_Property(_T("DC_ServiceI"),&var);
         pObj->m_gDC[pgsTypes::ServiceI_PermitRoutine] = var.dblVal;

         pLoad->get_Property(_T("DW_ServiceI"),&var);
         pObj->m_gDW[pgsTypes::ServiceI_PermitRoutine] = var.dblVal;

         pLoad->get_Property(_T("LL_ServiceI"),&var);
         pObj->m_gLL[pgsTypes::ServiceI_PermitRoutine] = var.dblVal;

         pLoad->get_Property(_T("AllowableYieldStressCoefficient"),&var);
         pObj->m_AllowableYieldStressCoefficient = var.dblVal;

         var.vt = VT_BOOL;
         pLoad->get_Property(_T("RateForShear"),&var);
         pObj->m_bRateForShear[pgsTypes::lrPermit_Routine] = (var.boolVal == VARIANT_TRUE ? true : false);
         
         pLoad->get_Property(_T("RateForStress"),&var);
         pObj->m_bRateForStress[pgsTypes::lrPermit_Routine] = (var.boolVal == VARIANT_TRUE ? true : false);

         pLoad->EndUnit(); // PermitRoutineRating
         //////////////////////////////////////////////////////

         //////////////////////////////////////////////////////
         pLoad->BeginUnit(_T("PermitSpecialRating"));

         var.vt = VT_BOOL;
         pLoad->get_Property(_T("Enabled"),&var);
         pObj->m_bEnableRating[pgsTypes::lrPermit_Special] = (var.boolVal == VARIANT_TRUE ? true : false);

         var.vt = VT_R8;
         pLoad->get_Property(_T("DC_StrengthII"),&var);
         pObj->m_gDC[pgsTypes::StrengthII_PermitSpecial] = var.dblVal;

         pLoad->get_Property(_T("DW_StrengthII"),&var);
         pObj->m_gDW[pgsTypes::StrengthII_PermitSpecial] = var.dblVal;

         pLoad->get_Property(_T("LL_StrengthII"),&var);
         pObj->m_gLL[pgsTypes::StrengthII_PermitSpecial] = var.dblVal;

         pLoad->get_Property(_T("DC_ServiceI"),&var);
         pObj->m_gDC[pgsTypes::ServiceI_PermitSpecial] = var.dblVal;

         pLoad->get_Property(_T("DW_ServiceI"),&var);
         pObj->m_gDW[pgsTypes::ServiceI_PermitSpecial] = var.dblVal;

         pLoad->get_Property(_T("LL_ServiceI"),&var);
         pObj->m_gLL[pgsTypes::ServiceI_PermitSpecial] = var.dblVal;

         var.vt = VT_I4;
         pLoad->get_Property(_T("SpecialPermitType"),&var);
         pObj->m_SpecialPermitType = (pgsTypes::SpecialPermitType)var.lVal;

         var.vt = VT_BOOL;
         pLoad->get_Property(_T("RateForShear"),&var);
         pObj->m_bRateForShear[pgsTypes::lrPermit_Special] = (var.boolVal == VARIANT_TRUE ? true : false);
         
         pLoad->get_Property(_T("RateForStress"),&var);
         pObj->m_bRateForStress[pgsTypes::lrPermit_Special] = (var.boolVal == VARIANT_TRUE ? true : false);

         pLoad->EndUnit(); // PermitSpecialRating
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
         return hr;
   }
   else
   {
      eafTypes::UnitMode unitMode;
      CComVariant var;
      var.vt = VT_I4;
      hr = pLoad->get_Property(_T("Units"),&var);
      if ( FAILED(hr) )
         return hr;

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
         return hr;

// eliminated in version 4
//      hr = pSave->put_Property(_T("AlignmentOffset"),CComVariant(pObj->m_AlignmentOffset));
//      if ( FAILED(hr) )
//         return hr;
      
      // added in version 5
      hr = pSave->put_Property(_T("RefStation"),CComVariant(pObj->m_AlignmentData2.RefStation));
      if ( FAILED(hr) )
         return hr;
      
      // added in version 5
      hr = pSave->put_Property(_T("RefPointNorthing"),CComVariant(pObj->m_AlignmentData2.yRefPoint));
      if ( FAILED(hr) )
         return hr;
      
      // added in version 5
      hr = pSave->put_Property(_T("RefPointEasting"),CComVariant(pObj->m_AlignmentData2.xRefPoint));
      if ( FAILED(hr) )
         return hr;


      hr = pSave->put_Property(_T("Direction"),CComVariant(pObj->m_AlignmentData2.Direction));
      if ( FAILED(hr) )
         return hr;

      hr = pSave->put_Property(_T("HorzCurveCount"),CComVariant((long)pObj->m_AlignmentData2.HorzCurves.size()));
      if ( FAILED(hr) )
         return hr;

      std::vector<HorzCurveData>::iterator iter;
      for ( iter = pObj->m_AlignmentData2.HorzCurves.begin(); iter != pObj->m_AlignmentData2.HorzCurves.end(); iter++ )
      {
         HorzCurveData& hc = *iter;

         hr = pSave->BeginUnit(_T("HorzCurveData"),2.0);
         if ( FAILED(hr) )
            return hr;

         hr = pSave->put_Property(_T("PIStation"),CComVariant(hc.PIStation));
         if ( FAILED(hr) )
            return hr;

         hr = pSave->put_Property(_T("FwdTangent"),CComVariant(hc.FwdTangent));
         if ( FAILED(hr) )
            return hr;

         hr = pSave->put_Property(_T("Radius"),CComVariant(hc.Radius));
         if ( FAILED(hr) )
            return hr;

         hr = pSave->put_Property(_T("EntrySpiral"),CComVariant(hc.EntrySpiral));
         if ( FAILED(hr) )
            return hr;

         hr = pSave->put_Property(_T("ExitSpiral"),CComVariant(hc.ExitSpiral));
         if ( FAILED(hr) )
            return hr;

         hr = pSave->put_Property(_T("FwdTangentIsBearing"),CComVariant(hc.bFwdTangent));
         if ( FAILED(hr) )
            return hr;

         hr = pSave->EndUnit();
         if ( FAILED(hr) )
            return hr;
      }

      hr = pSave->EndUnit();
      if ( FAILED(hr) )
         return hr;
   }
   else
   {
      CComVariant var;

      bool bConvert = false;

      // If BeginUnit fails then this file was created before the unit was added
      // If this is the case, the Direction and Tangents need to be converted
      hr = pLoad->BeginUnit(_T("AlignmentData"));
      if ( FAILED(hr) )
         bConvert = true; 

      double version;
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
            return hr;

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
            return hr;
         Int16 method = var.iVal;

         var.vt = VT_R8;
         hr = pLoad->get_Property(_T("AlignmentDirection"),&var);
         if ( FAILED(hr) )
            return hr;
         double Direction = var.dblVal;

         var.vt = VT_R8;
         hr = pLoad->get_Property(_T("FwdTangent"),&var);
         if ( FAILED(hr) )
            return hr;
         double FwdTangent = var.dblVal;

         var.vt = VT_R8;
         hr = pLoad->get_Property(_T("BkTangent"),&var);
         if ( FAILED(hr) )
            return hr;
         double BkTangent = var.dblVal;

         var.vt = VT_R8;
         hr = pLoad->get_Property(_T("Radius"),&var);
         if ( FAILED(hr) )
            return hr;
         double Radius = var.dblVal;

         var.vt = VT_R8;
         hr = pLoad->get_Property(_T("PIStation"),&var);
         if ( FAILED(hr) )
            return hr;
         double PIStation = var.dblVal;

         if ( !bConvert )
         {
            // Don't need to convert, so don't need to call EndUnit
            hr = pLoad->EndUnit();
            if ( FAILED(hr) )
               return hr;
         }

         if ( bConvert )
         {
            // Need to do a coordinate transformation for the directions
            // Before 1.1... 0.0 was due north and increased clockwise
            // 1.1 and later... 0.0 is due east and increases counter-clockwise

            Direction  = PI_OVER_2 - Direction;
            if ( Direction < 0 )
               Direction += 2*M_PI;

            FwdTangent = PI_OVER_2 - FwdTangent;
            if (FwdTangent < 0)
               FwdTangent += 2*M_PI;

            BkTangent  = PI_OVER_2 - BkTangent;
            if ( BkTangent < 0 )
               BkTangent += 2*M_PI;
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
               return hr;
            pObj->m_AlignmentData2.RefStation = var.dblVal;

            var.vt = VT_R8;
            hr = pLoad->get_Property(_T("RefPointNorthing"),&var);
            if ( FAILED(hr) )
               return hr;
            pObj->m_AlignmentData2.yRefPoint = var.dblVal;

            var.vt = VT_R8;
            hr = pLoad->get_Property(_T("RefPointEasting"),&var);
            if ( FAILED(hr) )
               return hr;
            pObj->m_AlignmentData2.xRefPoint = var.dblVal;
         }

         var.vt = VT_R8;
         hr = pLoad->get_Property(_T("Direction"),&var);
         if ( FAILED(hr) )
            return hr;
         pObj->m_AlignmentData2.Direction = var.dblVal;

         var.vt = VT_I4;
         hr = pLoad->get_Property(_T("HorzCurveCount"),&var);
         if ( FAILED(hr) )
            return hr;
         int nCurves = var.iVal;

         for ( int c = 0; c < nCurves; c++ )
         {
            HorzCurveData hc;

            hr = pLoad->BeginUnit(_T("HorzCurveData"));
            if ( FAILED(hr) )
               return hr;

            double hc_version;
            pLoad->get_Version(&hc_version);

            var.vt = VT_R8;
            hr = pLoad->get_Property(_T("PIStation"),&var);
            if ( FAILED(hr) )
               return hr;
            hc.PIStation = var.dblVal;

            var.vt = VT_R8;
            hr = pLoad->get_Property(_T("FwdTangent"),&var);
            if ( FAILED(hr) )
               return hr;
            hc.FwdTangent = var.dblVal;

            var.vt = VT_R8;
            hr = pLoad->get_Property(_T("Radius"),&var);
            if ( FAILED(hr) )
               return hr;
            hc.Radius = var.dblVal;

            var.vt = VT_R8;
            hr = pLoad->get_Property(_T("EntrySpiral"),&var);
            if ( FAILED(hr) )
               return hr;
            hc.EntrySpiral = var.dblVal;

            var.vt = VT_R8;
            hr = pLoad->get_Property(_T("ExitSpiral"),&var);
            if ( FAILED(hr) )
               return hr;
            hc.ExitSpiral = var.dblVal;

            if ( 2.0 <= hc_version )
            {
               var.vt = VT_BOOL;
               hr = pLoad->get_Property(_T("FwdTangentIsBearing"),&var);
               if ( FAILED(hr) )
                  return hr;
               hc.bFwdTangent = (var.boolVal == VARIANT_TRUE);
            }
            else
            {
               hc.bFwdTangent = true;
            }


            pObj->m_AlignmentData2.HorzCurves.push_back(hc);

            hr = pLoad->EndUnit();
            if ( FAILED(hr) )
               return hr;
         }

         hr = pLoad->EndUnit();
         if ( FAILED(hr) )
            return hr;
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
         return hr;

      hr = pSave->put_Property(_T("Station"),CComVariant(pObj->m_ProfileData2.Station));
      if ( FAILED(hr) )
         return hr;

      hr = pSave->put_Property(_T("Elevation"),CComVariant(pObj->m_ProfileData2.Elevation));
      if ( FAILED(hr) )
         return hr;

      hr = pSave->put_Property(_T("Grade"),CComVariant(pObj->m_ProfileData2.Grade));
      if ( FAILED(hr) )
         return hr;

      hr = pSave->put_Property(_T("VertCurveCount"),CComVariant((long)pObj->m_ProfileData2.VertCurves.size()));
      if ( FAILED(hr) )
         return hr;

      std::vector<VertCurveData>::iterator iter;
      for ( iter = pObj->m_ProfileData2.VertCurves.begin(); iter != pObj->m_ProfileData2.VertCurves.end(); iter++ )
      {
         VertCurveData& vc = *iter;

         hr = pSave->BeginUnit(_T("VertCurveData"),1.0);
         if ( FAILED(hr) )
            return hr;

         hr = pSave->put_Property(_T("PVIStation"),CComVariant(vc.PVIStation));
         if ( FAILED(hr) )
            return hr;

         hr = pSave->put_Property(_T("L1"),CComVariant(vc.L1));
         if ( FAILED(hr) )
            return hr;

         hr = pSave->put_Property(_T("L2"),CComVariant(vc.L2));
         if ( FAILED(hr) )
            return hr;

         hr = pSave->put_Property(_T("ExitGrade"),CComVariant(vc.ExitGrade));
         if ( FAILED(hr) )
            return hr;

         hr = pSave->EndUnit();
         if ( FAILED(hr) )
            return hr;
      }

      hr = pSave->EndUnit();
      if ( FAILED(hr) )
         return hr;
   }
   else
   {
      CComVariant var;

      bool bNewFormat = true;
      hr = pLoad->BeginUnit(_T("ProfileData"));
      if ( FAILED(hr) )
         bNewFormat = false;

      if ( !bNewFormat )
      {
         // read the old data into temporary variables
         var.vt = VT_I4;
         hr = pLoad->get_Property(_T("ProfileMethod"),&var);
         if ( FAILED(hr) )
            return hr;
         long method = var.lVal;

         var.vt = VT_R8;
         hr = pLoad->get_Property(_T("Station"),&var);
         if ( FAILED(hr) )
            return hr;
         double Station = var.dblVal;

         var.vt = VT_R8;
         hr = pLoad->get_Property(_T("Elevation"),&var);
         if ( FAILED(hr) )
            return hr;
         double Elevation = var.dblVal;

         var.vt = VT_R8;
         hr = pLoad->get_Property(_T("Grade"),&var);
         if ( FAILED(hr) )
            return hr;
         double Grade = var.dblVal;

         var.vt = VT_R8;
         hr = pLoad->get_Property(_T("G1"),&var);
         if ( FAILED(hr) )
            return hr;
         double G1 = var.dblVal;

         var.vt = VT_R8;
         hr = pLoad->get_Property(_T("G2"),&var);
         if ( FAILED(hr) )
            return hr;
         double G2 = var.dblVal;

         var.vt = VT_R8;
         hr = pLoad->get_Property(_T("PVIStation"),&var);
         if ( FAILED(hr) )
            return hr;
         double PVIStation = var.dblVal;

         var.vt = VT_R8;
         hr = pLoad->get_Property(_T("PVIElevation"),&var);
         if ( FAILED(hr) )
            return hr;
         double PVIElevation = var.dblVal;

         var.vt = VT_R8;
         hr = pLoad->get_Property(_T("Length"),&var);
         if ( FAILED(hr) )
            return hr;
         double Length = var.dblVal;

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
            return hr;
         pObj->m_ProfileData2.Station = var.dblVal;

         var.vt = VT_R8;
         hr = pLoad->get_Property(_T("Elevation"),&var);
         if ( FAILED(hr) )
            return hr;
         pObj->m_ProfileData2.Elevation = var.dblVal;

         var.vt = VT_R8;
         hr = pLoad->get_Property(_T("Grade"),&var);
         if ( FAILED(hr) )
            return hr;
         pObj->m_ProfileData2.Grade = var.dblVal;

         var.vt = VT_I4;
         hr = pLoad->get_Property(_T("VertCurveCount"),&var);
         if ( FAILED(hr) )
            return hr;
         long nCurves = var.lVal;

         for ( int v = 0; v < nCurves; v++ )
         {
            VertCurveData vc;

            hr = pLoad->BeginUnit(_T("VertCurveData"));
            if ( FAILED(hr) )
               return hr;

            var.vt = VT_R8;
            hr = pLoad->get_Property(_T("PVIStation"),&var);
            if ( FAILED(hr) )
               return hr;
            vc.PVIStation = var.dblVal;

            var.vt = VT_R8;
            hr = pLoad->get_Property(_T("L1"),&var);
            if ( FAILED(hr) )
               return hr;
            vc.L1 = var.dblVal;

            hr = pLoad->get_Property(_T("L2"),&var);
            if ( FAILED(hr) )
               return hr;
            vc.L2 = var.dblVal;

            hr = pLoad->get_Property(_T("ExitGrade"),&var);
            if ( FAILED(hr) )
               return hr;
            vc.ExitGrade = var.dblVal;

            pObj->m_ProfileData2.VertCurves.push_back(vc);

            hr = pLoad->EndUnit();
            if ( FAILED(hr) )
               return hr;
         }
      }

      if ( bNewFormat )
      {
         hr = pLoad->EndUnit();
         if ( FAILED(hr) )
            return hr;
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
         return hr;

      hr = pSave->put_Property(_T("SectionCount"),CComVariant((long)pObj->m_RoadwaySectionData.Superelevations.size()));
      if ( FAILED(hr) )
         return hr;

      std::vector<CrownData2>::iterator iter;
      for ( iter = pObj->m_RoadwaySectionData.Superelevations.begin(); iter != pObj->m_RoadwaySectionData.Superelevations.end(); iter++ )
      {
         CrownData2& super = *iter;

         hr = pSave->BeginUnit(_T("CrownSlope"),1.0);
         if ( FAILED(hr) )
            return hr;

         hr = pSave->put_Property(_T("Station"),CComVariant(super.Station));
         if ( FAILED(hr) )
            return hr;

         hr = pSave->put_Property(_T("Left"),CComVariant(super.Left));
         if ( FAILED(hr) )
            return hr;

         hr = pSave->put_Property(_T("Right"),CComVariant(super.Right));
         if ( FAILED(hr) )
            return hr;

         hr = pSave->put_Property(_T("CrownPointOffset"),CComVariant(super.CrownPointOffset));
         if ( FAILED(hr) )
            return hr;

         hr = pSave->EndUnit();
         if ( FAILED(hr) )
            return hr;
      }

      hr = pSave->EndUnit();
      if ( FAILED(hr) )
         return hr;
   }
   else
   {
      CComVariant var;

      bool bNewFormat = true;
      hr = pLoad->BeginUnit(_T("SuperelevationData"));
      if ( FAILED(hr) )
         bNewFormat = false;

      pObj->m_RoadwaySectionData.Superelevations.clear();

      if ( !bNewFormat )
      {
         var.vt = VT_R8;
         hr = pLoad->get_Property(_T("Left"),&var);
         if ( FAILED(hr) )
            return hr;
         double left = var.dblVal;

         var.vt = VT_R8;
         hr = pLoad->get_Property(_T("Right"),&var);
         if ( FAILED(hr) )
            return hr;
         double right = var.dblVal;

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
            return hr;
         long nSections = var.lVal;

         for ( long s = 0; s < nSections; s++ )
         {
            CrownData2 super;

            hr = pLoad->BeginUnit(_T("CrownSlope"));
            if ( FAILED(hr) )
               return hr;

            var.vt = VT_R8;
            hr = pLoad->get_Property(_T("Station"),&var);
            if ( FAILED(hr) )
               return hr;
            super.Station = var.dblVal;

            var.vt = VT_R8;
            hr = pLoad->get_Property(_T("Left"),&var);
            if ( FAILED(hr) )
               return hr;
            super.Left = var.dblVal;

            var.vt = VT_R8;
            hr = pLoad->get_Property(_T("Right"),&var);
            if ( FAILED(hr) )
               return hr;
            super.Right = var.dblVal;

            var.vt = VT_R8;
            hr = pLoad->get_Property(_T("CrownPointOffset"),&var);
            if ( FAILED(hr) )
               return hr;
            super.CrownPointOffset = var.dblVal;

            pObj->m_RoadwaySectionData.Superelevations.push_back(super);

            hr = pLoad->EndUnit();
            if ( FAILED(hr) )
               return hr;
         }
      }

      if ( bNewFormat )
      {
         hr = pLoad->EndUnit();
         if ( FAILED(hr) ) 
            return hr;
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
      double version;
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
      double version;
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
         return hr;

      CPierData firstPier;
      for ( int i = 0; i < var.lVal; i++ )
      {
         CPierData pd;
         
         pd.SetPierIndex(i);

         hr = pd.Load( pLoad, pProgress );
         if ( FAILED(hr) )
            return hr;

         if ( i == 0 )
         {
            firstPier = pd;
         }
         else if ( i == 1 )
         {
            CSpanData span;
            span.SetSpanIndex(i-1);
            pObj->m_BridgeDescription.CreateFirstSpan(&firstPier,&span,&pd);
         }
         else
         {
            CSpanData span;
            span.SetSpanIndex(i-1);
            pObj->m_BridgeDescription.AppendSpan(&span,&pd);
         }
      }
      
      // make sure that if the first and last pier are marked as continuous, change them to integral
      // continuous is not a valid input for the first and last pier, but bugs in the preview releases
      // made it possible to have this input
      CPierData* pFirstPier = pObj->m_BridgeDescription.GetPier(0);
      if ( pFirstPier->GetConnectionType() == pgsTypes::ContinuousAfterDeck )
      {
         pFirstPier->SetConnectionType(pgsTypes::IntegralAfterDeck );
      }
      else if ( pFirstPier->GetConnectionType() == pgsTypes::ContinuousBeforeDeck )
      {
         pFirstPier->SetConnectionType( pgsTypes::IntegralBeforeDeck );
      }


      CPierData* pLastPier = pObj->m_BridgeDescription.GetPier( pObj->m_BridgeDescription.GetPierCount()-1 );
      if ( pLastPier->GetConnectionType() == pgsTypes::ContinuousAfterDeck )
      {
         pLastPier->SetConnectionType( pgsTypes::IntegralAfterDeck );
      }
      else if ( pLastPier->GetConnectionType() == pgsTypes::ContinuousBeforeDeck )
      {
         pLastPier->SetConnectionType( pgsTypes::IntegralBeforeDeck );
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
      double version;
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
         return hr;

      pObj->m_strOldGirderConcreteName = xSectionData.m_strGirderConcreteName;

      pObj->m_BridgeDescription.UseSameNumberOfGirdersInAllSpans(true);
      pObj->m_BridgeDescription.SetGirderCount(xSectionData.GdrLineCount);

      pObj->m_BridgeDescription.UseSameGirderForEntireBridge(true);
      pObj->m_BridgeDescription.SetGirderName(xSectionData.Girder.c_str());

      const GirderLibraryEntry* pGdrEntry = pObj->GetGirderEntry(xSectionData.Girder.c_str());
      CComPtr<IBeamFactory> factory;
      pGdrEntry->GetBeamFactory(&factory);
      pObj->m_BridgeDescription.SetGirderFamilyName( factory->GetGirderFamilyName().c_str() );

      pgsTypes::SupportedBeamSpacings sbs = factory->GetSupportedBeamSpacings();
      pObj->m_BridgeDescription.SetGirderSpacingType(sbs[0]);

      pObj->m_BridgeDescription.SetGirderSpacing(xSectionData.GdrSpacing);
      pObj->m_BridgeDescription.SetMeasurementType(xSectionData.GdrSpacingMeasurement == CXSectionData::Normal ? pgsTypes::NormalToItem : pgsTypes::AlongItem);
      pObj->m_BridgeDescription.SetMeasurementLocation(pgsTypes::AtCenterlinePier);
      pObj->m_BridgeDescription.SetGirderOrientation((pgsTypes::GirderOrientationType)xSectionData.GirderOrientation);

      // update the deck data
      CDeckDescription* pDeck       = pObj->m_BridgeDescription.GetDeckDescription();
      pDeck->DeckRebarData          = xSectionData.DeckRebarData;
      pDeck->DeckType               = xSectionData.DeckType;
      pDeck->Fillet                 = xSectionData.Fillet;
      pDeck->GrossDepth             = xSectionData.GrossDepth;
      pDeck->OverhangEdgeDepth      = xSectionData.OverhangEdgeDepth;
      pDeck->OverhangTaper          = xSectionData.OverhangTaper;
      pDeck->OverlayWeight          = xSectionData.OverlayWeight;
      pDeck->OverlayDensity         = xSectionData.OverlayDensity;
      pDeck->OverlayDepth           = xSectionData.OverlayDepth;
      pDeck->PanelDepth             = xSectionData.PanelDepth;
      pDeck->PanelSupport           = xSectionData.PanelSupport;
      pDeck->SacrificialDepth       = xSectionData.SacrificialDepth;
      pDeck->SlabEc                 = xSectionData.SlabEc;
      pDeck->SlabFc                 = xSectionData.SlabFc;
      pDeck->SlabEcK1               = xSectionData.SlabEcK1;
      pDeck->SlabEcK2               = xSectionData.SlabEcK2;
      pDeck->SlabCreepK1            = xSectionData.SlabCreepK1;
      pDeck->SlabCreepK2            = xSectionData.SlabCreepK2;
      pDeck->SlabShrinkageK1        = xSectionData.SlabShrinkageK1;
      pDeck->SlabShrinkageK2        = xSectionData.SlabShrinkageK2;
      pDeck->SlabMaxAggregateSize   = xSectionData.SlabMaxAggregateSize;
      pDeck->SlabStrengthDensity    = xSectionData.SlabStrengthDensity;
      pDeck->SlabUserEc             = xSectionData.SlabUserEc;
      pDeck->SlabWeightDensity      = xSectionData.SlabWeightDensity;
      pDeck->TransverseConnectivity = xSectionData.TransverseConnectivity;
      pDeck->WearingSurface         = xSectionData.WearingSurface;

      pObj->m_BridgeDescription.SetSlabOffset( xSectionData.SlabOffset );
      pObj->m_BridgeDescription.SetSlabOffsetType( pgsTypes::sotBridge );

      double spacing_width = (xSectionData.GdrLineCount-1)*(xSectionData.GdrSpacing);
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
      pLeftRailingSystem->fc                 = pDeck->SlabFc;
      pLeftRailingSystem->bUserEc            = pDeck->SlabUserEc;
      pLeftRailingSystem->Ec                 = pDeck->SlabEc;
      pLeftRailingSystem->EcK1               = pDeck->SlabEcK1;
      pLeftRailingSystem->EcK2               = pDeck->SlabEcK2;
      pLeftRailingSystem->CreepK1            = pDeck->SlabCreepK1;
      pLeftRailingSystem->CreepK2            = pDeck->SlabCreepK2;
      pLeftRailingSystem->ShrinkageK1        = pDeck->SlabShrinkageK1;
      pLeftRailingSystem->ShrinkageK2        = pDeck->SlabShrinkageK2;
      pLeftRailingSystem->MaxAggSize         = pDeck->SlabMaxAggregateSize;
      pLeftRailingSystem->StrengthDensity    = pDeck->SlabStrengthDensity;
      pLeftRailingSystem->WeightDensity      = pDeck->SlabWeightDensity;

      // update the right railing
      CRailingSystem* pRightRailingSystem = pObj->m_BridgeDescription.GetRightRailingSystem();
      pRightRailingSystem->strExteriorRailing = xSectionData.RightTrafficBarrier;
      pRightRailingSystem->fc                 = pDeck->SlabFc;
      pRightRailingSystem->bUserEc            = pDeck->SlabUserEc;
      pRightRailingSystem->Ec                 = pDeck->SlabEc;
      pRightRailingSystem->EcK1               = pDeck->SlabEcK1;
      pRightRailingSystem->EcK2               = pDeck->SlabEcK2;
      pRightRailingSystem->CreepK1            = pDeck->SlabCreepK1;
      pRightRailingSystem->CreepK2            = pDeck->SlabCreepK2;
      pRightRailingSystem->ShrinkageK1        = pDeck->SlabShrinkageK1;
      pRightRailingSystem->ShrinkageK2        = pDeck->SlabShrinkageK2;
      pRightRailingSystem->MaxAggSize         = pDeck->SlabMaxAggregateSize;
      pRightRailingSystem->StrengthDensity    = pDeck->SlabStrengthDensity;
      pRightRailingSystem->WeightDensity      = pDeck->SlabWeightDensity;

      pObj->m_BridgeDescription.CopyDown(true,true,true,true);

#if defined _DEBUG
      GirderIndexType nGirders = pObj->m_BridgeDescription.GetGirderCount();
      SpanIndexType nSpans = pObj->m_BridgeDescription.GetSpanCount();
      for ( SpanIndexType spanIdx = 0; spanIdx < nSpans; spanIdx++ )
      {
         const CSpanData* pSpan = pObj->m_BridgeDescription.GetSpan(spanIdx);
         ATLASSERT( nGirders == pSpan->GetGirderCount() );
         ATLASSERT( nGirders == pSpan->GetGirderSpacing(pgsTypes::Ahead)->GetSpacingCount()+1);
         ATLASSERT( nGirders == pSpan->GetGirderSpacing(pgsTypes::Ahead)->Debug_GetGirderCount());
         ATLASSERT( nGirders == pSpan->GetGirderSpacing(pgsTypes::Back)->GetSpacingCount()+1);
         ATLASSERT( nGirders == pSpan->GetGirderSpacing(pgsTypes::Back)->Debug_GetGirderCount());
         ATLASSERT( nGirders == pSpan->GetGirderTypes()->Debug_GetNumGirderTypes());
      }
#endif
   }

   return S_OK;
}

HRESULT CProjectAgentImp::BridgeDescriptionProc(IStructuredSave* pSave,IStructuredLoad* pLoad,IProgress* pProgress,CProjectAgentImp* pObj)
{
   if ( pSave )
   {
      HRESULT hr = pObj->m_BridgeDescription.Save( pSave, pProgress );
      if ( FAILED(hr) )
         return hr;
   }
   else
   {
      double version;
      pLoad->get_Version(&version);
      if ( version <= 1.0 ) // if ProjectData version 1 or less, load the old format, otherwise do nothing
         return S_OK;

      HRESULT hr = pObj->m_BridgeDescription.Load( pLoad, pProgress );
      if ( FAILED(hr) )
         return hr;
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
      double version;
      pLoad->get_Version(&version);
      if ( version <= 1.0 ) // if ProjectData version 1 or less, load the old format, otherwise do nothing
      {
         pLoad->BeginUnit(_T("Prestressing"));
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
      std::map<SpanGirderHashType,CGirderData> girderData;

      CComVariant var;
      var.vt = VT_I4;

      HRESULT hr;

      double version;
      pLoad->get_Version(&version);

      // removed at version 3 of data block
      const matPsStrand* pStrandMaterial = 0;
      if ( version < 3 )
      {
         hr = pLoad->get_Property(_T("PrestressStrandKey"), &var );
         if ( FAILED(hr) )
            return hr;

         lrfdStrandPool* pPool = lrfdStrandPool::GetInstance();
         pStrandMaterial = pPool->GetStrand( var.lVal );
         CHECK(pStrandMaterial!=0);
      }


      // A bug released in 2.07 can allow files to be saved with more girder data
      // than spans exist. we need to trap this
      SpanIndexType nspans = pObj->m_BridgeDescription.GetSpanCount();

      hr = pLoad->get_Property(_T("PrestressDataCount"), &var );
      if ( FAILED(hr) )
         return hr;

      IndexType cnt = var.lVal;

      const ConcreteLibraryEntry* pConcreteLibraryEntry = pObj->GetConcreteEntry(pObj->m_strOldGirderConcreteName.c_str());
      for ( IndexType i = 0; i < cnt; i++ )
      {
         HRESULT hr = pLoad->BeginUnit(_T("GirderPrestressData"));
         if ( FAILED(hr) )
            return hr;

         var.vt = VT_I4;
         hr = pLoad->get_Property(_T("Span"), &var );
         if ( FAILED(hr) )
            return hr;
         
         SpanIndexType span = var.iVal;

         hr = pLoad->get_Property(_T("Girder"), &var );
         if ( FAILED(hr) )
            return hr;

         GirderIndexType girder = var.iVal;
         SpanGirderHashType hashval = HashSpanGirder(span, girder);

         CGirderData girder_data;
         if ( pConcreteLibraryEntry == NULL )
         {
            hr = girder_data.Load(pLoad,pProgress);
         }
         else
         {
            hr = girder_data.Load( pLoad, pProgress, pConcreteLibraryEntry->GetFc(),
                                                     pConcreteLibraryEntry->GetWeightDensity(),
                                                     pConcreteLibraryEntry->GetStrengthDensity(),
                                                     pConcreteLibraryEntry->GetAggregateSize());
         }

         if ( FAILED(hr) )
            return hr;

         hr = pLoad->EndUnit();
         if ( FAILED(hr) )
            return hr;

         // pStrandMaterial will not be NULL if this is a pre version 3 data block
         // in this case, the girder_data object does not have a strand material set.
         // Set it now
         if ( pStrandMaterial != 0 )
         {
            ATLASSERT(girder_data.Material.pStrandMaterial[pgsTypes::Straight]  == 0);
            ATLASSERT(girder_data.Material.pStrandMaterial[pgsTypes::Harped]    == 0);
            ATLASSERT(girder_data.Material.pStrandMaterial[pgsTypes::Temporary] == 0);
            girder_data.Material.pStrandMaterial[pgsTypes::Straight]  = pStrandMaterial;
            girder_data.Material.pStrandMaterial[pgsTypes::Harped]    = pStrandMaterial;
            girder_data.Material.pStrandMaterial[pgsTypes::Temporary] = pStrandMaterial;
         }

         if (span<nspans)
         {
            girderData.insert(std::make_pair(hashval, girder_data));
         }
         else
         {
            ATLASSERT(0); // file has data for span that doesn't exist
         }
      }

      // put girder data into the bridge description
      std::map<SpanGirderHashType,CGirderData>::iterator iter;
      for ( iter = girderData.begin(); iter != girderData.end(); iter++ )
      {
         SpanGirderHashType hash = (*iter).first;
         CGirderData gdrData = (*iter).second;

         SpanIndexType spanIdx;
         GirderIndexType gdrIdx;
         UnhashSpanGirder(hash,&spanIdx,&gdrIdx);

         CGirderTypes girderTypes = *pObj->m_BridgeDescription.GetSpan(spanIdx)->GetGirderTypes();
         girderTypes.SetGirderData(gdrIdx,gdrData);
         pObj->m_BridgeDescription.GetSpan(spanIdx)->SetGirderTypes(girderTypes);
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
      double version;
      pLoad->get_Version(&version);
      if ( version <= 1.0 ) // if ProjectData version 1 or less, load the old format, otherwise do nothing
      {
         pLoad->BeginUnit(_T("Shear"));
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

      double version;
      pLoad->get_Version(&version);

      if (version < 3)
      {
         // version 3 and later... this data member is gone
         // just load it and pretend it doesn't matter
         var.vt = VT_BSTR;

         HRESULT hr = pLoad->get_Property(_T("StirrupMaterial"), &var );
         if ( FAILED(hr) )
            return hr;

         ::SysFreeString( var.bstrVal );
      }

      // A bug released in 2.07 can allow files to be saved with more girder data
      // than spans exist. we need to trap this
      SpanIndexType nspans = pObj->m_BridgeDescription.GetSpanCount();

      var.vt = VT_I4;
      HRESULT hr = pLoad->get_Property(_T("ShearDataCount"), &var );
      if ( FAILED(hr) )
         return hr;

      IndexType cnt = var.lVal;

      for ( IndexType i = 0; i < cnt; i++ )
      {
         HRESULT hr = pLoad->BeginUnit(_T("GirderShearData"));
         if ( FAILED(hr) )
            return hr;

         var.vt = VT_I4;
         hr = pLoad->get_Property(_T("Span"), &var );
         if ( FAILED(hr) )
            return hr;

         SpanIndexType span = var.iVal;

         hr = pLoad->get_Property(_T("Girder"), &var );
         if ( FAILED(hr) )
            return hr;

         GirderIndexType girder = var.iVal;

         SpanGirderHashType hashval = HashSpanGirder(span, girder);

         CShearData pd;
         CStructuredLoad load( pLoad );
         hr = pd.Load( &load );
         if ( FAILED(hr) )
            return hr;

         if (span < nspans)
         {
            shearData.insert( std::make_pair(hashval, pd) );
         }

         hr = pLoad->EndUnit();// GirderShearData
         if ( FAILED(hr) )
            return hr;
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

         CGirderTypes girderTypes = *pObj->m_BridgeDescription.GetSpan(spanIdx)->GetGirderTypes();
         CGirderData gd = girderTypes.GetGirderData(gdrIdx);
         gd.ShearData = shear;
         girderTypes.SetGirderData(gdrIdx,gd);
         pObj->m_BridgeDescription.GetSpan(spanIdx)->SetGirderTypes(girderTypes);
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
      double version;
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
      double top_version;
      pLoad->get_TopVersion(&top_version);

      // file version is before we started saving this data... set up some default data and get the heck outta here
      if ( top_version < 1.0 )
      {
         // set values from the library entry
         SpanIndexType nSpans;
         nSpans = pObj->m_BridgeDescription.GetSpanCount();

         for ( SpanIndexType span = 0; span < nSpans; span++ )
         {
            CSpanData* pSpan = pObj->m_BridgeDescription.GetSpan(span);
            CGirderTypes girderTypes = *pSpan->GetGirderTypes();

            GirderIndexType nGirders = pSpan->GetGirderCount();
            for ( GirderIndexType gdr = 0; gdr < nGirders; gdr++ )
            {
               // we have to get the girder library entry this way instead of from the bridge
               // description because the library references haven't been set up yet.
               const GirderLibraryEntry* libGirder = pObj->GetGirderEntry( pObj->m_BridgeDescription.GetSpan(span)->GetGirderTypes()->GetGirderName(gdr) );

               SpanGirderHashType hashval = HashSpanGirder(span, gdr);
               CLongitudinalRebarData lrd;
               lrd.CopyGirderEntryData(*libGirder);

               girderTypes.GetGirderData(gdr).LongitudinalRebarData = lrd;
            }

            pSpan->SetGirderTypes(girderTypes);
         }
         return S_OK;
      }
      else
      {
         CComVariant var;
         var.vt = VT_BSTR;

         HRESULT hr = pLoad->BeginUnit(_T("LongitudinalRebarData"));
         if ( FAILED(hr) )
            return hr;

         double version;
         pLoad->get_Version(&version);
         if ( version < 2 )
         {
            // version 2 and later... this data member is gone
            // just load it and pretend it doesn't matter
            hr = pLoad->get_Property(_T("LongitudinalRebarMaterial"), &var );
            if ( FAILED(hr) )
               return hr;

            ::SysFreeString( var.bstrVal );
         }


         // A bug released in 2.07 can allow files to be saved with more girder data
         // than spans exist. we need to trap this
         SpanIndexType nspans = pObj->m_BridgeDescription.GetSpanCount();

         var.vt = VT_I4;
         hr = pLoad->get_Property(_T("LongitudinalRebarDataCount"), &var );
         if ( FAILED(hr) )
            return hr;

         IndexType cnt = var.lVal;

         for ( IndexType i = 0; i < cnt; i++ )
         {
            HRESULT hr = pLoad->BeginUnit(_T("GirderLongitudinalRebarData"));
            if ( FAILED(hr) )
               return hr;

            var.vt = VT_I4;
            hr = pLoad->get_Property(_T("Span"), &var );
            if ( FAILED(hr) )
               return hr;

            SpanIndexType span = var.iVal;

            hr = pLoad->get_Property(_T("Girder"), &var );
            if ( FAILED(hr) )
               return hr;
            
            GirderIndexType girder = var.iVal;

            SpanGirderHashType hashval = HashSpanGirder(span, girder);

            CLongitudinalRebarData lrd;
            hr = lrd.Load( pLoad, pProgress );
            if ( FAILED(hr) )
               return hr;

            hr = pLoad->EndUnit();// GirderLongitudinalRebarData
            if ( FAILED(hr) )
               return hr;

            if (span < nspans)
            {
               CGirderTypes girderTypes = *pObj->m_BridgeDescription.GetSpan(span)->GetGirderTypes();
               girderTypes.GetGirderData(girder).LongitudinalRebarData = lrd;
               pObj->m_BridgeDescription.GetSpan(span)->SetGirderTypes(girderTypes);
            }
         }

         hr = pLoad->EndUnit();// LongitudinalRebarData
         if ( FAILED(hr) )
            return hr;
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
      double version;
      pLoad->get_Version(&version);
      if ( 3 < version )
      {
         // added in version 4
         return pObj->m_LoadFactors.Load(pLoad,pProgress);
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
      double version;
      pLoad->get_Version(&version);
      if ( version <= 1.0 ) // if ProjectData version 1 or less, load the old format, otherwise do nothing
      {
         pLoad->BeginUnit(_T("LiftingAndHauling"));

         CComVariant var;

         double version;
         pLoad->get_Version(&version);

         if ( version < 3 )
         {
            var.vt = VT_I4;
            HRESULT hr = pLoad->get_Property(_T("LiftingAndHaulingDataCount"), &var );
            if ( FAILED(hr) )
               return hr;

            long count = var.lVal;
            for ( int i = 0; i < count; i++ )
            {
               hr =  LiftingAndHaulingLoadDataProc(pLoad,pProgress,pObj);
               if ( FAILED(hr) )
                  return hr;
            }
         }
         else
         {
            // version 3 and later data blocks... count isn't listed because
            // the number of girders can vary per span
            SpanIndexType nSpans = pObj->m_BridgeDescription.GetSpanCount();
            for ( SpanIndexType spanIdx = 0; spanIdx < nSpans; spanIdx++ )
            {
               GirderIndexType nGirders = pObj->m_BridgeDescription.GetSpan(spanIdx)->GetGirderCount();
               for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
               {
                  hr =  LiftingAndHaulingLoadDataProc(pLoad,pProgress,pObj);
                  if ( FAILED(hr) )
                     return hr;
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
      return hr;

   double version;
   pLoad->get_Version(&version);

   CComVariant var;
   var.ChangeType( VT_I2 );
   hr = pLoad->get_Property(_T("Span"), &var );
   if ( FAILED(hr) )
      return hr;

   SpanIndexType span = var.iVal;

   var.ChangeType( VT_I2 );
   hr = pLoad->get_Property(_T("Girder"), &var );
   if ( FAILED(hr) )
      return hr;

   GirderIndexType girder = var.iVal;

   CHandlingData handlingData;

   if ( version < 1.1 )
   {
      // only 1 lift point and truck support point... use same at each end of girder
      var.ChangeType( VT_R8 );
      hr = pLoad->get_Property(_T("LiftingLoopLocation"), &var );
      if ( FAILED(hr) )
         return hr;

      handlingData.LeftLiftPoint  = var.dblVal;
      handlingData.RightLiftPoint = var.dblVal;

      var.ChangeType( VT_R8 );
      hr = pLoad->get_Property(_T("TruckSupportLocation"), &var );
      if ( FAILED(hr) )
         return hr;

      handlingData.LeadingSupportPoint  = var.dblVal;
      handlingData.TrailingSupportPoint = var.dblVal;

      CGirderTypes girderTypes = *pObj->m_BridgeDescription.GetSpan(span)->GetGirderTypes();
      girderTypes.GetGirderData(girder).HandlingData = handlingData;
      pObj->m_BridgeDescription.GetSpan(span)->SetGirderTypes(girderTypes);
   }
   else
   {
      var.ChangeType( VT_R8 );
      hr = pLoad->get_Property(_T("LeftLiftingLoopLocation"), &var );
      if ( FAILED(hr) )
         return hr;

      handlingData.LeftLiftPoint = var.dblVal;

      var.ChangeType( VT_R8 );
      hr = pLoad->get_Property(_T("RightLiftingLoopLocation"), &var );
      if ( FAILED(hr) )
         return hr;

      handlingData.RightLiftPoint = var.dblVal;

      var.ChangeType( VT_R8 );
      hr = pLoad->get_Property(_T("LeadingOverhang"), &var );
      if ( FAILED(hr) )
         return hr;

      handlingData.LeadingSupportPoint = var.dblVal;

      var.ChangeType( VT_R8 );
      hr = pLoad->get_Property(_T("TrailingOverhang"), &var );
      if ( FAILED(hr) )
         return hr;

      handlingData.TrailingSupportPoint = var.dblVal;

      CGirderTypes girderTypes = *pObj->m_BridgeDescription.GetSpan(span)->GetGirderTypes();
      girderTypes.GetGirderData(girder).HandlingData = handlingData;
      pObj->m_BridgeDescription.GetSpan(span)->SetGirderTypes(girderTypes);
   }

   hr = pLoad->EndUnit();// LiftingAndHaulingData
   if ( FAILED(hr) )
      return hr;

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
      double parentVersion;
      pLoad->get_ParentVersion(&parentVersion);
      if ( parentVersion <= 1.0 ) // if ProjectData version 1 or less, load the old format, otherwise do nothing
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
         return hr;

      _bstr_t bval(var.bstrVal);
      if (bval != _bstr_t(_T("DirectlyInput")))
         return E_FAIL;


      // set up the bridge model
      pObj->m_BridgeDescription.SetDistributionFactorMethod(pgsTypes::DirectlyInput);


      double version;
      pLoad->get_Version(&version);

      if ( version < 1.1 )
      {
         // load data in pre- version 1.1 format
         // Load the old data into temporary variables
         var.vt = VT_R8 ;
         hr = pLoad->get_Property(_T("ShearDistFactor"), &var );
         if ( FAILED(hr) )
            return hr;

         double V = var.dblVal;

         hr = pLoad->get_Property(_T("MomentDistFactor"), &var );
         if ( FAILED(hr) )
            return hr;

         double M = var.dblVal;

         hr = pLoad->get_Property(_T("ReactionDistFactor"), &var );
         if ( FAILED(hr) )
            return hr;

         double R = var.dblVal;

         // Fill up the data structures
         CPierData* pPier = pObj->m_BridgeDescription.GetPier(0);
         CSpanData* pSpan;
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

         double M[2], V[2], R[2];

         var.vt = VT_R8 ;
         hr = pLoad->get_Property(_T("IntShearDistFactor"), &var );
         if ( FAILED(hr) )
            return hr;

         V[pgsTypes::Interior] = var.dblVal;

         hr = pLoad->get_Property(_T("IntMomentDistFactor"), &var );
         if ( FAILED(hr) )
            return hr;

         M[pgsTypes::Interior] = var.dblVal;

         hr = pLoad->get_Property(_T("IntReactionDistFactor"), &var );
         if ( FAILED(hr) )
            return hr;

         R[pgsTypes::Interior] = var.dblVal;

      
         hr = pLoad->get_Property(_T("ExtShearDistFactor"), &var );
         if ( FAILED(hr) )
            return hr;

         V[pgsTypes::Exterior] = var.dblVal;

         hr = pLoad->get_Property(_T("ExtMomentDistFactor"), &var );
         if ( FAILED(hr) )
            return hr;

         M[pgsTypes::Exterior] = var.dblVal;

         hr = pLoad->get_Property(_T("ExtReactionDistFactor"), &var );
         if ( FAILED(hr) )
            return hr;

         R[pgsTypes::Exterior] = var.dblVal;


         // Fill up the data structures
         CPierData* pPier = pObj->m_BridgeDescription.GetPier(0);
         CSpanData* pSpan;
         do
         {
            pSpan = pPier->GetNextSpan();
            if ( pSpan )
            {
               GirderIndexType ngdrs = pSpan->GetGirderCount();

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
            CPierData* pPier = pObj->m_BridgeDescription.GetPier(0);
            CSpanData* pSpan;

            hr = pLoad->BeginUnit((pgsTypes::GirderLocation)type == pgsTypes::Interior ? _T("InteriorGirders") : _T("ExteriorGirders"));
            if ( FAILED(hr) )
               return hr;

            do
            {
               double pM, nM, V, R;

               var.vt = VT_R8;
               hr = pLoad->BeginUnit(_T("Pier"));
               if ( FAILED(hr) )
                  return hr;

               hr = pLoad->get_Property(_T("pM"),&var);
               if ( FAILED(hr) )
                  return hr;

               hr = pLoad->get_Property(_T("nM"),&var);
               if ( FAILED(hr) )
                  return hr;


               pPier->SetLLDFNegMoment(type,pgsTypes::StrengthI,var.dblVal);
               pPier->SetLLDFNegMoment(type,pgsTypes::FatigueI, var.dblVal);

               hr = pLoad->get_Property(_T("V"), &var);
               if ( FAILED(hr) )
                  return hr;

               hr = pLoad->get_Property(_T("R"), &var);
               if ( FAILED(hr) )
                  return hr;

               pPier->SetLLDFReaction(type,pgsTypes::StrengthI,var.dblVal);
               pPier->SetLLDFReaction(type,pgsTypes::FatigueI, var.dblVal);

               pLoad->EndUnit();

               pSpan = pPier->GetNextSpan();

               if ( pSpan )
               {
                  hr = pLoad->BeginUnit(_T("Span"));
                  if ( FAILED(hr) )
                     return hr;


                  hr = pLoad->get_Property(_T("pM"),&var);
                  if ( FAILED(hr) )
                     return hr;

                  pM = var.dblVal;

                  hr = pLoad->get_Property(_T("nM"),&var);
                  if ( FAILED(hr) )
                     return hr;

                  nM = var.dblVal;


                  hr = pLoad->get_Property(_T("V"), &var);
                  if ( FAILED(hr) )
                     return hr;

                  V = var.dblVal;

                  hr = pLoad->get_Property(_T("R"), &var);
                  if ( FAILED(hr) )
                     return hr;

                  R = var.dblVal;

                  // Set for all girders
                  GirderIndexType ngdrs = pSpan->GetGirderCount();
                  for (GirderIndexType ig=0; ig<ngdrs; ig++)
                  {
                     if ((pgsTypes::GirderLocation)type==pgsTypes::Interior && pSpan->IsInteriorGirder(ig) ||
                         (pgsTypes::GirderLocation)type==pgsTypes::Exterior && pSpan->IsExteriorGirder(ig))
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
      pSave->BeginUnit(_T("UserDefinedLoads"),3.0);

      pObj->SavePointLoads(pSave);
      pObj->SaveDistributedLoads(pSave);
      pObj->SaveMomentLoads(pSave);

      pSave->put_Property(_T("ConstructionLoad"),CComVariant(pObj->m_ConstructionLoad));
      
      pSave->EndUnit();
   }
   else
   {
      // only load if the user defined loads are in the project. older versions have none
      if (!FAILED(pLoad->BeginUnit(_T("UserDefinedLoads"))))
      {

         hr = pObj->LoadPointLoads(pLoad);
         if ( FAILED(hr) )
            return hr;

         hr = pObj->LoadDistributedLoads(pLoad);
         if ( FAILED(hr) )
            return hr;

         double version;
         pLoad->get_Version(&version);
         if ( 2 <= version )
         {
            hr = pObj->LoadMomentLoads(pLoad);
            if ( FAILED(hr) )
               return hr;
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
            return hr;
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
         double version;
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
            return hr;

         hr = CProjectAgentImp::LoadLiveLoad(pLoad,pProgress,pObj,_T("PermitLiveLoads"),pgsTypes::lltPermit);
         if ( FAILED(hr) )
            return hr;

         if ( 4 <= version )
         {
            hr = CProjectAgentImp::LoadLiveLoad(pLoad,pProgress,pObj,_T("FatigueLiveLoads"),pgsTypes::lltFatigue);
            if ( FAILED(hr) )
               return hr;
         }

         if ( 5 <= version )
         {
            hr = CProjectAgentImp::LoadLiveLoad(pLoad,pProgress,pObj,_T("LegalRoutineLiveLoads"),pgsTypes::lltLegalRating_Routine);
            if ( FAILED(hr) )
               return hr;

            hr = CProjectAgentImp::LoadLiveLoad(pLoad,pProgress,pObj,_T("LegalSpecialLiveLoads"),pgsTypes::lltLegalRating_Special);
            if ( FAILED(hr) )
               return hr;

            hr = CProjectAgentImp::LoadLiveLoad(pLoad,pProgress,pObj,_T("PermitRoutineLiveLoads"),pgsTypes::lltPermitRating_Routine);
            if ( FAILED(hr) )
               return hr;

            hr = CProjectAgentImp::LoadLiveLoad(pLoad,pProgress,pObj,_T("PermitSpecialLiveLoads"),pgsTypes::lltPermitRating_Special);
            if ( FAILED(hr) )
               return hr;
         }

         hr = pLoad->EndUnit(); // LiveLoads
         if ( FAILED(hr) )
            return hr;
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

   VehicleIndexType cnt = pObj->m_SelectedLiveLoads[llType].size();
   pSave->put_Property(_T("VehicleCount"), CComVariant(cnt));
   {
      pSave->BeginUnit(_T("Vehicles"),1.0);
      
      for (VehicleIndexType itrk=0; itrk<cnt; itrk++)
      {
         LiveLoadSelection& sel = pObj->m_SelectedLiveLoads[llType][itrk];
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
      return hr;

   Float64 vers;
   pLoad->get_Version(&vers);

   CComVariant var;
   var.vt = VT_R8;
   hr = pLoad->get_Property(_T("TruckImpact"),&var);
   if ( FAILED(hr) )
      return hr;

   pObj->m_TruckImpact[llType] = var.dblVal;

   var.Clear();
   var.vt = VT_R8;
   hr = pLoad->get_Property(_T("LaneImpact"),&var);
   if ( FAILED(hr) )
      return hr;

   pObj->m_LaneImpact[llType] = var.dblVal;

   var.Clear();
   var.vt = VT_I4;
   hr = pLoad->get_Property(_T("VehicleCount"), &var);
   if ( FAILED(hr) )
      return hr;

   long cnt = var.lVal;

   {
      hr = pLoad->BeginUnit(_T("Vehicles"));
      if ( FAILED(hr) )
         return hr;

      pObj->m_SelectedLiveLoads[llType].clear();

      bool was_pedestrian=false; // pedestrian loads used to be saved as vehicles
      for (long itrk=0; itrk<cnt; itrk++)
      {
         var.Clear();
         var.vt = VT_BSTR;
         hr = pLoad->get_Property(_T("VehicleName"), &var);
         if ( FAILED(hr) )
            return hr;

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

            if ( !pObj->IsReservedLiveLoad(sel.EntryName) )
            {
               use_library_entry( &pObj->m_LibObserver,
                                  sel.EntryName, 
                                  &sel.pEntry, 
                                  *pLiveLoadLibrary);
            }
            
            pObj->m_SelectedLiveLoads[llType].push_back(sel);
         }
      }

      hr = pLoad->EndUnit(); // Vehicles
      if ( FAILED(hr) )
         return hr;

      // m_PedestrianLoadApplicationType
      if (vers>1.0)
      {
         var.Clear();
         var.vt = VT_I4;
         hr = pLoad->get_Property(_T("PedestrianLoadApplicationType"),&var);
         if ( FAILED(hr) )
            return hr;

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
            pObj->m_PedestrianLoadApplicationType[llType] = was_pedestrian ? ILiveLoads::PedConcurrentWithVehiculuar : ILiveLoads::PedDontApply;
         }
      }
   }



   hr = pLoad->EndUnit();
   if ( FAILED(hr) )
      return hr;

   return S_OK;
}

#if defined _DEBUG
bool CProjectAgentImp::AssertValid() const
{
   // Check Libraries
   if (m_pLibMgr==0)
      return false;

   if ( !m_pLibMgr->AssertValid() )
      return false;

   // Check pier order
   const CSpanData* pSpan = m_BridgeDescription.GetSpan(0);
   while ( pSpan )
   {
      const CPierData* pPrevPier = pSpan->GetPrevPier();
      const CPierData* pNextPier = pSpan->GetNextPier();

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

BEGIN_STRSTORAGEMAP(CProjectAgentImp,_T("ProjectData"),4.0)
   BEGIN_UNIT(_T("ProjectProperties"),_T("Project Properties"),1.0)
      PROPERTY(_T("BridgeName"),SDT_STDSTRING, m_BridgeName )
      PROPERTY(_T("BridgeId"),  SDT_STDSTRING, m_BridgeId )
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

   PROP_CALLBACK(CProjectAgentImp::LiftingAndHaulingDataProc )
   PROP_CALLBACK(CProjectAgentImp::DistFactorMethodDataProc )
   PROP_CALLBACK(CProjectAgentImp::UserLoadsDataProc )
   PROP_CALLBACK(CProjectAgentImp::LiveLoadsDataProc )

   PROP_CALLBACK(CProjectAgentImp::RatingSpecificationProc)
   PROP_CALLBACK(CProjectAgentImp::EffectiveFlangeWidthProc)

END_STRSTORAGEMAP

STDMETHODIMP CProjectAgentImp::SetBroker(IBroker* pBroker)
{
   AGENT_SET_BROKER(pBroker);
   return S_OK;
}

STDMETHODIMP CProjectAgentImp::RegInterfaces()
{
   CComQIPtr<IBrokerInitEx2,&IID_IBrokerInitEx2> pBrokerInit(m_pBroker);

   pBrokerInit->RegInterface( IID_IProjectProperties,    this );
   pBrokerInit->RegInterface( IID_IEnvironment,          this );
   pBrokerInit->RegInterface( IID_IRoadwayData,          this );
   pBrokerInit->RegInterface( IID_IBridgeDescription,    this );
   pBrokerInit->RegInterface( IID_IGirderData,           this );
   pBrokerInit->RegInterface( IID_IShear,                this );
   pBrokerInit->RegInterface( IID_ILongitudinalRebar,    this );
   pBrokerInit->RegInterface( IID_ISpecification,        this );
   pBrokerInit->RegInterface( IID_IRatingSpecification,  this );
   pBrokerInit->RegInterface( IID_ILibraryNames,         this );
   pBrokerInit->RegInterface( IID_ILibrary,              this );
   pBrokerInit->RegInterface( IID_ILoadModifiers,        this );
   pBrokerInit->RegInterface( IID_IGirderLifting,        this );
   pBrokerInit->RegInterface( IID_IGirderHauling,        this );
   pBrokerInit->RegInterface( IID_IImportProjectLibrary, this );
   pBrokerInit->RegInterface( IID_IUserDefinedLoadData,  this );
   pBrokerInit->RegInterface( IID_IEvents,               this );
   pBrokerInit->RegInterface( IID_ILimits,               this );
   pBrokerInit->RegInterface( IID_ILimits2,              this );
   pBrokerInit->RegInterface( IID_ILoadFactors,          this );
   pBrokerInit->RegInterface( IID_ILiveLoads,            this );
   pBrokerInit->RegInterface( IID_IEffectiveFlangeWidth, this );

   return S_OK;
};

STDMETHODIMP CProjectAgentImp::Init()
{
   AGENT_INIT;

   ////
   //// Attach to connection points for interfaces this agent depends on
   ////
   //CComQIPtr<IBrokerInitEx2,&IID_IBrokerInitEx2> pBrokerInit(m_pBroker);
   //CComPtr<IConnectionPoint> pCP;
   //HRESULT hr = S_OK;

   m_scidGirderDescriptionWarning = pStatusCenter->RegisterCallback(new pgsGirderDescriptionStatusCallback(m_pBroker,eafTypes::statusWarning));

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
         release_library_entry( &m_LibObserver,
                                m_pSpecEntry,
                                *(m_pLibMgr->GetSpecLibrary()) );


      // Rating Spec Library
      release_library_entry( &m_LibObserver, 
                             m_pRatingEntry,
                             *(m_pLibMgr->GetRatingLibrary()) );


      // Live Load Library
      const LiveLoadLibrary& pLiveLoadLibrary = *(m_pLibMgr->GetLiveLoadLibrary());
      int nLLTypes = sizeof(m_SelectedLiveLoads)/sizeof(LiveLoadSelectionContainer);
      ATLASSERT(nLLTypes == 8);
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
   AGENT_CLEAR_INTERFACE_CACHE;

   return S_OK;
}

void CProjectAgentImp::UseBridgeLibraryEntries()
{
   // Connections
   ConnectionLibrary& rconlib = m_pLibMgr->GetConnectionLibrary();
   CPierData* pPier = m_BridgeDescription.GetPier(0);
   while ( pPier != NULL )
   {
      // Add a reference to the library entry
      const ConnectionLibraryEntry* pConnLibEntry;
      if ( pPier->GetPrevSpan() )
      {
         // pier has a back side
         use_library_entry( &m_LibObserver,
                            pPier->GetConnection(pgsTypes::Back), 
                            &pConnLibEntry, 
                            rconlib);
         pPier->SetConnectionLibraryEntry(pgsTypes::Back,pConnLibEntry);
      }

      // Add a reference to the library entry
      if ( pPier->GetNextSpan() )
      {
         // pier has an ahead side
         use_library_entry( &m_LibObserver,
                            pPier->GetConnection(pgsTypes::Ahead), 
                            &pConnLibEntry, 
                            rconlib);
         pPier->SetConnectionLibraryEntry(pgsTypes::Ahead,pConnLibEntry);
      }

      if ( pPier->GetNextSpan() )
         pPier = pPier->GetNextSpan()->GetNextPier();
      else
         pPier = NULL;
   }

   UseGirderLibraryEntries();

   // left traffic barrier
   const TrafficBarrierLibrary& rbarlib = m_pLibMgr->GetTrafficBarrierLibrary();
   CRailingSystem* pRailingSystem = m_BridgeDescription.GetLeftRailingSystem();
   const TrafficBarrierEntry* pEntry = NULL;

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
   if ( m_pLibMgr )
   {
      // Girders
      const GirderLibrary&  girderLibrary = m_pLibMgr->GetGirderLibrary();
      const GirderLibraryEntry* pEntry;

      use_library_entry( &m_LibObserver,
                         m_BridgeDescription.GetGirderName(), 
                         &pEntry, 
                         girderLibrary);
      m_BridgeDescription.SetGirderLibraryEntry(pEntry);

      if ( pEntry )
      {
         ATLASSERT(m_BridgeDescription.GetGirderName() == m_BridgeDescription.GetGirderLibraryEntry()->GetName());
      }

      CSpanData* pSpan = m_BridgeDescription.GetSpan(0);
      while ( pSpan )
      {
         CGirderTypes girderTypes = *pSpan->GetGirderTypes();
         GroupIndexType nGroups = girderTypes.GetGirderGroupCount();
         for ( GroupIndexType groupIdx = 0; groupIdx < nGroups; groupIdx++ )
         {
            GirderIndexType firstGdrIdx,lastGdrIdx;
            std::_tstring strGirderName;

            girderTypes.GetGirderGroup(groupIdx,&firstGdrIdx,&lastGdrIdx,strGirderName);

            for ( GirderIndexType gdrIdx = firstGdrIdx; gdrIdx <= lastGdrIdx; gdrIdx++ )
            {
               use_library_entry( &m_LibObserver,
                                  strGirderName, 
                                  &pEntry, 
                                  girderLibrary);

               CGirderData& girderData = girderTypes.GetGirderData(gdrIdx);
               if ( girderData.Material.pStrandMaterial[pgsTypes::Straight] == NULL )
               {
                  // make sure the girder data has strand
                  girderData.Material.pStrandMaterial[pgsTypes::Straight] = lrfdStrandPool::GetInstance()->GetStrand(matPsStrand::Gr1725,
                                                                                                                     matPsStrand::StressRelieved,
                                                                                                                     matPsStrand::D635 );
               }

               if ( girderData.Material.pStrandMaterial[pgsTypes::Harped] == NULL )
               {
                  // make sure the girder data has strand
                  girderData.Material.pStrandMaterial[pgsTypes::Harped] = lrfdStrandPool::GetInstance()->GetStrand(matPsStrand::Gr1725,
                                                                                                                     matPsStrand::StressRelieved,
                                                                                                                     matPsStrand::D635 );
               }

               if ( girderData.Material.pStrandMaterial[pgsTypes::Temporary] == NULL )
               {
                  // make sure the girder data has strand
                  girderData.Material.pStrandMaterial[pgsTypes::Temporary] = lrfdStrandPool::GetInstance()->GetStrand(matPsStrand::Gr1725,
                                                                                                                     matPsStrand::StressRelieved,
                                                                                                                     matPsStrand::D635 );
               }
            }

            girderTypes.SetGirderLibraryEntry(groupIdx,pEntry);
         }

         pSpan->SetGirderTypes(girderTypes);
         pSpan = pSpan->GetNextPier()->GetNextSpan();
      }
   }
}

void CProjectAgentImp::ReleaseBridgeLibraryEntries()
{
   if ( m_pLibMgr )
   {
      // pier data
      ConnectionLibrary& rconlib = m_pLibMgr->GetConnectionLibrary();
      CPierData* pPier = m_BridgeDescription.GetPier(0);
      while ( pPier != NULL )
      {
         if (pPier->GetConnectionLibraryEntry(pgsTypes::Back) )
         {
            release_library_entry( &m_LibObserver,
                                   pPier->GetConnectionLibraryEntry(pgsTypes::Back), 
                                   rconlib);

         }

         if (pPier->GetConnectionLibraryEntry(pgsTypes::Ahead))
         {
            release_library_entry( &m_LibObserver,
                                   pPier->GetConnectionLibraryEntry(pgsTypes::Ahead), 
                                   rconlib);
         }

         if ( pPier->GetNextSpan() != NULL )
            pPier = pPier->GetNextSpan()->GetNextPier();
         else
            pPier = NULL;
      }

      ReleaseGirderLibraryEntries();

      // traffic barrier
      const TrafficBarrierLibrary& rbarlib = m_pLibMgr->GetTrafficBarrierLibrary();

      if ( m_BridgeDescription.GetLeftRailingSystem()->GetExteriorRailing() )
         release_library_entry( &m_LibObserver,
                                m_BridgeDescription.GetLeftRailingSystem()->GetExteriorRailing(),
                                m_pLibMgr->GetTrafficBarrierLibrary());

      if ( m_BridgeDescription.GetLeftRailingSystem()->GetInteriorRailing() )
         release_library_entry( &m_LibObserver,
                                m_BridgeDescription.GetLeftRailingSystem()->GetInteriorRailing(),
                                m_pLibMgr->GetTrafficBarrierLibrary());


      if ( m_BridgeDescription.GetRightRailingSystem()->GetExteriorRailing() )
         release_library_entry( &m_LibObserver,
                                m_BridgeDescription.GetRightRailingSystem()->GetExteriorRailing(),
                                m_pLibMgr->GetTrafficBarrierLibrary());

      if ( m_BridgeDescription.GetRightRailingSystem()->GetInteriorRailing() )
         release_library_entry( &m_LibObserver,
                                m_BridgeDescription.GetRightRailingSystem()->GetInteriorRailing(),
                                m_pLibMgr->GetTrafficBarrierLibrary());
   }
}

void CProjectAgentImp::ReleaseGirderLibraryEntries()
{
   if ( m_pLibMgr )
   {
      // girder entry
      const GirderLibrary&  girderLibrary = m_pLibMgr->GetGirderLibrary();
      if (m_BridgeDescription.GetGirderLibraryEntry() != 0)
      {
         release_library_entry( &m_LibObserver,
                                m_BridgeDescription.GetGirderLibraryEntry(), 
                                girderLibrary);
      
      }

      CSpanData* pSpan = m_BridgeDescription.GetSpan(0);
      while ( pSpan )
      {
         CGirderTypes girderTypes = *pSpan->GetGirderTypes();
         GroupIndexType nGroups = girderTypes.GetGirderGroupCount();
         for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
         {
            GirderIndexType firstGdrIdx, lastGdrIdx;
            std::_tstring strGirderName;
            girderTypes.GetGirderGroup(grpIdx,&firstGdrIdx,&lastGdrIdx,strGirderName);
            for ( GirderIndexType gdrIdx = firstGdrIdx; gdrIdx <= lastGdrIdx; gdrIdx++ )
            {
               const GirderLibraryEntry* pEntry = girderTypes.GetGirderLibraryEntry(gdrIdx);
               if ( pEntry != NULL )
               {
                  release_library_entry( &m_LibObserver,
                                         pEntry, 
                                         girderLibrary);
               }
            }

            girderTypes.SetGirderLibraryEntry(grpIdx,NULL);
         }

         pSpan->SetGirderTypes(girderTypes);
         pSpan = pSpan->GetNextPier()->GetNextSpan();
      }

      // this must be done dead last because the GirderTypes could be referencing the
      // bridge's girder library entry
      m_BridgeDescription.SetGirderLibraryEntry(NULL);
   }
}

STDMETHODIMP CProjectAgentImp::Load(IStructuredLoad* pStrLoad)
{
   HRESULT hr = S_OK;

//   GET_IFACE( IProgress, pProgress );
//   CEAFAutoProgress ap(pProgress);
   IProgress* pProgress = 0; // progress window causes big trouble running in windowless mode

   // Load the library data first into a temporary library. Then deal with entry
   // conflict resolution.
   // This library manager contains data that has been removed from some library entries
   psgProjectLibraryManager temp_manager;
   try
   {
//      pProgress->UpdateMessage( _T(_T("Loading the Project Libraries")) );
      CStructuredLoad load( pStrLoad );
      if ( !temp_manager.LoadMe( &load ) )
         return E_FAIL;

      // recover parameters that are no longer part of the spec library entry
      for ( int i = 0; i < 6; i++ )
      {
         m_LoadFactors.DCmin[i]   = temp_manager.m_DCmin[i];
         m_LoadFactors.DWmin[i]   = temp_manager.m_DWmin[i];
         m_LoadFactors.LLIMmin[i] = temp_manager.m_LLIMmin[i];

         m_LoadFactors.DCmax[i]   = temp_manager.m_DCmax[i];
         m_LoadFactors.DWmax[i]   = temp_manager.m_DWmax[i];
         m_LoadFactors.LLIMmax[i] = temp_manager.m_LLIMmax[i];
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
         return E_FAIL;
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
      return E_FAIL;

   // load project data - conflicts not resolved yet
   STRSTG_LOAD( hr, pStrLoad, pProgress );
   if ( FAILED(hr) )
      return hr;

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

   pTempSpecEntry->Release();

   // resolve library name conflicts and update references
   if (!ResolveLibraryConflicts(the_conflict_list))
      return E_FAIL;

   // check to see if the bridge girder spacing type is correct
   // because of changes in the way girder spacing is modeled, the girder spacing
   // type can take on invalid values... this seems to be the only place to fix it
   //
   // This problem happens with the adjacent beams
   const GirderLibraryEntry* pEntry = m_BridgeDescription.GetSpan(0)->GetGirderTypes()->GetGirderLibraryEntry(0);
   CComPtr<IBeamFactory> beamFactory;
   pEntry->GetBeamFactory(&beamFactory);
   pgsTypes::SupportedBeamSpacings sbs = beamFactory->GetSupportedBeamSpacings();
   pgsTypes::SupportedBeamSpacings::iterator iter;
   bool bIsValidSpacingType = false;
   for ( iter = sbs.begin(); iter != sbs.end(); iter++ )
   {
      if ( m_BridgeDescription.GetGirderSpacingType() == *iter )
         bIsValidSpacingType = true;
   }

   if ( !bIsValidSpacingType )
   {
      m_BridgeDescription.SetGirderSpacingType(sbs[0]);
   }
   else
   {
      if ( IsJointSpacing(m_BridgeDescription.GetGirderSpacingType()) )
      {
         // input is girder spacing, but should be joint width
         GirderLibraryEntry::Dimensions dimensions = pEntry->GetDimensions();
         CComPtr<IGirderSection> gdrSection;
         beamFactory->CreateGirderSection(NULL,0,INVALID_INDEX,INVALID_INDEX,dimensions,&gdrSection);
         double topWidth, botWidth;
         gdrSection->get_TopWidth(&topWidth);
         gdrSection->get_BottomWidth(&botWidth);
         double width = _cpp_max(topWidth,botWidth);
         double jointWidth = m_BridgeDescription.GetGirderSpacing() - width;
         jointWidth = IsZero(jointWidth) ? 0 : jointWidth;

         if ( 0 <= jointWidth )
         {
            m_BridgeDescription.SetGirderSpacing(jointWidth);
            m_BridgeDescription.CopyDown(false,false,IsBridgeSpacing(m_BridgeDescription.GetGirderSpacingType()),false); // copy the joint spacing down
         }
      }
   }

   // old alignment offset was read... copy it to all the piers
   if ( m_bUseTempAlignmentOffset )
   {
      m_BridgeDescription.SetAlignmentOffset(m_AlignmentOffset_Temp);
   }

   // make sure no library problems snuck in the back door.
   DealWithGirderLibraryChanges(true);
   DealWithConnectionLibraryChanges(true);

   // Deal with missing library entries
   bool bUpdateLibraryUsage = false;

   ConcreteLibrary& concLib = GetConcreteLibrary();
   ATLASSERT(concLib.GetMinCount() == 0); // did this change???

   ConnectionLibrary& connLib = GetConnectionLibrary();
   CollectionIndexType nEntries    = connLib.GetCount();
   CollectionIndexType nMinEntries = connLib.GetMinCount();
   if ( nEntries < nMinEntries )
   {
      CString strMsg;
      strMsg.Format(_T("The %s library needs at least %d entries. Default entries have been created."),connLib.GetDisplayName().c_str(),nMinEntries);
      AfxMessageBox(strMsg,MB_OK | MB_ICONEXCLAMATION);
      for ( CollectionIndexType i = 0; i < (nMinEntries-nEntries); i++ )
      {
         connLib.NewEntry(connLib.GetUniqueEntryName().c_str());
      }

      libKeyListType keys;
      connLib.KeyList(keys);
      std::_tstring strConnectionName = keys.front();
      PierIndexType nPiers = m_BridgeDescription.GetPierCount();
      for ( PierIndexType pierIdx = 0; pierIdx < nPiers; pierIdx++ )
      {
         CPierData* pPier = m_BridgeDescription.GetPier(pierIdx);

         if ( pierIdx != 0 )
            pPier->SetConnection(pgsTypes::Back,strConnectionName.c_str());

         if ( pierIdx != nPiers-1 )
            pPier->SetConnection(pgsTypes::Ahead,strConnectionName.c_str());

         bUpdateLibraryUsage = true;
      }
   }

   GirderLibrary& gdrLib = GetGirderLibrary();
   nEntries    = gdrLib.GetCount();
   nMinEntries = gdrLib.GetMinCount();
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
         SpanIndexType nSpans = m_BridgeDescription.GetSpanCount();
         for ( SpanIndexType spanIdx = 0; spanIdx < nSpans; spanIdx++ )
         {
            CSpanData* pSpan = m_BridgeDescription.GetSpan(spanIdx);
            CGirderTypes* pGirderTypes = pSpan->GetGirderTypes();
            GroupIndexType nGroups = pGirderTypes->GetGirderGroupCount();
            for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
            {
               pGirderTypes->SetGirderName(grpIdx,strGirderName.c_str());
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
         pLeftRailing->strInteriorRailing = strBarrierName;

      CRailingSystem* pRightRailing = m_BridgeDescription.GetRightRailingSystem();
      pRightRailing->strExteriorRailing = strBarrierName;
      if ( pRightRailing->bUseInteriorRailing )
         pRightRailing->strInteriorRailing = strBarrierName;
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

   LiveLoadLibrary* pLiveLoadLib = GetLiveLoadLibrary();
   ATLASSERT(pLiveLoadLib->GetMinCount() == 0); // did this change???

   if ( bUpdateLibraryUsage )
   {
      ReleaseBridgeLibraryEntries();
      UseBridgeLibraryEntries();
   }

   SpecificationChanged(false);  
   RatingSpecificationChanged(false);

   ASSERTVALID;

   return hr;
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
      pProgress->UpdateMessage(_T("Saving the Project Libraries") );
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

void CProjectAgentImp::ValidateStrands(SpanIndexType span,GirderIndexType girder,CGirderData& girder_data,bool fromLibrary)
{
   const CSpanData* pSpan = m_BridgeDescription.GetSpan(span);
   const GirderLibraryEntry* pGirderEntry = pSpan->GetGirderTypes()->GetGirderLibraryEntry(girder);

   if (!pGirderEntry->IsVerticalAdjustmentAllowedEnd() && girder_data.PrestressData.HpOffsetAtEnd!=0.0 
                                                       && girder_data.PrestressData.HsoEndMeasurement!=hsoLEGACY)
   {
      girder_data.PrestressData.HpOffsetAtEnd = 0.0;
      girder_data.PrestressData.HsoEndMeasurement = hsoLEGACY;

      std::_tstring msg(_T("Vertical adjustment of harped strands at girder end reset to zero. Library entry forbids offset"));
      AddGirderStatusItem(span, girder, msg);
   }

   if (!pGirderEntry->IsVerticalAdjustmentAllowedHP() && girder_data.PrestressData.HpOffsetAtHp!=0.0 && girder_data.PrestressData.HsoHpMeasurement!=hsoLEGACY)
   {
      girder_data.PrestressData.HpOffsetAtHp = 0.0;
      girder_data.PrestressData.HsoHpMeasurement = hsoLEGACY;

      std::_tstring msg(_T("Vertical adjustment of harped strands at harping points reset to zero. Library entry forbids offset"));
      AddGirderStatusItem(span, girder, msg);
   }

   // There are many, many ways that strand data can get hosed if a library entry is changed for an existing project. 
   // If strands no longer fit as original, zero them out and inform user.
   bool clean = true;
   if (girder_data.PrestressData.GetNumPermStrandsType() == NPS_DIRECT_SELECTION)
   {
      // Direct Fill
      bool vst = IsValidStraightStrandFill(girder_data.PrestressData.GetDirectStrandFillStraight(), pGirderEntry);
      bool vhp = IsValidHarpedStrandFill(girder_data.PrestressData.GetDirectStrandFillHarped(), pGirderEntry);
      bool vtp = IsValidTemporaryStrandFill(girder_data.PrestressData.GetDirectStrandFillTemporary(), pGirderEntry);

      if ( !(vst&&vhp&&vtp) )
      {
         std::_tstring msg(_T("Direct filled strands no longer fit in girder because library entry changed. All strands were removed."));
         AddGirderStatusItem(span, girder, msg);

         clean = false;
         girder_data.PrestressData.ResetPrestressData();
      }

      // Check validity of debond data for direct filled strands
      bool debond_changed(false);
      if (! girder_data.PrestressData.Debond[pgsTypes::Straight].empty())
      {
         StrandIndexType nStrandCoordinates = pGirderEntry->GetNumStraightStrandCoordinates();

         // Make sure selected strands are filled and debondable
         std::vector<CDebondInfo>::iterator it    = girder_data.PrestressData.Debond[pgsTypes::Straight].begin();
         std::vector<CDebondInfo>::iterator itend = girder_data.PrestressData.Debond[pgsTypes::Straight].end();
         while(it!=itend)
         {
            bool can_db = true;

            // Get strand index and check if debonded strand is actually filled
            if( ! girder_data.PrestressData.GetDirectStrandFillStraight()->IsStrandFilled( it->strandTypeGridIdx ) )
            {
               can_db = false;
            }

            // Make sure strand fits in grid
            if (nStrandCoordinates <= it->strandTypeGridIdx)
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
               girder_data.PrestressData.Debond[pgsTypes::Straight].erase(it);

               debond_changed = true;

               // restart loop
               it    = girder_data.PrestressData.Debond[pgsTypes::Straight].begin();
               itend = girder_data.PrestressData.Debond[pgsTypes::Straight].end();
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
            msg<< _T(" Girder ")<<LABEL_GIRDER(girder)<<_T(", span ")<<LABEL_SPAN(span)<<_T(" specified debonding that is not allowed by the library entry. The invalid debond regions were removed.");
            AddGirderStatusItem(span, girder, msg.str());
         }
      }
   }
   else
   {
      if (girder_data.PrestressData.GetNumPermStrandsType() == NPS_TOTAL_NUMBER)
      {
         // make sure number of strands fits library
         StrandIndexType ns, nh;
         bool st = pGirderEntry->GetPermStrandDistribution(girder_data.PrestressData.GetNstrands(pgsTypes::Permanent), &ns, &nh);

         if (!st || girder_data.PrestressData.GetNstrands(pgsTypes::Straight) !=ns || girder_data.PrestressData.GetNstrands(pgsTypes::Harped) != nh)
         {
            std::_tostringstream msg;
            msg<< girder_data.PrestressData.GetNstrands(pgsTypes::Permanent)<<_T(" permanent strands no longer fit in girder ")<<LABEL_GIRDER(girder)<<_T(", span ")<<LABEL_SPAN(span)<<_T(" because library entry changed. All strands were removed.");
            AddGirderStatusItem(span, girder, msg.str());

            clean = false;
            girder_data.PrestressData.ResetPrestressData();
         }
      }
      else if (girder_data.PrestressData.GetNumPermStrandsType() == NPS_STRAIGHT_HARPED) // input is by straight/harped
      {
         bool vst = pGirderEntry->IsValidNumberOfStraightStrands(girder_data.PrestressData.GetNstrands(pgsTypes::Straight));
         bool vhp = pGirderEntry->IsValidNumberOfHarpedStrands(girder_data.PrestressData.GetNstrands(pgsTypes::Harped));

         if ( !(vst&&vhp) )
         {
            std::_tostringstream msg;
            msg<< girder_data.PrestressData.GetNstrands(pgsTypes::Straight)<<_T(" straight, ")<<girder_data.PrestressData.GetNstrands(pgsTypes::Harped)<<_T(" harped strands no longer fit in girder ")<<LABEL_GIRDER(girder)<<_T(", span ")<<LABEL_SPAN(span)<<_T(" because library entry changed. All strands were removed.");
            AddGirderStatusItem(span, girder, msg.str());

            clean = false;
            girder_data.PrestressData.ResetPrestressData();
         }
      }
      else
         ATLASSERT(0);

      // Temporary Strands
      bool vhp = pGirderEntry->IsValidNumberOfTemporaryStrands(girder_data.PrestressData.GetNstrands(pgsTypes::Temporary));
      if ( !vhp )
      {
         std::_tostringstream msg;
         msg<< girder_data.PrestressData.GetNstrands(pgsTypes::Temporary)<<_T(" temporary strands no longer fit in girder ")<<LABEL_GIRDER(girder)<<_T(", span ")<<LABEL_SPAN(span)<<_T(" because library entry changed. All temporary strands were removed.");
         AddGirderStatusItem(span, girder, msg.str());

         clean = false;
         girder_data.PrestressData.ResetPrestressData();
      }

      // Debond information for sequentially filled strands
      if (clean)
      {
         // check validity of debond data - this can come from library, or project changes
         if (! girder_data.PrestressData.Debond[pgsTypes::Straight].empty())
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
            std::vector<CDebondInfo>::iterator iter;
            for ( iter = girder_data.PrestressData.Debond[pgsTypes::Straight].begin(); iter != girder_data.PrestressData.Debond[pgsTypes::Straight].end(); )
            {
               bool good=true;
               CDebondInfo& debond_info = *iter;

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
                  iter = girder_data.PrestressData.Debond[pgsTypes::Straight].erase(iter);
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
               msg<< _T(" Girder ")<<LABEL_GIRDER(girder)<<_T(", span ")<<LABEL_SPAN(span)<<_T(" specified debonding that is not allowed by the library entry. The invalid debond regions were removed.");
               AddGirderStatusItem(span, girder, msg.str());
            }
         }
      }
   }

   if (clean)
   {
      // clear out status items on a clean pass, because we made them
      RemoveGirderStatusItems(span, girder);
   }
}

//////////////////////////////////////////////////////////////////////
// IProjectProperties
std::_tstring CProjectAgentImp::GetBridgeName() const
{
   return m_BridgeName;
}

void CProjectAgentImp::SetBridgeName(const std::_tstring& name)
{
   if ( m_BridgeName != name )
   {
      m_BridgeName = name;

      if ( m_bPropertyUpdatesEnabled )
         Fire_ProjectPropertiesChanged();
      else
         m_bPropertyUpdatesPending = true;
   }
}

std::_tstring CProjectAgentImp::GetBridgeId() const
{
   return m_BridgeId;
}

void CProjectAgentImp::SetBridgeId(const std::_tstring& bid)
{
   if ( m_BridgeId != bid )
   {
      m_BridgeId = bid;

      if ( m_bPropertyUpdatesEnabled )
         Fire_ProjectPropertiesChanged();
      else
         m_bPropertyUpdatesPending = true;
   }
}

std::_tstring CProjectAgentImp::GetJobNumber() const
{
   return m_JobNumber;
}

void CProjectAgentImp::SetJobNumber(const std::_tstring& jid)
{
   if ( m_JobNumber != jid )
   {
      m_JobNumber = jid;

      if ( m_bPropertyUpdatesEnabled )
         Fire_ProjectPropertiesChanged();
      else
         m_bPropertyUpdatesPending = true;
   }
}

std::_tstring CProjectAgentImp::GetEngineer() const
{
   return m_Engineer;
}

void CProjectAgentImp::SetEngineer(const std::_tstring& eng)
{
   if ( m_Engineer != eng )
   {
      m_Engineer = eng;

      if ( m_bPropertyUpdatesEnabled )
         Fire_ProjectPropertiesChanged();
      else
         m_bPropertyUpdatesPending = true;
   }
}

std::_tstring CProjectAgentImp::GetCompany() const
{
   return m_Company;
}

void CProjectAgentImp::SetCompany(const std::_tstring& company)
{
   if ( m_Company != company )
   {
      m_Company = company;

      if ( m_bPropertyUpdatesEnabled )
         Fire_ProjectPropertiesChanged();
      else
         m_bPropertyUpdatesPending = true;
   }
}

std::_tstring CProjectAgentImp::GetComments() const
{
   return m_Comments;
}

void CProjectAgentImp::SetComments(const std::_tstring& comments)
{
   if ( m_Comments != comments )
   {
      m_Comments = comments;

      if ( m_bPropertyUpdatesEnabled )
         Fire_ProjectPropertiesChanged();
      else
         m_bPropertyUpdatesPending = true;
   }
}

void CProjectAgentImp::EnableUpdate(bool bEnable)
{
   if ( m_bPropertyUpdatesEnabled != bEnable )
   {
      m_bPropertyUpdatesEnabled = bEnable;
      if ( m_bPropertyUpdatesEnabled && m_bPropertyUpdatesPending )
      {
         Fire_ProjectPropertiesChanged();
         m_bPropertyUpdatesPending = false;
      }
   }
}

bool CProjectAgentImp::IsUpdatedEnabled()
{
   return m_bPropertyUpdatesEnabled;
}

bool CProjectAgentImp::AreUpdatesPending()
{
   return m_bPropertyUpdatesPending;
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
   CHECK( 0 <= newVal && newVal <= 100 );

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
const CBridgeDescription* CProjectAgentImp::GetBridgeDescription()
{
   return &m_BridgeDescription;
}

void CProjectAgentImp::SetBridgeDescription(const CBridgeDescription& desc)
{
   if ( desc != m_BridgeDescription )
   {
      ReleaseBridgeLibraryEntries();

      m_BridgeDescription = desc;
      
      UseBridgeLibraryEntries();

      Fire_BridgeChanged();
   }
}

const CDeckDescription* CProjectAgentImp::GetDeckDescription()
{
   return m_BridgeDescription.GetDeckDescription();
}

void CProjectAgentImp::SetDeckDescription(const CDeckDescription& deck)
{
   if ( deck != (*m_BridgeDescription.GetDeckDescription()) )
   {
      *m_BridgeDescription.GetDeckDescription() = deck;
      Fire_BridgeChanged();
   }
}

const CSpanData* CProjectAgentImp::GetSpan(SpanIndexType spanIdx)
{
   return m_BridgeDescription.GetSpan(spanIdx);
}

void CProjectAgentImp::SetSpan(SpanIndexType spanIdx,const CSpanData& spanData)
{
   if ( *m_BridgeDescription.GetSpan(spanIdx) != spanData )
   {
      ReleaseBridgeLibraryEntries();

      *m_BridgeDescription.GetSpan(spanIdx) = spanData;
      
      UseBridgeLibraryEntries();

      Fire_BridgeChanged();
   }
}

const CPierData* CProjectAgentImp::GetPier(PierIndexType pierIdx)
{
   return m_BridgeDescription.GetPier(pierIdx);
}

void CProjectAgentImp::SetPier(PierIndexType pierIdx,const CPierData& pierData)
{
   if ( *m_BridgeDescription.GetPier(pierIdx) != pierData )
   {
      ReleaseBridgeLibraryEntries();
      
      *m_BridgeDescription.GetPier(pierIdx) = pierData;

      UseBridgeLibraryEntries();
      
      Fire_BridgeChanged();
   }
}

void CProjectAgentImp::SetSpanLength(SpanIndexType spanIdx,double newLength)
{
   if ( m_BridgeDescription.SetSpanLength(spanIdx,newLength) )
      Fire_BridgeChanged();
}

void CProjectAgentImp::MovePier(PierIndexType pierIdx,double newStation,pgsTypes::MovePierOption moveOption)
{
   if ( m_BridgeDescription.MovePier(pierIdx,newStation,moveOption) )
      Fire_BridgeChanged();
}

void CProjectAgentImp::SetMeasurementType(PierIndexType pierIdx,pgsTypes::PierFaceType pierFace,pgsTypes::MeasurementType mt)
{
   CPierData* pPier = m_BridgeDescription.GetPier(pierIdx);

   pgsTypes::MeasurementType measureType;
   if ( pierFace == pgsTypes::Ahead )
   {
      measureType = pPier->GetNextSpan()->GetGirderSpacing(pgsTypes::metStart)->GetMeasurementType();
   }
   else
   {
      measureType = pPier->GetPrevSpan()->GetGirderSpacing(pgsTypes::metEnd)->GetMeasurementType();
   }

   if ( measureType != mt )
   {
      if ( pierFace == pgsTypes::Ahead )
      {
         pPier->GetNextSpan()->GetGirderSpacing(pgsTypes::metStart)->SetMeasurementType(mt);
      }
      else
      {
         pPier->GetPrevSpan()->GetGirderSpacing(pgsTypes::metEnd)->SetMeasurementType(mt);
      }
      Fire_BridgeChanged();
   }
}

void CProjectAgentImp::SetMeasurementLocation(PierIndexType pierIdx,pgsTypes::PierFaceType pierFace,pgsTypes::MeasurementLocation ml)
{
   CPierData* pPier = m_BridgeDescription.GetPier(pierIdx);

   pgsTypes::MeasurementLocation measureLocation;
   if ( pierFace == pgsTypes::Ahead )
   {
      measureLocation = pPier->GetNextSpan()->GetGirderSpacing(pgsTypes::metStart)->GetMeasurementLocation();
   }
   else
   {
      measureLocation = pPier->GetPrevSpan()->GetGirderSpacing(pgsTypes::metEnd)->GetMeasurementLocation();
   }

   if ( measureLocation != ml )
   {
      if ( pierFace == pgsTypes::Ahead )
      {
         pPier->GetNextSpan()->GetGirderSpacing(pgsTypes::metStart)->SetMeasurementLocation(ml);
      }
      else
      {
         pPier->GetPrevSpan()->GetGirderSpacing(pgsTypes::metEnd)->SetMeasurementLocation(ml);
      }
      Fire_BridgeChanged();
   }
}

void CProjectAgentImp::SetSlabOffset( Float64 slabOffset)
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

void CProjectAgentImp::SetSlabOffset( SpanIndexType spanIdx, Float64 start, Float64 end)
{
   if ( m_BridgeDescription.GetSlabOffsetType() != pgsTypes::sotSpan ||
       (!IsEqual(start,m_BridgeDescription.GetSpan(spanIdx)->GetSlabOffset(pgsTypes::metStart)) ||
        !IsEqual(end,  m_BridgeDescription.GetSpan(spanIdx)->GetSlabOffset(pgsTypes::metEnd)) )
      )
   {
      m_BridgeDescription.SetSlabOffsetType(pgsTypes::sotSpan);
      m_BridgeDescription.GetSpan(spanIdx)->SetSlabOffset(pgsTypes::metStart,start);
      m_BridgeDescription.GetSpan(spanIdx)->SetSlabOffset(pgsTypes::metEnd,  end);

      Fire_BridgeChanged();
   }
}

void CProjectAgentImp::SetSlabOffset( SpanIndexType spanIdx, GirderIndexType gdrIdx, Float64 start, Float64 end)
{
   // slab offset type is changing... are slab offsets values changing
   CGirderTypes* pGirderTypes = m_BridgeDescription.GetSpan(spanIdx)->GetGirderTypes();
   if ( m_BridgeDescription.GetSlabOffsetType() != pgsTypes::sotGirder || // offset type changed
       ( m_BridgeDescription.GetSlabOffsetType() == pgsTypes::sotGirder && // offset type same
         (!IsEqual(pGirderTypes->GetSlabOffset(gdrIdx,pgsTypes::metStart),start) || // and offset changed
          !IsEqual(pGirderTypes->GetSlabOffset(gdrIdx,pgsTypes::metEnd),end))
        )
      )
   {
      // slab offset values are changing too
      m_BridgeDescription.SetSlabOffsetType(pgsTypes::sotGirder);
      pGirderTypes->SetSlabOffset(gdrIdx,pgsTypes::metStart,start);
      pGirderTypes->SetSlabOffset(gdrIdx,pgsTypes::metEnd,  end);
      Fire_BridgeChanged();
   }
}

pgsTypes::SlabOffsetType CProjectAgentImp::GetSlabOffsetType()
{
   return m_BridgeDescription.GetSlabOffsetType();
}

void CProjectAgentImp::GetSlabOffset( SpanIndexType spanIdx, GirderIndexType gdrIdx, Float64* pStart, Float64* pEnd)
{
   CGirderTypes* pGirderTypes = m_BridgeDescription.GetSpan(spanIdx)->GetGirderTypes();
   *pStart = pGirderTypes->GetSlabOffset(gdrIdx,pgsTypes::metStart);
   *pEnd   = pGirderTypes->GetSlabOffset(gdrIdx,pgsTypes::metEnd);
}

void CProjectAgentImp::SetGirderSpacing(PierIndexType pierIdx,pgsTypes::PierFaceType face,const CGirderSpacing& spacing)
{
   CPierData* pPier = m_BridgeDescription.GetPier(pierIdx);
   CGirderSpacing* pSpacing;
   
   if ( face == pgsTypes::Ahead )
   {
      pSpacing = pPier->GetNextSpan()->GetGirderSpacing(pgsTypes::metStart);
   }
   else
   {
      pSpacing = pPier->GetPrevSpan()->GetGirderSpacing(pgsTypes::metEnd);
   }

   if ( *pSpacing != spacing )
   {
      *pSpacing = spacing;
      Fire_BridgeChanged();
   }
}

void CProjectAgentImp::SetGirderSpacingAtStartOfSpan(SpanIndexType spanIdx,const CGirderSpacing& spacing)
{
   CSpanData* pSpan = m_BridgeDescription.GetSpan(spanIdx);
   if ( *pSpan->GetGirderSpacing(pgsTypes::metStart) != spacing )
   {
      *pSpan->GetGirderSpacing(pgsTypes::metStart) = spacing;
      Fire_BridgeChanged();
   }
}

void CProjectAgentImp::SetGirderSpacingAtEndOfSpan(SpanIndexType spanIdx,const CGirderSpacing& spacing)
{
   CSpanData* pSpan = m_BridgeDescription.GetSpan(spanIdx);
   if ( *pSpan->GetGirderSpacing(pgsTypes::metEnd) != spacing )
   {
      *pSpan->GetGirderSpacing(pgsTypes::metEnd) = spacing;
      Fire_BridgeChanged();
   }
}

void CProjectAgentImp::UseSameGirderSpacingAtBothEndsOfSpan(SpanIndexType spanIdx,bool bUseSame)
{
   CSpanData* pSpan = m_BridgeDescription.GetSpan(spanIdx);
   if ( pSpan->UseSameSpacingAtBothEndsOfSpan() != bUseSame )
   {
      pSpan->UseSameSpacingAtBothEndsOfSpan(bUseSame);
      Fire_BridgeChanged();
   }
}

void CProjectAgentImp::SetGirderTypes(SpanIndexType spanIdx,const CGirderTypes& newGirderTypes)
{
   CSpanData* pSpan = m_BridgeDescription.GetSpan(spanIdx);
   CGirderTypes girderTypes = *pSpan->GetGirderTypes();
   if ( girderTypes != newGirderTypes )
   {
      ReleaseBridgeLibraryEntries();
      pSpan->SetGirderTypes(newGirderTypes);
      UseBridgeLibraryEntries();

      Fire_BridgeChanged();
   }
}

void CProjectAgentImp::SetGirderName( SpanIndexType spanIdx, GirderIndexType gdrIdx, LPCTSTR strGirderName)
{
   ATLASSERT( !m_BridgeDescription.UseSameGirderForEntireBridge() );
   CSpanData* pSpan = m_BridgeDescription.GetSpan(spanIdx);
   CGirderTypes girderTypes = *pSpan->GetGirderTypes();

   if ( std::_tstring(strGirderName) != girderTypes.GetGirderName(gdrIdx) )
   {
      ReleaseGirderLibraryEntries();

      girderTypes = *pSpan->GetGirderTypes();
      GroupIndexType grpIdx = girderTypes.CreateGroup(gdrIdx,gdrIdx);
      girderTypes.SetGirderName(grpIdx,strGirderName);

      pSpan->SetGirderTypes(girderTypes);

      UseGirderLibraryEntries();

#if defined _DEBUG
      girderTypes = *pSpan->GetGirderTypes();
      ATLASSERT( girderTypes.GetGirderName(gdrIdx) == girderTypes.GetGirderLibraryEntry(gdrIdx)->GetName() );
#endif

      Fire_BridgeChanged();
   }
}

void CProjectAgentImp::SetGirderCount(SpanIndexType spanIdx,GirderIndexType nGirders)
{
   ReleaseGirderLibraryEntries();
   
   CSpanData* pSpan = m_BridgeDescription.GetSpan(spanIdx);
   pSpan->SetGirderCount(nGirders);

   UseGirderLibraryEntries();
   Fire_BridgeChanged();
}

void CProjectAgentImp::SetBoundaryCondition(PierIndexType pierIdx,pgsTypes::PierConnectionType connectionType)
{
   CPierData* pPier = m_BridgeDescription.GetPier(pierIdx);
   if ( pPier->GetConnectionType() != connectionType )
   {
      pPier->SetConnectionType(connectionType);
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

   Fire_BridgeChanged();
}

void CProjectAgentImp::InsertSpan(PierIndexType refPierIdx,pgsTypes::PierFaceType pierFace,Float64 spanLength,const CSpanData* pSpanData,const CPierData* pPierData)
{
   ReleaseBridgeLibraryEntries();
   
   m_BridgeDescription.InsertSpan(refPierIdx,pierFace,spanLength,pSpanData,pPierData);
   
   UseBridgeLibraryEntries();

   Fire_BridgeChanged();
}

void CProjectAgentImp::SetLiveLoadDistributionFactorMethod(pgsTypes::DistributionFactorMethod method)
{
   if ( m_BridgeDescription.GetDistributionFactorMethod() != method )
   {
      m_BridgeDescription.SetDistributionFactorMethod(method);
      Fire_BridgeChanged();
   }
}

pgsTypes::DistributionFactorMethod CProjectAgentImp::GetLiveLoadDistributionFactorMethod()
{
   return m_BridgeDescription.GetDistributionFactorMethod();
}

void CProjectAgentImp::UseSameNumberOfGirdersInAllSpans(bool bUseSame)
{
   if ( m_BridgeDescription.UseSameNumberOfGirdersInAllSpans() != bUseSame )
   {
      m_BridgeDescription.UseSameNumberOfGirdersInAllSpans(bUseSame);
      Fire_BridgeChanged();
   }
}

bool CProjectAgentImp::UseSameNumberOfGirdersInAllSpans()
{
   return m_BridgeDescription.UseSameNumberOfGirdersInAllSpans();
}

void CProjectAgentImp::SetGirderCount(GirderIndexType nGirders)
{
   if ( m_BridgeDescription.GetGirderCount() != nGirders )
   {
      m_BridgeDescription.SetGirderCount(nGirders);
      if ( m_BridgeDescription.UseSameNumberOfGirdersInAllSpans() )
         Fire_BridgeChanged();
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

bool CProjectAgentImp::UseSameGirderForEntireBridge()
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
         Fire_BridgeChanged();
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

pgsTypes::SupportedBeamSpacing CProjectAgentImp::GetGirderSpacingType()
{
   return m_BridgeDescription.GetGirderSpacingType();
}

void CProjectAgentImp::SetGirderSpacing(double spacing)
{
   if ( !IsEqual(m_BridgeDescription.GetGirderSpacing(),spacing) )
   {
      m_BridgeDescription.SetGirderSpacing(spacing);
      if ( IsBridgeSpacing(m_BridgeDescription.GetGirderSpacingType()) )
         Fire_BridgeChanged();
   }
}

void CProjectAgentImp::SetMeasurementType(pgsTypes::MeasurementType mt)
{
   if ( m_BridgeDescription.GetMeasurementType() != mt )
   {
      m_BridgeDescription.SetMeasurementType(mt);
      if ( IsBridgeSpacing(m_BridgeDescription.GetGirderSpacingType()) )
         Fire_BridgeChanged();
   }
}

pgsTypes::MeasurementType CProjectAgentImp::GetMeasurementType()
{
   return m_BridgeDescription.GetMeasurementType();
}


void CProjectAgentImp::SetMeasurementLocation(pgsTypes::MeasurementLocation ml)
{
   if ( m_BridgeDescription.GetMeasurementLocation() != ml )
   {
      m_BridgeDescription.SetMeasurementLocation(ml);
      if ( IsBridgeSpacing(m_BridgeDescription.GetGirderSpacingType()) )
         Fire_BridgeChanged();
   }
}

pgsTypes::MeasurementLocation  CProjectAgentImp::GetMeasurementLocation()
{
   return  m_BridgeDescription.GetMeasurementLocation();
}

//bool CProjectAgentImp::CanModelPostTensioning()
//{
//   // post-tensioning be modeled only when the following conditions are true
//   // 1) the same number of girders are used in all spans
//   // 2) the same girder spacing is used on both sides of intermediate piers
//
//   bool bCanModelPostTensioning = false;
//
//   bool bSameNumberOfGirdersInAllSpans = m_BridgeDescription.UseSameNumberOfGirdersInAllSpans();
//   if ( !bSameNumberOfGirdersInAllSpans )
//   {
//      // the global setting is for different number of girders in each span... 
//      // we will go span by span and check the girder counts
//
//      bSameNumberOfGirdersInAllSpans = true; // assume they are all the same
//      const CSpanData* pSpan = m_BridgeDescription.GetSpan(0);
//      GirderIndexType nGirders = pSpan->GetGirderCount();
//      while ( pSpan->GetNextPier() && pSpan->GetNextPier()->GetNextSpan() )
//      {
//         pSpan = pSpan->GetNextPier()->GetNextSpan();
//         if ( nGirders != pSpan->GetGirderCount() )
//         {
//            bSameNumberOfGirdersInAllSpans = false;
//            break;
//         }
//      }
//   }
//
//   bool bSameSpacingAtPiers = false;
//   if ( IsBridgeSpacing(m_BridgeDescription.GetGirderSpacingType()) )
//   {
//      bSameSpacingAtPiers = true;
//   }
//   else
//   {
//      const CPierData* pPierData = m_BridgeDescription.GetPier(1); // first interior pier
//      while ( pPierData->GetNextSpan() != NULL )
//      {
//         // this is an intermediate pier
//         const CGirderSpacing* pBackGirderSpacing  = pPierData->GetPrevSpan()->GetGirderSpacing(pgsTypes::metEnd);
//         const CGirderSpacing* pAheadGirderSpacing = pPierData->GetNextSpan()->GetGirderSpacing(pgsTypes::metStart);
//
//         if ( (*pBackGirderSpacing) != (*pAheadGirderSpacing) )
//         {
//            bSameSpacingAtPiers = false;
//            break; // no need to continue if it isn't true for any one pier
//         }
//
//         pPierData = pPierData->GetNextSpan()->GetNextPier();
//      }
//   }
//
//   if ( bSameNumberOfGirdersInAllSpans && bSameSpacingAtPiers )
//   {
//      bCanModelPostTensioning = true;
//   }
//
//   return bCanModelPostTensioning;
//}
//
//bool CProjectAgentImp::IsPostTensioningModeled()
//{
//   // returning true because I am simulating that we have PT data so
//   // the can be developed
//   // return the actual state based on input date
//   return false;
//}
//
//void CProjectAgentImp::ConfigureBridgeForPostTensioning()
//{
//   // NOTE: This is the first attempt at putting editing into the agent that
//   // owns the data rather that in the document class.
//   //
//   // If a UI needs to be displayed, the UI comes from this agent
//   //
//   // One thing that is glaringly obvious is that the main PGSuper architecture needs
//   // a global transaction stack for undoable transactions. For now, the transaction
//   // has to be handled by the caller or this cannot be undone
//
//   ///////////////////////////////////////////////////////////////////////////////////////////
//
//   // post-tensioning be modeled only when the following conditions are true
//   // 1) the same number of girders are used in all spans
//   // 2) the same girder spacing is used on both sides of intermediate piers
//
//   // if the bridge is in good shape, get the heck outta here
//   if ( CanModelPostTensioning() )
//   {
//      ATLASSERT(false); // if everything is ok, why is this function being called?
//      return;
//   }
//
//   // NOTE: This is a brute force kind of edit... future versions may ask the user questions
//   // so that a more customized change can be made
//
//   bool bSameNumberOfGirdersInAllSpans = m_BridgeDescription.UseSameNumberOfGirdersInAllSpans();
//   GirderIndexType nGirdersMin = 999;
//   if ( !bSameNumberOfGirdersInAllSpans )
//   {
//      // the global setting is for different number of girders in each span... 
//      // we will go span by span and check the girder counts
//
//      bSameNumberOfGirdersInAllSpans = true; // assume they are all the same
//      const CSpanData* pSpan = m_BridgeDescription.GetSpan(0);
//      GirderIndexType nGirders = pSpan->GetGirderCount();
//      nGirdersMin = min(nGirdersMin,nGirders);
//
//      while ( pSpan->GetNextPier() && pSpan->GetNextPier()->GetNextSpan() )
//      {
//         pSpan = pSpan->GetNextPier()->GetNextSpan();
//         if ( nGirders != pSpan->GetGirderCount() )
//         {
//            bSameNumberOfGirdersInAllSpans = false;
//            nGirdersMin = min(nGirdersMin,pSpan->GetGirderCount());
//         }
//      }
//   }
//
//   CBridgeDescription newBridgeDesc = m_BridgeDescription;
//
//   // force number of girders to be the minimum found in any span
//   newBridgeDesc.UseSameNumberOfGirdersInAllSpans(true);
//   newBridgeDesc.SetGirderCount(nGirdersMin);
//
//   if ( newBridgeDesc.GetGirderSpacingType() != pgsTypes::sbsUniform )
//   {
//      // a single girder spacing is NOT used for the entire bridge
//      // get the girder spacing on the ahead side of pier 1 (end of span 0)
//      // and use that spacing everywhere
//
//      const CPierData* pPierData = newBridgeDesc.GetPier(1); // first interior pier
//      const CGirderSpacing* pGirderSpacing  = pPierData->GetPrevSpan()->GetGirderSpacing(pgsTypes::metEnd);
//
//      CSpanData* pSpanData = newBridgeDesc.GetSpan(0);
//      while ( pSpanData->GetNextPier()->GetNextSpan() )
//      {
//         *pSpanData->GetGirderSpacing(pgsTypes::metStart) = *pGirderSpacing;
//         *pSpanData->GetGirderSpacing(pgsTypes::metEnd)   = *pGirderSpacing;
//      }
//   }
//
//   // execute as a transaction so that this edit can be undone
//   GET_IFACE(IEAFTransactions,pTransactions);
//   txnEditBridgeDescription* pTxn = new txnEditBridgeDescription(m_pBroker,m_BridgeDescription,newBridgeDesc);
//   pTransactions->Execute(pTxn);
//}
//
//bool CProjectAgentImp::CanBePostTensioned(SpanIndexType spanIdx,GirderIndexType gdrIdx)
//{
//   const CSpanData* pSpan = m_BridgeDescription.GetSpan(spanIdx);
//   const GirderLibraryEntry* pGirderEntry = pSpan->GetGirderTypes()->GetGirderLibraryEntry(gdrIdx);
//
//   return pGirderEntry->CanPostTension();
//}

////////////////////////////////////////////////////////////////////////
// IGirderData Methods
//
const matPsStrand* CProjectAgentImp::GetStrandMaterial(SpanIndexType span,GirderIndexType gdr,pgsTypes::StrandType type) const
{
   if ( type == pgsTypes::Permanent )
   {
      ATLASSERT(m_BridgeDescription.GetSpan(span)->GetGirderTypes()->GetGirderData(gdr).Material.pStrandMaterial[pgsTypes::Straight] == m_BridgeDescription.GetSpan(span)->GetGirderTypes()->GetGirderData(gdr).Material.pStrandMaterial[pgsTypes::Harped]);
      type = pgsTypes::Straight;
   }

   const CGirderData& girder_data = m_BridgeDescription.GetSpan(span)->GetGirderTypes()->GetGirderData(gdr);
   return girder_data.Material.pStrandMaterial[type];
}

void CProjectAgentImp::SetStrandMaterial(SpanIndexType span,GirderIndexType gdr,pgsTypes::StrandType type,const matPsStrand* pmat)
{
   CHECK(pmat!=0);

   if ( type == pgsTypes::Permanent )
   {
      ATLASSERT(m_BridgeDescription.GetSpan(span)->GetGirderTypes()->GetGirderData(gdr).Material.pStrandMaterial[pgsTypes::Straight] == m_BridgeDescription.GetSpan(span)->GetGirderTypes()->GetGirderData(gdr).Material.pStrandMaterial[pgsTypes::Harped]);
      type = pgsTypes::Straight;
   }

   CSpanData* pSpan = m_BridgeDescription.GetSpan(span);
   CGirderTypes girderTypes = *pSpan->GetGirderTypes();
   CGirderData& girder_data = girderTypes.GetGirderData(gdr);

   if ( pmat != girder_data.Material.pStrandMaterial[type] )
   {
      girder_data.Material.pStrandMaterial[type] = pmat;
      pSpan->SetGirderTypes(girderTypes);

      m_bUpdateJackingForce = true;

      Fire_GirderChanged(span,gdr,GCH_STRAND_MATERIAL);
   }
}

Float64 CProjectAgentImp::GetMaxPjack(SpanIndexType span,GirderIndexType gdr,pgsTypes::StrandType type,StrandIndexType nStrands) const
{
   if ( type == pgsTypes::Permanent )
      type = pgsTypes::Straight;

   GET_IFACE(IPrestressForce,pPrestress);
   return pPrestress->GetPjackMax(span,gdr,*GetStrandMaterial(span,gdr,type),nStrands);
}

Float64 CProjectAgentImp::GetMaxPjack(SpanIndexType span,GirderIndexType gdr,StrandIndexType nStrands,const matPsStrand* pStrand) const
{
   GET_IFACE(IPrestressForce,pPrestress);
   return pPrestress->GetPjackMax(span,gdr,*pStrand,nStrands);
}

const CGirderData* CProjectAgentImp::GetGirderData(SpanIndexType span,GirderIndexType gdr) const
{
   if ( m_bUpdateJackingForce )
      UpdateJackingForce();

   const CGirderTypes* pgirderTypes = m_BridgeDescription.GetSpan(span)->GetGirderTypes();
   const CGirderData& rgd = pgirderTypes->GetGirderData(gdr);
   return &rgd;
}

const CGirderMaterial* CProjectAgentImp::GetGirderMaterial(SpanIndexType span,GirderIndexType gdr) const
{
   return &m_BridgeDescription.GetSpan(span)->GetGirderTypes()->GetGirderData(gdr).Material;
}

void CProjectAgentImp::SetGirderMaterial(SpanIndexType span,GirderIndexType gdr,const CGirderMaterial& material)
{
   CSpanData* pSpan = m_BridgeDescription.GetSpan(span);
   CGirderTypes girderTypes = *pSpan->GetGirderTypes();
   girderTypes.GetGirderData(gdr).Material = material;
   pSpan->SetGirderTypes(girderTypes);
}

bool CProjectAgentImp::SetGirderData(const CGirderData& data,SpanIndexType span,GirderIndexType gdr)
{
   int change_type = DoSetGirderData(data,span, gdr);

   Uint32 lHint = 0;

   if (change_type & CGirderData::ctConcrete)
   {
      lHint |= GCH_CONCRETE;
   }

   if ( change_type & CGirderData::ctPrestress )
   {
      lHint |= GCH_PRESTRESSING_CONFIGURATION;
   }

   if ( change_type & CGirderData::ctStrand )
   {
      lHint |= GCH_STRAND_MATERIAL;
   }

   if ( change_type & CGirderData::ctShearData )
   {
      lHint |= GCH_STIRRUPS;
   }

   if ( change_type & CGirderData::ctLongRebar )
   {
      lHint |= GCH_LONGITUDINAL_REBAR;
   }

   Fire_GirderChanged(span,gdr,lHint);

   return true;
}

int CProjectAgentImp::DoSetGirderData(const CGirderData& const_data,SpanIndexType span,GirderIndexType gdr)
{

   CGirderData data = const_data;

   const CSpanData* pSpan = m_BridgeDescription.GetSpan(span);
   const GirderLibraryEntry* pGdrEntry = pSpan->GetGirderTypes()->GetGirderLibraryEntry(gdr);

   // This code is here to handle legacy data from back when only one method was used to input strands
   // Convert total number of permanent strands to num straight - num harped data
   if (data.PrestressData.GetNumPermStrandsType()==NPS_TOTAL_NUMBER)
   {

      StrandIndexType ns, nh, np;
      if (pGdrEntry->GetPermStrandDistribution(data.PrestressData.GetNstrands(pgsTypes::Permanent), &ns, &nh))
      {
         np = ns + nh;
         data.PrestressData.SetTotalPermanentNstrands(np, ns, nh);

         if (np > 0)
         {
            data.PrestressData.Pjack[pgsTypes::Straight] = data.PrestressData.Pjack[pgsTypes::Permanent] * (Float64)ns/np;
            data.PrestressData.Pjack[pgsTypes::Harped]   = data.PrestressData.Pjack[pgsTypes::Permanent] * (Float64)nh/np;
         }
         else
         {
            data.PrestressData.Pjack[pgsTypes::Straight] = 0.0;
            data.PrestressData.Pjack[pgsTypes::Harped]   = 0.0;
         }

         data.PrestressData.bPjackCalculated[pgsTypes::Straight] = data.PrestressData.bPjackCalculated[pgsTypes::Permanent];
         data.PrestressData.bPjackCalculated[pgsTypes::Harped]   = data.PrestressData.bPjackCalculated[pgsTypes::Permanent];

         if ( !data.PrestressData.bPjackCalculated[pgsTypes::Permanent] )
            data.PrestressData.LastUserPjack[pgsTypes::Permanent] = data.PrestressData.Pjack[pgsTypes::Permanent];

      }
      else
      {
         // This is a fail, but let it pass through to ValidateStrands to handle it
      }
   }
   else
   {
      for ( Uint16 is = 0; is < 3; is++ )
      {
         if ( !data.PrestressData.bPjackCalculated[is] )
            data.PrestressData.LastUserPjack[is] = data.PrestressData.Pjack[is];
      }
   }

   // clean up any bad data that my have gotten into strand data
   ValidateStrands(span,gdr,data,false);

   // store data
   const CGirderData* pOriginalGirderData = GetGirderData(span,gdr);
   int change_type = data.GetChangeType(*pOriginalGirderData);

   if ( change_type != CGirderData::ctNone )
   {
      CGirderTypes girderTypes = *m_BridgeDescription.GetSpan(span)->GetGirderTypes();
      girderTypes.SetGirderData(gdr,data);
      m_BridgeDescription.GetSpan(span)->SetGirderTypes(girderTypes);
   }

   if ( change_type & CGirderData::ctStrand )
   {
      // the strand material was changed so update the jacking force
      UpdateJackingForce(span,gdr);
   }

   return change_type;
}

void CProjectAgentImp::ConvertLegacyDebondData(CGirderData& gdrData, const GirderLibraryEntry* pGdrEntry)
{
   // legacy versions only had straight debonding
   std::vector<CDebondInfo>& rdebonds = gdrData.PrestressData.Debond[pgsTypes::Straight];

   std::vector<CDebondInfo>::iterator it = rdebonds.begin();
   std::vector<CDebondInfo>::iterator itend = rdebonds.end();
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
         ATLASSERT(0);
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

void CProjectAgentImp::ConvertLegacyExtendedStrandData(CGirderData& gdrData, const GirderLibraryEntry* pGdrEntry)
{
   if ( gdrData.PrestressData.bConvertExtendedStrands == false )
      return; // no conversion needed

   for ( int i = 0; i < 2; i++ )
   {
      pgsTypes::MemberEndType endType = (pgsTypes::MemberEndType)i;

      // legacy versions only had straight extended strands
      const std::vector<GridIndexType>& oldExtendStrands( gdrData.PrestressData.GetExtendedStrands(pgsTypes::Straight,endType) );
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
      gdrData.PrestressData.SetExtendedStrands(pgsTypes::Straight,endType,newExtendedStrands);
   }
}

////////////////////////////////////////////////////////////////////////
// IShear Methods
//
std::_tstring CProjectAgentImp::GetStirrupMaterial(SpanIndexType span,GirderIndexType gdr) const
{
   // All shear bars have the same type (limited by the PGSuper interface)
   // so just use the confinement bar
   CShearData shearData = GetShearData(span,gdr);
   return lrfdRebarPool::GetMaterialName(shearData.ShearBarType,shearData.ShearBarGrade);
}

void CProjectAgentImp::GetStirrupMaterial(SpanIndexType spanIdx,GirderIndexType gdrIdx,matRebar::Type& type,matRebar::Grade& grade)
{
   CShearData shearData = GetShearData(spanIdx,gdrIdx);
   type = shearData.ShearBarType;
   grade = shearData.ShearBarGrade;
}

void CProjectAgentImp::SetStirrupMaterial(SpanIndexType span,GirderIndexType gdr,matRebar::Type type,matRebar::Grade grade)
{
   CShearData shearData = GetShearData(span,gdr);
   if (  shearData.ShearBarType != type || shearData.ShearBarGrade != grade )
   {
      shearData.ShearBarType = type;
      shearData.ShearBarGrade = grade;
      DoSetShearData(shearData,span,gdr);
      Fire_GirderChanged(span,gdr,GCH_STIRRUPS);
   }
}

CShearData CProjectAgentImp::GetShearData(SpanIndexType span,GirderIndexType gdr) const
{
   const CGirderTypes* pgirderTypes = m_BridgeDescription.GetSpan(span)->GetGirderTypes();
   return pgirderTypes->GetGirderData(gdr).ShearData;
}

bool CProjectAgentImp::SetShearData(const CShearData& data,SpanIndexType span,GirderIndexType gdr)
{
   if (DoSetShearData(data,span,gdr))
   {
      Fire_GirderChanged(span,gdr,GCH_STIRRUPS);
   }

   return true;
}

bool CProjectAgentImp::DoSetShearData(const CShearData& data,SpanIndexType span,GirderIndexType gdr)
{
   CSpanData* pSpan = m_BridgeDescription.GetSpan(span);
   CGirderTypes girderTypes = *pSpan->GetGirderTypes();
   girderTypes.GetGirderData(gdr).ShearData = data;
   pSpan->SetGirderTypes(girderTypes);
   return true;
}

/////////////////////////////////////////////////
// ILongitudinalRebar
std::_tstring CProjectAgentImp::GetLongitudinalRebarMaterial(SpanIndexType span,GirderIndexType gdr) const
{
   CLongitudinalRebarData lrd = GetLongitudinalRebarData(span,gdr);
   return lrfdRebarPool::GetMaterialName(lrd.BarType,lrd.BarGrade);
}

void CProjectAgentImp::GetLongitudinalRebarMaterial(SpanIndexType spanIdx,GirderIndexType gdrIdx,matRebar::Type& type,matRebar::Grade& grade)
{
   CLongitudinalRebarData lrd = GetLongitudinalRebarData(spanIdx,gdrIdx);
   grade = lrd.BarGrade;
   type = lrd.BarType;
}

void CProjectAgentImp::SetLongitudinalRebarMaterial(SpanIndexType span,GirderIndexType gdr,matRebar::Type type,matRebar::Grade grade)
{
   CLongitudinalRebarData lrd = GetLongitudinalRebarData(span,gdr);
   if ( lrd.BarGrade != grade || lrd.BarType != type )
   {
      lrd.BarGrade = grade;
      lrd.BarType = type;
      DoSetLongitudinalRebarData(lrd,span,gdr);
      Fire_GirderChanged(span,gdr,GCH_LONGITUDINAL_REBAR);
   }
}

CLongitudinalRebarData CProjectAgentImp::GetLongitudinalRebarData(SpanIndexType span,GirderIndexType gdr) const
{
   CSpanData* pSpan = m_BridgeDescription.GetSpan(span);
   const CGirderTypes* pgirderTypes = pSpan->GetGirderTypes();

   CLongitudinalRebarData lrd = pgirderTypes->GetGirderData(gdr).LongitudinalRebarData;
   return lrd;
}

bool CProjectAgentImp::SetLongitudinalRebarData(const CLongitudinalRebarData& data,SpanIndexType span,GirderIndexType gdr)
{
   if (DoSetLongitudinalRebarData(data,span,gdr))
   {
      Fire_GirderChanged(span,gdr,GCH_LONGITUDINAL_REBAR);
   }

   return true;
}

bool CProjectAgentImp::DoSetLongitudinalRebarData(const CLongitudinalRebarData& data,SpanIndexType span,GirderIndexType gdr)
{
   CSpanData* pSpan = m_BridgeDescription.GetSpan(span);
   CGirderTypes girderTypes = *pSpan->GetGirderTypes();
   girderTypes.GetGirderData(gdr).LongitudinalRebarData = data;
   pSpan->SetGirderTypes(girderTypes);
   return true;
}

////////////////////////////////////////////////////////////////////////
// IGirderLifting Methods
//
Float64 CProjectAgentImp::GetLeftLiftingLoopLocation(SpanIndexType span,GirderIndexType gdr)
{
   const CGirderData* pgirderData = GetGirderData(span,gdr);
   return pgirderData->HandlingData.LeftLiftPoint;
}

Float64 CProjectAgentImp::GetRightLiftingLoopLocation(SpanIndexType span,GirderIndexType gdr)
{
   const CGirderData* pgirderData = GetGirderData(span,gdr);
   return pgirderData->HandlingData.RightLiftPoint;
}

bool CProjectAgentImp::SetLiftingLoopLocations(SpanIndexType span,GirderIndexType gdr,Float64 left,Float64 right)
{
   if (DoSetLiftingLoopLocations(span,gdr,left,right))
   {
      Fire_GirderChanged(span,gdr,GCH_LIFTING_CONFIGURATION);
      return true;
   }
   else
      return false;
}

bool CProjectAgentImp::DoSetLiftingLoopLocations(SpanIndexType span,GirderIndexType gdr,Float64 left,Float64 right)
{
   CGirderData girderData = *GetGirderData(span,gdr);

   if ( !IsEqual(girderData.HandlingData.LeftLiftPoint,left) ||
        !IsEqual(girderData.HandlingData.RightLiftPoint,right) )
   {
      girderData.HandlingData.LeftLiftPoint = left;
      girderData.HandlingData.RightLiftPoint = right;

      DoSetGirderData(girderData,span,gdr);

      return true;
   }

   return false;
}

////////////////////////////////////////////////////////////////////////
// IGirderHauling Methods
//

Float64 CProjectAgentImp::GetTrailingOverhang(SpanIndexType span,GirderIndexType gdr)
{
   const CGirderData* pgirderData = GetGirderData(span,gdr);
   return pgirderData->HandlingData.TrailingSupportPoint;
}

Float64 CProjectAgentImp::GetLeadingOverhang(SpanIndexType span,GirderIndexType gdr)
{
   const CGirderData* pgirderData = GetGirderData(span,gdr);
   return pgirderData->HandlingData.LeadingSupportPoint;
}

bool CProjectAgentImp::SetTruckSupportLocations(SpanIndexType span,GirderIndexType gdr,Float64 trailing,Float64 leading)
{
   if (DoSetTruckSupportLocations(span,gdr,trailing,leading))
   {
      Fire_GirderChanged(span,gdr,GCH_SHIPPING_CONFIGURATION);
      return true;
   }
   else
      return false;
}

bool CProjectAgentImp::DoSetTruckSupportLocations(SpanIndexType span,GirderIndexType gdr,Float64 trailing,Float64 leading)
{
   CGirderData girderData = *(GetGirderData(span,gdr));

   if ( !IsEqual(girderData.HandlingData.TrailingSupportPoint,trailing) ||
        !IsEqual(girderData.HandlingData.LeadingSupportPoint,leading) )
   {
      girderData.HandlingData.TrailingSupportPoint = trailing;
      girderData.HandlingData.LeadingSupportPoint = leading;

      DoSetGirderData(girderData,span,gdr);

      return true;
   }

   return false;
}

////////////////////////////////////////////////////////////////////////
// IRatingSpecification Methods
//
bool CProjectAgentImp::AlwaysLoadRate() const
{
   const RatingLibraryEntry* pRatingEntry = GetRatingEntry(m_RatingSpec.c_str());
   return pRatingEntry->AlwaysLoadRate();
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

void CProjectAgentImp::SetGirderConditionFactor(SpanIndexType spanIdx,GirderIndexType gdrIdx,pgsTypes::ConditionFactorType conditionFactorType,Float64 conditionFactor)
{
   CGirderData girderData = *(GetGirderData(spanIdx,gdrIdx));
   if ( girderData.Condition != conditionFactorType || !IsEqual(girderData.ConditionFactor,conditionFactor) )
   {
      girderData.Condition = conditionFactorType;
      girderData.ConditionFactor = conditionFactor;
      DoSetGirderData(girderData,spanIdx,gdrIdx);
   }
}

void CProjectAgentImp::GetGirderConditionFactor(SpanIndexType spanIdx,GirderIndexType gdrIdx,pgsTypes::ConditionFactorType* pConditionFactorType,Float64 *pConditionFactor) const
{
   const CGirderData* pgirderData = GetGirderData(spanIdx,gdrIdx);
   *pConditionFactorType = pgirderData->Condition;
   *pConditionFactor = pgirderData->ConditionFactor;
}

Float64 CProjectAgentImp::GetGirderConditionFactor(SpanIndexType spanIdx,GirderIndexType gdrIdx) const
{
   const CGirderData* pgirderData = GetGirderData(spanIdx,gdrIdx);
   return pgirderData->ConditionFactor;
}

void CProjectAgentImp::SetDeckConditionFactor(pgsTypes::ConditionFactorType conditionFactorType,Float64 conditionFactor)
{
   CDeckDescription* pDeck = m_BridgeDescription.GetDeckDescription();

   if ( pDeck->Condition != conditionFactorType ||
        !IsEqual(pDeck->ConditionFactor,conditionFactor) )
   {
      pDeck->Condition = conditionFactorType;
      pDeck->ConditionFactor = conditionFactor;

      RatingSpecificationChanged(true);
   }
}

void CProjectAgentImp::GetDeckConditionFactor(pgsTypes::ConditionFactorType* pConditionFactorType,Float64 *pConditionFactor) const
{
   const CDeckDescription* pDeck = m_BridgeDescription.GetDeckDescription();
   *pConditionFactorType = pDeck->Condition;
   *pConditionFactor     = pDeck->ConditionFactor;
}

Float64 CProjectAgentImp::GetDeckConditionFactor() const
{
   const CDeckDescription* pDeck = m_BridgeDescription.GetDeckDescription();
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
   if ( !IsEqual(m_gDC[IndexFromLimitState(ls)],gDC) )
   {
      m_gDC[IndexFromLimitState(ls)] = gDC;
      RatingSpecificationChanged(true);
   }
}

Float64 CProjectAgentImp::GetDeadLoadFactor(pgsTypes::LimitState ls) const
{
   return m_gDC[IndexFromLimitState(ls)];
}

void CProjectAgentImp::SetWearingSurfaceFactor(pgsTypes::LimitState ls,Float64 gDW)
{
   if ( !IsEqual(m_gDW[IndexFromLimitState(ls)],gDW) )
   {
      m_gDW[IndexFromLimitState(ls)] = gDW;
      RatingSpecificationChanged(true);
   }
}

Float64 CProjectAgentImp::GetWearingSurfaceFactor(pgsTypes::LimitState ls) const
{
   return m_gDW[IndexFromLimitState(ls)];
}

void CProjectAgentImp::SetLiveLoadFactor(pgsTypes::LimitState ls,Float64 gLL)
{
   if ( !IsEqual(m_gLL[IndexFromLimitState(ls)],gLL) )
   {
      m_gLL[IndexFromLimitState(ls)] = gLL;
      RatingSpecificationChanged(true);
   }
}

Float64 CProjectAgentImp::GetLiveLoadFactor(pgsTypes::LimitState ls,bool bResolveIfDefault) const
{
   const RatingLibraryEntry* pRatingEntry = GetRatingEntry( m_RatingSpec.c_str() );
   return GetLiveLoadFactor(ls,GetADTT(),pRatingEntry,bResolveIfDefault);
}

Float64 CProjectAgentImp::GetLiveLoadFactor(pgsTypes::LimitState ls,Int16 adtt,const RatingLibraryEntry* pRatingEntry,bool bResolveIfDefault) const
{
   // returns < 0 if needs to be computed from rating library entry
   Float64 gLL = m_gLL[IndexFromLimitState(ls)];
   if ( gLL < 0 && bResolveIfDefault )
   {
      pgsTypes::LoadRatingType ratingType = ::RatingTypeFromLimitState(ls);
      CLiveLoadFactorModel model;
      if ( ratingType == pgsTypes::lrPermit_Routine )
         model = pRatingEntry->GetLiveLoadFactorModel(pgsTypes::lrPermit_Routine);
      else if ( ratingType == pgsTypes::lrPermit_Special )
         model = pRatingEntry->GetLiveLoadFactorModel( GetSpecialPermitType() );
      else
         model = pRatingEntry->GetLiveLoadFactorModel(ratingType);

      if ( ::IsStrengthLimitState(ls) )
      {
         if ( model.GetLiveLoadFactorType() != pgsTypes::gllBilinearWithWeight )
            gLL = model.GetStrengthLiveLoadFactor(adtt,0);

         // gLL will be < 0 if gLL is a function of axle weight and load combination poi
      }
      else
      {
         gLL = model.GetServiceLiveLoadFactor(adtt);
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

Float64 CProjectAgentImp::GetAllowableTension(pgsTypes::LoadRatingType ratingType,SpanIndexType spanIdx,GirderIndexType gdrIdx) const
{
   GET_IFACE(IBridgeMaterial,pMaterial);
   Float64 fc = pMaterial->GetFcGdr(spanIdx,gdrIdx);
   Float64 t = GetAllowableTensionCoefficient(ratingType);
   return t*sqrt(fc);
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
      InitSpecification(spec);
      SpecificationChanged(true);
   }
}

void CProjectAgentImp::GetTrafficBarrierDistribution(GirderIndexType* pNGirders,pgsTypes::TrafficBarrierDistribution* pDistType)
{
   *pNGirders = m_pSpecEntry->GetMaxGirdersDistTrafficBarrier();
   *pDistType = m_pSpecEntry->GetTrafficBarrierDistributionType();
}

pgsTypes::OverlayLoadDistributionType CProjectAgentImp::GetOverlayLoadDistributionType()
{
   return m_pSpecEntry->GetOverlayLoadDistributionType();
}


Uint16 CProjectAgentImp::GetMomentCapacityMethod()
{
   return m_pSpecEntry->GetBs3LRFDOverreinforcedMomentCapacity() == true ? LRFD_METHOD : WSDOT_METHOD;
}

void CProjectAgentImp::SetAnalysisType(pgsTypes::AnalysisType analysisType)
{
   if ( m_AnalysisType != analysisType )
   {
      m_AnalysisType = analysisType;
      Fire_AnalysisTypeChanged();
   }
}

pgsTypes::AnalysisType CProjectAgentImp::GetAnalysisType()
{
   return m_AnalysisType;
}

bool CProjectAgentImp::IsSlabOffsetDesignEnabled()
{
   GET_IFACE(ILibrary,pLib);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(m_Spec.c_str());

   return pSpecEntry->IsSlabOffsetDesignEnabled();
}

arDesignOptions CProjectAgentImp::GetDesignOptions(SpanIndexType spanIdx,GirderIndexType gdrIdx)
{
   arDesignOptions options;

   const CSpanData* pSpan = m_BridgeDescription.GetSpan(spanIdx);
   const GirderLibraryEntry* pGirderEntry = pSpan->GetGirderTypes()->GetGirderLibraryEntry(gdrIdx);

   // determine flexural design from girder attributes
   StrandIndexType  nh = pGirderEntry->GetNumHarpedStrandCoordinates();
   if (nh>0)
   {
      options.doDesignForFlexure = dtDesignForHarping;
   }
   else if (pGirderEntry->CanDebondStraightStrands())
   {
      options.doDesignForFlexure = dtDesignForDebonding;
   }
   else
   {
      options.doDesignForFlexure = dtDesignFullyBonded;
   }

   GET_IFACE(ILibrary,pLib);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(m_Spec.c_str());

   options.doDesignSlabOffset = pSpecEntry->IsSlabOffsetDesignEnabled();
   options.doDesignHauling = pSpecEntry->IsHaulingDesignEnabled();
   options.doDesignLifting = pSpecEntry->IsLiftingDesignEnabled();

   if (options.doDesignForFlexure == dtDesignForHarping)
   {
      bool check, design;
      Float64 d1, d2, d3;
      pSpecEntry->GetHoldDownForce(&check, &design, &d1);
      options.doDesignHoldDown = design;

      pSpecEntry->GetMaxStrandSlope(&check, &design, &d1, &d2, &d3);
      options.doDesignSlope =  design;
   }
   else
   {
      options.doDesignHoldDown = false;
      options.doDesignSlope    = false;
   }

   options.doForceHarpedStrandsStraight = pGirderEntry->IsForceHarpedStrandsStraight();

   if (!options.doForceHarpedStrandsStraight)
   {
      options.doStrandFillType = pSpecEntry->GetDesignStrandFillType();
   }
   else
   {
      // For straight-web designs we always fill using grid order (until design algorithm is changed to work for other option)
      options.doStrandFillType = ftGridOrder;
   }

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

   libKeyListType::iterator iter;
   for ( iter = keys.begin(); iter != keys.end(); iter++ )
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

void CProjectAgentImp::EnumGirderFamilyNames( std::vector<std::_tstring>* pNames )
{
   USES_CONVERSION;
   if ( m_GirderFamilyNames.size() == 0 )
   {
      std::vector<CString> names = CBeamFamilyManager::GetBeamFamilyNames();
      std::vector<CString>::iterator iter;
      for ( iter = names.begin(); iter != names.end(); iter++ )
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

const ConnectionLibraryEntry* CProjectAgentImp::GetConnectionEntry(LPCTSTR lpszName ) const
{
   const ConnectionLibraryEntry* pEntry;
   const ConnectionLibrary& prj_lib = m_pLibMgr->GetConnectionLibrary();
   pEntry = prj_lib.LookupEntry( lpszName );

   if (pEntry!=0)
      pEntry->Release();

   return pEntry;
}

const GirderLibraryEntry* CProjectAgentImp::GetGirderEntry( LPCTSTR lpszName ) const
{
   const GirderLibraryEntry* pEntry;
   const GirderLibrary& prj_lib = m_pLibMgr->GetGirderLibrary();
   pEntry = prj_lib.LookupEntry( lpszName );

   if (pEntry!=0)
      pEntry->Release();

   return pEntry;
}

const ConcreteLibraryEntry* CProjectAgentImp::GetConcreteEntry( LPCTSTR lpszName ) const
{
   const ConcreteLibraryEntry* pEntry;
   const ConcreteLibrary& prj_lib = m_pLibMgr->GetConcreteLibrary();
   pEntry = prj_lib.LookupEntry( lpszName );

   if (pEntry!=0)
      pEntry->Release();

   return pEntry;
}

const DiaphragmLayoutEntry* CProjectAgentImp::GetDiaphragmEntry( LPCTSTR lpszName ) const
{
   const DiaphragmLayoutEntry* pEntry;
   const DiaphragmLayoutLibrary& prj_lib = m_pLibMgr->GetDiaphragmLayoutLibrary();
   pEntry = prj_lib.LookupEntry( lpszName );

   if (pEntry!=0)
      pEntry->Release();

   return pEntry;
}

const TrafficBarrierEntry* CProjectAgentImp::GetTrafficBarrierEntry( LPCTSTR lpszName ) const
{
   const TrafficBarrierEntry* pEntry;
   const TrafficBarrierLibrary& prj_lib = m_pLibMgr->GetTrafficBarrierLibrary();
   pEntry = prj_lib.LookupEntry( lpszName );

   if (pEntry!=0)
      pEntry->Release();

   return pEntry;
}

const SpecLibraryEntry* CProjectAgentImp::GetSpecEntry( LPCTSTR lpszName ) const
{
   const SpecLibraryEntry* pEntry;
   const SpecLibrary& prj_lib = *(m_pLibMgr->GetSpecLibrary());
   pEntry = prj_lib.LookupEntry( lpszName );

   if (pEntry!=0)
      pEntry->Release();

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
      pEntry->Release();

   return pEntry;
}

const RatingLibraryEntry* CProjectAgentImp::GetRatingEntry( LPCTSTR lpszName ) const
{
   const RatingLibraryEntry* pEntry;
   const RatingLibrary* prj_lib = m_pLibMgr->GetRatingLibrary();
   pEntry = prj_lib->LookupEntry( lpszName );

   if (pEntry!=0)
      pEntry->Release();

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

Float64 CProjectAgentImp::GetDuctilityFactor()
{
   return m_DuctilityFactor;
}

Float64 CProjectAgentImp::GetImportanceFactor()
{
   return m_ImportanceFactor;
}

Float64 CProjectAgentImp::GetRedundancyFactor()
{
   return m_RedundancyFactor;
}

ILoadModifiers::Level CProjectAgentImp::GetDuctilityLevel()
{
   return m_DuctilityLevel;
}

ILoadModifiers::Level CProjectAgentImp::GetImportanceLevel()
{
   return m_ImportanceLevel;
}

ILoadModifiers::Level CProjectAgentImp::GetRedundancyLevel()
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
   // Get the lookup key for the strand material based on the current units
   lrfdStrandPool* pPool = lrfdStrandPool::GetInstance();

   std::map<SpanGirderHashType,Int32> keys[3]; // map _T("key") is span/girder, map _T("value") is strand pool key
   SpanIndexType nSpans;
   nSpans   = m_BridgeDescription.GetSpanCount();

   SpanIndexType spanIdx;
   for ( spanIdx = 0; spanIdx < nSpans; spanIdx++ )
   {
      GirderIndexType nGirders = m_BridgeDescription.GetSpan(spanIdx)->GetGirderCount();
      for (GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
      {
         SpanGirderHashType hash = HashSpanGirder(spanIdx, gdrIdx);

         for ( int i = 0; i < 3; i++ )
         {
            pgsTypes::StrandType type = (pgsTypes::StrandType)i;
            const matPsStrand* pStrandMaterial = GetStrandMaterial(spanIdx,gdrIdx,type);
            
            Int32 strand_pool_key = pPool->GetStrandKey(pStrandMaterial);

            keys[type].insert(std::make_pair(hash,strand_pool_key));
         }
      }
   }

   // change the units
   lrfdVersionMgr::SetVersion( m_pSpecEntry->GetSpecificationType() );
   lrfdVersionMgr::SetUnits( m_pSpecEntry->GetSpecificationUnits() );

   // Get the new strand material based on the new units
   for ( spanIdx = 0; spanIdx < nSpans; spanIdx++ )
   {
      CSpanData* pSpan = m_BridgeDescription.GetSpan(spanIdx);
      CGirderTypes girderTypes = *pSpan->GetGirderTypes();
      GirderIndexType nGirders = pSpan->GetGirderCount();

      for (GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
      {
         SpanGirderHashType hash = HashSpanGirder(spanIdx, gdrIdx);

         // get girder data directly so we can set the strand material without causing
         // the jacking force to be updated
         CGirderData& girderData = girderTypes.GetGirderData(gdrIdx);
         
         for ( int i = 0; i < 3; i++ )
         {
            Int32 strand_pool_key = keys[i][hash];
            girderData.Material.pStrandMaterial[i] = pPool->GetStrand(strand_pool_key);
         }
      }

      pSpan->SetGirderTypes(girderTypes);
   }

   m_bUpdateJackingForce = true;

   if ( bFireEvent )
      Fire_SpecificationChanged();
}

void CProjectAgentImp::RatingSpecificationChanged(bool bFireEvent)
{
   if ( bFireEvent )
      Fire_RatingSpecificationChanged();
}

/////////////////////////////////////////////////////////////////////////////
// IUserDefinedLoadData
CollectionIndexType CProjectAgentImp::GetPointLoadCount() const
{
   return m_PointLoads.size();
}

CollectionIndexType CProjectAgentImp::AddPointLoad(const CPointLoadData& pld)
{
   m_PointLoads.push_back(pld);

   FireContinuityRelatedGirderChange(pld.m_Span,pld.m_Girder,GCH_LOADING_ADDED);

   return GetPointLoadCount()-1;
}

const CPointLoadData& CProjectAgentImp::GetPointLoad(CollectionIndexType idx) const
{
   ATLASSERT(0 <= idx && idx < GetPointLoadCount() );

   return m_PointLoads[idx];
}

void CProjectAgentImp::UpdatePointLoad(CollectionIndexType idx, const CPointLoadData& pld)
{
   ATLASSERT(0 <= idx && idx < GetPointLoadCount() );

   // must fire a delete event if load is moved to another girder
   SpanIndexType prevspn = m_PointLoads[idx].m_Span;
   GirderIndexType prevgdr = m_PointLoads[idx].m_Girder;

   if (prevspn != pld.m_Span || prevgdr != pld.m_Girder)
   {
      FireContinuityRelatedGirderChange(prevspn,prevgdr,GCH_LOADING_REMOVED);
   }

   m_PointLoads[idx] = pld;

   FireContinuityRelatedGirderChange(pld.m_Span,pld.m_Girder,GCH_LOADING_CHANGED);
}

void CProjectAgentImp::DeletePointLoad(CollectionIndexType idx)
{
   ATLASSERT(0 <= idx && idx < GetPointLoadCount() );

   PointLoadListIterator it = m_PointLoads.begin();
   it += idx;

   SpanIndexType span = (*it).m_Span;
   GirderIndexType gdr =  (*it).m_Girder;

   m_PointLoads.erase(it);

   FireContinuityRelatedGirderChange(span,gdr,GCH_LOADING_REMOVED);
}

CollectionIndexType CProjectAgentImp::GetDistributedLoadCount() const
{
   return m_DistributedLoads.size();
}


CollectionIndexType CProjectAgentImp::AddDistributedLoad(const CDistributedLoadData& pld)
{
   m_DistributedLoads.push_back(pld);

   FireContinuityRelatedGirderChange(pld.m_Span,pld.m_Girder,GCH_LOADING_ADDED);

   return GetDistributedLoadCount()-1;
}

const CDistributedLoadData& CProjectAgentImp::GetDistributedLoad(CollectionIndexType idx) const
{
   ATLASSERT(0 <= idx && idx < GetDistributedLoadCount() );

   return m_DistributedLoads[idx];
}

void CProjectAgentImp::UpdateDistributedLoad(CollectionIndexType idx, const CDistributedLoadData& pld)
{
   ATLASSERT(0 <= idx && idx < GetDistributedLoadCount() );

   // must fire a delete event if load is moved to another girder
   SpanIndexType prevspn = m_DistributedLoads[idx].m_Span;
   GirderIndexType prevgdr = m_DistributedLoads[idx].m_Girder;

   if (prevspn != pld.m_Span || prevgdr != pld.m_Girder)
   {
      FireContinuityRelatedGirderChange(prevspn,prevgdr,GCH_LOADING_REMOVED);
   }

   m_DistributedLoads[idx] = pld;

   FireContinuityRelatedGirderChange(pld.m_Span,pld.m_Girder,GCH_LOADING_CHANGED);
}

void CProjectAgentImp::DeleteDistributedLoad(CollectionIndexType idx)
{
   ATLASSERT(0 <= idx && idx < GetDistributedLoadCount() );

   DistributedLoadListIterator it = m_DistributedLoads.begin();
   it += idx;

   SpanIndexType span = (*it).m_Span;
   GirderIndexType gdr  = (*it).m_Girder;

   m_DistributedLoads.erase(it);

   FireContinuityRelatedGirderChange(span,gdr,GCH_LOADING_REMOVED);
}

CollectionIndexType CProjectAgentImp::GetMomentLoadCount() const
{
   return m_MomentLoads.size();
}

CollectionIndexType CProjectAgentImp::AddMomentLoad(const CMomentLoadData& pld)
{
   m_MomentLoads.push_back(pld);

   FireContinuityRelatedGirderChange(pld.m_Span,pld.m_Girder,GCH_LOADING_ADDED);

   return GetMomentLoadCount()-1;
}

const CMomentLoadData& CProjectAgentImp::GetMomentLoad(CollectionIndexType idx) const
{
   ATLASSERT(0 <= idx && idx < GetMomentLoadCount() );

   return m_MomentLoads[idx];
}

void CProjectAgentImp::UpdateMomentLoad(CollectionIndexType idx, const CMomentLoadData& pld)
{
   ATLASSERT(0 <= idx && idx < GetMomentLoadCount() );

   // must fire a delete event if load is moved to another girder
   SpanIndexType prevspn = m_MomentLoads[idx].m_Span;
   GirderIndexType prevgdr = m_MomentLoads[idx].m_Girder;

   if (prevspn != pld.m_Span || prevgdr != pld.m_Girder)
   {
      FireContinuityRelatedGirderChange(prevspn,prevgdr,GCH_LOADING_REMOVED);
   }

   m_MomentLoads[idx] = pld;

   FireContinuityRelatedGirderChange(pld.m_Span,pld.m_Girder,GCH_LOADING_CHANGED);
}

void CProjectAgentImp::DeleteMomentLoad(CollectionIndexType idx)
{
   ATLASSERT(0 <= idx && idx < GetMomentLoadCount() );

   MomentLoadListIterator it = m_MomentLoads.begin();
   it += idx;

   SpanIndexType span = (*it).m_Span;
   GirderIndexType gdr =  (*it).m_Girder;

   m_MomentLoads.erase(it);

   FireContinuityRelatedGirderChange(span,gdr,GCH_LOADING_REMOVED);
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

HRESULT CProjectAgentImp::SavePointLoads(IStructuredSave* pSave)
{
   HRESULT hr=S_OK;

   pSave->BeginUnit(_T("UserDefinedPointLoads"),1.0);

   hr = pSave->put_Property(_T("Count"),CComVariant((long)m_PointLoads.size()));
   if ( FAILED(hr) )
      return hr;

   for (PointLoadListIterator it=m_PointLoads.begin(); it!=m_PointLoads.end(); it++)
   {
      hr = it->Save(pSave);
      if ( FAILED(hr) )
         return hr;
   }
   
   pSave->EndUnit();
   return hr;
}

HRESULT CProjectAgentImp::LoadPointLoads(IStructuredLoad* pLoad)
{
   HRESULT hr=S_OK;

   hr = pLoad->BeginUnit(_T("UserDefinedPointLoads"));
   if ( FAILED(hr) )
      return hr;

   CComVariant var;
   var.vt = VT_I4;
   hr = pLoad->get_Property(_T("Count"),&var);
   if ( FAILED(hr) )
      return hr;

   long cnt = var.lVal;

   for (long i=0; i<cnt; i++)
   {
      CPointLoadData data;

      hr = data.Load(pLoad);
      if (FAILED(hr))
         return hr;

      m_PointLoads.push_back(data);
   }
   
   hr = pLoad->EndUnit();

   return hr;
}

HRESULT CProjectAgentImp::SaveDistributedLoads(IStructuredSave* pSave)
{
   HRESULT hr=S_OK;

   pSave->BeginUnit(_T("UserDefinedDistributedLoads"),1.0);

   hr = pSave->put_Property(_T("Count"),CComVariant((long)m_DistributedLoads.size()));
   if ( FAILED(hr) )
      return hr;

   for (DistributedLoadListIterator it=m_DistributedLoads.begin(); it!=m_DistributedLoads.end(); it++)
   {
      hr = it->Save(pSave);
      if ( FAILED(hr) )
         return hr;
   }
   
   pSave->EndUnit();
   return hr;
}

HRESULT CProjectAgentImp::LoadDistributedLoads(IStructuredLoad* pLoad)
{
   HRESULT hr=S_OK;

   hr = pLoad->BeginUnit(_T("UserDefinedDistributedLoads"));
   if ( FAILED(hr) )
      return hr;

   CComVariant var;
   var.vt = VT_I4;
   hr = pLoad->get_Property(_T("Count"),&var);
   if ( FAILED(hr) )
      return hr;

   long cnt = var.lVal;

   for (long i=0; i<cnt; i++)
   {
      CDistributedLoadData data;

      hr = data.Load(pLoad);
      if (FAILED(hr))
         return hr;

      m_DistributedLoads.push_back(data);
   }
   
   hr = pLoad->EndUnit();

   return hr;
}

HRESULT CProjectAgentImp::SaveMomentLoads(IStructuredSave* pSave)
{
   HRESULT hr=S_OK;

   pSave->BeginUnit(_T("UserDefinedMomentLoads"),1.0);

   hr = pSave->put_Property(_T("Count"),CComVariant((long)m_MomentLoads.size()));
   if ( FAILED(hr) )
      return hr;

   for (MomentLoadListIterator it=m_MomentLoads.begin(); it!=m_MomentLoads.end(); it++)
   {
      hr = it->Save(pSave);
      if ( FAILED(hr) )
         return hr;
   }
   
   pSave->EndUnit();
   return hr;
}

HRESULT CProjectAgentImp::LoadMomentLoads(IStructuredLoad* pLoad)
{
   HRESULT hr=S_OK;

   hr = pLoad->BeginUnit(_T("UserDefinedMomentLoads"));
   if ( FAILED(hr) )
      return hr;

   CComVariant var;
   var.vt = VT_I4;
   hr = pLoad->get_Property(_T("Count"),&var);
   if ( FAILED(hr) )
      return hr;

   long cnt = var.lVal;

   for (long i=0; i<cnt; i++)
   {
      CMomentLoadData data;

      hr = data.Load(pLoad);
      if (FAILED(hr))
         return hr;

      m_MomentLoads.push_back(data);
   }
   
   hr = pLoad->EndUnit();

   return hr;
}

////////////////////////////////////////////////////////////////////////
// IEvents
void CProjectAgentImp::HoldEvents()
{
   m_bHoldingEvents = true;

   GET_IFACE(IUIEvents,pUIEvents);
   pUIEvents->HoldEvents(true);
}

void CProjectAgentImp::FirePendingEvents()
{
   if ( !m_bHoldingEvents )
      return;

   m_bHoldingEvents = false;

   if ( sysFlags<Uint32>::IsSet(m_PendingEvents,EVT_PROJECTPROPERTIES) )
      Fire_ProjectPropertiesChanged();

   //if ( sysFlags<Uint32>::IsSet(m_PendingEvents,EVT_UNITS) )
   //   Fire_UnitsChanged(m_Units);

   if ( sysFlags<Uint32>::IsSet(m_PendingEvents,EVT_ANALYSISTYPE) )
      Fire_AnalysisTypeChanged();

   if ( sysFlags<Uint32>::IsSet(m_PendingEvents,EVT_EXPOSURECONDITION) )
      Fire_ExposureConditionChanged();

   if ( sysFlags<Uint32>::IsSet(m_PendingEvents,EVT_RELHUMIDITY) )
      Fire_RelHumidityChanged();

   if ( sysFlags<Uint32>::IsSet(m_PendingEvents,EVT_BRIDGE) || sysFlags<Uint32>::IsSet(m_PendingEvents,EVT_GIRDERFAMILY))
      Fire_BridgeChanged();
//
// pretty much all the event handers do the same thing when the bridge or girder family changes
// no sence firing two events
//   if ( sysFlags<Uint32>::IsSet(m_PendingEvents,EVT_GIRDERFAMILY) )
//      Fire_GirderFamilyChanged();

   if ( sysFlags<Uint32>::IsSet(m_PendingEvents,EVT_SPECIFICATION) )
      SpecificationChanged(true);

   if ( sysFlags<Uint32>::IsSet(m_PendingEvents,EVT_RATING_SPECIFICATION) )
      RatingSpecificationChanged(true);

   if ( sysFlags<Uint32>::IsSet(m_PendingEvents,EVT_LIBRARYCONFLICT) )
      Fire_OnLibraryConflictResolved();

   if ( sysFlags<Uint32>::IsSet(m_PendingEvents,EVT_LOADMODIFIER) )
      Fire_LoadModifiersChanged();

   if ( sysFlags<Uint32>::IsSet(m_PendingEvents,EVT_CONSTRUCTIONLOAD) )
      Fire_ConstructionLoadChanged();

   if ( sysFlags<Uint32>::IsSet(m_PendingEvents,EVT_LIVELOAD) )
      Fire_LiveLoadChanged();

   std::map<SpanGirderHashType,Uint32>::iterator iter;
   for (iter = m_PendingEventsHash.begin(); iter != m_PendingEventsHash.end(); iter++ )
   {
      SpanIndexType span;
      GirderIndexType gdr;
      UnhashSpanGirder((*iter).first,&span,&gdr);
      Uint32 lHint = (*iter).second;

      Fire_GirderChanged(span,gdr,lHint);
   }

   m_PendingEventsHash.clear();
   m_PendingEvents = 0;

   GET_IFACE(IUIEvents,pUIEvents);
   pUIEvents->FirePendingEvents();
}

void CProjectAgentImp::CancelPendingEvents()
{
   m_bHoldingEvents = false;
   m_PendingEventsHash.clear();
   m_PendingEvents = 0;
}

////////////////////////////////////////////////////////////////////////
// ILimits
double CProjectAgentImp::GetMaxSlabFc()
{
   ATLASSERT(false); // obsolete - only applicable to normal weight concrete
   return m_pSpecEntry->GetMaxSlabFc(pgsTypes::Normal);
}

double CProjectAgentImp::GetMaxGirderFci()
{
   ATLASSERT(false); // obsolete - only applicable to normal weight concrete
   return m_pSpecEntry->GetMaxGirderFci(pgsTypes::Normal);
}

double CProjectAgentImp::GetMaxGirderFc()
{
   ATLASSERT(false); // obsolete - only applicable to normal weight concrete
   return m_pSpecEntry->GetMaxGirderFc(pgsTypes::Normal);
}

double CProjectAgentImp::GetMaxConcreteUnitWeight()
{
   ATLASSERT(false); // obsolete - only applicable to normal weight concrete
   return m_pSpecEntry->GetMaxConcreteUnitWeight(pgsTypes::Normal);
}

double CProjectAgentImp::GetMaxConcreteAggSize()
{
   ATLASSERT(false); // obsolete - only applicable to normal weight concrete
   return m_pSpecEntry->GetMaxConcreteAggSize(pgsTypes::Normal);
}

////////////////////////////////////////////////////////////////////////
// ILimits2
double CProjectAgentImp::GetMaxSlabFc(pgsTypes::ConcreteType concType)
{
   return m_pSpecEntry->GetMaxSlabFc(concType);
}

double CProjectAgentImp::GetMaxGirderFci(pgsTypes::ConcreteType concType)
{
   return m_pSpecEntry->GetMaxGirderFci(concType);
}

double CProjectAgentImp::GetMaxGirderFc(pgsTypes::ConcreteType concType)
{
   return m_pSpecEntry->GetMaxGirderFc(concType);
}

double CProjectAgentImp::GetMaxConcreteUnitWeight(pgsTypes::ConcreteType concType)
{
   return m_pSpecEntry->GetMaxConcreteUnitWeight(concType);
}

double CProjectAgentImp::GetMaxConcreteAggSize(pgsTypes::ConcreteType concType)
{
   return m_pSpecEntry->GetMaxConcreteAggSize(concType);
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
bool CProjectAgentImp::IsLiveLoadDefined(pgsTypes::LiveLoadType llType)
{
   if ( m_SelectedLiveLoads[llType].empty() )
      return false;

   return true;
}

ILiveLoads::PedestrianLoadApplicationType CProjectAgentImp::GetPedestrianLoadApplication(pgsTypes::LiveLoadType llType)
{
   if (llType<pgsTypes::lltDesign || llType>pgsTypes::lltFatigue)
   {
      // Rating Live Loads
      return m_bIncludePedestrianLiveLoad ? ILiveLoads::PedConcurrentWithVehiculuar : ILiveLoads::PedDontApply;
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
      ATLASSERT(0);
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

std::vector<std::_tstring> CProjectAgentImp::GetLiveLoadNames(pgsTypes::LiveLoadType llType)
{
   std::vector<std::_tstring> strNames;
   strNames.reserve(m_SelectedLiveLoads[llType].size());

   for (LiveLoadSelectionIterator it = m_SelectedLiveLoads[llType].begin(); it != m_SelectedLiveLoads[llType].end(); it++)
   {
      const LiveLoadSelection& lls = *it;

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
      for (LiveLoadSelectionIterator it = m_SelectedLiveLoads[llType].begin(); it != m_SelectedLiveLoads[llType].end(); it++)
      {
         std::vector<std::_tstring>::const_iterator its = std::find(names.begin(), names.end(), it->EntryName);
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
      for (LiveLoadSelectionIterator it = m_SelectedLiveLoads[llType].begin(); it != m_SelectedLiveLoads[llType].end(); it++)
      {
         LiveLoadSelection& selection = *it;

         if ( selection.pEntry != NULL )
         {
            release_library_entry( &m_LibObserver, 
                                   it->pEntry,
                                   pLiveLoadLibrary);
         }
      }

      m_SelectedLiveLoads[llType].clear();

      // add new references
      for (std::vector<std::_tstring>::const_iterator its = names.begin(); its != names.end(); its++)
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

         m_SelectedLiveLoads[llType].push_back(selection);
      }

      Fire_LiveLoadChanged();
   }
}

double CProjectAgentImp::GetTruckImpact(pgsTypes::LiveLoadType llType)
{
   return m_TruckImpact[llType];
}

void CProjectAgentImp::SetTruckImpact(pgsTypes::LiveLoadType llType,double impact)
{
   if ( !IsEqual(m_TruckImpact[llType],impact) )
   {
      m_TruckImpact[llType] = impact;
      Fire_LiveLoadChanged();
   }
}

double CProjectAgentImp::GetLaneImpact(pgsTypes::LiveLoadType llType)
{
   return m_LaneImpact[llType];
}

void CProjectAgentImp::SetLaneImpact(pgsTypes::LiveLoadType llType,double impact)
{
   if ( !IsEqual(m_LaneImpact[llType],impact) )
   {
      m_LaneImpact[llType] = impact;
      Fire_LiveLoadChanged();
   }
}

bool CProjectAgentImp::IsReservedLiveLoad(const std::_tstring& strName)
{
   std::vector<std::_tstring>::const_iterator iter(m_ReservedLiveLoads.begin());
   std::vector<std::_tstring>::const_iterator iter_end(m_ReservedLiveLoads.end());
   for ( ; iter != iter_end; iter++ )
   {
      const std::_tstring& rname = *iter;
      if ( rname == strName )
         return true;
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

LldfRangeOfApplicabilityAction CProjectAgentImp::GetLldfRangeOfApplicabilityAction()
{
   return m_LldfRangeOfApplicabilityAction;
}

bool CProjectAgentImp::IgnoreLLDFRangeOfApplicability()
{
   if (m_BridgeDescription.GetDistributionFactorMethod() == pgsTypes::Calculated)
      return m_LldfRangeOfApplicabilityAction != roaEnforce;
   else if (m_BridgeDescription.GetDistributionFactorMethod() == pgsTypes::LeverRule)
      return true;
   else
      return false;
}

std::_tstring CProjectAgentImp::GetLLDFSpecialActionText()
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
      return std::_tstring(_T(" Note that range of applicability requirements are ignored. Equations are used regardless of their validity"));
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
bool CProjectAgentImp::IgnoreEffectiveFlangeWidthLimits()
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
// Helper Methods
//
bool CProjectAgentImp::ResolveLibraryConflicts(const ConflictList& rList)
{
   // NOTE: that the name of this function is misleading. It not only deals with conflicts,
   //       it also creates the library entries.
   std::_tstring new_name;

   // Connections
   ConnectionLibrary& rconlib = m_pLibMgr->GetConnectionLibrary();
   CPierData* pPier = m_BridgeDescription.GetPier(0);
   while ( pPier != NULL )
   {
      const CSpanData* pPrevSpan = pPier->GetPrevSpan();
      if (pPrevSpan)
      {
         // rename reference if user renamed entry on load
         if (rList.IsConflict(rconlib, pPier->GetConnection(pgsTypes::Back), &new_name))
            pPier->SetConnection(pgsTypes::Back,new_name.c_str());

         // Add a reference to the library entry
         const ConnectionLibraryEntry* pConnLibEntry;
         use_library_entry( &m_LibObserver,
                            pPier->GetConnection(pgsTypes::Back), 
                            &pConnLibEntry, 
                            rconlib);
         pPier->SetConnectionLibraryEntry(pgsTypes::Back,pConnLibEntry);
      }

      CSpanData* pNextSpan = pPier->GetNextSpan();
      if (pNextSpan)
      {
         if (rList.IsConflict(rconlib, pPier->GetConnection(pgsTypes::Ahead), &new_name))
            pPier->SetConnection( pgsTypes::Ahead, new_name.c_str() );

         // Add a reference to the library entry
         const ConnectionLibraryEntry* pConnLibEntry;
         use_library_entry( &m_LibObserver,
                            pPier->GetConnection(pgsTypes::Ahead), 
                            &pConnLibEntry, 
                            rconlib);
         pPier->SetConnectionLibraryEntry(pgsTypes::Ahead,pConnLibEntry);

         pPier = pNextSpan->GetNextPier();
      }
      else
      {
         pPier = NULL;
      }
   }

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

   CSpanData* pSpan = m_BridgeDescription.GetSpan(0);
   while ( pSpan )
   {
      CGirderTypes girderTypes = *pSpan->GetGirderTypes();
      GroupIndexType nGroups = girderTypes.GetGirderGroupCount();
      for ( GroupIndexType groupIdx = 0; groupIdx < nGroups; groupIdx++ )
      {
         GirderIndexType firstGdrIdx,lastGdrIdx;
         std::_tstring strGirderName;

         girderTypes.GetGirderGroup(groupIdx,&firstGdrIdx,&lastGdrIdx,strGirderName);
         if (rList.IsConflict(girderLibrary, strGirderName, &new_name))
            girderTypes.RenameGirder(groupIdx,new_name.c_str());

      }

      pSpan->SetGirderTypes(girderTypes);
      pSpan = pSpan->GetNextPier()->GetNextSpan();
   }

   // now use all the girder library entries that are in this model
   UseGirderLibraryEntries();

   // left traffic barrier
   const TrafficBarrierLibrary& rbarlib = m_pLibMgr->GetTrafficBarrierLibrary();
   CRailingSystem* pRailingSystem = m_BridgeDescription.GetLeftRailingSystem();

   if (rList.IsConflict(rbarlib, pRailingSystem->strExteriorRailing, &new_name))
      pRailingSystem->strExteriorRailing = new_name;

   const TrafficBarrierEntry* pEntry = NULL;

   use_library_entry( &m_LibObserver,
                      pRailingSystem->strExteriorRailing,
                      &pEntry,
                      m_pLibMgr->GetTrafficBarrierLibrary());

   pRailingSystem->SetExteriorRailing(pEntry);

   if ( pRailingSystem->bUseInteriorRailing )
   {
      if (rList.IsConflict(rbarlib, pRailingSystem->strInteriorRailing, &new_name))
         pRailingSystem->strInteriorRailing = new_name;

      use_library_entry( &m_LibObserver,
                         pRailingSystem->strInteriorRailing,
                         &pEntry,
                        m_pLibMgr->GetTrafficBarrierLibrary());

      pRailingSystem->SetInteriorRailing(pEntry);
   }

   // right traffic barrier
   pRailingSystem = m_BridgeDescription.GetRightRailingSystem();
   if (rList.IsConflict(rbarlib, pRailingSystem->strExteriorRailing, &new_name))
      pRailingSystem->strExteriorRailing = new_name;

   use_library_entry( &m_LibObserver,
                      pRailingSystem->strExteriorRailing,
                      &pEntry,
                      m_pLibMgr->GetTrafficBarrierLibrary());

   pRailingSystem->SetExteriorRailing(pEntry);

   if ( pRailingSystem->bUseInteriorRailing )
   {
      if (rList.IsConflict(rbarlib, pRailingSystem->strInteriorRailing, &new_name))
         pRailingSystem->strInteriorRailing = new_name;

      use_library_entry( &m_LibObserver,
                         pRailingSystem->strInteriorRailing,
                         &pEntry,
                         m_pLibMgr->GetTrafficBarrierLibrary());

      pRailingSystem->SetInteriorRailing(pEntry);
   }

   // Spec Library
   const SpecLibrary& rspeclib = *(m_pLibMgr->GetSpecLibrary());
   if (rList.IsConflict(rspeclib, m_Spec, &new_name))
      m_Spec = new_name;


   // a little game here to set up the spec library entry
   // when the library manager is set, PGSuper latches onto the first
   // spec lib entry. We need to release that entry and hook onto the
   // the real entry. Save the current name, clear the m_Spec data entry
   // so it looks like the spec is being changed, then set the specification
   std::_tstring specName = m_Spec;
   m_Spec = _T("");
   InitSpecification(specName);


   // Rating Library
   const RatingLibrary& rratinglib = *(m_pLibMgr->GetRatingLibrary());
   if (rList.IsConflict(rratinglib, m_RatingSpec, &new_name))
      m_RatingSpec = new_name;


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

   return true;
}

void CProjectAgentImp::UpdateJackingForce() const
{
   if ( !m_bUpdateJackingForce )
      return; // jacking force is up to date

   // this parameter is to prevent a problem with recurssion
   // it must be a static data member
   static bool bUpdating = true;
   if ( bUpdating )
      return;

   bUpdating = true;

   // If the jacking force in the strands is computed by PGSuper, update the
   // jacking force to reflect the maximum value for the current specifications
   CSpanData* pSpan = m_BridgeDescription.GetSpan(0);
   while ( pSpan )
   {
      GirderIndexType nGirders = pSpan->GetGirderCount();
      CGirderTypes girderTypes = *pSpan->GetGirderTypes();

      SpanIndexType spanIdx = pSpan->GetSpanIndex();

      for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
      {
         CGirderData& girderData = girderTypes.GetGirderData(gdrIdx);

         for ( Uint16 i = 0; i < 4; i++ )
         {
            if ( girderData.PrestressData.bPjackCalculated[i] )
               girderData.PrestressData.Pjack[i] = GetMaxPjack( spanIdx, gdrIdx, pgsTypes::StrandType(i), girderData.PrestressData.GetNstrands(pgsTypes::StrandType(i)) );
         }
      }

      pSpan->SetGirderTypes(girderTypes);
      pSpan = pSpan->GetNextPier()->GetNextSpan();
   }

   m_bUpdateJackingForce = false;
   bUpdating = false;
}

void CProjectAgentImp::UpdateJackingForce(SpanIndexType span,GirderIndexType gdr)
{
   CGirderData girderData = *(GetGirderData(span,gdr));

   for ( Uint16 i = 0; i < 4; i++ )
   {
      if ( girderData.PrestressData.bPjackCalculated[i] )
         girderData.PrestressData.Pjack[i] = GetMaxPjack(span,gdr,pgsTypes::StrandType(i),girderData.PrestressData.GetNstrands(pgsTypes::StrandType(i)));
   }

   DoSetGirderData(girderData,span,gdr);
}

void CProjectAgentImp::DealWithGirderLibraryChanges(bool fromLibrary)
{
// more work required here - check number of strands, pjack, all stuff changed in CGirderData::ResetPrestressData()
//  PROGRAM WILL CRASH IF:
   // number of strands is incompatible with library strand grid
   // debonding data when number of strands in grid changes - so just blast debond data
   // CHECK IF
   // debond length exceeds 1/2 girder length
#pragma Reminder("Need thorough check of library changes affect to project data")

   GET_IFACE(IPrestressForce,pPrestress);

   CSpanData* pSpan = m_BridgeDescription.GetSpan(0);
   while ( pSpan )
   {
      CGirderTypes girderTypes = *pSpan->GetGirderTypes();
      GirderIndexType nGirders = pSpan->GetGirderCount();

      SpanIndexType spanIdx = pSpan->GetSpanIndex();

      for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
      {
         CGirderData& girder_data = girderTypes.GetGirderData(gdrIdx);
         const GirderLibraryEntry* pGdrEntry = girderTypes.GetGirderLibraryEntry(gdrIdx);

         // Convert legacy data as needed
         ConvertLegacyDebondData(girder_data, pGdrEntry);
         ConvertLegacyExtendedStrandData(girder_data,pGdrEntry);

         ValidateStrands(spanIdx,gdrIdx,girder_data,fromLibrary);

         Float64 xfer_length = pPrestress->GetXferLength(spanIdx,gdrIdx,pgsTypes::Permanent);
         Float64 min_xfer = pGdrEntry->GetMinDebondSectionLength(); 

         if (min_xfer < xfer_length)
         {
            std::_tostringstream os;
            os << _T("Span ") << LABEL_SPAN(spanIdx) << _T(" Girder ") << LABEL_GIRDER(gdrIdx) << _T(": The minimum debond section length in the girder library is shorter than the transfer length (e.g., 60*Db). This may cause the debonding design algorithm to generate designs that do not pass a specification check.") << std::endl;
            AddGirderStatusItem(spanIdx, gdrIdx, os.str() );
         }
      }

      pSpan->SetGirderTypes(girderTypes);

      pSpan = pSpan->GetNextPier()->GetNextSpan();
   }
}

void CProjectAgentImp::DealWithConnectionLibraryChanges(bool fromLibrary)
{
   // problem here can be if connection's bearing location is incompatible with spacing type
   bool is_ubiquitous_spacing = IsBridgeSpacing( GetGirderSpacingType() );

   bool problem = false;
   std::_tostringstream os;
   os << _T("Warning: The girder spacing at ");

   SpanIndexType nSpans   = m_BridgeDescription.GetSpanCount();
   for ( SpanIndexType spanIdx = 0; spanIdx < nSpans; spanIdx++ )
   {
      CSpanData* pSpan = m_BridgeDescription.GetSpan(spanIdx);
      const CPierData* pPrevPier = pSpan->GetPrevPier();
      const CPierData* pNextPier = pSpan->GetNextPier();

      const ConnectionLibraryEntry* start_connection_entry = pPrevPier->GetConnectionLibraryEntry(pgsTypes::Ahead);
      const ConnectionLibraryEntry* end_connection_entry   = pNextPier->GetConnectionLibraryEntry(pgsTypes::Back);

      CGirderSpacing* pStartSpacing = pSpan->GetGirderSpacing(pgsTypes::metStart);
      CGirderSpacing* pEndSpacing   = pSpan->GetGirderSpacing(pgsTypes::metEnd);

      pgsTypes::MeasurementLocation mlStart = pStartSpacing->GetMeasurementLocation();
      pgsTypes::MeasurementLocation mlEnd = pEndSpacing->GetMeasurementLocation();

      // Check that spacing type is compatible with the current connection. We cannot
      // allow spacing measured along the girder if the connection's bearing length is measured at the bearing
      ConnectionLibraryEntry::BearingOffsetMeasurementType mtBearingOffset = start_connection_entry->GetBearingOffsetMeasurementType();
      if (mlStart==pgsTypes::AtCenterlineBearing && mtBearingOffset==ConnectionLibraryEntry::AlongGirder)
      {
         problem = true;
         if (!is_ubiquitous_spacing)
         {
            // for non-uniform, we must set at each bearing line
            pStartSpacing->SetMeasurementLocation(pgsTypes::AtCenterlinePier);
         }

         os << _T("left end of Span ") << LABEL_SPAN(spanIdx) <<_T(", ");
      }

      mtBearingOffset = end_connection_entry->GetBearingOffsetMeasurementType();
      if (mlEnd==pgsTypes::AtCenterlineBearing && mtBearingOffset==ConnectionLibraryEntry::AlongGirder)
      {
         problem = true;

         if (!is_ubiquitous_spacing)
         {
            pEndSpacing->SetMeasurementLocation(pgsTypes::AtCenterlinePier);
         }

         os << _T("right end of Span ") << LABEL_SPAN(spanIdx) <<_T(", ");
      }

   }

   if (problem)
   {
      // we had a spacing mismatch.
      if (is_ubiquitous_spacing)
      {
         // same spacing everywhere - set it
         SetMeasurementLocation(pgsTypes::AtCenterlinePier);
      }

      // one status message for all changes made
      GET_IFACE(IEAFStatusCenter,pStatusCenter);
      os << _T("was measured from the bearing line; and the connection's bearing offset was measured along the girder. These settings are not compatible. The spacing was Automatically Forced to be measured from CL Pier. Please check your bearing locations and span length!");
      pgsInformationalStatusItem* pStatusItem = new pgsInformationalStatusItem(m_StatusGroupID,124,os.str().c_str());
      pStatusCenter->Add(pStatusItem);
   }

   // Another possible problem is that orphaned connections at ends of bridge can become invalid if the connection library
   // changes. Make sure our orphans have valid connection names
   GET_IFACE(ILibrary,pLib);
  
   CPierData* pPier = m_BridgeDescription.GetPier(0);
   pPier->SetConnectionLibraryEntry(pgsTypes::Back,NULL);
   const ConnectionLibraryEntry* pConn = pLib->GetConnectionEntry(pPier->GetConnection(pgsTypes::Back));
   if (pConn==NULL)
   {
      pPier->SetConnection(pgsTypes::Back, pPier->GetConnection(pgsTypes::Ahead)); // our ahead connection is good or we wouldn't be here
   }

   pPier = m_BridgeDescription.GetPier(nSpans);
   pPier->SetConnectionLibraryEntry(pgsTypes::Ahead,NULL);
   pConn = pLib->GetConnectionEntry(pPier->GetConnection(pgsTypes::Ahead));
   if (pConn==NULL)
   {
      pPier->SetConnection(pgsTypes::Ahead, pPier->GetConnection(pgsTypes::Back));
   }

}

void CProjectAgentImp::AddGirderStatusItem(SpanIndexType span,GirderIndexType girder, std::_tstring& message)
{
   // first post message
   GET_IFACE(IEAFStatusCenter,pStatusCenter);
   pgsGirderDescriptionStatusItem* pStatusItem =  new pgsGirderDescriptionStatusItem(span,girder,0,m_StatusGroupID,m_scidGirderDescriptionWarning,message.c_str());
   StatusItemIDType st_id = pStatusCenter->Add(pStatusItem);

   // then store message id's for a given span/girder
   SpanGirderHashType hashval = HashSpanGirder(span, girder);

   StatusIterator iter = m_CurrentGirderStatusItems.find(hashval);
   if (iter==m_CurrentGirderStatusItems.end())
   {
      // not found, must insert
      std::pair<StatusContainer::iterator, bool> itbp = m_CurrentGirderStatusItems.insert(StatusContainer::value_type(hashval, std::vector<StatusItemIDType>() ));
      ATLASSERT(itbp.second);
      itbp.first->second.push_back(st_id);
   }
   else
   {
      iter->second.push_back(st_id);
   }
}

void CProjectAgentImp::RemoveGirderStatusItems(SpanIndexType span,GirderIndexType girder)
{
   GET_IFACE(IEAFStatusCenter,pStatusCenter);

   SpanGirderHashType hashval = HashSpanGirder(span, girder);

   StatusIterator iter = m_CurrentGirderStatusItems.find(hashval);
   if (iter!=m_CurrentGirderStatusItems.end())
   {
      std::vector<StatusItemIDType>& rvec = iter->second;

      // remove all status items
      for (std::vector<StatusItemIDType>::iterator viter = rvec.begin(); viter!=rvec.end(); viter++)
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
      const CLiveLoadFactorModel& design_inventory_model = m_pRatingEntry->GetLiveLoadFactorModel(pgsTypes::lrDesign_Inventory);
      if ( !design_inventory_model.AllowUserOverride() )
      {
         m_gLL[IndexFromLimitState(pgsTypes::StrengthI_Inventory)]  = -1;
         m_gLL[IndexFromLimitState(pgsTypes::ServiceIII_Inventory)] = -1;
      }

      const CLiveLoadFactorModel& design_operating_model = m_pRatingEntry->GetLiveLoadFactorModel(pgsTypes::lrDesign_Operating);
      if ( !design_operating_model.AllowUserOverride() )
      {
         m_gLL[IndexFromLimitState(pgsTypes::StrengthI_Operating)]  = -1;
         m_gLL[IndexFromLimitState(pgsTypes::ServiceIII_Operating)] = -1;
      }

      const CLiveLoadFactorModel& legal_routine_model = m_pRatingEntry->GetLiveLoadFactorModel(pgsTypes::lrLegal_Routine);
      if ( !legal_routine_model.AllowUserOverride() )
      {
         m_gLL[IndexFromLimitState(pgsTypes::StrengthI_LegalRoutine)]  = -1;
         m_gLL[IndexFromLimitState(pgsTypes::ServiceIII_LegalRoutine)] = -1;
      }

      const CLiveLoadFactorModel& legal_special_model = m_pRatingEntry->GetLiveLoadFactorModel(pgsTypes::lrLegal_Special);
      if ( !legal_special_model.AllowUserOverride() )
      {
         m_gLL[IndexFromLimitState(pgsTypes::StrengthI_LegalSpecial)]  = -1;
         m_gLL[IndexFromLimitState(pgsTypes::ServiceIII_LegalSpecial)] = -1;
      }

      const CLiveLoadFactorModel& permit_routine_model = m_pRatingEntry->GetLiveLoadFactorModel(pgsTypes::lrPermit_Routine);
      if ( !permit_routine_model.AllowUserOverride() )
      {
         m_gLL[IndexFromLimitState(pgsTypes::StrengthII_PermitRoutine)]  = -1;
         m_gLL[IndexFromLimitState(pgsTypes::ServiceI_PermitRoutine)]    = -1;
      }

      if ( m_SpecialPermitType == pgsTypes::ptSingleTripWithEscort )
      {
         const CLiveLoadFactorModel& permit_special_single_trip_escorted_model = m_pRatingEntry->GetLiveLoadFactorModel(pgsTypes::ptSingleTripWithEscort);
         if ( !permit_special_single_trip_escorted_model.AllowUserOverride() )
         {
            m_gLL[IndexFromLimitState(pgsTypes::StrengthII_PermitSpecial)]  = -1;
            m_gLL[IndexFromLimitState(pgsTypes::ServiceI_PermitSpecial)]    = -1;
         }
      }
      else if ( m_SpecialPermitType == pgsTypes::ptSingleTripWithTraffic )
      {
         const CLiveLoadFactorModel& permit_special_single_trip_traffic_model = m_pRatingEntry->GetLiveLoadFactorModel(pgsTypes::ptSingleTripWithTraffic);
         if ( !permit_special_single_trip_traffic_model.AllowUserOverride() )
         {
            m_gLL[IndexFromLimitState(pgsTypes::StrengthII_PermitSpecial)]  = -1;
            m_gLL[IndexFromLimitState(pgsTypes::ServiceI_PermitSpecial)]    = -1;
         }
      }
      else if ( m_SpecialPermitType == pgsTypes::ptMultipleTripWithTraffic )
      {
         const CLiveLoadFactorModel& permit_special_multiple_trip_traffic_model = m_pRatingEntry->GetLiveLoadFactorModel(pgsTypes::ptMultipleTripWithTraffic);
         if ( !permit_special_multiple_trip_traffic_model.AllowUserOverride() )
         {
            m_gLL[IndexFromLimitState(pgsTypes::StrengthII_PermitSpecial)]  = -1;
            m_gLL[IndexFromLimitState(pgsTypes::ServiceI_PermitSpecial)]    = -1;
         }
      }
      else
      {
         ATLASSERT(false); // should never get here
      }
   }
}

HRESULT CProjectAgentImp::FireContinuityRelatedGirderChange(SpanIndexType span,GirderIndexType gdr,Uint32 lHint)
{
   // Some individual girder changes can affect an entire girderline
   GirderIndexType contgdr = (gdr==ALL_GIRDERS) ? 0 :gdr; // test in girder 1 if all girders

   GET_IFACE(IContinuity,pContinuity);
   if (pContinuity->IsContinuityFullyEffective(contgdr))
   {
      span = ALL_SPANS;
   }

   return Fire_GirderChanged(span, gdr, lHint);
}

// Static functions for checking direct strand fills
bool IsValidStraightStrandFill(const DirectStrandFillCollection* pFill, const GirderLibraryEntry* pGirderEntry)
{
    StrandIndexType ns =  pGirderEntry->GetNumStraightStrandCoordinates();

    DirectStrandFillCollection::const_iterator it    = pFill->begin();
    DirectStrandFillCollection::const_iterator itend = pFill->end();

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
                return false;
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

bool IsValidHarpedStrandFill(const DirectStrandFillCollection* pFill, const GirderLibraryEntry* pGirderEntry)
{
    StrandIndexType ns =  pGirderEntry->GetNumHarpedStrandCoordinates();

    DirectStrandFillCollection::const_iterator it    = pFill->begin();
    DirectStrandFillCollection::const_iterator itend = pFill->end();

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
                return false; // can only fill 1
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

bool IsValidTemporaryStrandFill(const DirectStrandFillCollection* pFill, const GirderLibraryEntry* pGirderEntry)
{
    StrandIndexType ns =  pGirderEntry->GetNumTemporaryStrandCoordinates();

    DirectStrandFillCollection::const_iterator it    = pFill->begin();
    DirectStrandFillCollection::const_iterator itend = pFill->end();

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
                return false;
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

